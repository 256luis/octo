#ifndef TYPE_H
#define TYPE_H

#define TYPE_UNSPECIFIED ( Type ){ .kind = TYPEKIND_UNSPECIFIED }
#define TYPE_NONE        ( Type ){ .kind = TYPEKIND_NONE }
#define TYPE_STRING      ( Type ){ .kind = TYPEKIND_PRIMITIVE_STRING }
#define TYPE_CHARACTER   ( Type ){ .kind = TYPEKIND_PRIMITIVE_CHARACTER }
#define TYPE_BOOLEAN     ( Type ){ .kind = TYPEKIND_PRIMITIVE_BOOLEAN }
#define TYPE_INT         ( Type ){ .kind = TYPEKIND_PRIMITIVE_INT }
#define TYPE_UINT        ( Type ){ .kind = TYPEKIND_PRIMITIVE_UINT }
#define TYPE_FLOAT       ( Type ){ .kind = TYPEKIND_PRIMITIVE_FLOAT }

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
bool type_equals( Type t1, Type t2 );
void type_print( Type type );

#endif
