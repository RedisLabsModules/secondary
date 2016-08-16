
add_executable(test_index test.c)
target_link_libraries(test_index libsecondary)
add_test(test_index test_index)
