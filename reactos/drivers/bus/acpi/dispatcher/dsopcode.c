/******************************************************************************
 *
 * Module Name: dsopcode - Dispatcher Op Region support and handling of
 *                         "control" opcodes
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
#include "acdispat.h"
#include "acinterp.h"
#include "acnamesp.h"
#include "acevents.h"
#include "actables.h"

#define _COMPONENT          ACPI_DISPATCHER
	 MODULE_NAME         ("dsopcode")


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_get_field_unit_arguments
 *
 * PARAMETERS:  Obj_desc        - A valid Field_unit object
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Get Field_unit Buffer and Index. This implements the late
 *              evaluation of these field attributes.
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_get_field_unit_arguments (
	ACPI_OPERAND_OBJECT     *obj_desc)
{
	ACPI_OPERAND_OBJECT     *extra_desc;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_PARSE_OBJECT       *op;
	ACPI_PARSE_OBJECT       *field_op;
	ACPI_STATUS             status;
	ACPI_TABLE_DESC         *table_desc;


	if (obj_desc->common.flags & AOPOBJ_DATA_VALID) {
		return (AE_OK);
	}


	/* Get the AML pointer (method object) and Field_unit node */

	extra_desc = obj_desc->field_unit.extra;
	node = obj_desc->field_unit.node;

	/*
	 * Allocate a new parser op to be the root of the parsed
	 * Op_region tree
	 */

	op = acpi_ps_alloc_op (AML_SCOPE_OP);
	if (!op) {
		return (AE_NO_MEMORY);
	}

	/* Save the Node for use in Acpi_ps_parse_aml */

	op->node = acpi_ns_get_parent_object (node);

	/* Get a handle to the parent ACPI table */

	status = acpi_tb_handle_to_object (node->owner_id, &table_desc);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Pass1: Parse the entire Field_unit declaration */

	status = acpi_ps_parse_aml (op, extra_desc->extra.pcode,
			  extra_desc->extra.pcode_length, 0,
			  NULL, NULL, NULL, acpi_ds_load1_begin_op, acpi_ds_load1_end_op);
	if (ACPI_FAILURE (status)) {
		acpi_ps_delete_parse_tree (op);
		return (status);
	}


	/* Get and init the actual Fiel_unit_op created above */

	field_op = op->value.arg;
	op->node = node;


	field_op = op->value.arg;
	field_op->node = node;
	acpi_ps_delete_parse_tree (op);

	/* Acpi_evaluate the address and length arguments for the Op_region */

	op = acpi_ps_alloc_op (AML_SCOPE_OP);
	if (!op) {
		return (AE_NO_MEMORY);
	}

	op->node = acpi_ns_get_parent_object (node);

	status = acpi_ps_parse_aml (op, extra_desc->extra.pcode,
			  extra_desc->extra.pcode_length,
			  ACPI_PARSE_EXECUTE | ACPI_PARSE_DELETE_TREE,
			  NULL /*Method_desc*/, NULL, NULL,
			  acpi_ds_exec_begin_op, acpi_ds_exec_end_op);
	/* All done with the parse tree, delete it */

	acpi_ps_delete_parse_tree (op);


	/*
	 * The pseudo-method object is no longer needed since the region is
	 * now initialized
	 */
	acpi_cm_remove_reference (obj_desc->field_unit.extra);
	obj_desc->field_unit.extra = NULL;

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_get_region_arguments
 *
 * PARAMETERS:  Obj_desc        - A valid region object
 *
 * RETURN:      Status.
 *
 * DESCRIPTION: Get region address and length.  This implements the late
 *              evaluation of these region attributes.
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_get_region_arguments (
	ACPI_OPERAND_OBJECT     *obj_desc)
{
	ACPI_OPERAND_OBJECT     *extra_desc = NULL;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_PARSE_OBJECT       *op;
	ACPI_PARSE_OBJECT       *region_op;
	ACPI_STATUS             status;
	ACPI_TABLE_DESC         *table_desc;


	if (obj_desc->region.flags & AOPOBJ_DATA_VALID) {
		return (AE_OK);
	}


	/* Get the AML pointer (method object) and region node */

	extra_desc = obj_desc->region.extra;
	node = obj_desc->region.node;

	/*
	 * Allocate a new parser op to be the root of the parsed
	 * Op_region tree
	 */

	op = acpi_ps_alloc_op (AML_SCOPE_OP);
	if (!op) {
		return (AE_NO_MEMORY);
	}

	/* Save the Node for use in Acpi_ps_parse_aml */

	op->node = acpi_ns_get_parent_object (node);

	/* Get a handle to the parent ACPI table */

	status = acpi_tb_handle_to_object (node->owner_id, &table_desc);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Parse the entire Op_region declaration, creating a parse tree */

	status = acpi_ps_parse_aml (op, extra_desc->extra.pcode,
			  extra_desc->extra.pcode_length, 0,
			  NULL, NULL, NULL, acpi_ds_load1_begin_op, acpi_ds_load1_end_op);

	if (ACPI_FAILURE (status)) {
		acpi_ps_delete_parse_tree (op);
		return (status);
	}


	/* Get and init the actual Region_op created above */

	region_op = op->value.arg;
	op->node = node;


	region_op = op->value.arg;
	region_op->node = node;
	acpi_ps_delete_parse_tree (op);

	/* Acpi_evaluate the address and length arguments for the Op_region */

	op = acpi_ps_alloc_op (AML_SCOPE_OP);
	if (!op) {
		return (AE_NO_MEMORY);
	}

	op->node = acpi_ns_get_parent_object (node);

	status = acpi_ps_parse_aml (op, extra_desc->extra.pcode,
			  extra_desc->extra.pcode_length,
			  ACPI_PARSE_EXECUTE | ACPI_PARSE_DELETE_TREE,
			  NULL /*Method_desc*/, NULL, NULL,
			  acpi_ds_exec_begin_op, acpi_ds_exec_end_op);

	/* All done with the parse tree, delete it */

	acpi_ps_delete_parse_tree (op);

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_initialize_region
 *
 * PARAMETERS:  Op              - A valid region Op object
 *
 * RETURN:      Status
 *
 * DESCRIPTION:
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_initialize_region (
	ACPI_HANDLE             obj_handle)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status;


	obj_desc = acpi_ns_get_attached_object (obj_handle);

	/* Namespace is NOT locked */

	status = acpi_ev_initialize_region (obj_desc, FALSE);

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_eval_field_unit_operands
 *
 * PARAMETERS:  Op              - A valid Field_unit Op object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get Field_unit Buffer and Index
 *              Called from Acpi_ds_exec_end_op during Field_unit parse tree walk
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_eval_field_unit_operands (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *field_desc;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_PARSE_OBJECT       *next_op;
	u32                     offset;
	u32                     bit_offset;
	u16                     bit_count;


	ACPI_OPERAND_OBJECT     *res_desc = NULL;
	ACPI_OPERAND_OBJECT     *cnt_desc = NULL;
	ACPI_OPERAND_OBJECT     *off_desc = NULL;
	ACPI_OPERAND_OBJECT     *src_desc = NULL;
	u32                     num_operands = 3;


	/*
	 * This is where we evaluate the address and length fields of the Op_field_unit declaration
	 */

	node =  op->node;

	/* Next_op points to the op that holds the Buffer */
	next_op = op->value.arg;

	/* Acpi_evaluate/create the address and length operands */

	status = acpi_ds_create_operands (walk_state, next_op);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	field_desc = acpi_ns_get_attached_object (node);
	if (!field_desc) {
		return (AE_NOT_EXIST);
	}


	/* Resolve the operands */

	status = acpi_aml_resolve_operands (op->opcode, WALK_OPERANDS, walk_state);

	/* Get the operands */

	status |= acpi_ds_obj_stack_pop_object (&res_desc, walk_state);
	if (AML_CREATE_FIELD_OP == op->opcode) {
		num_operands = 4;
		status |= acpi_ds_obj_stack_pop_object (&cnt_desc, walk_state);
	}

	status |= acpi_ds_obj_stack_pop_object (&off_desc, walk_state);
	status |= acpi_ds_obj_stack_pop_object (&src_desc, walk_state);

	if (ACPI_FAILURE (status)) {
		/* Invalid parameters on object stack  */

		goto cleanup;
	}


	offset = (u32) off_desc->integer.value;


	/*
	 * If Res_desc is a Name, it will be a direct name pointer after
	 * Acpi_aml_resolve_operands()
	 */

	if (!VALID_DESCRIPTOR_TYPE (res_desc, ACPI_DESC_TYPE_NAMED)) {
		status = AE_AML_OPERAND_TYPE;
		goto cleanup;
	}


	/*
	 * Setup the Bit offsets and counts, according to the opcode
	 */

	switch (op->opcode) {

	/* Def_create_bit_field */

	case AML_BIT_FIELD_OP:

		/* Offset is in bits, Field is a bit */

		bit_offset = offset;
		bit_count = 1;
		break;


	/* Def_create_byte_field */

	case AML_BYTE_FIELD_OP:

		/* Offset is in bytes, field is a byte */

		bit_offset = 8 * offset;
		bit_count = 8;
		break;


	/* Def_create_word_field */

	case AML_WORD_FIELD_OP:

		/* Offset is in bytes, field is a word */

		bit_offset = 8 * offset;
		bit_count = 16;
		break;


	/* Def_create_dWord_field */

	case AML_DWORD_FIELD_OP:

		/* Offset is in bytes, field is a dword */

		bit_offset = 8 * offset;
		bit_count = 32;
		break;


	/* Def_create_field */

	case AML_CREATE_FIELD_OP:

		/* Offset is in bits, count is in bits */

		bit_offset = offset;
		bit_count = (u16) cnt_desc->integer.value;
		break;


	default:

		status = AE_AML_BAD_OPCODE;
		goto cleanup;
	}


	/*
	 * Setup field according to the object type
	 */

	switch (src_desc->common.type) {

	/* Source_buff :=  Term_arg=>Buffer */

	case ACPI_TYPE_BUFFER:

		if (bit_offset + (u32) bit_count >
			(8 * (u32) src_desc->buffer.length)) {
			status = AE_AML_BUFFER_LIMIT;
			goto cleanup;
		}


		/* Construct the remainder of the field object */

		field_desc->field_unit.access     = (u8) ACCESS_ANY_ACC;
		field_desc->field_unit.lock_rule  = (u8) GLOCK_NEVER_LOCK;
		field_desc->field_unit.update_rule = (u8) UPDATE_PRESERVE;
		field_desc->field_unit.length     = bit_count;
		field_desc->field_unit.bit_offset = (u8) (bit_offset % 8);
		field_desc->field_unit.offset     = DIV_8 (bit_offset);
		field_desc->field_unit.container  = src_desc;

		/* Reference count for Src_desc inherits Field_desc count */

		src_desc->common.reference_count = (u16) (src_desc->common.reference_count +
				   field_desc->common.reference_count);

		break;


	/* Improper object type */

	default:



		status = AE_AML_OPERAND_TYPE;
		goto cleanup;
	}


	if (AML_CREATE_FIELD_OP == op->opcode) {
		/* Delete object descriptor unique to Create_field */

		acpi_cm_remove_reference (cnt_desc);
		cnt_desc = NULL;
	}


cleanup:

	/* Always delete the operands */

	acpi_cm_remove_reference (off_desc);
	acpi_cm_remove_reference (src_desc);

	if (AML_CREATE_FIELD_OP == op->opcode) {
		acpi_cm_remove_reference (cnt_desc);
	}

	/* On failure, delete the result descriptor */

	if (ACPI_FAILURE (status)) {
		acpi_cm_remove_reference (res_desc); /* Result descriptor */
	}

	else {
		/* Now the address and length are valid for this op_field_unit */

		field_desc->field_unit.flags |= AOPOBJ_DATA_VALID;
	}

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_eval_region_operands
 *
 * PARAMETERS:  Op              - A valid region Op object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get region address and length
 *              Called from Acpi_ds_exec_end_op during Op_region parse tree walk
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_eval_region_operands (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *operand_desc;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_PARSE_OBJECT       *next_op;


	/*
	 * This is where we evaluate the address and length fields of the Op_region declaration
	 */

	node =  op->node;

	/* Next_op points to the op that holds the Space_iD */
	next_op = op->value.arg;

	/* Next_op points to address op */
	next_op = next_op->next;

	/* Acpi_evaluate/create the address and length operands */

	status = acpi_ds_create_operands (walk_state, next_op);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Resolve the length and address operands to numbers */

	status = acpi_aml_resolve_operands (op->opcode, WALK_OPERANDS, walk_state);
	if (ACPI_FAILURE (status)) {
		return (status);
	}


	obj_desc = acpi_ns_get_attached_object (node);
	if (!obj_desc) {
		return (AE_NOT_EXIST);
	}

	/*
	 * Get the length operand and save it
	 * (at Top of stack)
	 */
	operand_desc = walk_state->operands[walk_state->num_operands - 1];

	obj_desc->region.length = (u32) operand_desc->integer.value;
	acpi_cm_remove_reference (operand_desc);

	/*
	 * Get the address and save it
	 * (at top of stack - 1)
	 */
	operand_desc = walk_state->operands[walk_state->num_operands - 2];

	obj_desc->region.address = (ACPI_PHYSICAL_ADDRESS) operand_desc->integer.value;
	acpi_cm_remove_reference (operand_desc);


	/* Now the address and length are valid for this opregion */

	obj_desc->region.flags |= AOPOBJ_DATA_VALID;

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_exec_begin_control_op
 *
 * PARAMETERS:  Walk_list       - The list that owns the walk stack
 *              Op              - The control Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handles all control ops encountered during control method
 *              execution.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_exec_begin_control_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_GENERIC_STATE      *control_state;


	switch (op->opcode) {
	case AML_IF_OP:
	case AML_WHILE_OP:

		/*
		 * IF/WHILE: Create a new control state to manage these
		 * constructs. We need to manage these as a stack, in order
		 * to handle nesting.
		 */

		control_state = acpi_cm_create_control_state ();
		if (!control_state) {
			status = AE_NO_MEMORY;
			break;
		}

		acpi_cm_push_generic_state (&walk_state->control_state, control_state);

		/*
		 * Save a pointer to the predicate for multiple executions
		 * of a loop
		 */
		walk_state->control_state->control.aml_predicate_start =
				 walk_state->parser_state->aml - 1;
				 /* TBD: can this be removed? */
				 /*Acpi_ps_pkg_length_encoding_size (GET8 (Walk_state->Parser_state->Aml));*/
		break;


	case AML_ELSE_OP:

		/* Predicate is in the state object */
		/* If predicate is true, the IF was executed, ignore ELSE part */

		if (walk_state->last_predicate) {
			status = AE_CTRL_TRUE;
		}

		break;


	case AML_RETURN_OP:

		break;


	default:
		break;
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_exec_end_control_op
 *
 * PARAMETERS:  Walk_list       - The list that owns the walk stack
 *              Op              - The control Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handles all control ops encountered during control method
 *              execution.
 *
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_exec_end_control_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_GENERIC_STATE      *control_state;


	switch (op->opcode) {
	case AML_IF_OP:

		/*
		 * Save the result of the predicate in case there is an
		 * ELSE to come
		 */

		walk_state->last_predicate =
				(u8) walk_state->control_state->common.value;

		/*
		 * Pop the control state that was created at the start
		 * of the IF and free it
		 */

		control_state =
				acpi_cm_pop_generic_state (&walk_state->control_state);

		acpi_cm_delete_generic_state (control_state);

		break;


	case AML_ELSE_OP:

		break;


	case AML_WHILE_OP:

		if (walk_state->control_state->common.value) {
			/* Predicate was true, go back and evaluate it again! */

			status = AE_CTRL_PENDING;
		}


		/* Pop this control state and free it */

		control_state =
				acpi_cm_pop_generic_state (&walk_state->control_state);

		walk_state->aml_last_while = control_state->control.aml_predicate_start;
		acpi_cm_delete_generic_state (control_state);

		break;


	case AML_RETURN_OP:


		/*
		 * One optional operand -- the return value
		 * It can be either an immediate operand or a result that
		 * has been bubbled up the tree
		 */
		if (op->value.arg) {
			/* Return statement has an immediate operand */

			status = acpi_ds_create_operands (walk_state, op->value.arg);
			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/*
			 * If value being returned is a Reference (such as
			 * an arg or local), resolve it now because it may
			 * cease to exist at the end of the method.
			 */
			status = acpi_aml_resolve_to_value (&walk_state->operands [0], walk_state);
			if (ACPI_FAILURE (status)) {
				return (status);
			}

			/*
			 * Get the return value and save as the last result
			 * value.  This is the only place where Walk_state->Return_desc
			 * is set to anything other than zero!
			 */

			walk_state->return_desc = walk_state->operands[0];
		}

		else if ((walk_state->results) &&
				 (walk_state->results->results.num_results > 0)) {
			/*
			 * The return value has come from a previous calculation.
			 *
			 * If value being returned is a Reference (such as
			 * an arg or local), resolve it now because it may
			 * cease to exist at the end of the method.
			 *
			 * Allow references created by the Index operator to return unchanged.
			 */

			if (VALID_DESCRIPTOR_TYPE (walk_state->results->results.obj_desc [0], ACPI_DESC_TYPE_INTERNAL) &&
				((walk_state->results->results.obj_desc [0])->common.type == INTERNAL_TYPE_REFERENCE) &&
				((walk_state->results->results.obj_desc [0])->reference.opcode != AML_INDEX_OP)) {
					status = acpi_aml_resolve_to_value (&walk_state->results->results.obj_desc [0], walk_state);
					if (ACPI_FAILURE (status)) {
						return (status);
					}
			}

			walk_state->return_desc = walk_state->results->results.obj_desc [0];
		}

		else {
			/* No return operand */

			if (walk_state->num_operands) {
				acpi_cm_remove_reference (walk_state->operands [0]);
			}

			walk_state->operands [0]    = NULL;
			walk_state->num_operands    = 0;
			walk_state->return_desc     = NULL;
		}


		/* End the control method execution right now */
		status = AE_CTRL_TERMINATE;
		break;


	case AML_NOOP_OP:

		/* Just do nothing! */
		break;


	case AML_BREAK_POINT_OP:

		/* Call up to the OS dependent layer to handle this */

		acpi_os_breakpoint (NULL);

		/* If it returns, we are done! */

		break;


	case AML_BREAK_OP:

		/*
		 * As per the ACPI specification:
		 *      "The break operation causes the current package
		 *          execution to complete"
		 *      "Break -- Stop executing the current code package
		 *          at this point"
		 *
		 * Returning AE_FALSE here will cause termination of
		 * the current package, and execution will continue one
		 * level up, starting with the completion of the parent Op.
		 */

		status = AE_CTRL_FALSE;
		break;


	default:

		status = AE_AML_BAD_OPCODE;
		break;
	}


	return (status);
}

