/*++

Copyright (c) Microsoft Corporation

Module Name:

    Version.cpp

Abstract:

    This module forms a loadable library from the WDF core libs

Revision History:

--*/

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <ntverp.h>

extern "C" {
#include <ntddk.h>
#include <ntstrsafe.h>
}

#define  FX_DYNAMICS_GENERATE_TABLE   1

#include "fx.hpp"

#include <fxldr.h>
#include "fxbugcheck.h"
#include "wdfversionlog.h"

#define DRIVER_OBJECT_EXTENSION_IDENTIFIER      DriverEntry
#define DRIVER_PARAMETERS L"Parameters"
#define REGISTRY_KMDF_MAJOR_VERSION L"MajorVersion"
#define REGISTRY_KMDF_MINOR_VERSION L"MinorVersion"
#define REGISTRY_KMDF_BUILD_NUMBER L"BuildNumber"

//-----------------------------------------------------------------------------
// These header files are referenced in order to make internal structures
// available in public symbols. Various WDFKD debug commands use these
// internal structures to provide information about WDF.
//-----------------------------------------------------------------------------
#include "FxIFR.h"

extern "C" {

//
// This is the collection of all structure/types to be make public.
// This union forces the structure type-info into the PDB file.
//
union {

    WDF_IFR_HEADER                    * typeWDF_IFR_HEADER;
    WDF_IFR_RECORD                    * typeWDF_IFR_RECORD;
    WDF_IFR_OFFSET                    * typeWDF_IFR_OFFSET;
    WDF_BIND_INFO                     * typeWDF_BIND_INFO;
    WDF_OBJECT_CONTEXT_TYPE_INFO      * typeWDF_OBJECT_CONTEXT_TYPE_INFO;
    WDF_POWER_ROUTINE_TIMED_OUT_DATA  * typeWDF_POWER_ROUTINE_TIMED_OUT_DATA;
    WDF_BUGCHECK_CODES                * typeWDF_BUGCHECK_CODES;
    WDF_REQUEST_FATAL_ERROR_CODES     * typeWDF_REQUEST_FATAL_ERROR_CODES;
    FX_OBJECT_INFO                    * typeFX_OBJECT_INFO;
    FX_POOL_HEADER                    * typeFX_POOL_HEADER;
    FX_POOL                           * typeFX_POOL;
    FxObject                          * typeFxObject;
    FxContextHeader                   * typeFxContextHeader;
    FX_DUMP_DRIVER_INFO_ENTRY         * typeFX_DUMP_DRIVER_INFO_ENTRY;
    FxTargetSubmitSyncParams          * typeFxTargetSubmitSyncParams;

} uAllPublicTypes;

} // extern "C" end

//-----------------------------------------    ------------------------------------

extern "C" {

#include "FxDynamics.h"

#include "FxLibraryCommon.h"

#define  KMDF_DEFAULT_NAME   "Wdf" ## \
                             LITERAL(__WDF_MAJOR_VERSION_STRING)   ## \
                             "000" //minor version

//-----------------------------------------------------------------------------
// local prototype definitions
//-----------------------------------------------------------------------------
extern "C"
DRIVER_UNLOAD DriverUnload;

extern "C"
DRIVER_INITIALIZE DriverEntry;

extern "C"
__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLEANUP)
__drv_dispatchType(IRP_MJ_CLOSE)
DRIVER_DISPATCH FxLibraryDispatch;

RTL_OSVERSIONINFOW  gOsVersion = { sizeof(RTL_OSVERSIONINFOW) };

ULONG    WdfLdrDbgPrintOn = 0;

PCHAR WdfLdrType = KMDF_DEFAULT_NAME;

}  // extern "C"


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C"
_Must_inspect_result_
NTSTATUS
WDF_LIBRARY_COMMISSION(
    VOID
    );

extern "C"
_Must_inspect_result_
NTSTATUS
WDF_LIBRARY_DECOMMISSION(
    VOID
    );

extern "C"
_Must_inspect_result_
NTSTATUS
WDF_LIBRARY_REGISTER_CLIENT(
    __inout  PWDF_BIND_INFO             Info,
    __deref_out   PWDF_DRIVER_GLOBALS * WdfDriverGlobals,
    __deref_inout PVOID               * Context
    );

extern "C"
_Must_inspect_result_
NTSTATUS
WDF_LIBRARY_UNREGISTER_CLIENT(
    __in PWDF_BIND_INFO        Info,
    __in PWDF_DRIVER_GLOBALS   WdfDriverGlobals
    );

extern "C"
VOID
FxLibraryDeleteDevice(
    VOID
    );

VOID
FxLibraryCleanup(
    VOID
    );

VOID
WdfWriteKmdfVersionToRegistry(
    __in PDRIVER_OBJECT   DriverObject,
    __in PUNICODE_STRING  RegistryPath
    );

VOID
WdfDeleteKmdfVersionFromRegistry(
    __in PDRIVER_OBJECT   DriverObject
    );

typedef struct _DRV_EXTENSION {
    UNICODE_STRING ParametersRegistryPath;
} DRV_EXTENSION, *PDRV_EXTENSION;

//-----------------------------------------------------------------------------
// Library registeration information
//-----------------------------------------------------------------------------
extern "C" {
#pragma prefast(suppress:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "kernel component.");
WDF_LIBRARY_INFO  WdfLibraryInfo = {
    sizeof(WDF_LIBRARY_INFO),
    (PFNLIBRARYCOMMISSION)        WDF_LIBRARY_COMMISSION,
    (PFNLIBRARYDECOMMISSION)      WDF_LIBRARY_DECOMMISSION,
    (PFNLIBRARYREGISTERCLIENT)    WDF_LIBRARY_REGISTER_CLIENT,
    (PFNLIBRARYUNREGISTERCLIENT)  WDF_LIBRARY_UNREGISTER_CLIENT,
    { __WDF_MAJOR_VERSION, __WDF_MINOR_VERSION, __WDF_BUILD_NUMBER }
};

} // extern "C" end

extern "C"
NTSTATUS
FxLibraryDispatch (
    __in struct _DEVICE_OBJECT * DeviceObject,
    __in PIRP Irp
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceObject);
    ASSERT(FxLibraryGlobals.LibraryDeviceObject == DeviceObject);

    status = STATUS_INVALID_DEVICE_REQUEST;

    switch (IoGetCurrentIrpStackLocation(Irp)->MajorFunction) {
    case IRP_MJ_CREATE:
        //
        // To limit our exposure for this device object, only allow kernel mode
        // creates.
        //
        if (Irp->RequestorMode == KernelMode) {
            status = STATUS_SUCCESS;
        }
        break;

    case IRP_MJ_CLEANUP:
    case IRP_MJ_CLOSE:
        //
        // Since we allowed a create to succeed, succeed the cleanup and close
        //
        status = STATUS_SUCCESS;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0x0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

#define KMDF_DEVICE_NAME L"\\Device\\KMDF"

_Must_inspect_result_
NTSTATUS
FxLibraryCreateDevice(
    __in PUNICODE_STRING DeviceName
    )
{
    NTSTATUS status;
    ULONG i;

    i = 0;

    //
    // Repeatedly try to create a named device object until we run out of buffer
    // space or we succeed.
    //
    do {
        status = RtlUnicodeStringPrintf(DeviceName, L"%s%d", KMDF_DEVICE_NAME, i++);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        //
        // Create a device with no device extension
        //
        status = IoCreateDevice(
            FxLibraryGlobals.DriverObject,
            0,
            DeviceName,
            FILE_DEVICE_UNKNOWN,
            0,
            FALSE,
            &FxLibraryGlobals.LibraryDeviceObject
            );
    } while (STATUS_OBJECT_NAME_COLLISION == status);

    if (NT_SUCCESS(status)) {
        //
        // Clear the initializing bit now because the loader will attempt to
        // open the device before we return from DriverEntry
        //
        ASSERT(FxLibraryGlobals.LibraryDeviceObject != NULL);
        FxLibraryGlobals.LibraryDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    return status;
}

extern "C"
VOID
FxLibraryDeleteDevice(
    VOID
    )
{
    FxLibraryCleanup();
}

VOID
FxLibraryCleanup(
    VOID
    )
{
    if (FxLibraryGlobals.LibraryDeviceObject != NULL) {
        IoDeleteDevice(FxLibraryGlobals.LibraryDeviceObject);
        FxLibraryGlobals.LibraryDeviceObject = NULL;
    }
}

extern "C"
NTSTATUS
DriverEntry(
    __in PDRIVER_OBJECT   DriverObject,
    __in PUNICODE_STRING  RegistryPath
    )
{
    UNICODE_STRING name;
    UNICODE_STRING string;
    NTSTATUS status;

    //
    // This creates a local buffer which is big enough to hold a copy of the
    // constant string assigned to it.  It does not point to the constant
    // string.  As such, it is a writeable buffer.
    //
    // NOTE:  KMDF_DEVICE_NAME L"XXXX" creates a concatenated string of
    //        KMDF_DEVICE_NAME + L"XXXX".  This is done to give us room for
    //        appending a number up to 4 digits long after KMDF_DEVICE_NAME if
    //        you want a null terminated string, 5 digits long if the string is
    //        not null terminated (as is the case for a UNICODE_STRING)
    //
    WCHAR buffer[] = KMDF_DEVICE_NAME L"XXXX";

    //
    // Initialize global to make NonPagedPool be treated as NxPool on Win8
    // and NonPagedPool on down-level
    //
    ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

    RtlInitUnicodeString(&string, WDF_REGISTRY_DBGPRINT_ON);

    //
    // Determine if debug prints are on.
    //
    (void) WdfLdrDiagnosticsValueByNameAsULONG(&string, &WdfLdrDbgPrintOn);

    __Print(("DriverEntry\n"));

    DriverObject->DriverUnload = DriverUnload;

    FxLibraryGlobals.DriverObject = DriverObject;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = FxLibraryDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = FxLibraryDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = FxLibraryDispatch;

    RtlZeroMemory(&name, sizeof(name));
    name.Buffer = buffer;
    name.Length = 0x0;
    name.MaximumLength = sizeof(buffer);

    //
    // We use the string when we declare the buffer to get the right sized
    // buffer.  Now we want to make sure there are no contents before we
    // use it to create a device object.
    //
    RtlZeroMemory(buffer, sizeof(buffer));

    status = FxLibraryCreateDevice(&name);
    if (!NT_SUCCESS(status)) {
        __Print(("ERROR: FxLibraryCreateDevice failed with Status 0x%x\n", status));
        return status;
    }

    //
    // Register this library with WdfLdr
    //
    // NOTE:  Once WdfRegisterLibrary returns NT_SUCCESS() we must return
    //        NT_SUCCESS from DriverEntry!
    //
    status = WdfRegisterLibrary( &WdfLibraryInfo, RegistryPath, &name );
    if (!NT_SUCCESS(status)) {
        __Print(("ERROR: WdfRegisterLibrary failed with Status 0x%x\n", status));
        FxLibraryCleanup();
        return status;
    }

    //
    // Write KMDF version to registry
    //
    WdfWriteKmdfVersionToRegistry(DriverObject, RegistryPath);

    return STATUS_SUCCESS;
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C"
VOID
DriverUnload(
    __in PDRIVER_OBJECT   DriverObject
    )
{
    __Print(("DriverUnload\n"));

    //
    // Delete KMDF version from registry before destroying the Driver Object
    //
    WdfDeleteKmdfVersionFromRegistry(DriverObject);

    //
    // Make sure everything is deleted.  Since the driver is considered a legacy
    // driver, it can be unloaded while there are still outstanding device objects.
    //
    FxLibraryCleanup();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C"
_Must_inspect_result_
NTSTATUS
WDF_LIBRARY_COMMISSION(
    VOID
    )
{
    return FxLibraryCommonCommission();
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C"
_Must_inspect_result_
NTSTATUS
WDF_LIBRARY_DECOMMISSION(
    VOID
    )
{
    return FxLibraryCommonDecommission();
}

#define EVTLOG_MESSAGE_SIZE 70
#define RAW_DATA_SIZE 4

extern "C"
_Must_inspect_result_
NTSTATUS
WDF_LIBRARY_REGISTER_CLIENT(
    __in  PWDF_BIND_INFO        Info,
    __deref_out   PWDF_DRIVER_GLOBALS * WdfDriverGlobals,
    __deref_inout PVOID             * Context
    )
{
    NTSTATUS           status = STATUS_INVALID_PARAMETER;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WCHAR              insertString[EVTLOG_MESSAGE_SIZE];
    ULONG              rawData[RAW_DATA_SIZE];
    PCLIENT_INFO       clientInfo = NULL;

    __Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT) ": enter\n"));

    clientInfo = (PCLIENT_INFO)*Context;
    *Context = NULL;

    ASSERT(Info->Version.Major == WdfLibraryInfo.Version.Major);

    //
    // NOTE: If the currently loaded  library < drivers minor version fail the load
    // instead of binding to a lower minor version. The reason for that if there
    // is a newer API or new contract change made the driver shouldn't be using older
    // API than it was compiled with.
    //

    if (Info->Version.Minor > WdfLibraryInfo.Version.Minor) {
        status = RtlStringCchPrintfW(insertString,
                                     RTL_NUMBER_OF(insertString),
                                     L"Driver Version: %d.%d Kmdf Lib. Version: %d.%d",
                                     Info->Version.Major,
                                     Info->Version.Minor,
                                     WdfLibraryInfo.Version.Major,
                                     WdfLibraryInfo.Version.Minor);
        if (!NT_SUCCESS(status)) {
            __Print(("ERROR: RtlStringCchPrintfW failed with Status 0x%x\n", status));
            return status;
        }
        rawData[0] = Info->Version.Major;
        rawData[1] = Info->Version.Minor;
        rawData[2] = WdfLibraryInfo.Version.Major;
        rawData[3] = WdfLibraryInfo.Version.Minor;

        LibraryLogEvent(FxLibraryGlobals.DriverObject,
                       WDFVER_MINOR_VERSION_NOT_SUPPORTED,
                       STATUS_OBJECT_TYPE_MISMATCH,
                       insertString,
                       rawData,
                       sizeof(rawData) );
        //
        // this looks like the best status to return
        //
        return STATUS_OBJECT_TYPE_MISMATCH;

    }

    status = FxLibraryCommonRegisterClient(Info,
                                           WdfDriverGlobals,
                                           clientInfo);

    if (NT_SUCCESS(status)) {
        //
        // The context will be a pointer to FX_DRIVER_GLOBALS
        //
        *Context = GetFxDriverGlobals(*WdfDriverGlobals);

        //
        // Set the WDF_BIND_INFO structure pointer in FxDriverGlobals
        //
        pFxDriverGlobals = GetFxDriverGlobals(*WdfDriverGlobals);
        pFxDriverGlobals->WdfBindInfo = Info;
    }

    return status;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C"
_Must_inspect_result_
NTSTATUS
WDF_LIBRARY_UNREGISTER_CLIENT(
    __in PWDF_BIND_INFO        Info,
    __in PWDF_DRIVER_GLOBALS   WdfDriverGlobals
    )
{
    return FxLibraryCommonUnregisterClient(Info, WdfDriverGlobals);
}

VOID
WdfWriteKmdfVersionToRegistry(
    __in PDRIVER_OBJECT   DriverObject,
    __in PUNICODE_STRING  RegistryPath
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE driverKey;
    HANDLE parametersKey;
    UNICODE_STRING valueName;
    UNICODE_STRING parametersPath;
    PDRV_EXTENSION driverExtension;

    driverKey = NULL;
    parametersKey = NULL;
    driverExtension = NULL;

    RtlInitUnicodeString(&parametersPath, DRIVER_PARAMETERS);

    InitializeObjectAttributes(&objectAttributes,
                               RegistryPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&driverKey, KEY_CREATE_SUB_KEY, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        __Print(("WdfWriteKmdfVersionToRegistry: Failed to open HKLM\\%S\n",
                 RegistryPath->Buffer));
        goto out;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &parametersPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               driverKey,
                               NULL);

    //
    // Open or create key and get a handle
    //
    status = ZwCreateKey(&parametersKey,
                         KEY_SET_VALUE,
                         &objectAttributes,
                         0,
                         (PUNICODE_STRING) NULL,
                         REG_OPTION_VOLATILE,
                         NULL);

    if (!NT_SUCCESS(status)) {
        __Print(("WdfWriteKmdfVersionToRegistry: Failed to open HKLM\\%S\\%S\n",
                 RegistryPath->Buffer, parametersPath.Buffer));
        goto out;
    }

    //
    // Set Major Version
    //
    RtlInitUnicodeString(&valueName, REGISTRY_KMDF_MAJOR_VERSION);

    status = ZwSetValueKey(parametersKey,
                           &valueName,
                           0,
                           REG_DWORD,
                           &WdfLibraryInfo.Version.Major,
                           sizeof(WdfLibraryInfo.Version.Major));

    if (!NT_SUCCESS(status)) {
        __Print(("WdfWriteKmdfVersionToRegistry: Failed to set Major Version\n"));
        goto out;
    }

    //
    // Set Minor Version
    //
    RtlInitUnicodeString(&valueName, REGISTRY_KMDF_MINOR_VERSION);

    status = ZwSetValueKey(parametersKey,
                           &valueName,
                           0,
                           REG_DWORD,
                           &WdfLibraryInfo.Version.Minor,
                           sizeof(WdfLibraryInfo.Version.Minor));

    if (!NT_SUCCESS(status)) {
        __Print(("WdfWriteKmdfVersionToRegistry: Failed to set Minor Version\n"));
        goto out;
    }


    //
    // Set Build Number
    //
    RtlInitUnicodeString(&valueName, REGISTRY_KMDF_BUILD_NUMBER);

    status = ZwSetValueKey(parametersKey,
                           &valueName,
                           0,
                           REG_DWORD,
                           &WdfLibraryInfo.Version.Build,
                           sizeof(WdfLibraryInfo.Version.Build));

    if (!NT_SUCCESS(status)) {
        __Print(("WdfWriteKmdfVersionToRegistry: Failed to set Build Number\n"));
        goto out;
    }

    //
    // Create a Driver Extension to store the registry path, where we write the
    // version of the wdf01000.sys that's loaded in memory
    //
    status = IoAllocateDriverObjectExtension(DriverObject,
                                             (PVOID) DRIVER_OBJECT_EXTENSION_IDENTIFIER,
                                             sizeof(DRV_EXTENSION),
                                             (PVOID *)&driverExtension);

    if (!NT_SUCCESS(status) || driverExtension == NULL) {
        goto out;
    }

    driverExtension->ParametersRegistryPath.Buffer = (PWCHAR) ExAllocatePoolWithTag(
                                                                PagedPool,
                                                                RegistryPath->MaximumLength,
                                                                FX_TAG);
    if (driverExtension->ParametersRegistryPath.Buffer == NULL) {
        goto out;
    }

    driverExtension->ParametersRegistryPath.MaximumLength = RegistryPath->MaximumLength;
    RtlCopyUnicodeString(&(driverExtension->ParametersRegistryPath), RegistryPath);

out:
    if (driverKey != NULL) {
        ZwClose(driverKey);
    }

    if (parametersKey != NULL) {
        ZwClose(parametersKey);
    }

    return;
}

VOID
WdfDeleteKmdfVersionFromRegistry(
    __in PDRIVER_OBJECT   DriverObject
    )
{
    PUNICODE_STRING registryPath;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE driverKey;
    HANDLE parametersKey;
    UNICODE_STRING valueName;
    NTSTATUS status;
    UNICODE_STRING parametersPath;
    PDRV_EXTENSION driverExtension;

    RtlInitUnicodeString(&parametersPath, DRIVER_PARAMETERS);

    driverKey = NULL;
    parametersKey = NULL;

    driverExtension = (PDRV_EXTENSION)IoGetDriverObjectExtension(DriverObject,
                                                                 DRIVER_OBJECT_EXTENSION_IDENTIFIER);

    if (driverExtension == NULL || driverExtension->ParametersRegistryPath.Buffer == NULL) {
        return;
    }

    registryPath = &driverExtension->ParametersRegistryPath;

    InitializeObjectAttributes(&objectAttributes,
                               registryPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&driverKey, KEY_SET_VALUE, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        goto out;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &parametersPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               driverKey,
                               NULL);
    //
    // Open the key for deletion
    //
    status = ZwOpenKey(&parametersKey,
                       DELETE,
                       &objectAttributes);

    if (!NT_SUCCESS(status)) {
        goto out;
    }

    RtlInitUnicodeString(&valueName, REGISTRY_KMDF_MAJOR_VERSION);
    ZwDeleteValueKey(parametersKey, &valueName);

    RtlInitUnicodeString(&valueName, REGISTRY_KMDF_MINOR_VERSION);
    ZwDeleteValueKey(parametersKey, &valueName);

    RtlInitUnicodeString(&valueName, REGISTRY_KMDF_BUILD_NUMBER);
    ZwDeleteValueKey(parametersKey, &valueName);

    ZwDeleteKey(parametersKey);

out:
    if (driverExtension->ParametersRegistryPath.Buffer != NULL) {
        ExFreePool(driverExtension->ParametersRegistryPath.Buffer);
    }

    if (driverKey != NULL) {
        ZwClose(driverKey);
    }

    if (parametersKey != NULL) {
        ZwClose(parametersKey);
    }

    return;
}

