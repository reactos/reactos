
/******************************************************************************
 *
 * Module Name: hwgpe - Low level GPE enable/disable/clear functions
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
#include "acnamesp.h"
#include "acevents.h"

#define _COMPONENT          ACPI_HARDWARE
	 MODULE_NAME         ("hwgpe")


/******************************************************************************
 *
 * FUNCTION:    Acpi_hw_enable_gpe
 *
 * PARAMETERS:  Gpe_number      - The GPE
 *
 * RETURN:      None
 *
 * DESCRIPTION: Enable a single GPE.
 *
 ******************************************************************************/

void
acpi_hw_enable_gpe (
	u32                     gpe_number)
{
	u8                      in_byte;
	u32                     register_index;
	u8                      bit_mask;

	/*
	 * Translate GPE number to index into global registers array.
	 */
	register_index = acpi_gbl_gpe_valid[gpe_number];

	/*
	 * Figure out the bit offset for this GPE within the target register.
	 */
	bit_mask = acpi_gbl_decode_to8bit [MOD_8 (gpe_number)];

	/*
	 * Read the current value of the register, set the appropriate bit
	 * to enable the GPE, and write out the new register.
	 */
	in_byte = acpi_os_in8 (acpi_gbl_gpe_registers[register_index].enable_addr);
	acpi_os_out8 (acpi_gbl_gpe_registers[register_index].enable_addr,
			 (u8)(in_byte | bit_mask));
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_hw_disable_gpe
 *
 * PARAMETERS:  Gpe_number      - The GPE
 *
 * RETURN:      None
 *
 * DESCRIPTION: Disable a single GPE.
 *
 ******************************************************************************/

void
acpi_hw_disable_gpe (
	u32                     gpe_number)
{
	u8                      in_byte;
	u32                     register_index;
	u8                      bit_mask;

	/*
	 * Translate GPE number to index into global registers array.
	 */
	register_index = acpi_gbl_gpe_valid[gpe_number];

	/*
	 * Figure out the bit offset for this GPE within the target register.
	 */
	bit_mask = acpi_gbl_decode_to8bit [MOD_8 (gpe_number)];

	/*
	 * Read the current value of the register, clear the appropriate bit,
	 * and write out the new register value to disable the GPE.
	 */
	in_byte = acpi_os_in8 (acpi_gbl_gpe_registers[register_index].enable_addr);
	acpi_os_out8 (acpi_gbl_gpe_registers[register_index].enable_addr,
			 (u8)(in_byte & ~bit_mask));
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_hw_clear_gpe
 *
 * PARAMETERS:  Gpe_number      - The GPE
 *
 * RETURN:      None
 *
 * DESCRIPTION: Clear a single GPE.
 *
 ******************************************************************************/

void
acpi_hw_clear_gpe (
	u32                     gpe_number)
{
	u32                     register_index;
	u8                      bit_mask;

	/*
	 * Translate GPE number to index into global registers array.
	 */
	register_index = acpi_gbl_gpe_valid[gpe_number];

	/*
	 * Figure out the bit offset for this GPE within the target register.
	 */
	bit_mask = acpi_gbl_decode_to8bit [MOD_8 (gpe_number)];

	/*
	 * Write a one to the appropriate bit in the status register to
	 * clear this GPE.
	 */
	acpi_os_out8 (acpi_gbl_gpe_registers[register_index].status_addr, bit_mask);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_hw_get_gpe_status
 *
 * PARAMETERS:  Gpe_number      - The GPE
 *
 * RETURN:      None
 *
 * DESCRIPTION: Return the status of a single GPE.
 *
 ******************************************************************************/

void
acpi_hw_get_gpe_status (
	u32                     gpe_number,
	ACPI_EVENT_STATUS       *event_status)
{
	u8                      in_byte = 0;
	u32                     register_index = 0;
	u8                      bit_mask = 0;

	if (!event_status) {
		return;
	}

	(*event_status) = 0;

	/*
	 * Translate GPE number to index into global registers array.
	 */
	register_index = acpi_gbl_gpe_valid[gpe_number];

	/*
	 * Figure out the bit offset for this GPE within the target register.
	 */
	bit_mask = acpi_gbl_decode_to8bit [MOD_8 (gpe_number)];

	/*
	 * Enabled?:
	 */
	in_byte = acpi_os_in8 (acpi_gbl_gpe_registers[register_index].enable_addr);

	if (bit_mask & in_byte) {
		(*event_status) |= ACPI_EVENT_FLAG_ENABLED;
	}

	/*
	 * Set?
	 */
	in_byte = acpi_os_in8 (acpi_gbl_gpe_registers[register_index].status_addr);

	if (bit_mask & in_byte) {
		(*event_status) |= ACPI_EVENT_FLAG_SET;
	}
}
