cmake_minimum_required(VERSION 3.25)

# 1: Setup
project(az80embler
        VERSION 0.1.0
        LANGUAGES C)

# Default to Debug if no build type is specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Force C23 standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Coverage build type flags
set(CMAKE_C_FLAGS_COVERAGE
    "-O0 -g -fprofile-instr-generate -fcoverage-mapping"
    CACHE STRING "Flags used for coverage builds" FORCE)

set(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    "-fprofile-instr-generate"
    CACHE STRING "Linker flags for coverage builds" FORCE)

# Register it as a valid build type
mark_as_advanced(
    CMAKE_C_FLAGS_COVERAGE
    CMAKE_EXE_LINKER_FLAGS_COVERAGE)

set(CMAKE_CONFIGURATION_TYPES
    "Debug;Release;RelWithDebInfo;MinSizeRel;Coverage"
    CACHE STRING "Available build types" FORCE)

# Add source files
set(sources
    ${PROJECT_SOURCE_DIR}/source/lexer/lexer.c
    ${PROJECT_SOURCE_DIR}/source/lexer/token.c
    ${PROJECT_SOURCE_DIR}/source/logging/logging.c
    ${PROJECT_SOURCE_DIR}/source/parser/parser.c
    ${PROJECT_SOURCE_DIR}/source/parser/directive.c
    ${PROJECT_SOURCE_DIR}/source/parser/instruction.c
    ${PROJECT_SOURCE_DIR}/source/parser/operand.c
    ${PROJECT_SOURCE_DIR}/source/utility/linked_list.c
    ${PROJECT_SOURCE_DIR}/source/utility/argument_parser.c
    ${PROJECT_SOURCE_DIR}/source/utility/alloc_w.c
    ${PROJECT_SOURCE_DIR}/source/assembler/assembler.c
)

set(includes
  ${PROJECT_SOURCE_DIR}/source/include
  ${PROJECT_SOURCE_DIR}/source/
)

# Lib containing the assembler. Reused in test application and for building the executable
add_library(az80sm-lib ${sources})
target_include_directories(az80sm-lib PUBLIC ${includes})
set_target_properties(az80sm-lib PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# The final executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCE_DIR}/source/main.c)
target_link_libraries(${PROJECT_NAME} az80sm-lib)

#target_include_directories(${PROJECT_NAME} PRIVATE ${includes})
#target_sources(${PROJECT_NAME} PRIVATE ${sources})

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        -Werror)
        target_compile_options(az80sm-lib PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        -Werror)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    add_compile_options(--coverage)
    add_link_options(--coverage)
endif()

enable_testing()
add_subdirectory(tests)