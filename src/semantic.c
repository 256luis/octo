#include <string.h>
#include "debug.h"
#include "error.h"
#include "parser.h"
#include "lvec.h"
#include "tokenizer.h"
#include "semantic.h"

#define TYPEKINDPAIR_TERMINATOR (( TypeKindPair ){ -1, -1 })

typedef struct Symbol
{
    Token token;
    Type type;
} Symbol;

typedef struct SymbolTable
{
    Symbol* symbols;
    int length;
} SymbolTable;

typedef struct TypeKindPair
{
    TypeKind tk1;
    TypeKind tk2;
} TypeKindPair;

typedef struct SemanticContext
{
    SymbolTable* symbol_table_stack;
    Type* return_type_stack;
} SemanticContext;

static SemanticContext context;

void push_return_type( Type type )
{
    lvec_append_aggregate( context.return_type_stack, type );
}

void pop_return_type()
{
    lvec_remove_last( context.return_type_stack );
}

Type get_top_return_type()
{
    int last_index = lvec_get_length( context.return_type_stack ) - 1;
    return context.return_type_stack[ last_index ];
}

void push_scope()
{
    SymbolTable st = {
        .symbols = lvec_new( Symbol ),
        .length = 0,
    };
    lvec_append_aggregate( context.symbol_table_stack, st );
}

void pop_scope()
{
    lvec_remove_last( context.symbol_table_stack );
}

void add_symbol_to_scope( Symbol symbol )
{
    int symbol_table_stack_top_index = lvec_get_length( context.symbol_table_stack ) - 1;
    SymbolTable* current_scope_table = &context.symbol_table_stack[ symbol_table_stack_top_index ];

    lvec_append_aggregate( current_scope_table->symbols, symbol );
    current_scope_table->length++;
}

// searches the entire symbol table stack backwards for identifier
Symbol* symbol_lookup( char* identifier )
{
    int symbol_stack_length = lvec_get_length( context.symbol_table_stack );
    for( int i = 0; i < symbol_stack_length ; i++ )
    {
        SymbolTable current_scope_table = context.symbol_table_stack[ i ];

        for( int j = 0; j < current_scope_table.length; j++ )
        {
            Symbol* symbol = &current_scope_table.symbols[ j ];

            // if identifier matches entry in symbol table
            if( strcmp( identifier, symbol->token.identifier ) == 0 )
            {
                return symbol;
            }
        }
    }

    return NULL;
}

bool type_equals( Type t1, Type t2 )
{
    bool are_kinds_equal = t1.kind == t2.kind;
    if( !are_kinds_equal )
    {
        return false;
    }

    bool are_custom_identifiers_equal = true;
    if( t1.kind == TYPEKIND_CUSTOM )
    {
        are_custom_identifiers_equal = strcmp( t1.custom_identifier, t2.custom_identifier ) == 0;
    }

    if( !are_custom_identifiers_equal )
    {
        return false;
    }

    return true;
}

static bool is_binary_operation_valid( BinaryOperation operation, Type left_type, Type right_type )
{
    TypeKindPair* valid_binary_operations[] = {
        [ BINARYOPERATION_ADD ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },

            // this is needed because we do not know the length of each entry in the
            // valid_binary_operation array. this would kinda act like the null
            // terminator in strings.
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_SUBTRACT ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_MULTIPLY ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_DIVIDE ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_EQUAL ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_BOOLEAN, TYPEKIND_BOOLEAN },
            ( TypeKindPair ){ TYPEKIND_STRING,  TYPEKIND_STRING },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_NOTEQUAL ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_BOOLEAN, TYPEKIND_BOOLEAN },
            ( TypeKindPair ){ TYPEKIND_STRING,  TYPEKIND_STRING },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_GREATER ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_GREATEREQUAL ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_LESS ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_LESSEQUAL ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,   TYPEKIND_INTEGER },
            TYPEKINDPAIR_TERMINATOR,
        },
    };

    TypeKindPair* pairs = valid_binary_operations[ operation ];

    bool is_valid = false;

    // iterate over all valid pairs
    for( TypeKindPair pair = *pairs; pair.tk1 != -1; pairs++, pair = *pairs )
    {
        TypeKind pair_left = pair.tk1;
        TypeKind pair_right = pair.tk2;

        if( pair_left == left_type.kind && pair_right == right_type.kind )
        {
            is_valid = true;
            break;
        }
    }

    return is_valid;
}

static bool check_rvalue( Expression* expression, Type* inferred_type );
static bool check_binary( Expression* expression, Type* inferred_type )
{
    Expression* left_expression = expression->binary.left;
    Expression* right_expression = expression->binary.right;

    Type left_type;
    Type right_type;

    bool is_left_valid = check_rvalue( left_expression, &left_type );
    bool is_right_valid = check_rvalue( right_expression, &right_type );

    if( !is_left_valid || !is_right_valid )
    {
        return false;
    }

    BinaryOperation operation = expression->binary.operation;
    bool is_operation_valid = is_binary_operation_valid( operation, left_type, right_type );

    if( !is_operation_valid )
    {
        // todo: clean this shit up

        // find the token that corresponds with the operator
        int depth = 0; // measures how deep we are in the expression
                       // +1 for every left paren found
                       // -1 for every right paren found
        bool first = true;
        int lowest_depth;
        int operator_index_lowest_depth;
        for( size_t i = 0; i < lvec_get_length( expression->associated_tokens ); i++ )
        {
            Token current_token = expression->associated_tokens[ i ];
            if( current_token.kind == TOKENKIND_LEFTPAREN )
            {
                depth++;
            }
            else if( current_token.kind == TOKENKIND_RIGHTPAREN )
            {
                depth--;
            }

            if( IS_TOKENKIND_IN_GROUP( current_token.kind, TOKENKIND_BINARY_OPERATORS ) )
            {
                if( first )
                {
                    first = false;
                    operator_index_lowest_depth = i;
                    lowest_depth = depth;
                    continue;
                }

                if( depth < lowest_depth )
                {
                    operator_index_lowest_depth = i;
                }
            }
        }

        Token operator_token = expression->associated_tokens[ operator_index_lowest_depth ];
        Error error = {
            .kind = ERRORKIND_INVALIDOPERATION,
            .offending_token = operator_token,
            .invalid_operation = {
                .left_type = left_type,
                .right_type = right_type,
            },
        };
        report_error( error );
        return false;
    }

    // if operation is one of the boolean operators
    if( operation >= BINARYOPERATION_BOOLEAN_START && operation < BINARYOPERATION_BOOLEAN_END )
    {
        inferred_type->kind = TYPEKIND_BOOLEAN;
    }
    else
    {
        // if left_type or right_type is float, then float. otherwise its int
        if( left_type.kind == TYPEKIND_FLOAT || right_type.kind == TYPEKIND_FLOAT )
        {
            inferred_type->kind = TYPEKIND_FLOAT;
        }
        else
        {
            inferred_type->kind = TYPEKIND_INTEGER;
        }
    }

    return true;
}

static bool check_rvalue( Expression* expression, Type* inferred_type )
{
    // initialized to true for the base cases
    bool is_valid = true;

    switch( expression->kind )
    {
        case EXPRESSIONKIND_INTEGER:   inferred_type->kind = TYPEKIND_INTEGER;   break;
        case EXPRESSIONKIND_STRING:    inferred_type->kind = TYPEKIND_STRING;    break;
        case EXPRESSIONKIND_CHARACTER: inferred_type->kind = TYPEKIND_CHARACTER; break;
        // case EXPRESSIONKIND_FLOAT:  type.kind  = TYPEKIND_FLOAT;     break;

        case EXPRESSIONKIND_IDENTIFIER:
        {
            UNIMPLEMENTED();
            break;
        }

        case EXPRESSIONKIND_BINARY:
        {
            is_valid = check_binary( expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            UNIMPLEMENTED();
            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            UNIMPLEMENTED();
            break;
        }

        default:
        {
            // report error
            UNIMPLEMENTED();
            break;
        }
    }

    return is_valid;
}

static bool check_variable_declaration( Expression* expression )
{
    Token identifier_token = expression->associated_tokens[ 1 ];

    // check if identifier already in symbol table
    Symbol* original_declaration = symbol_lookup( identifier_token.identifier );
    if( original_declaration != NULL )
    {
        Error error = {
            .kind = ERRORKIND_SYMBOLREDECLARATION,
            .offending_token = identifier_token,
            .symbol_redeclaration.original_declaration_token = original_declaration->token,
        };

        report_error( error );
        return false;
    }

    // check if the type at the left hand side matches the type on the right hand
    // side (if not ommitted)

    Type declared_type = expression->variable_declaration.type;
    Type inferred_type;
    Expression* value = expression->variable_declaration.rvalue;
    if( value != NULL )
    {
        // infer type from value
        bool is_valid = check_rvalue( value, &inferred_type );
        if( !is_valid )
        {
            return false;
        }

        if( declared_type.kind == TYPEKIND_TOINFER )
        {
            declared_type = inferred_type;
        }
        else if( !type_equals( declared_type, inferred_type ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = expression->variable_declaration.rvalue->associated_tokens[ 0 ],
                .type_mismatch = {
                    .expected = declared_type,
                    .found = inferred_type,
                },
            };
            report_error( error );

            return false;
        }
    }

    expression->variable_declaration.type = declared_type;

    // add to symbol table
    Symbol symbol = {
        .token = identifier_token,
        .type = declared_type,
    };
    add_symbol_to_scope( symbol );

    return true;
}

static bool check_compound( Expression* expression )
{
    bool is_valid = true;

    push_scope();

    size_t length = lvec_get_length( expression->compound.expressions );
    for( size_t i = 0; i < length; i++ )
    {
        Expression* e = expression->compound.expressions[ i ];
        bool _is_valid = check_semantics( e );

        if( !_is_valid )
        {
            is_valid = false;
        }
    }

    pop_scope();

    return is_valid;
}

static bool check_function_declaration( Expression* expression )
{
    Token identifier_token = expression->associated_tokens[ 1 ];

    // check if identifier already in symbol table
    Symbol* original_declaration = symbol_lookup( identifier_token.identifier );
    if( original_declaration != NULL )
    {
        Error error = {
            .kind = ERRORKIND_SYMBOLREDECLARATION,
            .offending_token = identifier_token,
            .symbol_redeclaration.original_declaration_token = original_declaration->token,
        };

        report_error( error );
        return false;
    }

    // add to symbol table
    Symbol symbol = {
        .token = identifier_token,
        .type = ( Type ){ .kind = TYPEKIND_FUNCTION },
    };
    add_symbol_to_scope( symbol );

    // check function params
    push_scope();
    for( int i = 0; i < expression->function_declaration.param_count; i++ )
    {
        int associated_token_index = 3 + ( i * 4 );
        Token param_identifier_token = expression->associated_tokens[ associated_token_index ];
        Type param_type = expression->function_declaration.param_types[ i ];

        Symbol* declaration = symbol_lookup( param_identifier_token.identifier );
        if( declaration != NULL )
        {
            Error error = {
                .kind = ERRORKIND_SYMBOLREDECLARATION,
                .offending_token = param_identifier_token,
                .symbol_redeclaration.original_declaration_token = declaration->token,
            };

            report_error( error );
            return false;
        }

        Symbol symbol = {
            .token = param_identifier_token,
            .type = param_type,
        };
        add_symbol_to_scope( symbol );
    }

    push_return_type( expression->function_declaration.return_type );

    // check function body
    Expression* function_body = expression->function_declaration.body;
    bool is_body_valid = check_compound( function_body );
    pop_scope();

    pop_return_type();

    if( !is_body_valid )
    {
        return false;
    }

    return true;
}

bool check_return( Expression* expression )
{
    Type found_return_type = ( Type ){ .kind = TYPEKIND_VOID };
    if( expression->return_expression.rvalue != NULL )
    {
        bool is_return_value_valid = check_rvalue( expression->return_expression.rvalue, &found_return_type );

        if( !is_return_value_valid )
        {
            // no need to report error here because error has already been reported
            // from the check_expression_rvalue function
            return false;
        }
    }

    Type expected_return_type = get_top_return_type();
    if( !type_equals( found_return_type, expected_return_type ) )
    {
        Error error = {
            .kind = ERRORKIND_TYPEMISMATCH,
            .offending_token = expression->return_expression.rvalue->associated_tokens[ 0 ],
            .type_mismatch = {
                .expected = expected_return_type,
                .found = found_return_type,
            },
        };
        report_error( error );
        return false;
    }

    return true;
}

bool check_assignment( Expression* expression )
{
    // check if identifier exists;
    Token identifier_token = expression->associated_tokens[ 0 ];
    Symbol* original_declaration = symbol_lookup( identifier_token.identifier );
    if( original_declaration == NULL )
    {
        Error error = {
            .kind = ERRORKIND_UNDECLAREDSYMBOL,
            .offending_token = identifier_token,
        };

        report_error( error );
        return false;
    }

    // check if rvalue is a valid rvalue
    Type found_type;
    bool is_valid = check_rvalue( expression->assignment.rvalue, &found_type );
    if( !is_valid )
    {
        // no need for error reporting here because that was already handled by
        // check_expression_rvalue
        return false;
    }

    // check if types match with original declaration
    Type expected_type = original_declaration->type;
    if( !type_equals( expected_type, found_type ) )
    {
        Error error = {
            .kind = ERRORKIND_TYPEMISMATCH,
            .offending_token = expression->assignment.rvalue->associated_tokens[ 0 ],
            .type_mismatch = {
                .expected = expected_type,
                .found = found_type,
            },
        };
        report_error( error );
        return false;
    }

    return true;
}

bool check_semantics( Expression* expression )
{
    // initialize semantic_context;
    static bool is_context_initialized = false;
    if( !is_context_initialized )
    {
        is_context_initialized = true;

        context.symbol_table_stack = lvec_new( SymbolTable );
        context.return_type_stack = lvec_new( Type );
    }

    bool is_valid;

    switch( expression->kind )
    {
        case EXPRESSIONKIND_VARIABLEDECLARATION:
        {
            is_valid = check_variable_declaration( expression );
            break;
        }

        case EXPRESSIONKIND_COMPOUND:
        {
            is_valid = check_compound( expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONDECLARATION:
        {
            is_valid = check_function_declaration( expression );
            break;
        }

        case EXPRESSIONKIND_RETURN:
        {
            is_valid = check_return( expression );
            break;
        }

        case EXPRESSIONKIND_ASSIGNMENT:
        {
            is_valid = check_assignment( expression );
            break;
        }

        default:
        {
            UNIMPLEMENTED();
        }
    }
    return is_valid;
}
