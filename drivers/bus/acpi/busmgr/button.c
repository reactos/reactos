/*
 *  acpi_button.c - ACPI Button Driver ($Revision: 29 $)
 *
 *  Copyright (C) 2001, 2002 Andy Grover <andrew.grover@intel.com>
 *  Copyright (C) 2001, 2002 Paul Diefenbaugh <paul.s.diefenbaugh@intel.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define _COMPONENT		ACPI_BUTTON_COMPONENT
ACPI_MODULE_NAME		("acpi_button")


static int acpi_button_add (struct acpi_device *device);
static int acpi_button_remove (struct acpi_device *device, int type);

static struct acpi_driver acpi_button_driver = {
    {0,0},
    ACPI_BUTTON_DRIVER_NAME,
    ACPI_BUTTON_CLASS,
    0,
    0,
    "ACPI_FPB,ACPI_FSB,PNP0C0D,PNP0C0C,PNP0C0E",
    {acpi_button_add,acpi_button_remove}
};

struct acpi_button {
	ACPI_HANDLE		handle;
	struct acpi_device	*device;	/* Fixed button kludge */
	UINT8			type;
	unsigned long		pushed;
};

struct acpi_device *power_button;
struct acpi_device *sleep_button;
struct acpi_device *lid_button;

/* --------------------------------------------------------------------------
                                Driver Interface
   -------------------------------------------------------------------------- */

void
acpi_button_notify (
	ACPI_HANDLE		handle,
	UINT32			event,
	void			*data)
{
	struct acpi_button	*button = (struct acpi_button *) data;

	ACPI_FUNCTION_TRACE("acpi_button_notify");

	if (!button || !button->device)
		return_VOID;

	switch (event) {
	case ACPI_BUTTON_NOTIFY_STATUS:
		acpi_bus_generate_event(button->device, event, ++button->pushed);
		break;
	default:
		ACPI_DEBUG_PRINT((ACPI_DB_INFO,
			"Unsupported event [0x%x]\n", event));
		break;
	}

	return_VOID;
}


ACPI_STATUS
acpi_button_notify_fixed (
	void			*data)
{
	struct acpi_button	*button = (struct acpi_button *) data;

	ACPI_FUNCTION_TRACE("acpi_button_notify_fixed");

	if (!button)
		return_ACPI_STATUS(AE_BAD_PARAMETER);

	acpi_button_notify(button->handle, ACPI_BUTTON_NOTIFY_STATUS, button);

	return_ACPI_STATUS(AE_OK);
}


static int
acpi_button_add (
	struct acpi_device	*device)
{
	int			result = 0;
	ACPI_STATUS		status = AE_OK;
	struct acpi_button	*button = NULL;

	ACPI_FUNCTION_TRACE("acpi_button_add");

	if (!device)
		return_VALUE(-1);

	button = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct acpi_button), 'IPCA');
	if (!button)
		return_VALUE(-4);
	memset(button, 0, sizeof(struct acpi_button));

	button->device = device;
	button->handle = device->handle;
	acpi_driver_data(device) = button;

	/*
	 * Determine the button type (via hid), as fixed-feature buttons
	 * need to be handled a bit differently than generic-space.
	 */
	if (!strcmp(acpi_device_hid(device), ACPI_BUTTON_HID_POWER)) {
		button->type = ACPI_BUTTON_TYPE_POWER;
		sprintf(acpi_device_name(device), "%s",
			ACPI_BUTTON_DEVICE_NAME_POWER);
		sprintf(acpi_device_class(device), "%s/%s",
			ACPI_BUTTON_CLASS, ACPI_BUTTON_SUBCLASS_POWER);
	}
	else if (!strcmp(acpi_device_hid(device), ACPI_BUTTON_HID_POWERF)) {
		button->type = ACPI_BUTTON_TYPE_POWERF;
		sprintf(acpi_device_name(device), "%s",
			ACPI_BUTTON_DEVICE_NAME_POWERF);
		sprintf(acpi_device_class(device), "%s/%s",
			ACPI_BUTTON_CLASS, ACPI_BUTTON_SUBCLASS_POWER);
	}
	else if (!strcmp(acpi_device_hid(device), ACPI_BUTTON_HID_SLEEP)) {
		button->type = ACPI_BUTTON_TYPE_SLEEP;
		sprintf(acpi_device_name(device), "%s",
			ACPI_BUTTON_DEVICE_NAME_SLEEP);
		sprintf(acpi_device_class(device), "%s/%s",
			ACPI_BUTTON_CLASS, ACPI_BUTTON_SUBCLASS_SLEEP);
	}
	else if (!strcmp(acpi_device_hid(device), ACPI_BUTTON_HID_SLEEPF)) {
		button->type = ACPI_BUTTON_TYPE_SLEEPF;
		sprintf(acpi_device_name(device), "%s",
			ACPI_BUTTON_DEVICE_NAME_SLEEPF);
		sprintf(acpi_device_class(device), "%s/%s",
			ACPI_BUTTON_CLASS, ACPI_BUTTON_SUBCLASS_SLEEP);
	}
	else if (!strcmp(acpi_device_hid(device), ACPI_BUTTON_HID_LID)) {
		button->type = ACPI_BUTTON_TYPE_LID;
		sprintf(acpi_device_name(device), "%s",
			ACPI_BUTTON_DEVICE_NAME_LID);
		sprintf(acpi_device_class(device), "%s/%s",
			ACPI_BUTTON_CLASS, ACPI_BUTTON_SUBCLASS_LID);
	}
	else {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR, "Unsupported hid [%s]\n",
			acpi_device_hid(device)));
		result = -15;
		goto end;
	}

	/*
	 * Ensure only one button of each type is used.
	 */
	switch (button->type) {
	case ACPI_BUTTON_TYPE_POWER:
	case ACPI_BUTTON_TYPE_POWERF:
		if (!power_button)
			power_button = device;
		else {
			ExFreePoolWithTag(button, 'IPCA');
			return_VALUE(-15);
		}
		break;
	case ACPI_BUTTON_TYPE_SLEEP:
	case ACPI_BUTTON_TYPE_SLEEPF:
		if (!sleep_button)
			sleep_button = device;
		else {
			ExFreePoolWithTag(button, 'IPCA');
			return_VALUE(-15);
		}
		break;
	case ACPI_BUTTON_TYPE_LID:
		if (!lid_button)
			lid_button = device;
		else {
			ExFreePoolWithTag(button, 'IPCA');
			return_VALUE(-15);
		}
		break;
	}

	switch (button->type) {
	case ACPI_BUTTON_TYPE_POWERF:
		status = AcpiInstallFixedEventHandler (
			ACPI_EVENT_POWER_BUTTON,
			acpi_button_notify_fixed,
			button);
		break;
	case ACPI_BUTTON_TYPE_SLEEPF:
		status = AcpiInstallFixedEventHandler (
			ACPI_EVENT_SLEEP_BUTTON,
			acpi_button_notify_fixed,
			button);
		break;
	case ACPI_BUTTON_TYPE_LID:
		status = AcpiInstallFixedEventHandler (
			ACPI_BUTTON_TYPE_LID,
			acpi_button_notify_fixed,
			button);
		break;
	default:
		status = AcpiInstallNotifyHandler (
			button->handle,
			ACPI_DEVICE_NOTIFY,
			acpi_button_notify,
			button);
		break;
	}

	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
			"Error installing notify handler\n"));
		result = -15;
		goto end;
	}

	DPRINT("%s [%s]\n",
		acpi_device_name(device), acpi_device_bid(device));

end:
	if (result) {
		ExFreePoolWithTag(button, 'IPCA');
	}

	return_VALUE(result);
}


static int
acpi_button_remove (struct acpi_device *device, int type)
{
	ACPI_STATUS		status = 0;
	struct acpi_button	*button = NULL;

	ACPI_FUNCTION_TRACE("acpi_button_remove");

	if (!device || !acpi_driver_data(device))
		return_VALUE(-1);

	button = acpi_driver_data(device);

	/* Unregister for device notifications. */
	switch (button->type) {
	case ACPI_BUTTON_TYPE_POWERF:
		status = AcpiRemoveFixedEventHandler(
			ACPI_EVENT_POWER_BUTTON, acpi_button_notify_fixed);
		break;
	case ACPI_BUTTON_TYPE_SLEEPF:
		status = AcpiRemoveFixedEventHandler(
			ACPI_EVENT_SLEEP_BUTTON, acpi_button_notify_fixed);
		break;
	case ACPI_BUTTON_TYPE_LID:
		status = AcpiRemoveFixedEventHandler(
			ACPI_BUTTON_TYPE_LID, acpi_button_notify_fixed);
		break;
	default:
		status = AcpiRemoveNotifyHandler(button->handle,
			ACPI_DEVICE_NOTIFY, acpi_button_notify);
		break;
	}

	if (ACPI_FAILURE(status))
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
			"Error removing notify handler\n"));

	ExFreePoolWithTag(button, 'IPCA');

	return_VALUE(0);
}


int
acpi_button_init (void)
{
	int			result = 0;

	ACPI_FUNCTION_TRACE("acpi_button_init");

	result = acpi_bus_register_driver(&acpi_button_driver);
	if (result < 0) {
		return_VALUE(-15);
	}

	return_VALUE(0);
}


void
acpi_button_exit (void)
{
	ACPI_FUNCTION_TRACE("acpi_button_exit");

	acpi_bus_unregister_driver(&acpi_button_driver);

	return_VOID;
}


