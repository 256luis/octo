#include <stdarg.h>
#include <stdio.h>
#include "codegen.h"
#include "debug.h"
#include "parser.h"
#include "lvec.h"
#include "semantic.h"

#define INDENTATION_WIDTH 4

static int depth = 0;
static void append( FILE* file, const char* format, ... )
{
    va_list args;
    va_start( args, format );
    vfprintf( file, format, args );
    va_end( args );
}

static void generate_compound( FILE* file, SemanticContext* context, Expression* expression )
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
        generate_code( file, context, e );
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
            append( file, "%s", type_kind_to_string[ type.kind ] );
            break;
        }

        case TYPEKIND_INTEGER:
        {
            append( file, "%c%zu",
                    type.integer.is_signed ? 'i' : 'u',
                    type.integer.bit_count );
            break;
        }

        case TYPEKIND_FLOAT:
        {
            append( file, "f%zu", type.integer.bit_count );
            break;
        }

        case TYPEKIND_CUSTOM:
        {
            append( file, "%s", type.custom_identifier );
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
            append( file, "OctoPtr_" );
            generate_type( file, *type.pointer.base_type );
            break;
        }

        case TYPEKIND_ARRAY:
        {
            append( file, "OctoArray_" );
            generate_type( file, *type.array.base_type );
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

static void generate_rvalue( FILE* file, Expression* expression );
static void generate_array( FILE* file, Expression* expression )
{
    // ( OctoArray_T ){
    //     .length = <length>,
    //     .data = ( T[<length>] ){
    //         <rvalues>
    //     }
    // };

    int length = expression->array.type.array.length;
    Type base_type = *expression->array.type.array.base_type;

    append( file,  "(" );
    generate_type( file, expression->array.type );
    append( file, "){\n" );
    append( file, ".length = %d,\n", length );
    append( file, ".data = (" );
    generate_type( file, base_type );
    append( file, "[%d]){", length );
    for( int i = 0; i < expression->array.count_initialized; i++ )
    {
        Expression* e = &expression->array.initialized_rvalues[ i ];
        generate_rvalue( file, e );
        append( file, ", " );
    }
    append( file, "}\n" );
    append( file, "}" );
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

        case EXPRESSIONKIND_ARRAY:
        {
            generate_array( file, expression );
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
    append( file, " %s", expression->variable_declaration.identifier );

    if( expression->variable_declaration.rvalue != NULL )
    {
        append( file, " = " );
        generate_rvalue( file, expression->variable_declaration.rvalue );
    }

    append( file, ";\n" );
}

static void generate_function_declaration( FILE* file, SemanticContext* context,  Expression* expression )
{
    Type return_type = expression->function_declaration.return_type;
    char* identifier = expression->function_declaration.identifier;
    Type* param_types = expression->function_declaration.param_types;
    char** param_identifiers = expression->function_declaration.param_identifiers;
    int param_count = expression->function_declaration.param_count;
    bool is_variadic = expression->function_declaration.is_variadic;

    generate_type( file, return_type );
    append( file, " %s(", identifier );

    if( param_count > 0 )
    {
        // append the first param
        Type param_type = param_types[ 0 ];
        char* param_identifier = param_identifiers[ 0 ];
        generate_type( file, param_type );
        append( file, " %s", param_identifier );

        for( int i = 1; i < param_count; i++ )
        {
            Type param_type = param_types[ i ];
            char* param_identifier = param_identifiers[ i ];

            append( file, ", " );
            generate_type( file, param_type );
            append( file, " %s", param_identifier );
        }
    }

    if( is_variadic )
    {
        append( file, ", ..." );
    }

    append( file, ")"  );

    Expression* function_body = expression->function_declaration.body;
    if( function_body != NULL )
    {
        append( file, "\n" );
        generate_compound( file, context, function_body );
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

static void generate_conditional( FILE* file, SemanticContext* context, Expression* expression )
{
    append( file, "%s (", expression->conditional.is_loop ? "while" : "if" );
    generate_rvalue( file, expression->conditional.condition );
    append( file, ")\n" );
    generate_code( file, context, expression->conditional.true_body );

    if( expression->conditional.false_body != NULL )
    {
        append( file, "else " );
        generate_code( file, context, expression->conditional.false_body );
    }
}

void generate_code( FILE* file, SemanticContext* context, Expression* expression )
{
    // temporary
    static bool first = true;
    if( first )
    {
        first = false;

        append( file, "#include \"octoruntime/types.h\"\n" );

        // generate forward declarations of array types
        for( size_t i = 0; i < lvec_get_length( context->array_types ); i++ )
        {
            // "typedef struct OctoArray_T OctoArray_T; "

            Type base_type = context->array_types[ i ];
            append( file, "typedef struct OctoArray_" );
            generate_type( file, base_type );
            append( file, " OctoArray_" );
            generate_type( file, base_type );
            append( file, ";\n" );
        }

        // generate definitions for pointer types
        for( size_t i = 0; i < lvec_get_length( context->pointer_types ); i++ )
        {
            // "typedef T* T_ptr;"

            Type base_type = context->pointer_types[ i ];
            append( file, "#define OctoPtr_" );
            generate_type( file, base_type );
            append( file, " ");
            generate_type( file, base_type );
            append( file, "*\n");
        }

        // generate definitions for array_types
        for( size_t i = 0; i < lvec_get_length( context->array_types ); i++ )
        {
            // "OCTO_DEFINE_ARRAY(T);"

            Type base_type = context->array_types[ i ];
            append( file, "OCTO_DEFINE_ARRAY(" );
            generate_type( file, base_type );
            append( file, ");\n" );
        }
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
            generate_compound( file, context, expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONDECLARATION:
        {
            generate_function_declaration( file, context, expression );
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
            generate_function_declaration( file, context, expression->extern_expression.function );
            break;
        }

        case EXPRESSIONKIND_CONDITIONAL:
        {
            generate_conditional( file, context, expression );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }
}
