# Proposed Redis Secondary Index Module API

## What's This?

Redis is a very sophisticated data structure server, that can be used as a powerful in-memory database. However, if you look at redis as database per-se, it only has primary keys. There is no native way to ask redis for something like "what are the names of users over 18 who have visited my website yesterday?". 

If you're familiar with traditional relational databases, this is usually done by creating an index on the relevant columns in your table, allowing to efficiently add complex *WHERE* clauses to your query. Such an index is called a Secondary Index.

While it is possible (and done by many many people) to implement these indexes on top of redis manually, doing them right and in a performant way is hard. 

This is why I think it is a very strong case in which Redis modules can be put to work, adding abilities to redis that make it more complex, but more powerful for some use cases. This doucment describes a proposed API and query language for secondary indexes in  Redis. *Note that it does not go into the detail of the internal design of how they work. *

The API includes three layers that can be used separately but are orthogonal to one another:

1. **Explicit Anonymous Indexes** - providing raw indexes as a primitive data type in Redis.

2. **Indexed Redis Objects** - Adding secondary indexes to common redis data structures, and a way to query them.

3. **Aggregations API** - Building on indexed objects, provide a way to run various aggregations of query results.

---         

## 1. Explicit Anonymous Indexes

### Purpose: 

* Use Redis as an index for external databases.
* Index existing data already inside redis, independent of how this data is modeled.
* Facilitate writing ORMs and database engines on top of redis.

### Basic Concepts:
1. Indexes map n-tuples of values to ids only (e.g. (string, string, int)).
2. Indexes are created before they are updated, with a given schema (as above).
2. The lookups are anonymous and refer to index columns only by sequence number ($1, $2, $3...).
3. The index does not track any change implicitly and must be called explicitly. 
4. The user "inserts" records into the index, each record is a string Id, followed by a tuple of values that must conform to the index's schma.
4. The query language is minimal and contains mostly predicates. Queries return just Ids, and it is up to the user to decide what to do with them.
5. Book-keeping of the index (allowing deletion of records by id) is not handled by the index, or is optional. 
    * If it is not handled, the user needs to remove and add records to the index manually when values change.
    * If it is handled, we keep a reverse index of {id => values}, and when an id is indexed, if it already exists
        we simply remove the old record. 
        This has a lot of memory overhead, so if a higher level engine can do this by comparing object states on updates,
        it is preferable not to do this.

    ### Proposed API:

    * Creating an index:

            IDX.CREATE <index_name> [OPTIONS [TRACKING] [UNIQUE]] SCHEMA <type> [<type> ...]`

    * Inserting ids and tuples into an index:

            IDX.INSERT <index_name> <id> <value> [<value> ...]

    * Deleting ids from an index (if the index is not tracking, the values must also be given):
            
            IDX.DELETE <index_name> <id> [<value> [<value> ...]] 

    * Selecting ids from an index:
        
            IDX.SELECT <index_name> WHERE "<predicates>" [LIMIT offset num]

        Returns a list of ids matching the predicates, with the index's natural ordering.

        Predicates are similar to the WHERE clause of an SQL query. e.g.:
        
            WHERE "$1 < 1 AND $2 = 'foo' AND $3 IN (1,2,'foo','bar') 

    * General info (cardinality, memory, etc):

            IDX.INFO <index_name>

    * Flushing an index without deleting its schema:
            
            IDX.FLUSH <index_name>

    * Predicate expressions syntax:

        Since they are anonymous, the index properties or columns are represented by a dollar sign and their numeric order in the index's schema.
        i.e. $1, $2, $3 etc. Of course, they must be within the range of the index's schema length.  

        pseudo BNF query syntax:

            <query> ::= <predicate> | <predicate> "AND" <predicate> ... 
            <predicate> ::= <property> <operator> <value>
            <property> ::= "$" <digit>
            <operator> ::= "=" | "!=" | ">" | "<" | ">=" | "<=" | "BETWEEN" | "IN" | "LIKE"
            <value> ::= <number> | <string> | "TRUE" | "FALSE" | <list>
            <list> ::= "(" <value>, ... ")"
    
    * Example Usage:
    
            # Creating an index on two strings and an integer, or first/last name and age.
            
            > IDX.CREATE myidx OPTIONS TRACKING SCHEMA STRING STRING INT32

            # Adding some records

            > IDX.INSERT myidx user1 "John" "Doe" 18
            > IDX.INSERT myidx user2 "Herp" "Derp" 37
            > IDX.INSERT myidx user3 "Jeff" "Lebowski" 45

            # Querying the index - getting all users called Jeff Lebowski
            > IDX.SELECT myidx WHERE "$1 = 'Jeff' AND $2 = 'Lebowski'"  

            # Querying the index - getting all users called Jeff aged over 18
            > IDX.SELECT myidx WHERE "$1 = 'Jeff' AND $3 > 18"  

            # Removing a record. Since this is a tracking index, no need for values
            > IDX.DELETE myidx user3
        

---

## 2. Indexed Redis Structures

### Purpose:
* Extend redis common patterns with secondary indexing
* Keep using the basic redis concepts and structures with as little disruption
* Allow more automation and less boilerplate than #1 
* Provide a query language that feels familiar to redis users *and* SQL users.

### How It Works:

* Indexes are created either on HASHes, complete strings, (and possibly ZSETs in the future).
* For hashes, the schema includes not only types, but also the entity names.
* For strings, the index just indexes the entire value of the string (up until a limited length). Numeric indexes assume that string values represent numbers.
* For sorted sets, the index indexes values and scores.
* The index DOES NOT track changes in keys implicitly.
* Instead, write operations to the keys are done **through** the index.
* This is done by piping write commands via the index.
* Reading is done by applying generic redis commands using the index.

    Proposed API:

    * Creating an index:

            IDX.CREATE <index_name> [OPTIONS [UNIQUE]] TYPE [HASH|STR|ZSET] [SCHEMA <field> <type> ...]

    * Generic index based execution:

            IDX.FROM <index_name> WHERE <predicates> [LIMIT offset num] [ANY REDIS READONLY COMMAND]
            IDX.INTO <index_name> [WHERE <predicates>] DO [ANY REDIS WRITE COMMAND]  

            we denote the id using something like $, * or _
            e.g.:
        
            IDX.FROM myidx WHERE "name='foofi'" HGETALL $
            IDX.FROM myidx WHERE "$1='foofi'" GET $

            IDX.INTO users HMSET user1 name foo last bar
            IDX.INTO users WHERE "name='foo'" HSET $ last_access = NOW()
            
    * Index Rebuilding

        Creating an index doesn't do anything, but we provide a mechanism for rebuilding indexes in a non blocking way:

            IDX.REBUILD <index_name> [MATCH <key pattern>] [ASYNC <cursor id>]

        The MATCH is used to tell the index which id pattern should be scanned to rebuild it. 
        If it is an existing index that's been corrupted due to bugs or consistency issues, we don't need to use pattern,
        and can just use the index's backwards reference table of ids to recreate itself automatically. 

        ASYNC tells the index not to block redis entirely, but do this iteratively using SCAN. In this case the command returns 
        SCAN iterator ids, and works just like SCAN does. This means the client needs to call REBUILD many times with cursor ids, 
        but each REBUILD call is very short and will not block redis.

        **NOTE**: When redis will include asynchronous operations this complexity will not be needed, and we will be able to do
        index rebuilding and maintenance on a separate thread.   

    
## 3. Aggregations:

The above indexing scheme also allows us to do aggregations on HASH properties or complete string values. This is useful for analytics, for example.

The idea is to do an indexed scan, and for each matching redis object, feed the existing values (be them hash element values, complete values of strings, values and scores of sorted sets) into an aggregation function. This may produce a single number, a list of values, etc. This can be done by accessing the HASH elements themselves, or for better speed, accessing values already in the index.

* Proposed Aggregation Syntax: 

            IDX.AGGREGATE <index_name> <aggregation_func> ... WHERE <predicates> 
    
* Aggregation Function Grammar:

            <property> ::= <identifier> | "_"

            <aggregation> ::= <agg_func> <arglist>

            <agg_func> ::= "SUM" | "COUNT" | "HISTOGRAM" | "AVG" | "MEAN" | "MAX" | "NLARGEST" | "NSMALLEST" 

            // "_" is used to refer to "self value" when aggregating indexes on whole redis string keys
            <arglist> ::= "(" <ident> | "_" | <transformation> { <param> "," ... } ")"

            <param> ::= <number> | <string>  

            <transformation> ::= <trans_func> <arglist>

            <trans_func> ::= "ROUND" | "FLOOR" | "CEIL" | ...
    
    * Example Usage

            # Getting age distribution of users named Jeff:
            > IDX.AGGREGATE users COUNT_DISTINCT(FLOOR(age)) WHERE "name = 'Jeff'
            (returns a list of age,count pairs)

    * Proposed Aggregation functions:

    > | function        | Description  |
    > |-------------|-------------|
    > |SUM(P) | Sum numeric values of P for all matching objects| 
    > | COUNT() | Count the total matching objects for the query |
    > | COUNT_DISTINCT(property) || 
    > | HISTOGRAM(property)  || 
    > | AVG(property) || 
    > | MEAN(property)  || 
    > | MAX(property) || 
        
    * Proposed Transformation functions:
    
    > | function        | Description  |
    > |-------------|-------------|
    > | ROUND(x)
    > | FLOOR(x)
    > | CEIL(x)
    > | LOG(x, base)
    > | MODULO(x, y)
    > | QUANTIZE(x, ranges)
    > | FILTER_ABOVE(x, y)
    > | FILTER_BELOW(x, y)
    > | AND(x, y)
    > | XOR(x, y)
    > | OR (x, y)
    > |
    > | String Functions |
    > | 
    > | LOWER(x)
    > | UPPER(x)
    > | REPLACE(x, y)
    > ||
    > | Time Functions |
    > ||
    > | DAY(x) | Day of Month|
    > | MONTH(x) | Month Of Year|
    > | YEAR(x) | |
    > | WEEK(x) ||
    > |  WEEKDAY(x) | |
         
  

  userId,timestamp


  <agg> ::= <expr> | <func> | <pipeline> 
  <pipeline> ::= <func> "." <func> | <pipeline> -> <func> // recursive  
  <func> ::= <ident> "(" <arglist> ")" | <ident> "()"
  <arglist> ::= <expr> {"," <expr> ... }
  <expr> ::= <func> | <const> | <ident> | <index> | <predicate> | <property>
  <predicate> ::= <expr> <operator> <expr> | <func>
  <operator> ::= "=" | "!=" | ">" | "<" | ">=" | "<=" | "BETWEEN" | "IN" | "LIKE" | "MATCH"




  select(userId,timestamp) // => [(uid,ts), ....]
        .mapElement(1, quantize(3600)) // => [(uid,hour), ...]
        .unique(0,1) // => {(uid,hour), ...} 
        .groupBy(1) // => [(hour, [(uid,hour)])]
        .mapElement(1, reduce(count())) /// [(hour, count), ...]
        .orderBy(0)

select(userId,timestamp) 
        transform(quantize(3600), _1) ->

        unique(format("{}::{}", _0 , _1)) ->
        reduce(countDistinct, _1) ->
        orderBy(_0, order:"asc");

clientId, joinDate, month, sum

select(clientId, joinDate, month, amount).
groupBy(@month).                                                    
.transform(_1, 
        transform(_1, timediff('months')).alias( _1, @age).         
        groupBy(@age).                                              
        transform(_1, reduce(sum)).  // [(age, total)]
        orderBy(_0) 
).orderBy(@month)


tranformations:
  zip(seq, seq, ...) -> seq | return a sequence of tuples where each element is the next element of each of the input sequences
  chain(seq, seq, ...) -> seq | return a sequence of tuples flattening the sequences into one
  extract(index, ...) -> seq | return a sequence composed of a subset of the elements of the input sequence
  flatten() -> seq | return a sequence that flatten input tuples into one iterator
  map(f, [index]) -> seq | apply a mapper func for each element of the input, or a specific element at index
  filter(pred) -> seq | apply a filter func for each element of the input
  reduce(f, [index]) -> seq | apply a func of type (v,ac)->ac on each of seq
  append(expr) | append a constant or the result of a function to each tuple
  groupBy(seq, index) -> seq<key, seq>
  orderBy(seq) -> seq

filters:
  <, <=, >, >=, ==, !=, IN, NOT IN,
  matches
  like

reducers:
  avg
  sum
  min
  MAX
  len
  card
  movavg
  med
  stddev

mappers:
  
  quantize(x)
  round
  floor
  ceil
  sqrt
  log(n)
  pow(n)
  
  +, -, *, / 


  lower
  upper
  substr
  format

  day
  month
  year
  hour
  dow

  

  


  

  