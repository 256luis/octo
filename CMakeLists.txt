cmake_minimum_required(VERSION 3.21)
project(octo)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

include(CheckCSourceCompiles)
check_c_source_compiles(
    "int main() { typeof(int) a = 10; }"
    HAS_TYPEOF
)

if(NOT HAS_TYPEOF)
    message(FATAL_ERROR "Compiler does not support `typeof`.")
endif()
unset(HAS_TYPEOF CACHE)

add_subdirectory(lvec.c)
add_executable(${PROJECT_NAME}
               src/main.c
               src/array_list.c
               src/debug.c
               src/parser.c
               src/tokenizer.c
               src/error.c

               include/array_list.h
               include/debug.h
               include/parser.h
               include/tokenizer.h
               include/error.h)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} PUBLIC lvec)
target_compile_options(${PROJECT_NAME} PRIVATE
                       -Wall
                       -Wextra
                       -Wpedantic
                       -Wno-deprecated-declarations)
