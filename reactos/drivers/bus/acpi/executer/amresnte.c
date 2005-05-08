
/******************************************************************************
 *
 * Module Name: amresnte - AML Interpreter object resolution
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
#include "acinterp.h"
#include "acnamesp.h"
#include "actables.h"
#include "acevents.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amresnte")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_resolve_node_to_value
 *
 * PARAMETERS:  Stack_ptr       - Pointer to a location on a stack that contains
 *                                a pointer to an Node
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Resolve a ACPI_NAMESPACE_NODE (Node,
 *              A.K.A. a "direct name pointer")
 *
 * Note: for some of the data types, the pointer attached to the Node
 * can be either a pointer to an actual internal object or a pointer into the
 * AML stream itself.  These types are currently:
 *
 *      ACPI_TYPE_INTEGER
 *      ACPI_TYPE_STRING
 *      ACPI_TYPE_BUFFER
 *      ACPI_TYPE_MUTEX
 *      ACPI_TYPE_PACKAGE
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_resolve_node_to_value (
	ACPI_NAMESPACE_NODE     **stack_ptr,
	ACPI_WALK_STATE         *walk_state)

{
	ACPI_STATUS             status = AE_OK;
	ACPI_OPERAND_OBJECT     *val_desc = NULL;
	ACPI_OPERAND_OBJECT     *obj_desc = NULL;
	ACPI_NAMESPACE_NODE     *node;
	u8                      *aml_pointer = NULL;
	OBJECT_TYPE_INTERNAL    entry_type;
	u8                      locked;
	u8                      attached_aml_pointer = FALSE;
	u8                      aml_opcode = 0;
	ACPI_INTEGER            temp_val;
	OBJECT_TYPE_INTERNAL    object_type;


	node = *stack_ptr;


	/*
	 * The stack pointer is a "Direct name ptr", and points to a
	 * a ACPI_NAMESPACE_NODE (Node).  Get the pointer that is attached to
	 * the Node.
	 */

	val_desc    = acpi_ns_get_attached_object ((ACPI_HANDLE) node);
	entry_type  = acpi_ns_get_type ((ACPI_HANDLE) node);

	/*
	 * The Val_desc attached to the Node can be either:
	 * 1) An internal ACPI object
	 * 2) A pointer into the AML stream (into one of the ACPI system tables)
	 */

	if (acpi_tb_system_table_pointer (val_desc)) {
		attached_aml_pointer = TRUE;
		aml_opcode = *((u8 *) val_desc);
		aml_pointer = ((u8 *) val_desc) + 1;

	}


	/*
	 * Several Entry_types do not require further processing, so
	 *  we will return immediately
	 */
	/* Devices rarely have an attached object, return the Node
	 *  and Method locals and arguments have a pseudo-Node
	 */
	if (entry_type == ACPI_TYPE_DEVICE ||
		(node->flags & (ANOBJ_METHOD_ARG | ANOBJ_METHOD_LOCAL))) {
		return (AE_OK);
	}

	if (!val_desc) {
		return (AE_AML_NO_OPERAND);
	}

	/*
	 * Action is based on the type of the Node, which indicates the type
	 * of the attached object or pointer
	 */
	switch (entry_type) {

	case ACPI_TYPE_PACKAGE:

		if (attached_aml_pointer) {
			/*
			 * This means that the package initialization is not parsed
			 * -- should not happen
			 */
			return (AE_NOT_IMPLEMENTED);
		}

		/* Val_desc is an internal object in all cases by the time we get here */

		if (ACPI_TYPE_PACKAGE != val_desc->common.type) {
			return (AE_AML_OPERAND_TYPE);
		}

		/* Return an additional reference to the object */

		obj_desc = val_desc;
		acpi_cm_add_reference (obj_desc);
		break;


	case ACPI_TYPE_BUFFER:

		if (attached_aml_pointer) {
			/*
			 * This means that the buffer initialization is not parsed
			 * -- should not happen
			 */
			return (AE_NOT_IMPLEMENTED);
		}

		/* Val_desc is an internal object in all cases by the time we get here */

		if (ACPI_TYPE_BUFFER != val_desc->common.type) {
			return (AE_AML_OPERAND_TYPE);
		}

		/* Return an additional reference to the object */

		obj_desc = val_desc;
		acpi_cm_add_reference (obj_desc);
		break;


	case ACPI_TYPE_STRING:

		if (attached_aml_pointer) {
			/* Allocate a new string object */

			obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_STRING);
			if (!obj_desc) {
				return (AE_NO_MEMORY);
			}

			/* Init the internal object */

			obj_desc->string.pointer = (NATIVE_CHAR *) aml_pointer;
			obj_desc->string.length = STRLEN (obj_desc->string.pointer);
		}

		else {
			if (ACPI_TYPE_STRING != val_desc->common.type) {
				return (AE_AML_OPERAND_TYPE);
			}

			/* Return an additional reference to the object */

			obj_desc = val_desc;
			acpi_cm_add_reference (obj_desc);
		}

		break;


	case ACPI_TYPE_INTEGER:

		/*
		 * The Node has an attached internal object, make sure that it's a
		 * number
		 */

		if (ACPI_TYPE_INTEGER != val_desc->common.type) {
			return (AE_AML_OPERAND_TYPE);
		}

		/* Return an additional reference to the object */

		obj_desc = val_desc;
		acpi_cm_add_reference (obj_desc);
		break;


	case INTERNAL_TYPE_DEF_FIELD:

		/*
		 * TBD: [Investigate] Is this the correct solution?
		 *
		 * This section was extended to convert to generic buffer if
		 *  the return length is greater than 32 bits, but still allows
		 *  for returning a type Number for smaller values because the
		 *  caller can then apply arithmetic operators on those fields.
		 *
		 * XXX - Implementation limitation: Fields are implemented as type
		 * XXX - Number, but they really are supposed to be type Buffer.
		 * XXX - The two are interchangeable only for lengths <= 32 bits.
		 */
		if(val_desc->field.length > 32) {
			object_type = ACPI_TYPE_BUFFER;
		}
		else {
			object_type = ACPI_TYPE_INTEGER;
		}

		/*
		 * Create the destination buffer object and the buffer space.
		 */
		obj_desc = acpi_cm_create_internal_object (object_type);
		if (!obj_desc) {
			return (AE_NO_MEMORY);
		}

		/*
		 * Fill in the object specific details
		 */
		if (ACPI_TYPE_BUFFER == object_type) {
			obj_desc->buffer.pointer = acpi_cm_callocate (val_desc->field.length);
			if (!obj_desc->buffer.pointer) {
				acpi_cm_remove_reference(obj_desc);
				return (AE_NO_MEMORY);
			}

			obj_desc->buffer.length = val_desc->field.length;

			status = acpi_aml_access_named_field (ACPI_READ, (ACPI_HANDLE) node,
					  obj_desc->buffer.pointer, obj_desc->buffer.length);

			if (ACPI_FAILURE (status)) {
				return (status);
			}
		}
		else {
			status = acpi_aml_access_named_field (ACPI_READ, (ACPI_HANDLE) node,
					  &temp_val, sizeof (temp_val));

			if (ACPI_FAILURE (status)) {
				return (status);
			}

			obj_desc->integer.value = temp_val;
		}


		break;


	case INTERNAL_TYPE_BANK_FIELD:

		if (attached_aml_pointer) {
			return (AE_AML_OPERAND_TYPE);
		}

		if (INTERNAL_TYPE_BANK_FIELD != val_desc->common.type) {
			return (AE_AML_OPERAND_TYPE);
		}


		/* Get the global lock if needed */

		obj_desc = (ACPI_OPERAND_OBJECT *) *stack_ptr;
		locked = acpi_aml_acquire_global_lock (obj_desc->field_unit.lock_rule);

		/* Set Index value to select proper Data register */
		/* perform the update */

		status = acpi_aml_access_named_field (ACPI_WRITE,
				 val_desc->bank_field.bank_select, &val_desc->bank_field.value,
				 sizeof (val_desc->bank_field.value));

		acpi_aml_release_global_lock (locked);


		if (ACPI_FAILURE (status)) {
			return (status);
		}

		/* Read Data value */

		status = acpi_aml_access_named_field (ACPI_READ,
				 (ACPI_HANDLE) val_desc->bank_field.container,
				 &temp_val, sizeof (temp_val));
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		/* Create an object for the result */

		obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
		if (!obj_desc) {
			return (AE_NO_MEMORY);
		}

		obj_desc->integer.value = temp_val;
		break;


	case INTERNAL_TYPE_INDEX_FIELD:

		if (attached_aml_pointer) {
			return (AE_AML_OPERAND_TYPE);
		}

		if (INTERNAL_TYPE_INDEX_FIELD != val_desc->common.type) {
			return (AE_AML_OPERAND_TYPE);
		}


		/* Set Index value to select proper Data register */
		/* Get the global lock if needed */

		obj_desc = (ACPI_OPERAND_OBJECT *) *stack_ptr;
		locked = acpi_aml_acquire_global_lock (obj_desc->field_unit.lock_rule);

		/* Perform the update */

		status = acpi_aml_access_named_field (ACPI_WRITE,
				  val_desc->index_field.index, &val_desc->index_field.value,
				  sizeof (val_desc->index_field.value));

		acpi_aml_release_global_lock (locked);

		if (ACPI_FAILURE (status)) {
			return (status);
		}

		/* Read Data value */

		status = acpi_aml_access_named_field (ACPI_READ, val_desc->index_field.data,
				  &temp_val, sizeof (temp_val));
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		/* Create an object for the result */

		obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
		if (!obj_desc) {
			return (AE_NO_MEMORY);
		}

		obj_desc->integer.value = temp_val;
		break;


	case ACPI_TYPE_FIELD_UNIT:

		if (attached_aml_pointer) {
			return (AE_AML_OPERAND_TYPE);
		}

		if (val_desc->common.type != (u8) entry_type) {
			return (AE_AML_OPERAND_TYPE);
			break;
		}

		/* Create object for result */

		obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_ANY);
		if (!obj_desc) {
			return (AE_NO_MEMORY);
		}

		status = acpi_aml_get_field_unit_value (val_desc, obj_desc);
		if (ACPI_FAILURE (status)) {
			acpi_cm_remove_reference (obj_desc);
			return (status);
		}

		break;


	/*
	 * For these objects, just return the object attached to the Node
	 */

	case ACPI_TYPE_MUTEX:
	case ACPI_TYPE_METHOD:
	case ACPI_TYPE_POWER:
	case ACPI_TYPE_PROCESSOR:
	case ACPI_TYPE_THERMAL:
	case ACPI_TYPE_EVENT:
	case ACPI_TYPE_REGION:


		/* Return an additional reference to the object */

		obj_desc = val_desc;
		acpi_cm_add_reference (obj_desc);
		break;


	/* TYPE_Any is untyped, and thus there is no object associated with it */

	case ACPI_TYPE_ANY:

		return (AE_AML_OPERAND_TYPE);  /* Cannot be AE_TYPE */
		break;


	/*
	 * The only named references allowed are named constants
	 *
	 * e.g.     Name (\OSFL, Ones)
	 */
	case INTERNAL_TYPE_REFERENCE:

		switch (val_desc->reference.opcode) {

		case AML_ZERO_OP:

			temp_val = 0;
			break;


		case AML_ONE_OP:

			temp_val = 1;
			break;


		case AML_ONES_OP:

			temp_val = ACPI_INTEGER_MAX;
			break;


		default:

			return (AE_AML_BAD_OPCODE);
		}

		/* Create object for result */

		obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
		if (!obj_desc) {
			return (AE_NO_MEMORY);
		}

		obj_desc->integer.value = temp_val;

		/* Truncate value if we are executing from a 32-bit ACPI table */

		acpi_aml_truncate_for32bit_table (obj_desc, walk_state);
		break;


	/* Default case is for unknown types */

	default:

		return (AE_AML_OPERAND_TYPE);

	} /* switch (Entry_type) */


	/* Put the object descriptor on the stack */

	*stack_ptr = (void *) obj_desc;

	return (status);
}


