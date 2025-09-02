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

static bool ensure_identifier_free( Token identifier_token, SymbolTable* st )
{
    if( st_get( *st, identifier_token.as_string ) != NULL )
    {
        Error error = {
            .kind = ERRORKIND_SYMBOLREDECLARATION,
            .offending_token = identifier_token,
        };
        report_error( error );
        return false;
    }

    return true;
}

static bool check_compound( AstNodeCompound compound, SymbolTable* st, Type* found_type  )
{
    size_t length = lvec_get_length( compound.nodes );
    bool result = true;

    st_push_scope( st );
    for( size_t i = 0; i < length; i++ )
    {
        // TODO: think about if we should quit when we encounter the first error or not
        if ( !check_expression( compound.nodes[ i ], st, TYPE_UNSPECIFIED ) )
        {
            result = false;
            continue;
        }

        *found_type = compound.nodes[ i ]->type;
    }
    st_pop_scope( st );

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

static bool check_struct_definition( AstNodeStructDefinition struct_definition, SymbolTable* st, Type* resulting_type )
{
    SymbolTable* struct_st = octo_malloc( sizeof( SymbolTable ) );
    st_initialize( struct_st );

    size_t member_count = lvec_get_length( struct_definition.member_type_definitions );
    for( size_t i = 0; i < member_count; i++ )
    {
        if( !check_type_definition( struct_definition.member_type_definitions[ i ], st ) )
        {
            return false;
        }

        Symbol member_symbol = {
            .key = struct_definition.member_identifiers[ i ],
            .type = type_unwrap_type( struct_definition.member_type_definitions[ i ]->type )
        };
        st_insert( struct_st, member_symbol );
    }

    Type definition = {
        .kind = TYPEKIND_STRUCT,
        .structure = {
            .members = struct_st,
            .member_count = member_count,
        },
    };

    *resulting_type = type_wrap_type( definition );
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
            return check_struct_definition( type_definition->struct_definition, st, &type_definition->type );
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

    if( rvalue->type.kind == TYPEKIND_NONE && type_hint.kind != TYPEKIND_NONE )
    {
        Error error = {
            .kind = ERRORKIND_ILLEGALNONETYPE,
            .offending_token = rvalue->starting_token,
        };
        report_error( error );
        return false;
    }

    if( rvalue->type.kind == TYPEKIND_TYPE )
    {
        Error error = {
            .kind = ERRORKIND_ILLEGALTYPETYPE,
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
    if( !ensure_identifier_free( identifier_token, st ) )
    {
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

    if( variable_declaration.value->type.kind == TYPEKIND_UNSPECIFIED &&
        declared_type.kind == TYPEKIND_UNSPECIFIED )
    {
        Error error = {
            .kind = ERRORKIND_CANNOTINFERTYPE,
            .offending_token = variable_declaration.value->starting_token,
        };
        report_error( error );
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
    if( base_type.kind == TYPEKIND_UNSPECIFIED && length > 0 )
    {
        AstNode* first = array_literal.initialized_elements[ 0 ];
        if( !check_rvalue( first, st, base_type ) )
        {
            return false;
        }

        base_type = first->type;
    }

    for( size_t i = 0; i < length; i++ )
    {
        AstNode* expression = array_literal.initialized_elements[ i ];
        if( !check_rvalue( expression, st, base_type ) )
        {
            return false;
        }
    }


    if( base_type.kind == TYPEKIND_UNSPECIFIED )
    {
        *found_type = TYPE_UNSPECIFIED;
    }
    else if( declared_type.kind != TYPEKIND_UNSPECIFIED )
    {
        *found_type = declared_type;
    }
    else
    {
        *found_type = type_wrap_array( base_type );
    }
    return true;
}

static bool check_type_declaration( AstNodeTypeDeclaration type_declaration, SymbolTable* st )
{
    Token identifier_token = type_declaration.identifier_token;
    if( !ensure_identifier_free( identifier_token, st ) )
    {
        return false;
    }

    if( !check_type_definition( type_declaration.type_definition, st ) )
    {
        return false;
    }

    // this chain of members is CURSED!!!!
    type_declaration.type_definition->type.type.definition->identifier_token = octo_malloc( sizeof( Token ) );
    *type_declaration.type_definition->type.type.definition->identifier_token = identifier_token;

    Symbol symbol = {
        .key = identifier_token,
        .type = type_declaration.type_definition->type,
    };

    st_insert( st, symbol );

    return true;
}

static bool check_struct_literal( AstNodeStructLiteral struct_literal, SymbolTable* st, Type* found_type, Type type_hint )
{
    Type type = TYPE_UNSPECIFIED;
    if( struct_literal.type_definition != NULL )
    {
        if( !check_type_definition( struct_literal.type_definition, st ) )
        {
            return false;
        }

        type = type_unwrap_type( struct_literal.type_definition->type );

        if( type_hint.kind != TYPEKIND_UNSPECIFIED &&
            !type_equals( type, type_hint ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = struct_literal.type_definition->starting_token,
                .type_mismatch = {
                    .expected = type_hint,
                    .found = type
                },
            };
            report_error( error );
            return false;
        }
    }
    else
    {
        type = type_hint;
    }

    if( type.kind == TYPEKIND_UNSPECIFIED && type_hint.kind == TYPEKIND_UNSPECIFIED )
    {
        *found_type = TYPE_UNSPECIFIED;
        return true;
    }

    SymbolTable* struct_st = type.structure.members;

    // check initialized values
    size_t initialized_count = lvec_get_length( struct_literal.initialized_member_values );
    for( size_t i = 0; i < initialized_count; i++ )
    {
        Token identifier_token = struct_literal.initialized_member_tokens[ i ];
        Symbol* member_symbol = st_get( *struct_st, identifier_token.as_string );
        if( member_symbol == NULL )
        {
            Error error = {
                .kind = ERRORKIND_UNDECLAREDSYMBOL,
                .offending_token = identifier_token,
            };
            report_error( error );
            return false;
        }

        AstNode* initialized_member_value = struct_literal.initialized_member_values[ i ];
        if( !check_rvalue( initialized_member_value, st, member_symbol->type ) )
        {
            return false;
        }
    }

    *found_type = type;
    return true;
}

static bool check_routine_definition( AstNode* node, SymbolTable* st, Token routine_identifier_token )
{
    if( node->kind != ASTNODEKIND_ROUTINEDEFINITION )
    {
        Error error = {
            .kind = ERRORKIND_NOTAROUTINE,
            .offending_token = node->starting_token,
        };
        report_error( error );
        return false;
    }

    AstNodeRoutineDefinition routine_definition = node->routine_definition;
    bool is_func = routine_definition.is_func;

    // TODO: perform purity analysis for function types

    // check the return type
    Type* return_type = octo_malloc( sizeof( Type ) );
    *return_type = TYPE_NONE;
    if( is_func )
    {
        // functions must return a value
        if( routine_definition.return_type_definition == NULL )
        {
            Error error = {
                .kind = ERRORKIND_MISSINGTYPE,
                .offending_token = routine_definition.body->starting_token,
                .note = "functions must return a value"
            };
            report_error( error );
            return false;
        }

        if( !check_type_definition( routine_definition.return_type_definition, st ) )
        {
            return false;
        }

        *return_type = type_unwrap_type( routine_definition.return_type_definition->type );
    }
    // procedures must not return a value
    else if( routine_definition.return_type_definition != NULL )
    {
        Error error = {
            .kind = ERRORKIND_PROCEDUREWITHRETURN,
            .offending_token = routine_definition.body->starting_token,
        };
        report_error( error );
        return false;
    }

    // check params
    size_t param_count = lvec_get_length( routine_definition.param_type_definitions );
    Type* param_types = lvec_new( Type );
    for( size_t i = 0; i < param_count; i++ )
    {
        Token param_identifier = routine_definition.param_identifier_tokens[ i ];
        AstNode* param_type_definition = routine_definition.param_type_definitions[ i ];

        if( !ensure_identifier_free( param_identifier, st ) )
        {
            return false;
        }

        if( !check_type_definition( param_type_definition, st ) )
        {
            return false;
        }

        Type param_type = type_unwrap_type( param_type_definition->type );
        lvec_append_aggregate( param_types, param_type );
    }

    Type routine_type = {
        .kind = TYPEKIND_ROUTINE,
        .routine = {
            .is_func = is_func,
            .return_type = return_type,
            .param_types = param_types,
        }
    };

    Symbol routine_symbol = {
        .key = routine_identifier_token,
        .type = routine_type,
    };
    st_insert( st, routine_symbol );

    st_push_scope( st );

    // insert params to symbol table
    for( size_t i = 0; i < param_count; i++ )
    {
        Token param_identifier = routine_definition.param_identifier_tokens[ i ];
        Type param_type = param_types[ i ];

        Symbol param_symbol = {
            .key = param_identifier,
            .type = param_type,
        };
        st_insert( st, param_symbol );
    }

    if( !check_rvalue( routine_definition.body, st, *return_type ) )
    {
        st_pop_scope( st );
        return false;
    }

    st_pop_scope( st );

    return true;
}

static bool check_routine_declaration( AstNodeRoutineDeclaration routine_declaration, SymbolTable* st )
{
    Token identifier_token = routine_declaration.identifier_token;
    if( !ensure_identifier_free( identifier_token, st ) )
    {
        return false;
    }

    if( !check_routine_definition( routine_declaration.routine_definition, st, identifier_token ) )
    {
        return false;
    }

    return true;
}

static bool check_routine_call( AstNodeRoutineCall routine_call, SymbolTable* st, Type* found_type )
{
    if( !check_expression( routine_call.routine, st, TYPE_UNSPECIFIED ) )
    {
        return false;
    }

    // ensure that the routine being called is actually a routine
    if( routine_call.routine->type.kind != TYPEKIND_ROUTINE )
    {
        Error error = {
            .kind = ERRORKIND_NOTAROUTINE,
            .offending_token = routine_call.routine->starting_token,
        };
        report_error( error );
        return false;
    }

    TypeRoutine routine_type = routine_call.routine->type.routine;
    *found_type = *routine_type.return_type;

    // check if the args match
    size_t arg_count = lvec_get_length( routine_call.args );
    size_t routine_param_count = lvec_get_length(routine_type.param_types);
    if( arg_count != routine_param_count )
    {
        Error error = {
            .kind = ERRORKIND_INCORRECTARGCOUNT,
            .offending_token = routine_call.routine->starting_token,
            .incorrect_arg_count = {
                .expected = routine_param_count,
                .found = arg_count,
            },
        };
        report_error( error );
        return false;
    }

    for( size_t i = 0; i < arg_count; i++ )
    {
        Type expected_type = routine_type.param_types[ i ];
        AstNode* arg = routine_call.args[ i ];
        if( !check_rvalue( arg, st, expected_type ) )
        {
            return false;
        }
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

        case ASTNODEKIND_TYPEDECLARATION:
        {
            return check_type_declaration( node->type_declaration, st );
        }

        case ASTNODEKIND_STRUCTLITERAL:
        {
            return check_struct_literal( node->struct_literal, st, &node->type, type_hint );
        }

        case ASTNODEKIND_ROUTINEDECLARATION:
        {
            return check_routine_declaration( node->routine_declaration, st );
        }

        case ASTNODEKIND_ROUTINECALL:
        {
            return check_routine_call( node->routine_call, st, &node->type );
        }

        case ASTNODEKIND_STRUCTDEFINITION:
        case ASTNODEKIND_ENUMDEFINITION:
        case ASTNODEKIND_ARRAYDEFINITION:
        case ASTNODEKIND_POINTERDEFINITION:
        {
            return check_type_definition( node, st );
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
