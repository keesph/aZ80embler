function(add_unity_test test_name)
  add_executable(${test_name}
    ${CMAKE_CURRENT_SOURCE_DIR}/source/${test_name}.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity.c
  )
  target_include_directories(${test_name} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${PROJECT_SOURCE_DIR}/source/include
    ${PROJECT_SOURCE_DIR}/source/
  )
  target_link_libraries(${test_name} PRIVATE az80sm-lib)
  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_unity_test(test_parser_LD)
add_unity_test(test_parser_PUSH_POP)
add_unity_test(test_parser_EX_LD_CP)
add_unity_test(test_parser_arithmetic)