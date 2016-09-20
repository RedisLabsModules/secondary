#include "redismodule.h"
#include "index.h"
#include "key.h"
#include "index_type.h"
#include "rmutil/alloc.h"

RedisModuleType *IndexType;

void __redisIndex_SaveSpec(RedisIndex *idx, RedisModuleIO *io) {

  RedisModule_SaveUnsigned(io, (u_int64_t)idx->spec.numProps);
  for (size_t i = 0; i < idx->spec.numProps; i++) {
    printf("saving prop type %d flags %x\n", idx->spec.properties[i].type,
           idx->spec.properties[i].flags);
    RedisModule_SaveSigned(io, (int)idx->spec.properties[i].type);
    RedisModule_SaveSigned(io, (int)idx->spec.properties[i].flags);
  }
}

void __redisIndex_LoadSpec(RedisIndex *idx, RedisModuleIO *io) {
  idx->spec.numProps = RedisModule_LoadUnsigned(io);
  idx->spec.properties = calloc(idx->spec.numProps, sizeof(SIIndexProperty));
  printf("loading index with %d props!\n", idx->spec.numProps);

  for (size_t i = 0; i < idx->spec.numProps; i++) {
    idx->spec.properties[i].type = RedisModule_LoadSigned(io);
    idx->spec.properties[i].flags = RedisModule_LoadSigned(io);
    printf("loaded prop type %d flags %x\n", idx->spec.properties[i].type,
           idx->spec.properties[i].flags);
  }
}

SIValue __readValue(RedisModuleIO *rdb) {
  SIValue v;
  v.type = RedisModule_LoadUnsigned(rdb);
  switch (v.type) {
  case T_STRING:
    v.stringval.str = RedisModule_LoadStringBuffer(rdb, &v.stringval.len);
    break;
  case T_INT32:
    v.intval = (int32_t)RedisModule_LoadSigned(rdb);
    break;
  case T_INT64:
    v.longval = RedisModule_LoadSigned(rdb);
    break;
  case T_UINT:
    v.uintval = RedisModule_LoadUnsigned(rdb);
    break;
  case T_BOOL:
    v.boolval = RedisModule_LoadUnsigned(rdb);
    break;
  case T_FLOAT:
    v.floatval = (float)RedisModule_LoadDouble(rdb);
    break;
  case T_DOUBLE:
    v.doubleval = RedisModule_LoadDouble(rdb);
    break;
  case T_TIME:
    v.timeval = (time_t)RedisModule_LoadSigned(rdb);
    break;
  case T_NULL:
  default:
    // NULL value for all unsupported stuff
    // this will probably break the loading for any wrong type
    v.type = T_NULL;
    break;
  }
  return v;
}

void __writeValue(void *v, SIType t, RedisModuleIO *rdb) {
  RedisModule_SaveUnsigned(rdb, t);
  switch (t) {
  case T_STRING: {
    SIString *s = v;
    RedisModule_SaveStringBuffer(rdb, s->str, s->len);
    break;
  }
  case T_INT32:
    RedisModule_SaveSigned(rdb, *(int32_t *)v);
    break;
  case T_INT64:
    RedisModule_SaveSigned(rdb, *(int64_t *)v);
    break;
  case T_UINT:
    RedisModule_SaveUnsigned(rdb, *(u_int64_t *)v);
    break;
  case T_BOOL:
    RedisModule_SaveUnsigned(rdb, *(int32_t *)v);
    break;
  case T_FLOAT:
    RedisModule_SaveDouble(rdb, *(float *)v);
    break;
  case T_DOUBLE:
    RedisModule_SaveDouble(rdb, *(double *)v);
    break;
  case T_TIME:
    RedisModule_SaveSigned(rdb, *(time_t *)v);
    break;
  case T_NULL:
  default:
    // NULL value for all unsupported stuff.
    break;
  }
}

typedef struct {
  RedisModuleIO *w;
  RedisIndex *idx;
  int num;
} __redisIndexVisitorCtx;

void __redisIndex_Visitor(SIId id, void *key, void *ctx) {
  __redisIndexVisitorCtx *vx = ctx;
  SIMultiKey *mk = key;
  RedisModule_SaveStringBuffer(vx->w, id, strlen(id));

  for (int i = 0; i < vx->idx->spec.numProps; i++) {
    __writeValue(&mk->keys[i], vx->idx->spec.properties[i].type, vx->w);
    vx->num++;
  }
}
void __redisIndex_SaveIndex(RedisIndex *idx, RedisModuleIO *w) {

  size_t len = idx->idx.Len(idx->idx.ctx);
  printf("saving index len %zd\n", len);
  // save the number of elements in the indes
  RedisModule_SaveUnsigned(w, (u_int64_t)len);

  __redisIndexVisitorCtx vx = {w, idx, 0};

  idx->idx.Traverse(idx->idx.ctx, __redisIndex_Visitor, &vx);
  printf("saved %d elemets\n", vx.num);
}

int __redisIndex_LoadIndex(RedisIndex *idx, RedisModuleIO *rdb) {

  // 1. create an index
  // TODO: Check idx kind for multiple kind support
  idx->idx = SI_NewCompoundIndex(idx->spec);

  // read the total number of elements in the index
  u_int64_t elements = RedisModule_LoadUnsigned(rdb);
  printf("oading index len %zd\n", elements);
  // create a mock changeset
  SIChangeSet cs;

  cs.numChanges = 1;
  cs.changes = calloc(1, sizeof(SIChange));
  cs.changes[0].v = SI_NewValueVector(idx->spec.numProps);
  cs.changes[0].id = NULL;

  while (elements--) {

    // create an ADD change
    size_t idlen;
    char *id = RedisModule_LoadStringBuffer(rdb, &idlen);
    printf("inserting id %s\n", id);
    cs.changes[0].id = id;
    cs.changes[0].v.len = 0;
    for (int i = 0; i < idx->spec.numProps; i++) {

      SIValueVector_Append(&cs.changes[0].v, __readValue(rdb));
    }

    idx->idx.Apply(idx->idx.ctx, cs);
  }

  SIChangeSet_Free(&cs);

  return REDISMODULE_OK;
}

int SI_ParseSpec(RedisModuleString **argv, int argc, SISpec *spec) {

  spec->numProps = argc;
  spec->properties = calloc(argc, sizeof(SIIndexProperty));

  const char *types[] = {"STRING", "INT32",  "INT64", "UINT", "BOOL",
                         "FLOAT",  "DOUBLE", "TIME",  NULL};
  SIType typeEnums[] = {
      T_STRING, T_INT32,  T_INT64, T_UINT, T_BOOL,
      T_FLOAT,  T_DOUBLE, T_TIME,  T_NULL,
  };

  for (int i = 0; i < spec->numProps; i++) {

    size_t len;
    const char *str = RedisModule_StringPtrLen(argv[i], &len);
    int ok = 0;
    for (int t = 0; types[t] != NULL; t++) {
      if (strlen(types[t]) == len && !strncasecmp(str, types[t], len)) {
        spec->properties[i].type = typeEnums[t];
        ok = 1;
        break;
      }
    }

    if (!ok) {
      printf("could not parse %.*s\n", (int)len, str);
      free(spec->properties);
      spec->properties = NULL;
      return REDISMODULE_ERR;
    }
  }

  return REDISMODULE_OK;
}

void *NewRedisIndex(SIIndexKind kind, u_int32_t flags, SISpec spec) {
  RedisIndex *idx = malloc(sizeof(RedisIndex));
  idx->kind = kind;
  idx->flags = flags;
  idx->spec = spec;
  idx->idx = SI_NewCompoundIndex(idx->spec);

  return idx;
}

void *RedisIndex_RdbLoad(RedisModuleIO *rdb, int encver) {
  if (encver != 0) {
    return NULL;
  }

  RedisIndex *idx = malloc(sizeof(RedisIndex));
  idx->kind = RedisModule_LoadUnsigned(rdb);
  idx->flags = RedisModule_LoadUnsigned(rdb);

  // read the spec
  __redisIndex_LoadSpec(idx, rdb);

  // create and populat the index
  __redisIndex_LoadIndex(idx, rdb);

  return idx;
}

void RedisIndex_RdbSave(RedisModuleIO *rdb, void *value) {
  RedisIndex *idx = value;
  RedisModule_SaveUnsigned(rdb, idx->kind);
  RedisModule_SaveUnsigned(rdb, idx->flags);

  // save the spec
  __redisIndex_SaveSpec(idx, rdb);

  // save the index data
  __redisIndex_SaveIndex(idx, rdb);
}

void RedisIndex_AofRewrite(RedisModuleIO *aof, RedisModuleString *key,
                           void *value) {
  printf("AOF rewrite not implemented!");
}

void RedisIndex_Digest(RedisModuleDigest *digest, void *value) {}

void RedisIndex_Free(void *value) {
  RedisIndex *idx = value;
  idx->idx.Free(idx->idx.ctx);
  free(idx);
}

int RedisIndex_Register(RedisModuleCtx *ctx) {
  IndexType = RedisModule_CreateDataType(
      ctx, "indextype", 0, RedisIndex_RdbLoad, RedisIndex_RdbSave,
      RedisIndex_AofRewrite, RedisIndex_Digest, RedisIndex_Free);
  if (IndexType == NULL) {
    return REDISMODULE_ERR;
  }

  return REDISMODULE_OK;
}
