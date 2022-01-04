/******************************************************************************
 *
 * Name: acnetbsd.h - OS specific defines, etc.
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

#ifndef __ACNETBSD_H__
#define __ACNETBSD_H__

#define ACPI_UINTPTR_T          uintptr_t
#define ACPI_USE_LOCAL_CACHE
#define ACPI_CAST_PTHREAD_T(x)  ((ACPI_THREAD_ID) ACPI_TO_INTEGER (x))

#ifdef _LP64
#define ACPI_MACHINE_WIDTH      64
#else
#define ACPI_MACHINE_WIDTH      32
#endif

#define COMPILER_DEPENDENT_INT64  int64_t
#define COMPILER_DEPENDENT_UINT64 uint64_t

#if defined(_KERNEL) || defined(_STANDALONE)
#ifdef _KERNEL_OPT
#include "opt_acpi.h"           /* collect build-time options here */
#endif /* _KERNEL_OPT */

#include <sys/param.h>
#include <sys/systm.h>
#include <machine/stdarg.h>
#include <machine/acpi_func.h>

#define asm         __asm

#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_NATIVE_MATH64

#define ACPI_SYSTEM_XFACE
#define ACPI_EXTERNAL_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE

#ifdef ACPI_DEBUG
#define ACPI_DEBUG_OUTPUT
#define ACPI_DBG_TRACK_ALLOCATIONS
#ifdef DEBUGGER_THREADING
#undef DEBUGGER_THREADING
#endif /* DEBUGGER_THREADING */
#define DEBUGGER_THREADING 0    /* integrated with DDB */
#include "opt_ddb.h"
#ifdef DDB
#define ACPI_DISASSEMBLER
#define ACPI_DEBUGGER
#endif /* DDB */
#endif /* ACPI_DEBUG */

#else /* defined(_KERNEL) || defined(_STANDALONE) */

#include <ctype.h>
#include <stdint.h>

/* Not building kernel code, so use libc */
#define ACPI_USE_STANDARD_HEADERS

#define __cli()
#define __sti()
#define __cdecl

#endif /* defined(_KERNEL) || defined(_STANDALONE) */

/* Always use NetBSD code over our local versions */
#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_NATIVE_MATH64

#endif /* __ACNETBSD_H__ */
