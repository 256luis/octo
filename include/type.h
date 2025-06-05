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
    TYPEKIND_COMPOUND,

    // referential types
    TYPEKIND_POINTER,
    TYPEKIND_REFERENCE,
    TYPEKIND_ARRAY,
    TYPEKIND_DEFINITION,

    TYPEKIND_TOINFER,
    TYPEKIND_INVALID,
} TypeKind;

typedef struct Type
{
    TypeKind kind;
    Token token;

    union
    {
        struct
        {
            struct Type* info;
            struct Type* pointer_types;
            struct Type* array_types;
        } definition;

        struct
        {
            char* identifier;
            SymbolTable* member_symbols;
        } compound;

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
