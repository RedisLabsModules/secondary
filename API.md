# Proposed Secondary Index External API

The following document is a proposal for **three** different APIs that can take advantage of Secondary Indexing in Redis.

The three APIs described are:

1. Explicit Anonymous Indexes

    providing raw indexes as a primitive data type in Redis.

2. Indexed Redis Objects

    Adding secondary indexes to common redis data structures, and a way to query them.

3. Aggregations API

    Building on indexed objects, provide a way to run various aggregations o query results.

---         

## 1. Explicit Anonymous Indexes

### Purpose: 

* Use as an index for external databases
* Index existing data already inside redis, independent of how this data is modeled.
* Facilitate writing ORMs and database engines on top of redis

### Basic Concepts:
1. Indexes map n-tuples of values to ids only (e.g. (string, string, int)).
2. Indexes are created before they are updated, with a given schema (as above)
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

        #### pseudo BNF query syntax:


                <query> ::= <predicate> | <predicate> "AND" <predicate> ... 

                <predicate> ::= <property> <operator> <value>

                <property> ::= "$" <digit>
                
                <operator> ::= "=" | "!=" | ">" | "<" | ">=" | "<=" | "BETWEEN" | "IN" | "LIKE"

                <value> ::= <number> | <string> | "TRUE" | "FALSE" | <list>

                <list> ::= "(" <value>, ... ")"

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

    i.e. in the case of a HASH, the indexing module implements HSET/HMSET/HINCRBY so it knows which key is updated automatically.

* The index also implements the GET family of functions for the type, using WHERE clauses.

    Proposed API:

    * Creating an index:

            IDX.CREATE [OPTIONS [UNIQUE]] TYPE [HASH|STR|ZSET] [SCHEMA <field> <type> ...]

        **Note:** more options TBD

    * Writing syntax - HASH values:

            HSET <key> <element> <value>

            IDX.HSET <element> <value> USING <index_name> [<index_name> ...] WHERE <predicates>

            IDX.HMSET <num_pairs> <element> <value> ... USING <index_name> [<index_name> ...] WHERE <predicates>

            IDX.HDEL <index_name> <key> USING <index_name> [<index_name> ...] WHERE <predicates>

            IDX.HINCRBY <element> <amount> USING <index_name> [<index_name> ...] WHERE <predicates>

    * Reading - HASH values:

            IDX.HGETALL USING <index_name>  WHERE <predicates> [LIMIT <offset> <num>] 
            
        returns an array of interleaved key, values, where values is a nested array of all the hash values.
            
            IDX.HMGET USING <index_name> <num_elements> <elem> <elem> ... WHERE <predicates> [LIMIT offset num]
            
        same as HGETALL but gets only some of the elements, and does not return the element names, just the values.
            
            IDX.HGET USING <index_name> <elem> WHERE <predicates> [LIMIT offset num]
            
        same as HGETALL but gets only one element's value for each matching HASH.

    * Writing - strings:

            IDX.SET USING <index_name> <key> <value> <key> <value> ...
            IDX.SETNX USING <index_name> <key> <value> <key> <value> ...
            IDX.SETEX USING <index_name> <key> <value> <key> <value> ...
            IDX.INCRBY USING <index_name> <key> <value> <key> <value> ...
            IDX.DEL USING <index_name> WHERE <predicates> [LIMIT offset num]

    * Reading - strings:
            
            IDX.GET USING <index_name> WHERE <predicates> [LIMIT offset num]

        > ### TBD: how do we handle expires? 
        > 
        > for now we simply assume no expiration support, until redis has keyspace notifications for modules. 

## 3. Aggregations:

The above indexing scheme also allows us to do aggregations on HASH properties or complete string values. This is useful for analytics. 

The idea is to do an indexed scan, and for each matching redis object, feed the existing values (be them hash element values, complete values of strings, values and scores of sorted sets) into an aggregation function. This may produce a single number, a list of values, etc.

* Proposed Aggregation Syntax: 

        IDX.AGGREGATE USING <index_name> <aggregation_func> ... WHERE <predicates> 
    
* Aggregation Function Grammar:

            <property> ::= <identifier> | "_"

            <aggregation> ::= <agg_func> <arglist>

            <agg_func> ::= "SUM" | "COUNT" | "HISTOGRAM" | "AVG" | "MEAN" | "MAX" | "NLARGEST" | "NSMALLEST" 

            // "_" is used to refer to "self value" when aggregating indexes on whole redis string keys
            <arglist> ::= "(" <ident> | "_" | <transformation> { <param> "," ... } ")"

            <param> ::= <number> | <string>  

            <transformation> ::= <trans_func> <arglist>

            <trans_func> ::= "ROUND" | "FLOOR" | "CEIL" | ...

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
    > | NLARGEST(property) || 
    > | NSMALLEST(property) || 
        
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
        
    * String:
        * LOWER(x)
        * UPPER(x)
        * REPLACE(x, y)

    * Time:
        * DAY(x)
        * MONTH(x)
        * YEAR(x)
        * WEEK(x)
        * DOM(x)
         
  
