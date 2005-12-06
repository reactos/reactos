/*******************************************************************************
 *
 * Module Name: rsxface - Public interfaces to the ACPI subsystem
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
#include "acinterp.h"
#include "acnamesp.h"
#include "acresrc.h"

#define _COMPONENT          ACPI_RESOURCES
	 MODULE_NAME         ("rsxface")


/*******************************************************************************
 *
 * FUNCTION:    Acpi_get_irq_routing_table
 *
 * PARAMETERS:  Device_handle   - a handle to the Bus device we are querying
 *              Ret_buffer      - a pointer to a buffer to receive the
 *                                current resources for the device
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to get the IRQ routing table for a
 *              specific bus.  The caller must first acquire a handle for the
 *              desired bus.  The routine table is placed in the buffer pointed
 *              to by the Ret_buffer variable parameter.
 *
 *              If the function fails an appropriate status will be returned
 *              and the value of Ret_buffer is undefined.
 *
 *              This function attempts to execute the _PRT method contained in
 *              the object indicated by the passed Device_handle.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_get_irq_routing_table (
	ACPI_HANDLE             device_handle,
	ACPI_BUFFER             *ret_buffer)
{
	ACPI_STATUS             status;


	/*
	 *  Must have a valid handle and buffer, So we have to have a handle
	 *  and a return buffer structure, and if there is a non-zero buffer length
	 *  we also need a valid pointer in the buffer. If it's a zero buffer length,
	 *  we'll be returning the needed buffer size, so keep going.
	 */
	if ((!device_handle)        ||
		(!ret_buffer)           ||
		((!ret_buffer->pointer) && (ret_buffer->length))) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_rs_get_prt_method_data (device_handle, ret_buffer);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_get_current_resources
 *
 * PARAMETERS:  Device_handle   - a handle to the device object for the
 *                                device we are querying
 *              Ret_buffer      - a pointer to a buffer to receive the
 *                                current resources for the device
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to get the current resources for a
 *              specific device.  The caller must first acquire a handle for
 *              the desired device.  The resource data is placed in the buffer
 *              pointed to by the Ret_buffer variable parameter.
 *
 *              If the function fails an appropriate status will be returned
 *              and the value of Ret_buffer is undefined.
 *
 *              This function attempts to execute the _CRS method contained in
 *              the object indicated by the passed Device_handle.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_get_current_resources (
	ACPI_HANDLE             device_handle,
	ACPI_BUFFER             *ret_buffer)
{
	ACPI_STATUS             status;


	/*
	 *  Must have a valid handle and buffer, So we have to have a handle
	 *  and a return buffer structure, and if there is a non-zero buffer length
	 *  we also need a valid pointer in the buffer. If it's a zero buffer length,
	 *  we'll be returning the needed buffer size, so keep going.
	 */
	if ((!device_handle)        ||
		(!ret_buffer)           ||
		((ret_buffer->length) && (!ret_buffer->pointer))) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_rs_get_crs_method_data (device_handle, ret_buffer);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_get_possible_resources
 *
 * PARAMETERS:  Device_handle   - a handle to the device object for the
 *                                device we are querying
 *              Ret_buffer      - a pointer to a buffer to receive the
 *                                resources for the device
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to get a list of the possible resources
 *              for a specific device.  The caller must first acquire a handle
 *              for the desired device.  The resource data is placed in the
 *              buffer pointed to by the Ret_buffer variable.
 *
 *              If the function fails an appropriate status will be returned
 *              and the value of Ret_buffer is undefined.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_get_possible_resources (
	ACPI_HANDLE             device_handle,
	ACPI_BUFFER             *ret_buffer)
{
	ACPI_STATUS             status;


	/*
	 *  Must have a valid handle and buffer, So we have to have a handle
	 *  and a return buffer structure, and if there is a non-zero buffer length
	 *  we also need a valid pointer in the buffer. If it's a zero buffer length,
	 *  we'll be returning the needed buffer size, so keep going.
	 */
	if ((!device_handle)        ||
		(!ret_buffer)           ||
		((ret_buffer->length) && (!ret_buffer->pointer))) {
		return (AE_BAD_PARAMETER);
   }

	status = acpi_rs_get_prs_method_data (device_handle, ret_buffer);

	return (status);
}


/*******************************************************************************
 *
 * FUNCTION:    Acpi_set_current_resources
 *
 * PARAMETERS:  Device_handle   - a handle to the device object for the
 *                                device we are changing the resources of
 *              In_buffer       - a pointer to a buffer containing the
 *                                resources to be set for the device
 *
 * RETURN:      Status          - the status of the call
 *
 * DESCRIPTION: This function is called to set the current resources for a
 *              specific device.  The caller must first acquire a handle for
 *              the desired device.  The resource data is passed to the routine
 *              the buffer pointed to by the In_buffer variable.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_set_current_resources (
	ACPI_HANDLE             device_handle,
	ACPI_BUFFER             *in_buffer)
{
	ACPI_STATUS             status;


	/*
	 *  Must have a valid handle and buffer
	 */
	if ((!device_handle)      ||
		(!in_buffer)          ||
		(!in_buffer->pointer) ||
		(!in_buffer->length)) {
		return (AE_BAD_PARAMETER);
	}

	status = acpi_rs_set_srs_method_data (device_handle, in_buffer);

	return (status);
}
