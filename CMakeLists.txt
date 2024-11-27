cmake_minimum_required(VERSION 3.27)
project(skard C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(skard
        src/main.c
        src/skard.h
        src/sk_utils.c
        src/sk_utils.h
        src/sk_vm.c
        src/sk_vm.h
        src/sk_debug.c
        src/sk_debug.h
        src/sk_value.c
        src/sk_value.h
        src/sk_lexer.c
        src/sk_lexer.h)
target_compile_options(skard PRIVATE -Wall -Wextra -Wpedantic -Werror)
