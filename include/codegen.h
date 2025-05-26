#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "parser.h"
#include "semantic.h"

void generate_code( FILE* file, SemanticContext* context, Expression* program );

#endif
