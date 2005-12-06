/*******************************************************************************
 *
 * Module Name: rslist - Acpi_rs_byte_stream_to_list
 *                       Acpi_list_to_byte_stream
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
	 MODULE_NAME         ("rslist")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_byte_stream_to_list
 *
 * PARAMETERS:  Byte_stream_buffer      - Pointer to the resource byte stream
 *              Byte_stream_buffer_length - Length of Byte_stream_buffer
 *              Output_buffer           - Pointer to the buffer that will
 *                                          contain the output structures
 *
 * RETURN:      Status  AE_OK if okay, else a valid ACPI_STATUS code
 *
 * DESCRIPTION: Takes the resource byte stream and parses it, creating a
 *              linked list of resources in the caller's output buffer
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_byte_stream_to_list (
	u8                      *byte_stream_buffer,
	u32                     byte_stream_buffer_length,
	u8                      **output_buffer)
{
	ACPI_STATUS             status;
	u32                     bytes_parsed = 0;
	u8                      resource_type = 0;
	u32                     bytes_consumed = 0;
	u8                      **buffer = output_buffer;
	u32                     structure_size = 0;
	u8                      end_tag_processed = FALSE;


	while (bytes_parsed < byte_stream_buffer_length &&
			FALSE == end_tag_processed) {
		/*
		 * Look at the next byte in the stream
		 */
		resource_type = *byte_stream_buffer;

		/*
		 * See if this is a small or large resource
		 */
		if(resource_type & 0x80) {
			/*
			 * Large Resource Type
			 */
			switch (resource_type) {
			case MEMORY_RANGE_24:
				/*
				 * 24-Bit Memory Resource
				 */
				status = acpi_rs_memory24_resource(byte_stream_buffer,
						   &bytes_consumed,
						   buffer,
						   &structure_size);

				break;

			case LARGE_VENDOR_DEFINED:
				/*
				 * Vendor Defined Resource
				 */
				status = acpi_rs_vendor_resource(byte_stream_buffer,
						 &bytes_consumed,
						 buffer,
						 &structure_size);

				break;

			case MEMORY_RANGE_32:
				/*
				 * 32-Bit Memory Range Resource
				 */
				status = acpi_rs_memory32_range_resource(byte_stream_buffer,
						  &bytes_consumed,
						  buffer,
						  &structure_size);

				break;

			case FIXED_MEMORY_RANGE_32:
				/*
				 * 32-Bit Fixed Memory Resource
				 */
				status = acpi_rs_fixed_memory32_resource(byte_stream_buffer,
						  &bytes_consumed,
						  buffer,
						  &structure_size);

				break;

			case DWORD_ADDRESS_SPACE:
				/*
				 * 32-Bit Address Resource
				 */
				status = acpi_rs_address32_resource(byte_stream_buffer,
						 &bytes_consumed,
						 buffer,
						 &structure_size);

				break;

			case WORD_ADDRESS_SPACE:
				/*
				 * 16-Bit Address Resource
				 */
				status = acpi_rs_address16_resource(byte_stream_buffer,
						 &bytes_consumed,
						 buffer,
						 &structure_size);

				break;

			case EXTENDED_IRQ:
				/*
				 * Extended IRQ
				 */
				status = acpi_rs_extended_irq_resource(byte_stream_buffer,
						   &bytes_consumed,
						   buffer,
						   &structure_size);

				break;

/* TBD: [Future] 64-bit not currently supported */
/*
			case 0x8A:
				break;
*/

			default:
				/*
				 * If we get here, everything is out of sync,
				 *  so exit with an error
				 */
				return (AE_AML_ERROR);
				break;
			}
		}

		else {
			/*
			 * Small Resource Type
			 *  Only bits 7:3 are valid
			 */
			resource_type >>= 3;

			switch(resource_type) {
			case IRQ_FORMAT:
				/*
				 * IRQ Resource
				 */
				status = acpi_rs_irq_resource(byte_stream_buffer,
						 &bytes_consumed,
						 buffer,
						 &structure_size);

				break;

			case DMA_FORMAT:
				/*
				 * DMA Resource
				 */
				status = acpi_rs_dma_resource(byte_stream_buffer,
						 &bytes_consumed,
						 buffer,
						 &structure_size);

				break;

			case START_DEPENDENT_TAG:
				/*
				 * Start Dependent Functions Resource
				 */
				status = acpi_rs_start_dependent_functions_resource(byte_stream_buffer,
						   &bytes_consumed,
						   buffer,
						   &structure_size);

				break;

			case END_DEPENDENT_TAG:
				/*
				 * End Dependent Functions Resource
				 */
				status = acpi_rs_end_dependent_functions_resource(byte_stream_buffer,
						 &bytes_consumed,
						 buffer,
						 &structure_size);

				break;

			case IO_PORT_DESCRIPTOR:
				/*
				 * IO Port Resource
				 */
				status = acpi_rs_io_resource(byte_stream_buffer,
						   &bytes_consumed,
						   buffer,
						   &structure_size);

				break;

			case FIXED_LOCATION_IO_DESCRIPTOR:
				/*
				 * Fixed IO Port Resource
				 */
				status = acpi_rs_fixed_io_resource(byte_stream_buffer,
						  &bytes_consumed,
						  buffer,
						  &structure_size);

				break;

			case SMALL_VENDOR_DEFINED:
				/*
				 * Vendor Specific Resource
				 */
				status = acpi_rs_vendor_resource(byte_stream_buffer,
						 &bytes_consumed,
						 buffer,
						 &structure_size);

				break;

			case END_TAG:
				/*
				 * End Tag
				 */
				status = acpi_rs_end_tag_resource(byte_stream_buffer,
						 &bytes_consumed,
						 buffer,
						 &structure_size);
				end_tag_processed = TRUE;

				break;

			default:
				/*
				 * If we get here, everything is out of sync,
				 *  so exit with an error
				 */
				return (AE_AML_ERROR);
				break;

			} /* switch */
		}  /* end else */

		/*
		 * Update the return value and counter
		 */
		bytes_parsed += bytes_consumed;

		/*
		 * Set the byte stream to point to the next resource
		 */
		byte_stream_buffer += bytes_consumed;

		/*
		 * Set the Buffer to the next structure
		 */
		*buffer += structure_size;

	} /*  end while */

	/*
	 * Check the reason for exiting the while loop
	 */
	if (TRUE != end_tag_processed) {
		return (AE_AML_ERROR);
	}

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_list_to_byte_stream
 *
 * PARAMETERS:  Linked_list             - Pointer to the resource linked list
 *              Byte_steam_size_needed  - Calculated size of the byte stream
 *                                          needed from calling
 *                                          Acpi_rs_calculate_byte_stream_length()
 *                                          The size of the Output_buffer is
 *                                          guaranteed to be >=
 *                                          Byte_stream_size_needed
 *              Output_buffer           - Pointer to the buffer that will
 *                                          contain the byte stream
 *
 * RETURN:      Status  AE_OK if okay, else a valid ACPI_STATUS code
 *
 * DESCRIPTION: Takes the resource linked list and parses it, creating a
 *              byte stream of resources in the caller's output buffer
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_list_to_byte_stream (
	RESOURCE                *linked_list,
	u32                     byte_stream_size_needed,
	u8                      **output_buffer)
{
	ACPI_STATUS             status;
	u8                      *buffer = *output_buffer;
	u32                     bytes_consumed = 0;
	u8                      done = FALSE;


	while (!done) {
		switch (linked_list->id) {
		case irq:
			/*
			 * IRQ Resource
			 */
			status = acpi_rs_irq_stream (linked_list,
					   &buffer,
					   &bytes_consumed);
			break;

		case dma:
			/*
			 * DMA Resource
			 */
			status = acpi_rs_dma_stream (linked_list,
					   &buffer,
					   &bytes_consumed);
			break;

		case start_dependent_functions:
			/*
			 * Start Dependent Functions Resource
			 */
			status = acpi_rs_start_dependent_functions_stream (linked_list,
					  &buffer,
					  &bytes_consumed);
			break;

		case end_dependent_functions:
			/*
			 * End Dependent Functions Resource
			 */
			status = acpi_rs_end_dependent_functions_stream (linked_list,
					   &buffer,
					   &bytes_consumed);
			break;

		case io:
			/*
			 * IO Port Resource
			 */
			status = acpi_rs_io_stream (linked_list,
					  &buffer,
					  &bytes_consumed);
			break;

		case fixed_io:
			/*
			 * Fixed IO Port Resource
			 */
			status = acpi_rs_fixed_io_stream (linked_list,
					 &buffer,
					 &bytes_consumed);
			break;

		case vendor_specific:
			/*
			 * Vendor Defined Resource
			 */
			status = acpi_rs_vendor_stream (linked_list,
					   &buffer,
					   &bytes_consumed);
			break;

		case end_tag:
			/*
			 * End Tag
			 */
			status = acpi_rs_end_tag_stream (linked_list,
					   &buffer,
					   &bytes_consumed);

			/*
			 * An End Tag indicates the end of the Resource Template
			 */
			done = TRUE;
			break;

		case memory24:
			/*
			 * 24-Bit Memory Resource
			 */
			status = acpi_rs_memory24_stream (linked_list,
					  &buffer,
					  &bytes_consumed);
			break;

		case memory32:
			/*
			 * 32-Bit Memory Range Resource
			 */
			status = acpi_rs_memory32_range_stream (linked_list,
					 &buffer,
					 &bytes_consumed);
			break;

		case fixed_memory32:
			/*
			 * 32-Bit Fixed Memory Resource
			 */
			status = acpi_rs_fixed_memory32_stream (linked_list,
					 &buffer,
					 &bytes_consumed);
			break;

		case address16:
			/*
			 * 16-Bit Address Descriptor Resource
			 */
			status = acpi_rs_address16_stream (linked_list,
					   &buffer,
					   &bytes_consumed);
			break;

		case address32:
			/*
			 * 32-Bit Address Descriptor Resource
			 */
			status = acpi_rs_address32_stream (linked_list,
					   &buffer,
					   &bytes_consumed);
			break;

		case extended_irq:
			/*
			 * Extended IRQ Resource
			 */
			status = acpi_rs_extended_irq_stream (linked_list,
					  &buffer,
					  &bytes_consumed);
			break;

		default:
			/*
			 * If we get here, everything is out of sync,
			 *  so exit with an error
			 */
			return (AE_BAD_DATA);
			break;

		} /* switch (Linked_list->Id) */

		/*
		 * Set the Buffer to point to the open byte
		 */
		buffer += bytes_consumed;

		/*
		 * Point to the next object
		 */
		linked_list = (RESOURCE *) ((NATIVE_UINT) linked_list +
				  (NATIVE_UINT) linked_list->length);
	}

	return  (AE_OK);
}

