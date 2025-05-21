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

    bool result;
    switch( t1.kind )
    {
        // case TYPEKIND_FLOAT:
        case TYPEKIND_INTEGER:
        {
            bool is_same_size = t1.integer.bit_count == t2.integer.bit_count;
            bool is_same_signedness = t1.integer.is_signed == t2.integer.is_signed;
            result = is_same_size && is_same_signedness;
            break;
        }

        case TYPEKIND_FLOAT:
        {
            bool is_same_size = t1.integer.bit_count == t2.integer.bit_count;
            result = is_same_size;
            break;
        }

        case TYPEKIND_CUSTOM:
        {
            result = strcmp( t1.custom_identifier, t2.custom_identifier ) == 0;
            break;
        }

        case TYPEKIND_POINTER:
        {
            result = type_equals( *t1.pointer.type, *t2.pointer.type );
            break;
        }

        case TYPEKIND_FUNCTION:
        {
            UNIMPLEMENTED();
            break;
        }

        default:
        {
            UNREACHABLE();
            break;
        }
    }

    return result;
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
        Token operator_token = expression->binary.operator_token;
        Error error = {
            .kind = ERRORKIND_INVALIDBINARYOPERATION,
            .offending_token = operator_token,
            .invalid_binary_operation = {
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

static bool check_rvalue_identifier( Expression* expression, Type* inferred_type )
{
    Token identifier_token = expression->associated_token;

    // check if identifier already in symbol table
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

    *inferred_type = original_declaration->type;

    return true;
}

static bool check_function_call( Expression* expression, Type* inferred_type )
{
    Token identifier_token = expression->function_call.identifier_token;
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

    // check if args count match param count
    Type* param_types = original_declaration->type.function.param_types;

    int param_count = lvec_get_length( param_types );
    int arg_count = expression->function_call.arg_count;
    if( arg_count != param_count )
    {
        Error error = {
            .kind = ERRORKIND_TOOMANYARGUMENTS,
            .offending_token = identifier_token,
            .too_many_arguments = {
                .expected = param_count,
                .found = arg_count,
            },
        };
        report_error( error );
        return false;
    }

    // check if arg types match param types
    for( int i = 0; i < param_count; i++ )
    {
        Type param_type = param_types[ i ];
        Expression* arg = expression->function_call.args[ i ];
        Type arg_type;

        bool is_arg_valid = check_rvalue( arg, &arg_type );
        if( !is_arg_valid )
        {
            // no need to report error here because that is handled by check_rvalue
            return false;
        }

        if( !type_equals( param_type, arg_type ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = arg->starting_token,
                .type_mismatch = {
                    .expected = param_type,
                    .found = arg_type,
                },
            };
            report_error( error );
            return false;
        }
    }

    if( inferred_type != NULL )
    {
        *inferred_type = *original_declaration->type.function.return_type;
    }

    return true;
}

static bool is_unary_operation_valid( UnaryOperation operation, Type type )
{
    TypeKind* valid_unary_operation[] = {
        [ UNARYOPERATION_NEGATIVE ] = ( TypeKind[] ) {
            TYPEKIND_INTEGER, TYPEKIND_FLOAT, -1,
            // the -1 kinda acts like a null terminator
        },

        [ UNARYOPERATION_NOT ] = ( TypeKind[] ) {
            TYPEKIND_BOOLEAN, -1,
        },

        [ UNARYOPERATION_DEREFERENCE ] = ( TypeKind[] ) {
            TYPEKIND_POINTER, -1,
        },

        // not applicable (code path handles it separately)
        [ UNARYOPERATION_ADDRESSOF ] = NULL,
    };

    TypeKind* valid_type_kind = valid_unary_operation[ operation ];
    bool is_valid = false;
    for( TypeKind tk = *valid_type_kind; tk != -1; valid_type_kind++, tk = *valid_type_kind )
    {
        if( tk == type.kind )
        {
            is_valid = true;
            break;
        }
    }

    return is_valid;
}

static bool check_unary( Expression* expression, Type* inferred_type )
{
    Expression* operand = expression->unary.operand;
    Type operand_type;
    bool is_operand_valid = check_rvalue( operand, &operand_type );
    if( !is_operand_valid )
    {
        return false;
    }

    UnaryOperation operation = expression->unary.operation;
    if( operation == UNARYOPERATION_ADDRESSOF )
    {
        // operand must be identifier
        if( operand->kind != EXPRESSIONKIND_IDENTIFIER )
        {
            Error error = {
                .kind = ERRORKIND_INVALIDADDRESSOF,
                .offending_token = operand->starting_token
            };

            report_error( error );
            return false;
        }

        // check if operand is in symbol table
        Symbol* operand_symbol = symbol_lookup( operand->associated_token.identifier );
        if( operand_symbol == NULL )
        {
            Error error = {
                .kind = ERRORKIND_UNDECLAREDSYMBOL,
                .offending_token = operand->associated_token,
            };

            report_error( error );
            return false;
        }

        *inferred_type = ( Type ){
            .kind = TYPEKIND_POINTER,
            .pointer.type = &operand_symbol->type,
        };
    }
    else
    {
        bool is_operation_valid = is_unary_operation_valid( operation, operand_type );
        if( !is_operation_valid )
        {
            Token operator_token = expression->unary.operator_token;
            Error error = {
                .kind = ERRORKIND_INVALIDUNARYOPERATION,
                .offending_token = operator_token,
                .invalid_unary_operation = {
                    .operand_type = operand_type
                }
            };
            report_error( error );
            return false;
        }

        *inferred_type = operand_type;
    }

    return true;
}

static bool check_rvalue( Expression* expression, Type* inferred_type )
{
    // initialized to true for the base cases
    bool is_valid = true;

    switch( expression->kind )
    {
        case EXPRESSIONKIND_STRING:    inferred_type->kind = TYPEKIND_STRING;    break;
        case EXPRESSIONKIND_CHARACTER: inferred_type->kind = TYPEKIND_CHARACTER; break;

        case EXPRESSIONKIND_INTEGER:
        {
            inferred_type->kind = TYPEKIND_INTEGER;

            // default int type is i32
            inferred_type->integer.bit_count = 32;
            inferred_type->integer.is_signed = true;
            break;
        }

        case EXPRESSIONKIND_FLOAT:
        {
            inferred_type->kind = TYPEKIND_FLOAT;

            // default float type is f32
            inferred_type->integer.bit_count = 32;
            break;
        }

        case EXPRESSIONKIND_IDENTIFIER:
        {
            is_valid = check_rvalue_identifier( expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_BINARY:
        {
            is_valid = check_binary( expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            is_valid = check_unary( expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            is_valid = check_function_call( expression, inferred_type );
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
    Token identifier_token = expression->variable_declaration.identifier_token;

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
    Expression* rvalue = expression->variable_declaration.rvalue;
    if( rvalue != NULL )
    {
        // infer type from value
        bool is_valid = check_rvalue( rvalue, &inferred_type );
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
                .offending_token = expression->variable_declaration.rvalue->starting_token,
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
    Token identifier_token = expression->function_declaration.identifier_token;

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

    Type* param_types = expression->function_declaration.param_types;
    Type* return_type = &expression->function_declaration.return_type;

    // add to symbol table
    Symbol symbol = {
        .token = identifier_token,
        .type = ( Type ){
            .kind = TYPEKIND_FUNCTION,
            .function = {
                .param_types = param_types,
                .return_type = return_type
            }
        },
    };
    add_symbol_to_scope( symbol );

    // check function params
    push_scope();
    for( int i = 0; i < expression->function_declaration.param_count; i++ )
    {
        Token param_identifier_token = expression->function_declaration.param_identifiers_tokens[ i ];
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

    push_return_type( *return_type );

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
        Token associated_token;
        if( found_return_type.kind == TYPEKIND_VOID )
        {
            associated_token = expression->starting_token;
        }
        else
        {
            associated_token = expression->return_expression.rvalue->starting_token;
        }

        Error error = {
            .kind = ERRORKIND_TYPEMISMATCH,
            .offending_token = associated_token,
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
    Token identifier_token = expression->assignment.identifier_token;
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
            .offending_token = expression->assignment.rvalue->associated_token,
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

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            is_valid = check_function_call( expression, NULL );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }
    return is_valid;
}
