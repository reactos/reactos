
/******************************************************************************
 *
 * Module Name: hwacpi - ACPI Hardware Initialization/Mode Interface
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


#define _COMPONENT          ACPI_HARDWARE
	 MODULE_NAME         ("hwacpi")


/******************************************************************************
 *
 * FUNCTION:    Acpi_hw_initialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize and validate various ACPI registers
 *
 ******************************************************************************/

ACPI_STATUS
acpi_hw_initialize (
	void)
{
	ACPI_STATUS             status = AE_OK;
	u32                     index;


	/* We must have the ACPI tables by the time we get here */

	if (!acpi_gbl_FADT) {
		acpi_gbl_restore_acpi_chipset = FALSE;

		return (AE_NO_ACPI_TABLES);
	}

	/* Must support *some* mode! */
/*
	if (!(System_flags & SYS_MODES_MASK))
	{
		Restore_acpi_chipset = FALSE;

		return (AE_ERROR);
	}

*/


	switch (acpi_gbl_system_flags & SYS_MODES_MASK) {
		/* Identify current ACPI/legacy mode   */

	case (SYS_MODE_ACPI):

		acpi_gbl_original_mode = SYS_MODE_ACPI;
		break;


	case (SYS_MODE_LEGACY):

		acpi_gbl_original_mode = SYS_MODE_LEGACY;
		break;


	case (SYS_MODE_ACPI | SYS_MODE_LEGACY):

		if (acpi_hw_get_mode () == SYS_MODE_ACPI) {
			acpi_gbl_original_mode = SYS_MODE_ACPI;
		}
		else {
			acpi_gbl_original_mode = SYS_MODE_LEGACY;
		}

		break;
	}


	if (acpi_gbl_system_flags & SYS_MODE_ACPI) {
		/* Target system supports ACPI mode */

		/*
		 * The purpose of this code is to save the initial state
		 * of the ACPI event enable registers. An exit function will be
		 * registered which will restore this state when the application
		 * exits. The exit function will also clear all of the ACPI event
		 * status bits prior to restoring the original mode.
		 *
		 * The location of the PM1a_evt_blk enable registers is defined as the
		 * base of PM1a_evt_blk + DIV_2(PM1a_evt_blk_length). Since the spec further
		 * fully defines the PM1a_evt_blk to be a total of 4 bytes, the offset
		 * for the enable registers is always 2 from the base. It is hard
		 * coded here. If this changes in the spec, this code will need to
		 * be modified. The PM1b_evt_blk behaves as expected.
		 */

		acpi_gbl_pm1_enable_register_save = (u16) acpi_hw_register_read (ACPI_MTX_LOCK, PM1_EN);


		/*
		 * The GPEs behave similarly, except that the length of the register
		 * block is not fixed, so the buffer must be allocated with malloc
		 */

		if (ACPI_VALID_ADDRESS (acpi_gbl_FADT->Xgpe0blk.address) &&
			acpi_gbl_FADT->gpe0blk_len) {
			/* GPE0 specified in FADT  */

			acpi_gbl_gpe0enable_register_save =
				acpi_cm_allocate (DIV_2 (acpi_gbl_FADT->gpe0blk_len));
			if (!acpi_gbl_gpe0enable_register_save) {
				return (AE_NO_MEMORY);
			}

			/* Save state of GPE0 enable bits */

			for (index = 0; index < DIV_2 (acpi_gbl_FADT->gpe0blk_len); index++) {
				acpi_gbl_gpe0enable_register_save[index] =
					(u8) acpi_hw_register_read (ACPI_MTX_LOCK, GPE0_EN_BLOCK | index);
			}
		}

		else {
			acpi_gbl_gpe0enable_register_save = NULL;
		}

		if (ACPI_VALID_ADDRESS (acpi_gbl_FADT->Xgpe1_blk.address) &&
			acpi_gbl_FADT->gpe1_blk_len) {
			/* GPE1 defined */

			acpi_gbl_gpe1_enable_register_save =
				acpi_cm_allocate (DIV_2 (acpi_gbl_FADT->gpe1_blk_len));
			if (!acpi_gbl_gpe1_enable_register_save) {
				return (AE_NO_MEMORY);
			}

			/* save state of GPE1 enable bits */

			for (index = 0; index < DIV_2 (acpi_gbl_FADT->gpe1_blk_len); index++) {
				acpi_gbl_gpe1_enable_register_save[index] =
					(u8) acpi_hw_register_read (ACPI_MTX_LOCK, GPE1_EN_BLOCK | index);
			}
		}

		else {
			acpi_gbl_gpe1_enable_register_save = NULL;
		}
	}

	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_hw_set_mode
 *
 * PARAMETERS:  Mode            - SYS_MODE_ACPI or SYS_MODE_LEGACY
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transitions the system into the requested mode or does nothing
 *              if the system is already in that mode.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_hw_set_mode (
	u32                     mode)
{

	ACPI_STATUS             status = AE_NO_HARDWARE_RESPONSE;


	if (mode == SYS_MODE_ACPI) {
		/* BIOS should have disabled ALL fixed and GP events */

		acpi_os_out8 (acpi_gbl_FADT->smi_cmd, acpi_gbl_FADT->acpi_enable);
	}

	else if (mode == SYS_MODE_LEGACY) {
		/*
		 * BIOS should clear all fixed status bits and restore fixed event
		 * enable bits to default
		 */

		acpi_os_out8 (acpi_gbl_FADT->smi_cmd, acpi_gbl_FADT->acpi_disable);
	}

	if (acpi_hw_get_mode () == mode) {
		status = AE_OK;
	}

	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_hw_get_mode
 *
 * PARAMETERS:  none
 *
 * RETURN:      SYS_MODE_ACPI or SYS_MODE_LEGACY
 *
 * DESCRIPTION: Return current operating state of system.  Determined by
 *              querying the SCI_EN bit.
 *
 ******************************************************************************/

u32
acpi_hw_get_mode (void)
{


	if (acpi_hw_register_bit_access (ACPI_READ, ACPI_MTX_LOCK, SCI_EN)) {
		return (SYS_MODE_ACPI);
	}
	else {
		return (SYS_MODE_LEGACY);
	}
}

/******************************************************************************
 *
 * FUNCTION:    Acpi_hw_get_mode_capabilities
 *
 * PARAMETERS:  none
 *
 * RETURN:      logical OR of SYS_MODE_ACPI and SYS_MODE_LEGACY determined at initial
 *              system state.
 *
 * DESCRIPTION: Returns capablities of system
 *
 ******************************************************************************/

u32
acpi_hw_get_mode_capabilities (void)
{


	if (!(acpi_gbl_system_flags & SYS_MODES_MASK)) {
		if (acpi_hw_get_mode () == SYS_MODE_LEGACY) {
			/*
			 * Assume that if this call is being made, Acpi_init has been called
			 * and ACPI support has been established by the presence of the
			 * tables.  Therefore since we're in SYS_MODE_LEGACY, the system
			 * must support both modes
			 */

			acpi_gbl_system_flags |= (SYS_MODE_ACPI | SYS_MODE_LEGACY);
		}

		else {
			/* TBD: [Investigate] !!! this may be unsafe... */
			/*
			 * system is is ACPI mode, so try to switch back to LEGACY to see if
			 * it is supported
			 */
			acpi_hw_set_mode (SYS_MODE_LEGACY);

			if (acpi_hw_get_mode () == SYS_MODE_LEGACY) {
				/* Now in SYS_MODE_LEGACY, so both are supported */

				acpi_gbl_system_flags |= (SYS_MODE_ACPI | SYS_MODE_LEGACY);
				acpi_hw_set_mode (SYS_MODE_ACPI);
			}

			else {
				/* Still in SYS_MODE_ACPI so this must be an ACPI only system */

				acpi_gbl_system_flags |= SYS_MODE_ACPI;
			}
		}
	}

	return (acpi_gbl_system_flags & SYS_MODES_MASK);
}


