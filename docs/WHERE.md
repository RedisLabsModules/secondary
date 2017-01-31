# WHERE Clause Syntax

The WHERE clause query language is a subset of standard SQL, with the currently supported predicates:	

```sql
<, <=, >, >=, IN, LIKE, IS NULL
```

Predicates can be combined using `AND` and `OR` operations, grouped by `(` and `)` symbols. For example:

```sql
(foo = 'bar' AND baz LIKE 'boo%') OR (wat <= 1337 and word IN ('hello', 'world'))
```

### Named vs indexed properties:

Since they may be anonymous, the index properties or columns are represented by either a name (in a HASH index), or a dollar sign and their numeric order in the index's schema.
i.e. $1, $2, $3 etc. Of course, they must be within the range of the index's schema length.  


### pseudo BNF query syntax:

```
    <query> ::= <predicate> | <predicate> "AND" <predicate> ... 
    <predicate> ::= <property> <operator> <value>
    <property> ::= "$" <digit> | <identifier>
    <operator> ::= "=" | "!=" | ">" | "<" | ">=" | "<=" | "IN" | "LIKE" | "IS"
    <value> ::= <number> | <string> | "TRUE" | "FALSE" | <list> | "NULL" 
    <list> ::= "(" <value>, ... ")"
```



### Time Functions

For time typed index properties, we support a few convenience functions for WHERE expressions (note that they can ONLY be used in WHERE expressions and not passed to the redis commands):

| Function                          | Meaning                                  |
| --------------------------------- | ---------------------------------------- |
| NOW                               | The current Unix timestamp               |
| TODAY                             | Unix timestamp of today's midnight UTC   |
| TIME_ADD({timetsamp}, {duration}) | Add a duration (minutes, hours etc) to a stimestamp. i.e. `TIME_ADD(NOW, HOURS(5))` |
| TIME_SUB({timestamp}, {duration}) | Subtract a duration (minutes, hours etc) from a stimestamp. |
| UNIX({int})      | Convert an integer to a Timestamp type representing a Unix timestamp.  |
| HOURS(N)                          | Return the number of seconds in N hours  |
| DAYS(N)                           | Return the number of seconds in N days   |
| MINUTES(N)                        | Return the number of seconds in N minutes |

​	

#### Example usage of time functions in WHERE:

```sql
 # Selecting users with join_time in the last 30 days
 WHERE "join_time >= TIME_SUB(TODAY, DAYS(30))"

 # Selecting blog posts with pub_time in the past 4 hours
 WHERE "pub_time >= TIME_SUB(NOW, HOURS(4))"
```



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



