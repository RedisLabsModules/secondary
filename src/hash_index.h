#ifndef __SI_HASH_INDEX__
#define __SI_HASH_INDEX__

#include "index.h"
#include "spec.h"
#include "redismodule.h"
#include "index_type.h"

/* run RedisModule_Call of a command, substituting $ with the given id in the
 * argument list */
RedisModuleCallReply *__callParametricCommand(RedisModuleCtx *ctx, SIId id,
                                              RedisModuleString **argv,
                                              int argc);

int HashIndex_ExecuteReadCommand(RedisModuleCtx *ctx, RedisIndex *idx,
                                 SIQuery *query, RedisModuleString **argv,
                                 int argc);

RedisModuleString *HashIndex_GetKey(RedisModuleString **argv, int argc);
int HashIndex_IndexHashObject(RedisModuleCtx *ctx, RedisIndex *idx,
                              RedisModuleString *hkey);
#endif
