/******************************************************************************
 *
 * Module Name: evxfevnt - External Interfaces, ACPI event disable/enable
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
#include "amlcode.h"
#include "acinterp.h"

#define _COMPONENT          ACPI_EVENTS
	 MODULE_NAME         ("evxfevnt")


/**************************************************************************
 *
 * FUNCTION:    Acpi_enable
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Transfers the system into ACPI mode.
 *
 *************************************************************************/

ACPI_STATUS
acpi_enable (void)
{
	ACPI_STATUS             status;


	/* Make sure we've got ACPI tables */

	if (!acpi_gbl_DSDT) {
		return (AE_NO_ACPI_TABLES);
	}

	/* Make sure the BIOS supports ACPI mode */

	if (SYS_MODE_LEGACY == acpi_hw_get_mode_capabilities()) {
		return (AE_ERROR);
	}

	/* Transition to ACPI mode */

	status = acpi_hw_set_mode (SYS_MODE_ACPI);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	return (status);
}


/**************************************************************************
 *
 * FUNCTION:    Acpi_disable
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Returns the system to original ACPI/legacy mode, and
 *              uninstalls the SCI interrupt handler.
 *
 *************************************************************************/

ACPI_STATUS
acpi_disable (void)
{
	ACPI_STATUS             status;


	/* Restore original mode  */

	status = acpi_hw_set_mode (acpi_gbl_original_mode);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Unload the SCI interrupt handler  */

	acpi_ev_remove_sci_handler ();
	acpi_ev_restore_acpi_state ();

	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_enable_event
 *
 * PARAMETERS:  Event           - The fixed event or GPE to be enabled
 *              Type            - The type of event
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Enable an ACPI event (fixed and general purpose)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_enable_event (
	u32                     event,
	u32                     type)
{
	ACPI_STATUS             status = AE_OK;
	u32                     register_id;


	/* The Type must be either Fixed Acpi_event or GPE */

	switch (type) {

	case ACPI_EVENT_FIXED:

		/* Decode the Fixed Acpi_event */

		switch (event) {
		case ACPI_EVENT_PMTIMER:
			register_id = TMR_EN;
			break;

		case ACPI_EVENT_GLOBAL:
			register_id = GBL_EN;
			break;

		case ACPI_EVENT_POWER_BUTTON:
			register_id = PWRBTN_EN;
			break;

		case ACPI_EVENT_SLEEP_BUTTON:
			register_id = SLPBTN_EN;
			break;

		case ACPI_EVENT_RTC:
			register_id = RTC_EN;
			break;

		default:
			return (AE_BAD_PARAMETER);
			break;
		}

		/*
		 * Enable the requested fixed event (by writing a one to the
		 * enable register bit)
		 */

		acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK, register_id, 1);

		if (1 != acpi_hw_register_bit_access(ACPI_READ, ACPI_MTX_LOCK, register_id)) {
			return (AE_NO_HARDWARE_RESPONSE);
		}

		break;


	case ACPI_EVENT_GPE:

		/* Ensure that we have a valid GPE number */

		if ((event >= NUM_GPE) ||
			(acpi_gbl_gpe_valid[event] == ACPI_GPE_INVALID)) {
			return (AE_BAD_PARAMETER);
		}


		/* Enable the requested GPE number */

		acpi_hw_enable_gpe (event);
		break;


	default:

		status = AE_BAD_PARAMETER;
	}


	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_disable_event
 *
 * PARAMETERS:  Event           - The fixed event or GPE to be enabled
 *              Type            - The type of event
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Disable an ACPI event (fixed and general purpose)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_disable_event (
	u32                     event,
	u32                     type)
{
	ACPI_STATUS             status = AE_OK;
	u32                     register_id;


	/* The Type must be either Fixed Acpi_event or GPE */

	switch (type) {

	case ACPI_EVENT_FIXED:

		/* Decode the Fixed Acpi_event */

		switch (event) {
		case ACPI_EVENT_PMTIMER:
			register_id = TMR_EN;
			break;

		case ACPI_EVENT_GLOBAL:
			register_id = GBL_EN;
			break;

		case ACPI_EVENT_POWER_BUTTON:
			register_id = PWRBTN_EN;
			break;

		case ACPI_EVENT_SLEEP_BUTTON:
			register_id = SLPBTN_EN;
			break;

		case ACPI_EVENT_RTC:
			register_id = RTC_EN;
			break;

		default:
			return (AE_BAD_PARAMETER);
			break;
		}

		/*
		 * Disable the requested fixed event (by writing a zero to the
		 * enable register bit)
		 */

		acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK, register_id, 0);

		if (0 != acpi_hw_register_bit_access(ACPI_READ, ACPI_MTX_LOCK, register_id)) {
			return (AE_NO_HARDWARE_RESPONSE);
		}

		break;


	case ACPI_EVENT_GPE:

		/* Ensure that we have a valid GPE number */

		if ((event >= NUM_GPE) ||
			(acpi_gbl_gpe_valid[event] == ACPI_GPE_INVALID)) {
			return (AE_BAD_PARAMETER);
		}

		/* Disable the requested GPE number */

		acpi_hw_disable_gpe (event);
		break;


	default:
		status = AE_BAD_PARAMETER;
	}

	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_clear_event
 *
 * PARAMETERS:  Event           - The fixed event or GPE to be cleared
 *              Type            - The type of event
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Clear an ACPI event (fixed and general purpose)
 *
 ******************************************************************************/

ACPI_STATUS
acpi_clear_event (
	u32                     event,
	u32                     type)
{
	ACPI_STATUS             status = AE_OK;
	u32                     register_id;


	/* The Type must be either Fixed Acpi_event or GPE */

	switch (type) {

	case ACPI_EVENT_FIXED:

		/* Decode the Fixed Acpi_event */

		switch (event) {
		case ACPI_EVENT_PMTIMER:
			register_id = TMR_STS;
			break;

		case ACPI_EVENT_GLOBAL:
			register_id = GBL_STS;
			break;

		case ACPI_EVENT_POWER_BUTTON:
			register_id = PWRBTN_STS;
			break;

		case ACPI_EVENT_SLEEP_BUTTON:
			register_id = SLPBTN_STS;
			break;

		case ACPI_EVENT_RTC:
			register_id = RTC_STS;
			break;

		default:
			return (AE_BAD_PARAMETER);
			break;
		}

		/*
		 * Clear the requested fixed event (By writing a one to the
		 * status register bit)
		 */

		acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK, register_id, 1);
		break;


	case ACPI_EVENT_GPE:

		/* Ensure that we have a valid GPE number */

		if ((event >= NUM_GPE) ||
			(acpi_gbl_gpe_valid[event] == ACPI_GPE_INVALID)) {
			return (AE_BAD_PARAMETER);
		}


		acpi_hw_clear_gpe (event);
		break;


	default:

		status = AE_BAD_PARAMETER;
	}

	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_get_event_status
 *
 * PARAMETERS:  Event           - The fixed event or GPE
 *              Type            - The type of event
 *              Status          - Where the current status of the event will
 *                                be returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Obtains and returns the current status of the event
 *
 ******************************************************************************/


ACPI_STATUS
acpi_get_event_status (
	u32                     event,
	u32                     type,
	ACPI_EVENT_STATUS       *event_status)
{
	ACPI_STATUS             status = AE_OK;
	u32                     register_id;


	if (!event_status) {
		return (AE_BAD_PARAMETER);
	}


	/* The Type must be either Fixed Acpi_event or GPE */

	switch (type) {

	case ACPI_EVENT_FIXED:

		/* Decode the Fixed Acpi_event */

		switch (event) {
		case ACPI_EVENT_PMTIMER:
			register_id = TMR_STS;
			break;

		case ACPI_EVENT_GLOBAL:
			register_id = GBL_STS;
			break;

		case ACPI_EVENT_POWER_BUTTON:
			register_id = PWRBTN_STS;
			break;

		case ACPI_EVENT_SLEEP_BUTTON:
			register_id = SLPBTN_STS;
			break;

		case ACPI_EVENT_RTC:
			register_id = RTC_STS;
			break;

		default:
			return (AE_BAD_PARAMETER);
			break;
		}

		/* Get the status of the requested fixed event */

		*event_status = acpi_hw_register_bit_access (ACPI_READ, ACPI_MTX_LOCK, register_id);
		break;


	case ACPI_EVENT_GPE:

		/* Ensure that we have a valid GPE number */

		if ((event >= NUM_GPE) ||
			(acpi_gbl_gpe_valid[event] == ACPI_GPE_INVALID)) {
			return (AE_BAD_PARAMETER);
		}


		/* Obtain status on the requested GPE number */

		acpi_hw_get_gpe_status (event, event_status);
		break;


	default:
		status = AE_BAD_PARAMETER;
	}


	return (status);
}

