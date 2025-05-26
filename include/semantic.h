#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

typedef struct Symbol
{
    Token token;
    Type type;
} Symbol;

typedef struct SymbolTable
{
    Symbol* symbols;
    int length;
} SymbolTable;

typedef struct SemanticContext
{
    SymbolTable* symbol_table_stack;
    Type* return_type_stack;
    Type* pointer_types;
    Type* array_types;
} SemanticContext;

void semantic_context_initialize( SemanticContext* context );
bool check_semantics( SemanticContext* context, Expression* expression );

#endif
