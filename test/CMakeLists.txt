
add_executable(test_index test.c ${secondary_files})
add_test(test_index test_index)

add_executable(test_query test_query.c ${secondary_files})
add_test(test_query test_query)

add_executable(test_value test_value.c ${secondary_files})
add_test(test_value test_value)
