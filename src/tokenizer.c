#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "tokenizer.h"
#include "error.h"
#include "lvec.h"
#include "globals.h"

#define MAX_SYMBOL_LENGTH 512

typedef enum CharacterType
{
    CHARACTERTYPE_SPACE    = 0x00,
    CHARACTERTYPE_NUMBER   = 0x01,
    CHARACTERTYPE_SPECIAL  = 0x02,
    CHARACTERTYPE_WORD    = 0x03,
} CharacterType;

bool _is_token_kind_in_group( TokenKind kind, TokenKind* group, size_t count )
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

static CharacterType get_character_type(char c)
{
    if( isdigit( c ) )             return CHARACTERTYPE_NUMBER;
    if( isspace( c ) )             return CHARACTERTYPE_SPACE;
    if( isalpha( c ) || c == '_' ) return CHARACTERTYPE_WORD;

    return CHARACTERTYPE_SPECIAL;
}

static char* valid_special_symbols[] = {
    ";", ":", "::", ".", ",",
    "+", "-", "*", "/", "=", "->",
    "!", ">", "<", "==", "!=", ">=", "<=",
    "(", ")",
    "{", "}",
    "[", "]",
    "&", "..",
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

    if( strcmp( special_symbol, "[" ) == 0 )  return TOKENKIND_LEFTBRACKET;
    if( strcmp( special_symbol, "]" ) == 0 )  return TOKENKIND_RIGHTBRACKET;
    if( strcmp( special_symbol, "&" ) == 0 )  return TOKENKIND_AMPERSAND;
    if( strcmp( special_symbol, ".." ) == 0 ) return TOKENKIND_DOUBLEPERIOD;

    UNREACHABLE();
}

// temporary function
// TODO: hashmap
static TokenKind word_symbol_to_token_kind( const char* word_symbol )
{
    if( strcmp( word_symbol, "let" ) == 0 )    return TOKENKIND_LET;
    if( strcmp( word_symbol, "func" ) == 0 )   return TOKENKIND_FUNC;
    if( strcmp( word_symbol, "return" ) == 0 ) return TOKENKIND_RETURN;
    if( strcmp( word_symbol, "true" ) == 0 )   return TOKENKIND_BOOLEAN;
    if( strcmp( word_symbol, "false" ) == 0 )  return TOKENKIND_BOOLEAN;
    if( strcmp( word_symbol, "extern" ) == 0 ) return TOKENKIND_EXTERN;
    if( strcmp( word_symbol, "if" ) == 0 )     return TOKENKIND_IF;
    if( strcmp( word_symbol, "else" ) == 0 )   return TOKENKIND_ELSE;
    if( strcmp( word_symbol, "while" ) == 0 )  return TOKENKIND_WHILE;
    if( strcmp( word_symbol, "for" ) == 0 )    return TOKENKIND_FOR;
    if( strcmp( word_symbol, "in" ) == 0 )     return TOKENKIND_IN;
    if( strcmp( word_symbol, "type" ) == 0 )   return TOKENKIND_TYPE;
    if( strcmp( word_symbol, "struct" ) == 0 ) return TOKENKIND_STRUCT;
    if( strcmp( word_symbol, "union" ) == 0 )  return TOKENKIND_UNION;

    return TOKENKIND_IDENTIFIER;

    // UNREACHABLE();
}

static bool advance( Tokenizer* tokenizer )
{
    tokenizer->character = g_source_code.code[ tokenizer->current_character_index ];
    tokenizer->next_character = g_source_code.code[ tokenizer->current_character_index + 1 ];
    tokenizer->current_character_index++;
    tokenizer->column++;
    if( tokenizer->character == '\n' )
    {
        tokenizer->line++;
        tokenizer->column = 0;
    }

    return tokenizer->current_character_index <= g_source_code.length;
}

static void append_to_symbol( Tokenizer* tokenizer )
{
    tokenizer->symbol[ tokenizer->symbol_last_index ] = tokenizer->character;
    tokenizer->symbol[ tokenizer->symbol_last_index + 1 ] = 0;
    tokenizer->symbol_last_index++;
}

static void finalize_symbol( Tokenizer* tokenizer, Token** tokens )
{
    int symbol_start_column = tokenizer->column - strlen( tokenizer->symbol );
    Token token = {
        .line = tokenizer->line,
        .column = symbol_start_column,
        .as_string = malloc( strlen( tokenizer->symbol ) + 1 ),
    };
    strcpy( token.as_string, tokenizer->symbol );

    switch( tokenizer->state )
    {
        case TOKENIZERSTATE_STRING:
        {
            token.kind = TOKENKIND_STRING;
            token.string = malloc( strlen( tokenizer->symbol ) + 1 );
            if( token.string == NULL ) ALLOC_ERROR();

            strcpy( token.string, tokenizer->symbol );
            break;
        }

        case TOKENIZERSTATE_CHARACTER:
        {
            // verify that the current symbol only has one character
            if( strlen( tokenizer->symbol ) != 1 )
            {
                Error error = {
                    .kind = ERRORKIND_MULTICHARACTERCHARACTER,
                    .offending_token = token,
                };

                report_error( error );
                tokenizer->error_found = true;
            }
            else
            {
                token.kind = TOKENKIND_CHARACTER;
                token.character = tokenizer->symbol[ 0 ];
            }
            break;
        }

        case TOKENIZERSTATE_FLOAT:
        {
            token.kind = TOKENKIND_FLOAT;
            token.floating = strtod( tokenizer->symbol, NULL );
            break;
        }

        default:
        {
            CharacterType character_type = get_character_type( tokenizer->symbol[ 0 ] );
            switch( character_type )
            {
                case CHARACTERTYPE_NUMBER:
                {
                    token.kind = TOKENKIND_INTEGER;
                    token.integer = strtoull( tokenizer->symbol, NULL, 10 );
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
                            .kind = ERRORKIND_INVALIDSYMBOL,
                            /* .line = tokenizer->line, */
                            /* .column = symbol_start_column, */

                            .offending_token = token
                        };

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
                    else if( token.kind == TOKENKIND_BOOLEAN )
                    {
                        token.boolean = strcmp( token.as_string, "true" ) ? true : false;
                    }

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
    }

    tokenizer->symbol_last_index = 0;
    memset( tokenizer->symbol, 0, sizeof( char ) * MAX_SYMBOL_LENGTH );

    if( !tokenizer->error_found )
    {
        lvec_append_aggregate( *tokens, token );
    }
}

Token* tokenize()
{
    // initialize tokenizer
    Tokenizer tokenizer = { 0 };
    tokenizer.line = 1;
    tokenizer.column = 0;
    tokenizer.state = TOKENIZERSTATE_START;
    tokenizer.current_character_index = 0;
    tokenizer.in_string = false;
    tokenizer.in_character = false;

    Token* tokens = lvec_new( Token );

    // this is needed because the parser will only parse multiple statements if
    // they are enclosed in `{}`
    lvec_append_aggregate( tokens, ( Token ){ .kind = TOKENKIND_LEFTBRACE } );

    bool in_comment = false;
    while( advance( &tokenizer ) )
    {
        // comment handling
        if( in_comment )
        {
            if( tokenizer.character == '\n' )
            {
                in_comment = false;
            }
            else
            {
                continue;
            }
        }

        if( tokenizer.character == '/' && tokenizer.next_character == '/' )
        {
            in_comment = true;
            continue;
        }

        // string handling
        if( tokenizer.character == '\"')
        {
            tokenizer.in_string = !tokenizer.in_string;

            if( tokenizer.state != TOKENIZERSTATE_START )
            {
                finalize_symbol( &tokenizer, &tokens );
            }

            // if exiting string
            if( !tokenizer.in_string )
            {
                tokenizer.state = TOKENIZERSTATE_START;
            }


            continue;
        }

        if( tokenizer.in_string )
        {
            append_to_symbol( &tokenizer );
            tokenizer.state = TOKENIZERSTATE_STRING;
            continue;
        }

        // character handling
        if( tokenizer.character == '\'')
        {
            // printf("here\n");
            tokenizer.in_character = !tokenizer.in_character;

            if( tokenizer.state != TOKENIZERSTATE_START )
            {
                finalize_symbol( &tokenizer, &tokens );
            }

            // if exiting character
            if( !tokenizer.in_character )
            {
                tokenizer.state = TOKENIZERSTATE_START;
            }

            continue;
        }

        if( tokenizer.in_character )
        {
            append_to_symbol( &tokenizer );
            tokenizer.state = TOKENIZERSTATE_CHARACTER;
            continue;
        }

        CharacterType current_ctype = get_character_type( tokenizer.character );
        CharacterType next_ctype = get_character_type( tokenizer.next_character );
        switch( tokenizer.state | current_ctype )
        {
            case TOKENIZERSTATE_START | CHARACTERTYPE_SPACE:
            {
                // do nothing
                break;
            }

            case TOKENIZERSTATE_START | CHARACTERTYPE_NUMBER:
            {
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_INTEGER;
                break;
            }

            case TOKENIZERSTATE_START | CHARACTERTYPE_SPECIAL:
            {
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_SPECIAL;
                break;
            }

            case TOKENIZERSTATE_START | CHARACTERTYPE_WORD:
            {
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_WORD;

                break;
            }

            case TOKENIZERSTATE_FLOAT | CHARACTERTYPE_SPACE:
            {
                finalize_symbol( &tokenizer, &tokens );
                tokenizer.state = TOKENIZERSTATE_START;
                break;
            }

            case TOKENIZERSTATE_FLOAT | CHARACTERTYPE_NUMBER:
            {
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_FLOAT;
                break;
            }

            case TOKENIZERSTATE_FLOAT | CHARACTERTYPE_SPECIAL:
            {
                finalize_symbol( &tokenizer, &tokens );
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_SPECIAL;

                break;
            }

            case TOKENIZERSTATE_FLOAT | CHARACTERTYPE_WORD:
            {
                finalize_symbol( &tokenizer, &tokens );
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_WORD;

                break;
            }

            case TOKENIZERSTATE_INTEGER | CHARACTERTYPE_SPACE:
            {
                finalize_symbol( &tokenizer, &tokens );
                tokenizer.state = TOKENIZERSTATE_START;
                break;
            }

            case TOKENIZERSTATE_INTEGER | CHARACTERTYPE_NUMBER:
            {
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_INTEGER;
                break;
            }

            case TOKENIZERSTATE_INTEGER | CHARACTERTYPE_SPECIAL:
            {
                if( tokenizer.character == '.' && next_ctype == CHARACTERTYPE_NUMBER )
                {
                    // float token
                    append_to_symbol( &tokenizer );
                    tokenizer.state = TOKENIZERSTATE_FLOAT;
                }
                else
                {
                    // integer token
                    finalize_symbol( &tokenizer, &tokens );
                    append_to_symbol( &tokenizer );
                    tokenizer.state = TOKENIZERSTATE_SPECIAL;
                }

                break;
            }

            case TOKENIZERSTATE_INTEGER | CHARACTERTYPE_WORD:
            {
                finalize_symbol( &tokenizer, &tokens );
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_WORD;

                break;
            }

            case TOKENIZERSTATE_SPECIAL | CHARACTERTYPE_SPACE:
            {
                finalize_symbol( &tokenizer, &tokens );
                tokenizer.state = TOKENIZERSTATE_START;
                break;
            }

            case TOKENIZERSTATE_SPECIAL | CHARACTERTYPE_NUMBER:
            {
                finalize_symbol( &tokenizer, &tokens );
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_INTEGER;
                break;
            }

            case TOKENIZERSTATE_SPECIAL | CHARACTERTYPE_SPECIAL:
            {
                // check if tokenizer.symbol + tokenizer.character is a valid symbol
                char new_symbol[ MAX_SYMBOL_LENGTH ] = { 0 };
                strcpy(new_symbol, tokenizer.symbol);
                new_symbol[ strlen(new_symbol) ] = tokenizer.character;

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
                    append_to_symbol( &tokenizer );
                }
                else
                {
                    finalize_symbol( &tokenizer, &tokens );
                    append_to_symbol( &tokenizer );
                }
                tokenizer.state = TOKENIZERSTATE_SPECIAL;

                break;
            }

            case TOKENIZERSTATE_SPECIAL | CHARACTERTYPE_WORD:
            {
                finalize_symbol( &tokenizer, &tokens );
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_WORD;

                break;
            }

            case TOKENIZERSTATE_WORD | CHARACTERTYPE_SPACE:
            {
                finalize_symbol( &tokenizer, &tokens );
                tokenizer.state = TOKENIZERSTATE_START;
                break;
            }

            case TOKENIZERSTATE_WORD | CHARACTERTYPE_NUMBER:
            {
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_WORD;
                break;
            }

            case TOKENIZERSTATE_WORD | CHARACTERTYPE_SPECIAL:
            {
                finalize_symbol( &tokenizer, &tokens );
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_SPECIAL;
                break;
            }

            case TOKENIZERSTATE_WORD | CHARACTERTYPE_WORD:
            {
                append_to_symbol( &tokenizer );
                tokenizer.state = TOKENIZERSTATE_WORD;
                break;
            }
        }
    }

    if( strlen( tokenizer.symbol ) > 0 )
    {
        finalize_symbol( &tokenizer, &tokens );
    }

    // this is needed because the parser will only parse multiple statements if
    // they are enclosed in `{}`
    lvec_append_aggregate( tokens, ( Token ){ .kind = TOKENKIND_RIGHTBRACE } );

    // eof token
    Token eof = {
        .kind = TOKENKIND_EOF,
        .line = tokenizer.line,
        .column = tokenizer.column,
    };

    lvec_append_aggregate( tokens, eof );

    if( tokenizer.error_found )
    {
        return NULL;
    }

    return tokens;
}
