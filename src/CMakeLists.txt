
add_library(libsecondary STATIC 
            tree.c 
            key.c 
            value.c 
            changeset.c
            query.c
            cursor.c
            compound_index.c
        )
target_compile_options(libsecondary PUBLIC "-fPIC")


add_library(module MODULE
    index_type.c
    module.c
)
target_link_libraries(module libsecondary)