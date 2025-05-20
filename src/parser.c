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

#define EXPECT( ... )\
    do {\
        TokenKind expecteds[] = { __VA_ARGS__ };\
        bool is_valid = false;\
        for( size_t i = 0; i < sizeof( expecteds ) / sizeof( *expecteds ); i++ )\
        {\
            if( parser.current_token.kind == expecteds[ i ] )\
            {\
                is_valid = true;\
                break;\
            }\
        }\
        if( !is_valid )\
        {\
            printf( "ERROR CALLED FROM LINE %d\n", __LINE__ );\
            Error error = {\
                .kind = ERRORKIND_UNEXPECTEDSYMBOL,\
                .offending_token = parser.current_token,\
            };\
            report_error( error );\
            return NULL;\
        }\
    } while( 0 )

#define EXPECT_NEXT( ... )\
    do {\
        TokenKind expecteds[] = { __VA_ARGS__ };\
        bool is_valid = false;\
        for( size_t i = 0; i < sizeof( expecteds ) / sizeof( *expecteds ); i++ )\
        {\
            if( parser.next_token.kind == expecteds[ i ] )\
            {\
                is_valid = true;\
                break;\
            }\
        }\
        if( !is_valid )\
        {\
            printf( "ERROR CALLED FROM LINE %d\n", __LINE__ );\
            Error error = {\
                .kind = ERRORKIND_UNEXPECTEDSYMBOL,\
                .offending_token = parser.current_token,\
            };\
            report_error( error );\
            return NULL;\
        }\
    } while( 0 )

static Parser parser;
static Token* tokens;

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

static void advance()
{
    if( parser.current_token.kind != TOKENKIND_EOF )
    {
        parser.current_token_index++;
    }

    parser.current_token = tokens[ parser.current_token_index ];
    parser.next_token = tokens[ parser.current_token_index + 1 ];
}

static void add_associated_tokens( Expression* expression, int token_start_index, int token_end_index )
{
    int associated_tokens_length = token_end_index - token_start_index;

    expression->associated_tokens = lvec_new( Token );
    if( expression->associated_tokens == NULL ) ALLOC_ERROR();

    lvec_reserve_minimum( expression->associated_tokens, associated_tokens_length );
    for( int i = token_start_index; i <= token_end_index; i++ )
    {
        lvec_append_aggregate( expression->associated_tokens, tokens[ i ] );
    }
}


static Expression* parse_integer()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_INTEGER;
    expression->integer = parser.current_token.integer;

    return expression;
}

static Expression* parse_float()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FLOAT;
    expression->float_ = parser.current_token.float_;

    return expression;
}

static Expression* parse_identifier()
{

    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_IDENTIFIER;
    expression->identifier = parser.current_token.identifier;

    return expression;
}

static Expression* parse_string()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_STRING;
    expression->string = parser.current_token.string;

    return expression;
}

static Expression* parse_character()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_CHARACTER;
    expression->character = parser.current_token.character;

    return expression;
}

static Expression* parse_base_expression()
{
    int token_start_index = parser.current_token_index;

    Expression* expression;

    switch( parser.current_token.kind )
    {
        case TOKENKIND_INTEGER:    expression = parse_integer();    break;
        case TOKENKIND_FLOAT:    expression = parse_float();    break;
        case TOKENKIND_IDENTIFIER: expression = parse_identifier(); break;
        case TOKENKIND_STRING:     expression = parse_string();     break;
        case TOKENKIND_CHARACTER:  expression = parse_character();  break;
        default: UNREACHABLE();
    }

    if( expression == NULL )
    {
        return NULL;
    }

    int token_end_index = parser.current_token_index;
    add_associated_tokens( expression, token_start_index, token_end_index );

    return expression;
}

static Expression* parse_rvalue();
static Expression* parse_parentheses()
{
    int token_start_index = parser.current_token_index;

    Expression* expression;

    advance();
    EXPECT( TOKENKIND_RVALUE_STARTERS );

    expression = parse_rvalue();

    advance();
    EXPECT( TOKENKIND_RIGHTPAREN );

    int token_end_index = parser.current_token_index;
    add_associated_tokens( expression, token_start_index, token_end_index );

    return expression;
}

static Expression* parse_unary()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_UNARY;
    expression->unary.operation = token_kind_to_unary_operation( parser.current_token.kind );

    advance();
    EXPECT( TOKENKIND_RVALUE_STARTERS );

    expression->unary.operand = parse_rvalue();

    return expression;
}

static Expression* parse_function_call()
{
    int token_start_index = parser.current_token_index;

    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FUNCTIONCALL;
    expression->function_call.identifier = parser.current_token.identifier;

    advance();
    // parser.current_token == TOKENKIND_LEFTPAREN

    advance();
    EXPECT( TOKENKIND_RIGHTPAREN, TOKENKIND_RVALUE_STARTERS );

    // if current token is right paren, there are no args to the function call
    if( parser.current_token.kind == TOKENKIND_RIGHTPAREN )
    {
        expression->function_call.args = NULL;
        expression->function_call.arg_count = 0;
    }
    else
    {
        Expression** args = lvec_new( Expression* );
        if( args == NULL ) ALLOC_ERROR();

        while( parser.current_token.kind != TOKENKIND_RIGHTPAREN )
        {
            Expression* e = parse_rvalue();
            if( e == NULL )
            {
                return NULL;
            }
            lvec_append( args, e );

            expression->function_call.arg_count++;

            advance();
            EXPECT( TOKENKIND_RIGHTPAREN, TOKENKIND_COMMA );

            if( parser.current_token.kind == TOKENKIND_COMMA )
            {
                advance();
            }
        }

        expression->function_call.args = args;
    }

    int token_end_index = parser.current_token_index;
    add_associated_tokens( expression, token_start_index, token_end_index );

    return expression;
}

static Expression* parse_rvalue()
{
    int token_start_index = parser.current_token_index;

    Expression* expression;

    switch( parser.current_token.kind )
    {
        case TOKENKIND_IDENTIFIER:
        {
            // check if function call
            if( parser.next_token.kind == TOKENKIND_LEFTPAREN )
            {
                expression = parse_function_call();
                break;
            }

            // if not function call, fallthrough
        }
        case TOKENKIND_INTEGER:
        case TOKENKIND_FLOAT:
        case TOKENKIND_STRING:
        case TOKENKIND_CHARACTER:
        {
            expression = parse_base_expression();
            break;
        }

        case TOKENKIND_LEFTPAREN:
        {
            expression = parse_parentheses();
            break;
        }

        case TOKENKIND_BANG:
        case TOKENKIND_MINUS:
        {
            expression = parse_unary();
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

    bool is_next_token_kind_binary_operator = IS_TOKENKIND_IN_GROUP( parser.next_token.kind, TOKENKIND_BINARY_OPERATORS );
    if( is_next_token_kind_binary_operator )
    {
        advance();

        Expression* left = calloc( 1, sizeof( Expression ) );
        if( left == NULL ) ALLOC_ERROR();

        memcpy( left, expression, sizeof( Expression ) );
        memset( expression, 0, sizeof( Expression ) );

        expression->kind = EXPRESSIONKIND_BINARY;
        expression->binary.operation = token_kind_to_binary_operation( parser.current_token.kind );
        expression->binary.left = left;

        advance();
        EXPECT( TOKENKIND_RVALUE_STARTERS );

        expression->binary.right = parse_rvalue();
        if( expression->binary.right == NULL )
        {
            return NULL;
        }
    }

    int token_end_index = parser.current_token_index;
    add_associated_tokens( expression, token_start_index, token_end_index );

    return expression;
}

// todo HASHMAP!!!
static Type parse_type()
{
    Type result;

    char* type_identifier = parser.current_token.identifier;

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

static Expression* parse_variable_declaration()
{
    int token_start_index = parser.current_token_index;

    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_VARIABLEDECLARATION;

    advance();
    EXPECT( TOKENKIND_IDENTIFIER );

    expression->variable_declaration.identifier = parser.current_token.identifier;

    advance();
    EXPECT( TOKENKIND_COLON, TOKENKIND_EQUAL );

    expression->variable_declaration.type = ( Type ){ .kind = TYPEKIND_TOINFER };
    if( parser.current_token.kind == TOKENKIND_COLON )
    {
        advance();
        EXPECT( TOKENKIND_IDENTIFIER );

        expression->variable_declaration.type = parse_type();

        advance();
        EXPECT( TOKENKIND_SEMICOLON, TOKENKIND_EQUAL );

        if( parser.current_token.kind == TOKENKIND_SEMICOLON )
        {
            return expression;
        }
    }

    advance();
    EXPECT( TOKENKIND_RVALUE_STARTERS );
    expression->variable_declaration.rvalue = parse_rvalue();
    if( expression->variable_declaration.rvalue == NULL )
    {
        return NULL;
    }

    advance();
    EXPECT( TOKENKIND_SEMICOLON );

    int token_end_index = parser.current_token_index;
    add_associated_tokens( expression, token_start_index, token_end_index );

    return expression;
}

Expression* parse_compound()
{
    int token_start_index = parser.current_token_index;

    Expression* compound_expression = calloc( 1, sizeof( Expression ) );
    if( compound_expression == NULL ) ALLOC_ERROR();

    compound_expression->compound.expressions = lvec_new( Expression* );
    if( compound_expression->compound.expressions == NULL ) ALLOC_ERROR();

    compound_expression->kind = EXPRESSIONKIND_COMPOUND;

    advance();
    EXPECT( TOKENKIND_EXPRESSION_STARTERS, TOKENKIND_RIGHTBRACE );

    if( parser.current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        while( IS_TOKENKIND_IN_GROUP( parser.current_token.kind, TOKENKIND_EXPRESSION_STARTERS ) )
        {
            Expression* expression = parse( tokens );
            if( expression == NULL )
            {
                return NULL;
            }

            // technically we are leaking memory here but it should be okay since *expression
            // has to live for the rest of the program anyway
            lvec_append( compound_expression->compound.expressions, expression );
            advance();
        }

        EXPECT( TOKENKIND_RIGHTBRACE );
    }

    int token_end_index = parser.current_token_index;
    add_associated_tokens( compound_expression, token_start_index, token_end_index );

    return compound_expression;
}

Expression* parse_function_declaration()
{
    int token_start_index = parser.current_token_index;

    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FUNCTIONDECLARATION;

    advance();
    EXPECT( TOKENKIND_IDENTIFIER );

    expression->kind = EXPRESSIONKIND_FUNCTIONDECLARATION;
    expression->function_declaration.identifier = parser.current_token.identifier;

    advance();
    EXPECT( TOKENKIND_LEFTPAREN );

    advance();
    EXPECT( TOKENKIND_IDENTIFIER, TOKENKIND_RIGHTPAREN );

    expression->function_declaration.param_count = 0;

    if( parser.current_token.kind == TOKENKIND_IDENTIFIER )
    {
        char** param_identifiers = lvec_new( char* );
        if( param_identifiers == NULL ) ALLOC_ERROR();

        Type* param_types = lvec_new( Type );
        if( param_types == NULL ) ALLOC_ERROR();

        while( parser.current_token.kind != TOKENKIND_RIGHTPAREN )
        {
            char* param_identifier = parser.current_token.identifier;
            lvec_append( param_identifiers, param_identifier );

            advance();
            EXPECT( TOKENKIND_COLON );

            advance();
            EXPECT( TOKENKIND_IDENTIFIER );

            Type param_type = parse_type();
            lvec_append_aggregate( param_types, param_type );

            expression->function_declaration.param_count++;

            advance();
            EXPECT( TOKENKIND_COMMA, TOKENKIND_RIGHTPAREN );

            if( parser.current_token.kind == TOKENKIND_COMMA )
            {
                advance();
            }
        }

        expression->function_declaration.param_identifiers = param_identifiers;
        expression->function_declaration.param_types = param_types;
    }

    // current token is right paren

    advance();
    EXPECT( TOKENKIND_ARROW );

    advance();
    EXPECT( TOKENKIND_IDENTIFIER );

    expression->function_declaration.return_type = parse_type();

    advance();
    EXPECT( TOKENKIND_LEFTBRACE );

    expression->function_declaration.body = parse_compound();
    if( expression->function_declaration.body == NULL )
    {
        return NULL;
    }

    int token_end_index = parser.current_token_index;
    add_associated_tokens( expression, token_start_index, token_end_index );

    return expression;
}

Expression* parse_return()
{
    int token_start_index = parser.current_token_index;

    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_RETURN;

    advance();
    EXPECT( TOKENKIND_RVALUE_STARTERS, TOKENKIND_SEMICOLON );

    if( parser.current_token.kind != TOKENKIND_SEMICOLON )
    {
        expression->return_expression.rvalue = parse_rvalue();
        if( expression->return_expression.rvalue == NULL )
        {
            return NULL;
        }

        advance();
        EXPECT( TOKENKIND_SEMICOLON );
    }

    int token_end_index = parser.current_token_index;
    add_associated_tokens( expression, token_start_index, token_end_index );

    return expression;
}

Expression* parse_assignment()
{
    int token_start_index = parser.current_token_index;

    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_ASSIGNMENT;
    expression->assignment.identifier = parser.current_token.identifier;

    advance();
    EXPECT( TOKENKIND_EQUAL );

    advance();
    EXPECT( TOKENKIND_RVALUE_STARTERS );

    expression->assignment.rvalue = parse_rvalue();
    if( expression->assignment.rvalue == NULL )
    {
        return NULL;
    }

    advance();
    EXPECT( TOKENKIND_SEMICOLON );

    int token_end_index = parser.current_token_index;
    add_associated_tokens( expression, token_start_index, token_end_index );

    return expression;
}

Expression* parse( Token* _tokens )
{
    // initialize parser
    static bool is_parser_initialized = false;
    if( !is_parser_initialized )
    {
        is_parser_initialized = true;
        tokens = _tokens;
        parser.current_token_index = -1;
        advance();
    }

    Expression* expression;

    EXPECT( TOKENKIND_EXPRESSION_STARTERS );

    int token_start_index = parser.current_token_index;
    switch( parser.current_token.kind )
    {
        case TOKENKIND_LET:
        {
            expression = parse_variable_declaration();
            break;
        }

        case TOKENKIND_LEFTBRACE:
        {
            expression = parse_compound();
            break;
        }

        case TOKENKIND_FUNC:
        {
            expression = parse_function_declaration();
            break;
        }

        case TOKENKIND_IDENTIFIER:
        {
            EXPECT_NEXT( TOKENKIND_LEFTPAREN, TOKENKIND_EQUAL);

            switch( parser.next_token.kind )
            {
                case TOKENKIND_LEFTPAREN:
                {
                    // function call
                    expression = parse_function_call();
                    advance();
                    EXPECT( TOKENKIND_SEMICOLON );
                    break;
                }

                case TOKENKIND_EQUAL:
                {
                    // assignment
                    expression = parse_assignment();
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
            expression = parse_return();
            break;
        }

        default:
        {
            UNIMPLEMENTED();
        }
    }

    if( expression != NULL )
    {
        int token_end_index = parser.current_token_index;
        add_associated_tokens( expression, token_start_index, token_end_index );
    }

    return expression;
}
