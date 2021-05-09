/*
 * aux_klib.h
 *
 * Auxiliary Kernel-Mode Library
 *
 * Contributors:
 *   Victor Perevertkin <victor.perevertkin@reactos.org>
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

#ifndef PIMAGE_EXPORT_DIRECTORY
#include <ntimage.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AUX_KLIB_MODULE_PATH_LEN 0x100

typedef struct _AUX_MODULE_BASIC_INFO {
    PVOID ImageBase;
} AUX_MODULE_BASIC_INFO, *PAUX_MODULE_BASIC_INFO;

typedef struct _AUX_MODULE_EXTENDED_INFO {
    AUX_MODULE_BASIC_INFO BasicInfo;
    ULONG ImageSize;
    USHORT FileNameOffset;
    CHAR FullPathName[AUX_KLIB_MODULE_PATH_LEN];
} AUX_MODULE_EXTENDED_INFO, *PAUX_MODULE_EXTENDED_INFO;

typedef struct _KBUGCHECK_DATA {
    ULONG BugCheckDataSize;
    ULONG BugCheckCode;
    ULONG_PTR Parameter1;
    ULONG_PTR Parameter2;
    ULONG_PTR Parameter3;
    ULONG_PTR Parameter4;
} KBUGCHECK_DATA, *PKBUGCHECK_DATA;

NTSTATUS
NTAPI
AuxKlibInitialize(VOID);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
AuxKlibQueryModuleInformation(
    _Inout_ PULONG InformationLength,
    _In_ ULONG SizePerModule,
    _Out_writes_bytes_opt_(*InformationLength) PAUX_MODULE_EXTENDED_INFO ModuleInfo);

NTSTATUS
AuxKlibGetBugCheckData(
    _Inout_ PKBUGCHECK_DATA BugCheckData);

PIMAGE_EXPORT_DIRECTORY
AuxKlibGetImageExportDirectory(
    _In_ PVOID ImageBase);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
AuxKlibEnumerateSystemFirmwareTables (
    _In_ ULONG FirmwareTableProviderSignature,
    _Out_writes_bytes_to_opt_(BufferLength, *ReturnLength) PVOID FirmwareTableBuffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
AuxKlibGetSystemFirmwareTable (
    _In_ ULONG FirmwareTableProviderSignature,
    _In_ ULONG FirmwareTableID,
    _Out_writes_bytes_to_opt_(BufferLength, *ReturnLength) PVOID FirmwareTableBuffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength);

#ifdef __cplusplus
}
#endif
