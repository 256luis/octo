#include <stdarg.h>
#include <stdio.h>
#include "codegen.h"
#include "debug.h"
#include "parser.h"
#include "lvec.h"
#include "semantic.h"
#include "symboltable.h"
#include "type.h"

#define INDENTATION_WIDTH 4

#define MAX(a,b) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

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
        case TYPEKIND_NAMED:
        {
            append( file, "%s", type.named.as_string );
            break;
        }

        case TYPEKIND_COMPOUND:
        {
            append( file, "%s { ", type.compound.is_struct ? "struct" : "union" );

            Symbol* member_symbols = type.compound.member_symbol_table->symbols;
            int member_count = type.compound.member_symbol_table->length;
            for( int i = 0; i < member_count; i++ )
            {
                char* member_identifier = member_symbols[ i ].token.as_string;
                Type member_type = member_symbols[ i ].type;

                generate_type( file, member_type );
                append( file, " %s; ", member_identifier );
            }
            append( file, "}" );
            break;
        }

        case TYPEKIND_FUNCTION:
        {
            UNIMPLEMENTED();
            break;
        }

        case TYPEKIND_POINTER:
        {
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

        case TYPEKIND_REFERENCE:
        {
            generate_type( file, *type.reference.base_type );
            append( file, "*" );
            break;
        }

        case TYPEKIND_TOINFER:
        {
            UNREACHABLE();
            break;
        }

    }
}

static void generate_rvalue( FILE* file, SemanticContext* context, Expression* expression );
static void generate_array_literal( FILE* file, SemanticContext* context, Expression* expression )
{
    // result:
    /* ( OctoArray_T ){ */
    /*     .length = <length>, */
    /*     .data = ( T[<length>] ){ */
    /*         <rvalues> */
    /*     } */
    /* }; */

    append( file,  "(" );
    generate_type( file, expression->array_literal.type );
    append( file, "){\n" );

    int length = expression->array_literal.type.array.length;
    append( file, ".length = %d,\n", length );
    append( file, ".data = (" );

    Type base_type = *expression->array_literal.type.array.base_type;
    generate_type( file, base_type );
    append( file, "[%d]){", length );

    int count_initialized = expression->array_literal.count_initialized;
    for( int i = 0; i < count_initialized; i++ )
    {
        Expression* e = &expression->array_literal.initialized_rvalues[ i ];
        generate_rvalue( file, context, e );
        append( file, ", " );
    }
    append( file, "}\n" );
    append( file, "}" );
}

static void generate_array_subscript( FILE* file, SemanticContext* context, Expression* expression )
{
    Type type = expression->array_subscript.element_type;
    Expression* lvalue = expression->array_subscript.lvalue;
    Expression* index_rvalue = expression->array_subscript.index_rvalue;

    // example: hello[10]
    //          *OctoArray_i32_at(hello, 10)

    append( file, "*OctoArray_" );
    generate_type( file, type );
    append( file, "_at(" );
    generate_rvalue( file, context, lvalue );
    append( file, ", " );
    generate_rvalue( file, context, index_rvalue );
    append( file, ")" );
}

static void generate_member_access( FILE* file, SemanticContext* context, Expression* expression )
{
    Expression* lvalue = expression->member_access.lvalue;
    generate_rvalue( file, context, lvalue );

    char* member_identifier = expression->member_access.member_identifier_token.as_string;
    append( file, ".%s", member_identifier );
}

static void generate_compound_literal( FILE* file, SemanticContext* context, Expression* expression )
{
    char* type_identifier = expression->compound_literal.type_identifier_token.as_string;
    append( file, "(%s){\n", type_identifier );

    int initialized_count = expression->compound_literal.initialized_count;
    for( int i = 0; i < initialized_count; i++ )
    {
        char* member_identifier = expression->compound_literal.member_identifier_tokens[ i ].as_string;
        append( file, ".%s = ", member_identifier );

        Expression initialized_member_rvalue = expression->compound_literal.initialized_member_rvalues[ i ];
        generate_rvalue( file, context, &initialized_member_rvalue );

        append( file, ",\n" );
    }

    append( file, "}" );
}

static void generate_function_call( FILE* file, SemanticContext* context, Expression* expression );
static void generate_rvalue( FILE* file, SemanticContext* context, Expression* expression )
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
            if( expression->identifier.type.kind == TYPEKIND_REFERENCE )
            {
                append( file, "*" );
            }
            append( file, "%s", expression->identifier.as_string );
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
            generate_rvalue( file, context, expression->binary.left );

            switch( expression->binary.operation )
            {
                case BINARYOPERATION_ADD:          append( file, " + " ); break;
                case BINARYOPERATION_SUBTRACT:     append( file, " - " ); break;
                case BINARYOPERATION_MULTIPLY:     append( file, " * " ); break;
                case BINARYOPERATION_DIVIDE:       append( file, " / " ); break;
                case BINARYOPERATION_MODULO:       append( file, " % " ); break;
                case BINARYOPERATION_EQUAL:        append( file, " == " ); break;
                case BINARYOPERATION_GREATER:      append( file, " > " ); break;
                case BINARYOPERATION_LESS:         append( file, " < " ); break;
                case BINARYOPERATION_NOTEQUAL:     append( file, " != " ); break;
                case BINARYOPERATION_GREATEREQUAL: append( file, " >= " ); break;
                case BINARYOPERATION_LESSEQUAL:    append( file, " <= " ); break;
                case BINARYOPERATION_AND:          append( file, " && " ); break;
                case BINARYOPERATION_OR:           append( file, " || " ); break;
            }

            generate_rvalue( file, context, expression->binary.right );
            append( file, ")" );

            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            append( file, "(" );
            switch( expression->unary.operation )
            {
                case UNARYOPERATION_NEGATIVE:    append( file, "-" ); break;
                case UNARYOPERATION_NOT:         append( file, "!" ); break;
                case UNARYOPERATION_ADDRESSOF:   append( file, "&" ); break;
                case UNARYOPERATION_DEREFERENCE: append( file, "*" ); break;
            }
            generate_rvalue( file, context, expression->unary.operand );
            append( file, ")" );

            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            generate_function_call( file, context, expression );
            break;
        }

        case EXPRESSIONKIND_ARRAYLITERAL:
        {
            generate_array_literal( file, context, expression );
            break;
        }

        case EXPRESSIONKIND_ARRAYSUBSCRIPT:
        {
            generate_array_subscript( file, context, expression );
            break;
        }

        case EXPRESSIONKIND_MEMBERACCESS:
        {
            generate_member_access( file, context, expression );
            break;
        }

        case EXPRESSIONKIND_COMPOUNDLITERAL:
        {
            generate_compound_literal( file, context, expression );
            break;
        }

        default:
        {
            UNREACHABLE();
            break;
        }
    }
}

static void generate_variable_declaration( FILE* file, SemanticContext* context, Expression* expression )
{
    Type type = expression->variable_declaration.variable_type;
    char* identifier = expression->variable_declaration.identifier_token.as_string;
    Expression* rvalue = expression->variable_declaration.rvalue;

    generate_type( file, type );
    append( file, " %s", identifier );

    if( rvalue != NULL )
    {
        append( file, " = " );
        generate_rvalue( file, context, rvalue );
    }
    else if( type.kind == TYPEKIND_ARRAY && rvalue == NULL )
    {
        Expression right_side = {
            .kind = EXPRESSIONKIND_ARRAYLITERAL,
            .array_literal = {
                .type = type,
                .count_initialized = 0,
                .initialized_rvalues = NULL,
            },
        };

        append( file, " = " );
        generate_array_literal( file, context, &right_side );
    }

    append( file, ";\n" );
}

static void generate_function_declaration( FILE* file, SemanticContext* context,  Expression* expression )
{
    Type return_type = expression->function_declaration.return_type;
    generate_type( file, return_type );

    char* identifier = expression->function_declaration.identifier_token.as_string;
    append( file, " %s(", identifier );

    int param_count = expression->function_declaration.param_count;
    bool is_variadic = expression->function_declaration.is_variadic;
    Type* param_types = expression->function_declaration.param_types;
    Token* param_identifiers_tokens = expression->function_declaration.param_identifiers_tokens;
    if( param_count > 0 )
    {
        // append the first param
        Type param_type = param_types[ 0 ];
        char* param_identifier = param_identifiers_tokens[ 0 ].as_string;
        generate_type( file, param_type );
        append( file, " %s", param_identifier );

        // the rest of the params
        for( int i = 1; i < param_count; i++ )
        {
            Type param_type = param_types[ i ];
            char* param_identifier = param_identifiers_tokens[ i ].as_string;

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

static void generate_return( FILE* file, SemanticContext* context, Expression* expression )
{
    append( file, "return " );

    Expression* rvalue = expression->return_expression.rvalue;
    if ( rvalue != NULL )
    {
        generate_rvalue( file, context, rvalue );
    }

    append( file, ";\n" );
}

static void generate_assignment( FILE* file, SemanticContext* context, Expression* expression )
{
    generate_rvalue( file, context, expression->assignment.lvalue );
    append( file, " = ", expression->assignment.lvalue );
    generate_rvalue( file, context, expression->assignment.rvalue );
    append( file, ";\n" );
}

static void generate_function_call( FILE* file, SemanticContext* context, Expression* expression )
{
    append( file, "%s(", expression->function_call.identifier_token.as_string );

    for( size_t i = 0; i < expression->function_call.arg_count; i++ )
    {
        Expression arg = expression->function_call.args[ i ];
        generate_rvalue( file, context, &arg );
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
    generate_rvalue( file, context, expression->conditional.condition );
    append( file, ")\n" );
    generate_code( file, context, expression->conditional.true_body );

    if( expression->conditional.false_body != NULL )
    {
        append( file, "else " );
        generate_code( file, context, expression->conditional.false_body );
    }
}

static void generate_for_loop( FILE* file, SemanticContext* context, Expression* expression )
{
    Expression* iterable_rvalue = expression->for_loop.iterable_rvalue;
    Token iterator_token = expression->for_loop.iterator_token;
    Type iterator_type = expression->for_loop.iterator_type;

    append( file, "for (u64 octo_index = 0; octo_index < ");
    generate_rvalue( file, context, iterable_rvalue );
    append( file, ".length; octo_index++)\n{\n");
    generate_type( file, iterator_type );
    append( file, " %s = ", iterator_token.identifier);
    generate_rvalue( file, context, iterable_rvalue );
    append( file, ".data + octo_index;\n");

    Expression* body = expression->for_loop.body;

    size_t length = lvec_get_length( body->compound.expressions );
    for( size_t i = 0; i < length; i++ )
    {
        Expression* e = body->compound.expressions[ i ];
        generate_code( file, context, e );
    }

    append( file, "}\n");
}

static void generate_pointer_type_definition( FILE* file, Type base_type )
{
    append( file, "#define OctoPtr_" );
    generate_type( file, base_type );
    append( file, " ");
    generate_type( file, base_type );
    append( file, "*\n");
}

static void generate_array_type_definition( FILE* file, Type base_type )
{
    append( file, "OCTO_DEFINE_ARRAY(" );
    generate_type( file, base_type );
    append( file, ")\n" );
}

static void generate_type_rvalue( FILE* file, Expression* type_rvalue );
static void generate_compound_definition( FILE* file,  Expression* expression )
{
    bool is_struct = expression->compound_definition.is_struct;
    append( file, "%s {\n", is_struct ? "struct" : "union" );

    /* SymbolTable* member_symbol_table = type_definition.definition.info->compound.member_symbol_table; */
    int member_count = expression->compound_definition.member_count;
    for( int i = 0; i < member_count; i++ )
    {
        char* member_identifier = expression->compound_definition.member_identifier_tokens[ i ].as_string;
        Type member_type = expression->compound_definition.member_types[ i ];
        generate_type( file, member_type );
        append( file, " %s;\n", member_identifier );
    }
    append( file, "}" );
}

static void generate_type_rvalue( FILE* file, Expression* type_rvalue )
{
    switch( type_rvalue->kind )
    {
        case EXPRESSIONKIND_COMPOUNDDEFINITION:
        {
            generate_compound_definition( file, type_rvalue );
            break;
        }

        case EXPRESSIONKIND_TYPEIDENTIFIER:
        {
            append( file, "%s", type_rvalue->type_identifier.token.as_string );
            break;
        }

        case EXPRESSIONKIND_POINTERTYPE:
        {
            append( file, "OctoPtr_" );
            generate_type_rvalue( file, type_rvalue->pointer_type.base_type_rvalue );
            break;
        }

        case EXPRESSIONKIND_ARRAYTYPE:
        {
            append( file, "OctoArray_" );
            generate_type_rvalue( file, type_rvalue->array_type.base_type_rvalue );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }
}

static void generate_type_declaration( FILE* file, SemanticContext* context, Expression* expression )
{
    char* type_identifier = expression->type_declaration.identifier_token.as_string;
    Type type_definition = *symbol_table_lookup( context->symbol_table, type_identifier )->type.type.info;

    append( file, "typedef " );

    Expression* type_rvalue = expression->type_declaration.rvalue;
    generate_type_rvalue( file, type_rvalue );
    append( file, " %s;\n", type_definition.named.as_string );


    // generate all pointer and array types associated with the declared type
    Type* pointer_types = type_definition.named.pointer_types;
    int pointer_types_length = lvec_get_length( pointer_types );
    for( int j = pointer_types_length - 1; j >= 0; j-- )
    {
        Type base_type = pointer_types[ j ];
        generate_pointer_type_definition( file, base_type );
    }

    // generate typedefs for arrays
    Type* array_types = type_definition.named.array_types;
    int array_types_length = lvec_get_length( array_types );
    for( int j = array_types_length - 1; j >= 0; j-- )
    {
        Type base_type = array_types[ j ];
        generate_array_type_definition( file, base_type );
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

        // generate code for pointers and arrays for primitive types
        for( int i = 0; i < context->symbol_table.length; i++ )
        {
            Type type = context->symbol_table.symbols[ i ].type;
            if( type.kind != TYPEKIND_TYPE )
            {
                continue;
            }

            if( type.type.info->kind != TYPEKIND_NAMED )
            {
                continue;
            }

            // generate typedefs for pointers
            Type* pointer_types = type.type.info->named.pointer_types;
            int pointer_types_length = lvec_get_length( pointer_types );
            for( int j = pointer_types_length - 1; j >= 0; j-- )
            {
                Type base_type = pointer_types[ j ];
                generate_pointer_type_definition( file, base_type );
            }

            // generate typedefs for arrays
            Type* array_types = type.type.info->named.array_types;
            int array_types_length = lvec_get_length( array_types );
            for( int j = array_types_length - 1; j >= 0; j-- )
            {
                Type base_type = array_types[ j ];
                generate_array_type_definition( file, base_type );
            }
        }
    }

    switch( expression->kind )
    {
        case EXPRESSIONKIND_VARIABLEDECLARATION:
        {
            generate_variable_declaration( file, context, expression );
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
            generate_return( file, context, expression );
            break;
        }

        case EXPRESSIONKIND_ASSIGNMENT:
        {
            generate_assignment( file, context, expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            generate_function_call( file, context, expression );
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

        case EXPRESSIONKIND_FORLOOP:
        {
            generate_for_loop( file, context, expression );
            break;
        }

        case EXPRESSIONKIND_TYPEDECLARATION:
        {
            generate_type_declaration( file, context, expression );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }
}
