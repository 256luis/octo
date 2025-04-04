#ifndef PARSER_H
#define PARSER_H

#include "error.h"
#include "tokenizer.h"

typedef enum BinaryOperation
{
    // arithmetic
    BINARYOPERATION_ADD,
    BINARYOPERATION_SUBTRACT,
    BINARYOPERATION_MULTIPLY,
    BINARYOPERATION_DIVIDE,

    // boolean
    BINARYOPERATION_GREATER,
    BINARYOPERATION_LESS,
    BINARYOPERATION_EQUAL,
    BINARYOPERATION_NOTEQUAL,
    BINARYOPERATION_GREATEREQUAL,
    BINARYOPERATION_LESSEQUAL,
} BinaryOperation;

typedef enum UnaryOperation
{
    UNARYOPERATION_NEGATIVE,
    UNARYOPERATION_NOT
} UnaryOperation;

typedef enum ExpressionKind
{
    EXPRESSIONKIND_NUMBER,
    EXPRESSIONKIND_IDENTIFIER,
    EXPRESSIONKIND_STRING,
    EXPRESSIONKIND_CHARACTER,
    EXPRESSIONKIND_BINARY,
    EXPRESSIONKIND_UNARY,
    EXPRESSIONKIND_FUNCTIONCALL,
    EXPRESSIONKIND_VARIABLEDECLARATION,
    EXPRESSIONKIND_FUNCTIONDECLARATION,
    EXPRESSIONKIND_COMPOUND,
    EXPRESSIONKIND_RETURN,
    EXPRESSIONKIND_ASSIGNMENT,
} ExpressionKind;

typedef struct ExpressionList
{
    ArrayList list;
} ExpressionList;

typedef struct Expression
{
    ExpressionKind kind;
    union
    {
        // base cases
        int number;
        char* identifier;
        char* string;
        char character;

        // recursive expressions
        struct
        {
            BinaryOperation operation;
            struct Expression* left;
            struct Expression* right;
        } binary;

        struct
        {
            UnaryOperation operation;
            struct Expression* operand;
        } unary;

        struct
        {
            char* identifier;
            struct Expression** args; // array of expression pointers
            size_t arg_count;
        } function_call;

                struct
        {
            char* identifier;
            char* type;
            struct Expression* value;
        } variable_declaration;

        struct
        {
            ExpressionList expressions;
            int statement_count;
        } compound;

        struct
        {
            char* identifier;
            char* return_type;

            // arrays to hold params info
            char** param_identifiers;
            char** param_types;
            int param_count;

            struct Expression* body;
        } function_declaration;

        struct
        {
            struct Expression* value;
        } return_expression;

        struct
        {
            char* identifier;
            struct Expression* value;
        } assignment;
    };
} Expression;

/* typedef enum StatementKind */
/* { */
/*     STATEMENTKIND_VARIABLEDECLARATION, */
/*     STATEMENTKIND_FUNCTIONDECLARATION, */
/*     STATEMENTKIND_FUNCTIONCALL, */
/*     STATEMENTKIND_COMPOUND, */
/*     STATEMENTKIND_RETURN, */
/* } StatementKind; */

/* typedef struct Statement */
/* { */
/*     StatementKind kind; */
/*     union */
/*     { */
/*         struct */
/*         { */
/*             char* identifier; */
/*             char* type; */
/*             Expression* value; */
/*         } variable_declaration; */

/*         struct */
/*         { */
/*             StatementList statements; */
/*             int statement_count; */
/*         } compound; */

/*         struct */
/*         { */
/*             char* identifier; */
/*             char* return_type; */

/*             // arrays to hold params info */
/*             char** param_identifiers; */
/*             char** param_types; */
/*             int param_count; */

/*             struct Statement* body; */
/*         } function_declaration; */

/*         struct */
/*         { */
/*             char* identifier; */
/*             struct Expression** args; // array of expression pointers */
/*             size_t arg_count; */
/*         } function_call; */

/*         struct */
/*         { */
/*             Expression* value; */
/*         } return_statement; */
/*     }; */
/* } Statement; */

typedef struct Parser
{
    SourceCode source_code;
    TokenList tokens;
    // TokenNode* current_token_node;
    int current_token_index;
    Token current_token;
    Token next_token;
} Parser;

Parser* parser_new( TokenList token_list, SourceCode source_code );
void parser_free( Parser* parser );
Expression* parser_parse( Parser* parser );

void expression_print( Expression* expression );
// void statement_print( Expression* statement );

ExpressionList expression_list_new();
void expression_list_free( ExpressionList expressions );
void expression_list_append( ExpressionList* expressions, Expression value );
Expression expression_list_get( ExpressionList expressions, int index );

#endif
