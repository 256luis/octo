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
    ASTNODEKIND_ROUTINECALL,
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
    ASTNODEKIND_ASSIGNMENT,
    ASTNODEKIND_POINTERDEFINITION,
    ASTNODEKIND_UNINITIALIZED,
    ASTNODEKIND_RETURN,
    ASTNODEKIND_ECHO,
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
    Token operation_token;
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

typedef struct AstNodeRoutineCall
{
    AstNode* routine;
    AstNode** args; // lvec
} AstNodeRoutineCall;

typedef struct AstNodeStructDefinition
{
    Token* member_identifiers; // lvec
    AstNode** member_type_definitions; // lvec
} AstNodeStructDefinition;

typedef struct AstNodeEnumDefinition
{
    Token* identifier_token; // can be null
    Token* variant_names;    // lvec
} AstNodeEnumDefinition;

typedef struct AstNodeVariableDeclaration
{
    bool is_mutable;
    Token identifier_token;
    AstNode* type_definition; // can be null
    AstNode* value;           // can be null
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
    Token* param_identifier_tokens;   // lvec
    AstNode** param_type_definitions; // lvec
    bool* params_mutability;          // lvec
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
    AstNode* main_body;
    AstNode* else_body; // can be null
    bool is_while; // if while statement, true. else false
} AstNodeConditional;

typedef struct AstNodeArrayLiteral
{
    AstNode* base_type_definition;   // can be null
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
    Token member_token;
} AstNodeMemberAccess;

typedef struct AstNodeArrayDefinition
{
    AstNode* length; // can be null
    AstNode* base_type_definition;
} AstNodeArrayDefinition;

typedef struct AstNodeAssignment
{
    AstNode* target;
    AstNode* value;
} AstNodeAssignment;

typedef struct AstNodePointerDefinition
{
    AstNode* base_type_definition;
} AstNodePointerDefinition;

typedef struct AstNodeReturn
{
    AstNode* value; // can be null
} AstNodeReturn;

typedef struct AstNodeEcho
{
    AstNode* value;
} AstNodeEcho;

typedef struct AstNode
{
    AstNodeKind kind;
    Type type;
    Token starting_token;

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
        AstNodeRoutineCall routine_call;
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
        AstNodeAssignment assignment;
        AstNodePointerDefinition pointer_definition;
        AstNodeReturn return_statement;
        AstNodeEcho echo;
    };
} AstNode;

AstNode* ast_from_tokens( Token* tokens );
void ast_node_print( AstNode node );

#endif
