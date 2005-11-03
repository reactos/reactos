/******************************************************************************
 *
 * Module Name: evevent - Fixed and General Purpose Acpi_event
 *                          handling and dispatch
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
#include "acevents.h"
#include "acnamesp.h"
#include "accommon.h"

#define _COMPONENT          ACPI_EVENTS
	 MODULE_NAME         ("evevent")


/**************************************************************************
 *
 * FUNCTION:    Acpi_ev_initialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Ensures that the system control interrupt (SCI) is properly
 *              configured, disables SCI event sources, installs the SCI
 *              handler
 *
 *************************************************************************/

ACPI_STATUS
acpi_ev_initialize (
	void)
{
	ACPI_STATUS             status;


	/* Make sure we have ACPI tables */

	if (!acpi_gbl_DSDT) {
		return (AE_NO_ACPI_TABLES);
	}


	/* Make sure the BIOS supports ACPI mode */

	if (SYS_MODE_LEGACY == acpi_hw_get_mode_capabilities()) {
		return (AE_ERROR);
	}


	acpi_gbl_original_mode = acpi_hw_get_mode();

	/*
	 * Initialize the Fixed and General Purpose Acpi_events prior. This is
	 * done prior to enabling SCIs to prevent interrupts from occuring
	 * before handers are installed.
	 */

	status = acpi_ev_fixed_event_initialize ();
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	status = acpi_ev_gpe_initialize ();
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Install the SCI handler */

	status = acpi_ev_install_sci_handler ();
	if (ACPI_FAILURE (status)) {
		return (status);
	}


	/* Install handlers for control method GPE handlers (_Lxx, _Exx) */

	status = acpi_ev_init_gpe_control_methods ();
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Install the handler for the Global Lock */

	status = acpi_ev_init_global_lock_handler ();
	if (ACPI_FAILURE (status)) {
		return (status);
	}


	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_fixed_event_initialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the Fixed Acpi_event data structures
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ev_fixed_event_initialize(void)
{
	int                     i = 0;

	/* Initialize the structure that keeps track of fixed event handlers */

	for (i = 0; i < NUM_FIXED_EVENTS; i++) {
		acpi_gbl_fixed_event_handlers[i].handler = NULL;
		acpi_gbl_fixed_event_handlers[i].context = NULL;
	}

	acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK, TMR_EN, 0);
	acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK, GBL_EN, 0);
	acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK, PWRBTN_EN, 0);
	acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK, SLPBTN_EN, 0);
	acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_LOCK, RTC_EN, 0);

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_fixed_event_detect
 *
 * PARAMETERS:  None
 *
 * RETURN:      INTERRUPT_HANDLED or INTERRUPT_NOT_HANDLED
 *
 * DESCRIPTION: Checks the PM status register for fixed events
 *
 ******************************************************************************/

u32
acpi_ev_fixed_event_detect(void)
{
	u32                     int_status = INTERRUPT_NOT_HANDLED;
	u32                     status_register;
	u32                     enable_register;

	/*
	 * Read the fixed feature status and enable registers, as all the cases
	 * depend on their values.
	 */

	status_register = acpi_hw_register_read (ACPI_MTX_DO_NOT_LOCK, PM1_STS);
	enable_register = acpi_hw_register_read (ACPI_MTX_DO_NOT_LOCK, PM1_EN);


	/* power management timer roll over */

	if ((status_register & ACPI_STATUS_PMTIMER) &&
		(enable_register & ACPI_ENABLE_PMTIMER)) {
		int_status |= acpi_ev_fixed_event_dispatch (ACPI_EVENT_PMTIMER);
	}

	/* global event (BIOS want's the global lock) */

	if ((status_register & ACPI_STATUS_GLOBAL) &&
		(enable_register & ACPI_ENABLE_GLOBAL)) {
		int_status |= acpi_ev_fixed_event_dispatch (ACPI_EVENT_GLOBAL);
	}

	/* power button event */

	if ((status_register & ACPI_STATUS_POWER_BUTTON) &&
		(enable_register & ACPI_ENABLE_POWER_BUTTON)) {
		int_status |= acpi_ev_fixed_event_dispatch (ACPI_EVENT_POWER_BUTTON);
	}

	/* sleep button event */

	if ((status_register & ACPI_STATUS_SLEEP_BUTTON) &&
		(enable_register & ACPI_ENABLE_SLEEP_BUTTON)) {
		int_status |= acpi_ev_fixed_event_dispatch (ACPI_EVENT_SLEEP_BUTTON);
	}

	return (int_status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_fixed_event_dispatch
 *
 * PARAMETERS:  Event               - Event type
 *
 * RETURN:      INTERRUPT_HANDLED or INTERRUPT_NOT_HANDLED
 *
 * DESCRIPTION: Clears the status bit for the requested event, calls the
 *              handler that previously registered for the event.
 *
 ******************************************************************************/

u32
acpi_ev_fixed_event_dispatch (
	u32                     event)
{
	u32 register_id;

	/* Clear the status bit */

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
		return 0;
		break;
	}

	acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_DO_NOT_LOCK, register_id, 1);

	/*
	 * Make sure we've got a handler.  If not, report an error.
	 * The event is disabled to prevent further interrupts.
	 */
	if (NULL == acpi_gbl_fixed_event_handlers[event].handler) {
		register_id = (PM1_EN | REGISTER_BIT_ID(register_id));

		acpi_hw_register_bit_access (ACPI_WRITE, ACPI_MTX_DO_NOT_LOCK,
				 register_id, 0);

		REPORT_ERROR (
			("Ev_gpe_dispatch: No installed handler for fixed event [%08X]\n",
			event));

		return (INTERRUPT_NOT_HANDLED);
	}

	/* Invoke the handler */

	return ((acpi_gbl_fixed_event_handlers[event].handler)(
			  acpi_gbl_fixed_event_handlers[event].context));
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_gpe_initialize
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the GPE data structures
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ev_gpe_initialize (void)
{
	u32                     i;
	u32                     j;
	u32                     register_index;
	u32                     gpe_number;
	u16                     gpe0register_count;
	u16                     gpe1_register_count;


	/*
	 * Set up various GPE counts
	 *
	 * You may ask,why are the GPE register block lengths divided by 2?
	 * From the ACPI 2.0 Spec, section, 4.7.1.6 General-Purpose Event
	 * Registers, we have,
	 *
	 * "Each register block contains two registers of equal length
	 * GPEx_STS and GPEx_EN (where x is 0 or 1). The length of the
	 * GPE0_STS and GPE0_EN registers is equal to half the GPE0_LEN
	 * The length of the GPE1_STS and GPE1_EN registers is equal to
	 * half the GPE1_LEN. If a generic register block is not supported
	 * then its respective block pointer and block length values in the
	 * FADT table contain zeros. The GPE0_LEN and GPE1_LEN do not need
	 * to be the same size."
	 */

	gpe0register_count          = (u16) DIV_2 (acpi_gbl_FADT->gpe0blk_len);
	gpe1_register_count         = (u16) DIV_2 (acpi_gbl_FADT->gpe1_blk_len);
	acpi_gbl_gpe_register_count = gpe0register_count + gpe1_register_count;

	if (!acpi_gbl_gpe_register_count) {
		REPORT_WARNING (("Zero GPEs are defined in the FADT\n"));
		return (AE_OK);
	}

	/*
	 * Allocate the Gpe information block
	 */

	acpi_gbl_gpe_registers = acpi_cm_callocate (acpi_gbl_gpe_register_count *
			 sizeof (ACPI_GPE_REGISTERS));
	if (!acpi_gbl_gpe_registers) {
		return (AE_NO_MEMORY);
	}

	/*
	 * Allocate the Gpe dispatch handler block
	 * There are eight distinct GP events per register.
	 * Initialization to zeros is sufficient
	 */

	acpi_gbl_gpe_info = acpi_cm_callocate (MUL_8 (acpi_gbl_gpe_register_count) *
			 sizeof (ACPI_GPE_LEVEL_INFO));
	if (!acpi_gbl_gpe_info) {
		acpi_cm_free (acpi_gbl_gpe_registers);
		return (AE_NO_MEMORY);
	}

	/* Set the Gpe validation table to GPE_INVALID */

	MEMSET (acpi_gbl_gpe_valid, (int) ACPI_GPE_INVALID, NUM_GPE);

	/*
	 * Initialize the Gpe information and validation blocks.  A goal of these
	 * blocks is to hide the fact that there are two separate GPE register sets
	 * In a given block, the status registers occupy the first half, and
	 * the enable registers occupy the second half.
	 */

	/* GPE Block 0 */

	register_index = 0;

	for (i = 0; i < gpe0register_count; i++) {
		acpi_gbl_gpe_registers[register_index].status_addr =
				 (u16) (ACPI_GET_ADDRESS (acpi_gbl_FADT->Xgpe0blk.address) + i);

		acpi_gbl_gpe_registers[register_index].enable_addr =
				 (u16) (ACPI_GET_ADDRESS (acpi_gbl_FADT->Xgpe0blk.address) + i + gpe0register_count);

		acpi_gbl_gpe_registers[register_index].gpe_base = (u8) MUL_8 (i);

		for (j = 0; j < 8; j++) {
			gpe_number = acpi_gbl_gpe_registers[register_index].gpe_base + j;
			acpi_gbl_gpe_valid[gpe_number] = (u8) register_index;
		}

		/*
		 * Clear the status/enable registers.  Note that status registers
		 * are cleared by writing a '1', while enable registers are cleared
		 * by writing a '0'.
		 */
		acpi_os_out8 (acpi_gbl_gpe_registers[register_index].enable_addr, 0x00);
		acpi_os_out8 (acpi_gbl_gpe_registers[register_index].status_addr, 0xFF);

		register_index++;
	}

	/* GPE Block 1 */

	for (i = 0; i < gpe1_register_count; i++) {
		acpi_gbl_gpe_registers[register_index].status_addr =
				 (u16) (ACPI_GET_ADDRESS (acpi_gbl_FADT->Xgpe1_blk.address) + i);

		acpi_gbl_gpe_registers[register_index].enable_addr =
				 (u16) (ACPI_GET_ADDRESS (acpi_gbl_FADT->Xgpe1_blk.address) + i + gpe1_register_count);

		acpi_gbl_gpe_registers[register_index].gpe_base =
				 (u8) (acpi_gbl_FADT->gpe1_base + MUL_8 (i));

		for (j = 0; j < 8; j++) {
			gpe_number = acpi_gbl_gpe_registers[register_index].gpe_base + j;
			acpi_gbl_gpe_valid[gpe_number] = (u8) register_index;
		}

		/*
		 * Clear the status/enable registers.  Note that status registers
		 * are cleared by writing a '1', while enable registers are cleared
		 * by writing a '0'.
		 */
		acpi_os_out8 (acpi_gbl_gpe_registers[register_index].enable_addr, 0x00);
		acpi_os_out8 (acpi_gbl_gpe_registers[register_index].status_addr, 0xFF);

		register_index++;
	}

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_save_method_info
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Called from Acpi_walk_namespace. Expects each object to be a
 *              control method under the _GPE portion of the namespace.
 *              Extract the name and GPE type from the object, saving this
 *              information for quick lookup during GPE dispatch
 *
 *              The name of each GPE control method is of the form:
 *                  "_Lnn" or "_Enn"
 *              Where:
 *                  L      - means that the GPE is level triggered
 *                  E      - means that the GPE is edge triggered
 *                  nn     - is the GPE number
 *
 ******************************************************************************/

static ACPI_STATUS
acpi_ev_save_method_info (
	ACPI_HANDLE             obj_handle,
	u32                     level,
	void                    *obj_desc,
	void                    **return_value)
{
	u32                     gpe_number;
	NATIVE_CHAR             name[ACPI_NAME_SIZE + 1];
	u8                      type;


	/* Extract the name from the object and convert to a string */

	MOVE_UNALIGNED32_TO_32 (name, &((ACPI_NAMESPACE_NODE *) obj_handle)->name);
	name[ACPI_NAME_SIZE] = 0;

	/*
	 * Edge/Level determination is based on the 2nd s8 of the method name
	 */
	if (name[1] == 'L') {
		type = ACPI_EVENT_LEVEL_TRIGGERED;
	}
	else if (name[1] == 'E') {
		type = ACPI_EVENT_EDGE_TRIGGERED;
	}
	else {
		/* Unknown method type, just ignore it! */

		return (AE_OK);
	}

	/* Convert the last two characters of the name to the Gpe Number */

	gpe_number = STRTOUL (&name[2], NULL, 16);
	if (gpe_number == ACPI_UINT32_MAX) {
		/* Conversion failed; invalid method, just ignore it */

		return (AE_OK);
	}

	/* Ensure that we have a valid GPE number */

	if (acpi_gbl_gpe_valid[gpe_number] == ACPI_GPE_INVALID) {
		/* Not valid, all we can do here is ignore it */

		return (AE_OK);
	}

	/*
	 * Now we can add this information to the Gpe_info block
	 * for use during dispatch of this GPE.
	 */

	acpi_gbl_gpe_info [gpe_number].type         = type;
	acpi_gbl_gpe_info [gpe_number].method_handle = obj_handle;


	/*
	 * Enable the GPE (SCIs should be disabled at this point)
	 */

	acpi_hw_enable_gpe (gpe_number);

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_init_gpe_control_methods
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Obtain the control methods associated with the GPEs.
 *
 *              NOTE: Must be called AFTER namespace initialization!
 *
 ******************************************************************************/

ACPI_STATUS
acpi_ev_init_gpe_control_methods (void)
{
	ACPI_STATUS             status;


	/* Get a permanent handle to the _GPE object */

	status = acpi_get_handle (NULL, "\\_GPE", &acpi_gbl_gpe_obj_handle);
	if (ACPI_FAILURE (status)) {
		return (status);
	}

	/* Traverse the namespace under \_GPE to find all methods there */

	status = acpi_walk_namespace (ACPI_TYPE_METHOD, acpi_gbl_gpe_obj_handle,
			  ACPI_UINT32_MAX, acpi_ev_save_method_info,
			  NULL, NULL);

	return (status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_gpe_detect
 *
 * PARAMETERS:  None
 *
 * RETURN:      INTERRUPT_HANDLED or INTERRUPT_NOT_HANDLED
 *
 * DESCRIPTION: Detect if any GP events have occurred
 *
 ******************************************************************************/

u32
acpi_ev_gpe_detect (void)
{
	u32                     int_status = INTERRUPT_NOT_HANDLED;
	u32                     i;
	u32                     j;
	u8                      enabled_status_byte;
	u8                      bit_mask;


	/*
	 * Read all of the 8-bit GPE status and enable registers
	 * in both of the register blocks, saving all of it.
	 * Find all currently active GP events.
	 */

	for (i = 0; i < acpi_gbl_gpe_register_count; i++) {
		acpi_gbl_gpe_registers[i].status =
				   acpi_os_in8 (acpi_gbl_gpe_registers[i].status_addr);

		acpi_gbl_gpe_registers[i].enable =
				   acpi_os_in8 (acpi_gbl_gpe_registers[i].enable_addr);

		/* First check if there is anything active at all in this register */

		enabled_status_byte = (u8) (acpi_gbl_gpe_registers[i].status &
				  acpi_gbl_gpe_registers[i].enable);

		if (!enabled_status_byte) {
			/* No active GPEs in this register, move on */

			continue;
		}

		/* Now look at the individual GPEs in this byte register */

		for (j = 0, bit_mask = 1; j < 8; j++, bit_mask <<= 1) {
			/* Examine one GPE bit */

			if (enabled_status_byte & bit_mask) {
				/*
				 * Found an active GPE.  Dispatch the event to a handler
				 * or method.
				 */
				int_status |=
					acpi_ev_gpe_dispatch (acpi_gbl_gpe_registers[i].gpe_base + j);
			}
		}
	}

	return (int_status);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_asynch_execute_gpe_method
 *
 * PARAMETERS:  Gpe_number      - The 0-based Gpe number
 *
 * RETURN:      None
 *
 * DESCRIPTION: Perform the actual execution of a GPE control method.  This
 *              function is called from an invocation of Acpi_os_queue_for_execution
 *              (and therefore does NOT execute at interrupt level) so that
 *              the control method itself is not executed in the context of
 *              the SCI interrupt handler.
 *
 ******************************************************************************/

static void
acpi_ev_asynch_execute_gpe_method (
	void                    *context)
{
	u32                     gpe_number = (u32) context;
	ACPI_GPE_LEVEL_INFO     gpe_info;


	/*
	 * Take a snapshot of the GPE info for this level
	 */
	acpi_cm_acquire_mutex (ACPI_MTX_EVENTS);
	gpe_info = acpi_gbl_gpe_info [gpe_number];
	acpi_cm_release_mutex (ACPI_MTX_EVENTS);

	/*
	 * Method Handler (_Lxx, _Exx):
	 * ----------------------------
	 * Evaluate the _Lxx/_Exx control method that corresponds to this GPE.
	 */
	if (gpe_info.method_handle) {
		acpi_ns_evaluate_by_handle (gpe_info.method_handle, NULL, NULL);
	}

	/*
	 * Level-Triggered?
	 * ----------------
	 * If level-triggered we clear the GPE status bit after handling the event.
	 */
	if (gpe_info.type & ACPI_EVENT_LEVEL_TRIGGERED) {
		acpi_hw_clear_gpe (gpe_number);
	}

	/*
	 * Enable the GPE.
	 */
	acpi_hw_enable_gpe (gpe_number);

	return;
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_ev_gpe_dispatch
 *
 * PARAMETERS:  Gpe_number      - The 0-based Gpe number
 *
 * RETURN:      INTERRUPT_HANDLED or INTERRUPT_NOT_HANDLED
 *
 * DESCRIPTION: Handle and dispatch a General Purpose Acpi_event.
 *              Clears the status bit for the requested event.
 *
 * TBD: [Investigate] is this still valid or necessary:
 * The Gpe handler differs from the fixed events in that it clears the enable
 * bit rather than the status bit to clear the interrupt.  This allows
 * software outside of interrupt context to determine what caused the SCI and
 * dispatch the correct AML.
 *
 ******************************************************************************/

u32
acpi_ev_gpe_dispatch (
	u32                     gpe_number)
{
	ACPI_GPE_LEVEL_INFO     gpe_info;

	/*
	 * Valid GPE number?
	 */
	if (acpi_gbl_gpe_valid[gpe_number] == ACPI_GPE_INVALID) {
		return (INTERRUPT_NOT_HANDLED);
	}

	/*
	 * Disable the GPE.
	 */
	acpi_hw_disable_gpe (gpe_number);

	gpe_info = acpi_gbl_gpe_info [gpe_number];

	/*
	 * Edge-Triggered?
	 * ---------------
	 * If edge-triggered, clear the GPE status bit now.  Note that
	 * level-triggered events are cleared after the GPE is serviced.
	 */
	if (gpe_info.type & ACPI_EVENT_EDGE_TRIGGERED) {
		acpi_hw_clear_gpe (gpe_number);
	}
		/*
		 * Function Handler (e.g. EC)?
		 */
	if (gpe_info.handler) {
		/* Invoke function handler (at interrupt level). */
		gpe_info.handler (gpe_info.context);

		/* Level-Triggered? */
		if (gpe_info.type & ACPI_EVENT_LEVEL_TRIGGERED) {
			acpi_hw_clear_gpe (gpe_number);
		}

		/* Enable GPE */
		acpi_hw_enable_gpe (gpe_number);
	}
	/*
	 * Method Handler (e.g. _Exx/_Lxx)?
	 */
	else if (gpe_info.method_handle) {
		if (ACPI_FAILURE(acpi_os_queue_for_execution (OSD_PRIORITY_GPE,
			acpi_ev_asynch_execute_gpe_method, (void*)(NATIVE_UINT)gpe_number))) {
			/*
			 * Shoudn't occur, but if it does report an error. Note that
			 * the GPE will remain disabled until the ACPI Core Subsystem
			 * is restarted, or the handler is removed/reinstalled.
			 */
			REPORT_ERROR (("Acpi_ev_gpe_dispatch: Unable to queue handler for GPE bit [%X]\n", gpe_number));
		}
	}
	/*
	 * No Handler? Report an error and leave the GPE disabled.
	 */
	else {
		REPORT_ERROR (("Acpi_ev_gpe_dispatch: No installed handler for GPE [%X]\n", gpe_number));

		/* Level-Triggered? */
		if (gpe_info.type & ACPI_EVENT_LEVEL_TRIGGERED) {
			acpi_hw_clear_gpe (gpe_number);
		}
	}

	return (INTERRUPT_HANDLED);
}
