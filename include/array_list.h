#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include <assert.h>
#include <stdint.h>

#define ARRAY_LIST_INITIAL_CAPACITY 10
#define ARRAY_LIST_GROWTH_FACTOR 1.5

typedef struct ArrayList
{
    uint8_t* data;
    size_t length;
    size_t capacity;
    size_t element_size;
} ArrayList;

ArrayList array_list_new( size_t element_size );
void array_list_append( ArrayList* list, void* data );
void array_list_free( ArrayList list );
void* array_list_get( ArrayList list, int index );

#endif
