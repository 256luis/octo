#include <stddef.h>
#include <string.h>
#include "lvec.h"
#include "symbol.h"

void st_initialize( SymbolTable* st )
{
    st->symbols = lvec_new( Symbol );
    st->symbol_depths = lvec_new( int );
    st->current_depth = 0;
}

void st_push_scope( SymbolTable* st )
{
    st->current_depth++;
}

void st_pop_scope( SymbolTable* st )
{
    size_t length = lvec_get_length( st->symbols );
    for( size_t i = 0; i < length; i++ )
    {
        int symbol_depth = st->symbol_depths[ i ];
        if( symbol_depth == st->current_depth )
        {
            lvec_remove( st->symbols, i );
        }
    }

    st->current_depth--;
}

void st_insert( SymbolTable* st, Symbol symbol )
{
    lvec_append_aggregate( st->symbols, symbol );
    lvec_append( st->symbol_depths, st->current_depth );
}

Symbol* st_get( SymbolTable st, char* key )
{
    size_t length = lvec_get_length( st.symbols );
    for( size_t i = 0; i < length; i++ )
    {
        char* symbol_key = st.symbols[ i ].key.as_string;
        if( strcmp( symbol_key, key ) == 0 )
        {
            return &st.symbols[ i ];
        }
    }

    return NULL;
}
