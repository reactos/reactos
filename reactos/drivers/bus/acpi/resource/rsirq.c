/*******************************************************************************
 *
 * Module Name: rsirq - Acpi_rs_irq_resource,
 *                      Acpi_rs_irq_stream
 *                      Acpi_rs_extended_irq_resource
 *                      Acpi_rs_extended_irq_stream
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
#include "acresrc.h"

#define _COMPONENT          ACPI_RESOURCES
	 MODULE_NAME         ("rsirq")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_irq_resource
 *
 * PARAMETERS:  Byte_stream_buffer      - Pointer to the resource input byte
 *                                          stream
 *              Bytes_consumed          - u32 pointer that is filled with
 *                                          the number of bytes consumed from
 *                                          the Byte_stream_buffer
 *              Output_buffer           - Pointer to the user's return buffer
 *              Structure_size          - u32 pointer that is filled with
 *                                          the number of bytes in the filled
 *                                          in structure
 *
 * RETURN:      Status  AE_OK if okay, else a valid ACPI_STATUS code
 *
 * DESCRIPTION: Take the resource byte stream and fill out the appropriate
 *                  structure pointed to by the Output_buffer. Return the
 *                  number of bytes consumed from the byte stream.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_irq_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size)
{
	u8                      *buffer = byte_stream_buffer;
	RESOURCE                *output_struct = (RESOURCE *) * output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;
	u8                      index;
	u8                      i;
	u32                     struct_size = sizeof (IRQ_RESOURCE) +
			  RESOURCE_LENGTH_NO_DATA;


	/*
	 * The number of bytes consumed are contained in the descriptor
	 *  (Bits:0-1)
	 */
	temp8 = *buffer;
	*bytes_consumed = (temp8 & 0x03) + 1;
	output_struct->id = irq;

	/*
	 * Point to the 16-bits of Bytes 1 and 2
	 */
	buffer += 1;
	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);

	output_struct->data.irq.number_of_interrupts = 0;

	/* Decode the IRQ bits */

	for (i = 0, index = 0; index < 16; index++) {
		if((temp16 >> index) & 0x01) {
			output_struct->data.irq.interrupts[i] = index;
			i++;
		}
	}
	output_struct->data.irq.number_of_interrupts = i;

	/*
	 * Calculate the structure size based upon the number of interrupts
	 */
	struct_size += (output_struct->data.irq.number_of_interrupts - 1) * 4;

	/*
	 * Point to Byte 3 if it is used
	 */
	if (4 == *bytes_consumed) {
		buffer += 2;
		temp8 = *buffer;

		/*
		 * Check for HE, LL or HL
		 */
		if (temp8 & 0x01) {
			output_struct->data.irq.edge_level = EDGE_SENSITIVE;
			output_struct->data.irq.active_high_low = ACTIVE_HIGH;
		}

		else {
			if (temp8 & 0x8) {
				output_struct->data.irq.edge_level = LEVEL_SENSITIVE;
				output_struct->data.irq.active_high_low = ACTIVE_LOW;
			}

			else {
				/*
				 * Only _LL and _HE polarity/trigger interrupts
				 *  are allowed (ACPI spec v1.0b ection 6.4.2.1),
				 *  so an error will occur if we reach this point
				 */
				return (AE_BAD_DATA);
			}
		}

		/*
		 * Check for sharable
		 */
		output_struct->data.irq.shared_exclusive = (temp8 >> 3) & 0x01;
	}

	else {
		/*
		 * Assume Edge Sensitive, Active High, Non-Sharable
		 *  per ACPI Specification
		 */
		output_struct->data.irq.edge_level = EDGE_SENSITIVE;
		output_struct->data.irq.active_high_low = ACTIVE_HIGH;
		output_struct->data.irq.shared_exclusive = EXCLUSIVE;
	}

	/*
	 * Set the Length parameter
	 */
	output_struct->length = struct_size;

	/*
	 * Return the final size of the structure
	 */
	*structure_size = struct_size;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_irq_stream
 *
 * PARAMETERS:  Linked_list             - Pointer to the resource linked list
 *              Output_buffer           - Pointer to the user's return buffer
 *              Bytes_consumed          - u32 pointer that is filled with
 *                                          the number of bytes of the
 *                                          Output_buffer used
 *
 * RETURN:      Status  AE_OK if okay, else a valid ACPI_STATUS code
 *
 * DESCRIPTION: Take the linked list resource structure and fills in the
 *                  the appropriate bytes in a byte stream
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_irq_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed)
{
	u8                      *buffer = *output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;
	u8                      index;
	u8                      IRQinfo_byte_needed;


	/*
	 * The descriptor field is set based upon whether a third byte is
	 *  needed to contain the IRQ Information.
	 */
	if (EDGE_SENSITIVE == linked_list->data.irq.edge_level &&
		ACTIVE_HIGH == linked_list->data.irq.active_high_low &&
		EXCLUSIVE == linked_list->data.irq.shared_exclusive) {
		*buffer = 0x22;
		IRQinfo_byte_needed = FALSE;
	}
	else {
		*buffer = 0x23;
		IRQinfo_byte_needed = TRUE;
	}

	buffer += 1;
	temp16 = 0;

	/*
	 * Loop through all of the interrupts and set the mask bits
	 */
	for(index = 0;
		index < linked_list->data.irq.number_of_interrupts;
		index++) {
		temp8 = (u8) linked_list->data.irq.interrupts[index];
		temp16 |= 0x1 << temp8;
	}

	MOVE_UNALIGNED16_TO_16 (buffer, &temp16);
	buffer += 2;

	/*
	 * Set the IRQ Info byte if needed.
	 */
	if (IRQinfo_byte_needed) {
		temp8 = 0;
		temp8 = (u8) ((linked_list->data.irq.shared_exclusive &
				 0x01) << 4);

		if (LEVEL_SENSITIVE == linked_list->data.irq.edge_level &&
			ACTIVE_LOW == linked_list->data.irq.active_high_low) {
			temp8 |= 0x08;
		}

		else {
			temp8 |= 0x01;
		}

		*buffer = temp8;
		buffer += 1;
	}

	/*
	 * Return the number of bytes consumed in this operation
	 */
	*bytes_consumed = (u32) ((NATIVE_UINT) buffer -
			   (NATIVE_UINT) *output_buffer);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_extended_irq_resource
 *
 * PARAMETERS:  Byte_stream_buffer      - Pointer to the resource input byte
 *                                          stream
 *              Bytes_consumed          - u32 pointer that is filled with
 *                                          the number of bytes consumed from
 *                                          the Byte_stream_buffer
 *              Output_buffer           - Pointer to the user's return buffer
 *              Structure_size          - u32 pointer that is filled with
 *                                          the number of bytes in the filled
 *                                          in structure
 *
 * RETURN:      Status  AE_OK if okay, else a valid ACPI_STATUS code
 *
 * DESCRIPTION: Take the resource byte stream and fill out the appropriate
 *                  structure pointed to by the Output_buffer. Return the
 *                  number of bytes consumed from the byte stream.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_extended_irq_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size)
{
	u8                      *buffer = byte_stream_buffer;
	RESOURCE                *output_struct = (RESOURCE *) * output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;
	u8                      index;
	u32                     struct_size = sizeof (EXTENDED_IRQ_RESOURCE) +
			  RESOURCE_LENGTH_NO_DATA;


	/*
	 * Point past the Descriptor to get the number of bytes consumed
	 */
	buffer += 1;
	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);

	*bytes_consumed = temp16 + 3;
	output_struct->id = extended_irq;

	/*
	 * Point to the Byte3
	 */
	buffer += 2;
	temp8 = *buffer;

	output_struct->data.extended_irq.producer_consumer = temp8 & 0x01;

	/*
	 * Check for HE, LL or HL
	 */
	if(temp8 & 0x02) {
		output_struct->data.extended_irq.edge_level = EDGE_SENSITIVE;
		output_struct->data.extended_irq.active_high_low = ACTIVE_HIGH;
	}

	else {
		if(temp8 & 0x4) {
			output_struct->data.extended_irq.edge_level = LEVEL_SENSITIVE;
			output_struct->data.extended_irq.active_high_low = ACTIVE_LOW;
		}

		else {
			/*
			 * Only _LL and _HE polarity/trigger interrupts
			 *  are allowed (ACPI spec v1.0b ection 6.4.2.1),
			 *  so an error will occur if we reach this point
			 */
			return (AE_BAD_DATA);
		}
	}

	/*
	 * Check for sharable
	 */
	output_struct->data.extended_irq.shared_exclusive =
			(temp8 >> 3) & 0x01;

	/*
	 * Point to Byte4 (IRQ Table length)
	 */
	buffer += 1;
	temp8 = *buffer;

	output_struct->data.extended_irq.number_of_interrupts = temp8;

	/*
	 * Add any additional structure size to properly calculate
	 *  the next pointer at the end of this function
	 */
	 struct_size += (temp8 - 1) * 4;

	/*
	 * Point to Byte5 (First IRQ Number)
	 */
	buffer += 1;

	/*
	 * Cycle through every IRQ in the table
	 */
	for (index = 0; index < temp8; index++) {
		output_struct->data.extended_irq.interrupts[index] =
				(u32)*buffer;

		/* Point to the next IRQ */

		buffer += 4;
	}

	/*
	 * This will leave us pointing to the Resource Source Index
	 *  If it is present, then save it off and calculate the
	 *  pointer to where the null terminated string goes:
	 *  Each Interrupt takes 32-bits + the 5 bytes of the
	 *  stream that are default.
	 */
	if (*bytes_consumed >
		(u32)(output_struct->data.extended_irq.number_of_interrupts *
		 4) + 5) {
		/* Dereference the Index */

		temp8 = *buffer;
		output_struct->data.extended_irq.resource_source_index =
				(u32)temp8;

		/* Point to the String */

		buffer += 1;

		/* Copy the string into the buffer */

		index = 0;

		while (0x00 != *buffer) {
			output_struct->data.extended_irq.resource_source[index] =
					*buffer;

			buffer += 1;
			index += 1;
		}

		/*
		 * Add the terminating null
		 */
		output_struct->data.extended_irq.resource_source[index] = 0x00;
		output_struct->data.extended_irq.resource_source_string_length =
				index + 1;

		/*
		 * In order for the Struct_size to fall on a 32-bit boundry,
		 *  calculate the length of the string and expand the
		 *  Struct_size to the next 32-bit boundry.
		 */
		temp8 = (u8) (index + 1);
		temp8 = (u8) ROUND_UP_TO_32_bITS (temp8);
	}

	else {
		output_struct->data.extended_irq.resource_source_index = 0x00;
		output_struct->data.extended_irq.resource_source_string_length = 0;
		output_struct->data.extended_irq.resource_source[0] = 0x00;
	}

	/*
	 * Set the Length parameter
	 */
	output_struct->length = struct_size;

	/*
	 * Return the final size of the structure
	 */
	*structure_size = struct_size;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_extended_irq_stream
 *
 * PARAMETERS:  Linked_list             - Pointer to the resource linked list
 *              Output_buffer           - Pointer to the user's return buffer
 *              Bytes_consumed          - u32 pointer that is filled with
 *                                          the number of bytes of the
 *                                          Output_buffer used
 *
 * RETURN:      Status  AE_OK if okay, else a valid ACPI_STATUS code
 *
 * DESCRIPTION: Take the linked list resource structure and fills in the
 *              the appropriate bytes in a byte stream
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_extended_irq_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed)
{
	u8                      *buffer = *output_buffer;
	u16                     *length_field;
	u8                      temp8 = 0;
	u8                      index;
	NATIVE_CHAR             *temp_pointer = NULL;


	/*
	 * The descriptor field is static
	 */
	*buffer = 0x89;
	buffer += 1;

	/*
	 * Set a pointer to the Length field - to be filled in later
	 */

	length_field = (u16 *)buffer;
	buffer += 2;

	/*
	 * Set the Interrupt vector flags
	 */
	temp8 = (u8)(linked_list->data.extended_irq.producer_consumer & 0x01);

	temp8 |= ((linked_list->data.extended_irq.shared_exclusive & 0x01) << 3);

	if (LEVEL_SENSITIVE == linked_list->data.extended_irq.edge_level &&
	   ACTIVE_LOW == linked_list->data.extended_irq.active_high_low) {
		temp8 |= 0x04;
	}
	else {
		temp8 |= 0x02;
	}

	*buffer = temp8;
	buffer += 1;

	/*
	 * Set the Interrupt table length
	 */
	temp8 = (u8) linked_list->data.extended_irq.number_of_interrupts;

	*buffer = temp8;
	buffer += 1;

	for (index = 0;
		 index < linked_list->data.extended_irq.number_of_interrupts;
		 index++) {
		MOVE_UNALIGNED32_TO_32 (buffer,
				  &linked_list->data.extended_irq.interrupts[index]);
		buffer += 4;
	}

	/*
	 * Resource Source Index and Resource Source are optional
	 */
	if (0 != linked_list->data.extended_irq.resource_source_string_length) {
		*buffer = (u8) linked_list->data.extended_irq.resource_source_index;
		buffer += 1;

		temp_pointer = (NATIVE_CHAR *) buffer;

		/*
		 * Copy the string
		 */
		STRCPY (temp_pointer, linked_list->data.extended_irq.resource_source);

		/*
		 * Buffer needs to be set to the length of the sting + one for the
		 *  terminating null
		 */
		buffer += (STRLEN (linked_list->data.extended_irq.resource_source) + 1);
	}

	/*
	 * Return the number of bytes consumed in this operation
	 */
	*bytes_consumed = (u32) ((NATIVE_UINT) buffer -
			   (NATIVE_UINT) *output_buffer);

	/*
	 * Set the length field to the number of bytes consumed
	 *  minus the header size (3 bytes)
	 */
	*length_field = (u16) (*bytes_consumed - 3);

	return (AE_OK);
}

