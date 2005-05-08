/*
 * time.c - NTFS time conversion functions. Part of the Linux-NTFS project.
 *
 * Copyright (c) 2001 Anton Altaparmakov.
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS 
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/sched.h>	/* For CURRENT_TIME. */
#include <asm/div64.h>		/* For do_div(). */

#include "ntfs.h"

#define NTFS_TIME_OFFSET ((s64)(369 * 365 + 89) * 24 * 3600 * 10000000)

/**
 * utc2ntfs - convert Linux time to NTFS time
 * @time:		Linux time to convert to NTFS
 *
 * Convert the Linux time @time to its corresponding NTFS time and return that
 * in little endian format.
 *
 * Linux stores time in a long at present and measures it as the number of
 * 1-second intervals since 1st January 1970, 00:00:00 UTC.
 *
 * NTFS uses Microsoft's standard time format which is stored in a s64 and is
 * measured as the number of 100 nano-second intervals since 1st January 1601,
 * 00:00:00 UTC.
 */
inline s64 utc2ntfs(const time_t time)
{
	/* Convert to 100ns intervals and then add the NTFS time offset. */
	return cpu_to_sle64((s64)time * 10000000 + NTFS_TIME_OFFSET);
}

/**
 * get_current_ntfs_time - get the current time in little endian NTFS format
 *
 * Get the current time from the Linux kernel, convert it to its corresponding
 * NTFS time and return that in little endian format.
 */
inline s64 get_current_ntfs_time(void)
{
	/* ignores leap second */
	return utc2ntfs(get_seconds()) + xtime.tv_nsec/1000;
}

/**
 * ntfs2utc - convert NTFS time to Linux time
 * @time:		NTFS time (little endian) to convert to Linux
 *
 * Convert the little endian NTFS time @time to its corresponding Linux time
 * and return that in cpu format.
 *
 * Linux stores time in a long at present and measures it as the number of
 * 1-second intervals since 1st January 1970, 00:00:00 UTC.
 *
 * NTFS uses Microsoft's standard time format which is stored in a s64 and is
 * measured as the number of 100 nano-second intervals since 1st January 1601,
 * 00:00:00 UTC.
 */
inline time_t ntfs2utc(const s64 time)
{
	/* Subtract the NTFS time offset, then convert to 1s intervals. */
	s64 t = sle64_to_cpu(time) - NTFS_TIME_OFFSET;
	do_div(t, 10000000);
	return (time_t)t;
}

