
  add_executable(test_main
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_arithmetic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_BIT.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_call_return.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_EX_LD_CP.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_gparithmetic_cpucontrol.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_input_output.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_jump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_LD.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_PUSH_POP.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/test_parser_rotate_shift.c
    ${CMAKE_CURRENT_SOURCE_DIR}/unity/unity.c
  )
  target_include_directories(test_main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/unity
    ${PROJECT_SOURCE_DIR}/source/include
    ${PROJECT_SOURCE_DIR}/source/
    ${PROJECT_SOURCE_DIR}/tests/source
  )
  target_link_libraries(test_main PRIVATE az80sm-lib)
  add_test(NAME test_main COMMAND test_main)

