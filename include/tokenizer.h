#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "error.h"
#include <stdbool.h>
#include <stdint.h>
#include "array_list.h"

typedef enum TokenKind
{
    TOKENKIND_PLUS,
    TOKENKIND_MINUS,
    TOKENKIND_STAR,
    TOKENKIND_FORWARDSLASH,
    TOKENKIND_GREATER,
    TOKENKIND_LESS,
    TOKENKIND_DOUBLEEQUAL,
    TOKENKIND_NOTEQUAL,
    TOKENKIND_GREATEREQUAL,
    TOKENKIND_LESSEQUAL,

    TOKENKIND_LET,
    TOKENKIND_RETURN,
    TOKENKIND_FUNC,
    TOKENKIND_NUMBER,
    TOKENKIND_IDENTIFIER,
    TOKENKIND_STRING,
    TOKENKIND_CHARACTER,

    TOKENKIND_SEMICOLON,
    TOKENKIND_COLON,
    TOKENKIND_DOUBLECOLON,
    TOKENKIND_PERIOD,
    TOKENKIND_COMMA,
    TOKENKIND_ARROW,

    TOKENKIND_EQUAL,

    TOKENKIND_BANG,

    TOKENKIND_LEFTPAREN,
    TOKENKIND_RIGHTPAREN,
    TOKENKIND_LEFTBRACE,
    TOKENKIND_RIGHTBRACE,
    TOKENKIND_LEFTBRACKET,
    TOKENKIND_RIGHTBRACKET,

    TOKENKIND_EOF,
} TokenKind;

typedef struct Token
{
    TokenKind kind;
    int line;
    int column;

    union
    {
        int number;
        char character;
        char* string;
        char* identifier;
    };
} Token;

/* typedef struct TokenNode */
/* { */
/*     Token token; */
/*     struct TokenNode* next; */
/* } TokenNode; */

typedef struct TokenList
{
    ArrayList list;
    /* TokenNode* head; */
    /* TokenNode* tail; */
    /* int length; */
} TokenList;

typedef enum TokenizerState
{
    TOKENIZERSTATE_START     = 0x00,
    TOKENIZERSTATE_NUMBER    = 0x10,
    TOKENIZERSTATE_SPECIAL   = 0x20,
    TOKENIZERSTATE_WORD      = 0x30,
    TOKENIZERSTATE_STRING    = 0x40,
    TOKENIZERSTATE_CHARACTER = 0x50,
} TokenizerState;

typedef struct Tokenizer
{
    // FILE* source_file;
    // char* source_code;
    SourceCode source_code;
    // int source_code_length;
    int current_character_index;
    char character;
    char symbol[512];
    int symbol_last_index;
    TokenizerState state;
    int line;
    int column;
    bool error_found;
    bool in_string;
    bool in_character;
} Tokenizer;

Tokenizer* tokenizer_new( SourceCode source_code );
void tokenizer_free( Tokenizer* tokenizer );
TokenList tokenizer_tokenize( Tokenizer* tokenizer );

TokenList token_list_new();
void token_list_free( TokenList tokens );
void token_list_append( TokenList* tokens, Token value );
Token token_list_get( TokenList tokens, int index );

#endif
