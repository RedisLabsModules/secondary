# Automatic HASH object indexing

While raw indexes are flexible and you can use them for whatever you like, automatic HASH indexes offer a nicer, 
easier way to work with secondary indexes, if you use Redis HASH objects as an object store. 

Since the modules API cannot yet track changes to keys automatically, the idea is that you proxy your HASH manipulation commands via the indexes.  

## Creating HASH indexes:

```sql
> IDX.CREATE users_name TYPE HASH SCHEMA name STRING
```

## Proxying read commands

Read commands are proxied with the syntax:	

```sql
IDX.FROM {index} WHERE {WHERE clause} {ANY REDIS COMMAND}
```

The command is executed per matching id, and the result is chained to the result of the FROM command. **The matching ids in the commands are represented with a `$` character.** 

## Proxying write commands

Write commands are proxied with the syntax:

```sql
IDX.INTO {index} [WHERE {WHERE clause}] {HASH WRITE REDIS COMMAND}
```

The supported commands for writing are: `HINCRBY, HINCRBYFLOAT, HMSET, HSET, HSETNX` and `DEL`.

A WHERE clause is not necessary - if you just perform a HMSET for example, the index assumes it just needs to index a specific object and takes its key from the command.
