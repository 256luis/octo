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

#define EXPECT( parser_ptr, ... )                                       \
    {                                                                   \
        TokenKind expecteds[] = { __VA_ARGS__ };                        \
        bool is_valid = false;                                          \
        for( size_t i = 0; i < sizeof( expecteds ) / sizeof( *expecteds ); i++ ) \
        {                                                               \
            if( parser_ptr->current_token.kind == expecteds[ i ] )      \
            {                                                           \
                is_valid = true;                                        \
                break;                                                  \
            }                                                           \
        }                                                               \
        if( !is_valid )                                                 \
        {                                                               \
            printf( "ERROR CALLED FROM LINE %d\n", __LINE__ );          \
            Error error = {                                             \
                .kind = ERRORKIND_UNEXPECTEDSYMBOL,                     \
                .line = parser->current_token.line,                     \
                .column = parser->current_token.column,                 \
            };                                                          \
            report_error( error );                                      \
            return NULL;                                                \
        }                                                               \
    } ( ( void )0 )

#define EXPECT_NEXT( parser_ptr, ... )                                  \
    {                                                                   \
        TokenKind expecteds[] = { __VA_ARGS__ };                        \
        bool is_valid = false;                                          \
        for( size_t i = 0; i < sizeof( expecteds ) / sizeof( *expecteds ); i++ ) \
        {                                                               \
            if( parser_ptr->next_token.kind == expecteds[ i ] )         \
            {                                                           \
                is_valid = true;                                        \
                break;                                                  \
            }                                                           \
        }                                                               \
        if( !is_valid )                                                 \
        {                                                               \
            printf( "ERROR CALLED FROM LINE %d\n", __LINE__ );          \
            Error error = {                                             \
                .kind = ERRORKIND_UNEXPECTEDSYMBOL,                     \
                .line = parser->next_token.line,                        \
                .column = parser->next_token.column,                    \
            };                                                          \
            report_error( error );                                      \
            return NULL;                                                \
        }                                                               \
    } ( ( void )0 )

#define TOKENKIND_EXPRESSION_BASES                                      \
    TOKENKIND_INTEGER, TOKENKIND_IDENTIFIER, TOKENKIND_STRING, TOKENKIND_CHARACTER

#define TOKENKIND_UNARY_OPERATORS               \
    TOKENKIND_MINUS, TOKENKIND_BANG

#define TOKENKIND_EXPRESSION_STARTERS                                   \
    TOKENKIND_EXPRESSION_BASES, TOKENKIND_UNARY_OPERATORS, TOKENKIND_LEFTPAREN, \
    TOKENKIND_LET, TOKENKIND_FUNC, TOKENKIND_IDENTIFIER, TOKENKIND_LEFTBRACE, \
    TOKENKIND_RETURN

#define TOKENKIND_BINARY_OPERATORS                                      \
    TOKENKIND_PLUS, TOKENKIND_MINUS,TOKENKIND_STAR, TOKENKIND_FORWARDSLASH, \
    TOKENKIND_GREATER, TOKENKIND_LESS, TOKENKIND_DOUBLEEQUAL, TOKENKIND_NOTEQUAL, \
    TOKENKIND_GREATEREQUAL, TOKENKIND_LESSEQUAL

#define TOKENKIND_EXPRESSION_STATEMENT_STARTERS\
    TOKENKIND_LET, TOKENKIND_LEFTBRACE, TOKENKIND_FUNC, TOKENKIND_IDENTIFIER, \
    TOKENKIND_RETURN

#define GET_TOKENKIND_GROUP_COUNT( ... )                                \
    ( sizeof( ( TokenKind[] ){ __VA_ARGS__ } ) / sizeof( TokenKind ) )

#define IS_TOKENKIND_IN_GROUP( token_kind, ... )                        \
    _is_token_kind_in_group( token_kind,                                \
                             ( TokenKind[] ){ __VA_ARGS__ },            \
                             GET_TOKENKIND_GROUP_COUNT( __VA_ARGS__ ) )

#define MAX_FUNCTION_PARAM_COUNT 100

static bool _is_token_kind_in_group( TokenKind kind, TokenKind* group, size_t count )
{
    for( size_t i = 0; i < count; i++ )
    {
        if( kind == group[ i ] )
        {
            return true;
        }
    }

    return false;
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
        default: UNREACHABLE();
    }
}

static UnaryOperation token_kind_to_unary_operation( TokenKind token_kind )
{
    switch( token_kind )
    {
        case TOKENKIND_BANG: return UNARYOPERATION_NOT;
        case TOKENKIND_MINUS: return UNARYOPERATION_NEGATIVE;
        default: UNREACHABLE();
    }
}

static void advance( Parser* parser )
{
    parser->current_token = parser->tokens[ parser->current_token_index ];

    if( parser->current_token.kind != TOKENKIND_EOF )
    {
        parser->current_token_index++;
    }

    parser->next_token = parser->tokens[ parser->current_token_index ];
}

Parser* parser_new( Token* token_list )
{
    Parser* parser = calloc( 1, sizeof( Parser ) );
    if( parser == NULL ) ALLOC_ERROR();

    parser->tokens = token_list;
    advance( parser );

    return parser;
}

void parser_free( Parser* parser )
{
    free( parser );
}

static Expression* parse_integer( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_INTEGER;
    expression->number = parser->current_token.number;

    return expression;
}

static Expression* parse_identifier( Parser* parser )
{

    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_IDENTIFIER;
    expression->identifier = parser->current_token.identifier;

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

static Expression* parse_base_expression( Parser* parser )
{
    switch( parser->current_token.kind )
    {
        case TOKENKIND_INTEGER:     return parse_integer( parser );
        case TOKENKIND_IDENTIFIER: return parse_identifier( parser );
        case TOKENKIND_STRING:     return parse_string( parser );
        case TOKENKIND_CHARACTER:  return parse_character( parser );
        default: UNREACHABLE();
    }
}

static Expression* parse_expression( Parser* parser );
static Expression* parse_parentheses( Parser* parser )
{
    Expression* expression;

    advance( parser );
    EXPECT( parser, TOKENKIND_EXPRESSION_STARTERS );

    expression = parse_expression( parser );

    advance( parser );
    EXPECT( parser, TOKENKIND_RIGHTPAREN );

    return expression;
}

static Expression* parse_unary( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_UNARY;
    expression->unary.operation = token_kind_to_unary_operation( parser->current_token.kind );

    advance( parser );
    EXPECT( parser,
            TOKENKIND_EXPRESSION_BASES,
            TOKENKIND_UNARY_OPERATORS,
            TOKENKIND_LEFTPAREN );

    bool is_current_token_expression_base = IS_TOKENKIND_IN_GROUP( parser->current_token.kind, TOKENKIND_EXPRESSION_BASES );
    bool is_current_token_unary_operator  = IS_TOKENKIND_IN_GROUP( parser->current_token.kind, TOKENKIND_UNARY_OPERATORS );
    if( is_current_token_expression_base )
    {
        expression->unary.operand = parse_base_expression( parser );
    }
    else if( is_current_token_unary_operator )
    {
        expression->unary.operand = parse_unary( parser );
    }
    else // parser->current_token.kind == TOKENKIND_LEFTPAREN
    {
        expression->unary.operand = parse_parentheses( parser );
    }

    return expression;
}

static Expression* parse_function_call( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FUNCTIONCALL;
    expression->function_call.identifier = parser->current_token.identifier;

    advance( parser );
    // parser->current_token == TOKENKIND_LEFTPAREN

    advance( parser );
    EXPECT( parser,
            TOKENKIND_RIGHTPAREN,
            TOKENKIND_EXPRESSION_STARTERS );

    // if current token is right paren, there are no args to the function call
    if( parser->current_token.kind == TOKENKIND_RIGHTPAREN )
    {
        expression->function_call.args = NULL;
        expression->function_call.arg_count = 0;
    }
    else
    {
        Expression** args = lvec_new( Expression* );
        while( parser->current_token.kind != TOKENKIND_RIGHTPAREN )
        {
            Expression* e = parse_expression( parser );
            if( e == NULL )
            {
                return NULL;
            }
            lvec_append( args, e );

            expression->function_call.arg_count++;

            advance( parser );
            EXPECT( parser,
                    TOKENKIND_RIGHTPAREN,
                    TOKENKIND_COMMA );

            if( parser->current_token.kind == TOKENKIND_COMMA )
            {
                advance( parser );
            }
        }

        expression->function_call.args = args;
    }

    return expression;
}

static Expression* parse_expression( Parser* parser )
{
    Expression* expression;

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

            // if not function call, fallthrough
        }
        case TOKENKIND_INTEGER:
        case TOKENKIND_STRING:
        case TOKENKIND_CHARACTER:
        {
            expression = parse_base_expression( parser );
            break;
        }

        case TOKENKIND_LEFTPAREN:
        {
            expression = parse_parentheses( parser );
            break;
        }

        case TOKENKIND_BANG:
        case TOKENKIND_MINUS:
        {
            expression = parse_unary( parser );
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
        expression->binary.left = left;

        advance( parser );
        EXPECT( parser, TOKENKIND_EXPRESSION_STARTERS );

        expression->binary.right = parse_expression( parser );
        if( expression->binary.right == NULL )
        {
            return NULL;
        }
    }

    return expression;
}

// todo HASHMAP!!!
static Type parse_type( Parser* parser )
{
    Type result;

    char* type_identifier = parser->current_token.identifier;

    if( strcmp( type_identifier, "int" ) == 0 )
    {
        result.kind = TYPEKIND_INTEGER;
    }
    else if( strcmp( type_identifier, "float" ) == 0 )
    {
        result.kind = TYPEKIND_FLOAT;
    }
    else if( strcmp( type_identifier, "char" ) == 0 )
    {
        result.kind = TYPEKIND_CHARACTER;
    }
    else if( strcmp( type_identifier, "string" ) == 0 )
    {
        result.kind = TYPEKIND_STRING;
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
        result.custom_identifier = type_identifier;
    }

    return result;
}

static Expression* parse_variable_declaration( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_VARIABLEDECLARATION;

    advance( parser );
    EXPECT( parser, TOKENKIND_IDENTIFIER );

    expression->variable_declaration.identifier = parser->current_token.identifier;

    advance( parser );
    EXPECT( parser,
            TOKENKIND_COLON,
            TOKENKIND_EQUAL );

    expression->variable_declaration.type = ( Type ){ .kind = TYPEKIND_TOINFER };
    if( parser->current_token.kind == TOKENKIND_COLON )
    {
        advance( parser );
        EXPECT( parser, TOKENKIND_IDENTIFIER );

        expression->variable_declaration.type = parse_type( parser );

        advance( parser );
        EXPECT( parser,
                TOKENKIND_SEMICOLON,
                TOKENKIND_EQUAL );

        if( parser->current_token.kind == TOKENKIND_SEMICOLON )
        {
            return expression;
        }
    }

    advance( parser );
    EXPECT( parser, TOKENKIND_EXPRESSION_STARTERS );
    expression->variable_declaration.value = parse_expression( parser );
    if( expression->variable_declaration.value == NULL )
    {
        return NULL;
    }

    advance( parser );
    EXPECT( parser, TOKENKIND_SEMICOLON );

    return expression;
}

Expression* parse_compound( Parser* parser )
{
    Expression* compound_expression = calloc( 1, sizeof( Expression ) );
    if( compound_expression == NULL ) ALLOC_ERROR();

    compound_expression->compound.expressions = lvec_new( Expression );
    compound_expression->kind = EXPRESSIONKIND_COMPOUND;

    advance( parser );
    EXPECT( parser,
            TOKENKIND_EXPRESSION_STARTERS,
            TOKENKIND_RIGHTBRACE );

    if( parser->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        while( IS_TOKENKIND_IN_GROUP( parser->current_token.kind, TOKENKIND_EXPRESSION_STARTERS ) )
        {
            Expression* expression = parser_parse( parser );
            if( expression == NULL )
            {
                return NULL;
            }

            // technically we are leaking memory here but it should be okay since *expression
            // has to live for the rest of the program anyway
            lvec_append_aggregate( compound_expression->compound.expressions, *expression );
            advance( parser );
        }

        // advance( parser );
        EXPECT( parser, TOKENKIND_RIGHTBRACE );
    }

    return compound_expression;
}

Expression* parse_function_declaration( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FUNCTIONDECLARATION;

    advance( parser );
    EXPECT( parser, TOKENKIND_IDENTIFIER );

    expression->kind = EXPRESSIONKIND_FUNCTIONDECLARATION;
    expression->function_declaration.identifier = parser->current_token.identifier;

    advance( parser );
    EXPECT( parser, TOKENKIND_LEFTPAREN );

    advance( parser );
    EXPECT( parser, TOKENKIND_IDENTIFIER, TOKENKIND_RIGHTPAREN );

    expression->function_declaration.param_count = 0;

    if( parser->current_token.kind == TOKENKIND_IDENTIFIER )
    {
        char** param_identifiers = lvec_new( char* );
        Type* param_types = lvec_new( Type );

        while( parser->current_token.kind != TOKENKIND_RIGHTPAREN )
        {
            char* param_identifier = parser->current_token.identifier;
            lvec_append( param_identifiers, param_identifier );

            advance( parser );
            EXPECT( parser, TOKENKIND_COLON );

            advance( parser );
            EXPECT( parser, TOKENKIND_IDENTIFIER );

            Type param_type = parse_type( parser );
            lvec_append_aggregate( param_types, param_type );

            expression->function_declaration.param_count++;

            advance( parser );
            EXPECT( parser,
                    TOKENKIND_COMMA,
                    TOKENKIND_RIGHTPAREN );

            if( parser->current_token.kind == TOKENKIND_COMMA )
            {
                advance( parser );
            }
        }

        expression->function_declaration.param_identifiers = param_identifiers;
        expression->function_declaration.param_types = param_types;
    }

    // current token is right paren

    advance( parser );
    EXPECT( parser, TOKENKIND_ARROW );

    advance( parser );
    EXPECT( parser, TOKENKIND_IDENTIFIER );

    expression->function_declaration.return_type = parse_type( parser );

    advance( parser );
    EXPECT( parser, TOKENKIND_LEFTBRACE );

    expression->function_declaration.body = parse_compound( parser );
    if( expression->function_declaration.body == NULL )
    {
        return NULL;
    }

    return expression;
}

Expression* parse_return( Parser* parser )
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_RETURN;

    advance( parser );
    EXPECT( parser, TOKENKIND_EXPRESSION_STARTERS );

    expression->return_expression.value = parse_expression( parser );
    if( expression->return_expression.value == NULL )
    {
        return NULL;
    }

    advance( parser );
    EXPECT( parser, TOKENKIND_SEMICOLON );

    return expression;
}

Expression* parse_assignment( Parser* parser )
{
    // UNIMPLEMENTED();
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_ASSIGNMENT;
    expression->assignment.identifier = parser->current_token.identifier;

    advance( parser );
    EXPECT( parser, TOKENKIND_EQUAL );

    advance( parser );
    EXPECT( parser, TOKENKIND_EXPRESSION_STARTERS );

    expression->assignment.value = parse_expression( parser );
    if( expression->assignment.value == NULL )
    {
        return NULL;
    }

    advance( parser );
    EXPECT( parser, TOKENKIND_SEMICOLON );

    return expression;
}

Expression* parser_parse( Parser* parser )
{
    Expression* expression;
    printf( "PARSING: %s\n", token_kind_to_string[ parser->current_token.kind ] );

    EXPECT( parser, TOKENKIND_EXPRESSION_STATEMENT_STARTERS );

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
            EXPECT_NEXT( parser,
                         TOKENKIND_LEFTPAREN,
                         TOKENKIND_EQUAL);

            switch( parser->next_token.kind )
            {
                case TOKENKIND_LEFTPAREN:
                {
                    // function call
                    expression = parse_function_call( parser );
                    advance( parser );
                    EXPECT( parser, TOKENKIND_SEMICOLON );
                    break;
                }

                case TOKENKIND_EQUAL:
                {
                    // assignment
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

        case TOKENKIND_RETURN:
        {
            expression = parse_return( parser );
            break;
        }

        default:
        {
            UNIMPLEMENTED();
        }
    }

    return expression;
}
