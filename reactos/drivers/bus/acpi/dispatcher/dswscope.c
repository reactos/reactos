/******************************************************************************
 *
 * Module Name: dswscope - Scope stack manipulation
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
#include "acinterp.h"
#include "acdispat.h"


#define _COMPONENT          ACPI_DISPATCHER
	 MODULE_NAME         ("dswscope")


#define STACK_POP(head) head


/****************************************************************************
 *
 * FUNCTION:    Acpi_ds_scope_stack_clear
 *
 * PARAMETERS:  None
 *
 * DESCRIPTION: Pop (and free) everything on the scope stack except the
 *              root scope object (which remains at the stack top.)
 *
 ***************************************************************************/

void
acpi_ds_scope_stack_clear (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *scope_info;


	while (walk_state->scope_info) {
		/* Pop a scope off the stack */

		scope_info = walk_state->scope_info;
		walk_state->scope_info = scope_info->scope.next;

		acpi_cm_delete_generic_state (scope_info);
	}
}


/****************************************************************************
 *
 * FUNCTION:    Acpi_ds_scope_stack_push
 *
 * PARAMETERS:  *Node,              - Name to be made current
 *              Type,               - Type of frame being pushed
 *
 * DESCRIPTION: Push the current scope on the scope stack, and make the
 *              passed Node current.
 *
 ***************************************************************************/

ACPI_STATUS
acpi_ds_scope_stack_push (
	ACPI_NAMESPACE_NODE     *node,
	OBJECT_TYPE_INTERNAL    type,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *scope_info;


	if (!node) {
		/*  invalid scope   */

		REPORT_ERROR (("Ds_scope_stack_push: null scope passed\n"));
		return (AE_BAD_PARAMETER);
	}

	/* Make sure object type is valid */

	if (!acpi_aml_validate_object_type (type)) {
		REPORT_WARNING (("Ds_scope_stack_push: type code out of range\n"));
	}


	/* Allocate a new scope object */

	scope_info = acpi_cm_create_generic_state ();
	if (!scope_info) {
		return (AE_NO_MEMORY);
	}

	/* Init new scope object */

	scope_info->scope.node = node;
	scope_info->common.value = (u16) type;

	/* Push new scope object onto stack */

	acpi_cm_push_generic_state (&walk_state->scope_info, scope_info);

	return (AE_OK);
}


/****************************************************************************
 *
 * FUNCTION:    Acpi_ds_scope_stack_pop
 *
 * PARAMETERS:  Type                - The type of frame to be found
 *
 * DESCRIPTION: Pop the scope stack until a frame of the requested type
 *              is found.
 *
 * RETURN:      Count of frames popped.  If no frame of the requested type
 *              was found, the count is returned as a negative number and
 *              the scope stack is emptied (which sets the current scope
 *              to the root).  If the scope stack was empty at entry, the
 *              function is a no-op and returns 0.
 *
 ***************************************************************************/

ACPI_STATUS
acpi_ds_scope_stack_pop (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *scope_info;


	/*
	 * Pop scope info object off the stack.
	 */

	scope_info = acpi_cm_pop_generic_state (&walk_state->scope_info);
	if (!scope_info) {
		return (AE_STACK_UNDERFLOW);
	}

	acpi_cm_delete_generic_state (scope_info);

	return (AE_OK);
}


