#ifndef TYPE_H
#define TYPE_H

#define TYPE_STRING    ( Type ){ .kind = TYPEKIND_PRIMITIVE_STRING }
#define TYPE_CHARACTER ( Type ){ .kind = TYPEKIND_PRIMITIVE_CHARACTER }
#define TYPE_BOOLEAN   ( Type ){ .kind = TYPEKIND_PRIMITIVE_BOOLEAN }
#define TYPE_I8        ( Type ){ .kind = TYPEKIND_PRIMITIVE_I8 }
#define TYPE_I16       ( Type ){ .kind = TYPEKIND_PRIMITIVE_I16 }
#define TYPE_I32       ( Type ){ .kind = TYPEKIND_PRIMITIVE_I32 }
#define TYPE_I64       ( Type ){ .kind = TYPEKIND_PRIMITIVE_I64 }
#define TYPE_U8        ( Type ){ .kind = TYPEKIND_PRIMITIVE_U8 }
#define TYPE_U16       ( Type ){ .kind = TYPEKIND_PRIMITIVE_U16 }
#define TYPE_U32       ( Type ){ .kind = TYPEKIND_PRIMITIVE_U32 }
#define TYPE_U64       ( Type ){ .kind = TYPEKIND_PRIMITIVE_U64 }
#define TYPE_F32       ( Type ){ .kind = TYPEKIND_PRIMITIVE_F32 }
#define TYPE_F64       ( Type ){ .kind = TYPEKIND_PRIMITIVE_F64 }

typedef enum TypeKind
{
    TYPEKIND_PRIMITIVE_STRING,
    TYPEKIND_PRIMITIVE_CHARACTER,
    TYPEKIND_PRIMITIVE_BOOLEAN,
    TYPEKIND_PRIMITIVE_I8,
    TYPEKIND_PRIMITIVE_I16,
    TYPEKIND_PRIMITIVE_I32,
    TYPEKIND_PRIMITIVE_I64,
    TYPEKIND_PRIMITIVE_U8,
    TYPEKIND_PRIMITIVE_U16,
    TYPEKIND_PRIMITIVE_U32,
    TYPEKIND_PRIMITIVE_U64,
    TYPEKIND_PRIMITIVE_F32,
    TYPEKIND_PRIMITIVE_F64,

    TYPEKIND_STRUCT,
    TYPEKIND_ENUM,
} TypeKind;

typedef struct Type
{
    TypeKind kind;
} Type;

bool type_is_integer( Type type );
bool type_is_float( Type type );
bool type_is_numeric( Type type );

#endif
