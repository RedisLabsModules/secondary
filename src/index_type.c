#include "redismodule.h"
#include "index.h"
#include "key.h"
#include "index_type.h"
#include "rmutil/util.h"
#include "rmutil/vector.h"
#include "rmutil/alloc.h"
#include "rmutil/logging.h"

RedisModuleType *IndexType;

/* Serialize the index spec into an rdb/replication buffer */
void __redisIndex_SaveSpec(RedisIndex *idx, RedisModuleIO *io) {
  RedisModule_SaveUnsigned(io, (u_int64_t)idx->spec.flags);
  RedisModule_SaveUnsigned(io, (u_int64_t)idx->spec.numProps);
  RedisModuleCtx *ctx = RedisModule_GetContextFromIO(io);

  for (size_t i = 0; i < idx->spec.numProps; i++) {
    RM_LOG_NOTICE(ctx, "saving prop '%s type %d flags %x\n",
                  idx->spec.properties[i].name, idx->spec.properties[i].type,
                  idx->spec.properties[i].flags);

    if (idx->spec.flags & SI_INDEX_NAMED) {
      char *name = idx->spec.properties[i].name;
      RedisModule_SaveStringBuffer(io, name ? name : "",
                                   name ? strlen(name) : 0);
    }
    RedisModule_SaveSigned(io, (int)idx->spec.properties[i].type);
    RedisModule_SaveSigned(io, (int)idx->spec.properties[i].flags);
  }
}

/* Load the index spec from an rdb/replication buffer */
void __redisIndex_LoadSpec(RedisIndex *idx, RedisModuleIO *io) {
  idx->spec.flags = RedisModule_LoadUnsigned(io);
  idx->spec.numProps = RedisModule_LoadUnsigned(io);
  idx->spec.properties = calloc(idx->spec.numProps, sizeof(SIIndexProperty));

  for (size_t i = 0; i < idx->spec.numProps; i++) {
    if (idx->spec.flags & SI_INDEX_NAMED) {
      size_t slen;
      char *s = idx->spec.properties[i].name =
          RedisModule_LoadStringBuffer(io, &slen);
    }
    idx->spec.properties[i].type = RedisModule_LoadSigned(io);
    idx->spec.properties[i].flags = RedisModule_LoadSigned(io);
    // printf("loaded prop type %d flags %x\n", idx->spec.properties[i].type,
    //        idx->spec.properties[i].flags);
  }
}

/* Read a single SIValue from a redis io buffer. Returns NULL value if the value
 * type is unknown */
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
      v = SI_NullVal();
      break;
  }
  return v;
}

/* Write a single SIValue's data into an rdb/replication buffer */
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

/* The context for visitor callback functions traversing an index for
 * persistence */
typedef struct {
  RedisModuleIO *w;
  RedisIndex *idx;
  int num;
  RedisModuleString *indexKey;

} __redisIndexVisitorCtx;

/* a visitor callback for saving an index's records to RDB */
void __redisIndex_RdbVisitor(SIId id, void *key, void *ctx) {
  __redisIndexVisitorCtx *vx = ctx;
  SIMultiKey *mk = key;
  RedisModule_SaveStringBuffer(vx->w, id, strlen(id));

  for (int i = 0; i < vx->idx->spec.numProps; i++) {
    __writeValue(&mk->keys[i], mk->keys[i].type, vx->w);
    vx->num++;
  }
}

/* Serialize an index's records into an rdb buffer */
void __redisIndex_SaveIndex(RedisIndex *idx, RedisModuleIO *w) {
  size_t len = idx->idx.Len(idx->idx.ctx);
  RedisModuleCtx *ctx = RedisModule_GetContextFromIO(w);
  RedisModule_Log(ctx, "notice", "saving index of len %zd", len);

  // save the number of elements in the indes
  RedisModule_SaveUnsigned(w, (u_int64_t)len);

  __redisIndexVisitorCtx vx = {w, idx, 0, NULL};

  idx->idx.Traverse(idx->idx.ctx, __redisIndex_RdbVisitor, &vx);
  printf("saved %d elemets\n", vx.num);
}

/* Load all the index's data from an rdb buffer */
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

/* IDX.CREATE {name} [TYPE [HASH|STRING]] [UNIQUE] SCHEMA [{t}... ]|[{p1} {t1}]
  Create an index according to its spec string
*/
int SI_ParseSpec(RedisModuleCtx *ctx, RedisModuleString **argv, int argc,
                 SISpec *spec, SIIndexKind *kind) {
  // the index kind
  *kind = SI_AbstractIndex;

  // is the schema named or not
  int named = 0;

  int unique = RMUtil_ArgExists("UNIQUE", argv, argc, 2);

  RedisModuleString *typestr = NULL;
  RMUtil_ParseArgsAfter("TYPE", argv, argc, "s", &typestr);
  if (typestr != NULL) {
    const char *cts = RedisModule_StringPtrLen(typestr, NULL);
    if (!strcasecmp(cts, "HASH")) {
      *kind = SI_HashIndex;
      named = 1;
    }
  }

  int schemaPos =
      RMUtil_ArgExists("SCHEMA", argv, argc, unique ? unique + 1 : 2);
  if (!schemaPos || schemaPos >= argc - 1) {
    RedisModule_Log(ctx, "warning", "No schema found");
    return REDISMODULE_ERR;
  }

  if (named && (argc - (schemaPos + 1)) % 2 != 0) {
    RedisModule_Log(ctx, "warning", "Invalid schema argument count");
    return REDISMODULE_ERR;
  }

  spec->flags =
      0 | (unique ? SI_INDEX_UNIQUE : 0) | (named ? SI_INDEX_NAMED : 0);
  printf("flags: %x\n", spec->flags);
  spec->numProps =
      named ? (argc - (schemaPos + 1)) / 2 : argc - (schemaPos + 1);
  spec->properties = calloc(spec->numProps, sizeof(SIIndexProperty));
  int p = 0, i = schemaPos + 1;
  while (p < spec->numProps) {
    size_t len;

    if (named) {
      const char *propName = RedisModule_StringPtrLen(argv[i++], &len);
      spec->properties[p].name = strndup(propName, len);
    } else {
      spec->properties[p].name = NULL;
    }

    const char *str = RedisModule_StringPtrLen(argv[i++], &len);
    int ok = 0;
    for (int t = 0; types[t] != NULL; t++) {
      if (strlen(types[t]) == len && !strncasecmp(str, types[t], len)) {
        spec->properties[p].type = typeEnums[t];
        ok = 1;
        break;
      }
    }

    if (!ok) {
      RedisModule_Log(ctx, "warning", "could not parse %.*s", (int)len, str);
      SISpec_Free(spec);
      return REDISMODULE_ERR;
    }
    p++;
  }

  return REDISMODULE_OK;
}

/* Create a new redis index object with the given kind, flags and spec */
void *NewRedisIndex(SIIndexKind kind, u_int32_t flags, SISpec spec) {
  RedisIndex *idx = malloc(sizeof(RedisIndex));
  idx->kind = kind;
  idx->flags = flags;
  idx->spec = spec;
  idx->idx = SI_NewCompoundIndex(idx->spec);

  return idx;
}

/* Load the index's spec and data from rdb */
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

/* Save the index's spec and data to rdb */
void RedisIndex_RdbSave(RedisModuleIO *rdb, void *value) {
  RedisIndex *idx = value;
  RedisModule_SaveUnsigned(rdb, idx->kind);
  RedisModule_SaveUnsigned(rdb, idx->flags);

  // save the spec
  __redisIndex_SaveSpec(idx, rdb);

  // save the index data
  __redisIndex_SaveIndex(idx, rdb);
}

#define __vpushStr(v, ctx, str) \
  Vector_Push(v, RedisModule_CreateString(ctx, str, strlen(str)))
;

/* Convert the value of an SIValue to a redis module string */
RedisModuleString *siValueToRMString(RedisModuleCtx *ctx, SIValue v) {
  if (v.type == T_STRING) {
    return RedisModule_CreateString(ctx, v.stringval.str, v.stringval.len);
  }
  static char buf[128];
  SIValue_ToString(v, buf, 128);
  return RedisModule_CreateString(ctx, buf, strlen(buf));
}

/* Visitor callback for AOF rewriting */
void __redisIndex_AofVisitor(SIId id, void *key, void *ctx) {
  __redisIndexVisitorCtx *vx = ctx;
  SIMultiKey *mk = key;

  Vector *args = NewVector(RedisModuleString *, vx->idx->spec.numProps + 1);
  RedisModuleCtx *rctx = RedisModule_GetContextFromIO(vx->w);
  Vector_Push(args, RedisModule_CreateString(rctx, id, strlen(id)));

  for (int i = 0; i < vx->idx->spec.numProps; i++) {
    Vector_Push(args, siValueToRMString(rctx, mk->keys[i]));
  }

  RedisModule_EmitAOF(vx->w, "IDX.INSERT", "sv", vx->indexKey,
                      (RedisModuleString *)args->data, Vector_Size(args));
  Vector_Free(args);
}

void RedisIndex_AofRewrite(RedisModuleIO *aof, RedisModuleString *key,
                           void *value) {
  RedisIndex *idx = value;
  Vector *args = NewVector(RedisModuleString *, idx->spec.numProps);

  RedisModuleCtx *ctx = RedisModule_GetContextFromIO(aof);
  RedisModule_AutoMemory(ctx);

  if (idx->kind == SI_HashIndex) {
    __vpushStr(args, ctx, "TYPE");
    __vpushStr(args, ctx, "HASH");
  }

  __vpushStr(args, ctx, "SCHEMA");
  for (int i = 0; i < idx->spec.numProps; i++) {
    if (idx->spec.flags & SI_INDEX_NAMED) {
      __vpushStr(args, ctx, idx->spec.properties[i].name);
    }

    __vpushStr(args, ctx, types[idx->spec.properties[i].type]);
  }
  RedisModule_EmitAOF(aof, "IDX.CREATE", "sv", key,
                      (RedisModuleString *)args->data, Vector_Size(args));

  Vector_Free(args);

  __redisIndexVisitorCtx vx = {.w = aof, .idx = idx, .num = 0, .indexKey = key};

  idx->idx.Traverse(idx->idx.ctx, __redisIndex_AofVisitor, &vx);
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
