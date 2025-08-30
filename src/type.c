#include "type.h"

bool type_is_integer( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_PRIMITIVE_I8:
        case TYPEKIND_PRIMITIVE_I16:
        case TYPEKIND_PRIMITIVE_I32:
        case TYPEKIND_PRIMITIVE_I64:
        case TYPEKIND_PRIMITIVE_U8:
        case TYPEKIND_PRIMITIVE_U16:
        case TYPEKIND_PRIMITIVE_U32:
        case TYPEKIND_PRIMITIVE_U64:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

bool type_is_float( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_PRIMITIVE_F32:
        case TYPEKIND_PRIMITIVE_F64:
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
        case TYPEKIND_PRIMITIVE_I8:
        case TYPEKIND_PRIMITIVE_I16:
        case TYPEKIND_PRIMITIVE_I32:
        case TYPEKIND_PRIMITIVE_I64:
        case TYPEKIND_PRIMITIVE_U8:
        case TYPEKIND_PRIMITIVE_U16:
        case TYPEKIND_PRIMITIVE_U32:
        case TYPEKIND_PRIMITIVE_U64:
        case TYPEKIND_PRIMITIVE_F32:
        case TYPEKIND_PRIMITIVE_F64:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}
