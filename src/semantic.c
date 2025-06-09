#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "error.h"
#include "parser.h"
#include "lvec.h"
#include "symboltable.h"
#include "tokenizer.h"
#include "type.h"
#include "semantic.h"

#define MAX(a,b) ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )

#define TYPEKINDPAIR_TERMINATOR (( TypeKindPair ){ -1, -1 })

typedef struct TypeKindPair
{
    TypeKind tk1;
    TypeKind tk2;
} TypeKindPair;

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

void semantic_context_initialize( SemanticContext* context )
{
    symbol_table_initialize( &context->symbol_table );
    context->return_type_stack = lvec_new( Type );

    Type* void_definition = malloc( sizeof( Type ) );
    *void_definition = ( Type ){
        .kind = TYPEKIND_VOID,
    };

    Type* void_info = malloc( sizeof( Type ) );
    *void_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "void",
            .definition = void_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol void_symbol = {
        .token = { .as_string = "void" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = void_info,
        }
    };

    Type* char_definition = malloc( sizeof( Type ) );
    *char_definition = ( Type ){
        .kind = TYPEKIND_CHARACTER,
    };

    Type* char_info = malloc( sizeof( Type ) );
    *char_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "char",
            .definition = char_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol char_symbol = {
        .token = { .as_string = "char" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = char_info,
        }
    };

    Type* bool_definition = malloc( sizeof( Type ) );
    *bool_definition = ( Type ){
        .kind = TYPEKIND_BOOLEAN,
    };

    Type* bool_info = malloc( sizeof( Type ) );
    *bool_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "bool",
            .definition = bool_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol bool_symbol = {
        .token = { .as_string = "bool" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = bool_info,
        }
    };

    Symbol true_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "true",
            .identifier = "true",
        },
        .type = ( Type ){
            .kind = TYPEKIND_TYPE,
            .type.info = bool_info,
        },
    };

    Symbol false_symbol = {
        .token = ( Token ){
            .kind = TOKENKIND_IDENTIFIER,
            .as_string = "false",
            .identifier = "false",
        },
        .type = ( Type ){
            .kind = TYPEKIND_TYPE,
            .type.info = bool_info,
        },
    };

    Type* i8_definition = malloc( sizeof( Type ) );
    *i8_definition = ( Type ){
        .kind = TYPEKIND_INTEGER,
        .integer = {
            .bit_count = 8,
            .is_signed = true,
        }
    };

    Type* i8_info = malloc( sizeof( Type ) );
    *i8_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "i8",
            .definition = i8_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol i8_symbol = {
        .token = { .as_string = "i8" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = i8_info,
        }
    };

    Type* i16_definition = malloc( sizeof( Type ) );
    *i16_definition = ( Type ){
        .kind = TYPEKIND_INTEGER,
        .integer = {
            .bit_count = 16,
            .is_signed = true,
        }
    };

    Type* i16_info = malloc( sizeof( Type ) );
    *i16_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "i16",
            .definition = i16_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol i16_symbol = {
        .token = { .as_string = "i16" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = i16_info,
        }
    };

    Type* i32_definition = malloc( sizeof( Type ) );
    *i32_definition = ( Type ){
        .kind = TYPEKIND_INTEGER,
        .integer = {
            .bit_count = 32,
            .is_signed = true,
        }
    };

    Type* i32_info = malloc( sizeof( Type ) );
    *i32_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "i32",
            .definition = i32_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol i32_symbol = {
        .token = { .as_string = "i32" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = i32_info,
        }
    };

    Type* i64_definition = malloc( sizeof( Type ) );
    *i64_definition = ( Type ){
        .kind = TYPEKIND_INTEGER,
        .integer = {
            .bit_count = 64,
            .is_signed = true,
        }
    };

    Type* i64_info = malloc( sizeof( Type ) );
    *i64_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "i64",
            .definition = i64_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol i64_symbol = {
        .token = { .as_string = "i64" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = i64_info,
        }
    };

    Type* u8_definition = malloc( sizeof( Type ) );
    *u8_definition = ( Type ){
        .kind = TYPEKIND_INTEGER,
        .integer = {
            .bit_count = 8,
            .is_signed = false,
        }
    };

    Type* u8_info = malloc( sizeof( Type ) );
    *u8_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "u8",
            .definition = u8_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol u8_symbol = {
        .token = { .as_string = "u8" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = u8_info,
        }
    };

    Type* u16_definition = malloc( sizeof( Type ) );
    *u16_definition = ( Type ){
        .kind = TYPEKIND_INTEGER,
        .integer = {
            .bit_count = 16,
            .is_signed = false,
        }
    };

    Type* u16_info = malloc( sizeof( Type ) );
    *u16_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "u16",
            .definition = u16_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol u16_symbol = {
        .token = { .as_string = "u16" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = u16_info,
        }
    };

    Type* u32_definition = malloc( sizeof( Type ) );
    *u32_definition = ( Type ){
        .kind = TYPEKIND_INTEGER,
        .integer = {
            .bit_count = 32,
            .is_signed = false,
        }
    };

    Type* u32_info = malloc( sizeof( Type ) );
    *u32_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "u32",
            .definition = u32_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol u32_symbol = {
        .token = { .as_string = "u32" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = u32_info,
        }
    };

    Type* u64_definition = malloc( sizeof( Type ) );
    *u64_definition = ( Type ){
        .kind = TYPEKIND_INTEGER,
        .integer = {
            .bit_count = 64,
            .is_signed = false,
        }
    };

    Type* u64_info = malloc( sizeof( Type ) );
    *u64_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "u64",
            .definition = u64_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol u64_symbol = {
        .token = { .as_string = "u64" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = u64_info,
        }
    };

    Type* f32_definition = malloc( sizeof( Type ) );
    *f32_definition = ( Type ){
        .kind = TYPEKIND_FLOAT,
        .floating.bit_count = 32,
    };

    Type* f32_info = malloc( sizeof( Type ) );
    *f32_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "f32",
            .definition = f32_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol f32_symbol = {
        .token = { .as_string = "f32" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = f32_info,
        }
    };

    Type* f64_definition = malloc( sizeof( Type ) );
    *f64_definition = ( Type ){
        .kind = TYPEKIND_FLOAT,
        .floating.bit_count = 64,
    };

    Type* f64_info = malloc( sizeof( Type ) );
    *f64_info = ( Type ){
        .kind = TYPEKIND_NAMED,
        .named = {
            .as_string = "f64",
            .definition = f64_definition,
            .pointer_types = lvec_new( Type ),
            .array_types = lvec_new( Type ),
        }
    };

    Symbol f64_symbol = {
        .token = { .as_string = "f64" },
        .type = {
            .kind = TYPEKIND_TYPE,
            .type.info = f64_info,
        }
    };

    symbol_table_push_symbol( &context->symbol_table, void_symbol );
    symbol_table_push_symbol( &context->symbol_table, i8_symbol );
    symbol_table_push_symbol( &context->symbol_table, i16_symbol );
    symbol_table_push_symbol( &context->symbol_table, i32_symbol );
    symbol_table_push_symbol( &context->symbol_table, i64_symbol );
    symbol_table_push_symbol( &context->symbol_table, u8_symbol );
    symbol_table_push_symbol( &context->symbol_table, u16_symbol );
    symbol_table_push_symbol( &context->symbol_table, u32_symbol );
    symbol_table_push_symbol( &context->symbol_table, u64_symbol );
    symbol_table_push_symbol( &context->symbol_table, f32_symbol );
    symbol_table_push_symbol( &context->symbol_table, f64_symbol );
    symbol_table_push_symbol( &context->symbol_table, bool_symbol );
    symbol_table_push_symbol( &context->symbol_table, char_symbol );
    symbol_table_push_symbol( &context->symbol_table, true_symbol );
    symbol_table_push_symbol( &context->symbol_table, false_symbol );
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

        case TYPEKIND_COMPOUND:
        {
            UNIMPLEMENTED();
            // return strcmp( t1.compound.identifier, t2.compound.identifier ) == 0;
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

        case TYPEKIND_TYPE:
        {
            return type_equals( *t1.type.info, *t2.type.info );
        }

        case TYPEKIND_NAMED:
        {
            // printf("%s, %s\n", t1.named.as_string, t2.named.as_string);
            // return type_equals( *t1.named.definition, *t2.named.definition );
            return strcmp( t1.named.as_string, t2.named.as_string ) == 0;
        }

        default: // TOINFER and INVALID
        {
            UNREACHABLE();
            break;
        }
    }
}

/* Type get_definition_type( SemanticContext* context, Type type ) */
/* { */
/*     switch( type.kind ) */
/*     { */
/*         case TYPEKIND_VOID: */
/*         case TYPEKIND_INTEGER: */
/*         case TYPEKIND_FLOAT: */
/*         case TYPEKIND_CHARACTER: */
/*         case TYPEKIND_BOOLEAN: */
/*         case TYPEKIND_FUNCTION: */
/*         case TYPEKIND_COMPOUND: */
/*         { */
/*             Symbol* symbol = symbol_table_lookup( context->symbol_table, type.token.as_string ); */
/*             return symbol->type; */
/*         } */

/*         case TYPEKIND_POINTER: */
/*         { */
/*             return get_definition_type( context, *type.pointer.base_type ); */
/*         } */

/*         case TYPEKIND_REFERENCE: */
/*         { */
/*             return get_definition_type( context, *type.reference.base_type ); */
/*         } */

/*         case TYPEKIND_ARRAY: */
/*         { */
/*             return get_definition_type( context, *type.array.base_type ); */
/*         } */

/*         case TYPEKIND_TYPE: */
/*         { */
/*             return get_definition_type( context, *type.type.info ); */
/*         } */

/*         default: // invalid and toinfer */
/*         { */
/*             UNREACHABLE(); */
/*         } */
/*     } */
/* } */

void add_array_type( Type base_type )
{
    // get named type
    Type named_type = base_type;
    while( named_type.kind != TYPEKIND_NAMED )
    {
        switch( named_type.kind )
        {
            case TYPEKIND_POINTER:
            {
                named_type = *named_type.pointer.base_type;
                break;
            }

            case TYPEKIND_REFERENCE:
            {
                named_type = *named_type.reference.base_type;
                break;
            }

            case TYPEKIND_ARRAY:
            {
                named_type = *named_type.array.base_type;
                break;
            }

            case TYPEKIND_TYPE:
            {
                named_type = *named_type.type.info;
                break;
            }

            default:
            {
                UNREACHABLE();
            }
        }
    }

    Type* array_types = named_type.named.array_types;

    // check if type is already in array
    for( size_t i = 0; i < lvec_get_length( array_types ); i++ )
    {
        Type t = array_types[ i ];
        if( type_equals( t, base_type ) )
        {
            return;
        }
    }

    lvec_append_aggregate( named_type.named.array_types, base_type );
}

void add_pointer_type( Type base_type )
{
    // get named type
    Type named_type = base_type;
    while( named_type.kind != TYPEKIND_NAMED )
    {
        switch( named_type.kind )
        {
            case TYPEKIND_POINTER:
            {
                named_type = *named_type.pointer.base_type;
                break;
            }

            case TYPEKIND_REFERENCE:
            {
                named_type = *named_type.reference.base_type;
                break;
            }

            case TYPEKIND_ARRAY:
            {
                named_type = *named_type.array.base_type;
                break;
            }

            case TYPEKIND_TYPE:
            {
                named_type = *named_type.type.info;
                break;
            }

            default:
            {
                UNREACHABLE();
            }
        }
    }

    Type* pointer_types = named_type.named.pointer_types;

    // check if type is already in array
    for( size_t i = 0; i < lvec_get_length( pointer_types ); i++ )
    {
        Type t = pointer_types[ i ];
        if( type_equals( t, base_type ) )
        {
            return;
        }
    }

    lvec_append_aggregate( named_type.named.pointer_types, base_type );
}

static bool try_implicit_cast( Type destination_type, Type* out_type )
{
    // implicit casts are only for numeric and pointer types
    static TypeKind implicitly_castable_type_kinds[] = {
        TYPEKIND_INTEGER, TYPEKIND_FLOAT, TYPEKIND_POINTER, TYPEKIND_REFERENCE
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

/* bool check_type( SemanticContext* context, Type* out_type ) */
/* { */
/*     UNIMPLEMENTED(); */

/*     switch( out_type->kind ) */
/*     { */
/*         case TYPEKIND_VOID: */
/*         case TYPEKIND_INTEGER: */
/*         case TYPEKIND_FLOAT: */
/*         case TYPEKIND_CHARACTER: */
/*         case TYPEKIND_BOOLEAN: */
/*         { */
/*             return true; */
/*         } */

/*         case TYPEKIND_FUNCTION: */
/*         { */
/*             bool is_valid = true; */

/*             // check params */
/*             for( int i = 0; i < out_type->function.param_count; i++ ) */
/*             { */
/*                 Type* t = &out_type->function.param_types[ i ]; */
/*                 is_valid = is_valid && check_type( context, t ); */
/*             } */

/*             // void is allowed */
/*             if( out_type->function.return_type->kind != TYPEKIND_VOID ) */
/*             { */
/*                 is_valid = is_valid && check_type( context, out_type->function.return_type ); */
/*             } */
/*             return is_valid; */
/*         } */

/*         case TYPEKIND_POINTER: */
/*         { */
/*             if( !check_type( context, out_type->pointer.base_type ) ) */
/*             { */
/*                 return false; */
/*             } */

/*             Type definition_type = get_definition_type( context, *out_type ); */
/*             add_pointer_type( definition_type, *out_type->pointer.base_type ); */
/*             return check_type( context, out_type->pointer.base_type ); */
/*         } */

/*         case TYPEKIND_REFERENCE: */
/*         { */
/*             return check_type( context, out_type->reference.base_type ); */
/*         } */

/*         case TYPEKIND_ARRAY: */
/*         { */
/*             if( out_type->array.length == 0 ) */
/*             { */
/*                 Error error = { */
/*                     .kind = ERRORKIND_ZEROLENGTHARRAY, */
/*                     .offending_token = out_type->token, */
/*                 }; */
/*                 report_error( error ); */
/*                 return false; */
/*             } */

/*             if( !check_type( context, out_type->array.base_type ) ) */
/*             { */
/*                 return false; */
/*             } */

/*             Type definition_type = get_definition_type( context, *out_type ); */
/*             if( !check_type( context, &definition_type ) ) */
/*             { */
/*                 return false; */
/*             } */

/*             add_array_type( definition_type, *out_type->array.base_type ); */
/*             return check_type( context, out_type->array.base_type ); */
/*         } */

/*         case TYPEKIND_COMPOUND: */
/*         { */
/*             UNIMPLEMENTED(); */
/*             /\* Symbol* symbol = symbol_table_lookup( context->symbol_table, out_type->compound.identifier ); *\/ */
/*             /\* if( symbol == NULL ) *\/ */
/*             /\* { *\/ */
/*             /\*     Error error = { *\/ */
/*             /\*         .kind = ERRORKIND_UNDECLAREDSYMBOL, *\/ */
/*             /\*         .offending_token = out_type->token, *\/ */
/*             /\*     }; *\/ */
/*             /\*     report_error( error ); *\/ */
/*             /\*     return false; *\/ */
/*             /\* } *\/ */

/*             /\* if( symbol->type.kind != TYPEKIND_DEFINITION ) *\/ */
/*             /\* { *\/ */
/*             /\*     Error error = { *\/ */
/*             /\*         .kind = ERRORKIND_NOTATYPE, *\/ */
/*             /\*         .offending_token = out_type->token, *\/ */
/*             /\*     }; *\/ */
/*             /\*     report_error( error ); *\/ */
/*             /\*     return false; *\/ */
/*             /\* } *\/ */

/*             /\* *out_type = *symbol->type.definition.info; *\/ */

/*             /\* return true; *\/ */
/*         } */

/*         case TYPEKIND_TYPE: */
/*         { */
/*             return check_type( context, out_type->type.info ); */
/*         } */

/*         default: // TOINFER and INVALID */
/*         { */
/*             UNREACHABLE(); */
/*             break; */
/*         } */
/*     } */
/* } */

bool check_type_compatibility( Type t1, Type* out_type, Error* out_error )
{
    Error error;
    bool result = true;

    if( type_equals( t1, *out_type ) )
    {
        return true;
    }

    if( t1.kind == TYPEKIND_REFERENCE || out_type->kind == TYPEKIND_REFERENCE )
    {
        Type reference_type = t1.kind == TYPEKIND_REFERENCE ? t1 : *out_type;
        Type other_type = t1.kind == TYPEKIND_REFERENCE ? *out_type : t1;

        return check_type_compatibility( *reference_type.reference.base_type, &other_type, out_error );
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

        [ BINARYOPERATION_MODULO ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_INTEGER, TYPEKIND_INTEGER },
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

        [ BINARYOPERATION_AND ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_BOOLEAN,   TYPEKIND_BOOLEAN },
            TYPEKINDPAIR_TERMINATOR,
        },

        [ BINARYOPERATION_OR ] = ( TypeKindPair[] ){
            ( TypeKindPair ){ TYPEKIND_BOOLEAN,   TYPEKIND_BOOLEAN },
            TYPEKINDPAIR_TERMINATOR,
        },
    };

    while( left_type.kind == TYPEKIND_NAMED )
    {
        left_type = *left_type.named.definition;
    }

    while( right_type.kind == TYPEKIND_NAMED )
    {
        right_type = *right_type.named.definition;
    }

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
    if( left_type.kind == TYPEKIND_REFERENCE || right_type.kind == TYPEKIND_REFERENCE )
    {
        Type* reference_type = left_type.kind == TYPEKIND_REFERENCE ? &left_type : &right_type;
        *reference_type = *reference_type->reference.base_type;
    }

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

    // TODO: handle pointer types

    Type left_type_definition = left_type;
    while( left_type_definition.kind == TYPEKIND_NAMED )
    {
        left_type_definition = *left_type_definition.named.definition;
    }

    Type right_type_definition = right_type;
    while( right_type_definition.kind == TYPEKIND_NAMED )
    {
        right_type_definition = *right_type_definition.named.definition;
    }

    // if operation is one of the boolean operators
    if( operation >= BINARYOPERATION_BOOLEAN_START && operation < BINARYOPERATION_BOOLEAN_END )
    {
        *inferred_type = symbol_table_lookup( context->symbol_table, "bool" )->type;
    }
    else
    {
        // if left_type or right_type is float, then float. otherwise its int
        if( left_type_definition.kind == TYPEKIND_FLOAT || right_type_definition.kind == TYPEKIND_FLOAT )
        {
            char as_string[4] = { 0 };
            sprintf( as_string, "f%zu",
                     MAX( left_type_definition.floating.bit_count, right_type_definition.floating.bit_count ) );

            *inferred_type = *symbol_table_lookup( context->symbol_table, as_string )->type.type.info;
        }
        else
        {
            char as_string[4] = { 0 };
            sprintf( as_string, "%c%zu",
                     right_type_definition.integer.is_signed ? 'i' : 'u',
                     MAX( left_type_definition.integer.bit_count, right_type_definition.integer.bit_count ) );

            *inferred_type = *symbol_table_lookup( context->symbol_table, as_string )->type.type.info;

            /* inferred_type->kind = TYPEKIND_INTEGER; */
            /* inferred_type->integer.is_signed = right_type.integer.is_signed; */
            /* inferred_type->integer.bit_count = fmax( left_type.integer.bit_count, right_type.integer.bit_count ); */
        }
    }

    return true;
}

static bool check_rvalue_identifier( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Token identifier_token = expression->associated_token;

    // check if identifier already in symbol table
    Symbol* original_declaration = symbol_table_lookup( context->symbol_table, identifier_token.identifier );
    if( original_declaration == NULL )
    {
        Error error = {
            .kind = ERRORKIND_UNDECLAREDSYMBOL,
            .offending_token = identifier_token,
        };

        report_error( error );
        return false;
    }

    if( original_declaration->type.kind == TYPEKIND_TYPE )
    {
        Error error = {
            .kind = ERRORKIND_CANNOTUSETYPEASVALUE,
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
    Symbol* original_declaration = symbol_table_lookup( context->symbol_table, identifier_token.identifier );
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

    // check if args are valid
    for( int i = 0; i < arg_count; i++ )
    {
        Expression arg = expression->function_call.args[ i ];
        Type arg_type;

        bool is_arg_valid = check_rvalue( context, &arg, &arg_type );
        if( !is_arg_valid )
        {
            // no need to report error here because that is handled by check_rvalue
            return false;
        }

        if( i < param_count )
        {
            Type param_type = param_types[ i ];
            Error error;
            bool are_types_compatible = check_type_compatibility( param_type, &arg_type, &error );
            if( !are_types_compatible )
            {
                error.offending_token = arg.starting_token;
                report_error( error );
                return false;
            }
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
    UNIMPLEMENTED();
    /* Expression* operand = expression->unary.operand; */
    /* Type operand_type; */
    /* bool is_operand_valid = check_rvalue( context, operand, &operand_type ); */
    /* if( !is_operand_valid ) */
    /* { */
    /*     return false; */
    /* } */

    /* UnaryOperation operation = expression->unary.operation; */
    /* if( operation == UNARYOPERATION_ADDRESSOF ) */
    /* { */
    /*     // operand must be lvalue */
    /*     bool operand_lvalue_check = check_lvalue( context, operand, &operand_type ); */
    /*     if( !operand_lvalue_check ) */
    /*     { */
    /*         Error error = { */
    /*             .kind = ERRORKIND_INVALIDADDRESSOF, */
    /*             .offending_token = operand->starting_token */
    /*         }; */

    /*         report_error( error ); */
    /*         return false; */
    /*     } */

    /*     // check if operand is in symbol table */
    /*     Symbol* operand_symbol = symbol_table_lookup( context->symbol_table, operand->associated_token.identifier ); */
    /*     if( operand_symbol == NULL ) */
    /*     { */
    /*         Error error = { */
    /*             .kind = ERRORKIND_UNDECLAREDSYMBOL, */
    /*             .offending_token = operand->associated_token, */
    /*         }; */

    /*         report_error( error ); */
    /*         return false; */
    /*     } */

    /*     *inferred_type = ( Type ){ */
    /*         .kind = TYPEKIND_POINTER, */
    /*         .pointer.base_type = &operand_symbol->type, */
    /*     }; */

    /*     Type operand_type_definition = get_definition_type( context, *inferred_type->pointer.base_type ); */
    /*     add_pointer_type( operand_type_definition, *inferred_type->pointer.base_type ); */
    /* } */
    /* else */
    /* { */
    /*     bool is_operation_valid = is_unary_operation_valid( operation, operand_type ); */
    /*     if( !is_operation_valid ) */
    /*     { */
    /*         Token operator_token = expression->unary.operator_token; */
    /*         Error error = { */
    /*             .kind = ERRORKIND_INVALIDUNARYOPERATION, */
    /*             .offending_token = operator_token, */
    /*             .invalid_unary_operation = { */
    /*                 .operand_type = operand_type */
    /*             } */
    /*         }; */
    /*         report_error( error ); */
    /*         return false; */
    /*     } */

    /*     if( operation == UNARYOPERATION_DEREFERENCE ) */
    /*     { */
    /*         *inferred_type = *operand_type.pointer.base_type; */
    /*     } */
    /*     else */
    /*     { */
    /*         *inferred_type = operand_type; */
    /*     } */
    /* } */

    /* return true; */
}
static bool implicit_cast_possible( Type to, Type from )
{
    if( type_equals( to, from ) )
    {
        return true;
    }

    if( to.kind != from.kind )
    {
        return false;
    }

    switch( to.kind )
    {
        case TYPEKIND_NAMED:
        {
            return implicit_cast_possible( *to.named.definition, *from.named.definition );
        }

        case TYPEKIND_POINTER:
        {
            return implicit_cast_possible( *to.pointer.base_type, *from.pointer.base_type );
        }

        case TYPEKIND_INTEGER:
        {
            // only lossless conversions are allowed
            // u8 -> u16 OK
            // u8 -> i16 OK
            // u8 -> i8  NOT OK
            // i8 -> u* NOT OK
            // i8 -> i16 OK

            if( to.integer.is_signed == from.integer.is_signed )
            {
                return to.integer.bit_count >= from.integer.bit_count;
            }
            else if( to.integer.is_signed && !from.integer.is_signed )
            {
                return to.integer.bit_count > from.integer.bit_count;
            }

            return false;
        }

        case TYPEKIND_FLOAT:
        {
            // only lossless conversions are allowed
            printf("%zu %zu\n", to.floating.bit_count, from.floating.bit_count);
            return to.floating.bit_count >= from.floating.bit_count;
        }

        case TYPEKIND_FUNCTION:
        {
            UNIMPLEMENTED();
        }

        case TYPEKIND_ARRAY:
        {
            if( !type_equals( *to.array.base_type, *from.array.base_type ) )
            {
                return false;
            }

            if( to.array.length == -1 )
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        default:
        {
            UNREACHABLE();
        }
    }
}

static bool check_type_rvalue( SemanticContext* context, Expression* type_rvalue, Type* out_type );
static bool check_array( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Expression* declared_type_rvalue = expression->array.base_type_rvalue;
    Type array_type;
    if( !check_type_rvalue( context, declared_type_rvalue, &array_type ) )
    {
        return false;
    }

    array_type = *array_type.type.info;

    int found_length = array_type.array.length;
    int count_initialized = expression->array.count_initialized;

    // declared_length being -1 means it is to be inferred
    if( ( found_length == -1 && count_initialized == 0 ) ||
        found_length == 0 )
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
    array_type.array.length = found_length;

    bool are_initializers_valid = true;
    for( int i = 0; i < count_initialized; i++ )
    {
        Type element_type;
        Expression* element_rvalue = &expression->array.initialized_rvalues[ i ];
        if( !check_rvalue( context, element_rvalue, &element_type ) )
        {
            are_initializers_valid = false;
        }

        if( !implicit_cast_possible( *array_type.array.base_type, element_type ) )
        {
            Error error = {
                .kind = ERRORKIND_TYPEMISMATCH,
                .offending_token = element_rvalue->starting_token,
                .type_mismatch = {
                    .expected = *array_type.array.base_type,
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

    *inferred_type = array_type;
    add_array_type( *array_type.array.base_type );

    return true;
}

static bool check_array_subscript( SemanticContext* context, Expression* expression, Type* out_type )
{
    UNIMPLEMENTED();

    /* Expression* index_rvalue = expression->array_subscript.index_rvalue; */
    /* Expression* lvalue = expression->array_subscript.lvalue; */

    /* Type lvalue_type; */
    /* if( !check_lvalue( context, lvalue, &lvalue_type ) ) */
    /* { */
    /*     return false; */
    /* } */

    /* // only arrays can be subscripted */
    /* if( lvalue_type.kind != TYPEKIND_ARRAY ) */
    /* { */
    /*     Error error = { */
    /*         .kind = ERRORKIND_NOTANARRAY, */
    /*         .offending_token = lvalue->starting_token, */
    /*     }; */
    /*     report_error( error ); */
    /*     return false; */
    /* } */

    /* Type index_rvalue_type; */
    /* if( !check_rvalue( context, index_rvalue, &index_rvalue_type ) ) */
    /* { */
    /*     return false; */
    /* } */

    /* if( index_rvalue_type.kind != TYPEKIND_INTEGER ) */
    /* { */
    /*     Error error = { */
    /*         .kind = ERRORKIND_INVALIDARRAYSUBSCRIPT, */
    /*         .offending_token = index_rvalue->starting_token, */
    /*     }; */
    /*     report_error( error ); */
    /*     return false; */
    /* } */

    /* expression->array_subscript.type = lvalue_type; */
    /* *out_type = *lvalue_type.array.base_type; */

    /* return true; */
}

static bool check_member_access( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Expression* lvalue = expression->member_access.lvalue;
    Type lvalue_type;
    if( !check_lvalue( context, lvalue, &lvalue_type ) )
    {
        return false;
    }

    while( lvalue_type.kind == TYPEKIND_REFERENCE )
    {
        lvalue_type = *lvalue_type.reference.base_type;
    }

    if( lvalue_type.kind != TYPEKIND_COMPOUND )
    {
        Error error = {
            .kind = ERRORKIND_NOTCOMPOUND,
            .offending_token = lvalue->starting_token,
        };
        report_error( error );
        return false;
    }

    Token member_identifier_token = expression->member_access.member_identifier_token;
    Symbol* member_symbol = symbol_table_lookup( *lvalue_type.compound.member_symbol_table, member_identifier_token.as_string );
    if( member_symbol == NULL )
    {
        Error error = {
            .kind = ERRORKIND_MISSINGMEMBER,
            .offending_token = member_identifier_token,
            .missing_member.parent_type = lvalue_type,
        };
        report_error( error );
        return false;
    }

    *inferred_type = member_symbol->type;

    return true;
}

static bool check_compound_literal( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    Token type_identifier_token = expression->compound_literal.type_identifier_token;

    // check if type has been declared
    Symbol* type_symbol = symbol_table_lookup( context->symbol_table, type_identifier_token.as_string );
    if( type_symbol == NULL )
    {
        Error error = {
            .kind = ERRORKIND_UNDECLAREDSYMBOL,
            .offending_token = type_identifier_token,
        };
        report_error( error );
        return false;
    }

    Type type = type_symbol->type;
    if( type.kind != TYPEKIND_TYPE )
    {
        Error error = {
            .kind = ERRORKIND_INVALIDCOMPOUNDLITERAL,
            .offending_token = type_identifier_token,
        };
        report_error( error );
        return false;
    }

    Type type_info = *type.type.info;
    if( type_info.kind != TYPEKIND_COMPOUND )
    {
        Error error = {
            .kind = ERRORKIND_INVALIDCOMPOUNDLITERAL,
            .offending_token = type.token,
        };
        report_error( error );
        return false;
    }

    int initialized_count = expression->compound_literal.initialized_count;
    for( int i = 0; i < initialized_count; i++ )
    {
        Token member_identifier_token = expression->compound_literal.member_identifier_tokens[ i ];
        Symbol* member_symbol = symbol_table_lookup( *type_info.compound.member_symbol_table, member_identifier_token.as_string );
        if( member_symbol == NULL )
        {
            Error error = {
                .kind = ERRORKIND_UNDECLAREDSYMBOL,
                .offending_token = member_identifier_token,
            };
            report_error( error );
            return false;
        }

        Expression rvalue = expression->compound_literal.initialized_member_rvalues[ i ];
        Type rvalue_type;
        if( !check_rvalue( context, &rvalue, &rvalue_type ) )
        {
            return false;
        }

        Error error;
        bool are_types_compatible = check_type_compatibility( member_symbol->type, &rvalue_type, &error );
        if( !are_types_compatible )
        {
            error.offending_token = rvalue.starting_token;
            report_error( error );
            return false;
        }
    }

    *inferred_type = type_info;

    return true;
}

static bool check_rvalue( SemanticContext* context, Expression* expression, Type* inferred_type )
{
    // initialized to true for the base cases
    bool is_valid = true;

    switch( expression->kind )
    {
        // case EXPRESSIONKIND_STRING:    inferred_type->kind = TYPEKIND_STRING;    break;
        case EXPRESSIONKIND_CHARACTER:
        {
            *inferred_type = *symbol_table_lookup( context->symbol_table, "char" )->type.type.info;
            break;
        }

        case EXPRESSIONKIND_BOOLEAN:
        {
            *inferred_type = *symbol_table_lookup( context->symbol_table, "bool" )->type.type.info;
            break;
        }

        case EXPRESSIONKIND_STRING:
        {
            inferred_type->kind = TYPEKIND_POINTER;
            inferred_type->pointer.base_type = symbol_table_lookup( context->symbol_table, "char" )->type.type.info;
            break;
        }

        case EXPRESSIONKIND_INTEGER:
        {
            uint64_t num = expression->integer;
            int bit_count;
            bool is_signed;
            if( num <= INT32_MAX )
            {
                bit_count = 32;
                is_signed = true;
            }
            else if( num <= INT64_MAX )
            {
                bit_count = 64;
                is_signed = true;
            }
            else // if( num <= UINT64_MAX )
            {
                bit_count = 64;
                is_signed = false;
            }

            char as_string[4] = { 0 };
            sprintf( as_string, "%c%d",
                     is_signed ? 'i' : 'u',
                     bit_count );

            *inferred_type = *symbol_table_lookup( context->symbol_table, as_string )->type.type.info;
            break;
        }

        case EXPRESSIONKIND_FLOAT:
        {
            double floating = expression->floating;
            int bit_count;

            if( floating <= FLT_MAX )
            {
                bit_count = 32;
            }
            else// if( floating <= INT64_MAX )
            {
                printf("here\n");
                bit_count = 64;
            }

            char as_string[4] = { 0 };
            sprintf( as_string, "f%d",
                     bit_count );

            *inferred_type = *symbol_table_lookup( context->symbol_table, as_string )->type.type.info;
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

        case EXPRESSIONKIND_MEMBERACCESS:
        {
            is_valid = check_member_access( context, expression, inferred_type );
            break;
        }

        case EXPRESSIONKIND_COMPOUNDLITERAL:
        {
            is_valid = check_compound_literal( context, expression, inferred_type );
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
    Symbol* original_declaration = symbol_table_lookup( context->symbol_table, identifier_token.identifier );
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

    // get declared type if its there
    Type declared_type = ( Type ){ .kind = TYPEKIND_TOINFER };
    Expression* type_rvalue = expression->variable_declaration.type_rvalue;
    if( type_rvalue != NULL )
    {
        Type declared_type_type;
        if ( !check_type_rvalue( context, type_rvalue, &declared_type_type ) )
        {
            return false;
        }

        if( declared_type_type.kind != TYPEKIND_TYPE )
        {
            Error error = {
                .kind = ERRORKIND_NOTATYPE,
                .offending_token = type_rvalue->starting_token
            };
            report_error( error );
            return false;
        }

        // anonymous types are not allowed!
        // TODO: update this when new typekinds are made
        if( declared_type_type.type.info->kind == TYPEKIND_COMPOUND )
        {
            Error error = {
                .kind = ERRORKIND_INVALIDANONYMOUSTYPE,
                .offending_token = type_rvalue->starting_token
            };
            report_error( error );
            return false;
        }

        declared_type = *declared_type_type.type.info;
    }

    // void variables are not allowed
    if( declared_type.kind == TYPEKIND_NAMED && declared_type.named.definition->kind == TYPEKIND_VOID )
    {
        Error error = {
            .kind = ERRORKIND_VOIDVARIABLE,
            .offending_token = type_rvalue->starting_token
        };
        report_error( error );
        return false;
    }

    // check if inferred type matches declared type
    Type variable_type;
    Expression* rvalue = expression->variable_declaration.rvalue;
    if( rvalue != NULL )
    {
        Type inferred_type;
        if( !check_rvalue( context, rvalue, &inferred_type ) )
        {
            return false;
        }

        if( declared_type.kind == TYPEKIND_TOINFER )
        {
            variable_type = inferred_type;
        }
        else
        {
            // check compatibility between types
            /* Type declared_type_definition = *declared_type.named.definition; */
            /* Type inferred_type_definition = *inferred_type.named.definition; */

            if( !implicit_cast_possible( declared_type, inferred_type ) )
            {
                Error error = {
                    .kind = ERRORKIND_INVALIDIMPLICITCAST,
                    .offending_token = rvalue->starting_token,
                    .invalid_implicit_cast = {
                        .to = declared_type,
                        .from = inferred_type,
                    }
                };
                report_error( error );
                return false;
            }

            if( inferred_type.kind == TYPEKIND_ARRAY )
            {
                // because we want to infer the length
                variable_type = inferred_type;
            }
            else
            {
                variable_type = declared_type;
            }
        }
    }
    else
    {
        // we can assume here that declared_type.kind can never be TOINFER because
        // that would be a parsing error
        variable_type = declared_type;
    }

    // for debug purposes
    expression->variable_declaration.variable_type = variable_type;

    // add to symbol table
    Symbol symbol = {
        .token = identifier_token,
        .type = variable_type,
    };
    symbol_table_push_symbol( &context->symbol_table, symbol );

    return true;
}

static bool check_compound( SemanticContext* context, Expression* expression )
{
    bool is_valid = true;

    symbol_table_push_scope( &context->symbol_table );

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

    if( context->symbol_table.scope_depth > 1 )
    {
        symbol_table_pop_scope( &context->symbol_table );
    }

    return is_valid;
}

static bool check_function_declaration( SemanticContext* context,Expression* expression, bool is_extern )
{
    Token identifier_token = expression->function_declaration.identifier_token;

    // check if identifier already in symbol table
    Symbol* original_declaration = symbol_table_lookup( context->symbol_table, identifier_token.identifier );
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

    Expression* return_type_rvalue = expression->function_declaration.return_type_rvalue;
    Type* return_type = malloc( sizeof( Type ) );
    if( !check_type_rvalue( context, return_type_rvalue, return_type ) )
    {
        return false;
    }

    if( return_type->kind != TYPEKIND_TYPE )
    {
        Error error = {
            .kind = ERRORKIND_NOTATYPE,
            .offending_token = return_type_rvalue->starting_token
        };
        report_error( error );
        return false;
    }

    // anonymous types are not allowed!
    // TODO: update this when new typekinds are made
    if( return_type->type.info->kind == TYPEKIND_COMPOUND )
    {
        Error error = {
            .kind = ERRORKIND_INVALIDANONYMOUSTYPE,
            .offending_token = return_type_rvalue->starting_token
        };
        report_error( error );
        return false;
    }

    *return_type = *return_type->type.info;

    Token* param_identifiers_tokens = expression->function_declaration.param_identifiers_tokens;
    Expression* param_type_rvalues = expression->function_declaration.param_type_rvalues;
    int param_count = expression->function_declaration.param_count;
    bool is_variadic = expression->function_declaration.is_variadic;
    Type* param_types = lvec_new( Type );

    for( int i = 0; i < param_count; i++ )
    {
        // check if param type is good
        Expression param_type_rvalue = param_type_rvalues[ i ];
        Type param_type;
        if( !check_type_rvalue( context, &param_type_rvalue, &param_type ) )
        {
            return false;
        }

        if( param_type.kind != TYPEKIND_TYPE )
        {
            Error error = {
                .kind = ERRORKIND_NOTATYPE,
                .offending_token = param_type_rvalue.starting_token
            };
            report_error( error );
            return false;
        }

        // anonymous types are not allowed!
        // TODO: update this when new typekinds are made
        if( param_type.type.info->kind == TYPEKIND_COMPOUND )
        {
            Error error = {
                .kind = ERRORKIND_INVALIDANONYMOUSTYPE,
                .offending_token = param_type_rvalue.starting_token
            };
            report_error( error );
            return false;
        }

        param_type = *param_type.type.info;
        lvec_append_aggregate( param_types, param_type );

        // check if param identifier is good
        Token param_identifier_token = param_identifiers_tokens[ i ];
        Symbol* lookup_result = symbol_table_lookup( context->symbol_table, param_identifier_token.as_string );
        if( lookup_result != NULL )
        {
            Error error = {
                .kind = ERRORKIND_SYMBOLREDECLARATION,
                .offending_token = param_identifier_token,
                .symbol_redeclaration.original_declaration_token = lookup_result->token,
            };
            report_error( error );
            return false;
        }
        else if( strcmp( param_identifier_token.as_string, identifier_token.as_string ) == 0 )
        {
            Error error = {
                .kind = ERRORKIND_SYMBOLREDECLARATION,
                .offending_token = param_identifier_token,
                .symbol_redeclaration.original_declaration_token = identifier_token,
            };
            report_error( error );
            return false;
        }
    }

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
    symbol_table_push_symbol( &context->symbol_table, symbol );

    // have to do this here because we have to push the function identifier to the
    // symbol table first
    symbol_table_push_scope( &context->symbol_table );
    for( int i = 0; i < param_count; i++ )
    {
        Symbol param_symbol = {
            .token = param_identifiers_tokens[ i ],
            .type = param_types[ i ],
        };
        symbol_table_push_symbol( &context->symbol_table, param_symbol );
    }
    push_return_type( context, *return_type );

    // check function body
    Expression* body = expression->function_declaration.body;
    if( body == NULL && !is_extern )
    {
        Error error = {
            .kind = ERRORKIND_MISSINGFUNCTIONBODY,
            .offending_token = expression->starting_token,
        };
        report_error( error );
        return false;
    }
    else if( body != NULL && is_extern )
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
        is_body_valid = check_compound( context, body );
    }

    symbol_table_pop_scope( &context->symbol_table );
    pop_return_type( context );

    if( !is_body_valid )
    {
        return false;
    }

    return true;
}

bool check_return( SemanticContext* context, Expression* expression )
{
    Type found_return_type = *symbol_table_lookup( context->symbol_table, "void" )->type.type.info;
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

    if( !implicit_cast_possible( expected_return_type, found_return_type ) )
    {
        Token offending_token;
        Expression* rvalue = expression->return_expression.rvalue;
        if( rvalue == NULL )
        {
            offending_token = expression->starting_token;
        }
        else
        {
            offending_token = rvalue->starting_token;
        }

        Error error = {
            .kind = ERRORKIND_TYPEMISMATCH,
            .offending_token = offending_token,
            .type_mismatch = {
                .expected = expected_return_type,
                .found = found_return_type,
            },
        };
        report_error( error );
        return false;
    }

    /* Error error; */
    /* bool are_types_compatible = check_type_compatibility( expected_return_type, &found_return_type, &error ); */
    /* if( !are_types_compatible ) */
    /* { */
    /*     Token associated_token; */
    /*     if( found_return_type.kind == TYPEKIND_VOID ) */
    /*     { */
    /*         associated_token = expression->starting_token; */
    /*     } */
    /*     else */
    /*     { */
    /*         associated_token = expression->return_expression.rvalue->starting_token; */
    /*     } */

    /*     error.offending_token = associated_token; */
    /*     report_error( error ); */
    /*     return false; */
    /* } */

    return true;
}

static bool check_lvalue( SemanticContext* context, Expression* expression, Type* out_type )
{
    bool is_valid;
    switch( expression->kind )
    {
        case EXPRESSIONKIND_COMPOUNDLITERAL:
        case EXPRESSIONKIND_MEMBERACCESS:
        case EXPRESSIONKIND_ARRAYSUBSCRIPT:
        case EXPRESSIONKIND_IDENTIFIER:
        {
            is_valid = check_rvalue( context, expression, out_type );
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

    if( found_rvalue_type.kind == TYPEKIND_TYPE )
    {
        Error error = {
            .kind = ERRORKIND_CANNOTUSETYPEASVALUE,
            .offending_token = expression->assignment.rvalue->starting_token,
        };
        report_error( error );
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

static bool check_for_loop( SemanticContext* context, Expression* expression )
{
    UNIMPLEMENTED();

    /* // no symbol redeclarations! */
    /* Token iterator_token = expression->for_loop.iterator_token; */
    /* Symbol* iterator_symbol = symbol_table_lookup( context->symbol_table, iterator_token.identifier ); */
    /* if( iterator_symbol != NULL ) */
    /* { */
    /*     Error error = { */
    /*         .kind = ERRORKIND_SYMBOLREDECLARATION, */
    /*         .offending_token = iterator_token, */
    /*         .symbol_redeclaration.original_declaration_token = iterator_token */
    /*     }; */
    /*     report_error( error ); */
    /*     return false; */
    /* } */

    /* // check iterable and get type */
    /* Expression* iterable_rvalue = expression->for_loop.iterable_rvalue; */
    /* Type inferred_type; */
    /* if ( !check_rvalue( context, iterable_rvalue, &inferred_type ) ) */
    /* { */
    /*     return NULL; */
    /* } */

    /* if( inferred_type.kind != TYPEKIND_ARRAY ) */
    /* { */
    /*     Error error = { */
    /*         .kind = ERRORKIND_NOTANITERATOR, */
    /*         .offending_token = iterable_rvalue->starting_token */
    /*     }; */
    /*     report_error( error ); */
    /*     return false; */
    /* } */

    /* symbol_table_push_scope( &context->symbol_table ); */

    /* // add iterator to symbeol table */
    /* // calling it iterator_symbol2 because i dont wanna reuse iterator_symbol */
    /* // because its a pointer and id have to either allocate memory or create */
    /* // a compound literal then get its address and i DONT WANNA DO EITHER OF THOSE */
    /* Type iterator_type = { */
    /*     .kind = TYPEKIND_REFERENCE, */
    /*     .reference.base_type = inferred_type.array.base_type */
    /* }; */
    /* // *iterator_type.reference.base_type = *; */

    /* Symbol iterator_symbol2 = { */
    /*     .token = iterator_token, */
    /*     .type = iterator_type */
    /* }; */
    /* symbol_table_push_symbol( &context->symbol_table, iterator_symbol2 ); */
    /* expression->for_loop.iterator_type = iterator_type; */

    /* Expression* body = expression->for_loop.body; */
    /* if( !check_compound( context, body ) ) */
    /* { */
    /*     symbol_table_pop_scope( &context->symbol_table ); */
    /*     return false; */
    /* } */

    /* symbol_table_pop_scope( &context->symbol_table ); */

    /* return true; */
}

static bool check_compound_definition( SemanticContext* context, Expression* type_rvalue, Type* out_type )
{
    UNIMPLEMENTED();

    /* bool is_struct = type_rvalue->compound_definition.is_struct; */
    /* Expression* member_type_rvalues = type_rvalue->compound_definition.member_type_rvalues; */
    /* Token* member_identifier_tokens = type_rvalue->compound_definition.member_identifier_tokens; */
    /* int member_count = type_rvalue->compound_definition.member_count; */

    /* SymbolTable* member_symbol_table = malloc( sizeof( SymbolTable ) ); */
    /* symbol_table_initialize( member_symbol_table ); */

    /* for( int i = 0; i < member_count; i++ ) */
    /* { */
    /*     // check type of member */
    /*     Expression member_type_rvalue = member_type_rvalues[ i ]; */
    /*     Type member_type_definition; */
    /*     if( !check_type_rvalue( context, &member_type_rvalue, &member_type_definition ) ) */
    /*     { */
    /*         return false; */
    /*     } */

    /*     Type member_type = *member_type_definition.definition.info; */

    /*     // void members are not allowed in struct types */
    /*     if( member_type.kind == TYPEKIND_VOID && is_struct ) */
    /*     { */
    /*         Error error = { */
    /*             .kind = ERRORKIND_VOIDVARIABLE, */
    /*             .offending_token = member_type_rvalue.type_identifier.token, */
    /*         }; */
    /*         report_error( error ); */
    /*         return false; */
    /*     } */

    /*     // check if member is already declared within the struct */
    /*     Token member_identifier_token = member_identifier_tokens[ i ]; */
    /*     Symbol* lookup_result = symbol_table_lookup( *member_symbol_table, member_identifier_token.as_string ); */
    /*     if( lookup_result != NULL ) */
    /*     { */
    /*         Error error = { */
    /*             .kind = ERRORKIND_SYMBOLREDECLARATION, */
    /*             .offending_token = member_identifier_token, */
    /*             .symbol_redeclaration.original_declaration_token = lookup_result->token, */
    /*         }; */
    /*         report_error( error ); */
    /*         return false; */
    /*     } */

    /*     Symbol member_symbol = { */
    /*         .token = member_identifier_token, */
    /*         .type = member_type */
    /*     }; */
    /*     symbol_table_push_symbol( member_symbol_table, member_symbol ); */
    /* } */

    /* Type* info = malloc( sizeof( Type ) ); */
    /* *info = ( Type ){ */
    /*     .kind = TYPEKIND_COMPOUND, */
    /*     .token = ( Token ){ .as_string = "<anonymous>"}, */
    /*     .compound = { */
    /*         // .identifier = "<anonymous>", */
    /*         .member_symbol_table = member_symbol_table, */
    /*     } */
    /* }; */

    /* *out_type = ( Type ){ */
    /*     .kind = TYPEKIND_DEFINITION, */
    /*     .definition = { */
    /*         .info = info, */
    /*         .pointer_types = lvec_new( Type ), */
    /*         .array_types = lvec_new( Type ), */
    /*     } */
    /* }; */

    /* return true; */
}

static bool check_type_identifier( SemanticContext* context, Expression* expression, Type* out_type )
{
    Token identifier_token = expression->type_identifier.token;
    Symbol* lookup_result = symbol_table_lookup( context->symbol_table, identifier_token.as_string );
    if( lookup_result == NULL )
    {
        Error error = {
            .kind = ERRORKIND_UNDECLAREDSYMBOL,
            .offending_token = identifier_token,
        };
        report_error( error );
        return false;
    }
    Type type = lookup_result->type;
    if( type.kind != TYPEKIND_TYPE )
    {
        Error error = {
            .kind = ERRORKIND_NOTATYPE,
            .offending_token = identifier_token
        };
        report_error( error );
        return false;
    }

    *out_type = type;

    return true;
}

static bool check_pointer_type( SemanticContext* context, Expression* type_rvalue, Type* out_type )
{
    Expression* base_type_rvalue = type_rvalue->pointer_type.base_type_rvalue;
    Type base_type_definition;
    if ( !check_type_rvalue( context, base_type_rvalue, &base_type_definition ) )
    {
        return false;
    }

    if( base_type_definition.kind != TYPEKIND_TYPE )
    {
        Error error = {
            .kind = ERRORKIND_NOTATYPE,
            .offending_token = base_type_rvalue->starting_token
        };
        report_error( error );
        return false;
    }

    // anonymous types are not allowed!
    if( base_type_definition.type.info->kind != TYPEKIND_NAMED )
    {
        Error error = {
            .kind = ERRORKIND_INVALIDANONYMOUSTYPE,
            .offending_token = base_type_rvalue->starting_token
        };
        report_error( error );
        return false;
    }

    Type* info = malloc( sizeof( Type ) );
    *info = ( Type ){
        .kind = TYPEKIND_POINTER,
        .pointer.base_type = base_type_definition.type.info,
    };

    *out_type = ( Type ){
        .kind = TYPEKIND_TYPE,
        .type.info = info,
    };

    return true;
}

static bool check_array_type( SemanticContext* context, Expression* type_rvalue, Type* out_type )
{
    Expression* base_type_rvalue = type_rvalue->array_type.base_type_rvalue;
    Type* base_type_definition = malloc( sizeof( Type ) );
    if ( !check_type_rvalue( context, base_type_rvalue, base_type_definition ) )
    {
        return false;
    }

    // anonymous types are not allowed!
    if( base_type_definition->type.info->kind == TYPEKIND_COMPOUND )
    {
        Error error = {
            .kind = ERRORKIND_INVALIDANONYMOUSTYPE,
            .offending_token = base_type_rvalue->starting_token
        };
        report_error( error );
        return false;
    }
    base_type_definition = base_type_definition->type.info;

    // array of voids are not allowed!
    if( base_type_definition->kind == TYPEKIND_NAMED && base_type_definition->named.definition->kind == TYPEKIND_VOID)
    {
        Error error = {
            .kind = ERRORKIND_VOIDVARIABLE,
            .offending_token = type_rvalue->starting_token
        };
        report_error( error );
        return false;
    }

    // zero-length arrays are not allowed
    int length = type_rvalue->array_type.length;
    if( length == 0 )
    {
        Error error = {
            .kind = ERRORKIND_ZEROLENGTHARRAY,
            .offending_token = type_rvalue->starting_token,
        };
        report_error( error );
        return false;
    }

    Type* info = malloc( sizeof( Type ) );
    *info = ( Type ){
        .kind = TYPEKIND_ARRAY,
        .array = {
            .base_type = base_type_definition,
            .length = length,
        },
    };

    *out_type = ( Type ){
        .kind = TYPEKIND_TYPE,
        .type.info = info,
    };

    return true;
}

static bool check_type_rvalue( SemanticContext* context, Expression* type_rvalue, Type* out_type )
{
    switch( type_rvalue->kind )
    {
        case EXPRESSIONKIND_COMPOUNDDEFINITION:
        {
            return check_compound_definition( context, type_rvalue, out_type );
        }

        case EXPRESSIONKIND_TYPEIDENTIFIER:
        {
            return check_type_identifier( context, type_rvalue, out_type );
        }

        case EXPRESSIONKIND_POINTERTYPE:
        {
            return check_pointer_type( context, type_rvalue, out_type );
        }

        case EXPRESSIONKIND_ARRAYTYPE:
        {
            return check_array_type( context, type_rvalue, out_type );
        }

        default:
        {
            UNREACHABLE();
        }
    }

    // return is_valid;
}

static bool check_type_declaration( SemanticContext* context, Expression* expression )
{
    UNIMPLEMENTED();

    /* // check if type name is already in symbol table */
    /* Token identifier_token = expression->type_declaration.identifier_token; */
    /* Symbol* symbol = symbol_table_lookup( context->symbol_table, identifier_token.as_string ); */
    /* if( symbol != NULL ) */
    /* { */
    /*     Error error = { */
    /*         .kind = ERRORKIND_SYMBOLREDECLARATION, */
    /*         .offending_token = identifier_token, */
    /*         .symbol_redeclaration.original_declaration_token = identifier_token */
    /*     }; */
    /*     report_error( error ); */
    /*     return false; */
    /* } */

    /* Expression* type_rvalue = expression->type_declaration.rvalue; */
    /* Type* definition = malloc( sizeof( Type ) ); */
    /* if( !check_type_rvalue( context, type_rvalue, definition ) ) */
    /* { */
    /*     return false; */
    /* } */

    /* Type* info = malloc( sizeof( Type ) ); */
    /* *info = ( Type ){ */
    /*     .kind = TYPEKIND_CUSTOM, */
    /*     .custom.definition = definition */
    /* }; */

    /* Symbol type_declaration_symbol = { */
    /*     .token = identifier_token, */
    /*     .type = ( Type ){ */
    /*         .kind = TYPEKIND_DEFINITION, */
    /*         .definition = { */
    /*             .info = info, */
    /*             .pointer_types = lvec_new( Type ), */
    /*             .array_types = lvec_new( Type ), */
    /*         } */
    /*     } */
    /* }; */

    /* symbol_table_push_symbol( &context->symbol_table, type_declaration_symbol ); */
    /* return true; */
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

        case EXPRESSIONKIND_FORLOOP:
        {
            is_valid = check_for_loop( context, expression );
            break;
        }

        case EXPRESSIONKIND_TYPEDECLARATION:
        {
            is_valid = check_type_declaration( context, expression );
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }
    return is_valid;
}
