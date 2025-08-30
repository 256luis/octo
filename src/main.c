#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "error.h"
#include "tokenizer.h"
#include "ast.h"
#include "semantic.h"

SourceCode g_source_code;

void* octo_malloc( size_t size )
{
    void* ptr = malloc( size );
    if( ptr == NULL )
    {
        ALLOC_ERROR();
    }

    return ptr;
}

void* octo_calloc( size_t size )
{
    void* ptr = calloc( 1, size );
    if( ptr == NULL )
    {
        ALLOC_ERROR();
    }

    return ptr;
}

int main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        printf( "No file specified.\n" );
        return -1;
    }

    char* source_file_path = argv[ 1 ];
    g_source_code = source_code_load( source_file_path );

    Token* tokens = tokenize();
    if( tokens == NULL )
    {
        return 1;
    }

    AstNode* ast = ast_from_tokens( tokens );
    if( ast == NULL )
    {
        return 1;
    }

    /* if( !check_ast( ast ) ) */
    /* { */
    /*     // printf( "here\n" ); */
    /*     return 1; */
    /* } */

    ast_node_print( *ast );
}
