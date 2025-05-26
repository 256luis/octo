#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "parser.h"

FILE* generate_code( FILE* file, Expression* program );

#endif
