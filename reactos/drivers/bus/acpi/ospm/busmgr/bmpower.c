/****************************************************************************
 *
 * Module Name: bmpower.c - Driver for ACPI Power Resource 'devices'
 *   $Revision: 1.1 $
 *
 ****************************************************************************/

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

/*
 * TODO:
 * -----
 *	1. Sequencing of power resource list transitions.
 *	2. Global serialization of power resource transtions (see ACPI 
 *         spec section 7.1.2/7.1.3).
 *      3. Better error handling.
 */


#include <acpi.h>
#include "bm.h"
#include "bmpower.h"


#define _COMPONENT		ACPI_POWER_CONTROL
	MODULE_NAME		("bmpower")


/****************************************************************************
 *                             Function Prototypes
 ****************************************************************************/

ACPI_STATUS
bm_pr_notify (
	BM_NOTIFY               notify_type,
	BM_HANDLE               device_handle,
	void                    **context);
	
ACPI_STATUS
bm_pr_request (
	BM_REQUEST		*request,
	void                    *context);

	
/****************************************************************************
 *                             Internal Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_pr_print
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_print (
	BM_POWER_RESOURCE	*pr)
{
	ACPI_BUFFER             buffer;

	if (!pr) {
		return(AE_BAD_PARAMETER);
	}

	buffer.length = 256;
	buffer.pointer = acpi_os_callocate(buffer.length);
	if (!buffer.pointer) {
		return(AE_NO_MEMORY);
	}

	acpi_get_name(pr->acpi_handle, ACPI_FULL_PATHNAME, &buffer);

	acpi_os_printf("Power Resource: found\n");

	DEBUG_PRINT(ACPI_INFO, ("+------------------------------------------------------------\n"));
	DEBUG_PRINT(ACPI_INFO, ("PowerResource[0x%02X]|[0x%08X] %s\n", pr->device_handle, pr->acpi_handle, buffer.pointer));
	DEBUG_PRINT(ACPI_INFO, ("  system_level[S%d] resource_order[%d]\n", pr->system_level, pr->resource_order));
	DEBUG_PRINT(ACPI_INFO, ("  state[D%d] reference_count[%d]\n", pr->state, pr->reference_count));
	DEBUG_PRINT(ACPI_INFO, ("+------------------------------------------------------------\n"));

	acpi_os_free(buffer.pointer);

	return(AE_OK);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_get_state
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_get_state (
	BM_POWER_RESOURCE	*pr)
{
	ACPI_STATUS             status = AE_OK;
	BM_DEVICE_STATUS        device_status = BM_STATUS_UNKNOWN;

	FUNCTION_TRACE("bm_pr_get_state");

	if (!pr) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	pr->state = ACPI_STATE_UNKNOWN;

	/* 
	 * Evaluate _STA:
	 * --------------
	 * Evalute _STA to determine whether the power resource is ON or OFF.
	 * Note that if the power resource isn't present we'll get AE_OK but
	 * an unknown status.
	 */
	status = bm_get_device_status(pr->device_handle, &device_status);
	if (ACPI_FAILURE(status)) {
		DEBUG_PRINT(ACPI_ERROR, ("Error reading status for power resource [0x%02x].\n", pr->device_handle));
		return_ACPI_STATUS(status);
	}
	if (device_status == BM_STATUS_UNKNOWN) {
		DEBUG_PRINT(ACPI_ERROR, ("Error reading status for power resource [0x%02x].\n", pr->device_handle));
		return_ACPI_STATUS(AE_NOT_EXIST);
	}

	/*
	 * Mask off all bits but the first as some systems return non-standard 
	 * values (e.g. 0x51).
	 */
	switch (device_status & 0x01) {
	case 0:
		DEBUG_PRINT(ACPI_INFO, ("Power resource [0x%02x] is OFF.\n", pr->device_handle));
		pr->state = ACPI_STATE_D3;
		break;
	case 1:
		DEBUG_PRINT(ACPI_INFO, ("Power resource [0x%02x] is ON.\n", pr->device_handle));
		pr->state = ACPI_STATE_D0;
		break;
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_set_state
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_set_state (
	BM_POWER_RESOURCE	*pr,
	BM_POWER_STATE          target_state)
{
	ACPI_STATUS             status = AE_OK;

	FUNCTION_TRACE("bm_pr_set_state");

	if (!pr) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	status = bm_pr_get_state(pr);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	if (target_state == pr->state) {
		DEBUG_PRINT(ACPI_INFO, ("Power resource [0x%02X] already at target power state [D%d].\n", pr->device_handle, pr->state));
		return_ACPI_STATUS(AE_OK);
	}

	switch (target_state) {

	case ACPI_STATE_D0:
		DEBUG_PRINT(ACPI_INFO, ("Turning power resource [0x%02X] ON.\n", pr->device_handle));
		status = bm_evaluate_object(pr->acpi_handle, "_ON", NULL, NULL);
		break;

	case ACPI_STATE_D3:
		DEBUG_PRINT(ACPI_INFO, ("Turning power resource [0x%02X] OFF.\n", pr->device_handle));
		status = bm_evaluate_object(pr->acpi_handle, "_OFF", NULL, NULL);
		break;

	default:
		status = AE_BAD_PARAMETER;
		break;
	}

	status = bm_pr_get_state(pr);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_list_get_state
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_list_get_state (
	BM_HANDLE_LIST          *pr_list,
	BM_POWER_STATE          *power_state)
{
	ACPI_STATUS             status = AE_OK;
	BM_POWER_RESOURCE	*pr = NULL;
	u32                     i = 0;

	FUNCTION_TRACE("bm_pr_list_get_state");

	if (!pr_list || !power_state) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	if (pr_list->count < 1) {
		pr->state = ACPI_STATE_UNKNOWN;
		return_ACPI_STATUS(AE_ERROR);
	}

	(*power_state) = ACPI_STATE_D0;

	/*
	 * Calculate Current power_state:
	 * -----------------------------
	 * The current state of a list of power resources is ON if all
	 * power resources are currently in the ON state.  In other words,
	 * if any power resource in the list is OFF then the collection 
	 * isn't fully ON.
	 */
	for (i = 0; i < pr_list->count; i++) {

		status = bm_get_device_context(pr_list->handles[i],
			(BM_DRIVER_CONTEXT*)(&pr));
		if (ACPI_FAILURE(status)) {
			DEBUG_PRINT(ACPI_WARN, ("Invalid reference to power resource [0x%02X].\n", pr_list->handles[i]));
			(*power_state) = ACPI_STATE_UNKNOWN;
			break;
		}

		status = bm_pr_get_state(pr);
		if (ACPI_FAILURE(status)) {
			(*power_state) = ACPI_STATE_UNKNOWN;
			break;
		}

		if (pr->state != ACPI_STATE_D0) {
			(*power_state) = pr->state;
			break;
		}
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_list_transition
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_list_transition (
	BM_HANDLE_LIST          *current_list,
	BM_HANDLE_LIST          *target_list)
{
	ACPI_STATUS             status = AE_OK;
	BM_POWER_RESOURCE	*pr = NULL;
	u32                     i = 0;

	FUNCTION_TRACE("bm_pr_list_transition");

	if (!current_list || !target_list) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Reference Target:
	 * -----------------
	 * Reference all resources for the target power state first (so 
	 * the device doesn't get turned off while transitioning).  Power 
	 * resources that aren't on (new reference count of 1) are turned on.
	 */
	for (i = 0; i < target_list->count; i++) {

		status = bm_get_device_context(target_list->handles[i], 
			(BM_DRIVER_CONTEXT*)(&pr));
		if (ACPI_FAILURE(status)) {
			DEBUG_PRINT(ACPI_WARN, ("Invalid reference to power resource [0x%02X].\n", target_list->handles[i]));
			continue;
		}

		if (++pr->reference_count == 1) {
			/* TODO: Need ordering based upon resource_order */
			status = bm_pr_set_state(pr, ACPI_STATE_D0);
			if (ACPI_FAILURE(status)) {
				/* TODO: How do we handle this? */
				DEBUG_PRINT(ACPI_WARN, ("Unable to change power state for power resource [0x%02X].\n", target_list->handles[i]));
			}
		}
	}

	/*
	 * Dereference Current:
	 * --------------------
	 * Dereference all resources for the current power state.  Power
	 * resources no longer referenced (new reference count of 0) are 
	 * turned off.
	 */
	for (i = 0; i < current_list->count; i++) {

		status = bm_get_device_context(current_list->handles[i], 
			(BM_DRIVER_CONTEXT*)(&pr));
		if (ACPI_FAILURE(status)) {
			DEBUG_PRINT(ACPI_WARN, ("Invalid reference to power resource [0x%02X].\n", target_list->handles[i]));
			continue;
		}

		if (--pr->reference_count == 0) {
			/* TODO: Need ordering based upon resource_order */
			status = bm_pr_set_state(pr, ACPI_STATE_D3);
			if (ACPI_FAILURE(status)) {
				/* TODO: How do we handle this? */
				DEBUG_PRINT(ACPI_ERROR, ("Unable to change power state for power resource [0x%02X].\n", current_list->handles[i]));
			}
		}
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_add_device
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_add_device (
	BM_HANDLE               device_handle,
	void                    **context)
{
	ACPI_STATUS             status = AE_OK;
	BM_POWER_RESOURCE	*pr = NULL;
	BM_DEVICE		*device = NULL;
	ACPI_BUFFER		buffer;
	ACPI_OBJECT		acpi_object;

	FUNCTION_TRACE("bm_pr_add_device");

	DEBUG_PRINT(ACPI_INFO, ("Adding power resource [0x%02X].\n", device_handle));

	if (!context || *context) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	buffer.length = sizeof(ACPI_OBJECT);
	buffer.pointer = &acpi_object;

	/*
	 * Get information on this device.
	 */
	status = bm_get_device_info(device_handle, &device);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Allocate a new BM_POWER_RESOURCE structure.
	 */
	pr = acpi_os_callocate(sizeof(BM_POWER_RESOURCE));
	if (!pr) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	pr->device_handle = device->handle;
	pr->acpi_handle = device->acpi_handle;

	/* 
	 * Get information on this power resource.
	 */
	status = acpi_evaluate_object(pr->acpi_handle, NULL, NULL, &buffer);
	if (ACPI_FAILURE(status)) {
		goto end;
	}

	pr->system_level = acpi_object.power_resource.system_level;
	pr->resource_order = acpi_object.power_resource.resource_order;
	pr->state = ACPI_STATE_UNKNOWN;
	pr->reference_count = 0;

	/*
	 * Get the power resource's current state (ON|OFF).
	 */
	status = bm_pr_get_state(pr);

end:
	if (ACPI_FAILURE(status)) {
		acpi_os_free(pr);
	}
	else {
		*context = pr;
		bm_pr_print(pr);
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_remove_device
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_remove_device (
	void                    **context)
{
	ACPI_STATUS             status = AE_OK;
	BM_POWER_RESOURCE	*pr = NULL;

	FUNCTION_TRACE("bm_pr_remove_device");

	if (!context || !*context) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	pr = (BM_POWER_RESOURCE*)*context;

	DEBUG_PRINT(ACPI_INFO, ("Removing power resource [0x%02X].\n", pr->device_handle));

	acpi_os_free(pr);

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *                             External Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_pr_initialize
 *
 * PARAMETERS:  <none>
 *
 * RETURN:
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_initialize (void)
{
	ACPI_STATUS             status = AE_OK;
	BM_DEVICE_ID		criteria;
	BM_DRIVER		driver;

	FUNCTION_TRACE("bm_pr_initialize");

	MEMSET(&criteria, 0, sizeof(BM_DEVICE_ID));
	MEMSET(&driver, 0, sizeof(BM_DRIVER));

	criteria.type = BM_TYPE_POWER_RESOURCE;

	driver.notify = &bm_pr_notify;
	driver.request = &bm_pr_request;

	status = bm_register_driver(&criteria, &driver);

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_terminate
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:	<TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_terminate (void)
{
	ACPI_STATUS             status = AE_OK;
	BM_DEVICE_ID		criteria;
	BM_DRIVER		driver;

	FUNCTION_TRACE("bm_pr_terminate");

	MEMSET(&criteria, 0, sizeof(BM_DEVICE_ID));
	MEMSET(&driver, 0, sizeof(BM_DRIVER));

	criteria.type = BM_TYPE_POWER_RESOURCE;

	driver.notify = &bm_pr_notify;
	driver.request = &bm_pr_request;

	status = bm_unregister_driver(&criteria, &driver);

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_notify
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:	<TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_notify (
	BM_NOTIFY               notify_type,
	BM_HANDLE               device_handle,
	void                    **context)
{
	ACPI_STATUS             status = AE_OK;

	FUNCTION_TRACE("bm_pr_notify");

	switch (notify_type) {

	case BM_NOTIFY_DEVICE_ADDED:
		status = bm_pr_add_device(device_handle, context);
		break;

	case BM_NOTIFY_DEVICE_REMOVED:
		status = bm_pr_remove_device(context);
		break;

	default:
		status = AE_SUPPORT;
		break;
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_pr_request
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_pr_request (
	BM_REQUEST		*request,
	void                    *context)
{
	ACPI_STATUS             status = AE_OK;
	BM_POWER_RESOURCE	*pr = NULL;

	FUNCTION_TRACE("bm_pr_request");

	/*
	 * Must have a valid request structure and context.
	 */
	if (!request || !context) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * context contains information specific to this power resource.
	 */
	pr = (BM_POWER_RESOURCE*)context;

	/*
	 * Handle request:
	 * ---------------
	 */
	switch (request->command) {

	default:
		status = AE_SUPPORT;
		break;
	}

	request->status = status;

	return_ACPI_STATUS(status);
}



