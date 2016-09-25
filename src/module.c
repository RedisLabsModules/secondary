#include "index_type.h"
#include "redismodule.h"
#include "rmutil/alloc.h"

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

  size_t len;
  char *id = (char *)RedisModule_StringPtrLen(argv[2], &len);
  id = strndup(id, len);

  SIValueVector vals = SI_NewValueVector(argc - 3);
  for (int i = 0; i < argc - 3; i++) {
    size_t vlen;
    const char *vstr = RedisModule_StringPtrLen(argv[i + 3], &vlen);
    SIValue val = {.type = idx->spec.properties[i].type};
    if (!SI_ParseValue(&val, (char *)vstr, vlen)) {
      printf("Could not parse %.*s\n", (int)vlen, vstr);
      return RedisModule_ReplyWithError(ctx, "Invalid value given");
    }
    SIValueVector_Append(&vals, val);
  }

  SIChange ch = (SIChange){.type = SI_CHADD, .id = (SIId)id, .v = vals};

  SIChangeSet cs = SI_NewChangeSet(1);
  SIChangeSet_AddCahnge(&cs, ch);

  if (idx->idx.Apply(idx->idx.ctx, cs) != SI_INDEX_OK) {
    return RedisModule_ReplyWithError(ctx, "Could not apply change to index");
  }

  return RedisModule_ReplyWithSimpleString(ctx, "OK");
}

/* IDX.SELECT FROM <index_name> WHERE <predicates> [LIMIT offset num] */
int IndexSelectCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
                       int argc) {

  RedisModule_AutoMemory(ctx); /* Use automatic memory management. */

  if (argc < 5)
    return RedisModule_WrongArity(ctx);

  RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[2], REDISMODULE_READ);

  // make sure it's an index key
  if (RedisModule_KeyType(key) == REDISMODULE_KEYTYPE_EMPTY ||
      RedisModule_ModuleTypeGetType(key) != IndexType) {
    return RedisModule_ReplyWithError(ctx, REDISMODULE_ERRORMSG_WRONGTYPE);
  }
  RedisIndex *idx = RedisModule_ModuleTypeGetValue(key);

  size_t len;
  char *qstr = (char *)RedisModule_StringPtrLen(argv[4], &len);
  SIQuery q = SI_NewQuery();
  if (!SI_ParseQuery(&q, qstr, len)) {
    return RedisModule_ReplyWithError(ctx, "Error parsing query string");
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

// int TestDoubleFormatting(RedisModuleCtx *ctx, RedisModuleString **argv,
//                          int argc) {

//   RedisModule_AutoMemory(ctx); /* Use automatic memory management. */
//   if (argc < 2) {
//     return RedisModule_WrongArity(ctx);
//   }

//   RedisModuleCallReply *r =
//       RedisModule_Call(ctx, "INCRBYFLOAT", "sd", argv[1], 3.14159265359);
//   return RedisModule_ReplyWithCallReply(ctx, r);
// }

int RedisModule_OnLoad(RedisModuleCtx *ctx) {
  // LOGGING_INIT(0xFFFFFFFF);
  if (RedisModule_Init(ctx, "idx", 1, REDISMODULE_APIVER_1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  // register index type
  if (RedisIndex_Register(ctx) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.create", CreateIndexCommand,
                                "write deny-oom no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.add", IndexAddCommand,
                                "write deny-oom no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)
    return REDISMODULE_ERR;

  if (RedisModule_CreateCommand(ctx, "idx.select", IndexSelectCommand,
                                "readonly no-cluster", 1, 1,
                                1) == REDISMODULE_ERR)

    return REDISMODULE_OK;
}