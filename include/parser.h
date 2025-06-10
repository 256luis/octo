#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "tokenizer.h"
#include "type.h"
// #include "type.h"

typedef enum BinaryOperation
{
    // arithmetic
#define BINARYOPERATION_ARITHMETIC_START BINARYOPERATION_ADD
    BINARYOPERATION_ADD,
    BINARYOPERATION_SUBTRACT,
    BINARYOPERATION_MULTIPLY,
    BINARYOPERATION_DIVIDE,
    BINARYOPERATION_MODULO,
#define BINARYOPERATION_ARITHMETIC_END BINARYOPERATION_EQUAL

    // boolean
#define BINARYOPERATION_BOOLEAN_START BINARYOPERATION_EQUAL
    BINARYOPERATION_EQUAL,
    BINARYOPERATION_GREATER,
    BINARYOPERATION_LESS,
    BINARYOPERATION_NOTEQUAL,
    BINARYOPERATION_GREATEREQUAL,
    BINARYOPERATION_LESSEQUAL,
    BINARYOPERATION_AND,
    BINARYOPERATION_OR,
#define BINARYOPERATION_BOOLEAN_END ( BINARYOPERATION_OR + 1 )
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
    EXPRESSIONKIND_ARRAYLITERAL,
    EXPRESSIONKIND_COMPOUNDLITERAL,

    // can be lvalue or rvalue
    EXPRESSIONKIND_IDENTIFIER,
    EXPRESSIONKIND_UNARY, // only for dereference (will be checked in semantic analysis)
    EXPRESSIONKIND_ARRAYSUBSCRIPT,
    EXPRESSIONKIND_MEMBERACCESS,

    // base statements
    EXPRESSIONKIND_VARIABLEDECLARATION,
    EXPRESSIONKIND_FUNCTIONDECLARATION,
    EXPRESSIONKIND_COMPOUND,
    EXPRESSIONKIND_RETURN,
    EXPRESSIONKIND_ASSIGNMENT,
    EXPRESSIONKIND_EXTERN,
    EXPRESSIONKIND_CONDITIONAL, // if statements and while-loops
    EXPRESSIONKIND_FORLOOP,
    EXPRESSIONKIND_TYPEDECLARATION,

    // type rvalues
    EXPRESSIONKIND_TYPEIDENTIFIER,
    EXPRESSIONKIND_POINTERTYPE,
    EXPRESSIONKIND_ARRAYTYPE,
    EXPRESSIONKIND_COMPOUNDDEFINITION,
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
        char* string;
        char character;
        bool boolean;

        struct
        {
            struct Expression* base_type_rvalue;
            int length; // if -1, then its to be inferred
        } array_type;

        struct
        {
            struct Expression* base_type_rvalue;
        } pointer_type;

        struct
        {
            Token token;
        } type_identifier;

        struct
        {
            char* as_string;
        } identifier;

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
            Token identifier_token;

            struct Expression* args; // array of expressions
            size_t arg_count;
        } function_call;

        struct
        {
            Token identifier_token;

            Type variable_type; // to be filled in during semantic analysis
            struct Expression* type_rvalue;
            struct Expression* rvalue;
        } variable_declaration;

        struct
        {
            struct Expression** expressions;
            int statement_count;
        } compound;

        struct
        {
            Token identifier_token;

            Token* param_identifiers_tokens;
            struct Expression* param_type_rvalues;
            int param_count;
            bool is_variadic;

            struct Expression* return_type_rvalue;
            struct Expression* body;

            // to be filled in during semantic analysis
            Type return_type;
            Type* param_types;
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

        struct
        {
            struct Expression* base_type_rvalue;
            int count_initialized; // number of values initialized in the array literal
            struct Expression* initialized_rvalues;
        } array_literal;

        struct
        {
            // Type type; // to be filled in during semantic analysis
            struct Expression* lvalue;
            struct Expression* index_rvalue;
        } array_subscript;

        struct
        {
            // in "for num in nums"
            // "num" is the iterator
            // "nums" is the iterable

            // Type iterator_type; // to be filled in during semantic analysis
            Token iterator_token;
            struct Expression* iterable_rvalue;
            struct Expression* body;
        } for_loop;

        struct
        {
            Token identifier_token;
            struct Expression* rvalue;
            Type type; // to be filled in during semantic analysis
        } type_declaration;

        struct
        {
            struct Expression* lvalue;
            Token member_identifier_token;
        } member_access;

        struct
        {
            bool is_struct; // if false, is union
            int member_count;
            Token* member_identifier_tokens;
            struct Expression* member_type_rvalues;
            Type* member_types; // to be filled in during semantic analysis
        } compound_definition;

        struct
        {
            Token type_identifier_token;
            Token* member_identifier_tokens;
            struct Expression* initialized_member_rvalues;
            int initialized_count;
        } compound_literal;
    };
} Expression;

typedef struct Parser
{
    Token* tokens;
    int current_token_index;
    Token current_token;
    Token next_token;
} Parser;

void parser_initialize( Parser* parser, Token* _tokens );

Expression* parse( Parser* parser );

void expression_print( Expression* expression );

#endif
