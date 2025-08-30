#include "debug.h"
#include "lvec.h"
#include "type.h"
#include "ast.h"
#include "error.h"
#include "symbol.h"
#include <stdio.h>

static bool check_expression( AstNode* node, SymbolTable* st );

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
    switch( unary.operation )
    {
        case UNARYOPERATION_NOT:
        {
            if( !check_expression( unary.operand, st ) )
            {
                return false;
            }

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
        case UNARYOPERATION_ADDRESSOF:
        case UNARYOPERATION_DEREFERENCE:
        {
            UNIMPLEMENTED();
        }
    }

    *found_type = TYPE_BOOLEAN;
    return true;
}

static bool check_expression( AstNode* node, SymbolTable* st )
{
    switch( node->kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        {
            node->type = ( Type ){ .kind = TYPEKIND_PRIMITIVE_STRING };
            break;
        }

        case ASTNODEKIND_CHARACTERLITERAL:
        {
            node->type = ( Type ){ .kind = TYPEKIND_PRIMITIVE_CHARACTER };
            break;
        }

        case ASTNODEKIND_BOOLEANLITERAL:
        {
            node->type = ( Type ){ .kind = TYPEKIND_PRIMITIVE_BOOLEAN };
            break;
        }

        case ASTNODEKIND_INTEGERLITERAL:
        {
            // TODO: figure out what to do with literal integer types
            node->type = ( Type ){ .kind = TYPEKIND_PRIMITIVE_I32 };
            break;
        }

        case ASTNODEKIND_FLOATLITERAL:
        {
            // TODO: figure out what to do with literal floating point types
            node->type = ( Type ){ .kind = TYPEKIND_PRIMITIVE_F32 };
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
