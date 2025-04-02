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
    SourceCode source_code;
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

/* typedef struct ErrorNode */
/* { */
/*     Error error; */
/*     struct ErrorNode* next; */
/* } ErrorNode; */

/* typedef struct ErrorList */
/* { */
/*     ErrorNode* head; */
/*     ErrorNode* tail; */
/*     int length; */
/* } ErrorList; */

/* ErrorList error_list_new(); */
/* void error_list_free( ErrorList* list ); */
/* void error_list_append( ErrorList* list, Error value ); */

void report_error( Error error );

#endif
