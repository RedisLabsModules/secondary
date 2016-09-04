
add_library(libbtree STATIC 
            btree.c 
            print_tree.c 
        )
target_compile_options(libsecondary PUBLIC "-fPIC")
