@echo off

clang -g src/*.c -o main.exe -Iinclude -Wall -Wextra -Wpedantic -Wno-strict-prototypes -Wno-deprecated-declarations -std=c17
