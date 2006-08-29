
/******************************************************************************
 *
 * Name: acpixf.h - External interfaces to the ACPI subsystem
 *
 *****************************************************************************/

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


#ifndef __ACXFACE_H__
#define __ACXFACE_H__

#include "actypes.h"
#include "actbl.h"


/*
 * Global interfaces
 */

ACPI_STATUS
acpi_initialize_subsystem (
	void);

ACPI_STATUS
acpi_enable_subsystem (
	u32                     flags);

ACPI_STATUS
acpi_terminate (
	void);

ACPI_STATUS
acpi_enable (
	void);

ACPI_STATUS
acpi_disable (
	void);

ACPI_STATUS
acpi_get_system_info(
	ACPI_BUFFER             *ret_buffer);

ACPI_STATUS
acpi_format_exception (
	ACPI_STATUS             exception,
	ACPI_BUFFER             *out_buffer);


/*
 * ACPI Memory manager
 */

void *
acpi_allocate (
	u32                     size);

void *
acpi_callocate (
	u32                     size);

void
acpi_free (
	void                    *address);


/*
 * ACPI table manipulation interfaces
 */

ACPI_STATUS
acpi_find_root_pointer (
	ACPI_PHYSICAL_ADDRESS   *rsdp_physical_address);

ACPI_STATUS
acpi_load_tables (
	ACPI_PHYSICAL_ADDRESS   rsdp_physical_address);

ACPI_STATUS
acpi_load_table (
	ACPI_TABLE_HEADER       *table_ptr);

ACPI_STATUS
acpi_unload_table (
	ACPI_TABLE_TYPE         table_type);

ACPI_STATUS
acpi_get_table_header (
	ACPI_TABLE_TYPE         table_type,
	u32                     instance,
	ACPI_TABLE_HEADER       *out_table_header);

ACPI_STATUS
acpi_get_table (
	ACPI_TABLE_TYPE         table_type,
	u32                     instance,
	ACPI_BUFFER             *ret_buffer);


/*
 * Namespace and name interfaces
 */

ACPI_STATUS
acpi_walk_namespace (
	ACPI_OBJECT_TYPE        type,
	ACPI_HANDLE             start_object,
	u32                     max_depth,
	WALK_CALLBACK           user_function,
	void                    *context,
	void *                  *return_value);

ACPI_STATUS
acpi_get_devices (
	NATIVE_CHAR             *HID,
	WALK_CALLBACK           user_function,
	void                    *context,
	void                    **return_value);

ACPI_STATUS
acpi_get_name (
	ACPI_HANDLE             handle,
	u32                     name_type,
	ACPI_BUFFER             *ret_path_ptr);

ACPI_STATUS
acpi_get_handle (
	ACPI_HANDLE             parent,
	ACPI_STRING             pathname,
	ACPI_HANDLE             *ret_handle);


/*
 * Object manipulation and enumeration
 */

ACPI_STATUS
acpi_evaluate_object (
	ACPI_HANDLE             object,
	ACPI_STRING             pathname,
	ACPI_OBJECT_LIST        *parameter_objects,
	ACPI_BUFFER             *return_object_buffer);

ACPI_STATUS
acpi_get_object_info (
	ACPI_HANDLE             device,
	ACPI_DEVICE_INFO        *info);

ACPI_STATUS
acpi_get_next_object (
	ACPI_OBJECT_TYPE        type,
	ACPI_HANDLE             parent,
	ACPI_HANDLE             child,
	ACPI_HANDLE             *out_handle);

ACPI_STATUS
acpi_get_type (
	ACPI_HANDLE             object,
	ACPI_OBJECT_TYPE        *out_type);

ACPI_STATUS
acpi_get_parent (
	ACPI_HANDLE             object,
	ACPI_HANDLE             *out_handle);


/*
 * Event handler interfaces
 */

ACPI_STATUS
acpi_install_fixed_event_handler (
	u32                     acpi_event,
	FIXED_EVENT_HANDLER     handler,
	void                    *context);

ACPI_STATUS
acpi_remove_fixed_event_handler (
	u32                     acpi_event,
	FIXED_EVENT_HANDLER     handler);

ACPI_STATUS
acpi_install_notify_handler (
	ACPI_HANDLE             device,
	u32                     handler_type,
	NOTIFY_HANDLER          handler,
	void                    *context);

ACPI_STATUS
acpi_remove_notify_handler (
	ACPI_HANDLE             device,
	u32                     handler_type,
	NOTIFY_HANDLER          handler);

ACPI_STATUS
acpi_install_address_space_handler (
	ACPI_HANDLE             device,
	ACPI_ADDRESS_SPACE_TYPE space_id,
	ADDRESS_SPACE_HANDLER   handler,
	ADDRESS_SPACE_SETUP     setup,
	void                    *context);

ACPI_STATUS
acpi_remove_address_space_handler (
	ACPI_HANDLE             device,
	ACPI_ADDRESS_SPACE_TYPE space_id,
	ADDRESS_SPACE_HANDLER   handler);

ACPI_STATUS
acpi_install_gpe_handler (
	u32                     gpe_number,
	u32                     type,
	GPE_HANDLER             handler,
	void                    *context);

ACPI_STATUS
acpi_acquire_global_lock (
	void);

ACPI_STATUS
acpi_release_global_lock (
	void);

ACPI_STATUS
acpi_remove_gpe_handler (
	u32                     gpe_number,
	GPE_HANDLER             handler);

ACPI_STATUS
acpi_enable_event (
	u32                     acpi_event,
	u32                     type);

ACPI_STATUS
acpi_disable_event (
	u32                     acpi_event,
	u32                     type);

ACPI_STATUS
acpi_clear_event (
	u32                     acpi_event,
	u32                     type);

ACPI_STATUS
acpi_get_event_status (
	u32                     acpi_event,
	u32                     type,
	ACPI_EVENT_STATUS       *event_status);

/*
 * Resource interfaces
 */

ACPI_STATUS
acpi_get_current_resources(
	ACPI_HANDLE             device_handle,
	ACPI_BUFFER             *ret_buffer);

ACPI_STATUS
acpi_get_possible_resources(
	ACPI_HANDLE             device_handle,
	ACPI_BUFFER             *ret_buffer);

ACPI_STATUS
acpi_set_current_resources (
	ACPI_HANDLE             device_handle,
	ACPI_BUFFER             *in_buffer);

ACPI_STATUS
acpi_get_irq_routing_table (
	ACPI_HANDLE             bus_device_handle,
	ACPI_BUFFER             *ret_buffer);


/*
 * Hardware (ACPI device) interfaces
 */

ACPI_STATUS
acpi_set_firmware_waking_vector (
	ACPI_PHYSICAL_ADDRESS   physical_address);

ACPI_STATUS
acpi_get_firmware_waking_vector (
	ACPI_PHYSICAL_ADDRESS   *physical_address);

ACPI_STATUS
acpi_enter_sleep_state (
	u8 sleep_state);

ACPI_STATUS
acpi_get_processor_throttling_info (
	ACPI_HANDLE             processor_handle,
	ACPI_BUFFER             *user_buffer);

ACPI_STATUS
acpi_set_processor_throttling_state (
	ACPI_HANDLE             processor_handle,
	u32                     throttle_state);

ACPI_STATUS
acpi_get_processor_throttling_state (
	ACPI_HANDLE             processor_handle,
	u32                     *throttle_state);

ACPI_STATUS
acpi_get_processor_cx_info (
	ACPI_HANDLE             processor_handle,
	ACPI_BUFFER             *user_buffer);

ACPI_STATUS
acpi_set_processor_sleep_state (
	ACPI_HANDLE             processor_handle,
	u32                     cx_state);

ACPI_STATUS
acpi_processor_sleep (
	ACPI_HANDLE             processor_handle,
	u32                     *pm_timer_ticks);


#endif /* __ACXFACE_H__ */
