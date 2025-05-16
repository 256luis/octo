#include <stdint.h>
#include <stdio.h>
#include "debug.h"
#include "error.h"
#include "lvec.h"
#include "parser.h"
#include "tokenizer.h"
#include "semantic.h"

SourceCode g_source_code;

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

    Expression* program = parse( tokens );
    if( program == NULL )
    {
        return 1;
    }

    bool is_valid = check_semantics( program );
    if( !is_valid )
    {
        return 1;
    }

    /* for( size_t i = 0; i < lvec_get_length( program->associated_tokens ); i++ ) */
    /* { */
    /*     printf("%s\n", token_kind_to_string[ program->associated_tokens[ i ].kind ]); */
    /* } */

    // expression_print( program );
}
