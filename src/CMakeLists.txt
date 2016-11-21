
# The file list for building the secondary index library.
# They can be used directly for unit tests.
# the ../src part is so that they can be accessed from outside the src tree
set(_secondary_files 
            ../src/key.c 
            ../src/value.c 
            ../src/changeset.c
            ../src/query.c
            ../src/cursor.c
            ../src/spec.c
            ../src/index.c
            ../src/reverse_index.c
            ../src/query_parse.c
            ../src/query_plan.c
            ../src/query_normalize.c
            ../src/parser/ast.c
            ../src/parser/parser.c
            ../src/parser/lex.yy.c
            

            ../src/rmutil/vector.c
            ../src/rmutil/alloc.c
            ../src/skiplist/skiplist.c

            ../src/aggregate/aggregate.c
            ../src/aggregate/functions.c
            ../src/aggregate/pipeline.c
            ../src/aggregate/property_getter.c
            ../src/aggregate/parser/aggregation.c
            ../src/aggregate/parser/lex.yyagg.c
            ../src/aggregate/parser/ast.c


            
            )


set (secondary_files ${_secondary_files} PARENT_SCOPE)            

# Libsecondary target - a static library for secondary indexing that can be linked in any module
add_library(libsecondary STATIC 
                ${_secondary_files}      
        )
target_compile_options(libsecondary PUBLIC "-fPIC" "-DREDIS_MODULE_TARGET" "-I${CMAKE_CURRENT_LIST_DIR}")

# build the parser source code before building libsecondary
# TODO: remove this
add_custom_command(
                TARGET libsecondary
                PRE_BUILD
                COMMAND lex -i tokenizer.l
                COMMAND lemon -s parser.y 
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/parser)


add_library(module MODULE
    index_type.c
    hash_index.c
    module.c
    rmutil/util.c
    rmutil/strings.c

)


add_library(librmutil STATIC IMPORTED ../../src/rmutil/librmutil.a)

target_compile_options(module PUBLIC "-DREDIS_MODULE_TARGET")

target_link_libraries(module libsecondary)
