/******************************************************************************
 *
 * Name: acos2.h - OS/2 specific defines, etc.
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACOS2_H__
#define __ACOS2_H__

#define ACPI_USE_STANDARD_HEADERS
#define ACPI_USE_SYSTEM_CLIBRARY

#define INCL_LONGLONG
#include <os2.h>


#define ACPI_MACHINE_WIDTH          32

#define COMPILER_DEPENDENT_INT64    long long
#define COMPILER_DEPENDENT_UINT64   unsigned long long
#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_NATIVE_MATH64

#define ACPI_SYSTEM_XFACE           APIENTRY
#define ACPI_EXTERNAL_XFACE         APIENTRY
#define ACPI_INTERNAL_XFACE         APIENTRY
#define ACPI_INTERNAL_VAR_XFACE     APIENTRY

/*
 * Some compilers complain about unused variables. Sometimes we don't want to
 * use all the variables (most specifically for _THIS_MODULE). This allow us
 * to to tell the compiler warning in a per-variable manner that a variable
 * is unused.
 */
#define ACPI_UNUSED_VAR

#include <io.h>

#define ACPI_FLUSH_CPU_CACHE() Wbinvd()
void Wbinvd(void);

#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq)       Acq = OSPMAcquireGlobalLock(GLptr)
#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Pnd)       Pnd = OSPMReleaseGlobalLock(GLptr)
unsigned short OSPMAcquireGlobalLock (void *);
unsigned short OSPMReleaseGlobalLock (void *);

#define ACPI_SHIFT_RIGHT_64(n_hi, n_lo) \
{ \
    unsigned long long val = 0LL; \
    val = n_lo | ( ((unsigned long long)h_hi) << 32 ); \
    __llrotr (val,1); \
    n_hi = (unsigned long)((val >> 32 ) & 0xffffffff ); \
    n_lo = (unsigned long)(val & 0xffffffff); \
}

#ifndef ACPI_ASL_COMPILER
#define ACPI_USE_LOCAL_CACHE
#undef ACPI_DEBUGGER
#endif

#endif /* __ACOS2_H__ */
