#include <assert.h>
#include <stdio.h>
#include "debug.h"
#include "lvec.h"
#include "ast.h"
#include "operation.h"
#include "type.h"
#include "symbol.h"

#define IDENTIFIER_PREFIX "IDENT_"
#define TAB_WIDTH 4

static int depth = 0;
static int temp_counter = 0;

static void codegen_temp()
{
    printf( "temp%d", temp_counter );
    temp_counter++;
}

static void newline()
{
    putchar('\n');
    for( int i = 0; i < depth * TAB_WIDTH; i++ )
    {
        putchar(' ');
    }
}

static void codegen_expression( AstNode* node );

static void codegen_compound( AstNodeCompound compound )
{
    size_t length = lvec_get_length( compound.nodes );
    temp_counter = 0;
    printf( "{" );
    depth++;
    for( size_t i = 0; i < length; i++ )
    {
        newline();
        codegen_expression( compound.nodes[ i ] );
    }
    depth--;
    newline();
    printf( "}" );
}

static void codegen_type( Type type )
{
    if( type.identifier_token != NULL )
    {
        printf( IDENTIFIER_PREFIX "%s", type.identifier_token->as_string );
        return;
    }

    switch( type.kind )
    {
        case TYPEKIND_UNSPECIFIED:
        {
            UNREACHABLE();
        }

        case TYPEKIND_NONE:
        {
            printf( "void" );
            break;
        }

        case TYPEKIND_PRIMITIVE_STRING:
        {
            UNIMPLEMENTED();
        }

        case TYPEKIND_PRIMITIVE_CHARACTER:
        {
            printf( "char" );
            break;
        }

        case TYPEKIND_PRIMITIVE_BOOLEAN:
        {
            printf( "uint8_t" );
            break;
        }

        case TYPEKIND_PRIMITIVE_INT:
        {
            printf( "int64_t" );
            break;
        }

        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            printf( "double" );
            break;
        }

        case TYPEKIND_ARRAY:
        {
            UNIMPLEMENTED();
        }

        case TYPEKIND_POINTER:
        {
            codegen_type( *type.pointer.base );
            printf( "*" );
            break;
        }

        case TYPEKIND_TYPE:
        {
            UNIMPLEMENTED();
        }

        case TYPEKIND_STRUCT:
        {
            printf( "struct {" );
            for( size_t i = 0; i < type.structure.member_count; i++ )
            {
                Symbol member_symbol = *type.structure.members[ i ].symbols;
                codegen_type( member_symbol.type );
                printf( " %s, ", member_symbol.key.as_string );
            }
            break;
        }

        case TYPEKIND_ENUM:
        {
            UNIMPLEMENTED();
        }

        case TYPEKIND_ROUTINE:
        {
            UNIMPLEMENTED();
        }
    }
}

static void codegen_routine_declaration( AstNodeRoutineDeclaration routine_declaration )
{
    AstNodeRoutineDefinition routine_definition = routine_declaration.routine_definition->routine_definition;
    TypeRoutine routine_type = routine_declaration.routine_definition->type.routine;

    Type return_type = *routine_type.return_type;
    codegen_type( return_type );

    printf( " " IDENTIFIER_PREFIX "%s(", routine_declaration.identifier_token.as_string );

    size_t param_count = lvec_get_length( routine_definition.param_identifier_tokens );
    for( size_t i = 0; i < param_count; i++ )
    {
        Type param_type = routine_type.param_types[ i ];
        codegen_type( param_type );

        char* param_identifier = routine_definition.param_identifier_tokens[ i ].as_string;
        printf( " " IDENTIFIER_PREFIX "%s, ", param_identifier );
    }

    printf( ") {" );
    codegen_expression( routine_definition.body );
    printf( "}" );
}

static void codegen_binary_operation( BinaryOperation operation )
{
    switch( operation )
    {
        case BINARYOPERATION_ADDITION:             printf( "+" ); break;
        case BINARYOPERATION_SUBTRACTION:          printf( "-" ); break;
        case BINARYOPERATION_MULTIPLICATION:       printf( "*" ); break;
        case BINARYOPERATION_DIVISION:             printf( "/" ); break;
        case BINARYOPERATION_MODULO:               printf( "%%" ); break;
        case BINARYOPERATION_EQUALTO:              printf( "==" ); break;
        case BINARYOPERATION_NOTEQUALTO:           printf( "!=" ); break;
        case BINARYOPERATION_LESSTHAN:             printf( "<" ); break;
        case BINARYOPERATION_LESSTHANOREQUALTO:    printf( "+=" ); break;
        case BINARYOPERATION_GREATERTHAN:          printf( ">" ); break;
        case BINARYOPERATION_GREATERTHANOREQUALTO: printf( ">=" ); break;
        case BINARYOPERATION_AND:                  printf( "&&" ); break;
        case BINARYOPERATION_OR:                   printf( "||" ); break;
    }
}

static void codegen_binary( AstNodeBinary binary, Type type )
{
    codegen_type( binary.left->type );
    printf( " " );
    codegen_temp();
    printf( ";" );
    newline();

    printf( "{" );
    depth++;
    newline();
    switch( binary.left->kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        case ASTNODEKIND_CHARACTERLITERAL:
        case ASTNODEKIND_BOOLEANLITERAL:
        case ASTNODEKIND_INTEGERLITERAL:
        case ASTNODEKIND_FLOATLITERAL:
        case ASTNODEKIND_IDENTIFIER:
        {
            codegen_type( binary.left->type );
            printf( " left = ");
            codegen_expression( binary.left );
            printf( ";" );
            newline();
            break;
        }

        default:
        {
            codegen_expression( binary.left );
            break;
        }
    }

    switch( binary.right->kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        case ASTNODEKIND_CHARACTERLITERAL:
        case ASTNODEKIND_BOOLEANLITERAL:
        case ASTNODEKIND_INTEGERLITERAL:
        case ASTNODEKIND_FLOATLITERAL:
        case ASTNODEKIND_IDENTIFIER:
        {
            codegen_type( binary.right->type );
            printf( " right = ");
            codegen_expression( binary.right );
            printf( ";" );
            newline();
            break;
        }

        default:
        {
            codegen_expression( binary.right );
            break;
        }
    }

    printf( "result = left ");
    codegen_binary_operation( binary.operation );
    printf( " right;" );

    depth--;
    newline();
    printf( "}" );
    newline();
}

static void codegen_expression( AstNode* node )
{
    switch( node->kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        {
            printf( "\"%s\"", node->string_literal.token.as_string );
            break;
        }

        case ASTNODEKIND_CHARACTERLITERAL:
        {
            printf( "%d", node->character_literal.token.as_string[ 0 ] );
            break;
        }

        case ASTNODEKIND_BOOLEANLITERAL:
        {
            printf( "%d", node->boolean_literal.boolean );
            break;
        }

        case ASTNODEKIND_INTEGERLITERAL:
        {
            printf( "%s", node->integer_literal.token.as_string );
            break;
        }

        case ASTNODEKIND_FLOATLITERAL:
        {
            printf( "%s", node->float_literal.token.as_string );
            break;
        }

        case ASTNODEKIND_IDENTIFIER:
        {
            printf( IDENTIFIER_PREFIX "%s", node->identifier.token.as_string );
            break;
        }

        case ASTNODEKIND_COMPOUND:
        {
            codegen_compound( node->compound );
            break;
        }

        case ASTNODEKIND_ROUTINEDECLARATION:
        {
            codegen_routine_declaration( node->routine_declaration );
            break;
        }

        case ASTNODEKIND_BINARY:
        {
            codegen_binary( node->binary, node->type );
        }
    }
}

void codegen( AstNode* ast )
{
    assert( ast->kind == ASTNODEKIND_MODULE );

    size_t length = lvec_get_length( ast->module.nodes );
    for( size_t i = 0; i < length; i++ )
    {
        AstNode* node = ast->module.nodes[ i ];
        codegen_expression( node );
        printf( "\n" );
    }
}
