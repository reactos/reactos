/******************************************************************************
 *
 * Module Name: dswexec - Dispatcher method execution callbacks;
 *                        dispatch to interpreter.
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
#include "acdebug.h"


#define _COMPONENT          ACPI_DISPATCHER
	 MODULE_NAME         ("dswexec")


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_get_predicate_value
 *
 * PARAMETERS:  Walk_state      - Current state of the parse tree walk
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get the result of a predicate evaluation
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_get_predicate_value (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	u32                     has_result_obj)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OPERAND_OBJECT     *obj_desc;


	walk_state->control_state->common.state = 0;

	if (has_result_obj) {
		status = acpi_ds_result_pop (&obj_desc, walk_state);
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	else {
		status = acpi_ds_create_operand (walk_state, op, 0);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		status = acpi_aml_resolve_to_value (&walk_state->operands [0], walk_state);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		obj_desc = walk_state->operands [0];
	}

	if (!obj_desc) {
		return (AE_AML_NO_OPERAND);
	}


	/*
	 * Result of predicate evaluation currently must
	 * be a number
	 */

	if (obj_desc->common.type != ACPI_TYPE_INTEGER) {
		status = AE_AML_OPERAND_TYPE;
		goto cleanup;
	}


	/* Truncate the predicate to 32-bits if necessary */

	acpi_aml_truncate_for32bit_table (obj_desc, walk_state);

	/*
	 * Save the result of the predicate evaluation on
	 * the control stack
	 */

	if (obj_desc->integer.value) {
		walk_state->control_state->common.value = TRUE;
	}

	else {
		/*
		 * Predicate is FALSE, we will just toss the
		 * rest of the package
		 */

		walk_state->control_state->common.value = FALSE;
		status = AE_CTRL_FALSE;
	}


cleanup:

	 /* Break to debugger to display result */

	DEBUGGER_EXEC (acpi_db_display_result_object (obj_desc, walk_state));

	/*
	 * Delete the predicate result object (we know that
	 * we don't need it anymore)
	 */

	acpi_cm_remove_reference (obj_desc);

	walk_state->control_state->common.state = CONTROL_NORMAL;

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_exec_begin_op
 *
 * PARAMETERS:  Walk_state      - Current state of the parse tree walk
 *              Op              - Op that has been just been reached in the
 *                                walk;  Arguments have not been evaluated yet.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Descending callback used during the execution of control
 *              methods.  This is where most operators and operands are
 *              dispatched to the interpreter.
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_exec_begin_op (
	u16                     opcode,
	ACPI_PARSE_OBJECT       *op,
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       **out_op)
{
	ACPI_OPCODE_INFO        *op_info;
	ACPI_STATUS             status = AE_OK;


	if (!op) {
		status = acpi_ds_load2_begin_op (opcode, NULL, walk_state, out_op);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		op = *out_op;
	}

	if (op == walk_state->origin) {
		if (out_op) {
			*out_op = op;
		}

		return (AE_OK);
	}

	/*
	 * If the previous opcode was a conditional, this opcode
	 * must be the beginning of the associated predicate.
	 * Save this knowledge in the current scope descriptor
	 */

	if ((walk_state->control_state) &&
		(walk_state->control_state->common.state ==
			CONTROL_CONDITIONAL_EXECUTING)) {
		walk_state->control_state->common.state = CONTROL_PREDICATE_EXECUTING;

		/* Save start of predicate */

		walk_state->control_state->control.predicate_op = op;
	}


	op_info = acpi_ps_get_opcode_info (op->opcode);

	/* We want to send namepaths to the load code */

	if (op->opcode == AML_NAMEPATH_OP) {
		op_info->flags = OPTYPE_NAMED_OBJECT;
	}


	/*
	 * Handle the opcode based upon the opcode type
	 */

	switch (ACPI_GET_OP_CLASS (op_info)) {
	case OPTYPE_CONTROL:

		status = acpi_ds_result_stack_push (walk_state);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		status = acpi_ds_exec_begin_control_op (walk_state, op);
		break;


	case OPTYPE_NAMED_OBJECT:

		if (walk_state->walk_type == WALK_METHOD) {
			/*
			 * Found a named object declaration during method
			 * execution;  we must enter this object into the
			 * namespace.  The created object is temporary and
			 * will be deleted upon completion of the execution
			 * of this method.
			 */

			status = acpi_ds_load2_begin_op (op->opcode, op, walk_state, NULL);
		}


		if (op->opcode == AML_REGION_OP) {
			status = acpi_ds_result_stack_push (walk_state);
		}

		break;


	/* most operators with arguments */

	case OPTYPE_MONADIC1:
	case OPTYPE_DYADIC1:
	case OPTYPE_MONADIC2:
	case OPTYPE_MONADIC2_r:
	case OPTYPE_DYADIC2:
	case OPTYPE_DYADIC2_r:
	case OPTYPE_DYADIC2_s:
	case OPTYPE_RECONFIGURATION:
	case OPTYPE_INDEX:
	case OPTYPE_MATCH:
	case OPTYPE_FATAL:
	case OPTYPE_CREATE_FIELD:

		/* Start a new result/operand state */

		status = acpi_ds_result_stack_push (walk_state);
		break;


	default:
		break;
	}

	/* Nothing to do here during method execution */

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_exec_end_op
 *
 * PARAMETERS:  Walk_state      - Current state of the parse tree walk
 *              Op              - Op that has been just been completed in the
 *                                walk;  Arguments have now been evaluated.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Ascending callback used during the execution of control
 *              methods.  The only thing we really need to do here is to
 *              notice the beginning of IF, ELSE, and WHILE blocks.
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_exec_end_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_STATUS             status = AE_OK;
	u16                     opcode;
	u8                      optype;
	ACPI_PARSE_OBJECT       *next_op;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_PARSE_OBJECT       *first_arg;
	ACPI_OPERAND_OBJECT     *result_obj = NULL;
	ACPI_OPCODE_INFO        *op_info;
	u32                     operand_index;


	opcode = (u16) op->opcode;


	op_info = acpi_ps_get_opcode_info (op->opcode);
	if (ACPI_GET_OP_TYPE (op_info) != ACPI_OP_TYPE_OPCODE) {
		return (AE_NOT_IMPLEMENTED);
	}

	optype = (u8) ACPI_GET_OP_CLASS (op_info);
	first_arg = op->value.arg;

	/* Init the walk state */

	walk_state->num_operands = 0;
	walk_state->return_desc = NULL;
	walk_state->op_info = op_info;
	walk_state->opcode = opcode;


	/* Call debugger for single step support (DEBUG build only) */

	DEBUGGER_EXEC (status = acpi_db_single_step (walk_state, op, optype));
	DEBUGGER_EXEC (if (ACPI_FAILURE (status)) {return (status);});


	/* Decode the opcode */

	switch (optype) {
	case OPTYPE_UNDEFINED:

		return (AE_NOT_IMPLEMENTED);
		break;


	case OPTYPE_BOGUS:
		break;

	case OPTYPE_CONSTANT:           /* argument type only */
	case OPTYPE_LITERAL:            /* argument type only */
	case OPTYPE_DATA_TERM:          /* argument type only */
	case OPTYPE_LOCAL_VARIABLE:     /* argument type only */
	case OPTYPE_METHOD_ARGUMENT:    /* argument type only */
		break;


	/* most operators with arguments */

	case OPTYPE_MONADIC1:
	case OPTYPE_DYADIC1:
	case OPTYPE_MONADIC2:
	case OPTYPE_MONADIC2_r:
	case OPTYPE_DYADIC2:
	case OPTYPE_DYADIC2_r:
	case OPTYPE_DYADIC2_s:
	case OPTYPE_RECONFIGURATION:
	case OPTYPE_INDEX:
	case OPTYPE_MATCH:
	case OPTYPE_FATAL:


		/* Build resolved operand stack */

		status = acpi_ds_create_operands (walk_state, first_arg);
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}

		operand_index = walk_state->num_operands - 1;


		/* Done with this result state (Now that operand stack is built) */

		status = acpi_ds_result_stack_pop (walk_state);
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}

		switch (optype) {
		case OPTYPE_MONADIC1:

			/* 1 Operand, 0 External_result, 0 Internal_result */

			status = acpi_aml_exec_monadic1 (opcode, walk_state);
			break;


		case OPTYPE_MONADIC2:

			/* 1 Operand, 0 External_result, 1 Internal_result */

			status = acpi_aml_exec_monadic2 (opcode, walk_state, &result_obj);
			break;


		case OPTYPE_MONADIC2_r:

			/* 1 Operand, 1 External_result, 1 Internal_result */

			status = acpi_aml_exec_monadic2_r (opcode, walk_state, &result_obj);
			break;


		case OPTYPE_DYADIC1:

			/* 2 Operands, 0 External_result, 0 Internal_result */

			status = acpi_aml_exec_dyadic1 (opcode, walk_state);
			break;


		case OPTYPE_DYADIC2:

			/* 2 Operands, 0 External_result, 1 Internal_result */

			status = acpi_aml_exec_dyadic2 (opcode, walk_state, &result_obj);
			break;


		case OPTYPE_DYADIC2_r:

			/* 2 Operands, 1 or 2 External_results, 1 Internal_result */

			status = acpi_aml_exec_dyadic2_r (opcode, walk_state, &result_obj);
			break;


		case OPTYPE_DYADIC2_s:  /* Synchronization Operator */

			/* 2 Operands, 0 External_result, 1 Internal_result */

			status = acpi_aml_exec_dyadic2_s (opcode, walk_state, &result_obj);
			break;


		case OPTYPE_INDEX:  /* Type 2 opcode with 3 operands */

			/* 3 Operands, 1 External_result, 1 Internal_result */

			status = acpi_aml_exec_index (walk_state, &result_obj);
			break;


		case OPTYPE_MATCH:  /* Type 2 opcode with 6 operands */

			/* 6 Operands, 0 External_result, 1 Internal_result */

			status = acpi_aml_exec_match (walk_state, &result_obj);
			break;


		case OPTYPE_RECONFIGURATION:

			/* 1 or 2 operands, 0 Internal Result */

			status = acpi_aml_exec_reconfiguration (opcode, walk_state);
			break;


		case OPTYPE_FATAL:

			/* 3 Operands, 0 External_result, 0 Internal_result */

			status = acpi_aml_exec_fatal (walk_state);
			break;
		}

		/*
		 * If a result object was returned from above, push it on the
		 * current result stack
		 */
		if (ACPI_SUCCESS (status) &&
			result_obj) {
			status = acpi_ds_result_push (result_obj, walk_state);
		}

		break;


	case OPTYPE_CONTROL:    /* Type 1 opcode, IF/ELSE/WHILE/NOOP */

		/* 1 Operand, 0 External_result, 0 Internal_result */

		status = acpi_ds_exec_end_control_op (walk_state, op);

		acpi_ds_result_stack_pop (walk_state);
		break;


	case OPTYPE_METHOD_CALL:

		/*
		 * (AML_METHODCALL) Op->Value->Arg->Node contains
		 * the method Node pointer
		 */
		/* Next_op points to the op that holds the method name */

		next_op = first_arg;
		node = next_op->node;

		/* Next_op points to first argument op */

		next_op = next_op->next;

		/*
		 * Get the method's arguments and put them on the operand stack
		 */
		status = acpi_ds_create_operands (walk_state, next_op);
		if (ACPI_FAILURE (status)) {
			break;
		}

		/*
		 * Since the operands will be passed to another
		 * control method, we must resolve all local
		 * references here (Local variables, arguments
		 * to *this* method, etc.)
		 */

		status = acpi_ds_resolve_operands (walk_state);
		if (ACPI_FAILURE (status)) {
			break;
		}

		/*
		 * Tell the walk loop to preempt this running method and
		 * execute the new method
		 */
		status = AE_CTRL_TRANSFER;

		/*
		 * Return now; we don't want to disturb anything,
		 * especially the operand count!
		 */
		return (status);
		break;


	case OPTYPE_CREATE_FIELD:

		status = acpi_ds_load2_end_op (walk_state, op);
		if (ACPI_FAILURE (status)) {
			break;
		}

		status = acpi_ds_eval_field_unit_operands (walk_state, op);
		break;


	case OPTYPE_NAMED_OBJECT:

		status = acpi_ds_load2_end_op (walk_state, op);
		if (ACPI_FAILURE (status)) {
			break;
		}

		switch (op->opcode) {
		case AML_REGION_OP:

			status = acpi_ds_eval_region_operands (walk_state, op);
			if (ACPI_FAILURE (status)) {
				break;
			}

			status = acpi_ds_result_stack_pop (walk_state);
			break;


		case AML_METHOD_OP:
			break;


		case AML_ALIAS_OP:

			/* Alias creation was already handled by call
			to psxload above */
			break;


		default:
			/* Nothing needs to be done */

			status = AE_OK;
			break;
		}

		break;

	default:

		status = AE_NOT_IMPLEMENTED;
		break;
	}


	/*
	 * ACPI 2.0 support for 64-bit integers:
	 * Truncate numeric result value if we are executing from a 32-bit ACPI table
	 */
	acpi_aml_truncate_for32bit_table (result_obj, walk_state);

	/*
	 * Check if we just completed the evaluation of a
	 * conditional predicate
	 */

	if ((walk_state->control_state) &&
		(walk_state->control_state->common.state ==
			CONTROL_PREDICATE_EXECUTING) &&
		(walk_state->control_state->control.predicate_op == op)) {
		status = acpi_ds_get_predicate_value (walk_state, op, (u32) result_obj);
		result_obj = NULL;
	}


cleanup:
	if (result_obj) {
		/* Break to debugger to display result */

		DEBUGGER_EXEC (acpi_db_display_result_object (result_obj, walk_state));

		/*
		 * Delete the result op if and only if:
		 * Parent will not use the result -- such as any
		 * non-nested type2 op in a method (parent will be method)
		 */
		acpi_ds_delete_result_if_not_used (op, result_obj, walk_state);
	}

	/* Always clear the object stack */

	/* TBD: [Investigate] Clear stack of return value,
	but don't delete it */
	walk_state->num_operands = 0;

	return (status);
}


