/******************************************************************************
 *
 * Name: achware.h -- hardware specific interfaces
 *       $Revision: 1.1 $
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

#ifndef __ACHWARE_H__
#define __ACHWARE_H__


/* PM Timer ticks per second (HZ) */
#define PM_TIMER_FREQUENCY  3579545


/* Prototypes */


ACPI_STATUS
acpi_hw_initialize (
	void);

ACPI_STATUS
acpi_hw_shutdown (
	void);

ACPI_STATUS
acpi_hw_initialize_system_info (
	void);

ACPI_STATUS
acpi_hw_set_mode (
	u32                     mode);

u32
acpi_hw_get_mode (
	void);

u32
acpi_hw_get_mode_capabilities (
	void);

/* Register I/O Prototypes */


u32
acpi_hw_register_bit_access (
	NATIVE_UINT             read_write,
	u8                      use_lock,
	u32                     register_id,
	... /* DWORD Write Value */);

u32
acpi_hw_register_read (
	u8                      use_lock,
	u32                     register_id);

void
acpi_hw_register_write (
	u8                      use_lock,
	u32                     register_id,
	u32                     value);

u32
acpi_hw_low_level_read (
	u32                     width,
	ACPI_GAS                *reg,
	u32                     offset);

void
acpi_hw_low_level_write (
	u32                     width,
	u32                     value,
	ACPI_GAS                *reg,
	u32                     offset);

void
acpi_hw_clear_acpi_status (
   void);

u32
acpi_hw_get_bit_shift (
	u32                     mask);


/* GPE support */

void
acpi_hw_enable_gpe (
	u32                     gpe_index);

void
acpi_hw_disable_gpe (
	u32                     gpe_index);

void
acpi_hw_clear_gpe (
	u32                     gpe_index);

void
acpi_hw_get_gpe_status (
	u32                     gpe_number,
	ACPI_EVENT_STATUS       *event_status);

/* Sleep Prototypes */

ACPI_STATUS
acpi_hw_obtain_sleep_type_register_data (
	u8                      sleep_state,
	u8                      *slp_typ_a,
	u8                      *slp_typ_b);


/* ACPI Timer prototypes */

ACPI_STATUS
acpi_get_timer_resolution (
	u32                     *resolution);

ACPI_STATUS
acpi_get_timer (
	u32                     *ticks);

ACPI_STATUS
acpi_get_timer_duration (
	u32                     start_ticks,
	u32                     end_ticks,
	u32                     *time_elapsed);


#endif /* __ACHWARE_H__ */
