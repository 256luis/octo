#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdint.h>
#include "ast.h"
#include "symbol.h"

typedef struct InterpreterContext
{
    SymbolTable* st;
    bool is_return;
} InterpreterContext;

typedef struct RuntimeValue
{
    Type type;
    union
    {
        int64_t integer;
        double floating;
        bool boolean;
        char character;
        char* string;
        struct RuntimeValue* array;
        SymbolTable structure_st;
        AstNodeRoutineDefinition routine_definition;
        void* pointer;
    };
} RuntimeValue;

void interpret( AstNode* ast, SymbolTable* st );
RuntimeValue walk_node( AstNode* node, InterpreterContext* ctx );

#endif
