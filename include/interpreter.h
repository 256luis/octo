#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "symbol.h"

typedef struct InterpreterContext
{
    SymbolTable* st;
} InterpreterContext;

void interpret( AstNode* ast, SymbolTable* st );
AstNode walk_node( AstNode* node, InterpreterContext* ctx );

#endif
