/*****************************************************************************
 *
 * Module Name: bmxface.c
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
	MODULE_NAME		("bmxface")


/****************************************************************************
 *                            External Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_get_device_status
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/
ACPI_STATUS
bm_get_device_status (
	BM_HANDLE               device_handle,
	BM_DEVICE_STATUS        *device_status)
{
	ACPI_STATUS             status = AE_OK;
	BM_NODE			*node = NULL;

	FUNCTION_TRACE("bm_get_device_status");

	if (!device_status) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	*device_status = BM_STATUS_UNKNOWN;

	/*
	 * Resolve device handle to node.
	 */
	status = bm_get_node(device_handle, 0, &node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Parent Present?
	 * ---------------
	 * If the parent isn't present we can't evalute _STA on the child.
	 * Return an unknown status.
	 */
	if (!BM_NODE_PRESENT(node->parent)) {
		return_ACPI_STATUS(AE_OK);
	}
	
	/*
	 * Dynamic Status?
	 * ---------------
	 * If _STA isn't present we just return the default status.
	 */
	if (!(node->device.flags & BM_FLAGS_DYNAMIC_STATUS)) {
		*device_status = BM_STATUS_DEFAULT;
		return_ACPI_STATUS(AE_OK);
	}

	/*
	 * Evaluate _STA:
	 * --------------
	 */
	status = bm_evaluate_simple_integer(node->device.acpi_handle, "_STA", 
		&(node->device.status));
	if (ACPI_SUCCESS(status)) {
		*device_status = node->device.status;
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_get_device_info
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/
ACPI_STATUS
bm_get_device_info (
	BM_HANDLE               device_handle,
	BM_DEVICE               **device)
{
	ACPI_STATUS             status = AE_OK;
	BM_NODE			*node = NULL;

	FUNCTION_TRACE("bm_get_device_info");

	if (!device) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Resolve device handle to internal device.
	 */
	status = bm_get_node(device_handle, 0, &node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	*device = &(node->device);

	return_ACPI_STATUS(AE_OK);
}


/****************************************************************************
 *
 * FUNCTION:    bm_get_device_context
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/
ACPI_STATUS
bm_get_device_context (
	BM_HANDLE               device_handle,
	BM_DRIVER_CONTEXT	*context)
{
	ACPI_STATUS             status = AE_OK;
	BM_NODE			*node = NULL;

	FUNCTION_TRACE("bm_get_device_context");

	if (!context) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	*context = NULL;

	/*
	 * Resolve device handle to internal device.
	 */
	status = bm_get_node(device_handle, 0, &node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	if (!node->driver.context) {
		return_ACPI_STATUS(AE_NULL_ENTRY);
	}

	*context = node->driver.context;

	return_ACPI_STATUS(AE_OK);
}


/****************************************************************************
 *
 * FUNCTION:    bm_register_driver
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_register_driver (
	BM_DEVICE_ID		*criteria,
	BM_DRIVER		*driver)
{
	ACPI_STATUS             status = AE_NOT_FOUND;
	BM_HANDLE_LIST          device_list;
	BM_NODE			*node = NULL;
	u32                     i = 0;

	FUNCTION_TRACE("bm_register_driver");

	if (!criteria || !driver || !driver->notify || !driver->request) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	MEMSET(&device_list, 0, sizeof(BM_HANDLE_LIST));

	/*
	 * Find Matches:
	 * -------------
	 * Search through the entire device hierarchy for matches against
	 * the given device criteria.
	 */
	status = bm_search(BM_HANDLE_ROOT, criteria, &device_list);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Install driver:
	 * ----------------
	 * For each match, record the driver information and execute the 
	 * driver's Notify() funciton (if present) to notify the driver
	 * of the device's presence.
	 */
	for (i = 0; i < device_list.count; i++) {

		/* Resolve the device handle. */
		status = bm_get_node(device_list.handles[i], 0, &node);
		if (ACPI_FAILURE(status)) {
			continue;
		}

		DEBUG_PRINT(ACPI_INFO, ("Registering driver for device [0x%02x].\n", node->device.handle));

		/* Notify driver of new device. */
		status = driver->notify(BM_NOTIFY_DEVICE_ADDED, 
			node->device.handle, &(node->driver.context));
		if (ACPI_SUCCESS(status)) {
			node->driver.notify = driver->notify;
			node->driver.request = driver->request;
			node->device.flags |= BM_FLAGS_DRIVER_CONTROL;
		}
	}

	return_ACPI_STATUS(AE_OK);
}


/****************************************************************************
 *
 * FUNCTION:    bm_unregister_driver
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_unregister_driver (
	BM_DEVICE_ID		*criteria,
	BM_DRIVER		*driver)
{
	ACPI_STATUS             status = AE_NOT_FOUND;
	BM_HANDLE_LIST          device_list;
	BM_NODE			*node = NULL;
	u32                     i = 0;

	FUNCTION_TRACE("bm_unregister_driver");

	if (!criteria || !driver || !driver->notify || !driver->request) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	MEMSET(&device_list, 0, sizeof(BM_HANDLE_LIST));

	/*
	 * Find Matches:
	 * -------------
	 * Search through the entire device hierarchy for matches against
	 * the given device criteria.
	 */
	status = bm_search(BM_HANDLE_ROOT, criteria, &device_list);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Remove driver:
	 * ---------------
	 * For each match, execute the driver's Notify() function to allow
	 * the driver to cleanup each device instance.
	 */
	for (i = 0; i < device_list.count; i++) {
		/*
		 * Resolve the device handle.
		 */
		status = bm_get_node(device_list.handles[i], 0, &node);
		if (ACPI_FAILURE(status)) {
			continue;
		}

		DEBUG_PRINT(ACPI_INFO, ("Unregistering driver for device [0x%02x].\n", node->device.handle));

		/* Notify driver of device removal. */
		status = node->driver.notify(BM_NOTIFY_DEVICE_REMOVED, 
			node->device.handle, &(node->driver.context));

		node->device.flags &= ~BM_FLAGS_DRIVER_CONTROL;

		MEMSET(&(node->driver), 0, sizeof(BM_DRIVER));
	}

	return_ACPI_STATUS(AE_OK);
}
