# Redis Secondary Index Module

This project is a Redis Module that adds the ability to create and query secondary indexes in Redis. 

> ### This is a preview of the module. 
>
> The API is not stable, and should be used with caution


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


## Building and setting up

1. Clone/download the module source code.

2. using cmake, bootstrap and build the project: 

   `cmake build ./ && make all`

3. run redis (unstable or >4.0) with the module library `src/libmodule.so`:

   `redis-server --loadmodule ./src/libmodule.so`
