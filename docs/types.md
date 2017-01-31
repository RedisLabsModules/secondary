# Supported Property Types

Although the module does not enforce any type of schema, the indexes themselves are strictly typed. 
When creating an index, whether it is named or not, the module expects a list of properties and their types.

For example:

```sql
# Named Hash index:
IDX.CREATE users_name_age TYPE HASH SCHEMA name STRING age INT32

# Unnamed raw index
IDX.CREATE raw_index SCHEMA STRING INT32 FLOAT

```

When creating an index, you specify the schema and the types of properties. The supported types are:


| Type       | Length                  | Details                                  |
| ---------- | ----------------------- | ---------------------------------------- |
| **STRING** | up to 1024 bytes        | Binary safe string                       |
| **INT32**  | 32 bit                  | Singed 32 bit integer                    |
| **INT64**  | 64 bit                  | Singed 64 bit integer                    |
| **UINT**   | 64 bit                  | unsigned 64 bit integer                  |
| **BOOL**   | 8 bit (to be optimized) | boolean                                  |
| **FLOAT**  | 32 bit                  | 32 bit floating point number             |
| **DOUBLE** | 64 bit                  | 64 bit double                            |
| **TIME**   | 64 bit                  | 64 bit Unix timestamp (with helper functions) |


