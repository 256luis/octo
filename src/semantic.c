#include <string.h>
#include "semantic.h"
#include "debug.h"
#include "parser.h"
#include "error.h"

static bool is_symbol_identifier_taken( SymbolTable symbol_table, char* identifier )
{
    for( int i = 0; i < symbol_table.symbol_count; i++ )
    {
        if( strcmp( symbol_table.identifiers[ i ], identifier) == 0 )
        {
            return false;
        }
    }

    return true;
}

Type analyze_rvalue_type( Expression* rvalue )
{
    Type type;

    switch( rvalue->kind )
    {
        case EXPRESSIONKIND_NUMBER:
        {
            type.kind = TYPEKIND_INT;
            break;
        }

        case EXPRESSIONKIND_IDENTIFIER:
        {
            // lookup identifier from symbol table
            // return the type

            UNIMPLEMENTED();
            break;
        }

        case EXPRESSIONKIND_STRING:
        {
            type.kind = TYPEKIND_STRING;
            break;
        }

        case EXPRESSIONKIND_CHARACTER:
        {
            type.kind = TYPEKIND_CHAR;
            break;
        }

        case EXPRESSIONKIND_BINARY:
        {
            // TODO: when TYPEKIND_FLOAT has been added, update this block

            Type left_type = analyze_rvalue_type( rvalue->binary.left );
            Type right_type = analyze_rvalue_type( rvalue->binary.right );

            if( left_type.kind != TYPEKIND_INT )
            {
                // report invalid operation for type error
                UNIMPLEMENTED();
            }

            if( right_type.kind != TYPEKIND_INT )
            {
                // report invalid operation for type error
                UNIMPLEMENTED();
            }

            type.kind = TYPEKIND_INT;

            break;
        }

        case EXPRESSIONKIND_UNARY:
        {
            UNIMPLEMENTED();
            break;
        }

        default:
        {
            UNREACHABLE();
        }
    }
}

static void analyze_variable_declaration( SemanticContext context, Expression* expression )
{
    // check if variable name already exists on symbol table
    if( is_symbol_identifier_taken( context.symbol_table, expression->variable_declaration.identifier ) )
    {
        // TODO: add line member to Error struct
        Error error = {
            .kind = ERRORKIND_SYMBOLREDECLARATION,
            .source_code = context.source_code,
            .symbol_redeclaration = {
                .identifier = expression->variable_declaration.identifier,
            },
        };

        report_error( error );

        return;
    }

    // infer type if needed
    if( expression->variable_declaration.type == NULL )
    {
        Type type = analyze_rvalue_type( expression->variable_declaration.rvalue );
    }
}

void semantic_analyze( SemanticContext context, Expression* expression )
{
    switch( expression->kind )
    {
        case EXPRESSIONKIND_VARIABLEDECLARATION:
        {
            analyze_variable_declaration( context, expression );
            break;
        }

        case EXPRESSIONKIND_FUNCTIONDECLARATION:
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
}
