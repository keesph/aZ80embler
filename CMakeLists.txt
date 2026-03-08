cmake_minimum_required(VERSION 3.25)

# 1: Setup
project(az80embler
        VERSION 0.1.0
        LANGUAGES C)

# Default to Debug if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# 2: Force C23 standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 4: Add common source files
set(sources
    ${PROJECT_SOURCE_DIR}/source/main.c
    ${PROJECT_SOURCE_DIR}/source/lexer/lexer.c
    ${PROJECT_SOURCE_DIR}/source/lexer/token.c
    ${PROJECT_SOURCE_DIR}/source/lexer/token_list.c
    ${PROJECT_SOURCE_DIR}/source/logging/logging.c
)

set(includes
  ${PROJECT_SOURCE_DIR}/source/include
  ${PROJECT_SOURCE_DIR}/source/
)

add_executable(${PROJECT_NAME} )

target_include_directories(${PROJECT_NAME} PRIVATE ${includes})
target_sources(${PROJECT_NAME} PRIVATE ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 6. Compiler Flags
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        -Werror)
endif()
