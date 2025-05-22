#include <stdarg.h>
#include <stdio.h>
#include "codegen.h"
#include "debug.h"
#include "parser.h"
#include "lvec.h"

#define INDENTATION_WIDTH 4

FILE* file;

static int depth = 0;
static void append( const char* format, ... )
{
    va_list args;
    va_start( args, format );
    vfprintf( file, format, args );
    va_end( args );
}

static void generate_compound( Expression* expression )
{
    if( depth != 0 )
    {
        append( "{\n" );
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
        append( "}\n\n" );
    }
}

static void generate_type( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_VOID:
        case TYPEKIND_CHARACTER:
        case TYPEKIND_BOOLEAN:
        case TYPEKIND_STRING:
        {
            append( "%s ", type_kind_to_string[ type.kind ] );
            break;
        }

        case TYPEKIND_INTEGER:
        {
            append( "%c%zu ",
                    type.integer.is_signed ? 'i' : 'u',
                    type.integer.bit_count );
            break;
        }

        case TYPEKIND_FLOAT:
        {
            append( "f%zu ", type.integer.bit_count );
            break;
        }

        case TYPEKIND_CUSTOM:
        {
            append( "%s ", type.custom_identifier );
            break;
        }

        case TYPEKIND_FUNCTION:
        {
            UNIMPLEMENTED();
            break;
        }

        case TYPEKIND_POINTER:
        {
            // UNIMPLEMENTED();
            generate_type( *type.pointer.type );
            append( "*" );
            break;
        }

        case TYPEKIND_INVALID:
        case TYPEKIND_TOINFER:
        {
            UNREACHABLE();
            break;
        }

    }
}

static void generate_function_call( Expression* expression );
static void generate_rvalue( Expression* expression )
{
    switch( expression->kind )
    {
        case EXPRESSIONKIND_INTEGER:
        {
            append( "%lld", expression->integer );
            break;
        }

        case EXPRESSIONKIND_FLOAT:
        {
            append( "%lf", expression->floating );
            break;
        }

        case EXPRESSIONKIND_IDENTIFIER:
        {
            append( "%s", expression->identifier );
            break;
        }

        case EXPRESSIONKIND_STRING:
        {
            append( "\"%s\"", expression->string );
            break;
        }

        case EXPRESSIONKIND_CHARACTER:
        {
            append( "\'%c\'", expression->character );
            break;
        }

        case EXPRESSIONKIND_BINARY:
        {
            append( "(" );
            generate_rvalue( expression->binary.left );
            switch( expression->binary.operation )
            {
                case BINARYOPERATION_ADD:          append( " + " ); break;
                case BINARYOPERATION_SUBTRACT:     append( " - " ); break;
                case BINARYOPERATION_MULTIPLY:     append( " * " ); break;
                case BINARYOPERATION_DIVIDE:       append( " / " ); break;
                case BINARYOPERATION_EQUAL:        append( " == " ); break;
                case BINARYOPERATION_GREATER:      append( " > " ); break;
                case BINARYOPERATION_LESS:         append( " < " ); break;
                case BINARYOPERATION_NOTEQUAL:     append( " != " ); break;
                case BINARYOPERATION_GREATEREQUAL: append( " >= " ); break;
                case BINARYOPERATION_LESSEQUAL:    append( " <= " ); break;
            }

            generate_rvalue( expression->binary.right );
            append( ")" );

            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            switch( expression->unary.operation )
            {
                case UNARYOPERATION_NEGATIVE:  append( "-" ); break;
                case UNARYOPERATION_NOT:       append( "!" ); break;
                case UNARYOPERATION_ADDRESSOF: append( "&" ); break;
            }
            generate_rvalue( expression->unary.operand );

            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            generate_function_call( expression );
            break;
        }
    }
}

static void generate_variable_declaration( Expression* expression )
{
    generate_type( expression->variable_declaration.type );
    append( "%s", expression->variable_declaration.identifier );

    if( expression->variable_declaration.rvalue != NULL )
    {
        append(" = ");
        generate_rvalue( expression->variable_declaration.rvalue );
    }

    append( ";\n" );
}

static void generate_function_declaration( Expression* expression )
{
    generate_type( expression->function_declaration.return_type );
    append( "%s(", expression->function_declaration.identifier );

    for( int i = 0; i < expression->function_declaration.param_count; i++ )
    {
        Type param_type = expression->function_declaration.param_types[ i ];
        char* param_identifier = expression->function_declaration.param_identifiers[ i ];
        generate_type( param_type );
        append( "%s", param_identifier );
        if( i < expression->function_declaration.param_count - 1 )
        {
            append( ", " );
        }
    }
    append( ")\n"  );

    generate_compound( expression->function_declaration.body );
}

static void generate_return( Expression* expression )
{
    append( "return " );

    Expression* rvalue = expression->return_expression.rvalue;
    if ( rvalue != NULL )
    {
        generate_rvalue( rvalue );
    }

    append( ";\n" );
}

static void generate_assignment( Expression* expression )
{
    append( "%s = ", expression->assignment.identifier );
    generate_rvalue( expression->assignment.rvalue );
    append( ";\n" );
}

static void generate_function_call( Expression* expression )
{
    append( "%s(", expression->function_call.identifier );

    for( size_t i = 0; i < expression->function_call.arg_count; i++ )
    {
        Expression* arg = expression->function_call.args[ i ];
        generate_rvalue( arg );
        /* generate_type( param_type ); */
        /* append( "%s ", param_identifier ); */
        if( i < expression->function_call.arg_count - 1 )
        {
            append( ", " );
        }
    }
    append( ")" );

}

FILE* generate_code( Expression* expression )
{
    static bool is_file_initialized = false;
    if( !is_file_initialized )
    {
        is_file_initialized = true;
        file = fopen( "generated.c", "w+" );

        // temporary
        append( "typedef signed char        i8;\n" );
        append( "typedef short              i16;\n" );
        append( "typedef int                i32;\n" );
        append( "typedef long long          i64;\n" );
        append( "typedef unsigned char      u8;\n" );
        append( "typedef unsigned short     u16;\n" );
        append( "typedef unsigned int       u32;\n" );
        append( "typedef unsigned long long u64;\n" );
        append( "typedef float              f32;\n" );
        append( "typedef double             f64;\n" );
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

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            generate_function_call( expression );
            append( ";\n" );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }

    return file;
}
