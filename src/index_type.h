#ifndef __SI_INDEX_TYPE_H
#define __SI_INDEX_TYPE_H
#include "redismodule.h"
#include "index.h"

extern RedisModuleType *IndexType;
typedef enum { SI_AbstractIndex, SI_HashIndex, SI_StringIndex } SIIndexKind;

typedef struct {
  SIIndexKind kind;
  u_int32_t flags;
  SISpec spec;
  SIIndex idx;
} RedisIndex;

void *RedisIndex_RdbLoad(RedisModuleIO *rdb, int encver);
void RedisIndex_RdbSave(RedisModuleIO *rdb, void *value);
void RedisIndex_Digest(RedisModuleDigest *digest, void *value);
void RedisIndex_AofRewrite(RedisModuleIO *aof, RedisModuleString *key,
                           void *value);
void RedisIndex_Digest(RedisModuleDigest *digest, void *value);
void RedisIndex_Free(void *value);

int SI_ParseSpec(RedisModuleCtx *ctx, RedisModuleString **argv, int argc,
                 SISpec *spec, SIIndexKind *kind);

void *NewRedisIndex(SIIndexKind kind, u_int32_t flags, SISpec spec);
int RedisIndex_Register(RedisModuleCtx *ctx);
#endif // !__SI_INDEX_TYPE_