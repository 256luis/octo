#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "tokenizer.h"
#include "type.h"
#include "operation.h"

typedef struct AstNode AstNode;

typedef enum AstNodeKind
{
    ASTNODEKIND_STRINGLITERAL,
    ASTNODEKIND_CHARACTERLITERAL,
    ASTNODEKIND_INTEGERLITERAL,
    ASTNODEKIND_FLOATLITERAL,
    ASTNODEKIND_BOOLEANLITERAL,
    ASTNODEKIND_IDENTIFIER,
    ASTNODEKIND_COMPOUND,
    ASTNODEKIND_BINARY,
    ASTNODEKIND_UNARY,
    ASTNODEKIND_SUBSCRIPT,
    ASTNODEKIND_FUNCTIONCALL,
    ASTNODEKIND_VARIABLEDECLARATION,
    ASTNODEKIND_TYPEDECLARATION,
    ASTNODEKIND_STRUCTDEFINITION,
    ASTNODEKIND_ENUMDEFINITION,
    ASTNODEKIND_ROUTINEDECLARATION,
    ASTNODEKIND_ROUTINEDEFINITION,
    ASTNODEKIND_CONDITIONAL,
    ASTNODEKIND_ARRAYLITERAL,
    ASTNODEKIND_STRUCTLITERAL,
    ASTNODEKIND_MEMBERACCESS,
    ASTNODEKIND_ARRAYDEFINITION,
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
    AstNode* left;
    AstNode* right;
} AstNodeBinary;

typedef struct AstNodeUnary
{
    UnaryOperation operation;
    AstNode* operand;
} AstNodeUnary;

typedef struct AstNodeSubscript
{
    AstNode* target;
    AstNode* index;
} AstNodeSubscript;

typedef struct AstNodeCompound
{
    AstNode** nodes; // lvec
} AstNodeCompound;

typedef struct AstNodeFunctionCall
{
    AstNode* function;
    AstNode** args; // lvec
} AstNodeFunctionCall;

typedef struct AstNodeStructDefinition
{
    Token* member_identifiers; // lvec
    AstNode** member_types; // lvec
} AstNodeStructDefinition;

typedef struct AstNodeEnumDefinition
{
    Token* variant_names; // lvec
} AstNodeEnumDefinition;

typedef struct AstNodeVariableDeclaration
{
    Token identifier_token;
    AstNode* type_definition;
    AstNode* value;
} AstNodeVariableDeclaration;

typedef struct AstNodeTypeDeclaration
{
    Token identifier_token;
    AstNode* type_definition;
} AstNodeTypeDeclaration;

typedef struct AstNodeRoutineDefinition
{
    bool is_func;
    AstNode* return_type_definition;
    Token* param_identifier_tokens; // lvec
    AstNode** param_type_definitions; // lvec
    AstNode* body;
} AstNodeRoutineDefinition;

typedef struct AstNodeRoutineDeclaration
{
    Token identifier_token;
    AstNode* routine_definition;
} AstNodeRoutineDeclaration;

typedef struct AstNodeConditional
{
    AstNode* condition;
    AstNode* body;
    bool is_while; // if while statement, true. else false
} AstNodeConditional;

typedef struct AstNodeArrayLiteral
{
    AstNode* type_definition;
    AstNode* length;                 // can be null
    AstNode** initialized_elements;  // lvec
} AstNodeArrayLiteral;

typedef struct AstNodeStructLiteral
{
    AstNode* type_definition;           // can be null
    Token* initialized_member_tokens;   // lvec
    AstNode** initialized_member_values; // lvec
} AstNodeStructLiteral;

typedef struct AstNodeMemberAccess
{
    AstNode* target; // can be null
    AstNode* member;
} AstNodeMemberAccess;

typedef struct AstNodeArrayDefinition
{
    AstNode* length; // can be null
    AstNode* type_definition;
} AstNodeArrayDefinition;

typedef struct AstNode
{
    AstNodeKind kind;
    Type type;

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
        AstNodeTypeDeclaration type_declaration;
        AstNodeStructDefinition struct_definition;
        AstNodeEnumDefinition enum_definition;
        AstNodeRoutineDeclaration routine_declaration;
        AstNodeRoutineDefinition routine_definition;
        AstNodeConditional conditional;
        AstNodeArrayLiteral array_literal;
        AstNodeStructLiteral struct_literal;
        AstNodeMemberAccess member_access;
        AstNodeArrayDefinition array_definition;
    };
} AstNode;

AstNode* ast_from_tokens( Token* tokens );
void ast_node_print( AstNode node );

#endif
