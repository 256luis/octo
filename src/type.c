#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "type.h"
#include "debug.h"
#include "globals.h"
#include "symbol.h"
#include "lvec.h"

const Type TYPE_UNSPECIFIED = { .kind = TYPEKIND_UNSPECIFIED };
const Type TYPE_NONE        = { .kind = TYPEKIND_NONE };
const Type TYPE_STRING      = { .kind = TYPEKIND_PRIMITIVE_STRING };
const Type TYPE_CHARACTER   = { .kind = TYPEKIND_PRIMITIVE_CHARACTER };
const Type TYPE_BOOLEAN     = { .kind = TYPEKIND_PRIMITIVE_BOOLEAN };
const Type TYPE_INT         = { .kind = TYPEKIND_PRIMITIVE_INT };
const Type TYPE_FLOAT       = { .kind = TYPEKIND_PRIMITIVE_FLOAT };

bool type_is_integer( Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_PRIMITIVE_INT:
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

    if( t1.identifier_token != NULL && t2.identifier_token != NULL )
    {
        return strcmp( t1.identifier_token->as_string, t2.identifier_token->as_string ) == 0;
    }
    else if( t1.identifier_token == NULL && t2.identifier_token == NULL )
    {
        // do nothing
    }
    else
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
        case TYPEKIND_PRIMITIVE_FLOAT:
        {
            return true;
        }

        case TYPEKIND_ARRAY:
        {
            if( t1.array.length != t2.array.length )
            {
                return false;
            }

            if( !type_equals( *t1.array.base, *t2.array.base ) )
            {
                return false;
            }

            return true;
        }

        case TYPEKIND_POINTER:
        {
            bool bases_are_equal = type_equals( *t1.pointer.base, *t2.pointer.base );
            bool mutabilities_are_equal = t1.pointer.is_mutable == t2.pointer.is_mutable;
            return bases_are_equal && mutabilities_are_equal;
        }

        case TYPEKIND_STRUCT:
        {
            Symbol* t1_members = t1.structure.members->symbols;
            Symbol* t2_members = t2.structure.members->symbols;

            size_t t1_member_count = lvec_get_length( t1_members );
            size_t t2_member_count = lvec_get_length( t2_members );
            if( t1_member_count != t2_member_count )
            {
                return false;
            }

            for( size_t i = 0; i < t1_member_count; i++ )
            {
                Type t1_member_type = t1_members[ i ].type;
                Type t2_member_type = t2_members[ i ].type;
                if( !type_equals( t1_member_type, t2_member_type ) )
                {
                    return false;
                }
            }

            return true;
        }

        case TYPEKIND_ENUM:
        {
            return false;
        }

        default:
        {
            UNIMPLEMENTED();
        }
    }
}

void type_print( Type type )
{
    if( type.identifier_token != NULL )
    {
        printf("%s", type.identifier_token->as_string);
        return;
    }

    switch( type.kind )
    {
        case TYPEKIND_UNSPECIFIED:         printf("UNSPECIFIED"); break;
        case TYPEKIND_NONE:                printf("NONE"); break;
        case TYPEKIND_PRIMITIVE_STRING:    printf("string"); break;
        case TYPEKIND_PRIMITIVE_CHARACTER: printf("char"); break;
        case TYPEKIND_PRIMITIVE_BOOLEAN:   printf("bool"); break;
        case TYPEKIND_PRIMITIVE_INT:       printf("int"); break;
        case TYPEKIND_PRIMITIVE_FLOAT:     printf("float"); break;
        case TYPEKIND_TYPE:                printf("type"); break;

        case TYPEKIND_POINTER:
        {
            printf( "&%s", type.pointer.is_mutable ? "mut ": "" );
            type_print( *type.pointer.base );
            break;
        }

        case TYPEKIND_ARRAY:
        {
            printf( "[%ld]", type.array.length );
            type_print( *type.array.base );
            break;
        }

        case TYPEKIND_STRUCT:
        {
            Symbol* members = type.structure.members->symbols;
            size_t member_count = lvec_get_length( members );

            printf( "struct { " );
            for( size_t i = 0; i < member_count; i++ )
            {
                printf( "%s: ", members[ i ].key.as_string );
                type_print( members[ i ].type );
                printf( ", " );
            }
            printf( "}" );

            break;
        }

        case TYPEKIND_ROUTINE:
        {
            printf( "%s(", type.routine.is_func ? "func" : "proc" );

            size_t param_length = lvec_get_length( type.routine.param_types );
            for( size_t i = 0; i < param_length; i++ )
            {
                Type param_type = type.routine.param_types[ i ];
                type_print( param_type );
                printf( ", " );
            }

            printf( ")" );
            if( type.routine.is_func )
            {
                printf( " -> " );
                type_print( *type.routine.return_type );
            }

            break;
        }

        case TYPEKIND_ENUM:
        {
            Symbol* variants = type.enumuration.variants->symbols;
            size_t variant_count = lvec_get_length( variants );

            printf( "enum { " );
            for( size_t i = 0; i < variant_count; i++ )
            {
                printf( "%s, ", variants[ i ].key.as_string );
            }
            printf( "}" );

            break;
        }
    }
}

Type type_wrap_type( Type type )
{
    Type* base = octo_malloc( sizeof( Type ) );
    *base = type;
    return ( Type ){
        .kind = TYPEKIND_TYPE,
        .type.definition = base
    };
}

Type type_wrap_array( Type type )
{
    Type* base = octo_malloc( sizeof( Type ) );
    *base = type;
    return ( Type ){
        .kind = TYPEKIND_ARRAY,
        .array.base = base
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

Type type_unwrap_pointer( Type type )
{
    assert( type.kind == TYPEKIND_POINTER );
    return *type.pointer.base;
}

void type_propagate_mutability( Type* type, bool is_mutable )
{
    type->is_mutable = is_mutable;
    switch( type->kind )
    {
        case TYPEKIND_ARRAY:
        {
            type_propagate_mutability( type->array.base, is_mutable );
            break;
        }

        case TYPEKIND_POINTER:
        {
            // type_propagate_mutability( type->pointer.base, is_mutable );
            // do nothing
            break;
        }

        case TYPEKIND_STRUCT:
        {
            size_t member_count = type->structure.member_count;
            for( size_t i = 0; i < member_count; i++ )
            {
                Symbol* member_symbol = &type->structure.members->symbols[ i ];
                type_propagate_mutability( &member_symbol->type, is_mutable );
            }
            break;
        }

        default:
        {
            // do nothing
        }
    }
}
