#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"
#include "error.h"
#include "lvec.h"
#include "parser.h"
#include "tokenizer.h"
#include "semantic.h"
#include "whereami.h"

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

    SemanticContext semantic_context;
    semantic_context_initialize( &semantic_context );
    bool is_valid = check_semantics( &semantic_context, program );
    if( !is_valid )
    {
        return 1;
    }

    /* char file_name[256]; */
    /* sprintf( file_name, "%s.c", g_source_code.path ); */
    /* FILE* generated_c = fopen( file_name, "w+" ); */
    /* generate_code( generated_c, &semantic_context, program ); */
    /* fclose( generated_c ); */

    /* for( int i = 0; i < semantic_context.symbol_table.length; i++ ) */
    /* { */
    /*     Symbol symbol = semantic_context.symbol_table.symbols[ i ]; */
    /*     printf( "%s\n", symbol.token.as_string ); */
    /* } */

    expression_print( program );

    /* int octo_exe_path_length = wai_getExecutablePath( NULL, 0, NULL ); */
    /* char* octo_exe_dir = calloc( 1, octo_exe_path_length + 1 ); */
    /* wai_getExecutablePath( octo_exe_dir, octo_exe_path_length, NULL ); */

    /* // get only the directory */
    /* for( int i = octo_exe_path_length; i >= 0; i-- ) */
    /* { */
    /*     char* c = &octo_exe_dir[ i ]; */
    /*     if( *c == '\\' || *c == '/' ) */
    /*     { */
    /*         *c = 0; */
    /*         break; */
    /*     } */
    /* } */

    /* char command[1024]; */
    /* sprintf( command, "gcc %s -I%s/.. -o main.exe -std=gnu99 -Wall -Wextra", */
    /*          file_name, */
    /*          octo_exe_dir, */
    /*          file_name ); */

    /* // temporarily use this to test */
    /* system( command ); */
    /* // system("del generated.c"); */
}
