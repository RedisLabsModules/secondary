#include "index_type.h"
#include "redismodule.h"

/*
* IDX.CREATE <index_name> {options} SCHEMA
* [[STRING|INT32|INT64|UINT|BOOL|FLOAT|DOUBLE|TIME] ...]
*/
int CreateIndexCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                       int argc) {

  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 4)
    return RedisModule_WrongArity(ctx);

  SISpec spec;
  if (SI_ParseSpec(&argv[3], argc - 3, &spec) == REDISMODULE_ERR) {
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

  if (argc < 4)
    return RedisModule_WrongArity(ctx);

  RedisModuleKey *key =
      RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);

  // make sure it's an index key
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY ||
      RedisModule_ModuleTypeGetType(key) != IndexType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }

  RedisIndex *idx = RedisModule_ModuleTypeGetValue(key);

  
}

int RedisModule_OnLoad(RedisModuleCtx *ctx) {
  // LOGGING_INIT(0xFFFFFFFF);
  if (RedisModule_Init(ctx, "idx", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  // register trie type
  if (RedisIndex_Register(ctx) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.create", CreateIndexCommand,
                                "write deny-oom no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  return REDISMODULE_OK;
}