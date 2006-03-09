
/******************************************************************************
 *
 * Module Name: amstore - AML Interpreter object store support
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
#include "actables.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amstore")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_store
 *
 * PARAMETERS:  *Val_desc           - Value to be stored
 *              *Dest_desc          - Where to store it 0 Must be (ACPI_HANDLE)
 *                                    or an ACPI_OPERAND_OBJECT  of type
 *                                    Reference; if the latter the descriptor
 *                                    will be either reused or deleted.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store the value described by Val_desc into the location
 *              described by Dest_desc. Called by various interpreter
 *              functions to store the result of an operation into
 *              the destination operand.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_exec_store (
	ACPI_OPERAND_OBJECT     *val_desc,
	ACPI_OPERAND_OBJECT     *dest_desc,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OPERAND_OBJECT     *ref_desc = dest_desc;


	/* Validate parameters */

	if (!val_desc || !dest_desc) {
		return (AE_AML_NO_OPERAND);
	}

	/* Dest_desc can be either a namespace node or an ACPI object */

	if (VALID_DESCRIPTOR_TYPE (dest_desc, ACPI_DESC_TYPE_NAMED)) {
		/*
		 * Dest is a namespace node,
		 * Storing an object into a Name "container"
		 */
		status = acpi_aml_store_object_to_node (val_desc,
				 (ACPI_NAMESPACE_NODE *) dest_desc, walk_state);

		/* All done, that's it */

		return (status);
	}


	/* Destination object must be an object of type Reference */

	if (dest_desc->common.type != INTERNAL_TYPE_REFERENCE) {
		/* Destination is not an Reference */

		return (AE_AML_OPERAND_TYPE);
	}


	/*
	 * Examine the Reference opcode.  These cases are handled:
	 *
	 * 1) Store to Name (Change the object associated with a name)
	 * 2) Store to an indexed area of a Buffer or Package
	 * 3) Store to a Method Local or Arg
	 * 4) Store to the debug object
	 * 5) Store to a constant -- a noop
	 */

	switch (ref_desc->reference.opcode) {

	case AML_NAME_OP:

		/* Storing an object into a Name "container" */

		status = acpi_aml_store_object_to_node (val_desc, ref_desc->reference.object,
				  walk_state);
		break;


	case AML_INDEX_OP:

		/* Storing to an Index (pointer into a packager or buffer) */

		status = acpi_aml_store_object_to_index (val_desc, ref_desc, walk_state);
		break;


	case AML_LOCAL_OP:
	case AML_ARG_OP:

		/* Store to a method local/arg  */

		status = acpi_ds_store_object_to_local (ref_desc->reference.opcode,
				  ref_desc->reference.offset, val_desc, walk_state);
		break;


	case AML_DEBUG_OP:

		/*
		 * Storing to the Debug object causes the value stored to be
		 * displayed and otherwise has no effect -- see ACPI Specification
		 *
		 * TBD: print known object types "prettier".
		 */

		break;


	case AML_ZERO_OP:
	case AML_ONE_OP:
	case AML_ONES_OP:

		/*
		 * Storing to a constant is a no-op -- see ACPI Specification
		 * Delete the reference descriptor, however
		 */
		break;


	default:

		/* TBD: [Restructure] use object dump routine !! */

		status = AE_AML_INTERNAL;
		break;

	}   /* switch (Ref_desc->Reference.Opcode) */


	/* Always delete the reference descriptor object */

	if (ref_desc) {
		acpi_cm_remove_reference (ref_desc);
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_store_object_to_index
 *
 * PARAMETERS:  *Val_desc           - Value to be stored
 *              *Node               - Named object to receive the value
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store the object to the named object.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_store_object_to_index (
	ACPI_OPERAND_OBJECT     *val_desc,
	ACPI_OPERAND_OBJECT     *dest_desc,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OPERAND_OBJECT     *obj_desc;
	u32                     length;
	u32                     i;
	u8                      value = 0;


	/*
	 * Destination must be a reference pointer, and
	 * must point to either a buffer or a package
	 */

	switch (dest_desc->reference.target_type) {
	case ACPI_TYPE_PACKAGE:
		/*
		 * Storing to a package element is not simple.  The source must be
		 * evaluated and converted to the type of the destination and then the
		 * source is copied into the destination - we can't just point to the
		 * source object.
		 */
		if (dest_desc->reference.target_type == ACPI_TYPE_PACKAGE) {
			/*
			 * The object at *(Dest_desc->Reference.Where) is the
			 *  element within the package that is to be modified.
			 */
			obj_desc = *(dest_desc->reference.where);
			if (obj_desc) {
				/*
				 * If the Destination element is a package, we will delete
				 *  that object and construct a new one.
				 *
				 * TBD: [Investigate] Should both the src and dest be required
				 *      to be packages?
				 *       && (Val_desc->Common.Type == ACPI_TYPE_PACKAGE)
				 */
				if (obj_desc->common.type == ACPI_TYPE_PACKAGE) {
					/*
					 * Take away the reference for being part of a package and
					 * delete
					 */
					acpi_cm_remove_reference (obj_desc);
					acpi_cm_remove_reference (obj_desc);

					obj_desc = NULL;
				}
			}

			if (!obj_desc) {
				/*
				 * If the Obj_desc is NULL, it means that an uninitialized package
				 * element has been used as a destination (this is OK), therefore,
				 * we must create the destination element to match the type of the
				 * source element NOTE: Val_desc can be of any type.
				 */
				obj_desc = acpi_cm_create_internal_object (val_desc->common.type);
				if (!obj_desc) {
					return (AE_NO_MEMORY);
				}

				/*
				 * If the source is a package, copy the source to the new dest
				 */
				if (ACPI_TYPE_PACKAGE == obj_desc->common.type) {
					status = acpi_cm_copy_ipackage_to_ipackage (val_desc, obj_desc, walk_state);
					if (ACPI_FAILURE (status)) {
						acpi_cm_remove_reference (obj_desc);
						return (status);
					}
				}

				/*
				 * Install the new descriptor into the package and add a
				 * reference to the newly created descriptor for now being
				 * part of the parent package
				 */

				*(dest_desc->reference.where) = obj_desc;
				acpi_cm_add_reference (obj_desc);
			}

			if (ACPI_TYPE_PACKAGE != obj_desc->common.type) {
				/*
				 * The destination element is not a package, so we need to
				 * convert the contents of the source (Val_desc) and copy into
				 * the destination (Obj_desc)
				 */
				status = acpi_aml_store_object_to_object (val_desc, obj_desc,
						  walk_state);
				if (ACPI_FAILURE (status)) {
					/*
					 * An error occurrered when copying the internal object
					 * so delete the reference.
					 */
					return (AE_AML_OPERAND_TYPE);
				}
			}
		}
		break;


	case ACPI_TYPE_BUFFER_FIELD:
		/*
		 * Storing into a buffer at a location defined by an Index.
		 *
		 * Each 8-bit element of the source object is written to the
		 * 8-bit Buffer Field of the Index destination object.
		 */

		/*
		 * Set the Obj_desc to the destination object and type check.
		 */
		obj_desc = dest_desc->reference.object;
		if (obj_desc->common.type != ACPI_TYPE_BUFFER) {
			return (AE_AML_OPERAND_TYPE);
		}

		/*
		 * The assignment of the individual elements will be slightly
		 * different for each source type.
		 */

		switch (val_desc->common.type) {
		/*
		 * If the type is Integer, assign bytewise
		 * This loop to assign each of the elements is somewhat
		 * backward because of the Big Endian-ness of IA-64
		 */
		case ACPI_TYPE_INTEGER:
			length = sizeof (ACPI_INTEGER);
			for (i = length; i != 0; i--) {
				value = (u8)(val_desc->integer.value >> (MUL_8 (i - 1)));
				obj_desc->buffer.pointer[dest_desc->reference.offset] = value;
			}
			break;

		/*
		 * If the type is Buffer, the Length is in the structure.
		 * Just loop through the elements and assign each one in turn.
		 */
		case ACPI_TYPE_BUFFER:
			length = val_desc->buffer.length;
			for (i = 0; i < length; i++) {
				value = *(val_desc->buffer.pointer + i);
				obj_desc->buffer.pointer[dest_desc->reference.offset] = value;
			}
			break;

		/*
		 * If the type is String, the Length is in the structure.
		 * Just loop through the elements and assign each one in turn.
		 */
		case ACPI_TYPE_STRING:
			length = val_desc->string.length;
			for (i = 0; i < length; i++) {
				value = *(val_desc->string.pointer + i);
				obj_desc->buffer.pointer[dest_desc->reference.offset] = value;
			}
			break;

		/*
		 * If source is not a valid type so return an error.
		 */
		default:
			status = AE_AML_OPERAND_TYPE;
			break;
		}
		break;


	default:
		status = AE_AML_OPERAND_TYPE;
		break;
	}


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_store_object_to_node
 *
 * PARAMETERS:  *Source_desc           - Value to be stored
 *              *Node                  - Named object to receive the value
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store the object to the named object.
 *
 *              The Assignment of an object to a named object is handled here
 *              The val passed in will replace the current value (if any)
 *              with the input value.
 *
 *              When storing into an object the data is converted to the
 *              target object type then stored in the object.  This means
 *              that the target object type (for an initialized target) will
 *              not be changed by a store operation.
 *
 *              NOTE: the global lock is acquired early.  This will result
 *              in the global lock being held a bit longer.  Also, if the
 *              function fails during set up we may get the lock when we
 *              don't really need it.  I don't think we care.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_store_object_to_node (
	ACPI_OPERAND_OBJECT     *source_desc,
	ACPI_NAMESPACE_NODE     *node,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OPERAND_OBJECT     *target_desc;
	OBJECT_TYPE_INTERNAL    target_type = ACPI_TYPE_ANY;


	/*
	 * Assuming the parameters were already validated
	 */
	ACPI_ASSERT((node) && (source_desc));


	/*
	 * Get current type of the node, and object attached to Node
	 */
	target_type = acpi_ns_get_type (node);
	target_desc = acpi_ns_get_attached_object (node);


	/*
	 * Resolve the source object to an actual value
	 * (If it is a reference object)
	 */
	status = acpi_aml_resolve_object (&source_desc, target_type, walk_state);
	if (ACPI_FAILURE (status)) {
		return (status);
	}


	/*
	 * Do the actual store operation
	 */
	switch (target_type) {
	case INTERNAL_TYPE_DEF_FIELD:

		/* Raw data copy for target types Integer/String/Buffer */

		status = acpi_aml_copy_data_to_named_field (source_desc, node);
		break;


	case ACPI_TYPE_INTEGER:
	case ACPI_TYPE_STRING:
	case ACPI_TYPE_BUFFER:
	case INTERNAL_TYPE_BANK_FIELD:
	case INTERNAL_TYPE_INDEX_FIELD:
	case ACPI_TYPE_FIELD_UNIT:

		/*
		 * These target types are all of type Integer/String/Buffer, and
		 * therefore support implicit conversion before the store.
		 *
		 * Copy and/or convert the source object to a new target object
		 */
		status = acpi_aml_store_object (source_desc, target_type, &target_desc, walk_state);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		/*
		 * Store the new Target_desc as the new value of the Name, and set
		 * the Name's type to that of the value being stored in it.
		 * Source_desc reference count is incremented by Attach_object.
		 */
		status = acpi_ns_attach_object (node, target_desc, target_type);
		break;


	default:

		/* No conversions for all other types.  Just attach the source object */

		status = acpi_ns_attach_object (node, source_desc, source_desc->common.type);

		break;
	}


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_store_object_to_object
 *
 * PARAMETERS:  *Source_desc           - Value to be stored
 *              *Dest_desc          - Object to receive the value
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store an object to another object.
 *
 *              The Assignment of an object to another (not named) object
 *              is handled here.
 *              The val passed in will replace the current value (if any)
 *              with the input value.
 *
 *              When storing into an object the data is converted to the
 *              target object type then stored in the object.  This means
 *              that the target object type (for an initialized target) will
 *              not be changed by a store operation.
 *
 *              This module allows destination types of Number, String,
 *              and Buffer.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_store_object_to_object (
	ACPI_OPERAND_OBJECT     *source_desc,
	ACPI_OPERAND_OBJECT     *dest_desc,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;
	OBJECT_TYPE_INTERNAL    destination_type = dest_desc->common.type;


	/*
	 *  Assuming the parameters are valid!
	 */
	ACPI_ASSERT((dest_desc) && (source_desc));


	/*
	 * From this interface, we only support Integers/Strings/Buffers
	 */
	switch (destination_type) {
	case ACPI_TYPE_INTEGER:
	case ACPI_TYPE_STRING:
	case ACPI_TYPE_BUFFER:
		break;

	default:
		return (AE_NOT_IMPLEMENTED);
	}


	/*
	 * Resolve the source object to an actual value
	 * (If it is a reference object)
	 */
	status = acpi_aml_resolve_object (&source_desc, destination_type, walk_state);
	if (ACPI_FAILURE (status)) {
		return (status);
	}


	/*
	 * Copy and/or convert the source object to the destination object
	 */
	status = acpi_aml_store_object (source_desc, destination_type, &dest_desc, walk_state);


	return (status);
}

