#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "lvec.h"
#include "type.h"
#include "ast.h"
#include "error.h"
#include "symbol.h"
#include "globals.h"

static bool check_expression( AstNode* node, SymbolTable* st );
static bool check_type_definition( AstNode* type_definition, SymbolTable* st );

static bool check_compound( AstNodeCompound compound, SymbolTable* st  )
{
    size_t length = lvec_get_length( compound.nodes );
    bool result = true;
    for( size_t i = 0; i < length; i++ )
    {
        // TODO: think about if we should quit when we encounter the first error or not
        if ( !check_expression( compound.nodes[ i ], st ) )
        {
            result = false;
        }
    }

    return result;
}

static bool check_unary( AstNodeUnary unary, SymbolTable* st, Type* found_type )
{
    if( !check_expression( unary.operand, st ) )
    {
        return false;
    }

    switch( unary.operation )
    {
        case UNARYOPERATION_NOT:
        {
            if( unary.operand->type.kind != TYPEKIND_PRIMITIVE_BOOLEAN )
            {
                Error error = {
                    .kind = ERRORKIND_TYPEMISMATCH,
                    .offending_token = unary.operand->starting_token,
                    .type_mismatch = {
                        .expected = TYPE_BOOLEAN,
                        .found = unary.operand->type,
                    },
                };
                report_error( error );
                return false;
            }

            break;
        }

        case UNARYOPERATION_NEGATION:
        case UNARYOPERATION_DEREFERENCE:
        {
            UNIMPLEMENTED();
            break;
        }

        case UNARYOPERATION_ADDRESSOF:
        {
            *found_type = ( Type ){
                .kind = TYPEKIND_POINTER,
                .pointer.base = &unary.operand->type
            };
            break;
        }
    }

    *found_type = TYPE_BOOLEAN;
    return true;
}

static bool check_type_identifier( AstNodeIdentifier identifier, SymbolTable* st, Type* resulting_type )
{
    char* identifier_string = identifier.token.as_string;

    if     ( strcmp( identifier_string, "string" ) == 0 ) *resulting_type = type_wrap( TYPE_STRING );
    else if( strcmp( identifier_string, "char" ) == 0 )   *resulting_type = type_wrap( TYPE_CHARACTER );
    else if( strcmp( identifier_string, "bool" ) == 0 )   *resulting_type = type_wrap( TYPE_BOOLEAN );
    else if( strcmp( identifier_string, "int" ) == 0 )    *resulting_type = type_wrap( TYPE_INT );
    else if( strcmp( identifier_string, "uint" ) == 0 )   *resulting_type = type_wrap( TYPE_UINT );
    else if( strcmp( identifier_string, "float" ) == 0 )  *resulting_type = type_wrap( TYPE_FLOAT );
    else
    {
        Symbol* symbol = st_get( *st, identifier_string );
        if( symbol == NULL )
        {
            Error error = {
                .kind = ERRORKIND_UNDECLAREDSYMBOL,
                .offending_token = identifier.token,
            };
            report_error( error );
            return false;
        }

        if( symbol->type.kind != TYPEKIND_TYPE )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = identifier.token,
                .type_mismatch = {
                    .expected = type_wrap( TYPE_UNSPECIFIED ),
                    .found = symbol->type,
                },
            };
            report_error( error );
            return false;
        }

        *resulting_type = symbol->type;
    }

    return true;
}

static bool check_pointer_definition( AstNodePointerDefinition pointer_definition, SymbolTable* st, Type* resulting_type )
{
    if( !check_type_definition( pointer_definition.base_type_definition, st ) )
    {
        return false;
    }

    Type* base = octo_malloc( sizeof( Type ) );
    *base = type_unwrap( pointer_definition.base_type_definition->type );

    Type* definition = octo_malloc( sizeof( Type ) );
    *definition = ( Type ){
        .kind = TYPEKIND_POINTER,
        .pointer.base = base,
    };

    *resulting_type = ( Type ){
        .kind = TYPEKIND_TYPE,
        .type.definition = definition,
    };

    return true;
}

static bool check_type_definition( AstNode* type_definition, SymbolTable* st )
{
    switch( type_definition->kind )
    {
        case ASTNODEKIND_IDENTIFIER:
        {
            return check_type_identifier( type_definition->identifier, st, &type_definition->type );
        }

        case ASTNODEKIND_STRUCTDEFINITION:
        {
            UNIMPLEMENTED();
        }

        case ASTNODEKIND_ENUMDEFINITION:
        {
            UNIMPLEMENTED();
        }

        case ASTNODEKIND_ARRAYDEFINITION:
        {
            UNIMPLEMENTED();
        }

        case ASTNODEKIND_POINTERDEFINITION:
        {
            return check_pointer_definition( type_definition->pointer_definition, st, &type_definition->type);
        }

        default:
        {
            UNREACHABLE();
        }
    }

    return true;
}

static bool check_variable_declaration( AstNodeVariableDeclaration variable_declaration, SymbolTable* st )
{
    // variable declared must not already be in the symbol table
    Token identifier_token = variable_declaration.identifier_token;
    if( st_get( *st, identifier_token.as_string ) != NULL )
    {
        Error error = {
            .kind = ERRORKIND_SYMBOLREDECLARATION,
            .offending_token = identifier_token,
        };
        report_error( error );
        return false;
    }

    // get type if provided explicitly
    Type declared_type = TYPE_UNSPECIFIED;
    if( variable_declaration.type_definition != NULL )
    {
        if( !check_type_definition( variable_declaration.type_definition, st ) )
        {
            return false;
        }
        // printf("here\n");
        declared_type = type_unwrap( variable_declaration.type_definition->type );
    }

    // check if value is valid
    if( !check_expression( variable_declaration.value, st ) )
    {
        return false;
    }

    Type found_type = variable_declaration.value->type;
    if( declared_type.kind != TYPEKIND_UNSPECIFIED )
    {
        // check if declared type is same as found type
        if( !type_equals( declared_type, found_type ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = variable_declaration.value->starting_token,
                .type_mismatch = {
                    .expected = declared_type,
                    .found = found_type,
                },
            };
            report_error( error );
            return false;
        }
    }

    // add to symbol table
    Symbol symbol = {
        .key = identifier_token,
        .type = found_type
    };
    st_insert( st, symbol );
    return true;
}

static bool check_expression( AstNode* node, SymbolTable* st )
{
    switch( node->kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        {
            node->type = TYPE_STRING;
            break;
        }

        case ASTNODEKIND_CHARACTERLITERAL:
        {
            node->type = TYPE_CHARACTER;
            break;
        }

        case ASTNODEKIND_BOOLEANLITERAL:
        {
            node->type = TYPE_BOOLEAN;
            break;
        }

        case ASTNODEKIND_INTEGERLITERAL:
        {
            node->type = TYPE_INT;
            break;
        }

        case ASTNODEKIND_FLOATLITERAL:
        {
            node->type = TYPE_UINT;
            break;
        }

        case ASTNODEKIND_IDENTIFIER:
        {
            // identifier must exist
            Symbol* symbol = st_get( *st, node->identifier.token.as_string );
            if( symbol == NULL )
            {
                Error error = {
                    .kind = ERRORKIND_UNDECLAREDSYMBOL,
                    .offending_token = node->identifier.token,
                };
                report_error( error );
                return false;
            }

            node->type = symbol->type;
            break;
        }

        case ASTNODEKIND_COMPOUND:
        {
            return check_compound( node->compound, st );
        }

        case ASTNODEKIND_BINARY:
        {
            UNIMPLEMENTED();
            break;
        }

        case ASTNODEKIND_UNARY:
        {
            return check_unary( node->unary, st, &node->type );
        }

        case ASTNODEKIND_VARIABLEDECLARATION:
        {
            return check_variable_declaration( node->variable_declaration, st );
        }
    }

    return true;
}

bool check_ast( AstNode* ast )
{
    SymbolTable st;
    st_initialize( &st );

    bool is_valid = check_expression( ast, &st );
    return is_valid;
}
