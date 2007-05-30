/******************************************************************************
 *
 * Name: acresrc.h - Resource Manager function prototypes
 *       $Revision: 1.1 $
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

#ifndef __ACRESRC_H__
#define __ACRESRC_H__


/*
 *  Function prototypes called from Acpi* APIs
 */

ACPI_STATUS
acpi_rs_get_prt_method_data (
	ACPI_HANDLE             handle,
	ACPI_BUFFER             *ret_buffer);


ACPI_STATUS
acpi_rs_get_crs_method_data (
	ACPI_HANDLE             handle,
	ACPI_BUFFER             *ret_buffer);

ACPI_STATUS
acpi_rs_get_prs_method_data (
	ACPI_HANDLE             handle,
	ACPI_BUFFER             *ret_buffer);

ACPI_STATUS
acpi_rs_set_srs_method_data (
	ACPI_HANDLE             handle,
	ACPI_BUFFER             *ret_buffer);

ACPI_STATUS
acpi_rs_create_resource_list (
	ACPI_OPERAND_OBJECT     *byte_stream_buffer,
	u8                      *output_buffer,
	u32                     *output_buffer_length);

ACPI_STATUS
acpi_rs_create_byte_stream (
	RESOURCE                *linked_list_buffer,
	u8                      *output_buffer,
	u32                     *output_buffer_length);

ACPI_STATUS
acpi_rs_create_pci_routing_table (
	ACPI_OPERAND_OBJECT     *method_return_object,
	u8                      *output_buffer,
	u32                     *output_buffer_length);


/*
 *Function prototypes called from Acpi_rs_create*APIs
 */

void
acpi_rs_dump_resource_list (
	RESOURCE                *resource);

void
acpi_rs_dump_irq_list (
	u8                      *route_table);

ACPI_STATUS
acpi_rs_get_byte_stream_start (
	u8                      *byte_stream_buffer,
	u8                      **byte_stream_start,
	u32                     *size);

ACPI_STATUS
acpi_rs_calculate_list_length (
	u8                      *byte_stream_buffer,
	u32                     byte_stream_buffer_length,
	u32                     *size_needed);

ACPI_STATUS
acpi_rs_calculate_byte_stream_length (
	RESOURCE                *linked_list_buffer,
	u32                     *size_needed);

ACPI_STATUS
acpi_rs_calculate_pci_routing_table_length (
	ACPI_OPERAND_OBJECT     *package_object,
	u32                     *buffer_size_needed);

ACPI_STATUS
acpi_rs_byte_stream_to_list (
	u8                      *byte_stream_buffer,
	u32                     byte_stream_buffer_length,
	u8                      **output_buffer);

ACPI_STATUS
acpi_rs_list_to_byte_stream (
	RESOURCE                *linked_list,
	u32                     byte_stream_size_needed,
	u8                      **output_buffer);

ACPI_STATUS
acpi_rs_io_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_fixed_io_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_io_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_fixed_io_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_irq_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_irq_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_dma_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_dma_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_address16_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_address16_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_address32_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_address32_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_start_dependent_functions_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_end_dependent_functions_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_start_dependent_functions_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_end_dependent_functions_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_memory24_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_memory24_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_memory32_range_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size
);

ACPI_STATUS
acpi_rs_fixed_memory32_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_memory32_range_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_fixed_memory32_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_extended_irq_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_extended_irq_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_end_tag_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_end_tag_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);

ACPI_STATUS
acpi_rs_vendor_resource (
	u8                      *byte_stream_buffer,
	u32                     *bytes_consumed,
	u8                      **output_buffer,
	u32                     *structure_size);

ACPI_STATUS
acpi_rs_vendor_stream (
	RESOURCE                *linked_list,
	u8                      **output_buffer,
	u32                     *bytes_consumed);


#endif  /* __ACRESRC_H__ */
