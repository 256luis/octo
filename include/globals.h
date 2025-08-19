#ifndef GLOBALS_H
#define GLOBALS_H

#include "error.h"

extern SourceCode g_source_code;

void* octo_malloc( size_t size );
void* octo_calloc( size_t size );

#endif
