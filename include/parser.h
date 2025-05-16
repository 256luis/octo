#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"

typedef enum TypeKind
{
    TYPEKIND_VOID,
    TYPEKIND_INTEGER,
    TYPEKIND_FLOAT, // not yet parsed
    TYPEKIND_CHARACTER,
    TYPEKIND_BOOLEAN,
    TYPEKIND_STRING,
    TYPEKIND_CUSTOM,
    TYPEKIND_TOINFER,
} TypeKind;

typedef struct Type
{
    TypeKind kind;

    union
    {
        char* custom_identifier;
    };
} Type;

typedef enum BinaryOperation
{
    // arithmetic
#define BINARYOPERATION_ARITHMETIC_START BINARYOPERATION_ADD
    BINARYOPERATION_ADD,
    BINARYOPERATION_SUBTRACT,
    BINARYOPERATION_MULTIPLY,
    BINARYOPERATION_DIVIDE,
#define BINARYOPERATION_ARITHMETIC_END BINARYOPERATION_EQUAL

    // boolean
#define BINARYOPERATION_BOOLEAN_START BINARYOPERATION_EQUAL
    BINARYOPERATION_EQUAL,
    BINARYOPERATION_GREATER,
    BINARYOPERATION_LESS,
    BINARYOPERATION_NOTEQUAL,
    BINARYOPERATION_GREATEREQUAL,
    BINARYOPERATION_LESSEQUAL,
#define BINARYOPERATION_BOOLEAN_END ( BINARYOPERATION_LESSEQUAL + 1 )
} BinaryOperation;

typedef enum UnaryOperation
{
    UNARYOPERATION_NEGATIVE,
    UNARYOPERATION_NOT
} UnaryOperation;

typedef enum ExpressionKind
{
    // rvalues
    EXPRESSIONKIND_INTEGER,
    // EXPRESSIONKIND_FLOAT, // unimplemented
    EXPRESSIONKIND_IDENTIFIER,
    EXPRESSIONKIND_STRING,
    EXPRESSIONKIND_CHARACTER,
    EXPRESSIONKIND_BINARY,
    EXPRESSIONKIND_UNARY,
    EXPRESSIONKIND_FUNCTIONCALL,

    // not rvalues
    EXPRESSIONKIND_VARIABLEDECLARATION,
    EXPRESSIONKIND_FUNCTIONDECLARATION,
    EXPRESSIONKIND_COMPOUND,
    EXPRESSIONKIND_RETURN,
    EXPRESSIONKIND_ASSIGNMENT,
} ExpressionKind;

typedef struct Expression
{
    ExpressionKind kind;
    Token* associated_tokens;
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
            Type type;
            struct Expression* value;
        } variable_declaration;

        struct
        {
            struct Expression** expressions;
            int statement_count;
        } compound;

        struct
        {
            char* identifier;
            Type return_type;

            // arrays to hold params info
            char** param_identifiers;
            Type* param_types;
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

typedef struct Parser
{
    // Token* tokens;
    int current_token_index;
    Token current_token;
    Token next_token;
} Parser;

Expression* parse( Token* tokens );

void expression_print( Expression* expression );

#endif
