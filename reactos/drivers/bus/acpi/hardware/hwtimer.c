
/******************************************************************************
 *
 * Name: hwtimer.c - ACPI Power Management Timer Interface
 *              $Revision: 1.1 $
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

#include "acpi.h"
#include "achware.h"

#define _COMPONENT          ACPI_HARDWARE
	 MODULE_NAME         ("hwtimer")


/******************************************************************************
 *
 * FUNCTION:    Acpi_get_timer_resolution
 *
 * PARAMETERS:  none
 *
 * RETURN:      Number of bits of resolution in the PM Timer (24 or 32).
 *
 * DESCRIPTION: Obtains resolution of the ACPI PM Timer.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_get_timer_resolution (
	u32                     *resolution)
{
	if (!resolution) {
		return (AE_BAD_PARAMETER);
	}

	if (0 == acpi_gbl_FADT->tmr_val_ext) {
		*resolution = 24;
	}
	else {
		*resolution = 32;
	}

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_get_timer
 *
 * PARAMETERS:  none
 *
 * RETURN:      Current value of the ACPI PM Timer (in ticks).
 *
 * DESCRIPTION: Obtains current value of ACPI PM Timer.
 *
 ******************************************************************************/

ACPI_STATUS
acpi_get_timer (
	u32                     *ticks)
{
	if (!ticks) {
		return (AE_BAD_PARAMETER);
	}

	*ticks = acpi_os_in32 ((ACPI_IO_ADDRESS) ACPI_GET_ADDRESS (acpi_gbl_FADT->Xpm_tmr_blk.address));

	return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    Acpi_get_timer_duration
 *
 * PARAMETERS:  Start_ticks
 *              End_ticks
 *              Time_elapsed
 *
 * RETURN:      Time_elapsed
 *
 * DESCRIPTION: Computes the time elapsed (in microseconds) between two
 *              PM Timer time stamps, taking into account the possibility of
 *              rollovers, the timer resolution, and timer frequency.
 *
 *              The PM Timer's clock ticks at roughly 3.6 times per
 *              _microsecond_, and its clock continues through Cx state
 *              transitions (unlike many CPU timestamp counters) -- making it
 *              a versatile and accurate timer.
 *
 *              Note that this function accomodates only a single timer
 *              rollover.  Thus for 24-bit timers, this function should only
 *              be used for calculating durations less than ~4.6 seconds
 *              (~20 hours for 32-bit timers).
 *
 ******************************************************************************/

ACPI_STATUS
acpi_get_timer_duration (
	u32                     start_ticks,
	u32                     end_ticks,
	u32                     *time_elapsed)
{
	u32                     delta_ticks = 0;
	u32                     seconds = 0;
	u32                     milliseconds = 0;
	u32                     microseconds = 0;
	u32                     remainder = 0;

	if (!time_elapsed) {
		return (AE_BAD_PARAMETER);
	}

	/*
	 * Compute Tick Delta:
	 * -------------------
	 * Handle (max one) timer rollovers on 24- versus 32-bit timers.
	 */
	if (start_ticks < end_ticks) {
		delta_ticks = end_ticks - start_ticks;
	}
	else if (start_ticks > end_ticks) {
		/* 24-bit Timer */
		if (0 == acpi_gbl_FADT->tmr_val_ext) {
			delta_ticks = (((0x00FFFFFF - start_ticks) + end_ticks) & 0x00FFFFFF);
		}
		/* 32-bit Timer */
		else {
			delta_ticks = (0xFFFFFFFF - start_ticks) + end_ticks;
		}
	}
	else {
		*time_elapsed = 0;
		return (AE_OK);
	}

	/*
	 * Compute Duration:
	 * -----------------
	 * Since certain compilers (gcc/Linux, argh!) don't support 64-bit
	 * divides in kernel-space we have to do some trickery to preserve
	 * accuracy while using 32-bit math.
	 *
	 * TODO: Change to use 64-bit math when supported.
	 *
	 * The process is as follows:
	 *  1. Compute the number of seconds by dividing Delta Ticks by
	 *     the timer frequency.
	 *  2. Compute the number of milliseconds in the remainder from step #1
	 *     by multiplying by 1000 and then dividing by the timer frequency.
	 *  3. Compute the number of microseconds in the remainder from step #2
	 *     by multiplying by 1000 and then dividing by the timer frequency.
	 *  4. Add the results from steps 1, 2, and 3 to get the total duration.
	 *
	 * Example: The time elapsed for Delta_ticks = 0xFFFFFFFF should be
	 *          1199864031 microseconds.  This is computed as follows:
	 *          Step #1: Seconds = 1199; Remainder = 3092840
	 *          Step #2: Milliseconds = 864; Remainder = 113120
	 *          Step #3: Microseconds = 31; Remainder = <don't care!>
	 */

	/* Step #1 */
	seconds = delta_ticks / PM_TIMER_FREQUENCY;
	remainder = delta_ticks % PM_TIMER_FREQUENCY;

	/* Step #2 */
	milliseconds = (remainder * 1000) / PM_TIMER_FREQUENCY;
	remainder = (remainder * 1000) % PM_TIMER_FREQUENCY;

	/* Step #3 */
	microseconds = (remainder * 1000) / PM_TIMER_FREQUENCY;

	/* Step #4 */
	*time_elapsed = seconds * 1000000;
	*time_elapsed += milliseconds * 1000;
	*time_elapsed += microseconds;

	return (AE_OK);
}


