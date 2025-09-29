#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "ast.h"
#include "debug.h"
#include "lvec.h"
#include "operation.h"
#include "symbol.h"
#include "type.h"
#include "globals.h"
#include "interpreter.h"

// TODO: actual string type!!!!!

AstNode walk_node( AstNode* node, InterpreterContext* ctx );

void walk_echo( AstNodeEcho echo, InterpreterContext* ctx )
{
    AstNode to_echo = walk_node( echo.value, ctx );

    switch( to_echo.kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        {
            printf( "%s", to_echo.string_literal.token.as_string );
            break;
        }

        case ASTNODEKIND_CHARACTERLITERAL:
        {
            printf( "%s", to_echo.character_literal.token.as_string );
            break;
        }

        case ASTNODEKIND_INTEGERLITERAL:
        {
            printf( "%ld", to_echo.integer_literal.integer );
            break;
        }

        case ASTNODEKIND_FLOATLITERAL:
        {
            printf( "%lf", to_echo.float_literal.floating );
            break;
        }

        case ASTNODEKIND_BOOLEANLITERAL:
        {
            printf( "%s", to_echo.boolean_literal.boolean ? "true" : "false" );
            break;
        }

        case ASTNODEKIND_ARRAYLITERAL:
        {
            type_print( to_echo.type );
            printf( ".[" );

            for( int64_t i = 0; i < to_echo.type.array.length; i++ )
            {
                AstNodeEcho echo = {
                   .value = to_echo.array_literal.initialized_elements[i]
                };
                walk_echo( echo, ctx );
                printf( ", " );
            }
            printf( "]" );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }
}

AstNode walk_compound( AstNodeCompound compound, InterpreterContext* ctx )
{
    size_t length = lvec_get_length( compound.nodes );
    AstNode last_node;

    st_push_scope( ctx->st );
    for( size_t i = 0; i < length; i++ )
    {
        last_node = walk_node( compound.nodes[ i ], ctx );
    }
    st_pop_scope( ctx->st );

    return last_node;
}

void walk_variable_declaration( AstNodeVariableDeclaration variable_declaration, InterpreterContext* ctx )
{
    Symbol symbol = {
        .key = variable_declaration.identifier_token,
        .type = variable_declaration.value->type,
        .value = octo_malloc( sizeof( AstNode ) ),
    };

    AstNode value = walk_node( variable_declaration.value, ctx );
    *symbol.value = value;

    st_insert( ctx->st, symbol );
}

AstNode walk_binary( AstNodeBinary binary, InterpreterContext* ctx )
{
    AstNode left_result = walk_node( binary.left, ctx );
    AstNode right_result = walk_node( binary.right, ctx );
    AstNode result;

    switch( left_result.type.kind )
    {
        case TYPEKIND_PRIMITIVE_INT:
        {
            int64_t left_integer = left_result.integer_literal.integer;
            int64_t right_integer = right_result.integer_literal.integer;

            result = ( AstNode ){
                .kind = ASTNODEKIND_INTEGERLITERAL,
                .type = ( Type ){
                    .kind = TYPEKIND_PRIMITIVE_INT
                },
            };

            switch( binary.operation )
            {
                case BINARYOPERATION_ADDITION:       result.integer_literal.integer = left_integer + right_integer; return result;
                case BINARYOPERATION_SUBTRACTION:    result.integer_literal.integer = left_integer - right_integer; return result;
                case BINARYOPERATION_MULTIPLICATION: result.integer_literal.integer = left_integer * right_integer; return result;
                case BINARYOPERATION_DIVISION:       result.integer_literal.integer = left_integer / right_integer; return result;
                case BINARYOPERATION_MODULO:         result.integer_literal.integer = left_integer % right_integer; return result;
                default: // do nothing
            }

            // if this point is reached, then it must be a boolean operation
            result = ( AstNode ){
                .kind = ASTNODEKIND_BOOLEANLITERAL,
                .type = ( Type ){
                    .kind = TYPEKIND_PRIMITIVE_BOOLEAN
                },
            };

            switch( binary.operation )
            {
                case BINARYOPERATION_EQUALTO:              result.boolean_literal.boolean = left_integer == right_integer; return result;
                case BINARYOPERATION_NOTEQUALTO:           result.boolean_literal.boolean = left_integer != right_integer; return result;
                case BINARYOPERATION_LESSTHAN:             result.boolean_literal.boolean = left_integer < right_integer; return result;
                case BINARYOPERATION_LESSTHANOREQUALTO:    result.boolean_literal.boolean = left_integer <= right_integer; return result;
                case BINARYOPERATION_GREATERTHAN:          result.boolean_literal.boolean = left_integer > right_integer; return result;
                case BINARYOPERATION_GREATERTHANOREQUALTO: result.boolean_literal.boolean = left_integer >= right_integer; return result;
                default: UNREACHABLE();
            }
        }

        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            double left_float = left_result.float_literal.floating;
            double right_float = right_result.float_literal.floating;

            result = ( AstNode ){
                .kind = ASTNODEKIND_FLOATLITERAL,
                .type = ( Type ){
                    .kind = TYPEKIND_PRIMITIVE_FLOAT
                },
            };

            switch( binary.operation )
            {
                case BINARYOPERATION_ADDITION:       result.float_literal.floating = left_float + right_float; return result;
                case BINARYOPERATION_SUBTRACTION:    result.float_literal.floating = left_float - right_float; return result;
                case BINARYOPERATION_MULTIPLICATION: result.float_literal.floating = left_float * right_float; return result;
                case BINARYOPERATION_DIVISION:       result.float_literal.floating = left_float / right_float; return result;
                default: // do nothing
            }

            // if this point is reached, then it must be a boolean operation
            result = ( AstNode ){
                .kind = ASTNODEKIND_BOOLEANLITERAL,
                .type = ( Type ){
                    .kind = TYPEKIND_PRIMITIVE_BOOLEAN
                },
            };

            switch( binary.operation )
            {
                case BINARYOPERATION_EQUALTO:              result.boolean_literal.boolean = left_float == right_float; return result;
                case BINARYOPERATION_NOTEQUALTO:           result.boolean_literal.boolean = left_float != right_float; return result;
                case BINARYOPERATION_LESSTHAN:             result.boolean_literal.boolean = left_float < right_float; return result;
                case BINARYOPERATION_LESSTHANOREQUALTO:    result.boolean_literal.boolean = left_float <= right_float; return result;
                case BINARYOPERATION_GREATERTHAN:          result.boolean_literal.boolean = left_float > right_float; return result;
                case BINARYOPERATION_GREATERTHANOREQUALTO: result.boolean_literal.boolean = left_float >= right_float; return result;
                default: UNREACHABLE();
            }
        }

        case TYPEKIND_PRIMITIVE_STRING:
        {
            // TODO: actual string type
            char* left_string = left_result.string_literal.token.as_string;
            char* right_string = right_result.string_literal.token.as_string;

            result = ( AstNode ){
                .kind = ASTNODEKIND_BOOLEANLITERAL,
                .type = ( Type ){
                    .kind = TYPEKIND_PRIMITIVE_BOOLEAN
                },
            };

            switch( binary.operation )
            {                                                                      // TODO: better string compare
                case BINARYOPERATION_EQUALTO:    result.boolean_literal.boolean = strcmp( left_string, right_string ) == 0; return result;
                case BINARYOPERATION_NOTEQUALTO: result.boolean_literal.boolean = strcmp( left_string, right_string ) != 0; return result;
                default: UNREACHABLE();
            }
        }

        case TYPEKIND_PRIMITIVE_CHARACTER:
        {
            char left_char = left_result.character_literal.token.as_string[ 0 ];
            char right_char = right_result.character_literal.token.as_string[ 0 ];

            result = ( AstNode ){
                .kind = ASTNODEKIND_BOOLEANLITERAL,
                .type = ( Type ){
                    .kind = TYPEKIND_PRIMITIVE_BOOLEAN
                },
            };

            switch( binary.operation )
            {
                case BINARYOPERATION_EQUALTO:    result.boolean_literal.boolean = left_char == right_char; return result;
                case BINARYOPERATION_NOTEQUALTO: result.boolean_literal.boolean = left_char != right_char; return result;
                default: UNREACHABLE();
            }
        }

        case TYPEKIND_PRIMITIVE_BOOLEAN:
        {
            bool left_bool = left_result.boolean_literal.boolean;
            bool right_bool = right_result.boolean_literal.boolean;

            result = ( AstNode ){
                .kind = ASTNODEKIND_BOOLEANLITERAL,
                .type = ( Type ){
                    .kind = TYPEKIND_PRIMITIVE_BOOLEAN
                },
            };

            switch( binary.operation )
            {
                case BINARYOPERATION_EQUALTO:    result.boolean_literal.boolean = left_bool == right_bool; return result;
                case BINARYOPERATION_NOTEQUALTO: result.boolean_literal.boolean = left_bool != right_bool; return result;
                case BINARYOPERATION_AND:        result.boolean_literal.boolean = left_bool && right_bool; return result;
                case BINARYOPERATION_OR:         result.boolean_literal.boolean = left_bool || right_bool; return result;
                default: UNREACHABLE();
            }
        }

        default:
        {
            printf("unimplemented: %d\n", left_result.type.kind );
            UNIMPLEMENTED();
        }
    }
}

AstNode* walk_lvalue( AstNode* lvalue, InterpreterContext* ctx )
{
    switch( lvalue->kind )
    {
        case ASTNODEKIND_IDENTIFIER:
        {
            char* identifier_string = lvalue->identifier.token.as_string;
            Symbol* symbol = st_get( *ctx->st, identifier_string );
            return symbol->value;
        }

        case ASTNODEKIND_SUBSCRIPT:
        {
            AstNode* array = walk_lvalue( lvalue->subscript.target, ctx );
            int64_t index = walk_node( lvalue->subscript.index, ctx ).integer_literal.integer;

            return array->array_literal.initialized_elements[ index ];
        }

        case ASTNODEKIND_MEMBERACCESS:
        {
            UNIMPLEMENTED();
        }

        default:
        {
            UNREACHABLE();
        }
    }
}

void walk_assignment( AstNodeAssignment assignment, InterpreterContext* ctx )
{
    AstNode* lvalue = walk_lvalue( assignment.target, ctx );
    *lvalue = walk_node( assignment.value, ctx );
}

AstNode get_default_value( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_PRIMITIVE_STRING:
        {
            return ( AstNode ){
                .kind = ASTNODEKIND_STRINGLITERAL,
                .type = TYPE_STRING,
                .string_literal = {
                    .token.as_string = ""
                }
            };
        }

        case TYPEKIND_PRIMITIVE_CHARACTER:
        {
            return ( AstNode ){
                .kind = ASTNODEKIND_CHARACTERLITERAL,
                .type = TYPE_CHARACTER,
                .character_literal = {
                    .token.as_string = ""
                }
            };
        }

        case TYPEKIND_PRIMITIVE_BOOLEAN:
        {
            return ( AstNode ){
                .kind = ASTNODEKIND_BOOLEANLITERAL,
                .type = TYPE_BOOLEAN,
                .boolean_literal = {
                    .boolean = false,
                }
            };
        }

        case TYPEKIND_PRIMITIVE_INT:
        {
            return ( AstNode ){
                .kind = ASTNODEKIND_INTEGERLITERAL,
                .type = TYPE_INT,
                .integer_literal = {
                    .integer = 0,
                }
            };
        }

        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            return ( AstNode ){
                .kind = ASTNODEKIND_FLOATLITERAL,
                .type = TYPE_FLOAT,
                .float_literal = {
                    .floating = 0,
                }
            };
        }

        case TYPEKIND_ARRAY:
        case TYPEKIND_POINTER:
        case TYPEKIND_TYPE:
        case TYPEKIND_STRUCT:
        case TYPEKIND_ENUM:
        case TYPEKIND_ROUTINE:
        {
            UNIMPLEMENTED();
        }

        default:
        {
            UNREACHABLE();
        }
    }
}

AstNode walk_array_literal( AstNodeArrayLiteral array_literal, InterpreterContext* ctx, Type type )
{
    int64_t length = type.array.length;
    int64_t initialized_length = lvec_get_length( array_literal.initialized_elements );

    AstNode** initialized_elements = lvec_new( AstNode* );
    lvec_reserve_minimum( initialized_elements, length );

    // evaluate initialized elements
    for( int64_t i = 0; i < initialized_length; i++ )
    {
        AstNode* result = octo_malloc( sizeof( AstNode ) );
        *result = walk_node( array_literal.initialized_elements[ i ], ctx );
        lvec_append_aggregate( initialized_elements, result );
    }

    for( int64_t i = initialized_length; i < length; i++ )
    {
        AstNode* result = octo_malloc( sizeof( AstNode ) );
        *result = get_default_value( type_unwrap_array( type ) );
        lvec_append_aggregate( initialized_elements, result );
    }

    AstNode result = {
        .kind = ASTNODEKIND_ARRAYLITERAL,
        .array_literal = ( AstNodeArrayLiteral ){
            // .length = array_literal.length,
            .initialized_elements = initialized_elements
        }
    };

    return result;
}

AstNode walk_subscript( AstNodeSubscript subscript, InterpreterContext* ctx )
{
    AstNode* array = walk_lvalue( subscript.target, ctx );
    int64_t index = walk_node( subscript.index, ctx ).integer_literal.integer;

    return *array->array_literal.initialized_elements[ index ];
}

AstNode walk_routine_call( AstNodeRoutineCall routine_call, InterpreterContext* ctx )
{
    size_t arg_count = lvec_get_length( routine_call.args );
    AstNode* lvalue = walk_lvalue( routine_call.routine, ctx );

    st_push_scope( ctx->st );

    // insert routine call args
    AstNode* args = octo_malloc( sizeof( AstNode ) * arg_count );
    for( size_t i = 0; i < arg_count; i++ )
    {
        args[ i ] = walk_node( routine_call.args[ i ], ctx );
        Token arg_identifier = lvalue->routine_definition.param_identifier_tokens[ i ];

        Symbol arg_symbol = {
            .key = arg_identifier,
            .value = &args[ i ],
        };

        st_insert( ctx->st, arg_symbol );
    }

    AstNode result = walk_node( lvalue->routine_definition.body, ctx );

    st_pop_scope( ctx->st );
    free( args );
    return result;
}

AstNode walk_if( AstNodeConditional conditional, InterpreterContext* ctx )
{
    bool condition_result = walk_node( conditional.condition, ctx ).boolean_literal.boolean;

    AstNode result = {};
    if( condition_result )
    {
        result = walk_node( conditional.main_body, ctx );
    }
    else if( conditional.else_body != NULL )
    {
        result = walk_node( conditional.else_body, ctx );
    }

    return result;
}

AstNode walk_while( AstNodeConditional conditional, InterpreterContext* ctx )
{
    bool condition_result = walk_node( conditional.condition, ctx ).boolean_literal.boolean;

    AstNode result = {};
    while( condition_result )
    {
        result = walk_node( conditional.main_body, ctx );
        condition_result = walk_node( conditional.condition, ctx ).boolean_literal.boolean;
    }

    return result;
}

AstNode walk_node( AstNode* node, InterpreterContext* ctx )
{
    AstNode result;
    switch( node->kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        case ASTNODEKIND_CHARACTERLITERAL:
        case ASTNODEKIND_INTEGERLITERAL:
        case ASTNODEKIND_FLOATLITERAL:
        case ASTNODEKIND_BOOLEANLITERAL:
        {
            result = *node;
            break;
        }

        case ASTNODEKIND_IDENTIFIER:
        {
            Symbol* symbol = st_get( *ctx->st, node->identifier.token.as_string );
            result = *symbol->value;
            break;
        }

        case ASTNODEKIND_ECHO:
        {
            walk_echo( node->echo, ctx );
            result = *node;
            break;
        }

        case ASTNODEKIND_COMPOUND:
        {
            result = walk_compound( node->compound, ctx );
            break;
        }

        case ASTNODEKIND_VARIABLEDECLARATION:
        {
            walk_variable_declaration( node->variable_declaration, ctx );
            result = *node;
            break;
        }

        case ASTNODEKIND_ASSIGNMENT:
        {
            walk_assignment( node->assignment, ctx );
            result = *node;
            break;
        }

        case ASTNODEKIND_BINARY:
        {
            result = walk_binary( node->binary, ctx );
            break;
        }

        case ASTNODEKIND_ROUTINECALL:
        {
            result = walk_routine_call( node->routine_call, ctx );
            break;
        }

        case ASTNODEKIND_ARRAYLITERAL:
        {
            result = walk_array_literal( node->array_literal, ctx, node->type );
            break;
        }

        case ASTNODEKIND_SUBSCRIPT:
        {
            result = walk_subscript( node->subscript, ctx );
            break;
        }

        case ASTNODEKIND_CONDITIONAL:
        {
            if( node->conditional.is_while )
            {
                result = walk_while( node->conditional, ctx );
            }
            else
            {
                result = walk_if( node->conditional, ctx );
            }
            break;
        }

        default:
        {
            printf( "unimplemented: %d\n", node->kind );
            UNIMPLEMENTED();
        }
    }

    result.type = node->type;
    return result;
}

void interpret( AstNode* ast, SymbolTable* st )
{
    assert( ast->kind == ASTNODEKIND_MODULE );

    InterpreterContext ctx = {
        .st = st
    };

    AstNode* main_routine = st_get( *ctx.st, "main" )->value;
    walk_node( main_routine->routine_definition.body, &ctx );
}
