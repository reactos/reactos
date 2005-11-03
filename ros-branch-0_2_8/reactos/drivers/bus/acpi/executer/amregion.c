
/******************************************************************************
 *
 * Module Name: amregion - ACPI default Op_region (address space) handlers
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
	 MODULE_NAME         ("amregion")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_memory_space_handler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              Bit_width           - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              Handler_context     - Pointer to Handler's context
 *              Region_context      - Pointer to context specific to the
 *                                      accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the System Memory address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_system_memory_space_handler (
	u32                     function,
	ACPI_PHYSICAL_ADDRESS   address,
	u32                     bit_width,
	u32                     *value,
	void                    *handler_context,
	void                    *region_context)
{
	ACPI_STATUS             status = AE_OK;
	void                    *logical_addr_ptr = NULL;
	MEM_HANDLER_CONTEXT     *mem_info = region_context;
	u32                     length;


	/* Validate and translate the bit width */

	switch (bit_width) {
	case 8:
		length = 1;
		break;

	case 16:
		length = 2;
		break;

	case 32:
		length = 4;
		break;

	default:
		return (AE_AML_OPERAND_VALUE);
		break;
	}


	/*
	 * Does the request fit into the cached memory mapping?
	 * Is 1) Address below the current mapping? OR
	 *    2) Address beyond the current mapping?
	 */

	if ((address < mem_info->mapped_physical_address) ||
		(((ACPI_INTEGER) address + length) >
			((ACPI_INTEGER) mem_info->mapped_physical_address + mem_info->mapped_length))) {
		/*
		 * The request cannot be resolved by the current memory mapping;
		 * Delete the existing mapping and create a new one.
		 */

		if (mem_info->mapped_length) {
			/* Valid mapping, delete it */

			acpi_os_unmap_memory (mem_info->mapped_logical_address,
					   mem_info->mapped_length);
		}

		mem_info->mapped_length = 0; /* In case of failure below */

		/* Create a new mapping starting at the address given */

		status = acpi_os_map_memory (address, SYSMEM_REGION_WINDOW_SIZE,
				  (void **) &mem_info->mapped_logical_address);
		if (ACPI_FAILURE (status)) {
			return (status);
		}

		/* TBD: should these pointers go to 64-bit in all cases ? */

		mem_info->mapped_physical_address = address;
		mem_info->mapped_length = SYSMEM_REGION_WINDOW_SIZE;
	}


	/*
	 * Generate a logical pointer corresponding to the address we want to
	 * access
	 */

	/* TBD: should these pointers go to 64-bit in all cases ? */

	logical_addr_ptr = mem_info->mapped_logical_address +
			  ((ACPI_INTEGER) address - (ACPI_INTEGER) mem_info->mapped_physical_address);

	/* Perform the memory read or write */

	switch (function) {

	case ADDRESS_SPACE_READ:

		switch (bit_width) {
		case 8:
			*value = (u32)* (u8 *) logical_addr_ptr;
			break;

		case 16:
			MOVE_UNALIGNED16_TO_32 (value, logical_addr_ptr);
			break;

		case 32:
			MOVE_UNALIGNED32_TO_32 (value, logical_addr_ptr);
			break;
		}

		break;


	case ADDRESS_SPACE_WRITE:

		switch (bit_width) {
		case 8:
			*(u8 *) logical_addr_ptr = (u8) *value;
			break;

		case 16:
			MOVE_UNALIGNED16_TO_16 (logical_addr_ptr, value);
			break;

		case 32:
			MOVE_UNALIGNED32_TO_32 (logical_addr_ptr, value);
			break;
		}

		break;


	default:
		status = AE_BAD_PARAMETER;
		break;
	}

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_system_io_space_handler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              Bit_width           - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              Handler_context     - Pointer to Handler's context
 *              Region_context      - Pointer to context specific to the
 *                                      accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the System IO address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_system_io_space_handler (
	u32                     function,
	ACPI_PHYSICAL_ADDRESS   address,
	u32                     bit_width,
	u32                     *value,
	void                    *handler_context,
	void                    *region_context)
{
	ACPI_STATUS             status = AE_OK;


	/* Decode the function parameter */

	switch (function) {

	case ADDRESS_SPACE_READ:

		switch (bit_width) {
		/* I/O Port width */

		case 8:
			*value = (u32) acpi_os_in8 ((ACPI_IO_ADDRESS) address);
			break;

		case 16:
			*value = (u32) acpi_os_in16 ((ACPI_IO_ADDRESS) address);
			break;

		case 32:
			*value = acpi_os_in32 ((ACPI_IO_ADDRESS) address);
			break;

		default:
			status = AE_AML_OPERAND_VALUE;
		}

		break;


	case ADDRESS_SPACE_WRITE:

		switch (bit_width) {
		/* I/O Port width */
		case 8:
			acpi_os_out8 ((ACPI_IO_ADDRESS) address, (u8) *value);
			break;

		case 16:
			acpi_os_out16 ((ACPI_IO_ADDRESS) address, (u16) *value);
			break;

		case 32:
			acpi_os_out32 ((ACPI_IO_ADDRESS) address, *value);
			break;

		default:
			status = AE_AML_OPERAND_VALUE;
		}

		break;


	default:
		status = AE_BAD_PARAMETER;
		break;
	}

	return (status);
}

/*******************************************************************************
 *
 * FUNCTION:    Acpi_aml_pci_config_space_handler
 *
 * PARAMETERS:  Function            - Read or Write operation
 *              Address             - Where in the space to read or write
 *              Bit_width           - Field width in bits (8, 16, or 32)
 *              Value               - Pointer to in or out value
 *              Handler_context     - Pointer to Handler's context
 *              Region_context      - Pointer to context specific to the
 *                                      accessed region
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Handler for the PCI Config address space (Op Region)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_aml_pci_config_space_handler (
	u32                     function,
	ACPI_PHYSICAL_ADDRESS   address,
	u32                     bit_width,
	u32                     *value,
	void                    *handler_context,
	void                    *region_context)
{
	ACPI_STATUS             status = AE_OK;
	u32                     pci_bus;
	u32                     dev_func;
	u8                      pci_reg;
	PCI_HANDLER_CONTEXT    *PCIcontext;


	/*
	 *  The arguments to Acpi_os(Read|Write)Pci_cfg(Byte|Word|Dword) are:
	 *
	 *  Seg_bus - 0xSSSSBBBB    - SSSS is the PCI bus segment
	 *                            BBBB is the PCI bus number
	 *
	 *  Dev_func - 0xDDDDFFFF   - DDDD is the PCI device number
	 *                            FFFF is the PCI device function number
	 *
	 *  Reg_num - Config space register must be < 40h
	 *
	 *  Value - input value for write, output for read
	 *
	 */

	PCIcontext = (PCI_HANDLER_CONTEXT *) region_context;

	pci_bus = LOWORD(PCIcontext->seg) << 16;
	pci_bus |= LOWORD(PCIcontext->bus);

	dev_func = PCIcontext->dev_func;

	pci_reg = (u8) address;

	switch (function) {

	case ADDRESS_SPACE_READ:

		*value  = 0;

		switch (bit_width) {
		/* PCI Register width */

		case 8:
			status = acpi_os_read_pci_cfg_byte (pci_bus, dev_func, pci_reg,
					   (u8 *) value);
			break;

		case 16:
			status = acpi_os_read_pci_cfg_word (pci_bus, dev_func, pci_reg,
					   (u16 *) value);
			break;

		case 32:
			status = acpi_os_read_pci_cfg_dword (pci_bus, dev_func, pci_reg,
					   value);
			break;

		default:
			status = AE_AML_OPERAND_VALUE;

		} /* Switch bit_width */

		break;


	case ADDRESS_SPACE_WRITE:

		switch (bit_width) {
		/* PCI Register width */

		case 8:
			status = acpi_os_write_pci_cfg_byte (pci_bus, dev_func, pci_reg,
					 *(u8 *) value);
			break;

		case 16:
			status = acpi_os_write_pci_cfg_word (pci_bus, dev_func, pci_reg,
					 *(u16 *) value);
			break;

		case 32:
			status = acpi_os_write_pci_cfg_dword (pci_bus, dev_func, pci_reg,
					 *value);
			break;

		default:
			status = AE_AML_OPERAND_VALUE;

		} /* Switch bit_width */

		break;


	default:

		status = AE_BAD_PARAMETER;
		break;

	}

	return (status);
}

