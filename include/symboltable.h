#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "tokenizer.h"
#include "type.h"

typedef struct Symbol
{
    Token token;
    Type type;
} Symbol;

typedef struct SymbolTable
{
    Symbol* symbols;
    int length;
    int* scope_index_stack;
} SymbolTable;

void symbol_table_initialize( SymbolTable* table );
Symbol* symbol_table_lookup( SymbolTable table, char* identifier );
void symbol_table_push_symbol( SymbolTable* table, Symbol symbol );
void symbol_table_push_scope( SymbolTable* table );
void symbol_table_pop_scope( SymbolTable* table );

#endif
