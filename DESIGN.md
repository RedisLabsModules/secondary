# Redis Secondary Index Design

## Objective

This project aims to automate and ease the creation of Secondary Indexes in Redis, 
allowing users to index their data and query the indexed data efficiently.

Since there are many ways of representing data in redis, the architecture of 
secondary indexing should be as flexible and abstract as possible.

At the heart of of it, a secondary should be able to answer the question "what entities map a given set of criteria?". 
To do that, it needs to map values to "ids", which usually represent redis keys. Values are arbitrary tuples 
of primitive types that are contained in an object with a given id. The index keeps tabs of what values are
related to what ids, and respond to queries.

## Architecture

To be really flexible and re-usable, we should create a multi-tiered approach:

0. Data Structures

    The lowest level is implementation of custom data structures that support indexing and querying of data,
    be it in trees, sorted sets, hash tables, etc. Each has its own API, but the index implementations might 
    choose between data structures according to use case, or replace them between versions of the module.
        
1. Low Level Indexes

    This level deals does the processing of indexing, updating records in indexes and searching over the data structures - 
    but it oblivious to things like schema, entities, and such. 
    These indexes are stateful, but only to a degree in which they know how to encode a tuple of values into its internal representation,
    and how to process search predicates. Such an index may know that it indexes a "string,string,integer" - and treat incomving value tuples
    as such. But it doesn't know that it actually indexes "first name,last name,age".

    Their API in high level terms:

    ```
    // create the index and save its state
    Create(Type *types, int numTypes);

    // apply a list of changes - ADD / DELETE. A change is the operation and a value tuple.
    Apply(Change *changes, int numChanges);

    // Find ids based on the given predicates
    Result *Find(Predicate *predicates, int numPredicates);
    ``` 

2. High level indexes

    These are module specific, and are used to manage mapping between different types of objects in redis to the underlying index representation.
    For example, one such index might know how to treat HASH objects as entities, another might know how to index JSON objects, etc. 

    They are also responsible for tracking the changes needed when udpating the lower level index. i.e. if we have an index
    on the user's name and email, and just the name changes, this level needs to know that we need to delete the old
    values from the low level index and add the high level values.

3. Querying / API

    The highest level, differs per module. It can be an SQL-like, table-like interface (similar to CQL), or indexing JSON objects.

## Data structures

We will start with a very simple, sorted set based POC. 

TBD on next data structures

