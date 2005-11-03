/******************************************************************************
 *
 * Module Name: dsmethod - Parser/Interpreter interface - control method parsing
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
#include "actables.h"
#include "acdebug.h"


#define _COMPONENT          ACPI_DISPATCHER
	 MODULE_NAME         ("dsmethod")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_parse_method
 *
 * PARAMETERS:  Obj_handle      - Node of the method
 *              Level           - Current nesting level
 *              Context         - Points to a method counter
 *              Return_value    - Not used
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Call the parser and parse the AML that is
 *              associated with the method.
 *
 * MUTEX:       Assumes parser is locked
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_parse_method (
	ACPI_HANDLE             obj_handle)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_PARSE_OBJECT       *op;
	ACPI_NAMESPACE_NODE     *node;
	ACPI_OWNER_ID           owner_id;


	/* Parameter Validation */

	if (!obj_handle) {
		return (AE_NULL_ENTRY);
	}


	/* Extract the method object from the method Node */

	node = (ACPI_NAMESPACE_NODE *) obj_handle;
	obj_desc = node->object;
	if (!obj_desc) {
		return (AE_NULL_OBJECT);
	}

	 /* Create a mutex for the method if there is a concurrency limit */

	if ((obj_desc->method.concurrency != INFINITE_CONCURRENCY) &&
		(!obj_desc->method.semaphore)) {
		status = acpi_os_create_semaphore (obj_desc->method.concurrency,
				   obj_desc->method.concurrency,
				   &obj_desc->method.semaphore);
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	/*
	 * Allocate a new parser op to be the root of the parsed
	 * method tree
	 */
	op = acpi_ps_alloc_op (AML_METHOD_OP);
	if (!op) {
		return (AE_NO_MEMORY);
	}

	/* Init new op with the method name and pointer back to the Node */

	acpi_ps_set_name (op, node->name);
	op->node = node;


	/*
	 * Parse the method, first pass
	 *
	 * The first pass load is
	 * where newly declared named objects are
	 * added into the namespace.  Actual evaluation of
	 * the named objects (what would be called a "second
	 * pass") happens during the actual execution of the
	 * method so that operands to the named objects can
	 * take on dynamic run-time values.
	 */
	status = acpi_ps_parse_aml (op, obj_desc->method.pcode,
			   obj_desc->method.pcode_length,
			   ACPI_PARSE_LOAD_PASS1 | ACPI_PARSE_DELETE_TREE,
			   node, NULL, NULL,
			   acpi_ds_load1_begin_op, acpi_ds_load1_end_op);

	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Get a new Owner_id for objects created by this method */

	owner_id = acpi_cm_allocate_owner_id (OWNER_TYPE_METHOD);
	obj_desc->method.owning_id = owner_id;

	/* Install the parsed tree in the method object */
	/* TBD: [Restructure] Obsolete field? */

	acpi_ps_delete_parse_tree (op);


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_begin_method_execution
 *
 * PARAMETERS:  Method_node         - Node of the method
 *              Obj_desc            - The method object
 *              Calling_method_node - Caller of this method (if non-null)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Prepare a method for execution.  Parses the method if necessary,
 *              increments the thread count, and waits at the method semaphore
 *              for clearance to execute.
 *
 * MUTEX:       Locks/unlocks parser.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_begin_method_execution (
	ACPI_NAMESPACE_NODE     *method_node,
	ACPI_OPERAND_OBJECT     *obj_desc,
	ACPI_NAMESPACE_NODE     *calling_method_node)
{
	ACPI_STATUS             status = AE_OK;


	if (!method_node) {
		return (AE_NULL_ENTRY);
	}


	/*
	 * If there is a concurrency limit on this method, we need to
	 * obtain a unit from the method semaphore.
	 */
	if (obj_desc->method.semaphore) {
		/*
		 * Allow recursive method calls, up to the reentrancy/concurrency
		 * limit imposed by the SERIALIZED rule and the Sync_level method
		 * parameter.
		 *
		 * The point of this code is to avoid permanently blocking a
		 * thread that is making recursive method calls.
		 */
		if (method_node == calling_method_node) {
			if (obj_desc->method.thread_count >= obj_desc->method.concurrency) {
				return (AE_AML_METHOD_LIMIT);
			}
		}

		/*
		 * Get a unit from the method semaphore. This releases the
		 * interpreter if we block
		 */
		status = acpi_aml_system_wait_semaphore (obj_desc->method.semaphore,
				 WAIT_FOREVER);
	}


	/*
	 * Increment the method parse tree thread count since it has been
	 * reentered one more time (even if it is the same thread)
	 */
	obj_desc->method.thread_count++;

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_call_control_method
 *
 * PARAMETERS:  Walk_state          - Current state of the walk
 *              Op                  - Current Op to be walked
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transfer execution to a called control method
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_call_control_method (
	ACPI_WALK_LIST          *walk_list,
	ACPI_WALK_STATE         *this_walk_state,
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_STATUS             status;
	ACPI_NAMESPACE_NODE     *method_node;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_WALK_STATE         *next_walk_state;
	ACPI_PARSE_STATE        *parser_state;
	u32                     i;


	/*
	 * Get the namespace entry for the control method we are about to call
	 */
	method_node = this_walk_state->method_call_node;
	if (!method_node) {
		return (AE_NULL_ENTRY);
	}

	obj_desc = acpi_ns_get_attached_object (method_node);
	if (!obj_desc) {
		return (AE_NULL_OBJECT);
	}


	/* Init for new method, wait on concurrency semaphore */

	status = acpi_ds_begin_method_execution (method_node, obj_desc,
			  this_walk_state->method_node);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Create and initialize a new parser state */

	parser_state = acpi_ps_create_state (obj_desc->method.pcode,
			   obj_desc->method.pcode_length);
	if (!parser_state) {
		return (AE_NO_MEMORY);
	}

	acpi_ps_init_scope (parser_state, NULL);
	parser_state->start_node = method_node;


	/* Create a new state for the preempting walk */

	next_walk_state = acpi_ds_create_walk_state (obj_desc->method.owning_id,
			  NULL, obj_desc, walk_list);
	if (!next_walk_state) {
		/* TBD: delete parser state */

		return (AE_NO_MEMORY);
	}

	next_walk_state->walk_type          = WALK_METHOD;
	next_walk_state->method_node        = method_node;
	next_walk_state->parser_state       = parser_state;
	next_walk_state->parse_flags        = this_walk_state->parse_flags;
	next_walk_state->descending_callback = this_walk_state->descending_callback;
	next_walk_state->ascending_callback = this_walk_state->ascending_callback;

	/* The Next_op of the Next_walk will be the beginning of the method */
	/* TBD: [Restructure] -- obsolete? */

	next_walk_state->next_op = NULL;

	/* Open a new scope */

	status = acpi_ds_scope_stack_push (method_node,
			   ACPI_TYPE_METHOD, next_walk_state);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}


	/*
	 * Initialize the arguments for the method.  The resolved
	 * arguments were put on the previous walk state's operand
	 * stack.  Operands on the previous walk state stack always
	 * start at index 0.
	 */
	status = acpi_ds_method_data_init_args (&this_walk_state->operands[0],
			 this_walk_state->num_operands,
			 next_walk_state);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}


	/* Create and init a Root Node */

	op = acpi_ps_alloc_op (AML_SCOPE_OP);
	if (!op) {
		return (AE_NO_MEMORY);
	}

	status = acpi_ps_parse_aml (op, obj_desc->method.pcode,
			  obj_desc->method.pcode_length,
			  ACPI_PARSE_LOAD_PASS1 | ACPI_PARSE_DELETE_TREE,
			  method_node, NULL, NULL,
			  acpi_ds_load1_begin_op, acpi_ds_load1_end_op);
	acpi_ps_delete_parse_tree (op);


	/*
	 * Delete the operands on the previous walkstate operand stack
	 * (they were copied to new objects)
	 */
	for (i = 0; i < obj_desc->method.param_count; i++) {
		acpi_cm_remove_reference (this_walk_state->operands [i]);
		this_walk_state->operands [i] = NULL;
	}

	/* Clear the operand stack */

	this_walk_state->num_operands = 0;


	return (AE_OK);


	/* On error, we must delete the new walk state */

cleanup:
	acpi_ds_terminate_control_method (next_walk_state);
	acpi_ds_delete_walk_state (next_walk_state);
	return (status);

}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_restart_control_method
 *
 * PARAMETERS:  Walk_state          - State of the method when it was preempted
 *              Op                  - Pointer to new current op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Restart a method that was preempted
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_restart_control_method (
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     *return_desc)
{
	ACPI_STATUS             status;


	if (return_desc) {
		if (walk_state->return_used) {
			/*
			 * Get the return value (if any) from the previous method.
			 * NULL if no return value
			 */
			status = acpi_ds_result_push (return_desc, walk_state);
			if (ACPI_FAILURE (status)) {
				acpi_cm_remove_reference (return_desc);
				return (status);
			}
		}

		else {
			/*
			 * Delete the return value if it will not be used by the
			 * calling method
			 */
			acpi_cm_remove_reference (return_desc);
		}

	}


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_terminate_control_method
 *
 * PARAMETERS:  Walk_state          - State of the method
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Terminate a control method.  Delete everything that the method
 *              created, delete all locals and arguments, and delete the parse
 *              tree if requested.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_terminate_control_method (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_NAMESPACE_NODE     *method_node;


	/* The method object should be stored in the walk state */

	obj_desc = walk_state->method_desc;
	if (!obj_desc) {
		return (AE_OK);
	}

	/* Delete all arguments and locals */

	acpi_ds_method_data_delete_all (walk_state);

	/*
	 * Lock the parser while we terminate this method.
	 * If this is the last thread executing the method,
	 * we have additional cleanup to perform
	 */
	acpi_cm_acquire_mutex (ACPI_MTX_PARSER);


	/* Signal completion of the execution of this method if necessary */

	if (walk_state->method_desc->method.semaphore) {
		status = acpi_os_signal_semaphore (
				 walk_state->method_desc->method.semaphore, 1);
	}

	/* Decrement the thread count on the method parse tree */

	walk_state->method_desc->method.thread_count--;
	if (!walk_state->method_desc->method.thread_count) {
		/*
		 * There are no more threads executing this method.  Perform
		 * additional cleanup.
		 *
		 * The method Node is stored in the walk state
		 */
		method_node = walk_state->method_node;

		/*
		 * Delete any namespace entries created immediately underneath
		 * the method
		 */
		acpi_cm_acquire_mutex (ACPI_MTX_NAMESPACE);
		if (method_node->child) {
			acpi_ns_delete_namespace_subtree (method_node);
		}

		/*
		 * Delete any namespace entries created anywhere else within
		 * the namespace
		 */
		acpi_ns_delete_namespace_by_owner (walk_state->method_desc->method.owning_id);
		acpi_cm_release_mutex (ACPI_MTX_NAMESPACE);
	}

	acpi_cm_release_mutex (ACPI_MTX_PARSER);
	return (AE_OK);
}


