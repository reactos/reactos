/*****************************************************************************
 *
 * Module Name: bn.c
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
 *  Foundation, Inc., 59 Temple Plxxe, Suite 330, Boston, MA  02111-1307  USA
 */

#include <acpi.h>
#include "bn.h"


#define _COMPONENT		ACPI_BUTTON
	MODULE_NAME		("bn")


static struct proc_dir_entry	*bn_proc_root = NULL;


/*****************************************************************************
 *                            Internal Functions
 *****************************************************************************/

/*****************************************************************************
 *
 * FUNCTION:	bn_print
 *
 * PARAMETERS:	<TBD>
 *
 * RETURN:	<TBD>
 *
 * DESCRIPTION: Prints out information on a specific button.
 *
 ****************************************************************************/

void
bn_print (
	BN_CONTEXT		*button)
{
#ifdef ACPI_DEBUG
	ACPI_BUFFER		buffer;
#endif /*ACPI_DEBUG*/

	if (!button) {
		return;
	}

	switch (button->type) {

	case BN_TYPE_POWER_BUTTON:
	case BN_TYPE_POWER_BUTTON_FIXED:
		acpi_os_printf("Power Button: found\n");
		break;

	case BN_TYPE_SLEEP_BUTTON:
	case BN_TYPE_SLEEP_BUTTON_FIXED:
		acpi_os_printf("Sleep Button: found\n");
		break;

	case BN_TYPE_LID_SWITCH:
		acpi_os_printf("Lid Switch: found\n");
		break;
	}

#ifdef ACPI_DEBUG
	buffer.length = 256;
	buffer.pointer = acpi_os_callocate(buffer.length);
	if (!buffer.pointer) {
		return;
	}

	/*
	 * Get the full pathname for this ACPI object.
	 */
	acpi_get_name(button->acpi_handle, ACPI_FULL_PATHNAME, &buffer);

	/*
	 * Print out basic button information.
	 */
	DEBUG_PRINT(ACPI_INFO, ("+------------------------------------------------------------\n"));

	switch (button->type) {

	case BN_TYPE_POWER_BUTTON:
	case BN_TYPE_POWER_BUTTON_FIXED:
		DEBUG_PRINT(ACPI_INFO, ("| PowerButton[0x%02x]|[%p] %s\n", button->device_handle, button->acpi_handle, buffer.pointer));
		break;

	case BN_TYPE_SLEEP_BUTTON:
	case BN_TYPE_SLEEP_BUTTON_FIXED:
		DEBUG_PRINT(ACPI_INFO, ("| SleepButton[0x%02x]|[%p] %s\n", button->device_handle, button->acpi_handle, buffer.pointer));
		break;

	case BN_TYPE_LID_SWITCH:
		DEBUG_PRINT(ACPI_INFO, ("| LidSwitch[0x%02x]|[%p] %s\n", button->device_handle, button->acpi_handle, buffer.pointer));
		break;
	}

	DEBUG_PRINT(ACPI_INFO, ("+------------------------------------------------------------\n"));

	acpi_os_free(buffer.pointer);
#endif /*ACPI_DEBUG*/

	return;
}


/****************************************************************************
 *
 * FUNCTION:	bn_add_device
 *
 * PARAMETERS:	<TBD>
 *
 * RETURN:	<TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bn_add_device(
	BM_HANDLE		device_handle,
	void			**context)
{
	ACPI_STATUS		status = AE_OK;
	BM_DEVICE		*device = NULL;
	BN_CONTEXT		*button = NULL;

	FUNCTION_TRACE("bn_add_device");

	DEBUG_PRINT(ACPI_INFO, ("Adding button device [0x%02x].\n", device_handle));

	if (!context || *context) {
		DEBUG_PRINT(ACPI_ERROR, ("Invalid context.\n"));
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Get information on this device.
	 */
	status = bm_get_device_info( device_handle, &device );
	if (ACPI_FAILURE(status)) {
		return_ACPI_STATUS(status);
	}

	/*
	 * Allocate a new BN_CONTEXT structure.
	 */
	button = acpi_os_callocate(sizeof(BN_CONTEXT));
	if (!button) {
		return_ACPI_STATUS(AE_NO_MEMORY);
	}

	button->device_handle = device->handle;
	button->acpi_handle = device->acpi_handle;

	/*
	 * Power Button?
	 * -------------
	 * Either fixed-feature or generic (namespace) types.
	 */
	if (strncmp(device->id.hid, BN_HID_POWER_BUTTON, 
		sizeof(BM_DEVICE_HID)) == 0) {

		if (device->id.type == BM_TYPE_FIXED_BUTTON) {

			button->type = BN_TYPE_POWER_BUTTON_FIXED;

			/* Register for fixed-feature events. */
			status = acpi_install_fixed_event_handler(
				ACPI_EVENT_POWER_BUTTON, bn_notify_fixed, 
				(void*)button);
		}
		else {
			button->type = BN_TYPE_POWER_BUTTON;
		}

		//proc_mkdir(BN_PROC_POWER_BUTTON, bn_proc_root);

	}

	/*
	 * Sleep Button?
	 * -------------
	 * Either fixed-feature or generic (namespace) types.
	 */
	else if (strncmp( device->id.hid, BN_HID_SLEEP_BUTTON, 
		sizeof(BM_DEVICE_HID)) == 0) {

		if (device->id.type == BM_TYPE_FIXED_BUTTON) {

			button->type = BN_TYPE_SLEEP_BUTTON_FIXED;

			/* Register for fixed-feature events. */
			status = acpi_install_fixed_event_handler(
				ACPI_EVENT_SLEEP_BUTTON, bn_notify_fixed, 
				(void*)button);
		}
		else {
			button->type = BN_TYPE_SLEEP_BUTTON;
		}

		//proc_mkdir(BN_PROC_SLEEP_BUTTON, bn_proc_root);
	}

	/*
	 * LID Switch?
	 * -----------
	 */
	else if (strncmp( device->id.hid, BN_HID_LID_SWITCH, 
		sizeof(BM_DEVICE_HID)) == 0) {

		button->type = BN_TYPE_LID_SWITCH;

		//proc_mkdir(BN_PROC_LID_SWITCH, bn_proc_root);
	}

	*context = button;

	bn_print(button);

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:	bn_remove_device
 *
 * PARAMETERS:	<TBD>
 *
 * RETURN:	<TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bn_remove_device(
	void			**context)
{
	ACPI_STATUS		status = AE_OK;
	BN_CONTEXT		*button = NULL;

	FUNCTION_TRACE("bn_remove_device");

	if (!context || !*context) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	button = (BN_CONTEXT*)*context;

	DEBUG_PRINT(ACPI_INFO, ("Removing button device [0x%02x].\n", button->device_handle));

	/*
	 * Remove the /proc entry for this button.
	 */
	switch (button->type) {

	case BN_TYPE_POWER_BUTTON:
	case BN_TYPE_POWER_BUTTON_FIXED:
		/* Unregister for fixed-feature events. */
		status = acpi_remove_fixed_event_handler(
			ACPI_EVENT_POWER_BUTTON, bn_notify_fixed);
		//remove_proc_entry(BN_PROC_POWER_BUTTON, bn_proc_root);
		break;

	case BN_TYPE_SLEEP_BUTTON:
	case BN_TYPE_SLEEP_BUTTON_FIXED:
		/* Unregister for fixed-feature events. */
		status = acpi_remove_fixed_event_handler(
			ACPI_EVENT_SLEEP_BUTTON, bn_notify_fixed);
		//remove_proc_entry(BN_PROC_SLEEP_BUTTON, bn_proc_root);
		break;

	case BN_TYPE_LID_SWITCH:
		//remove_proc_entry(BN_PROC_LID_SWITCH, bn_proc_root);
		break;
	}

	acpi_os_free(button);

	*context = NULL;

	return_ACPI_STATUS(status);
}


/*****************************************************************************
 *			      External Functions
 *****************************************************************************/

/*****************************************************************************
 *
 * FUNCTION:	bn_initialize
 *
 * PARAMETERS:	<none>
 *
 * RETURN:
 *
 * DESCRIPTION: <TBD>
 *

 ****************************************************************************/

ACPI_STATUS
bn_initialize (void)
{
	ACPI_STATUS		status = AE_OK;
	BM_DEVICE_ID		criteria;
	BM_DRIVER		driver;

	FUNCTION_TRACE("bn_initialize");

	MEMSET(&criteria, 0, sizeof(BM_DEVICE_ID));
	MEMSET(&driver, 0, sizeof(BM_DRIVER));

	driver.notify = &bn_notify;
	driver.request = &bn_request;

	/*
	 * Create button's root /proc entry.
	 */
	//bn_proc_root = proc_mkdir(BN_PROC_ROOT, bm_proc_root);
	//if (!bn_proc_root) {
//		return_ACPI_STATUS(AE_ERROR);
//	}

	/*
	 * Register for power buttons.
	 */
	MEMCPY(criteria.hid, BN_HID_POWER_BUTTON, sizeof(BN_HID_POWER_BUTTON));
	status = bm_register_driver(&criteria, &driver);

	/*
	 * Register for sleep buttons.
	 */
	MEMCPY(criteria.hid, BN_HID_SLEEP_BUTTON, sizeof(BN_HID_SLEEP_BUTTON));
	status = bm_register_driver(&criteria, &driver);

	/*
	 * Register for LID switches.
	 */
	MEMCPY(criteria.hid, BN_HID_LID_SWITCH, sizeof(BN_HID_LID_SWITCH));
	status = bm_register_driver(&criteria, &driver);

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:	bn_terminate
 *
 * PARAMETERS:	<none>
 *
 * RETURN:	<TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bn_terminate (void)
{
	ACPI_STATUS		status = AE_OK;
	BM_DEVICE_ID		criteria;
	BM_DRIVER		driver;

	FUNCTION_TRACE("bn_terminate");

	MEMSET(&criteria, 0, sizeof(BM_DEVICE_ID));
	MEMSET(&driver, 0, sizeof(BM_DRIVER));

	driver.notify = &bn_notify;
	driver.request = &bn_request;

	/*
	 * Unregister for power buttons.
	 */
	MEMCPY(criteria.hid, BN_HID_POWER_BUTTON, sizeof(BN_HID_POWER_BUTTON));
	status = bm_unregister_driver(&criteria, &driver);

	/*
	 * Unregister for sleep buttons.
	 */
	MEMCPY(criteria.hid, BN_HID_SLEEP_BUTTON, sizeof(BN_HID_SLEEP_BUTTON));
	status = bm_unregister_driver(&criteria, &driver);

	/*
	 * Unregister for LID switches.
	 */
	MEMCPY(criteria.hid, BN_HID_LID_SWITCH, sizeof(BN_HID_LID_SWITCH));
	status = bm_unregister_driver(&criteria, &driver);

	/*
	 * Remove button's root /proc entry.
	 */
	if (bn_proc_root) {
		//remove_proc_entry(BN_PROC_ROOT, bm_proc_root);
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:	bn_notify_fixed
 *
 * PARAMETERS:	<none>
 *
 * RETURN:
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bn_notify_fixed (
	void			*context)
{
	ACPI_STATUS		status = AE_OK;
	BN_CONTEXT		*button = NULL;

	FUNCTION_TRACE("bn_notify_fixed");

	if (!context) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	button = (BN_CONTEXT*)context;

  DbgPrint("Fixed button status change event detected.\n");

	switch (button->type) {

	case BN_TYPE_POWER_BUTTON_FIXED:
		DEBUG_PRINT(ACPI_INFO, ("Fixed-feature button status change event detected.\n"));
		/*bm_generate_event(button->device_handle, BN_PROC_ROOT, 
			BN_PROC_POWER_BUTTON, BN_NOTIFY_STATUS_CHANGE, 0);*/
		break;

	case BN_TYPE_SLEEP_BUTTON_FIXED:
		DEBUG_PRINT(ACPI_INFO, ("Fixed-feature button status change event detected.\n"));
		/*bm_generate_event(button->device_handle, BN_PROC_ROOT, 
			BN_PROC_SLEEP_BUTTON, BN_NOTIFY_STATUS_CHANGE, 0);*/
		break;

	default:
		DEBUG_PRINT(ACPI_INFO, ("Unsupported fixed-feature event detected.\n"));
		status = AE_SUPPORT;
		break;
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:	bn_notify
 *
 * PARAMETERS:	<none>
 *
 * RETURN:
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bn_notify (
	BM_NOTIFY		notify_type,
	BM_HANDLE		device_handle,
	void			**context)
{
	ACPI_STATUS		status = AE_OK;

	FUNCTION_TRACE("bn_notify");

	if (!context) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	switch (notify_type) {
	case BM_NOTIFY_DEVICE_ADDED:
		status = bn_add_device(device_handle, context);
		break;
		
	case BM_NOTIFY_DEVICE_REMOVED:
		status = bn_remove_device(context);
		break;
		
	case BN_NOTIFY_STATUS_CHANGE:
		DEBUG_PRINT(ACPI_INFO, ("Button status change event detected.\n"));

    DbgPrint("Button status change event detected.\n");

		if (!context || !*context) {
			return_ACPI_STATUS(AE_BAD_PARAMETER);
		}

		switch(((BN_CONTEXT*)*context)->type) {

		case BN_TYPE_POWER_BUTTON:
		case BN_TYPE_POWER_BUTTON_FIXED:
			/*bm_generate_event(device_handle, BN_PROC_ROOT, 
				BN_PROC_POWER_BUTTON, notify_type, 0);*/
			break;

		case BN_TYPE_SLEEP_BUTTON:
		case BN_TYPE_SLEEP_BUTTON_FIXED:
			/*bm_generate_event(device_handle, BN_PROC_ROOT,
				BN_PROC_SLEEP_BUTTON, notify_type, 0);*/
			break;

		case BN_TYPE_LID_SWITCH:
			/*bm_generate_event(device_handle, BN_PROC_ROOT,
				BN_PROC_LID_SWITCH, notify_type, 0);*/
			break;

		default:
			status = AE_SUPPORT;
			break;
		}

		break;

	default:
		status = AE_SUPPORT;
		break;
	}

	return_ACPI_STATUS(status);
}


/****************************************************************************
 *
 * FUNCTION:	bn_request
 *
 * PARAMETERS:	<TBD>
 *
 * RETURN:	<TBD>
 *
 * DESCRIPTION: <TBD>
 *
 ****************************************************************************/

ACPI_STATUS
bn_request (
	BM_REQUEST		*request,
	void			*context)
{
	ACPI_STATUS		status = AE_OK;

	FUNCTION_TRACE("bn_request");

	/*
	 * Must have a valid request structure and context.
	 */
	if (!request || !context) {
		return_ACPI_STATUS(AE_BAD_PARAMETER);
	}

	/*
	 * Handle Request:
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
