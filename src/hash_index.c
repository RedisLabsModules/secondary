#include "hash_index.h"
#include "rmutil/strings.h"
#include "rmutil/alloc.h"
#define ID_SUB_TOKEN "$"

RedisModuleCallReply *__callParametricCommand(RedisModuleCtx *ctx, SIId id,
                                              RedisModuleString **argv,
                                              int argc) {

  for (int i = 0; i < argc; i++) {
    if (RMUtil_StringEqualsC(argv[i], ID_SUB_TOKEN)) {
      argv[i] = RedisModule_CreateString(ctx, id, strlen(id));
      break;
    }
  }

  return RedisModule_Call(ctx, RedisModule_StringPtrLen(argv[0], NULL), "v",
                          &argv[1], argc - 1);
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
  while (NULL != (id = c->Next(c->ctx))) {
    printf("Id: %s\n", id);
    RedisModule_ReplyWithCallReply(
        ctx, __callParametricCommand(ctx, id, argv, argc));
    num++;
  }
  SICursor_Free(c);
  RedisModule_ReplySetArrayLength(ctx, num);
  return REDISMODULE_OK;
}

static const char *supportedWriteCommands[] = {
    "HDEL", "HINCRBY", "HINCRBYFLOAT", "HMSET", "HSET", "HSETNX", NULL};
// TODO - support these as well "DEL",  "EXPIRE",  "EXPIREAT",     "PEXPIRE",
// "PEXPIREAT", NULL};

RedisModuleString *HashIndex_GetKey(RedisModuleString **argv, int argc) {
  if (argc <= 1) {
    return NULL;
  }
  const char *cmd = RedisModule_StringPtrLen(argv[0], NULL);

  for (int i = 0; supportedWriteCommands[i] != NULL; i++) {
    if (!strcasecmp(cmd, supportedWriteCommands[i])) {
      // TODO: Support variadic commands
      return argv[1];
    }
  }
  return NULL;
}

int HashIndex_IndexHashObject(RedisModuleCtx *ctx, RedisIndex *idx,
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
    if (rc == REDISMODULE_ERR || vstr == NULL) {
      goto error;
    }

    char *val = (char *)RedisModule_StringPtrLen(vstr, NULL);

    SIValue v = (SIValue){.type = idx->spec.properties[i].type};
    if (!SI_ParseValue(&v, val, strlen(val))) {
      RedisModule_Log(ctx, "error", "could not parse value from hash %s\n",
                      val);
      goto error;
    }
    SIValueVector_Append(&ch.v, v);
  }

  SIChangeSet_AddCahnge(&cs, ch);

  if (!idx->idx.Apply(idx->idx.ctx, cs)) {
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
