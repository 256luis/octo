#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "array_list.h"

ArrayList array_list_new( size_t element_size )
{
    ArrayList list = {
        .data = malloc( element_size * ARRAY_LIST_INITIAL_CAPACITY ),
        .length = 0,
        .capacity = ARRAY_LIST_INITIAL_CAPACITY,
        .element_size = element_size,
    };

    if( list.data == NULL ) ALLOC_ERROR();

    return list;
}

void array_list_free( ArrayList list )
{
    free( list.data );
}

void array_list_append( ArrayList* list, void* data )
{
    // if we dont have enough allocated memory for the new element
    if( ( list->length + 1 ) > list->capacity )
    {
        size_t new_size = list->element_size * list->capacity * ARRAY_LIST_GROWTH_FACTOR;
        list->data = realloc( list->data, new_size );
        list->capacity *= ARRAY_LIST_GROWTH_FACTOR;

        if( list->data == NULL ) ALLOC_ERROR();
    }

    list->length++;
    memcpy( &list->data[ ( list->length - 1 ) * list->element_size ], data, list->element_size );
}

void* array_list_get( ArrayList list, int index )
{
    return &list.data[ index * list.element_size ];
}
