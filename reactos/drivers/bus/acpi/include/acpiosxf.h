
/******************************************************************************
 *
 * Name: acpiosxf.h - All interfaces to the OS Services Layer (OSL).  These
 *                    interfaces must be implemented by OSL to interface the
 *                    ACPI components to the host operating system.
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

#ifndef __ACPIOSXF_H__
#define __ACPIOSXF_H__

#include "platform/acenv.h"
#include "actypes.h"


/* Priorities for Acpi_os_queue_for_execution */

#define OSD_PRIORITY_GPE            1
#define OSD_PRIORITY_HIGH           2
#define OSD_PRIORITY_MED            3
#define OSD_PRIORITY_LO             4

#define ACPI_NO_UNIT_LIMIT          ((u32) -1)
#define ACPI_MUTEX_SEM              1


/*
 * Types specific to the OS service interfaces
 */

typedef
u32 (*OSD_HANDLER) (
	void                    *context);

typedef
void (*OSD_EXECUTION_CALLBACK) (
	void                    *context);


/*
 * OSL Initialization and shutdown primitives
 */

ACPI_STATUS
acpi_os_initialize (
	void);

ACPI_STATUS
acpi_os_terminate (
	void);


/*
 * Synchronization primitives
 */

ACPI_STATUS
acpi_os_create_semaphore (
	u32                     max_units,
	u32                     initial_units,
	ACPI_HANDLE             *out_handle);

ACPI_STATUS
acpi_os_delete_semaphore (
	ACPI_HANDLE             handle);

ACPI_STATUS
acpi_os_wait_semaphore (
	ACPI_HANDLE             handle,
	u32                     units,
	u32                     timeout);

ACPI_STATUS
acpi_os_signal_semaphore (
	ACPI_HANDLE             handle,
	u32                     units);


/*
 * Memory allocation and mapping
 */

void *
acpi_os_allocate (
	u32                     size);

void *
acpi_os_callocate (
	u32                     size);

void
acpi_os_free (
	void *                  memory);

ACPI_STATUS
acpi_os_map_memory (
	ACPI_PHYSICAL_ADDRESS   physical_address,
	u32                     length,
	void                    **logical_address);

void
acpi_os_unmap_memory (
	void                    *logical_address,
	u32                     length);

ACPI_STATUS
acpi_os_get_physical_address (
	void                    *logical_address,
	ACPI_PHYSICAL_ADDRESS   *physical_address);


/*
 * Interrupt handlers
 */

ACPI_STATUS
acpi_os_install_interrupt_handler (
	u32                     interrupt_number,
	OSD_HANDLER             service_routine,
	void                    *context);

ACPI_STATUS
acpi_os_remove_interrupt_handler (
	u32                     interrupt_number,
	OSD_HANDLER             service_routine);


/*
 * Threads and Scheduling
 */

u32
acpi_os_get_thread_id (
	void);

ACPI_STATUS
acpi_os_queue_for_execution (
	u32                     priority,
	OSD_EXECUTION_CALLBACK  function,
	void                    *context);

void
acpi_os_sleep (
	u32                     seconds,
	u32                     milliseconds);

void
acpi_os_sleep_usec (
	u32                     microseconds);


/*
 * Platform/Hardware independent I/O interfaces
 */

u8
acpi_os_in8 (
	ACPI_IO_ADDRESS         in_port);


u16
acpi_os_in16 (
	ACPI_IO_ADDRESS         in_port);

u32
acpi_os_in32 (
	ACPI_IO_ADDRESS         in_port);

void
acpi_os_out8 (
	ACPI_IO_ADDRESS         out_port,
	u8                      value);

void
acpi_os_out16 (
	ACPI_IO_ADDRESS         out_port,
	u16                     value);

void
acpi_os_out32 (
	ACPI_IO_ADDRESS         out_port,
	u32                     value);


/*
 * Platform/Hardware independent physical memory interfaces
 */

u8
acpi_os_mem_in8 (
	ACPI_PHYSICAL_ADDRESS   in_addr);

u16
acpi_os_mem_in16 (
	ACPI_PHYSICAL_ADDRESS   in_addr);

u32
acpi_os_mem_in32 (
	ACPI_PHYSICAL_ADDRESS   in_addr);

void
acpi_os_mem_out8 (
	ACPI_PHYSICAL_ADDRESS   out_addr,
	u8                      value);

void
acpi_os_mem_out16 (
	ACPI_PHYSICAL_ADDRESS   out_addr,
	u16                     value);

void
acpi_os_mem_out32 (
	ACPI_PHYSICAL_ADDRESS   out_addr,
	u32                     value);


/*
 * Standard access to PCI configuration space
 */

ACPI_STATUS
acpi_os_read_pci_cfg_byte (
	u32                     bus,
	u32                     device_function,
	u32                     register,
	u8                      *value);

ACPI_STATUS
acpi_os_read_pci_cfg_word (
	u32                     bus,
	u32                     device_function,
	u32                     register,
	u16                     *value);

ACPI_STATUS
acpi_os_read_pci_cfg_dword (
	u32                     bus,
	u32                     device_function,
	u32                     register,
	u32                     *value);

ACPI_STATUS
acpi_os_write_pci_cfg_byte (
	u32                     bus,
	u32                     device_function,
	u32                     register,
	u8                      value);

ACPI_STATUS
acpi_os_write_pci_cfg_word (
	u32                     bus,
	u32                     device_function,
	u32                     register,
	u16                     value);


ACPI_STATUS
acpi_os_write_pci_cfg_dword (
	u32                     bus,
	u32                     device_function,
	u32                     register,
	u32                     value);


/*
 * Miscellaneous
 */

ACPI_STATUS
acpi_os_breakpoint (
	NATIVE_CHAR             *message);

u8
acpi_os_readable (
	void                    *pointer,
	u32                     length);


u8
acpi_os_writable (
	void                    *pointer,
	u32                     length);


/*
 * Debug print routines
 */

s32
acpi_os_printf (
	const NATIVE_CHAR       *format,
	...);

s32
acpi_os_vprintf (
	const NATIVE_CHAR       *format,
	va_list                 args);


/*
 * Debug input
 */

u32
acpi_os_get_line (
	NATIVE_CHAR             *buffer);


/*
 * Debug
 */

void
acpi_os_dbg_assert(
	void                    *failed_assertion,
	void                    *file_name,
	u32                     line_number,
	NATIVE_CHAR             *message);


#endif /* __ACPIOSXF_H__ */
