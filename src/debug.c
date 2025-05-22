#include "debug.h"
#include "lvec.h"
#include "parser.h"
#include "tokenizer.h"
#include <stdbool.h>

#define INDENT() for( int i = 0; i < depth; i++ ) printf( "    " )

char* token_kind_to_string[] = {
    [ TOKENKIND_LET ]          = "let",
    [ TOKENKIND_RETURN ]       = "return",
    [ TOKENKIND_FUNC ]         = "func",
    [ TOKENKIND_INTEGER ]      = "INTEGER",
    [ TOKENKIND_FLOAT ]        = "FLOAT",
    [ TOKENKIND_IDENTIFIER ]   = "IDENTIFIER",
    [ TOKENKIND_STRING ]       = "STRING",
    [ TOKENKIND_CHARACTER ]    = "CHARACTER",
    [ TOKENKIND_BOOLEAN ]      = "BOOLEAN",
    [ TOKENKIND_SEMICOLON ]    = ";",
    [ TOKENKIND_COLON ]        = ":",
    [ TOKENKIND_DOUBLECOLON ]  = "::",
    [ TOKENKIND_PERIOD ]       = ".",
    [ TOKENKIND_COMMA ]        = ",",
    [ TOKENKIND_PLUS ]         = "+",
    [ TOKENKIND_MINUS ]        = "-",
    [ TOKENKIND_STAR ]         = "*",
    [ TOKENKIND_FORWARDSLASH ] = "/",
    [ TOKENKIND_EQUAL ]        = "=",
    [ TOKENKIND_ARROW ]        = "->",
    [ TOKENKIND_BANG ]         = "!",
    [ TOKENKIND_GREATER ]      = ">",
    [ TOKENKIND_LESS ]         = "<",
    [ TOKENKIND_DOUBLEEQUAL ]  = "==",
    [ TOKENKIND_NOTEQUAL ]     = "!=",
    [ TOKENKIND_GREATEREQUAL ] = ">=",
    [ TOKENKIND_LESSEQUAL ]    = "<=",
    [ TOKENKIND_LEFTPAREN ]    = "(",
    [ TOKENKIND_RIGHTPAREN ]   = ")",
    [ TOKENKIND_LEFTBRACE ]    = "{",
    [ TOKENKIND_RIGHTBRACE ]   = "}",
    [ TOKENKIND_LEFTBRACKET ]  = "[",
    [ TOKENKIND_RIGHTBRACKET ] = "]",
    [ TOKENKIND_AMPERSAND ]    = "&",
    [ TOKENKIND_EOF ]          = "EOF",
};

char* expression_kind_to_string[] = {
    [ EXPRESSIONKIND_INTEGER ]             = "INTEGER",
    [ EXPRESSIONKIND_FLOAT ]               = "FLOAT",
    [ EXPRESSIONKIND_IDENTIFIER ]          = "IDENTIFIER",
    [ EXPRESSIONKIND_STRING ]              = "STRING",
    [ EXPRESSIONKIND_CHARACTER ]           = "CHARACTER",
    [ EXPRESSIONKIND_BOOLEAN ]             = "BOOLEAN",
    [ EXPRESSIONKIND_BINARY ]              = "BINARY",
    [ EXPRESSIONKIND_UNARY ]               = "UNARY",
    [ EXPRESSIONKIND_FUNCTIONCALL ]        = "FUNCTIONCALL",
    [ EXPRESSIONKIND_VARIABLEDECLARATION ] = "VARIABLE DECLARATION",
    [ EXPRESSIONKIND_FUNCTIONDECLARATION ] = "FUNCTION DECLARATION",
    [ EXPRESSIONKIND_COMPOUND ]            = "COMPOUND",
    [ EXPRESSIONKIND_RETURN ]              = "RETURN",
    [ EXPRESSIONKIND_ASSIGNMENT ]          = "ASSIGNMENT",
};

char* binary_operation_to_string[] = {
    [ BINARYOPERATION_ADD ]          = "ADD",
    [ BINARYOPERATION_SUBTRACT ]     = "SUBTRACT",
    [ BINARYOPERATION_MULTIPLY ]     = "MULTIPLY",
    [ BINARYOPERATION_DIVIDE ]       = "DIVIDE",
    [ BINARYOPERATION_GREATER ]      = "GREATER",
    [ BINARYOPERATION_LESS ]         = "LESS",
    [ BINARYOPERATION_EQUAL ]        = "EQUAL",
    [ BINARYOPERATION_NOTEQUAL ]     = "NOTEQUAL",
    [ BINARYOPERATION_GREATEREQUAL ] = "GREATEREQUAL",
    [ BINARYOPERATION_LESSEQUAL ]    = "LESSEQUAL",
};

char* unary_operation_to_string[] = {
    [ UNARYOPERATION_NEGATIVE ]    = "NEGATIVE",
    [ UNARYOPERATION_NOT ]         = "NOT",
    [ UNARYOPERATION_ADDRESSOF ]   = "ADDRESSOF",
    [ UNARYOPERATION_DEREFERENCE ] = "DEREFERENCE",
};

char* type_kind_to_string[] = {
    [ TYPEKIND_VOID ]      = "void",
    [ TYPEKIND_INTEGER ]   = "int",
    [ TYPEKIND_FLOAT ]     = "float",
    [ TYPEKIND_CHARACTER ] = "char",
    [ TYPEKIND_BOOLEAN ]   = "bool",
    [ TYPEKIND_STRING ]    = "string",
    [ TYPEKIND_CUSTOM ]    = "CUSTOM",
    [ TYPEKIND_POINTER ]   = "POINTER",
    [ TYPEKIND_FUNCTION ]  = "FUNCTION",
    [ TYPEKIND_TOINFER ]   = "TOINFER",
    [ TYPEKIND_INVALID ]   = "INVALID",
};

void debug_print_type( Type type )
{
    printf( "%s", type_kind_to_string[ type.kind ] );
    switch( type.kind )
    {
        case TYPEKIND_VOID:
        case TYPEKIND_CHARACTER:
        case TYPEKIND_BOOLEAN:
        case TYPEKIND_STRING:
        {
            // do nothing
            break;
        }

        case TYPEKIND_INTEGER:
        {
            printf( "(%c%zu)",
                    type.integer.is_signed ? 'i' : 'u',
                    type.integer.bit_count );
            break;
        }

        case TYPEKIND_FLOAT:
        {
            printf( "(f%zu)", type.integer.bit_count );
            break;
        }

        case TYPEKIND_CUSTOM:
        {
            printf( "(%s)", type.custom_identifier );
            break;
        }

        case TYPEKIND_POINTER:
        {
            printf( "(" );
            debug_print_type( *type.pointer.type );
            printf( ")" );
            break;
        }

        case TYPEKIND_INVALID:
        {
            UNREACHABLE();
            break;
        }
    }
}

static int depth = 0;
void expression_print( Expression* expression )
{
    if( expression == NULL )
    {
        printf("NONE");
        return;
    }

    printf( "%s", expression_kind_to_string[ expression->kind ] );
    bool should_newline = true;
    switch( expression->kind )
    {
        case EXPRESSIONKIND_INTEGER:
        {
            printf( "(%lld)", expression->integer );
            should_newline = false;
            break;
        }

        case EXPRESSIONKIND_FLOAT:
        {
            printf( "(%lf)", expression->floating );
            should_newline = false;
            break;
        }

        case EXPRESSIONKIND_IDENTIFIER:
        {
            printf( "(%s)", expression->identifier );
            should_newline = false;
            break;
        }

        case EXPRESSIONKIND_STRING:
        {
            printf( "(\"%s\")", expression->string );
            should_newline = false;
            break;
        }

        case EXPRESSIONKIND_CHARACTER:
        {
            printf( "(\'%c\')", expression->character );
            should_newline = false;
            break;
        }

        case EXPRESSIONKIND_BINARY:
        {
            char* operation_as_string = binary_operation_to_string[ expression->binary.operation ];

            printf( " {\n" );
            depth++;

            INDENT();
            printf( "operation = %s\n", operation_as_string );

            INDENT();
            printf( "left = " );
            expression_print( expression->binary.left );
            putchar( '\n' );

            INDENT();
            printf( "right = " );
            expression_print( expression->binary.right );
            putchar( '\n' );

            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            char* operation_as_string = unary_operation_to_string[ expression->unary.operation ];

            printf(" {\n");
            depth++;

            INDENT();
            printf( "operation = %s\n", operation_as_string );

            INDENT();
            printf( "operand = " );
            expression_print( expression->unary.operand );

            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            printf(" {\n");
            depth++;

            INDENT();
            printf( "identifier = %s\n", expression->function_call.identifier );

            INDENT();
            printf( "args = {\n" );
            depth++;

            for( size_t i = 0; i < expression->function_call.arg_count; i++ )
            {
                INDENT();
                printf( "[%lld] = ", i );
                expression_print( expression->function_call.args[ i ] );
            }

            depth--;
            INDENT();
            printf( "}\n" );

            // expression_print( expression->unary.operand );

            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_VARIABLEDECLARATION:
        {
            printf( " {\n" );
            depth++;

            INDENT();
            printf( "identifier = %s\n", expression->variable_declaration.identifier );

            INDENT();
            printf( "type = " );
            debug_print_type( expression->variable_declaration.type );
            putchar( '\n' );

            INDENT();
            printf( "value = " );

            if( expression->variable_declaration.rvalue != NULL )
            {
                expression_print( expression->variable_declaration.rvalue );
            }
            else
            {
                printf( "(null)" );
            }

            putchar( '\n' );
            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_COMPOUND:
        {
            printf( " {\n" );
            depth++;


            for( size_t i = 0; i < lvec_get_length( expression->compound.expressions ); i++ )
            {
                INDENT();
                printf( "[%lld] = ", i );
                Expression* s = expression->compound.expressions[ i ];
                expression_print( s );
            }

            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_FUNCTIONDECLARATION:
        {
            printf( " {\n" );
            depth++;

            INDENT();
            printf( "identifier = %s\n", expression->function_declaration.identifier );

            INDENT();
            printf( "return type = " );
            debug_print_type( expression->function_declaration.return_type );
            putchar( '\n' );

            for( int i = 0; i < expression->function_declaration.param_count; i++ )
            {
                char* param_identifier = expression->function_declaration.param_identifiers[ i ];
                Type param_type = expression->function_declaration.param_types[ i ];

                INDENT();
                // printf( "param[%d] = %s: %d\n", i, param_identifier, param_type.kind );
                printf( "param[%d] = %s: ", i, param_identifier );
                debug_print_type( param_type );
                putchar( '\n' );
            }

            INDENT();
            printf( "body = " );
            expression_print( expression->function_declaration.body );

            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_RETURN:
        {
            printf( "(");
            expression_print( expression->return_expression.rvalue );
            printf( ")");
            break;
        }

        case EXPRESSIONKIND_ASSIGNMENT:
        {
            printf( " {\n" );
            depth++;

            INDENT();
            printf( "identifier = %s\n", expression->assignment.identifier );

            INDENT();
            printf( "value = " );

            expression_print( expression->assignment.rvalue );

            putchar( '\n' );
            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        default:
        {
            UNREACHABLE();
            break;
        }
    }

    if( should_newline )
    {
        putchar( '\n' );
    }
}
