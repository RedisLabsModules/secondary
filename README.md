# Redis Secondary Indexes

This project is a Redis Module that adds the ability to create and query secondary indexes in Redis. 

## Quick Example:


        127.0.0.1:6379> IDX.CREATE users_name_age TYPE HASH SCHEMA name STRING age INT32
        OK
    
        127.0.0.1:6379> IDX.INTO users_name_age HMSET user1 name "alice" age 30
        (integer) 1
        127.0.0.1:6379> IDX.INTO users_name_age HMSET user2 name "bob" age 25
        (integer) 1
    
        127.0.0.1:6379> IDX.FROM users_name_age WHERE "name LIKE 'b%'" HGET $ name
        1) user2
        2) "bob"
    
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


## How It Works

This module adds a new data type to Redis: a compound index, internally based on a **SkipList**. 
It stores tuples of values mapped to keys in redis, and can be queried using simple WHERE expressions
in a SQL-like language. 

The indexes can be used independently using commands that add and remove data to them; Or 
by proxying ordinary Redis commands, allowing to index HASH objects automatically. 

Indexes need to be pre-defined with a schema in order to work, but they do not enforce a schema. 

## Using Raw Indexes 

Using a raw index,there is no degree of automation, the index isjust a way to automate storing 
and querying values that map to ids. These ids do not even need to represent Redis keys - you 
can use Redis indexes for objects stored in another databases, files, or anything you like. 

* **Creating a raw index**

  You begin by creating the index, and specifying a schema. In a raw index, fields have no names, just an index number:

  ```sql
  > IDX.CREATE myindex SCHEMA STRING INT32
  ```

  This means that each id in the index will be indexed by two values, a string and a 32-bit signed integer. The string will be referred to as `$1` in queries, and the integer as `$2`

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



## Automatic HASH object indexing

While raw indexes are flexible and you can use them for whatever you like, automatic HASH indexes offer a nicer, easier way to work with secondary indexes, if you use Redis HASH objects as an object store. 

Since the modules API cannot yet track changes to keys automatically, the idea is that you proxy your HASH manipulation commands via the indexes.  

* Creating HASH indexes:

  ```sql
  > IDX.CREATE users_name TYPE HASH SCHEMA name STRING
  ```

* Proxying write commands

Read commands are proxied with the syntax:

```sql
IDX.FROM {index} WHERE {WHERE clause} {ANY REDIS COMMAND}
```

The command is executed per matching id, and the result is chained to the result of the FROM command. **The matching ids in the commands are represented with a `$` character.** 

Write comman