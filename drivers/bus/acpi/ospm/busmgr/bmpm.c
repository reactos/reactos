/*****************************************************************************
 *
 * Module Name: bmpm.c
 *   $Revision: 1.1 $
 *
 *****************************************************************************/

/*
 *  Copyright (C) 2000, 2001 Andrew Grover
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


#include <acpi.h>
#include "bm.h"
#include "bmpower.h"


#define _COMPONENT		ACPI_POWER_CONTROL
	MODULE_NAME		("bmpm")


/****************************************************************************
 *                             Internal Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_get_inferred_power_state
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_get_inferred_power_state (
	BM_DEVICE               *device)
{
	ACPI_STATUS             status = AE_OK;
	BM_HANDLE_LIST          pr_list;
	BM_POWER_STATE          list_state = ACPI_STATE_UNKNOWN;
	char                    object_name[5] = {'_','P','R','0','\0'};
	u32                     i = 0;

	FUNCTION_TRACE("bm_get_inferred_power_state");

	if (!device) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	MEMSET(&pr_list, 0, sizeof(BM_HANDLE_LIST));

	device->power.state = ACPI_STATE_D3;

	/*
	 * Calculate Power State:
	 * ----------------------
	 * Try to infer the devices's power state by checking the state of 
	 * the devices's power resources.  We start by evaluating _PR0 
	 * (resource requirements at D0) and work through _PR1 and _PR2.  
	 * We know the current devices power state when all resources (for 
	 * a give Dx state) are ON.  If no power resources are on then the 
	 * device is assumed to be off (D3).
	 */
	for (i=ACPI_STATE_D0; i<ACPI_STATE_D3; i++) {

		status = bm_evaluate_reference_list(device->acpi_handle, 
			object_name, &pr_list);

		if (ACPI_SUCCESS(status)) {

			status = bm_pr_list_get_state(&pr_list, 
				&list_state);

			if (ACPI_SUCCESS(status)) {

				if (list_state == ACPI_STATE_D0) {
					device->power.state = i;
					break;
				}
			}
		}
	}

	return_ACPI_STATUS(AE_OK);
}


/****************************************************************************
 *                             External Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_get_power_state
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_get_power_state (
	BM_NODE			*node)
{
	ACPI_STATUS             status = AE_OK;
	BM_DEVICE               *device = NULL;

	FUNCTION_TRACE("bm_get_power_state");

	if (!node) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	device = &(node->device);

	device->power.state = ACPI_STATE_UNKNOWN;

	if (device->flags & BM_FLAGS_POWER_STATE) {
		status = bm_evaluate_simple_integer(device->acpi_handle, 
			"_PSC", &(device->power.state));
	}
	else {
		status = bm_get_inferred_power_state(device);
	}

	if (ACPI_SUCCESS(status)) {
		DEBUG_PRINT(ACPI_INFO, ("Device [0x%02x] is at power state [D%d].\n", device->handle, device->power.state));
	}
	else {
		DEBUG_PRINT(ACPI_INFO, ("Error getting power state for device [0x%02x]\n", device->handle));
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_set_power_state
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_set_power_state (
	BM_NODE			*node,
	BM_POWER_STATE          state)
{
	ACPI_STATUS             status = AE_OK;
	BM_DEVICE		*device = NULL;
	BM_DEVICE		*parent_device = NULL;
	BM_HANDLE_LIST          current_list;
	BM_HANDLE_LIST          target_list;
	char                    object_name[5] = {'_','P','R','0','\0'};

	FUNCTION_TRACE("bm_set_power_state");

	if (!node || !node->parent || (state > ACPI_STATE_D3)) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	MEMSET(&current_list, 0, sizeof(BM_HANDLE_LIST));
	MEMSET(&target_list, 0, sizeof(BM_HANDLE_LIST));

	device = &(node->device);
	parent_device = &(node->parent->device);

	/*
	 * Check Parent's Power State:
	 * ---------------------------
	 * Can't be in a higher power state (lower Dx value) than parent.
	 */
	if (state < parent_device->power.state) {
		DEBUG_PRINT(ACPI_WARN, ("Cannot set device [0x%02x] to a higher-powered state than parent_device.\n", device->handle));
		return_ACPI_STATUS(AE_ERROR);
	}

	/*
	 * Get Resources:
	 * --------------
	 * Get the power resources associated with the device's current 
	 * and target power states.
	 */
	if (device->power.state != ACPI_STATE_UNKNOWN) {
		object_name[3] = '0' + device->power.state;
		bm_evaluate_reference_list(device->acpi_handle, 
			object_name, &current_list);
	}

	object_name[3] = '0' + state;
	bm_evaluate_reference_list(device->acpi_handle, object_name, 
		&target_list);

	/*
	 * Transition Resources:
	 * ---------------------
	 * Transition all power resources referenced by this device to 
	 * the correct power state (taking into consideration sequencing 
	 * and dependencies to other devices).
	 */
	if (current_list.count || target_list.count) {
		status = bm_pr_list_transition(&current_list, &target_list);
	}
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Execute _PSx:
	 * -------------
	 * Execute the _PSx method corresponding to the target Dx state, 
	 * if it exists.
	 */
	object_name[2] = 'S';
	object_name[3] = '0' + state;
	bm_evaluate_object(device->acpi_handle, object_name, NULL, NULL);

	if (ACPI_SUCCESS(status)) {
		DEBUG_PRINT(ACPI_INFO, ("Device [0x%02x] is now at [D%d].\n", device->handle, state));
		device->power.state = state;
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_get_pm_capabilities
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_get_pm_capabilities (
	BM_NODE			*node)
{
	ACPI_STATUS             status = AE_OK;
	BM_DEVICE		*device = NULL;
	BM_DEVICE		*parent_device = NULL;
	ACPI_HANDLE             acpi_handle = NULL;
	BM_POWER_STATE          dx_supported = ACPI_STATE_UNKNOWN;
	char                    object_name[5] = {'_','S','0','D','\0'};
	u32                     i = 0;

	FUNCTION_TRACE("bm_get_pm_capabilities");

	if (!node || !node->parent) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	device = &(node->device);
	parent_device = &(node->parent->device);

	/*
	 * Power Management Flags:
	 * -----------------------
	 */
	if (ACPI_SUCCESS(acpi_get_handle(device->acpi_handle, "_PSC", 
		&acpi_handle))) {
		device->power.flags |= BM_FLAGS_POWER_STATE;
	}

	if (ACPI_SUCCESS(acpi_get_handle(device->acpi_handle, "_IRC", 
		&acpi_handle))) {
		device->power.flags |= BM_FLAGS_INRUSH_CURRENT;
	}

	if (ACPI_SUCCESS(acpi_get_handle(device->acpi_handle, "_PRW", 
		&acpi_handle))) {
		device->power.flags |= BM_FLAGS_WAKE_CAPABLE;
	}

	/*
	 * Device Power State:
	 * -------------------
	 * Note that we can't get the device's power state until we've
	 * initialized all power resources, so for now we just set to
	 * unknown.
	 */
	device->power.state = ACPI_STATE_UNKNOWN;

	/*
	 * Dx Supported in S0:
	 * -------------------
	 * Figure out which Dx states are supported by this device for the
	 * S0 (working) state.  Note that D0 and D3 are required (assumed).
	 */
	device->power.dx_supported[ACPI_STATE_S0] = BM_FLAGS_D0_SUPPORT | 
		BM_FLAGS_D3_SUPPORT;

	if ((ACPI_SUCCESS(acpi_get_handle(device->acpi_handle, "_PR1", 
		&acpi_handle))) || 
		(ACPI_SUCCESS(acpi_get_handle(device->acpi_handle, "_PS1", 
		&acpi_handle)))) {
		device->power.dx_supported[ACPI_STATE_S0] |= 
			BM_FLAGS_D1_SUPPORT;
	}

	if ((ACPI_SUCCESS(acpi_get_handle(device->acpi_handle, "_PR2", 
		&acpi_handle))) || 
		(ACPI_SUCCESS(acpi_get_handle(device->acpi_handle, "_PS2", 
		&acpi_handle)))) {
		device->power.dx_supported[ACPI_STATE_S0] |= 
			BM_FLAGS_D2_SUPPORT;
	}

	/*
	 * Dx Supported in S1-S5:
	 * ----------------------
	 * Figure out which Dx states are supported by this device for
	 * all other Sx states.
	 */
	for (i = ACPI_STATE_S1; i <= ACPI_STATE_S5; i++) {

		/*
		 * D3 support is assumed (off is always possible!).
		 */
		device->power.dx_supported[i] = BM_FLAGS_D3_SUPPORT;

		/*
		 * Evalute _SxD:
		 * -------------
		 * Which returns the highest (power) Dx state supported in 
		 * this system (Sx) state.  We convert this value to a bit 
		 * mask of supported states (conceptually simpler).
		 */
		status = bm_evaluate_simple_integer(device->acpi_handle, 
			object_name, &dx_supported);
		if (ACPI_SUCCESS(status)) {
			switch (dx_supported) {
			case 0:
				device->power.dx_supported[i] |= 
					BM_FLAGS_D0_SUPPORT;
				/* fall through */
			case 1:
				device->power.dx_supported[i] |= 
					BM_FLAGS_D1_SUPPORT;
				/* fall through */
			case 2:
				device->power.dx_supported[i] |= 
					BM_FLAGS_D2_SUPPORT;
				/* fall through */
			case 3:
				device->power.dx_supported[i] |= 
					BM_FLAGS_D3_SUPPORT;
				break;
			}

			/*
			 * Validate:
			 * ---------
			 * Mask of any states that _Sx_d falsely advertises 
			 * (e.g.claims D1 support but neither _PR2 or _PS2 
			 * exist).  In other words, S1-S5 can't offer a Dx 
			 * state that isn't supported by S0.
			 */
			device->power.dx_supported[i] &= 
				device->power.dx_supported[ACPI_STATE_S0];
		}

		object_name[2]++;
	}

	return_ACPI_STATUS(status);
}
