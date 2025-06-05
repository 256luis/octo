#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"
#include "symboltable.h"

typedef struct SemanticContext
{
    SymbolTable symbol_table;
    Type* return_type_stack;
} SemanticContext;

void semantic_context_initialize( SemanticContext* context );
bool check_semantics( SemanticContext* context, Expression* expression );

#endif
