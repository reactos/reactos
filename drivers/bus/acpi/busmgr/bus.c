/*
 *  acpi_bus.c - ACPI Bus Driver ($Revision: 80 $)
 *
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
  * Modified for ReactOS and latest ACPICA
  * Copyright (C)2009  Samuel Serapion
  */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define _COMPONENT		ACPI_BUS_COMPONENT
ACPI_MODULE_NAME		("acpi_bus")

#define WALK_UP			0
#define WALK_DOWN		1

#define STRUCT_TO_INT(s)	(*((int*)&s))
#define HAS_CHILDREN(d)		((d)->children.next != &((d)->children))
#define HAS_SIBLINGS(d)		(((d)->parent) && ((d)->node.next != &(d)->parent->children))
#define NODE_TO_DEVICE(n)	(list_entry(n, struct acpi_device, node))

int			event_is_open;
extern void acpi_pic_sci_set_trigger(unsigned int irq, UINT16 trigger);

typedef int (*acpi_bus_walk_callback)(struct acpi_device*, int, void*);

struct acpi_device		*acpi_root;
KSPIN_LOCK	acpi_bus_event_lock;
LIST_HEAD(acpi_bus_event_list);
//DECLARE_WAIT_QUEUE_HEAD(acpi_bus_event_queue);
KEVENT AcpiEventQueue;
KDPC event_dpc;

int ProcessorCount, PowerDeviceCount, PowerButtonCount, FixedPowerButtonCount;
int FixedSleepButtonCount, SleepButtonCount, ThermalZoneCount;

static int
acpi_device_register (
	struct acpi_device	*device,
	struct acpi_device	*parent)
{
	int			result = 0;

	if (!device)
		return_VALUE(AE_BAD_PARAMETER);

	return_VALUE(result);
}


static int
acpi_device_unregister (
	struct acpi_device	*device)
{
	if (!device)
		return_VALUE(AE_BAD_PARAMETER);

#ifdef CONFIG_LDM
	put_device(&device->dev);
#endif /*CONFIG_LDM*/

	return_VALUE(0);
}


/* --------------------------------------------------------------------------
                                Device Management
   -------------------------------------------------------------------------- */

void
acpi_bus_data_handler (
	ACPI_HANDLE		handle,
	void			*context)
{
	DPRINT1("acpi_bus_data_handler not implemented\n");

	/* TBD */

	return;
}


int
acpi_bus_get_device (
	ACPI_HANDLE		handle,
	struct acpi_device	**device)
{
	ACPI_STATUS		status = AE_OK;

	if (!device)
		return_VALUE(AE_BAD_PARAMETER);

	/* TBD: Support fixed-feature devices */

	status = AcpiGetData(handle, acpi_bus_data_handler, (void**)device);
	if (ACPI_FAILURE(status) || !*device) {
		DPRINT( "Error getting context for object [%p]\n",
			handle);
		return_VALUE(AE_NOT_FOUND);
	}

	return 0;
}

ACPI_STATUS acpi_bus_get_status_handle(ACPI_HANDLE handle,
				       unsigned long long *sta)
{
	ACPI_STATUS status;

	status = acpi_evaluate_integer(handle, "_STA", NULL, sta);
	if (ACPI_SUCCESS(status))
		return AE_OK;

	if (status == AE_NOT_FOUND) {
		*sta = ACPI_STA_DEVICE_PRESENT | ACPI_STA_DEVICE_ENABLED |
		       ACPI_STA_DEVICE_UI      | ACPI_STA_DEVICE_FUNCTIONING;
		return AE_OK;
	}
	return status;
}

int
acpi_bus_get_status (
	struct acpi_device	*device)
{
	ACPI_STATUS status;
	unsigned long long sta;

	status = acpi_bus_get_status_handle(device->handle, &sta);
	if (ACPI_FAILURE(status))
		return -1;

	STRUCT_TO_INT(device->status) = (int) sta;

	if (device->status.functional && !device->status.present) {
		ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] status [%08x]: "
		       "functional but not present;\n",
			device->pnp.bus_id,
			(UINT32) STRUCT_TO_INT(device->status)));
	}

	ACPI_DEBUG_PRINT((ACPI_DB_INFO, "Device [%s] status [%08x]\n",
			  device->pnp.bus_id,
			  (UINT32) STRUCT_TO_INT(device->status)));
	return 0;
}

void acpi_bus_private_data_handler(ACPI_HANDLE handle,
				   void *context)
{
	return;
}

int acpi_bus_get_private_data(ACPI_HANDLE handle, void **data)
{
	ACPI_STATUS status = AE_OK;

	if (!*data)
		return -1;

	status = AcpiGetData(handle, acpi_bus_private_data_handler, data);
	if (ACPI_FAILURE(status) || !*data) {
		DPRINT("No context for object [%p]\n", handle);
		return -1;
	}

	return 0;
}
/* --------------------------------------------------------------------------
                                 Power Management
   -------------------------------------------------------------------------- */

int
acpi_bus_get_power (
	ACPI_HANDLE		handle,
	int			*state)
{
	int			result = 0;
	ACPI_STATUS             status = 0;
	struct acpi_device	*device = NULL;
	unsigned long long		psc = 0;

	result = acpi_bus_get_device(handle, &device);
	if (result)
		return_VALUE(result);

	*state = ACPI_STATE_UNKNOWN;

	if (!device->flags.power_manageable) {
		/* TBD: Non-recursive algorithm for walking up hierarchy */
		if (device->parent)
			*state = device->parent->power.state;
		else
			*state = ACPI_STATE_D0;
	}
	else {
		/*
		 * Get the device's power state either directly (via _PSC) or
		 * indirectly (via power resources).
		 */
		if (device->power.flags.explicit_get) {
			status = acpi_evaluate_integer(device->handle, "_PSC",
				NULL, &psc);
			if (ACPI_FAILURE(status))
				return_VALUE(AE_NOT_FOUND);
			device->power.state = (int) psc;
		}
		else if (device->power.flags.power_resources) {
			result = acpi_power_get_inferred_state(device);
			if (result)
				return_VALUE(result);
		}

		*state = device->power.state;
	}

	DPRINT("Device [%s] power state is D%d\n",
		device->pnp.bus_id, device->power.state);

	return_VALUE(0);
}


int
acpi_bus_set_power (
	ACPI_HANDLE		handle,
	int			state)
{
	int			result = 0;
	ACPI_STATUS		status = AE_OK;
	struct acpi_device	*device = NULL;
	char			object_name[5] = {'_','P','S','0'+state,'\0'};


	result = acpi_bus_get_device(handle, &device);
	if (result)
		return_VALUE(result);

	if ((state < ACPI_STATE_D0) || (state > ACPI_STATE_D3))
		return_VALUE(AE_BAD_PARAMETER);

	/* Make sure this is a valid target state */

	if (!device->flags.power_manageable) {
		DPRINT1( "Device is not power manageable\n");
		return_VALUE(AE_NOT_FOUND);
	}
	/*
	 * Get device's current power state
	 */
	//if (!acpi_power_nocheck) {
		/*
		 * Maybe the incorrect power state is returned on the bogus
		 * bios, which is different with the real power state.
		 * For example: the bios returns D0 state and the real power
		 * state is D3. OS expects to set the device to D0 state. In
		 * such case if OS uses the power state returned by the BIOS,
		 * the device can't be transisted to the correct power state.
		 * So if the acpi_power_nocheck is set, it is unnecessary to
		 * get the power state by calling acpi_bus_get_power.
		 */
		acpi_bus_get_power(device->handle, &device->power.state);
	//}

	if ((state == device->power.state) && !device->flags.force_power_state) {
		DPRINT1("Device is already at D%d\n", state);
		return 0;
	}
	if (!device->power.states[state].flags.valid) {
		DPRINT1( "Device does not support D%d\n", state);
		return AE_NOT_FOUND;
	}
	if (device->parent && (state < device->parent->power.state)) {
		DPRINT1( "Cannot set device to a higher-powered state than parent\n");
		return AE_NOT_FOUND;
	}

	/*
	 * Transition Power
	 * ----------------
	 * On transitions to a high-powered state we first apply power (via
	 * power resources) then evalute _PSx.  Conversly for transitions to
	 * a lower-powered state.
	 */
	if (state < device->power.state) {
		if (device->power.flags.power_resources) {
			result = acpi_power_transition(device, state);
			if (result)
				goto end;
		}
		if (device->power.states[state].flags.explicit_set) {
			status = AcpiEvaluateObject(device->handle,
				object_name, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				result = AE_NOT_FOUND;
				goto end;
			}
		}
	}
	else {
		if (device->power.states[state].flags.explicit_set) {
			status = AcpiEvaluateObject(device->handle,
				object_name, NULL, NULL);
			if (ACPI_FAILURE(status)) {
				result = AE_NOT_FOUND;
				goto end;
			}
		}
		if (device->power.flags.power_resources) {
			result = acpi_power_transition(device, state);
			if (result)
				goto end;
		}
	}

end:
	if (result)
		DPRINT( "Error transitioning device [%s] to D%d\n",
			device->pnp.bus_id, state);
	else
		DPRINT("Device [%s] transitioned to D%d\n",
			device->pnp.bus_id, state);

	return result;
}

BOOLEAN acpi_bus_power_manageable(ACPI_HANDLE handle)
{
	struct acpi_device *device;
	int result;

	result = acpi_bus_get_device(handle, &device);
	return result ? 0 : device->flags.power_manageable;
}

BOOLEAN acpi_bus_can_wakeup(ACPI_HANDLE handle)
{
	struct acpi_device *device;
	int result;

	result = acpi_bus_get_device(handle, &device);
	return result ? 0 : device->wakeup.flags.valid;
}

static int
acpi_bus_get_power_flags (
	struct acpi_device	*device)
{
	ACPI_STATUS             status = 0;
	ACPI_HANDLE		handle = 0;
	UINT32                     i = 0;

	if (!device)
		return AE_NOT_FOUND;

	/*
	 * Power Management Flags
	 */
	status = AcpiGetHandle(device->handle, "_PSC", &handle);
	if (ACPI_SUCCESS(status))
		device->power.flags.explicit_get = 1;
	status = AcpiGetHandle(device->handle, "_IRC", &handle);
	if (ACPI_SUCCESS(status))
		device->power.flags.inrush_current = 1;
	status = AcpiGetHandle(device->handle, "_PRW", &handle);
	if (ACPI_SUCCESS(status))
		device->flags.wake_capable = 1;

	/*
	 * Enumerate supported power management states
	 */
	for (i = ACPI_STATE_D0; i <= ACPI_STATE_D3; i++) {
		struct acpi_device_power_state *ps = &device->power.states[i];
		char		object_name[5] = {'_','P','R','0'+i,'\0'};

		/* Evaluate "_PRx" to se if power resources are referenced */
		status = acpi_evaluate_reference(device->handle, object_name, NULL,
			&ps->resources);
		if (ACPI_SUCCESS(status) && ps->resources.count) {
			device->power.flags.power_resources = 1;
			ps->flags.valid = 1;
		}

		/* Evaluate "_PSx" to see if we can do explicit sets */
		object_name[2] = 'S';
		status = AcpiGetHandle(device->handle, object_name, &handle);
		if (ACPI_SUCCESS(status)) {
			ps->flags.explicit_set = 1;
			ps->flags.valid = 1;
		}

		/* State is valid if we have some power control */
		if (ps->resources.count || ps->flags.explicit_set)
			ps->flags.valid = 1;

		ps->power = -1;		/* Unknown - driver assigned */
		ps->latency = -1;	/* Unknown - driver assigned */
	}

	/* Set defaults for D0 and D3 states (always valid) */
	device->power.states[ACPI_STATE_D0].flags.valid = 1;
	device->power.states[ACPI_STATE_D0].power = 100;
	device->power.states[ACPI_STATE_D3].flags.valid = 1;
	device->power.states[ACPI_STATE_D3].power = 0;

	device->power.state = ACPI_STATE_UNKNOWN;

	return 0;
}

/* --------------------------------------------------------------------------
                              Performance Management
   -------------------------------------------------------------------------- */

static int
acpi_bus_get_perf_flags (
	struct acpi_device	*device)
{
	if (!device)
		return AE_NOT_FOUND;

	device->performance.state = ACPI_STATE_UNKNOWN;

	return 0;
}


/* --------------------------------------------------------------------------
                                Event Management
   -------------------------------------------------------------------------- */

void
NTAPI
acpi_bus_generate_event_dpc(PKDPC Dpc,
                            PVOID DeferredContext,
                            PVOID SystemArgument1,
                            PVOID SystemArgument2)
{
	struct acpi_bus_event *event;
    struct acpi_device *device = SystemArgument1;
    ULONG_PTR TypeData = (ULONG_PTR)SystemArgument2;
	KIRQL OldIrql;

    event = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct acpi_bus_event), 'epcA');
	if (!event)
		return;

	sprintf(event->device_class, "%s", device->pnp.device_class);
	sprintf(event->bus_id, "%s", device->pnp.bus_id);
	event->type = (TypeData & 0xFF000000) >> 24;
	event->data = (TypeData & 0x00FFFFFF);

	KeAcquireSpinLock(&acpi_bus_event_lock, &OldIrql);
	list_add_tail(&event->node, &acpi_bus_event_list);
	KeReleaseSpinLock(&acpi_bus_event_lock, OldIrql);

	KeSetEvent(&AcpiEventQueue, IO_NO_INCREMENT, FALSE);
}

int
acpi_bus_generate_event (
	struct acpi_device	*device,
	UINT8			type,
	int			data)
{
    ULONG_PTR TypeData = 0;

	DPRINT("acpi_bus_generate_event\n");

	if (!device)
		return_VALUE(AE_BAD_PARAMETER);

	/* drop event on the floor if no one's listening */
	if (!event_is_open)
		return_VALUE(0);

    /* Data shouldn't even get near 24 bits */
    ASSERT(!(data & 0xFF000000));

    TypeData = data;
    TypeData |= type << 24;

	KeInsertQueueDpc(&event_dpc, device, (PVOID)TypeData);

	return_VALUE(0);
}

int
acpi_bus_receive_event (
	struct acpi_bus_event	*event)
{
//	unsigned long		flags = 0;
	struct acpi_bus_event	*entry = NULL;
	KIRQL OldIrql;

	//DECLARE_WAITQUEUE(wait, current);

	DPRINT("acpi_bus_receive_event\n");

	if (!event)
		return AE_BAD_PARAMETER;

	event_is_open++;
	KeWaitForSingleObject(&AcpiEventQueue,
			      Executive,
			      KernelMode,
			      FALSE,
			      NULL);
	event_is_open--;
	KeClearEvent(&AcpiEventQueue);

	if (list_empty(&acpi_bus_event_list))
		return_VALUE(AE_NOT_FOUND);

//	spin_lock_irqsave(&acpi_bus_event_lock, flags);
	KeAcquireSpinLock(&acpi_bus_event_lock, &OldIrql);
	entry = list_entry(acpi_bus_event_list.next, struct acpi_bus_event, node);
	if (entry)
		list_del(&entry->node);
	KeReleaseSpinLock(&acpi_bus_event_lock, OldIrql);
//	spin_unlock_irqrestore(&acpi_bus_event_lock, flags);

	if (!entry)
		return_VALUE(AE_NOT_FOUND);

	memcpy(event, entry, sizeof(struct acpi_bus_event));

	ExFreePoolWithTag(entry, 'epcA');
	return_VALUE(0);
}


/* --------------------------------------------------------------------------
                               Namespace Management
   -------------------------------------------------------------------------- */


/**
 * acpi_bus_walk
 * -------------
 * Used to walk the ACPI Bus's device namespace.  Can walk down (depth-first)
 * or up.  Able to parse starting at any node in the namespace.  Note that a
 * callback return value of -249 will terminate the walk.
 *
 * @start:	starting point
 * callback:	function to call for every device encountered while parsing
 * direction:	direction to parse (up or down)
 * @data:	context for this search operation
 */
static int
acpi_bus_walk (
	struct acpi_device	*start,
	acpi_bus_walk_callback	callback,
	int			direction,
	void			*data)
{
	int			result = 0;
	int			level = 0;
	struct acpi_device	*device = NULL;

	if (!start || !callback)
		return AE_BAD_PARAMETER;

	device = start;

	/*
	 * Parse Namespace
	 * ---------------
	 * Parse a given subtree (specified by start) in the given direction.
	 * Walking 'up' simply means that we execute the callback on leaf
	 * devices prior to their parents (useful for things like removing
	 * or powering down a subtree).
	 */

	while (device) {

		if (direction == WALK_DOWN)
			if (-249 == callback(device, level, data))
				break;

		/* Depth First */

		if (HAS_CHILDREN(device)) {
			device = NODE_TO_DEVICE(device->children.next);
			++level;
			continue;
		}

		if (direction == WALK_UP)
			if (-249 == callback(device, level, data))
				break;

		/* Now Breadth */

		if (HAS_SIBLINGS(device)) {
			device = NODE_TO_DEVICE(device->node.next);
			continue;
		}

		/* Scope Exhausted - Find Next */

		while ((device = device->parent)) {
			--level;
			if (HAS_SIBLINGS(device)) {
				device = NODE_TO_DEVICE(device->node.next);
				break;
			}
		}
	}

	if ((direction == WALK_UP) && (result == 0))
		callback(start, level, data);

	return result;
}


/* --------------------------------------------------------------------------
                             Notification Handling
   -------------------------------------------------------------------------- */

static void
acpi_bus_check_device (ACPI_HANDLE handle)
{
	struct acpi_device *device;
	ACPI_STATUS status = 0;
	struct acpi_device_status old_status;

	if (acpi_bus_get_device(handle, &device))
		return;
	if (!device)
		return;

	old_status = device->status;

	/*
	 * Make sure this device's parent is present before we go about
	 * messing with the device.
	 */
	if (device->parent && !device->parent->status.present) {
		device->status = device->parent->status;
		return;
	}

	status = acpi_bus_get_status(device);
	if (ACPI_FAILURE(status))
		return;

	if (STRUCT_TO_INT(old_status) == STRUCT_TO_INT(device->status))
		return;


	/*
	 * Device Insertion/Removal
	 */
	if ((device->status.present) && !(old_status.present)) {
		DPRINT("Device insertion detected\n");
		/* TBD: Handle device insertion */
	}
	else if (!(device->status.present) && (old_status.present)) {
		DPRINT("Device removal detected\n");
		/* TBD: Handle device removal */
	}

}


static void
acpi_bus_check_scope (ACPI_HANDLE handle)
{
	/* Status Change? */
	acpi_bus_check_device(handle);

	/*
	 * TBD: Enumerate child devices within this device's scope and
	 *       run acpi_bus_check_device()'s on them.
	 */
}


/**
 * acpi_bus_notify
 * ---------------
 * Callback for all 'system-level' device notifications (values 0x00-0x7F).
 */
static void
acpi_bus_notify (
	ACPI_HANDLE             handle,
	UINT32                     type,
	void                    *data)
{
	struct acpi_device *device = NULL;
	struct acpi_driver *driver;

	DPRINT1("Notification %#02x to handle %p\n", type, handle);

	//blocking_notifier_call_chain(&acpi_bus_notify_list,
	//	type, (void *)handle);

	acpi_bus_get_device(handle, &device);

	switch (type) {

	case ACPI_NOTIFY_BUS_CHECK:
		DPRINT("Received BUS CHECK notification for device [%s]\n",
			device ? device->pnp.bus_id : "n/a");
		acpi_bus_check_scope(handle);
		/*
		 * TBD: We'll need to outsource certain events to non-ACPI
		 *	drivers via the device manager (device.c).
		 */
		break;

	case ACPI_NOTIFY_DEVICE_CHECK:
		DPRINT("Received DEVICE CHECK notification for device [%s]\n",
			device ? device->pnp.bus_id : "n/a");
		acpi_bus_check_device(handle);
		/*
		 * TBD: We'll need to outsource certain events to non-ACPI
		 *	drivers via the device manager (device.c).
		 */
		break;

	case ACPI_NOTIFY_DEVICE_WAKE:
		DPRINT("Received DEVICE WAKE notification for device [%s]\n",
			device ? device->pnp.bus_id : "n/a");
		acpi_bus_check_device(handle);
		/*
		 * TBD: We'll need to outsource certain events to non-ACPI
		 *      drivers via the device manager (device.c).
		 */
		break;

	case ACPI_NOTIFY_EJECT_REQUEST:
		DPRINT1("Received EJECT REQUEST notification for device [%s]\n",
			device ? device->pnp.bus_id : "n/a");
		/* TBD */
		break;

	case ACPI_NOTIFY_DEVICE_CHECK_LIGHT:
		DPRINT1("Received DEVICE CHECK LIGHT notification for device [%s]\n",
			device ? device->pnp.bus_id : "n/a");
		/* TBD: Exactly what does 'light' mean? */
		break;

	case ACPI_NOTIFY_FREQUENCY_MISMATCH:
		DPRINT1("Received FREQUENCY MISMATCH notification for device [%s]\n",
			device ? device->pnp.bus_id : "n/a");
		/* TBD */
		break;

	case ACPI_NOTIFY_BUS_MODE_MISMATCH:
		DPRINT1("Received BUS MODE MISMATCH notification for device [%s]\n",
			device ? device->pnp.bus_id : "n/a");
		/* TBD */
		break;

	case ACPI_NOTIFY_POWER_FAULT:
		DPRINT1("Received POWER FAULT notification for device [%s]\n",
			device ? device->pnp.bus_id : "n/a");
		/* TBD */
		break;

	default:
		DPRINT1("Received unknown/unsupported notification [%08x] for device [%s]\n",
			type, device ? device->pnp.bus_id : "n/a");
		break;
	}

	if (device) {
		driver = device->driver;
		if (driver && driver->ops.notify &&
		    (driver->flags & ACPI_DRIVER_ALL_NOTIFY_EVENTS))
			driver->ops.notify(device, type);
	}
}


/* --------------------------------------------------------------------------
                                 Driver Management
   -------------------------------------------------------------------------- */


static LIST_HEAD(acpi_bus_drivers);
//static DECLARE_MUTEX(acpi_bus_drivers_lock);
static FAST_MUTEX acpi_bus_drivers_lock;


/**
 * acpi_bus_match
 * --------------
 * Checks the device's hardware (_HID) or compatible (_CID) ids to see if it
 * matches the specified driver's criteria.
 */
static int
acpi_bus_match (
	struct acpi_device	*device,
	struct acpi_driver	*driver)
{
	int error = 0;

	if (device->flags.hardware_id)
		if (strstr(driver->ids, device->pnp.hardware_id))
			goto Done;

	if (device->flags.compatible_ids) {
		ACPI_PNP_DEVICE_ID_LIST *cid_list = device->pnp.cid_list;
		int i;

		/* compare multiple _CID entries against driver ids */
		for (i = 0; i < cid_list->Count; i++)
		{
			if (strstr(driver->ids, cid_list->Ids[i].String))
				goto Done;
		}
	}
	error = -2;

 Done:

	return error;
}


/**
 * acpi_bus_driver_init
 * --------------------
 * Used to initialize a device via its device driver.  Called whenever a
 * driver is bound to a device.  Invokes the driver's add() and start() ops.
 */
static int
acpi_bus_driver_init (
	struct acpi_device	*device,
	struct acpi_driver	*driver)
{
	int			result = 0;

	if (!device || !driver)
		return_VALUE(AE_BAD_PARAMETER);

	if (!driver->ops.add)
		return_VALUE(-38);

	result = driver->ops.add(device);
	if (result) {
		device->driver = NULL;
		//acpi_driver_data(device) = NULL;
		return_VALUE(result);
	}

	device->driver = driver;

	/*
	 * TBD - Configuration Management: Assign resources to device based
	 * upon possible configuration and currently allocated resources.
	 */

	if (driver->ops.start) {
		result = driver->ops.start(device);
		if (result && driver->ops.remove)
			driver->ops.remove(device, ACPI_BUS_REMOVAL_NORMAL);
		return_VALUE(result);
	}

	DPRINT("Driver successfully bound to device\n");

	if (driver->ops.scan) {
		driver->ops.scan(device);
	}

	return_VALUE(0);
}


/**
 * acpi_bus_attach
 * -------------
 * Callback for acpi_bus_walk() used to find devices that match a specific
 * driver's criteria and then attach the driver.
 */
static int
acpi_bus_attach (
	struct acpi_device	*device,
	int			level,
	void			*data)
{
	int			result = 0;
	struct acpi_driver	*driver = NULL;

	if (!device || !data)
		return_VALUE(AE_BAD_PARAMETER);

	driver = (struct acpi_driver *) data;

	if (device->driver)
		return_VALUE(-9);

	if (!device->status.present)
		return_VALUE(AE_NOT_FOUND);

	result = acpi_bus_match(device, driver);
	if (result)
		return_VALUE(result);

	DPRINT("Found driver [%s] for device [%s]\n",
		driver->name, device->pnp.bus_id);

	result = acpi_bus_driver_init(device, driver);
	if (result)
		return_VALUE(result);

	down(&acpi_bus_drivers_lock);
	++driver->references;
	up(&acpi_bus_drivers_lock);

	return_VALUE(0);
}


/**
 * acpi_bus_unattach
 * -----------------
 * Callback for acpi_bus_walk() used to find devices that match a specific
 * driver's criteria and unattach the driver.
 */
static int
acpi_bus_unattach (
	struct acpi_device	*device,
	int			level,
	void			*data)
{
	int			result = 0;
	struct acpi_driver	*driver = (struct acpi_driver *) data;

	if (!device || !driver)
		return_VALUE(AE_BAD_PARAMETER);

	if (device->driver != driver)
		return_VALUE(-6);

	if (!driver->ops.remove)
		return_VALUE(-23);

	result = driver->ops.remove(device, ACPI_BUS_REMOVAL_NORMAL);
	if (result)
		return_VALUE(result);

	device->driver = NULL;
	acpi_driver_data(device) = NULL;

	down(&acpi_bus_drivers_lock);
	driver->references--;
	up(&acpi_bus_drivers_lock);

	return_VALUE(0);
}


/**
 * acpi_bus_find_driver
 * --------------------
 * Parses the list of registered drivers looking for a driver applicable for
 * the specified device.
 */
static int
acpi_bus_find_driver (
	struct acpi_device	*device)
{
	int			result = AE_NOT_FOUND;
	struct list_head	*entry = NULL;
	struct acpi_driver	*driver = NULL;

	if (!device || device->driver)
		return_VALUE(AE_BAD_PARAMETER);

	down(&acpi_bus_drivers_lock);

	list_for_each(entry, &acpi_bus_drivers) {

		driver = list_entry(entry, struct acpi_driver, node);

		if (acpi_bus_match(device, driver))
			continue;

		result = acpi_bus_driver_init(device, driver);
		if (!result)
			++driver->references;

		break;
	}

	up(&acpi_bus_drivers_lock);

	return_VALUE(result);
}


/**
 * acpi_bus_register_driver
 * ------------------------
 * Registers a driver with the ACPI bus.  Searches the namespace for all
 * devices that match the driver's criteria and binds.
 */
int
acpi_bus_register_driver (
	struct acpi_driver	*driver)
{
	if (!driver)
		return_VALUE(AE_BAD_PARAMETER);

	//if (acpi_disabled)
	//	return_VALUE(AE_NOT_FOUND);

	down(&acpi_bus_drivers_lock);
	list_add_tail(&driver->node, &acpi_bus_drivers);
	up(&acpi_bus_drivers_lock);

	acpi_bus_walk(acpi_root, acpi_bus_attach,
		WALK_DOWN, driver);

	return_VALUE(driver->references);
}


/**
 * acpi_bus_unregister_driver
 * --------------------------
 * Unregisters a driver with the ACPI bus.  Searches the namespace for all
 * devices that match the driver's criteria and unbinds.
 */
void
acpi_bus_unregister_driver (
	struct acpi_driver	*driver)
{
	if (!driver)
		return;

	acpi_bus_walk(acpi_root, acpi_bus_unattach, WALK_UP, driver);

	if (driver->references)
		return;

	down(&acpi_bus_drivers_lock);
	list_del(&driver->node);
	up(&acpi_bus_drivers_lock);

	return;
}


/* --------------------------------------------------------------------------
                                 Device Enumeration
   -------------------------------------------------------------------------- */

static int
acpi_bus_get_flags (
	struct acpi_device	*device)
{
	ACPI_STATUS		status = AE_OK;
	ACPI_HANDLE		temp = NULL;

	/* Presence of _STA indicates 'dynamic_status' */
	status = AcpiGetHandle(device->handle, "_STA", &temp);
	if (ACPI_SUCCESS(status))
		device->flags.dynamic_status = 1;

	/* Presence of _CID indicates 'compatible_ids' */
	status = AcpiGetHandle(device->handle, "_CID", &temp);
	if (ACPI_SUCCESS(status))
		device->flags.compatible_ids = 1;

	/* Presence of _RMV indicates 'removable' */
	status = AcpiGetHandle(device->handle, "_RMV", &temp);
	if (ACPI_SUCCESS(status))
		device->flags.removable = 1;

	/* Presence of _EJD|_EJ0 indicates 'ejectable' */
	status = AcpiGetHandle(device->handle, "_EJD", &temp);
	if (ACPI_SUCCESS(status))
		device->flags.ejectable = 1;
	else {
		status = AcpiGetHandle(device->handle, "_EJ0", &temp);
		if (ACPI_SUCCESS(status))
			device->flags.ejectable = 1;
	}

	/* Presence of _LCK indicates 'lockable' */
	status = AcpiGetHandle(device->handle, "_LCK", &temp);
	if (ACPI_SUCCESS(status))
		device->flags.lockable = 1;

	/* Presence of _PS0|_PR0 indicates 'power manageable' */
	status = AcpiGetHandle(device->handle, "_PS0", &temp);
	if (ACPI_FAILURE(status))
		status = AcpiGetHandle(device->handle, "_PR0", &temp);
	if (ACPI_SUCCESS(status))
		device->flags.power_manageable = 1;

	/* TBD: Peformance management */

	return_VALUE(0);
}


int
acpi_bus_add (
	struct acpi_device	**child,
	struct acpi_device	*parent,
	ACPI_HANDLE		handle,
	int			type)
{
	int			result = 0;
	ACPI_STATUS		status = AE_OK;
	struct acpi_device	*device = NULL;
	char			bus_id[5] = {'?',0};
	ACPI_BUFFER	buffer;
	ACPI_DEVICE_INFO	*info;
	char			*hid = NULL;
	char			*uid = NULL;
	ACPI_PNP_DEVICE_ID_LIST *cid_list = NULL;
	int			i = 0;
	acpi_unique_id		static_uid_buffer;

	if (!child)
		return_VALUE(AE_BAD_PARAMETER);

	device = ExAllocatePoolWithTag(NonPagedPool,sizeof(struct acpi_device), 'DpcA');
	if (!device) {
		DPRINT1("Memory allocation error\n");
		return_VALUE(-12);
	}
	memset(device, 0, sizeof(struct acpi_device));

	device->handle = handle;
	device->parent = parent;

	/*
	 * Bus ID
	 * ------
	 * The device's Bus ID is simply the object name.
	 * TBD: Shouldn't this value be unique (within the ACPI namespace)?
	 */
	switch (type) {
	case ACPI_BUS_TYPE_SYSTEM:
		snprintf(device->pnp.bus_id, sizeof(device->pnp.bus_id), "%s", "ACPI");
		break;
	case ACPI_BUS_TYPE_POWER_BUTTONF:
	case ACPI_BUS_TYPE_POWER_BUTTON:
		snprintf(device->pnp.bus_id, sizeof(device->pnp.bus_id), "%s", "PWRF");
		break;
	case ACPI_BUS_TYPE_SLEEP_BUTTONF:
	case ACPI_BUS_TYPE_SLEEP_BUTTON:
		snprintf(device->pnp.bus_id, sizeof(device->pnp.bus_id), "%s", "SLPF");
		break;
	default:
		buffer.Length = sizeof(bus_id);
		buffer.Pointer = bus_id;
		AcpiGetName(handle, ACPI_SINGLE_NAME, &buffer);


		/* Clean up trailing underscores (if any) */
		for (i = 3; i > 1; i--) {
			if (bus_id[i] == '_')
				bus_id[i] = '\0';
			else
				break;
		}
		snprintf(device->pnp.bus_id, sizeof(device->pnp.bus_id), "%s", bus_id);
		buffer.Pointer = NULL;

        /* HACK: Skip HPET */
        if (strstr(device->pnp.bus_id, "HPET"))
        {
            DPRINT("Using HPET hack\n");
            result = -1;
            goto end;
        }

		break;
	}

	/*
	 * Flags
	 * -----
	 * Get prior to calling acpi_bus_get_status() so we know whether
	 * or not _STA is present.  Note that we only look for object
	 * handles -- cannot evaluate objects until we know the device is
	 * present and properly initialized.
	 */
	result = acpi_bus_get_flags(device);
	if (result)
		goto end;

	/*
	 * Status
	 * ------
	 * See if the device is present.  We always assume that non-Device()
	 * objects (e.g. thermal zones, power resources, processors, etc.) are
	 * present, functioning, etc. (at least when parent object is present).
	 * Note that _STA has a different meaning for some objects (e.g.
	 * power resources) so we need to be careful how we use it.
	 */
	switch (type) {
	case ACPI_BUS_TYPE_DEVICE:
		result = acpi_bus_get_status(device);
		if (result)
			goto end;
		break;
	default:
		STRUCT_TO_INT(device->status) = 0x0F;
		break;
	}
	if (!device->status.present) {
		result = -2;
		goto end;
	}

	/*
	 * Initialize Device
	 * -----------------
	 * TBD: Synch with Core's enumeration/initialization process.
	 */

	/*
	 * Hardware ID, Unique ID, & Bus Address
	 * -------------------------------------
	 */
	switch (type) {
	case ACPI_BUS_TYPE_DEVICE:
		status = AcpiGetObjectInfo(handle,&info);
		if (ACPI_FAILURE(status)) {
			ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
				"Error reading device info\n"));
			result = AE_NOT_FOUND;
			goto end;
		}
		if (info->Valid & ACPI_VALID_HID)
			hid = info->HardwareId.String;
		if (info->Valid & ACPI_VALID_UID)
			uid = info->UniqueId.String;
		if (info->Valid & ACPI_VALID_CID) {
			cid_list = &info->CompatibleIdList;
			device->pnp.cid_list = ExAllocatePoolWithTag(NonPagedPool,cid_list->ListSize, 'DpcA');
			if (device->pnp.cid_list)
				memcpy(device->pnp.cid_list, cid_list, cid_list->ListSize);
			else
				DPRINT("Memory allocation error\n");
		}
		if (info->Valid & ACPI_VALID_ADR) {
			device->pnp.bus_address = info->Address;
			device->flags.bus_address = 1;
		}
		break;
	case ACPI_BUS_TYPE_POWER:
		hid = ACPI_POWER_HID;
        uid = static_uid_buffer;
		snprintf(uid, sizeof(static_uid_buffer), "%d", (PowerDeviceCount++));
		break;
	case ACPI_BUS_TYPE_PROCESSOR:
		hid = ACPI_PROCESSOR_HID;
		uid = static_uid_buffer;
		snprintf(uid, sizeof(static_uid_buffer), "_%d", (ProcessorCount++));
		break;
	case ACPI_BUS_TYPE_SYSTEM:
		hid = ACPI_SYSTEM_HID;
		break;
	case ACPI_BUS_TYPE_THERMAL:
		hid = ACPI_THERMAL_HID;
        uid = static_uid_buffer;
		snprintf(uid, sizeof(static_uid_buffer), "%d", (ThermalZoneCount++));
		break;
	case ACPI_BUS_TYPE_POWER_BUTTON:
		hid = ACPI_BUTTON_HID_POWER;
        uid = static_uid_buffer;
		snprintf(uid, sizeof(static_uid_buffer), "%d", (PowerButtonCount++));
		break;
	case ACPI_BUS_TYPE_POWER_BUTTONF:
		hid = ACPI_BUTTON_HID_POWERF;
        uid = static_uid_buffer;
		snprintf(uid, sizeof(static_uid_buffer), "%d", (FixedPowerButtonCount++));
		break;
	case ACPI_BUS_TYPE_SLEEP_BUTTON:
		hid = ACPI_BUTTON_HID_SLEEP;
        uid = static_uid_buffer;
		snprintf(uid, sizeof(static_uid_buffer), "%d", (SleepButtonCount++));
		break;
	case ACPI_BUS_TYPE_SLEEP_BUTTONF:
		hid = ACPI_BUTTON_HID_SLEEPF;
        uid = static_uid_buffer;
		snprintf(uid, sizeof(static_uid_buffer), "%d", (FixedSleepButtonCount++));
		break;
	}

	/*
	 * \_SB
	 * ----
	 * Fix for the system root bus device -- the only root-level device.
	 */
	if (((ACPI_HANDLE)parent == ACPI_ROOT_OBJECT) && (type == ACPI_BUS_TYPE_DEVICE)) {
		hid = ACPI_BUS_HID;
		snprintf(device->pnp.device_name, sizeof(device->pnp.device_name), "%s", ACPI_BUS_DEVICE_NAME);
		snprintf(device->pnp.device_class, sizeof(device->pnp.device_class), "%s", ACPI_BUS_CLASS);
	}

	if (hid) {
        device->pnp.hardware_id = ExAllocatePoolWithTag(NonPagedPool, strlen(hid) + 1, 'DpcA');
        if (device->pnp.hardware_id) {
            snprintf(device->pnp.hardware_id, strlen(hid) + 1, "%s", hid);
            device->flags.hardware_id = 1;
        }
	}
	if (uid) {
		snprintf(device->pnp.unique_id, sizeof(device->pnp.unique_id), "%s", uid);
		device->flags.unique_id = 1;
	}

	/*
	 * If we called get_object_info, we now are finished with the buffer,
	 * so we can free it.
	 */
	//if (buffer.Pointer)
		//AcpiOsFree(buffer.Pointer);

	/*
	 * Power Management
	 * ----------------
	 */
	if (device->flags.power_manageable) {
		result = acpi_bus_get_power_flags(device);
		if (result)
			goto end;
	}

	/*
	 * Performance Management
	 * ----------------------
	 */
	if (device->flags.performance_manageable) {
		result = acpi_bus_get_perf_flags(device);
		if (result)
			goto end;
	}

	/*
	 * Context
	 * -------
	 * Attach this 'struct acpi_device' to the ACPI object.  This makes
	 * resolutions from handle->device very efficient.  Note that we need
	 * to be careful with fixed-feature devices as they all attach to the
	 * root object.
	 */
	switch (type) {
	case ACPI_BUS_TYPE_POWER_BUTTON:
	case ACPI_BUS_TYPE_POWER_BUTTONF:
	case ACPI_BUS_TYPE_SLEEP_BUTTON:
	case ACPI_BUS_TYPE_SLEEP_BUTTONF:
		break;
	default:
		status = AcpiAttachData(device->handle,
			acpi_bus_data_handler, device);
		break;
	}
	if (ACPI_FAILURE(status)) {
		ACPI_DEBUG_PRINT((ACPI_DB_ERROR,
			"Error attaching device data\n"));
		result = AE_NOT_FOUND;
		goto end;
	}

	/*
	 * Linkage
	 * -------
	 * Link this device to its parent and siblings.
	 */
	INIT_LIST_HEAD(&device->children);
	if (!device->parent)
		INIT_LIST_HEAD(&device->node);
	else
		list_add_tail(&device->node, &device->parent->children);

	/*
	 * Global Device Hierarchy:
	 * ------------------------
	 * Register this device with the global device hierarchy.
	 */
	acpi_device_register(device, parent);

	/*
	 * Bind _ADR-Based Devices
	 * -----------------------
	 * If there's a a bus address (_ADR) then we utilize the parent's
	 * 'bind' function (if exists) to bind the ACPI- and natively-
	 * enumerated device representations.
	 */
	if (device->flags.bus_address) {
		if (device->parent && device->parent->ops.bind)
			device->parent->ops.bind(device);
	}

	/*
	 * Locate & Attach Driver
	 * ----------------------
	 * If there's a hardware id (_HID) or compatible ids (_CID) we check
	 * to see if there's a driver installed for this kind of device.  Note
	 * that drivers can install before or after a device is enumerated.
	 *
	 * TBD: Assumes LDM provides driver hot-plug capability.
	 */
	if (device->flags.hardware_id || device->flags.compatible_ids)
		acpi_bus_find_driver(device);

end:
	if (result) {
		if (device->pnp.cid_list) {
			ExFreePoolWithTag(device->pnp.cid_list, 'DpcA');
		}
        if (device->pnp.hardware_id) {
            ExFreePoolWithTag(device->pnp.hardware_id, 'DpcA');
        }
		ExFreePoolWithTag(device, 'DpcA');
		return_VALUE(result);
	}
	*child = device;

	return_VALUE(0);
}


static int
acpi_bus_remove (
	struct acpi_device	*device,
	int			type)
{

	if (!device)
		return_VALUE(AE_NOT_FOUND);

	acpi_device_unregister(device);

	if (device->pnp.cid_list)
		ExFreePoolWithTag(device->pnp.cid_list, 'DpcA');

    if (device->pnp.hardware_id)
        ExFreePoolWithTag(device->pnp.hardware_id, 'DpcA');

	if (device)
		ExFreePoolWithTag(device, 'DpcA');

	return_VALUE(0);
}


int
acpi_bus_scan (
	struct acpi_device	*start)
{
	ACPI_STATUS		status = AE_OK;
	struct acpi_device	*parent = NULL;
	struct acpi_device	*child = NULL;
	ACPI_HANDLE		phandle = 0;
	ACPI_HANDLE		chandle = 0;
	ACPI_OBJECT_TYPE	type = 0;
	UINT32			level = 1;

	if (!start)
		return_VALUE(AE_BAD_PARAMETER);

	parent = start;
	phandle = start->handle;

	/*
	 * Parse through the ACPI namespace, identify all 'devices', and
	 * create a new 'struct acpi_device' for each.
	 */
	while ((level > 0) && parent) {

		status = AcpiGetNextObject(ACPI_TYPE_ANY, phandle,
			chandle, &chandle);

		/*
		 * If this scope is exhausted then move our way back up.
		 */
		if (ACPI_FAILURE(status)) {
			level--;
			chandle = phandle;
			AcpiGetParent(phandle, &phandle);
			if (parent->parent)
				parent = parent->parent;
			continue;
		}

		status = AcpiGetType(chandle, &type);
		if (ACPI_FAILURE(status))
			continue;

		/*
		 * If this is a scope object then parse it (depth-first).
		 */
		if (type == ACPI_TYPE_LOCAL_SCOPE) {
			level++;
			phandle = chandle;
			chandle = 0;
			continue;
		}

		/*
		 * We're only interested in objects that we consider 'devices'.
		 */
		switch (type) {
		case ACPI_TYPE_DEVICE:
			type = ACPI_BUS_TYPE_DEVICE;
			break;
		case ACPI_TYPE_PROCESSOR:
			type = ACPI_BUS_TYPE_PROCESSOR;
			break;
		case ACPI_TYPE_THERMAL:
			type = ACPI_BUS_TYPE_THERMAL;
			break;
		case ACPI_TYPE_POWER:
			type = ACPI_BUS_TYPE_POWER;
			break;
		default:
			continue;
		}

		status = acpi_bus_add(&child, parent, chandle, type);
		if (ACPI_FAILURE(status))
			continue;

		/*
		 * If the device is present, enabled, and functioning then
		 * parse its scope (depth-first).  Note that we need to
		 * represent absent devices to facilitate PnP notifications
		 * -- but only the subtree head (not all of its children,
		 * which will be enumerated when the parent is inserted).
		 *
		 * TBD: Need notifications and other detection mechanisms
		 *	in place before we can fully implement this.
		 */
		if (child->status.present) {
			status = AcpiGetNextObject(ACPI_TYPE_ANY, chandle,
				0, NULL);
			if (ACPI_SUCCESS(status)) {
				level++;
				phandle = chandle;
				chandle = 0;
				parent = child;
			}
		}
	}

	return_VALUE(0);
}


static int
acpi_bus_scan_fixed (
	struct acpi_device	*root)
{
	int			result = 0;
	struct acpi_device	*device = NULL;

	if (!root)
		return_VALUE(AE_NOT_FOUND);

	/* If ACPI_FADT_POWER_BUTTON is set, then a control
	 * method power button is present. Otherwise, a fixed
	 * power button is present.
	 */
	if (AcpiGbl_FADT.Flags & ACPI_FADT_POWER_BUTTON)
		result = acpi_bus_add(&device, acpi_root,
			NULL, ACPI_BUS_TYPE_POWER_BUTTON);
	else
	{
		/* Enable the fixed power button so we get notified if it is pressed */
		AcpiWriteBitRegister(ACPI_BITREG_POWER_BUTTON_ENABLE, 1);

		result = acpi_bus_add(&device, acpi_root,
			NULL, ACPI_BUS_TYPE_POWER_BUTTONF);
	}

	/* This one is a bit more complicated and we do it wrong
	 * right now. If ACPI_FADT_SLEEP_BUTTON is set but no
	 * device object is present then no sleep button is present, but
	 * if the flags is clear and there is no device object then it is
	 * a fixed sleep button. If the flag is set and there is a device object
	 * the we have a control method button just like above.
	 */
	if (AcpiGbl_FADT.Flags & ACPI_FADT_SLEEP_BUTTON)
		result = acpi_bus_add(&device, acpi_root,
			NULL, ACPI_BUS_TYPE_SLEEP_BUTTON);
	else
	{
		/* Enable the fixed sleep button so we get notified if it is pressed */
		AcpiWriteBitRegister(ACPI_BITREG_SLEEP_BUTTON_ENABLE, 1);

		result = acpi_bus_add(&device, acpi_root,
			NULL, ACPI_BUS_TYPE_SLEEP_BUTTONF);
	}

	return_VALUE(result);
}


/* --------------------------------------------------------------------------
                             Initialization/Cleanup
   -------------------------------------------------------------------------- */

int
acpi_bus_init (void)
{
	int			result = 0;
	ACPI_STATUS		status = AE_OK;

	DPRINT("acpi_bus_init\n");

        KeInitializeDpc(&event_dpc, acpi_bus_generate_event_dpc, NULL);

	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		DPRINT1("Unable to start the ACPI Interpreter\n");
		goto error1;
	}

	/*
	 * ACPI 2.0 requires the EC driver to be loaded and work before
	 * the EC device is found in the namespace. This is accomplished
	 * by looking for the ECDT table, and getting the EC parameters out
	 * of that.
	 */
	//result = acpi_ec_ecdt_probe();
	/* Ignore result. Not having an ECDT is not fatal. */

	status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status)) {
		DPRINT1("Unable to initialize ACPI objects\n");
		goto error1;
	}

	/*
	 * Maybe EC region is required at bus_scan/acpi_get_devices. So it
	 * is necessary to enable it as early as possible.
	 */
	//acpi_boot_ec_enable();

	/* Initialize sleep structures */
	//acpi_sleep_init();

	/*
	 * Register the for all standard device notifications.
	 */
	status = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, acpi_bus_notify, NULL);
	if (ACPI_FAILURE(status)) {
		DPRINT1("Unable to register for device notifications\n");
		result = AE_NOT_FOUND;
		goto error1;
	}

	/*
	 * Create the root device in the bus's device tree
	 */
	result = acpi_bus_add(&acpi_root, NULL, ACPI_ROOT_OBJECT,
		ACPI_BUS_TYPE_SYSTEM);
	if (result)
		goto error2;


	/*
	 * Enumerate devices in the ACPI namespace.
	 */
	result = acpi_bus_scan_fixed(acpi_root);
	if (result)
		DPRINT1("acpi_bus_scan_fixed failed\n");
	result = acpi_bus_scan(acpi_root);
	if (result)
		DPRINT1("acpi_bus_scan failed\n");

	return_VALUE(0);

	/* Mimic structured exception handling */
error2:
	AcpiRemoveNotifyHandler(ACPI_ROOT_OBJECT,
		ACPI_SYSTEM_NOTIFY, acpi_bus_notify);
error1:
	AcpiTerminate();
	return_VALUE(AE_NOT_FOUND);
}

static void
acpi_bus_exit (void)
{
	ACPI_STATUS		status = AE_OK;

	DPRINT1("acpi_bus_exit\n");

	status = AcpiRemoveNotifyHandler(ACPI_ROOT_OBJECT,
		ACPI_SYSTEM_NOTIFY, acpi_bus_notify);
	if (ACPI_FAILURE(status))
		DPRINT1("Error removing notify handler\n");

#ifdef CONFIG_ACPI_PCI
	acpi_pci_root_exit();
	acpi_pci_link_exit();
#endif
#ifdef CONFIG_ACPI_EC
	acpi_ec_exit();
#endif
	//acpi_power_exit();
	acpi_system_exit();

	acpi_bus_remove(acpi_root, ACPI_BUS_REMOVAL_NORMAL);

	status = AcpiTerminate();
	if (ACPI_FAILURE(status))
		DPRINT1("Unable to terminate the ACPI Interpreter\n");
	else
		DPRINT1("Interpreter disabled\n");

	return_VOID;
}


int
acpi_init (void)
{
	int			result = 0;

	DPRINT("acpi_init\n");

	DPRINT("Subsystem revision %08x\n",ACPI_CA_VERSION);

	KeInitializeSpinLock(&acpi_bus_event_lock);
	KeInitializeEvent(&AcpiEventQueue, NotificationEvent, FALSE);
	ExInitializeFastMutex(&acpi_bus_drivers_lock);

	result = acpi_bus_init();

	//if (!result) {
		//pci_mmcfg_late_init();
		//if (!(pm_flags & PM_APM))
		//	pm_flags |= PM_ACPI;
		//else {
			//DPRINT1("APM is already active, exiting\n");
			//disable_acpi();
			//result = -ENODEV;
		//}
	//} else
	//	disable_acpi();

	/*
	 * If the laptop falls into the DMI check table, the power state check
	 * will be disabled in the course of device power transistion.
	 */
	//dmi_check_system(power_nocheck_dmi_table);

	/*
	 * Install drivers required for proper enumeration of the
	 * ACPI namespace.
	 */
	acpi_system_init();	/* ACPI System */
	acpi_power_init();	/* ACPI Bus Power Management */
	acpi_button_init();
	//acpi_ec_init();		/* ACPI Embedded Controller */
#ifdef CONFIG_ACPI_PCI
	if (!acpi_pci_disabled) {
		acpi_pci_link_init();	/* ACPI PCI Interrupt Link */
		acpi_pci_root_init();	/* ACPI PCI Root Bridge */
	}
#endif

	//acpi_scan_init();
	//acpi_ec_init();
	//acpi_power_init();
	//acpi_system_init();
	//acpi_debug_init();
	//acpi_sleep_proc_init();
	//acpi_wakeup_device_init();

	return result;
}


void
acpi_exit (void)
{
	DPRINT("acpi_exit\n");

#ifdef CONFIG_PM
	pm_active = 0;
#endif

	acpi_bus_exit();

	return_VOID;
}

