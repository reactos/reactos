/******************************************************************************
 *
 * Name: acdragonflyex.h - Extra OS specific defines, etc. for DragonFly BSD
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACDRAGONFLYEX_H__
#define __ACDRAGONFLYEX_H__

#ifdef _KERNEL

#ifdef ACPI_DEBUG_CACHE
ACPI_STATUS
_AcpiOsReleaseObject (
    ACPI_CACHE_T                *Cache,
    void                        *Object,
    const char                  *func,
    int                         line);
#endif

#ifdef ACPI_DEBUG_LOCKS
ACPI_CPU_FLAGS
_AcpiOsAcquireLock (
    ACPI_SPINLOCK               Spin,
    const char                  *func,
    int                         line);
#endif

#ifdef ACPI_DEBUG_MEMMAP
void *
_AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS       Where,
    ACPI_SIZE                   Length,
    const char                  *caller,
    int                         line);

void
_AcpiOsUnmapMemory (
    void                        *LogicalAddress,
    ACPI_SIZE                   Length,
    const char                  *caller,
    int                         line);
#endif

#endif /* _KERNEL */

#endif /* __ACDRAGONFLYEX_H__ */
