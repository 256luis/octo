// TODO: fix the formatting on the output of the ast

#include "debug.h"
#include "lvec.h"
#include "parser.h"
#include "tokenizer.h"
#include "type.h"
#include <stdbool.h>

#define INDENT() for( int i = 0; i < depth; i++ ) printf( "    " )

char* token_kind_to_string[] = {
    [ TOKENKIND_LET ]          = "let",
    [ TOKENKIND_RETURN ]       = "return",
    [ TOKENKIND_FUNC ]         = "func",
    [ TOKENKIND_EXTERN ]       = "extern",
    [ TOKENKIND_IF ]           = "if",
    [ TOKENKIND_ELSE ]         = "else",
    [ TOKENKIND_WHILE ]        = "while",
    [ TOKENKIND_FOR ]          = "for",
    [ TOKENKIND_IN ]           = "in",
    [ TOKENKIND_TYPE ]         = "type",
    [ TOKENKIND_STRUCT ]       = "struct",
    [ TOKENKIND_UNION ]        = "union",
    [ TOKENKIND_AND ]          = "and",
    [ TOKENKIND_OR ]           = "or",
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
    [ TOKENKIND_DOUBLEPERIOD ] = "..",
    [ TOKENKIND_MODULO ]       = "%",
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
    [ EXPRESSIONKIND_EXTERN ]              = "EXTERN",
    [ EXPRESSIONKIND_CONDITIONAL ]         = "CONDITIONAL",
    [ EXPRESSIONKIND_ARRAYLITERAL ]        = "ARRAY LITERAL",
    [ EXPRESSIONKIND_ARRAYSUBSCRIPT ]      = "ARRAY SUBSCRIPT",
    [ EXPRESSIONKIND_FORLOOP ]             = "FOR LOOP",
    [ EXPRESSIONKIND_TYPEDECLARATION ]     = "TYPE DECLARATION",
    [ EXPRESSIONKIND_MEMBERACCESS ]        = "MEMBER ACCESS",
    [ EXPRESSIONKIND_COMPOUNDLITERAL ]     = "COMPOUND LITERAL",
    [ EXPRESSIONKIND_COMPOUNDDEFINITION ]  = "COMPOUND DEFINITION",
    [ EXPRESSIONKIND_TYPEIDENTIFIER ]      = "TYPE IDENTIFIER",
    [ EXPRESSIONKIND_POINTERTYPE ]         = "POINTER TYPE",
    [ EXPRESSIONKIND_ARRAYTYPE ]           = "ARRAY TYPE",
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
    [ BINARYOPERATION_MODULO ]       = "MODULO",
    [ BINARYOPERATION_AND ]          = "AND",
    [ BINARYOPERATION_OR ]           = "OR",
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
    [ TYPEKIND_FUNCTION ]  = "FUNCTION",
    [ TYPEKIND_COMPOUND ]  = "COMPOUND",
    [ TYPEKIND_POINTER ]   = "POINTER",
    [ TYPEKIND_REFERENCE ] = "REFERENCE",
    [ TYPEKIND_ARRAY ]     = "ARRAY",
    [ TYPEKIND_TYPE ]      = "TYPE",
    [ TYPEKIND_NAMED ]     = "NAMED",
    // [ TYPEKIND_INVALID ]   = "INVALID",
    [ TYPEKIND_TOINFER ]   = "TOINFER",
};

void debug_print_type( Type type )
{
    printf( "%s", type_kind_to_string[ type.kind ] );
    switch( type.kind )
    {
        case TYPEKIND_TOINFER:
        case TYPEKIND_VOID:
        case TYPEKIND_CHARACTER:
        case TYPEKIND_BOOLEAN:
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

        case TYPEKIND_COMPOUND:
        {
            // TODO: finish this
            printf( "(%s)", type.compound.is_struct ? "struct" : "union" );
            break;
        }

        case TYPEKIND_POINTER:
        {
            printf( "(" );
            debug_print_type( *type.pointer.base_type );
            printf( ")" );
            break;
        }

        case TYPEKIND_FUNCTION:
        {
            // UNIMPLEMENTED();
            break;
        }

        case TYPEKIND_NAMED:
        {
            printf( "(%s)", type.named.as_string );
            break;
        }

        case TYPEKIND_ARRAY:
        {
            printf( "(" );
            debug_print_type( *type.array.base_type );
            printf( "; %d)", type.array.length );
            break;
        }

        case TYPEKIND_TYPE:
        {
            printf( "(" );
            debug_print_type( *type.type.info );
            printf( ")" );
            break;
        }

        case TYPEKIND_REFERENCE:
        {
            printf( "(" );
            debug_print_type( *type.reference.base_type );
            printf( ")" );
            break;
        }

        default:
        {
            UNIMPLEMENTED();
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
    switch( expression->kind )
    {
        case EXPRESSIONKIND_INTEGER:
        {
            printf( "(%lld)", expression->integer );
            break;
        }

        case EXPRESSIONKIND_FLOAT:
        {
            printf( "(%lf)", expression->floating );
            break;
        }

        case EXPRESSIONKIND_BOOLEAN:
        {
            printf( "(%s)", expression->boolean ? "true" : "false" );
            break;
        }

        case EXPRESSIONKIND_IDENTIFIER:
        {
            printf( "(%s)", expression->identifier.as_string );
            break;
        }

        case EXPRESSIONKIND_STRING:
        {
            printf( "(\"%s\")", expression->string );
            break;
        }

        case EXPRESSIONKIND_CHARACTER:
        {
            printf( "(\'%c\')", expression->character );
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

            INDENT();
            printf( "right = " );
            expression_print( expression->binary.right );

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
            printf( "identifier = %s\n", expression->function_call.identifier_token.as_string );

            INDENT();
            printf( "args = {\n" );
            depth++;

            for( size_t i = 0; i < expression->function_call.arg_count; i++ )
            {
                INDENT();
                printf( "[%lld] = ", i );
                expression_print( &expression->function_call.args[ i ] );
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
            printf( "identifier = %s\n", expression->variable_declaration.identifier_token.as_string );

            INDENT();
            printf( "type = " );
            debug_print_type( expression->variable_declaration.variable_type );

            // debug_print_type( expression->variable_declaration.type );
            putchar( '\n' );

            INDENT();
            printf( "value = " );

            if( expression->variable_declaration.rvalue != NULL )
            {
                expression_print( expression->variable_declaration.rvalue );
            }
            else
            {
                printf( "UNINITIALIZED\n" );
            }

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
            // UNIMPLEMENTED();

            printf( " {\n" );
            depth++;

            INDENT();
            printf( "identifier = %s\n", expression->function_declaration.identifier_token.as_string );

            INDENT();
            printf( "return type = " );
            // expression_print( expression->function_declaration.return_type_rvalue );
            debug_print_type( expression->function_declaration.return_type );
            putchar( '\n' );

            for( int i = 0; i < expression->function_declaration.param_count; i++ )
            {
                Token param_identifier_token = expression->function_declaration.param_identifiers_tokens[ i ];
                // Expression param_type_rvalue = expression->function_declaration.param_type_rvalues[ i ];
                Type param_type = expression->function_declaration.param_types[ i ];

                INDENT();
                // printf( "param[%d] = %s: %d\n", i, param_identifier, param_type.kind );
                printf( "param[%d] = %s: ", i, param_identifier_token.as_string );
                // expression_print( &param_type_rvalue );
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
            printf( " {\n");
            depth++;
            INDENT();
            expression_print( expression->return_expression.rvalue );

            // putchar( '\n' );
            depth--;
            INDENT();
            printf( "}");
            break;
        }

        case EXPRESSIONKIND_ASSIGNMENT:
        {
            printf( " {\n" );
            depth++;

            INDENT();
            printf( "lvalue = " );
            expression_print( expression->assignment.lvalue );

            INDENT();
            printf( "value = " );

            expression_print( expression->assignment.rvalue );

            putchar( '\n' );
            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_EXTERN:
        {
            printf( " {\n" );
            depth++;

            INDENT();
            expression_print( expression->extern_expression.function );

            depth--;
            INDENT();
            printf( "}" );
            break;
        }

        case EXPRESSIONKIND_CONDITIONAL:
        {
            printf( "(%s) {\n", expression->conditional.is_loop ? "while" : "if" );
            depth++;

            INDENT();
            printf( "condition = " );
            expression_print( expression->conditional.condition );

            INDENT();
            printf( "true body = ");
            expression_print( expression->conditional.true_body );

            INDENT();
            printf( "false body = ");
            expression_print( expression->conditional.false_body );

            depth--;
            INDENT();
            printf( "}" );
            break;
        }

        case EXPRESSIONKIND_ARRAYLITERAL:
        {
            /* UNIMPLEMENTED(); */
            printf( " {\n" );
            depth++;
            // INDENT();

            /* printf( "type = "); */
            /* expression_print( expression->array.base_type_rvalue ); */

            for( int i = 0; i < expression->array_literal.count_initialized; i++ )
            {
                INDENT();
                printf( "[%d] = ", i );
                expression_print( &expression->array_literal.initialized_rvalues[ i ] );
            }

            depth--;
            INDENT();
            printf( "}" );
            break;
        }

        case EXPRESSIONKIND_ARRAYSUBSCRIPT:
        {
            printf( " {\n" );
            depth++;
            INDENT();

            printf( "lvalue = " );
            expression_print( expression->array_subscript.lvalue );

            INDENT();
            printf( "index = " );
            expression_print( expression->array_subscript.index_rvalue );

            depth--;
            INDENT();
            printf( "}" );
            break;
        }

        case EXPRESSIONKIND_FORLOOP:
        {
            printf( " {\n" );
            depth++;
            INDENT();

            printf( "iterator = %s: ", expression->for_loop.iterator_token.as_string );
            debug_print_type( expression->for_loop.iterator_type );
            putchar('\n');

            INDENT();
            printf( "iterable = " );
            expression_print( expression->for_loop.iterable_rvalue );

            printf( "\n" );
            INDENT();
            printf( "body = " );
            expression_print( expression->for_loop.body );

            depth--;
            printf( "\n");
            INDENT();
            printf( "}" );
            break;
        }

        case EXPRESSIONKIND_TYPEDECLARATION:
        {
            printf( " {\n" );

            depth++;
            INDENT();
            printf( "identifier = %s\n",
                    expression->type_declaration.identifier_token.as_string );

            INDENT();
            printf( "rvalue = " );
            expression_print( expression->type_declaration.rvalue );
            // debug_print_type( expression->type_declaration.type );
            // putchar( '\n' );

            depth--;
            INDENT();
            printf( "}" );
            break;
        }

        case EXPRESSIONKIND_COMPOUNDDEFINITION:
        {
            printf( "(%s) {\n",
                    expression->compound_definition.is_struct ? "struct" : "union" );

            depth++;
            for( int i = 0; i < expression->compound_definition.member_count; i++ )
            {
                char* member_identifier = expression->compound_definition.member_identifier_tokens[ i ].as_string;
                // Expression member_type_rvalue = expression->compound_definition.member_type_rvalues[ i ];
                Type member_type = expression->compound_definition.member_types[ i ];

                INDENT();
                printf( "member[%d] = %s: ", i, member_identifier );
                debug_print_type( member_type );
                putchar( '\n' );
            }
            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_MEMBERACCESS:
        {
            printf( " {\n" );
            depth++;
            INDENT();

            printf( "lvalue = " );
            expression_print( expression->member_access.lvalue );
            printf( "\n" );
            INDENT();

            printf( "member identifier = %s\n", expression->member_access.member_identifier_token.as_string );

            depth--;
            printf( "\n");
            INDENT();
            printf( "}" );
            break;
        }

        case EXPRESSIONKIND_COMPOUNDLITERAL:
        {
            printf( " {\n" );
            depth++;
            INDENT();

            printf( "type name = %s\n", expression->compound_literal.type_identifier_token.as_string );
            INDENT();

            printf( "initialized members = {\n" );
            depth++;
            for( int i = 0; i < expression->compound_literal.initialized_count; i++ )
            {
                char* member_identifier = expression->compound_literal.member_identifier_tokens[ i ].as_string;
                Expression initialized_member_rvalue = expression->compound_literal.initialized_member_rvalues[ i ];

                INDENT();
                printf( ".%s = ", member_identifier );
                expression_print( &initialized_member_rvalue );
            }
            depth--;
            INDENT();
            printf( "}\n" );

            depth--;
            INDENT();
            printf( "}" );
            break;
        }

        case EXPRESSIONKIND_TYPEIDENTIFIER:
        {
            printf( "(%s)", expression->type_identifier.token.as_string );
            break;
        }

        case EXPRESSIONKIND_POINTERTYPE:
        {
            printf( " {\n" );
            depth++;
            INDENT();

            printf( "base type = " );
            expression_print( expression->pointer_type.base_type_rvalue );
            depth--;
            INDENT();
            printf( "}" );

            break;
        }

        case EXPRESSIONKIND_ARRAYTYPE:
        {
            printf(" {\n");
            depth++;
            INDENT();

            printf( "base type = " );
            expression_print( expression->array_type.base_type_rvalue );

            INDENT();
            printf( "length = %d\n", expression->array_type.length );

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

    putchar('\n');
}
