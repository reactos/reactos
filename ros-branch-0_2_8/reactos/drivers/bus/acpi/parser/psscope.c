/******************************************************************************
 *
 * Module Name: psscope - Parser scope stack management routines
 *              $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 R. Byron Moore
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "acpi.h"
#include "acparser.h"

#define _COMPONENT          ACPI_PARSER
	 MODULE_NAME         ("psscope")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_parent_scope
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:      Pointer to an Op object
 *
 * DESCRIPTION: Get parent of current op being parsed
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
acpi_ps_get_parent_scope (
	ACPI_PARSE_STATE        *parser_state)
{
	return (parser_state->scope->parse_scope.op);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_has_completed_scope
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:      Boolean, TRUE = scope completed.
 *
 * DESCRIPTION: Is parsing of current argument complete?  Determined by
 *              1) AML pointer is at or beyond the end of the scope
 *              2) The scope argument count has reached zero.
 *
 ******************************************************************************/

u8
acpi_ps_has_completed_scope (
	ACPI_PARSE_STATE        *parser_state)
{
	return ((u8) ((parser_state->aml >= parser_state->scope->parse_scope.arg_end ||
			   !parser_state->scope->parse_scope.arg_count)));
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_init_scope
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *              Root                - the Root Node of this new scope
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate and init a new scope object
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ps_init_scope (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       *root_op)
{
	ACPI_GENERIC_STATE      *scope;


	scope = acpi_cm_create_generic_state ();
	if (!scope) {
		return (AE_NO_MEMORY);
	}

	scope->parse_scope.op       = root_op;
	scope->parse_scope.arg_count = ACPI_VAR_ARGS;
	scope->parse_scope.arg_end  = parser_state->aml_end;
	scope->parse_scope.pkg_end  = parser_state->aml_end;

	parser_state->scope         = scope;
	parser_state->start_op      = root_op;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_push_scope
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *              Op                  - Current op to be pushed
 *              Remaining_args      - List of args remaining
 *              Arg_count           - Fixed or variable number of args
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Push current op to begin parsing its argument
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ps_push_scope (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       *op,
	u32                     remaining_args,
	u32                     arg_count)
{
	ACPI_GENERIC_STATE      *scope;


	scope = acpi_cm_create_generic_state ();
	if (!scope) {
		return (AE_NO_MEMORY);
	}


	scope->parse_scope.op          = op;
	scope->parse_scope.arg_list    = remaining_args;
	scope->parse_scope.arg_count   = arg_count;
	scope->parse_scope.pkg_end     = parser_state->pkg_end;

	/* Push onto scope stack */

	acpi_cm_push_generic_state (&parser_state->scope, scope);


	if (arg_count == ACPI_VAR_ARGS) {
		/* multiple arguments */

		scope->parse_scope.arg_end = parser_state->pkg_end;
	}

	else {
		/* single argument */

		scope->parse_scope.arg_end = ACPI_MAX_AML;
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_pop_scope
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *              Op                  - Where the popped op is returned
 *              Arg_list            - Where the popped "next argument" is
 *                                    returned
 *              Arg_count           - Count of objects in Arg_list
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Return to parsing a previous op
 *
 ******************************************************************************/

void
acpi_ps_pop_scope (
	ACPI_PARSE_STATE        *parser_state,
	ACPI_PARSE_OBJECT       **op,
	u32                     *arg_list,
	u32                     *arg_count)
{
	ACPI_GENERIC_STATE      *scope = parser_state->scope;


	/*
	 * Only pop the scope if there is in fact a next scope
	 */
	if (scope->common.next) {
		scope = acpi_cm_pop_generic_state (&parser_state->scope);


		/* return to parsing previous op */

		*op                     = scope->parse_scope.op;
		*arg_list               = scope->parse_scope.arg_list;
		*arg_count              = scope->parse_scope.arg_count;
		parser_state->pkg_end   = scope->parse_scope.pkg_end;

		/* All done with this scope state structure */

		acpi_cm_delete_generic_state (scope);
	}

	else {
		/* empty parse stack, prepare to fetch next opcode */

		*op                     = NULL;
		*arg_list               = 0;
		*arg_count              = 0;
	}


	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_cleanup_scope
 *
 * PARAMETERS:  Parser_state        - Current parser state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Destroy available list, remaining stack levels, and return
 *              root scope
 *
 ******************************************************************************/

void
acpi_ps_cleanup_scope (
	ACPI_PARSE_STATE        *parser_state)
{
	ACPI_GENERIC_STATE      *scope;


	if (!parser_state) {
		return;
	}


	/* Delete anything on the scope stack */

	while (parser_state->scope) {
		scope = acpi_cm_pop_generic_state (&parser_state->scope);
		acpi_cm_delete_generic_state (scope);
	}

	return;
}

