#include "hash_index.h"
#include "rmutil/strings.h"
#include "rmutil/alloc.h"

#define ID_SUB_TOKEN "$"

RedisModuleCallReply *__callParametricCommand(RedisModuleCtx *ctx, SIId id,
                                              RedisModuleString **argv,
                                              int argc) {
  // we save the substitution token so we won't actually change argv
  RedisModuleString *subToken = NULL;
  int i;
  for (i = 0; i < argc; i++) {
    if (RMUtil_StringEqualsC(argv[i], ID_SUB_TOKEN)) {
      subToken = argv[i];
      argv[i] = RedisModule_CreateString(ctx, id, strlen(id));
      break;
    }
  }

  RedisModuleCallReply *r = RedisModule_Call(
      ctx, RedisModule_StringPtrLen(argv[0], NULL), "v", &argv[1], argc - 1);

  // if we've replaced the substitution token, we must put it back in argv
  if (subToken) {
    argv[i] = subToken;
  }

  return r;
}

int HashIndex_ExecuteReadCommand(RedisModuleCtx *ctx, RedisIndex *idx,
                                 SIQuery *query, RedisModuleString **argv,
                                 int argc) {
  SICursor *c = idx->idx.Find(idx->idx.ctx, query);
  if (c->error != QE_OK) {
    // TODO: proper error reporting in cursor
    return RedisModule_ReplyWithError(ctx, "Error executing query");
  }

  RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);
  int num = 0;
  SIId id;
  while (NULL != (id = c->Next(c->ctx, NULL))) {
    RedisModule_ReplyWithSimpleString(ctx, id);

    RedisModuleCallReply *r = __callParametricCommand(ctx, id, argv, argc);
    if (r) {
      RedisModule_ReplyWithCallReply(ctx, r);
    } else {
      // null reply means redis cannot even try to execute the command
      RedisModule_ReplyWithError(
          ctx, "Could not call command: command non "
               "existing, wrong arity or wrong format specifier");
    }

    num += 2;
  }

  SICursor_Free(c);
  RedisModule_ReplySetArrayLength(ctx, num);
  return REDISMODULE_OK;
}

typedef struct {
  const char *name;
  IndexedCommandProxy handler;
  int variadic;
} commandDesc;

int reindexHashHandler(RedisModuleCtx *ctx, RedisIndex *idx,
                       RedisModuleString *hkey);

int deleteHandler(RedisModuleCtx *ctx, RedisIndex *idx,
                  RedisModuleString *hkey);

typedef struct {
  RedisModuleString **keys;
  int argc;
  int keystep;
  int offset;
} staticIdSourceCtx;

SIId staticIdSource(void *ctx) {
  staticIdSourceCtx *c = ctx;
  if (c->offset < c->argc) {
    SIId ret = (SIId)RedisModule_StringPtrLen(c->keys[c->offset], NULL);
    c->offset += c->keystep;
    return ret;
  }
  return NULL;
}

static const commandDesc supportedHashWriteCommands[] = {
    {"HDEL", reindexHashHandler, 0},
    {"HINCRBY", reindexHashHandler, 0},
    {"HINCRBYFLOAT", reindexHashHandler, 0},
    {"HMSET", reindexHashHandler, 0},
    {"HSET", reindexHashHandler, 0},
    {"HSETNX", reindexHashHandler, 0},
    {"DEL", deleteHandler, 1},
    {NULL}};

SIId queryIdSource(void *ctx) {
  SICursor *c = ctx;
  return c->Next(c->ctx, NULL);
}

IdSource HashIndex_GetIdsFromQuery(RedisIndex *idx, SIQuery *q, void **pctx) {
  *pctx = NULL;
  if (!q)
    return NULL;

  SICursor *c = idx->idx.Find(idx->idx.ctx, q);
  if (c->error != QE_OK) {
    SICursor_Free(c);
    // TODO: proper error reporting in cursor
    return NULL;
  }
  *pctx = c;
  return queryIdSource;
}

IdSource HashIndex_GetIdsForCommand(RedisModuleString **argv, int argc,
                                    void **pctx) {
  *pctx = NULL;
  if (argc <= 1) {
    return NULL;
  }
  const char *cmd = RedisModule_StringPtrLen(argv[0], NULL);

  for (int i = 0; supportedHashWriteCommands[i].name != NULL; i++) {
    if (!strcasecmp(cmd, supportedHashWriteCommands[i].name)) {
      staticIdSourceCtx *ctx = malloc(sizeof(staticIdSourceCtx));
      ctx->keys = &argv[1];
      ctx->offset = 0;
      ctx->keystep = 1; // TODO: we might need to support more complex keysteps

      // for variadic commands, we assume the rest of the argv is just keys
      //
      // TODO: Use a complex pattern of offset, step, last
      ctx->argc = supportedHashWriteCommands[i].variadic ? argc - 1 : 1;
      *pctx = ctx;
      return staticIdSource;
    }
  }

  return NULL;
}

/* Post command handler that reindexes a HASH object in redis by reading its
 * current values */
int reindexHashHandler(RedisModuleCtx *ctx, RedisIndex *idx,
                       RedisModuleString *hkey) {
  RedisModuleKey *k = RedisModule_OpenKey(ctx, hkey, REDISMODULE_READ);

  if (k == NULL || RedisModule_KeyType(k) != REDISMODULE_KEYTYPE_HASH ||
      RedisModule_KeyType(k) == REDISMODULE_KEYTYPE_EMPTY) {
    return REDISMODULE_ERR;
  }

  SIId id = (SIId)strdup(RedisModule_StringPtrLen(hkey, NULL));
  SIChangeSet cs = SI_NewChangeSet(1);
  SIChange ch = SI_NewEmptyAddChange(id, idx->spec.numProps);

  for (int i = 0; i < idx->spec.numProps; i++) {
    RedisModuleString *vstr;

    int rc = RedisModule_HashGet(k, REDISMODULE_HASH_CFIELDS,
                                 idx->spec.properties[i].name, &vstr, NULL);
    if (rc == REDISMODULE_ERR) {
      goto error;
    }

    SIValue v = SI_NullVal();
    size_t vlen;
    char *val = vstr ? (char *)RedisModule_StringPtrLen(vstr, &vlen) : NULL;
    // if the hash element did not exist, we put a NULL value
    if (val) {
      v = (SIValue){.type = idx->spec.properties[i].type};
      if (!SI_ParseValue(&v, val, vlen)) {
        RedisModule_Log(ctx, "error", "could not parse value from hash %s\n",
                        val);
        goto error;
      }
    }
    SIValueVector_Append(&ch.v, v);
  }

  SIChangeSet_AddCahnge(&cs, ch);

  if (idx->idx.Apply(idx->idx.ctx, cs) != SI_INDEX_OK) {
    RedisModule_Log(ctx, "error", "Could not index id %s\n", id);
  }

  RedisModule_CloseKey(k);
  SIValueVector_Free(&ch.v);
  SIChangeSet_Free(&cs);
  return REDISMODULE_OK;

error:
  if (k)
    RedisModule_CloseKey(k);

  SIValueVector_Free(&ch.v);
  SIChangeSet_Free(&cs);
  return REDISMODULE_ERR;
}

/* post command handler that deletes an entry from the index following a key
 * deletion */
int deleteHandler(RedisModuleCtx *ctx, RedisIndex *idx,
                  RedisModuleString *hkey) {
  SIId id = (char *)RedisModule_StringPtrLen(hkey, NULL);
  SIChangeSet cs = SI_NewChangeSet(1);
  SIChangeSet_AddCahnge(&cs, SI_NewDelChange(id));

  if (idx->idx.Apply(idx->idx.ctx, cs) != SI_INDEX_OK) {
    RedisModule_Log(ctx, "error", "Could not delete id %s\n", id);
    return REDISMODULE_ERR;
  }
  SIChangeSet_Free(&cs);
  return REDISMODULE_OK;
}

IndexedCommandProxy HashIndex_GetHandler(RedisModuleString *cmd) {
  for (int i = 0; supportedHashWriteCommands[i].name != NULL; i++) {
    if (RMUtil_StringEqualsC(cmd, supportedHashWriteCommands[i].name)) {
      return supportedHashWriteCommands[i].handler;
    }
  }

  return NULL;
}

IndexedTransaction CreateIndexedTransaction(RedisModuleCtx *ctx,
                                            RedisIndex *idx, SIQuery *q,
                                            RedisModuleString **argv,
                                            int argc) {
  IndexedTransaction ret;
  ret.ctx = NULL;
  ret.err = NULL;

  ret.cmd = HashIndex_GetHandler(argv[0]);
  if (ret.cmd == NULL) {
    ret.err = "Command not supported";
    return ret;
  }

  if (!q) {
    ret.ids = HashIndex_GetIdsForCommand(argv, argc, &ret.ctx);
    if (!ret.ctx || !ret.ids) {
      ret.err = "No key for requested command";
      return ret;
    }
  } else {
    ret.ids = HashIndex_GetIdsFromQuery(idx, q, &ret.ctx);
    if (!ret.ctx || !ret.ids) {
      ret.err = "Error executing WHERE clause";
      return ret;
    }
  }

  return ret;
}
