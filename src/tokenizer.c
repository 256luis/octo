#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "tokenizer.h"
#include "error.h"
#include "lvec.h"

#define MAX_SYMBOL_LENGTH 512

typedef enum CharacterType
{
    CHARACTERTYPE_SPACE    = 0x00,
    CHARACTERTYPE_NUMBER   = 0x01,
    CHARACTERTYPE_SPECIAL  = 0x02,
    CHARACTERTYPE_WORD    = 0x03,
} CharacterType;

static CharacterType get_character_type(char c)
{
    if( isdigit( c ) )             return CHARACTERTYPE_NUMBER;
    if( isspace( c ) )             return CHARACTERTYPE_SPACE;
    if( isalpha( c ) || c == '_' ) return CHARACTERTYPE_WORD;

    return CHARACTERTYPE_SPECIAL;
}

Tokenizer* tokenizer_new( SourceCode source_code )
{
    Tokenizer* tokenizer = calloc( 1, sizeof( Tokenizer ) );
    if( tokenizer == NULL ) return NULL;

    tokenizer->line = 1;
    tokenizer->column = 0;
    tokenizer->state = TOKENIZERSTATE_START;
    tokenizer->current_character_index = 0;

    /* tokenizer->source_file = fopen( source_file_path, "r" ); */
    /* if( tokenizer->source_file == NULL ) return NULL; */

    tokenizer->source_code = source_code;


    return tokenizer;
}

void tokenizer_free( Tokenizer* tokenizer )
{
    free( tokenizer );
}

static char* valid_special_symbols[] = {
    ";", ":", "::", ".", ",",
    "+", "-", "*", "/", "=", "->",
    "!", ">", "<", "==", "!=", ">=", "<=",
    "(", ")",
    "{", "}",
    "[", "]",
};

// temporary function
// TODO: hashmap
static TokenKind special_symbol_to_token_kind( const char* special_symbol )
{
    if( strcmp( special_symbol, ";" ) == 0 )  return TOKENKIND_SEMICOLON;
    if( strcmp( special_symbol, ":" ) == 0 )  return TOKENKIND_COLON;
    if( strcmp( special_symbol, "::" ) == 0 ) return TOKENKIND_DOUBLECOLON;
    if( strcmp( special_symbol, "." ) == 0 )  return TOKENKIND_PERIOD;
    if( strcmp( special_symbol, "," ) == 0 )  return TOKENKIND_COMMA;

    if( strcmp( special_symbol, "+" ) == 0 )  return TOKENKIND_PLUS;
    if( strcmp( special_symbol, "-" ) == 0 )  return TOKENKIND_MINUS;
    if( strcmp( special_symbol, "*" ) == 0 )  return TOKENKIND_STAR;
    if( strcmp( special_symbol, "/" ) == 0 )  return TOKENKIND_FORWARDSLASH;
    if( strcmp( special_symbol, "=" ) == 0 )  return TOKENKIND_EQUAL;
    if( strcmp( special_symbol, "->" ) == 0 ) return TOKENKIND_ARROW;

    if( strcmp( special_symbol, "!" ) == 0 )  return TOKENKIND_BANG;
    if( strcmp( special_symbol, ">" ) == 0 )  return TOKENKIND_GREATER;
    if( strcmp( special_symbol, "<" ) == 0 )  return TOKENKIND_LESS;
    if( strcmp( special_symbol, "==" ) == 0 ) return TOKENKIND_DOUBLEEQUAL;
    if( strcmp( special_symbol, "!=" ) == 0 ) return TOKENKIND_NOTEQUAL;
    if( strcmp( special_symbol, ">=" ) == 0 ) return TOKENKIND_GREATEREQUAL;
    if( strcmp( special_symbol, "<=" ) == 0 ) return TOKENKIND_LESSEQUAL;

    if( strcmp( special_symbol, "(" ) == 0 )  return TOKENKIND_LEFTPAREN;
    if( strcmp( special_symbol, ")" ) == 0 )  return TOKENKIND_RIGHTPAREN;

    if( strcmp( special_symbol, "{" ) == 0 )  return TOKENKIND_LEFTBRACE;
    if( strcmp( special_symbol, "}" ) == 0 )  return TOKENKIND_RIGHTBRACE;

    if( strcmp( special_symbol, "{" ) == 0 )  return TOKENKIND_LEFTBRACKET;
    if( strcmp( special_symbol, "}" ) == 0 )  return TOKENKIND_RIGHTBRACKET;

    UNREACHABLE();
}

// temporary function
// TODO: hashmap
static TokenKind word_symbol_to_token_kind( const char* word_symbol )
{
    if( strcmp( word_symbol, "let" ) == 0 )  return TOKENKIND_LET;
    if( strcmp( word_symbol, "func" ) == 0 ) return TOKENKIND_FUNC;
    if( strcmp( word_symbol, "return" ) == 0 ) return TOKENKIND_RETURN;

    return TOKENKIND_IDENTIFIER;

    // UNREACHABLE();
}

static bool advance( Tokenizer* tokenizer )
{
    // tokenizer->character = tokenizer->next_character;
    // tokenizer->next_character = fgetc( tokenizer->source_file );
    tokenizer->character = tokenizer->source_code.code[ tokenizer->current_character_index ];
    tokenizer->current_character_index++;
    tokenizer->column++;
    if( tokenizer->character == '\n' )
    {
        tokenizer->line++;
        tokenizer->column = 1;
    }

    return tokenizer->current_character_index <= tokenizer->source_code.length;
}

static void tokenizer_append_to_symbol( Tokenizer* tokenizer )
{
    tokenizer->symbol[ tokenizer->symbol_last_index ] = tokenizer->character;
    tokenizer->symbol[ tokenizer->symbol_last_index + 1 ] = 0;
    tokenizer->symbol_last_index++;
}

static void tokenizer_finalize_symbol( Tokenizer* tokenizer, Token* tokens )
{
    int symbol_start_column = tokenizer->column - strlen( tokenizer->symbol );
    Token token = {
        .line = tokenizer->line,
        .column = symbol_start_column
    };

    if( tokenizer->state == TOKENIZERSTATE_STRING )
    {
        token.kind = TOKENKIND_STRING;
        token.string = malloc( strlen( tokenizer->symbol ) + 1 );
        if( token.string == NULL ) ALLOC_ERROR();

        strcpy( token.string, tokenizer->symbol );
    }
    else if( tokenizer->state == TOKENIZERSTATE_CHARACTER )
    {
        // verify that the current symbol only has one character
        if( strlen( tokenizer->symbol ) != 1 )
        {
            Error error = {
                // .source_file = tokenizer->source_file,
                .kind = ERRORKIND_MULTICHARACTERCHARACTER,
                .line = tokenizer->line,
                .column = symbol_start_column,
                .source_code = tokenizer->source_code
            };

            // strcpy( error.symbol, tokenizer->symbol );
            report_error( error );
            tokenizer->error_found = true;
        }
        else
        {
            token.kind = TOKENKIND_CHARACTER;
            token.character = tokenizer->symbol[ 0 ];
        }
    }
    else
    {
        CharacterType character_type = get_character_type( tokenizer->symbol[ 0 ] );
        switch( character_type )
        {
            case CHARACTERTYPE_NUMBER:
            {
                token.kind = TOKENKIND_INTEGER;
                token.number = strtoull( tokenizer->symbol, NULL, 10 );
                break;
            }

            case CHARACTERTYPE_SPECIAL:
            {
                // check if symbol is valid
                bool symbol_is_valid = false;
                for( size_t i = 0; i < sizeof( valid_special_symbols ) / sizeof( *valid_special_symbols ); i++ )
                {
                    if( strcmp(tokenizer->symbol, valid_special_symbols[i] ) == 0 )
                    {
                        symbol_is_valid = true;
                        break;
                    }
                }

                if( symbol_is_valid )
                {
                    // find the corresponding TokenKind for the symbol
                    token.kind = special_symbol_to_token_kind( tokenizer->symbol );
                }
                else
                {
                    Error error = {
                        // .source_file = tokenizer->source_file,
                        .kind = ERRORKIND_INVALIDSYMBOL,
                        .line = tokenizer->line,
                        .column = symbol_start_column,
                        .source_code = tokenizer->source_code
                    };

                    // strcpy( error.symbol, tokenizer->symbol );
                    report_error( error );
                    tokenizer->error_found = true;
                }

                break;
            }

            case CHARACTERTYPE_WORD:
            {
                token.kind = word_symbol_to_token_kind( tokenizer->symbol );

                if( token.kind == TOKENKIND_IDENTIFIER )
                {
                    token.identifier = malloc( strlen( tokenizer->symbol ) + 1 );
                    if( token.identifier == NULL ) ALLOC_ERROR();

                    strcpy( token.identifier, tokenizer->symbol );
                }

                break;
            }

            default:
            {
                UNREACHABLE();
                break;
            }
        }
    }

    tokenizer->symbol_last_index = 0;
    memset( tokenizer->symbol, 0, sizeof( char ) * MAX_SYMBOL_LENGTH );
    if( !tokenizer->error_found )
    {
        lvec_append_aggregate( tokens, token );
    }
}

Token* tokenizer_tokenize( Tokenizer* tokenizer )
{
    Token* tokens = lvec_new( Token );

    tokenizer->in_string = false;
    tokenizer->in_character = false;

    while( advance( tokenizer ) )
    {
        // string handling
        if( tokenizer->character == '\"')
        {
            tokenizer->in_string = !tokenizer->in_string;

            if( tokenizer->state != TOKENIZERSTATE_START )
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
            }

            // if exiting string
            if( !tokenizer->in_string )
            {
                tokenizer->state = TOKENIZERSTATE_START;
            }


            continue;
        }

        if( tokenizer->in_string )
        {
            tokenizer_append_to_symbol( tokenizer );
            tokenizer->state = TOKENIZERSTATE_STRING;
            continue;
        }

        // character handling
        if( tokenizer->character == '\'')
        {
            // printf("here\n");
            tokenizer->in_character = !tokenizer->in_character;

            if( tokenizer->state != TOKENIZERSTATE_START )
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
            }

            // if exiting character
            if( !tokenizer->in_character )
            {
                tokenizer->state = TOKENIZERSTATE_START;
            }

            continue;
        }

        if( tokenizer->in_character )
        {
            tokenizer_append_to_symbol( tokenizer );
            tokenizer->state = TOKENIZERSTATE_CHARACTER;
            continue;
        }

        CharacterType current_ctype = get_character_type( tokenizer->character );
        switch( tokenizer->state | current_ctype )
        {
            case TOKENIZERSTATE_START | CHARACTERTYPE_SPACE:
            {
                // do nothing
                break;
            }

            case TOKENIZERSTATE_START | CHARACTERTYPE_NUMBER:
            {
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_NUMBER;
                break;
            }

            case TOKENIZERSTATE_START | CHARACTERTYPE_SPECIAL:
            {
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_SPECIAL;
                break;
            }

            case TOKENIZERSTATE_START | CHARACTERTYPE_WORD:
            {
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_WORD;

                break;
            }

            case TOKENIZERSTATE_NUMBER | CHARACTERTYPE_SPACE:
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
                tokenizer->state = TOKENIZERSTATE_START;
                break;
            }

            case TOKENIZERSTATE_NUMBER | CHARACTERTYPE_NUMBER:
            {
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_NUMBER;
                break;
            }

            case TOKENIZERSTATE_NUMBER | CHARACTERTYPE_SPECIAL:
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_SPECIAL;
                break;
            }

            case TOKENIZERSTATE_NUMBER | CHARACTERTYPE_WORD:
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_WORD;

                break;
            }

            case TOKENIZERSTATE_SPECIAL | CHARACTERTYPE_SPACE:
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
                tokenizer->state = TOKENIZERSTATE_START;
                break;
            }

            case TOKENIZERSTATE_SPECIAL | CHARACTERTYPE_NUMBER:
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_NUMBER;
                break;
            }

            case TOKENIZERSTATE_SPECIAL | CHARACTERTYPE_SPECIAL:
            {
                // check if tokenizer->symbol + tokenizer->character is a valid symbol
                char new_symbol[ MAX_SYMBOL_LENGTH ] = { 0 };
                strcpy(new_symbol, tokenizer->symbol);
                new_symbol[ strlen(new_symbol) ] = tokenizer->character;

                bool new_symbol_is_valid = false;
                for( size_t i = 0; i < sizeof( valid_special_symbols ) / sizeof( *valid_special_symbols ); i++ )
                {
                    if( strcmp(new_symbol, valid_special_symbols[i] ) == 0 )
                    {
                        new_symbol_is_valid = true;
                        break;
                    }
                }

                if( new_symbol_is_valid )
                {
                    tokenizer_append_to_symbol( tokenizer );
                }
                else
                {
                    tokenizer_finalize_symbol( tokenizer, tokens );
                    tokenizer_append_to_symbol( tokenizer );
                }
                tokenizer->state = TOKENIZERSTATE_SPECIAL;

                break;
            }

            case TOKENIZERSTATE_SPECIAL | CHARACTERTYPE_WORD:
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_WORD;

                break;
            }

            case TOKENIZERSTATE_WORD | CHARACTERTYPE_SPACE:
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
                tokenizer->state = TOKENIZERSTATE_START;
                break;
            }

            case TOKENIZERSTATE_WORD | CHARACTERTYPE_NUMBER:
            {
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_WORD;
                break;
            }

            case TOKENIZERSTATE_WORD | CHARACTERTYPE_SPECIAL:
            {
                tokenizer_finalize_symbol( tokenizer, tokens );
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_SPECIAL;
                break;
            }

            case TOKENIZERSTATE_WORD | CHARACTERTYPE_WORD:
            {
                tokenizer_append_to_symbol( tokenizer );
                tokenizer->state = TOKENIZERSTATE_WORD;
                break;
            }
        }
    }

    if( strlen( tokenizer->symbol ) > 0 )
    {
        tokenizer_finalize_symbol( tokenizer, tokens );
    }

    // eof token
    Token eof = {
        .kind = TOKENKIND_EOF,
        .line = tokenizer->line,
        .column = tokenizer->column,
    };

    lvec_append_aggregate( tokens, eof );

    return tokens;
}
