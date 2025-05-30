#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"
#include "error.h"
#include "lvec.h"
#include "parser.h"
#include "symboltable.h"
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

    Parser parser;
    parser_initialize( &parser, tokens );

    Expression* program = parse( &parser );
    if( program == NULL )
    {
        return 1;
    }
    lvec_free( tokens );

    /* expression_print( program ); */

    SemanticContext semantic_context;
    semantic_context_initialize( &semantic_context );
    bool is_valid = check_semantics( &semantic_context, program );
    if( !is_valid )
    {
        return 1;
    }

    FILE* generated_c = fopen( "generated.c", "w+" );
    generate_code( generated_c, &semantic_context, program );
    fclose( generated_c );

    // temporarily use this to test
    system("gcc generated.c -std=gnu99");
    // system("del generated.c");
}
