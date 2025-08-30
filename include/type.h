#ifndef TYPE_H
#define TYPE_H

#include <stdint.h>

typedef struct Type Type;

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
    int length;
} TypeArray;

typedef struct TypePointer
{
    Type* base;
} TypePointer;

typedef struct TypeType
{
    Type* definition;
} TypeType;

typedef struct Type
{
    TypeKind kind;
    union
    {
        TypeArray array;
        TypePointer pointer;
        TypeType type;
    };
} Type;

bool type_is_integer( Type type );
bool type_is_float( Type type );
bool type_is_numeric( Type type );
bool type_equals( Type t1, Type t2 );
void type_print( Type type );
Type type_wrap( Type type );
Type type_unwrap( Type type );

extern const Type TYPE_UNSPECIFIED;
extern const Type TYPE_NONE;
extern const Type TYPE_STRING;
extern const Type TYPE_CHARACTER;
extern const Type TYPE_BOOLEAN;
extern const Type TYPE_INT;
extern const Type TYPE_UINT;
extern const Type TYPE_FLOAT;

#endif
