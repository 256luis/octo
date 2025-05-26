#include <stdarg.h>
#include <stdio.h>
#include "codegen.h"
#include "debug.h"
#include "parser.h"
#include "lvec.h"

#define INDENTATION_WIDTH 4

// FILE* file;

static int depth = 0;
static void append( FILE* file, const char* format, ... )
{
    va_list args;
    va_start( args, format );
    vfprintf( file, format, args );
    va_end( args );
}

static void generate_compound( FILE* file, Expression* expression )
{
    if( depth != 0 )
    {
        append( file, "{\n" );
    }
    depth++;

    size_t length = lvec_get_length( expression->compound.expressions );
    for( size_t i = 0; i < length; i++ )
    {
        Expression* e = expression->compound.expressions[ i ];
        generate_code( file, e );
    }

    depth--;
    if( depth != 0)
    {
        append( file, "}\n" );
    }
}

static void generate_type( FILE* file, Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_VOID:
        case TYPEKIND_CHARACTER:
        case TYPEKIND_BOOLEAN:
            // case TYPEKIND_STRING:
        {
            append( file, "%s ", type_kind_to_string[ type.kind ] );
            break;
        }

        case TYPEKIND_INTEGER:
        {
            append( file, "%c%zu ",
                    type.integer.is_signed ? 'i' : 'u',
                    type.integer.bit_count );
            break;
        }

        case TYPEKIND_FLOAT:
        {
            append( file, "f%zu ", type.integer.bit_count );
            break;
        }

        case TYPEKIND_CUSTOM:
        {
            append( file, "%s ", type.custom_identifier );
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
            generate_type( file, *type.pointer.type );
            append( file, "*" );
            break;
        }

        case TYPEKIND_ARRAY:
        {
            UNIMPLEMENTED();
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

static void generate_function_call( FILE* file, Expression* expression );
static void generate_rvalue( FILE* file, Expression* expression )
{
    switch( expression->kind )
    {
        case EXPRESSIONKIND_INTEGER:
        {
            append( file, "%lld", expression->integer );
            break;
        }

        case EXPRESSIONKIND_FLOAT:
        {
            append( file, "%lf", expression->floating );
            break;
        }

        case EXPRESSIONKIND_IDENTIFIER:
        {
            append( file, "%s", expression->identifier );
            break;
        }

        case EXPRESSIONKIND_STRING:
        {
            append( file, "\"%s\"", expression->string );
            break;
        }

        case EXPRESSIONKIND_CHARACTER:
        {
            append( file, "\'%c\'", expression->character );
            break;
        }

        case EXPRESSIONKIND_BOOLEAN:
        {
            append( file, "%s", expression->associated_token.as_string );
            break;
        }

        case EXPRESSIONKIND_BINARY:
        {
            append( file, "(" );
            generate_rvalue( file, expression->binary.left );
            switch( expression->binary.operation )
            {
                case BINARYOPERATION_ADD:          append( file, " + " ); break;
                case BINARYOPERATION_SUBTRACT:     append( file, " - " ); break;
                case BINARYOPERATION_MULTIPLY:     append( file, " * " ); break;
                case BINARYOPERATION_DIVIDE:       append( file, " / " ); break;
                case BINARYOPERATION_EQUAL:        append( file, " == " ); break;
                case BINARYOPERATION_GREATER:      append( file, " > " ); break;
                case BINARYOPERATION_LESS:         append( file, " < " ); break;
                case BINARYOPERATION_NOTEQUAL:     append( file, " != " ); break;
                case BINARYOPERATION_GREATEREQUAL: append( file, " >= " ); break;
                case BINARYOPERATION_LESSEQUAL:    append( file, " <= " ); break;
            }

            generate_rvalue( file, expression->binary.right );
            append( file, ")" );

            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            switch( expression->unary.operation )
            {
                case UNARYOPERATION_NEGATIVE:    append( file, "-" ); break;
                case UNARYOPERATION_NOT:         append( file, "!" ); break;
                case UNARYOPERATION_ADDRESSOF:   append( file, "&" ); break;
                case UNARYOPERATION_DEREFERENCE: append( file, "*" ); break;
            }
            generate_rvalue( file, expression->unary.operand );

            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            generate_function_call( file, expression );
            break;
        }

        default:
        {
            UNREACHABLE();
            break;
        }
    }
}

static void generate_variable_declaration( FILE* file, Expression* expression )
{
    generate_type( file, expression->variable_declaration.type );
    append( file, "%s", expression->variable_declaration.identifier );

    if( expression->variable_declaration.rvalue != NULL )
    {
        append( file, " = " );
        generate_rvalue( file, expression->variable_declaration.rvalue );
    }

    append( file, ";\n" );
}

static void generate_function_declaration( FILE* file, Expression* expression )
{
    generate_type( file, expression->function_declaration.return_type );
    append( file, "%s(", expression->function_declaration.identifier );

    for( int i = 0; i < expression->function_declaration.param_count; i++ )
    {
        Type param_type = expression->function_declaration.param_types[ i ];
        char* param_identifier = expression->function_declaration.param_identifiers[ i ];
        generate_type( file, param_type );
        append( file, "%s", param_identifier );
        if( i < expression->function_declaration.param_count - 1 )
        {
            append( file, ", " );
        }
    }
    append( file, ")"  );

    Expression* function_body = expression->function_declaration.body;
    if( function_body != NULL )
    {
        append( file, "\n" );
        generate_compound( file, function_body );
    }
    else
    {
        append( file, ";\n" );
    }
}

static void generate_return( FILE* file, Expression* expression )
{
    append( file, "return " );

    Expression* rvalue = expression->return_expression.rvalue;
    if ( rvalue != NULL )
    {
        generate_rvalue( file, rvalue );
    }

    append( file, ";\n" );
}

static void generate_assignment( FILE* file, Expression* expression )
{
    generate_rvalue( file, expression->assignment.lvalue );
    append( file, " = ", expression->assignment.lvalue );
    generate_rvalue( file, expression->assignment.rvalue );
    append( file, ";\n" );
}

static void generate_function_call( FILE* file, Expression* expression )
{
    append( file, "%s(", expression->function_call.identifier );

    for( size_t i = 0; i < expression->function_call.arg_count; i++ )
    {
        Expression* arg = expression->function_call.args[ i ];
        generate_rvalue( file, arg );
        /* generate_type( param_type ); */
        /* append( file, "%s ", param_identifier ); */
        if( i < expression->function_call.arg_count - 1 )
        {
            append( file, ", " );
        }
    }
    append( file, ")" );

}

static void generate_conditional( FILE* file, Expression* expression )
{
    append( file, "%s (", expression->conditional.is_loop ? "while" : "if" );
    generate_rvalue( file, expression->conditional.condition );
    append( file, ")\n" );
    generate_code( file, expression->conditional.true_body );

    if( expression->conditional.false_body != NULL )
    {
        append( file, "else " );
        generate_code( file, expression->conditional.false_body );
    }
}



FILE* generate_code( FILE* file, Expression* expression )
{
    // temporary
    static bool first = true;
    if( first )
    {
        first = false;
        append( file, "typedef signed char        i8;\n" );
        append( file, "typedef short              i16;\n" );
        append( file, "typedef int                i32;\n" );
        append( file, "typedef long long          i64;\n" );
        append( file, "typedef unsigned char      u8;\n" );
        append( file, "typedef unsigned short     u16;\n" );
        append( file, "typedef unsigned int       u32;\n" );
        append( file, "typedef unsigned long long u64;\n" );
        append( file, "typedef float              f32;\n" );
        append( file, "typedef double             f64;\n" );
    }

    switch( expression->kind )
    {
        case EXPRESSIONKIND_VARIABLEDECLARATION:
        {
            generate_variable_declaration( file, expression );
            break;
        }

        case EXPRESSIONKIND_COMPOUND:
        {
            generate_compound( file, expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONDECLARATION:
        {
            generate_function_declaration( file, expression );
            break;
        }

        case EXPRESSIONKIND_RETURN:
        {
            generate_return( file, expression );
            break;
        }

        case EXPRESSIONKIND_ASSIGNMENT:
        {
            generate_assignment( file, expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            generate_function_call( file, expression );
            append( file, ";\n" );
            break;
        }

        case EXPRESSIONKIND_EXTERN:
        {
            generate_function_declaration( file, expression->extern_expression.function );
            break;
        }

        case EXPRESSIONKIND_CONDITIONAL:
        {
            generate_conditional( file, expression );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }

    // return file;
}
