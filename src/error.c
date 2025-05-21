#include <stdio.h>
#include <string.h>
#include "error.h"
#include "debug.h"
#include "globals.h"
#include "lvec.h"
#include "parser.h"

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

void print_type( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_INVALID:
        case TYPEKIND_TOINFER:
        case TYPEKIND_FLOAT:
        case TYPEKIND_VOID:
        case TYPEKIND_CHARACTER:
        case TYPEKIND_BOOLEAN:
        case TYPEKIND_STRING:
        {
            printf( "%s", type_kind_to_string[ type.kind ] );
            break;
        }

        // case TYPEKIND_FLOAT:
        case TYPEKIND_INTEGER:
        {
            printf( "%c%zu",
                    type.integer.is_signed ? 'i' : 'u',
                    type.integer.bit_count );
            break;
        }

        case TYPEKIND_FUNCTION:
        {
            printf( "func(" );
            size_t param_count = lvec_get_length( type.function.param_types );
            for( size_t i = 0; i < param_count; i++ )
            {
                Type param_type = type.function.param_types[ i ];
                print_type( param_type );
                if( i < param_count - 1 )
                {
                    printf( ", " );
                }
            }
            printf( ") -> " );
            print_type( *type.function.return_type );
            break;
        }

        case TYPEKIND_POINTER:
        {
            printf( "&" );
            print_type( *type.pointer.type );
            break;
        }

        case TYPEKIND_CUSTOM:
        {
            printf( "%s", type.custom_identifier );
            break;
        }
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
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_MISMATCHEDPARENS:
        {
            printf( "mismatched parentheses\n" );
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_UNCLOSEDPARENS:
        {
            printf( "unclosed parentheses\n" );
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_UNEXPECTEDSYMBOL:
        {
            printf( "unexpected symbol\n" );
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_MULTICHARACTERCHARACTER:
        {
            printf( "use double quotes for strings\n" );
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_SYMBOLREDECLARATION:
        {
            Token original_declaration_token = error.symbol_redeclaration.original_declaration_token;

            printf( "redeclaration of '%s'\n", original_declaration_token.identifier );
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );

            printf( "%s:%d:%d: note: ", g_source_code.path, original_declaration_token.line, original_declaration_token.column );
            printf( "previous declaration here\n");
            source_code_print_line( g_source_code, original_declaration_token.line );
            printf( "\n        %*c\n", original_declaration_token.column, '^' );

            break;
        }

        case ERRORKIND_INVALIDBINARYOPERATION:
        {
            Type left_type = error.invalid_binary_operation.left_type;
            Type right_type = error.invalid_binary_operation.right_type;

            // example: invalid operation for types 'int' and 'bool'
            printf( "invalid operation for types \'" );
            print_type( left_type );
            printf( "\' and \'");
            print_type( right_type );
            printf( "\'\n");

            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_INVALIDUNARYOPERATION:
        {
            Type operand_type = error.invalid_binary_operation.left_type;

            // example: invalid operation for type '&int'
            printf( "invalid operation for type \'" );
            print_type( operand_type );
            printf( "\'\n");

            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_TYPEMISMATCH:
        {
            Type expected_type = error.type_mismatch.expected;
            Type found_type = error.type_mismatch.found;

            /* printf( "expected type '%s', found '%s'\n", */
            /*         type_kind_to_string[ expected_type.kind ], */
            /*         type_kind_to_string[ found_type.kind ] ); */

            printf( "expected type \'" );
            print_type( expected_type );
            printf( "\', found type \'" );
            print_type( found_type );
            printf( "\'\n" );

            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_UNDECLAREDSYMBOL:
        {
            printf( "undeclared symbol\n" );
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_TOOMANYARGUMENTS:
        {
            int expected_arg_count = error.too_many_arguments.expected;
            int found_arg_count = error.too_many_arguments.found;

            printf( "expected %d arguments, found %d\n",
                    expected_arg_count,
                    found_arg_count );
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        case ERRORKIND_INVALIDADDRESSOF:
        {
            printf( "cannot get address of expression\n" );
            source_code_print_line( g_source_code, offending_token.line );
            printf( "\n        %*c\n", offending_token.column, '^' );
            break;
        }

        default:
        {
            UNIMPLEMENTED();
            break;
        }
    }
}
