#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "tokenizer.h"
#include "type.h"
#include "operation.h"

typedef enum AstNodeKind {
    ASTNODEKIND_STRINGLITERAL,
    ASTNODEKIND_CHARACTERLITERAL,
    ASTNODEKIND_INTEGERLITERAL,
    ASTNODEKIND_FLOATLITERAL,
    ASTNODEKIND_BOOLEANLITERAL,
    ASTNODEKIND_IDENTIFIER,
    ASTNODEKIND_COMPOUND,

    // rvalue nodes
    ASTNODEKIND_BINARY,
    ASTNODEKIND_UNARY,
    ASTNODEKIND_SUBSCRIPT,
    ASTNODEKIND_FUNCTIONCALL,
    ASTNODEKIND_VARIABLEDECLARATION,
} AstNodeKind;

typedef struct AstNodeString
{
    Token token;
} AstNodeString;

typedef struct AstNodeCharacter
{
    Token token;
} AstNodeCharacter;

typedef struct AstNodeInteger
{
    uint64_t integer;
    Token token;
} AstNodeInteger;

typedef struct AstNodeFloat
{
    double floating;
    Token token;
} AstNodeFloat;

typedef struct AstNodeBoolean
{
    bool boolean;
    Token token;
} AstNodeBoolean;

typedef struct AstNodeIdentifier
{
    Token token;
} AstNodeIdentifier;

typedef struct AstNodeBinary
{
    BinaryOperation operation;
    struct AstNode* left;
    struct AstNode* right;
} AstNodeBinary;

typedef struct AstNodeUnary
{
    UnaryOperation operation;
    struct AstNode* operand;
} AstNodeUnary;

typedef struct AstNodeSubscript
{
    struct AstNode* target;
    struct AstNode* index;
} AstNodeSubscript;

typedef struct AstNodeCompound
{
    struct AstNode** nodes; // lvec
} AstNodeCompound;

typedef struct AstNodeFunctionCall
{
    struct AstNode* function;
    struct AstNode** args; // lvec
} AstNodeFunctionCall;

typedef enum AstNodeTypeKind
{
    ASTNOTETYPEKIND_NONE,
    ASTNODETYPEKIND_IDENTIFIER,
    ASTNODETYPEKIND_STRUCT,
    ASTNODETYPEKIND_ENUM,
    ASTNODETYPEKIND_UNION,
} AstNodeTypeKind;

typedef struct AstNodeTypeIdentifier
{
    Token token;
} AstNodeTypeIdentifier;

typedef struct AstNodeType
{
    AstNodeTypeKind kind;
    union
    {
        AstNodeTypeIdentifier identifier;
    };
} AstNodeType;

typedef struct AstNodeVariableDeclaration
{
    Token identifier_token;
    AstNodeType type_node;
    struct AstNode* value;
} AstNodeVariableDeclaration;

typedef struct AstNode {
    AstNodeKind kind;
    Type type;
    bool is_return;

    union
    {
        AstNodeString string_literal;
        AstNodeCharacter character_literal;
        AstNodeInteger integer_literal;
        AstNodeFloat float_literal;
        AstNodeBoolean boolean_literal;
        AstNodeIdentifier identifier;
        AstNodeCompound compound;
        AstNodeBinary binary;
        AstNodeUnary unary;
        AstNodeSubscript subscript;
        AstNodeFunctionCall function_call;
        AstNodeVariableDeclaration variable_declaration;
    };
} AstNode;

AstNode* ast_from_tokens( Token* tokens );
void ast_node_print( AstNode node );

#endif
