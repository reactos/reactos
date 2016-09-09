/*
 *  acpi_power.c - ACPI Bus Power Management ($Revision: 39 $)
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

/*
 * ACPI power-managed devices may be controlled in two ways:
 * 1. via "Device Specific (D-State) Control"
 * 2. via "Power Resource Control".
 * This module is used to manage devices relying on Power Resource Control.
 *
 * An ACPI "power resource object" describes a software controllable power
 * plane, clock plane, or other resource used by a power managed device.
 * A device may rely on multiple power resources, and a power resource
 * may be shared by multiple devices.
 */

/*
 * Modified for ReactOS and latest ACPICA
 * Copyright (C)2009  Samuel Serapion
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define _COMPONENT		ACPI_POWER_COMPONENT
ACPI_MODULE_NAME		("acpi_power")

#define ACPI_POWER_RESOURCE_STATE_OFF	0x00
#define ACPI_POWER_RESOURCE_STATE_ON	0x01
#define ACPI_POWER_RESOURCE_STATE_UNKNOWN 0xFF

int acpi_power_nocheck;

static int acpi_power_add (struct acpi_device *device);
static int acpi_power_remove (struct acpi_device *device, int type);
static int acpi_power_resume(struct acpi_device *device, int state);

static struct acpi_driver acpi_power_driver = {
	{0,0},
	ACPI_POWER_DRIVER_NAME,
	ACPI_POWER_CLASS,
	0,
	0,
	ACPI_POWER_HID,
	{acpi_power_add, acpi_power_remove, NULL, NULL, acpi_power_resume}
};

struct acpi_power_reference {
	struct list_head node;
	struct acpi_device *device;
};

struct acpi_power_resource
{
	struct acpi_device * device;
	acpi_bus_id		name;
	UINT32			system_level;
	UINT32			order;
	//struct mutex resource_lock;
	struct list_head reference;
};

static struct list_head		acpi_power_resource_list;


/* --------------------------------------------------------------------------
                             Power Resource Management
   -------------------------------------------------------------------------- */

static int
acpi_power_get_context (
	ACPI_HANDLE		handle,
	struct acpi_power_resource **resource)
{
	int			result = 0;
	struct acpi_device	*device = NULL;

	if (!resource)
		return_VALUE(-15);

	result = acpi_bus_get_device(handle, &device);
	if (result) {
		ACPI_DEBUG_PRINT((ACPI_DB_WARN, "Error getting context [%p]\n",
			handle));
		return_VALUE(result);
	}

	*resource = (struct acpi_power_resource *) acpi_driver_data(device);
	if (!*resource)
		return_VALUE(-15);

	return 0;
}


static int
acpi_power_get_state (
	ACPI_HANDLE handle,
	int *state)
{
	ACPI_STATUS		status = AE_OK;
	unsigned long long	sta = 0;
	char node_name[5];
	ACPI_BUFFER buffer = { sizeof(node_name), node_name };


	if (!handle || !state)
		return_VALUE(-1);

	status = acpi_evaluate_integer(handle, "_STA", NULL, &sta);
	if (ACPI_FAILURE(status))
		return_VALUE(-15);

	*state = (sta & 0x01)?ACPI_POWER_RESOURCE_STATE_ON:
			      ACPI_POWER_RESOURCE_STATE_OFF;

	AcpiGetName(handle, ACPI_SINGLE_NAME, &buffer);

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Resource [%s] is %s\n",
		node_name, *state?"on":"off"));

	return 0;
}


static int
acpi_power_get_list_state (
	struct acpi_handle_list	*list,
	int			*state)
{
	int result = 0, state1;
	UINT32 i = 0;

	if (!list || !state)
		return_VALUE(-1);

	/* The state of the list is 'on' IFF all resources are 'on'. */

	for (i=0; i<list->count; i++) {
		/*
		 * The state of the power resource can be obtained by
		 * using the ACPI handle. In such case it is unnecessary to
		 * get the Power resource first and then get its state again.
		 */
		result = acpi_power_get_state(list->handles[i], &state1);
		if (result)
			return result;

		*state = state1;

		if (*state != ACPI_POWER_RESOURCE_STATE_ON)
			break;
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Resource list is %s\n",
		*state?"on":"off"));

	return result;
}


static int
acpi_power_on (
	ACPI_HANDLE handle, struct acpi_device *dev)
{
	int result = 0;
	int found = 0;
	ACPI_STATUS status = AE_OK;
	struct acpi_power_resource *resource = NULL;
	struct list_head *node, *next;
	struct acpi_power_reference *ref;

	result = acpi_power_get_context(handle, &resource);
	if (result)
		return result;

	//mutex_lock(&resource->resource_lock);
	list_for_each_safe(node, next, &resource->reference) {
		ref = container_of(node, struct acpi_power_reference, node);
		if (dev->handle == ref->device->handle) {
			ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] already referenced by resource [%s]\n",
				  dev->pnp.bus_id, resource->name));
			found = 1;
			break;
		}
	}

	if (!found) {
		ref = ExAllocatePoolWithTag(NonPagedPool,sizeof (struct acpi_power_reference),'IPCA');
		if (!ref) {
			ACPI_DEBUG_PRINT((ACPI_DB_INFO, "kmalloc() failed\n"));
			//mutex_unlock(&resource->resource_lock);
			return -1;//-ENOMEM;
		}
		list_add_tail(&ref->node, &resource->reference);
		ref->device = dev;
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] added to resource [%s] references\n",
			  dev->pnp.bus_id, resource->name));
	}
	//mutex_unlock(&resource->resource_lock);

	status = AcpiEvaluateObject(resource->device->handle, "_ON", NULL, NULL);
	if (ACPI_FAILURE(status))
		return_VALUE(-15);

	/* Update the power resource's _device_ power state */
	resource->device->power.state = ACPI_STATE_D0;

	return 0;
}


static int
acpi_power_off_device (
	ACPI_HANDLE handle,
	struct acpi_device *dev)
{
	int result = 0;
	ACPI_STATUS status = AE_OK;
	struct acpi_power_resource *resource = NULL;
	struct list_head *node, *next;
	struct acpi_power_reference *ref;

	result = acpi_power_get_context(handle, &resource);
	if (result)
		return result;

	//mutex_lock(&resource->resource_lock);
	list_for_each_safe(node, next, &resource->reference) {
		ref = container_of(node, struct acpi_power_reference, node);
		if (dev->handle == ref->device->handle) {
			list_del(&ref->node);
			ExFreePool(ref);
			ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] removed from resource [%s] references\n",
			    dev->pnp.bus_id, resource->name));
			break;
		}
	}

	if (!list_empty(&resource->reference)) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Cannot turn resource [%s] off - resource is in use\n",
		    resource->name));
		//mutex_unlock(&resource->resource_lock);
		return 0;
	}
	//mutex_unlock(&resource->resource_lock);

	status = AcpiEvaluateObject(resource->device->handle, "_OFF", NULL, NULL);
	if (ACPI_FAILURE(status))
		return -1;

	/* Update the power resource's _device_ power state */
	resource->device->power.state = ACPI_STATE_D3;

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Resource [%s] turned off\n",
			  resource->name));

	return 0;
}

/**
 * acpi_device_sleep_wake - execute _DSW (Device Sleep Wake) or (deprecated in
 *                          ACPI 3.0) _PSW (Power State Wake)
 * @dev: Device to handle.
 * @enable: 0 - disable, 1 - enable the wake capabilities of the device.
 * @sleep_state: Target sleep state of the system.
 * @dev_state: Target power state of the device.
 *
 * Execute _DSW (Device Sleep Wake) or (deprecated in ACPI 3.0) _PSW (Power
 * State Wake) for the device, if present.  On failure reset the device's
 * wakeup.flags.valid flag.
 *
 * RETURN VALUE:
 * 0 if either _DSW or _PSW has been successfully executed
 * 0 if neither _DSW nor _PSW has been found
 * -ENODEV if the execution of either _DSW or _PSW has failed
 */
int acpi_device_sleep_wake(struct acpi_device *dev,
                           int enable, int sleep_state, int dev_state)
{
	union acpi_object in_arg[3];
	struct acpi_object_list arg_list = { 3, in_arg };
	ACPI_STATUS status = AE_OK;

	/*
	 * Try to execute _DSW first.
	 *
	 * Three agruments are needed for the _DSW object:
	 * Argument 0: enable/disable the wake capabilities
	 * Argument 1: target system state
	 * Argument 2: target device state
	 * When _DSW object is called to disable the wake capabilities, maybe
	 * the first argument is filled. The values of the other two agruments
	 * are meaningless.
	 */
	in_arg[0].Type = ACPI_TYPE_INTEGER;
	in_arg[0].Integer.Value = enable;
	in_arg[1].Type = ACPI_TYPE_INTEGER;
	in_arg[1].Integer.Value = sleep_state;
	in_arg[2].Type = ACPI_TYPE_INTEGER;
	in_arg[2].Integer.Value = dev_state;
	status = AcpiEvaluateObject(dev->handle, "_DSW", &arg_list, NULL);
	if (ACPI_SUCCESS(status)) {
		return 0;
	} else if (status != AE_NOT_FOUND) {
		DPRINT1("_DSW execution failed\n");
		dev->wakeup.flags.valid = 0;
		return -1;
	}

	/* Execute _PSW */
	arg_list.Count = 1;
	in_arg[0].Integer.Value = enable;
	status = AcpiEvaluateObject(dev->handle, "_PSW", &arg_list, NULL);
	if (ACPI_FAILURE(status) && (status != AE_NOT_FOUND)) {
		DPRINT1("_PSW execution failed\n");
		dev->wakeup.flags.valid = 0;
		return -1;
	}

	return 0;
}

/*
 * Prepare a wakeup device, two steps (Ref ACPI 2.0:P229):
 * 1. Power on the power resources required for the wakeup device
 * 2. Execute _DSW (Device Sleep Wake) or (deprecated in ACPI 3.0) _PSW (Power
 *    State Wake) for the device, if present
 */
int acpi_enable_wakeup_device_power(struct acpi_device *dev, int sleep_state)
{
	unsigned int i;
	int err = 0;

	if (!dev || !dev->wakeup.flags.valid)
		return -1;

	//mutex_lock(&acpi_device_lock);

	if (dev->wakeup.prepare_count++)
		goto out;

	/* Open power resource */
	for (i = 0; i < dev->wakeup.resources.count; i++) {
		int ret = acpi_power_on(dev->wakeup.resources.handles[i], dev);
		if (ret) {
			DPRINT( "Transition power state\n");
			dev->wakeup.flags.valid = 0;
			err = -1;
			goto err_out;
		}
	}

	/*
	 * Passing 3 as the third argument below means the device may be placed
	 * in arbitrary power state afterwards.
	 */
	err = acpi_device_sleep_wake(dev, 1, sleep_state, 3);

 err_out:
	if (err)
		dev->wakeup.prepare_count = 0;

 out:
	//mutex_unlock(&acpi_device_lock);
	return err;
}

/*
 * Shutdown a wakeup device, counterpart of above method
 * 1. Execute _DSW (Device Sleep Wake) or (deprecated in ACPI 3.0) _PSW (Power
 *    State Wake) for the device, if present
 * 2. Shutdown down the power resources
 */
int acpi_disable_wakeup_device_power(struct acpi_device *dev)
{
	unsigned int i;
	int err = 0;

	if (!dev || !dev->wakeup.flags.valid)
		return -1;

	//mutex_lock(&acpi_device_lock);

	if (--dev->wakeup.prepare_count > 0)
		goto out;

	/*
	 * Executing the code below even if prepare_count is already zero when
	 * the function is called may be useful, for example for initialisation.
	 */
	if (dev->wakeup.prepare_count < 0)
		dev->wakeup.prepare_count = 0;

	err = acpi_device_sleep_wake(dev, 0, 0, 0);
	if (err)
		goto out;

	/* Close power resource */
	for (i = 0; i < dev->wakeup.resources.count; i++) {
		int ret = acpi_power_off_device(
				dev->wakeup.resources.handles[i], dev);
		if (ret) {
			DPRINT("Transition power state\n");
			dev->wakeup.flags.valid = 0;
			err = -1;
			goto out;
		}
	}

 out:
	//mutex_unlock(&acpi_device_lock);
	return err;
}

/* --------------------------------------------------------------------------
                             Device Power Management
   -------------------------------------------------------------------------- */

int
acpi_power_get_inferred_state (
	struct acpi_device	*device)
{
	int			result = 0;
	struct acpi_handle_list	*list = NULL;
	int			list_state = 0;
	int			i = 0;


	if (!device)
		return_VALUE(-1);

	device->power.state = ACPI_STATE_UNKNOWN;

	/*
	 * We know a device's inferred power state when all the resources
	 * required for a given D-state are 'on'.
	 */
	for (i=ACPI_STATE_D0; i<ACPI_STATE_D3; i++) {
		list = &device->power.states[i].resources;
		if (list->count < 1)
			continue;

		result = acpi_power_get_list_state(list, &list_state);
		if (result)
			return_VALUE(result);

		if (list_state == ACPI_POWER_RESOURCE_STATE_ON) {
			device->power.state = i;
			return_VALUE(0);
		}
	}

	device->power.state = ACPI_STATE_D3;

	return_VALUE(0);
}


int
acpi_power_transition (
	struct acpi_device	*device,
	int			state)
{
	int			result = 0;
	struct acpi_handle_list	*cl = NULL;	/* Current Resources */
	struct acpi_handle_list	*tl = NULL;	/* Target Resources */
	unsigned int	i = 0;

	if (!device || (state < ACPI_STATE_D0) || (state > ACPI_STATE_D3))
		return_VALUE(-1);

	if ((device->power.state < ACPI_STATE_D0) || (device->power.state > ACPI_STATE_D3))
		return_VALUE(-15);

	cl = &device->power.states[device->power.state].resources;
	tl = &device->power.states[state].resources;

	/* TBD: Resources must be ordered. */

	/*
	 * First we reference all power resources required in the target list
	 * (e.g. so the device doesn't lose power while transitioning).
	 */
	for (i = 0; i < tl->count; i++) {
		result = acpi_power_on(tl->handles[i], device);
		if (result)
			goto end;
	}

	if (device->power.state == state) {
		goto end;
	}

	/*
	 * Then we dereference all power resources used in the current list.
	 */
	for (i = 0; i < cl->count; i++) {
		result = acpi_power_off_device(cl->handles[i], device);
		if (result)
			goto end;
	}

     end:
	if (result)
		device->power.state = ACPI_STATE_UNKNOWN;
	else {
	/* We shouldn't change the state till all above operations succeed */
		device->power.state = state;
	}

	return result;
}

/* --------------------------------------------------------------------------
                                Driver Interface
   -------------------------------------------------------------------------- */

int
acpi_power_add (
	struct acpi_device	*device)
{
	int	 result = 0, state;
	ACPI_STATUS		status = AE_OK;
	struct acpi_power_resource *resource = NULL;
	union acpi_object	acpi_object;
	ACPI_BUFFER	buffer = {sizeof(ACPI_OBJECT), &acpi_object};


	if (!device)
		return_VALUE(-1);

	resource = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct acpi_power_resource),'IPCA');
	if (!resource)
		return_VALUE(-4);

	resource->device = device;
	//mutex_init(&resource->resource_lock);
	INIT_LIST_HEAD(&resource->reference);

	strcpy(resource->name, device->pnp.bus_id);
	strcpy(acpi_device_name(device), ACPI_POWER_DEVICE_NAME);
	strcpy(acpi_device_class(device), ACPI_POWER_CLASS);
	device->driver_data = resource;

	/* Evalute the object to get the system level and resource order. */
	status = AcpiEvaluateObject(device->handle, NULL, NULL, &buffer);
	if (ACPI_FAILURE(status)) {
		result = -15;
		goto end;
	}
	resource->system_level = acpi_object.PowerResource.SystemLevel;
	resource->order = acpi_object.PowerResource.ResourceOrder;

	result = acpi_power_get_state(device->handle, &state);
	if (result)
		goto end;

	switch (state) {
	case ACPI_POWER_RESOURCE_STATE_ON:
		device->power.state = ACPI_STATE_D0;
		break;
	case ACPI_POWER_RESOURCE_STATE_OFF:
		device->power.state = ACPI_STATE_D3;
		break;
	default:
		device->power.state = ACPI_STATE_UNKNOWN;
		break;
	}


	DPRINT("%s [%s] (%s)\n", acpi_device_name(device),
		acpi_device_bid(device), state?"on":"off");

end:
	if (result)
		ExFreePool(resource);

	return result;
}


int
acpi_power_remove (
	struct acpi_device	*device,
	int			type)
{
	struct acpi_power_resource *resource = NULL;
	struct list_head *node, *next;

	if (!device || !acpi_driver_data(device))
		return_VALUE(-1);

	resource = acpi_driver_data(device);

	//mutex_lock(&resource->resource_lock);
	list_for_each_safe(node, next, &resource->reference) {
		struct acpi_power_reference *ref = container_of(node, struct acpi_power_reference, node);
		list_del(&ref->node);
		ExFreePool(ref);
	}
	//mutex_unlock(&resource->resource_lock);
	ExFreePool(resource);

	return_VALUE(0);
}

static int acpi_power_resume(struct acpi_device *device, int state)
{
	int result = 0;
	struct acpi_power_resource *resource = NULL;
	struct acpi_power_reference *ref;

	if (!device || !acpi_driver_data(device))
		return -1;

	resource = acpi_driver_data(device);

	result = acpi_power_get_state(device->handle, &state);
	if (result)
		return result;

	//mutex_lock(&resource->resource_lock);
	if (state == ACPI_POWER_RESOURCE_STATE_OFF &&
	    !list_empty(&resource->reference)) {
		ref = container_of(resource->reference.next, struct acpi_power_reference, node);
		//mutex_unlock(&resource->resource_lock);
		result = acpi_power_on(device->handle, ref->device);
		return result;
	}

	//mutex_unlock(&resource->resource_lock);
	return 0;
}

int
acpi_power_init (void)
{
	int			result = 0;

	DPRINT("acpi_power_init\n");

	INIT_LIST_HEAD(&acpi_power_resource_list);


	result = acpi_bus_register_driver(&acpi_power_driver);
	if (result < 0) {
		return_VALUE(-15);
	}

	return_VALUE(0);
}
