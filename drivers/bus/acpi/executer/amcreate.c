/******************************************************************************
 *
 * Module Name: amcreate - Named object creation
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
#include "acnamesp.h"
#include "acevents.h"
#include "acdispat.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amcreate")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_create_field
 *
 * PARAMETERS:  Opcode              - The opcode to be executed
 *              Operands            - List of operands for the opcode
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Create_field operators: Create_bit_field_op,
 *              Create_byte_field_op, Create_word_field_op, Create_dWord_field_op,
 *              Create_field_op (which define fields in buffers)
 *
 * ALLOCATION:  Deletes Create_field_op's count operand descriptor
 *
 *
 *  ACPI SPECIFICATION REFERENCES:
 *  Def_create_bit_field := Create_bit_field_op Src_buf Bit_idx   Name_string
 *  Def_create_byte_field := Create_byte_field_op Src_buf Byte_idx Name_string
 *  Def_create_dWord_field := Create_dWord_field_op Src_buf Byte_idx Name_string
 *  Def_create_field    :=  Create_field_op     Src_buf Bit_idx   Num_bits    Name_string
 *  Def_create_word_field := Create_word_field_op Src_buf Byte_idx Name_string
 *  Bit_index           :=  Term_arg=>Integer
 *  Byte_index          :=  Term_arg=>Integer
 *  Num_bits            :=  Term_arg=>Integer
 *  Source_buff         :=  Term_arg=>Buffer
 *
 ******************************************************************************/


ACPI_STATUS
acpi_aml_exec_create_field (
	u8                      *aml_ptr,
	u32                     aml_length,
	ACPI_NAMESPACE_NODE     *node,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *tmp_desc;


	/* Create the region descriptor */

	obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_FIELD_UNIT);
	if (!obj_desc) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	/* Construct the field object */

	obj_desc->field_unit.access     = (u8) ACCESS_ANY_ACC;
	obj_desc->field_unit.lock_rule  = (u8) GLOCK_NEVER_LOCK;
	obj_desc->field_unit.update_rule = (u8) UPDATE_PRESERVE;

	/*
	 * Allocate a method object for this field unit
	 */

	obj_desc->field_unit.extra = acpi_cm_create_internal_object (
			 INTERNAL_TYPE_EXTRA);
	if (!obj_desc->field_unit.extra) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	/*
	 * Remember location in AML stream of the field unit
	 * opcode and operands -- since the buffer and index
	 * operands must be evaluated.
	 */

	obj_desc->field_unit.extra->extra.pcode     = aml_ptr;
	obj_desc->field_unit.extra->extra.pcode_length = aml_length;
	obj_desc->field_unit.node = node;


	/*
	 * This operation is supposed to cause the destination Name to refer
	 * to the defined Field_unit -- it must not store the constructed
	 * Field_unit object (or its current value) in some location that the
	 * Name may already be pointing to.  So, if the Name currently contains
	 * a reference which would cause Acpi_aml_exec_store() to perform an indirect
	 * store rather than setting the value of the Name itself, clobber that
	 * reference before calling Acpi_aml_exec_store().
	 */

	/* Type of Name's existing value */

	switch (acpi_ns_get_type (node)) {

	case ACPI_TYPE_FIELD_UNIT:

	case INTERNAL_TYPE_ALIAS:
	case INTERNAL_TYPE_BANK_FIELD:
	case INTERNAL_TYPE_DEF_FIELD:
	case INTERNAL_TYPE_INDEX_FIELD:

		tmp_desc = acpi_ns_get_attached_object (node);
		if (tmp_desc) {
			/*
			 * There is an existing object here;  delete it and zero out the
			 * object field within the Node
			 */

			acpi_cm_remove_reference (tmp_desc);
			acpi_ns_attach_object ((ACPI_NAMESPACE_NODE *) node, NULL,
					 ACPI_TYPE_ANY);
		}

		/* Set the type to ANY (or the store below will fail) */

		((ACPI_NAMESPACE_NODE *) node)->type = ACPI_TYPE_ANY;

		break;


	default:

		break;
	}


	/* Store constructed field descriptor in result location */

	status = acpi_aml_exec_store (obj_desc, (ACPI_OPERAND_OBJECT *) node, walk_state);

	/*
	 * If the field descriptor was not physically stored (or if a failure
	 * above), we must delete it
	 */
	if (obj_desc->common.reference_count <= 1) {
		acpi_cm_remove_reference (obj_desc);
	}


	return (AE_OK);


cleanup:

	/* Delete region object and method subobject */

	if (obj_desc) {
		/* Remove deletes both objects! */

		acpi_cm_remove_reference (obj_desc);
		obj_desc = NULL;
	}

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_create_alias
 *
 * PARAMETERS:  Operands            - List of operands for the opcode
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new named alias
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_exec_create_alias (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_NAMESPACE_NODE     *source_node;
	ACPI_NAMESPACE_NODE     *alias_node;
	ACPI_STATUS             status;


	/* Get the source/alias operands (both NTEs) */

	status = acpi_ds_obj_stack_pop_object ((ACPI_OPERAND_OBJECT **) &source_node,
			 walk_state);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/*
	 * Don't pop it, it gets removed in the calling routine
	 */

	alias_node = acpi_ds_obj_stack_get_value (0, walk_state);

	/* Add an additional reference to the object */

	acpi_cm_add_reference (source_node->object);

	/*
	 * Attach the original source Node to the new Alias Node.
	 */
	status = acpi_ns_attach_object (alias_node, source_node->object,
			   source_node->type);


	/*
	 * The new alias assumes the type of the source, but it points
	 * to the same object.  The reference count of the object has two
	 * additional references to prevent deletion out from under either the
	 * source or the alias Node
	 */

	/* Since both operands are NTEs, we don't need to delete them */

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_create_event
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new event object
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_exec_create_event (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *obj_desc;


 BREAKPOINT3;

	obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_EVENT);
	if (!obj_desc) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	/* Create the actual OS semaphore */

	/* TBD: [Investigate] should be created with 0 or 1 units? */

	status = acpi_os_create_semaphore (ACPI_NO_UNIT_LIMIT, 1,
			   &obj_desc->event.semaphore);
	if (ACPI_FAILURE (status)) {
		acpi_cm_remove_reference (obj_desc);
		goto cleanup;
	}

	/* Attach object to the Node */

	status = acpi_ns_attach_object (acpi_ds_obj_stack_get_value (0, walk_state),
			   obj_desc, (u8) ACPI_TYPE_EVENT);
	if (ACPI_FAILURE (status)) {
		acpi_os_delete_semaphore (obj_desc->event.semaphore);
		acpi_cm_remove_reference (obj_desc);
		goto cleanup;
	}


cleanup:

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_create_mutex
 *
 * PARAMETERS:  Interpreter_mode    - Current running mode (load1/Load2/Exec)
 *              Operands            - List of operands for the opcode
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new mutex object
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_exec_create_mutex (
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OPERAND_OBJECT     *sync_desc;
	ACPI_OPERAND_OBJECT     *obj_desc;


	/* Get the operand */

	status = acpi_ds_obj_stack_pop_object (&sync_desc, walk_state);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Attempt to allocate a new object */

	obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_MUTEX);
	if (!obj_desc) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	/* Create the actual OS semaphore */

	status = acpi_os_create_semaphore (1, 1, &obj_desc->mutex.semaphore);
	if (ACPI_FAILURE (status)) {
		acpi_cm_remove_reference (obj_desc);
		goto cleanup;
	}

	obj_desc->mutex.sync_level = (u8) sync_desc->integer.value;

	/* Obj_desc was on the stack top, and the name is below it */

	status = acpi_ns_attach_object (acpi_ds_obj_stack_get_value (0, walk_state),
			  obj_desc, (u8) ACPI_TYPE_MUTEX);
	if (ACPI_FAILURE (status)) {
		acpi_os_delete_semaphore (obj_desc->mutex.semaphore);
		acpi_cm_remove_reference (obj_desc);
		goto cleanup;
	}


cleanup:

	/* Always delete the operand */

	acpi_cm_remove_reference (sync_desc);

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_create_region
 *
 * PARAMETERS:  Aml_ptr             - Pointer to the region declaration AML
 *              Aml_length          - Max length of the declaration AML
 *              Operands            - List of operands for the opcode
 *              Interpreter_mode    - Load1/Load2/Execute
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new operation region object
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_exec_create_region (
	u8                      *aml_ptr,
	u32                     aml_length,
	u8                      region_space,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_NAMESPACE_NODE     *node;


	/*
	 * Space ID must be one of the predefined IDs, or in the user-defined
	 * range
	 */
	if ((region_space >= NUM_REGION_TYPES) &&
		(region_space < USER_REGION_BEGIN)) {
		REPORT_ERROR (("Invalid Address_space type %X\n", region_space));
		return (AE_AML_INVALID_SPACE_ID);
	}


	/* Get the Node from the object stack  */

	node = (ACPI_NAMESPACE_NODE *) acpi_ds_obj_stack_get_value (0, walk_state);

	/* Create the region descriptor */

	obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_REGION);
	if (!obj_desc) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	/*
	 * Allocate a method object for this region.
	 */

	obj_desc->region.extra = acpi_cm_create_internal_object (
			 INTERNAL_TYPE_EXTRA);
	if (!obj_desc->region.extra) {
		status = AE_NO_MEMORY;
		goto cleanup;
	}

	/*
	 * Remember location in AML stream of address & length
	 * operands since they need to be evaluated at run time.
	 */

	obj_desc->region.extra->extra.pcode      = aml_ptr;
	obj_desc->region.extra->extra.pcode_length = aml_length;

	/* Init the region from the operands */

	obj_desc->region.space_id     = region_space;
	obj_desc->region.address      = 0;
	obj_desc->region.length       = 0;


	/* Install the new region object in the parent Node */

	obj_desc->region.node = node;

	status = acpi_ns_attach_object (node, obj_desc,
			  (u8) ACPI_TYPE_REGION);

	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}

	/*
	 * If we have a valid region, initialize it
	 * Namespace is NOT locked at this point.
	 */

	status = acpi_ev_initialize_region (obj_desc, FALSE);

	if (ACPI_FAILURE (status)) {
		/*
		 *  If AE_NOT_EXIST is returned, it is not fatal
		 *  because many regions get created before a handler
		 *  is installed for said region.
		 */
		if (AE_NOT_EXIST == status) {
			status = AE_OK;
		}
	}

cleanup:

	if (ACPI_FAILURE (status)) {
		/* Delete region object and method subobject */

		if (obj_desc) {
			/* Remove deletes both objects! */

			acpi_cm_remove_reference (obj_desc);
			obj_desc = NULL;
		}
	}

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_create_processor
 *
 * PARAMETERS:  Op              - Op containing the Processor definition and
 *                                args
 *              Processor_nTE   - Node for the containing Node
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new processor object and populate the fields
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_exec_create_processor (
	ACPI_PARSE_OBJECT       *op,
	ACPI_HANDLE             processor_nTE)
{
	ACPI_STATUS             status;
	ACPI_PARSE_OBJECT       *arg;
	ACPI_OPERAND_OBJECT     *obj_desc;


	obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_PROCESSOR);
	if (!obj_desc) {
		status = AE_NO_MEMORY;
		return (status);
	}

	/* Install the new processor object in the parent Node */

	status = acpi_ns_attach_object (processor_nTE, obj_desc,
			   (u8) ACPI_TYPE_PROCESSOR);
	if (ACPI_FAILURE (status)) {
		return(status);
	}

	arg = op->value.arg;

	/* check existence */

	if (!arg) {
		status = AE_AML_NO_OPERAND;
		return (status);
	}

	/* First arg is the Processor ID */

	obj_desc->processor.proc_id = (u8) arg->value.integer;

	/* Move to next arg and check existence */

	arg = arg->next;
	if (!arg) {
		status = AE_AML_NO_OPERAND;
		return (status);
	}

	/* Second arg is the PBlock Address */

	obj_desc->processor.address = (ACPI_IO_ADDRESS) arg->value.integer;

	/* Move to next arg and check existence */

	arg = arg->next;
	if (!arg) {
		status = AE_AML_NO_OPERAND;
		return (status);
	}

	/* Third arg is the PBlock Length */

	obj_desc->processor.length = (u8) arg->value.integer;

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_create_power_resource
 *
 * PARAMETERS:  Op              - Op containing the Power_resource definition
 *                                and args
 *              Power_res_nTE   - Node for the containing Node
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new Power_resource object and populate the fields
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_exec_create_power_resource (
	ACPI_PARSE_OBJECT       *op,
	ACPI_HANDLE             power_res_nTE)
{
	ACPI_STATUS             status;
	ACPI_PARSE_OBJECT       *arg;
	ACPI_OPERAND_OBJECT     *obj_desc;


	obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_POWER);
	if (!obj_desc) {
		status = AE_NO_MEMORY;
		return (status);
	}

	/* Install the new power resource object in the parent Node */

	status = acpi_ns_attach_object (power_res_nTE, obj_desc,
			  (u8) ACPI_TYPE_POWER);
	if (ACPI_FAILURE (status)) {
		return(status);
	}

	arg = op->value.arg;

	/* check existence */

	if (!arg) {
		status = AE_AML_NO_OPERAND;
		return (status);
	}

	/* First arg is the System_level */

	obj_desc->power_resource.system_level = (u8) arg->value.integer;

	/* Move to next arg and check existence */

	arg = arg->next;
	if (!arg) {
		status = AE_AML_NO_OPERAND;
		return (status);
	}

	/* Second arg is the PBlock Address */

	obj_desc->power_resource.resource_order = (u16) arg->value.integer;

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_create_method
 *
 * PARAMETERS:  Aml_ptr         - First byte of the method's AML
 *              Aml_length      - AML byte count for this method
 *              Method_flags    - AML method flag byte
 *              Method          - Method Node
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a new method object
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_exec_create_method (
	u8                      *aml_ptr,
	u32                     aml_length,
	u32                     method_flags,
	ACPI_HANDLE             method)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status;


	/* Create a new method object */

	obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_METHOD);
	if (!obj_desc) {
	   return (AE_NO_MEMORY);
	}

	/* Get the method's AML pointer/length from the Op */

	obj_desc->method.pcode      = aml_ptr;
	obj_desc->method.pcode_length = aml_length;

	/*
	 * First argument is the Method Flags (contains parameter count for the
	 * method)
	 */

	obj_desc->method.method_flags = (u8) method_flags;
	obj_desc->method.param_count = (u8) (method_flags &
			  METHOD_FLAGS_ARG_COUNT);

	/*
	 * Get the concurrency count.  If required, a semaphore will be
	 * created for this method when it is parsed.
	 */
	if (method_flags & METHOD_FLAGS_SERIALIZED) {
		/*
		 * ACPI 1.0: Concurrency = 1
		 * ACPI 2.0: Concurrency = (Sync_level (in method declaration) + 1)
		 */
		obj_desc->method.concurrency = (u8)
				  (((method_flags & METHOD_FLAGS_SYNCH_LEVEL) >> 4) + 1);
	}

	else {
		obj_desc->method.concurrency = INFINITE_CONCURRENCY;
	}

	/* Attach the new object to the method Node */

	status = acpi_ns_attach_object (method, obj_desc, (u8) ACPI_TYPE_METHOD);
	if (ACPI_FAILURE (status)) {
		acpi_cm_delete_object_desc (obj_desc);
	}

	return (status);
}


