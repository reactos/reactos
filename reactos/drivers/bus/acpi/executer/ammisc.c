
/******************************************************************************
 *
 * Module Name: ammisc - ACPI AML (p-code) execution - specific opcodes
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
#include "acinterp.h"
#include "amlcode.h"
#include "acdispat.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("ammisc")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_fatal
 *
 * PARAMETERS:  none
 *
 * RETURN:      Status.  If the OS returns from the OSD call, we just keep
 *              on going.
 *
 * DESCRIPTION: Execute Fatal operator
 *
 * ACPI SPECIFICATION REFERENCES:
 *  Def_fatal   :=  Fatal_op Fatal_type Fatal_code  Fatal_arg
 *  Fatal_type  :=  Byte_data
 *  Fatal_code  :=  DWord_data
 *  Fatal_arg   :=  Term_arg=>Integer
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_exec_fatal (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_OPERAND_OBJECT     *type_desc;
	ACPI_OPERAND_OBJECT     *code_desc;
	ACPI_OPERAND_OBJECT     *arg_desc;
	ACPI_STATUS             status;


	/* Resolve operands */

	status = acpi_aml_resolve_operands (AML_FATAL_OP, WALK_OPERANDS, walk_state);
	/* Get operands */

	status |= acpi_ds_obj_stack_pop_object (&arg_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&code_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&type_desc, walk_state);
	if (ACPI_FAILURE (status)) {
		/* Invalid parameters on object stack  */

		goto cleanup;
	}


	/* Def_fatal   :=  Fatal_op Fatal_type Fatal_code  Fatal_arg   */


	/*
	 * TBD: [Unhandled] call OSD interface to notify OS of fatal error
	 * requiring shutdown!
	 */


cleanup:

	/* Free the operands */

	acpi_cm_remove_reference (arg_desc);
	acpi_cm_remove_reference (code_desc);
	acpi_cm_remove_reference (type_desc);


	/* If we get back from the OS call, we might as well keep going. */

	REPORT_WARNING (("An AML \"fatal\" Opcode (Fatal_op) was executed\n"));
	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_index
 *
 * PARAMETERS:  none
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Index operator
 *
 * ALLOCATION:  Deletes one operand descriptor -- other remains on stack
 *
 *  ACPI SPECIFICATION REFERENCES:
 *  Def_index   :=  Index_op Buff_pkg_obj Index_value Result
 *  Index_value :=  Term_arg=>Integer
 *  Name_string :=  <Root_char Name_path> | <Prefix_path Name_path>
 *  Result      :=  Super_name
 *  Super_name  :=  Name_string | Arg_obj | Local_obj | Debug_obj | Def_index
 *                             Local4_op | Local5_op | Local6_op | Local7_op
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_exec_index (
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     **return_desc)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *idx_desc;
	ACPI_OPERAND_OBJECT     *res_desc;
	ACPI_OPERAND_OBJECT     *ret_desc = NULL;
	ACPI_OPERAND_OBJECT     *tmp_desc;
	ACPI_STATUS             status;


	/* Resolve operands */
	/* First operand can be either a package or a buffer */

	status = acpi_aml_resolve_operands (AML_INDEX_OP, WALK_OPERANDS, walk_state);
	/* Get all operands */

	status |= acpi_ds_obj_stack_pop_object (&res_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&idx_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&obj_desc, walk_state);
	if (ACPI_FAILURE (status)) {
		/* Invalid parameters on object stack  */

		goto cleanup;
	}


	/* Create the internal return object */

	ret_desc = acpi_cm_create_internal_object (INTERNAL_TYPE_REFERENCE);
	if (!ret_desc) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}


	/*
	 * At this point, the Obj_desc operand is either a Package or a Buffer
	 */

	if (obj_desc->common.type == ACPI_TYPE_PACKAGE) {
		/* Object to be indexed is a Package */

		if (idx_desc->integer.value >= obj_desc->package.count) {
			status = AE_AML_PACKAGE_LIMIT;
			goto cleanup;
		}

		if ((res_desc->common.type == INTERNAL_TYPE_REFERENCE) &&
			(res_desc->reference.opcode == AML_ZERO_OP)) {
			/*
			 * There is no actual result descriptor (the Zero_op Result
			 * descriptor is a placeholder), so just delete the placeholder and
			 * return a reference to the package element
			 */

			acpi_cm_remove_reference (res_desc);
		}

		else {
			/*
			 * Each element of the package is an internal object.  Get the one
			 * we are after.
			 */

			tmp_desc                      = obj_desc->package.elements[idx_desc->integer.value];
			ret_desc->reference.opcode    = AML_INDEX_OP;
			ret_desc->reference.target_type = tmp_desc->common.type;
			ret_desc->reference.object    = tmp_desc;

			status = acpi_aml_exec_store (ret_desc, res_desc, walk_state);
			ret_desc->reference.object    = NULL;
		}

		/*
		 * The local return object must always be a reference to the package element,
		 * not the element itself.
		 */
		ret_desc->reference.opcode    = AML_INDEX_OP;
		ret_desc->reference.target_type = ACPI_TYPE_PACKAGE;
		ret_desc->reference.where     = &obj_desc->package.elements[idx_desc->integer.value];
	}

	else {
		/* Object to be indexed is a Buffer */

		if (idx_desc->integer.value >= obj_desc->buffer.length) {
			status = AE_AML_BUFFER_LIMIT;
			goto cleanup;
		}

		ret_desc->reference.opcode      = AML_INDEX_OP;
		ret_desc->reference.target_type = ACPI_TYPE_BUFFER_FIELD;
		ret_desc->reference.object      = obj_desc;
		ret_desc->reference.offset      = (u32) idx_desc->integer.value;

		status = acpi_aml_exec_store (ret_desc, res_desc, walk_state);
	}


cleanup:

	/* Always delete operands */

	acpi_cm_remove_reference (obj_desc);
	acpi_cm_remove_reference (idx_desc);

	/* Delete return object on error */

	if (ACPI_FAILURE (status)) {
		acpi_cm_remove_reference (res_desc);

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
 * FUNCTION:    Acpi_aml_exec_match
 *
 * PARAMETERS:  none
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Match operator
 *
 * ACPI SPECIFICATION REFERENCES:
 *  Def_match   :=  Match_op Search_pkg Opcode1     Operand1
 *                              Opcode2 Operand2    Start_index
 *  Opcode1     :=  Byte_data: MTR, MEQ, MLE, MLT, MGE, or MGT
 *  Opcode2     :=  Byte_data: MTR, MEQ, MLE, MLT, MGE, or MGT
 *  Operand1    :=  Term_arg=>Integer
 *  Operand2    :=  Term_arg=>Integer
 *  Search_pkg  :=  Term_arg=>Package_object
 *  Start_index :=  Term_arg=>Integer
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_exec_match (
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     **return_desc)
{
	ACPI_OPERAND_OBJECT     *pkg_desc;
	ACPI_OPERAND_OBJECT     *op1_desc;
	ACPI_OPERAND_OBJECT     *V1_desc;
	ACPI_OPERAND_OBJECT     *op2_desc;
	ACPI_OPERAND_OBJECT     *V2_desc;
	ACPI_OPERAND_OBJECT     *start_desc;
	ACPI_OPERAND_OBJECT     *ret_desc = NULL;
	ACPI_STATUS             status;
	u32                     index;
	u32                     match_value = (u32) -1;


	/* Resolve all operands */

	status = acpi_aml_resolve_operands (AML_MATCH_OP, WALK_OPERANDS, walk_state);
	/* Get all operands */

	status |= acpi_ds_obj_stack_pop_object (&start_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&V2_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&op2_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&V1_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&op1_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&pkg_desc, walk_state);

	if (ACPI_FAILURE (status)) {
		/* Invalid parameters on object stack  */

		goto cleanup;
	}

	/* Validate match comparison sub-opcodes */

	if ((op1_desc->integer.value > MAX_MATCH_OPERATOR) ||
		(op2_desc->integer.value > MAX_MATCH_OPERATOR)) {
		status = AE_AML_OPERAND_VALUE;
		goto cleanup;
	}

	index = (u32) start_desc->integer.value;
	if (index >= (u32) pkg_desc->package.count) {
		status = AE_AML_PACKAGE_LIMIT;
		goto cleanup;
	}

	ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
	if (!ret_desc) {
		status = AE_NO_MEMORY;
		goto cleanup;

	}

	/*
	 * Examine each element until a match is found.  Within the loop,
	 * "continue" signifies that the current element does not match
	 * and the next should be examined.
	 * Upon finding a match, the loop will terminate via "break" at
	 * the bottom.  If it terminates "normally", Match_value will be -1
	 * (its initial value) indicating that no match was found.  When
	 * returned as a Number, this will produce the Ones value as specified.
	 */

	for ( ; index < pkg_desc->package.count; ++index) {
		/*
		 * Treat any NULL or non-numeric elements as non-matching.
		 * TBD [Unhandled] - if an element is a Name,
		 *      should we examine its value?
		 */
		if (!pkg_desc->package.elements[index] ||
			ACPI_TYPE_INTEGER != pkg_desc->package.elements[index]->common.type) {
			continue;
		}

		/*
		 * Within these switch statements:
		 *      "break" (exit from the switch) signifies a match;
		 *      "continue" (proceed to next iteration of enclosing
		 *          "for" loop) signifies a non-match.
		 */
		switch (op1_desc->integer.value) {

		case MATCH_MTR:   /* always true */

			break;


		case MATCH_MEQ:   /* true if equal   */

			if (pkg_desc->package.elements[index]->integer.value
				 != V1_desc->integer.value) {
				continue;
			}
			break;


		case MATCH_MLE:   /* true if less than or equal  */

			if (pkg_desc->package.elements[index]->integer.value
				 > V1_desc->integer.value) {
				continue;
			}
			break;


		case MATCH_MLT:   /* true if less than   */

			if (pkg_desc->package.elements[index]->integer.value
				 >= V1_desc->integer.value) {
				continue;
			}
			break;


		case MATCH_MGE:   /* true if greater than or equal   */

			if (pkg_desc->package.elements[index]->integer.value
				 < V1_desc->integer.value) {
				continue;
			}
			break;


		case MATCH_MGT:   /* true if greater than    */

			if (pkg_desc->package.elements[index]->integer.value
				 <= V1_desc->integer.value) {
				continue;
			}
			break;


		default:    /* undefined   */

			continue;
		}


		switch(op2_desc->integer.value) {

		case MATCH_MTR:

			break;


		case MATCH_MEQ:

			if (pkg_desc->package.elements[index]->integer.value
				 != V2_desc->integer.value) {
				continue;
			}
			break;


		case MATCH_MLE:

			if (pkg_desc->package.elements[index]->integer.value
				 > V2_desc->integer.value) {
				continue;
			}
			break;


		case MATCH_MLT:

			if (pkg_desc->package.elements[index]->integer.value
				 >= V2_desc->integer.value) {
				continue;
			}
			break;


		case MATCH_MGE:

			if (pkg_desc->package.elements[index]->integer.value
				 < V2_desc->integer.value) {
				continue;
			}
			break;


		case MATCH_MGT:

			if (pkg_desc->package.elements[index]->integer.value
				 <= V2_desc->integer.value) {
				continue;
			}
			break;


		default:

			continue;
		}

		/* Match found: exit from loop */

		match_value = index;
		break;
	}

	/* Match_value is the return value */

	ret_desc->integer.value = match_value;


cleanup:

	/* Free the operands */

	acpi_cm_remove_reference (start_desc);
	acpi_cm_remove_reference (V2_desc);
	acpi_cm_remove_reference (op2_desc);
	acpi_cm_remove_reference (V1_desc);
	acpi_cm_remove_reference (op1_desc);
	acpi_cm_remove_reference (pkg_desc);


	/* Delete return object on error */

	if (ACPI_FAILURE (status) &&
		(ret_desc)) {
		acpi_cm_remove_reference (ret_desc);
		ret_desc = NULL;
	}


	/* Set the return object and exit */

	*return_desc = ret_desc;
	return (status);
}
