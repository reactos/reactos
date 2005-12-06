/*******************************************************************************
 *
 * Module Name: cmutils - common utility procedures
 *              $Revision: 1.1 $
 *
 ******************************************************************************/

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
#include "acevents.h"
#include "achware.h"
#include "acnamesp.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acdebug.h"


#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmutils")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_valid_acpi_name
 *
 * PARAMETERS:  Character           - The character to be examined
 *
 * RETURN:      1 if Character may appear in a name, else 0
 *
 * DESCRIPTION: Check for a valid ACPI name.  Each character must be one of:
 *              1) Upper case alpha
 *              2) numeric
 *              3) underscore
 *
 ******************************************************************************/

u8
acpi_cm_valid_acpi_name (
	u32                     name)
{
	NATIVE_CHAR             *name_ptr = (NATIVE_CHAR *) &name;
	u32                     i;


	for (i = 0; i < ACPI_NAME_SIZE; i++) {
		if (!((name_ptr[i] == '_') ||
			  (name_ptr[i] >= 'A' && name_ptr[i] <= 'Z') ||
			  (name_ptr[i] >= '0' && name_ptr[i] <= '9'))) {
			return (FALSE);
		}
	}


	return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_valid_acpi_character
 *
 * PARAMETERS:  Character           - The character to be examined
 *
 * RETURN:      1 if Character may appear in a name, else 0
 *
 * DESCRIPTION: Check for a printable character
 *
 ******************************************************************************/

u8
acpi_cm_valid_acpi_character (
	NATIVE_CHAR             character)
{

	return ((u8)   ((character == '_') ||
			   (character >= 'A' && character <= 'Z') ||
			   (character >= '0' && character <= '9')));
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_mutex_initialize
 *
 * PARAMETERS:  None.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create the system mutex objects.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_mutex_initialize (
	void)
{
	u32                     i;
	ACPI_STATUS             status;


	/*
	 * Create each of the predefined mutex objects
	 */
	for (i = 0; i < NUM_MTX; i++) {
		status = acpi_cm_create_mutex (i);
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_mutex_terminate
 *
 * PARAMETERS:  None.
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete all of the system mutex objects.
 *
 ******************************************************************************/

void
acpi_cm_mutex_terminate (
	void)
{
	u32                     i;


	/*
	 * Delete each predefined mutex object
	 */
	for (i = 0; i < NUM_MTX; i++) {
		acpi_cm_delete_mutex (i);
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_create_mutex
 *
 * PARAMETERS:  Mutex_iD        - ID of the mutex to be created
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a mutex object.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_create_mutex (
	ACPI_MUTEX_HANDLE       mutex_id)
{
	ACPI_STATUS             status = AE_OK;


	if (mutex_id > MAX_MTX) {
		return (AE_BAD_PARAMETER);
	}


	if (!acpi_gbl_acpi_mutex_info[mutex_id].mutex) {
		status = acpi_os_create_semaphore (1, 1,
				   &acpi_gbl_acpi_mutex_info[mutex_id].mutex);
		acpi_gbl_acpi_mutex_info[mutex_id].locked = FALSE;
		acpi_gbl_acpi_mutex_info[mutex_id].use_count = 0;
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_delete_mutex
 *
 * PARAMETERS:  Mutex_iD        - ID of the mutex to be deleted
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete a mutex object.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_delete_mutex (
	ACPI_MUTEX_HANDLE       mutex_id)
{
	ACPI_STATUS             status;


	if (mutex_id > MAX_MTX) {
		return (AE_BAD_PARAMETER);
	}


	status = acpi_os_delete_semaphore (acpi_gbl_acpi_mutex_info[mutex_id].mutex);

	acpi_gbl_acpi_mutex_info[mutex_id].mutex = NULL;
	acpi_gbl_acpi_mutex_info[mutex_id].locked = FALSE;

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_acquire_mutex
 *
 * PARAMETERS:  Mutex_iD        - ID of the mutex to be acquired
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire a mutex object.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_acquire_mutex (
	ACPI_MUTEX_HANDLE       mutex_id)
{
	ACPI_STATUS             status;
	u32                     i;
	u32                     this_thread_id;


	if (mutex_id > MAX_MTX) {
		return (AE_BAD_PARAMETER);
	}


	this_thread_id = acpi_os_get_thread_id ();

	/*
	 * Deadlock prevention.  Check if this thread owns any mutexes of value
	 * greater than or equal to this one.  If so, the thread has violated
	 * the mutex ordering rule.  This indicates a coding error somewhere in
	 * the ACPI subsystem code.
	 */
	for (i = mutex_id; i < MAX_MTX; i++) {
		if (acpi_gbl_acpi_mutex_info[i].owner_id == this_thread_id) {
			if (i == mutex_id) {
				return (AE_ALREADY_ACQUIRED);
			}

			return (AE_ACQUIRE_DEADLOCK);
		}
	}


	status = acpi_os_wait_semaphore (acpi_gbl_acpi_mutex_info[mutex_id].mutex,
			   1, WAIT_FOREVER);

	if (ACPI_SUCCESS (status)) {
		acpi_gbl_acpi_mutex_info[mutex_id].locked = TRUE;
		acpi_gbl_acpi_mutex_info[mutex_id].use_count++;
		acpi_gbl_acpi_mutex_info[mutex_id].owner_id = this_thread_id;
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_release_mutex
 *
 * PARAMETERS:  Mutex_iD        - ID of the mutex to be released
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Release a mutex object.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_release_mutex (
	ACPI_MUTEX_HANDLE       mutex_id)
{
	ACPI_STATUS             status;
	u32                     i;
	u32                     this_thread_id;


	if (mutex_id > MAX_MTX) {
		return (AE_BAD_PARAMETER);
	}


	/*
	 * Mutex must be acquired in order to release it!
	 */
	if (!acpi_gbl_acpi_mutex_info[mutex_id].locked) {
		return (AE_NOT_ACQUIRED);
	}


	/*
	 * Deadlock prevention.  Check if this thread owns any mutexes of value
	 * greater than this one.  If so, the thread has violated
	 * the mutex ordering rule.  This indicates a coding error somewhere in
	 * the ACPI subsystem code.
	 */
	this_thread_id = acpi_os_get_thread_id ();
	for (i = mutex_id; i < MAX_MTX; i++) {
		if (acpi_gbl_acpi_mutex_info[i].owner_id == this_thread_id) {
			if (i == mutex_id) {
				continue;
			}

			return (AE_RELEASE_DEADLOCK);
		}
	}

	acpi_gbl_acpi_mutex_info[mutex_id].locked = FALSE; /* Mark before unlocking */
	acpi_gbl_acpi_mutex_info[mutex_id].owner_id = 0;

	status = acpi_os_signal_semaphore (acpi_gbl_acpi_mutex_info[mutex_id].mutex, 1);


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_create_update_state_and_push
 *
 * PARAMETERS:  *Object         - Object to be added to the new state
 *              Action          - Increment/Decrement
 *              State_list      - List the state will be added to
 *
 * RETURN:      None
 *
 * DESCRIPTION: Create a new state and push it
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_create_update_state_and_push (
	ACPI_OPERAND_OBJECT     *object,
	u16                     action,
	ACPI_GENERIC_STATE      **state_list)
{
	ACPI_GENERIC_STATE       *state;


	/* Ignore null objects; these are expected */

	if (!object) {
		return (AE_OK);
	}

	state = acpi_cm_create_update_state (object, action);
	if (!state) {
		return (AE_NO_MEMORY);
	}


	acpi_cm_push_generic_state (state_list, state);
	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_create_pkg_state_and_push
 *
 * PARAMETERS:  *Object         - Object to be added to the new state
 *              Action          - Increment/Decrement
 *              State_list      - List the state will be added to
 *
 * RETURN:      None
 *
 * DESCRIPTION: Create a new state and push it
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_create_pkg_state_and_push (
	void                    *internal_object,
	void                    *external_object,
	u16                     index,
	ACPI_GENERIC_STATE      **state_list)
{
	ACPI_GENERIC_STATE       *state;


	state = acpi_cm_create_pkg_state (internal_object, external_object, index);
	if (!state) {
		return (AE_NO_MEMORY);
	}


	acpi_cm_push_generic_state (state_list, state);
	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_push_generic_state
 *
 * PARAMETERS:  List_head           - Head of the state stack
 *              State               - State object to push
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Push a state object onto a state stack
 *
 ******************************************************************************/

void
acpi_cm_push_generic_state (
	ACPI_GENERIC_STATE      **list_head,
	ACPI_GENERIC_STATE      *state)
{
	/* Push the state object onto the front of the list (stack) */

	state->common.next = *list_head;
	*list_head = state;

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_pop_generic_state
 *
 * PARAMETERS:  List_head           - Head of the state stack
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Pop a state object from a state stack
 *
 ******************************************************************************/

ACPI_GENERIC_STATE *
acpi_cm_pop_generic_state (
	ACPI_GENERIC_STATE      **list_head)
{
	ACPI_GENERIC_STATE      *state;


	/* Remove the state object at the head of the list (stack) */

	state = *list_head;
	if (state) {
		/* Update the list head */

		*list_head = state->common.next;
	}

	return (state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_create_generic_state
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a generic state object.  Attempt to obtain one from
 *              the global state cache;  If none available, create a new one.
 *
 ******************************************************************************/

ACPI_GENERIC_STATE *
acpi_cm_create_generic_state (void)
{
	ACPI_GENERIC_STATE      *state;


	acpi_cm_acquire_mutex (ACPI_MTX_CACHES);

	acpi_gbl_state_cache_requests++;

	/* Check the cache first */

	if (acpi_gbl_generic_state_cache) {
		/* There is an object available, use it */

		state = acpi_gbl_generic_state_cache;
		acpi_gbl_generic_state_cache = state->common.next;
		state->common.next = NULL;

		acpi_gbl_state_cache_hits++;
		acpi_gbl_generic_state_cache_depth--;

		acpi_cm_release_mutex (ACPI_MTX_CACHES);

	}

	else {
		/* The cache is empty, create a new object */

		acpi_cm_release_mutex (ACPI_MTX_CACHES);

		state = acpi_cm_callocate (sizeof (ACPI_GENERIC_STATE));
	}

	/* Initialize */

	if (state) {
		/* Always zero out the object before init */

		MEMSET (state, 0, sizeof (ACPI_GENERIC_STATE));

		state->common.data_type = ACPI_DESC_TYPE_STATE;
	}

	return (state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_create_update_state
 *
 * PARAMETERS:  Object              - Initial Object to be installed in the
 *                                    state
 *              Action              - Update action to be performed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create an "Update State" - a flavor of the generic state used
 *              to update reference counts and delete complex objects such
 *              as packages.
 *
 ******************************************************************************/

ACPI_GENERIC_STATE *
acpi_cm_create_update_state (
	ACPI_OPERAND_OBJECT     *object,
	u16                     action)
{
	ACPI_GENERIC_STATE      *state;


	/* Create the generic state object */

	state = acpi_cm_create_generic_state ();
	if (!state) {
		return (NULL);
	}

	/* Init fields specific to the update struct */

	state->update.object = object;
	state->update.value  = action;

	return (state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_create_pkg_state
 *
 * PARAMETERS:  Object              - Initial Object to be installed in the
 *                                    state
 *              Action              - Update action to be performed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create an "Update State" - a flavor of the generic state used
 *              to update reference counts and delete complex objects such
 *              as packages.
 *
 ******************************************************************************/

ACPI_GENERIC_STATE *
acpi_cm_create_pkg_state (
	void                    *internal_object,
	void                    *external_object,
	u16                     index)
{
	ACPI_GENERIC_STATE      *state;


	/* Create the generic state object */

	state = acpi_cm_create_generic_state ();
	if (!state) {
		return (NULL);
	}

	/* Init fields specific to the update struct */

	state->pkg.source_object = (ACPI_OPERAND_OBJECT *) internal_object;
	state->pkg.dest_object  = external_object;
	state->pkg.index        = index;
	state->pkg.num_packages = 1;

	return (state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_create_control_state
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a "Control State" - a flavor of the generic state used
 *              to support nested IF/WHILE constructs in the AML.
 *
 ******************************************************************************/

ACPI_GENERIC_STATE *
acpi_cm_create_control_state (
	void)
{
	ACPI_GENERIC_STATE      *state;


	/* Create the generic state object */

	state = acpi_cm_create_generic_state ();
	if (!state) {
		return (NULL);
	}


	/* Init fields specific to the control struct */

	state->common.state = CONTROL_CONDITIONAL_EXECUTING;

	return (state);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_delete_generic_state
 *
 * PARAMETERS:  State               - The state object to be deleted
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Put a state object back into the global state cache.  The object
 *              is not actually freed at this time.
 *
 ******************************************************************************/

void
acpi_cm_delete_generic_state (
	ACPI_GENERIC_STATE      *state)
{

	/* If cache is full, just free this state object */

	if (acpi_gbl_generic_state_cache_depth >= MAX_STATE_CACHE_DEPTH) {
		acpi_cm_free (state);
	}

	/* Otherwise put this object back into the cache */

	else {
		acpi_cm_acquire_mutex (ACPI_MTX_CACHES);

		/* Clear the state */

		MEMSET (state, 0, sizeof (ACPI_GENERIC_STATE));
		state->common.data_type = ACPI_DESC_TYPE_STATE;

		/* Put the object at the head of the global cache list */

		state->common.next = acpi_gbl_generic_state_cache;
		acpi_gbl_generic_state_cache = state;
		acpi_gbl_generic_state_cache_depth++;


		acpi_cm_release_mutex (ACPI_MTX_CACHES);
	}
	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_delete_generic_state_cache
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
acpi_cm_delete_generic_state_cache (
	void)
{
	ACPI_GENERIC_STATE      *next;


	/* Traverse the global cache list */

	while (acpi_gbl_generic_state_cache) {
		/* Delete one cached state object */

		next = acpi_gbl_generic_state_cache->common.next;
		acpi_cm_free (acpi_gbl_generic_state_cache);
		acpi_gbl_generic_state_cache = next;
		acpi_gbl_generic_state_cache_depth--;
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_resolve_package_references
 *
 * PARAMETERS:  Obj_desc        - The Package object on which to resolve refs
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk through a package and turn internal references into values
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_resolve_package_references (
	ACPI_OPERAND_OBJECT     *obj_desc)
{
	u32                     count;
	ACPI_OPERAND_OBJECT     *sub_object;


	if (obj_desc->common.type != ACPI_TYPE_PACKAGE) {
		/* The object must be a package */

		REPORT_ERROR (("Must resolve Package Refs on a Package\n"));
		return(AE_ERROR);
	}

	/*
	 * TBD: what about nested packages? */

	for (count = 0; count < obj_desc->package.count; count++) {
		sub_object = obj_desc->package.elements[count];

		if (sub_object->common.type == INTERNAL_TYPE_REFERENCE) {
			if (sub_object->reference.opcode == AML_ZERO_OP) {
				sub_object->common.type = ACPI_TYPE_INTEGER;
				sub_object->integer.value = 0;
			}

			else if (sub_object->reference.opcode == AML_ONE_OP) {
				sub_object->common.type = ACPI_TYPE_INTEGER;
				sub_object->integer.value = 1;
			}

			else if (sub_object->reference.opcode == AML_ONES_OP) {
				sub_object->common.type = ACPI_TYPE_INTEGER;
				sub_object->integer.value = ACPI_INTEGER_MAX;
			}
		}
	}

	return(AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_walk_package_tree
 *
 * PARAMETERS:  Obj_desc        - The Package object on which to resolve refs
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk through a package
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_walk_package_tree (
	ACPI_OPERAND_OBJECT     *source_object,
	void                    *target_object,
	ACPI_PKG_CALLBACK       walk_callback,
	void                    *context)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_GENERIC_STATE      *state_list = NULL;
	ACPI_GENERIC_STATE      *state;
	u32                     this_index;
	ACPI_OPERAND_OBJECT     *this_source_obj;


	state = acpi_cm_create_pkg_state (source_object, target_object, 0);
	if (!state) {
		return (AE_NO_MEMORY);
	}

	while (state) {
		this_index    = state->pkg.index;
		this_source_obj = (ACPI_OPERAND_OBJECT *)
				  state->pkg.source_object->package.elements[this_index];

		/*
		 * Check for
		 * 1) An uninitialized package element.  It is completely
		 *      legal to declare a package and leave it uninitialized
		 * 2) Not an internal object - can be a namespace node instead
		 * 3) Any type other than a package.  Packages are handled in else
		 *      case below.
		 */
		if ((!this_source_obj) ||
			(!VALID_DESCRIPTOR_TYPE (
					this_source_obj, ACPI_DESC_TYPE_INTERNAL)) ||
			(!IS_THIS_OBJECT_TYPE (
					this_source_obj, ACPI_TYPE_PACKAGE))) {

			status = walk_callback (ACPI_COPY_TYPE_SIMPLE, this_source_obj,
					 state, context);
			if (ACPI_FAILURE (status)) {
				/* TBD: must delete package created up to this point */

				return (status);
			}

			state->pkg.index++;
			while (state->pkg.index >= state->pkg.source_object->package.count) {
				/*
				 * We've handled all of the objects at this level,  This means
				 * that we have just completed a package.  That package may
				 * have contained one or more packages itself.
				 *
				 * Delete this state and pop the previous state (package).
				 */
				acpi_cm_delete_generic_state (state);
				state = acpi_cm_pop_generic_state (&state_list);


				/* Finished when there are no more states */

				if (!state) {
					/*
					 * We have handled all of the objects in the top level
					 * package just add the length of the package objects
					 * and exit
					 */
					return (AE_OK);
				}

				/*
				 * Go back up a level and move the index past the just
				 * completed package object.
				 */
				state->pkg.index++;
			}
		}

		else {
			/* This is a sub-object of type package */

			status = walk_callback (ACPI_COPY_TYPE_PACKAGE, this_source_obj,
					  state, context);
			if (ACPI_FAILURE (status)) {
				/* TBD: must delete package created up to this point */

				return (status);
			}


			/*
			 * The callback above returned a new target package object.
			 */

			/*
			 * Push the current state and create a new one
			 */
			acpi_cm_push_generic_state (&state_list, state);
			state = acpi_cm_create_pkg_state (this_source_obj,
					   state->pkg.this_target_obj, 0);
			if (!state) {
				/* TBD: must delete package created up to this point */

				return (AE_NO_MEMORY);
			}
		}
	}

	/* We should never get here */

	return (AE_AML_INTERNAL);

}


/*******************************************************************************
 *
 * FUNCTION:    _Report_error
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Message             - Error message to use on failure
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print error message
 *
 ******************************************************************************/

void
_report_error (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id)
{


	acpi_os_printf ("%8s-%04d: *** Error: ", module_name, line_number);
}


/*******************************************************************************
 *
 * FUNCTION:    _Report_warning
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Message             - Error message to use on failure
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print warning message
 *
 ******************************************************************************/

void
_report_warning (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id)
{

	acpi_os_printf ("%8s-%04d: *** Warning: ", module_name, line_number);
}


/*******************************************************************************
 *
 * FUNCTION:    _Report_info
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Message             - Error message to use on failure
 *
 * RETURN:      None
 *
 * DESCRIPTION: Print information message
 *
 ******************************************************************************/

void
_report_info (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id)
{

	acpi_os_printf ("%8s-%04d: *** Info: ", module_name, line_number);
}


