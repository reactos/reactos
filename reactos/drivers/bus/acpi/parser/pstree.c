/******************************************************************************
 *
 * Module Name: pstree - Parser op tree manipulation/traversal/search
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
#include "amlcode.h"

#define _COMPONENT          ACPI_PARSER
	 MODULE_NAME         ("pstree")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_arg
 *
 * PARAMETERS:  Op              - Get an argument for this op
 *              Argn            - Nth argument to get
 *
 * RETURN:      The argument (as an Op object).  NULL if argument does not exist
 *
 * DESCRIPTION: Get the specified op's argument.
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
acpi_ps_get_arg (
	ACPI_PARSE_OBJECT       *op,
	u32                     argn)
{
	ACPI_PARSE_OBJECT       *arg = NULL;
	ACPI_OPCODE_INFO        *op_info;


	/* Get the info structure for this opcode */

	op_info = acpi_ps_get_opcode_info (op->opcode);
	if (ACPI_GET_OP_TYPE (op_info) != ACPI_OP_TYPE_OPCODE) {
		/* Invalid opcode or ASCII character */

		return (NULL);
	}

	/* Check if this opcode requires argument sub-objects */

	if (!(ACPI_GET_OP_ARGS (op_info))) {
		/* Has no linked argument objects */

		return (NULL);
	}

	/* Get the requested argument object */

	arg = op->value.arg;
	while (arg && argn) {
		argn--;
		arg = arg->next;
	}

	return (arg);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_append_arg
 *
 * PARAMETERS:  Op              - Append an argument to this Op.
 *              Arg             - Argument Op to append
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Append an argument to an op's argument list (a NULL arg is OK)
 *
 ******************************************************************************/

void
acpi_ps_append_arg (
	ACPI_PARSE_OBJECT       *op,
	ACPI_PARSE_OBJECT       *arg)
{
	ACPI_PARSE_OBJECT       *prev_arg;
	ACPI_OPCODE_INFO        *op_info;


	if (!op) {
		return;
	}

	/* Get the info structure for this opcode */

	op_info = acpi_ps_get_opcode_info (op->opcode);
	if (ACPI_GET_OP_TYPE (op_info) != ACPI_OP_TYPE_OPCODE) {
		/* Invalid opcode */

		return;
	}

	/* Check if this opcode requires argument sub-objects */

	if (!(ACPI_GET_OP_ARGS (op_info))) {
		/* Has no linked argument objects */

		return;
	}


	/* Append the argument to the linked argument list */

	if (op->value.arg) {
		/* Append to existing argument list */

		prev_arg = op->value.arg;
		while (prev_arg->next) {
			prev_arg = prev_arg->next;
		}
		prev_arg->next = arg;
	}

	else {
		/* No argument list, this will be the first argument */

		op->value.arg = arg;
	}


	/* Set the parent in this arg and any args linked after it */

	while (arg) {
		arg->parent = op;
		arg = arg->next;
	}
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_child
 *
 * PARAMETERS:  Op              - Get the child of this Op
 *
 * RETURN:      Child Op, Null if none is found.
 *
 * DESCRIPTION: Get op's children or NULL if none
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
acpi_ps_get_child (
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_PARSE_OBJECT       *child = NULL;


	switch (op->opcode) {
	case AML_SCOPE_OP:
	case AML_ELSE_OP:
	case AML_DEVICE_OP:
	case AML_THERMAL_ZONE_OP:
	case AML_METHODCALL_OP:

		child = acpi_ps_get_arg (op, 0);
		break;


	case AML_BUFFER_OP:
	case AML_PACKAGE_OP:
	case AML_METHOD_OP:
	case AML_IF_OP:
	case AML_WHILE_OP:
	case AML_DEF_FIELD_OP:

		child = acpi_ps_get_arg (op, 1);
		break;


	case AML_POWER_RES_OP:
	case AML_INDEX_FIELD_OP:

		child = acpi_ps_get_arg (op, 2);
		break;


	case AML_PROCESSOR_OP:
	case AML_BANK_FIELD_OP:

		child = acpi_ps_get_arg (op, 3);
		break;

	}

	return (child);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ps_get_depth_next
 *
 * PARAMETERS:  Origin          - Root of subtree to search
 *              Op              - Last (previous) Op that was found
 *
 * RETURN:      Next Op found in the search.
 *
 * DESCRIPTION: Get next op in tree (walking the tree in depth-first order)
 *              Return NULL when reaching "origin" or when walking up from root
 *
 ******************************************************************************/

ACPI_PARSE_OBJECT *
acpi_ps_get_depth_next (
	ACPI_PARSE_OBJECT       *origin,
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_PARSE_OBJECT       *next = NULL;
	ACPI_PARSE_OBJECT       *parent;
	ACPI_PARSE_OBJECT       *arg;


	if (!op) {
		return (NULL);
	}

	/* look for an argument or child */

	next = acpi_ps_get_arg (op, 0);
	if (next) {
		return (next);
	}

	/* look for a sibling */

	next = op->next;
	if (next) {
		return (next);
	}

	/* look for a sibling of parent */

	parent = op->parent;

	while (parent) {
		arg = acpi_ps_get_arg (parent, 0);
		while (arg && (arg != origin) && (arg != op)) {
			arg = arg->next;
		}

		if (arg == origin) {
			/* reached parent of origin, end search */

			return (NULL);
		}

		if (parent->next) {
			/* found sibling of parent */
			return (parent->next);
		}

		op = parent;
		parent = parent->parent;
	}

	return (next);
}


