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

static SymbolTable symbol_table;

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

Symbol* symbol_table_lookup( SymbolTable* symbol_table, char* identifier )
{
    for( int i = 0; i < symbol_table->length; i++ )
    {
        Symbol* symbol = &symbol_table->symbols[ i ];

        // if identifier matches entry in symbol table
        if( strcmp( identifier, symbol->token.identifier ) == 0 )
        {
            return symbol;
        }
    }

    return NULL;
}

void symbol_table_add( SymbolTable* symbol_table, Symbol symbol )
{
    lvec_append_aggregate( symbol_table->symbols, symbol );
    symbol_table->length++;
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

static bool check_expression_rvalue( Expression* expression, Type* inferred_type );
static bool check_binary( Expression* expression, Type* inferred_type )
{
    Expression* left_expression = expression->binary.left;
    Expression* right_expression = expression->binary.right;

    Type left_type;
    Type right_type;

    bool is_left_valid = check_expression_rvalue( left_expression, &left_type );
    bool is_right_valid = check_expression_rvalue( right_expression, &right_type );

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

static bool check_expression_rvalue( Expression* expression, Type* inferred_type )
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
    Symbol* original_declaration = symbol_table_lookup( &symbol_table, identifier_token.identifier );
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

    Type declared_type = expression->variable_declaration.type;
    Type inferred_type;
    Expression* value = expression->variable_declaration.value;
    if( value != NULL )
    {
        // infer type from value
        bool is_valid = check_expression_rvalue( value, &inferred_type );
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
            return false;
        }
    }

    expression->variable_declaration.type = declared_type;

    // add to symbol table
    Symbol symbol = {
        .token = identifier_token,
        .type = declared_type,
    };
    symbol_table_add( &symbol_table, symbol );

    return true;
}

static bool check_compound( Expression* expression )
{
    bool is_valid = true;

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

    return is_valid;
}

bool check_semantics( Expression* expression )
{
    // initialize symbol_table;
    static bool is_symbol_table_initialized = false;
    if( !is_symbol_table_initialized )
    {
        is_symbol_table_initialized = true;
        symbol_table = ( SymbolTable ){
            .symbols = lvec_new( Symbol ),
            .length = 0,
        };
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

        default:
        {
            UNIMPLEMENTED();
        }
    }

    return is_valid;
}
