/*******************************************************************************
 *
 * Module Name: rsdump - Functions do dump out the resource structures.
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
	 MODULE_NAME         ("rsdump")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_irq
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_irq (
	RESOURCE_DATA           *data)
{
	IRQ_RESOURCE            *irq_data = (IRQ_RESOURCE*) data;
	u8                      index = 0;


	acpi_os_printf ("\t_iRQ Resource\n");

	acpi_os_printf ("\t\t%s Triggered\n",
			 LEVEL_SENSITIVE == irq_data->edge_level ?
			 "Level" : "Edge");

	acpi_os_printf ("\t\t_active %s\n",
			 ACTIVE_LOW == irq_data->active_high_low ?
			 "Low" : "High");

	acpi_os_printf ("\t\t%s\n",
			 SHARED == irq_data->shared_exclusive ?
			 "Shared" : "Exclusive");

	acpi_os_printf ("\t\t%X Interrupts ( ",
			 irq_data->number_of_interrupts);

	for (index = 0; index < irq_data->number_of_interrupts; index++) {
		acpi_os_printf ("%X ", irq_data->interrupts[index]);
	}

	acpi_os_printf (")\n");
	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_dma
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_dma (
	RESOURCE_DATA           *data)
{
	DMA_RESOURCE            *dma_data = (DMA_RESOURCE*) data;
	u8                      index = 0;


	acpi_os_printf ("\t_dMA Resource\n");

	switch (dma_data->type) {
	case COMPATIBILITY:
		acpi_os_printf ("\t\t_compatibility mode\n");
		break;

	case TYPE_A:
		acpi_os_printf ("\t\t_type A\n");
		break;

	case TYPE_B:
		acpi_os_printf ("\t\t_type B\n");
		break;

	case TYPE_F:
		acpi_os_printf ("\t\t_type F\n");
		break;

	default:
		acpi_os_printf ("\t\t_invalid DMA type\n");
		break;
	}

	acpi_os_printf ("\t\t%sBus Master\n",
			 BUS_MASTER == dma_data->bus_master ?
			 "" : "Not a ");

	switch (dma_data->transfer) {
	case TRANSFER_8:
		acpi_os_printf ("\t\t8-bit only transfer\n");
		break;

	case TRANSFER_8_16:
		acpi_os_printf ("\t\t8 and 16-bit transfer\n");
		break;

	case TRANSFER_16:
		acpi_os_printf ("\t\t16 bit only transfer\n");
		break;

	default:
		acpi_os_printf ("\t\t_invalid transfer preference\n");
		break;
	}

	acpi_os_printf ("\t\t_number of Channels: %X ( ",
			 dma_data->number_of_channels);

	for (index = 0; index < dma_data->number_of_channels; index++) {
		acpi_os_printf ("%X ", dma_data->channels[index]);
	}

	acpi_os_printf (")\n");
	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_start_dependent_functions
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_start_dependent_functions (
	RESOURCE_DATA               *data)
{
	START_DEPENDENT_FUNCTIONS_RESOURCE *sdf_data =
			   (START_DEPENDENT_FUNCTIONS_RESOURCE*) data;


	acpi_os_printf ("\t_start Dependent Functions Resource\n");

	switch (sdf_data->compatibility_priority) {
	case GOOD_CONFIGURATION:
		acpi_os_printf ("\t\t_good configuration\n");
		break;

	case ACCEPTABLE_CONFIGURATION:
		acpi_os_printf ("\t\t_acceptable configuration\n");
		break;

	case SUB_OPTIMAL_CONFIGURATION:
		acpi_os_printf ("\t\t_sub-optimal configuration\n");
		break;

	default:
		acpi_os_printf ("\t\t_invalid compatibility priority\n");
		break;
	}

	switch(sdf_data->performance_robustness) {
	case GOOD_CONFIGURATION:
		acpi_os_printf ("\t\t_good configuration\n");
		break;

	case ACCEPTABLE_CONFIGURATION:
		acpi_os_printf ("\t\t_acceptable configuration\n");
		break;

	case SUB_OPTIMAL_CONFIGURATION:
		acpi_os_printf ("\t\t_sub-optimal configuration\n");
		break;

	default:
		acpi_os_printf ("\t\t_invalid performance "
				  "robustness preference\n");
		break;
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_io
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_io (
	RESOURCE_DATA       *data)
{
	IO_RESOURCE         *io_data = (IO_RESOURCE*) data;


	acpi_os_printf ("\t_io Resource\n");

	acpi_os_printf ("\t\t%d bit decode\n",
			 DECODE_16 == io_data->io_decode ? 16 : 10);

	acpi_os_printf ("\t\t_range minimum base: %08X\n",
			 io_data->min_base_address);

	acpi_os_printf ("\t\t_range maximum base: %08X\n",
			 io_data->max_base_address);

	acpi_os_printf ("\t\t_alignment: %08X\n",
			 io_data->alignment);

	acpi_os_printf ("\t\t_range Length: %08X\n",
			 io_data->range_length);

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_fixed_io
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_fixed_io (
	RESOURCE_DATA           *data)
{
	FIXED_IO_RESOURCE       *fixed_io_data = (FIXED_IO_RESOURCE*) data;


	acpi_os_printf ("\t_fixed Io Resource\n");
	acpi_os_printf ("\t\t_range base address: %08X",
			 fixed_io_data->base_address);

	acpi_os_printf ("\t\t_range length: %08X",
			 fixed_io_data->range_length);

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_vendor_specific
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_vendor_specific (
	RESOURCE_DATA           *data)
{
	VENDOR_RESOURCE         *vendor_data = (VENDOR_RESOURCE*) data;
	u16                     index = 0;


	acpi_os_printf ("\t_vendor Specific Resource\n");

	acpi_os_printf ("\t\t_length: %08X\n", vendor_data->length);

	for (index = 0; index < vendor_data->length; index++) {
		acpi_os_printf ("\t\t_byte %X: %08X\n",
				 index, vendor_data->reserved[index]);
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_memory24
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_memory24 (
	RESOURCE_DATA           *data)
{
	MEMORY24_RESOURCE       *memory24_data = (MEMORY24_RESOURCE*) data;


	acpi_os_printf ("\t24-Bit Memory Range Resource\n");

	acpi_os_printf ("\t\t_read%s\n",
			 READ_WRITE_MEMORY ==
			 memory24_data->read_write_attribute ?
			 "/Write" : " only");

	acpi_os_printf ("\t\t_range minimum base: %08X\n",
			 memory24_data->min_base_address);

	acpi_os_printf ("\t\t_range maximum base: %08X\n",
			 memory24_data->max_base_address);

	acpi_os_printf ("\t\t_alignment: %08X\n",
			 memory24_data->alignment);

	acpi_os_printf ("\t\t_range length: %08X\n",
			 memory24_data->range_length);

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_memory32
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_memory32 (
	RESOURCE_DATA           *data)
{
	MEMORY32_RESOURCE       *memory32_data = (MEMORY32_RESOURCE*) data;


	acpi_os_printf ("\t32-Bit Memory Range Resource\n");

	acpi_os_printf ("\t\t_read%s\n",
			 READ_WRITE_MEMORY ==
			 memory32_data->read_write_attribute ?
			 "/Write" : " only");

	acpi_os_printf ("\t\t_range minimum base: %08X\n",
			 memory32_data->min_base_address);

	acpi_os_printf ("\t\t_range maximum base: %08X\n",
			 memory32_data->max_base_address);

	acpi_os_printf ("\t\t_alignment: %08X\n",
			 memory32_data->alignment);

	acpi_os_printf ("\t\t_range length: %08X\n",
			 memory32_data->range_length);

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_fixed_memory32
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_fixed_memory32 (
	RESOURCE_DATA           *data)
{
	FIXED_MEMORY32_RESOURCE *fixed_memory32_data = (FIXED_MEMORY32_RESOURCE*) data;


	acpi_os_printf ("\t32-Bit Fixed Location Memory Range Resource\n");

	acpi_os_printf ("\t\t_read%s\n",
			 READ_WRITE_MEMORY ==
			 fixed_memory32_data->read_write_attribute ?
			 "/Write" : " Only");

	acpi_os_printf ("\t\t_range base address: %08X\n",
			 fixed_memory32_data->range_base_address);

	acpi_os_printf ("\t\t_range length: %08X\n",
			 fixed_memory32_data->range_length);

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_address16
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_address16 (
	RESOURCE_DATA           *data)
{
	ADDRESS16_RESOURCE      *address16_data = (ADDRESS16_RESOURCE*) data;


	acpi_os_printf ("\t16-Bit Address Space Resource\n");
	acpi_os_printf ("\t\t_resource Type: ");

	switch (address16_data->resource_type) {
	case MEMORY_RANGE:

		acpi_os_printf ("Memory Range\n");

		switch (address16_data->attribute.memory.cache_attribute) {
		case NON_CACHEABLE_MEMORY:
			acpi_os_printf ("\t\t_type Specific: "
					  "Noncacheable memory\n");
			break;

		case CACHABLE_MEMORY:
			acpi_os_printf ("\t\t_type Specific: "
					  "Cacheable memory\n");
			break;

		case WRITE_COMBINING_MEMORY:
			acpi_os_printf ("\t\t_type Specific: "
					  "Write-combining memory\n");
			break;

		case PREFETCHABLE_MEMORY:
			acpi_os_printf ("\t\t_type Specific: "
					  "Prefetchable memory\n");
			break;

		default:
			acpi_os_printf ("\t\t_type Specific: "
					  "Invalid cache attribute\n");
			break;
		}

		acpi_os_printf ("\t\t_type Specific: Read%s\n",
			READ_WRITE_MEMORY ==
			address16_data->attribute.memory.read_write_attribute ?
			"/Write" : " Only");
		break;

	case IO_RANGE:

		acpi_os_printf ("I/O Range\n");

		switch (address16_data->attribute.io.range_attribute) {
		case NON_ISA_ONLY_RANGES:
			acpi_os_printf ("\t\t_type Specific: "
					  "Non-ISA Io Addresses\n");
			break;

		case ISA_ONLY_RANGES:
			acpi_os_printf ("\t\t_type Specific: "
					  "ISA Io Addresses\n");
			break;

		case ENTIRE_RANGE:
			acpi_os_printf ("\t\t_type Specific: "
					  "ISA and non-ISA Io Addresses\n");
			break;

		default:
			acpi_os_printf ("\t\t_type Specific: "
					  "Invalid range attribute\n");
			break;
		}
		break;

	case BUS_NUMBER_RANGE:

		acpi_os_printf ("Bus Number Range\n");
		break;

	default:

		acpi_os_printf ("Invalid resource type. Exiting.\n");
		return;
	}

	acpi_os_printf ("\t\t_resource %s\n",
			CONSUMER == address16_data->producer_consumer ?
			"Consumer" : "Producer");

	acpi_os_printf ("\t\t%s decode\n",
			 SUB_DECODE == address16_data->decode ?
			 "Subtractive" : "Positive");

	acpi_os_printf ("\t\t_min address is %s fixed\n",
			 ADDRESS_FIXED == address16_data->min_address_fixed ?
			 "" : "not");

	acpi_os_printf ("\t\t_max address is %s fixed\n",
			 ADDRESS_FIXED == address16_data->max_address_fixed ?
			 "" : "not");

	acpi_os_printf ("\t\t_granularity: %08X\n",
			 address16_data->granularity);

	acpi_os_printf ("\t\t_address range min: %08X\n",
			 address16_data->min_address_range);

	acpi_os_printf ("\t\t_address range max: %08X\n",
			 address16_data->max_address_range);

	acpi_os_printf ("\t\t_address translation offset: %08X\n",
			 address16_data->address_translation_offset);

	acpi_os_printf ("\t\t_address Length: %08X\n",
			 address16_data->address_length);

	if (0xFF != address16_data->resource_source_index) {
		acpi_os_printf ("\t\t_resource Source Index: %X\n",
				 address16_data->resource_source_index);
		acpi_os_printf ("\t\t_resource Source: %s\n",
				 address16_data->resource_source);
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_address32
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_address32 (
	RESOURCE_DATA           *data)
{
	ADDRESS32_RESOURCE      *address32_data = (ADDRESS32_RESOURCE*) data;


	acpi_os_printf ("\t32-Bit Address Space Resource\n");

	switch (address32_data->resource_type) {
	case MEMORY_RANGE:

		acpi_os_printf ("\t\t_resource Type: Memory Range\n");

		switch (address32_data->attribute.memory.cache_attribute) {
		case NON_CACHEABLE_MEMORY:
			acpi_os_printf ("\t\t_type Specific: "
					  "Noncacheable memory\n");
			break;

		case CACHABLE_MEMORY:
			acpi_os_printf ("\t\t_type Specific: "
					  "Cacheable memory\n");
			break;

		case WRITE_COMBINING_MEMORY:
			acpi_os_printf ("\t\t_type Specific: "
					  "Write-combining memory\n");
			break;

		case PREFETCHABLE_MEMORY:
			acpi_os_printf ("\t\t_type Specific: "
					  "Prefetchable memory\n");
			break;

		default:
			acpi_os_printf ("\t\t_type Specific: "
					  "Invalid cache attribute\n");
			break;
		}

		acpi_os_printf ("\t\t_type Specific: Read%s\n",
			READ_WRITE_MEMORY ==
			address32_data->attribute.memory.read_write_attribute ?
			"/Write" : " Only");
		break;

	case IO_RANGE:

		acpi_os_printf ("\t\t_resource Type: Io Range\n");

		switch (address32_data->attribute.io.range_attribute) {
			case NON_ISA_ONLY_RANGES:
				acpi_os_printf ("\t\t_type Specific: "
						  "Non-ISA Io Addresses\n");
				break;

			case ISA_ONLY_RANGES:
				acpi_os_printf ("\t\t_type Specific: "
						  "ISA Io Addresses\n");
				break;

			case ENTIRE_RANGE:
				acpi_os_printf ("\t\t_type Specific: "
						  "ISA and non-ISA Io Addresses\n");
				break;

			default:
				acpi_os_printf ("\t\t_type Specific: "
						  "Invalid Range attribute");
				break;
			}
		break;

	case BUS_NUMBER_RANGE:

		acpi_os_printf ("\t\t_resource Type: Bus Number Range\n");
		break;

	default:

		acpi_os_printf ("\t\t_invalid Resource Type..exiting.\n");
		return;
	}

	acpi_os_printf ("\t\t_resource %s\n",
			 CONSUMER == address32_data->producer_consumer ?
			 "Consumer" : "Producer");

	acpi_os_printf ("\t\t%s decode\n",
			 SUB_DECODE == address32_data->decode ?
			 "Subtractive" : "Positive");

	acpi_os_printf ("\t\t_min address is %s fixed\n",
			 ADDRESS_FIXED == address32_data->min_address_fixed ?
			 "" : "not ");

	acpi_os_printf ("\t\t_max address is %s fixed\n",
			 ADDRESS_FIXED == address32_data->max_address_fixed ?
			 "" : "not ");

	acpi_os_printf ("\t\t_granularity: %08X\n",
			 address32_data->granularity);

	acpi_os_printf ("\t\t_address range min: %08X\n",
			 address32_data->min_address_range);

	acpi_os_printf ("\t\t_address range max: %08X\n",
			 address32_data->max_address_range);

	acpi_os_printf ("\t\t_address translation offset: %08X\n",
			 address32_data->address_translation_offset);

	acpi_os_printf ("\t\t_address Length: %08X\n",
			 address32_data->address_length);

	if(0xFF != address32_data->resource_source_index) {
		acpi_os_printf ("\t\t_resource Source Index: %X\n",
				 address32_data->resource_source_index);
		acpi_os_printf ("\t\t_resource Source: %s\n",
				 address32_data->resource_source);
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_extended_irq
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Prints out the various members of the Data structure type.
 *
 ******************************************************************************/

void
acpi_rs_dump_extended_irq (
	RESOURCE_DATA           *data)
{
	EXTENDED_IRQ_RESOURCE   *ext_irq_data = (EXTENDED_IRQ_RESOURCE*) data;
	u8                      index = 0;


	acpi_os_printf ("\t_extended IRQ Resource\n");

	acpi_os_printf ("\t\t_resource %s\n",
			 CONSUMER == ext_irq_data->producer_consumer ?
			 "Consumer" : "Producer");

	acpi_os_printf ("\t\t%s\n",
			 LEVEL_SENSITIVE == ext_irq_data->edge_level ?
			 "Level" : "Edge");

	acpi_os_printf ("\t\t_active %s\n",
			 ACTIVE_LOW == ext_irq_data->active_high_low ?
			 "low" : "high");

	acpi_os_printf ("\t\t%s\n",
			 SHARED == ext_irq_data->shared_exclusive ?
			 "Shared" : "Exclusive");

	acpi_os_printf ("\t\t_interrupts : %X ( ",
			 ext_irq_data->number_of_interrupts);

	for (index = 0; index < ext_irq_data->number_of_interrupts; index++) {
		acpi_os_printf ("%X ", ext_irq_data->interrupts[index]);
	}

	acpi_os_printf (")\n");

	if(0xFF != ext_irq_data->resource_source_index) {
		acpi_os_printf ("\t\t_resource Source Index: %X",
				 ext_irq_data->resource_source_index);
		acpi_os_printf ("\t\t_resource Source: %s",
				 ext_irq_data->resource_source);
	}

	return;
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_resource_list
 *
 * PARAMETERS:  Data            - pointer to the resource structure to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Dispatches the structure to the correct dump routine.
 *
 ******************************************************************************/

void
acpi_rs_dump_resource_list (
	RESOURCE            *resource)
{
	u8                  count = 0;
	u8                  done = FALSE;


	if (acpi_dbg_level & TRACE_RESOURCES && _COMPONENT & acpi_dbg_layer) {
		while (!done) {
			acpi_os_printf ("\t_resource structure %x.\n", count++);

			switch (resource->id) {
			case irq:
				acpi_rs_dump_irq (&resource->data);
				break;

			case dma:
				acpi_rs_dump_dma (&resource->data);
				break;

			case start_dependent_functions:
				acpi_rs_dump_start_dependent_functions (&resource->data);
				break;

			case end_dependent_functions:
				acpi_os_printf ("\t_end_dependent_functions Resource\n");
				/* Acpi_rs_dump_end_dependent_functions (Resource->Data);*/
				break;

			case io:
				acpi_rs_dump_io (&resource->data);
				break;

			case fixed_io:
				acpi_rs_dump_fixed_io (&resource->data);
				break;

			case vendor_specific:
				acpi_rs_dump_vendor_specific (&resource->data);
				break;

			case end_tag:
				/*Rs_dump_end_tag (Resource->Data);*/
				acpi_os_printf ("\t_end_tag Resource\n");
				done = TRUE;
				break;

			case memory24:
				acpi_rs_dump_memory24 (&resource->data);
				break;

			case memory32:
				acpi_rs_dump_memory32 (&resource->data);
				break;

			case fixed_memory32:
				acpi_rs_dump_fixed_memory32 (&resource->data);
				break;

			case address16:
				acpi_rs_dump_address16 (&resource->data);
				break;

			case address32:
				acpi_rs_dump_address32 (&resource->data);
				break;

			case extended_irq:
				acpi_rs_dump_extended_irq (&resource->data);
				break;

			default:
				acpi_os_printf ("Invalid resource type\n");
				break;

			}

			resource = (RESOURCE *) ((NATIVE_UINT) resource +
					 (NATIVE_UINT) resource->length);
		}
	}

	return;
}

/*******************************************************************************
 *
 * FUNCTION:    Acpi_rs_dump_irq_list
 *
 * PARAMETERS:  Data            - pointer to the routing table to dump.
 *
 * RETURN:
 *
 * DESCRIPTION: Dispatches the structures to the correct dump routine.
 *
 ******************************************************************************/

void
acpi_rs_dump_irq_list (
	u8                  *route_table)
{
	u8                  *buffer = route_table;
	u8                  count = 0;
	u8                  done = FALSE;
	PCI_ROUTING_TABLE   *prt_element;


	if (acpi_dbg_level & TRACE_RESOURCES && _COMPONENT & acpi_dbg_layer) {
		prt_element = (PCI_ROUTING_TABLE *) buffer;

		while (!done) {
			acpi_os_printf ("\t_pCI IRQ Routing Table structure %X.\n", count++);

			acpi_os_printf ("\t\t_address: %X\n",
					 prt_element->address);

			acpi_os_printf ("\t\t_pin: %X\n", prt_element->pin);

			acpi_os_printf ("\t\t_source: %s\n", prt_element->source);

			acpi_os_printf ("\t\t_source_index: %X\n",
					 prt_element->source_index);

			buffer += prt_element->length;

			prt_element = (PCI_ROUTING_TABLE *) buffer;

			if(0 == prt_element->length) {
				done = TRUE;
			}
		}
	}

	return;
}

