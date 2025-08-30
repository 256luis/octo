#include "ast.h"
#include "debug.h"
#include "error.h"
#include "lvec.h"
#include "operation.h"
#include "tokenizer.h"
#include "globals.h"

#define EXPECT( ctx_ptr, ... )\
    _expect( ( ctx_ptr ),\
             ( TokenKind[] ){ __VA_ARGS__ },\
             sizeof( ( TokenKind[] ){ __VA_ARGS__ } ) / sizeof( TokenKind ) )

typedef struct AstContext
{
    Token* tokens;
    Token current_token;
    Token next_token;
    int current_token_index;
    bool error_found;
} AstContext;

static bool _expect( AstContext* ctx, TokenKind* expecteds, size_t length )
{
    for( size_t i = 0; i < length; i++ )
    {
        if( ctx->current_token.kind == expecteds[i] )
        {
            return true;
        }
    }

    ctx->error_found = true;
    return false;
}

static void advance( AstContext* ctx )
{
    ctx->current_token_index++;
    ctx->current_token = ctx->tokens[ ctx->current_token_index ];
    ctx->next_token = ctx->tokens[ ctx->current_token_index + 1 ];
}

static AstNode* parse_expression_rvalue( AstContext* ctx );
static AstNode* parse_expression( AstContext* ctx );
static AstNode* parse_term( AstContext* ctx );
static AstNode* parse_postfix( AstContext* ctx, AstNode* previous );
static AstNode* parse_type_definition( AstContext* ctx );

static AstNodeCompound parse_compound( AstContext* ctx )
{
    AstNodeCompound compound = {
        .nodes = lvec_new( AstNode* ),
    };

    advance( ctx );
    while( ctx->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        AstNode* n = parse_expression( ctx );
        if( ctx->error_found )
        {
            return compound;
        }

        lvec_append( compound.nodes, n );
        advance( ctx );
    }

    return compound;
}

// TODO: operator precedence!!!!!
static AstNodeBinary parse_binary( AstContext* ctx, AstNode* left )
{
    AstNodeBinary binary = {
        .left = left,
    };

    switch( ctx->current_token.kind )
    {
        case TOKENKIND_PLUS:         binary.operation = BINARYOPERATION_ADDITION; break;
        case TOKENKIND_MINUS:        binary.operation = BINARYOPERATION_SUBTRACTION; break;
        case TOKENKIND_STAR:         binary.operation = BINARYOPERATION_MULTIPLICATION; break;
        case TOKENKIND_FORWARDSLASH: binary.operation = BINARYOPERATION_DIVISION; break;
        case TOKENKIND_MODULO:       binary.operation = BINARYOPERATION_MODULO; break;
        case TOKENKIND_DOUBLEEQUAL:  binary.operation = BINARYOPERATION_EQUALTO; break;
        case TOKENKIND_NOTEQUAL:     binary.operation = BINARYOPERATION_NOTEQUALTO; break;
        case TOKENKIND_LESS:         binary.operation = BINARYOPERATION_LESSTHAN; break;
        case TOKENKIND_LESSEQUAL:    binary.operation = BINARYOPERATION_LESSTHANOREQUALTO; break;
        case TOKENKIND_GREATER:      binary.operation = BINARYOPERATION_GREATERTHAN; break;
        case TOKENKIND_GREATEREQUAL: binary.operation = BINARYOPERATION_GREATERTHANOREQUALTO; break;
        case TOKENKIND_AND:          binary.operation = BINARYOPERATION_AND; break;
        case TOKENKIND_OR:           binary.operation = BINARYOPERATION_OR; break;
        default: UNREACHABLE();
    }

    advance( ctx );
    binary.right = parse_expression_rvalue( ctx );

    return binary;
}

static AstNodeSubscript parse_subscript( AstContext* ctx, AstNode* target )
{
    AstNodeSubscript subscript = {
        .target = target
    };

    advance( ctx );
    subscript.index = parse_expression_rvalue( ctx );
    if ( ctx->error_found )
    {
        return subscript;
    }

    advance( ctx );
    if ( !EXPECT( ctx, TOKENKIND_RIGHTBRACKET ) )
    {
        Error error = {
            .kind = ERRORKIND_INCORRECTSYNTAX,
            .offending_token = ctx->current_token,
            .note = "array subscripts take the form `<array>[<expression>]`"
        };
        report_error( error );
    }

    return subscript;
}

static AstNodeUnary parse_unary( AstContext* ctx )
{
    AstNodeUnary unary;

    switch( ctx->current_token.kind )
    {
        case TOKENKIND_BANG:      unary.operation = UNARYOPERATION_NOT; break;
        case TOKENKIND_MINUS:     unary.operation = UNARYOPERATION_NEGATION; break;
        case TOKENKIND_AMPERSAND: unary.operation = UNARYOPERATION_ADDRESSOF; break;
        case TOKENKIND_STAR:      unary.operation = UNARYOPERATION_DEREFERENCE; break;
        default: UNREACHABLE();
    }

    advance( ctx );
    unary.operand = parse_term( ctx );
    return unary;
}

static AstNodeFunctionCall parse_function_call( AstContext* ctx, AstNode* function )
{
    AstNodeFunctionCall function_call = {
        .function = function,
    };

    function_call.args = lvec_new( AstNode* );

    advance( ctx );
    while( ctx->current_token.kind != TOKENKIND_RIGHTPAREN )
    {
        AstNode* arg = parse_expression_rvalue( ctx );
        if( ctx->error_found )
        {
            return function_call;
        }

        lvec_append( function_call.args, arg );
        advance( ctx );

        if ( !EXPECT( ctx, TOKENKIND_COMMA, TOKENKIND_RIGHTPAREN ) )
        {
            Error error = {
                .kind = ERRORKIND_INCORRECTSYNTAX,
                .offending_token = ctx->current_token,
                // TODO: check if this notation is correct
                .note = "function calls take the form `<identifier>(<expression>[, <expression>])`"
            };
            report_error( error );
            return function_call;
        }

        if( ctx->current_token.kind == TOKENKIND_COMMA )
        {
            advance( ctx );
        }
    }

    return function_call;
}

static AstNodeStructLiteral parse_struct_literal( AstContext* ctx, AstNode* type_definition )
{
    AstNodeStructLiteral struct_literal = {
        .initialized_member_tokens = lvec_new( Token ),
        .initialized_member_values = lvec_new( AstNode* ),
        .type_definition = type_definition,
    };

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_LEFTBRACE ) )
    {
        goto return_error;
    }

    advance( ctx );
    while( ctx->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        if( !EXPECT( ctx, TOKENKIND_PERIOD ) )
        {
            goto return_error;
        }

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_IDENTIFIER ) )
        {
            goto return_error;
        }

        lvec_append_aggregate( struct_literal.initialized_member_tokens, ctx->current_token );

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_EQUAL ) )
        {
            goto return_error;
        }

        advance( ctx );
        AstNode* member_value = parse_expression_rvalue( ctx );
        if( ctx->error_found )
        {
            return struct_literal;
        }

        lvec_append_aggregate( struct_literal.initialized_member_values, member_value );

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_COMMA, TOKENKIND_RIGHTBRACE ) )
        {
            goto return_error;
        }

        if( ctx->current_token.kind == TOKENKIND_COMMA )
        {
            advance( ctx );
        }
    }

    return struct_literal;

 return_error:
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = "struct literals take the form `<identifier>.{ .<identifier>: <type>, ... }`"
    };
    report_error( error );
    return struct_literal;
}

static AstNodeMemberAccess parse_member_access( AstContext* ctx, AstNode* target )
{
    AstNodeMemberAccess member_access = {
        .target = target,
    };

    advance( ctx );
    member_access.member = parse_term( ctx );
    return member_access;
}

static AstNode* parse_postfix( AstContext* ctx, AstNode* previous )
{
    AstNode* node = octo_malloc( sizeof( AstNode ) );

    switch( ctx->current_token.kind )
    {
        // subscript
        case TOKENKIND_LEFTBRACKET:
        {
            node->kind = ASTNODEKIND_SUBSCRIPT;
            node->subscript = parse_subscript( ctx, previous );
            break;
        }

        case TOKENKIND_LEFTPAREN:
        {
            node->kind = ASTNODEKIND_FUNCTIONCALL;
            node->function_call = parse_function_call( ctx, previous );
            break;
        }

        case TOKENKIND_PERIOD:
        {
            switch( ctx->next_token.kind )
            {
                case TOKENKIND_LEFTBRACE:
                {
                    node->kind = ASTNODEKIND_STRUCTLITERAL;
                    node->struct_literal = parse_struct_literal( ctx, previous );
                    break;
                }

                case TOKENKIND_IDENTIFIER:
                {
                    node->kind = ASTNODEKIND_MEMBERACCESS;
                    node->member_access = parse_member_access( ctx, previous );
                    break;
                }

                default:
                {
                    Error error = {
                        .kind = ERRORKIND_UNEXPECTEDSYMBOL,
                        .offending_token = ctx->next_token,
                    };
                    report_error( error );
                    ctx->error_found = true;
                    return NULL;
                }
            }

            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }

    // this is kinda pointless but im keeping this here for symmetry
    if( ctx->error_found )
    {
        return NULL;
    }

    return node;
}

static AstNodeEnumDefinition parse_enum_definition( AstContext* ctx )
{
    AstNodeEnumDefinition enum_definition = {
        .variant_names = lvec_new( Token ),
    };

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_LEFTBRACE ) )
    {
        goto return_error;
    }

    advance( ctx );
    while( ctx->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        if( !EXPECT( ctx, TOKENKIND_IDENTIFIER ) )
        {
            goto return_error;
        }

        lvec_append_aggregate( enum_definition.variant_names, ctx->current_token );

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_COMMA, TOKENKIND_RIGHTBRACE ) )
        {
            goto return_error;
        }

        if( ctx->current_token.kind == TOKENKIND_COMMA )
        {
            advance( ctx );
        }
    }

    return enum_definition;

 return_error:
    char* error_note =
        "enum definitions take the form\n"
        "enum {\n"
        "    <identifier>,\n"
        "    ...\n"
        "}";
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = error_note,
    };
    report_error( error );
    return enum_definition;
}

static AstNodeStructDefinition parse_struct_definition( AstContext* ctx )
{
    AstNodeStructDefinition struct_definition = {
        .member_identifiers = lvec_new( Token ),
        .member_types = lvec_new( AstNode* ),
    };

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_LEFTBRACE ) )
    {
        goto return_error;
    }

    advance( ctx );
    while( ctx->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        if( !EXPECT( ctx, TOKENKIND_IDENTIFIER ) )
        {
            goto return_error;
        }

        lvec_append_aggregate( struct_definition.member_identifiers, ctx->current_token );

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_COLON ) )
        {
            goto return_error;
        }

        advance( ctx );
        AstNode* member_type = parse_type_definition( ctx );
        if( ctx->error_found )
        {
            return struct_definition;
        }

        lvec_append_aggregate( struct_definition.member_types, member_type );

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_COMMA, TOKENKIND_RIGHTBRACE ) )
        {
            goto return_error;
        }

        if( ctx->current_token.kind == TOKENKIND_COMMA )
        {
            advance( ctx );
        }
    }

    return struct_definition;

 return_error:
    char* error_note =
        "struct definitions take the form\n"
        "struct {\n"
        "    <identifier> : <type> ,\n"
        "    ...\n"
        "}";
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = error_note,
    };
    report_error( error );
    return struct_definition;
}

static AstNodeVariableDeclaration parse_variable_declaration( AstContext* ctx )
{
    AstNodeVariableDeclaration variable_declaration = { 0 };
    advance( ctx );

    if( !EXPECT( ctx, TOKENKIND_IDENTIFIER ) )
    {
        goto return_error;
    }

    variable_declaration.identifier_token = ctx->current_token;

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_EQUAL, TOKENKIND_COLON ) )
    {
        goto return_error;
    }

    if( ctx->current_token.kind == TOKENKIND_COLON )
    {
        advance( ctx );

        variable_declaration.type_definition = parse_type_definition( ctx );
        if( ctx->error_found )
        {
            return variable_declaration;
        }

        if( ctx->next_token.kind == TOKENKIND_EQUAL )
        {
            advance( ctx );
        }
    }

    if( ctx->current_token.kind == TOKENKIND_EQUAL )
    {
        advance( ctx );
        variable_declaration.value = parse_expression_rvalue( ctx );
    }

    return variable_declaration;

 return_error:
    char* error_note =
        "variable declarations may take one of the following forms:\n"
        "- `let <identifier> = <expression>`\n"
        "- `let <identifier> : <type>`\n"
        "- `let <identifier> : <type> = <expression>`";
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = error_note,
    };
    report_error( error );
    return variable_declaration;
}

static AstNodeArrayDefinition parse_array_definition( AstContext* ctx )
{
    AstNodeArrayDefinition array_definition = { 0 };

    advance( ctx );
    if( ctx->current_token.kind != TOKENKIND_RIGHTBRACKET )
    {

        array_definition.length = parse_expression_rvalue( ctx );
        if( ctx->error_found )
        {
            return array_definition;
        }

        advance( ctx );
    }

    if( !EXPECT( ctx, TOKENKIND_RIGHTBRACKET ) )
    {
        goto return_error;
    }

    advance( ctx );
    array_definition.type_definition = parse_type_definition( ctx );
    if( ctx->error_found )
    {
        return array_definition;
    }

    return array_definition;

 return_error:
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = "array definitions take the form `[<expression>]T`"
    };
    report_error( error );
    return array_definition;
}

static AstNode* parse_type_definition( AstContext* ctx )
{
    AstNode* node = octo_malloc( sizeof( AstNode ) );

    switch( ctx->current_token.kind )
    {
        case TOKENKIND_IDENTIFIER:
        {
            node->kind = ASTNODEKIND_IDENTIFIER;
            node->identifier.token = ctx->current_token;
            break;
        }

        case TOKENKIND_STRUCT:
        {
            node->kind = ASTNODEKIND_STRUCTDEFINITION;
            node->struct_definition = parse_struct_definition( ctx );
            break;
        }

        case TOKENKIND_ENUM:
        {
            node->kind = ASTNODEKIND_ENUMDEFINITION;
            node->enum_definition = parse_enum_definition( ctx );
            break;
        }

        case TOKENKIND_LEFTBRACKET:
        {
            node->kind = ASTNODEKIND_ARRAYDEFINITION;
            node->array_definition = parse_array_definition( ctx );
            break;
        }

        default:
        {
            Error error = {
                .kind = ERRORKIND_INCORRECTSYNTAX,
                .offending_token = ctx->current_token,
                .note = "expected type"
            };
            report_error( error );
            ctx->error_found = true;
            return node;
        }
    }

    return node;
}

static AstNodeTypeDeclaration parse_type_declaration( AstContext* ctx )
{
    AstNodeTypeDeclaration type_declaration = { 0 };

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_IDENTIFIER ) )
    {
        goto return_error;
    }

    type_declaration.identifier_token = ctx->current_token;

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_EQUAL ) )
    {
        goto return_error;
    }

    advance( ctx );
    type_declaration.type_definition = parse_type_definition( ctx );

    return type_declaration;

 return_error:
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = "type declarations take the form `type <identifier> = <type>`",
    };
    report_error( error );
    return type_declaration;
}

static AstNodeRoutineDefinition parse_routine_definition( AstContext* ctx )
{
    AstNodeRoutineDefinition routine_definition = {
        .param_identifier_tokens = lvec_new( Token ),
        .param_type_definitions = lvec_new( AstNode* ),
    };

    switch( ctx->current_token.kind )
    {
        case TOKENKIND_FUNC:
        {
            routine_definition.is_func = true;
            break;
        }

        case TOKENKIND_PROC:
        {
            routine_definition.is_func = false;
            break;
        }

        default:
        {
            goto return_error;
        }
    }

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_LEFTPAREN ) )
    {
        goto return_error;
    }

    advance( ctx );
    while( ctx->current_token.kind != TOKENKIND_RIGHTPAREN )
    {
        if( !EXPECT( ctx, TOKENKIND_IDENTIFIER ) )
        {
            goto return_error;
        }

        lvec_append_aggregate( routine_definition.param_identifier_tokens, ctx->current_token );

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_COLON ) )
        {
            goto return_error;
        }

        advance( ctx );
        AstNode* param_type_definition = parse_type_definition( ctx );
        if( ctx->error_found )
        {
            return routine_definition;
        }

        lvec_append_aggregate( routine_definition.param_type_definitions, param_type_definition );

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_COMMA, TOKENKIND_RIGHTPAREN ) )
        {
            goto return_error;
        }

        if( ctx->current_token.kind == TOKENKIND_COMMA )
        {
            advance( ctx );
        }
    }

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_ARROW ) )
    {
        goto return_error;
    }

    advance( ctx );
    routine_definition.return_type_definition = parse_type_definition( ctx );
    if( ctx->error_found )
    {
        return routine_definition;
    }

    advance( ctx );
    routine_definition.body = parse_expression_rvalue( ctx );
    if( ctx->error_found )
    {
        return routine_definition;
    }

    return routine_definition;

 return_error:
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = "routine definitions take the form `func | proc ( <identifier> : <type>, ... ) -> <type> <expression>`",
    };
    report_error( error );
    ctx->error_found = true;
    return routine_definition;
}

static AstNodeRoutineDeclaration parse_routine_declaration( AstContext* ctx )
{
    AstNodeRoutineDeclaration routine_declaration = { 0 };

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_IDENTIFIER ) )
    {
        goto return_error;
    }

    routine_declaration.identifier_token = ctx->current_token;

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_EQUAL ) )
    {
        goto return_error;
    }

    advance( ctx );
    routine_declaration.routine_definition = parse_expression_rvalue( ctx );

    return routine_declaration;

 return_error:
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = "routine declarations take the form `routine <identifier> = <definition>`"
    };
    report_error( error );
    return routine_declaration;
}

static AstNodeConditional parse_conditional( AstContext* ctx )
{
    AstNodeConditional conditional = { 0 };

    if( ctx->current_token.kind == TOKENKIND_WHILE )
    {
        conditional.is_while = true;
    } // no else because it is false by default

    advance( ctx );
    conditional.condition = parse_expression_rvalue( ctx );
    if( ctx->error_found )
    {
        return conditional;
    }

    advance( ctx );
    conditional.main_body = parse_expression_rvalue( ctx );
    if( ctx->error_found )
    {
        return conditional;
    }

    if( ctx->next_token.kind != TOKENKIND_ELSE )
    {
        return conditional;
    }

    advance( ctx );
    advance( ctx ); // skip the else
    conditional.else_body = parse_expression_rvalue( ctx );
    if( ctx->error_found )
    {
        return conditional;
    }

    return conditional;
}

static AstNodeArrayLiteral parse_array_literal( AstContext* ctx )
{
    AstNodeArrayLiteral array_literal = {
        .initialized_elements = lvec_new( AstNode* ),
    };

    advance( ctx );
    if( ctx->current_token.kind != TOKENKIND_RIGHTBRACKET )
    {

        array_literal.length = parse_expression_rvalue( ctx );
        if( ctx->error_found )
        {
            return array_literal;
        }

        advance( ctx );
    }

    if( !EXPECT( ctx, TOKENKIND_RIGHTBRACKET ) )
    {
        goto return_error;
    }

    advance( ctx );
    array_literal.type_definition = parse_type_definition( ctx );
    if( ctx->error_found )
    {
        return array_literal;
    }

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_PERIOD ) )
    {
        goto return_error;
    }

    advance( ctx );
    if( !EXPECT( ctx, TOKENKIND_LEFTBRACE ) )
    {
        goto return_error;
    }

    advance( ctx );
    while( ctx->current_token.kind != TOKENKIND_RIGHTBRACE )
    {
        AstNode* initialized_element = parse_expression_rvalue( ctx );
        if( ctx->error_found )
        {
            goto return_error;
        }

        lvec_append( array_literal.initialized_elements, initialized_element );

        advance( ctx );
        if( !EXPECT( ctx, TOKENKIND_COMMA, TOKENKIND_RIGHTBRACE ) )
        {
            goto return_error;
        }

        if( ctx->current_token.kind == TOKENKIND_COMMA )
        {
            advance( ctx );
        }
    }

    return array_literal;

 return_error:
    Error error = {
        .kind = ERRORKIND_INCORRECTSYNTAX,
        .offending_token = ctx->current_token,
        .note = "array literals take the form `[<expression>]T.{<expression[, <expression>]}`"
    };
    report_error( error );
    return array_literal;
}

static AstNode* parse_term( AstContext* ctx )
{
    AstNode* node = octo_malloc( sizeof( AstNode ) );
    node->starting_token = ctx->current_token;

    switch( ctx->current_token.kind )
    {
        case TOKENKIND_STRING:
        {
            node->kind = ASTNODEKIND_STRINGLITERAL;
            node->string_literal.token = ctx->current_token;
            break;
        }

        case TOKENKIND_CHARACTER:
        {
            node->kind = ASTNODEKIND_CHARACTERLITERAL;
            node->character_literal.token = ctx->current_token;
            break;
        }

        case TOKENKIND_INTEGER:
        {
            node->kind = ASTNODEKIND_INTEGERLITERAL;
            node->integer_literal.integer = ctx->current_token.integer;
            break;
        }

        case TOKENKIND_FLOAT:
        {
            node->kind = ASTNODEKIND_FLOATLITERAL;
            node->float_literal.floating = ctx->current_token.floating;
            break;
        }

        case TOKENKIND_BOOLEAN:
        {
            node->kind = ASTNODEKIND_BOOLEANLITERAL;
            node->boolean_literal.boolean = ctx->current_token.boolean;
            break;
        }

        case TOKENKIND_IDENTIFIER:
        {
            node->kind = ASTNODEKIND_IDENTIFIER;
            node->identifier.token = ctx->current_token;
            break;
        }

        case TOKENKIND_LEFTBRACE:
        {
            node->kind = ASTNODEKIND_COMPOUND;
            node->compound = parse_compound( ctx );
            break;
        }

        // unary operations
        case TOKENKIND_BANG:
        case TOKENKIND_MINUS:
        case TOKENKIND_AMPERSAND:
        case TOKENKIND_STAR:
        {
            node->kind = ASTNODEKIND_UNARY;
            node->unary = parse_unary( ctx );
            break;
        }

        case TOKENKIND_LET:
        {
            node->kind = ASTNODEKIND_VARIABLEDECLARATION;
            node->variable_declaration = parse_variable_declaration( ctx );
            break;
        }

        case TOKENKIND_TYPE:
        {
            node->kind = ASTNODEKIND_TYPEDECLARATION;
            node->type_declaration = parse_type_declaration( ctx );
            break;
        }

        case TOKENKIND_STRUCT:
        {
            node->kind = ASTNODEKIND_STRUCTDEFINITION;
            node->struct_definition = parse_struct_definition( ctx );
            break;
        }

        case TOKENKIND_ENUM:
        {
            node->kind = ASTNODEKIND_ENUMDEFINITION;
            node->enum_definition = parse_enum_definition( ctx );
            break;
        }

        case TOKENKIND_ROUTINE:
        {
            node->kind = ASTNODEKIND_ROUTINEDECLARATION;
            node->routine_declaration = parse_routine_declaration( ctx );
            break;
        }

        case TOKENKIND_FUNC:
        case TOKENKIND_PROC:
        {
            node->kind = ASTNODEKIND_ROUTINEDEFINITION;
            node->routine_definition = parse_routine_definition( ctx );
            break;
        }

        case TOKENKIND_WHILE:
        case TOKENKIND_IF:
        {
            node->kind = ASTNODEKIND_CONDITIONAL;
            node->conditional = parse_conditional( ctx );
            break;
        }

        case TOKENKIND_LEFTBRACKET:
        {
            node->kind = ASTNODEKIND_ARRAYLITERAL;
            node->array_literal = parse_array_literal( ctx );
            break;
        }

        case TOKENKIND_PERIOD:
        {
            switch( ctx->next_token.kind )
            {
                case TOKENKIND_LEFTBRACE:
                {
                    node->kind = ASTNODEKIND_STRUCTLITERAL;
                    node->struct_literal = parse_struct_literal( ctx, NULL );
                    break;
                }

                case TOKENKIND_IDENTIFIER:
                {
                    node->kind = ASTNODEKIND_MEMBERACCESS;
                    node->member_access = parse_member_access( ctx, NULL );
                    break;
                }

                default:
                {
                    advance( ctx );
                    goto return_error;
                }
            }

            break;
        }

        default:
        {
            goto return_error;
        }
    }

    if( ctx->error_found )
    {
        return NULL;
    }

    // postfix operator parsing
    while( TOKENKIND_IS_IN_GROUP( ctx->next_token.kind, TOKENKIND_POSTFIX_OPERATORS ) )
    {
        advance( ctx );
        AstNode* previous = node;

        node = parse_postfix( ctx, previous );
        if( ctx->error_found )
        {
            return NULL;
        }
    }

    return node;

 return_error:
    Error error = {
        .kind = ERRORKIND_UNEXPECTEDSYMBOL,
        .offending_token = ctx->current_token,
    };
    report_error( error );
    ctx->error_found = true;
    return NULL;
}

static AstNodeAssignment parse_assignment( AstContext* ctx, AstNode* target )
{
    AstNodeAssignment assignment = {
        .target = target,
    };

    advance( ctx );

    // no need to call EXPECT here
    // we already know that the current token is TOKENKIND_EQUAL because it has already been
    // checked by parse_expression which is what called this function

    assignment.value = parse_expression_rvalue( ctx );
    return assignment;
}

static AstNode* parse_expression_rvalue( AstContext* ctx )
{
    AstNode* node = parse_term( ctx );
    if( ctx->error_found )
    {
        return NULL;
    }

    // binary operator parsing
    if( TOKENKIND_IS_IN_GROUP( ctx->next_token.kind, TOKENKIND_BINARY_OPERATORS ) )
    {
        advance( ctx );
        AstNode* left = node;

        node = octo_malloc( sizeof( AstNode ) );
        node->kind = ASTNODEKIND_BINARY;
        node->binary = parse_binary( ctx, left );
    }

    if( ctx->error_found )
    {
        return NULL;
    }

    return node;
}

// TODO: think about if we really need a duplicate of parse_expression_rvalue just for assignment
//       or if we should offload checking of assignments to semantic analysis
static AstNode* parse_expression( AstContext* ctx )
{
    AstNode* node = parse_term( ctx );
    if( ctx->error_found )
    {
        return NULL;
    }

    // assignment
    if( ctx->next_token.kind == TOKENKIND_EQUAL )
    {
        advance( ctx );
        AstNode* target = node;

        node = octo_malloc( sizeof( AstNode ) );
        node->kind = ASTNODEKIND_ASSIGNMENT;
        node->assignment = parse_assignment( ctx, target );
    }
    // binary operator parsing
    else if( TOKENKIND_IS_IN_GROUP( ctx->next_token.kind, TOKENKIND_BINARY_OPERATORS ) )
    {
        advance( ctx );
        AstNode* left = node;

        node = octo_malloc( sizeof( AstNode ) );
        node->kind = ASTNODEKIND_BINARY;
        node->binary = parse_binary( ctx, left );
    }

    if( ctx->error_found )
    {
        return NULL;
    }

    return node;
}

AstNode* ast_from_tokens( Token* tokens )
{
    AstContext ctx = {
        .tokens = tokens,
        .current_token = tokens[0],
        .next_token = tokens[1],
        .current_token_index = 0,
        .error_found = false,
    };

    AstNode* program = octo_malloc( sizeof( AstNode ) );
    program->kind = ASTNODEKIND_COMPOUND;
    program->compound.nodes = lvec_new( AstNode* );

    while( ctx.current_token.kind != TOKENKIND_EOF )
    {
        AstNode* n = parse_expression( &ctx );
        if( ctx.error_found )
        {
            return NULL;
        }

        lvec_append( program->compound.nodes, n );
        advance( &ctx );
    }

    return program;
}
