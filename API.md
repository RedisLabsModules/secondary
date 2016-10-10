# Proposed Secondary Index External API

## 1. Explicit Anonymous Indexes

### Purpose: 

* Use as an index for external databases
* Index existing data already inside redis, independent of how this data is modeled.
* Facilitate writing ORMs and database engines on top of redis

### Basic Concepts:
1. Indexes map n-tuples of values to ids only (e.g. (string, string, int)).
2. Indexes are created before they are updated, with a given schema (as above)
3. The lookups are anonymous and refer to index columns only by sequence number ($1, $2, $3...).
4. The index does not track anything and must be called explicitly. 
5. The query language is minimal and contains mostly predicates.
6. Book-keeping of the index is not handled by the index, or is optional.
    * If it is not handled, the user needs to remove and add records to the index manually when values change.
    * If it is handled, we keep a reverse index of {id => values}, and when an id is indexed, if it already exists
        we simply remove the old record. 
        This has a lot of memory overhead, so if a higher level engine can do this by comparing object states on updates,
        it is preferable not to do this.

### Propose API:


* Creating an index:

        IDX.CREATE <index_name> [OPTIONS [TRACKING] [UNIQUE]] SCHEMA <type> [<type> ...]`

* Inserting ids and tuples into an index:

        IDX.INSERT <index_name> <id> <value> [<value> ...]

* Deleting ids from an index (if the index is not tracking, the values must also be given):
  ​      
        IDX.DELETE <index_name> <id> [<value> [<value> ...]] 

* Selecting ids from an index:

        IDX.SELECT <index_name> WHERE "<predicates>" [LIMIT offset num]

    Returns a list of ids matching the predicates, with the index's natural ordering.

    Predicates are similar to the WHERE clause of an SQL query. e.g.:

        WHERE "$1 < 1 AND $2 = 'foo' AND $3 IN (1,2,'foo','bar') 

* General info (cardinality, memory, etc):

        IDX.INFO <index_name>

* Flushing an index without deleting its schema:
  ​      
        IDX.FLUSH <index_name>

* Predicate expressions syntax:

    Since they are anonymous, the index properties or columns are represented by a dollar sign and their numeric order in the index's schema.
    i.e. $1, $2, $3 etc. Of course, they must be within the range of the index's schema length.  

    #### pseudo EBNF query syntax:


        <query> ::= <predicate> | <predicate> "AND" <predicate> ... 

        <predicate> ::= <property> <operator> <value>

        <property> ::= "$" <digit>

        <operator> ::= "=" | "!=" | ">" | "<" | ">=" | "<=" | "BETWEEN" | "IN" | "LIKE"

        <value> ::= <number> | <string> | "TRUE" | "FALSE" | <list> | <timestamp>

        <list> ::= "(" <value>, ... ")"
        
        
        <timestamp> ::= <integer> | "NOW" | "TODAY" | "TIME_SUB" "(" <timestamp> "," <duration> ")" | "TIME_ADD" "(" <timestamp> "," <duration> ")"
        
        <duration> ::= SECONDS "(" <integer> ")" | DAYS "(" <integer> ")" | HOURS "(" <integer> ")" | MINUTES "(" <number> ")" | <seconds>
        
        <seconds> ::= <integer>
        
        e.g. TIME_ADD(TIME_SUB(NOW, DAYS(1)), HOURS(5))


## 2. Indexed Redis Structures

### Purpose:
* Extend redis common patterns with secondary indexing
* Keep using the basic redis concepts and structures with as little disruption
* Allow more automation and less boiler plate than #1 

### How It Works:

* Indexes are created either on HASHes, complete strings, and possibly ZSETs.
* For hashes, the schema includes not only types, but also the entity names.
* For strings, the index just indexes the entire value of the string (up until a limited value)
* For sorted sets, the index indexes values and scores.
* The index DOES NOT track changes in keys implicitly.
* Instead, write operations to the keys are done **through** the index.

    i.e. in the case of a HASH, the indexing module implements HSET/HMSET/HINCRBY so it knows which key is updated automatically.

* The index also implements the GET family of functions for the type, using WHERE clauses.

Proposed API:

* Creating an index:

        IDX.CREATE [OPTIONS [UNIQUE]] TYPE [HASH|STR|ZSET] [SCHEMA <field> <type> ...]

    **Note:** more options TBD

* Writing - HASH values:

        IDX.HSET <index_name> <key> <element> <value>

        IDX.HMSET <index_name> <key> <element> <value> ...

        IDX.HDEL <index_name> <key> <element>

        IDX.HINCRBY <index_name> <key> <element> <amount>

1. First ordered list item
2. Another item
   ⋅⋅* Unordered sub-list. 
3. Actual numbers don't matter, just that it's a number
   ⋅⋅1. Ordered sub-list
4. And another item.

* ## Reading - HASH values:
  .....foo
```
        IDX.HGETALL <index_name> WHERE <predicates> [LIMIT offset num] 
        (returns an array of interleaved key, values, where values is a nested array of all the hash values)
        
        IDX.HMGET <index_name> <num_elements> <elem> <elem> ... WHERE <predicates> [LIMIT offset num]
        (same as HGETALL but gets only some of the elements, and does not return the element names, just the values)
        
        IDX.HGET <index_name> <elem> WHERE <predicates> [LIMIT offset num]
        (same as HGETALL but gets only one element's value for each matching HASH)
```

* ## Writing - strings:

```
        IDX.SET <index_name> <key> <value> <key> <value> ...
        IDX.SETNX <index_name> <key> <value> <key> <value> ...
        IDX.SETEX <index_name> <key> <value> <key> <value> ...
        IDX.INCRBY <index_name> <key> <value> <key> <value> ...

```

### TBD: how do we handle expires? 

for now we simply assume no expiration support, until redis has keyspace notifications for modules. 
​        
3. Generic Entity Store

Purpose:
    * Abstract both indexing and storing of schemaless or semi-schemaless objects in redis.
    * Add higher level constructs that are not native to redis but use redis structures internally.
    * Allow usage of redis as a more advanced entity store than it is with little friction.

4. Table based RDBMS with an SQL-like langues

Purpose:
    * Allow a more familiar usage pattern, while still retaining some of redis' power.
    * Allow more compatibility with existing code and libraries (ORMs etc)
    * Allow a more efficient representation of tabular/coulmnar data than its native data types.  