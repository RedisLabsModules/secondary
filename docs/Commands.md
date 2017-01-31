
## IDX.CREATE

### Format

```
IDX.CREATE {index_name} [TYPE HASH] [UNIQUE] 
    SCHEMA [{property}] {type} ...
```

### Description

Create and index key `index_name` with a given schema.

If `TYPE HASH` is set, the index will have a named schema and can be used to index Hash keys. 

If `TYPE HASH` is not set, it is considered a raw index that can only be used with property ids (`$1, $2, ...`). More options will be available later.

If UNIQUE is set, the index is considered a unique index, and can only hold one id per value tuple.

**See [Supported Types](types.md) for the list of types in the schema.**


### Parameters

- **index_name**: The name of the index that will be used to query it.
- **TYPE HASH**: If set, the index will have a named schema and will be used to index Hash keys. More types might be supported in the future.
- **UNIQUE**: If set, the index is considered a unique index, and can only hold one id per value tuple.
- **SCHEMA**: the beginning of the schema specification, which is comprised of `property type` pairs in named indexes, and just `type` specifiers in unnamed indexes.

### Complexity

O(1)

### Returns

Status Reply: OK

### Examples

```sql
# Named Hash index:
IDX.CREATE users_name_age TYPE HASH SCHEMA name STRING age INT32

# Unnamed raw index
IDX.CREATE raw_index SCHEMA STRING INT32

# Named unique Hash index:
IDX.CREATE users_email TYPE HASH UNIQUE SCHEMA email STRING
```

## IDX.INSERT

### Format

```
IDX.INSERT index_name {id} {value} ...
```

### Description

**For raw indexes only** - add a value tuple consistent with the index's schema, and link it to the given id. 

### Parameters

- **index_name**: The name of the index that will be used to query it.
- **id**: The id of the object being indexed. This can be any string or number.
- **value(s)**: A list of values to be indexed. Its length must match the length of the index schema.

### Complexity

O(log(n)), where n is the size of the index

### Returns

Status Reply: OK

### Example

```sql
IDX.INSERT raw_index myId "foo" 32
```

---
## IDX.SELECT


### Format

```
 IDX.SELECT {index_name} WHERE {predicates}
```

### Description

**For Raw Indexes Only**: Select ids stored  in the index based on the WHERE clauses. Returns a list of ids. 

Columns in the index are referenced with 1-based numeric indexes, i.e. `$1, $2, ...`. 

See WHERE Expression Syntax for details on predicates.

### Parameters

- **index_name**: The name of the index that we want to query.
- **WHERE {predicates}**: WHERE expression with at least one predicate (condition).

### Complexity

O(log(n) + m), where n is the size of the index, and m is the number of matching ids.

### Returns

Array Reply: An array of matching ids.

### Example

```sql
IDX.SELECT users WHERE "$1='john' AND $2 IN (1,2,3,4)"
```

---


## IDX.DEL

### Format
```
IDX.DEL {index_name} {id} ...
```

### Description

**For raw indexes**: Delete multiple ids from the index.

### Parameters

- **index_name**: The index we want to delete from.
- **id(s)**: A list of ids that were inserted into the index.

### Complexity

O(log(n)), where n is the size of the index. This is multiplied by the number of ids deleted.

### Returns

Status Reply: OK

---

## IDX.CARD

### Format

```
IDX.CARD {index_name}
```

### Description

Return the number of keys (ids) stored in the index. 

**Note**: Only in a unique index it is guaranteed to be the same number of distinct value tuples in the index.

### Parameters

- **index_name**: The index we want to get the cardinality for.

### Complexity

O(1)

### Returns

Integer Reply: the index cardinality

---

## IDX.FROM

### Format

``` 
  IDX.FROM {index_name} 
    WHERE {predicates} 
    {ANY REDIS READ COMMAND}
```

### Description

Proxy a Redis read command to all the keys matching the WHERE clause predicates. 

In the specified command, the `$` character is substituted by the matching Redis key. 

An array of the responses of running the command per each matching key is returned (it may include errors).

### Parameters

- **index_name**: The index we want to use.
- **WHERE {predicates}**: WHERE expression with at least one predicate (condition).

### Example

```sql
IDX.FROM users_name_age WHERE "name LIKE 'john%'" HGETALL $
```

### Complexity

O(log(n) + m), where n is the index size, and m is the number of matching ids. On top of that, the complexity of the proxied command should also be taken into account.

### Returns

Array Reply: an array of the responses of running the command per each matching key is returned (it may include errors).

---

## IDX.INTO

### Format

``` 
IDX.INTO {index_name} 
    [WHERE {predicates}] 
    {HASH WRITE COMMAND} 
```

### Description

**For Hash Indexes Only:** Execute a HASH write command, and update the key to reindex the object. 

Optionally the command (such as HINCRBY) can be executed on any key matching a WHERE predicate, but this is not needed. 

Normally you would just update the key as you would ordinarily in Redis, and the index will track the changes automatically. 

The supported commands are: `HINCRBY, HINCRBYFLOAT, HMSET, HSET, HSETNX` and `DEL`.

### Parameters

- **index_name**: The index we want to use.
- **WHERE {predicates}**: Optional WHERE expression with at least one predicate (condition).

### Examples:

```sql

# Simple tracking of HMSET on an index
IDX.INTO users_name_age HMSET user1 name "John Doe" age 42

# Deleting objects matching a predicate
IDX.INTO users_name_age WHERE "name LIKE 'j%'" DEL $

```

### Complexity

O(log(n) + m), where n is the index size, and m is the number of matching ids. On top of that, the complexity of the proxied command should also be taken into account.

### Returns

Array Reply: an array of the responses of running the command per each matching key is returned (it may include errors).

