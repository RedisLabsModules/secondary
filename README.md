# Redis Secondary Index Module

This project is a Redis Module that adds the ability to create and query secondary indexes in Redis. 

> ### This is a preview of the module. 
>
> The API is not stable, and it should not yet be used in production. You are, however more than welcome to try it out and report any problems or wishes!



## Quick Example:

```sql
    # Creating an index
    127.0.0.1:6379> IDX.CREATE users_name_age TYPE HASH SCHEMA name STRING age INT32
    OK
    
    # Inserting some indexed Hash keys
    127.0.0.1:6379> IDX.INTO users_name_age HMSET user1 name "alice" age 30
    (integer) 1
    127.0.0.1:6379> IDX.INTO users_name_age HMSET user2 name "bob" age 25
    (integer) 1

    # Indexed HGET
    127.0.0.1:6379> IDX.FROM users_name_age WHERE "name LIKE 'b%'" HGET $ name
    1) user2
    2) "bob"

    # Indexed HGETALL
    127.0.0.1:6379> IDX.FROM users_name_age WHERE "name >= 'alice' AND age < 31" HGETALL $
    1) user1
    2) 1) "name"
    2) "alice"
    3) "age"
    4) "30"
    3) user2
    4) 1) "name"
    2) "bob"
    3) "age"
    4) "25"
```


## How It Works

This module adds a new data type to Redis: a compound index, internally based on a **SkipList**.

It stores tuples of values mapped to keys in redis, and can be queried using simple WHERE expressions
in a SQL-like language. 

The indexes can be used independently using commands that add and remove data to them; Or 
by proxying ordinary Redis commands, allowing to index HASH objects automatically. 

Indexes need to be pre-defined with a schema in order to work, but they do not enforce a schema. 



# Building and setting up

1. Clone/download the module source code.
2. using cmake, boo

## Using Raw Indexes 

Using a raw index,there is no degree of automation, the index is just a way to automate storing 
and querying values that map to ids. These ids do not even need to represent Redis keys - you 
can use Redis indexes for objects stored in another databases, files, or anything you like. 

* **Creating a raw index**

  You begin by creating the index, and specifying a schema. In a raw index, fields have no names, just an index number:

  ```sql
  > IDX.CREATE myindex SCHEMA STRING INT32
  ```

  This means that each id in the index will be indexed by two values, a string and a 32-bit signed integer (See *Supported Property Types* below) 

  The string will be referred to as `$1` in queries, and the integer as `$2`

* **Inserting data to the index:**

  Again, in raw indexes there is no automation, you just push an **id** and a tuple of values that must match the index's schema.

  ```sql
  > IDX.INSERT myindex id1 "hello world" 1337
  ```

  This means that we've indexed a key named **id** with the value tuple `("hello world", 1337)`. 

* **Querying the index:**

  Now that the we have data in the index, we can query it and get ids matching the query:

  ```sql
  > IDX.SELECT FROM myindex WHERE "$1 = 'hello wolrd' AND $1 = 1337"
  1) "id1"

  ```

  we simply get a list of ids matching our query.  You can page the results with `LIMIT {offset} {num}`.

  >  **NOTE:** The full syntax of **WHERE** expressions, which loosely follows SQL syntax, can be found in the `SYNTAX.md` file.




## Supported Property Types

Although the module does not enforce any type of schema, the indexes themselves are strictly type. When creating an index, you specify the schema and the types of properties. The supported types are:

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




## Automatic HASH object indexing

While raw indexes are flexible and you can use them for whatever you like, automatic HASH indexes offer a nicer, 
easier way to work with secondary indexes, if you use Redis HASH objects as an object store. 

Since the modules API cannot yet track changes to keys automatically, the idea is that you proxy your HASH manipulation commands via the indexes.  

* ### Creating HASH indexes:

  ```sql
  > IDX.CREATE users_name TYPE HASH SCHEMA name STRING
  ```

* ### Proxying write commands

  Read commands are proxied with the syntax:	

  ```sql
  IDX.FROM {index} WHERE {WHERE clause} {ANY REDIS COMMAND}
  ```

  The command is executed per matching id, and the result is chained to the result of the FROM command. 

  > **The matching ids in the commands are represented with a `$` character.** 

  Write commands are proxied with the syntax:

  ```sql
  IDX.INTO {index} [WHERE {WHERE clause}] {HASH WRITE REDIS COMMAND}
  ```

  The supported commands for writing are: `HINCRBY, HINCRBYFLOAT, HMSET, HSET, HSETNX` and `DEL`.

  A WHERE clause is not necessary - if you just perform a HMSET for example, the index assumes it just needs to index a specific object and takes its key from the command.


## WHERE Claus Syntax

The WHERE clause query language is a subset of standard SQL, with the currently supported predicates:	

```sql
<, <=, >, >=, IN, LIKE, IS NULL
```

Predicates can be combined using `AND` and `OR` operations, grouped by `(` and `)` symbols. For example:

```sql
(foo = 'bar' AND baz LIKE 'boo%') OR (wat <= 1337 and word IN ('hello', 'world'))
```

### Time Functions

For time typed index properties, we support a few convenience functions for WHERE expressions (note that they can ONLY be used in WHERE expressions and not passed to the redis commands):

| Function                          | Meaning                                  |
| --------------------------------- | ---------------------------------------- |
| NOW                               | The current Unix timestamp               |
| TODAY                             | Unix timestamp of today's midnight UTC   |
| TIME_ADD({timetsamp}, {duration}) | Add a duration (minutes, hours etc) to a stimestamp. i.e. `TIME_ADD(NOW, HOURS(5))` |
| TIME_SUB({timestamp}, {duration}) | Subtract a duration (minutes, hours etc) from a stimestamp. |
| TIME({int}), UNIXTIME({int})      | Convert an integer to a Timestamp type.  |
| HOURS(N)                          | Return the number of seconds in N hours  |
| DAYS(N)                           | Return the number of seconds in N days   |
| MINUTES(N)                        | Return the number of seconds in N minutes |

​	

> #### Example usage of time functions in WHERE:

> ```sql
> # Selecting users with join_time in the last 30 days
> WHERE "join_time >= TIME_SUB(TODAY, DAYS(30))"
>
> # Selecting blog posts with pub_time in the past 4 hours
> WHERE "pub_time >= TIME_SUB(NOW, HOURS(4))"
> ```



### Tips and gotchas for  WHERE

*  Inequality operators (`!=`, `NOT NULL`) are not yet supported, but the <, > etc operators work fine.

*  The `LIKE ` syntax is not compatible to SQL standards. It only supports full equality, or prefix matching with `%` at the end of the string.

*  You can only query the index for properties indexed in it, we do not support full scans. Also `WHERE TRUE` style scans are not supported. A temporary workaround would be to do `$1 >= ''` for strings.

*  The order of properties in the index affects the query efficiency. We produce scan ranges on the index from the left field onwards. Once we cannot produce efficient scan keys (for example if you search on the first and third field, we can only scan on the first one), the rest of the predicates are evaluated per scanned key, and **actually reduce performance**. A few examples:

   ```sql
   # Good - produces only a single scan key
   WHERE "$1 = 'foo' AND $2 = 'bar' "

   # Bad - produces a scan key and a filter:
   WHERE "$1 = 'foo' AND $3 = 'bar'"

   # Good: Produces a few scan ranges:
   WHERE "$1 IN ('foo', 'bar') AND $2 > 13"

   # Bad: produces complex filters:
   WHERE "$1 IN ('foo', 'bar') AND ($2 = 13 OR $3 < 4)"
   ```



## Full Commands API

### IDX.CREATE index_name [TYPE HASH] [UNIQUE] SCHEMA [ [property] TYPE ...]

Create and index key `index_name` with a given schema. If `TYPE HASH` is set, the index will have a named schema and can be used to index Hash keys. 

If `TYPE HASH` is not set, it is considered a raw index that can only be used with property ids (`$1, $2, ...`). More options will be available later.

If UNIQUE is set, the index is considered a unique index, and can only hold one id per value tuple.

Examples:

```sql
# Named Hash index:
IDX.CREATE users_name_age TYPE HASH SCHEMA name STRING age INT32

# Unnamed raw index
IDX.CREATE raw_index SCHEMA STRING INT32

# Named unique Hash index:
IDX.CREATE users_email TYPE HASH UNIQUE SCHEMA email STRING
```


See Supported Types for the list of types in the schema.

### IDX.INSERT index_name id val1 val2 ...

**For raw indexes only** - add a value tuple consistent with the index's schema, and link it to the given id. 

Example:

```sql
IDX.INSERT raw_index myId "foo" 32
```

### IDX.SELECT index_name WHERE predicates

**For raw indexes** - Select ids stored  in the index based on the WHERE clauses. Returns a list of ids.

> **Note**: Paging is not yet supported but will be soon.

### IDX.DEL index_name id id ...

**For raw indexes -** Delete ids from the index.

###  IDX.CARD index_name

Return the number of keys (ids) stored in the index. Only in a unique index it is guaranteed to be the same number of distinct value tuples in the index.

### IDX.FROM index_name WHERE predicates {ANY REDIS READ COMMAND}

Proxy a Redis read command to all the keys matching the WHERE clause predicates. In the specified command, the `$` character is substituted by the matching Redis key. An array of the responses of running the command per each matching key is returned (it may include errors).

Example:

```sql
IDX.FROM users_name_age WHERE "name LIKE 'john%'" HGETALL $
```

### IDX.INTO index_name [WHERE  predicates] { HASH WRITE COMMAND } 

**For Hash indexes only:** execute a HASH write command, and update the key to reindex the object. Optionally the command (such as HINCRBY) can be executed on any key matching a WHERE predicate, but this is not needed. Normally you would just update the key as you would ordinarily in Redis, and the index will track the changes automatically. 

The supported commands are: `HINCRBY, HINCRBYFLOAT, HMSET, HSET, HSETNX` and `DEL`.

Examples:

```sql
IDX.INTO users_name_age HMSET user1 name "John Doe" age 42

IDX.INTO users_name_age WHERE "name LIKE 'j%'" DEL $
```



## On The Roadmap:

This is a pre-release of the module, and it is currently in active development. The tasks for the full release include:

* A powerful aggregation engine that can be used for analytics
* More index types:
  * Geospatial
  * FullText (probably with a merge of the RediSearch module)
  * Generic multi-dimensional indexes
* Asynchronous cursors that will not block redis on very big records sets.
* Indexing of sorted sets and string values.
* Index rebuilding and creation without blocking redis.
* Allowing multiple indexes to proxy a single command.
* Distributed mode working across many cluster shards.
* Automatic repair thread for consistency.