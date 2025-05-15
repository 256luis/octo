#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "error.h"
#include <stdbool.h>
#include <stdint.h>

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
    TOKENKIND_INTEGER,
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

// Tokenizer* tokenizer_new();
void tokenizer_free( Tokenizer* tokenizer );
Token* tokenize();

#endif
