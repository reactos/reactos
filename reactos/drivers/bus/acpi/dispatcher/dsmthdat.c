/*******************************************************************************
 *
 * Module Name: dsmthdat - control method arguments and local variables
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
#include "acparser.h"
#include "acdispat.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_DISPATCHER
	 MODULE_NAME         ("dsmthdat")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_init
 *
 * PARAMETERS:  Walk_state          - Current walk state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the data structures that hold the method's arguments
 *              and locals.  The data struct is an array of NTEs for each.
 *              This allows Ref_of and De_ref_of to work properly for these
 *              special data types.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_method_data_init (
	ACPI_WALK_STATE         *walk_state)
{
	u32                     i;


	/*
	 * Walk_state fields are initialized to zero by the
	 * Acpi_cm_callocate().
	 *
	 * An Node is assigned to each argument and local so
	 * that Ref_of() can return a pointer to the Node.
	 */

	/* Init the method arguments */

	for (i = 0; i < MTH_NUM_ARGS; i++) {
		MOVE_UNALIGNED32_TO_32 (&walk_state->arguments[i].name,
				 NAMEOF_ARG_NTE);
		walk_state->arguments[i].name      |= (i << 24);
		walk_state->arguments[i].data_type  = ACPI_DESC_TYPE_NAMED;
		walk_state->arguments[i].type       = ACPI_TYPE_ANY;
		walk_state->arguments[i].flags      = ANOBJ_END_OF_PEER_LIST | ANOBJ_METHOD_ARG;
	}

	/* Init the method locals */

	for (i = 0; i < MTH_NUM_LOCALS; i++) {
		MOVE_UNALIGNED32_TO_32 (&walk_state->local_variables[i].name,
				 NAMEOF_LOCAL_NTE);

		walk_state->local_variables[i].name  |= (i << 24);
		walk_state->local_variables[i].data_type = ACPI_DESC_TYPE_NAMED;
		walk_state->local_variables[i].type   = ACPI_TYPE_ANY;
		walk_state->local_variables[i].flags  = ANOBJ_END_OF_PEER_LIST | ANOBJ_METHOD_LOCAL;
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_delete_all
 *
 * PARAMETERS:  Walk_state          - Current walk state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete method locals and arguments.  Arguments are only
 *              deleted if this method was called from another method.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_method_data_delete_all (
	ACPI_WALK_STATE         *walk_state)
{
	u32                     index;
	ACPI_OPERAND_OBJECT     *object;


	/* Delete the locals */

	for (index = 0; index < MTH_NUM_LOCALS; index++) {
		object = walk_state->local_variables[index].object;
		if (object) {
			/* Remove first */

			walk_state->local_variables[index].object = NULL;

			/* Was given a ref when stored */

			acpi_cm_remove_reference (object);
	   }
	}


	/* Delete the arguments */

	for (index = 0; index < MTH_NUM_ARGS; index++) {
		object = walk_state->arguments[index].object;
		if (object) {
			/* Remove first */

			walk_state->arguments[index].object = NULL;

			 /* Was given a ref when stored */

			acpi_cm_remove_reference (object);
		}
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_init_args
 *
 * PARAMETERS:  *Params         - Pointer to a parameter list for the method
 *              Max_param_count - The arg count for this method
 *              Walk_state      - Current walk state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize arguments for a method
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_method_data_init_args (
	ACPI_OPERAND_OBJECT     **params,
	u32                     max_param_count,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	u32                     mindex;
	u32                     pindex;


	if (!params) {
		return (AE_OK);
	}

	/* Copy passed parameters into the new method stack frame  */

	for (pindex = mindex = 0;
		(mindex < MTH_NUM_ARGS) && (pindex < max_param_count);
		mindex++) {
		if (params[pindex]) {
			/*
			 * A valid parameter.
			 * Set the current method argument to the
			 * Params[Pindex++] argument object descriptor
			 */
			status = acpi_ds_store_object_to_local (AML_ARG_OP, mindex,
					 params[pindex], walk_state);
			if (ACPI_FAILURE (status)) {
				break;
			}

			pindex++;
		}

		else {
			break;
		}
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_get_entry
 *
 * PARAMETERS:  Opcode              - Either AML_LOCAL_OP or AML_ARG_OP
 *              Index               - Which local_var or argument to get
 *              Entry               - Pointer to where a pointer to the stack
 *                                    entry is returned.
 *              Walk_state          - Current walk state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get the address of the object entry given by Opcode:Index
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_method_data_get_entry (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     ***entry)
{


	/*
	 * Get the requested object.
	 * The stack "Opcode" is either a Local_variable or an Argument
	 */

	switch (opcode) {

	case AML_LOCAL_OP:

		if (index > MTH_MAX_LOCAL) {
			return (AE_BAD_PARAMETER);
		}

		*entry = (ACPI_OPERAND_OBJECT  **)
				 &walk_state->local_variables[index].object;
		break;


	case AML_ARG_OP:

		if (index > MTH_MAX_ARG) {
			return (AE_BAD_PARAMETER);
		}

		*entry = (ACPI_OPERAND_OBJECT  **)
				 &walk_state->arguments[index].object;
		break;


	default:
		return (AE_BAD_PARAMETER);
	}


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_set_entry
 *
 * PARAMETERS:  Opcode              - Either AML_LOCAL_OP or AML_ARG_OP
 *              Index               - Which local_var or argument to get
 *              Object              - Object to be inserted into the stack entry
 *              Walk_state          - Current walk state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Insert an object onto the method stack at entry Opcode:Index.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_method_data_set_entry (
	u16                     opcode,
	u32                     index,
	ACPI_OPERAND_OBJECT     *object,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     **entry;


	/* Get a pointer to the stack entry to set */

	status = acpi_ds_method_data_get_entry (opcode, index, walk_state, &entry);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Increment ref count so object can't be deleted while installed */

	acpi_cm_add_reference (object);

	/* Install the object into the stack entry */

	*entry = object;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_get_type
 *
 * PARAMETERS:  Opcode              - Either AML_LOCAL_OP or AML_ARG_OP
 *              Index               - Which local_var or argument whose type
 *                                      to get
 *              Walk_state          - Current walk state object
 *
 * RETURN:      Data type of selected Arg or Local
 *              Used only in Exec_monadic2()/Type_op.
 *
 ******************************************************************************/

OBJECT_TYPE_INTERNAL
acpi_ds_method_data_get_type (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     **entry;
	ACPI_OPERAND_OBJECT     *object;


	/* Get a pointer to the requested stack entry */

	status = acpi_ds_method_data_get_entry (opcode, index, walk_state, &entry);
	if (ACPI_FAILURE (status)) {
		return ((ACPI_TYPE_NOT_FOUND));
	}

	/* Get the object from the method stack */

	object = *entry;

	/* Get the object type */

	if (!object) {
		/* Any == 0 => "uninitialized" -- see spec 15.2.3.5.2.28 */
		return (ACPI_TYPE_ANY);
	}

	return (object->common.type);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_get_node
 *
 * PARAMETERS:  Opcode              - Either AML_LOCAL_OP or AML_ARG_OP
 *              Index               - Which local_var or argument whose type
 *                                      to get
 *              Walk_state          - Current walk state object
 *
 * RETURN:      Get the Node associated with a local or arg.
 *
 ******************************************************************************/

ACPI_NAMESPACE_NODE *
acpi_ds_method_data_get_node (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_NAMESPACE_NODE     *node = NULL;


	switch (opcode) {

	case AML_LOCAL_OP:

		if (index > MTH_MAX_LOCAL) {
			return (node);
		}

		node =  &walk_state->local_variables[index];
		break;


	case AML_ARG_OP:

		if (index > MTH_MAX_ARG) {
			return (node);
		}

		node = &walk_state->arguments[index];
		break;


	default:
		break;
	}


	return (node);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_get_value
 *
 * PARAMETERS:  Opcode              - Either AML_LOCAL_OP or AML_ARG_OP
 *              Index               - Which local_var or argument to get
 *              Walk_state          - Current walk state object
 *              *Dest_desc          - Ptr to Descriptor into which selected Arg
 *                                    or Local value should be copied
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve value of selected Arg or Local from the method frame
 *              at the current top of the method stack.
 *              Used only in Acpi_aml_resolve_to_value().
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_method_data_get_value (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state,
	ACPI_OPERAND_OBJECT     **dest_desc)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     **entry;
	ACPI_OPERAND_OBJECT     *object;


	/* Validate the object descriptor */

	if (!dest_desc) {
		return (AE_BAD_PARAMETER);
	}


	/* Get a pointer to the requested method stack entry */

	status = acpi_ds_method_data_get_entry (opcode, index, walk_state, &entry);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Get the object from the method stack */

	object = *entry;


	/* Examine the returned object, it must be valid. */

	if (!object) {
		/*
		 * Index points to uninitialized object stack value.
		 * This means that either 1) The expected argument was
		 * not passed to the method, or 2) A local variable
		 * was referenced by the method (via the ASL)
		 * before it was initialized.  Either case is an error.
		 */

		switch (opcode) {
		case AML_ARG_OP:

			return (AE_AML_UNINITIALIZED_ARG);
			break;

		case AML_LOCAL_OP:

			return (AE_AML_UNINITIALIZED_LOCAL);
			break;
		}
	}


	/*
	 * Index points to initialized and valid object stack value.
	 * Return an additional reference to the object
	 */

	*dest_desc = object;
	acpi_cm_add_reference (object);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_method_data_delete_value
 *
 * PARAMETERS:  Opcode              - Either AML_LOCAL_OP or AML_ARG_OP
 *              Index               - Which local_var or argument to delete
 *              Walk_state          - Current walk state object
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete the entry at Opcode:Index on the method stack.  Inserts
 *              a null into the stack slot after the object is deleted.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_method_data_delete_value (
	u16                     opcode,
	u32                     index,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     **entry;
	ACPI_OPERAND_OBJECT     *object;


	/* Get a pointer to the requested entry */

	status = acpi_ds_method_data_get_entry (opcode, index, walk_state, &entry);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Get the current entry in this slot k */

	object = *entry;

	/*
	 * Undefine the Arg or Local by setting its descriptor
	 * pointer to NULL. Locals/Args can contain both
	 * ACPI_OPERAND_OBJECTS and ACPI_NAMESPACE_NODEs
	 */
	*entry = NULL;


	if ((object) &&
		(VALID_DESCRIPTOR_TYPE (object, ACPI_DESC_TYPE_INTERNAL))) {
		/*
		 * There is a valid object in this slot
		 * Decrement the reference count by one to balance the
		 * increment when the object was stored in the slot.
		 */
		acpi_cm_remove_reference (object);
	}


	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_store_object_to_local
 *
 * PARAMETERS:  Opcode              - Either AML_LOCAL_OP or AML_ARG_OP
 *              Index               - Which local_var or argument to set
 *              Src_desc            - Value to be stored
 *              Walk_state          - Current walk state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store a value in an Arg or Local.  The Src_desc is installed
 *              as the new value for the Arg or Local and the reference count
 *              for Src_desc is incremented.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_store_object_to_local (
	u16                     opcode,
	u32                     index,
	ACPI_OPERAND_OBJECT     *src_desc,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     **entry;


	/* Parameter validation */

	if (!src_desc) {
		return (AE_BAD_PARAMETER);
	}


	/* Get a pointer to the requested method stack entry */

	status = acpi_ds_method_data_get_entry (opcode, index, walk_state, &entry);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}

	if (*entry == src_desc) {
		goto cleanup;
	}


	/*
	 * If there is an object already in this slot, we either
	 * have to delete it, or if this is an argument and there
	 * is an object reference stored there, we have to do
	 * an indirect store!
	 */

	if (*entry) {
		/*
		 * Check for an indirect store if an argument
		 * contains an object reference (stored as an Node).
		 * We don't allow this automatic dereferencing for
		 * locals, since a store to a local should overwrite
		 * anything there, including an object reference.
		 *
		 * If both Arg0 and Local0 contain Ref_of (Local4):
		 *
		 * Store (1, Arg0)             - Causes indirect store to local4
		 * Store (1, Local0)           - Stores 1 in local0, overwriting
		 *                                  the reference to local4
		 * Store (1, De_refof (Local0)) - Causes indirect store to local4
		 *
		 * Weird, but true.
		 */

		if ((opcode == AML_ARG_OP) &&
			(VALID_DESCRIPTOR_TYPE (*entry, ACPI_DESC_TYPE_NAMED))) {
			/* Detach an existing object from the Node */

			acpi_ns_detach_object ((ACPI_NAMESPACE_NODE *) *entry);

			/*
			 * Store this object into the Node
			 * (do the indirect store)
			 */
			status = acpi_ns_attach_object ((ACPI_NAMESPACE_NODE *) *entry, src_desc,
					   src_desc->common.type);
			return (status);
		}


#ifdef ACPI_ENABLE_IMPLICIT_CONVERSION
		/*
		 * Perform "Implicit conversion" of the new object to the type of the
		 * existing object
		 */
		status = acpi_aml_convert_to_target_type ((*entry)->common.type, &src_desc, walk_state);
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}
#endif

		/*
		 * Delete the existing object
		 * before storing the new one
		 */
		acpi_ds_method_data_delete_value (opcode, index, walk_state);
	}


	/*
	 * Install the Obj_stack descriptor (*Src_desc) into
	 * the descriptor for the Arg or Local.
	 * Install the new object in the stack entry
	 * (increments the object reference count by one)
	 */
	status = acpi_ds_method_data_set_entry (opcode, index, src_desc, walk_state);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}

	/* Normal exit */

	return (AE_OK);


	/* Error exit */

cleanup:

	return (status);
}

