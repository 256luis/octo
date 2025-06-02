#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser.h"
#include "tokenizer.h"
#include "debug.h"
#include "error.h"
#include "lvec.h"
#include "type.h"

#define EXPECT( parser_ptr, ... )\
    _expect( parser_ptr, ( TokenKind[] ){ __VA_ARGS__ },\
             sizeof( ( TokenKind[] ){ __VA_ARGS__ } ) / sizeof( TokenKind ),\
             __LINE__ )

#define EXPECT_NEXT( parser_ptr, ... )\
    _expect_next( parser_ptr, ( TokenKind[] ){ __VA_ARGS__ },\
                  sizeof( ( TokenKind[] ){ __VA_ARGS__ } ) / sizeof( TokenKind ),\
                  __LINE__ )

static bool _expect( Parser* parser, TokenKind* expected_token_kinds, size_t length, int line )
{
    bool is_valid = false;
    for( size_t i = 0; i < length; i++ )
    {
        if( parser->current_token.kind == expected_token_kinds[ i ] )
        {
            is_valid = true;
            break;
        }
    }
    if( !is_valid )
    {
        printf( "ERROR CALLED FROM LINE %d\n", line );
        Error error = {
            .kind = ERRORKIND_UNEXPECTEDSYMBOL,
            .offending_token = parser->current_token,
        };
        report_error( error );
    }

    return is_valid;
}

static bool _expect_next( Parser* parser, TokenKind* expected_token_kinds, size_t length, int line )
{
    bool is_valid = false;
    for( size_t i = 0; i < length; i++ )
    {
        if( parser->next_token.kind == expected_token_kinds[ i ] )
        {
            is_valid = true;
            break;
        }
    }
    if( !is_valid )
    {
        printf( "ERROR CALLED FROM LINE %d\n", line );
        Error error = {
            .kind = ERRORKIND_UNEXPECTEDSYMBOL,
            .offending_token = parser->next_token,
        };
        report_error( error );
    }

    return is_valid;
}

static BinaryOperation token_kind_to_binary_operation( TokenKind token_kind )
{
    switch( token_kind )
    {
        case TOKENKIND_PLUS:         return BINARYOPERATION_ADD;
        case TOKENKIND_MINUS:        return BINARYOPERATION_SUBTRACT;
        case TOKENKIND_STAR:         return BINARYOPERATION_MULTIPLY;
        case TOKENKIND_FORWARDSLASH: return BINARYOPERATION_DIVIDE;
        case TOKENKIND_GREATER:      return BINARYOPERATION_GREATER;
        case TOKENKIND_LESS:         return BINARYOPERATION_LESS;
        case TOKENKIND_DOUBLEEQUAL:  return BINARYOPERATION_EQUAL;
        case TOKENKIND_NOTEQUAL:     return BINARYOPERATION_NOTEQUAL;
        case TOKENKIND_GREATEREQUAL: return BINARYOPERATION_GREATEREQUAL;
        case TOKENKIND_LESSEQUAL:    return BINARYOPERATION_LESSEQUAL;
            // case TOKENKIND_LEFTBRACKET:  return BINARYOPERATION_SUBSCRIPT;
        default: UNREACHABLE();
    }
}

static UnaryOperation token_kind_to_unary_operation( TokenKind token_kind )
{
    switch( token_kind )
    {
        case TOKENKIND_BANG:      return UNARYOPERATION_NOT;
        case TOKENKIND_MINUS:     return UNARYOPERATION_NEGATIVE;
        case TOKENKIND_AMPERSAND: return UNARYOPERATION_ADDRESSOF;
        case TOKENKIND_STAR:      return UNARYOPERATION_DEREFERENCE;
        default: UNREACHABLE();
    }
}

static void advance( Parser* parser )
{
    if( parser->current_token.kind != TOKENKIND_EOF )
    {
        parser->current_token_index++;
    }

    parser->current_token = parser->tokens[ parser->current_token_index ];
    parser->next_token = parser->tokens[ parser->current_token_index + 1 ];
}

void parser_initialize( Parser* parser, Token* _tokens )
{
    parser->tokens = _tokens;
    parser->current_token_index = -1;
    advance( parser );
}

static Expression* parse_integer( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_INTEGER;
    expression->integer = parser->current_token.integer;

    return expression;
}

static Expression* parse_float( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FLOAT;
    expression->floating = parser->current_token.floating;

    return expression;
}

static Expression* parse_identifier( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_IDENTIFIER;
    expression->identifier.as_string = parser->current_token.identifier;
    expression->associated_token = parser->current_token;

    return expression;
}

static Expression* parse_string( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_STRING;
    expression->string = parser->current_token.string;

    return expression;
}

static Expression* parse_character( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_CHARACTER;
    expression->character = parser->current_token.character;

    return expression;
}


static Expression* parse_boolean( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_BOOLEAN;
    expression->boolean = parser->current_token.boolean;

    return expression;
}

static Expression* parse_base_expression( Parser* parser)
{
    Expression* expression;

    switch( parser->current_token.kind )
    {
        case TOKENKIND_INTEGER:    expression = parse_integer( parser );    break;
        case TOKENKIND_FLOAT:      expression = parse_float( parser );      break;
        case TOKENKIND_IDENTIFIER: expression = parse_identifier( parser ); break;
        case TOKENKIND_STRING:     expression = parse_string( parser );     break;
        case TOKENKIND_CHARACTER:  expression = parse_character( parser );  break;
        case TOKENKIND_BOOLEAN:    expression = parse_boolean( parser );  break;
        default: UNREACHABLE();
    }

    if( expression == NULL )
    {
        return NULL;
    }

    expression->associated_token = parser->current_token;

    return expression;
}

static Expression* parse_atom( Parser* parser );
static Expression* parse_unary( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_UNARY;
    expression->unary.operation = token_kind_to_unary_operation( parser->current_token.kind );
    expression->unary.operator_token = parser->current_token;

    advance( parser );
    expression->unary.operand = parse_atom( parser );

    return expression;
}

static Expression* parse_rvalue( Parser* parser );
static Expression* parse_function_call( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FUNCTIONCALL;
    expression->function_call.identifier = parser->current_token.identifier;
    expression->function_call.identifier_token = parser->current_token;

    advance( parser );
    // parser.current_token == TOKENKIND_LEFTPAREN

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_RIGHTPAREN, TOKENKIND_RVALUE_STARTERS ) )
    {
        return NULL;
    }

    // if current token is right paren, there are no args to the function call
    if( parser->current_token.kind == TOKENKIND_RIGHTPAREN )
    {
        expression->function_call.args = NULL;
        expression->function_call.arg_count = 0;
    }
    else
    {
        Expression** args = lvec_new( Expression* );
        if( args == NULL ) ALLOC_ERROR();

        while( parser->current_token.kind != TOKENKIND_RIGHTPAREN )
        {
            Expression* e = parse_rvalue( parser );
            if( e == NULL )
            {
                return NULL;
            }
            lvec_append( args, e );

            expression->function_call.arg_count++;

            advance( parser );
            if( !EXPECT( parser, TOKENKIND_RIGHTPAREN, TOKENKIND_COMMA ) )
            {
                return NULL;
            }

            if( parser->current_token.kind == TOKENKIND_COMMA )
            {
                advance( parser );
            }
        }

        expression->function_call.args = args;
    }

    return expression;
}

static Type parse_base_type( Parser* parser )
{
    Type result;
    result.token = parser->current_token;
    char* type_identifier = parser->current_token.identifier;

    if( strcmp( type_identifier, "i8" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
        result.integer.bit_count = 8;
        result.integer.is_signed = true;
    }
    else if( strcmp( type_identifier, "i16" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
        result.integer.bit_count = 16;
        result.integer.is_signed = true;
    }
    else if( strcmp( type_identifier, "i32" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
        result.integer.bit_count = 32;
        result.integer.is_signed = true;
    }
    else if( strcmp( type_identifier, "i64" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
        result.integer.bit_count = 64;
        result.integer.is_signed = true;
    }
    else if( strcmp( type_identifier, "u8" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
        result.integer.bit_count = 8;
        result.integer.is_signed = false;
    }
    else if( strcmp( type_identifier, "u16" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
        result.integer.bit_count = 16;
        result.integer.is_signed = false;
    }
    else if( strcmp( type_identifier, "u32" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
        result.integer.bit_count = 32;
        result.integer.is_signed = false;
    }
    else if( strcmp( type_identifier, "u64" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
        result.integer.bit_count = 64;
        result.integer.is_signed = false;
    }
    else if( strcmp( type_identifier, "f32" ) == 0 )
    {
        result.kind = TYPEKIND_FLOAT;
        result.floating.bit_count = 32;
    }
    else if( strcmp( type_identifier, "f64" ) == 0 )
    {
        result.kind = TYPEKIND_FLOAT;
        result.floating.bit_count = 64;
    }
    else if( strcmp( type_identifier, "char" ) == 0 )
    {
        result.kind = TYPEKIND_CHARACTER;
    }
    else if( strcmp( type_identifier, "bool" ) == 0 )
    {
        result.kind = TYPEKIND_BOOLEAN;
    }
    else if( strcmp( type_identifier, "void" ) == 0 )
    {
        result.kind = TYPEKIND_VOID;
    }
    else // custom type
    {
        result.kind = TYPEKIND_CUSTOM;
        result.custom.identifier = type_identifier;
    }

    return result;
}

static Type parse_type( Parser* parser );
static Type parse_array_type( Parser* parser )
{
    Type result;
    result.kind = TYPEKIND_ARRAY;
    result.token = parser->current_token;
    advance( parser );
    if( !EXPECT( parser, TOKENKIND_INTEGER, TOKENKIND_RIGHTBRACKET ) )
    {
        result.kind = TYPEKIND_INVALID;
        return result;
    }

    // default value to indicate if length is to be infered
    result.array.length = -1;
    if( parser->current_token.kind == TOKENKIND_INTEGER )
    {
        result.array.length = parser->current_token.integer;
        advance( parser );
        if( !EXPECT( parser, TOKENKIND_RIGHTBRACKET ) )
        {
            result.kind = TYPEKIND_INVALID;
            return result;
        }
    }

    advance( parser );
    result.array.base_type = malloc( sizeof( Type ) );
    *result.array.base_type = parse_type( parser );

    return result;
}

// todo HASHMAP!!!
static Type parse_type( Parser* parser )
{
    if( !EXPECT( parser, TOKENKIND_TYPE_STARTERS ) )
    {
        return ( Type ){ .kind = TYPEKIND_INVALID };
    }

    Type result;
    result.token = parser->current_token;

    switch( parser->current_token.kind )
    {
        case TOKENKIND_IDENTIFIER:
        {
            result = parse_base_type( parser );
            break;
        }

        case TOKENKIND_LEFTBRACKET:
        {
            result = parse_array_type( parser );
            break;
        }

        case TOKENKIND_AMPERSAND: // pointers
        {
            result.kind = TYPEKIND_POINTER;
            advance( parser );
            if( !EXPECT( parser, TOKENKIND_TYPE_STARTERS ) )
            {
                result.kind = TYPEKIND_INVALID;
                return result;
            }

            // i have to allocate here... SO ANNOYING!!!
            result.pointer.base_type = malloc( sizeof( Type ) );
            *result.pointer.base_type = parse_type( parser );
            break;
        }

        default:
        {
            UNREACHABLE();
            break;
        }
    }

    return result;
}

static Expression* parse_array_literal( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_ARRAY;
    expression->starting_token = parser->current_token;

    expression->array.type = parse_type( parser );
    // expression->array.type_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_LEFTBRACKET ) )
    {
        return NULL;
    }

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_RVALUE_STARTERS, TOKENKIND_RIGHTBRACKET ) )
    {
        return NULL;
    }

    if( parser->current_token.kind != TOKENKIND_RIGHTBRACKET )
    {
        Expression* initialized_rvalues = lvec_new( Expression );
        int count_initialized = 0;

        Expression* first_initialized = parse_rvalue( parser );
        lvec_append_aggregate( initialized_rvalues, *first_initialized );
        count_initialized++;

        advance( parser );
        while( parser->current_token.kind != TOKENKIND_RIGHTBRACKET )
        {
            if( !EXPECT( parser, TOKENKIND_COMMA ) )
            {
                return NULL;
            }

            advance( parser );
            Expression* e = parse_rvalue( parser );
            if( e == NULL )
            {
                return NULL;
            }

            lvec_append_aggregate( initialized_rvalues, *e );
            count_initialized++;
            advance( parser );
        }

        expression->array.initialized_rvalues = initialized_rvalues;
        expression->array.count_initialized = count_initialized;
    }

    return expression;
}

static Expression* parse_lvalue( Parser* parser );
static Expression* parse_array_subscript( Parser* parser, Expression* lvalue )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_ARRAYSUBSCRIPT;
    expression->starting_token = parser->current_token;
    expression->array_subscript.lvalue = lvalue;

    advance( parser );
    Expression* index_rvalue = parse_rvalue( parser );
    if( index_rvalue == NULL )
    {
        return NULL;
    }

    expression->array_subscript.index_rvalue = index_rvalue;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_RIGHTBRACKET ) )
    {
        return NULL;
    }

    return expression;
}

static Expression* parse_member_access( Parser* parser, Expression* lvalue )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_MEMBERACCESS;
    expression->starting_token = parser->current_token;
    expression->member_access.lvalue = lvalue;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_IDENTIFIER ) )
    {
        return NULL;
    }

    expression->member_access.member_identifier_token = parser->current_token;

    return expression;
}

static Expression* parse_compound_literal( Parser* parser, Expression* lvalue )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_COMPOUNDLITERAL;
    expression->starting_token = lvalue->starting_token;
    expression->compound_literal.type_identifier_token = lvalue->starting_token;

    advance( parser );
    // current token is left brace

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_PERIOD, TOKENKIND_RIGHTBRACE ) )
    {
        return NULL;
    }

    Token* member_identifier_tokens = lvec_new( Token );
    Expression* initialized_member_rvalues = lvec_new( Expression );

    while( parser->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        if( !EXPECT( parser, TOKENKIND_PERIOD ) )
        {
            return NULL;
        }

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_IDENTIFIER ) )
        {
            return NULL;
        }

        lvec_append_aggregate( member_identifier_tokens, parser->current_token );

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_EQUAL ) )
        {
            return NULL;
        }

        advance( parser );
        Expression* initialized_member_rvalue = parse_rvalue( parser );
        if( initialized_member_rvalue == NULL )
        {
            return NULL;
        }

        lvec_append_aggregate( initialized_member_rvalues, *initialized_member_rvalue );

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_COMMA, TOKENKIND_RIGHTBRACE ) )
        {
            return NULL;
        }

        if( parser->current_token.kind != TOKENKIND_RIGHTBRACE)
        {
            advance( parser );
        }
    }

    expression->compound_literal.initialized_member_rvalues = initialized_member_rvalues;
    expression->compound_literal.member_identifier_tokens = member_identifier_tokens;
    expression->compound_literal.initialized_count = lvec_get_length( initialized_member_rvalues );

    if( !EXPECT( parser, TOKENKIND_RIGHTBRACE ) )
    {
        return NULL;
    }

    return expression;
}

static Expression* parse_postfix( Parser* parser, Expression* left )
{
    Expression* expression;
    switch( parser->current_token.kind )
    {
        case TOKENKIND_LEFTBRACKET:
        {
            expression = parse_array_subscript( parser, left );
            break;
        }

        case TOKENKIND_PERIOD:
        {
            if( !EXPECT_NEXT( parser, TOKENKIND_IDENTIFIER, TOKENKIND_LEFTBRACE) )
            {
                return NULL;
            }

            if( parser->next_token.kind == TOKENKIND_IDENTIFIER )
            {
                expression = parse_member_access( parser, left );
            }
            // left brace || compound literal
            else
            {
                expression = parse_compound_literal( parser, left );
            }

            break;
        }

        default:
        {
            UNREACHABLE();
            break;
        }
    }

    return expression;
}

static Expression* parse_atom( Parser* parser )
{
    Expression* expression;
    Token starting_token = parser->current_token;

    switch( parser->current_token.kind )
    {
        case TOKENKIND_IDENTIFIER:
        {
            // check if function call
            if( parser->next_token.kind == TOKENKIND_LEFTPAREN )
            {
                expression = parse_function_call( parser );
                break;
            }

            /* // if struct/union instantiation */
            /* if( parser->next_token.kind == TOKENKIND_LEFTBRACE) */
            /* { */
            /*     expression = parse_compound_literal( parser ); */
            /*     break; */
            /* } */

            [[ fallthrough ]];
        }
        case TOKENKIND_INTEGER:
        case TOKENKIND_FLOAT:
        case TOKENKIND_STRING:
        case TOKENKIND_CHARACTER:
        case TOKENKIND_BOOLEAN:
        {
            expression = parse_base_expression( parser );
            break;
        }

        case TOKENKIND_LEFTPAREN:
        {
            // expression = parse_parentheses();
            advance( parser );
            expression = parse_rvalue( parser );

            advance( parser );
            if( !EXPECT( parser, TOKENKIND_RIGHTPAREN ) )
            {
                return NULL;
            }
            break;
        }

        case TOKENKIND_AMPERSAND:
        case TOKENKIND_STAR:
        case TOKENKIND_BANG:
        case TOKENKIND_MINUS:
        {
            expression = parse_unary( parser );
            break;
        }

        case TOKENKIND_LEFTBRACKET:
        {
            expression = parse_array_literal( parser );
            break;
        }

        default:
        {
            UNIMPLEMENTED();
            break;
        }
    }

    if( expression == NULL )
    {
        return NULL;
    }

    expression->starting_token = starting_token;

    bool is_next_token_kind_postfix_operator = IS_TOKENKIND_IN_GROUP( parser->next_token.kind, TOKENKIND_POSTFIX_OPERATORS );
    while( is_next_token_kind_postfix_operator )
    {
        advance( parser );

        Expression* lvalue = calloc( 1, sizeof( Expression ) );
        if( lvalue == NULL ) ALLOC_ERROR();

        memcpy( lvalue, expression, sizeof( Expression ) );
        memset( expression, 0, sizeof( Expression ) );

        expression = parse_postfix( parser, lvalue );
        is_next_token_kind_postfix_operator = IS_TOKENKIND_IN_GROUP( parser->next_token.kind, TOKENKIND_POSTFIX_OPERATORS );
    }

    return expression;
}

static Expression* parse_rvalue( Parser* parser )
{
    if( !EXPECT( parser, TOKENKIND_RVALUE_STARTERS ) )
    {
        return NULL;
    }

    Token starting_token = parser->current_token;
    Expression* expression = parse_atom( parser );

    if( expression == NULL )
    {
        return NULL;
    }

    expression->starting_token = starting_token;

    bool is_next_token_kind_binary_operator = IS_TOKENKIND_IN_GROUP( parser->next_token.kind, TOKENKIND_BINARY_OPERATORS );
    if( is_next_token_kind_binary_operator )
    {
        advance( parser );

        Expression* left = calloc( 1, sizeof( Expression ) );
        if( left == NULL ) ALLOC_ERROR();

        memcpy( left, expression, sizeof( Expression ) );
        memset( expression, 0, sizeof( Expression ) );

        expression->kind = EXPRESSIONKIND_BINARY;
        expression->binary.operation = token_kind_to_binary_operation( parser->current_token.kind );
        expression->binary.operator_token = parser->current_token;
        expression->binary.left = left;

        advance( parser );
        expression->binary.right = parse_rvalue( parser );
        if( expression->binary.right == NULL )
        {
            return NULL;
        }
    }

    return expression;
}

static Expression* parse_variable_declaration( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_VARIABLEDECLARATION;
    expression->starting_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_IDENTIFIER ) )
    {
        return NULL;
    }

    expression->variable_declaration.identifier = parser->current_token.identifier;
    expression->variable_declaration.identifier_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_COLON, TOKENKIND_EQUAL ) )
    {
        return NULL;
    }

    expression->variable_declaration.type = ( Type ){ .kind = TYPEKIND_TOINFER };
    if( parser->current_token.kind == TOKENKIND_COLON )
    {
        advance( parser );
        expression->variable_declaration.type = parse_type( parser );

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_SEMICOLON, TOKENKIND_EQUAL ) )
        {
            return NULL;
        }

        if( parser->current_token.kind == TOKENKIND_SEMICOLON )
        {
            return expression;
        }
    }

    advance( parser );
    expression->variable_declaration.rvalue = parse_rvalue( parser );
    if( expression->variable_declaration.rvalue == NULL )
    {
        return NULL;
    }

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_SEMICOLON ) )
    {
        return NULL;
    }

    return expression;
}

static Expression* parse_compound( Parser* parser )
{
    if( !EXPECT( parser, TOKENKIND_LEFTBRACE ) )
    {
        return NULL;
    }

    Expression* compound_expression = calloc( 1, sizeof( Expression ) );
    if( compound_expression == NULL ) ALLOC_ERROR();

    compound_expression->compound.expressions = lvec_new( Expression* );
    if( compound_expression->compound.expressions == NULL ) ALLOC_ERROR();

    compound_expression->kind = EXPRESSIONKIND_COMPOUND;

    advance( parser );
    if ( !EXPECT( parser,TOKENKIND_EXPRESSION_STARTERS, TOKENKIND_RIGHTBRACE ) )
    {
        return NULL;
    }

    if( parser->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        while( IS_TOKENKIND_IN_GROUP( parser->current_token.kind, TOKENKIND_EXPRESSION_STARTERS ) )
        {
            Expression* expression = parse( parser );
            if( expression == NULL )
            {
                return NULL;
            }

            // technically we are leaking memory here but it should be okay since *expression
            // has to live for the rest of the program anyway
            lvec_append( compound_expression->compound.expressions, expression );
            advance( parser );
        }

        if( !EXPECT( parser, TOKENKIND_RIGHTBRACE ) )
        {
            return NULL;
        }
    }

    return compound_expression;
}

static Expression* parse_function_declaration( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FUNCTIONDECLARATION;
    expression->starting_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_IDENTIFIER ) )
    {
        return NULL;
    }

    expression->kind = EXPRESSIONKIND_FUNCTIONDECLARATION;
    expression->function_declaration.identifier = parser->current_token.identifier;
    expression->function_declaration.identifier_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_LEFTPAREN ) )
    {
        return NULL;
    }

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_IDENTIFIER, TOKENKIND_DOUBLEPERIOD, TOKENKIND_RIGHTPAREN ) )
    {
        return NULL;
    }

    expression->function_declaration.param_count = 0;

    if( parser->current_token.kind != TOKENKIND_RIGHTPAREN )
    {
        char** param_identifiers = lvec_new( char* );
        if( param_identifiers == NULL ) ALLOC_ERROR();

        Type* param_types = lvec_new( Type );
        if( param_types == NULL ) ALLOC_ERROR();

        Token* param_identifiers_tokens = lvec_new( Token );
        if( param_identifiers_tokens == NULL ) ALLOC_ERROR();

        Token* param_types_tokens = lvec_new( Token );
        if( param_types_tokens == NULL ) ALLOC_ERROR();

        // this entire block is so ugly
        while( parser->current_token.kind != TOKENKIND_RIGHTPAREN )
        {
            if( parser->current_token.kind == TOKENKIND_IDENTIFIER )
            {
                char* param_identifier = parser->current_token.identifier;
                Token param_identifier_token = parser->current_token;

                lvec_append( param_identifiers, param_identifier );
                lvec_append_aggregate( param_identifiers_tokens, param_identifier_token );

                advance( parser );
                if( !EXPECT( parser, TOKENKIND_COLON ) )
                {
                    return NULL;
                }

                advance( parser );
                Type param_type = parse_type( parser );
                Token param_type_token = parser->current_token;

                lvec_append_aggregate( param_types, param_type );
                lvec_append_aggregate( param_types_tokens, param_type_token );

                expression->function_declaration.param_count++;
            }
            else // TOKENKIND_DOUBLEPERIOD
            {
                expression->function_declaration.is_variadic = true;
            }

            advance( parser );
            bool is_variadic = expression->function_declaration.is_variadic;
            if( is_variadic && !EXPECT( parser, TOKENKIND_RIGHTPAREN ) )
            {
                return NULL;
            }

            if( !is_variadic && !EXPECT( parser, TOKENKIND_COMMA, TOKENKIND_RIGHTPAREN ) )
            {
                return NULL;
            }

            if( parser->current_token.kind == TOKENKIND_COMMA )
            {
                advance( parser );
            }
        }

        expression->function_declaration.param_identifiers = param_identifiers;
        expression->function_declaration.param_types = param_types;

        expression->function_declaration.param_identifiers_tokens = param_identifiers_tokens;
    }

    // current token is right paren

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_ARROW ) )
    {
        return NULL;
    }

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_TYPE_STARTERS ) )
    {
        return NULL;
    }

    expression->function_declaration.return_type = parse_type( parser );

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_LEFTBRACE, TOKENKIND_SEMICOLON ) )
    {
        return NULL;
    }

    if( parser->current_token.kind == TOKENKIND_LEFTBRACE )
    {
        expression->function_declaration.body = parse_compound( parser );
        if( expression->function_declaration.body == NULL )
        {
            return NULL;
        }
    }

    return expression;
}

static Expression* parse_return( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_RETURN;
    expression->starting_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_RVALUE_STARTERS, TOKENKIND_SEMICOLON ) )
    {
        return NULL;
    }

    if( parser->current_token.kind != TOKENKIND_SEMICOLON )
    {
        expression->return_expression.rvalue = parse_rvalue( parser );
        if( expression->return_expression.rvalue == NULL )
        {
            return NULL;
        }

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_SEMICOLON ) )
        {
            return NULL;
        }

    }

    return expression;
}

static Expression* parse_assignment( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_ASSIGNMENT;
    expression->starting_token = parser->current_token;
    expression->assignment.lvalue = parse_lvalue( parser );
    if( expression->assignment.lvalue == NULL )
    {
        return NULL;
    }

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_EQUAL ) )
    {
        return NULL;
    }

    advance( parser );
    expression->assignment.rvalue = parse_rvalue( parser );
    if( expression->assignment.rvalue == NULL )
    {
        return NULL;
    }

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_SEMICOLON ) )
    {
        return NULL;
    }

    return expression;
}

static Expression* parse_extern( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_EXTERN;
    expression->starting_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_FUNC ) )
    {
        return NULL;
    }

    expression->extern_expression.function = parse_function_declaration( parser );

    return expression;
}

static Expression* parse_conditional( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_CONDITIONAL;
    expression->starting_token = parser->current_token;

    bool is_loop;
    switch( parser->current_token.kind )
    {
        case TOKENKIND_IF:    is_loop = false; break;
        case TOKENKIND_WHILE: is_loop = true;  break;
        default: UNREACHABLE();
    }
    expression->conditional.is_loop = is_loop;

    advance( parser );
    expression->conditional.condition = parse_rvalue( parser );

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_EXPRESSION_STARTERS ) )
    {
        return NULL;
    }
    expression->conditional.true_body = parse( parser );

    if( parser->next_token.kind == TOKENKIND_ELSE )
    {
        advance( parser );
        advance( parser );
        if( !EXPECT( parser, TOKENKIND_EXPRESSION_STARTERS ) )
        {
            return NULL;
        }
        expression->conditional.false_body = parse( parser );
    }

    return expression;
}

static Expression* parse_lvalue( Parser* parser )
{
    if( !EXPECT( parser, TOKENKIND_LVALUE_STARTERS ) )
    {
        return NULL;
    }

    Expression* expression = parse_rvalue( parser );
    return expression;
}

static Expression* parse_for( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FORLOOP;
    expression->starting_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_IDENTIFIER ) )
    {
        return NULL;
    }

    expression->for_loop.iterator_token = parser->current_token;
    advance( parser );
    if( !EXPECT( parser, TOKENKIND_IN ) )
    {
        return NULL;
    }

    advance( parser );
    Expression* iterable_rvalue = parse_rvalue( parser );
    if( iterable_rvalue == NULL )
    {
        return NULL;
    }
    expression->for_loop.iterable_rvalue = iterable_rvalue;

    advance( parser );
    Expression* body = parse_compound( parser );
    if( body == NULL )
    {
        return NULL;
    }
    expression->for_loop.body = body;

    return expression;
}

static Expression* parse_type_declaration( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_TYPEDECLARATION;
    expression->starting_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_STRUCT, TOKENKIND_UNION ) )
    {
        return NULL;
    }

    expression->type_declaration.is_struct = parser->current_token.kind == TOKENKIND_STRUCT;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_IDENTIFIER ) )
    {
        return NULL;
    }

    expression->type_declaration.type_identifier_token = parser->current_token;

    advance( parser );
    if( !EXPECT( parser, TOKENKIND_LEFTBRACE ) )
    {
        return NULL;
    }

    Token* member_identifier_tokens = lvec_new( Token );
    Token* member_type_identifier_tokens = lvec_new( Token );

    advance( parser );
    while( parser->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        if( !EXPECT( parser, TOKENKIND_IDENTIFIER ) )
        {
            return NULL;
        }

        Token member_identifier_token = parser->current_token;
        lvec_append_aggregate( member_identifier_tokens, member_identifier_token );

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_COLON ) )
        {
            return NULL;
        }

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_IDENTIFIER ) )
        {
            return NULL;
        }

        Token member_type_identifier_token = parser->current_token;
        lvec_append_aggregate( member_type_identifier_tokens, member_type_identifier_token );

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_SEMICOLON ) )
        {
            return NULL;
        }

        advance( parser );
        if( !EXPECT( parser, TOKENKIND_IDENTIFIER, TOKENKIND_RIGHTBRACE ) )
        {
            return NULL;
        }
    }

    // current token is right brace

    expression->type_declaration.member_identifier_tokens = member_identifier_tokens;
    expression->type_declaration.member_type_identifier_tokens = member_type_identifier_tokens;
    expression->type_declaration.member_count = lvec_get_length( member_identifier_tokens );

    return expression;
}

Expression* parse( Parser* parser )
{
    Expression* expression;

    if( !EXPECT( parser, TOKENKIND_EXPRESSION_STARTERS ) )
    {
        return NULL;
    }

    switch( parser->current_token.kind )
    {
        case TOKENKIND_LET:
        {
            expression = parse_variable_declaration( parser );
            break;
        }

        case TOKENKIND_LEFTBRACE:
        {
            expression = parse_compound( parser );
            break;
        }

        case TOKENKIND_FUNC:
        {
            expression = parse_function_declaration( parser );
            break;
        }

        case TOKENKIND_IDENTIFIER:
        {
            if ( !EXPECT_NEXT( parser,
                               TOKENKIND_LEFTPAREN,
                               TOKENKIND_EQUAL,
                               TOKENKIND_LEFTBRACKET,
                               TOKENKIND_PERIOD ) )
            {
                return NULL;
            }

            switch( parser->next_token.kind )
            {
                // function call
                case TOKENKIND_LEFTPAREN:
                {
                    expression = parse_function_call( parser );
                    advance( parser );
                    if ( !EXPECT( parser, TOKENKIND_SEMICOLON ) )
                    {
                        return NULL;
                    }
                    break;
                }

                // assignment
                case TOKENKIND_LEFTBRACKET:
                case TOKENKIND_PERIOD:
                case TOKENKIND_EQUAL:
                {
                    expression = parse_assignment( parser );
                    break;
                }

                default:
                {
                    UNREACHABLE();
                    break;
                }
            }

            break;
        }

        // other lvalue starters (identifier was handled above)
        case TOKENKIND_STAR:
        case TOKENKIND_LEFTPAREN:
        {
            expression = parse_assignment( parser );
            break;
        }

        case TOKENKIND_RETURN:
        {
            expression = parse_return( parser );
            break;
        }

        case TOKENKIND_EXTERN:
        {
            expression = parse_extern( parser );
            break;
        }

        case TOKENKIND_WHILE:
        case TOKENKIND_IF:
        {
            expression = parse_conditional( parser );
            break;
        }

        case TOKENKIND_FOR:
        {
            expression = parse_for( parser );
            break;
        }

        case TOKENKIND_TYPE:
        {
            expression = parse_type_declaration( parser );
            break;
        }

        default:
        {
            UNIMPLEMENTED();
        }
    }

    return expression;
}
