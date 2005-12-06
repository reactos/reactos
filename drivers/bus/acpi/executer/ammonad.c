
/******************************************************************************
 *
 * Module Name: ammonad - ACPI AML (p-code) execution for monadic operators
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
#include "acdispat.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("ammonad")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_get_object_reference
 *
 * PARAMETERS:  Obj_desc        - Create a reference to this object
 *              Ret_desc        - Where to store the reference
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Obtain and return a "reference" to the target object
 *              Common code for the Ref_of_op and the Cond_ref_of_op.
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_aml_get_object_reference (
	ACPI_OPERAND_OBJECT     *obj_desc,
	ACPI_OPERAND_OBJECT     **ret_desc,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;


	if (VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_INTERNAL)) {
		if (obj_desc->common.type != INTERNAL_TYPE_REFERENCE) {
			*ret_desc = NULL;
			status = AE_TYPE;
			goto cleanup;
		}

		/*
		 * Not a Name -- an indirect name pointer would have
		 * been converted to a direct name pointer in Acpi_aml_resolve_operands
		 */
		switch (obj_desc->reference.opcode) {
		case AML_LOCAL_OP:
		case AML_ARG_OP:

			*ret_desc = (void *) acpi_ds_method_data_get_node (obj_desc->reference.opcode,
					  obj_desc->reference.offset, walk_state);
			break;

		default:

			*ret_desc = NULL;
			status = AE_AML_INTERNAL;
			goto cleanup;
		}

	}

	else if (VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_NAMED)) {
		/* Must be a named object;  Just return the Node */

		*ret_desc = obj_desc;
	}

	else {
		*ret_desc = NULL;
		status = AE_TYPE;
	}


cleanup:

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_monadic1
 *
 * PARAMETERS:  Opcode              - The opcode to be executed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Type 1 monadic operator with numeric operand on
 *              object stack
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_exec_monadic1 (
	u16                     opcode,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status;


	/* Resolve all operands */

	status = acpi_aml_resolve_operands (opcode, WALK_OPERANDS, walk_state);
	/* Get all operands */

	status |= acpi_ds_obj_stack_pop_object (&obj_desc, walk_state);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}


	/* Examine the opcode */

	switch (opcode) {

	/*  Def_release :=  Release_op  Mutex_object */

	case AML_RELEASE_OP:

		status = acpi_aml_release_mutex (obj_desc, walk_state);
		break;


	/*  Def_reset   :=  Reset_op    Acpi_event_object */

	case AML_RESET_OP:

		status = acpi_aml_system_reset_event (obj_desc);
		break;


	/*  Def_signal  :=  Signal_op   Acpi_event_object */

	case AML_SIGNAL_OP:

		status = acpi_aml_system_signal_event (obj_desc);
		break;


	/*  Def_sleep   :=  Sleep_op    Msec_time   */

	case AML_SLEEP_OP:

		acpi_aml_system_do_suspend ((u32) obj_desc->integer.value);
		break;


	/*  Def_stall   :=  Stall_op    Usec_time   */

	case AML_STALL_OP:

		acpi_aml_system_do_stall ((u32) obj_desc->integer.value);
		break;


	/*  Unknown opcode  */

	default:

		REPORT_ERROR (("Acpi_aml_exec_monadic1: Unknown monadic opcode %X\n",
			opcode));
		status = AE_AML_BAD_OPCODE;
		break;

	} /* switch */


cleanup:

	/* Always delete the operand */

	acpi_cm_remove_reference (obj_desc);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_monadic2_r
 *
 * PARAMETERS:  Opcode              - The opcode to be executed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Type 2 monadic operator with numeric operand and
 *              result operand on operand stack
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_exec_monadic2_r (
	u16                     opcode,
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     **return_desc)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *res_desc;
	ACPI_OPERAND_OBJECT     *ret_desc = NULL;
	ACPI_OPERAND_OBJECT     *ret_desc2 = NULL;
	u32                     res_val;
	ACPI_STATUS             status;
	u32                     i;
	u32                     j;
	ACPI_INTEGER            digit;


	/* Resolve all operands */

	status = acpi_aml_resolve_operands (opcode, WALK_OPERANDS, walk_state);
	/* Get all operands */

	status |= acpi_ds_obj_stack_pop_object (&res_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&obj_desc, walk_state);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}


	/* Create a return object of type NUMBER for most opcodes */

	switch (opcode) {
	case AML_BIT_NOT_OP:
	case AML_FIND_SET_LEFT_BIT_OP:
	case AML_FIND_SET_RIGHT_BIT_OP:
	case AML_FROM_BCD_OP:
	case AML_TO_BCD_OP:
	case AML_COND_REF_OF_OP:

		ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
		if (!ret_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		break;
	}


	switch (opcode) {
	/*  Def_not :=  Not_op  Operand Result  */

	case AML_BIT_NOT_OP:

		ret_desc->integer.value = ~obj_desc->integer.value;
		break;


	/*  Def_find_set_left_bit := Find_set_left_bit_op Operand Result */

	case AML_FIND_SET_LEFT_BIT_OP:

		ret_desc->integer.value = obj_desc->integer.value;

		/*
		 * Acpi specification describes Integer type as a little
		 * endian unsigned value, so this boundry condition is valid.
		 */
		for (res_val = 0; ret_desc->integer.value && res_val < ACPI_INTEGER_BIT_SIZE; ++res_val) {
			ret_desc->integer.value >>= 1;
		}

		ret_desc->integer.value = res_val;
		break;


	/*  Def_find_set_right_bit := Find_set_right_bit_op Operand Result */

	case AML_FIND_SET_RIGHT_BIT_OP:

		ret_desc->integer.value = obj_desc->integer.value;

		/*
		 * Acpi specification describes Integer type as a little
		 * endian unsigned value, so this boundry condition is valid.
		 */
		for (res_val = 0; ret_desc->integer.value && res_val < ACPI_INTEGER_BIT_SIZE; ++res_val) {
			ret_desc->integer.value <<= 1;
		}

		/* Since returns must be 1-based, subtract from 33 (65) */

		ret_desc->integer.value = res_val == 0 ? 0 : (ACPI_INTEGER_BIT_SIZE + 1) - res_val;
		break;


	/*  Def_from_bDC := From_bCDOp  BCDValue    Result  */

	case AML_FROM_BCD_OP:

		/*
		 * The 64-bit ACPI integer can hold 16 4-bit BCD integers
		 */
		ret_desc->integer.value = 0;
		for (i = 0; i < ACPI_MAX_BCD_DIGITS; i++) {
			/* Get one BCD digit */

			digit = (ACPI_INTEGER) ((obj_desc->integer.value >> (i * 4)) & 0xF);

			/* Check the range of the digit */

			if (digit > 9) {
				status = AE_AML_NUMERIC_OVERFLOW;
				goto cleanup;
			}

			if (digit > 0) {
				/* Sum into the result with the appropriate power of 10 */

				for (j = 0; j < i; j++) {
					digit *= 10;
				}

				ret_desc->integer.value += digit;
			}
		}
		break;


	/*  Def_to_bDC  :=  To_bCDOp Operand Result */

	case AML_TO_BCD_OP:


		if (obj_desc->integer.value > ACPI_MAX_BCD_VALUE) {
			status = AE_AML_NUMERIC_OVERFLOW;
			goto cleanup;
		}

		ret_desc->integer.value = 0;
		for (i = 0; i < ACPI_MAX_BCD_DIGITS; i++) {
			/* Divide by nth factor of 10 */

			digit = obj_desc->integer.value;
			for (j = 0; j < i; j++) {
				digit /= 10;
			}

			/* Create the BCD digit */

			if (digit > 0) {
				ret_desc->integer.value += (ACPI_MODULO (digit, 10) << (i * 4));
			}
		}
		break;


	/*  Def_cond_ref_of     :=  Cond_ref_of_op  Source_object   Result  */

	case AML_COND_REF_OF_OP:

		/*
		 * This op is a little strange because the internal return value is
		 * different than the return value stored in the result descriptor
		 * (There are really two return values)
		 */

		if ((ACPI_NAMESPACE_NODE *) obj_desc == acpi_gbl_root_node) {
			/*
			 * This means that the object does not exist in the namespace,
			 * return FALSE
			 */

			ret_desc->integer.value = 0;

			/*
			 * Must delete the result descriptor since there is no reference
			 * being returned
			 */

			acpi_cm_remove_reference (res_desc);
			goto cleanup;
		}

		/* Get the object reference and store it */

		status = acpi_aml_get_object_reference (obj_desc, &ret_desc2, walk_state);
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}

		status = acpi_aml_exec_store (ret_desc2, res_desc, walk_state);

		/* The object exists in the namespace, return TRUE */

		ret_desc->integer.value = ACPI_INTEGER_MAX;
		goto cleanup;
		break;


	case AML_STORE_OP:

		/*
		 * A store operand is typically a number, string, buffer or lvalue
		 * TBD: [Unhandled] What about a store to a package?
		 */

		/*
		 * Do the store, and be careful about deleting the source object,
		 * since the object itself may have been stored.
		 */

		status = acpi_aml_exec_store (obj_desc, res_desc, walk_state);
		if (ACPI_FAILURE (status)) {
			/* On failure, just delete the Obj_desc */

			acpi_cm_remove_reference (obj_desc);
		}

		else {
			/*
			 * Normally, we would remove a reference on the Obj_desc parameter;
			 * But since it is being used as the internal return object
			 * (meaning we would normally increment it), the two cancel out,
			 * and we simply don't do anything.
			 */
			*return_desc = obj_desc;
		}

		obj_desc = NULL;
		return (status);

		break;


	case AML_DEBUG_OP:

		/* Reference, returning an Reference */

		return (AE_OK);
		break;


	/*
	 * These are obsolete opcodes
	 */

	/*  Def_shift_left_bit  :=  Shift_left_bit_op   Source          Bit_num */
	/*  Def_shift_right_bit :=  Shift_right_bit_op  Source          Bit_num */

	case AML_SHIFT_LEFT_BIT_OP:
	case AML_SHIFT_RIGHT_BIT_OP:

		status = AE_SUPPORT;
		goto cleanup;
		break;


	default:

		REPORT_ERROR (("Acpi_aml_exec_monadic2_r: Unknown monadic opcode %X\n",
			opcode));
		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}


	status = acpi_aml_exec_store (ret_desc, res_desc, walk_state);


cleanup:
	/* Always delete the operand object */

	acpi_cm_remove_reference (obj_desc);

	/* Delete return object(s) on error */

	if (ACPI_FAILURE (status)) {
		acpi_cm_remove_reference (res_desc); /* Result descriptor */
		if (ret_desc) {
			acpi_cm_remove_reference (ret_desc);
			ret_desc = NULL;
		}
	}

	/* Set the return object and exit */

	*return_desc = ret_desc;
	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_monadic2
 *
 * PARAMETERS:  Opcode              - The opcode to be executed
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Type 2 monadic operator with numeric operand:
 *              Deref_of_op, Ref_of_op, Size_of_op, Type_op, Increment_op,
 *              Decrement_op, LNot_op,
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_exec_monadic2 (
	u16                     opcode,
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     **return_desc)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *tmp_desc;
	ACPI_OPERAND_OBJECT     *ret_desc = NULL;
	ACPI_STATUS             resolve_status;
	ACPI_STATUS             status;
	u32                     type;
	ACPI_INTEGER            value;


	/* Attempt to resolve the operands */

	resolve_status = acpi_aml_resolve_operands (opcode, WALK_OPERANDS, walk_state);
	/* Always get all operands */

	status = acpi_ds_obj_stack_pop_object (&obj_desc, walk_state);


	/* Now we can check the status codes */

	if (ACPI_FAILURE (resolve_status)) {
		goto cleanup;
	}

	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}


	/* Get the operand and decode the opcode */


	switch (opcode) {

	/*  Def_lNot := LNot_op Operand */

	case AML_LNOT_OP:

		ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
		if (!ret_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		ret_desc->integer.value = !obj_desc->integer.value;
		break;


	/*  Def_decrement   :=  Decrement_op Target */
	/*  Def_increment   :=  Increment_op Target */

	case AML_DECREMENT_OP:
	case AML_INCREMENT_OP:

		/*
		 * Since we are expecting an Reference on the top of the stack, it
		 * can be either an Node or an internal object.
		 *
		 * TBD: [Future] This may be the prototype code for all cases where
		 * an Reference is expected!! 10/99
		 */

		if (VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_NAMED)) {
			ret_desc = obj_desc;
		}

		else {
			/*
			 * Duplicate the Reference in a new object so that we can resolve it
			 * without destroying the original Reference object
			 */

			ret_desc = acpi_cm_create_internal_object (INTERNAL_TYPE_REFERENCE);
			if (!ret_desc) {
				status = AE_NO_MEMORY;
				goto cleanup;
			}

			ret_desc->reference.opcode = obj_desc->reference.opcode;
			ret_desc->reference.offset = obj_desc->reference.offset;
			ret_desc->reference.object = obj_desc->reference.object;
		}


		/*
		 * Convert the Ret_desc Reference to a Number
		 * (This deletes the original Ret_desc)
		 */

		status = acpi_aml_resolve_operands (AML_LNOT_OP, &ret_desc, walk_state);
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}

		/* Do the actual increment or decrement */

		if (AML_INCREMENT_OP == opcode) {
			ret_desc->integer.value++;
		}
		else {
			ret_desc->integer.value--;
		}

		/* Store the result back in the original descriptor */

		status = acpi_aml_exec_store (ret_desc, obj_desc, walk_state);

		/* Objdesc was just deleted (because it is an Reference) */

		obj_desc = NULL;

		break;


	/*  Def_object_type :=  Object_type_op  Source_object   */

	case AML_TYPE_OP:

		if (INTERNAL_TYPE_REFERENCE == obj_desc->common.type) {
			/*
			 * Not a Name -- an indirect name pointer would have
			 * been converted to a direct name pointer in Resolve_operands
			 */
			switch (obj_desc->reference.opcode) {
			case AML_ZERO_OP:
			case AML_ONE_OP:
			case AML_ONES_OP:

				/* Constants are of type Number */

				type = ACPI_TYPE_INTEGER;
				break;


			case AML_DEBUG_OP:

				/* Per 1.0b spec, Debug object is of type Debug_object */

				type = ACPI_TYPE_DEBUG_OBJECT;
				break;


			case AML_INDEX_OP:

				/* Get the type of this reference (index into another object) */

				type = obj_desc->reference.target_type;
				if (type == ACPI_TYPE_PACKAGE) {
					/*
					 * The main object is a package, we want to get the type
					 * of the individual package element that is referenced by
					 * the index.
					 */
					type = (*(obj_desc->reference.where))->common.type;
				}

				break;


			case AML_LOCAL_OP:
			case AML_ARG_OP:

				type = acpi_ds_method_data_get_type (obj_desc->reference.opcode,
						  obj_desc->reference.offset, walk_state);
				break;


			default:

				REPORT_ERROR (("Acpi_aml_exec_monadic2/Type_op: Internal error - Unknown Reference subtype %X\n",
					obj_desc->reference.opcode));
				status = AE_AML_INTERNAL;
				goto cleanup;
			}
		}

		else {
			/*
			 * It's not a Reference, so it must be a direct name pointer.
			 */
			type = acpi_ns_get_type ((ACPI_HANDLE) obj_desc);
		}

		/* Allocate a descriptor to hold the type. */

		ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
		if (!ret_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		ret_desc->integer.value = type;
		break;


	/*  Def_size_of :=  Size_of_op  Source_object   */

	case AML_SIZE_OF_OP:

		if (VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_NAMED)) {
			obj_desc = acpi_ns_get_attached_object (obj_desc);
		}

		if (!obj_desc) {
			value = 0;
		}

		else {
			switch (obj_desc->common.type) {

			case ACPI_TYPE_BUFFER:

				value = obj_desc->buffer.length;
				break;


			case ACPI_TYPE_STRING:

				value = obj_desc->string.length;
				break;


			case ACPI_TYPE_PACKAGE:

				value = obj_desc->package.count;
				break;

			case INTERNAL_TYPE_REFERENCE:

				value = 4;
				break;

			default:

				status = AE_AML_OPERAND_TYPE;
				goto cleanup;
			}
		}

		/*
		 * Now that we have the size of the object, create a result
		 * object to hold the value
		 */

		ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
		if (!ret_desc) {
			status = AE_NO_MEMORY;
			goto cleanup;
		}

		ret_desc->integer.value = value;
		break;


	/*  Def_ref_of  :=  Ref_of_op   Source_object   */

	case AML_REF_OF_OP:

		status = acpi_aml_get_object_reference (obj_desc, &ret_desc, walk_state);
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}
		break;


	/*  Def_deref_of := Deref_of_op Obj_reference   */

	case AML_DEREF_OF_OP:


		/* Check for a method local or argument */

		if (!VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_NAMED)) {
			/*
			 * Must resolve/dereference the local/arg reference first
			 */
			switch (obj_desc->reference.opcode) {
			/* Set Obj_desc to the value of the local/arg */

			case AML_LOCAL_OP:
			case AML_ARG_OP:

				acpi_ds_method_data_get_value (obj_desc->reference.opcode,
						obj_desc->reference.offset, walk_state, &tmp_desc);

				/*
				 * Delete our reference to the input object and
				 * point to the object just retrieved
				 */
				acpi_cm_remove_reference (obj_desc);
				obj_desc = tmp_desc;
				break;

			default:

				/* Index op - handled below */
				break;
			}
		}


		/* Obj_desc may have changed from the code above */

		if (VALID_DESCRIPTOR_TYPE (obj_desc, ACPI_DESC_TYPE_NAMED)) {
			/* Get the actual object from the Node (This is the dereference) */

			ret_desc = ((ACPI_NAMESPACE_NODE *) obj_desc)->object;

			/* Returning a pointer to the object, add another reference! */

			acpi_cm_add_reference (ret_desc);
		}

		else {
			/*
			 * This must be a reference object produced by the Index
			 * ASL operation -- check internal opcode
			 */

			if ((obj_desc->reference.opcode != AML_INDEX_OP) &&
				(obj_desc->reference.opcode != AML_REF_OF_OP)) {
				status = AE_TYPE;
				goto cleanup;
			}


			switch (obj_desc->reference.opcode) {
			case AML_INDEX_OP:

				/*
				 * Supported target types for the Index operator are
				 * 1) A Buffer
				 * 2) A Package
				 */

				if (obj_desc->reference.target_type == ACPI_TYPE_BUFFER_FIELD) {
					/*
					 * The target is a buffer, we must create a new object that
					 * contains one element of the buffer, the element pointed
					 * to by the index.
					 *
					 * NOTE: index into a buffer is NOT a pointer to a
					 * sub-buffer of the main buffer, it is only a pointer to a
					 * single element (byte) of the buffer!
					 */
					ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
					if (!ret_desc) {
						status = AE_NO_MEMORY;
						goto cleanup;
					}

					tmp_desc = obj_desc->reference.object;
					ret_desc->integer.value =
						tmp_desc->buffer.pointer[obj_desc->reference.offset];

					/* TBD: [Investigate] (see below) Don't add an additional
					 * ref!
					 */
				}

				else if (obj_desc->reference.target_type == ACPI_TYPE_PACKAGE) {
					/*
					 * The target is a package, we want to return the referenced
					 * element of the package.  We must add another reference to
					 * this object, however.
					 */

					ret_desc = *(obj_desc->reference.where);
					if (!ret_desc) {
						/*
						 * We can't return a NULL dereferenced value.  This is
						 * an uninitialized package element and is thus a
						 * severe error.
						 */

						status = AE_AML_UNINITIALIZED_ELEMENT;
						goto cleanup;
					}

					acpi_cm_add_reference (ret_desc);
				}

				else {
					status = AE_AML_OPERAND_TYPE;
					goto cleanup;
				}

				break;


			case AML_REF_OF_OP:

				ret_desc = obj_desc->reference.object;

				/* Add another reference to the object! */

				acpi_cm_add_reference (ret_desc);
				break;
			}
		}

		break;


	default:

		REPORT_ERROR (("Acpi_aml_exec_monadic2: Unknown monadic opcode %X\n",
			opcode));
		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}


cleanup:

	if (obj_desc) {
		acpi_cm_remove_reference (obj_desc);
	}

	/* Delete return object on error */

	if (ACPI_FAILURE (status) &&
		(ret_desc)) {
		acpi_cm_remove_reference (ret_desc);
		ret_desc = NULL;
	}

	*return_desc = ret_desc;
	return (status);
}

