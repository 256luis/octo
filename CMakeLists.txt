cmake_minimum_required(VERSION 3.21)
project(octo)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

if (MSVC)
    set(COMPILE_OPTIONS
        /Wall
        /wd4820
        /wd4456
        /wd5045
        /wd4061
        /D_CRT_SECURE_NO_WARNINGS)
else()
    set(COMPILE_OPTIONS
        -Wall
        -Wextra
        -Wpedantic
        -Wno-deprecated-declarations)
endif()

include(CheckCSourceCompiles)
check_c_source_compiles(
    "int main() { __typeof__(int) a = 10; }"
    HAS_TYPEOF
)

if(NOT HAS_TYPEOF)
    message(FATAL_ERROR "Compiler does not support `typeof`.")
endif()
unset(HAS_TYPEOF CACHE)

add_subdirectory(lvec.c)
add_executable(${PROJECT_NAME}
               ${CMAKE_CURRENT_LIST_DIR}/src/main.c
               ${CMAKE_CURRENT_LIST_DIR}/src/debug.c
               ${CMAKE_CURRENT_LIST_DIR}/src/parser.c
               ${CMAKE_CURRENT_LIST_DIR}/src/tokenizer.c
               ${CMAKE_CURRENT_LIST_DIR}/src/error.c
               ${CMAKE_CURRENT_LIST_DIR}/src/semantic.c
               ${CMAKE_CURRENT_LIST_DIR}/src/codegen.c
               ${CMAKE_CURRENT_LIST_DIR}/src/symboltable.c

               ${CMAKE_CURRENT_LIST_DIR}/include/debug.h
               ${CMAKE_CURRENT_LIST_DIR}/include/parser.h
               ${CMAKE_CURRENT_LIST_DIR}/include/tokenizer.h
               ${CMAKE_CURRENT_LIST_DIR}/include/error.h
               ${CMAKE_CURRENT_LIST_DIR}/include/semantic.h
               ${CMAKE_CURRENT_LIST_DIR}/include/codegen.h
               ${CMAKE_CURRENT_LIST_DIR}/include/symboltable.h)


target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${PROJECT_NAME} PUBLIC lvec)
target_compile_options(${PROJECT_NAME} PRIVATE ${COMPILE_OPTIONS})
