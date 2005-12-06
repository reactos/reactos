/*******************************************************************************
 *
 * Module Name: evsci - System Control Interrupt configuration and
 *                      legacy to ACPI mode state transition functions
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
#include "acnamesp.h"
#include "achware.h"
#include "acevents.h"


#define _COMPONENT          ACPI_EVENTS
	 MODULE_NAME         ("evsci")


/*
 * Elements correspond to counts for TMR, NOT_USED, GBL, PWR_BTN, SLP_BTN, RTC,
 * and GENERAL respectively.  These counts are modified by the ACPI interrupt
 * handler.
 *
 * TBD: [Investigate] Note that GENERAL should probably be split out into
 * one element for each bit in the GPE registers
 */


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ev_sci_handler
 *
 * PARAMETERS:  Context   - Calling Context
 *
 * RETURN:      Status code indicates whether interrupt was handled.
 *
 * DESCRIPTION: Interrupt handler that will figure out what function or
 *              control method to call to deal with a SCI.  Installed
 *              using BU interrupt support.
 *
 ******************************************************************************/

static u32
acpi_ev_sci_handler (void *context)
{
	u32                     interrupt_handled = INTERRUPT_NOT_HANDLED;


	/*
	 * Make sure that ACPI is enabled by checking SCI_EN.  Note that we are
	 * required to treat the SCI interrupt as sharable, level, active low.
	 */
	if (!acpi_hw_register_bit_access (ACPI_READ, ACPI_MTX_DO_NOT_LOCK, SCI_EN)) {
		/* ACPI is not enabled;  this interrupt cannot be for us */

		return (INTERRUPT_NOT_HANDLED);
	}

	/*
	 * Fixed Acpi_events:
	 * -------------
	 * Check for and dispatch any Fixed Acpi_events that have occurred
	 */
	interrupt_handled |= acpi_ev_fixed_event_detect ();

	/*
	 * GPEs:
	 * -----
	 * Check for and dispatch any GPEs that have occurred
	 */
	interrupt_handled |= acpi_ev_gpe_detect ();

	return (interrupt_handled);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_install_sci_handler
 *
 * PARAMETERS:  none
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Installs SCI handler.
 *
 ******************************************************************************/

u32
acpi_ev_install_sci_handler (void)
{
	u32                     except = AE_OK;


	except = acpi_os_install_interrupt_handler ((u32) acpi_gbl_FADT->sci_int,
			  acpi_ev_sci_handler,
			  NULL);

	return (except);
}


/******************************************************************************

 *
 * FUNCTION:    Acpi_ev_remove_sci_handler
 *
 * PARAMETERS:  none
 *
 * RETURN:      E_OK if handler uninstalled OK, E_ERROR if handler was not
 *              installed to begin with
 *
 * DESCRIPTION: Restores original status of all fixed event enable bits and
 *              removes SCI handler.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ev_remove_sci_handler (void)
{
#if 0
	/* TBD:[Investigate] Figure this out!!  Disable all events first ???  */

	if (original_fixed_enable_bit_status ^ 1 << acpi_event_index (TMR_FIXED_EVENT)) {
		acpi_event_disable_event (TMR_FIXED_EVENT);
	}

	if (original_fixed_enable_bit_status ^ 1 << acpi_event_index (GBL_FIXED_EVENT)) {
		acpi_event_disable_event (GBL_FIXED_EVENT);
	}

	if (original_fixed_enable_bit_status ^ 1 << acpi_event_index (PWR_BTN_FIXED_EVENT)) {
		acpi_event_disable_event (PWR_BTN_FIXED_EVENT);
	}

	if (original_fixed_enable_bit_status ^ 1 << acpi_event_index (SLP_BTN_FIXED_EVENT)) {
		acpi_event_disable_event (SLP_BTN_FIXED_EVENT);
	}

	if (original_fixed_enable_bit_status ^ 1 << acpi_event_index (RTC_FIXED_EVENT)) {
		acpi_event_disable_event (RTC_FIXED_EVENT);
	}

	original_fixed_enable_bit_status = 0;

#endif

	acpi_os_remove_interrupt_handler ((u32) acpi_gbl_FADT->sci_int,
			   acpi_ev_sci_handler);

	return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_ev_restore_acpi_state
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: Restore the original ACPI state of the machine
 *
 ******************************************************************************/

void
acpi_ev_restore_acpi_state (void)
{
	u32                     index;


	/* Restore the state of the chipset enable bits. */

	if (acpi_gbl_restore_acpi_chipset == TRUE) {
		/* Restore the fixed events */

		if (acpi_hw_register_read (ACPI_MTX_LOCK, PM1_EN) !=
				acpi_gbl_pm1_enable_register_save) {
			acpi_hw_register_write (ACPI_MTX_LOCK, PM1_EN,
				acpi_gbl_pm1_enable_register_save);
		}


		/* Ensure that all status bits are clear */

		acpi_hw_clear_acpi_status ();


		/* Now restore the GPEs */

		for (index = 0; index < DIV_2 (acpi_gbl_FADT->gpe0blk_len); index++) {
			if (acpi_hw_register_read (ACPI_MTX_LOCK, GPE0_EN_BLOCK | index) !=
					acpi_gbl_gpe0enable_register_save[index]) {
				acpi_hw_register_write (ACPI_MTX_LOCK, GPE0_EN_BLOCK | index,
					acpi_gbl_gpe0enable_register_save[index]);
			}
		}

		/* GPE 1 present? */

		if (acpi_gbl_FADT->gpe1_blk_len) {
			for (index = 0; index < DIV_2 (acpi_gbl_FADT->gpe1_blk_len); index++) {
				if (acpi_hw_register_read (ACPI_MTX_LOCK, GPE1_EN_BLOCK | index) !=
					acpi_gbl_gpe1_enable_register_save[index]) {
					acpi_hw_register_write (ACPI_MTX_LOCK, GPE1_EN_BLOCK | index,
						acpi_gbl_gpe1_enable_register_save[index]);
				}
			}
		}

		if (acpi_hw_get_mode() != acpi_gbl_original_mode) {
			acpi_hw_set_mode (acpi_gbl_original_mode);
		}
	}

	return;
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_terminate
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: free memory allocated for table storage.
 *
 ******************************************************************************/

void
acpi_ev_terminate (void)
{


	/*
	 * Free global tables, etc.
	 */

	if (acpi_gbl_gpe_registers) {
		acpi_cm_free (acpi_gbl_gpe_registers);
	}

	if (acpi_gbl_gpe_info) {
		acpi_cm_free (acpi_gbl_gpe_info);
	}

	return;
}


