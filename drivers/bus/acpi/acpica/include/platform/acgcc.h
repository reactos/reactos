/******************************************************************************
 *
 * Name: acgcc.h - GCC specific defines, etc.
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

#ifndef __ACGCC_H__
#define __ACGCC_H__

/*
 * Use compiler specific <stdarg.h> is a good practice for even when
 * -nostdinc is specified (i.e., ACPI_USE_STANDARD_HEADERS undefined.
 */
#ifndef va_arg
#ifdef ACPI_USE_BUILTIN_STDARG
typedef __builtin_va_list       va_list;
#define va_start(v, l)          __builtin_va_start(v, l)
#define va_end(v)               __builtin_va_end(v)
#define va_arg(v, l)            __builtin_va_arg(v, l)
#define va_copy(d, s)           __builtin_va_copy(d, s)
#else
#include <stdarg.h>
#endif
#endif

#define ACPI_INLINE             __inline__

/* Function name is used for debug output. Non-ANSI, compiler-dependent */

#define ACPI_GET_FUNCTION_NAME          __func__

/*
 * This macro is used to tag functions as "printf-like" because
 * some compilers (like GCC) can catch printf format string problems.
 */
#define ACPI_PRINTF_LIKE(c) __attribute__ ((__format__ (__printf__, c, c+1)))

/*
 * Some compilers complain about unused variables. Sometimes we don't want to
 * use all the variables (for example, _AcpiModuleName). This allows us
 * to tell the compiler warning in a per-variable manner that a variable
 * is unused.
 */
#define ACPI_UNUSED_VAR __attribute__ ((unused))

/* GCC supports __VA_ARGS__ in macros */

#define COMPILER_VA_MACRO               1

/* GCC supports native multiply/shift on 32-bit platforms */

#define ACPI_USE_NATIVE_MATH64

/* GCC did not support __has_attribute until 5.1. */

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

/*
 * Explicitly mark intentional explicit fallthrough to silence
 * -Wimplicit-fallthrough in GCC 7.1+.
 */

#if __has_attribute(__fallthrough__)
#define ACPI_FALLTHROUGH __attribute__((__fallthrough__))
#endif

#ifdef __REACTOS__
/* Flush CPU cache - used when going to sleep. Wbinvd or similar. */

#ifdef ACPI_APPLICATION
#define ACPI_FLUSH_CPU_CACHE()
#else
#define ACPI_FLUSH_CPU_CACHE()  asm ("WBINVD")
#endif

/*
 * Global Lock acquire/release code
 *
 * Note: Handles case where the FACS pointer is null
 */
#define ACPI_ACQUIRE_GLOBAL_LOCK(FacsPtr, Acq)              \
do {                                                        \
    asm("orl    %2,%2;"                                     \
        "jz     2f;"                                        \
        "leal   16(%2),%2;"                                 \
        "1:"                                                \
        "movl   (%2),%%eax;"                                \
        "movl   %%eax,%%edx;"                               \
        "andl   %3,%%edx;"                                  \
        "btsl   $0x1,%%edx;"                                \
        "adcl   $0x0,%%edx;"                                \
        "lock;  cmpxchgl %%edx,(%2);"                       \
        "jnz    1b;"                                        \
        "cmpb   $0x3,%%dl;"                                 \
        "sbbl   %%eax,%%eax;"                               \
        "2:"                                                \
        :"=a"(Acq):"a"(0xFF),"c"(FacsPtr),"i"(~1L):"edx");\
} while(0)

#define ACPI_RELEASE_GLOBAL_LOCK(FacsPtr, Acq)              \
do {                                                        \
    asm("orl    %2,%2;"                                     \
        "jz     2f;"                                        \
        "leal   16(%2),%2;"                                 \
        "1:"                                                \
        "movl  (%2),%%eax;"                                 \
        "movl   %%eax,%%edx;"                               \
        "andl   %3,%%edx;"                                  \
        "lock;  cmpxchgl %%edx,(%2);"                       \
        "jnz    1b;"                                        \
        "cmpb   $0x3,%%dl;" /* FIXME: this is pointless */  \
        "andl   $0x1,%%eax;"                                \
        "2:"                                                \
        :"=a"(Acq):"a"(0),"c"(FacsPtr),"i"(~3L):"edx");\
} while(0)

#endif /* __REACTOS__ */
#endif /* __ACGCC_H__ */
