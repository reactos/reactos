/*******************************************************************************
 *
 * Module Name: rscalc - Acpi_rs_calculate_byte_stream_length
 *                       Acpi_rs_calculate_list_length
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
#include "amlcode.h"
#include "acnamesp.h"

#define _COMPONENT          ACPI_RESOURCES
	 MODULE_NAME         ("rscalc")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_calculate_byte_stream_length
 *
 * PARAMETERS:  Linked_list         - Pointer to the resource linked list
 *              Size_needed         - u32 pointer of the size buffer needed
 *                                      to properly return the parsed data
 *
 * RETURN:      Status  AE_OK if okay, else a valid ACPI_STATUS code
 *
 * DESCRIPTION: Takes the resource byte stream and parses it once, calculating
 *              the size buffer needed to hold the linked list that conveys
 *              the resource data.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_calculate_byte_stream_length (
	RESOURCE                *linked_list,
	u32                     *size_needed)
{
	u32                     byte_stream_size_needed = 0;
	u32                     segment_size;
	EXTENDED_IRQ_RESOURCE   *ex_irq = NULL;
	u8                      done = FALSE;


	while (!done) {

		/*
		 * Init the variable that will hold the size to add to the
		 *  total.
		 */
		segment_size = 0;

		switch (linked_list->id) {
		case irq:
			/*
			 * IRQ Resource
			 */
			/*
			 * For an IRQ Resource, Byte 3, although optional, will
			 *  always be created - it holds IRQ information.
			 */
			segment_size = 4;
			break;

		case dma:
			/*
			 * DMA Resource
			 */
			/*
			 * For this resource the size is static
			 */
			segment_size = 3;
			break;

		case start_dependent_functions:
			/*
			 * Start Dependent Functions Resource
			 */
			/*
			 * For a Start_dependent_functions Resource, Byte 1,
			 * although optional, will always be created.
			 */
			segment_size = 2;
			break;

		case end_dependent_functions:
			/*
			 * End Dependent Functions Resource
			 */
			/*
			 * For this resource the size is static
			 */
			segment_size = 1;
			break;

		case io:
			/*
			 * IO Port Resource
			 */
			/*
			 * For this resource the size is static
			 */
			segment_size = 8;
			break;

		case fixed_io:
			/*
			 * Fixed IO Port Resource
			 */
			/*
			 * For this resource the size is static
			 */
			segment_size = 4;
			break;

		case vendor_specific:
			/*
			 * Vendor Defined Resource
			 */
			/*
			 * For a Vendor Specific resource, if the Length is
			 *  between 1 and 7 it will be created as a Small
			 *  Resource data type, otherwise it is a Large
			 *  Resource data type.
			 */
			if(linked_list->data.vendor_specific.length > 7) {
				segment_size = 3;
			}
			else {
				segment_size = 1;
			}
			segment_size +=
				linked_list->data.vendor_specific.length;
			break;

		case end_tag:
			/*
			 * End Tag
			 */
			/*
			 * For this resource the size is static
			 */
			segment_size = 2;
			done = TRUE;
			break;

		case memory24:
			/*
			 * 24-Bit Memory Resource
			 */
			/*
			 * For this resource the size is static
			 */
			segment_size = 12;
			break;

		case memory32:
			/*
			 * 32-Bit Memory Range Resource
			 */
			/*
			 * For this resource the size is static
			 */
			segment_size = 20;
			break;

		case fixed_memory32:
			/*
			 * 32-Bit Fixed Memory Resource
			 */
			/*
			 * For this resource the size is static
			 */
			segment_size = 12;
			break;

		case address16:
			/*
			 * 16-Bit Address Resource
			 */
			/*
			 * The base size of this byte stream is 16. If a
			 *  Resource Source string is not NULL, add 1 for
			 *  the Index + the length of the null terminated
			 *  string Resource Source + 1 for the null.
			 */
			segment_size = 16;

			if(NULL != linked_list->data.address16.resource_source) {
				segment_size += (1 +
					linked_list->data.address16.resource_source_string_length);
			}
			break;

		case address32:
			/*
			 * 32-Bit Address Resource
			 */
			/*
			 * The base size of this byte stream is 26. If a Resource
			 *  Source string is not NULL, add 1 for the Index + the
			 *  length of the null terminated string Resource Source +
			 *  1 for the null.
			 */
			segment_size = 26;

			if(NULL != linked_list->data.address16.resource_source) {
				segment_size += (1 +
					linked_list->data.address16.resource_source_string_length);
			}
			break;

		case extended_irq:
			/*
			 * Extended IRQ Resource
			 */
			/*
			 * The base size of this byte stream is 9. This is for an
			 *  Interrupt table length of 1.  For each additional
			 *  interrupt, add 4.
			 * If a Resource Source string is not NULL, add 1 for the
			 *  Index + the length of the null terminated string
			 *  Resource Source + 1 for the null.
			 */
			segment_size = 9;

			segment_size +=
				(linked_list->data.extended_irq.number_of_interrupts -
				 1) * 4;

			if(NULL != ex_irq->resource_source) {
				segment_size += (1 +
					linked_list->data.extended_irq.resource_source_string_length);
			}
			break;

		default:
			/*
			 * If we get here, everything is out of sync,
			 *  so exit with an error
			 */
			return (AE_AML_ERROR);
			break;

		} /* switch (Linked_list->Id) */

		/*
		 * Update the total
		 */
		byte_stream_size_needed += segment_size;

		/*
		 * Point to the next object
		 */
		linked_list = (RESOURCE *) ((NATIVE_UINT) linked_list +
				  (NATIVE_UINT) linked_list->length);
	}

	/*
	 * This is the data the caller needs
	 */
	*size_needed = byte_stream_size_needed;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_calculate_list_length
 *
 * PARAMETERS:  Byte_stream_buffer      - Pointer to the resource byte stream
 *              Byte_stream_buffer_length - Size of Byte_stream_buffer
 *              Size_needed             - u32 pointer of the size buffer
 *                                          needed to properly return the
 *                                          parsed data
 *
 * RETURN:      Status  AE_OK if okay, else a valid ACPI_STATUS code
 *
 * DESCRIPTION: Takes the resource byte stream and parses it once, calculating
 *              the size buffer needed to hold the linked list that conveys
 *              the resource data.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_calculate_list_length (
	u8                      *byte_stream_buffer,
	u32                     byte_stream_buffer_length,
	u32                     *size_needed)
{
	u32                     buffer_size = 0;
	u32                     bytes_parsed = 0;
	u8                      number_of_interrupts = 0;
	u8                      number_of_channels = 0;
	u8                      resource_type;
	u32                     structure_size;
	u32                     bytes_consumed;
	u8                      *buffer;
	u8                      temp8;
	u16                     temp16;
	u8                      index;
	u8                      additional_bytes;


	while (bytes_parsed < byte_stream_buffer_length) {
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
				bytes_consumed = 12;

				structure_size = sizeof (MEMORY24_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA;
				break;

			case LARGE_VENDOR_DEFINED:
				/*
				 * Vendor Defined Resource
				 */
				buffer = byte_stream_buffer;
				++buffer;

				MOVE_UNALIGNED16_TO_16 (&temp16, buffer);
				bytes_consumed = temp16 + 3;

				/*
				 * Ensure a 32-bit boundary for the structure
				 */
				temp16 = (u16) ROUND_UP_TO_32_bITS (temp16);

				structure_size = sizeof (VENDOR_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA +
						  (temp16 * sizeof (u8));
				break;

			case MEMORY_RANGE_32:
				/*
				 * 32-Bit Memory Range Resource
				 */

				bytes_consumed = 20;

				structure_size = sizeof (MEMORY32_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA;
				break;

			case FIXED_MEMORY_RANGE_32:
				/*
				 * 32-Bit Fixed Memory Resource
				 */
				bytes_consumed = 12;

				structure_size = sizeof(FIXED_MEMORY32_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA;
				break;

			case DWORD_ADDRESS_SPACE:
				/*
				 * 32-Bit Address Resource
				 */
				buffer = byte_stream_buffer;

				++buffer;
				MOVE_UNALIGNED16_TO_16 (&temp16, buffer);

				bytes_consumed = temp16 + 3;

				/*
				 * Resource Source Index and Resource Source are
				 *  optional elements.  Check the length of the
				 *  Bytestream.  If it is greater than 23, that
				 *  means that an Index exists and is followed by
				 *  a null termininated string.  Therefore, set
				 *  the temp variable to the length minus the minimum
				 *  byte stream length plus the byte for the Index to
				 *  determine the size of the NULL terminiated string.
				 */
				if (23 < temp16) {
					temp8 = (u8) (temp16 - 24);
				}
				else {
					temp8 = 0;
				}

				/*
				 * Ensure a 32-bit boundary for the structure
				 */
				temp8 = (u8) ROUND_UP_TO_32_bITS (temp8);

				structure_size = sizeof (ADDRESS32_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA +
						  (temp8 * sizeof (u8));
				break;

			case WORD_ADDRESS_SPACE:
				/*
				 * 16-Bit Address Resource
				 */
				buffer = byte_stream_buffer;

				++buffer;
				MOVE_UNALIGNED16_TO_16 (&temp16, buffer);

				bytes_consumed = temp16 + 3;

				/*
				 * Resource Source Index and Resource Source are
				 *  optional elements.  Check the length of the
				 *  Bytestream.  If it is greater than 13, that
				 *  means that an Index exists and is followed by
				 *  a null termininated string.  Therefore, set
				 *  the temp variable to the length minus the minimum
				 *  byte stream length plus the byte for the Index to
				 *  determine the size of the NULL terminiated string.
				 */
				if (13 < temp16) {
					temp8 = (u8) (temp16 - 14);
				}
				else {
					temp8 = 0;
				}

				/*
				 * Ensure a 32-bit boundry for the structure
				 */
				temp8 = (u8) ROUND_UP_TO_32_bITS (temp8);

				structure_size = sizeof (ADDRESS16_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA +
						  (temp8 * sizeof (u8));
				break;

			case EXTENDED_IRQ:
				/*
				 * Extended IRQ
				 */
				buffer = byte_stream_buffer;

				++buffer;
				MOVE_UNALIGNED16_TO_16 (&temp16, buffer);

				bytes_consumed = temp16 + 3;

				/*
				 * Point past the length field and the
				 *  Interrupt vector flags to save off the
				 *  Interrupt table length to the Temp8 variable.
				 */
				buffer += 3;
				temp8 = *buffer;

				/*
				 * To compensate for multiple interrupt numbers,
				 *  Add 4 bytes for each additional interrupts
				 *  greater than 1
				 */
				additional_bytes = (u8) ((temp8 - 1) * 4);

				/*
				 * Resource Source Index and Resource Source are
				 *  optional elements.  Check the length of the
				 *  Bytestream.  If it is greater than 9, that
				 *  means that an Index exists and is followed by
				 *  a null termininated string.  Therefore, set
				 *  the temp variable to the length minus the minimum
				 *  byte stream length plus the byte for the Index to
				 *  determine the size of the NULL terminiated string.
				 */
				if (9 + additional_bytes < temp16) {
					temp8 = (u8) (temp16 - (9 + additional_bytes));
				}

				else {
					temp8 = 0;
				}

				/*
				 * Ensure a 32-bit boundry for the structure
				 */
				temp8 = (u8) ROUND_UP_TO_32_bITS (temp8);

				structure_size = sizeof (EXTENDED_IRQ_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA +
						  (additional_bytes * sizeof (u8)) +
						  (temp8 * sizeof (u8));

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

			switch (resource_type) {
			case IRQ_FORMAT:
				/*
				 * IRQ Resource
				 */
				/*
				 * Determine if it there are two or three
				 *  trailing bytes
				 */
				buffer = byte_stream_buffer;
				temp8 = *buffer;

				if(temp8 & 0x01) {
					bytes_consumed = 4;
				}

				else {
					bytes_consumed = 3;
				}

				/*
				 * Point past the descriptor
				 */
				++buffer;

				/*
				 * Look at the number of bits set
				 */
				MOVE_UNALIGNED16_TO_16 (&temp16, buffer);

				for (index = 0; index < 16; index++) {
					if (temp16 & 0x1) {
						++number_of_interrupts;
					}

					temp16 >>= 1;
				}

				structure_size = sizeof (IO_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA +
						  (number_of_interrupts * sizeof (u32));
				break;


			case DMA_FORMAT:

				/*
				 * DMA Resource
				 */
				buffer = byte_stream_buffer;

				bytes_consumed = 3;

				/*
				 * Point past the descriptor
				 */
				++buffer;

				/*
				 * Look at the number of bits set
				 */
				temp8 = *buffer;

				for(index = 0; index < 8; index++) {
					if(temp8 & 0x1) {
						++number_of_channels;
					}

					temp8 >>= 1;
				}

				structure_size = sizeof (DMA_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA +
						  (number_of_channels * sizeof (u32));
				break;


			case START_DEPENDENT_TAG:

				/*
				 * Start Dependent Functions Resource
				 */
				/*
				 * Determine if it there are two or three trailing bytes
				 */
				buffer = byte_stream_buffer;
				temp8 = *buffer;

				if(temp8 & 0x01) {
					bytes_consumed = 2;
				}
				else {
					bytes_consumed = 1;
				}


				structure_size =
						sizeof (START_DEPENDENT_FUNCTIONS_RESOURCE) +
						RESOURCE_LENGTH_NO_DATA;
				break;


			case END_DEPENDENT_TAG:

				/*
				 * End Dependent Functions Resource
				 */
				bytes_consumed = 1;
				structure_size = RESOURCE_LENGTH;
				break;


			case IO_PORT_DESCRIPTOR:
				/*
				 * IO Port Resource
				 */
				bytes_consumed = 8;
				structure_size = sizeof (IO_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA;
				break;


			case FIXED_LOCATION_IO_DESCRIPTOR:

				/*
				 * Fixed IO Port Resource
				 */
				bytes_consumed = 4;
				structure_size = sizeof (FIXED_IO_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA;
				break;


			case SMALL_VENDOR_DEFINED:

				/*
				 * Vendor Specific Resource
				 */
				buffer = byte_stream_buffer;

				temp8 = *buffer;
				temp8 = (u8) (temp8 & 0x7);
				bytes_consumed = temp8 + 1;

				/*
				 * Ensure a 32-bit boundry for the structure
				 */
				temp8 = (u8) ROUND_UP_TO_32_bITS (temp8);
				structure_size = sizeof (VENDOR_RESOURCE) +
						  RESOURCE_LENGTH_NO_DATA +
						  (temp8 * sizeof (u8));
				break;


			case END_TAG:

				/*
				 * End Tag
				 */
				bytes_consumed = 2;
				structure_size = RESOURCE_LENGTH;
				byte_stream_buffer_length = bytes_parsed;
				break;


			default:
				/*
				 * If we get here, everything is out of sync,
				 *  so exit with an error
				 */
				return (AE_AML_ERROR);
				break;

			} /* switch */

		}  /* if(Resource_type & 0x80) */

		/*
		 * Update the return value and counter
		 */
		buffer_size += structure_size;
		bytes_parsed += bytes_consumed;

		/*
		 * Set the byte stream to point to the next resource
		 */
		byte_stream_buffer += bytes_consumed;

	}

	/*
	 * This is the data the caller needs
	 */
	*size_needed = buffer_size;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_calculate_pci_routing_table_length
 *
 * PARAMETERS:  Package_object          - Pointer to the package object
 *              Buffer_size_needed      - u32 pointer of the size buffer
 *                                          needed to properly return the
 *                                          parsed data
 *
 * RETURN:      Status  AE_OK
 *
 * DESCRIPTION: Given a package representing a PCI routing table, this
 *                calculates the size of the corresponding linked list of
 *                descriptions.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_rs_calculate_pci_routing_table_length (
	ACPI_OPERAND_OBJECT     *package_object,
	u32                     *buffer_size_needed)
{
	u32                     number_of_elements;
	u32                     temp_size_needed = 0;
	ACPI_OPERAND_OBJECT     **top_object_list;
	u32                     index;
	ACPI_OPERAND_OBJECT     *package_element;
	ACPI_OPERAND_OBJECT     **sub_object_list;
	u8                      name_found;
	u32                     table_index;


	number_of_elements = package_object->package.count;

	/*
	 * Calculate the size of the return buffer.
	 * The base size is the number of elements * the sizes of the
	 * structures.  Additional space for the strings is added below.
	 * The minus one is to subtract the size of the u8 Source[1]
	 * member because it is added below.
	 */

	/*
	 * But each PRT_ENTRY structure has a pointer to a string and
	 * the size of that string must be found.
	 */
	top_object_list = package_object->package.elements;

	for (index = 0; index < number_of_elements; index++) {
		/*
		 * Dereference the sub-package
		 */
		package_element = *top_object_list;

		/*
		 * The Sub_object_list will now point to an array of the
		 * four IRQ elements: Address, Pin, Source and Source_index
		 */
		sub_object_list = package_element->package.elements;

		/*
		 * Scan the Irq_table_elements for the Source Name String
		 */
		name_found = FALSE;

		for (table_index = 0; table_index < 4 && !name_found; table_index++) {
			if ((ACPI_TYPE_STRING == (*sub_object_list)->common.type) ||
				((INTERNAL_TYPE_REFERENCE == (*sub_object_list)->common.type) &&
					((*sub_object_list)->reference.opcode == AML_NAMEPATH_OP))) {
				name_found = TRUE;
			}

			else {
				/*
				 * Look at the next element
				 */
				sub_object_list++;
			}
		}

		temp_size_needed += (sizeof (PCI_ROUTING_TABLE) - 4);

		/*
		 * Was a String type found?
		 */
		if (TRUE == name_found) {
			if (ACPI_TYPE_STRING == (*sub_object_list)->common.type) {
				/*
				 * The length String.Length field includes the
				 * terminating NULL
				 */
				temp_size_needed += (*sub_object_list)->string.length;
			}
			else {
				temp_size_needed += acpi_ns_get_pathname_length ((*sub_object_list)->reference.node);
			}
		}

		else {
			/*
			 * If no name was found, then this is a NULL, which is
			 *  translated as a u32 zero.
			 */
			temp_size_needed += sizeof(u32);
		}


		/* Round up the size since each element must be aligned */

		temp_size_needed = ROUND_UP_TO_64_bITS (temp_size_needed);

		/*
		 * Point to the next ACPI_OPERAND_OBJECT
		 */
		top_object_list++;
	}


	/*
	 * Adding an extra element to the end of the list, essentially a NULL terminator
	 */
	*buffer_size_needed = temp_size_needed + sizeof (PCI_ROUTING_TABLE);

	return (AE_OK);
}
