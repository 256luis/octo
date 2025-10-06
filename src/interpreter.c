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

RuntimeValue walk_node( AstNode* node, InterpreterContext* ctx );

RuntimeValue walk_compound( AstNodeCompound compound, InterpreterContext* ctx )
{
    size_t length = lvec_get_length( compound.nodes );
    RuntimeValue last_result;

    st_push_scope( ctx->st );
    for( size_t i = 0; i < length; i++ )
    {
        ctx->is_return = false;
        last_result = walk_node( compound.nodes[ i ], ctx );
        if( ctx->is_return )
        {
            break;
        }
    }
    st_pop_scope( ctx->st );

    return last_result;
}

void print_runtime_value( RuntimeValue rv )
{
    switch( rv.type.kind )
    {
        case TYPEKIND_PRIMITIVE_STRING:
        {
            printf( "%s", rv.string );
            break;
        }

        case TYPEKIND_PRIMITIVE_CHARACTER:
        {
            printf( "%c", rv.character );
            break;
        }

        case TYPEKIND_PRIMITIVE_BOOLEAN:
        {
            printf( "%s", rv.boolean ? "true" : "false" );
            break;
        }

        case TYPEKIND_PRIMITIVE_INT:
        {
            printf( "%ld", rv.integer );
            break;
        }

        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            printf( "%lf", rv.floating );
            break;
        }

        case TYPEKIND_ARRAY:
        {
            type_print( rv.type );
            printf( ".[" );

            for( int64_t i = 0; i < rv.type.array.length; i++ )
            {
                RuntimeValue element = rv.array[ i ];
                print_runtime_value( element );
                printf( ", " );
            }
            printf( "]" );
            break;
        }

        case TYPEKIND_POINTER:
        {
            printf( "%p", ( void* )rv.pointer );
            break;
        }

        case TYPEKIND_UNSPECIFIED:
        case TYPEKIND_NONE:
        {
            UNREACHABLE();
        }

        default:
        {
            type_print( rv.type );
            UNIMPLEMENTED();
        }
    }
}

void walk_echo( AstNodeEcho echo, InterpreterContext* ctx )
{
    RuntimeValue to_echo = walk_node( echo.value, ctx );
    print_runtime_value( to_echo );
    putchar('\n');
}

void walk_variable_declaration( AstNodeVariableDeclaration variable_declaration, InterpreterContext* ctx )
{
    Symbol symbol = {
        .key = variable_declaration.identifier_token,
        .type = variable_declaration.value->type,
        .value = octo_malloc( sizeof( RuntimeValue ) ),
    };

    RuntimeValue value = walk_node( variable_declaration.value, ctx );
    *symbol.value = value;

    st_insert( ctx->st, symbol );
}

RuntimeValue* walk_lvalue( AstNode* lvalue, InterpreterContext* ctx )
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
            RuntimeValue* rv = walk_lvalue( lvalue->subscript.target, ctx );
            int64_t index = walk_node( lvalue->subscript.index, ctx ).integer;

            return &rv->array[ index ];
        }

        case ASTNODEKIND_MEMBERACCESS:
        {
            RuntimeValue* rv = walk_lvalue( lvalue->member_access.target, ctx );
            return st_get( rv->structure_st, lvalue->member_access.member_token.as_string )->value;
        }

        case ASTNODEKIND_UNARY:
        {
            // assume dereference
            return walk_lvalue( lvalue->unary.operand, ctx )->pointer;
        }

        default:
        {
            printf( "%d\n", lvalue->kind );
            UNREACHABLE();
        }
    }
}

void walk_assignment( AstNodeAssignment assignment, InterpreterContext* ctx )
{
    RuntimeValue* lvalue = walk_lvalue( assignment.target, ctx );
    *lvalue = walk_node( assignment.value, ctx );
}

RuntimeValue walk_binary( AstNodeBinary binary, InterpreterContext* ctx )
{
    RuntimeValue left_result = walk_node( binary.left, ctx );
    RuntimeValue right_result = walk_node( binary.right, ctx );
    RuntimeValue result;

    switch( left_result.type.kind )
    {
        case TYPEKIND_PRIMITIVE_INT:
        {
            int64_t left_integer = left_result.integer;
            int64_t right_integer = right_result.integer;

            switch( binary.operation )
            {
                case BINARYOPERATION_ADDITION:       result.integer = left_integer + right_integer; return result;
                case BINARYOPERATION_SUBTRACTION:    result.integer = left_integer - right_integer; return result;
                case BINARYOPERATION_MULTIPLICATION: result.integer = left_integer * right_integer; return result;
                case BINARYOPERATION_DIVISION:       result.integer = left_integer / right_integer; return result;
                case BINARYOPERATION_MODULO:         result.integer = left_integer % right_integer; return result;
                default: // do nothing
            }

            // if this point is reached, then it must be a boolean operation
            switch( binary.operation )
            {
                case BINARYOPERATION_EQUALTO:              result.boolean = left_integer == right_integer; return result;
                case BINARYOPERATION_NOTEQUALTO:           result.boolean = left_integer != right_integer; return result;
                case BINARYOPERATION_LESSTHAN:             result.boolean = left_integer < right_integer; return result;
                case BINARYOPERATION_LESSTHANOREQUALTO:    result.boolean = left_integer <= right_integer; return result;
                case BINARYOPERATION_GREATERTHAN:          result.boolean = left_integer > right_integer; return result;
                case BINARYOPERATION_GREATERTHANOREQUALTO: result.boolean = left_integer >= right_integer; return result;
                default: UNREACHABLE();
            }
        }

        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            double left_float = left_result.floating;
            double right_float = right_result.floating;

            switch( binary.operation )
            {
                case BINARYOPERATION_ADDITION:       result.floating = left_float + right_float; return result;
                case BINARYOPERATION_SUBTRACTION:    result.floating = left_float - right_float; return result;
                case BINARYOPERATION_MULTIPLICATION: result.floating = left_float * right_float; return result;
                case BINARYOPERATION_DIVISION:       result.floating = left_float / right_float; return result;
                default: // do nothing
            }

            switch( binary.operation )
            {
                case BINARYOPERATION_EQUALTO:              result.boolean = left_float == right_float; return result;
                case BINARYOPERATION_NOTEQUALTO:           result.boolean = left_float != right_float; return result;
                case BINARYOPERATION_LESSTHAN:             result.boolean = left_float < right_float; return result;
                case BINARYOPERATION_LESSTHANOREQUALTO:    result.boolean = left_float <= right_float; return result;
                case BINARYOPERATION_GREATERTHAN:          result.boolean = left_float > right_float; return result;
                case BINARYOPERATION_GREATERTHANOREQUALTO: result.boolean = left_float >= right_float; return result;
                default: UNREACHABLE();
            }
        }

        case TYPEKIND_PRIMITIVE_STRING:
        {
            // TODO: actual string type
            char* left_string = left_result.string;
            char* right_string = right_result.string;

            switch( binary.operation )
            {                                                                      // TODO: better string compare
                case BINARYOPERATION_EQUALTO:    result.boolean = strcmp( left_string, right_string ) == 0; return result;
                case BINARYOPERATION_NOTEQUALTO: result.boolean = strcmp( left_string, right_string ) != 0; return result;
                default: UNREACHABLE();
            }
        }

        case TYPEKIND_PRIMITIVE_CHARACTER:
        {
            char left_char = left_result.character;
            char right_char = right_result.character;

            switch( binary.operation )
            {
                case BINARYOPERATION_EQUALTO:    result.boolean = left_char == right_char; return result;
                case BINARYOPERATION_NOTEQUALTO: result.boolean = left_char != right_char; return result;
                default: UNREACHABLE();
            }
        }

        case TYPEKIND_PRIMITIVE_BOOLEAN:
        {
            bool left_bool = left_result.boolean;
            bool right_bool = right_result.boolean;

            switch( binary.operation )
            {
                case BINARYOPERATION_EQUALTO:    result.boolean = left_bool == right_bool; return result;
                case BINARYOPERATION_NOTEQUALTO: result.boolean = left_bool != right_bool; return result;
                case BINARYOPERATION_AND:        result.boolean = left_bool && right_bool; return result;
                case BINARYOPERATION_OR:         result.boolean = left_bool || right_bool; return result;
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

RuntimeValue walk_routine_call( AstNodeRoutineCall routine_call, InterpreterContext* ctx )
{
    size_t arg_count = lvec_get_length( routine_call.args );
    RuntimeValue* lvalue = walk_lvalue( routine_call.routine, ctx );

    st_push_scope( ctx->st );

    // insert routine call args
    RuntimeValue* args = octo_malloc( sizeof( RuntimeValue ) * arg_count );
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

    RuntimeValue result = walk_node( lvalue->routine_definition.body, ctx );

    st_pop_scope( ctx->st );
    free( args );
    return result;
}

RuntimeValue get_default_value( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_PRIMITIVE_STRING:
        {
            return ( RuntimeValue ){
                .type = TYPE_STRING,
                .string = "",
            };
        }

        case TYPEKIND_PRIMITIVE_CHARACTER:
        {
            return ( RuntimeValue ){
                .type = TYPE_CHARACTER,
                .character = '\0'
            };
        }

        case TYPEKIND_PRIMITIVE_BOOLEAN:
        {
            return ( RuntimeValue ){
                .type = TYPE_BOOLEAN,
                .boolean = false,
            };
        }

        case TYPEKIND_PRIMITIVE_INT:
        {
            return ( RuntimeValue ){
                .type = TYPE_INT,
                .integer = 0,
            };
        }

        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            return ( RuntimeValue ){
                .type = TYPE_FLOAT,
                .floating = 0,
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

RuntimeValue walk_array_literal( AstNodeArrayLiteral array_literal, InterpreterContext* ctx, Type type )
{
    int64_t length = type.array.length;
    int64_t initialized_length = lvec_get_length( array_literal.initialized_elements );

    RuntimeValue* elements = lvec_new( RuntimeValue );
    lvec_reserve_minimum( elements, length );

    // evaluate initialized elements
    for( int64_t i = 0; i < initialized_length; i++ )
    {
        RuntimeValue result = walk_node( array_literal.initialized_elements[ i ], ctx );
        lvec_append_aggregate( elements, result );
    }

    // set uninitialized elements to default value
    for( int64_t i = initialized_length; i < length; i++ )
    {
        RuntimeValue result = get_default_value( type_unwrap_array( type ) );
        lvec_append_aggregate( elements, result );
    }

    RuntimeValue result = {
        .array = elements
    };

    return result;
}

RuntimeValue walk_subscript( AstNodeSubscript subscript, InterpreterContext* ctx )
{
    RuntimeValue* array = walk_lvalue( subscript.target, ctx );
    int64_t index = walk_node( subscript.index, ctx ).integer;

    return array->array[ index ];
}

RuntimeValue walk_if( AstNodeConditional conditional, InterpreterContext* ctx )
{
    bool condition_result = walk_node( conditional.condition, ctx ).boolean;

    RuntimeValue result = {};
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

RuntimeValue walk_while( AstNodeConditional conditional, InterpreterContext* ctx )
{
    bool condition_result = walk_node( conditional.condition, ctx ).boolean;

    RuntimeValue result = {};
    while( condition_result )
    {
        result = walk_node( conditional.main_body, ctx );
        condition_result = walk_node( conditional.condition, ctx ).boolean;
    }

    return result;
}

RuntimeValue apply_unary_operation( RuntimeValue rv, UnaryOperation operation )
{
    RuntimeValue result = rv;
    switch( operation )
    {
        case UNARYOPERATION_NEGATION:
        {
            if( rv.type.kind == TYPEKIND_PRIMITIVE_INT )
            {
                result.integer = -rv.integer;
            }
            else if( rv.type.kind == TYPEKIND_PRIMITIVE_FLOAT )
            {
                result.floating = -rv.floating;
            }
            else
            {
                UNREACHABLE();
            }
            break;
        }

        case UNARYOPERATION_NOT:
        {
            result.boolean = !rv.boolean;
            break;
        }

        // case UNARYOPERATION_ADDRESSOF:
        case UNARYOPERATION_DEREFERENCE:
        {
            UNREACHABLE();
        }
    }

    return result;
}

RuntimeValue walk_unary( AstNodeUnary unary, InterpreterContext* ctx )
{
    RuntimeValue result;
    if( unary.operation == UNARYOPERATION_DEREFERENCE )
    {
        result = *walk_node( unary.operand, ctx ).pointer;
    }
    else
    {
        RuntimeValue operand_result = walk_node( unary.operand, ctx );
        result = apply_unary_operation( operand_result, unary.operation );
    }

    return result;
}

RuntimeValue walk_struct_literal( AstNodeStructLiteral struct_literal, InterpreterContext* ctx, Type type )
{
    size_t member_count = type.structure.member_count;
    size_t initialized_member_count = lvec_get_length( struct_literal.initialized_member_values );

    RuntimeValue result = {};
    st_initialize( &result.structure_st );

    // evaluate initialized members
    for( size_t i = 0; i < initialized_member_count; i++ )
    {
        AstNode* initialized_member_node = struct_literal.initialized_member_values[ i ];
        Token member_identifier_token = struct_literal.initialized_member_tokens[ i ];
        RuntimeValue* rv = octo_malloc( sizeof( RuntimeValue ) );
        *rv = walk_node( initialized_member_node, ctx );
        Symbol member_symbol = {
            .key = member_identifier_token,
            .value = rv,
        };
        st_insert( &result.structure_st, member_symbol );
    }

    // set uninitialized members to default
    for( size_t i = 0; i < member_count; i++ )
    {
        Token member_identifier_token = type.structure.members->symbols[ i ].key;
        if( st_get( result.structure_st, member_identifier_token.as_string ) != NULL )
        {
            continue;
        }

        RuntimeValue* rv = octo_malloc( sizeof( RuntimeValue ) );
        *rv = get_default_value( st_get( *type.structure.members, member_identifier_token.as_string )->type );
        Symbol member_symbol = {
            .key = member_identifier_token,
            .type = st_get( *type.structure.members, member_identifier_token.as_string )->type,
            .value = rv,
        };
        st_insert( &result.structure_st, member_symbol );
    }

    return result;
}

RuntimeValue walk_return( AstNodeReturn return_statement, InterpreterContext* ctx )
{
    RuntimeValue result = {};
    if( return_statement.value != NULL )
    {
        result = walk_node( return_statement.value, ctx );
    }

    ctx->is_return = true;
    return result;
}

RuntimeValue walk_member_access( AstNodeMemberAccess member_access, InterpreterContext* ctx )
{
    RuntimeValue* rv = walk_lvalue( member_access.target, ctx );
    return *st_get( rv->structure_st, member_access.member_token.as_string )->value;
}

RuntimeValue walk_address_of( AstNodeAddressOf address_of, InterpreterContext* ctx )
{
    return ( RuntimeValue ){
        .pointer = walk_lvalue( address_of.operand, ctx ),
    };
}

RuntimeValue walk_node( AstNode* node, InterpreterContext* ctx )
{
    RuntimeValue result = {};
    switch( node->kind )
    {
        case ASTNODEKIND_STRINGLITERAL:
        {
            result = ( RuntimeValue ){
                .string = node->string_literal.token.as_string,
            };
            break;
        }

        case ASTNODEKIND_CHARACTERLITERAL:
        {
            result = ( RuntimeValue ){
                .character = node->character_literal.token.as_string[ 0 ],
            };
            break;
        }

        case ASTNODEKIND_INTEGERLITERAL:
        {
            result = ( RuntimeValue ){
                .integer = node->integer_literal.integer
            };
            break;
        }

        case ASTNODEKIND_FLOATLITERAL:
        {
            result = ( RuntimeValue ){
                .floating = node->float_literal.floating,
            };
            break;
        }

        case ASTNODEKIND_BOOLEANLITERAL:
        {
            result = ( RuntimeValue ){
                .boolean = node->boolean_literal.boolean,
            };
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
            break;
        }

        case ASTNODEKIND_ASSIGNMENT:
        {
            walk_assignment( node->assignment, ctx );
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

        case ASTNODEKIND_UNARY:
        {
            result = walk_unary( node->unary, ctx );
            break;
        }

        case ASTNODEKIND_STRUCTLITERAL:
        {
            result = walk_struct_literal( node->struct_literal, ctx, node->type );
            break;
        }

        case ASTNODEKIND_MEMBERACCESS:
        {
            result = walk_member_access( node->member_access, ctx );
            break;
        }

        case ASTNODEKIND_RETURN:
        {
            result = walk_return( node->return_statement, ctx );
            break;
        }

        case ASTNODEKIND_UNINITIALIZED:
        {
            // do nothing
            break;
        }

        case ASTNODEKIND_ADDRESSOF:
        {
            result = walk_address_of( node->address_of, ctx );
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

    size_t node_count = lvec_get_length( ast->module.nodes );
    AstNode* main_routine_body = NULL;
    for( size_t i = 0; i < node_count; i++ )
    {
        AstNode* node = ast->module.nodes[ i ];
        if( node->kind != ASTNODEKIND_ROUTINEDECLARATION ) continue;

        AstNodeRoutineDeclaration routine_declaration = node->routine_declaration;
        if( strcmp( routine_declaration.identifier_token.as_string, "main" ) == 0 )
        {
            main_routine_body = routine_declaration.routine_definition->routine_definition.body;
        }
    }

    if( main_routine_body == NULL )
    {
        printf("no main routine found");
        return;
    }

    walk_node( main_routine_body, &ctx );
}
