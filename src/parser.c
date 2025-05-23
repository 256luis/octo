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
    _expect( ( TokenKind[] ){ __VA_ARGS__ },\
             sizeof( ( TokenKind[] ){ __VA_ARGS__ } ) / sizeof( TokenKind ),\
             __LINE__ )

#define EXPECT_NEXT( ... )\
    _expect_next( ( TokenKind[] ){ __VA_ARGS__ },\
                  sizeof( ( TokenKind[] ){ __VA_ARGS__ } ) / sizeof( TokenKind ),\
                  __LINE__ )

static Parser parser;
static Token* tokens;

static bool _expect( TokenKind* expected_token_kinds, size_t length, int line )
{
    bool is_valid = false;
    for( size_t i = 0; i < length; i++ )
    {
        if( parser.current_token.kind == expected_token_kinds[ i ] )
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
            .offending_token = parser.current_token,
        };
        report_error( error );
    }

    return is_valid;
}

static bool _expect_next( TokenKind* expected_token_kinds, size_t length, int line )
{
    bool is_valid = false;
    for( size_t i = 0; i < length; i++ )
    {
        if( parser.next_token.kind == expected_token_kinds[ i ] )
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
            .offending_token = parser.current_token,
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

static void advance()
{
    if( parser.current_token.kind != TOKENKIND_EOF )
    {
        parser.current_token_index++;
    }

    parser.current_token = tokens[ parser.current_token_index ];
    parser.next_token = tokens[ parser.current_token_index + 1 ];
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
    expression->floating = parser.current_token.floating;

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


static Expression* parse_boolean()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_BOOLEAN;
    expression->boolean = parser.current_token.boolean;

    return expression;
}

static Expression* parse_base_expression()
{
    Expression* expression;

    switch( parser.current_token.kind )
    {
        case TOKENKIND_INTEGER:    expression = parse_integer();    break;
        case TOKENKIND_FLOAT:      expression = parse_float();      break;
        case TOKENKIND_IDENTIFIER: expression = parse_identifier(); break;
        case TOKENKIND_STRING:     expression = parse_string();     break;
        case TOKENKIND_CHARACTER:  expression = parse_character();  break;
        case TOKENKIND_BOOLEAN:    expression = parse_boolean();  break;
        default: UNREACHABLE();
    }

    if( expression == NULL )
    {
        return NULL;
    }

    expression->associated_token = parser.current_token;

    return expression;
}

static Expression* parse_rvalue();
static Expression* parse_parentheses()
{
    Expression* expression;

    advance();
    if ( !EXPECT( TOKENKIND_RVALUE_STARTERS ) )
    {
        return NULL;
    }

    expression = parse_rvalue();

    advance();
    if( !EXPECT( TOKENKIND_RIGHTPAREN ) )
    {
        return NULL;
    }

    return expression;
}

static Expression* parse_unary()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_UNARY;
    expression->unary.operation = token_kind_to_unary_operation( parser.current_token.kind );
    expression->unary.operator_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_RVALUE_STARTERS ) )
    {
        return NULL;
    }

    expression->unary.operand = parse_rvalue();

    return expression;
}

static Expression* parse_function_call()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression ==  NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FUNCTIONCALL;
    expression->function_call.identifier = parser.current_token.identifier;
    expression->function_call.identifier_token = parser.current_token;

    advance();
    // parser.current_token == TOKENKIND_LEFTPAREN

    advance();
    if( !EXPECT( TOKENKIND_RIGHTPAREN, TOKENKIND_RVALUE_STARTERS ) )
    {
        return NULL;
    }

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
            if( !EXPECT( TOKENKIND_RIGHTPAREN, TOKENKIND_COMMA ) )
            {
                return NULL;
            }

            if( parser.current_token.kind == TOKENKIND_COMMA )
            {
                advance();
            }
        }

        expression->function_call.args = args;
    }

    return expression;
}

static Expression* parse_rvalue()
{
    Expression* expression;
    Token starting_token = parser.current_token;

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
        case TOKENKIND_BOOLEAN:
        {
            expression = parse_base_expression();
            break;
        }

        case TOKENKIND_LEFTPAREN:
        {
            expression = parse_parentheses();
            break;
        }

        case TOKENKIND_AMPERSAND:
        case TOKENKIND_STAR:
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
        expression->binary.operator_token = parser.current_token;
        expression->binary.left = left;

        advance();
        if( !EXPECT( TOKENKIND_RVALUE_STARTERS ) )
        {
            return NULL;
        }

        expression->binary.right = parse_rvalue();
        if( expression->binary.right == NULL )
        {
            return NULL;
        }
    }

    expression->starting_token = starting_token;

    return expression;
}

// todo HASHMAP!!!
static Type parse_type()
{
    Type result;
    if( parser.current_token.kind == TOKENKIND_IDENTIFIER )
    {
        char* type_identifier = parser.current_token.identifier;

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
        /* else if( strcmp( type_identifier, "string" ) == 0 ) */
        /* { */
        /*     result.kind = TYPEKIND_STRING; */
        /* } */
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
    }
    else // ampersand
    {
        result.kind = TYPEKIND_POINTER;
        advance();
        if( !EXPECT( TOKENKIND_IDENTIFIER, TOKENKIND_AMPERSAND ) )
        {
            result.kind = TYPEKIND_INVALID;
            return result;
        }

        // i have to allocate here... SO ANNOYING!!!
        result.pointer.type = malloc( sizeof( Type ) );
        *result.pointer.type = parse_type();
    }

    return result;
}

static Expression* parse_variable_declaration()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_VARIABLEDECLARATION;
    expression->starting_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_IDENTIFIER ) )
    {
        return NULL;
    }

    expression->variable_declaration.identifier = parser.current_token.identifier;
    expression->variable_declaration.identifier_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_COLON, TOKENKIND_EQUAL ) )
    {
        return NULL;
    }

    expression->variable_declaration.type = ( Type ){ .kind = TYPEKIND_TOINFER };
    if( parser.current_token.kind == TOKENKIND_COLON )
    {
        advance();
        if( !EXPECT( TOKENKIND_IDENTIFIER, TOKENKIND_AMPERSAND ) )
        {
            return NULL;
        }

        expression->variable_declaration.type = parse_type();
        expression->variable_declaration.type_token = parser.current_token;

        advance();
        if( !EXPECT( TOKENKIND_SEMICOLON, TOKENKIND_EQUAL ) )
        {
            return NULL;
        }

        if( parser.current_token.kind == TOKENKIND_SEMICOLON )
        {
            return expression;
        }
    }

    advance();
    if( !EXPECT( TOKENKIND_RVALUE_STARTERS ) )
    {
        return NULL;
    }
    expression->variable_declaration.rvalue = parse_rvalue();
    if( expression->variable_declaration.rvalue == NULL )
    {
        return NULL;
    }

    advance();
    if( !EXPECT( TOKENKIND_SEMICOLON ) )
    {
        return NULL;
    }

    return expression;
}

Expression* parse_compound()
{
    Expression* compound_expression = calloc( 1, sizeof( Expression ) );
    if( compound_expression == NULL ) ALLOC_ERROR();

    compound_expression->compound.expressions = lvec_new( Expression* );
    if( compound_expression->compound.expressions == NULL ) ALLOC_ERROR();

    compound_expression->kind = EXPRESSIONKIND_COMPOUND;

    advance();
    if ( !EXPECT( TOKENKIND_EXPRESSION_STARTERS, TOKENKIND_RIGHTBRACE ) )
    {
        return NULL;
    }

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

        if( !EXPECT( TOKENKIND_RIGHTBRACE ) )
        {
            return NULL;
        }
    }

    return compound_expression;
}

Expression* parse_function_declaration()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_FUNCTIONDECLARATION;
    expression->starting_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_IDENTIFIER ) )
    {
        return NULL;
    }

    expression->kind = EXPRESSIONKIND_FUNCTIONDECLARATION;
    expression->function_declaration.identifier = parser.current_token.identifier;
    expression->function_declaration.identifier_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_LEFTPAREN ) )
    {
        return NULL;
    }

    advance();
    if( !EXPECT( TOKENKIND_IDENTIFIER, TOKENKIND_RIGHTPAREN ) )
    {
        return NULL;
    }

    expression->function_declaration.param_count = 0;

    if( parser.current_token.kind == TOKENKIND_IDENTIFIER )
    {
        char** param_identifiers = lvec_new( char* );
        if( param_identifiers == NULL ) ALLOC_ERROR();

        Type* param_types = lvec_new( Type );
        if( param_types == NULL ) ALLOC_ERROR();

        Token* param_identifiers_tokens = lvec_new( Token );
        if( param_identifiers_tokens == NULL ) ALLOC_ERROR();

        Token* param_types_tokens = lvec_new( Token );
        if( param_types_tokens == NULL ) ALLOC_ERROR();

        while( parser.current_token.kind != TOKENKIND_RIGHTPAREN )
        {
            char* param_identifier = parser.current_token.identifier;
            Token param_identifier_token = parser.current_token;

            lvec_append( param_identifiers, param_identifier );
            lvec_append_aggregate( param_identifiers_tokens, param_identifier_token );

            advance();
            if( !EXPECT( TOKENKIND_COLON ) )
            {
                return NULL;
            }

            advance();
            if( !EXPECT( TOKENKIND_IDENTIFIER, TOKENKIND_AMPERSAND ) )
            {
                return NULL;
            }

            Type param_type = parse_type();
            Token param_type_token = parser.current_token;

            lvec_append_aggregate( param_types, param_type );
            lvec_append_aggregate( param_types_tokens, param_type_token );

            expression->function_declaration.param_count++;

            advance();
            if( !EXPECT( TOKENKIND_COMMA, TOKENKIND_RIGHTPAREN ) )
            {
                return NULL;
            }

            if( parser.current_token.kind == TOKENKIND_COMMA )
            {
                advance();
            }
        }

        expression->function_declaration.param_identifiers = param_identifiers;
        expression->function_declaration.param_types = param_types;

        expression->function_declaration.param_identifiers_tokens = param_identifiers_tokens;
        expression->function_declaration.param_types_tokens = param_types_tokens;
    }

    // current token is right paren

    advance();
    if( !EXPECT( TOKENKIND_ARROW ) )
    {
        return NULL;
    }

    advance();
    if( !EXPECT( TOKENKIND_IDENTIFIER, TOKENKIND_AMPERSAND ) )
    {
        return NULL;
    }

    expression->function_declaration.return_type = parse_type();
    expression->function_declaration.return_type_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_LEFTBRACE, TOKENKIND_SEMICOLON ) )
    {
        return NULL;
    }

    if( parser.current_token.kind == TOKENKIND_LEFTBRACE )
    {
        expression->function_declaration.body = parse_compound();
        if( expression->function_declaration.body == NULL )
        {
            return NULL;
        }
    }

    return expression;
}

Expression* parse_return()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_RETURN;
    expression->starting_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_RVALUE_STARTERS, TOKENKIND_SEMICOLON ) )
    {
        return NULL;
    }

    if( parser.current_token.kind != TOKENKIND_SEMICOLON )
    {
        expression->return_expression.rvalue = parse_rvalue();
        if( expression->return_expression.rvalue == NULL )
        {
            return NULL;
        }

        advance();
        if( !EXPECT( TOKENKIND_SEMICOLON ) )
        {
            return NULL;
        }

    }

    return expression;
}

Expression* parse_assignment()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_ASSIGNMENT;
    expression->starting_token = parser.current_token;
    expression->assignment.identifier = parser.current_token.identifier;
    expression->assignment.identifier_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_EQUAL ) )
    {
        return NULL;
    }

    advance();
    if( !EXPECT( TOKENKIND_RVALUE_STARTERS ) )
    {
        return NULL;
    }

    expression->assignment.rvalue = parse_rvalue();
    if( expression->assignment.rvalue == NULL )
    {
        return NULL;
    }

    advance();
    if( !EXPECT( TOKENKIND_SEMICOLON ) )
    {
        return NULL;
    }

    return expression;
}

static Expression* parse_extern()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_EXTERN;
    expression->starting_token = parser.current_token;

    advance();
    if( !EXPECT( TOKENKIND_FUNC ) )
    {
        return NULL;
    }

    expression->extern_expression.function = parse_function_declaration();

    return expression;
}

static Expression* parse_conditional()
{
    Expression* expression = calloc( 1, sizeof( Expression ) );
    if( expression == NULL ) ALLOC_ERROR();

    expression->kind = EXPRESSIONKIND_CONDITIONAL;
    expression->starting_token = parser.current_token;

    bool is_loop;
    switch( parser.current_token.kind )
    {
        case TOKENKIND_IF:    is_loop = false; break;
        case TOKENKIND_WHILE: is_loop = true;  break;
        default: UNREACHABLE();
    }
    expression->conditional.is_loop = is_loop;

    advance();
    if( !EXPECT( TOKENKIND_RVALUE_STARTERS ) )
    {
        return NULL;
    }
    expression->conditional.condition = parse_rvalue();

    advance();
    if( !EXPECT( TOKENKIND_EXPRESSION_STARTERS ) )
    {
        return NULL;
    }
    expression->conditional.true_body = parse( tokens );

    if( parser.next_token.kind == TOKENKIND_ELSE )
    {
        advance();
        advance();
        if( !EXPECT( TOKENKIND_EXPRESSION_STARTERS ) )
        {
            return NULL;
        }
        expression->conditional.false_body = parse( tokens );
    }

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

    if( !EXPECT( TOKENKIND_EXPRESSION_STARTERS ) )
    {
        return NULL;
    }

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
            if ( !EXPECT_NEXT( TOKENKIND_LEFTPAREN, TOKENKIND_EQUAL ) )
            {
                return NULL;
            }

            switch( parser.next_token.kind )
            {
                case TOKENKIND_LEFTPAREN:
                {
                    // function call
                    expression = parse_function_call();
                    advance();
                    if ( !EXPECT( TOKENKIND_SEMICOLON ) )
                    {
                        return NULL;
                    }
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

        case TOKENKIND_EXTERN:
        {
            expression = parse_extern();
            break;
        }

        case TOKENKIND_WHILE:
        case TOKENKIND_IF:
        {
            expression = parse_conditional();
            break;
        }

        default:
        {
            UNIMPLEMENTED();
        }
    }

    return expression;
}
