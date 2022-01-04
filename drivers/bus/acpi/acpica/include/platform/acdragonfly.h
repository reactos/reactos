/******************************************************************************
 *
 * Name: acdragonfly.h - OS specific for DragonFly BSD
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

#ifndef __ACDRAGONFLY_H_
#define __ACDRAGONFLY_H_

#include <sys/types.h>

#ifdef __LP64__
#define ACPI_MACHINE_WIDTH              64
#else
#define ACPI_MACHINE_WIDTH              32
#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_NATIVE_MATH64
#endif

#define ACPI_UINTPTR_T                  uintptr_t
#define COMPILER_DEPENDENT_INT64        int64_t
#define COMPILER_DEPENDENT_UINT64       uint64_t

#define ACPI_USE_DO_WHILE_0
#define ACPI_USE_SYSTEM_CLIBRARY

#ifdef _KERNEL

#include "opt_acpi.h"
#include <sys/ctype.h>
#include <sys/systm.h>
#include <machine/acpica_machdep.h>
#include <stdarg.h>

#ifdef ACPI_DEBUG
#define ACPI_DEBUG_OUTPUT       /* enable debug output */
#ifdef DEBUGGER_THREADING
#undef DEBUGGER_THREADING
#endif /* DEBUGGER_THREADING */
#define DEBUGGER_THREADING DEBUGGER_SINGLE_THREADED /* integrated with DDB */
#include "opt_ddb.h"
#ifdef DDB
#define ACPI_DEBUGGER
#endif /* DDB */
#define ACPI_DISASSEMBLER
#endif

#ifdef ACPI_DEBUG_CACHE
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseObject
#define AcpiOsReleaseObject(Cache, Object) \
        _AcpiOsReleaseObject((Cache), (Object), __func__, __LINE__)
#endif

#ifdef ACPI_DEBUG_LOCKS
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireLock
#define AcpiOsAcquireLock(Handle) \
        _AcpiOsAcquireLock((Handle), __func__, __LINE__)
#endif

#ifdef ACPI_DEBUG_MEMMAP
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsMapMemory
#define AcpiOsMapMemory(Where, Length) \
        _AcpiOsMapMemory((Where), (Length), __func__, __LINE__)

#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsUnmapMemory
#define AcpiOsUnmapMemory(LogicalAddress, Size) \
        _AcpiOsUnmapMemory((LogicalAddress), (Size), __func__, __LINE__)
#endif

/* XXX TBI */
#define ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitEventsComplete
#define AcpiOsWaitEventsComplete()

#define USE_NATIVE_ALLOCATE_ZEROED

#define ACPI_SPINLOCK   struct acpi_spinlock *
struct acpi_spinlock;

#define ACPI_CACHE_T    struct acpicache
struct acpicache;

#else /* _KERNEL */

#define ACPI_USE_STANDARD_HEADERS

#define ACPI_CAST_PTHREAD_T(pthread)    ((ACPI_THREAD_ID) ACPI_TO_INTEGER (pthread))
#define ACPI_FLUSH_CPU_CACHE()

#endif /* _KERNEL */

#endif /* __ACDRAGONFLY_H_ */
