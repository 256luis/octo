#ifndef ERROR_H
#define ERROR_H

#include "parser.h"
#include "tokenizer.h"

typedef enum ErrorKind
{
    // tokenizer errors
    ERRORKIND_INVALIDSYMBOL,
    ERRORKIND_MISMATCHEDPARENS,
    ERRORKIND_UNCLOSEDPARENS,
    ERRORKIND_MULTICHARACTERCHARACTER,

    // parser error
    ERRORKIND_UNEXPECTEDSYMBOL,
    ERRORKIND_INVALIDLVALUE,

    // semantic errors
    ERRORKIND_SYMBOLREDECLARATION,
    ERRORKIND_INVALIDBINARYOPERATION,
    ERRORKIND_INVALIDUNARYOPERATION,
    ERRORKIND_TYPEMISMATCH,
    ERRORKIND_UNDECLAREDSYMBOL,
    ERRORKIND_INVALIDARGUMENTCOUNT,
    ERRORKIND_INVALIDADDRESSOF,
    ERRORKIND_INVALIDIMPLICITCAST,
    ERRORKIND_MISSINGFUNCTIONBODY,
    ERRORKIND_EXTERNWITHBODY,
    ERRORKIND_WHILEWITHELSE,
    ERRORKIND_VOIDVARIABLE,
    ERRORKIND_ZEROLENGTHARRAY,
    ERRORKING_ARRAYLENGTHMISMATCH,
    ERRORKIND_CANNOTINFERARRAYLENGTH,
    ERRORKIND_INVALIDARRAYSUBSCRIPT,
    ERRORKIND_NOTANITERATOR,
    ERRORKIND_NOTANARRAY,
    ERRORKIND_MISSINGMEMBER,
    ERRORKIND_INVALIDCOMPOUNDLITERAL,
    ERRORKIND_CANNOTUSETYPEASVALUE,
    ERRORKIND_NOTATYPE,
    ERRORKIND_NOTCOMPOUND,
} ErrorKind;

typedef struct SourceCode
{
    char* code;
    char* path;
    int length;

    // array of indexes to the first character after a newline
    int* line_indexes;
} SourceCode;

typedef struct Error
{
    ErrorKind kind;
    Token offending_token;

    union
    {
        struct
        {
            Token original_declaration_token;
        } symbol_redeclaration;

        struct
        {
            Type left_type;
            Type right_type;
        } invalid_binary_operation;

        struct
        {
            Type operand_type;
        } invalid_unary_operation;

        struct
        {
            Type expected;
            Type found;
        } type_mismatch;

        struct
        {
            int expected;
            int found;
        } too_many_arguments;

        struct
        {
            Type to;
            Type from;
        } invalid_implicit_cast;

        struct
        {
            int expected;
            int found;
        } array_length_mismatch;

        struct
        {
            Type parent_type;
        } missing_member;
    };
} Error;

SourceCode source_code_load( char* path );
void source_code_print_line( SourceCode source_code, int line );

void report_error( Error error );

#endif
