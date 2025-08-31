#include <assert.h>
#include <stdio.h>
#include "type.h"
#include "debug.h"
#include "globals.h"

const Type TYPE_UNSPECIFIED = { .kind = TYPEKIND_UNSPECIFIED };
const Type TYPE_NONE        = { .kind = TYPEKIND_NONE };
const Type TYPE_STRING      = { .kind = TYPEKIND_PRIMITIVE_STRING };
const Type TYPE_CHARACTER   = { .kind = TYPEKIND_PRIMITIVE_CHARACTER };
const Type TYPE_BOOLEAN     = { .kind = TYPEKIND_PRIMITIVE_BOOLEAN };
const Type TYPE_INT         = { .kind = TYPEKIND_PRIMITIVE_INT };
const Type TYPE_UINT        = { .kind = TYPEKIND_PRIMITIVE_UINT };
const Type TYPE_FLOAT       = { .kind = TYPEKIND_PRIMITIVE_FLOAT };

bool type_is_integer( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_PRIMITIVE_INT:
        case TYPEKIND_PRIMITIVE_UINT:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

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
    if( t1.kind != t2.kind )
    {
        return false;
    }

    switch( t1.kind )
    {

        case TYPEKIND_UNSPECIFIED:
        case TYPEKIND_NONE:
        case TYPEKIND_PRIMITIVE_STRING:
        case TYPEKIND_PRIMITIVE_CHARACTER:
        case TYPEKIND_PRIMITIVE_BOOLEAN:
        case TYPEKIND_PRIMITIVE_INT:
        case TYPEKIND_PRIMITIVE_UINT:
        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            return true;
        }

        case TYPEKIND_ARRAY:
        {
            return type_equals( *t1.array.base, *t2.array.base );
        }

        case TYPEKIND_POINTER:
        {
            return type_equals( *t1.array.base, *t2.array.base );
        }

        default:
        {
            UNIMPLEMENTED();
        }
    }
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
        case TYPEKIND_TYPE:                printf("type"); break;

        case TYPEKIND_POINTER:
        {
            printf( "&" );
            type_print( *type.pointer.base );
            break;
        }

        case TYPEKIND_ARRAY:
        {
            printf( "[]" );
            type_print( *type.array.base );
            break;
        }

        case TYPEKIND_STRUCT:
        case TYPEKIND_ENUM:
            UNIMPLEMENTED();
    }
}

Type type_wrap( Type type )
{
    Type* base = octo_malloc( sizeof( Type ) );
    *base = type;
    return ( Type ){
        .kind = TYPEKIND_TYPE,
        .type.definition = base
    };
}

Type type_unwrap_type( Type type )
{
    assert( type.kind == TYPEKIND_TYPE );
    return *type.type.definition;
}

Type type_unwrap_array( Type type )
{
    assert( type.kind == TYPEKIND_ARRAY );
    return *type.array.base;
}
