/*
 * ntddk.h
 *
 * Windows NT Device Driver Kit
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Amine Khaldi
 *   Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#define _NTDDK_

#if !defined(_NTHAL_) && !defined(_NTIFS_)
#define _NTDDK_INCLUDED_
#define _DDK_DRIVER_
#endif

/* Dependencies */

#define NT_INCLUDED
#define _CTYPE_DISABLE_MACROS

#include <wdm.h>
#include <excpt.h>
#include <ntdef.h>
#include <ntstatus.h>
#include <mce.h>
#include <bugcodes.h>
#include <ntiologc.h>

#include <stdarg.h> // FIXME
#include <basetyps.h> // FIXME


#ifdef __cplusplus
extern "C" {
#endif

/* GUID and UUID */
#ifndef _NTLSA_IFS_
#ifndef _NTLSA_AUDIT_
#define _NTLSA_AUDIT_

#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

#endif /* _NTLSA_AUDIT_ */
#endif /* _NTLSA_IFS_ */

typedef GUID UUID;

/* Forward declarations */
struct _LOADER_PARAMETER_BLOCK;
struct _CREATE_DISK;
struct _DRIVE_LAYOUT_INFORMATION_EX;
struct _SET_PARTITION_INFORMATION_EX;
struct _DISK_GEOMETRY_EX;

/* Structures not exposed to drivers */
typedef struct _BUS_HANDLER *PBUS_HANDLER;
typedef struct _DEVICE_HANDLER_OBJECT *PDEVICE_HANDLER_OBJECT;
#if defined(_NTHAL_INCLUDED_)
typedef struct _KAFFINITY_EX *PKAFFINITY_EX;
#endif
typedef struct _PEB *PPEB;

#ifndef _NTIMAGE_

typedef struct _IMAGE_NT_HEADERS *PIMAGE_NT_HEADERS32;
typedef struct _IMAGE_NT_HEADERS64 *PIMAGE_NT_HEADERS64;

#ifdef _WIN64
typedef PIMAGE_NT_HEADERS64 PIMAGE_NT_HEADERS;
#else
typedef PIMAGE_NT_HEADERS32 PIMAGE_NT_HEADERS;
#endif

#endif /* _NTIMAGE_ */

$define (_NTDDK_)
$include (extypes.h)
$include (cmtypes.h)
$include (iotypes.h)
$include (haltypes.h)
$include (ketypes.h)
$include (kdtypes.h)
$include (mmtypes.h)
$include (pstypes.h)
$include (rtltypes.h)
$include (setypes.h)

#if defined(_M_IX86)
$include(x86/ke.h)
$include(x86/mm.h)
#elif defined(_M_AMD64)
$include(amd64/ke.h)
$include(amd64/mm.h)
#elif defined(_M_IA64)
$include(ia64/ke.h)
#elif defined(_M_PPC)
$include(ppc/ke.h)
#elif defined(_M_MIPS)
$include(mips/ke.h)
#elif defined(_M_ARM)
$include(arm/ke.h)
$include(arm/mm.h)
#elif defined(_M_ARM64)
$include(arm64/ke.h)
#else
#error Unknown Architecture
#endif

$include (exfuncs.h)
$include (halfuncs.h)
$include (iofuncs.h)
$include (kdfuncs.h)
$include (kefuncs.h)
$include (mmfuncs.h)
$include (psfuncs.h)
$include (rtlfuncs.h)
$include (sefuncs.h)
$include (zwfuncs.h)


/* UNSORTED */

#define VER_SET_CONDITION(ConditionMask, TypeBitMask, ComparisonType) \
  ((ConditionMask) = VerSetConditionMask((ConditionMask),             \
  (TypeBitMask), (ComparisonType)))

#if (NTDDI_VERSION >= NTDDI_WIN2K)
NTSYSAPI
ULONGLONG
NTAPI
VerSetConditionMask(
  IN ULONGLONG ConditionMask,
  IN ULONG TypeMask,
  IN UCHAR Condition);
#endif

typedef struct _KERNEL_USER_TIMES {
  LARGE_INTEGER CreateTime;
  LARGE_INTEGER ExitTime;
  LARGE_INTEGER KernelTime;
  LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

/* NtXxx Functions */

typedef enum _SYSTEM_FIRMWARE_TABLE_ACTION {
  SystemFirmwareTable_Enumerate,
  SystemFirmwareTable_Get
} SYSTEM_FIRMWARE_TABLE_ACTION;

typedef struct _SYSTEM_FIRMWARE_TABLE_INFORMATION {
  ULONG ProviderSignature;
  SYSTEM_FIRMWARE_TABLE_ACTION Action;
  ULONG TableID;
  ULONG TableBufferLength;
  UCHAR TableBuffer[ANYSIZE_ARRAY];
} SYSTEM_FIRMWARE_TABLE_INFORMATION, *PSYSTEM_FIRMWARE_TABLE_INFORMATION;

typedef NTSTATUS
(__cdecl *PFNFTH)(
  _Inout_ PSYSTEM_FIRMWARE_TABLE_INFORMATION SystemFirmwareTableInfo);

typedef struct _SYSTEM_FIRMWARE_TABLE_HANDLER {
  ULONG ProviderSignature;
  BOOLEAN Register;
  PFNFTH FirmwareTableHandler;
  PVOID DriverObject;
} SYSTEM_FIRMWARE_TABLE_HANDLER, *PSYSTEM_FIRMWARE_TABLE_HANDLER;

typedef ULONG_PTR
(NTAPI *PDRIVER_VERIFIER_THUNK_ROUTINE)(
  _In_ PVOID Context);

typedef struct _DRIVER_VERIFIER_THUNK_PAIRS {
  PDRIVER_VERIFIER_THUNK_ROUTINE PristineRoutine;
  PDRIVER_VERIFIER_THUNK_ROUTINE NewRoutine;
} DRIVER_VERIFIER_THUNK_PAIRS, *PDRIVER_VERIFIER_THUNK_PAIRS;

#define DRIVER_VERIFIER_SPECIAL_POOLING             0x0001
#define DRIVER_VERIFIER_FORCE_IRQL_CHECKING         0x0002
#define DRIVER_VERIFIER_INJECT_ALLOCATION_FAILURES  0x0004
#define DRIVER_VERIFIER_TRACK_POOL_ALLOCATIONS      0x0008
#define DRIVER_VERIFIER_IO_CHECKING                 0x0010

#define SHARED_GLOBAL_FLAGS_ERROR_PORT_V        0x0
#define SHARED_GLOBAL_FLAGS_ERROR_PORT          (1UL << SHARED_GLOBAL_FLAGS_ERROR_PORT_V)

#define SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED_V 0x1
#define SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED   (1UL << SHARED_GLOBAL_FLAGS_ELEVATION_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_VIRT_ENABLED_V      0x2
#define SHARED_GLOBAL_FLAGS_VIRT_ENABLED        (1UL << SHARED_GLOBAL_FLAGS_VIRT_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED_V  0x3
#define SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED    \
  (1UL << SHARED_GLOBAL_FLAGS_INSTALLER_DETECT_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_SPARE_V                     0x4
#define SHARED_GLOBAL_FLAGS_SPARE                       \
  (1UL << SHARED_GLOBAL_FLAGS_SPARE_V)

#define SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED_V      0x5
#define SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED        \
  (1UL << SHARED_GLOBAL_FLAGS_DYNAMIC_PROC_ENABLED_V)

#define SHARED_GLOBAL_FLAGS_SEH_VALIDATION_ENABLED_V    0x6
#define SHARED_GLOBAL_FLAGS_SEH_VALIDATION_ENABLED        \
  (1UL << SHARED_GLOBAL_FLAGS_SEH_VALIDATION_ENABLED_V)

#define EX_INIT_BITS(Flags, Bit) \
  *((Flags)) |= (Bit)             // Safe to use before concurrently accessible

#define EX_TEST_SET_BIT(Flags, Bit) \
  InterlockedBitTestAndSet ((PLONG)(Flags), (Bit))

#define EX_TEST_CLEAR_BIT(Flags, Bit) \
  InterlockedBitTestAndReset ((PLONG)(Flags), (Bit))

#define PCCARD_MAP_ERROR               0x01
#define PCCARD_DEVICE_PCI              0x10

#define PCCARD_SCAN_DISABLED           0x01
#define PCCARD_MAP_ZERO                0x02
#define PCCARD_NO_TIMER                0x03
#define PCCARD_NO_PIC                  0x04
#define PCCARD_NO_LEGACY_BASE          0x05
#define PCCARD_DUP_LEGACY_BASE         0x06
#define PCCARD_NO_CONTROLLERS          0x07

#define MAXIMUM_EXPANSION_SIZE (KERNEL_LARGE_STACK_SIZE - (PAGE_SIZE / 2))

/* Filesystem runtime library routines */

#if (NTDDI_VERSION >= NTDDI_WIN2K)
_Must_inspect_result_
NTKERNELAPI
BOOLEAN
NTAPI
FsRtlIsTotalDeviceFailure(
  _In_ NTSTATUS Status);
#endif

#ifdef __cplusplus
}
#endif
