/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * FILE:            ntoskrnl/include/internal/i386/segment.h
 * PURPOSE:         Segment selector definitions
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_I386_SEGMENT_H
#define __NTOSKRNL_INCLUDE_INTERNAL_i386_SEGMENT_H

#define NULL_SELECTOR        (0x0)
#define KERNEL_CS            (0x8)
#define KERNEL_DS            (0x10)
#define USER_CS              (0x18 + 0x3)
#define USER_DS              (0x20 + 0x3)
/* Task State Segment */
#define TSS_SELECTOR         (0x28)
/* Processor Control Region */
#define PCR_SELECTOR         (0x30)
/* Thread Environment Block */
#define TEB_SELECTOR         (0x38 + 0x3)
#define RESERVED_SELECTOR   (0x40)
/* Local Descriptor Table */
#define LDT_SELECTOR         (0x48)
#define TRAP_TSS_SELECTOR    (0x50)

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_I386_SEGMENT_H */




