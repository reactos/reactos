/******************************************************************************
 *
 * Module Name: tbxfroot - Find the root ACPI table (RSDT)
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
#include "achware.h"
#include "actables.h"


#define _COMPONENT          ACPI_TABLES
	 MODULE_NAME         ("tbxfroot")

#define RSDP_CHECKSUM_LENGTH 20


/*******************************************************************************
 *
 * FUNCTION:    Acpi_find_root_pointer
 *
 * PARAMETERS:  **Rsdp_physical_address     - Where to place the RSDP address
 *
 * RETURN:      Status, Physical address of the RSDP
 *
 * DESCRIPTION: Find the RSDP
 *
 ******************************************************************************/

ACPI_STATUS
acpi_find_root_pointer (
	ACPI_PHYSICAL_ADDRESS   *rsdp_physical_address)
{
	ACPI_TABLE_DESC         table_info;
	ACPI_STATUS             status;


	/* Get the RSDP */

	status = acpi_tb_find_rsdp (&table_info);
	if (ACPI_FAILURE (status)) {
		return (AE_NO_ACPI_TABLES);
	}

	*rsdp_physical_address = table_info.physical_address;

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_tb_scan_memory_for_rsdp
 *
 * PARAMETERS:  Start_address       - Starting pointer for search
 *              Length              - Maximum length to search
 *
 * RETURN:      Pointer to the RSDP if found, otherwise NULL.
 *
 * DESCRIPTION: Search a block of memory for the RSDP signature
 *
 ******************************************************************************/

u8 *
acpi_tb_scan_memory_for_rsdp (
	u8                      *start_address,
	u32                     length)
{
	u32                     offset;
	u8                      *mem_rover;


	/* Search from given start addr for the requested length  */

	for (offset = 0, mem_rover = start_address;
		 offset < length;
		 offset += RSDP_SCAN_STEP, mem_rover += RSDP_SCAN_STEP) {

		/* The signature and checksum must both be correct */

		if (STRNCMP ((NATIVE_CHAR *) mem_rover,
				RSDP_SIG, sizeof (RSDP_SIG)-1) == 0 &&
			acpi_tb_checksum (mem_rover, RSDP_CHECKSUM_LENGTH) == 0) {
			/* If so, we have found the RSDP */

			return (mem_rover);
		}
	}

	/* Searched entire block, no RSDP was found */

	return (NULL);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_tb_find_rsdp
 *
 * PARAMETERS:  *Buffer_ptr             - If == NULL, read data from buffer
 *                                        rather than searching memory
 *              *Table_info             - Where the table info is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Search lower 1_mbyte of memory for the root system descriptor
 *              pointer structure.  If it is found, set *RSDP to point to it.
 *
 *              NOTE: The RSDP must be either in the first 1_k of the Extended
 *              BIOS Data Area or between E0000 and FFFFF (ACPI 1.0 section
 *              5.2.2; assertion #421).
 *
 ******************************************************************************/

ACPI_STATUS
acpi_tb_find_rsdp (
	ACPI_TABLE_DESC         *table_info)
{
	u8                      *table_ptr;
	u8                      *mem_rover;
	UINT64                  phys_addr;
	ACPI_STATUS             status = AE_OK;


	/*
	 * Search memory for RSDP.  First map low physical memory.
	 */

	status = acpi_os_map_memory (LO_RSDP_WINDOW_BASE, LO_RSDP_WINDOW_SIZE,
			  (void **)&table_ptr);

	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/*
	 * 1) Search EBDA (low memory) paragraphs
	 */

	mem_rover = acpi_tb_scan_memory_for_rsdp (table_ptr, LO_RSDP_WINDOW_SIZE);

	/* This mapping is no longer needed */

	acpi_os_unmap_memory (table_ptr, LO_RSDP_WINDOW_SIZE);

	if (mem_rover) {
		/* Found it, return the physical address */

		phys_addr = LO_RSDP_WINDOW_BASE;
		phys_addr += (mem_rover - table_ptr);

		table_info->physical_address = phys_addr;

		return (AE_OK);
	}


	/*
	 * 2) Search upper memory: 16-byte boundaries in E0000h-F0000h
	 */

	status = acpi_os_map_memory (HI_RSDP_WINDOW_BASE, HI_RSDP_WINDOW_SIZE,
			  (void **)&table_ptr);

	if (ACPI_FAILURE (status)) {
		return (status);
	}

	mem_rover = acpi_tb_scan_memory_for_rsdp (table_ptr, HI_RSDP_WINDOW_SIZE);

	/* This mapping is no longer needed */

	acpi_os_unmap_memory (table_ptr, HI_RSDP_WINDOW_SIZE);

	if (mem_rover) {
		/* Found it, return the physical address */

		phys_addr = HI_RSDP_WINDOW_BASE;
		phys_addr += (mem_rover - table_ptr);

		table_info->physical_address = phys_addr;

		return (AE_OK);
	}


	/* RSDP signature was not found */

	return (AE_NOT_FOUND);
}


