#include <stdio.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "error.h"
#include "parser.h"
#include "lvec.h"
#include "tokenizer.h"
#include "semantic.h"

#define MAX(a,b) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

#define TYPEKINDPAIR_TERMINATOR (( TypeKindPair ){ -1, -1 })

typedef struct TypeKindPair
{
    TypeKind tk1;
    TypeKind tk2;
} TypeKindPair;

bool type_equals( Type t1, Type t2 );

void push_return_type( SemanticContext* context, Type type )
{
    lvec_append_aggregate( context->return_type_stack, type );
}

void pop_return_type( SemanticContext* context )
{
    lvec_remove_last( context->return_type_stack );
}

Type get_top_return_type( SemanticContext* context )
{
    int last_index = lvec_get_length( context->return_type_stack ) - 1;
    return context->return_type_stack[ last_index ];
}

void push_scope( SemanticContext* context )
{
    SymbolTable st = {
        .symbols = lvec_new( Symbol ),
        .length = 0,
    };
    lvec_append_aggregate( context->symbol_table_stack, st );
}

void pop_scope( SemanticContext* context )
{
    lvec_remove_last( context->symbol_table_stack );
}

void add_symbol_to_scope( SemanticContext* context, Symbol symbol )
{
    int symbol_table_stack_top_index = lvec_get_length( context->symbol_table_stack ) - 1;
    SymbolTable* current_scope_table = &context->symbol_table_stack[ symbol_table_stack_top_index ];

    lvec_append_aggregate( current_scope_table->symbols, symbol );
    current_scope_table->length++;
}

void semantic_context_initialize( SemanticContext* context )
{
    context->symbol_table_stack = lvec_new( SymbolTable );
    context->return_type_stack = lvec_new( Type );
    context->array_types = lvec_new( Type );
    context->pointer_types = lvec_new( Type );

    Symbol true_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "true",
            .identifier = "true",
        },
        .type = ( Type ){
            .kind = TYPEKIND_BOOLEAN,
        },
    };

    Symbol false_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "false",
            .identifier = "false",
        },
        .type = ( Type ){
            .kind = TYPEKIND_BOOLEAN,
        },
    };

    Symbol i8_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "i8",
            .identifier = "i8",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol i16_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "i16",
            .identifier = "i16",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol i32_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "i32",
            .identifier = "i32",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol i64_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "i64",
            .identifier = "i64",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol u8_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "u8",
            .identifier = "u8",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol u16_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "u16",
            .identifier = "u16",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol u32_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "u32",
            .identifier = "u32",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol u64_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "u64",
            .identifier = "u64",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol f32_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "f32",
            .identifier = "f32",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol f64_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "f64",
            .identifier = "f64",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol bool_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "bool",
            .identifier = "bool",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    Symbol char_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "char",
            .identifier = "char",
        },
        .type = ( Type ){
            .kind = TYPEKIND_PRIMITIVE,
        },
    };

    push_scope( context );

    add_symbol_to_scope( context, true_symbol );
    add_symbol_to_scope( context, false_symbol );
    add_symbol_to_scope( context, i8_symbol );
    add_symbol_to_scope( context, i16_symbol );
    add_symbol_to_scope( context, i32_symbol );
    add_symbol_to_scope( context, i64_symbol );
    add_symbol_to_scope( context, u8_symbol );
    add_symbol_to_scope( context, u16_symbol );
    add_symbol_to_scope( context, u32_symbol );
    add_symbol_to_scope( context, u64_symbol );
    add_symbol_to_scope( context, f32_symbol );
    add_symbol_to_scope( context, f64_symbol );
    add_symbol_to_scope( context, bool_symbol );
    add_symbol_to_scope( context, char_symbol );
}

// searches the entire symbol table stack backwards for identifier
Symbol* symbol_lookup( SemanticContext* context, char* identifier )
{
    int symbol_stack_length = lvec_get_length( context->symbol_table_stack );
    for( int i = 0; i < symbol_stack_length ; i++ )
    {
        SymbolTable current_scope_table = context->symbol_table_stack[ i ];

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

bool is_type_numeric( Type type )
{
    return type.kind == TYPEKIND_INTEGER || type.kind == TYPEKIND_FLOAT;
}

bool type_equals( Type t1, Type t2 )
{
    bool are_kinds_equal = t1.kind == t2.kind;
    if( !are_kinds_equal )
    {
        return false;
    }

    // bool result;
    switch( t1.kind )
    {
        case TYPEKIND_VOID:
        case TYPEKIND_CHARACTER:
        case TYPEKIND_BOOLEAN:
        {
            return true;
        }

        case TYPEKIND_INTEGER:
        {
            bool is_same_size = t1.integer.bit_count == t2.integer.bit_count;
            bool is_same_signedness = t1.integer.is_signed == t2.integer.is_signed;
            return is_same_size && is_same_signedness;
        }

        case TYPEKIND_FLOAT:
        {
            return t1.integer.bit_count == t2.integer.bit_count;
        }

        case TYPEKIND_CUSTOM:
        {
            return strcmp( t1.custom_identifier, t2.custom_identifier ) == 0;
        }

        case TYPEKIND_POINTER:
        {
            return type_equals( *t1.pointer.base_type, *t2.pointer.base_type );
        }

        case TYPEKIND_FUNCTION:
        {
            if( t1.function.param_count != t2.function.param_count )
            {
                return false;
            }

            if( t1.function.return_type != t2.function.return_type )
            {
                return false;
            }

            bool result = true;
            int param_count = t1.function.param_count;
            for( int i = 0; i < param_count; i++ )
            {
                Type t1_param_type = t1.function.param_types[ i ];
                Type t2_param_type = t2.function.param_types[ i ];
                if( !type_equals( t1_param_type, t2_param_type ) )
                {
                    result = false;
                }
            }

            return result;
        }

        case TYPEKIND_ARRAY:
        {
            bool are_lengths_equal = t1.array.length == t2.array.length;
            bool are_base_types_equal = type_equals( *t1.array.base_type, *t2.array.base_type );

            return are_lengths_equal && are_base_types_equal;
        }

        default: // TOINFER and INVALID
        {
            UNREACHABLE();
            break;
        }
    }
}

void add_array_type( SemanticContext* context, Type base_type )
{
    // check if type is already in array
    for( size_t i = 0; i < lvec_get_length( context->array_types ); i++ )
    {
        Type t = context->array_types[ i ];
        if( type_equals( t, base_type ) )
        {
            return;
        }
    }

    lvec_append_aggregate( context->array_types, base_type );
}

void add_pointer_type( SemanticContext* context, Type base_type )
{
    // check if type is already in array
    for( size_t i = 0; i < lvec_get_length( context->pointer_types ); i++ )
    {
        Type t = context->pointer_types[ i ];
        if( type_equals( t, base_type ) )
        {
            return;
        }
    }

    lvec_append_aggregate( context->pointer_types, base_type );
}

static bool try_implicit_cast( Type destination_type, Type* out_type )
{
    // implicit casts are only for numeric and pointer types
    static TypeKind implicitly_castable_type_kinds[] = {
        TYPEKIND_INTEGER, TYPEKIND_FLOAT, TYPEKIND_POINTER
    };
    int implicitly_castable_type_kinds_length =
        sizeof( implicitly_castable_type_kinds ) / sizeof( *implicitly_castable_type_kinds );

    bool destination_type_allowed = false;
    bool out_type_allowed = false;
    for( int i = 0; i < implicitly_castable_type_kinds_length; i++ )
    {
        if( destination_type.kind == implicitly_castable_type_kinds[ i ] )
        {
            destination_type_allowed = true;
        }

        if( out_type->kind == implicitly_castable_type_kinds[ i ] )
        {
            out_type_allowed = true;
        }
    }

    if( !( destination_type_allowed && out_type_allowed ) )
    {
        return false;
    }

    // must be the same type kind!!
    if( destination_type.kind != out_type->kind )
    {
        return false;
    }

    bool result = false;
    switch( destination_type.kind )
    {
        case TYPEKIND_INTEGER:
        {
            bool is_same_signedness = destination_type.integer.is_signed == out_type->integer.is_signed;
            size_t destination_bit_count = destination_type.integer.bit_count;
            size_t original_bit_count = out_type->integer.bit_count;
            result = is_same_signedness && ( destination_bit_count >= original_bit_count );

            break;
        }

        case TYPEKIND_FLOAT:
        {
            size_t destination_bit_count = destination_type.floating.bit_count;
            size_t original_bit_count = out_type->floating.bit_count;
            result = destination_bit_count >= original_bit_count;

            break;
        }

        case TYPEKIND_POINTER:
        {
            // at least one of the ptrs have to point at void
            // implicit cast from non-&void to non-&void is not allowed
            // non-&void to &void is allowed
            // &void to non-&void is allowed

            bool is_destination_void_ptr = destination_type.pointer.base_type->kind == TYPEKIND_VOID;
            bool is_original_void_ptr = out_type->pointer.base_type->kind == TYPEKIND_VOID;
            if( ( is_destination_void_ptr + is_original_void_ptr ) > 0 )
            {
                result = true;
            }

            break;
        }

        /* case TYPEKIND_ARRAY: */
        /* { */
        /*     bool is_destination_length_inferred = destination_type.array.length == -1; */
        /*     bool are_base_types_equal = type_equals( *destination_type.array.base_type, *out_type->array.base_type ); */
        /*     result = is_destination_length_inferred && are_base_types_equal; */
        /*     break; */
        /* } */

        default:
        {
            UNREACHABLE();
            break;
        }
    }

    if( result )
    {
        *out_type = destination_type;
    }

    return result;
}

bool check_type( SemanticContext* context, Type type )
{
    switch( type.kind )
    {
        case TYPEKIND_INTEGER:
        case TYPEKIND_FLOAT:
        case TYPEKIND_CHARACTER:
        case TYPEKIND_BOOLEAN:
        {
            return true;
        }

        case TYPEKIND_VOID:
        {
            Error error = {
                .kind = ERRORKIND_VOIDVARIABLE,
                .offending_token = type.token,
            };
            report_error( error );
            return false;
        }

        case TYPEKIND_FUNCTION:
        {
            bool is_valid = true;

            // check params
            for( int i = 0; i < type.function.param_count; i++ )
            {
                Type t = type.function.param_types[ i ];
                is_valid = is_valid && check_type( context, t );
            }

            // void is allowed
            if( type.function.return_type->kind != TYPEKIND_VOID )
            {
                is_valid = is_valid && check_type( context, *type.function.return_type );
            }
            return is_valid;
        }

        case TYPEKIND_POINTER:
        {
            // lvec_append_aggregate( context->pointer_types, *type.pointer.base_type );
            add_pointer_type( context, *type.pointer.base_type );
            return check_type( context, *type.pointer.base_type );
        }

        case TYPEKIND_ARRAY:
        {
            if( type.array.length == 0 )
            {
                Error error = {
                    .kind = ERRORKIND_ZEROLENGTHARRAY,
                    .offending_token = type.token,
                };
                report_error( error );
                return false;
            }

            // lvec_append_aggregate( context->array_types, *type.array.base_type );
            add_array_type( context, *type.array.base_type );
            return check_type( context, *type.array.base_type );
        }

        case TYPEKIND_CUSTOM:
        {
            Symbol* symbol = symbol_lookup( context, type.custom_identifier );
            if( symbol == NULL )
            {
                Error error = {
                    .kind = ERRORKIND_UNDECLAREDSYMBOL,
                    .offending_token = type.token,
                };
                report_error( error );
                return false;
            }
            return true;
        }

        default: // TOINFER and INVALID
        {
            UNREACHABLE();
            break;
        }
    }
}

bool check_type_compatibility( Type t1, Type* out_type, Error* out_error )
{
    Error error;
    bool result = true;

    if( type_equals( t1, *out_type ) )
    {
        return true;
    }

    bool implicit_cast_attempt = try_implicit_cast( t1, out_type );
    if( !implicit_cast_attempt )
    {
        bool are_both_types_numeric = is_type_numeric( t1 ) && is_type_numeric( *out_type );
        bool are_both_types_arrays = t1.kind == TYPEKIND_ARRAY && out_type->kind == TYPEKIND_ARRAY;
        if( are_both_types_numeric )
        {
            error = ( Error ){
                .kind = ERRORKIND_INVALIDIMPLICITCAST,
                .invalid_implicit_cast = {
                    .to = t1,
                    .from = *out_type,
                },
            };
            result = false;
        }
        else if( are_both_types_arrays )
        {
            bool is_destination_length_inferred = t1.array.length == -1;
            if( !is_destination_length_inferred )
            {
                error = ( Error ){
                    .kind = ERRORKING_ARRAYLENGTHMISMATCH,
                    .array_length_mismatch = {
                        .expected = t1.array.length,
                        .found = out_type->array.length
                    }
                };
                result = false;
            }

            bool are_base_types_equal = type_equals( *t1.array.base_type, *out_type->array.base_type );
            // result = is_destination_length_inferred && are_base_types_equal;
            if( !are_base_types_equal )
            {
                error = ( Error ){
                    .kind = ERRORKIND_TYPEMISMATCH,
                    .type_mismatch = {
                        .expected = t1,
                        .found = *out_type
                    }
                };
                result = false;
            }
        }
        else
        {
            error = ( Error ){
                .kind = ERRORKIND_TYPEMISMATCH,
                .type_mismatch = {
                    .expected = t1,
                    .found = *out_type,
                },
            };
            result = false;
        }
    }

    if( !result )
    {
        *out_error = error;
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
            ( TypeKindPair ){ TYPEKIND_INTEGER,   TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,     TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,     TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_BOOLEAN,   TYPEKIND_BOOLEAN },
            // ( TypeKindPair ){ TYPEKIND_STRING,    TYPEKIND_STRING },
            ( TypeKindPair ){ TYPEKIND_CHARACTER, TYPEKIND_CHARACTER },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_NOTEQUAL ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER,   TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_FLOAT,     TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_INTEGER,   TYPEKIND_FLOAT },
            ( TypeKindPair ){ TYPEKIND_FLOAT,     TYPEKIND_INTEGER },
            ( TypeKindPair ){ TYPEKIND_BOOLEAN,   TYPEKIND_BOOLEAN },
            // ( TypeKindPair ){ TYPEKIND_STRING,    TYPEKIND_STRING },
            ( TypeKindPair ){ TYPEKIND_CHARACTER, TYPEKIND_CHARACTER },
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

    // iterate over all valid pairs
    bool is_valid_pair = false;
    for( TypeKindPair pair = *pairs; pair.tk1 != -1; pairs++, pair = *pairs )
    {
        TypeKind pair_left = pair.tk1;
        TypeKind pair_right = pair.tk2;

        if( pair_left == left_type.kind && pair_right == right_type.kind )
        {
            is_valid_pair = true;
            break;
        }
    }

    if( !is_valid_pair )
    {
        return false;
    }

    // not allowed for numeric types
    // signed and unsigned ints
    // int and float
    switch( left_type.kind * right_type.kind )
    {
        case TYPEKIND_INTEGER * TYPEKIND_INTEGER:
        {
            if( left_type.integer.is_signed != right_type.integer.is_signed )
            {
                return false;
            }
            break;
        }

        case TYPEKIND_FLOAT * TYPEKIND_INTEGER:
        {
            return false;
            break;
        }
    }

    return true;
}

static bool check_rvalue( SemanticContext* context, Expression* expression, Type* inferred_type );
static bool check_binary( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Expression* left_expression = expression->binary.left;
    Expression* right_expression = expression->binary.right;

    Type left_type;
    Type right_type;

    bool is_left_valid = check_rvalue( context, left_expression, &left_type );
    bool is_right_valid = check_rvalue( context, right_expression, &right_type );

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
            inferred_type->floating.bit_count = MAX( left_type.integer.bit_count, right_type.integer.bit_count );
        }
        else
        {
            inferred_type->kind = TYPEKIND_INTEGER;
            inferred_type->integer.is_signed = right_type.integer.is_signed;
            inferred_type->integer.bit_count = fmax( left_type.integer.bit_count, right_type.integer.bit_count );
        }
    }

    return true;
}

static bool check_rvalue_identifier( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Token identifier_token = expression->associated_token;

    // check if identifier already in symbol table
    Symbol* original_declaration = symbol_lookup( context, identifier_token.identifier );
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

static bool check_function_call( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Token identifier_token = expression->function_call.identifier_token;
    Symbol* original_declaration = symbol_lookup( context, identifier_token.identifier );
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
    int param_count = original_declaration->type.function.param_count;
    int arg_count = expression->function_call.arg_count;
    bool is_variadic = original_declaration->type.function.is_variadic;

    bool is_argument_count_valid;
    if( is_variadic )
    {
        is_argument_count_valid = arg_count >= param_count;
    }
    else
    {
        is_argument_count_valid = arg_count == param_count;
    }

    if( !is_argument_count_valid )
    {
        Error error = {
            .kind = ERRORKIND_INVALIDARGUMENTCOUNT,
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

        bool is_arg_valid = check_rvalue( context, arg, &arg_type );
        if( !is_arg_valid )
        {
            // no need to report error here because that is handled by check_rvalue
            return false;
        }

        Error error;
        bool are_types_compatible = check_type_compatibility( param_type, &arg_type, &error );
        if( !are_types_compatible )
        {
            error.offending_token = arg->starting_token;
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
    Type void_pointer = {
        .kind = TYPEKIND_POINTER,
        .pointer.base_type = &( Type ){
            .kind = TYPEKIND_VOID,
        },
    };

    if( operation == UNARYOPERATION_DEREFERENCE && type_equals( type, void_pointer ) )
    {
        return false;
    }

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

static bool check_lvalue( SemanticContext* context, Expression* expression, Type* out_type );
static bool check_unary( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Expression* operand = expression->unary.operand;
    Type operand_type;
    bool is_operand_valid = check_rvalue( context, operand, &operand_type );
    if( !is_operand_valid )
    {
        return false;
    }

    UnaryOperation operation = expression->unary.operation;
    if( operation == UNARYOPERATION_ADDRESSOF )
    {

        // operand must be lvalue
        bool operand_lvalue_check = check_lvalue( context, operand, &operand_type );
        if( !operand_lvalue_check )
        {
            Error error = {
                .kind = ERRORKIND_INVALIDADDRESSOF,
                .offending_token = operand->starting_token
            };

            report_error( error );
            return false;
        }

        // check if operand is in symbol table
        Symbol* operand_symbol = symbol_lookup( context, operand->associated_token.identifier );
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
            .pointer.base_type = &operand_symbol->type,
        };
        add_pointer_type( context, *inferred_type->pointer.base_type );
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

        if( operation == UNARYOPERATION_DEREFERENCE )
        {
            *inferred_type = *operand_type.pointer.base_type;
        }
        else
        {
            *inferred_type = operand_type;
        }
    }

    return true;
}

static bool check_array( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Type declared_type = expression->array.type;

    if( !check_type( context, declared_type ) )
    {
        return false;
    }

    int found_length = declared_type.array.length;
    int count_initialized = expression->array.count_initialized;

    // declared_length being -1 means it is to be inferred
    if( found_length == -1 && count_initialized == 0 )
    {
        Error error = {
            .kind = ERRORKIND_ZEROLENGTHARRAY,
            .offending_token = expression->starting_token,
        };
        report_error( error );
        return false;
    }

    if( found_length == -1 )
    {
        found_length = count_initialized;
    }
    else if( found_length < count_initialized )
    {
        Error error = {
            .kind = ERRORKING_ARRAYLENGTHMISMATCH,
            .offending_token = expression->starting_token,
            .array_length_mismatch = {
                .expected = found_length,
                .found = count_initialized,
            },
        };
        report_error( error );
        return false;
    }

    bool are_initializers_valid = true;
    for( int i = 0; i < count_initialized; i++ )
    {
        Type element_type;
        Expression* element_rvalue = &expression->array.initialized_rvalues[ i ];
        if( !check_rvalue( context, element_rvalue, &element_type ) )
        {
            are_initializers_valid = false;
        }

        if( !type_equals( element_type, *declared_type.array.base_type ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = element_rvalue->starting_token,
                .type_mismatch = {
                    .expected = *declared_type.array.base_type,
                    .found = element_type,
                },
            };
            report_error( error );
            are_initializers_valid = false;
        }
    }

    if( !are_initializers_valid )
    {
        return false;
    }

    inferred_type->kind = TYPEKIND_ARRAY;
    inferred_type->array.length = found_length;
    inferred_type->array.base_type = declared_type.array.base_type;
    add_array_type( context, *inferred_type->array.base_type );

    // for AST printing in debug.c
    expression->array.type = *inferred_type;

    return true;
}

static bool check_array_subscript( SemanticContext* context, Expression* expression, Type* out_type )
{
    Token identifier_token = expression->array_subscript.identifier_token;
    Expression* index_rvalue = expression->array_subscript.index_rvalue;

    // check identifier
    Symbol* identifier_symbol = symbol_lookup( context, identifier_token.identifier );
    if( identifier_symbol == NULL )
    {
        Error error = {
            .kind = ERRORKIND_UNDECLAREDSYMBOL,
            .offending_token = identifier_token,
        };

        report_error( error );
        return false;
    }

    Type index_rvalue_type;
    if( !check_rvalue( context, index_rvalue, &index_rvalue_type ) )
    {
        return false;
    }

    if( index_rvalue_type.kind != TYPEKIND_INTEGER )
    {
        Error error = {
            .kind = ERRORKIND_INVALIDARRAYSUBSCRIPT,
            .offending_token = index_rvalue->starting_token,
        };
        report_error( error );
        return false;
    }

    *out_type = *identifier_symbol->type.array.base_type;
    return true;
}

static bool check_rvalue( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    // initialized to true for the base cases
    bool is_valid = true;

    switch( expression->kind )
    {
        // case EXPRESSIONKIND_STRING:    inferred_type->kind = TYPEKIND_STRING;    break;
        case EXPRESSIONKIND_CHARACTER: inferred_type->kind = TYPEKIND_CHARACTER; break;
        case EXPRESSIONKIND_BOOLEAN:   inferred_type->kind = TYPEKIND_BOOLEAN; break;
        case EXPRESSIONKIND_STRING:
        {
            inferred_type->kind = TYPEKIND_POINTER;

            // I HATE HOW I HAVE TO ALLOCATE HERE!!!!!!
            inferred_type->pointer.base_type = malloc( sizeof( Type ) );
            inferred_type->pointer.base_type->kind = TYPEKIND_CHARACTER;
            break;
        }

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
            is_valid = check_rvalue_identifier( context, expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_BINARY:
        {
            is_valid = check_binary( context, expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            is_valid = check_unary( context, expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            is_valid = check_function_call( context, expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_ARRAY:
        {
            is_valid = check_array( context, expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_ARRAYSUBSCRIPT:
        {
            is_valid = check_array_subscript( context, expression, inferred_type );
            break;
        }

        default:
        {
            // not rvalue expressions
            UNREACHABLE();
            break;
        }
    }

    return is_valid;
}

static bool check_variable_declaration( SemanticContext* context, Expression* expression )
{
    Token identifier_token = expression->variable_declaration.identifier_token;

    // check if identifier already in symbol table
    Symbol* original_declaration = symbol_lookup( context, identifier_token.identifier );
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

    Type found_type = expression->variable_declaration.type;
    if( found_type.kind != TYPEKIND_TOINFER && !check_type( context, found_type ) )
    {
        return false;
    }

    // check if the type at the left hand side matches the type on the right hand
    // side, or if there is an implicit cast possible.
    Type inferred_type;
    Expression* rvalue = expression->variable_declaration.rvalue;
    if( rvalue != NULL )
    {
        // infer type from value
        bool is_valid = check_rvalue( context, rvalue, &inferred_type );
        if( !is_valid )
        {
            return false;
        }

        if( inferred_type.kind == TYPEKIND_VOID )
        {
            Error error = {
                .kind = ERRORKIND_VOIDVARIABLE,
                .offending_token = expression->variable_declaration.rvalue->starting_token,
            };
            report_error( error );
            return false;
        }

        if( found_type.kind == TYPEKIND_TOINFER )
        {
            found_type = inferred_type;
        }
        else if( !check_type( context, found_type ) )
        {
            return false;
        }

        Error error;
        bool are_types_compatible = check_type_compatibility( found_type, &inferred_type, &error );
        if( !are_types_compatible )
        {
            error.offending_token = expression->variable_declaration.rvalue->starting_token;
            report_error( error );
            return false;
        }

        if( found_type.kind == TYPEKIND_ARRAY )
        {
            found_type = inferred_type;
        }
    }
    else if( found_type.kind == TYPEKIND_ARRAY && found_type.array.length == -1 )
    {
        Error error = {
            .kind = ERRORKIND_CANNOTINFERARRAYLENGTH,
            .offending_token = expression->variable_declaration.type.token,
        };
        report_error( error );
        return false;
    }

    expression->variable_declaration.type = found_type;

    // add to symbol table
    Symbol symbol = {
        .token = identifier_token,
        .type = found_type,
    };
    add_symbol_to_scope( context, symbol );

    return true;
}

static bool check_compound( SemanticContext* context, Expression* expression )
{
    bool is_valid = true;

    push_scope( context );

    size_t length = lvec_get_length( expression->compound.expressions );
    for( size_t i = 0; i < length; i++ )
    {
        Expression* e = expression->compound.expressions[ i ];
        bool _is_valid = check_semantics( context, e );

        if( !_is_valid )
        {
            is_valid = false;
        }
    }

    pop_scope( context );

    return is_valid;
}

static bool check_function_declaration( SemanticContext* context,Expression* expression, bool is_extern )
{
    Token identifier_token = expression->function_declaration.identifier_token;

    // check if identifier already in symbol table
    Symbol* original_declaration = symbol_lookup( context, identifier_token.identifier );
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
    int param_count = expression->function_declaration.param_count;
    bool is_variadic = expression->function_declaration.is_variadic;

    // add to symbol table
    Symbol symbol = {
        .token = identifier_token,
        .type = ( Type ){
            .kind = TYPEKIND_FUNCTION,
            .function = {
                .param_types = param_types,
                .return_type = return_type,
                .param_count = param_count,
                .is_variadic = is_variadic,
            }
        },
    };
    add_symbol_to_scope( context, symbol );

    // check function params
    push_scope( context );
    for( int i = 0; i < expression->function_declaration.param_count; i++ )
    {
        Token param_identifier_token = expression->function_declaration.param_identifiers_tokens[ i ];
        Type param_type = expression->function_declaration.param_types[ i ];
        // Token param_type_token = expression->function_declaration.param_types_tokens[ i ];
        if( !check_type( context, param_type ) )
        {
            return false;
        }

        Symbol* declaration = symbol_lookup( context, param_identifier_token.identifier );
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
        add_symbol_to_scope( context, symbol );
    }

    push_return_type( context, *return_type );

    // check function body
    Expression* function_body = expression->function_declaration.body;
    if( function_body == NULL && !is_extern )
    {
        Error error = {
            .kind = ERRORKIND_MISSINGFUNCTIONBODY,
            .offending_token = expression->starting_token,
        };
        report_error( error );
        return false;
    }
    else if( function_body != NULL && is_extern )
    {
        Error error = {
            .kind = ERRORKIND_EXTERNWITHBODY,
            .offending_token = expression->starting_token,
        };
        report_error( error );
        return false;
    }

    bool is_body_valid = true;
    if( !is_extern )
    {
        is_body_valid = check_compound( context, function_body );
    }

    pop_scope( context );
    pop_return_type( context );

    if( !is_body_valid )
    {
        return false;
    }

    return true;
}

bool check_return( SemanticContext* context, Expression* expression )
{
    Type found_return_type = ( Type ){ .kind = TYPEKIND_VOID };
    if( expression->return_expression.rvalue != NULL )
    {
        bool is_return_value_valid = check_rvalue( context, expression->return_expression.rvalue, &found_return_type );

        if( !is_return_value_valid )
        {
            // no need to report error here because error has already been reported
            // from the check_expression_rvalue function
            return false;
        }
    }

    Type expected_return_type = get_top_return_type( context );

    Error error;
    bool are_types_compatible = check_type_compatibility( expected_return_type, &found_return_type, &error );
    if( !are_types_compatible )
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

        error.offending_token = associated_token;
        report_error( error );
        return false;
    }

    return true;
}

static bool check_lvalue( SemanticContext* context, Expression* expression, Type* out_type )
{
    bool is_valid;
    switch( expression->kind )
    {
        case EXPRESSIONKIND_IDENTIFIER:
        {
            is_valid = check_rvalue_identifier( context, expression, out_type );
            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            Type lvalue_type;
            bool is_unary_valid = check_unary( context, expression, &lvalue_type );
            if( !is_unary_valid )
            {
                // returning false here because error has already been reported in
                // check_unary
                return false;
            }

            if( expression->unary.operation != UNARYOPERATION_DEREFERENCE )
            {
                is_valid = false;
                break;
            }

            *out_type = lvalue_type;
            is_valid = true;
            break;
        }

        case EXPRESSIONKIND_ARRAYSUBSCRIPT:
        {
            is_valid = check_array_subscript( context, expression, out_type );
            break;
        }

        default:
        {
            is_valid = false;
            break;
        }
    }

    return is_valid;
}

bool check_assignment( SemanticContext* context, Expression* expression )
{
    // check if lvalue is a valid lvalue
    Type found_lvalue_type;
    bool is_lvalue_valid = check_lvalue( context, expression->assignment.lvalue, &found_lvalue_type );
    if( !is_lvalue_valid )
    {
        Error error = {
            .kind = ERRORKIND_INVALIDLVALUE,
            .offending_token = expression->starting_token,
        };
        report_error( error );
        return false;
    }

    // check if rvalue is a valid rvalue
    Type found_rvalue_type;
    bool is_rvalue_valid = check_rvalue( context, expression->assignment.rvalue, &found_rvalue_type );
    if( !is_rvalue_valid )
    {
        // no need for error reporting here because that was already handled by
        // check_rvalue
        return false;
    }

    // check if types match with original declaration
    Type expected_type = found_lvalue_type;
    Error error;
    bool are_types_compatible = check_type_compatibility( expected_type, &found_rvalue_type, &error );
    if( !are_types_compatible )
    {
        error.offending_token = expression->assignment.rvalue->starting_token;
        report_error( error );
        return false;
    }

    return true;
}

static bool check_conditional( SemanticContext* context, Expression* expression )
{
    // check the condition (must evaluate to bool type)
    Type condition_type;
    bool condition_is_valid = check_rvalue( context, expression->conditional.condition, &condition_type );
    if( !condition_is_valid )
    {
        return false;
    }

    if( condition_type.kind != TYPEKIND_BOOLEAN )
    {
        Type expected_type = { .kind = TYPEKIND_BOOLEAN };
        Error error = {
            .kind = ERRORKIND_TYPEMISMATCH,
            .offending_token = expression->conditional.condition->starting_token,
            .type_mismatch = {
                .expected = expected_type,
                .found = condition_type
            },
        };
        report_error( error );
        return false;
    }

    // while loops must NOT have an else
    Expression* false_body = expression->conditional.false_body;
    if( expression->conditional.is_loop && false_body != NULL )
    {
        Error error = {
            .kind = ERRORKIND_WHILEWITHELSE,
            .offending_token = expression->starting_token,
        };
        report_error( error );
        return false;
    }

    // check true and false bodies (if applicable)
    if( !check_semantics( context, expression->conditional.true_body ) )
    {
        return false;
    }
    if( false_body != NULL && !check_semantics( context, false_body ) )
    {
        return false;
    }

    return true;
}

bool check_semantics( SemanticContext* context, Expression* expression )
{
    bool is_valid;

    switch( expression->kind )
    {
        case EXPRESSIONKIND_VARIABLEDECLARATION:
        {
            is_valid = check_variable_declaration( context, expression );
            break;
        }

        case EXPRESSIONKIND_COMPOUND:
        {
            is_valid = check_compound( context, expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONDECLARATION:
        {
            is_valid = check_function_declaration( context, expression, false );
            break;
        }

        case EXPRESSIONKIND_RETURN:
        {
            is_valid = check_return( context, expression );
            break;
        }

        case EXPRESSIONKIND_ASSIGNMENT:
        {
            is_valid = check_assignment( context, expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONCALL:
        {
            is_valid = check_function_call( context, expression, NULL );
            break;
        }

        case EXPRESSIONKIND_EXTERN:
        {
            is_valid = check_function_declaration( context, expression->extern_expression.function, true );
            break;
        }

        case EXPRESSIONKIND_CONDITIONAL:
        {
            is_valid = check_conditional( context, expression );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }
    return is_valid;
}
