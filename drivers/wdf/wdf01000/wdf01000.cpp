
#include <ntddk.h>
#include <wdm.h>
#include <ntstrsafe.h>


#include "fxlibrarycommon.h"
#include "common/fxldr.h"
#include "common/fxglobals.h"
#include "wdf.h"

#define KMDF_DEVICE_NAME L"\\Device\\KMDF"


//----- Declarations -----//
#ifndef __WDF_MAJOR_VERSION
#define __WDF_MAJOR_VERSION 1
#endif

#ifndef __WDF_MINOR_VERSION
#define __WDF_MINOR_VERSION 9
#endif

#ifndef __WDF_BUILD_NUMBER
#define __WDF_BUILD_NUMBER 7600
#endif

#ifndef __WDF_MAJOR_VERSION_STRING
#define __WDF_MAJOR_VERSION_STRING (01)
#endif


extern "C" {

//-----------------------------------------------------------------------------
// local prototype definitions
//-----------------------------------------------------------------------------
extern "C"
DRIVER_UNLOAD DriverUnload;

extern "C"
DRIVER_INITIALIZE DriverEntry;

RTL_OSVERSIONINFOW  gOsVersion = { sizeof(RTL_OSVERSIONINFOW) };

ULONG    WdfLdrDbgPrintOn = 0;

PCCH WdfLdrType = "Wdf01000";

FxLibraryGlobalsType FxLibraryGlobals = { 0 };

}

extern "C"
NTSTATUS
WDF_LIBRARY_COMMISSION(
	VOID
);

extern "C"
NTSTATUS
WDF_LIBRARY_DECOMMISSION(
	VOID
);

extern "C"
NTSTATUS
WDF_LIBRARY_REGISTER_CLIENT(
	__in  PWDF_BIND_INFO        Info,
	__deref_out   PWDF_DRIVER_GLOBALS * WdfDriverGlobals,
	__deref_inout PVOID * Context
);

extern "C"
NTSTATUS
WDF_LIBRARY_UNREGISTER_CLIENT(
	__in PWDF_BIND_INFO        Info,
	__in PWDF_DRIVER_GLOBALS   WdfDriverGlobals
);

extern "C" {
	WDF_LIBRARY_INFO  WdfLibraryInfo = {
		sizeof(WDF_LIBRARY_INFO),
		(PFNLIBRARYCOMMISSION) WDF_LIBRARY_COMMISSION,
		(PFNLIBRARYDECOMMISSION) WDF_LIBRARY_DECOMMISSION,
		(PFNLIBRARYREGISTERCLIENT) WDF_LIBRARY_REGISTER_CLIENT,
		(PFNLIBRARYUNREGISTERCLIENT) WDF_LIBRARY_UNREGISTER_CLIENT,
		{ __WDF_MAJOR_VERSION, __WDF_MINOR_VERSION, __WDF_BUILD_NUMBER }
	};
}

extern "C"
NTSTATUS
NTAPI
WdfLdrDiagnosticsValueByNameAsULONG(
	PUNICODE_STRING ValueName,
	PULONG Value
);

extern "C"
NTSTATUS
NTAPI
WdfRegisterLibrary(
	PWDF_LIBRARY_INFO LibraryInfo,
	PUNICODE_STRING ServicePath,
	PUNICODE_STRING LibraryDeviceName
);

//----------//

extern "C"
NTSTATUS
NTAPI
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


VOID
FxLibraryCleanup(
	VOID
)
{
	if (FxLibraryGlobals.LibraryDeviceObject != NULL)
	{
		IoDeleteDevice(FxLibraryGlobals.LibraryDeviceObject);
		FxLibraryGlobals.LibraryDeviceObject = NULL;
	}
}


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
	//WdfDeleteKmdfVersionFromRegistry(DriverObject);

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

NTSTATUS
WdfBindClientHelper(
	PWDF_BIND_INFO       BindInfo,
	ULONG       FxMajorVersion,
	ULONG       FxMinorVersion
)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;

	if (BindInfo->Version.Minor <= FxMinorVersion)
	{
		status = STATUS_SUCCESS;
	}

	return status;
}

#define EVTLOG_MESSAGE_SIZE 70
#define RAW_DATA_SIZE 4

extern "C"
NTSTATUS
WDF_LIBRARY_REGISTER_CLIENT(
	__in  PWDF_BIND_INFO        Info,
	__deref_out   PWDF_DRIVER_GLOBALS * WdfDriverGlobals,
	__deref_inout PVOID * Context
)
{
	NTSTATUS           status = STATUS_INVALID_PARAMETER;
	PFX_DRIVER_GLOBALS pFxDriverGlobals;
	WCHAR              insertString[EVTLOG_MESSAGE_SIZE];
	//ULONG              rawData[RAW_DATA_SIZE];
	PCLIENT_INFO       clientInfo = NULL;

	__Print((LITERAL(WDF_LIBRARY_REGISTER_CLIENT) ": enter\n"));

	clientInfo = (PCLIENT_INFO)*Context;
	*Context = NULL;

	status = WdfBindClientHelper(Info,
		WdfLibraryInfo.Version.Major,
		WdfLibraryInfo.Version.Minor);

	if (!NT_SUCCESS(status))
	{

		NTSTATUS status2;
		status2 = RtlStringCchPrintfW(insertString,
			RTL_NUMBER_OF(insertString),
			L"Driver Version: %d.%d Kmdf Lib. Version: %d.%d",
			Info->Version.Major,
			Info->Version.Minor,
			WdfLibraryInfo.Version.Major,
			WdfLibraryInfo.Version.Minor);

		if (!NT_SUCCESS(status2)) 
		{
			__Print(("ERROR: RtlStringCchPrintfW failed with Status 0x%x\n", status2));
			return status;
		}
		//rawData[0] = Info->Version.Major;
		//rawData[1] = Info->Version.Minor;
		//rawData[2] = WdfLibraryInfo.Version.Major;
		//rawData[3] = WdfLibraryInfo.Version.Minor;

		/*LibraryLogEvent(FxLibraryGlobals.DriverObject,
			WDFVER_MINOR_VERSION_NOT_SUPPORTED,
			status,
			insertString,
			rawData,
			sizeof(rawData));*/

		return status;
	}
	
	status = FxLibraryCommonRegisterClient(Info,
		WdfDriverGlobals,
		clientInfo);

	if (NT_SUCCESS(status)) 
	{
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

extern "C"
NTSTATUS
WDF_LIBRARY_UNREGISTER_CLIENT(
	__in PWDF_BIND_INFO        Info,
	__in PWDF_DRIVER_GLOBALS   WdfDriverGlobals
)
{
	return FxLibraryCommonUnregisterClient(Info, WdfDriverGlobals);
}


//_Must_inspect_result_
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
	do
	{
		status = RtlUnicodeStringPrintf(DeviceName, L"%s%d", KMDF_DEVICE_NAME, i++);
		
		if (!NT_SUCCESS(status))
		{
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

	if (NT_SUCCESS(status))
	{
		//
		// Clear the initializing bit now because the loader will attempt to
		// open the device before we return from DriverEntry
		//
		ASSERT(FxLibraryGlobals.LibraryDeviceObject != NULL);
		FxLibraryGlobals.LibraryDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
	}

	return status;
}

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT   DriverObject,
	PUNICODE_STRING  RegistryPath
)
{
	UNICODE_STRING name;
	UNICODE_STRING string;
	NTSTATUS status;

	//PAGED_CODE();

	__Print(("wdf01000: enter: \n"));

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
	//ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	RtlInitUnicodeString(&string, WDF_REGISTRY_DBGPRINT_ON);

	//
	// Determine if debug prints are on.
	//
	WdfLdrDiagnosticsValueByNameAsULONG(&string, &WdfLdrDbgPrintOn);

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
	if (!NT_SUCCESS(status))
	{
		__Print(("ERROR: FxLibraryCreateDevice failed with Status 0x%x\n", status));
		return status;
	}
	else
	{
		__Print(("SUCCESS: FxLibraryCreateDevice Status 0x%x\n", status));
	}

	//
	// Register this library with WdfLdr
	//
	// NOTE:  Once WdfRegisterLibrary returns NT_SUCCESS() we must return
	//        NT_SUCCESS from DriverEntry!
	//
	status = WdfRegisterLibrary(&WdfLibraryInfo, RegistryPath, &name);
	if (!NT_SUCCESS(status))
	{
		__Print(("ERROR: WdfRegisterLibrary failed with Status 0x%x\n", status));
		FxLibraryCleanup();
		return status;
	}
	else
	{
		__Print(("SUCCESS: WdfRegisterLibrary Status 0x%x\n", status));
	}

	//
	// Write KMDF version to registry
	//
	//WdfWriteKmdfVersionToRegistry(DriverObject, RegistryPath);

	return STATUS_SUCCESS;
}