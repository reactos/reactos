/******************************************************************************
 *
 * Module Name: cminit - Common ACPI subsystem initialization
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
#include "acparser.h"
#include "acdispat.h"

#define _COMPONENT          ACPI_UTILITIES
	 MODULE_NAME         ("cminit")


#define ACPI_OFFSET(d,o)    ((u32) &(((d *)0)->o))
#define ACPI_FADT_OFFSET(o) ACPI_OFFSET (FADT_DESCRIPTOR, o)

/*******************************************************************************
 *
 * FUNCTION:    Acpi_cm_fadt_register_error
 *
 * PARAMETERS:  *Register_name          - Pointer to string identifying register
 *              Value                   - Actual register contents value
 *              Acpi_test_spec_section  - TDS section containing assertion
 *              Acpi_assertion          - Assertion number being tested
 *
 * RETURN:      AE_BAD_VALUE
 *
 * DESCRIPTION: Display failure message and link failure to TDS assertion
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_cm_fadt_register_error (
	NATIVE_CHAR             *register_name,
	u32                     value,
	u32                     offset)
{

	REPORT_ERROR (
		("Invalid FADT value %s=%lX at offset %lX FADT=%p\n",
		register_name, value, offset, acpi_gbl_FADT));


	return (AE_BAD_VALUE);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_cm_validate_fadt
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Validate various ACPI registers in the FADT
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_validate_fadt (
	void)
{
	ACPI_STATUS                 status = AE_OK;


	/*
	 * Verify Fixed ACPI Description Table fields,
	 * but don't abort on any problems, just display error
	 */

	if (acpi_gbl_FADT->pm1_evt_len < 4) {
		status = acpi_cm_fadt_register_error ("PM1_EVT_LEN",
				  (u32) acpi_gbl_FADT->pm1_evt_len,
				  ACPI_FADT_OFFSET (pm1_evt_len));
	}

	if (!acpi_gbl_FADT->pm1_cnt_len) {
		status = acpi_cm_fadt_register_error ("PM1_CNT_LEN", 0,
				  ACPI_FADT_OFFSET (pm1_cnt_len));
	}

	if (!ACPI_VALID_ADDRESS (acpi_gbl_FADT->Xpm1a_evt_blk.address)) {
		status = acpi_cm_fadt_register_error ("X_PM1a_EVT_BLK", 0,
				  ACPI_FADT_OFFSET (Xpm1a_evt_blk.address));
	}

	if (!ACPI_VALID_ADDRESS (acpi_gbl_FADT->Xpm1a_cnt_blk.address)) {
		status = acpi_cm_fadt_register_error ("X_PM1a_CNT_BLK", 0,
				  ACPI_FADT_OFFSET (Xpm1a_cnt_blk.address));
	}

	if (!ACPI_VALID_ADDRESS (acpi_gbl_FADT->Xpm_tmr_blk.address)) {
		status = acpi_cm_fadt_register_error ("X_PM_TMR_BLK", 0,
				  ACPI_FADT_OFFSET (Xpm_tmr_blk.address));
	}

	if ((ACPI_VALID_ADDRESS (acpi_gbl_FADT->Xpm2_cnt_blk.address) &&
		!acpi_gbl_FADT->pm2_cnt_len)) {
		status = acpi_cm_fadt_register_error ("PM2_CNT_LEN",
				  (u32) acpi_gbl_FADT->pm2_cnt_len,
				  ACPI_FADT_OFFSET (pm2_cnt_len));
	}

	if (acpi_gbl_FADT->pm_tm_len < 4) {
		status = acpi_cm_fadt_register_error ("PM_TM_LEN",
				  (u32) acpi_gbl_FADT->pm_tm_len,
				  ACPI_FADT_OFFSET (pm_tm_len));
	}

	/* length of GPE blocks must be a multiple of 2 */


	if (ACPI_VALID_ADDRESS (acpi_gbl_FADT->Xgpe0blk.address) &&
		(acpi_gbl_FADT->gpe0blk_len & 1)) {
		status = acpi_cm_fadt_register_error ("(x)GPE0_BLK_LEN",
				  (u32) acpi_gbl_FADT->gpe0blk_len,
				  ACPI_FADT_OFFSET (gpe0blk_len));
	}

	if (ACPI_VALID_ADDRESS (acpi_gbl_FADT->Xgpe1_blk.address) &&
		(acpi_gbl_FADT->gpe1_blk_len & 1)) {
		status = acpi_cm_fadt_register_error ("(x)GPE1_BLK_LEN",
				  (u32) acpi_gbl_FADT->gpe1_blk_len,
				  ACPI_FADT_OFFSET (gpe1_blk_len));
	}

	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_cm_terminate
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: free memory allocated for table storage.
 *
 ******************************************************************************/

void
acpi_cm_terminate (void)
{


	/* Free global tables, etc. */

	if (acpi_gbl_gpe0enable_register_save) {
		acpi_cm_free (acpi_gbl_gpe0enable_register_save);
	}

	if (acpi_gbl_gpe1_enable_register_save) {
		acpi_cm_free (acpi_gbl_gpe1_enable_register_save);
	}


	return;
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_cm_subsystem_shutdown
 *
 * PARAMETERS:  none
 *
 * RETURN:      none
 *
 * DESCRIPTION: Shutdown the various subsystems.  Don't delete the mutex
 *              objects here -- because the AML debugger may be still running.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_cm_subsystem_shutdown (void)
{

	/* Just exit if subsystem is already shutdown */

	if (acpi_gbl_shutdown) {
		return (AE_OK);
	}

	/* Subsystem appears active, go ahead and shut it down */

	acpi_gbl_shutdown = TRUE;

	/* Close the Namespace */

	acpi_ns_terminate ();

	/* Close the Acpi_event Handling */

	acpi_ev_terminate ();

	/* Close the globals */

	acpi_cm_terminate ();

	/* Flush the local cache(s) */

	acpi_cm_delete_generic_state_cache ();
	acpi_cm_delete_object_cache ();
	acpi_ds_delete_walk_state_cache ();

	/* Close the Parser */

	/* TBD: [Restructure] Acpi_ps_terminate () */

	acpi_ps_delete_parse_cache ();

	/* Debug only - display leftover memory allocation, if any */
#ifdef ENABLE_DEBUGGER
	acpi_cm_dump_current_allocations (ACPI_UINT32_MAX, NULL);
#endif

	return (AE_OK);
}


