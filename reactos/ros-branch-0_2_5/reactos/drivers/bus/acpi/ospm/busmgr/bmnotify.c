/*****************************************************************************
 *
 * Module Name: bmnotify.c
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


#define _COMPONENT		ACPI_BUS_MANAGER
	 MODULE_NAME		("bmnotify")


/****************************************************************************
 *                            Internal Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_generate_notify
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_generate_notify (
	BM_NODE			*node,
	u32			notify_type)
{
	ACPI_STATUS		status = AE_OK;

	FUNCTION_TRACE("bm_generate_notify");

	if (!node) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	DEBUG_PRINT(ACPI_INFO, ("Sending notify [0x%02x] to device [0x%02x].\n", notify_type, node->device.handle));

	if (!(node->device.flags & BM_FLAGS_DRIVER_CONTROL) || 
		!(node->driver.notify)) {
		DEBUG_PRINT(ACPI_WARN, ("No driver installed for device [0x%02x].\n", node->device.handle));
		return_ACPI_STATUS(AE_NOT_EXIST);
	}

	status = node->driver.notify(notify_type, node->device.handle, 
		&(node->driver.context));

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_device_check
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_device_check (
	BM_NODE			*node,
	u32			*status_change)
{
	ACPI_STATUS             status = AE_OK;
	BM_DEVICE		*device = NULL;
	BM_DEVICE_STATUS	old_status = BM_STATUS_UNKNOWN;

	FUNCTION_TRACE("bm_device_check");

	if (!node) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	device = &(node->device);

	if (status_change) {
		*status_change = FALSE;
	}

	old_status = device->status;

	/*
	 * Parent Present?
	 * ---------------
	 * Only check this device if its parent is present (which implies
	 * this device MAY be present).
	 */
	if (!BM_NODE_PRESENT(node->parent)) {
		return_ACPI_STATUS(AE_OK);
	}

	/*
	 * Get Status:
	 * -----------
	 * And see if the status has changed.
	 */
	status = bm_get_status(device);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}
	
	if (old_status == node->device.status) {
		return_ACPI_STATUS(AE_OK);
	}

	if (status_change) {
		*status_change = TRUE;
	}
	
	/*
	 * Device Insertion?
	 * -----------------
	 */
	if ((device->status & BM_STATUS_PRESENT) && 
		!(old_status & BM_STATUS_PRESENT)) {
		/* TODO: Make sure driver is loaded, and if not, load. */
		status = bm_generate_notify(node, BM_NOTIFY_DEVICE_ADDED);
	}

	/*
	 * Device Removal?
	 * ---------------
	 */
	else if (!(device->status & BM_STATUS_PRESENT) && 
		(old_status & BM_STATUS_PRESENT)) {
		/* TODO: Unload driver if last device instance. */
		status = bm_generate_notify(node, BM_NOTIFY_DEVICE_REMOVED);
	}

	return_ACPI_STATUS(AE_OK);
}


/****************************************************************************
 *
 * FUNCTION:    bm_bus_check
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_bus_check (
	BM_NODE			*parent_node)
{
	ACPI_STATUS             status = AE_OK;
	u32			status_change = FALSE;

	FUNCTION_TRACE("bm_bus_check");

	if (!parent_node) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Status Change?
	 * --------------
	 */
	status = bm_device_check(parent_node, &status_change);
	if (ACPI_FAILURE(status) || !status_change) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Enumerate Scope:
	 * ----------------
	 * TODO: Enumerate child devices within this device's scope and
	 *       run bm_device_check()'s on them...
	 */

	return_ACPI_STATUS(AE_OK);
}


/****************************************************************************
 *                            External Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_notify
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

void
bm_notify (
	ACPI_HANDLE             acpi_handle,
	u32                     notify_value,
	void                    *context)
{
	ACPI_STATUS             status = AE_OK;
	BM_NODE			*node = NULL;

	FUNCTION_TRACE("bm_notify");

	/*
	 * Resolve the ACPI handle.
	 */
	status = bm_get_node(0, acpi_handle, &node);
	if (ACPI_FAILURE(status)) {
		DEBUG_PRINT(ACPI_INFO, ("Recieved notify [0x%02x] for unknown device [%p].\n", notify_value, acpi_handle));
		return_VOID;
	}

	/*
	 * Device-Specific or Standard?
	 * ----------------------------
	 * Device-specific notifies are forwarded to the control module's 
	 * notify() function for processing.  Standard notifies are handled
	 * internally.
	 */
	if (notify_value > 0x7F) {
		status = bm_generate_notify(node, notify_value);
	}
	else {
		switch (notify_value) {

		case BM_NOTIFY_BUS_CHECK:
			DEBUG_PRINT(ACPI_INFO, ("Received BUS CHECK notification.\n"));
			status = bm_bus_check(node);
			break;

		case BM_NOTIFY_DEVICE_CHECK:
			DEBUG_PRINT(ACPI_INFO, ("Received DEVICE CHECK notification.\n"));
			status = bm_device_check(node, NULL);
			break;

		case BM_NOTIFY_DEVICE_WAKE:
			DEBUG_PRINT(ACPI_INFO, ("Received DEVICE WAKE notification.\n"));
			/* TODO */
			break;

		case BM_NOTIFY_EJECT_REQUEST:
			DEBUG_PRINT(ACPI_INFO, ("Received EJECT REQUEST notification.\n"));
			/* TODO */
			break;

		case BM_NOTIFY_DEVICE_CHECK_LIGHT:
			DEBUG_PRINT(ACPI_INFO, ("Received DEVICE CHECK LIGHT notification.\n"));
			/* TODO: Exactly what does the 'light' mean? */
			status = bm_device_check(node, NULL);
			break;

		case BM_NOTIFY_FREQUENCY_MISMATCH:
			DEBUG_PRINT(ACPI_INFO, ("Received FREQUENCY MISMATCH notification.\n"));
			/* TODO */
			break;

		case BM_NOTIFY_BUS_MODE_MISMATCH:
			DEBUG_PRINT(ACPI_INFO, ("Received BUS MODE MISMATCH notification.\n"));
			/* TODO */
			break;

		case BM_NOTIFY_POWER_FAULT:
			DEBUG_PRINT(ACPI_INFO, ("Received POWER FAULT notification.\n"));
			/* TODO */
			break;

		default:
			DEBUG_PRINT(ACPI_INFO, ("Received unknown/unsupported notification.\n"));
			break;
		}
	}

	return_VOID;
}


