/******************************************************************************
 *
 * Name: achaiku.h - OS specific defines, etc. for Haiku (www.haiku-os.org)
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

#ifndef __ACHAIKU_H__
#define __ACHAIKU_H__

#define ACPI_USE_STANDARD_HEADERS
#define ACPI_USE_SYSTEM_CLIBRARY

#include <KernelExport.h>

struct mutex;


/* Host-dependent types and defines for user- and kernel-space ACPICA */

#define ACPI_MUTEX_TYPE             ACPI_OSL_MUTEX
#define ACPI_MUTEX                  struct mutex *

#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_NATIVE_MATH64

/* #define ACPI_THREAD_ID               thread_id */

#define ACPI_SEMAPHORE              sem_id
#define ACPI_SPINLOCK               spinlock *
#define ACPI_CPU_FLAGS              cpu_status

#define COMPILER_DEPENDENT_INT64    int64
#define COMPILER_DEPENDENT_UINT64   uint64


#ifdef B_HAIKU_64_BIT
#define ACPI_MACHINE_WIDTH          64
#else
#define ACPI_MACHINE_WIDTH          32
#endif


#ifdef _KERNEL_MODE
/* Host-dependent types and defines for in-kernel ACPICA */

/* ACPICA cache implementation is adequate. */
#define ACPI_USE_LOCAL_CACHE

#define ACPI_FLUSH_CPU_CACHE() __asm __volatile("wbinvd");

/* Based on FreeBSD's due to lack of documentation */
extern int AcpiOsAcquireGlobalLock(uint32 *lock);
extern int AcpiOsReleaseGlobalLock(uint32 *lock);

#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq)    do {                \
        (Acq) = AcpiOsAcquireGlobalLock(&((GLptr)->GlobalLock));    \
} while (0)

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq)    do {                \
        (Acq) = AcpiOsReleaseGlobalLock(&((GLptr)->GlobalLock));    \
} while (0)

#else /* _KERNEL_MODE */
/* Host-dependent types and defines for user-space ACPICA */

#error "We only support kernel mode ACPI atm."

#endif /* _KERNEL_MODE */
#endif /* __ACHAIKU_H__ */
