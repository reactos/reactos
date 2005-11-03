/******************************************************************************
 *
 * Module Name: cmcopy - Internal to external object translation utilities
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
#include "acinterp.h"
#include "acnamesp.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmcopy")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_isimple_to_esimple
 *
 * PARAMETERS:  *Internal_object    - Pointer to the object we are examining
 *              *Buffer             - Where the object is returned
 *              *Space_used         - Where the data length is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to place a simple object in a user
 *              buffer.
 *
 *              The buffer is assumed to have sufficient space for the object.
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_cm_copy_isimple_to_esimple (
	ACPI_OPERAND_OBJECT     *internal_object,
	ACPI_OBJECT             *external_object,
	u8                      *data_space,
	u32                     *buffer_space_used)
{
	u32                     length = 0;
	ACPI_STATUS             status = AE_OK;


	/*
	 * Check for NULL object case (could be an uninitialized
	 * package element
	 */

	if (!internal_object) {
		*buffer_space_used = 0;
		return (AE_OK);
	}

	/* Always clear the external object */

	MEMSET (external_object, 0, sizeof (ACPI_OBJECT));

	/*
	 * In general, the external object will be the same type as
	 * the internal object
	 */

	external_object->type = internal_object->common.type;

	/* However, only a limited number of external types are supported */

	switch (internal_object->common.type) {

	case ACPI_TYPE_STRING:

		length = internal_object->string.length + 1;
		external_object->string.length = internal_object->string.length;
		external_object->string.pointer = (NATIVE_CHAR *) data_space;
		MEMCPY ((void *) data_space, (void *) internal_object->string.pointer, length);
		break;


	case ACPI_TYPE_BUFFER:

		length = internal_object->buffer.length;
		external_object->buffer.length = internal_object->buffer.length;
		external_object->buffer.pointer = data_space;
		MEMCPY ((void *) data_space, (void *) internal_object->buffer.pointer, length);
		break;


	case ACPI_TYPE_INTEGER:

		external_object->integer.value= internal_object->integer.value;
		break;


	case INTERNAL_TYPE_REFERENCE:

		/*
		 * This is an object reference.  Attempt to dereference it.
		 */

		switch (internal_object->reference.opcode) {
		case AML_ZERO_OP:
			external_object->type = ACPI_TYPE_INTEGER;
			external_object->integer.value = 0;
			break;

		case AML_ONE_OP:
			external_object->type = ACPI_TYPE_INTEGER;
			external_object->integer.value = 1;
			break;

		case AML_ONES_OP:
			external_object->type = ACPI_TYPE_INTEGER;
			external_object->integer.value = ACPI_INTEGER_MAX;
			break;

		case AML_NAMEPATH_OP:
			/*
			 * This is a named reference, get the string.  We already know that
			 * we have room for it, use max length
			 */
			length = MAX_STRING_LENGTH;
			external_object->type = ACPI_TYPE_STRING;
			external_object->string.pointer = (NATIVE_CHAR *) data_space;
			status = acpi_ns_handle_to_pathname ((ACPI_HANDLE *) internal_object->reference.node,
					 &length, (char *) data_space);

			/* Converted (external) string length is returned from above */

			external_object->string.length = length;
			break;

		default:
			/*
			 * Use the object type of "Any" to indicate a reference
			 * to object containing a handle to an ACPI named object.
			 */
			external_object->type = ACPI_TYPE_ANY;
			external_object->reference.handle = internal_object->reference.node;
			break;
		}
		break;


	case ACPI_TYPE_PROCESSOR:

		external_object->processor.proc_id = internal_object->processor.proc_id;
		external_object->processor.pblk_address = internal_object->processor.address;
		external_object->processor.pblk_length = internal_object->processor.length;
		break;


	case ACPI_TYPE_POWER:

		external_object->power_resource.system_level =
				   internal_object->power_resource.system_level;

		external_object->power_resource.resource_order =
				   internal_object->power_resource.resource_order;
		break;


	default:
		/*
		 * There is no corresponding external object type
		 */
		return (AE_SUPPORT);
		break;
	}


	*buffer_space_used = (u32) ROUND_UP_TO_NATIVE_WORD (length);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_ielement_to_eelement
 *
 * PARAMETERS:  ACPI_PKG_CALLBACK
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Copy one package element to another package element
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_copy_ielement_to_eelement (
	u8                      object_type,
	ACPI_OPERAND_OBJECT     *source_object,
	ACPI_GENERIC_STATE      *state,
	void                    *context)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_PKG_INFO           *info = (ACPI_PKG_INFO *) context;
	u32                     object_space;
	u32                     this_index;
	ACPI_OBJECT             *target_object;


	this_index      = state->pkg.index;
	target_object   = (ACPI_OBJECT *)
			   &((ACPI_OBJECT *)(state->pkg.dest_object))->package.elements[this_index];


	switch (object_type) {
	case ACPI_COPY_TYPE_SIMPLE:

		/*
		 * This is a simple or null object -- get the size
		 */

		status = acpi_cm_copy_isimple_to_esimple (source_object,
				  target_object, info->free_space, &object_space);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		break;

	case ACPI_COPY_TYPE_PACKAGE:

		/*
		 * Build the package object
		 */
		target_object->type             = ACPI_TYPE_PACKAGE;
		target_object->package.count    = source_object->package.count;
		target_object->package.elements = (ACPI_OBJECT *) info->free_space;

		/*
		 * Pass the new package object back to the package walk routine
		 */
		state->pkg.this_target_obj = target_object;

		/*
		 * Save space for the array of objects (Package elements)
		 * update the buffer length counter
		 */
		object_space = (u32) ROUND_UP_TO_NATIVE_WORD (
				   target_object->package.count * sizeof (ACPI_OBJECT));
		break;

	default:
		return (AE_BAD_PARAMETER);
	}


	info->free_space  += object_space;
	info->length      += object_space;

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_ipackage_to_epackage
 *
 * PARAMETERS:  *Internal_object    - Pointer to the object we are returning
 *              *Buffer             - Where the object is returned
 *              *Space_used         - Where the object length is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to place a package object in a user
 *              buffer.  A package object by definition contains other objects.
 *
 *              The buffer is assumed to have sufficient space for the object.
 *              The caller must have verified the buffer length needed using the
 *              Acpi_cm_get_object_size function before calling this function.
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_cm_copy_ipackage_to_epackage (
	ACPI_OPERAND_OBJECT     *internal_object,
	u8                      *buffer,
	u32                     *space_used)
{
	ACPI_OBJECT             *external_object;
	ACPI_STATUS             status;
	ACPI_PKG_INFO           info;


	/*
	 * First package at head of the buffer
	 */
	external_object = (ACPI_OBJECT *) buffer;

	/*
	 * Free space begins right after the first package
	 */
	info.length      = 0;
	info.object_space = 0;
	info.num_packages = 1;
	info.free_space  = buffer + ROUND_UP_TO_NATIVE_WORD (sizeof (ACPI_OBJECT));


	external_object->type              = internal_object->common.type;
	external_object->package.count     = internal_object->package.count;
	external_object->package.elements  = (ACPI_OBJECT *) info.free_space;


	/*
	 * Build an array of ACPI_OBJECTS in the buffer
	 * and move the free space past it
	 */

	info.free_space += external_object->package.count *
			  ROUND_UP_TO_NATIVE_WORD (sizeof (ACPI_OBJECT));


	status = acpi_cm_walk_package_tree (internal_object, external_object,
			 acpi_cm_copy_ielement_to_eelement, &info);

	*space_used = info.length;

	return (status);

}

/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_iobject_to_eobject
 *
 * PARAMETERS:  *Internal_object    - The internal object to be converted
 *              *Buffer_ptr         - Where the object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to build an API object to be returned to
 *              the caller.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_copy_iobject_to_eobject (
	ACPI_OPERAND_OBJECT     *internal_object,
	ACPI_BUFFER             *ret_buffer)
{
	ACPI_STATUS             status;


	if (IS_THIS_OBJECT_TYPE (internal_object, ACPI_TYPE_PACKAGE)) {
		/*
		 * Package object:  Copy all subobjects (including
		 * nested packages)
		 */
		status = acpi_cm_copy_ipackage_to_epackage (internal_object,
				  ret_buffer->pointer, &ret_buffer->length);
	}

	else {
		/*
		 * Build a simple object (no nested objects)
		 */
		status = acpi_cm_copy_isimple_to_esimple (internal_object,
				  (ACPI_OBJECT *) ret_buffer->pointer,
				  ((u8 *) ret_buffer->pointer +
				  ROUND_UP_TO_NATIVE_WORD (sizeof (ACPI_OBJECT))),
				  &ret_buffer->length);
		/*
		 * build simple does not include the object size in the length
		 * so we add it in here
		 */
		ret_buffer->length += sizeof (ACPI_OBJECT);
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_esimple_to_isimple
 *
 * PARAMETERS:  *External_object   - The external object to be converted
 *              *Internal_object   - Where the internal object is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function copies an external object to an internal one.
 *              NOTE: Pointers can be copied, we don't need to copy data.
 *              (The pointers have to be valid in our address space no matter
 *              what we do with them!)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_copy_esimple_to_isimple (
	ACPI_OBJECT             *external_object,
	ACPI_OPERAND_OBJECT     *internal_object)
{


	internal_object->common.type = (u8) external_object->type;

	switch (external_object->type) {

	case ACPI_TYPE_STRING:

		internal_object->string.length = external_object->string.length;
		internal_object->string.pointer = external_object->string.pointer;
		break;


	case ACPI_TYPE_BUFFER:

		internal_object->buffer.length = external_object->buffer.length;
		internal_object->buffer.pointer = external_object->buffer.pointer;
		break;


	case ACPI_TYPE_INTEGER:
		/*
		 * Number is included in the object itself
		 */
		internal_object->integer.value  = external_object->integer.value;
		break;


	default:
		return (AE_CTRL_RETURN_VALUE);
		break;
	}


	return (AE_OK);
}


#ifdef ACPI_FUTURE_IMPLEMENTATION

/* Code to convert packages that are parameters to control methods */

/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_epackage_to_ipackage
 *
 * PARAMETERS:  *Internal_object   - Pointer to the object we are returning
 *              *Buffer         - Where the object is returned
 *              *Space_used     - Where the length of the object is returned
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to place a package object in a user
 *              buffer.  A package object by definition contains other objects.
 *
 *              The buffer is assumed to have sufficient space for the object.
 *              The caller must have verified the buffer length needed using the
 *              Acpi_cm_get_object_size function before calling this function.
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_cm_copy_epackage_to_ipackage (
	ACPI_OPERAND_OBJECT     *internal_object,
	u8                      *buffer,
	u32                     *space_used)
{
	u8                      *free_space;
	ACPI_OBJECT             *external_object;
	u32                     length = 0;
	u32                     this_index;
	u32                     object_space = 0;
	ACPI_OPERAND_OBJECT     *this_internal_obj;
	ACPI_OBJECT             *this_external_obj;


	/*
	 * First package at head of the buffer
	 */
	external_object = (ACPI_OBJECT *)buffer;

	/*
	 * Free space begins right after the first package
	 */
	free_space = buffer + sizeof(ACPI_OBJECT);


	external_object->type              = internal_object->common.type;
	external_object->package.count     = internal_object->package.count;
	external_object->package.elements  = (ACPI_OBJECT *)free_space;


	/*
	 * Build an array of ACPI_OBJECTS in the buffer
	 * and move the free space past it
	 */

	free_space += external_object->package.count * sizeof(ACPI_OBJECT);


	/* Call Walk_package */

}

#endif /* Future implementation */


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_eobject_to_iobject
 *
 * PARAMETERS:  *Internal_object   - The external object to be converted
 *              *Buffer_ptr     - Where the internal object is returned
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: Converts an external object to an internal object.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_copy_eobject_to_iobject (
	ACPI_OBJECT             *external_object,
	ACPI_OPERAND_OBJECT     *internal_object)
{
	ACPI_STATUS             status;


	if (external_object->type == ACPI_TYPE_PACKAGE) {
		/*
		 * Package objects contain other objects (which can be objects)
		 * buildpackage does it all
		 *
		 * TBD: Package conversion must be completed and tested
		 * NOTE: this code converts packages as input parameters to
		 * control methods only.  This is a very, very rare case.
		 */
/*
		Status = Acpi_cm_copy_epackage_to_ipackage(Internal_object,
				 Ret_buffer->Pointer,
				 &Ret_buffer->Length);
*/
		return (AE_NOT_IMPLEMENTED);
	}

	else {
		/*
		 * Build a simple object (no nested objects)
		 */
		status = acpi_cm_copy_esimple_to_isimple (external_object, internal_object);
		/*
		 * build simple does not include the object size in the length
		 * so we add it in here
		 */
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_ielement_to_ielement
 *
 * PARAMETERS:  ACPI_PKG_CALLBACK
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: Copy one package element to another package element
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_copy_ielement_to_ielement (
	u8                      object_type,
	ACPI_OPERAND_OBJECT     *source_object,
	ACPI_GENERIC_STATE      *state,
	void                    *context)
{
	ACPI_STATUS             status = AE_OK;
	u32                     this_index;
	ACPI_OPERAND_OBJECT     **this_target_ptr;
	ACPI_OPERAND_OBJECT     *target_object;


	this_index    = state->pkg.index;
	this_target_ptr = (ACPI_OPERAND_OBJECT **)
			   &state->pkg.dest_object->package.elements[this_index];

	switch (object_type) {
	case 0:

		/*
		 * This is a simple object, just copy it
		 */
		target_object = acpi_cm_create_internal_object (source_object->common.type);
		if (!target_object) {
			return (AE_NO_MEMORY);
		}

		status = acpi_aml_store_object_to_object (source_object, target_object,
				  (ACPI_WALK_STATE *) context);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		*this_target_ptr = target_object;
		break;


	case 1:
		/*
		 * This object is a package - go down another nesting level
		 * Create and build the package object
		 */
		target_object = acpi_cm_create_internal_object (ACPI_TYPE_PACKAGE);
		if (!target_object) {
			/* TBD: must delete package created up to this point */

			return (AE_NO_MEMORY);
		}

		target_object->package.count = source_object->package.count;

		/*
		 * Pass the new package object back to the package walk routine
		 */
		state->pkg.this_target_obj = target_object;

		/*
		 * Store the object pointer in the parent package object
		 */
		*this_target_ptr = target_object;
		break;

	default:
		return (AE_BAD_PARAMETER);
	}


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_copy_ipackage_to_ipackage
 *
 * PARAMETERS:  *Source_obj     - Pointer to the source package object
 *              *Dest_obj       - Where the internal object is returned
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to copy an internal package object
 *              into another internal package object.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_copy_ipackage_to_ipackage (
	ACPI_OPERAND_OBJECT     *source_obj,
	ACPI_OPERAND_OBJECT     *dest_obj,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;


	dest_obj->common.type   = source_obj->common.type;
	dest_obj->package.count = source_obj->package.count;


	/*
	 * Create the object array and walk the source package tree
	 */

	dest_obj->package.elements = acpi_cm_callocate ((source_obj->package.count + 1) *
			 sizeof (void *));
	dest_obj->package.next_element = dest_obj->package.elements;

	if (!dest_obj->package.elements) {
		REPORT_ERROR (
			("Aml_build_copy_internal_package_object: Package allocation failure\n"));
		return (AE_NO_MEMORY);
	}


	status = acpi_cm_walk_package_tree (source_obj, dest_obj,
			 acpi_cm_copy_ielement_to_ielement, walk_state);

	return (status);
}

