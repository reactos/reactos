/*
 *  acpi_bus.h - ACPI Bus Driver ($Revision: 22 $)
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

#ifndef __ACPI_BUS_H__
#define __ACPI_BUS_H__

#include <acpi.h>

#include "list.h"


/* TBD: Make dynamic */
#define ACPI_MAX_HANDLES	10
struct acpi_handle_list {
	UINT32			count;
	ACPI_HANDLE		handles[ACPI_MAX_HANDLES];
};


/* acpi_utils.h */
ACPI_STATUS
acpi_extract_package (
	ACPI_OBJECT       *package,
	ACPI_BUFFER      *format,
	ACPI_BUFFER      *buffer);
ACPI_STATUS
acpi_evaluate_integer (
	ACPI_HANDLE             handle,
	ACPI_STRING             pathname,
	struct acpi_object_list *arguments,
	unsigned long long      *data);
ACPI_STATUS
acpi_evaluate_reference (
	ACPI_HANDLE             handle,
	ACPI_STRING             pathname,
	struct acpi_object_list *arguments,
	struct acpi_handle_list *list);

enum acpi_bus_removal_type {
	ACPI_BUS_REMOVAL_NORMAL	= 0,
	ACPI_BUS_REMOVAL_EJECT,
	ACPI_BUS_REMOVAL_SUPRISE,
	ACPI_BUS_REMOVAL_TYPE_COUNT
};

enum acpi_bus_device_type {
	ACPI_BUS_TYPE_DEVICE	= 0,
	ACPI_BUS_TYPE_POWER,
	ACPI_BUS_TYPE_PROCESSOR,
	ACPI_BUS_TYPE_THERMAL,
	ACPI_BUS_TYPE_SYSTEM,
	ACPI_BUS_TYPE_POWER_BUTTON,
	ACPI_BUS_TYPE_SLEEP_BUTTON,
	ACPI_BUS_DEVICE_TYPE_COUNT
};

struct acpi_driver;
struct acpi_device;


/*
 * ACPI Driver
 * -----------
 */

typedef int (*acpi_op_add)	(struct acpi_device *device);
typedef int (*acpi_op_remove)	(struct acpi_device *device, int type);
typedef int (*acpi_op_start)	(struct acpi_device *device);
typedef int (*acpi_op_suspend)	(struct acpi_device *device, int state);
typedef int (*acpi_op_resume)	(struct acpi_device *device, int state);
typedef int (*acpi_op_scan)	(struct acpi_device *device);
typedef int (*acpi_op_bind)	(struct acpi_device *device);
typedef int (*acpi_op_unbind) (struct acpi_device * device);
typedef void (*acpi_op_notify) (struct acpi_device * device, UINT32 event);

struct acpi_bus_ops {
	UINT32 acpi_op_add:1;
	UINT32 acpi_op_start:1;
};

struct acpi_device_ops {
	acpi_op_add add;
	acpi_op_remove remove;
	acpi_op_start start;
	acpi_op_suspend suspend;
	acpi_op_resume resume;
	acpi_op_bind bind;
	acpi_op_unbind unbind;
	acpi_op_notify notify;
	acpi_op_scan		scan;
};

#define ACPI_DRIVER_ALL_NOTIFY_EVENTS	0x1	/* system AND device events */

struct acpi_driver {
	struct list_head	node;
	char			name[80];
	char			class[80];
	int			references;
	unsigned int flags;
	char			*ids;		/* Supported Hardware IDs */
	struct acpi_device_ops	ops;
};

/*
 * ACPI Device
 * -----------
 */

/* Status (_STA) */

struct acpi_device_status {
	UINT32			present:1;
	UINT32			enabled:1;
	UINT32			show_in_ui:1;
	UINT32			functional:1;
	UINT32			battery_present:1;
	UINT32			reserved:27;
};


/* Flags */

struct acpi_device_flags {
	UINT32			dynamic_status:1;
	UINT32			hardware_id:1;
	UINT32			compatible_ids:1;
	UINT32			bus_address:1;
	UINT32			unique_id:1;
	UINT32			removable:1;
	UINT32			ejectable:1;
	UINT32			lockable:1;
	UINT32			suprise_removal_ok:1;
	UINT32			power_manageable:1;
	UINT32			performance_manageable:1;
	UINT32			wake_capable:1;
	UINT32			force_power_state:1;
	UINT32			reserved:20;
};

/* Plug and Play */

typedef char			acpi_bus_id[8];
typedef unsigned long		acpi_bus_address;
typedef char			acpi_hardware_id[9];
typedef char			acpi_unique_id[9];
typedef char			acpi_device_name[40];
typedef char			acpi_device_class[20];

struct acpi_device_pnp {
	acpi_bus_id		bus_id;		               /* Object name */
	acpi_bus_address	bus_address;	                      /* _ADR */
	acpi_hardware_id	hardware_id;	                      /* _HID */
	ACPI_DEVICE_ID_LIST *cid_list;		     /* _CIDs */
	acpi_unique_id		unique_id;	                      /* _UID */
	acpi_device_name	device_name;	         /* Driver-determined */
	acpi_device_class	device_class;	         /*        "          */
};

#define acpi_device_bid(d)	((d)->pnp.bus_id)
#define acpi_device_adr(d)	((d)->pnp.bus_address)
#define acpi_device_hid(d)	((d)->pnp.hardware_id)
#define acpi_device_uid(d)	((d)->pnp.unique_id)
#define acpi_device_name(d)	((d)->pnp.device_name)
#define acpi_device_class(d)	((d)->pnp.device_class)


/* Power Management */

struct acpi_device_power_flags {
	UINT32			explicit_get:1;		     /* _PSC present? */
	UINT32			power_resources:1;	   /* Power resources */
	UINT32			inrush_current:1;	  /* Serialize Dx->D0 */
	UINT32			power_removed:1;	   /* Optimize Dx->D0 */
	UINT32			reserved:28;
};

struct acpi_device_power_state {
	struct {
		UINT8			valid:1;	
		UINT8			explicit_set:1;	     /* _PSx present? */
		UINT8			reserved:6;
	}			flags;
	int			power;		  /* % Power (compared to D0) */
	int			latency;	/* Dx->D0 time (microseconds) */
	struct acpi_handle_list	resources;	/* Power resources referenced */
};

struct acpi_device_power {
	int			state;		             /* Current state */
	struct acpi_device_power_flags flags;
	struct acpi_device_power_state states[4];     /* Power states (D0-D3) */
};


/* Performance Management */

struct acpi_device_perf_flags {
	UINT8			reserved:8;
};

struct acpi_device_perf_state {
	struct {
		UINT8			valid:1;	
		UINT8			reserved:7;
	}			flags;
	UINT8			power;		  /* % Power (compared to P0) */
	UINT8			performance;	  /* % Performance (    "   ) */
	int			latency;	/* Px->P0 time (microseconds) */
};

struct acpi_device_perf {
	int			state;
	struct acpi_device_perf_flags flags;
	int			state_count;
	struct acpi_device_perf_state *states;
};

/* Wakeup Management */
struct acpi_device_wakeup_flags {
	UINT8 valid:1;		/* Can successfully enable wakeup? */
	UINT8 run_wake:1;		/* Run-Wake GPE devices */
};

struct acpi_device_wakeup_state {
	UINT8 enabled:1;
};

struct acpi_device_wakeup {
	ACPI_HANDLE gpe_device;
	ACPI_INTEGER gpe_number;
	ACPI_INTEGER sleep_state;
	struct acpi_handle_list resources;
	struct acpi_device_wakeup_state state;
	struct acpi_device_wakeup_flags flags;
	int prepare_count;
};


/* Device */

struct acpi_device {
	int device_type;
	ACPI_HANDLE		handle;
	struct acpi_device	*parent;
	struct list_head 	children;
	struct list_head 	node;
	struct list_head wakeup_list;
	struct acpi_device_status status;
	struct acpi_device_flags flags;
	struct acpi_device_pnp	pnp;
	struct acpi_device_power power;
	struct acpi_device_wakeup wakeup;
	struct acpi_device_perf	performance;
	struct acpi_device_ops	ops;
	struct acpi_driver	*driver;
	void *driver_data;
	struct acpi_bus_ops bus_ops;	/* workaround for different code path for hotplug */
	enum acpi_bus_removal_type removal_type;	/* indicate for different removal type */

};

#define acpi_driver_data(d)	((d)->driver_data)

#define to_acpi_device(d)	container_of(d, struct acpi_device, dev)
#define to_acpi_driver(d)	container_of(d, struct acpi_driver, drv)

/* acpi_device.dev.bus == &acpi_bus_type */
extern struct bus_type acpi_bus_type;

/*
 * Events
 * ------
 */

struct acpi_bus_event {
	struct list_head	node;
	acpi_device_class	device_class;
	acpi_bus_id		bus_id;
	UINT32			type;
	UINT32			data;
};


/*
 * External Functions
 */
int acpi_bus_get_private_data(ACPI_HANDLE, void **);

void acpi_bus_data_handler(ACPI_HANDLE handle, void *context);
ACPI_STATUS acpi_bus_get_status_handle(ACPI_HANDLE handle,
				       unsigned long long *sta);
int acpi_bus_get_status(struct acpi_device *device);
int acpi_bus_get_power(ACPI_HANDLE handle, int *state);
int acpi_bus_set_power(ACPI_HANDLE handle, int state);
BOOLEAN acpi_bus_power_manageable(ACPI_HANDLE handle);
BOOLEAN acpi_bus_can_wakeup(ACPI_HANDLE handle);
int acpi_bus_generate_proc_event(struct acpi_device *device, UINT8 type, int data);
int acpi_bus_generate_event(struct acpi_device *device, UINT8 type, int data);
int acpi_bus_receive_event(struct acpi_bus_event *event);
int acpi_bus_register_driver(struct acpi_driver *driver);
void acpi_bus_unregister_driver(struct acpi_driver *driver);
int acpi_bus_add(struct acpi_device **child, struct acpi_device *parent,
		 ACPI_HANDLE handle, int type);
int acpi_bus_trim(struct acpi_device *start, int rmdevice);
int acpi_bus_start(struct acpi_device *device);
ACPI_STATUS acpi_bus_get_ejd(ACPI_HANDLE handle, ACPI_HANDLE * ejd);
int acpi_match_device_ids(struct acpi_device *device,
			  const struct acpi_device_id *ids);
int acpi_bus_get_device(ACPI_HANDLE handle, struct acpi_device **device);
int acpi_init(void);
ACPI_STATUS acpi_suspend (UINT32 state);

/*
 * Bind physical devices with ACPI devices
 */
//struct acpi_bus_type {
//	struct list_head list;
//	struct bus_type *bus;
//	/* For general devices under the bus */
//	int (*find_device) (struct device *, ACPI_HANDLE *);
//	/* For bridges, such as PCI root bridge, IDE controller */
//	int (*find_bridge) (struct device *, ACPI_HANDLE *);
//};
//int register_acpi_bus_type(struct acpi_bus_type *);
//int unregister_acpi_bus_type(struct acpi_bus_type *);
//struct device *acpi_get_physical_device(ACPI_HANDLE);

struct acpi_pci_root {
	struct list_head node;
	struct acpi_device * device;
	struct acpi_pci_id id;
	struct pci_bus *bus;
	UINT16 segment;
	UINT8 bus_nr;

	UINT32 osc_support_set;	/* _OSC state of support bits */
	UINT32 osc_control_set;	/* _OSC state of control bits */
	UINT32 osc_control_qry;	/* the latest _OSC query result */

	UINT32 osc_queried:1;	/* has _OSC control been queried? */
};

//static inline int acpi_pm_device_sleep_state(struct device *d, int *p)
//{
//	if (p)
//		*p = ACPI_STATE_D0;
//	return ACPI_STATE_D3;
//}
//static inline int acpi_pm_device_sleep_wake(struct device *dev, bool enable)
//{
//	return -1;
//}

/* system defines: move to bigger header */
extern enum acpi_irq_model_id	acpi_irq_model;

enum acpi_irq_model_id {
	ACPI_IRQ_MODEL_PIC = 0,
	ACPI_IRQ_MODEL_IOAPIC,
	ACPI_IRQ_MODEL_IOSAPIC,
	ACPI_IRQ_MODEL_COUNT
};



#endif /*__ACPI_BUS_H__*/
