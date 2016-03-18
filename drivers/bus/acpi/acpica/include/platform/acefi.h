/******************************************************************************
 *
 * Name: acefi.h - OS specific defines, etc.
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
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

#ifndef __ACEFI_H__
#define __ACEFI_H__

#include <stdarg.h>
#if defined(_GNU_EFI)
#include <stdint.h>
#include <unistd.h>
#endif

#if defined(__x86_64__)
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
#define USE_MS_ABI 1
#endif
#endif

#ifdef _MSC_EXTENSIONS
#define EFIAPI __cdecl
#elif USE_MS_ABI
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

typedef uint8_t     UINT8;
typedef uint16_t    UINT16;
typedef int16_t     INT16;
typedef uint32_t    UINT32;
typedef int32_t     INT32;
typedef uint64_t    UINT64;
typedef int64_t     INT64;
typedef uint8_t     BOOLEAN;
typedef uint16_t    CHAR16;

#define VOID        void

#if defined(__ia64__) || defined(__x86_64__)

#define ACPI_MACHINE_WIDTH          64

#if defined(__x86_64__)

/* for x86_64, EFI_FUNCTION_WRAPPER must be defined */

#ifndef USE_MS_ABI
#define USE_EFI_FUNCTION_WRAPPER
#endif

#ifdef _MSC_EXTENSIONS
#pragma warning ( disable : 4731 )  /* Suppress warnings about modification of EBP */
#endif

#endif

typedef uint64_t    UINTN;
typedef int64_t     INTN;

#define EFIERR(a)           (0x8000000000000000 | a)

#else

#define ACPI_MACHINE_WIDTH          32
#define ACPI_USE_NATIVE_DIVIDE

typedef uint32_t UINTN;
typedef int32_t INTN;

#define EFIERR(a)           (0x80000000 | a)

#endif


#ifdef USE_EFI_FUNCTION_WRAPPER
#define __VA_NARG__(...)                        \
  __VA_NARG_(_0, ## __VA_ARGS__, __RSEQ_N())
#define __VA_NARG_(...)                         \
  __VA_ARG_N(__VA_ARGS__)
#define __VA_ARG_N(                             \
  _0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
#define __RSEQ_N()                              \
  10, 9,  8,  7,  6,  5,  4,  3,  2,  1,  0

#define __VA_ARG_NSUFFIX__(prefix,...)                  \
  __VA_ARG_NSUFFIX_N(prefix, __VA_NARG__(__VA_ARGS__))
#define __VA_ARG_NSUFFIX_N(prefix,nargs)        \
  __VA_ARG_NSUFFIX_N_(prefix, nargs)
#define __VA_ARG_NSUFFIX_N_(prefix,nargs)       \
  prefix ## nargs

/* Prototypes of EFI cdecl -> stdcall trampolines */

UINT64 efi_call0(void *func);
UINT64 efi_call1(void *func, UINT64 arg1);
UINT64 efi_call2(void *func, UINT64 arg1, UINT64 arg2);
UINT64 efi_call3(void *func, UINT64 arg1, UINT64 arg2, UINT64 arg3);
UINT64 efi_call4(void *func, UINT64 arg1, UINT64 arg2, UINT64 arg3,
                 UINT64 arg4);
UINT64 efi_call5(void *func, UINT64 arg1, UINT64 arg2, UINT64 arg3,
                 UINT64 arg4, UINT64 arg5);
UINT64 efi_call6(void *func, UINT64 arg1, UINT64 arg2, UINT64 arg3,
                 UINT64 arg4, UINT64 arg5, UINT64 arg6);
UINT64 efi_call7(void *func, UINT64 arg1, UINT64 arg2, UINT64 arg3,
                 UINT64 arg4, UINT64 arg5, UINT64 arg6, UINT64 arg7);
UINT64 efi_call8(void *func, UINT64 arg1, UINT64 arg2, UINT64 arg3,
                 UINT64 arg4, UINT64 arg5, UINT64 arg6, UINT64 arg7,
                 UINT64 arg8);
UINT64 efi_call9(void *func, UINT64 arg1, UINT64 arg2, UINT64 arg3,
                 UINT64 arg4, UINT64 arg5, UINT64 arg6, UINT64 arg7,
                 UINT64 arg8, UINT64 arg9);
UINT64 efi_call10(void *func, UINT64 arg1, UINT64 arg2, UINT64 arg3,
                  UINT64 arg4, UINT64 arg5, UINT64 arg6, UINT64 arg7,
                  UINT64 arg8, UINT64 arg9, UINT64 arg10);

/* Front-ends to efi_callX to avoid compiler warnings */

#define _cast64_efi_call0(f) \
  efi_call0(f)
#define _cast64_efi_call1(f,a1) \
  efi_call1(f, (UINT64)(a1))
#define _cast64_efi_call2(f,a1,a2) \
  efi_call2(f, (UINT64)(a1), (UINT64)(a2))
#define _cast64_efi_call3(f,a1,a2,a3) \
  efi_call3(f, (UINT64)(a1), (UINT64)(a2), (UINT64)(a3))
#define _cast64_efi_call4(f,a1,a2,a3,a4) \
  efi_call4(f, (UINT64)(a1), (UINT64)(a2), (UINT64)(a3), (UINT64)(a4))
#define _cast64_efi_call5(f,a1,a2,a3,a4,a5) \
  efi_call5(f, (UINT64)(a1), (UINT64)(a2), (UINT64)(a3), (UINT64)(a4), \
            (UINT64)(a5))
#define _cast64_efi_call6(f,a1,a2,a3,a4,a5,a6) \
  efi_call6(f, (UINT64)(a1), (UINT64)(a2), (UINT64)(a3), (UINT64)(a4), \
            (UINT64)(a5), (UINT64)(a6))
#define _cast64_efi_call7(f,a1,a2,a3,a4,a5,a6,a7) \
  efi_call7(f, (UINT64)(a1), (UINT64)(a2), (UINT64)(a3), (UINT64)(a4), \
            (UINT64)(a5), (UINT64)(a6), (UINT64)(a7))
#define _cast64_efi_call8(f,a1,a2,a3,a4,a5,a6,a7,a8) \
  efi_call8(f, (UINT64)(a1), (UINT64)(a2), (UINT64)(a3), (UINT64)(a4), \
            (UINT64)(a5), (UINT64)(a6), (UINT64)(a7), (UINT64)(a8))
#define _cast64_efi_call9(f,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
  efi_call9(f, (UINT64)(a1), (UINT64)(a2), (UINT64)(a3), (UINT64)(a4), \
            (UINT64)(a5), (UINT64)(a6), (UINT64)(a7), (UINT64)(a8), \
            (UINT64)(a9))
#define _cast64_efi_call10(f,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
  efi_call10(f, (UINT64)(a1), (UINT64)(a2), (UINT64)(a3), (UINT64)(a4), \
             (UINT64)(a5), (UINT64)(a6), (UINT64)(a7), (UINT64)(a8), \
             (UINT64)(a9), (UINT64)(a10))

/* main wrapper (va_num ignored) */

#define uefi_call_wrapper(func,va_num,...)                        \
  __VA_ARG_NSUFFIX__(_cast64_efi_call, __VA_ARGS__) (func , ##__VA_ARGS__)

#else

#define uefi_call_wrapper(func, va_num, ...) func(__VA_ARGS__)

#endif

/* AED EFI definitions */

#if defined(_AED_EFI)

/* _int64 works for both IA32 and IA64 */

#define COMPILER_DEPENDENT_INT64   __int64
#define COMPILER_DEPENDENT_UINT64  unsigned __int64

/*
 * Calling conventions:
 *
 * ACPI_SYSTEM_XFACE        - Interfaces to host OS (handlers, threads)
 * ACPI_EXTERNAL_XFACE      - External ACPI interfaces
 * ACPI_INTERNAL_XFACE      - Internal ACPI interfaces
 * ACPI_INTERNAL_VAR_XFACE  - Internal variable-parameter list interfaces
 */
#define ACPI_SYSTEM_XFACE
#define ACPI_EXTERNAL_XFACE
#define ACPI_INTERNAL_XFACE
#define ACPI_INTERNAL_VAR_XFACE

/* warn C4142: redefinition of type */

#pragma warning(disable:4142)

#endif


/* GNU EFI definitions */

#if defined(_GNU_EFI)

/* Using GCC for GNU EFI */

#include "acgcc.h"

#undef ACPI_USE_SYSTEM_CLIBRARY
#undef ACPI_USE_STANDARD_HEADERS
#undef ACPI_USE_NATIVE_DIVIDE
#define ACPI_USE_SYSTEM_INTTYPES

/*
 * Math helpers
 */
#define ACPI_DIV_64_BY_32(n_hi, n_lo, d32, q32, r32) \
    do {                                             \
        UINT64 __n = ((UINT64) n_hi) << 32 | (n_lo); \
        (q32) = DivU64x32 ((__n), (d32), &(r32));    \
    } while (0)

#define ACPI_SHIFT_RIGHT_64(n_hi, n_lo) \
    do {                                \
        (n_lo) >>= 1;                   \
        (n_lo) |= (((n_hi) & 1) << 31); \
        (n_hi) >>= 1;                   \
    } while (0)


#endif

struct _SIMPLE_TEXT_OUTPUT_INTERFACE;
struct _SIMPLE_INPUT_INTERFACE;
struct _EFI_FILE_IO_INTERFACE;
struct _EFI_FILE_HANDLE;
struct _EFI_BOOT_SERVICES;
struct _EFI_SYSTEM_TABLE;

extern struct _EFI_SYSTEM_TABLE         *ST;
extern struct _EFI_BOOT_SERVICES        *BS;

#define ACPI_FILE           struct _SIMPLE_TEXT_OUTPUT_INTERFACE *
#define ACPI_FILE_OUT       ST->ConOut
#define ACPI_FILE_ERR       ST->ConOut

#endif /* __ACEFI_H__ */
