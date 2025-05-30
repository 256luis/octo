#ifndef TYPE_H
#define TYPE_H

#include "tokenizer.h"

// forward declaration to avoid circular include
typedef struct SymbolTable SymbolTable;

typedef enum TypeKind
{
    TYPEKIND_VOID,
    TYPEKIND_INTEGER,
    TYPEKIND_FLOAT,
    TYPEKIND_CHARACTER,
    TYPEKIND_BOOLEAN,
    TYPEKIND_FUNCTION,
    TYPEKIND_POINTER,
    TYPEKIND_REFERENCE,
    TYPEKIND_ARRAY,
    TYPEKIND_CUSTOM,
    TYPEKIND_PRIMITIVE,

    TYPEKIND_TOINFER,
    TYPEKIND_INVALID,
} TypeKind;

typedef struct Type
{
    TypeKind kind;
    Token token;

    union
    {
        // char* custom_identifier;
        struct
        {
            char* identifier;
            SymbolTable* member_symbols;
        } custom;

        struct
        {
            struct Type* param_types;
            struct Type* return_type;
            int param_count;
            bool is_variadic;
        } function;

        struct
        {
            struct Type* base_type;
        } pointer;

        struct
        {
            struct Type* base_type;
        } reference;

        struct
        {
            size_t bit_count; // CAN ONLY BE 8, 16, 32, 64
            bool is_signed;
        } integer;

        struct
        {
            size_t bit_count; // CAN ONLY BE 32, 64
        } floating;

        struct
        {
            struct Type* base_type;
            int length; // if this is -1, length is to be inferred
        } array;
    };
} Type;


#endif
