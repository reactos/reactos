/*
 *  acpi_drivers.h  ($Revision: 32 $)
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

#ifndef __ACPI_DRIVERS_H__
#define __ACPI_DRIVERS_H__

#define ACPI_MAX_STRING			80


/* --------------------------------------------------------------------------
                                    ACPI Bus
   -------------------------------------------------------------------------- */

#define ACPI_BUS_COMPONENT		0x00010000
#define ACPI_BUS_CLASS			"system_bus"
#define ACPI_BUS_HID			"ACPI_BUS"
#define ACPI_BUS_DRIVER_NAME		"ACPI Bus Driver"
#define ACPI_BUS_DEVICE_NAME		"System Bus"


/* --------------------------------------------------------------------------
                                  AC Adapter
   -------------------------------------------------------------------------- */

#define ACPI_AC_COMPONENT		0x00020000
#define ACPI_AC_CLASS			"ac_adapter"
#define ACPI_AC_HID 			"ACPI0003"
#define ACPI_AC_DRIVER_NAME		"ACPI AC Adapter Driver"
#define ACPI_AC_DEVICE_NAME		"AC Adapter"
#define ACPI_AC_FILE_STATE		"state"
#define ACPI_AC_NOTIFY_STATUS		0x80
#define ACPI_AC_STATUS_OFFLINE		0x00
#define ACPI_AC_STATUS_ONLINE		0x01
#define ACPI_AC_STATUS_UNKNOWN		0xFF


/* --------------------------------------------------------------------------
                                     Battery
   -------------------------------------------------------------------------- */

#define ACPI_BATTERY_COMPONENT		0x00040000
#define ACPI_BATTERY_CLASS		"battery"
#define ACPI_BATTERY_HID		"PNP0C0A"
#define ACPI_BATTERY_DRIVER_NAME	"ACPI Battery Driver"
#define ACPI_BATTERY_DEVICE_NAME	"Battery"
#define ACPI_BATTERY_FILE_INFO		"info"
#define ACPI_BATTERY_FILE_STATUS	"state"
#define ACPI_BATTERY_FILE_ALARM		"alarm"
#define ACPI_BATTERY_NOTIFY_STATUS	0x80
#define ACPI_BATTERY_NOTIFY_INFO	0x81
#define ACPI_BATTERY_UNITS_WATTS	"mW"
#define ACPI_BATTERY_UNITS_AMPS		"mA"


/* --------------------------------------------------------------------------
                                      Button
   -------------------------------------------------------------------------- */

#define ACPI_BUTTON_COMPONENT		0x00080000
#define ACPI_BUTTON_DRIVER_NAME		"ACPI Button Driver"
#define ACPI_BUTTON_CLASS		"button"
#define ACPI_BUTTON_FILE_INFO		"info"
#define ACPI_BUTTON_FILE_STATE		"state"
#define ACPI_BUTTON_TYPE_UNKNOWN	0x00
#define ACPI_BUTTON_NOTIFY_STATUS	0x80

#define ACPI_BUTTON_SUBCLASS_POWER	"power"
#define ACPI_BUTTON_HID_POWER		"PNP0C0C"
#define ACPI_BUTTON_HID_POWERF		"ACPI_FPB"
#define ACPI_BUTTON_DEVICE_NAME_POWER	"Power Button (CM)"
#define ACPI_BUTTON_DEVICE_NAME_POWERF	"Power Button (FF)"
#define ACPI_BUTTON_TYPE_POWER		0x01
#define ACPI_BUTTON_TYPE_POWERF		0x02

#define ACPI_BUTTON_SUBCLASS_SLEEP	"sleep"
#define ACPI_BUTTON_HID_SLEEP		"PNP0C0E"
#define ACPI_BUTTON_HID_SLEEPF		"ACPI_FSB"
#define ACPI_BUTTON_DEVICE_NAME_SLEEP	"Sleep Button (CM)"
#define ACPI_BUTTON_DEVICE_NAME_SLEEPF	"Sleep Button (FF)"
#define ACPI_BUTTON_TYPE_SLEEP		0x03
#define ACPI_BUTTON_TYPE_SLEEPF		0x04

#define ACPI_BUTTON_SUBCLASS_LID	"lid"
#define ACPI_BUTTON_HID_LID		"PNP0C0D"
#define ACPI_BUTTON_DEVICE_NAME_LID	"Lid Switch"
#define ACPI_BUTTON_TYPE_LID		0x05

int acpi_button_init (void);
void acpi_button_exit (void);

/* --------------------------------------------------------------------------
                                Embedded Controller
   -------------------------------------------------------------------------- */

#define ACPI_EC_COMPONENT		0x00100000
#define ACPI_EC_CLASS			"embedded_controller"
#define ACPI_EC_HID			"PNP0C09"
#define ACPI_EC_DRIVER_NAME		"ACPI Embedded Controller Driver"
#define ACPI_EC_DEVICE_NAME		"Embedded Controller"
#define ACPI_EC_FILE_INFO		"info"

#ifdef CONFIG_ACPI_EC

int acpi_ec_ecdt_probe (void);
int acpi_ec_init (void);
void acpi_ec_exit (void);

#endif


/* --------------------------------------------------------------------------
                                       Fan
   -------------------------------------------------------------------------- */

#define ACPI_FAN_COMPONENT		0x00200000
#define ACPI_FAN_CLASS			"fan"
#define ACPI_FAN_HID			"PNP0C0B"
#define ACPI_FAN_DRIVER_NAME		"ACPI Fan Driver"
#define ACPI_FAN_DEVICE_NAME		"Fan"
#define ACPI_FAN_FILE_STATE		"state"
#define ACPI_FAN_NOTIFY_STATUS		0x80


/* --------------------------------------------------------------------------
                                       PCI
   -------------------------------------------------------------------------- */

#ifdef CONFIG_ACPI_PCI

#define ACPI_PCI_COMPONENT		0x00400000

/* ACPI PCI Root Bridge (pci_root.c) */

#define ACPI_PCI_ROOT_CLASS		"pci_bridge"
#define ACPI_PCI_ROOT_HID		"PNP0A03"
#define ACPI_PCI_ROOT_DRIVER_NAME	"ACPI PCI Root Bridge Driver"
#define ACPI_PCI_ROOT_DEVICE_NAME	"PCI Root Bridge"

int acpi_pci_root_init (void);
void acpi_pci_root_exit (void);

/* ACPI PCI Interrupt Link (pci_link.c) */

#define ACPI_PCI_LINK_CLASS		"pci_irq_routing"
#define ACPI_PCI_LINK_HID		"PNP0C0F"
#define ACPI_PCI_LINK_DRIVER_NAME	"ACPI PCI Interrupt Link Driver"
#define ACPI_PCI_LINK_DEVICE_NAME	"PCI Interrupt Link"
#define ACPI_PCI_LINK_FILE_INFO		"info"
#define ACPI_PCI_LINK_FILE_STATUS	"state"

int acpi_pci_link_check (void);
int acpi_pci_link_get_irq (ACPI_HANDLE handle, int index, int* edge_level, int* active_high_low);
int acpi_pci_link_init (void);
void acpi_pci_link_exit (void);

/* ACPI PCI Interrupt Routing (pci_irq.c) */

int acpi_pci_irq_add_prt (ACPI_HANDLE handle, int segment, int bus);

/* ACPI PCI Device Binding (pci_bind.c) */

struct pci_bus;

int acpi_pci_bind (struct acpi_device *device);
int acpi_pci_bind_root (struct acpi_device *device, struct acpi_pci_id *id, struct pci_bus *bus);

#endif /*CONFIG_ACPI_PCI*/


/* --------------------------------------------------------------------------
                                  Power Resource
   -------------------------------------------------------------------------- */

#define ACPI_POWER_COMPONENT		0x00800000
#define ACPI_POWER_CLASS		"power_resource"
#define ACPI_POWER_HID			"ACPI_PWR"
#define ACPI_POWER_DRIVER_NAME		"ACPI Power Resource Driver"
#define ACPI_POWER_DEVICE_NAME		"Power Resource"
#define ACPI_POWER_FILE_INFO		"info"
#define ACPI_POWER_FILE_STATUS		"state"
#define ACPI_POWER_RESOURCE_STATE_OFF	0x00
#define ACPI_POWER_RESOURCE_STATE_ON	0x01
#define ACPI_POWER_RESOURCE_STATE_UNKNOWN 0xFF



int acpi_power_get_inferred_state (struct acpi_device *device);
int acpi_power_transition (struct acpi_device *device, int state);
int acpi_power_init (void);
void acpi_power_exit (void);


/* --------------------------------------------------------------------------
                                    Processor
   -------------------------------------------------------------------------- */

#define ACPI_PROCESSOR_COMPONENT	0x01000000
#define ACPI_PROCESSOR_CLASS		"processor"
#define ACPI_PROCESSOR_HID		"Processor"
#define ACPI_PROCESSOR_DRIVER_NAME	"ACPI Processor Driver"
#define ACPI_PROCESSOR_DEVICE_NAME	"Processor"
#define ACPI_PROCESSOR_FILE_INFO	"info"
#define ACPI_PROCESSOR_FILE_POWER	"power"
#define ACPI_PROCESSOR_FILE_PERFORMANCE	"performance"
#define ACPI_PROCESSOR_FILE_THROTTLING	"throttling"
#define ACPI_PROCESSOR_FILE_LIMIT	"limit"
#define ACPI_PROCESSOR_NOTIFY_PERFORMANCE 0x80
#define ACPI_PROCESSOR_NOTIFY_POWER	0x81
#define ACPI_PROCESSOR_LIMIT_NONE	0x00
#define ACPI_PROCESSOR_LIMIT_INCREMENT	0x01
#define ACPI_PROCESSOR_LIMIT_DECREMENT	0x02

int acpi_processor_set_thermal_limit(ACPI_HANDLE handle, int type);


/* --------------------------------------------------------------------------
                                     System
   -------------------------------------------------------------------------- */

#define ACPI_SYSTEM_COMPONENT		0x02000000
#define ACPI_SYSTEM_CLASS		"system"
#define ACPI_SYSTEM_HID			"ACPI_SYS"
#define ACPI_SYSTEM_DRIVER_NAME		"ACPI System Driver"
#define ACPI_SYSTEM_DEVICE_NAME		"System"
#define ACPI_SYSTEM_FILE_INFO		"info"
#define ACPI_SYSTEM_FILE_EVENT		"event"
#define ACPI_SYSTEM_FILE_ALARM		"alarm"
#define ACPI_SYSTEM_FILE_DSDT		"dsdt"
#define ACPI_SYSTEM_FILE_FADT		"fadt"
#define ACPI_SYSTEM_FILE_SLEEP		"sleep"
#define ACPI_SYSTEM_FILE_DEBUG_LAYER	"debug_layer"
#define ACPI_SYSTEM_FILE_DEBUG_LEVEL	"debug_level"

int acpi_system_init (void);
void acpi_system_exit (void);


/* --------------------------------------------------------------------------
                                 Thermal Zone
   -------------------------------------------------------------------------- */

#define ACPI_THERMAL_COMPONENT		0x04000000
#define ACPI_THERMAL_CLASS		"thermal_zone"
#define ACPI_THERMAL_HID		"ThermalZone"
#define ACPI_THERMAL_DRIVER_NAME	"ACPI Thermal Zone Driver"
#define ACPI_THERMAL_DEVICE_NAME	"Thermal Zone"
#define ACPI_THERMAL_FILE_STATE		"state"
#define ACPI_THERMAL_FILE_TEMPERATURE	"temperature"
#define ACPI_THERMAL_FILE_TRIP_POINTS	"trip_points"
#define ACPI_THERMAL_FILE_COOLING_MODE	"cooling_mode"
#define ACPI_THERMAL_FILE_POLLING_FREQ	"polling_frequency"
#define ACPI_THERMAL_NOTIFY_TEMPERATURE	0x80
#define ACPI_THERMAL_NOTIFY_THRESHOLDS	0x81
#define ACPI_THERMAL_NOTIFY_DEVICES	0x82
#define ACPI_THERMAL_NOTIFY_CRITICAL	0xF0
#define ACPI_THERMAL_NOTIFY_HOT		0xF1
#define ACPI_THERMAL_MODE_ACTIVE	0x00
#define ACPI_THERMAL_MODE_PASSIVE	0x01
#define ACPI_THERMAL_PATH_POWEROFF	"/sbin/poweroff"

/* Motherboard devices */
int acpi_motherboard_init(void);
/* --------------------------------------------------------------------------
                                Debug Support
   -------------------------------------------------------------------------- */

#define ACPI_DEBUG_RESTORE	0
#define ACPI_DEBUG_LOW		1
#define ACPI_DEBUG_MEDIUM	2
#define ACPI_DEBUG_HIGH		3
#define ACPI_DEBUG_DRIVERS	4

extern UINT32 acpi_dbg_level;
extern UINT32 acpi_dbg_layer;

static inline void
acpi_set_debug (
	UINT32			flag)
{
	static UINT32		layer_save;
	static UINT32		level_save;

	switch (flag) {
	case ACPI_DEBUG_RESTORE:
		acpi_dbg_layer = layer_save;
		acpi_dbg_level = level_save;
		break;
	case ACPI_DEBUG_LOW:
	case ACPI_DEBUG_MEDIUM:
	case ACPI_DEBUG_HIGH:
	case ACPI_DEBUG_DRIVERS:
		layer_save = acpi_dbg_layer;
		level_save = acpi_dbg_level;
		break;
	}

	switch (flag) {
	case ACPI_DEBUG_LOW:
		acpi_dbg_layer = ACPI_COMPONENT_DEFAULT | ACPI_ALL_DRIVERS;
		acpi_dbg_level = ACPI_DEBUG_DEFAULT;
		break;
	case ACPI_DEBUG_MEDIUM:
		acpi_dbg_layer = ACPI_COMPONENT_DEFAULT | ACPI_ALL_DRIVERS;
		acpi_dbg_level = ACPI_LV_FUNCTIONS | ACPI_LV_ALL_EXCEPTIONS;
		break;
	case ACPI_DEBUG_HIGH:
		acpi_dbg_layer = 0xFFFFFFFF;
		acpi_dbg_level = 0xFFFFFFFF;
		break;
	case ACPI_DEBUG_DRIVERS:
		acpi_dbg_layer = ACPI_ALL_DRIVERS;
		acpi_dbg_level = 0xFFFFFFFF;
		break;
	}
}


#endif /*__ACPI_DRIVERS_H__*/
