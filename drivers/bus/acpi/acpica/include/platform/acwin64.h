/******************************************************************************
 *
 * Name: acwin64.h - OS specific defines, etc.
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2022, Intel Corp.
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

#ifndef __ACWIN64_H__
#define __ACWIN64_H__

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


#define ACPI_MACHINE_WIDTH          64

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
#define snprintf        _snprintf
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

/*
 * Handle platform- and compiler-specific assembly language differences.
 *
 * Notes:
 * 1) Interrupt 3 is used to break into a debugger
 * 2) Interrupts are turned off during ACPI register setup
 */

/*! [Begin] no source code translation  */

#ifndef __REACTOS__
#define ACPI_FLUSH_CPU_CACHE()

/*
 * For Acpi applications, we don't want to try to access the global lock
 */
#ifdef ACPI_APPLICATION
#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq)       if (AcpiGbl_GlobalLockPresent) {Acq = 0xFF;} else {Acq = 0;}
#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Pnd)       if (AcpiGbl_GlobalLockPresent) {Pnd = 0xFF;} else {Pnd = 0;}
#else

#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq)

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Pnd)

#endif

#else /* __REACTOS__ */

#ifdef ACPI_APPLICATION
#define ACPI_FLUSH_CPU_CACHE()
#else
#define ACPI_FLUSH_CPU_CACHE()  __wbinvd()
#endif

#define ACPI_ACQUIRE_GLOBAL_LOCK(FacsPtr, Acq) \
{ \
    BOOLEAN acquired = 0xFF; \
\
    if ((FacsPtr) != 0) \
    { \
        UINT32 compare, prev, newval; \
        UINT32* lock = &((FacsPtr)->GlobalLock); \
        do \
        { \
            compare = *lock; \
            newval = (compare & ~1) | ((compare >> 1) & 1) | 2; \
            prev = InterlockedCompareExchange(lock, newval, compare); \
        } while (prev != compare); \
        acquired = ((newval & 0xFF) < 3) ? 0xFF : 0x00; \
    } \
    (Acq) = acquired; \
}

#define ACPI_RELEASE_GLOBAL_LOCK(FacsPtr, Pnd) \
{ \
    BOOLEAN pending = 0; \
\
    if ((FacsPtr) != 0) \
    { \
        pending = InterlockedAnd(&(FacsPtr)->GlobalLock, ~3) & 1; \
    } \
    (Pnd) = pending; \
}

#endif /* __REACTOS__ */

/*! [End] no source code translation !*/

#endif /* __ACWIN_H__ */
