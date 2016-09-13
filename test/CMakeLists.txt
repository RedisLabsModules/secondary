
add_executable(test_index test.c)
#dd_executable(test_btree test_btree.c)
target_link_libraries(test_index libsecondary)
#target_link_libraries(test_btree libbtree)
add_test(test_index test_index)
#add_test(test_btree test_btree)

add_executable(test_query test_query.c)
target_link_libraries(test_query libsecondary)

add_test(test_query test_query)
