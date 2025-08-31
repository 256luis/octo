#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "lvec.h"
#include "type.h"
#include "ast.h"
#include "error.h"
#include "symbol.h"
#include "globals.h"

static bool check_expression( AstNode* node, SymbolTable* st, Type type_hint );
static bool check_type_definition( AstNode* type_definition, SymbolTable* st );
static bool check_rvalue( AstNode* rvalue, SymbolTable* st, Type type_hint );

static bool check_compound( AstNodeCompound compound, SymbolTable* st, Type* found_type  )
{
    size_t length = lvec_get_length( compound.nodes );
    bool result = true;
    for( size_t i = 0; i < length; i++ )
    {
        // TODO: think about if we should quit when we encounter the first error or not
        if ( !check_expression( compound.nodes[ i ], st, TYPE_UNSPECIFIED ) )
        {
            result = false;
        }

        *found_type = compound.nodes[ i ]->type;
    }

    return result;
}

static bool check_unary( AstNodeUnary unary, SymbolTable* st, Type* found_type )
{
    if( !check_expression( unary.operand, st, TYPE_UNSPECIFIED ) )
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

            *found_type = TYPE_BOOLEAN;
            break;
        }

        case UNARYOPERATION_NEGATION:
        {
            if( !type_is_numeric( unary.operand->type ) )
            {
                Error error = {
                    .kind = ERRORKIND_EXPECTEDNUMERIC,
                    .offending_token = unary.operand->starting_token,
                };
                report_error( error );
                return false;
            }

            *found_type = unary.operand->type;
            break;
        }
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

    return true;
}

static bool check_type_identifier( AstNodeIdentifier identifier, SymbolTable* st, Type* resulting_type )
{
    char* identifier_string = identifier.token.as_string;

    if     ( strcmp( identifier_string, "string" ) == 0 ) *resulting_type = type_wrap_type( TYPE_STRING );
    else if( strcmp( identifier_string, "char" ) == 0 )   *resulting_type = type_wrap_type( TYPE_CHARACTER );
    else if( strcmp( identifier_string, "bool" ) == 0 )   *resulting_type = type_wrap_type( TYPE_BOOLEAN );
    else if( strcmp( identifier_string, "int" ) == 0 )    *resulting_type = type_wrap_type( TYPE_INT );
    else if( strcmp( identifier_string, "uint" ) == 0 )   *resulting_type = type_wrap_type( TYPE_UINT );
    else if( strcmp( identifier_string, "float" ) == 0 )  *resulting_type = type_wrap_type( TYPE_FLOAT );
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
                    .expected = type_wrap_type( TYPE_UNSPECIFIED ),
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
    *base = type_unwrap_type( pointer_definition.base_type_definition->type );

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

static bool check_array_definition( AstNodeArrayDefinition array_definition, SymbolTable* st, Type* resulting_type )
{
    if( !check_type_definition( array_definition.base_type_definition, st ) )
    {
        return false;
    }

    if( array_definition.length != NULL )
    {
        if( !check_expression( array_definition.length, st, TYPE_UNSPECIFIED ) )
        {
            return false;
        }

        if( !type_is_integer( array_definition.length->type ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = array_definition.length->starting_token,
                .type_mismatch = {
                    .expected = TYPE_INT,
                    .found = array_definition.length->type,
                },
            };
            report_error( error );
            return false;
        }

        // TODO: think about if we want to evaluate the length AST now ???
    }

    Type* base = octo_malloc( sizeof( Type ) );
    *base = type_unwrap_type( array_definition.base_type_definition->type );

    Type* definition = octo_malloc( sizeof( Type ) );
    *definition = ( Type ){
        .kind = TYPEKIND_ARRAY,
        .array = {
            .base = base,
            .length = array_definition.length
        },
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
            return check_array_definition( type_definition->array_definition, st, &type_definition->type );
        }

        case ASTNODEKIND_POINTERDEFINITION:
        {
            return check_pointer_definition( type_definition->pointer_definition, st, &type_definition->type );
        }

        default:
        {
            UNREACHABLE();
        }
    }

    return true;
}

static bool check_rvalue( AstNode* rvalue, SymbolTable* st, Type type_hint )
{
    if( !check_expression( rvalue, st, type_hint ) )
    {
        return false;
    }

    if( rvalue->type.kind == TYPEKIND_NONE )
    {
        Error error = {
            .kind = ERRORKIND_ILLEGALNONETYPE,
            .offending_token = rvalue->starting_token,
        };
        report_error( error );
        return false;
    }

    // if both unspecified, error
    if( rvalue->type.kind == TYPEKIND_UNSPECIFIED && type_hint.kind == TYPEKIND_UNSPECIFIED )
    {
        Error error = {
            .kind = ERRORKIND_CANNOTINFERTYPE,
            .offending_token = rvalue->starting_token,
        };
        report_error( error );
        return false;
    }
    // if only found type is unspecified
    else if( rvalue->type.kind == TYPEKIND_UNSPECIFIED )
    {
        rvalue->type = type_hint;
    }
    // if they are both specified
    else if( rvalue->type.kind != TYPEKIND_UNSPECIFIED && type_hint.kind != TYPEKIND_UNSPECIFIED )
    {
        // check if declared type is same as found type
        if( !type_equals( type_hint, rvalue->type ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = rvalue->starting_token,
                .type_mismatch = {
                    .expected = type_hint,
                    .found = rvalue->type,
                },
            };
            report_error( error );
            return false;
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
        declared_type = type_unwrap_type( variable_declaration.type_definition->type );
    }

    // check if value is valid
    assert( variable_declaration.value != NULL );
    if( !check_rvalue( variable_declaration.value, st, declared_type ) )
    {
        return false;
    }

    // add to symbol table
    Symbol symbol = {
        .key = identifier_token,
        .type = variable_declaration.value->type
    };
    st_insert( st, symbol );
    return true;
}

static bool check_array_literal( AstNodeArrayLiteral array_literal, SymbolTable* st, Type* found_type, Type type_hint )
{
    Type declared_type = TYPE_UNSPECIFIED;
    if( array_literal.base_type_definition != NULL )
    {
        if( !check_type_definition( array_literal.base_type_definition, st ) )
        {
            return false;
        }

        declared_type = array_literal.base_type_definition->type;
        declared_type = type_unwrap_type( declared_type );

        if( type_hint.kind != TYPEKIND_UNSPECIFIED &&
            !type_equals( declared_type, type_hint ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = array_literal.base_type_definition->starting_token,
                .type_mismatch = {
                    .expected = type_hint,
                    .found = declared_type
                },
            };
            report_error( error );
            return false;
        }
    }
    else
    {
        declared_type = type_hint;
    }

    if( array_literal.length != NULL &&
        !check_rvalue( array_literal.length, st, TYPE_INT ) )
    {
        return false;
    }

    Type base_type = TYPE_UNSPECIFIED;
    if( declared_type.kind != TYPEKIND_UNSPECIFIED )
    {
        base_type = type_unwrap_array( declared_type );
    }

    size_t length = lvec_get_length( array_literal.initialized_elements );
    Type inferred_type;
    if( length > 0 )
    {
        AstNode* first = array_literal.initialized_elements[ 0 ];
        if( !check_rvalue( first, st, base_type ) )
        {
            return false;
        }

        inferred_type = first->type;
    }

    for( size_t i = 0; i < length; i++ )
    {
        AstNode* expression = array_literal.initialized_elements[ i ];
        if( !check_rvalue( expression, st, base_type ) )
        {
            return false;
        }

        if( base_type.kind == TYPEKIND_UNSPECIFIED )
        {
            if( !type_equals( inferred_type, expression->type ) )
            {
                Error error = {
                    .kind = ERRORKIND_TYPEMISMATCH,
                    .offending_token = expression->starting_token,
                    .type_mismatch = {
                        .expected = inferred_type,
                        .found = expression->type
                    },
                };
                report_error( error );
                return false;
            }
        }
    }

    if( declared_type.kind != TYPEKIND_UNSPECIFIED )
    {
        *found_type = declared_type;
    }
    else
    {
        *found_type = type_wrap_array( inferred_type );
    }
    return true;
}

static bool check_expression( AstNode* node, SymbolTable* st, Type type_hint )
{
    node->type = TYPE_NONE;
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
            node->type = TYPE_FLOAT;
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
            return check_compound( node->compound, st, &node->type );
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

        case ASTNODEKIND_ARRAYLITERAL:
        {
            return check_array_literal( node->array_literal, st, &node->type, type_hint );
        }

        case ASTNODEKIND_UNINITIALIZED:
        {
            node->type = TYPE_UNSPECIFIED;
            break;
        }
    }

    return true;
}

bool check_ast( AstNode* ast )
{
    SymbolTable st;
    st_initialize( &st );

    bool is_valid = check_expression( ast, &st, TYPE_UNSPECIFIED );
    return is_valid;
}
