
/******************************************************************************
 *
 * Module Name: amstorob - AML Interpreter object store support, store to object
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
	 MODULE_NAME         ("amstorob")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_copy_buffer_to_buffer
 *
 * PARAMETERS:  Source_desc         - Source object to copy
 *              Target_desc         - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Copy a buffer object to another buffer object.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_copy_buffer_to_buffer (
	ACPI_OPERAND_OBJECT     *source_desc,
	ACPI_OPERAND_OBJECT     *target_desc)
{
	u32                     length;
	u8                      *buffer;


	/*
	 * We know that Source_desc is a buffer by now
	 */
	buffer = (u8 *) source_desc->buffer.pointer;
	length = source_desc->buffer.length;

	/*
	 * If target is a buffer of length zero, allocate a new
	 * buffer of the proper length
	 */
	if (target_desc->buffer.length == 0) {
		target_desc->buffer.pointer = acpi_cm_allocate (length);
		if (!target_desc->buffer.pointer) {
			return (AE_NO_MEMORY);
		}

		target_desc->buffer.length = length;
	}

	/*
	 * Buffer is a static allocation,
	 * only place what will fit in the buffer.
	 */
	if (length <= target_desc->buffer.length) {
		/* Clear existing buffer and copy in the new one */

		MEMSET(target_desc->buffer.pointer, 0, target_desc->buffer.length);
		MEMCPY(target_desc->buffer.pointer, buffer, length);
	}

	else {
		/*
		 * Truncate the source, copy only what will fit
		 */
		MEMCPY(target_desc->buffer.pointer, buffer, target_desc->buffer.length);

	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_copy_string_to_string
 *
 * PARAMETERS:  Source_desc         - Source object to copy
 *              Target_desc         - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Copy a String object to another String object
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_copy_string_to_string (
	ACPI_OPERAND_OBJECT     *source_desc,
	ACPI_OPERAND_OBJECT     *target_desc)
{
	u32                     length;
	u8                      *buffer;


	/*
	 * We know that Source_desc is a string by now.
	 */
	buffer = (u8 *) source_desc->string.pointer;
	length = source_desc->string.length;

	/*
	 * Setting a string value replaces the old string
	 */
	if (length < target_desc->string.length) {
		/* Clear old string and copy in the new one */

		MEMSET(target_desc->string.pointer, 0, target_desc->string.length);
		MEMCPY(target_desc->string.pointer, buffer, length);
	}

	else {
		/*
		 * Free the current buffer, then allocate a buffer
		 * large enough to hold the value
		 */
		if (target_desc->string.pointer &&
			!acpi_tb_system_table_pointer (target_desc->string.pointer)) {
			/*
			 * Only free if not a pointer into the DSDT
			 */
			acpi_cm_free(target_desc->string.pointer);
		}

		target_desc->string.pointer = acpi_cm_allocate (length + 1);
		if (!target_desc->string.pointer) {
			return (AE_NO_MEMORY);
		}
		target_desc->string.length = length;


		MEMCPY(target_desc->string.pointer, buffer, length);
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_copy_integer_to_index_field
 *
 * PARAMETERS:  Source_desc         - Source object to copy
 *              Target_desc         - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write an Integer to an Index Field
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_copy_integer_to_index_field (
	ACPI_OPERAND_OBJECT     *source_desc,
	ACPI_OPERAND_OBJECT     *target_desc)
{
	ACPI_STATUS             status;
	u8                      locked;


	/*
	 * Get the global lock if needed
	 */
	locked = acpi_aml_acquire_global_lock (target_desc->index_field.lock_rule);

	/*
	 * Set Index value to select proper Data register
	 * perform the update (Set index)
	 */
	status = acpi_aml_access_named_field (ACPI_WRITE,
			 target_desc->index_field.index,
			 &target_desc->index_field.value,
			 sizeof (target_desc->index_field.value));
	if (ACPI_SUCCESS (status)) {
		/* Set_index was successful, next set Data value */

		status = acpi_aml_access_named_field (ACPI_WRITE,
				   target_desc->index_field.data,
				   &source_desc->integer.value,
				   sizeof (source_desc->integer.value));

	}



	/*
	 * Release global lock if we acquired it earlier
	 */
	acpi_aml_release_global_lock (locked);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_copy_integer_to_bank_field
 *
 * PARAMETERS:  Source_desc         - Source object to copy
 *              Target_desc         - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write an Integer to a Bank Field
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_copy_integer_to_bank_field (
	ACPI_OPERAND_OBJECT     *source_desc,
	ACPI_OPERAND_OBJECT     *target_desc)
{
	ACPI_STATUS             status;
	u8                      locked;


	/*
	 * Get the global lock if needed
	 */
	locked = acpi_aml_acquire_global_lock (target_desc->index_field.lock_rule);


	/*
	 * Set Bank value to select proper Bank
	 * Perform the update (Set Bank Select)
	 */

	status = acpi_aml_access_named_field (ACPI_WRITE,
			 target_desc->bank_field.bank_select,
			 &target_desc->bank_field.value,
			 sizeof (target_desc->bank_field.value));
	if (ACPI_SUCCESS (status)) {
		/* Set bank select successful, set data value  */

		status = acpi_aml_access_named_field (ACPI_WRITE,
				   target_desc->bank_field.bank_select,
				   &source_desc->bank_field.value,
				   sizeof (source_desc->bank_field.value));
	}



	/*
	 * Release global lock if we acquired it earlier
	 */
	acpi_aml_release_global_lock (locked);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_copy_data_to_named_field
 *
 * PARAMETERS:  Source_desc         - Source object to copy
 *              Node                - Destination Namespace node
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Copy raw data to a Named Field.  No implicit conversion
 *              is performed on the source object
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_copy_data_to_named_field (
	ACPI_OPERAND_OBJECT     *source_desc,
	ACPI_NAMESPACE_NODE     *node)
{
	ACPI_STATUS             status;
	u8                      locked;
	u32                     length;
	u8                      *buffer;


	/*
	 * Named fields (Create_xxx_field) - We don't perform any conversions on the
	 * source operand, just use the raw data
	 */
	switch (source_desc->common.type) {
	case ACPI_TYPE_INTEGER:
		buffer = (u8 *) &source_desc->integer.value;
		length = sizeof (source_desc->integer.value);
		break;

	case ACPI_TYPE_BUFFER:
		buffer = (u8 *) source_desc->buffer.pointer;
		length = source_desc->buffer.length;
		break;

	case ACPI_TYPE_STRING:
		buffer = (u8 *) source_desc->string.pointer;
		length = source_desc->string.length;
		break;

	default:
		return (AE_TYPE);
	}

	/*
	 * Get the global lock if needed before the update
	 * TBD: not needed!
	 */
	locked = acpi_aml_acquire_global_lock (source_desc->field.lock_rule);

	status = acpi_aml_access_named_field (ACPI_WRITE,
			  node, buffer, length);

	acpi_aml_release_global_lock (locked);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_copy_integer_to_field_unit
 *
 * PARAMETERS:  Source_desc         - Source object to copy
 *              Target_desc         - Destination object of the copy
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Write an Integer to a Field Unit.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_copy_integer_to_field_unit (
	ACPI_OPERAND_OBJECT     *source_desc,
	ACPI_OPERAND_OBJECT     *target_desc)
{
	ACPI_STATUS             status = AE_OK;
	u8                      *location = NULL;
	u32                     mask;
	u32                     new_value;
	u8                      locked = FALSE;


	/*
	 * If the Field Buffer and Index have not been previously evaluated,
	 * evaluate them and save the results.
	 */
	if (!(target_desc->common.flags & AOPOBJ_DATA_VALID)) {
		status = acpi_ds_get_field_unit_arguments (target_desc);
		if (ACPI_FAILURE (status)) {
			return (status);
		}
	}

	if ((!target_desc->field_unit.container ||
		ACPI_TYPE_BUFFER != target_desc->field_unit.container->common.type)) {
		return (AE_AML_INTERNAL);
	}

	/*
	 * Get the global lock if needed
	 */
	locked = acpi_aml_acquire_global_lock (target_desc->field_unit.lock_rule);

	/*
	 * TBD: [Unhandled] REMOVE this limitation
	 * Make sure the operation is within the limits of our implementation
	 * this is not a Spec limitation!!
	 */
	if (target_desc->field_unit.length + target_desc->field_unit.bit_offset > 32) {
		return (AE_NOT_IMPLEMENTED);
	}

	/* Field location is (base of buffer) + (byte offset) */

	location = target_desc->field_unit.container->buffer.pointer
			  + target_desc->field_unit.offset;

	/*
	 * Construct Mask with 1 bits where the field is,
	 * 0 bits elsewhere
	 */
	mask = ((u32) 1 << target_desc->field_unit.length) - ((u32)1
			   << target_desc->field_unit.bit_offset);

	/* Zero out the field in the buffer */

	MOVE_UNALIGNED32_TO_32 (&new_value, location);
	new_value &= ~mask;

	/*
	 * Shift and mask the new value into position,
	 * and or it into the buffer.
	 */
	new_value |= (source_desc->integer.value << target_desc->field_unit.bit_offset) &
			 mask;

	/* Store back the value */

	MOVE_UNALIGNED32_TO_32 (location, &new_value);

	return (AE_OK);
}


