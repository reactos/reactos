/*
 * endian.h - Defines for endianness handling in NTFS Linux kernel driver.
 *	      Part of the Linux-NTFS project.
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

#ifndef _LINUX_NTFS_ENDIAN_H
#define _LINUX_NTFS_ENDIAN_H

#include <asm/byteorder.h>

/*
 * Signed endianness conversion defines.
 */
#define sle16_to_cpu(x)		((s16)__le16_to_cpu((s16)(x)))
#define sle32_to_cpu(x)		((s32)__le32_to_cpu((s32)(x)))
#define sle64_to_cpu(x)		((s64)__le64_to_cpu((s64)(x)))

#define sle16_to_cpup(x)	((s16)__le16_to_cpu(*(s16*)(x)))
#define sle32_to_cpup(x)	((s32)__le32_to_cpu(*(s32*)(x)))
#define sle64_to_cpup(x)	((s64)__le64_to_cpu(*(s64*)(x)))

#define cpu_to_sle16(x)		((s16)__cpu_to_le16((s16)(x)))
#define cpu_to_sle32(x)		((s32)__cpu_to_le32((s32)(x)))
#define cpu_to_sle64(x)		((s64)__cpu_to_le64((s64)(x)))

#define cpu_to_sle16p(x)	((s16)__cpu_to_le16(*(s16*)(x)))
#define cpu_to_sle32p(x)	((s32)__cpu_to_le32(*(s32*)(x)))
#define cpu_to_sle64p(x)	((s64)__cpu_to_le64(*(s64*)(x)))

#endif /* _LINUX_NTFS_ENDIAN_H */

