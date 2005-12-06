/******************************************************************************
 *
 * Module Name: dsobject - Dispatcher object management routines
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

#define _COMPONENT          ACPI_DISPATCHER
	 MODULE_NAME         ("dsobject")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_init_one_object
 *
 * PARAMETERS:  Obj_handle      - Node
 *              Level           - Current nesting level
 *              Context         - Points to a init info struct
 *              Return_value    - Not used
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Callback from Acpi_walk_namespace. Invoked for every object
 *              within the  namespace.
 *
 *              Currently, the only objects that require initialization are:
 *              1) Methods
 *              2) Op Regions
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_init_one_object (
	ACPI_HANDLE             obj_handle,
	u32                     level,
	void                    *context,
	void                    **return_value)
{
	OBJECT_TYPE_INTERNAL    type;
	ACPI_STATUS             status;
	ACPI_INIT_WALK_INFO     *info = (ACPI_INIT_WALK_INFO *) context;
	u8                      table_revision;


	info->object_count++;
	table_revision = info->table_desc->pointer->revision;

	/*
	 * We are only interested in objects owned by the table that
	 * was just loaded
	 */

	if (((ACPI_NAMESPACE_NODE *) obj_handle)->owner_id !=
			info->table_desc->table_id) {
		return (AE_OK);
	}


	/* And even then, we are only interested in a few object types */

	type = acpi_ns_get_type (obj_handle);

	switch (type) {

	case ACPI_TYPE_REGION:

		acpi_ds_initialize_region (obj_handle);

		info->op_region_count++;
		break;


	case ACPI_TYPE_METHOD:

		info->method_count++;


		/*
		 * Set the execution data width (32 or 64) based upon the
		 * revision number of the parent ACPI table.
		 */

		if (table_revision == 1) {
			((ACPI_NAMESPACE_NODE *)obj_handle)->flags |= ANOBJ_DATA_WIDTH_32;
		}

		/*
		 * Always parse methods to detect errors, we may delete
		 * the parse tree below
		 */

		status = acpi_ds_parse_method (obj_handle);

		/* TBD: [Errors] what do we do with an error? */

		if (ACPI_FAILURE (status)) {
			break;
		}

		/*
		 * Delete the parse tree.  We simple re-parse the method
		 * for every execution since there isn't much overhead
		 */
		acpi_ns_delete_namespace_subtree (obj_handle);
		break;

	default:
		break;
	}

	/*
	 * We ignore errors from above, and always return OK, since
	 * we don't want to abort the walk on a single error.
	 */
	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ds_initialize_objects
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk the entire namespace and perform any necessary
 *              initialization on the objects found therein
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ds_initialize_objects (
	ACPI_TABLE_DESC         *table_desc,
	ACPI_NAMESPACE_NODE     *start_node)
{
	ACPI_STATUS             status;
	ACPI_INIT_WALK_INFO     info;


	info.method_count   = 0;
	info.op_region_count = 0;
	info.object_count   = 0;
	info.table_desc     = table_desc;


	/* Walk entire namespace from the supplied root */

	status = acpi_walk_namespace (ACPI_TYPE_ANY, start_node,
			  ACPI_UINT32_MAX, acpi_ds_init_one_object,
			  &info, NULL);

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_init_object_from_op
 *
 * PARAMETERS:  Op              - Parser op used to init the internal object
 *              Opcode          - AML opcode associated with the object
 *              Obj_desc        - Namespace object to be initialized
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize a namespace object from a parser Op and its
 *              associated arguments.  The namespace object is a more compact
 *              representation of the Op and its arguments.
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_init_object_from_op (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	u16                     opcode,
	ACPI_OPERAND_OBJECT     **obj_desc)
{
	ACPI_STATUS             status;
	ACPI_PARSE_OBJECT       *arg;
	ACPI_PARSE2_OBJECT      *byte_list;
	ACPI_OPERAND_OBJECT     *arg_desc;
	ACPI_OPCODE_INFO        *op_info;


	op_info = acpi_ps_get_opcode_info (opcode);
	if (ACPI_GET_OP_TYPE (op_info) != ACPI_OP_TYPE_OPCODE) {
		/* Unknown opcode */

		return (AE_TYPE);
	}


	/* Get and prepare the first argument */

	switch ((*obj_desc)->common.type) {
	case ACPI_TYPE_BUFFER:

		/* First arg is a number */

		acpi_ds_create_operand (walk_state, op->value.arg, 0);
		arg_desc = walk_state->operands [walk_state->num_operands - 1];
		acpi_ds_obj_stack_pop (1, walk_state);

		/* Resolve the object (could be an arg or local) */

		status = acpi_aml_resolve_to_value (&arg_desc, walk_state);
		if (ACPI_FAILURE (status)) {
			acpi_cm_remove_reference (arg_desc);
			return (status);
		}

		/* We are expecting a number */

		if (arg_desc->common.type != ACPI_TYPE_INTEGER) {
			acpi_cm_remove_reference (arg_desc);
			return (AE_TYPE);
		}

		/* Get the value, delete the internal object */

		(*obj_desc)->buffer.length = (u32) arg_desc->integer.value;
		acpi_cm_remove_reference (arg_desc);

		/* Allocate the buffer */

		if ((*obj_desc)->buffer.length == 0) {
			(*obj_desc)->buffer.pointer = NULL;
			REPORT_WARNING (("Buffer created with zero length in AML\n"));
			break;
		}

		else {
			(*obj_desc)->buffer.pointer =
					  acpi_cm_callocate ((*obj_desc)->buffer.length);

			if (!(*obj_desc)->buffer.pointer) {
				return (AE_NO_MEMORY);
			}
		}

		/*
		 * Second arg is the buffer data (optional)
		 * Byte_list can be either individual bytes or a
		 * string initializer!
		 */

		/* skip first arg */
		arg = op->value.arg;
		byte_list = (ACPI_PARSE2_OBJECT *) arg->next;
		if (byte_list) {
			if (byte_list->opcode != AML_BYTELIST_OP) {
				return (AE_TYPE);
			}

			MEMCPY ((*obj_desc)->buffer.pointer, byte_list->data,
					(*obj_desc)->buffer.length);
		}

		break;


	case ACPI_TYPE_PACKAGE:

		/*
		 * When called, an internal package object has already
		 *  been built and is pointed to by *Obj_desc.
		 *  Acpi_ds_build_internal_object build another internal
		 *  package object, so remove reference to the original
		 *  so that it is deleted.  Error checking is done
		 *  within the remove reference function.
		 */
		acpi_cm_remove_reference(*obj_desc);

		status = acpi_ds_build_internal_object (walk_state, op, obj_desc);
		break;

	case ACPI_TYPE_INTEGER:
		(*obj_desc)->integer.value = op->value.integer;
		break;


	case ACPI_TYPE_STRING:
		(*obj_desc)->string.pointer = op->value.string;
		(*obj_desc)->string.length = STRLEN (op->value.string);
		break;


	case ACPI_TYPE_METHOD:
		break;


	case INTERNAL_TYPE_REFERENCE:

		switch (ACPI_GET_OP_CLASS (op_info)) {
		case OPTYPE_LOCAL_VARIABLE:

			/* Split the opcode into a base opcode + offset */

			(*obj_desc)->reference.opcode = AML_LOCAL_OP;
			(*obj_desc)->reference.offset = opcode - AML_LOCAL_OP;
			break;

		case OPTYPE_METHOD_ARGUMENT:

			/* Split the opcode into a base opcode + offset */

			(*obj_desc)->reference.opcode = AML_ARG_OP;
			(*obj_desc)->reference.offset = opcode - AML_ARG_OP;
			break;

		default: /* Constants, Literals, etc.. */

			if (op->opcode == AML_NAMEPATH_OP) {
				/* Node was saved in Op */

				(*obj_desc)->reference.node = op->node;
			}

			(*obj_desc)->reference.opcode = opcode;
			break;
		}

		break;


	default:

		break;
	}

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_build_internal_simple_obj
 *
 * PARAMETERS:  Op              - Parser object to be translated
 *              Obj_desc_ptr    - Where the ACPI internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate a parser Op object to the equivalent namespace object
 *              Simple objects are any objects other than a package object!
 *
 ****************************************************************************/

static ACPI_STATUS
acpi_ds_build_internal_simple_obj (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	ACPI_OPERAND_OBJECT     **obj_desc_ptr)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	OBJECT_TYPE_INTERNAL    type;
	ACPI_STATUS             status;
	u32                     length;
	char                    *name;


	if (op->opcode == AML_NAMEPATH_OP) {
		/*
		 * This is an object reference.  If The name was
		 * previously looked up in the NS, it is stored in this op.
		 * Otherwise, go ahead and look it up now
		 */

		if (!op->node) {
			status = acpi_ns_lookup (walk_state->scope_info,
					  op->value.string, ACPI_TYPE_ANY,
					  IMODE_EXECUTE,
					  NS_SEARCH_PARENT | NS_DONT_OPEN_SCOPE,
					  NULL,
					  (ACPI_NAMESPACE_NODE **)&(op->node));

			if (ACPI_FAILURE (status)) {
				if (status == AE_NOT_FOUND) {
					name = NULL;
					acpi_ns_externalize_name (ACPI_UINT32_MAX, op->value.string, &length, &name);

					if (name) {
						REPORT_WARNING (("Reference %s at AML %X not found\n",
								 name, op->aml_offset));
						acpi_cm_free (name);
					}
					else {
						REPORT_WARNING (("Reference %s at AML %X not found\n",
								   op->value.string, op->aml_offset));
					}
					*obj_desc_ptr = NULL;
				}

				else {
					return (status);
				}
			}
		}

		/*
		 * The reference will be a Reference
		 * TBD: [Restructure] unless we really need a separate
		 *  type of INTERNAL_TYPE_REFERENCE change
		 *  Acpi_ds_map_opcode_to_data_type to handle this case
		 */
		type = INTERNAL_TYPE_REFERENCE;
	}
	else {
		type = acpi_ds_map_opcode_to_data_type (op->opcode, NULL);
	}


	/* Create and init the internal ACPI object */

	obj_desc = acpi_cm_create_internal_object (type);
	if (!obj_desc) {
		return (AE_NO_MEMORY);
	}

	status = acpi_ds_init_object_from_op (walk_state, op,
			 op->opcode, &obj_desc);

	if (ACPI_FAILURE (status)) {
		acpi_cm_remove_reference (obj_desc);
		return (status);
	}

	*obj_desc_ptr = obj_desc;

	return (AE_OK);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_build_internal_package_obj
 *
 * PARAMETERS:  Op              - Parser object to be translated
 *              Obj_desc_ptr    - Where the ACPI internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate a parser Op package object to the equivalent
 *              namespace object
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_build_internal_package_obj (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	ACPI_OPERAND_OBJECT     **obj_desc_ptr)
{
	ACPI_PARSE_OBJECT       *arg;
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_STATUS             status = AE_OK;


	obj_desc = acpi_cm_create_internal_object (ACPI_TYPE_PACKAGE);
	if (!obj_desc) {
		return (AE_NO_MEMORY);
	}

	/* The first argument must be the package length */

	arg = op->value.arg;
	obj_desc->package.count = arg->value.integer;

	/*
	 * Allocate the array of pointers (ptrs to the
	 * individual objects) Add an extra pointer slot so
	 * that the list is always null terminated.
	 */

	obj_desc->package.elements =
			 acpi_cm_callocate ((obj_desc->package.count + 1) *
			 sizeof (void *));

	if (!obj_desc->package.elements) {
		/* Package vector allocation failure   */

		REPORT_ERROR (("Ds_build_internal_package_obj: Package vector allocation failure\n"));

		acpi_cm_delete_object_desc (obj_desc);
		return (AE_NO_MEMORY);
	}

	obj_desc->package.next_element = obj_desc->package.elements;

	/*
	 * Now init the elements of the package
	 */

	arg = arg->next;
	while (arg) {
		if (arg->opcode == AML_PACKAGE_OP) {
			status = acpi_ds_build_internal_package_obj (walk_state, arg,
					  obj_desc->package.next_element);
		}

		else {
			status = acpi_ds_build_internal_simple_obj (walk_state, arg,
					  obj_desc->package.next_element);
		}

		obj_desc->package.next_element++;
		arg = arg->next;
	}

	*obj_desc_ptr = obj_desc;
	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_build_internal_object
 *
 * PARAMETERS:  Op              - Parser object to be translated
 *              Obj_desc_ptr    - Where the ACPI internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Translate a parser Op object to the equivalent namespace
 *              object
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_build_internal_object (
	ACPI_WALK_STATE         *walk_state,
	ACPI_PARSE_OBJECT       *op,
	ACPI_OPERAND_OBJECT     **obj_desc_ptr)
{
	ACPI_STATUS             status;


	if (op->opcode == AML_PACKAGE_OP) {
		status = acpi_ds_build_internal_package_obj (walk_state, op,
				  obj_desc_ptr);
	}

	else {
		status = acpi_ds_build_internal_simple_obj (walk_state, op,
				  obj_desc_ptr);
	}

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_ds_create_node
 *
 * PARAMETERS:  Op              - Parser object to be translated
 *              Obj_desc_ptr    - Where the ACPI internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION:
 *
 ****************************************************************************/

ACPI_STATUS
acpi_ds_create_node (
	ACPI_WALK_STATE         *walk_state,
	ACPI_NAMESPACE_NODE     *node,
	ACPI_PARSE_OBJECT       *op)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *obj_desc;


	if (!op->value.arg) {
		/* No arguments, there is nothing to do */

		return (AE_OK);
	}


	/* Build an internal object for the argument(s) */

	status = acpi_ds_build_internal_object (walk_state,
			 op->value.arg, &obj_desc);
	if (ACPI_FAILURE (status)) {
		return (status);
	}


	/* Re-type the object according to it's argument */

	node->type = obj_desc->common.type;

	/* Init obj */

	status = acpi_ns_attach_object ((ACPI_HANDLE) node, obj_desc,
			   (u8) node->type);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}

	return (status);


cleanup:

	acpi_cm_remove_reference (obj_desc);

	return (status);
}


