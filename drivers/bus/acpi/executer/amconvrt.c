/******************************************************************************
 *
 * Module Name: amconvrt - Object conversion routines
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
#include "acnamesp.h"
#include "acinterp.h"
#include "acevents.h"
#include "amlcode.h"
#include "acdispat.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amconvrt")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_convert_to_integer
 *
 * PARAMETERS:  *Obj_desc       - Object to be converted.  Must be an
 *                                Integer, Buffer, or String
 *              Walk_state      - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an ACPI Object to an integer.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_convert_to_integer (
	ACPI_OPERAND_OBJECT     **obj_desc,
	ACPI_WALK_STATE         *walk_state)
{
	u32                     i;
	ACPI_OPERAND_OBJECT     *ret_desc;
	u32                     count;
	char                    *pointer;
	ACPI_INTEGER            result;
	u32                     integer_size = sizeof (ACPI_INTEGER);


	switch ((*obj_desc)->common.type) {
	case ACPI_TYPE_INTEGER:
		return (AE_OK);

	case ACPI_TYPE_STRING:
		pointer = (*obj_desc)->string.pointer;
		count = (*obj_desc)->string.length;
		break;

	case ACPI_TYPE_BUFFER:
		pointer = (char *) (*obj_desc)->buffer.pointer;
		count = (*obj_desc)->buffer.length;
		break;

	default:
		return (AE_TYPE);
	}

	/*
	 * Create a new integer
	 */
	ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_INTEGER);
	if (!ret_desc) {
		return (AE_NO_MEMORY);
	}


	/* Handle both ACPI 1.0 and ACPI 2.0 Integer widths */

	if (walk_state->method_node->flags & ANOBJ_DATA_WIDTH_32) {
		/*
		 * We are running a method that exists in a 32-bit ACPI table.
		 * Truncate the value to 32 bits by zeroing out the upper 32-bit field
		 */
		integer_size = sizeof (u32);
	}


	/*
	 * Convert the buffer/string to an integer.  Note that both buffers and
	 * strings are treated as raw data - we don't convert ascii to hex for
	 * strings.
	 *
	 * There are two terminating conditions for the loop:
	 * 1) The size of an integer has been reached, or
	 * 2) The end of the buffer or string has been reached
	 */
	result = 0;

	/* Transfer no more than an integer's worth of data */

	if (count > integer_size) {
		count = integer_size;
	}

	/*
	 * String conversion is different than Buffer conversion
	 */
	switch ((*obj_desc)->common.type) {
	case ACPI_TYPE_STRING:

		/* TBD: Need to use 64-bit STRTOUL */

		/*
		 * Convert string to an integer
		 * String must be hexadecimal as per the ACPI specification
		 */

		result = STRTOUL (pointer, NULL, 16);
		break;


	case ACPI_TYPE_BUFFER:

		/*
		 * Buffer conversion - we simply grab enough raw data from the
		 * buffer to fill an integer
		 */
		for (i = 0; i < count; i++) {
			/*
			 * Get next byte and shift it into the Result.
			 * Little endian is used, meaning that the first byte of the buffer
			 * is the LSB of the integer
			 */
			result |= (((ACPI_INTEGER) pointer[i]) << (i * 8));
		}

		break;
	}

	/* Save the Result, delete original descriptor, store new descriptor */

	ret_desc->integer.value = result;

	if (walk_state->opcode != AML_STORE_OP) {
		acpi_cm_remove_reference (*obj_desc);
	}

	*obj_desc = ret_desc;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_convert_to_buffer
 *
 * PARAMETERS:  *Obj_desc       - Object to be converted.  Must be an
 *                                Integer, Buffer, or String
 *              Walk_state      - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an ACPI Object to an Buffer
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_convert_to_buffer (
	ACPI_OPERAND_OBJECT     **obj_desc,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_OPERAND_OBJECT     *ret_desc;
	u32                     i;
	u32                     integer_size = sizeof (ACPI_INTEGER);
	u8                      *new_buf;


	switch ((*obj_desc)->common.type) {
	case ACPI_TYPE_INTEGER:

		/*
		 * Create a new Buffer
		 */
		ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_BUFFER);
		if (!ret_desc) {
			return (AE_NO_MEMORY);
		}

		/* Handle both ACPI 1.0 and ACPI 2.0 Integer widths */

		if (walk_state->method_node->flags & ANOBJ_DATA_WIDTH_32) {
			/*
			 * We are running a method that exists in a 32-bit ACPI table.
			 * Truncate the value to 32 bits by zeroing out the upper
			 * 32-bit field
			 */
			integer_size = sizeof (u32);
		}

		/* Need enough space for one integers */

		ret_desc->buffer.length = integer_size;
		new_buf = acpi_cm_callocate (integer_size);
		if (!new_buf) {
			REPORT_ERROR
				(("Aml_exec_dyadic2_r/Concat_op: Buffer allocation failure\n"));
			acpi_cm_remove_reference (ret_desc);
			return (AE_NO_MEMORY);
		}

		/* Copy the integer to the buffer */

		for (i = 0; i < integer_size; i++) {
			new_buf[i] = (u8) ((*obj_desc)->integer.value >> (i * 8));
		}
		ret_desc->buffer.pointer = new_buf;

		/* Return the new buffer descriptor */

		if (walk_state->opcode != AML_STORE_OP) {
			acpi_cm_remove_reference (*obj_desc);
		}
		*obj_desc = ret_desc;
		break;


	case ACPI_TYPE_STRING:
		break;


	case ACPI_TYPE_BUFFER:
		break;


	default:
		return (AE_TYPE);
		break;
   }

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_convert_to_string
 *
 * PARAMETERS:  *Obj_desc       - Object to be converted.  Must be an
 *                                Integer, Buffer, or String
 *              Walk_state      - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert an ACPI Object to a string
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_convert_to_string (
	ACPI_OPERAND_OBJECT     **obj_desc,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_OPERAND_OBJECT     *ret_desc;
	u32                     i;
	u32                     index;
	u32                     integer_size = sizeof (ACPI_INTEGER);
	u8                      *new_buf;
	u8                      *pointer;


	switch ((*obj_desc)->common.type) {
	case ACPI_TYPE_INTEGER:

		/*
		 * Create a new String
		 */
		ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_STRING);
		if (!ret_desc) {
			return (AE_NO_MEMORY);
		}

		/* Handle both ACPI 1.0 and ACPI 2.0 Integer widths */

		if (walk_state->method_node->flags & ANOBJ_DATA_WIDTH_32) {
			/*
			 * We are running a method that exists in a 32-bit ACPI table.
			 * Truncate the value to 32 bits by zeroing out the upper
			 * 32-bit field
			 */
			integer_size = sizeof (u32);
		}

		/* Need enough space for one ASCII integer plus null terminator */

		ret_desc->string.length = (integer_size * 2) + 1;
		new_buf = acpi_cm_callocate (ret_desc->string.length);
		if (!new_buf) {
			REPORT_ERROR
				(("Aml_exec_dyadic2_r/Concat_op: Buffer allocation failure\n"));
			acpi_cm_remove_reference (ret_desc);
			return (AE_NO_MEMORY);
		}

		/* Copy the integer to the buffer */

		for (i = 0; i < (integer_size * 2); i++) {
			new_buf[i] = acpi_gbl_hex_to_ascii [((*obj_desc)->integer.value >> (i * 4)) & 0xF];
		}

		/* Null terminate */

		new_buf [i] = 0;
		ret_desc->buffer.pointer = new_buf;

		/* Return the new buffer descriptor */

		if (walk_state->opcode != AML_STORE_OP) {
			acpi_cm_remove_reference (*obj_desc);
		}
		*obj_desc = ret_desc;

		return (AE_OK);


	case ACPI_TYPE_BUFFER:

		if (((*obj_desc)->buffer.length * 3) > ACPI_MAX_STRING_CONVERSION) {
			return (AE_AML_STRING_LIMIT);
		}

		/*
		 * Create a new String
		 */
		ret_desc = acpi_cm_create_internal_object (ACPI_TYPE_STRING);
		if (!ret_desc) {
			return (AE_NO_MEMORY);
		}

		/* Need enough space for one ASCII integer plus null terminator */

		ret_desc->string.length = (*obj_desc)->buffer.length * 3;
		new_buf = acpi_cm_callocate (ret_desc->string.length + 1);
		if (!new_buf) {
			REPORT_ERROR
				(("Aml_exec_dyadic2_r/Concat_op: Buffer allocation failure\n"));
			acpi_cm_remove_reference (ret_desc);
			return (AE_NO_MEMORY);
		}

		/*
		 * Convert each byte of the buffer to two ASCII characters plus a space.
		 */
		pointer = (*obj_desc)->buffer.pointer;
		index = 0;
		for (i = 0; i < (*obj_desc)->buffer.length; i++) {
			new_buf[index + 0] = acpi_gbl_hex_to_ascii [pointer[i] & 0x0F];
			new_buf[index + 1] = acpi_gbl_hex_to_ascii [(pointer[i] >> 4) & 0x0F];
			new_buf[index + 2] = ' ';
			index += 3;
		}

		/* Null terminate */

		new_buf [index] = 0;
		ret_desc->buffer.pointer = new_buf;

		/* Return the new buffer descriptor */

		if (walk_state->opcode != AML_STORE_OP) {
			acpi_cm_remove_reference (*obj_desc);
		}
		*obj_desc = ret_desc;
		break;


	case ACPI_TYPE_STRING:
		break;


	default:
		return (AE_TYPE);
		break;
   }

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_convert_to_target_type
 *
 * PARAMETERS:  *Obj_desc       - Object to be converted.
 *              Walk_state      - Current method state
 *
 * RETURN:      Status
 *
 * DESCRIPTION:
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_convert_to_target_type (
	OBJECT_TYPE_INTERNAL    destination_type,
	ACPI_OPERAND_OBJECT     **obj_desc,
	ACPI_WALK_STATE         *walk_state)
{
	ACPI_STATUS             status = AE_OK;


	/*
	 * If required by the target,
	 * perform implicit conversion on the source before we store it.
	 */

	switch (GET_CURRENT_ARG_TYPE (walk_state->op_info->runtime_args)) {
	case ARGI_SIMPLE_TARGET:
	case ARGI_FIXED_TARGET:
	case ARGI_INTEGER_REF:      /* Handles Increment, Decrement cases */

		switch (destination_type) {
		case INTERNAL_TYPE_DEF_FIELD:
			/*
			 * Named field can always handle conversions
			 */
			break;

		default:
			/* No conversion allowed for these types */

			if (destination_type != (*obj_desc)->common.type) {
				status = AE_TYPE;
			}
		}
		break;


	case ARGI_TARGETREF:

		switch (destination_type) {
		case ACPI_TYPE_INTEGER:
		case ACPI_TYPE_FIELD_UNIT:
		case INTERNAL_TYPE_BANK_FIELD:
		case INTERNAL_TYPE_INDEX_FIELD:
			/*
			 * These types require an Integer operand.  We can convert
			 * a Buffer or a String to an Integer if necessary.
			 */
			status = acpi_aml_convert_to_integer (obj_desc, walk_state);
			break;


		case ACPI_TYPE_STRING:

			/*
			 * The operand must be a String.  We can convert an
			 * Integer or Buffer if necessary
			 */
			status = acpi_aml_convert_to_string (obj_desc, walk_state);
			break;


		case ACPI_TYPE_BUFFER:

			/*
			 * The operand must be a String.  We can convert an
			 * Integer or Buffer if necessary
			 */
			status = acpi_aml_convert_to_buffer (obj_desc, walk_state);
			break;
		}
		break;


	case ARGI_REFERENCE:
		/*
		 * Create_xxxx_field cases - we are storing the field object into the name
		 */
		break;


	default:
		status = AE_AML_INTERNAL;
	}


	/*
	 * Source-to-Target conversion semantics:
	 *
	 * If conversion to the target type cannot be performed, then simply
	 * overwrite the target with the new object and type.
	 */
	if (status == AE_TYPE) {
		status = AE_OK;
	}

	return (status);
}


