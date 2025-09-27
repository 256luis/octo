#ifndef ERROR_H
#define ERROR_H

#include "operation.h"
#include "tokenizer.h"
#include "type.h"

typedef struct SourceCode
{
    char* code;
    char* path;
    int length;

    // array of indexes to the first character after a newline
    int* line_indexes;
} SourceCode;

typedef enum ErrorKind
{
    // tokenizer errors
    ERRORKIND_INVALIDSYMBOL,
    ERRORKIND_MISMATCHEDPARENS,
    ERRORKIND_UNCLOSEDPARENS,
    ERRORKIND_MULTICHARACTERCHARACTER,

    // parser errors
    ERRORKIND_INCORRECTSYNTAX,
    ERRORKIND_UNEXPECTEDSYMBOL,

    // semantic errors
    ERRORKIND_TYPEMISMATCH,
    ERRORKIND_UNDECLAREDSYMBOL,
    ERRORKIND_SYMBOLREDECLARATION,
    ERRORKIND_EXPECTEDNUMERIC,
    ERRORKIND_ILLEGALNONETYPE,
    ERRORKIND_CANNOTINFERTYPE,
    ERRORKIND_ILLEGALTYPETYPE,
    ERRORKIND_NOTAROUTINE,
    ERRORKIND_MISSINGTYPE,
    ERRORKIND_PROCEDUREWITHRETURN,
    ERRORKIND_INCORRECTARGCOUNT,
    ERRORKIND_WHILEWITHELSE,
    ERRORKIND_UNMATCHINGIFTYPES,
    ERRORKIND_NOTANAGGREGATETYPE,
    ERRORKIND_NOTAPOINTER,
    ERRORKIND_ILLEGALBINARYOPERATION,
    ERRORKIND_ATTEMPTTOMUTATEIMMUTABLE,
    ERRORKIND_ILLEGALTOPLEVEL,
    ERRORKIND_NOTANARRAY,
} ErrorKind;

typedef struct Error
{
    ErrorKind kind;
    Token offending_token;
    char* note;

    union
    {
        struct
        {
            Type expected;
            Type found;
        } type_mismatch;

        struct
        {
            int expected;
            int found;
        } incorrect_arg_count;

        struct
        {
            Type left_type;
            Type right_type;
        } illegal_binary_operation;
    };
} Error;

SourceCode source_code_load( char* path );
void source_code_print_line( SourceCode source_code, int line );

void report_error( Error error );

#endif
