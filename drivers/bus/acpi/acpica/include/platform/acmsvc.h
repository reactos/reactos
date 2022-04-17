/******************************************************************************
 *
 * Name: acmsvc.h - VC specific defines, etc.
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

#ifndef __ACMSVC_H__
#define __ACMSVC_H__

/* Note: do not include any C library headers here */

/*
 * Note: MSVC project files should define ACPI_DEBUGGER and ACPI_DISASSEMBLER
 * as appropriate to enable editor functions like "Find all references".
 * The editor isn't smart enough to dig through the include files to find
 * out if these are actually defined.
 */

/* Eliminate warnings for "old" (non-secure) versions of clib functions */

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

/* Eliminate warnings for POSIX clib function names (open, write, etc.) */

#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#define COMPILER_DEPENDENT_INT64    __int64
#define COMPILER_DEPENDENT_UINT64   unsigned __int64
#define ACPI_INLINE                 __inline

/*
 * Calling conventions:
 *
 * ACPI_SYSTEM_XFACE        - Interfaces to host OS (handlers, threads)
 * ACPI_EXTERNAL_XFACE      - External ACPI interfaces
 * ACPI_INTERNAL_XFACE      - Internal ACPI interfaces
 * ACPI_INTERNAL_VAR_XFACE  - Internal variable-parameter list interfaces
 */
#define ACPI_SYSTEM_XFACE           __cdecl
#define ACPI_EXTERNAL_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE     __cdecl


/* Do not maintain the architecture specific stuffs for the EFI ports */

#if defined(__i386__) && !defined(_GNU_EFI) && !defined(_EDK2_EFI)
/*
 * Math helper functions
 */
#ifndef ACPI_DIV_64_BY_32
#define ACPI_DIV_64_BY_32(n_hi, n_lo, d32, q32, r32) \
{                           \
    __asm mov    edx, n_hi  \
    __asm mov    eax, n_lo  \
    __asm div    d32        \
    __asm mov    q32, eax   \
    __asm mov    r32, edx   \
}
#endif

#ifndef ACPI_MUL_64_BY_32
#define ACPI_MUL_64_BY_32(n_hi, n_lo, m32, p32, c32) \
{                           \
    __asm mov    edx, n_hi  \
    __asm mov    eax, n_lo  \
    __asm mul    m32        \
    __asm mov    p32, eax   \
    __asm mov    c32, edx   \
}
#endif

#ifndef ACPI_SHIFT_LEFT_64_BY_32
#define ACPI_SHIFT_LEFT_64_BY_32(n_hi, n_lo, s32) \
{                               \
    __asm mov    edx, n_hi      \
    __asm mov    eax, n_lo      \
    __asm mov    ecx, s32       \
    __asm and    ecx, 31        \
    __asm shld   edx, eax, cl   \
    __asm shl    eax, cl        \
    __asm mov    n_hi, edx      \
    __asm mov    n_lo, eax      \
}
#endif

#ifndef ACPI_SHIFT_RIGHT_64_BY_32
#define ACPI_SHIFT_RIGHT_64_BY_32(n_hi, n_lo, s32) \
{                               \
    __asm mov    edx, n_hi      \
    __asm mov    eax, n_lo      \
    __asm mov    ecx, s32       \
    __asm and    ecx, 31        \
    __asm shrd   eax, edx, cl   \
    __asm shr    edx, cl        \
    __asm mov    n_hi, edx      \
    __asm mov    n_lo, eax      \
}
#endif

#ifndef ACPI_SHIFT_RIGHT_64
#define ACPI_SHIFT_RIGHT_64(n_hi, n_lo) \
{                           \
    __asm shr    n_hi, 1    \
    __asm rcr    n_lo, 1    \
}
#endif
#endif

#ifdef __REACTOS__
#ifdef _M_IX86
/* Flush CPU cache - used when going to sleep. Wbinvd or similar. */

#ifdef ACPI_APPLICATION
#define ACPI_FLUSH_CPU_CACHE()
#else
#define ACPI_FLUSH_CPU_CACHE()  __asm {WBINVD}
#endif

/*
 * Global Lock acquire/release code
 *
 * Note: Handles case where the FACS pointer is null
 */
#define ACPI_ACQUIRE_GLOBAL_LOCK(FacsPtr, Acq)  __asm \
{                                                   \
        __asm mov           eax, 0xFF               \
        __asm mov           ecx, FacsPtr            \
        __asm or            ecx, ecx                \
        __asm jz            exit_acq                \
        __asm lea           ecx, [ecx].GlobalLock   \
                                                    \
        __asm acq10:                                \
        __asm mov           eax, [ecx]              \
        __asm mov           edx, eax                \
        __asm and           edx, 0xFFFFFFFE         \
        __asm bts           edx, 1                  \
        __asm adc           edx, 0                  \
        __asm lock cmpxchg  dword ptr [ecx], edx    \
        __asm jnz           acq10                   \
                                                    \
        __asm cmp           dl, 3                   \
        __asm sbb           eax, eax                \
                                                    \
        __asm exit_acq:                             \
        __asm mov           Acq, al                 \
}

#define ACPI_RELEASE_GLOBAL_LOCK(FacsPtr, Pnd) __asm \
{                                                   \
        __asm xor           eax, eax                \
        __asm mov           ecx, FacsPtr            \
        __asm or            ecx, ecx                \
        __asm jz            exit_rel                \
        __asm lea           ecx, [ecx].GlobalLock   \
                                                    \
        __asm Rel10:                                \
        __asm mov           eax, [ecx]              \
        __asm mov           edx, eax                \
        __asm and           edx, 0xFFFFFFFC         \
        __asm lock cmpxchg  dword ptr [ecx], edx    \
        __asm jnz           Rel10                   \
                                                    \
        __asm cmp           dl, 3                   \
        __asm and           eax, 1                  \
                                                    \
        __asm exit_rel:                             \
        __asm mov           Pnd, al                 \
}
#endif /* _M_IX86 */
#endif /* __REACTOS__ */

/* warn C4001: use of slash-slash comments */
/* NOTE: MSVC 2015 headers use these extensively */
#pragma warning(disable:4001)

/* warn C4100: unreferenced formal parameter */
#pragma warning(disable:4100)

/* warn C4127: conditional expression is constant */
#pragma warning(disable:4127)

/* warn C4706: assignment within conditional expression */
#pragma warning(disable:4706)

/* warn C4131: uses old-style declarator (iASL compiler only) */
#pragma warning(disable:4131)

/* warn C4131: uses old-style declarator (iASL compiler only) */
#pragma warning(disable:4459)

/* warn c4200: allow flexible arrays (of zero length) */
#pragma warning(disable:4200)

#if _MSC_VER > 1200 /* Versions above VC++ 6 */
#pragma warning( disable : 4295 ) /* needed for acpredef.h array */
#endif

/*
 * MSVC 2015+
 */

 /* warn C4459: xxxx (identifier) hides global declaration */
#pragma warning(disable:4459)


/* Debug support. */

#ifdef _DEBUG

/*
 * Debugging memory corruption issues with windows:
 * Add #include <crtdbg.h> to accommon.h if necessary.
 * Add _ASSERTE(_CrtCheckMemory()); where needed to test memory integrity.
 * This can quickly localize the memory corruption.
 */
#define ACPI_DEBUG_INITIALIZE() \
    _CrtSetDbgFlag (\
        _CRTDBG_CHECK_ALWAYS_DF | \
        _CRTDBG_ALLOC_MEM_DF | \
        _CRTDBG_DELAY_FREE_MEM_DF | \
        _CRTDBG_LEAK_CHECK_DF | \
        _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));

#if 0
/*
 * _CrtSetBreakAlloc can be used to set a breakpoint at a particular
 * memory leak, add to the macro above.
 */
Detected memory leaks!
Dumping objects ->
..\..\source\os_specific\service_layers\oswinxf.c(701) : {937} normal block at 0x002E9190, 40 bytes long.
 Data: <                > 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

_CrtSetBreakAlloc (937);
#endif

#endif

#if _MSC_VER > 1200 /* Versions above VC++ 6 */
#define COMPILER_VA_MACRO               1
#else
#endif

/* Begin standard headers */

/*
 * warn C4001: nonstandard extension 'single line comment' was used
 *
 * We need to enable this for ACPICA internal files, but disable it for
 * buggy MS runtime headers.
 */
#pragma warning(push)
#pragma warning(disable:4001)

#endif /* __ACMSVC_H__ */
