/*
 * PROJECT:     ReactOS Networking Debugging Module
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Primary header for kdnet
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#define NOEXTAPI
#include <ntifs.h>
#include <windbgkd.h>
#include <arc/arc.h>
#include <kddll.h>
#include <kdnetextensibility.h>

/* slight hack, but will allow us to debug this kdnet across x86/x64/arm64 easier */
extern ULONG (*FrLdrDbgPrint)(const char *Format, ...);

//TODO: This needs to get moved.
VOID
NTAPI
PoSetHiberRange(IN PVOID HiberContext,
                IN ULONG Flags,
                IN OUT PVOID StartPage,
                IN ULONG Length,
                IN ULONG PageTag);

extern BOOLEAN KdNetInitialized;
extern KDNET_SHARED_DATA KdNetSharedData;
extern PVOID KdNetHardwareContext;

extern PKDNET_EXTENSIBILITY_EXPORTS KdNetExtensibilityExports;

typedef NTSTATUS
(NTAPI *PKDNET_INITIALIZE_LIBRARY)(
    _In_ PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ struct _DEBUG_DEVICE_DESCRIPTOR *Device);

NTSTATUS
KdNetInitializeExtensibility(
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ struct _DEBUG_DEVICE_DESCRIPTOR *Device,
    _In_opt_ PKDNET_INITIALIZE_LIBRARY KdInitializeLibrary,
    _Out_ PKDNET_EXTENSIBILITY_EXPORTS ExtensibilityExports,
    _Out_opt_ void *SerialExtensibility);
