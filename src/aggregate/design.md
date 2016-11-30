# Agg - Stream Aggregation Engine 

This aggregation engine, although developed for the redis secondary index module, 
is developed as a completely decoupled project, so it can be embedded in other modules. 
It is not even dependent on Redis itself and can be used outside of it, or tested individually.

## Basic Idea

The idea of the aggregation engine is to manipulate streams of data tuples. These can be 
records stored in keys, time series, hashes, etc. In the context of the secondary index engine,
these are records stored directly inside the index. Each records is a tuple of values, and the cursor 
represents a sequence of tuples. 

We can then use a composition of a set of pre-defined functions and operators to manipulate data 
nd create aggregations from it. 

For example, given a stream of website visits in the form of

    userId, timestamp

We should be able to create a report of number of website visits per hour, in the form of

    hour, count

This is done by quantizing the timestamps by hour, grouping the hits by hour, and then reducing
the groups with a count operation. i.e:

    Select(time, userId).Quantize(time).GroupBy(time).Reduce(Count()).SortBy(hour)

 These operations are composed by a minimal language of function calls and data manipulation operations.


## Language Basics

1. **Sequences and Tuples and Elements, Oh My!**

   The basic idea is creating pipelines for manipulating **sequences** of **tuples** containing **elements**. 

   * A sequence is an iterable generator of tuples, and each transformation, mapping or reduce is applied to an element of it. 
   * Tuples are fixed arrays of one or more values. Map and Reduce functions can be applied to the entire tuple or just an element of it. Tuples are recursive in that they can contain sequences.
   * Elements are values contained within a tuple. They can be referred to with an named identifier if they are named, e.g `@userId` or with an 1-based index number, e.g. `$1`, `$2` and so forth.

2. **Selection**

   Since we are building a pipeline for manipulating a stream of data, the first step is to select elements from the stream (which can be the results of an index WHERE expression for example). 

   We start a query by specifying the selected elements of the stream, either by name or index:

   ```
   select(@name, @age, @height)

   // or

   select($2, $3, $4)
   ```

   > notice that `$2` in this case is the numeric index of the element in the result set, but down stream it will be referred to as $1, respective to its position in the output stream.

3. **Applying Transformations**

   We create a pipeline of **transformations** on the input stream, that are chained to one another. 

   Each transformation step iterates its upstream input sequence, and outputs a downstream output sequence. It may filter the input, change some of its elements, or reshape it into a whole other sequence. 

   Basically transformations can be divided into **Map, Reduce and Filter**, but a few higher level operations such as groupBy, zip, etc exist for ease of use.

   Transformations can be declared by appending a transformation function to the sequence pipeline. e.g.:

   ```scala
   select(@age).filter($1 < 42).reduce(avg)
   ```

   A map or reduce function can also take a mapping operator, denoting that the function should only be applied to a specific element of the tuple:

   ```scala
   select(@name, @age, @height).map(@name -> tolower)
   ```

   ​

4. Map Operations

   A mapping operation is done by calling `map` on the sequence, with either a map function to apply to the entire tuple, or an element mapping. Mapping functions always return at least one output tuple per each input tuple.

5. Reduce Operations

   Reduce operations reduce a sequence using an accumulator, creating an aggregation on the entire sequence, such as the average of numeric values. They return a value that can be a constant or a sequence.

   ​

6. Filtering

   Filtering operations evaluate each tuple in the sequence according to a predicate, and allow only records that match the predicate to pass through.

7. Grouping

   Grouping is the transformation of taking a sequence, and grouping it by one of its elements or a mapping thereof. The result is a sequence of `(K, (sequence))` tuples, where each key has all its associated tuples in the input sequence as an output sequence

When 

## Function Types:

*   ### Transformations

     These take a sequence of tuples, manipulate it to a different stream of tuples, 
     or apply other functions to it. e.g. `groupBy`, `zip`, etc.

    | function (input) -> output               | description                              |
    | ---------------------------------------- | ---------------------------------------- |
    | seq.map([element->] operation) -> seq    | apply an operation to each element of the input sequence, or to a sub element of each tuple |
    | zip(seq, seq, ...) -> seq                | return a sequence of tuples where each element is the next element of each of the input sequences |
    | chain(seq, seq, ...) -> seq              | return a sequence of tuples flattening the sequences into one |
    | seq.flatten() -> seq                     | return a sequence that flatten input tuples into single value tuples |
    | seq.extract(element, ...) -> seq         | return a sequence composed of a subset of the elements of the input sequence |
    | seq.filter(pred, ...) -> seq             | apply a filter func for each element of the input |
    | seq.reduce([element->]reducer) -> seq    | apply a func of type (v,ac)->ac on each of seq, or on each element at index of seq |
    | seq.append(expr)                         | append a constant or the result of a function to each tuple |
    | seq.groupBy(element) -> seq              | extract a sequence of <key, seq> for each element |
    | seq.orderBy(element, ["asc"\|"desc"]) -> seq | order the sequence                       |
    | seq.sample(ratio)                        | filter 1:N elements of the sequence      |
    | seq.range(offset, num)                   | truncate the sequence                    |
    | seq.unique(element | identity_func) |
    | ------------------ | -------------- |
    |                    |                |

*   ### Map Operations

     These take an individual tuple or value, and manipulate it to another tuple or value. 
     e.g. `floor`, `tolower`, `format`, etc.

    * Numeric Operations
        * quantize(amount)
        * round()
        * floor()
        * ceil()
        * sqrt()
        * log(n)
        * pow(n)
        * add(expr)
        * mul(expr)

    * String Operations
        * format(fmt, [element, ...])
        * lower()
        * upper()
        * substr(start, end)

    * Time Operations
        * day()
        * month()
        * year()
        * hour()
        * dow()
        * format(fmt)

*   ### Predicates

     These take a tuple or value, and return TRUE if a condition is met, 0 otherwise. e.g. `<`, `IN`, etc.

    * eq(expr)
    * ne(expr)
    * lt(expr)
    * le(expr)
    * gt(expr)
    * ge(expr)
    * in(expr, expr, ...)
    * notIn(expr, expr, ...)


​    

*   ### Reducers

     These take a tuple or a value, and an accumulator, apply some accumulation on the value, and return the accumulator. 
     An accumulator must be able to yield a sequence of tuples. 

    * seq.histogram(buckets) 
    * seq.countDistinct()
    * seq.avg()
    * seq.count()
    * seq.sum()
    * seq.min()
    * seq.MAX()
    * seq.len()
    * seq.card()
    * seq.movingAvg(num)
    * seq.med()
    * seq.stddev()


## EBNF Aggregation Language Specification

```ebnf
  aggregation := expr | func | pipeline 
  pipeline := func "." func | pipeline.func   
  func := ident "(" arglist ")" | ident 
  arglist := expr { "," expr }
  expr := func | const | ident | element | mapping | predicate | property
  mapping := element "->" func
  const := integer | float | string | "TRUE" | "FALSE"
  index := "$" digit | "@" ident
  predicate := expr comparison expr | func
  comparison := "=" | "!=" | "<" | ">=" | "=" | "BETWEEN" | "IN" | "LIKE" | "MATCH"

```

## A few examples

### Averaging a stream of numeric values

```scala
select(@num).reduce(avg())

// or using moving average on 50 samples

select(@num).movingAvg(50)
```

### Counting users by name

```scala
select(@name).
    reduce(countDistinct())
```

### Transforming a visit log of (userId, timestamp) to hourly unique users

```scala

select(@userId, @timestamp). 
    map(@timestamp -> quantize(3600)).        // quantize the timestamps by hour
    unique( 
        format("{}:{}", @userId, @timestamp) // make the sequence unique by a key
    ).      
    groupBy(@timestamp).                     // -> (timestamp, [(userId,ts), ...])
    reduce($2 -> count).                     // replace element 2 with its count
    orderBy($1)


clientId, joinDate, month, sum

select(clientId, joinDate, month, amount).
groupBy(@month).                                                    
.transform(_1, 
                
        groupBy(@joinDate).                                              
        transform($1 -> reduce(sum)).  // [(age, total)]
        orderBy(_0) 
).orderBy(@month)

```
