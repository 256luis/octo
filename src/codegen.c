#include <stdio.h>
#include "codegen.h"
#include "debug.h"
#include "parser.h"
#include "lvec.h"

FILE* file;

static void generate_compound( Expression* expression )
{
    static int depth = 0;
    if( depth != 0 )
    {
        fprintf( file, "{\n" );
    }
    depth++;

    size_t length = lvec_get_length( expression->compound.expressions );
    for( size_t i = 0; i < length; i++ )
    {
        Expression* e = expression->compound.expressions[ i ];
        generate_code( e );
    }

    depth--;
    if( depth != 0)
    {
        fprintf( file, "}\n" );
    }
}

static void generate_type( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_VOID:
        case TYPEKIND_INTEGER:
        case TYPEKIND_FLOAT:
        case TYPEKIND_CHARACTER:
        case TYPEKIND_BOOLEAN:
        case TYPEKIND_STRING:
        {
            fprintf( file, "%s ", type_kind_to_string[ type.kind ] );
            break;
        }

        case TYPEKIND_CUSTOM:
        {
            fprintf( file, "%s ", type.custom_identifier );
            break;
        }

        case TYPEKIND_FUNCTION:
        {
            UNIMPLEMENTED();
            break;
        }

        case TYPEKIND_TOINFER:
        {
            UNREACHABLE();
            break;
        }
    }
}

static void generate_rvalue( Expression* expression )
{
    switch( expression->kind )
    {
        case EXPRESSIONKIND_INTEGER:
        {
            fprintf( file, "%d ", expression->integer );
            break;
        }

        case EXPRESSIONKIND_IDENTIFIER:
        {
            fprintf( file, "%s ", expression->identifier );
            break;
        }

        case EXPRESSIONKIND_STRING:
        {
            fprintf( file, "\"%s\" ", expression->string );
            break;
        }

        case EXPRESSIONKIND_CHARACTER:
        {
            fprintf( file, "%c ", expression->character );
            break;
        }

        case EXPRESSIONKIND_BINARY:
        {
            fprintf( file, "( " );
            generate_rvalue( expression->binary.left );
            switch( expression->binary.operation )
            {
                case BINARYOPERATION_ADD:          fprintf( file, "+ " ); break;
                case BINARYOPERATION_SUBTRACT:     fprintf( file, "- " ); break;
                case BINARYOPERATION_MULTIPLY:     fprintf( file, "* " ); break;
                case BINARYOPERATION_DIVIDE:       fprintf( file, "/ " ); break;
                case BINARYOPERATION_EQUAL:        fprintf( file, "== " ); break;
                case BINARYOPERATION_GREATER:      fprintf( file, "> " ); break;
                case BINARYOPERATION_LESS:         fprintf( file, "< " ); break;
                case BINARYOPERATION_NOTEQUAL:     fprintf( file, "!= " ); break;
                case BINARYOPERATION_GREATEREQUAL: fprintf( file, ">= " ); break;
                case BINARYOPERATION_LESSEQUAL:    fprintf( file, "<= " ); break;
            }

            generate_rvalue( expression->binary.right );
            fprintf( file, ") " );

            break;
        }

        case EXPRESSIONKIND_UNARY:
        case EXPRESSIONKIND_FUNCTIONCALL:

    }
}

static void generate_variable_declaration( Expression* expression )
{
    generate_type( expression->variable_declaration.type );
    fprintf( file, "%s = ", expression->variable_declaration.identifier );
    generate_rvalue( expression->variable_declaration.rvalue );
    fprintf( file, ";\n" );
}

static void generate_function_declaration( Expression* expression )
{
    generate_type( expression->function_declaration.return_type );
    fprintf( file, "%s ( ", expression->function_declaration.identifier );

    for( int i = 0; i < expression->function_declaration.param_count; i++ )
    {
        Type param_type = expression->function_declaration.param_types[ i ];
        char* param_identifier = expression->function_declaration.param_identifiers[ i ];
        generate_type( param_type );
        fprintf( file, "%s ", param_identifier );
        if( i < expression->function_declaration.param_count - 1 )
        {
            fprintf( file, ", " );
        }
    }
    fprintf( file, ") "  );

    generate_compound( expression->function_declaration.body );
}

static void generate_return( Expression* expression )
{
    fprintf( file, "return " );

    Expression* rvalue = expression->return_expression.rvalue;
    if ( rvalue != NULL )
    {
        generate_rvalue( rvalue );
    }

    fprintf( file, ";\n" );
}

static void generate_assignment( Expression* expression )
{
    // todo
}

FILE* generate_code( Expression* expression )
{
    static bool is_file_initialized = false;
    if( !is_file_initialized )
    {
        is_file_initialized = true;
        // file = fopen( "generated.c", "w+" );
        file = stdout;
    }

    switch( expression->kind )
    {
        case EXPRESSIONKIND_VARIABLEDECLARATION:
        {
            generate_variable_declaration( expression );
            break;
        }

        case EXPRESSIONKIND_COMPOUND:
        {
            generate_compound( expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONDECLARATION:
        {
            generate_function_declaration( expression );
            break;
        }

        case EXPRESSIONKIND_RETURN:
        {
            generate_return( expression );
            break;
        }

        case EXPRESSIONKIND_ASSIGNMENT:
        {
            generate_assignment( expression );
            break;
        }
    }

    return file;
}
