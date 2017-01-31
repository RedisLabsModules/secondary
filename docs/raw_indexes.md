# Using Raw Indexes 

Using a raw index,there is no degree of automation, the index is just a way to automate storing 
and querying values that map to ids. These ids do not even need to represent Redis keys - you 
can use Redis indexes for objects stored in another databases, files, or anything you like. 

## Creating a raw index

You begin by creating the index, and specifying a schema. In a raw index, fields have no names, just an index number:

```sql
> IDX.CREATE myindex SCHEMA STRING INT32
```

This means that each id in the index will be indexed by two values, a string and a 32-bit signed integer (See [Supported Property Types](types)) 

The string will be referred to as `$1` in queries, and the integer as `$2`

## Inserting data to the index

Again, in raw indexes there is no automation, you just push an **id** and a tuple of values that must match the index's schema.

```sql
> IDX.INSERT myindex id1 "hello world" 1337
```

This means that we've indexed a key named **id** with the value tuple `("hello world", 1337)`. 

## Querying the index

Now that the we have data in the index, we can query it and get ids matching the query:

```sql
> IDX.SELECT FROM myindex WHERE "$1 = 'hello wolrd' AND $1 = 1337"
1) "id1"

```

we simply get a list of ids matching our query.  You can page the results with `LIMIT {offset} {num}`.

>  **NOTE:** The full syntax of **WHERE** expressions, which loosely follows SQL syntax, can be found in the [WHERE reference](WHERE).


