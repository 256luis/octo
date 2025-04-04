#include "debug.h"
#include "parser.h"
#include <stdbool.h>

#define INDENT() for( int i = 0; i < depth; i++ ) printf( "    " )

char* token_kind_to_string[] = {
    [ TOKENKIND_LET ]          = "let",
    [ TOKENKIND_RETURN ]       = "return",
    [ TOKENKIND_FUNC ]         = "func",
    [ TOKENKIND_NUMBER ]       = "NUMBER",
    [ TOKENKIND_IDENTIFIER ]   = "IDENTIFIER",
    [ TOKENKIND_STRING ]       = "STRING",
    [ TOKENKIND_CHARACTER ]    = "CHARACTER",
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
    [ TOKENKIND_EOF ]          = "EOF",
};

char* expression_kind_to_string[] = {
    [ EXPRESSIONKIND_NUMBER ]              = "NUMBER",
    [ EXPRESSIONKIND_IDENTIFIER ]          = "IDENTIFIER",
    [ EXPRESSIONKIND_STRING ]              = "STRING",
    [ EXPRESSIONKIND_CHARACTER ]           = "CHARACTER",
    [ EXPRESSIONKIND_BINARY ]              = "BINARY",
    [ EXPRESSIONKIND_UNARY ]               = "UNARY",
    [ EXPRESSIONKIND_FUNCTIONCALL ]        = "FUNCTIONCALL",
    [ EXPRESSIONKIND_VARIABLEDECLARATION ] = "VARIABLE DECLARATION",
    [ EXPRESSIONKIND_FUNCTIONDECLARATION ] = "FUNCTION DECLARATION",
    [ EXPRESSIONKIND_COMPOUND ]            = "COMPOUND",
    [ EXPRESSIONKIND_RETURN ]              = "RETURN",
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
    [ UNARYOPERATION_NEGATIVE ] = "NEGATIVE",
    [ UNARYOPERATION_NOT ] = "NOT",
};


static int depth = 0;
void expression_print( Expression* expression )
{
    printf( "%s", expression_kind_to_string[ expression->kind ] );
    bool should_newline = true;
    switch( expression->kind )
    {
        case EXPRESSIONKIND_NUMBER:
        {
            printf( "(%d)", expression->number );
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
            printf( "type = %s\n", expression->variable_declaration.type );

            INDENT();
            printf( "value = " );

            if( expression->variable_declaration.value != NULL )
            {
                expression_print( expression->variable_declaration.value );
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

            for( size_t i = 0; i < expression->compound.expressions.list.length; i++ )
            {
                INDENT();
                printf( "[%lld] = ", i );
                Expression s = expression_list_get( expression->compound.expressions, i );
                expression_print( &s );
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
            printf( "return type = %s\n", expression->function_declaration.return_type );

            for( int i = 0; i < expression->function_declaration.param_count; i++ )
            {
                char* param_identifier = expression->function_declaration.param_identifiers[ i ];
                char* param_type = expression->function_declaration.param_types[ i ];

                INDENT();
                printf( "param[%d] = %s: %s\n", i, param_identifier, param_type );
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
            expression_print( expression->return_expression.value );
            printf( ")");
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
