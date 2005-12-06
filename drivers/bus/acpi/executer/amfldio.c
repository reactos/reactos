/******************************************************************************
 *
 * Module Name: amfldio - Aml Field I/O
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
#include "achware.h"
#include "acevents.h"


#define _COMPONENT          ACPI_EXECUTER
	 MODULE_NAME         ("amfldio")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_read_field_data
 *
 * PARAMETERS:  *Obj_desc           - Field to be read
 *              *Value              - Where to store value
 *              Field_bit_width     - Field Width in bits (8, 16, or 32)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve the value of the given field
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_read_field_data (
	ACPI_OPERAND_OBJECT     *obj_desc,
	u32                     field_byte_offset,
	u32                     field_bit_width,
	u32                     *value)
{
	ACPI_STATUS             status;
	ACPI_OPERAND_OBJECT     *rgn_desc = NULL;
	ACPI_PHYSICAL_ADDRESS   address;
	u32                     local_value = 0;
	u32                     field_byte_width;


	/* Obj_desc is validated by callers */

	if (obj_desc) {
		rgn_desc = obj_desc->field.container;
	}


	field_byte_width = DIV_8 (field_bit_width);
	status = acpi_aml_setup_field (obj_desc, rgn_desc, field_bit_width);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Setup_field validated Rgn_desc and Field_bit_width */

	if (!value) {
		value = &local_value;   /*  support reads without saving value  */
	}


	/*
	 * Set offset to next multiple of field width,
	 *  add region base address and offset within the field
	 */
	address = rgn_desc->region.address +
			  (obj_desc->field.offset * field_byte_width) +
			  field_byte_offset;


	/* Invoke the appropriate Address_space/Op_region handler */

	status = acpi_ev_address_space_dispatch (rgn_desc, ADDRESS_SPACE_READ,
			 address, field_bit_width, value);



	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_read_field
 *
 * PARAMETERS:  *Obj_desc           - Field to be read
 *              *Value              - Where to store value
 *              Field_bit_width     - Field Width in bits (8, 16, or 32)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Retrieve the value of the given field
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_read_field (
	ACPI_OPERAND_OBJECT     *obj_desc,
	void                    *buffer,
	u32                     buffer_length,
	u32                     byte_length,
	u32                     datum_length,
	u32                     bit_granularity,
	u32                     byte_granularity)
{
	ACPI_STATUS             status;
	u32                     this_field_byte_offset;
	u32                     this_field_datum_offset;
	u32                     previous_raw_datum;
	u32                     this_raw_datum = 0;
	u32                     valid_field_bits;
	u32                     mask;
	u32                     merged_datum = 0;


	/*
	 * Clear the caller's buffer (the whole buffer length as given)
	 * This is very important, especially in the cases where a byte is read,
	 * but the buffer is really a u32 (4 bytes).
	 */

	MEMSET (buffer, 0, buffer_length);

	/* Read the first raw datum to prime the loop */

	this_field_byte_offset = 0;
	this_field_datum_offset= 0;

	status = acpi_aml_read_field_data (obj_desc, this_field_byte_offset, bit_granularity,
			   &previous_raw_datum);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}

	/* We might actually be done if the request fits in one datum */

	if ((datum_length == 1) &&
		((obj_desc->field.bit_offset + obj_desc->field_unit.length) <=
			(u16) bit_granularity)) {
		merged_datum = previous_raw_datum;

		merged_datum = (merged_datum >> obj_desc->field.bit_offset);

		valid_field_bits = obj_desc->field_unit.length % bit_granularity;
		if (valid_field_bits) {
			mask = (((u32) 1 << valid_field_bits) - (u32) 1);
			merged_datum &= mask;
		}


		/*
		 * Place the Merged_datum into the proper format and return buffer
		 * field
		 */

		switch (byte_granularity) {
		case 1:
			((u8 *) buffer) [this_field_datum_offset] = (u8) merged_datum;
			break;

		case 2:
			MOVE_UNALIGNED16_TO_16 (&(((u16 *) buffer)[this_field_datum_offset]), &merged_datum);
			break;

		case 4:
			MOVE_UNALIGNED32_TO_32 (&(((u32 *) buffer)[this_field_datum_offset]), &merged_datum);
			break;
		}

		this_field_byte_offset = 1;
		this_field_datum_offset = 1;
	}

	else {
		/* We need to get more raw data to complete one or more field data */

		while (this_field_datum_offset < datum_length) {
			/*
			 * If the field is aligned on a byte boundary, we don't want
			 * to perform a final read, since this would potentially read
			 * past the end of the region.
			 *
			 * TBD: [Investigate] It may make more sense to just split the aligned
			 * and non-aligned cases since the aligned case is so very simple,
			 */
			if ((obj_desc->field.bit_offset != 0) ||
				((obj_desc->field.bit_offset == 0) &&
				(this_field_datum_offset < (datum_length -1)))) {
				/*
				 * Get the next raw datum, it contains some or all bits
				 * of the current field datum
				 */

				status = acpi_aml_read_field_data (obj_desc,
						  this_field_byte_offset + byte_granularity,
						  bit_granularity, &this_raw_datum);
				if (ACPI_FAILURE (status)) {
					goto cleanup;
				}

				/* Before merging the data, make sure the unused bits are clear */

				switch (byte_granularity) {
				case 1:
					this_raw_datum &= 0x000000FF;
					previous_raw_datum &= 0x000000FF;
					break;

				case 2:
					this_raw_datum &= 0x0000FFFF;
					previous_raw_datum &= 0x0000FFFF;
					break;
				}
			}


			/*
			 * Put together bits of the two raw data to make a complete
			 * field datum
			 */


			if (obj_desc->field.bit_offset != 0) {
				merged_datum =
					(previous_raw_datum >> obj_desc->field.bit_offset) |
					(this_raw_datum << (bit_granularity - obj_desc->field.bit_offset));
			}

			else {
				merged_datum = previous_raw_datum;
			}

			/*
			 * Prepare the merged datum for storing into the caller's
			 *  buffer.  It is possible to have a 32-bit buffer
			 *  (Byte_granularity == 4), but a Obj_desc->Field.Length
			 *  of 8 or 16, meaning that the upper bytes of merged data
			 *  are undesired.  This section fixes that.
			 */
			switch (obj_desc->field.length) {
			case 8:
				merged_datum &= 0x000000FF;
				break;

			case 16:
				merged_datum &= 0x0000FFFF;
				break;
			}

			/*
			 * Now store the datum in the caller's buffer, according to
			 * the data type
			 */
			switch (byte_granularity) {
			case 1:
				((u8 *) buffer) [this_field_datum_offset] = (u8) merged_datum;
				break;

			case 2:
				MOVE_UNALIGNED16_TO_16 (&(((u16 *) buffer) [this_field_datum_offset]), &merged_datum);
				break;

			case 4:
				MOVE_UNALIGNED32_TO_32 (&(((u32 *) buffer) [this_field_datum_offset]), &merged_datum);
				break;
			}

			/*
			 * Save the most recent datum since it contains bits of
			 * the *next* field datum
			 */

			previous_raw_datum = this_raw_datum;

			this_field_byte_offset += byte_granularity;
			this_field_datum_offset++;

		}  /* while */
	}

cleanup:

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_write_field_data
 *
 * PARAMETERS:  *Obj_desc           - Field to be set
 *              Value               - Value to store
 *              Field_bit_width     - Field Width in bits (8, 16, or 32)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store the value into the given field
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_aml_write_field_data (
	ACPI_OPERAND_OBJECT     *obj_desc,
	u32                     field_byte_offset,
	u32                     field_bit_width,
	u32                     value)
{
	ACPI_STATUS             status = AE_OK;
	ACPI_OPERAND_OBJECT     *rgn_desc = NULL;
	ACPI_PHYSICAL_ADDRESS   address;
	u32                     field_byte_width;


	/* Obj_desc is validated by callers */

	if (obj_desc) {
		rgn_desc = obj_desc->field.container;
	}

	field_byte_width = DIV_8 (field_bit_width);
	status = acpi_aml_setup_field (obj_desc, rgn_desc, field_bit_width);
	if (ACPI_FAILURE (status)) {
		return (status);
	}


	/*
	 * Set offset to next multiple of field width,
	 *  add region base address and offset within the field
	 */
	address = rgn_desc->region.address +
			  (obj_desc->field.offset * field_byte_width) +
			  field_byte_offset;

	/* Invoke the appropriate Address_space/Op_region handler */

	status = acpi_ev_address_space_dispatch (rgn_desc, ADDRESS_SPACE_WRITE,
			 address, field_bit_width, &value);



	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_write_field_data_with_update_rule
 *
 * PARAMETERS:  *Obj_desc           - Field to be set
 *              Value               - Value to store
 *              Field_bit_width     - Field Width in bits (8, 16, or 32)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Apply the field update rule to a field write
 *
 ****************************************************************************/

static ACPI_STATUS
acpi_aml_write_field_data_with_update_rule (
	ACPI_OPERAND_OBJECT     *obj_desc,
	u32                     mask,
	u32                     field_value,
	u32                     this_field_byte_offset,
	u32                     bit_granularity)
{
	ACPI_STATUS             status = AE_OK;
	u32                     merged_value;
	u32                     current_value;


	/* Start with the new bits  */

	merged_value = field_value;


	/* Decode the update rule */

	switch (obj_desc->field.update_rule) {

	case UPDATE_PRESERVE:

		/* Check if update rule needs to be applied (not if mask is all ones) */

		/* The left shift drops the bits we want to ignore. */
		if ((~mask << (sizeof(mask)*8 - bit_granularity)) != 0) {
			/*
			 * Read the current contents of the byte/word/dword containing
			 * the field, and merge with the new field value.
			 */
			status = acpi_aml_read_field_data (obj_desc, this_field_byte_offset,
					   bit_granularity, &current_value);
			merged_value |= (current_value & ~mask);
		}
		break;


	case UPDATE_WRITE_AS_ONES:

		/* Set positions outside the field to all ones */

		merged_value |= ~mask;
		break;


	case UPDATE_WRITE_AS_ZEROS:

		/* Set positions outside the field to all zeros */

		merged_value &= mask;
		break;


	default:
		status = AE_AML_OPERAND_VALUE;
	}


	/* Write the merged value */

	if (ACPI_SUCCESS (status)) {
		status = acpi_aml_write_field_data (obj_desc, this_field_byte_offset,
				   bit_granularity, merged_value);
	}

	return (status);
}


/*****************************************************************************
 *
 * FUNCTION:    Acpi_aml_write_field
 *
 * PARAMETERS:  *Obj_desc           - Field to be set
 *              Value               - Value to store
 *              Field_bit_width     - Field Width in bits (8, 16, or 32)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Store the value into the given field
 *
 ****************************************************************************/

ACPI_STATUS
acpi_aml_write_field (
	ACPI_OPERAND_OBJECT     *obj_desc,
	void                    *buffer,
	u32                     buffer_length,
	u32                     byte_length,
	u32                     datum_length,
	u32                     bit_granularity,
	u32                     byte_granularity)
{
	ACPI_STATUS             status;
	u32                     this_field_byte_offset;
	u32                     this_field_datum_offset;
	u32                     mask;
	u32                     merged_datum;
	u32                     previous_raw_datum;
	u32                     this_raw_datum;
	u32                     field_value;
	u32                     valid_field_bits;


	/*
	 * Break the request into up to three parts:
	 * non-aligned part at start, aligned part in middle, non-aligned part
	 * at end --- Just like an I/O request ---
	 */

	this_field_byte_offset = 0;
	this_field_datum_offset= 0;

	/* Get a datum */

	switch (byte_granularity) {
	case 1:
		previous_raw_datum = ((u8 *) buffer) [this_field_datum_offset];
		break;

	case 2:
		MOVE_UNALIGNED16_TO_32 (&previous_raw_datum, &(((u16 *) buffer) [this_field_datum_offset]));
		break;

	case 4:
		MOVE_UNALIGNED32_TO_32 (&previous_raw_datum, &(((u32 *) buffer) [this_field_datum_offset]));
		break;

	default:
		status = AE_AML_OPERAND_VALUE;
		goto cleanup;
	}


	/*
	 * Write a partial field datum if field does not begin on a datum boundary
	 *
	 * Construct Mask with 1 bits where the field is, 0 bits elsewhere
	 *
	 * 1) Bits above the field
	 */

	mask = (((u32)(-1)) << (u32)obj_desc->field.bit_offset);

	/* 2) Only the bottom 5 bits are valid for a shift operation. */

	if ((obj_desc->field.bit_offset + obj_desc->field_unit.length) < 32) {
		/* Bits above the field */

		mask &= (~(((u32)(-1)) << ((u32)obj_desc->field.bit_offset +
				   (u32)obj_desc->field_unit.length)));
	}

	/* 3) Shift and mask the value into the field position */

	field_value = (previous_raw_datum << obj_desc->field.bit_offset) & mask;

	status = acpi_aml_write_field_data_with_update_rule (obj_desc, mask, field_value,
			 this_field_byte_offset,
			 bit_granularity);
	if (ACPI_FAILURE (status)) {
		goto cleanup;
	}


	/* If the field fits within one datum, we are done. */

	if ((datum_length == 1) &&
	   ((obj_desc->field.bit_offset + obj_desc->field_unit.length) <=
			(u16) bit_granularity)) {
		goto cleanup;
	}

	/*
	 * We don't need to worry about the update rule for these data, because
	 * all of the bits are part of the field.
	 *
	 * Can't write the last datum, however, because it might contain bits that
	 * are not part of the field -- the update rule must be applied.
	 */

	while (this_field_datum_offset < (datum_length - 1)) {
		this_field_datum_offset++;

		/* Get the next raw datum, it contains bits of the current field datum... */

		switch (byte_granularity) {
		case 1:
			this_raw_datum = ((u8 *) buffer) [this_field_datum_offset];
			break;

		case 2:
			MOVE_UNALIGNED16_TO_32 (&this_raw_datum, &(((u16 *) buffer) [this_field_datum_offset]));
			break;

		case 4:
			MOVE_UNALIGNED32_TO_32 (&this_raw_datum, &(((u32 *) buffer) [this_field_datum_offset]));
			break;

		default:
			status = AE_AML_OPERAND_VALUE;
			goto cleanup;
		}

		/*
		 * Put together bits of the two raw data to make a complete field
		 * datum
		 */

		if (obj_desc->field.bit_offset != 0) {
			merged_datum =
				(previous_raw_datum >> (bit_granularity - obj_desc->field.bit_offset)) |
				(this_raw_datum << obj_desc->field.bit_offset);
		}

		else {
			merged_datum = this_raw_datum;
		}

		/* Now write the completed datum  */


		status = acpi_aml_write_field_data (obj_desc,
				   this_field_byte_offset + byte_granularity,
				   bit_granularity, merged_datum);
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}


		/*
		 * Save the most recent datum since it contains bits of
		 * the *next* field datum
		 */

		previous_raw_datum = this_raw_datum;

		this_field_byte_offset += byte_granularity;

	}  /* while */


	/* Write a partial field datum if field does not end on a datum boundary */

	if ((obj_desc->field_unit.length + obj_desc->field_unit.bit_offset) %
		bit_granularity) {
		switch (byte_granularity) {
		case 1:
			this_raw_datum = ((u8 *) buffer) [this_field_datum_offset];
			break;

		case 2:
			MOVE_UNALIGNED16_TO_32 (&this_raw_datum, &(((u16 *) buffer) [this_field_datum_offset]));
			break;

		case 4:
			MOVE_UNALIGNED32_TO_32 (&this_raw_datum, &(((u32 *) buffer) [this_field_datum_offset]));
			break;
		}

		/* Construct Mask with 1 bits where the field is, 0 bits elsewhere */

		valid_field_bits = ((obj_desc->field_unit.length % bit_granularity) +
				   obj_desc->field.bit_offset);

		mask = (((u32) 1 << valid_field_bits) - (u32) 1);

		/* Shift and mask the value into the field position */

		field_value = (previous_raw_datum >>
				  (bit_granularity - obj_desc->field.bit_offset)) & mask;

		status = acpi_aml_write_field_data_with_update_rule (obj_desc, mask, field_value,
				 this_field_byte_offset + byte_granularity,
				 bit_granularity);
		if (ACPI_FAILURE (status)) {
			goto cleanup;
		}
	}


cleanup:

	return (status);
}


