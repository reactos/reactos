/******************************************************************************
 *
 * Name: actables.h - ACPI table management
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

#ifndef __ACTABLES_H__
#define __ACTABLES_H__


/* Used in Acpi_tb_map_acpi_table for size parameter if table header is to be used */

#define SIZE_IN_HEADER          0


ACPI_STATUS
acpi_tb_handle_to_object (
	u16                     table_id,
	ACPI_TABLE_DESC         **table_desc);

/*
 * tbconvrt - Table conversion routines
 */

ACPI_STATUS
acpi_tb_convert_to_xsdt (
	ACPI_TABLE_DESC         *table_info,
	u32                     *number_of_tables);

ACPI_STATUS
acpi_tb_convert_table_fadt (
	void);

ACPI_STATUS
acpi_tb_build_common_facs (
	ACPI_TABLE_DESC         *table_info);


/*
 * tbget - Table "get" routines
 */

ACPI_STATUS
acpi_tb_get_table_ptr (
	ACPI_TABLE_TYPE         table_type,
	u32                     instance,
	ACPI_TABLE_HEADER       **table_ptr_loc);

ACPI_STATUS
acpi_tb_get_table (
	ACPI_PHYSICAL_ADDRESS   physical_address,
	ACPI_TABLE_HEADER       *buffer_ptr,
	ACPI_TABLE_DESC         *table_info);

ACPI_STATUS
acpi_tb_verify_rsdp (
	ACPI_PHYSICAL_ADDRESS   RSDP_physical_address);

ACPI_STATUS
acpi_tb_get_table_facs (
	ACPI_TABLE_HEADER       *buffer_ptr,
	ACPI_TABLE_DESC         *table_info);


/*
 * tbgetall - Get all firmware ACPI tables
 */

ACPI_STATUS
acpi_tb_get_all_tables (
	u32                     number_of_tables,
	ACPI_TABLE_HEADER       *buffer_ptr);


/*
 * tbinstall - Table installation
 */

ACPI_STATUS
acpi_tb_install_table (
	ACPI_TABLE_HEADER       *table_ptr,
	ACPI_TABLE_DESC         *table_info);

ACPI_STATUS
acpi_tb_recognize_table (
	ACPI_TABLE_HEADER       *table_ptr,
	ACPI_TABLE_DESC         *table_info);

ACPI_STATUS
acpi_tb_init_table_descriptor (
	ACPI_TABLE_TYPE         table_type,
	ACPI_TABLE_DESC         *table_info);


/*
 * tbremove - Table removal and deletion
 */

void
acpi_tb_delete_acpi_tables (
	void);

void
acpi_tb_delete_acpi_table (
	ACPI_TABLE_TYPE         type);

void
acpi_tb_delete_single_table (
	ACPI_TABLE_DESC         *table_desc);

ACPI_TABLE_DESC *
acpi_tb_uninstall_table (
	ACPI_TABLE_DESC         *table_desc);

void
acpi_tb_free_acpi_tables_of_type (
	ACPI_TABLE_DESC         *table_info);


/*
 * tbrsd - RSDP, RSDT utilities
 */

ACPI_STATUS
acpi_tb_get_table_rsdt (
	u32                     *number_of_tables);

u8 *
acpi_tb_scan_memory_for_rsdp (
	u8                      *start_address,
	u32                     length);

ACPI_STATUS
acpi_tb_find_rsdp (
	ACPI_TABLE_DESC         *table_info);


/*
 * tbutils - common table utilities
 */

u8
acpi_tb_system_table_pointer (
	void                    *where);

ACPI_STATUS
acpi_tb_map_acpi_table (
	ACPI_PHYSICAL_ADDRESS   physical_address,
	u32                     *size,
	void                    **logical_address);

ACPI_STATUS
acpi_tb_verify_table_checksum (
	ACPI_TABLE_HEADER       *table_header);

u8
acpi_tb_checksum (
	void                    *buffer,
	u32                     length);

ACPI_STATUS
acpi_tb_validate_table_header (
	ACPI_TABLE_HEADER       *table_header);


#endif /* __ACTABLES_H__ */
