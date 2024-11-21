cmake_minimum_required(VERSION 3.27)
project(skard C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(skard src/main.c)
target_compile_options(skard PRIVATE -Wall -Wextra -Wpedantic -Werror)
