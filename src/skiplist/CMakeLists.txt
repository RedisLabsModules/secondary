
add_library(libskiplist STATIC 
            skiplist.c 
        )
target_compile_options(libskiplist PUBLIC "-fPIC")

add_executable("skiplist" skiplist.c main.c)
