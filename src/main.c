#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "error.h"
#include "parser.h"
#include "tokenizer.h"
#include "debug.h"

int main( int argc, char* argv[] )
{
    if( argc < 2 )
    {
        printf( "No file specified.\n" );
        return -1;
    }

    char* source_file_path = argv[ 1 ];
    SourceCode source_code = source_code_load( source_file_path );

    Tokenizer* tokenizer = tokenizer_new( source_code );
    if( tokenizer == NULL )
    {
        ALLOC_ERROR();
    }

    TokenList tokens = tokenizer_tokenize( tokenizer );

    if( tokenizer->error_found ) return -1;
    // tokenizer_free( tokenizer );

    // iterate over list
    printf( "====== TOKENS FOUND ======\n" );
    for( size_t i = 0; i < tokens.list.length; i++ )
    {
        Token token = token_list_get( tokens, i );
        printf("%s", token_kind_to_string[ token.kind ]);
        switch( token.kind )
        {
            case TOKENKIND_NUMBER:
            {
                printf( "(%d)", token.number );
                break;
            }

            case TOKENKIND_IDENTIFIER:
            {
                printf( "(%s)", token.identifier );
                break;
            }

            case TOKENKIND_STRING:
            {
                printf( "(\"%s\")", token.string );
                break;
            }

            case TOKENKIND_CHARACTER:
            {
                printf( "(\'%c\')", token.character );
                break;
            }

            default:
            {
                // do nothing
                break;
            }
        }

        putchar( '\n' );
    }

    Parser* parser = parser_new( tokens, source_code );
    Expression* program = parser_parse( parser );

    putchar( '\n' );
    printf( "====== SYNTAX TREE ======\n" );
    if( program != NULL )
    {
        expression_print( program );
    }

}
