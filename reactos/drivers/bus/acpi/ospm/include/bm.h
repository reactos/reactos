/*****************************************************************************
 *
 * Module name: bm.h
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

#ifndef __BM_H__
#define __BM_H__

#include <actypes.h>
#include <acexcep.h>


/*****************************************************************************
 *                               Types & Defines
 *****************************************************************************/

/*
 * Output Flags (Debug):
 * ---------------------
 */
#define BM_PRINT_ALL                    (0x00000000)
#define BM_PRINT_GROUP                  (0x00000001)
#define BM_PRINT_LINKAGE                (0x00000002)
#define BM_PRINT_IDENTIFICATION		(0x00000004)
#define BM_PRINT_POWER			(0x00000008)
#define BM_PRINT_PRESENT		(0x00000010)


/*
 * /proc Interface:
 * ----------------
 */
#define BM_PROC_ROOT			"acpi"
#define BM_PROC_EVENT			"event"

extern struct proc_dir_entry		*bm_proc_root;


/*
 * BM_COMMAND:
 * -----------
 */
typedef u32                             BM_COMMAND;

#define BM_COMMAND_UNKNOWN              ((BM_COMMAND) 0x00)

#define BM_COMMAND_GET_POWER_STATE	((BM_COMMAND) 0x01)
#define BM_COMMAND_SET_POWER_STATE	((BM_COMMAND) 0x02)

#define BM_COMMAND_DEVICE_SPECIFIC      ((BM_COMMAND) 0x80)

/*
 * BM_NOTIFY:
 * ----------
 * Standard ACPI notification values, from section 5.6.3 of the ACPI 2.0
 * specification.  Note that the Bus Manager internally handles all
 * standard ACPI notifications -- driver modules are never sent these
 * values (see "Bus Manager Notifications", below).
 */
typedef u32                             BM_NOTIFY;

#define BM_NOTIFY_BUS_CHECK             ((BM_NOTIFY) 0x00)
#define BM_NOTIFY_DEVICE_CHECK          ((BM_NOTIFY) 0x01)
#define BM_NOTIFY_DEVICE_WAKE           ((BM_NOTIFY) 0x02)
#define BM_NOTIFY_EJECT_REQUEST         ((BM_NOTIFY) 0x03)
#define BM_NOTIFY_DEVICE_CHECK_LIGHT    ((BM_NOTIFY) 0x04)
#define BM_NOTIFY_FREQUENCY_MISMATCH    ((BM_NOTIFY) 0x05)
#define BM_NOTIFY_BUS_MODE_MISMATCH     ((BM_NOTIFY) 0x06)
#define BM_NOTIFY_POWER_FAULT           ((BM_NOTIFY) 0x07)

/*
 * These are a higher-level abstraction of ACPI notifications, intended
 * for consumption by driver modules to facilitate PnP.
 */
#define BM_NOTIFY_UNKNOWN               ((BM_NOTIFY) 0x00)
#define BM_NOTIFY_DEVICE_ADDED          ((BM_NOTIFY) 0x01)
#define BM_NOTIFY_DEVICE_REMOVED        ((BM_NOTIFY) 0x02)


/*
 * BM_HANDLE:
 * ----------
 */
typedef u32                             BM_HANDLE;

#define BM_HANDLE_UNKNOWN               ((BM_HANDLE) 0x00)
#define BM_HANDLE_ROOT                  ((BM_HANDLE) 0x00)
#define BM_HANDLES_MAX                  256



/*
 * BM_HANDLE_LIST:
 * ---------------
 */
typedef struct
{
	u32				count;
	BM_HANDLE			handles[BM_HANDLES_MAX];
} BM_HANDLE_LIST;


/*
 * BM_DEVICE_TYPE:
 * ---------------
 */
typedef u32                             BM_DEVICE_TYPE;

#define BM_TYPE_UNKNOWN			((BM_DEVICE_TYPE) 0x00000000)

#define BM_TYPE_SCOPE			((BM_DEVICE_TYPE) 0x00000001)
#define BM_TYPE_PROCESSOR		((BM_DEVICE_TYPE) 0x00000002)
#define BM_TYPE_THERMAL_ZONE		((BM_DEVICE_TYPE) 0x00000004)
#define BM_TYPE_POWER_RESOURCE		((BM_DEVICE_TYPE) 0x00000008)
#define BM_TYPE_DEVICE			((BM_DEVICE_TYPE) 0x00000010)
#define BM_TYPE_FIXED_BUTTON		((BM_DEVICE_TYPE) 0x00000020)
#define BM_TYPE_SYSTEM			((BM_DEVICE_TYPE) 0x80000000)
#define BM_TYPE_ALL			((BM_DEVICE_TYPE) 0xFFFFFFFF)


/*
 * BM_DEVICE_UID:
 * --------------
 */
typedef char                         	BM_DEVICE_UID[9];

#define BM_UID_UNKNOWN                  '0'


/*
 * BM_DEVICE_HID:
 * --------------
 */
typedef char                         	BM_DEVICE_HID[9];

#define BM_HID_UNKNOWN                  '\0'
#define BM_HID_POWER_BUTTON		"PNP0C0C"
#define BM_HID_SLEEP_BUTTON		"PNP0C0E"

/*
 * BM_DEVICE_CID: 
 *     The compatibility ID can be a string with 44 characters
 *     The extra pad is in case there is a change.  It also
 *     provides 8 byte alignment for the BM_DEVICE_ID structure.
 * -------------------------------------------------------------
 */
typedef char                         	BM_DEVICE_CID[46];


/*
 * BM_DEVICE_ADR:
 * --------------
 */
typedef u32				BM_DEVICE_ADR;

#define BM_ADDRESS_UNKNOWN		0


/*
 * BM_DEVICE_FLAGS:
 * ----------------
 * The encoding of BM_DEVICE_FLAGS is illustrated below.
 * Note that a set bit (1) indicates the property is TRUE
 * (e.g. if bit 0 is set then the device has dynamic status).
 * +--+------------+-+-+-+-+-+-+-+
 * |31| Bits 31:11 |6|5|4|3|2|1|0|
 * +--+------------+-+-+-+-+-+-+-+
 *   |       |      | | | | | | |
 *   |       |      | | | | | | +- Dynamic status?
 *   |       |      | | | | | +--- Identifiable?
 *   |       |      | | | | +----- Configurable?
 *   |       |      | | | +------- Power Manageable?
 *   |       |      | | +--------- Ejectable?
 *   |       |      | +----------- Docking Station?
 *   |       |      +------------- Fixed-Feature?
 *   |       +-------------------- <Reserved>
 *   +---------------------------- Driver Control?
 *
 * Dynamic status:  Device has a _STA object.
 * Identifiable:    Device has a _HID and/or _ADR and possibly other
 *                  identification objects defined.
 * Configurable:    Device has a _CRS and possibly other configuration
 *                  objects defined.
 * Power Control:   Device has a _PR0 and/or _PS0 and possibly other
 *                  power management objects defined.
 * Ejectable:       Device has an _EJD and/or _EJx and possibly other
 *                  dynamic insertion/removal objects defined.
 * Docking Station: Device has a _DCK object defined.
 * Fixed-Feature:   Device does not exist in the namespace; was
 *                  enumerated as a fixed-feature (e.g. power button).
 * Power Manageable:Can change device's power consumption behavior.
 * Has a HID:       In the BIOS ASL this device has a hardware ID as
 *                  defined in section 6.1.4 of ACPI Spec 2.0
 * Has a CID:       In the BIOS ASL this device has a compatible ID as
 *                  defined in section 6.1.2 of ACPI Spec 2.0
 * Has a ADR:       In the BIOS ASL this device has an address ID as
 *                  defined in section 6.1.1 of ACPI Spec 2.0
 * Is a bridge:     This device is recognized as a bridge to another bus.
 * Is on PCI bus:   This device is on a PCI bus or within PCI configuration
 *                  address space.
 * Is on USB bus:   This device is on or within USB address space.
 * Is on SCSI bus:  This device is on or within SCSI address space.
 * Driver Control:  A driver has been installed for this device.
 */
typedef u32                             BM_DEVICE_FLAGS;

#define BM_FLAGS_UNKNOWN                ((BM_DEVICE_FLAGS) 0x00000000)

#define BM_FLAGS_DYNAMIC_STATUS         ((BM_DEVICE_FLAGS) 0x00000001)
#define BM_FLAGS_IDENTIFIABLE           ((BM_DEVICE_FLAGS) 0x00000002)
#define BM_FLAGS_CONFIGURABLE           ((BM_DEVICE_FLAGS) 0x00000004)
#define BM_FLAGS_POWER_CONTROL          ((BM_DEVICE_FLAGS) 0x00000008)
#define BM_FLAGS_EJECTABLE              ((BM_DEVICE_FLAGS) 0x00000010)
#define BM_FLAGS_DOCKING_STATION        ((BM_DEVICE_FLAGS) 0x00000020)
#define BM_FLAGS_FIXED_FEATURE          ((BM_DEVICE_FLAGS) 0x00000040)
#define BM_FLAGS_IS_POWER_MANAGEABLE    ((BM_DEVICE_FLAGS) 0x00000080)
#define BM_FLAGS_HAS_A_HID              ((BM_DEVICE_FLAGS) 0x00000100)
#define BM_FLAGS_HAS_A_CID              ((BM_DEVICE_FLAGS) 0x00000200)
#define BM_FLAGS_HAS_A_ADR              ((BM_DEVICE_FLAGS) 0x00000400)
#define BM_FLAGS_IS_A_BRIDGE            ((BM_DEVICE_FLAGS) 0x00000800)
#define BM_FLAGS_IS_ON_PCI_BUS          ((BM_DEVICE_FLAGS) 0x00001000)
#define BM_FLAGS_IS_ON_USB_BUS          ((BM_DEVICE_FLAGS) 0x00002000)
#define BM_FLAGS_IS_ON_SCSI_BUS         ((BM_DEVICE_FLAGS) 0x00004000)
#define BM_FLAGS_DRIVER_CONTROL         ((BM_DEVICE_FLAGS) 0x80000000)

/*
 * Device PM Flags:
 * ----------------
 * +-----------+-+-+-+-+-+-+-+
 * | Bits 31:7 |6|5|4|3|2|1|0|
 * +-----------+-+-+-+-+-+-+-+
 *       |      | | | | | | |
 *       |      | | | | | | +- D0 Support?
 *       |      | | | | | +--- D1 Support?
 *       |      | | | | +----- D2 Support?
 *       |      | | | +------- D3 Support?
 *       |      | | +--------- Power State Queriable?
 *       |      | +----------- Inrush Current?
 *       |      +------------- Wake Capable?
 *       +-------------------- <Reserved>
 *
 * D0-D3 Support:   Device supports corresponding Dx state.
 * Power State:     Device has a _PSC (current power state) object defined.
 * Inrush Current:  Device has an _IRC (inrush current) object defined.
 * Wake Capable:    Device has a _PRW (wake-capable) object defined.
 */
#define BM_FLAGS_D0_SUPPORT		((BM_DEVICE_FLAGS) 0x00000001)
#define BM_FLAGS_D1_SUPPORT		((BM_DEVICE_FLAGS) 0x00000002)
#define BM_FLAGS_D2_SUPPORT		((BM_DEVICE_FLAGS) 0x00000004)
#define BM_FLAGS_D3_SUPPORT		((BM_DEVICE_FLAGS) 0x00000008)
#define BM_FLAGS_POWER_STATE		((BM_DEVICE_FLAGS) 0x00000010)
#define BM_FLAGS_INRUSH_CURRENT		((BM_DEVICE_FLAGS) 0x00000020)
#define BM_FLAGS_WAKE_CAPABLE		((BM_DEVICE_FLAGS) 0x00000040)


/*
 * BM_DEVICE_STATUS:
 * -----------------
 * The encoding of BM_DEVICE_STATUS is illustrated below.
 * Note that a set bit (1) indicates the property is TRUE
 * (e.g. if bit 0 is set then the device is present).
 * +-----------+-+-+-+-+-+
 * | Bits 31:4 |4|3|2|1|0|
 * +-----------+-+-+-+-+-+
 *       |      | | | | |
 *       |      | | | | +- Present?
 *       |      | | | +--- Enabled?
 *       |      | | +----- Show in UI?
 *       |      | +------- Functioning?
 *       |      +--------- Battery Present?
 *       +---------------- <Reserved>
 */
typedef u32                             BM_DEVICE_STATUS;

#define BM_STATUS_UNKNOWN               ((BM_DEVICE_STATUS) 0x00000000)
#define BM_STATUS_PRESENT               ((BM_DEVICE_STATUS) 0x00000001)
#define BM_STATUS_ENABLED               ((BM_DEVICE_STATUS) 0x00000002)
#define BM_STATUS_SHOW_UI               ((BM_DEVICE_STATUS) 0x00000004)
#define BM_STATUS_FUNCTIONING           ((BM_DEVICE_STATUS) 0x00000008)
#define BM_STATUS_BATTERY_PRESENT       ((BM_DEVICE_STATUS) 0x00000010)
#define BM_STATUS_DEFAULT               ((BM_DEVICE_STATUS) 0x0000000F)


typedef u32                             BM_POWER_STATE;

typedef u8				BM_PCI_BUS_NUM;
typedef u8				BM_PCI_DEVICE_NUM;
typedef u8				BM_PCI_FUNCTION_NUM;
typedef u8				BM_U8_RESERVED;
typedef u8				BM_PCI_DEVICE_CLASS_ID;
typedef u8				BM_PCI_DEVICE_SUBCLASS_ID;
typedef u8				BM_PCI_DEVICE_PROG_IF;
typedef u8				BM_PCI_DEVICE_REVISION;
typedef u16				BM_PCI_VENDOR_ID;
typedef u16				BM_PCI_DEVICE_ID;
typedef u32				BM_U32_RESERVED;


/*
 * BM_DEVICE_ID:
 *     This structure, when filled in for a device, provides
 *     an "association" between hardware space and ACPI.
 * -----------------------------------------------------------
 */
typedef struct
{
	BM_DEVICE_CID			cid;
	BM_DEVICE_HID			hid;
	BM_DEVICE_UID			uid;
	BM_DEVICE_TYPE			type;
	BM_DEVICE_ADR			adr;
	BM_PCI_BUS_NUM			pci_bus_num;
	BM_PCI_DEVICE_NUM		pci_device_num;
	BM_PCI_FUNCTION_NUM		pci_func_num;
	BM_U8_RESERVED			u8_reserved;
	BM_PCI_DEVICE_CLASS_ID		pci_device_class_id;
	BM_PCI_DEVICE_SUBCLASS_ID	pci_device_subclass_id;
	BM_PCI_DEVICE_PROG_IF		pci_device_prog_if;
	BM_PCI_DEVICE_REVISION		pci_device_rev_num;
	BM_PCI_VENDOR_ID		pci_vendor_id;
	BM_PCI_DEVICE_ID		pci_device_id;
	BM_U32_RESERVED			u32_reserved;
} BM_DEVICE_ID;


/*
 * BM_DEVICE_POWER:
 * ----------------
 * Structure containing basic device power management information.
 */
typedef struct
{
	BM_DEVICE_FLAGS			flags;
	BM_POWER_STATE			state;
	BM_DEVICE_FLAGS			dx_supported[ACPI_S_STATE_COUNT];
} BM_DEVICE_POWER;


/*
 * BM_DEVICE:
 * ----------
 */
typedef struct
{
	BM_HANDLE			handle;
	ACPI_HANDLE			acpi_handle;
	BM_DEVICE_FLAGS			flags;
	BM_DEVICE_STATUS		status;
	BM_DEVICE_ID			id;
	BM_DEVICE_POWER			power;
} BM_DEVICE;


/*
 * BM_SEARCH:
 * ----------
 * Structure used for searching the ACPI Bus Manager's device hierarchy.
 */
typedef struct
{
	BM_DEVICE_ID			criteria;
	BM_HANDLE_LIST			results;
} BM_SEARCH;


/*
 * BM_REQUEST:
 * -----------
 * Structure used for sending requests to/through the ACPI Bus Manager.
 */
typedef struct
{
	ACPI_STATUS			status;
	BM_COMMAND			command;
	BM_HANDLE			handle;
	ACPI_BUFFER			buffer;
} BM_REQUEST;


/*
 * Driver Registration:
 * --------------------
 */

/* Driver Context */
typedef void *				BM_DRIVER_CONTEXT;

/* Notification Callback Function */
typedef
ACPI_STATUS (*BM_DRIVER_NOTIFY) (
	BM_NOTIFY			notify_type,
	BM_HANDLE			device_handle,
	BM_DRIVER_CONTEXT		*context);

/* Request Callback Function */
typedef
ACPI_STATUS (*BM_DRIVER_REQUEST) (
	BM_REQUEST			*request,
	BM_DRIVER_CONTEXT		context);

/* Driver Registration */
typedef struct
{
	BM_DRIVER_NOTIFY		notify;
	BM_DRIVER_REQUEST		request;
	BM_DRIVER_CONTEXT		context;
} BM_DRIVER;


/*
 * BM_NODE:
 * --------
 * Structure used to maintain the device hierarchy.
 */
typedef struct _BM_NODE
{
	BM_DEVICE			device;
	BM_DRIVER			driver;
	struct _BM_NODE			*parent;
	struct _BM_NODE			*next;
	struct
	{
		struct _BM_NODE			*head;
		struct _BM_NODE			*tail;
	}				scope;
} BM_NODE;


/*
 * BM_NODE_LIST:
 * -------------
 * Structure used to maintain an array of node pointers.
 */
typedef struct
{
	u32				count;
	BM_NODE				*nodes[BM_HANDLES_MAX];
} BM_NODE_LIST;


/*****************************************************************************
 *                                  Macros
 *****************************************************************************/

#define BM_DEVICE_PRESENT(d)		(d->status & BM_STATUS_PRESENT)
#define BM_NODE_PRESENT(n)		(n->device.status & BM_STATUS_PRESENT)


/*****************************************************************************
 *                             Function Prototypes
 *****************************************************************************/

/* bm.c */

ACPI_STATUS
bm_initialize (void);

ACPI_STATUS
bm_terminate (void);

ACPI_STATUS
bm_get_status (
	BM_DEVICE		*device);

ACPI_STATUS
bm_get_handle (
	ACPI_HANDLE             acpi_handle,
	BM_HANDLE               *device_handle);

ACPI_STATUS
bm_get_node (
	BM_HANDLE               device_handle,
	ACPI_HANDLE             acpi_handle,
	BM_NODE			**node);

/* bmsearch.c */

ACPI_STATUS
bm_search(
	BM_HANDLE               device_handle,
	BM_DEVICE_ID            *criteria,
	BM_HANDLE_LIST          *results);

/* bmnotify.c */

void
bm_notify (
	ACPI_HANDLE             acpi_handle,
	u32                     notify_value,
	void                    *context);

/* bm_request.c */

ACPI_STATUS
bm_request (
	BM_REQUEST		*request_info);

/* bmxface.c */

ACPI_STATUS
bm_get_device_status (
	BM_HANDLE               device_handle,
	BM_DEVICE_STATUS        *device_status);

ACPI_STATUS
bm_get_device_info (
	BM_HANDLE               device_handle,
	BM_DEVICE		**device_info);

ACPI_STATUS
bm_get_device_context (
	BM_HANDLE               device_handle,
	BM_DRIVER_CONTEXT	*context);

ACPI_STATUS
bm_register_driver (
	BM_DEVICE_ID		*criteria,
	BM_DRIVER		*driver);

ACPI_STATUS
bm_unregister_driver (
	BM_DEVICE_ID		*criteria,
	BM_DRIVER		*driver);

/* bmpm.c */

ACPI_STATUS
bm_get_pm_capabilities (
	BM_NODE			*node);

ACPI_STATUS
bm_get_power_state (
	BM_NODE			*node);

ACPI_STATUS
bm_set_power_state (
	BM_NODE			*node,
	BM_POWER_STATE          target_state);

/* bmpower.c */

ACPI_STATUS
bm_pr_initialize (void);

ACPI_STATUS
bm_pr_terminate (void);
	
/* bmutils.c */

ACPI_STATUS
bm_cast_buffer (
	ACPI_BUFFER             *buffer,
	void                    **pointer,
	u32                     length);

ACPI_STATUS
bm_copy_to_buffer (
	ACPI_BUFFER             *buffer,
	void                    *data,
	u32                     length);

ACPI_STATUS
bm_extract_package_data (
	ACPI_OBJECT             *package,
	ACPI_BUFFER             *format,
	ACPI_BUFFER             *buffer);

ACPI_STATUS
bm_evaluate_object (
	ACPI_HANDLE             acpi_handle,
	ACPI_STRING             pathname,
	ACPI_OBJECT_LIST        *arguments,
	ACPI_BUFFER             *buffer);

ACPI_STATUS
bm_evaluate_simple_integer (
	ACPI_HANDLE             acpi_handle,
	ACPI_STRING             pathname,
	u32                     *data);

ACPI_STATUS
bm_evaluate_reference_list (
	ACPI_HANDLE             acpi_handle,
	ACPI_STRING             pathname,
	BM_HANDLE_LIST          *reference_list);

/* bm_proc.c */

ACPI_STATUS
bm_proc_initialize (void);

ACPI_STATUS
bm_proc_terminate (void);

ACPI_STATUS
bm_generate_event (
	BM_HANDLE		device_handle,
	char			*device_type,
	char			*device_instance,
	u32			event_type,
	u32			event_data);


#endif  /* __BM_H__ */
