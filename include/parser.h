#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include <stdint.h>

typedef enum TypeKind
{
    TYPEKIND_VOID,
    TYPEKIND_INTEGER,
    TYPEKIND_FLOAT,
    TYPEKIND_CHARACTER,
    TYPEKIND_BOOLEAN,
    TYPEKIND_STRING,
    TYPEKIND_FUNCTION,
    TYPEKIND_POINTER,
    TYPEKIND_CUSTOM,
    TYPEKIND_TOINFER,
    TYPEKIND_INVALID,
} TypeKind;

typedef struct Type
{
    TypeKind kind;
    // Token associated_token;

    union
    {
        char* custom_identifier;
        struct
        {
            struct Type* param_types;
            struct Type* return_type;
        } function;

        struct
        {
            struct Type* type;
        } pointer;
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
    UNARYOPERATION_NOT,
    UNARYOPERATION_ADDRESSOF,
    UNARYOPERATION_DEREFERENCE,
} UnaryOperation;

typedef enum ExpressionKind
{
    // rvalues
    EXPRESSIONKIND_INTEGER,
    EXPRESSIONKIND_FLOAT, // unimplemented
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

    // for the base cases
    Token associated_token;

    // for everything else
    Token starting_token;

    union
    {
        // base cases
        uint64_t integer;
        double float_;
        char* identifier;
        char* string;
        char character;

        // recursive expressions
        struct
        {
            BinaryOperation operation;
            Token operator_token;

            struct Expression* left;
            struct Expression* right;
        } binary;

        struct
        {
            UnaryOperation operation;
            Token operator_token;

            struct Expression* operand;
        } unary;

        struct
        {
            char* identifier;
            Token identifier_token;

            struct Expression** args; // array of expression pointers
            size_t arg_count;
        } function_call;

        struct
        {
            char* identifier;
            Token identifier_token;

            Type type;
            Token type_token;

            struct Expression* rvalue;
        } variable_declaration;

        struct
        {
            struct Expression** expressions;
            int statement_count;
        } compound;

        struct
        {
            char* identifier;
            Token identifier_token;

            Type return_type;
            Token return_type_token;

            // arrays to hold params info
            char** param_identifiers;
            Token* param_identifiers_tokens;

            Type* param_types;
            Token* param_types_tokens;

            int param_count;

            struct Expression* body;
        } function_declaration;

        struct
        {
            struct Expression* rvalue;
        } return_expression;

        struct
        {
            char* identifier;
            Token identifier_token;

            struct Expression* rvalue;
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
