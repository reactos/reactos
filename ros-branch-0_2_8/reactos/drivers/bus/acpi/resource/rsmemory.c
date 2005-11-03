/*******************************************************************************
 *
 * Module Name: rsmem24 - Acpi_rs_memory24_resource
 *                        Acpi_rs_memory24_stream
 *                        Acpi_rs_memory32_range_resource
 *                        Acpi_rs_fixed_memory32_resource
 *                        Acpi_rs_memory32_range_stream
 *                        Acpi_rs_fixed_memory32_stream
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
	 MODULE_NAME         ("rsmemory")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_memory24_resource
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
acpi_rs_memory24_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size)
{
	u8                      *buffer = byte_stream_buffer;
	RESOURCE                *output_struct = (RESOURCE *) * output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;
	u32                     struct_size = sizeof (MEMORY24_RESOURCE) +
			  RESOURCE_LENGTH_NO_DATA;


	/*
	 * Point past the Descriptor to get the number of bytes consumed
	 */
	buffer += 1;

	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);
	buffer += 2;
	*bytes_consumed = temp16 + 3;
	output_struct->id = memory24;

	/*
	 * Check Byte 3 the Read/Write bit
	 */
	temp8 = *buffer;
	buffer += 1;
	output_struct->data.memory24.read_write_attribute = temp8 & 0x01;

	/*
	 * Get Min_base_address (Bytes 4-5)
	 */
	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);
	buffer += 2;
	output_struct->data.memory24.min_base_address = temp16;

	/*
	 * Get Max_base_address (Bytes 6-7)
	 */
	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);
	buffer += 2;
	output_struct->data.memory24.max_base_address = temp16;

	/*
	 * Get Alignment (Bytes 8-9)
	 */
	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);
	buffer += 2;
	output_struct->data.memory24.alignment = temp16;

	/*
	 * Get Range_length (Bytes 10-11)
	 */
	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);
	output_struct->data.memory24.range_length = temp16;

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
 * FUNCTION:    Acpi_rs_memory24_stream
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
acpi_rs_memory24_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed)
{
	u8                      *buffer = *output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;


	/*
	 * The descriptor field is static
	 */
	*buffer = 0x81;
	buffer += 1;

	/*
	 * The length field is static
	 */
	temp16 = 0x09;
	MOVE_UNALIGNED16_TO_16 (buffer, &temp16);
	buffer += 2;

	/*
	 * Set the Information Byte
	 */
	temp8 = (u8) (linked_list->data.memory24.read_write_attribute & 0x01);
	*buffer = temp8;
	buffer += 1;

	/*
	 * Set the Range minimum base address
	 */
	MOVE_UNALIGNED16_TO_16 (buffer, &linked_list->data.memory24.min_base_address);
	buffer += 2;

	/*
	 * Set the Range maximum base address
	 */
	MOVE_UNALIGNED16_TO_16 (buffer, &linked_list->data.memory24.max_base_address);
	buffer += 2;

	/*
	 * Set the base alignment
	 */
	MOVE_UNALIGNED16_TO_16 (buffer, &linked_list->data.memory24.alignment);
	buffer += 2;

	/*
	 * Set the range length
	 */
	MOVE_UNALIGNED16_TO_16 (buffer, &linked_list->data.memory24.range_length);
	buffer += 2;

	/*
	 * Return the number of bytes consumed in this operation
	 */
	*bytes_consumed = (u32) ((NATIVE_UINT) buffer -
			   (NATIVE_UINT) *output_buffer);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_memory32_range_resource
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
acpi_rs_memory32_range_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size)
{
	u8                      *buffer = byte_stream_buffer;
	RESOURCE                *output_struct = (RESOURCE *) * output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;
	u32                     struct_size = sizeof (MEMORY32_RESOURCE) +
			  RESOURCE_LENGTH_NO_DATA;


	/*
	 * Point past the Descriptor to get the number of bytes consumed
	 */
	buffer += 1;

	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);
	buffer += 2;
	*bytes_consumed = temp16 + 3;

	output_struct->id = memory32;

	/*
	 *  Point to the place in the output buffer where the data portion will
	 *    begin.
	 *  1. Set the RESOURCE_DATA * Data to point to it's own address, then
	 *  2. Set the pointer to the next address.
	 *
	 *  NOTE: Output_struct->Data is cast to u8, otherwise, this addition adds
	 *  4 * sizeof(RESOURCE_DATA) instead of 4 * sizeof(u8)
	 */

	/*
	 * Check Byte 3 the Read/Write bit
	 */
	temp8 = *buffer;
	buffer += 1;

	output_struct->data.memory32.read_write_attribute = temp8 & 0x01;

	/*
	 * Get Min_base_address (Bytes 4-7)
	 */
	MOVE_UNALIGNED32_TO_32 (&output_struct->data.memory32.min_base_address,
			 buffer);
	buffer += 4;

	/*
	 * Get Max_base_address (Bytes 8-11)
	 */
	MOVE_UNALIGNED32_TO_32 (&output_struct->data.memory32.max_base_address,
			 buffer);
	buffer += 4;

	/*
	 * Get Alignment (Bytes 12-15)
	 */
	MOVE_UNALIGNED32_TO_32 (&output_struct->data.memory32.alignment, buffer);
	buffer += 4;

	/*
	 * Get Range_length (Bytes 16-19)
	 */
	MOVE_UNALIGNED32_TO_32 (&output_struct->data.memory32.range_length, buffer);

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
 * FUNCTION:    Acpi_rs_fixed_memory32_resource
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
acpi_rs_fixed_memory32_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size)
{
	u8                      *buffer = byte_stream_buffer;
	RESOURCE                *output_struct = (RESOURCE *) * output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;
	u32                     struct_size = sizeof (FIXED_MEMORY32_RESOURCE) +
			  RESOURCE_LENGTH_NO_DATA;


	/*
	 * Point past the Descriptor to get the number of bytes consumed
	 */
	buffer += 1;
	MOVE_UNALIGNED16_TO_16 (&temp16, buffer);

	buffer += 2;
	*bytes_consumed = temp16 + 3;

	output_struct->id = fixed_memory32;

	/*
	 * Check Byte 3 the Read/Write bit
	 */
	temp8 = *buffer;
	buffer += 1;
	output_struct->data.fixed_memory32.read_write_attribute = temp8 & 0x01;

	/*
	 * Get Range_base_address (Bytes 4-7)
	 */
	MOVE_UNALIGNED32_TO_32 (&output_struct->data.fixed_memory32.range_base_address,
			 buffer);
	buffer += 4;

	/*
	 * Get Range_length (Bytes 8-11)
	 */
	MOVE_UNALIGNED32_TO_32 (&output_struct->data.fixed_memory32.range_length,
			 buffer);

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
 * FUNCTION:    Acpi_rs_memory32_range_stream
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
acpi_rs_memory32_range_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed)
{
	u8                      *buffer = *output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;


	/*
	 * The descriptor field is static
	 */
	*buffer = 0x85;
	buffer += 1;

	/*
	 * The length field is static
	 */
	temp16 = 0x11;

	MOVE_UNALIGNED16_TO_16 (buffer, &temp16);
	buffer += 2;

	/*
	 * Set the Information Byte
	 */
	temp8 = (u8) (linked_list->data.memory32.read_write_attribute & 0x01);
	*buffer = temp8;
	buffer += 1;

	/*
	 * Set the Range minimum base address
	 */
	MOVE_UNALIGNED32_TO_32 (buffer, &linked_list->data.memory32.min_base_address);
	buffer += 4;

	/*
	 * Set the Range maximum base address
	 */
	MOVE_UNALIGNED32_TO_32 (buffer, &linked_list->data.memory32.max_base_address);
	buffer += 4;

	/*
	 * Set the base alignment
	 */
	MOVE_UNALIGNED32_TO_32 (buffer, &linked_list->data.memory32.alignment);
	buffer += 4;

	/*
	 * Set the range length
	 */
	MOVE_UNALIGNED32_TO_32 (buffer, &linked_list->data.memory32.range_length);
	buffer += 4;

	/*
	 * Return the number of bytes consumed in this operation
	 */
	*bytes_consumed = (u32) ((NATIVE_UINT) buffer -
			   (NATIVE_UINT) *output_buffer);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_fixed_memory32_stream
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
acpi_rs_fixed_memory32_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed)
{
	u8                      *buffer = *output_buffer;
	u16                     temp16 = 0;
	u8                      temp8 = 0;


	/*
	 * The descriptor field is static
	 */
	*buffer = 0x86;
	buffer += 1;

	/*
	 * The length field is static
	 */
	temp16 = 0x09;

	MOVE_UNALIGNED16_TO_16 (buffer, &temp16);
	buffer += 2;

	/*
	 * Set the Information Byte
	 */
	temp8 = (u8) (linked_list->data.fixed_memory32.read_write_attribute & 0x01);
	*buffer = temp8;
	buffer += 1;

	/*
	 * Set the Range base address
	 */
	MOVE_UNALIGNED32_TO_32 (buffer,
			 &linked_list->data.fixed_memory32.range_base_address);
	buffer += 4;

	/*
	 * Set the range length
	 */
	MOVE_UNALIGNED32_TO_32 (buffer,
			 &linked_list->data.fixed_memory32.range_length);
	buffer += 4;

	/*
	 * Return the number of bytes consumed in this operation
	 */
	*bytes_consumed = (u32) ((NATIVE_UINT) buffer -
			   (NATIVE_UINT) *output_buffer);

	return (AE_OK);
}

