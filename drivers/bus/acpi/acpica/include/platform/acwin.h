/******************************************************************************
 *
 * Name: acwin.h - OS specific defines, etc.
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

#ifndef __ACWIN_H__
#define __ACWIN_H__

#include <io.h>

#define ACPI_USE_STANDARD_HEADERS
#define ACPI_USE_SYSTEM_CLIBRARY

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


#define ACPI_MACHINE_WIDTH      32
#define ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_NATIVE_MATH64

#ifdef ACPI_DEFINE_ALTERNATE_TYPES
/*
 * Types used only in (Linux) translated source, defined here to enable
 * cross-platform compilation (i.e., generate the Linux code on Windows,
 * for test purposes only)
 */
typedef int                             s32;
typedef unsigned char                   u8;
typedef unsigned short                  u16;
typedef unsigned int                    u32;
typedef COMPILER_DEPENDENT_UINT64       u64;
#endif

/*
 * Map low I/O functions for MS. This allows us to disable MS language
 * extensions for maximum portability.
 */
#define open            _open
#define read            _read
#define write           _write
#define close           _close
#define stat            _stat
#define fstat           _fstat
#define mkdir           _mkdir
#define fileno          _fileno
#define isatty          _isatty

#if _MSC_VER <= 1200 /* Versions below VC++ 6 */
#define vsnprintf       _vsnprintf
#endif
#define O_RDONLY        _O_RDONLY
#define O_BINARY        _O_BINARY
#define O_CREAT         _O_CREAT
#define O_WRONLY        _O_WRONLY
#define O_TRUNC         _O_TRUNC
#define S_IREAD         _S_IREAD
#define S_IWRITE        _S_IWRITE
#define S_IFDIR         _S_IFDIR
#if _MSC_VER < 1900
#define snprintf        _snprintf
#endif


/*
 * Handle platform- and compiler-specific assembly language differences.
 *
 * Notes:
 * 1) Interrupt 3 is used to break into a debugger
 * 2) Interrupts are turned off during ACPI register setup
 */

/*! [Begin] no source code translation  */

#ifndef __REACTOS__
#ifdef ACPI_APPLICATION
#define ACPI_FLUSH_CPU_CACHE()
#else
#define ACPI_FLUSH_CPU_CACHE()  __asm {WBINVD}
#endif
#endif /* __REACTOS__ */

#ifdef _DEBUG
#define ACPI_SIMPLE_RETURN_MACROS
#endif

/*! [End] no source code translation !*/

#ifndef __REACTOS__
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
#endif /* __REACTOS__ */

#endif /* __ACWIN_H__ */
