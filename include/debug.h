#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdlib.h>
// #include "parser.h"

#define UNIMPLEMENTED()    ( fprintf(stderr, "Unimplemented functionality at %s:%d\n", __FILE__, __LINE__), abort() )
#define UNREACHABLE()      ( fprintf(stderr, "Unreachable code reached at %s:%d\n", __FILE__, __LINE__), abort() )
#define ALLOC_ERROR()      ( fprintf(stderr, "Memory allocation error at %s:%d\n", __FILE__, __LINE__), abort() )

extern char* token_kind_to_string[];
extern char* binary_operation_to_string[];
extern char* expression_kind_to_string[];
extern char* type_kind_to_string[];

#endif
