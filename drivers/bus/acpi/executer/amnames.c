
/******************************************************************************
 *
 * Module Name: amnames - interpreter/scanner name load/execute
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
#include "amlcode.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amnames")


/* AML Package Length encodings */

#define ACPI_AML_PACKAGE_TYPE1   0x40
#define ACPI_AML_PACKAGE_TYPE2   0x4000
#define ACPI_AML_PACKAGE_TYPE3   0x400000
#define ACPI_AML_PACKAGE_TYPE4   0x40000000


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_allocate_name_string
 *
 * PARAMETERS:  Prefix_count        - Count of parent levels. Special cases:
 *                                    (-1) = root,  0 = none
 *              Num_name_segs       - count of 4-character name segments
 *
 * RETURN:      A pointer to the allocated string segment.  This segment must
 *              be deleted by the caller.
 *
 * DESCRIPTION: Allocate a buffer for a name string. Ensure allocated name
 *              string is long enough, and set up prefix if any.
 *
 ******************************************************************************/

NATIVE_CHAR *
acpi_aml_allocate_name_string (
	u32                     prefix_count,
	u32                     num_name_segs)
{
	NATIVE_CHAR             *temp_ptr;
	NATIVE_CHAR             *name_string;
	u32                      size_needed;


	/*
	 * Allow room for all \ and ^ prefixes, all segments, and a Multi_name_prefix.
	 * Also, one byte for the null terminator.
	 * This may actually be somewhat longer than needed.
	 */

	if (prefix_count == (u32) -1) {
		/* Special case for root */

		size_needed = 1 + (ACPI_NAME_SIZE * num_name_segs) + 2 + 1;
	}
	else {
		size_needed = prefix_count + (ACPI_NAME_SIZE * num_name_segs) + 2 + 1;
	}

	/*
	 * Allocate a buffer for the name.
	 * This buffer must be deleted by the caller!
	 */

	name_string = acpi_cm_allocate (size_needed);
	if (!name_string) {
		REPORT_ERROR (("Aml_allocate_name_string: name allocation failure\n"));
		return (NULL);
	}

	temp_ptr = name_string;

	/* Set up Root or Parent prefixes if needed */

	if (prefix_count == (u32) -1) {
		*temp_ptr++ = AML_ROOT_PREFIX;
	}

	else {
		while (prefix_count--) {
			*temp_ptr++ = AML_PARENT_PREFIX;
		}
	}


	/* Set up Dual or Multi prefixes if needed */

	if (num_name_segs > 2) {
		/* Set up multi prefixes   */

		*temp_ptr++ = AML_MULTI_NAME_PREFIX_OP;
		*temp_ptr++ = (char) num_name_segs;
	}

	else if (2 == num_name_segs) {
		/* Set up dual prefixes */

		*temp_ptr++ = AML_DUAL_NAME_PREFIX;
	}

	/*
	 * Terminate string following prefixes. Acpi_aml_exec_name_segment() will
	 * append the segment(s)
	 */

	*temp_ptr = 0;

	return (name_string);
}

/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_exec_name_segment
 *
 * PARAMETERS:  Interpreter_mode    - Current running mode (load1/Load2/Exec)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute a name segment (4 bytes)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_exec_name_segment (
	u8                      **in_aml_address,
	NATIVE_CHAR             *name_string)
{
	u8                      *aml_address = *in_aml_address;
	ACPI_STATUS             status = AE_OK;
	u32                     index;
	NATIVE_CHAR             char_buf[5];


	/*
	 * If first character is a digit, then we know that we aren't looking at a
	 * valid name segment
	 */

	char_buf[0] = *aml_address;

	if ('0' <= char_buf[0] && char_buf[0] <= '9') {
		return (AE_CTRL_PENDING);
	}

	for (index = 4;
		(index > 0) && (acpi_cm_valid_acpi_character (*aml_address));
		--index) {
		char_buf[4 - index] = *aml_address++;
	}


	/* Valid name segment  */

	if (0 == index) {
		/* Found 4 valid characters */

		char_buf[4] = '\0';

		if (name_string) {
			STRCAT (name_string, char_buf);
		}

	}

	else if (4 == index) {
		/*
		 * First character was not a valid name character,
		 * so we are looking at something other than a name.
		 */
		status = AE_CTRL_PENDING;
	}

	else {
		/* Segment started with one or more valid characters, but fewer than 4 */

		status = AE_AML_BAD_NAME;
	}

	*in_aml_address = aml_address;

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_get_name_string
 *
 * PARAMETERS:  Data_type           - Data type to be associated with this name
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get a name, including any prefixes.
 *
 ******************************************************************************/


ACPI_STATUS
acpi_aml_get_name_string (
	OBJECT_TYPE_INTERNAL    data_type,
	u8                      *in_aml_address,
	NATIVE_CHAR             **out_name_string,
	u32                     *out_name_length)
{
	ACPI_STATUS             status = AE_OK;
	u8                      *aml_address = in_aml_address;
	NATIVE_CHAR             *name_string = NULL;
	u32                     num_segments;
	u32                     prefix_count = 0;
	u8                      prefix = 0;
	u8                      has_prefix = FALSE;


	if (INTERNAL_TYPE_DEF_FIELD == data_type  ||
		INTERNAL_TYPE_BANK_FIELD == data_type ||
		INTERNAL_TYPE_INDEX_FIELD == data_type) {
		/* Disallow prefixes for types associated with field names */

		name_string = acpi_aml_allocate_name_string (0, 1);
		if (!name_string) {
			status = AE_NO_MEMORY;
		}
		else {
			status = acpi_aml_exec_name_segment (&aml_address, name_string);
		}
	}

	else {
		/*
		 * Data_type is not a field name.
		 * Examine first character of name for root or parent prefix operators
		 */

		switch (*aml_address) {

		case AML_ROOT_PREFIX:

			prefix = *aml_address++;
			/*
			 * Remember that we have a Root_prefix --
			 * see comment in Acpi_aml_allocate_name_string()
			 */
			prefix_count = (u32) -1;
			has_prefix = TRUE;
			break;


		case AML_PARENT_PREFIX:

			/* Increment past possibly multiple parent prefixes */

			do {
				prefix = *aml_address++;
				++prefix_count;

			} while (*aml_address == AML_PARENT_PREFIX);
			has_prefix = TRUE;
			break;


		default:

			break;
		}


		/* Examine first character of name for name segment prefix operator */

		switch (*aml_address) {

		case AML_DUAL_NAME_PREFIX:

			prefix = *aml_address++;
			name_string = acpi_aml_allocate_name_string (prefix_count, 2);
			if (!name_string) {
				status = AE_NO_MEMORY;
				break;
			}

			/* Indicate that we processed a prefix */
			has_prefix = TRUE;

			status = acpi_aml_exec_name_segment (&aml_address, name_string);
			if (ACPI_SUCCESS (status)) {
				status = acpi_aml_exec_name_segment (&aml_address, name_string);
			}
			break;


		case AML_MULTI_NAME_PREFIX_OP:

			prefix = *aml_address++;
			/* Fetch count of segments remaining in name path */

			num_segments = *aml_address++;

			name_string = acpi_aml_allocate_name_string (prefix_count, num_segments);
			if (!name_string) {
				status = AE_NO_MEMORY;
				break;
			}

			/* Indicate that we processed a prefix */
			has_prefix = TRUE;

			while (num_segments &&
					(status = acpi_aml_exec_name_segment (&aml_address, name_string)) == AE_OK) {
				--num_segments;
			}

			break;


		case 0:

			/* Null_name valid as of 8-12-98 ASL/AML Grammar Update */


			/* Consume the NULL byte */

			aml_address++;
			name_string = acpi_aml_allocate_name_string (prefix_count, 0);
			if (!name_string) {
				status = AE_NO_MEMORY;
				break;
			}

			break;


		default:

			/* Name segment string */

			name_string = acpi_aml_allocate_name_string (prefix_count, 1);
			if (!name_string) {
				status = AE_NO_MEMORY;
				break;
			}

			status = acpi_aml_exec_name_segment (&aml_address, name_string);
			break;

		}   /* Switch (Peek_op ())   */
	}


	if (AE_CTRL_PENDING == status && has_prefix) {
		/* Ran out of segments after processing a prefix */

		REPORT_ERROR (
			("Aml_do_name: Malformed Name at %p\n", name_string));
		status = AE_AML_BAD_NAME;
	}


	*out_name_string = name_string;
	*out_name_length = (u32) (aml_address - in_aml_address);

	return (status);
}


