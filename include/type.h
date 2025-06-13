#ifndef TYPE_H
#define TYPE_H

// forward declaration to avoid circular include
typedef struct SymbolTable SymbolTable;

typedef enum TypeKind
{
    // built-in named types
    TYPEKIND_VOID,
    TYPEKIND_INTEGER,
    TYPEKIND_FLOAT,
    TYPEKIND_CHARACTER,
    TYPEKIND_BOOLEAN,

    // only for integers and floats
    TYPEKIND_NUMERICLITERAL,

    // anonymous types
    TYPEKIND_COMPOUND,
    TYPEKIND_FUNCTION,

    // referential types
    TYPEKIND_POINTER,
    TYPEKIND_REFERENCE,
    TYPEKIND_ARRAY,
    TYPEKIND_TYPE,
    TYPEKIND_NAMED,

    TYPEKIND_TOINFER,
    // TYPEKIND_INVALID,
} TypeKind;

typedef struct Type
{
    TypeKind kind;
    // Token token;

    union
    {
        struct
        {
            TypeKind kind;
        } literal;

        struct
        {
            struct Type* info;
        } type;

        struct
        {
            char* as_string;
            struct Type* definition;

            // arrays
            struct Type* pointer_types;
            struct Type* array_types;
        } named;

        struct
        {
            // char* identifier;
            SymbolTable* member_symbol_table;
            bool is_struct;
        } compound;

        struct
        {
            struct Type* param_types; // array
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
