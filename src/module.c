#include "index_type.h"
#include "redismodule.h"
#include "rmutil/util.h"
#include "rmutil/alloc.h"
#include "hash_index.h"
/*
* IDX.CREATE <index_name> {options} SCHEMA
* [[STRING|INT32|INT64|UINT|BOOL|FLOAT|DOUBLE|TIME] ...]
*/
int CreateIndexCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                       int argc) {
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 4) return RedisModule_WrongArity(ctx);

  SISpec spec;
  SIIndexKind kind;
  if (SI_ParseSpec(ctx, argv, argc, &spec, &kind) == REDISMODULE_ERR) {
    return RedisModule_ReplyWithError(ctx, "Invalid schema");
  }

  // Open the index key
  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
  int type = RedisModule_KeyType(key);
  if (type != REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  RedisIndex *idx = NewRedisIndex(0, 0, spec);
  RedisModule_ModuleTypeSetValue(key, IndexType, idx);

  return RedisModule_ReplyWithSimpleString(ctx, "OK");
}

/* IDX.ADD <index_name> <id> val1 ... */
int IndexAddCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 4) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  // make sure it's an index key
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY ||
      RedisModule_ModuleTypeGetType(key) != IndexType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  RedisIndex *idx = RedisModule_ModuleTypeGetValue(key);

  size_t len;
  char *id = (char *)RedisModule_StringPtrLen(argv[2], &len);
  id = strndup(id, len);

  SIValueVector vals = SI_NewValueVector(argc - 3);
  for (int i = 0; i < argc - 3; i++) {
    if (i == idx->spec.numProps) {
      SIValueVector_Free(&vals);
      return RedisModule_ReplyWithError(ctx, "Invalid value given");
    }

    size_t vlen;
    const char *vstr = RedisModule_StringPtrLen(argv[i + 3], &vlen);
    SIValue val = {.type = idx->spec.properties[i].type};
    if (!SI_ParseValue(&val, (char *)vstr, vlen)) {
      SIValueVector_Free(&vals);
      RedisModule_Log(ctx, "error", "Could not parse %.*s\n", (int)vlen, vstr);
      return RedisModule_ReplyWithError(ctx, "Invalid value given");
    }
    SIValueVector_Append(&vals, val);
  }

  SIChange ch = (SIChange){.type = SI_CHADD, .id = (SIId)id, .v = vals};

  SIChangeSet cs = SI_NewChangeSet(1);
  SIChangeSet_AddCahnge(&cs, ch);

  if (idx->idx.Apply(idx->idx.ctx, cs) != SI_INDEX_OK) {
    SIValueVector_Free(&vals);
    return RedisModule_ReplyWithError(ctx, "Could not apply change to index");
  }

  SIValueVector_Free(&vals);
  return RedisModule_ReplyWithSimpleString(ctx, "OK");
}

/* IDX.DEL <index_name> <id> [<id> ...] */
int IndexDelCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 3) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  // make sure it's an index key
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY ||
      RedisModule_ModuleTypeGetType(key) != IndexType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  RedisIndex *idx = RedisModule_ModuleTypeGetValue(key);

  SIChangeSet cs = SI_NewChangeSet(argc - 2);
  for (int i = 2; i < argc; i++) {
    SIId id = (char *)RedisModule_StringPtrLen(argv[2], NULL);
    SIChangeSet_AddCahnge(&cs, SI_NewDelChange(id));
  }

  if (idx->idx.Apply(idx->idx.ctx, cs) != SI_INDEX_OK) {
    SIChangeSet_Free(&cs);
    return RedisModule_ReplyWithError(ctx, "Could not apply change to index");
  }

  SIChangeSet_Free(&cs);
  return RedisModule_ReplyWithSimpleString(ctx, "OK");
}

/* IDX.CARD <index_name> */
int IndexCardinalityCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                            int argc) {
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc != 2) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);

  // and empty index - return 0
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY) {
    return RedisModule_ReplyWithLongLong(ctx, 0);
  }

  // make sure it's an index key
  if (RedisModule_ModuleTypeGetType(key) != IndexType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  RedisIndex *idx = RedisModule_ModuleTypeGetValue(key);

  return RedisModule_ReplyWithLongLong(ctx, idx->idx.Len(idx->idx.ctx));
}

/* IDX.SELECT FROM <index_name> WHERE <predicates> [LIMIT offset num] */
int IndexSelectCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                       int argc) {
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 5) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[2], REDISMODULE_READ);

  // make sure it's an index key
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY ||
      RedisModule_ModuleTypeGetType(key) != IndexType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  RedisIndex *idx = RedisModule_ModuleTypeGetValue(key);

  size_t len;
  char *qstr = (char *)RedisModule_StringPtrLen(argv[4], &len);
  char *parseError = NULL;
  SIQuery q = SI_NewQuery();
  if (!SI_ParseQuery(&q, qstr, len, &idx->spec, &parseError)) {
    RedisModule_ReplyWithError(
        ctx, parseError ? parseError : "Error parsing query string");
    if (parseError) {
      free(parseError);
    }
    return REDISMODULE_OK;
  }

  SICursor *c = idx->idx.Find(idx->idx.ctx, &q);
  if (c->error == SI_CURSOR_OK) {
    RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_ARRAY_LEN);
    SIId id;
    int i = 0;
    while (NULL != (id = c->Next(c->ctx))) {
      i++;
      RedisModule_ReplyWithStringBuffer(ctx, id, strlen(id));
    }
    RedisModule_ReplySetArrayLength(ctx, i);
  } else {
    RedisModule_ReplyWithError(ctx, "Error performing query");
  }

  SIQuery_Free(&q);
  SICursor_Free(c);

  return REDISMODULE_OK;
}

/* IDX.FROM {index_name} WHERE {predicates} ANY REDIS READ COMMAND */
int IndexFromCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 6) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);

  // make sure it's an index key
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY ||
      RedisModule_ModuleTypeGetType(key) != IndexType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  RedisIndex *idx = RedisModule_ModuleTypeGetValue(key);

  int wherePos = RMUtil_ArgExists("WHERE", argv, argc, 0);
  if (!wherePos || wherePos >= argc - 1) {
    RedisModule_ReplyWithError(ctx, "No Where clause given");
  }

  size_t len;
  char *qstr = (char *)RedisModule_StringPtrLen(argv[wherePos + 1], &len);
  char *parseError = NULL;

  SIQuery q = SI_NewQuery();
  if (!SI_ParseQuery(&q, qstr, len, &idx->spec, &parseError)) {
    RedisModule_ReplyWithError(
        ctx, parseError ? parseError : "Error parsing WHERE query string");
    if (parseError) {
      free(parseError);
    }
    return REDISMODULE_OK;
  }
  int rc = HashIndex_ExecuteReadCommand(ctx, idx, &q, &argv[wherePos + 2],
                                        argc - (wherePos + 2));
  SIQuery_Free(&q);
  return rc;
}

/* IDX.INTO {index_name} [WHERE ...] [HMSET|HSET|etc...] */
int IndexIntoCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc) {
  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 4) return RedisModule_WrongArity(ctx);

  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);

  // make sure it's an index key
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY ||
      RedisModule_ModuleTypeGetType(key) != IndexType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  RedisIndex *idx = RedisModule_ModuleTypeGetValue(key);

  int wherePos = RMUtil_ArgExists("WHERE", argv, argc, 0);
  SIQuery q;
  if (wherePos && wherePos < argc - 1) {
    size_t len;
    char *qstr = (char *)RedisModule_StringPtrLen(argv[wherePos + 1], &len);
    char *parseError = NULL;

    q = SI_NewQuery();
    if (!SI_ParseQuery(&q, qstr, len, &idx->spec, &parseError)) {
      RedisModule_ReplyWithError(
          ctx, parseError ? parseError : "Error parsing WHERE query string");
      if (parseError) {
        free(parseError);
      }
      return REDISMODULE_OK;
    }
  }

  // TODO: dynamic cmdPos if WHERE exists
  int cmdPos = wherePos ? wherePos + 2 : 2;

  IndexedTransaction tx = CreateIndexedTransaction(
      ctx, idx, wherePos ? &q : NULL, &argv[cmdPos], argc - cmdPos);

  if (tx.err != NULL) {
    return RedisModule_ReplyWithError(ctx, tx.err);
  }

  int num = 0;
  SIId id;
  while (NULL != (id = tx.ids(tx.ctx))) {
    RedisModuleCallReply *rep =
        wherePos
            ? __callParametricCommand(ctx, id, &argv[cmdPos], argc - cmdPos)
            : RedisModule_Call(ctx,
                               RedisModule_StringPtrLen(argv[cmdPos], NULL),
                               "v", &argv[cmdPos + 1], argc - (cmdPos + 1));

    if (RedisModule_CallReplyType(rep) != REDISMODULE_REPLY_ERROR) {
      num++;
      if (tx.cmd(ctx, idx, RedisModule_CreateString(ctx, id, strlen(id))) !=
          REDISMODULE_OK) {
        RedisModule_ReplyWithError(
            ctx, "Command performed but updating index failed");
        goto cleanup;
      }
    }
  }

  RedisModule_ReplyWithLongLong(ctx, num);

cleanup:
  if (tx.ctx) free(tx.ctx);

  return REDISMODULE_OK;
}

int RedisModule_OnLoad(RedisModuleCtx *ctx) {
  // LOGGING_INIT(0xFFFFFFFF);
  if (RedisModule_Init(ctx, "idx", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  // register index type
  if (RedisIndex_Register(ctx) == REDISMODULE_ERR) return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.create", CreateIndexCommand,
                                "write deny-oom no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.insert", IndexAddCommand,
                                "write deny-oom no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.del", IndexDelCommand,
                                "write deny-oom no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.select", IndexSelectCommand,
                                "readonly no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.from", IndexFromCommand,
                                "readonly no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  // TODO: this is not a "key at 1" command - needs better handling
  if (RedisModule_CreateCommand(ctx, "idx.into", IndexIntoCommand,
                                "write deny-oom no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.card", IndexCardinalityCommand,
                                "readonly no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

    return REDISMODULE_OK;
}