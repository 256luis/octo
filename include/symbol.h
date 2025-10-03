#ifndef SYMBOL_H
#define SYMBOL_H

#include "tokenizer.h"
#include "type.h"

typedef struct RuntimeValue RuntimeValue;
typedef struct Symbol
{
    Token key;
    Type type;
    RuntimeValue* value;
} Symbol;

typedef struct SymbolTable
{
    Symbol* symbols;    // lvec
    int* symbol_depths; // lvec
    int current_depth;
} SymbolTable;

void st_initialize( SymbolTable* st );
void st_push_scope( SymbolTable* st );
void st_pop_scope( SymbolTable* st );
void st_insert( SymbolTable* st, Symbol symbol );
Symbol* st_get( SymbolTable st, char* key );
void st_print( SymbolTable st );

#endif
