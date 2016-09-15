

add_library(libsecondary STATIC 
            tree.c 
            key.c 
            value.c 
            changeset.c
            query.c
            cursor.c
            compound_index.c
            query_parse.c
            query_plan.c
            query_normalize.c
            parser/ast.c
            parser/parser.c
            parser/lex.yy.c

            rmutil/vector.c
            skiplist/skiplist.c            
        )
target_compile_options(libsecondary PUBLIC "-fPIC")

# build the parser source code before building libsecondary
# TODO: remove this
add_custom_command(
                TARGET libsecondary
                PRE_BUILD
                COMMAND lex -i tokenizer.l
                COMMAND lemon -s parser.y 
                WORKING_DIRECTORY ../../src/parser)


add_library(module MODULE
    index_type.c
    module.c
)
target_link_libraries(module libsecondary)