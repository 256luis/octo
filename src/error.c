#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "globals.h"
#include "debug.h"
#include "lvec.h"
#include "tokenizer.h"

static char* file_to_string( const char* filename, int* length )
{
    FILE* file = fopen( filename, "rb" );
    if( file == NULL )
    {
        return NULL;
    }

    // Seek to the end to determine file size
    if( fseek( file, 0, SEEK_END ) != 0 )
    {
        fclose( file );
        return NULL;
    }

    long file_size = ftell( file );
    if( file_size == -1 )
    {
        fclose( file );
        return NULL;
    }

    // Allocate buffer with space for null terminator
    char* buffer = malloc( file_size + 1 );
    if( buffer == NULL )
    {
        fclose( file );
        return NULL;
    }

    // Read the entire file
    rewind( file );

    int char_count = 0;
    for( int  c = fgetc( file ); c != EOF; c = fgetc( file ) )
    {
        buffer[ char_count ] = c;
        char_count++;
    }

    // Null-terminate the string
    buffer[ char_count ] = '\0';
    *length = char_count;

    fclose( file );
    return buffer;
}

SourceCode source_code_load( char* path )
{
    SourceCode source_code;
    source_code.code = file_to_string( path, &source_code.length );
    source_code.path = malloc( strlen( path ) + 1 );
    strcpy( source_code.path, path );

    int line_count = 1;

    // get newline count
    for( int i = 0; source_code.code[ i ] != '\0'; i++ )
    {
        char c = source_code.code[ i ];

        if( c == '\n' )
        {
            line_count++;
        }
    }

    source_code.line_indexes = malloc( sizeof( int* ) * line_count );
    source_code.line_indexes[0] = 0;

    // get newline count
    for( int i = 0, j = 1; source_code.code[ i ] != '\0'; i++ )
    {
        char c = source_code.code[ i ];

        if( c == '\n' )
        {
            source_code.line_indexes[ j ] = i;
            j++;
        }
    }

    return source_code;
}

void source_code_print_line( SourceCode source_code, int line )
{
    int line_start_index = source_code.line_indexes[ line - 1 ];
    if( line_start_index > 0 )
    {
        line_start_index++;
    }

    printf( "%5d | ", line );
    for( int i = line_start_index; source_code.code[ i ] != '\n' && source_code.code[ i ] != '\0'; i++ )
    {
        printf( "%c", source_code.code[ i ] );
    }
}


void report_error( Error error )
{
    Token offending_token = error.offending_token;

    printf( "%s:%d:%d: error: ", g_source_code.path, offending_token.line, offending_token.column );
    switch( error.kind )
    {
        case ERRORKIND_INVALIDSYMBOL:
        {
            printf( "invalid symbol\n" );
            break;
        }

        case ERRORKIND_MISMATCHEDPARENS:
        {
            printf( "mismatched parentheses\n" );
            break;
        }

        case ERRORKIND_UNCLOSEDPARENS:
        {
            printf( "unclosed parentheses\n" );
            break;
        }

        case ERRORKIND_MULTICHARACTERCHARACTER:
        {
            printf( "use double quotes for strings\n" );
            break;
        }

        case ERRORKIND_INCORRECTSYNTAX:
        {
            printf( "incorrect syntax\n" );
            break;
        }

        case ERRORKIND_UNEXPECTEDSYMBOL:
        {
            printf( "unexpected symbol\n" );
            break;
        }

        case ERRORKIND_TYPEMISMATCH:
        {
            // TODO: this
            printf( "expected type ASDJSALD found AKJDSHSAKD\n" );
            break;
        }

        case ERRORKIND_UNDECLAREDSYMBOL:
        {
            printf( "use of undeclared symbol `%s`\n", error.offending_token.as_string );
            break;
        }

        default:
        {
            UNIMPLEMENTED();
            break;
        }
    }

    if( offending_token.kind != TOKENKIND_EOF )
    {
        source_code_print_line( g_source_code, offending_token.line );
        printf( "\n        %*c\n", offending_token.column, '^' );
    }

    if( error.note != NULL )
    {
        printf( "note: %s\n", error.note );
    }
}
