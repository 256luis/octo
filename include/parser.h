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
    TYPEKIND_FUNCTION,
    TYPEKIND_POINTER,
    TYPEKIND_ARRAY,
    TYPEKIND_CUSTOM,

    TYPEKIND_TOINFER,
    TYPEKIND_INVALID,
} TypeKind;

typedef struct Type
{
    TypeKind kind;

    union
    {
        char* custom_identifier;
        struct
        {
            struct Type* param_types;
            struct Type* return_type;
            int param_count;
        } function;

        struct
        {
            struct Type* type;
        } pointer;

        struct
        {
            size_t bit_count; // CAN ONLY BE 8, 16, 32, 64
            bool is_signed;
        } integer;

        struct
        {
            size_t bit_count; // CAN ONLY BE 32, 64
        } floating;

        struct
        {
            struct Type* type;
            int length; // if this is -1, length is to be inferred
        } array;
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
    EXPRESSIONKIND_FLOAT,
    EXPRESSIONKIND_STRING,
    EXPRESSIONKIND_CHARACTER,
    EXPRESSIONKIND_BINARY,
    EXPRESSIONKIND_FUNCTIONCALL,
    EXPRESSIONKIND_BOOLEAN,

    // can be lvalue or rvalue
    EXPRESSIONKIND_IDENTIFIER,
    EXPRESSIONKIND_UNARY, // only for dereference (will be checked in semantic analysis)

    // base statements
    EXPRESSIONKIND_VARIABLEDECLARATION,
    EXPRESSIONKIND_FUNCTIONDECLARATION,
    EXPRESSIONKIND_COMPOUND,
    EXPRESSIONKIND_RETURN,
    EXPRESSIONKIND_ASSIGNMENT,
    EXPRESSIONKIND_EXTERN,
    EXPRESSIONKIND_CONDITIONAL,
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
        double floating;
        char* identifier;
        char* string;
        char character;
        bool boolean;

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
            struct Expression* lvalue;
            struct Expression* rvalue;
        } assignment;

        struct
        {
            struct Expression* function;
        } extern_expression;

        struct
        {
            bool is_loop; // used to check if conditional is an if statement or a
                          // while loop
            struct Expression* condition;
            struct Expression* true_body;

            // will be null if there is no 'else' in if statements
            // will be null in while loops
            struct Expression* false_body;
        } conditional;
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
