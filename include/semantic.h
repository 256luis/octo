#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

typedef struct SymbolTable
{
    char** identifiers;
    Type* types;
    void* values; // idk if im doing this right

    int symbol_count;
} SymbolTable;

typedef struct SemanticContext
{
    SourceCode source_code;
    SymbolTable symbol_table;
} SemanticContext;

void semantic_analyze( SemanticContext context, Expression* program );

#endif
