/*******************************************************************************
 *
 * Module Name: cmdelete - object deletion and reference count utilities
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
#include "acinterp.h"
#include "acnamesp.h"
#include "actables.h"
#include "acparser.h"

#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmdelete")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_delete_internal_obj
 *
 * PARAMETERS:  *Object        - Pointer to the list to be deleted
 *
 * RETURN:      None
 *
 * DESCRIPTION: Low level object deletion, after reference counts have been
 *              updated (All reference counts, including sub-objects!)
 *
 ******************************************************************************/

void
acpi_cm_delete_internal_obj (
	ACPI_OPERAND_OBJECT     *object)
{
	void                    *obj_pointer = NULL;
	ACPI_OPERAND_OBJECT     *handler_desc;


	if (!object) {
		return;
	}

	/*
	 * Must delete or free any pointers within the object that are not
	 * actual ACPI objects (for example, a raw buffer pointer).
	 */

	switch (object->common.type) {

	case ACPI_TYPE_STRING:

		/* Free the actual string buffer */

		obj_pointer = object->string.pointer;
		break;


	case ACPI_TYPE_BUFFER:

		/* Free the actual buffer */

		obj_pointer = object->buffer.pointer;
		break;


	case ACPI_TYPE_PACKAGE:

		/*
		 * Elements of the package are not handled here, they are deleted
		 * separately
		 */

		/* Free the (variable length) element pointer array */

		obj_pointer = object->package.elements;
		break;


	case ACPI_TYPE_MUTEX:

		acpi_aml_unlink_mutex (object);
		acpi_os_delete_semaphore (object->mutex.semaphore);
		break;


	case ACPI_TYPE_EVENT:

		acpi_os_delete_semaphore (object->event.semaphore);
		object->event.semaphore = NULL;
		break;


	case ACPI_TYPE_METHOD:

		/* Delete the method semaphore if it exists */

		if (object->method.semaphore) {
			acpi_os_delete_semaphore (object->method.semaphore);
			object->method.semaphore = NULL;
		}

		break;


	case ACPI_TYPE_REGION:


		if (object->region.extra) {
			/*
			 * Free the Region_context if and only if the handler is one of the
			 * default handlers -- and therefore, we created the context object
			 * locally, it was not created by an external caller.
			 */
			handler_desc = object->region.addr_handler;
			if ((handler_desc) &&
				(handler_desc->addr_handler.hflags == ADDR_HANDLER_DEFAULT_INSTALLED)) {
				obj_pointer = object->region.extra->extra.region_context;
			}

			/* Now we can free the Extra object */

			acpi_cm_delete_object_desc (object->region.extra);
		}
		break;


	case ACPI_TYPE_FIELD_UNIT:

		if (object->field_unit.extra) {
			acpi_cm_delete_object_desc (object->field_unit.extra);
		}
		break;

	default:
		break;
	}


	/*
	 * Delete any allocated memory found above
	 */

	if (obj_pointer) {
		if (!acpi_tb_system_table_pointer (obj_pointer)) {
			acpi_cm_free (obj_pointer);
		}
	}


	/* Only delete the object if it was dynamically allocated */


	if (!(object->common.flags & AOPOBJ_STATIC_ALLOCATION)) {
		acpi_cm_delete_object_desc (object);

	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_delete_internal_object_list
 *
 * PARAMETERS:  *Obj_list       - Pointer to the list to be deleted
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function deletes an internal object list, including both
 *              simple objects and package objects
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_delete_internal_object_list (
	ACPI_OPERAND_OBJECT     **obj_list)
{
	ACPI_OPERAND_OBJECT     **internal_obj;


	/* Walk the null-terminated internal list */

	for (internal_obj = obj_list; *internal_obj; internal_obj++) {
		/*
		 * Check for a package
		 * Simple objects are simply stored in the array and do not
		 * need to be deleted separately.
		 */

		if (IS_THIS_OBJECT_TYPE ((*internal_obj), ACPI_TYPE_PACKAGE)) {
			/* Delete the package */

			/*
			 * TBD: [Investigate] This might not be the right thing to do,
			 * depending on how the internal package object was allocated!!!
			 */
			acpi_cm_delete_internal_obj (*internal_obj);
		}

	}

	/* Free the combined parameter pointer list and object array */

	acpi_cm_free (obj_list);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_update_ref_count
 *
 * PARAMETERS:  *Object         - Object whose ref count is to be updated
 *              Action          - What to do
 *
 * RETURN:      New ref count
 *
 * DESCRIPTION: Modify the ref count and return it.
 *
 ******************************************************************************/

static void
acpi_cm_update_ref_count (
	ACPI_OPERAND_OBJECT     *object,
	u32                     action)
{
	u16                     count;
	u16                     new_count;


	if (!object) {
		return;
	}


	count = object->common.reference_count;
	new_count = count;

	/*
	 * Reference count action (increment, decrement, or force delete)
	 */

	switch (action) {

	case REF_INCREMENT:

		new_count++;
		object->common.reference_count = new_count;

		break;


	case REF_DECREMENT:

		if (count < 1) {
			new_count = 0;
		}

		else {
			new_count--;

		}


		object->common.reference_count = new_count;
		if (new_count == 0) {
			acpi_cm_delete_internal_obj (object);
		}

		break;


	case REF_FORCE_DELETE:

		new_count = 0;
		object->common.reference_count = new_count;
		acpi_cm_delete_internal_obj (object);
		break;


	default:

		break;
	}


	/*
	 * Sanity check the reference count, for debug purposes only.
	 * (A deleted object will have a huge reference count)
	 */


	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_update_object_reference
 *
 * PARAMETERS:  *Object             - Increment ref count for this object
 *                                    and all sub-objects
 *              Action              - Either REF_INCREMENT or REF_DECREMENT or
 *                                    REF_FORCE_DELETE
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Increment the object reference count
 *
 * Object references are incremented when:
 * 1) An object is attached to a Node (namespace object)
 * 2) An object is copied (all subobjects must be incremented)
 *
 * Object references are decremented when:
 * 1) An object is detached from an Node
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_update_object_reference (
	ACPI_OPERAND_OBJECT     *object,
	u16                     action)
{
	ACPI_STATUS             status;
	u32                     i;
	ACPI_OPERAND_OBJECT     *next;
	ACPI_OPERAND_OBJECT     *new;
	ACPI_GENERIC_STATE       *state_list = NULL;
	ACPI_GENERIC_STATE       *state;


	/* Ignore a null object ptr */

	if (!object) {
		return (AE_OK);
	}


	/*
	 * Make sure that this isn't a namespace handle or an AML pointer
	 */

	if (VALID_DESCRIPTOR_TYPE (object, ACPI_DESC_TYPE_NAMED)) {
		return (AE_OK);
	}

	if (acpi_tb_system_table_pointer (object)) {
		return (AE_OK);
	}


	state = acpi_cm_create_update_state (object, action);

	while (state) {

		object = state->update.object;
		action = state->update.value;
		acpi_cm_delete_generic_state (state);

		/*
		 * All sub-objects must have their reference count incremented also.
		 * Different object types have different subobjects.
		 */
		switch (object->common.type) {

		case ACPI_TYPE_DEVICE:

			status = acpi_cm_create_update_state_and_push (object->device.addr_handler,
					   action, &state_list);
			if (ACPI_FAILURE (status)) {
				return (status);
			}

			acpi_cm_update_ref_count (object->device.sys_handler, action);
			acpi_cm_update_ref_count (object->device.drv_handler, action);
			break;


		case INTERNAL_TYPE_ADDRESS_HANDLER:

			/* Must walk list of address handlers */

			next = object->addr_handler.next;
			while (next) {
				new = next->addr_handler.next;
				acpi_cm_update_ref_count (next, action);

				next = new;
			}
			break;


		case ACPI_TYPE_PACKAGE:

			/*
			 * We must update all the sub-objects of the package
			 * (Each of whom may have their own sub-objects, etc.
			 */
			for (i = 0; i < object->package.count; i++) {
				/*
				 * Push each element onto the stack for later processing.
				 * Note: There can be null elements within the package,
				 * these are simply ignored
				 */

				status = acpi_cm_create_update_state_and_push (
						 object->package.elements[i], action, &state_list);
				if (ACPI_FAILURE (status)) {
					return (status);
				}
			}
			break;


		case ACPI_TYPE_FIELD_UNIT:

			status = acpi_cm_create_update_state_and_push (
					 object->field_unit.container, action, &state_list);

			if (ACPI_FAILURE (status)) {
				return (status);
			}
			break;


		case INTERNAL_TYPE_DEF_FIELD:

			status = acpi_cm_create_update_state_and_push (
					 object->field.container, action, &state_list);
			if (ACPI_FAILURE (status)) {
				return (status);
			}
		   break;


		case INTERNAL_TYPE_BANK_FIELD:

			status = acpi_cm_create_update_state_and_push (
					 object->bank_field.bank_select, action, &state_list);
			if (ACPI_FAILURE (status)) {
				return (status);
			}

			status = acpi_cm_create_update_state_and_push (
					 object->bank_field.container, action, &state_list);
			if (ACPI_FAILURE (status)) {
				return (status);
			}
			break;


		case ACPI_TYPE_REGION:

	/* TBD: [Investigate]
			Acpi_cm_update_ref_count (Object->Region.Addr_handler, Action);
	*/
/*
			Status =
				Acpi_cm_create_update_state_and_push (Object->Region.Addr_handler,
						   Action, &State_list);
			if (ACPI_FAILURE (Status))
			{
				return (Status);
			}
*/
			break;


		case INTERNAL_TYPE_REFERENCE:

			break;
		}


		/*
		 * Now we can update the count in the main object.  This can only
		 * happen after we update the sub-objects in case this causes the
		 * main object to be deleted.
		 */

		acpi_cm_update_ref_count (object, action);


		/* Move on to the next object to be updated */

		state = acpi_cm_pop_generic_state (&state_list);
	}


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_add_reference
 *
 * PARAMETERS:  *Object        - Object whose reference count is to be
 *                                  incremented
 *
 * RETURN:      None
 *
 * DESCRIPTION: Add one reference to an ACPI object
 *
 ******************************************************************************/

void
acpi_cm_add_reference (
	ACPI_OPERAND_OBJECT     *object)
{


	/*
	 * Ensure that we have a valid object
	 */

	if (!acpi_cm_valid_internal_object (object)) {
		return;
	}

	/*
	 * We have a valid ACPI internal object, now increment the reference count
	 */

	acpi_cm_update_object_reference (object, REF_INCREMENT);

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_remove_reference
 *
 * PARAMETERS:  *Object        - Object whose ref count will be decremented
 *
 * RETURN:      None
 *
 * DESCRIPTION: Decrement the reference count of an ACPI internal object
 *
 ******************************************************************************/

void
acpi_cm_remove_reference (
	ACPI_OPERAND_OBJECT     *object)
{


	/*
	 * Ensure that we have a valid object
	 */

	if (!acpi_cm_valid_internal_object (object)) {
		return;
	}

	/*
	 * Decrement the reference count, and only actually delete the object
	 * if the reference count becomes 0.  (Must also decrement the ref count
	 * of all subobjects!)
	 */

	acpi_cm_update_object_reference (object, REF_DECREMENT);

	return;
}


