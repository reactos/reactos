/******************************************************************************
 *
 * Module Name: bmrequest.c
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
	MODULE_NAME		("bmrequest")


/****************************************************************************
 *                            External Functions
 ****************************************************************************/

/****************************************************************************
 *
 * FUNCTION:    bm_generate_request
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_generate_request (
	BM_NODE			*node,
	BM_REQUEST		*request)
{
	ACPI_STATUS		status = AE_OK;

	FUNCTION_TRACE("bm_generate_request");

	if (!node || !request) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	DEBUG_PRINT(ACPI_INFO, ("Sending request [0x%02x] to device [0x%02x].\n", request->command, node->device.handle));

	if (!(node->device.flags & BM_FLAGS_DRIVER_CONTROL) || 
		!(node->driver.request)) {
		DEBUG_PRINT(ACPI_WARN, ("No driver installed for device [0x%02x].\n", node->device.handle));
		return_ACPI_STATUS(AE_NOT_EXIST);
	}

	status = node->driver.request(request, node->driver.context);

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:    bm_request
 *
 * PARAMETERS:  <TBD>
 *
 * RETURN:      <TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bm_request (
	BM_REQUEST              *request)
{
	ACPI_STATUS             status = AE_OK;
	BM_NODE			*node = NULL;
	BM_DEVICE		*device = NULL;

	FUNCTION_TRACE("bm_request");

	/*
	 * Must have a valid request structure.
	 */
	if (!request) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	DEBUG_PRINT(ACPI_INFO, ("Received request for device [0x%02x] command [0x%08x].\n", request->handle, request->command));

	/*
	 * Resolve the node.
	 */
	status = bm_get_node(request->handle, 0, &node);
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	device = &(node->device);

	/*
	 * Device-Specific Request?
	 * ------------------------
	 * If a device-specific command (>=0x80) forward this request to
	 * the appropriate driver.
	 */
	if (request->command & BM_COMMAND_DEVICE_SPECIFIC) {
		status = bm_generate_request(node, request);
		return_ACPI_STATUS(status);
	}

	/*
	 * Bus-Specific Requests:
	 * ----------------------
	 */
	switch (request->command) {

	case BM_COMMAND_GET_POWER_STATE:
		status = bm_get_power_state(node);
		if (ACPI_FAILURE(status)) {
			break;
		}
		status = bm_copy_to_buffer(&(request->buffer), 
			&(device->power.state), sizeof(BM_POWER_STATE));
		break;

	case BM_COMMAND_SET_POWER_STATE:
	{
		BM_POWER_STATE *power_state = NULL;

		status = bm_cast_buffer(&(request->buffer), 
			(void**)&power_state, sizeof(BM_POWER_STATE));
		if (ACPI_FAILURE(status)) {
			break;
		}
		status = bm_set_power_state(node, *power_state);
	}
		break;

	default:
		status = AE_SUPPORT;
		request->status = AE_SUPPORT;
		break;
	}

	return_ACPI_STATUS(status);
}
