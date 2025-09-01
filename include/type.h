#ifndef TYPE_H
#define TYPE_H

#include <stdint.h>
#include "tokenizer.h"

typedef struct SymbolTable SymbolTable;

typedef struct Type Type;
typedef struct AstNode AstNode;

typedef enum TypeKind
{
    TYPEKIND_UNSPECIFIED,
    TYPEKIND_NONE,
    TYPEKIND_PRIMITIVE_STRING,
    TYPEKIND_PRIMITIVE_CHARACTER,
    TYPEKIND_PRIMITIVE_BOOLEAN,
    TYPEKIND_PRIMITIVE_INT,
    TYPEKIND_PRIMITIVE_UINT,
    TYPEKIND_PRIMITIVE_FLOAT,
    TYPEKIND_ARRAY,
    TYPEKIND_POINTER,
    TYPEKIND_TYPE,
    TYPEKIND_STRUCT,
    TYPEKIND_ENUM,
} TypeKind;

typedef struct TypeArray
{
    Type* base;
    AstNode* length;
} TypeArray;

typedef struct TypePointer
{
    Type* base;
} TypePointer;

typedef struct TypeType
{
    Type* definition;
} TypeType;

typedef struct TypeStruct
{
    SymbolTable* members;
    size_t member_count;
} TypeStruct;

typedef struct Type
{
    TypeKind kind;
    Token* identifier_token; // for user defined types, can be null

    union
    {
        TypeArray array;
        TypePointer pointer;
        TypeType type;
        TypeStruct structure;
    };
} Type;

bool type_is_integer( Type type );
bool type_is_float( Type type );
bool type_is_numeric( Type type );
bool type_equals( Type t1, Type t2 );
void type_print( Type type );
Type type_wrap_type( Type type );
Type type_wrap_array( Type type );
Type type_unwrap_type( Type type );
Type type_unwrap_array( Type type );

extern const Type TYPE_UNSPECIFIED;
extern const Type TYPE_NONE;
extern const Type TYPE_STRING;
extern const Type TYPE_CHARACTER;
extern const Type TYPE_BOOLEAN;
extern const Type TYPE_INT;
extern const Type TYPE_UINT;
extern const Type TYPE_FLOAT;

#endif
