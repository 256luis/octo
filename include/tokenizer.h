#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define GET_TOKENKIND_GROUP_COUNT( ... )\
    ( sizeof( ( TokenKind[] ){ __VA_ARGS__ } ) / sizeof( TokenKind ) )

#define TOKENKIND_IS_IN_GROUP( token_kind, ... )\
    _token_kind_is_in_group( token_kind,\
                             ( TokenKind[] ){ __VA_ARGS__ },\
                             GET_TOKENKIND_GROUP_COUNT( __VA_ARGS__ ) )

#define TOKENKIND_BINARY_OPERATORS\
    TOKENKIND_PLUS,\
    TOKENKIND_MINUS,\
    TOKENKIND_STAR,\
    TOKENKIND_FORWARDSLASH,\
    TOKENKIND_MODULO,\
    TOKENKIND_DOUBLEEQUAL,\
    TOKENKIND_NOTEQUAL,\
    TOKENKIND_LESS,\
    TOKENKIND_LESSEQUAL,\
    TOKENKIND_GREATER,\
    TOKENKIND_GREATEREQUAL,\
    TOKENKIND_AND,\
    TOKENKIND_OR

#define TOKENKIND_POSTFIX_OPERATORS\
    TOKENKIND_LEFTBRACKET,\
    TOKENKIND_LEFTPAREN,

#define TOKENKIND_TYPE_STARTERS\
    TOKENKIND_IDENTIFIER,

typedef enum TokenKind
{
    TOKENKIND_MODULO,
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
    TOKENKIND_AND,
    TOKENKIND_OR,

    TOKENKIND_LET,
    TOKENKIND_RETURN,
    TOKENKIND_FUNC,
    TOKENKIND_PROC,
    TOKENKIND_EXTERN,
    TOKENKIND_IF,
    TOKENKIND_ELSE,
    TOKENKIND_WHILE,
    TOKENKIND_FOR,
    TOKENKIND_IN,
    TOKENKIND_TYPE,
    TOKENKIND_STRUCT,
    TOKENKIND_UNION,
    TOKENKIND_ROUTINE,

    TOKENKIND_INTEGER,
    TOKENKIND_FLOAT,
    TOKENKIND_IDENTIFIER,
    TOKENKIND_STRING,
    TOKENKIND_CHARACTER,
    TOKENKIND_BOOLEAN,

    TOKENKIND_SEMICOLON,
    TOKENKIND_COLON,
    TOKENKIND_DOUBLECOLON,
    TOKENKIND_PERIOD,
    TOKENKIND_DOUBLEPERIOD,
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
    TOKENKIND_AMPERSAND,

    TOKENKIND_EOF,
} TokenKind;

typedef struct Token
{
    TokenKind kind;
    int line;
    int column;
    char* as_string;

    union
    {
        uint64_t integer;
        double floating;
        char character;
        char* string;
        char* identifier;
        bool boolean;
    };
} Token;

typedef enum TokenizerState
{
    TOKENIZERSTATE_START     = 0x00,
    TOKENIZERSTATE_INTEGER   = 0x10,
    TOKENIZERSTATE_SPECIAL   = 0x20,
    TOKENIZERSTATE_WORD      = 0x30,
    TOKENIZERSTATE_STRING    = 0x40,
    TOKENIZERSTATE_CHARACTER = 0x50,
    TOKENIZERSTATE_FLOAT     = 0x60,
} TokenizerState;

typedef struct Tokenizer
{
    int current_character_index;
    char character;
    char next_character;
    char symbol[512];
    int symbol_last_index;
    TokenizerState state;
    int line;
    int column;
    bool error_found;
    bool in_string;
    bool in_character;
} Tokenizer;

Token* tokenize();

// helper functions
bool _token_kind_is_in_group( TokenKind kind, TokenKind* group, size_t count );

#endif
