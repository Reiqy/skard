cmake_minimum_required(VERSION 3.24)
project(skard C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(skard
        src/main.c
        src/skard.h
        src/sk_memory.c
        src/sk_memory.h
        src/sk_vm.c
        src/sk_vm.h
        src/sk_debug.c
        src/sk_debug.h
        src/sk_value.c
        src/sk_value.h
        src/sk_lexer.c
        src/sk_lexer.h
        src/sk_parser.c
        src/sk_parser.h
        src/sk_compiler.c
        src/sk_compiler.h
        src/sk_ast.c
        src/sk_ast.h
        src/sk_object.c
        src/sk_object.h
        src/sk_hashmap.c
        src/sk_hashmap.h)
target_compile_options(skard PRIVATE -Wall -Wextra -Wpedantic -Werror)

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE SKARD_FORMAT_FILES CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/src/*.c"
            "${CMAKE_SOURCE_DIR}/src/*.h")

    add_custom_target(format
            COMMAND ${CLANG_FORMAT} -i ${SKARD_FORMAT_FILES}
            COMMENT "Formatting C sources")

    add_custom_target(format-check
            COMMAND ${CLANG_FORMAT} --dry-run --Werror ${SKARD_FORMAT_FILES}
            COMMENT "Checking C source formatting")
else()
    message(STATUS "clang-format not found; format targets will not be available")
endif()
