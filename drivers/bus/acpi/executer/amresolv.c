
/******************************************************************************
 *
 * Module Name: amresolv - AML Interpreter object resolution
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
	 MODULE_NAME         ("amresolv")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_get_field_unit_value
 *
 * PARAMETERS:  *Field_desc         - Pointer to a Field_unit
 *              *Result_desc        - Pointer to an empty descriptor
 *                                    which will become a Number
 *                                    containing the field's value.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve the value from a Field_unit
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_get_field_unit_value (
	ACPI_OPERAND_OBJECT     *field_desc,
	ACPI_OPERAND_OBJECT     *result_desc)
{
	ACPI_STATUS             status = AE_OK;
	u32                     mask;
	u8                      *location = NULL;
	u8                      locked = FALSE;


	if (!field_desc) {
		status = AE_AML_NO_OPERAND;
	}

	if (!(field_desc->common.flags & AOPOBJ_DATA_VALID)) {
		status = acpi_ds_get_field_unit_arguments (field_desc);
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	if (!field_desc->field_unit.container) {
		status = AE_AML_INTERNAL;
	}

	else if (ACPI_TYPE_BUFFER != field_desc->field_unit.container->common.type) {
		status = AE_AML_OPERAND_TYPE;
	}

	else if (!result_desc) {
		status = AE_AML_INTERNAL;
	}

	if (ACPI_FAILURE (status)) {
		return (status);
	}


		/* Get the global lock if needed */

	locked = acpi_aml_acquire_global_lock (field_desc->field_unit.lock_rule);

	/* Field location is (base of buffer) + (byte offset) */

	location = field_desc->field_unit.container->buffer.pointer
			 + field_desc->field_unit.offset;

	/*
	 * Construct Mask with as many 1 bits as the field width
	 *
	 * NOTE: Only the bottom 5 bits are valid for a shift operation, so
	 *  special care must be taken for any shift greater than 31 bits.
	 *
	 * TBD: [Unhandled] Fields greater than 32-bits will not work.
	 */

	if (field_desc->field_unit.length < 32) {
		mask = ((u32) 1 << field_desc->field_unit.length) - (u32) 1;
	}
	else {
		mask = ACPI_UINT32_MAX;
	}

	result_desc->integer.type = (u8) ACPI_TYPE_INTEGER;

	/* Get the 32 bit value at the location */

	MOVE_UNALIGNED32_TO_32 (&result_desc->integer.value, location);

	/*
	 * Shift the 32-bit word containing the field, and mask off the
	 * resulting value
	 */

	result_desc->integer.value =
		(result_desc->integer.value >> field_desc->field_unit.bit_offset) & mask;

	/* Release global lock if we acquired it earlier */

	acpi_aml_release_global_lock (locked);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_resolve_to_value
 *
 * PARAMETERS:  **Stack_ptr         - Points to entry on Obj_stack, which can
 *                                    be either an (ACPI_OPERAND_OBJECT  *)
 *                                    or an ACPI_HANDLE.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert Reference objects to values
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_resolve_to_value (
	ACPI_OPERAND_OBJECT     **stack_ptr,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;


	if (!stack_ptr || !*stack_ptr) {
		return (AE_AML_NO_OPERAND);
	}


	/*
	 * The entity pointed to by the Stack_ptr can be either
	 * 1) A valid ACPI_OPERAND_OBJECT, or
	 * 2) A ACPI_NAMESPACE_NODE (Named_obj)
	 */

	if (VALID_DESCRIPTOR_TYPE (*stack_ptr, ACPI_DESC_TYPE_INTERNAL)) {

		status = acpi_aml_resolve_object_to_value (stack_ptr, walk_state);
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	/*
	 * Object on the stack may have changed if Acpi_aml_resolve_object_to_value()
	 * was called (i.e., we can't use an _else_ here.)
	 */

	if (VALID_DESCRIPTOR_TYPE (*stack_ptr, ACPI_DESC_TYPE_NAMED)) {
		status = acpi_aml_resolve_node_to_value ((ACPI_NAMESPACE_NODE **) stack_ptr, walk_state);
	}


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_resolve_object_to_value
 *
 * PARAMETERS:  Stack_ptr       - Pointer to a stack location that contains a
 *                                ptr to an internal object.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve the value from an internal object.  The Reference type
 *              uses the associated AML opcode to determine the value.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_resolve_object_to_value (
	ACPI_OPERAND_OBJECT     **stack_ptr,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_OPERAND_OBJECT     *stack_desc;
	ACPI_STATUS             status = AE_OK;
	ACPI_HANDLE             temp_handle = NULL;
	ACPI_OPERAND_OBJECT     *obj_desc = NULL;
	u32                     index = 0;
	u16                     opcode;


	stack_desc = *stack_ptr;

	/* This is an ACPI_OPERAND_OBJECT  */

	switch (stack_desc->common.type) {

	case INTERNAL_TYPE_REFERENCE:

		opcode = stack_desc->reference.opcode;

		switch (opcode) {

		case AML_NAME_OP:

			/*
			 * Convert indirect name ptr to a direct name ptr.
			 * Then, Acpi_aml_resolve_node_to_value can be used to get the value
			 */

			temp_handle = stack_desc->reference.object;

			/* Delete the Reference Object */

			acpi_cm_remove_reference (stack_desc);

			/* Put direct name pointer onto stack and exit */

			(*stack_ptr) = temp_handle;
			status = AE_OK;
			break;


		case AML_LOCAL_OP:
		case AML_ARG_OP:

			index = stack_desc->reference.offset;

			/*
			 * Get the local from the method's state info
			 * Note: this increments the local's object reference count
			 */

			status = acpi_ds_method_data_get_value (opcode, index,
					 walk_state, &obj_desc);
			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/*
			 * Now we can delete the original Reference Object and
			 * replace it with the resolve value
			 */

			acpi_cm_remove_reference (stack_desc);
			*stack_ptr = obj_desc;

			break;


		/*
		 * TBD: [Restructure] These next three opcodes change the type of
		 * the object, which is actually a no-no.
		 */

		case AML_ZERO_OP:

			stack_desc->common.type = (u8) ACPI_TYPE_INTEGER;
			stack_desc->integer.value = 0;
			break;


		case AML_ONE_OP:

			stack_desc->common.type = (u8) ACPI_TYPE_INTEGER;
			stack_desc->integer.value = 1;
			break;


		case AML_ONES_OP:

			stack_desc->common.type = (u8) ACPI_TYPE_INTEGER;
			stack_desc->integer.value = ACPI_INTEGER_MAX;

			/* Truncate value if we are executing from a 32-bit ACPI table */

			acpi_aml_truncate_for32bit_table (stack_desc, walk_state);
			break;


		case AML_INDEX_OP:

			switch (stack_desc->reference.target_type) {
			case ACPI_TYPE_BUFFER_FIELD:

				/* Just return - leave the Reference on the stack */
				break;


			case ACPI_TYPE_PACKAGE:
				obj_desc = *stack_desc->reference.where;
				if (obj_desc) {
					/*
					 * Valid obj descriptor, copy pointer to return value
					 * (i.e., dereference the package index)
					 * Delete the ref object, increment the returned object
					 */
					acpi_cm_remove_reference (stack_desc);
					acpi_cm_add_reference (obj_desc);
					*stack_ptr = obj_desc;
				}

				else {
					/*
					 * A NULL object descriptor means an unitialized element of
					 * the package, can't deref it
					 */

					status = AE_AML_UNINITIALIZED_ELEMENT;
				}
				break;

			default:
				/* Invalid reference OBJ*/

				status = AE_AML_INTERNAL;
				break;
			}

			break;


		case AML_DEBUG_OP:

			/* Just leave the object as-is */
			break;


		default:

			status = AE_AML_INTERNAL;

		}   /* switch (Opcode) */


		if (ACPI_FAILURE (status)) {
			return (status);
		}

		break; /* case INTERNAL_TYPE_REFERENCE */


	case ACPI_TYPE_FIELD_UNIT:

		obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_ANY);
		if (!obj_desc) {
			/* Descriptor allocation failure  */

			return (AE_NO_MEMORY);
		}

		status = acpi_aml_get_field_unit_value (stack_desc, obj_desc);
		if (ACPI_FAILURE (status)) {
			acpi_cm_remove_reference (obj_desc);
			obj_desc = NULL;
		}

		*stack_ptr = (void *) obj_desc;
		break;


	case INTERNAL_TYPE_BANK_FIELD:

		obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_ANY);
		if (!obj_desc) {
			/* Descriptor allocation failure */

			return (AE_NO_MEMORY);
		}

		status = acpi_aml_get_field_unit_value (stack_desc, obj_desc);
		if (ACPI_FAILURE (status)) {
			acpi_cm_remove_reference (obj_desc);
			obj_desc = NULL;
		}

		*stack_ptr = (void *) obj_desc;
		break;


	/* TBD: [Future] - may need to handle Index_field, and Def_field someday */

	default:

		break;

	}   /* switch (Stack_desc->Common.Type) */


	return (status);
}


