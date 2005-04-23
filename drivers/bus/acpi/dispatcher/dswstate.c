/******************************************************************************
 *
 * Module Name: dswstate - Dispatcher parse tree walk management routines
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
#include "amlcode.h"
#include "acparser.h"
#include "acdispat.h"
#include "acnamesp.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_DISPATCHER
	 MODULE_NAME         ("dswstate")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_result_insert
 *
 * PARAMETERS:  Object              - Object to push
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Push an object onto this walk's result stack
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_result_insert (
	void                    *object,
	u32                     index,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *state;


	state = walk_state->results;
	if (!state) {
		return (AE_NOT_EXIST);
	}

	if (index >= OBJ_NUM_OPERANDS) {
		return (AE_BAD_PARAMETER);
	}

	if (!object) {
		return (AE_BAD_PARAMETER);
	}

	state->results.obj_desc [index] = object;
	state->results.num_results++;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_result_remove
 *
 * PARAMETERS:  Object              - Where to return the popped object
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Pop an object off the bottom of this walk's result stack.  In
 *              other words, this is a FIFO.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_result_remove (
	ACPI_OPERAND_OBJECT     **object,
	u32                     index,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *state;


	state = walk_state->results;
	if (!state) {
		return (AE_NOT_EXIST);
	}



	/* Check for a valid result object */

	if (!state->results.obj_desc [index]) {
		return (AE_AML_NO_RETURN_VALUE);
	}

	/* Remove the object */

	state->results.num_results--;

	*object = state->results.obj_desc [index];
	state->results.obj_desc [index] = NULL;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_result_pop
 *
 * PARAMETERS:  Object              - Where to return the popped object
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Pop an object off the bottom of this walk's result stack.  In
 *              other words, this is a FIFO.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_result_pop (
	ACPI_OPERAND_OBJECT     **object,
	ACPI_WALK_STATE         *walk_state)
{
	u32                     index;
	ACPI_GENERIC_STATE      *state;


	state = walk_state->results;
	if (!state) {
		return (AE_OK);
	}


	if (!state->results.num_results) {
		return (AE_AML_NO_RETURN_VALUE);
	}

	/* Remove top element */

	state->results.num_results--;

	for (index = OBJ_NUM_OPERANDS; index; index--) {
		/* Check for a valid result object */

		if (state->results.obj_desc [index -1]) {
			*object = state->results.obj_desc [index -1];
			state->results.obj_desc [index -1] = NULL;

			return (AE_OK);
		}
	}


	return (AE_AML_NO_RETURN_VALUE);
}

/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_result_pop_from_bottom
 *
 * PARAMETERS:  Object              - Where to return the popped object
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Pop an object off the bottom of this walk's result stack.  In
 *              other words, this is a FIFO.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_result_pop_from_bottom (
	ACPI_OPERAND_OBJECT     **object,
	ACPI_WALK_STATE         *walk_state)
{
	u32                     index;
	ACPI_GENERIC_STATE      *state;


	state = walk_state->results;
	if (!state) {
		return (AE_NOT_EXIST);
	}


	if (!state->results.num_results) {
		return (AE_AML_NO_RETURN_VALUE);
	}

	/* Remove Bottom element */

	*object = state->results.obj_desc [0];


	/* Push entire stack down one element */

	for (index = 0; index < state->results.num_results; index++) {
		state->results.obj_desc [index] = state->results.obj_desc [index + 1];
	}

	state->results.num_results--;

	/* Check for a valid result object */

	if (!*object) {
		return (AE_AML_NO_RETURN_VALUE);
	}


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_result_push
 *
 * PARAMETERS:  Object              - Where to return the popped object
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Push an object onto the current result stack
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_result_push (
	ACPI_OPERAND_OBJECT     *object,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *state;


	state = walk_state->results;
	if (!state) {
		return (AE_AML_INTERNAL);
	}

	if (state->results.num_results == OBJ_NUM_OPERANDS) {
		return (AE_STACK_OVERFLOW);
	}

	if (!object) {
		return (AE_BAD_PARAMETER);
	}


	state->results.obj_desc [state->results.num_results] = object;
	state->results.num_results++;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_result_stack_push
 *
 * PARAMETERS:  Object              - Object to push
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION:
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_result_stack_push (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *state;


	state = acpi_cm_create_generic_state ();
	if (!state) {
		return (AE_NO_MEMORY);
	}

	acpi_cm_push_generic_state (&walk_state->results, state);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_result_stack_pop
 *
 * PARAMETERS:  Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION:
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_result_stack_pop (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *state;


	/* Check for stack underflow */

	if (walk_state->results == NULL) {
		return (AE_AML_NO_OPERAND);
	}


	state = acpi_cm_pop_generic_state (&walk_state->results);

	acpi_cm_delete_generic_state (state);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_obj_stack_delete_all
 *
 * PARAMETERS:  Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear the object stack by deleting all objects that are on it.
 *              Should be used with great care, if at all!
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_obj_stack_delete_all (
	ACPI_WALK_STATE         *walk_state)
{
	u32                     i;


	/* The stack size is configurable, but fixed */

	for (i = 0; i < OBJ_NUM_OPERANDS; i++) {
		if (walk_state->operands[i]) {
			acpi_cm_remove_reference (walk_state->operands[i]);
			walk_state->operands[i] = NULL;
		}
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_obj_stack_push
 *
 * PARAMETERS:  Object              - Object to push
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Push an object onto this walk's object/operand stack
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_obj_stack_push (
	void                    *object,
	ACPI_WALK_STATE         *walk_state)
{


	/* Check for stack overflow */

	if (walk_state->num_operands >= OBJ_NUM_OPERANDS) {
		return (AE_STACK_OVERFLOW);
	}

	/* Put the object onto the stack */

	walk_state->operands [walk_state->num_operands] = object;
	walk_state->num_operands++;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_obj_stack_pop_object
 *
 * PARAMETERS:  Pop_count           - Number of objects/entries to pop
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Pop this walk's object stack.  Objects on the stack are NOT
 *              deleted by this routine.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_obj_stack_pop_object (
	ACPI_OPERAND_OBJECT     **object,
	ACPI_WALK_STATE         *walk_state)
{


	/* Check for stack underflow */

	if (walk_state->num_operands == 0) {
		return (AE_AML_NO_OPERAND);
	}


	/* Pop the stack */

	walk_state->num_operands--;

	/* Check for a valid operand */

	if (!walk_state->operands [walk_state->num_operands]) {
		return (AE_AML_NO_OPERAND);
	}

	/* Get operand and set stack entry to null */

	*object = walk_state->operands [walk_state->num_operands];
	walk_state->operands [walk_state->num_operands] = NULL;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_obj_stack_pop
 *
 * PARAMETERS:  Pop_count           - Number of objects/entries to pop
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Pop this walk's object stack.  Objects on the stack are NOT
 *              deleted by this routine.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_obj_stack_pop (
	u32                     pop_count,
	ACPI_WALK_STATE         *walk_state)
{
	u32                     i;


	for (i = 0; i < pop_count; i++) {
		/* Check for stack underflow */

		if (walk_state->num_operands == 0) {
			return (AE_STACK_UNDERFLOW);
		}

		/* Just set the stack entry to null */

		walk_state->num_operands--;
		walk_state->operands [walk_state->num_operands] = NULL;
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_obj_stack_pop_and_delete
 *
 * PARAMETERS:  Pop_count           - Number of objects/entries to pop
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Pop this walk's object stack and delete each object that is
 *              popped off.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_obj_stack_pop_and_delete (
	u32                     pop_count,
	ACPI_WALK_STATE         *walk_state)
{
	u32                     i;
	ACPI_OPERAND_OBJECT     *obj_desc;


	for (i = 0; i < pop_count; i++) {
		/* Check for stack underflow */

		if (walk_state->num_operands == 0) {
			return (AE_STACK_UNDERFLOW);
		}

		/* Pop the stack and delete an object if present in this stack entry */

		walk_state->num_operands--;
		obj_desc = walk_state->operands [walk_state->num_operands];
		if (obj_desc) {
			acpi_cm_remove_reference (walk_state->operands [walk_state->num_operands]);
			walk_state->operands [walk_state->num_operands] = NULL;
		}
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_obj_stack_get_value
 *
 * PARAMETERS:  Index               - Stack index whose value is desired.  Based
 *                                    on the top of the stack (index=0 == top)
 *              Walk_state          - Current Walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve an object from this walk's object stack.  Index must
 *              be within the range of the current stack pointer.
 *
 ******************************************************************************/

void *
acpi_ds_obj_stack_get_value (
	u32                     index,
	ACPI_WALK_STATE         *walk_state)
{


	/* Can't do it if the stack is empty */

	if (walk_state->num_operands == 0) {
		return (NULL);
	}

	/* or if the index is past the top of the stack */

	if (index > (walk_state->num_operands - (u32) 1)) {
		return (NULL);
	}


	return (walk_state->operands[(NATIVE_UINT)(walk_state->num_operands - 1) -
			  index]);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_get_current_walk_state
 *
 * PARAMETERS:  Walk_list       - Get current active state for this walk list
 *
 * RETURN:      Pointer to the current walk state
 *
 * DESCRIPTION: Get the walk state that is at the head of the list (the "current"
 *              walk state.
 *
 ******************************************************************************/

ACPI_WALK_STATE *
acpi_ds_get_current_walk_state (
	ACPI_WALK_LIST          *walk_list)

{

	if (!walk_list) {
		return (NULL);
	}

	return (walk_list->walk_state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_push_walk_state
 *
 * PARAMETERS:  Walk_state      - State to push
 *              Walk_list       - The list that owns the walk stack
 *
 * RETURN:      None
 *
 * DESCRIPTION: Place the Walk_state at the head of the state list.
 *
 ******************************************************************************/

static void
acpi_ds_push_walk_state (
	ACPI_WALK_STATE         *walk_state,
	ACPI_WALK_LIST          *walk_list)
{


	walk_state->next    = walk_list->walk_state;
	walk_list->walk_state = walk_state;

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_pop_walk_state
 *
 * PARAMETERS:  Walk_list       - The list that owns the walk stack
 *
 * RETURN:      A Walk_state object popped from the stack
 *
 * DESCRIPTION: Remove and return the walkstate object that is at the head of
 *              the walk stack for the given walk list.  NULL indicates that
 *              the list is empty.
 *
 ******************************************************************************/

ACPI_WALK_STATE *
acpi_ds_pop_walk_state (
	ACPI_WALK_LIST          *walk_list)
{
	ACPI_WALK_STATE         *walk_state;


	walk_state = walk_list->walk_state;

	if (walk_state) {
		/* Next walk state becomes the current walk state */

		walk_list->walk_state = walk_state->next;

		/*
		 * Don't clear the NEXT field, this serves as an indicator
		 * that there is a parent WALK STATE
		 *     Walk_state->Next = NULL;
		 */
	}

	return (walk_state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_create_walk_state
 *
 * PARAMETERS:  Origin          - Starting point for this walk
 *              Walk_list       - Owning walk list
 *
 * RETURN:      Pointer to the new walk state.
 *
 * DESCRIPTION: Allocate and initialize a new walk state.  The current walk state
 *              is set to this new state.
 *
 ******************************************************************************/

ACPI_WALK_STATE *
acpi_ds_create_walk_state (
	ACPI_OWNER_ID           owner_id,
	ACPI_PARSE_OBJECT       *origin,
	ACPI_OPERAND_OBJECT     *mth_desc,
	ACPI_WALK_LIST          *walk_list)
{
	ACPI_WALK_STATE         *walk_state;
	ACPI_STATUS             status;


	acpi_cm_acquire_mutex (ACPI_MTX_CACHES);
	acpi_gbl_walk_state_cache_requests++;

	/* Check the cache first */

	if (acpi_gbl_walk_state_cache) {
		/* There is an object available, use it */

		walk_state = acpi_gbl_walk_state_cache;
		acpi_gbl_walk_state_cache = walk_state->next;

		acpi_gbl_walk_state_cache_hits++;
		acpi_gbl_walk_state_cache_depth--;

		acpi_cm_release_mutex (ACPI_MTX_CACHES);
	}

	else {
		/* The cache is empty, create a new object */

		/* Avoid deadlock with Acpi_cm_callocate */

		acpi_cm_release_mutex (ACPI_MTX_CACHES);

		walk_state = acpi_cm_callocate (sizeof (ACPI_WALK_STATE));
		if (!walk_state) {
			return (NULL);
		}
	}

	walk_state->data_type       = ACPI_DESC_TYPE_WALK;
	walk_state->owner_id        = owner_id;
	walk_state->origin          = origin;
	walk_state->method_desc     = mth_desc;
	walk_state->walk_list       = walk_list;

	/* Init the method args/local */

#ifndef _ACPI_ASL_COMPILER
	acpi_ds_method_data_init (walk_state);
#endif

	/* Create an initial result stack entry */

	status = acpi_ds_result_stack_push (walk_state);
	if (ACPI_FAILURE (status)) {
		return (NULL);
	}


	/* Put the new state at the head of the walk list */

	acpi_ds_push_walk_state (walk_state, walk_list);

	return (walk_state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_delete_walk_state
 *
 * PARAMETERS:  Walk_state      - State to delete
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete a walk state including all internal data structures
 *
 ******************************************************************************/

void
acpi_ds_delete_walk_state (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_GENERIC_STATE      *state;


	if (!walk_state) {
		return;
	}

	if (walk_state->data_type != ACPI_DESC_TYPE_WALK) {
		return;
	}


	/* Always must free any linked control states */

	while (walk_state->control_state) {
		state = walk_state->control_state;
		walk_state->control_state = state->common.next;

		acpi_cm_delete_generic_state (state);
	}

	/* Always must free any linked parse states */

	while (walk_state->scope_info) {
		state = walk_state->scope_info;
		walk_state->scope_info = state->common.next;

		acpi_cm_delete_generic_state (state);
	}

	/* Always must free any stacked result states */

	while (walk_state->results) {
		state = walk_state->results;
		walk_state->results = state->common.next;

		acpi_cm_delete_generic_state (state);
	}


	/* If walk cache is full, just free this wallkstate object */

	if (acpi_gbl_walk_state_cache_depth >= MAX_WALK_CACHE_DEPTH) {
		acpi_cm_free (walk_state);
	}

	/* Otherwise put this object back into the cache */

	else {
		acpi_cm_acquire_mutex (ACPI_MTX_CACHES);

		/* Clear the state */

		MEMSET (walk_state, 0, sizeof (ACPI_WALK_STATE));
		walk_state->data_type = ACPI_DESC_TYPE_WALK;

		/* Put the object at the head of the global cache list */

		walk_state->next = acpi_gbl_walk_state_cache;
		acpi_gbl_walk_state_cache = walk_state;
		acpi_gbl_walk_state_cache_depth++;


		acpi_cm_release_mutex (ACPI_MTX_CACHES);
	}

	return;
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ds_delete_walk_state_cache
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Purge the global state object cache.  Used during subsystem
 *              termination.
 *
 ******************************************************************************/

void
acpi_ds_delete_walk_state_cache (
	void)
{
	ACPI_WALK_STATE         *next;


	/* Traverse the global cache list */

	while (acpi_gbl_walk_state_cache) {
		/* Delete one cached state object */

		next = acpi_gbl_walk_state_cache->next;
		acpi_cm_free (acpi_gbl_walk_state_cache);
		acpi_gbl_walk_state_cache = next;
		acpi_gbl_walk_state_cache_depth--;
	}

	return;
}


