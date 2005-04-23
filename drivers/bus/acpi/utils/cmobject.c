/******************************************************************************
 *
 * Module Name: cmobject - ACPI object create/delete/size/cache routines
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
#include "actables.h"
#include "amlcode.h"


#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cmobject")


/*******************************************************************************
 *
 * FUNCTION:    _Cm_create_internal_object
 *
 * PARAMETERS:  Address             - Address of the memory to deallocate
 *              Component           - Component type of caller
 *              Module              - Source file name of caller
 *              Line                - Line number of caller
 *              Type                - ACPI Type of the new object
 *
 * RETURN:      Object              - The new object.  Null on failure
 *
 * DESCRIPTION: Create and initialize a new internal object.
 *
 * NOTE:        We always allocate the worst-case object descriptor because
 *              these objects are cached, and we want them to be
 *              one-size-satisifies-any-request.  This in itself may not be
 *              the most memory efficient, but the efficiency of the object
 *              cache should more than make up for this!
 *
 ******************************************************************************/

ACPI_OPERAND_OBJECT  *
_cm_create_internal_object (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id,
	OBJECT_TYPE_INTERNAL    type)
{
	ACPI_OPERAND_OBJECT     *object;


	/* Allocate the raw object descriptor */

	object = _cm_allocate_object_desc (module_name, line_number, component_id);
	if (!object) {
		/* Allocation failure */

		return (NULL);
	}

	/* Save the object type in the object descriptor */

	object->common.type = type;

	/* Init the reference count */

	object->common.reference_count = 1;

	/* Any per-type initialization should go here */


	return (object);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_valid_internal_object
 *
 * PARAMETERS:  Operand             - Object to be validated
 *
 * RETURN:      Validate a pointer to be an ACPI_OPERAND_OBJECT
 *
 ******************************************************************************/

u8
acpi_cm_valid_internal_object (
	void                    *object)
{

	/* Check for a null pointer */

	if (!object) {
		return (FALSE);
	}

	/* Check for a pointer within one of the ACPI tables */

	if (acpi_tb_system_table_pointer (object)) {
		return (FALSE);
	}

	/* Check the descriptor type field */

	if (!VALID_DESCRIPTOR_TYPE (object, ACPI_DESC_TYPE_INTERNAL)) {
		/* Not an ACPI internal object, do some further checking */




		return (FALSE);
	}


	/* The object appears to be a valid ACPI_OPERAND_OBJECT  */

	return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    _Cm_allocate_object_desc
 *
 * PARAMETERS:  Module_name         - Caller's module name (for error output)
 *              Line_number         - Caller's line number (for error output)
 *              Component_id        - Caller's component ID (for error output)
 *              Message             - Error message to use on failure
 *
 * RETURN:      Pointer to newly allocated object descriptor.  Null on error
 *
 * DESCRIPTION: Allocate a new object descriptor.  Gracefully handle
 *              error conditions.
 *
 ******************************************************************************/

void *
_cm_allocate_object_desc (
	NATIVE_CHAR             *module_name,
	u32                     line_number,
	u32                     component_id)
{
	ACPI_OPERAND_OBJECT     *object;


	acpi_cm_acquire_mutex (ACPI_MTX_CACHES);

	acpi_gbl_object_cache_requests++;

	/* Check the cache first */

	if (acpi_gbl_object_cache) {
		/* There is an object available, use it */

		object = acpi_gbl_object_cache;
		acpi_gbl_object_cache = object->cache.next;
		object->cache.next = NULL;

		acpi_gbl_object_cache_hits++;
		acpi_gbl_object_cache_depth--;

		acpi_cm_release_mutex (ACPI_MTX_CACHES);
	}

	else {
		/* The cache is empty, create a new object */

		acpi_cm_release_mutex (ACPI_MTX_CACHES);

		/* Attempt to allocate new descriptor */

		object = _cm_callocate (sizeof (ACPI_OPERAND_OBJECT), component_id,
				  module_name, line_number);
		if (!object) {
			/* Allocation failed */

			_REPORT_ERROR (module_name, line_number, component_id,
					  ("Could not allocate an object descriptor\n"));

			return (NULL);
		}

		/* Memory allocation metrics - compiled out in non debug mode. */

		INCREMENT_OBJECT_METRICS (sizeof (ACPI_OPERAND_OBJECT));
	}

	/* Mark the descriptor type */

	object->common.data_type = ACPI_DESC_TYPE_INTERNAL;

	return (object);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_delete_object_desc
 *
 * PARAMETERS:  Object          - Acpi internal object to be deleted
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Free an ACPI object descriptor or add it to the object cache
 *
 ******************************************************************************/

void
acpi_cm_delete_object_desc (
	ACPI_OPERAND_OBJECT     *object)
{


	/* Make sure that the object isn't already in the cache */

	if (object->common.data_type == (ACPI_DESC_TYPE_INTERNAL | ACPI_CACHED_OBJECT)) {
		return;
	}

	/* Object must be an ACPI_OPERAND_OBJECT  */

	if (object->common.data_type != ACPI_DESC_TYPE_INTERNAL) {
		return;
	}


	/* If cache is full, just free this object */

	if (acpi_gbl_object_cache_depth >= MAX_OBJECT_CACHE_DEPTH) {
		/*
		 * Memory allocation metrics.  Call the macro here since we only
		 * care about dynamically allocated objects.
		 */
		DECREMENT_OBJECT_METRICS (sizeof (ACPI_OPERAND_OBJECT));

		acpi_cm_free (object);
		return;
	}

	acpi_cm_acquire_mutex (ACPI_MTX_CACHES);

	/* Clear the entire object.  This is important! */

	MEMSET (object, 0, sizeof (ACPI_OPERAND_OBJECT));
	object->common.data_type = ACPI_DESC_TYPE_INTERNAL | ACPI_CACHED_OBJECT;

	/* Put the object at the head of the global cache list */

	object->cache.next = acpi_gbl_object_cache;
	acpi_gbl_object_cache = object;
	acpi_gbl_object_cache_depth++;


	acpi_cm_release_mutex (ACPI_MTX_CACHES);
	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_delete_object_cache
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Purge the global state object cache.  Used during subsystem
 *              termination.
 *
 ******************************************************************************/

void
acpi_cm_delete_object_cache (
	void)
{
	ACPI_OPERAND_OBJECT     *next;


	/* Traverse the global cache list */

	while (acpi_gbl_object_cache) {
		/* Delete one cached state object */

		next = acpi_gbl_object_cache->cache.next;
		acpi_gbl_object_cache->cache.next = NULL;

		/*
		 * Memory allocation metrics.  Call the macro here since we only
		 * care about dynamically allocated objects.
		 */
		DECREMENT_OBJECT_METRICS (sizeof (ACPI_OPERAND_OBJECT));

		acpi_cm_free (acpi_gbl_object_cache);
		acpi_gbl_object_cache = next;
		acpi_gbl_object_cache_depth--;
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_init_static_object
 *
 * PARAMETERS:  Obj_desc            - Pointer to a "static" object - on stack
 *                                    or in the data segment.
 *
 * RETURN:      None.
 *
 * DESCRIPTION: Initialize a static object.  Sets flags to disallow dynamic
 *              deletion of the object.
 *
 ******************************************************************************/

void
acpi_cm_init_static_object (
	ACPI_OPERAND_OBJECT     *obj_desc)
{


	if (!obj_desc) {
		return;
	}


	/*
	 * Clear the entire descriptor
	 */
	MEMSET ((void *) obj_desc, 0, sizeof (ACPI_OPERAND_OBJECT));


	/*
	 * Initialize the header fields
	 * 1) This is an ACPI_OPERAND_OBJECT  descriptor
	 * 2) The size is the full object (worst case)
	 * 3) The flags field indicates static allocation
	 * 4) Reference count starts at one (not really necessary since the
	 *    object can't be deleted, but keeps everything sane)
	 */

	obj_desc->common.data_type      = ACPI_DESC_TYPE_INTERNAL;
	obj_desc->common.flags          = AOPOBJ_STATIC_ALLOCATION;
	obj_desc->common.reference_count = 1;

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_get_simple_object_size
 *
 * PARAMETERS:  *Internal_object    - Pointer to the object we are examining
 *              *Ret_length         - Where the length is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to determine the space required to
 *              contain a simple object for return to an API user.
 *
 *              The length includes the object structure plus any additional
 *              needed space.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_get_simple_object_size (
	ACPI_OPERAND_OBJECT     *internal_object,
	u32                     *obj_length)
{
	u32                     length;
	ACPI_STATUS             status = AE_OK;


	/* Handle a null object (Could be a uninitialized package element -- which is legal) */

	if (!internal_object) {
		*obj_length = 0;
		return (AE_OK);
	}


	/* Start with the length of the Acpi object */

	length = sizeof (ACPI_OBJECT);

	if (VALID_DESCRIPTOR_TYPE (internal_object, ACPI_DESC_TYPE_NAMED)) {
		/* Object is a named object (reference), just return the length */

		*obj_length = (u32) ROUND_UP_TO_NATIVE_WORD (length);
		return (status);
	}


	/*
	 * The final length depends on the object type
	 * Strings and Buffers are packed right up against the parent object and
	 * must be accessed bytewise or there may be alignment problems on
	 * certain processors
	 */

	switch (internal_object->common.type) {

	case ACPI_TYPE_STRING:

		length += internal_object->string.length + 1;
		break;


	case ACPI_TYPE_BUFFER:

		length += internal_object->buffer.length;
		break;


	case ACPI_TYPE_INTEGER:
	case ACPI_TYPE_PROCESSOR:
	case ACPI_TYPE_POWER:

		/*
		 * No extra data for these types
		 */
		break;


	case INTERNAL_TYPE_REFERENCE:

		/*
		 * The only type that should be here is opcode AML_NAMEPATH_OP -- since
		 * this means an object reference
		 */
		if (internal_object->reference.opcode != AML_NAMEPATH_OP) {
			status = AE_TYPE;
		}

		else {
			/*
			 * Get the actual length of the full pathname to this object.
			 * The reference will be converted to the pathname to the object
			 */
			length += ROUND_UP_TO_NATIVE_WORD (acpi_ns_get_pathname_length (internal_object->reference.node));
		}
		break;


	default:

		status = AE_TYPE;
		break;
	}


	/*
	 * Account for the space required by the object rounded up to the next
	 * multiple of the machine word size.  This keeps each object aligned
	 * on a machine word boundary. (preventing alignment faults on some
	 * machines.)
	 */
	*obj_length = (u32) ROUND_UP_TO_NATIVE_WORD (length);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_get_element_length
 *
 * PARAMETERS:  ACPI_PKG_CALLBACK
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: Get the length of one package element.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_get_element_length (
	u8                      object_type,
	ACPI_OPERAND_OBJECT     *source_object,
	ACPI_GENERIC_STATE      *state,
	void                    *context)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_PKG_INFO           *info = (ACPI_PKG_INFO *) context;
	u32                     object_space;


	switch (object_type) {
	case 0:

		/*
		 * Simple object - just get the size (Null object/entry is handled
		 * here also) and sum it into the running package length
		 */
		status = acpi_cm_get_simple_object_size (source_object, &object_space);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		info->length += object_space;
		break;


	case 1:
		/* Package - nothing much to do here, let the walk handle it */

		info->num_packages++;
		state->pkg.this_target_obj = NULL;
		break;

	default:
		return (AE_BAD_PARAMETER);
	}


	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_get_package_object_size
 *
 * PARAMETERS:  *Internal_object    - Pointer to the object we are examining
 *              *Ret_length         - Where the length is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to determine the space required to
 *              contain a package object for return to an API user.
 *
 *              This is moderately complex since a package contains other
 *              objects including packages.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_get_package_object_size (
	ACPI_OPERAND_OBJECT     *internal_object,
	u32                     *obj_length)
{
	ACPI_STATUS             status;
	ACPI_PKG_INFO           info;


	info.length      = 0;
	info.object_space = 0;
	info.num_packages = 1;

	status = acpi_cm_walk_package_tree (internal_object, NULL,
			 acpi_cm_get_element_length, &info);

	/*
	 * We have handled all of the objects in all levels of the package.
	 * just add the length of the package objects themselves.
	 * Round up to the next machine word.
	 */
	info.length += ROUND_UP_TO_NATIVE_WORD (sizeof (ACPI_OBJECT)) *
			  info.num_packages;

	/* Return the total package length */

	*obj_length = info.length;
	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_get_object_size
 *
 * PARAMETERS:  *Internal_object    - Pointer to the object we are examining
 *              *Ret_length         - Where the length will be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to determine the space required to
 *              contain an object for return to an API user.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_get_object_size(
	ACPI_OPERAND_OBJECT     *internal_object,
	u32                     *obj_length)
{
	ACPI_STATUS             status;


	if ((VALID_DESCRIPTOR_TYPE (internal_object, ACPI_DESC_TYPE_INTERNAL)) &&
		(IS_THIS_OBJECT_TYPE (internal_object, ACPI_TYPE_PACKAGE))) {
		status = acpi_cm_get_package_object_size (internal_object, obj_length);
	}

	else {
		status = acpi_cm_get_simple_object_size (internal_object, obj_length);
	}

	return (status);
}


