#include <stddef.h>
#include <stdio.h>
#include "ast.h"
#include "debug.h"
#include "lvec.h"

#define TAB_WIDTH 4

static int depth = 0;

static char* binary_operation_string[] = {
    [ BINARYOPERATION_ADDITION ]             = "ADDITION",
    [ BINARYOPERATION_SUBTRACTION ]          = "SUBTRACTION",
    [ BINARYOPERATION_MULTIPLICATION ]       = "MULTIPLICATION",
    [ BINARYOPERATION_DIVISION ]             = "DIVISION",
    [ BINARYOPERATION_MODULO ]               = "MODULO",
    [ BINARYOPERATION_EQUALTO ]              = "EQUAL TO",
    [ BINARYOPERATION_NOTEQUALTO ]           = "NOT EQUAL TO",
    [ BINARYOPERATION_LESSTHAN ]             = "LESS THAN",
    [ BINARYOPERATION_LESSTHANOREQUALTO ]    = "LESS THAN OR EQUAL TO",
    [ BINARYOPERATION_GREATERTHAN ]          = "GREATER THAN",
    [ BINARYOPERATION_GREATERTHANOREQUALTO ] = "GREATER THAN OR EQUAL TO",
    [ BINARYOPERATION_AND ]                  = "AND",
    [ BINARYOPERATION_OR ]                   = "OR",
};

static char* unary_operation_string[] = {
    [ UNARYOPERATION_NOT ]         = "NOT",
    [ UNARYOPERATION_NEGATION ]    = "NEGATION",
    [ UNARYOPERATION_ADDRESSOF ]   = "ADDRESSOF",
    [ UNARYOPERATION_DEREFERENCE ] = "DEREFERENCE",
};

static void newline()
{
    putchar('\n');
    for( int i = 0; i < depth * TAB_WIDTH; i++ )
    {
        putchar(' ');
    }
}

void ast_node_print( AstNode node )
{
    newline();
    switch( node.kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        {
            printf( "STRING(\"%s\")", node.string_literal.token.as_string );
            break;
        }

        case ASTNODEKIND_CHARACTERLITERAL:
        {
            printf( "CHARACTER(\'%s\')", node.character_literal.token.as_string );
            break;
        }

        case ASTNODEKIND_INTEGERLITERAL:
        {
            printf( "INTEGER(%zu)", node.integer_literal.integer );
            break;
        }

        case ASTNODEKIND_FLOATLITERAL:
        {
            printf( "FLOAT(%f)", node.float_literal.floating );
            break;
        }

        case ASTNODEKIND_BOOLEANLITERAL:
        {
            printf( "BOOLEAN(%s)",
                    node.boolean_literal.boolean ? "true" : "false" );
            break;
        }

        case ASTNODEKIND_IDENTIFIER:
        {
            printf( "IDENTIFIER(\"%s\")", node.identifier.token.as_string );
            break;
        }

        case ASTNODEKIND_COMPOUND:
        {

            printf( "COMPOUND:" );
            depth++;


            for( size_t i = 0; i < lvec_get_length( node.compound.nodes ); i++)
            {
                AstNode* n = node.compound.nodes[i];
                ast_node_print( *n );
            }

            depth--;
            break;
        }

        case ASTNODEKIND_BINARY:
        {

            printf( "BINARY:" );
            depth++;
            newline();

            printf( "operation: %s", binary_operation_string[ node.binary.operation ] );
            newline();

            printf( "left:");
            depth++;


            ast_node_print( *node.binary.left );
            depth--;
            newline();

            printf( "right:");
            depth++;


            ast_node_print( *node.binary.right );
            depth--;

            depth--;
            break;
        }

        case ASTNODEKIND_UNARY:
        {

            printf( "UNARY:" );
            depth++;


            printf( "operation: %s", unary_operation_string[ node.unary.operation ] );
            newline();

            printf( "operand:" );
            depth++;
            ast_node_print( *node.unary.operand );
            depth--;

            depth--;
            break;
        }

        case ASTNODEKIND_SUBSCRIPT:
        {

            printf( "SUBSCRIPT:" );
            depth++;
            newline();

            printf( "target:" );
            depth++;


            ast_node_print( *node.subscript.target );

            depth--;
            newline();

            printf( "index:" );
            depth++;


            ast_node_print( *node.subscript.index );
            depth--;

            depth--;
            break;
        }

        case ASTNODEKIND_FUNCTIONCALL:
        {

            printf( "FUNCTION CALL:" );
            depth++;
            newline();

            printf( "function:" );
            depth++;


            ast_node_print( *node.function_call.function );
            depth--;

            size_t arg_count = lvec_get_length( node.function_call.args );
            if( arg_count > 0 )
            {
                newline();
                printf( "args:" );
                depth++;

                for( size_t i = 0; i < arg_count; i++ )
                {
                    AstNode* arg = node.function_call.args[i];
                    // printf( "- ");
                    ast_node_print( *arg );

                }

                depth--;
            }

            depth--;
            break;
        }

        case ASTNODEKIND_VARIABLEDECLARATION:
        {

            printf( "VARIABLE DECLARATION:" );
            depth++;
            newline();

            printf( "identifier: %s", node.variable_declaration.identifier_token.as_string );

            if( node.variable_declaration.type_definition != NULL )
            {
                newline();
                printf( "type:" );
                depth++;


                ast_node_print( *node.variable_declaration.type_definition );
                depth--;
            }

            if( node.variable_declaration.value != NULL )
            {
                newline();
                printf( "value:" );
                depth++;

                ast_node_print( *node.variable_declaration.value );
                depth--;
                newline();
            }

            depth--;
            break;
        }

        case ASTNODEKIND_TYPEDECLARATION:
        {

            printf( "TYPE DECLARATION:" );
            depth++;
            newline();

            printf( "identifier: %s", node.type_declaration.identifier_token.as_string );
            newline();

            printf( "type:" );
            depth++;

            ast_node_print( *node.type_declaration.type_definition );
            depth--;

            depth--;
            break;
        }

        case ASTNODEKIND_STRUCTDEFINITION:
        {

            printf( "STRUCT: " );
            depth++;
            newline();

            size_t member_count = lvec_get_length( node.struct_definition.member_identifiers );
            for( size_t i = 0; i < member_count; i++ )
            {
                printf( "%s: ", node.struct_definition.member_identifiers[ i ].as_string );
                depth++;
                ast_node_print( *node.struct_definition.member_types[ i ] );
                depth--;
                newline();
            }

            depth--;
            break;
        }

        case ASTNODEKIND_ENUMDEFINITION:
        {

            printf( "ENUM: " );
            depth++;
            newline();

            size_t variant_count = lvec_get_length( node.enum_definition.variant_names );
            for( size_t i = 0; i < variant_count; i++ )
            {
                printf( "%s", node.enum_definition.variant_names[ i ].as_string );
                newline();
            }

            depth--;
            break;
        }



        case ASTNODEKIND_ROUTINEDECLARATION:
        {
            printf( "ROUTINE DECLARATION:" );
            depth++;
            newline();

            printf( "identifier: %s", node.routine_declaration.identifier_token.as_string );
            newline();

            printf( "definition:" );
            depth++;

            ast_node_print( *node.routine_declaration.routine_definition );
            depth--;

            depth--;
            break;
        }

        case ASTNODEKIND_ROUTINEDEFINITION:
        {
            printf( "ROUTINE DEFINITION:" );
            depth++;
            newline();

            printf( "kind: %s",
                    node.routine_definition.is_func ? "func" : "proc" );
            newline();

            printf( "return type:" );
            depth++;
            ast_node_print( *node.routine_definition.return_type_definition );
            depth--;
            newline();

            size_t param_count = lvec_get_length( node.routine_definition.param_type_definitions );
            if( param_count > 0 )
            {
                printf( "params:");
                depth++;

                for( size_t i = 0; i < param_count; i++ )
                {
                    newline();

                    char* identifier = node.routine_definition.param_identifier_tokens[ i ].as_string;
                    printf( "identifier: %s", identifier );
                    newline();
                    printf( "type:" );
                    depth++;
                    ast_node_print( *node.routine_definition.param_type_definitions[ i ] );
                    depth--;
                }

                depth--;
                newline();
            }

            printf( "body:" );
            depth++;
            ast_node_print( *node.routine_definition.body);
            depth--;

            depth--;
            break;
        }

        case ASTNODEKIND_CONDITIONAL:
        {
            printf( "CONDITIONAL" );
            depth++;
            newline();

            printf( "condition:" );
            depth++;

            ast_node_print( *node.conditional.condition );
            depth--;
            newline();

            printf( "body:" );
            depth++;

            ast_node_print( *node.conditional.body );
            depth--;
            newline();

            depth--;
            break;
        }
    }
}
