#include <string.h>
#include "symboltable.h"
#include "lvec.h"

void symbol_table_initialize( SymbolTable* table )
{
    table->symbols = lvec_new( Symbol );
    table->scope_index_stack = lvec_new( int );
    table->length = 0;
}

Symbol* symbol_table_lookup( SymbolTable table, char* identifier )
{
    for( int i = 0; i < table.length; i++ )
    {
        Symbol* symbol = &table.symbols[ i ];
        if( strcmp( symbol->token.as_string, identifier ) == 0 )
        {
            return symbol;
        }
    }

    return NULL;
}

void symbol_table_push_symbol( SymbolTable* table, Symbol symbol )
{
    lvec_append_aggregate( table->symbols, symbol );
    table->length++;
}

void symbol_table_push_scope( SymbolTable* table )
{
    lvec_append( table->scope_index_stack, table->length );
}

void symbol_table_pop_scope( SymbolTable* table )
{
    int scope_index_stack_top_index = lvec_get_length( table->scope_index_stack ) - 1;
    int scope_index_stack_top = table->scope_index_stack[ scope_index_stack_top_index ];
    while( table->length >= scope_index_stack_top )
    {
        lvec_remove_last( table->symbols );
        table->length--;
    }
}
