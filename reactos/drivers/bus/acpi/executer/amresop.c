
/******************************************************************************
 *
 * Module Name: amresop - AML Interpreter operand/object resolution
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
	 MODULE_NAME         ("amresop")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_check_object_type
 *
 * PARAMETERS:  Type_needed         Object type needed
 *              This_type           Actual object type
 *              Object              Object pointer
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Check required type against actual type
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_check_object_type (
	ACPI_OBJECT_TYPE        type_needed,
	ACPI_OBJECT_TYPE        this_type,
	void                    *object)
{


	if (type_needed == ACPI_TYPE_ANY) {
		/* All types OK, so we don't perform any typechecks */

		return (AE_OK);
	}


	if (type_needed != this_type) {
		return (AE_AML_OPERAND_TYPE);
	}


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_resolve_operands
 *
 * PARAMETERS:  Opcode              Opcode being interpreted
 *              Stack_ptr           Top of operand stack
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert stack entries to required types
 *
 *      Each nibble in Arg_types represents one required operand
 *      and indicates the required Type:
 *
 *      The corresponding stack entry will be converted to the
 *      required type if possible, else return an exception
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_resolve_operands (
	u16                     opcode,
	ACPI_OPERAND_OBJECT     **stack_ptr,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status = AE_OK;
	u8                      object_type;
	ACPI_HANDLE             temp_handle;
	u32                     arg_types;
	ACPI_OPCODE_INFO        *op_info;
	u32                     this_arg_type;
	ACPI_OBJECT_TYPE        type_needed;


	op_info = acpi_ps_get_opcode_info (opcode);
	if (ACPI_GET_OP_TYPE (op_info) != ACPI_OP_TYPE_OPCODE) {
		return (AE_AML_BAD_OPCODE);
	}


	arg_types = op_info->runtime_args;
	if (arg_types == ARGI_INVALID_OPCODE) {
		return (AE_AML_INTERNAL);
	}


   /*
	 * Normal exit is with *Types == '\0' at end of string.
	 * Function will return an exception from within the loop upon
	 * finding an entry which is not, and cannot be converted
	 * to, the required type; if stack underflows; or upon
	 * finding a NULL stack entry (which "should never happen").
	 */

	while (GET_CURRENT_ARG_TYPE (arg_types)) {
		if (!stack_ptr || !*stack_ptr) {
			return (AE_AML_INTERNAL);
		}

		/* Extract useful items */

		obj_desc = *stack_ptr;

		/* Decode the descriptor type */

		if (VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_NAMED)) {
			/* Node */

			object_type = ((ACPI_NAMESPACE_NODE *) obj_desc)->type;
		}

		else if (VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_INTERNAL)) {
			/* ACPI internal object */

			object_type = obj_desc->common.type;

			/* Check for bad ACPI_OBJECT_TYPE */

			if (!acpi_aml_validate_object_type (object_type)) {
				return (AE_AML_OPERAND_TYPE);
			}

			if (object_type == (u8) INTERNAL_TYPE_REFERENCE) {
				/*
				 * Decode the Reference
				 */

				op_info = acpi_ps_get_opcode_info (opcode);
				if (ACPI_GET_OP_TYPE (op_info) != ACPI_OP_TYPE_OPCODE) {
					return (AE_AML_BAD_OPCODE);
				}


				switch (obj_desc->reference.opcode) {
				case AML_ZERO_OP:
				case AML_ONE_OP:
				case AML_ONES_OP:
				case AML_DEBUG_OP:
				case AML_NAME_OP:
				case AML_INDEX_OP:
				case AML_ARG_OP:
				case AML_LOCAL_OP:

					break;

				default:
					return (AE_AML_OPERAND_TYPE);
					break;
				}
			}
		}

		else {
			/* Invalid descriptor */

			return (AE_AML_OPERAND_TYPE);
		}


		/*
		 * Get one argument type, point to the next
		 */

		this_arg_type = GET_CURRENT_ARG_TYPE (arg_types);
		INCREMENT_ARG_LIST (arg_types);


		/*
		 * Handle cases where the object does not need to be
		 * resolved to a value
		 */

		switch (this_arg_type) {

		case ARGI_REFERENCE:            /* References */
		case ARGI_INTEGER_REF:
		case ARGI_OBJECT_REF:
		case ARGI_DEVICE_REF:
		case ARGI_TARGETREF:            /* TBD: must implement implicit conversion rules before store */
		case ARGI_FIXED_TARGET:         /* No implicit conversion before store to target */
		case ARGI_SIMPLE_TARGET:        /* Name, Local, or Arg - no implicit conversion */

			/* Need an operand of type INTERNAL_TYPE_REFERENCE */

			if (VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_NAMED))            /* direct name ptr OK as-is */ {
				goto next_operand;
			}

			status = acpi_aml_check_object_type (INTERNAL_TYPE_REFERENCE,
					  object_type, obj_desc);
			if (ACPI_FAILURE (status)) {
				return (status);
			}


			if (AML_NAME_OP == obj_desc->reference.opcode) {
				/*
				 * Convert an indirect name ptr to direct name ptr and put
				 * it on the stack
				 */

				temp_handle = obj_desc->reference.object;
				acpi_cm_remove_reference (obj_desc);
				(*stack_ptr) = temp_handle;
			}

			goto next_operand;
			break;


		case ARGI_ANYTYPE:

			/*
			 * We don't want to resolve Index_op reference objects during
			 * a store because this would be an implicit De_ref_of operation.
			 * Instead, we just want to store the reference object.
			 * -- All others must be resolved below.
			 */

			if ((opcode == AML_STORE_OP) &&
				((*stack_ptr)->common.type == INTERNAL_TYPE_REFERENCE) &&
				((*stack_ptr)->reference.opcode == AML_INDEX_OP)) {
				goto next_operand;
			}
			break;
		}


		/*
		 * Resolve this object to a value
		 */

		status = acpi_aml_resolve_to_value (stack_ptr, walk_state);
		if (ACPI_FAILURE (status)) {
			return (status);
		}


		/*
		 * Check the resulting object (value) type
		 */
		switch (this_arg_type) {
		/*
		 * For the simple cases, only one type of resolved object
		 * is allowed
		 */
		case ARGI_MUTEX:

			/* Need an operand of type ACPI_TYPE_MUTEX */

			type_needed = ACPI_TYPE_MUTEX;
			break;

		case ARGI_EVENT:

			/* Need an operand of type ACPI_TYPE_EVENT */

			type_needed = ACPI_TYPE_EVENT;
			break;

		case ARGI_REGION:

			/* Need an operand of type ACPI_TYPE_REGION */

			type_needed = ACPI_TYPE_REGION;
			break;

		case ARGI_IF:   /* If */

			/* Need an operand of type INTERNAL_TYPE_IF */

			type_needed = INTERNAL_TYPE_IF;
			break;

		case ARGI_PACKAGE:   /* Package */

			/* Need an operand of type ACPI_TYPE_PACKAGE */

			type_needed = ACPI_TYPE_PACKAGE;
			break;

		case ARGI_ANYTYPE:

			/* Any operand type will do */

			type_needed = ACPI_TYPE_ANY;
			break;


		/*
		 * The more complex cases allow multiple resolved object types
		 */

		case ARGI_INTEGER:   /* Number */

			/*
			 * Need an operand of type ACPI_TYPE_INTEGER,
			 * But we can implicitly convert from a STRING or BUFFER
			 */
			status = acpi_aml_convert_to_integer (stack_ptr, walk_state);
			if (ACPI_FAILURE (status)) {
				if (status == AE_TYPE) {
					return (AE_AML_OPERAND_TYPE);
				}

				return (status);
			}

			goto next_operand;
			break;


		case ARGI_BUFFER:

			/*
			 * Need an operand of type ACPI_TYPE_BUFFER,
			 * But we can implicitly convert from a STRING or INTEGER
			 */
			status = acpi_aml_convert_to_buffer (stack_ptr, walk_state);
			if (ACPI_FAILURE (status)) {
				if (status == AE_TYPE) {
					return (AE_AML_OPERAND_TYPE);
				}

				return (status);
			}

			goto next_operand;
			break;


		case ARGI_STRING:

			/*
			 * Need an operand of type ACPI_TYPE_STRING,
			 * But we can implicitly convert from a BUFFER or INTEGER
			 */
			status = acpi_aml_convert_to_string (stack_ptr, walk_state);
			if (ACPI_FAILURE (status)) {
				if (status == AE_TYPE) {
					return (AE_AML_OPERAND_TYPE);
				}

				return (status);
			}

			goto next_operand;
			break;


		case ARGI_COMPUTEDATA:

			/* Need an operand of type INTEGER, STRING or BUFFER */

			if ((ACPI_TYPE_INTEGER != (*stack_ptr)->common.type) &&
				(ACPI_TYPE_STRING != (*stack_ptr)->common.type) &&
				(ACPI_TYPE_BUFFER != (*stack_ptr)->common.type)) {
				return (AE_AML_OPERAND_TYPE);
			}
			goto next_operand;
			break;


		case ARGI_DATAOBJECT:
			/*
			 * ARGI_DATAOBJECT is only used by the Size_of operator.
			 *
			 * The ACPI specification allows Size_of to return the size of
			 *  a Buffer, String or Package.  However, the MS ACPI.SYS AML
			 *  Interpreter also allows an Node reference to return without
			 *  error with a size of 4.
			 */

			/* Need a buffer, string, package or Node reference */

			if (((*stack_ptr)->common.type != ACPI_TYPE_BUFFER) &&
				((*stack_ptr)->common.type != ACPI_TYPE_STRING) &&
				((*stack_ptr)->common.type != ACPI_TYPE_PACKAGE) &&
				((*stack_ptr)->common.type != INTERNAL_TYPE_REFERENCE)) {
				return (AE_AML_OPERAND_TYPE);
			}

			/*
			 * If this is a reference, only allow a reference to an Node.
			 */
			if ((*stack_ptr)->common.type == INTERNAL_TYPE_REFERENCE) {
				if (!(*stack_ptr)->reference.node) {
					return (AE_AML_OPERAND_TYPE);
				}
			}
			goto next_operand;
			break;


		case ARGI_COMPLEXOBJ:

			/* Need a buffer or package */

			if (((*stack_ptr)->common.type != ACPI_TYPE_BUFFER) &&
				((*stack_ptr)->common.type != ACPI_TYPE_PACKAGE)) {
				return (AE_AML_OPERAND_TYPE);
			}
			goto next_operand;
			break;


		default:

			/* Unknown type */

			return (AE_BAD_PARAMETER);
		}


		/*
		 * Make sure that the original object was resolved to the
		 * required object type (Simple cases only).
		 */
		status = acpi_aml_check_object_type (type_needed,
				  (*stack_ptr)->common.type, *stack_ptr);
		if (ACPI_FAILURE (status)) {
			return (status);
		}


next_operand:
		/*
		 * If more operands needed, decrement Stack_ptr to point
		 * to next operand on stack
		 */
		if (GET_CURRENT_ARG_TYPE (arg_types)) {
			stack_ptr--;
		}

	}   /* while (*Types) */


	return (status);
}


