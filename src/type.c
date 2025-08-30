#include <stdio.h>
#include "type.h"
#include "debug.h"

bool type_is_numeric( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_PRIMITIVE_INT:
        case TYPEKIND_PRIMITIVE_UINT:
        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

bool type_equals( Type t1, Type t2 )
{
    return t1.kind == t2.kind;
}

void type_print( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_UNSPECIFIED:         printf("UNSPECIFIED"); break;
        case TYPEKIND_NONE:                printf("NONE"); break;
        case TYPEKIND_PRIMITIVE_STRING:    printf("string"); break;
        case TYPEKIND_PRIMITIVE_CHARACTER: printf("char"); break;
        case TYPEKIND_PRIMITIVE_BOOLEAN:   printf("bool"); break;
        case TYPEKIND_PRIMITIVE_INT:       printf("int"); break;
        case TYPEKIND_PRIMITIVE_UINT:      printf("uint"); break;
        case TYPEKIND_PRIMITIVE_FLOAT:     printf("float"); break;

        case TYPEKIND_STRUCT:
        case TYPEKIND_ENUM:
            UNIMPLEMENTED();
    }
}
