/*******************************************************************************
 *
 * Module Name: nsobject - Utilities for objects attached to namespace
 *                         table entries
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
#include "amlcode.h"
#include "acnamesp.h"
#include "acinterp.h"
#include "actables.h"


#define _COMPONENT          ACPI_NAMESPACE
	 MODULE_NAME         ("nsobject")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_attach_object
 *
 * PARAMETERS:  Node            - Parent Node
 *              Object              - Object to be attached
 *              Type                - Type of object, or ACPI_TYPE_ANY if not
 *                                      known
 *
 * DESCRIPTION: Record the given object as the value associated with the
 *              name whose ACPI_HANDLE is passed.  If Object is NULL
 *              and Type is ACPI_TYPE_ANY, set the name as having no value.
 *
 * MUTEX:       Assumes namespace is locked
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ns_attach_object (
	ACPI_NAMESPACE_NODE     *node,
	ACPI_OPERAND_OBJECT     *object,
	OBJECT_TYPE_INTERNAL    type)
{
	ACPI_OPERAND_OBJECT     *obj_desc;
	ACPI_OPERAND_OBJECT     *previous_obj_desc;
	OBJECT_TYPE_INTERNAL    obj_type = ACPI_TYPE_ANY;
	u8                      flags;
	u16                     opcode;


	/*
	 * Parameter validation
	 */

	if (!acpi_gbl_root_node) {
		/* Name space not initialized  */

		REPORT_ERROR (("Ns_attach_object: Namespace not initialized\n"));
		return (AE_NO_NAMESPACE);
	}

	if (!node) {
		/* Invalid handle */

		REPORT_ERROR (("Ns_attach_object: Null Named_obj handle\n"));
		return (AE_BAD_PARAMETER);
	}

	if (!object && (ACPI_TYPE_ANY != type)) {
		/* Null object */

		REPORT_ERROR (("Ns_attach_object: Null object, but type not ACPI_TYPE_ANY\n"));
		return (AE_BAD_PARAMETER);
	}

	if (!VALID_DESCRIPTOR_TYPE (node, ACPI_DESC_TYPE_NAMED)) {
		/* Not a name handle */

		REPORT_ERROR (("Ns_attach_object: Invalid handle\n"));
		return (AE_BAD_PARAMETER);
	}

	/* Check if this object is already attached */

	if (node->object == object) {
		return (AE_OK);
	}


	/* Get the current flags field of the Node */

	flags = node->flags;
	flags &= ~ANOBJ_AML_ATTACHMENT;


	/* If null object, we will just install it */

	if (!object) {
		obj_desc = NULL;
		obj_type = ACPI_TYPE_ANY;
	}

	/*
	 * If the object is an Node with an attached object,
	 * we will use that (attached) object
	 */

	else if (VALID_DESCRIPTOR_TYPE (object, ACPI_DESC_TYPE_NAMED) &&
			((ACPI_NAMESPACE_NODE *) object)->object) {
		/*
		 * Value passed is a name handle and that name has a
		 * non-null value.  Use that name's value and type.
		 */

		obj_desc = ((ACPI_NAMESPACE_NODE *) object)->object;
		obj_type = ((ACPI_NAMESPACE_NODE *) object)->type;

		/*
		 * Copy appropriate flags
		 */

		if (((ACPI_NAMESPACE_NODE *) object)->flags & ANOBJ_AML_ATTACHMENT) {
			flags |= ANOBJ_AML_ATTACHMENT;
		}
	}


	/*
	 * Otherwise, we will use the parameter object, but we must type
	 * it first
	 */

	else {
		obj_desc = (ACPI_OPERAND_OBJECT *) object;


		/* If a valid type (non-ANY) was given, just use it */

		if (ACPI_TYPE_ANY != type) {
			obj_type = type;
		}


		/*
		 * Type is TYPE_Any, we must try to determinte the
		 * actual type of the object
		 */

		/*
		 * Check if value points into the AML code
		 */
		else if (acpi_tb_system_table_pointer (object)) {
			/*
			 * Object points into the AML stream.
			 * Set a flag bit in the Node to indicate this
			 */

			flags |= ANOBJ_AML_ATTACHMENT;

			/*
			 * The next byte (perhaps the next two bytes)
			 * will be the AML opcode
			 */

			MOVE_UNALIGNED16_TO_16 (&opcode, object);

			/* Check for a recognized Opcode */

			switch ((u8) opcode) {

			case AML_OP_PREFIX:

				if (opcode != AML_REVISION_OP) {
					/*
					 * Op_prefix is unrecognized unless part
					 * of Revision_op
					 */

					break;
				}

				/* Else fall through to set type as Number */


			case AML_ZERO_OP: case AML_ONES_OP: case AML_ONE_OP:
			case AML_BYTE_OP: case AML_WORD_OP: case AML_DWORD_OP:

				obj_type = ACPI_TYPE_INTEGER;
				break;


			case AML_STRING_OP:

				obj_type = ACPI_TYPE_STRING;
				break;


			case AML_BUFFER_OP:

				obj_type = ACPI_TYPE_BUFFER;
				break;


			case AML_MUTEX_OP:

				obj_type = ACPI_TYPE_MUTEX;
				break;


			case AML_PACKAGE_OP:

				obj_type = ACPI_TYPE_PACKAGE;
				break;


			default:

				return (AE_TYPE);
				break;
			}
		}

		else {
			/*
			 * Cannot figure out the type -- set to Def_any which
			 * will print as an error in the name table dump
			 */


			obj_type = INTERNAL_TYPE_DEF_ANY;
		}
	}


	/*
	 * Must increment the new value's reference count
	 * (if it is an internal object)
	 */

	acpi_cm_add_reference (obj_desc);

	/* Save the existing object (if any) for deletion later */

	previous_obj_desc = node->object;

	/* Install the object and set the type, flags */

	node->object   = obj_desc;
	node->type     = (u8) obj_type;
	node->flags    |= flags;


	/*
	 * Delete an existing attached object.
	 */

	if (previous_obj_desc) {
		/* One for the attach to the Node */

		acpi_cm_remove_reference (previous_obj_desc);

		/* Now delete */

		acpi_cm_remove_reference (previous_obj_desc);
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_detach_object
 *
 * PARAMETERS:  Node           - An object whose Value will be deleted
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Delete the Value associated with a namespace object.  If the
 *              Value is an allocated object, it is freed.  Otherwise, the
 *              field is simply cleared.
 *
 ******************************************************************************/

void
acpi_ns_detach_object (
	ACPI_NAMESPACE_NODE     *node)
{
	ACPI_OPERAND_OBJECT     *obj_desc;


	obj_desc = node->object;
	if (!obj_desc) {
		return;
	}

	/* Clear the entry in all cases */

	node->object = NULL;

	/* Found a valid value */

	/*
	 * Not every value is an object allocated via Acpi_cm_callocate,
	 * - must check
	 */

	if (!acpi_tb_system_table_pointer (obj_desc)) {
		/* Attempt to delete the object (and all subobjects) */

		acpi_cm_remove_reference (obj_desc);
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ns_get_attached_object
 *
 * PARAMETERS:  Handle              - Parent Node to be examined
 *
 * RETURN:      Current value of the object field from the Node whose
 *              handle is passed
 *
 ******************************************************************************/

void *
acpi_ns_get_attached_object (
	ACPI_HANDLE             handle)
{

	if (!handle) {
		/* handle invalid */

		return (NULL);
	}

	return (((ACPI_NAMESPACE_NODE *) handle)->object);
}


