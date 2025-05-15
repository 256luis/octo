#ifndef ERROR_H
#define ERROR_H

typedef enum ErrorKind
{
    // tokenizer errors
    ERRORKIND_INVALIDSYMBOL,
    ERRORKIND_MISMATCHEDPARENS,
    ERRORKIND_UNCLOSEDPARENS,
    ERRORKIND_MULTICHARACTERCHARACTER,

    // parser error
    ERRORKIND_UNEXPECTEDSYMBOL,
    // ERRORKIND_UNEXPECTEDEOF,

    // semantic errors
    ERRORKIND_SYMBOLREDECLARATION,
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
    // SourceCode source_code;
    int line;
    int column;

    union
    {
        struct
        {
            char left_paren_mismatched;
            char right_paren_mismatched;
        };

        char unclosed_paren;
    };
} Error;

SourceCode source_code_load( char* path );
void source_code_print_line( SourceCode source_code, int line );

void report_error( Error error );

#endif
