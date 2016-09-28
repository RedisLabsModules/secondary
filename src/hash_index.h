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

/* And indexed command proxy is a generic callback that based on the command at
 * hand, executes it and operates on the index accordingly */
typedef int (*IndexedCommandProxy)(RedisModuleCtx *ctx, RedisIndex *idx,
                                   RedisModuleString *hkey);

typedef SIId (*IdSource)(void *ctx);

typedef struct {
  IndexedCommandProxy cmd;
  void *ctx;
  IdSource ids;
  const char *err;
} IndexedTransaction;

RedisModuleCallReply *__callParametricCommand(RedisModuleCtx *ctx, SIId id,
                                              RedisModuleString **argv,
                                              int argc);
IndexedTransaction CreateIndexedTransaction(RedisModuleCtx *ctx,
                                            RedisIndex *idx, SIQuery *q,
                                            RedisModuleString **argv, int argc);

IdSource HashIndex_GetIdsForCommand(RedisModuleString **argv, int argc,
                                    void **pctx);
int HashIndex_IndexHashObject(RedisModuleCtx *ctx, RedisIndex *idx,
                              RedisModuleString *hkey);
#endif
 
        
        