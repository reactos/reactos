/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - library functions
 * COPYRIGHT:   Copyright 2019 mrmks04 (mrmks04@yandex.ru)
 */


#include "library.h"
#include "ntddk_ex.h"

#include <ntintsafe.h>
#include <ntstrsafe.h>


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, AuxKlibInitialize)
#endif


NTSTATUS
NTAPI
LibraryOpen(
	IN PLIBRARY_MODULE LibModule,
	IN PUNICODE_STRING ObjectName
)
{
	NTSTATUS status;
	PDEVICE_OBJECT devObj;

	if (ObjectName == NULL)
	{
		return STATUS_SUCCESS;
	}

	status = IoGetDeviceObjectPointer(ObjectName, FILE_READ_DATA, &LibModule->LibraryFileObject, &devObj);

	if (NT_SUCCESS(status))
	{
		LibModule->LibraryDriverObject = devObj->DriverObject;
	}

	return status;
}


VOID
NTAPI
LibraryClose(
	IN PLIBRARY_MODULE LibModule
)
{
	if (LibModule->LibraryFileObject != NULL)
	{
		ObfDereferenceObject(LibModule->LibraryFileObject);		
		LibModule->LibraryFileObject = NULL;
	}
}


PLIBRARY_MODULE
NTAPI
LibraryCreate(
	IN PWDF_LIBRARY_INFO LibInfo,
	IN PUNICODE_STRING DriverServiceName
)
{
	NTSTATUS status;
	PLIBRARY_MODULE pLibModule;

	pLibModule = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIBRARY_MODULE), WDFLDR_TAG);
	
	if (pLibModule == NULL)
	{
		return NULL;
	}

	RtlZeroMemory(pLibModule, sizeof(LIBRARY_MODULE));
	pLibModule->LibraryRefCount = 1;
	InitializeListHead(&pLibModule->LibraryListEntry);

	if (LibInfo != NULL)
	{
		pLibModule->ImplicitlyLoaded = TRUE;
		LibraryCopyInfo(pLibModule, LibInfo);
	}

	InitializeListHead(&pLibModule->ClientsListHead);
	ExInitializeResourceLite(&pLibModule->ClientsListLock);
	InitializeListHead(&pLibModule->ClassListHead);
	pLibModule->Service.Buffer = ExAllocatePoolWithTag(PagedPool, DriverServiceName->MaximumLength, WDFLDR_TAG);
		
	if (pLibModule->Service.Buffer != NULL)
	{
		pLibModule->Service.MaximumLength = DriverServiceName->MaximumLength;
		RtlCopyUnicodeString(&pLibModule->Service, DriverServiceName);
	}
	else
	{
		goto clean;
	}
	
	status = GetImageName(DriverServiceName, WDFLDR_TAG, &pLibModule->ImageName);
	if (!NT_SUCCESS(status))
	{
		goto clean;
	}

	pLibModule->IsBootDriver = ServiceCheckBootStart(&pLibModule->Service);
	status = GetImageBase(&pLibModule->ImageName, &pLibModule->ImageAddress, &pLibModule->ImageSize);

	if (!NT_SUCCESS(status))
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: LibraryCreate - ");
			DbgPrint("WdfLdr: LibraryCreate: GetImageBase failed\n");
		}
	}
	else
	{
		return pLibModule;
	}

clean:
	LibraryCleanupAndFree(pLibModule);
	return NULL;
}


PWDF_LIBRARY_INFO
NTAPI
LibraryCopyInfo(
	IN PLIBRARY_MODULE LibModule,
	IN OUT PWDF_LIBRARY_INFO LibInfo
)
{
	LibModule->LibraryInfo = LibInfo;
	LibModule->Version.Major = LibInfo->Version.Major;
	LibModule->Version.Minor = LibInfo->Version.Minor;
	LibModule->Version.Build = LibInfo->Version.Build;

	return LibInfo;
}


VOID
NTAPI
LibraryCleanupAndFree(
	IN PLIBRARY_MODULE LibModule
)
{
	LibraryClose(LibModule);

	if (LibModule->Service.Buffer != NULL)
	{
		ExFreePoolWithTag(LibModule->Service.Buffer, WDFLDR_TAG);
		LibModule->Service.Length = 0;
		LibModule->Service.Buffer = NULL;
	}

	if (LibModule->ImageName.Buffer)
	{
		ExFreePoolWithTag(LibModule->ImageName.Buffer, WDFLDR_TAG);
		LibModule->ImageName.Length = 0;
		LibModule->ImageName.Buffer = NULL;
	}

	ExDeleteResourceLite(&LibModule->ClientsListLock);
	ExFreePoolWithTag(LibModule, WDFLDR_TAG);
}


NTSTATUS
NTAPI
AuxKlibInitialize()
{
	NTSTATUS status;
	RTL_OSVERSIONINFOW osVersion;
	UNICODE_STRING strRtlQueryModuleInformation;

	RtlInitUnicodeString(&strRtlQueryModuleInformation, L"RtlQueryModuleInformation");
	PAGED_CODE();

	status = STATUS_SUCCESS;
	if (!gKlibInitialized)
	{
		RtlGetVersion(&osVersion);
		if (osVersion.dwMajorVersion >= 5)
		{
			pfnRtlQueryModuleInformation = (PRtlQueryModuleInformation)MmGetSystemRoutineAddress(&strRtlQueryModuleInformation);
			InterlockedExchange(&gKlibInitialized, 1);
		}
		else
		{
			status = STATUS_NOT_SUPPORTED;
		}
	}

	return status;
}



PLIST_ENTRY
NTAPI
LibraryAddToLibraryListLocked(
	IN PLIBRARY_MODULE LibModule
)
{
	PLIST_ENTRY result;

	result = &LibModule->LibraryListEntry;
	
	InsertHeadList(&gLibList, result);
		
	return result;
}


VOID
NTAPI
LibraryRemoveFromLibraryList(
	IN PLIBRARY_MODULE LibModule
)
{
	PLIST_ENTRY libListEntry;
	BOOLEAN removed;

	FxLdrAcquireLoadedModuleLock();
	libListEntry = &LibModule->LibraryListEntry;

	if (IsListEmpty(libListEntry))
	{
		removed = FALSE;
	}
	else
	{
		RemoveEntryList(libListEntry);
		InitializeListHead(libListEntry);
		
		removed = TRUE;
	}

	FxLdrReleaseLoadedModuleLock();
	if (removed)
	{
		if (!_InterlockedExchangeAdd(&LibModule->LibraryRefCount, -1))
			LibraryCleanupAndFree(LibModule);
	}
}


VOID
NTAPI
ClientCleanupAndFree(
	IN PCLIENT_MODULE ClientModule
)
{
	if (ClientModule->ImageName.Buffer != NULL)
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: ClientCleanupAndFree - ");
			DbgPrint("Client Image Name: %wZ\n", &ClientModule->ImageName);
		}

		ExFreePoolWithTag(ClientModule->ImageName.Buffer, WDFLDR_TAG);
		ClientModule->ImageName.Length = 0;
		ClientModule->ImageName.Buffer = NULL;
	}

	ExFreePoolWithTag(ClientModule, WDFLDR_TAG);
}

NTSTATUS
NTAPI
LibraryLinkInClient(
	IN PLIBRARY_MODULE LibModule,
	IN PUNICODE_STRING DriverServiceName,
	IN PWDF_BIND_INFO BindInfo,
	IN PVOID Context,
	OUT PCLIENT_MODULE* ClientModule
)
{
	PCLIENT_MODULE pClientModule;
	NTSTATUS status;

	*ClientModule = NULL;
	pClientModule = ExAllocatePoolWithTag(NonPagedPool, sizeof(CLIENT_MODULE), WDFLDR_TAG);

	if (pClientModule == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		if (!WdfLdrDiags)
		{
			return status;
		}

		DbgPrint("WdfLdr: LibraryLinkInClient - ");
		DbgPrint("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", STATUS_INSUFFICIENT_RESOURCES);
		goto error;
	}

	RtlZeroMemory(pClientModule, sizeof(CLIENT_MODULE));
	InitializeListHead(&pClientModule->ClassListHead);
	InitializeListHead(&pClientModule->LibListEntry);
	pClientModule->Context = Context;
	pClientModule->Info = BindInfo;
	status = GetImageName(DriverServiceName, WDFLDR_TAG, &pClientModule->ImageName);

	if (NT_SUCCESS(status))
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: LibraryLinkInClient - ");
			DbgPrint("Client Image Name: %wZ\n", &pClientModule->ImageName);
		}

		status = GetImageBase(&pClientModule->ImageName, &pClientModule->ImageAddr, &pClientModule->ImageSize);

		if (!NT_SUCCESS(status))
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: LibraryLinkInClient - ");
				DbgPrint("WdfLdr: LibraryLinkInClient: GetImageBase failed with status 0x%x\n", status);
			}
			ClientCleanupAndFree(pClientModule);
			goto error;
		}
	}
	else
	{
		pClientModule->ImageName.Length = 0;
		pClientModule->ImageName.Buffer = NULL;
	}

	LibraryAcquireClientLock(LibModule);
	InsertHeadList(&LibModule->ClientsListHead, &pClientModule->LibListEntry);
	LibraryReleaseClientLock(LibModule);
	*ClientModule = pClientModule;
	return STATUS_SUCCESS;	

error:
	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: LibraryLinkInClient - ");
		DbgPrint("ERROR: Client module NOT linked\n");
	}

	return status;
}


VOID
NTAPI
LibraryUnload(
	PLIBRARY_MODULE LibModule,
	BOOLEAN RemoveFromList
)
{
	PWDF_LIBRARY_INFO LibInfo;
	NTSTATUS status;

	if (LibModule->IsBootDriver == TRUE)
		return;

	LibInfo = LibModule->LibraryInfo;
	
	if (LibInfo)
	{
		status = LibInfo->LibraryDecommission();

		if (!NT_SUCCESS(status) && WdfLdrDiags)
		{
			DbgPrint("WdfLdr: LibraryUnload - ");
			DbgPrint("WdfLdr: LibraryUnload: LibraryDecommission failed %08X\n", status);
		}
	}
	
	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: LibraryUnload - ");
		DbgPrint("WdfLdr: LibraryUnload: Unload module %wZ\n", &LibModule->Service);
	}	

	LibraryClose(LibModule);
	status = ZwUnloadDriver(&LibModule->Service);

	if (!NT_SUCCESS(status) && WdfLdrDiags)
	{
		DbgPrint("WdfLdr: LibraryUnload - ");
		DbgPrint(
			"WdfLdr: LibraryUnload: unload of %wZ returned 0x%x (this may not be a true error if someone else attempted to stop"
			" the service first)\n",
			&LibModule->Service,
			status);
	}

	if (RemoveFromList)
		LibraryRemoveFromLibraryList(LibModule);
}


VOID
NTAPI
LibraryReleaseClientReference(
	PLIBRARY_MODULE LibModule
)
{
	int refs;

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: LibraryReleaseClientReference - ");
		DbgPrint("WdfLdr: LibraryReleaseClientReference: Dereference module %wZ\n", &LibModule->Service);
	}

	refs = _InterlockedDecrement(&LibModule->ClientRefCount);
	
	if (refs <= 0)
	{
		LibraryUnload(LibModule, TRUE);
	}
	else if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: LibraryReleaseClientReference - ");
		DbgPrint(
			"WdfLdr: LibraryReleaseClientReference: Dereference module %wZ still has %d references\n",
			&LibModule->Service,
			refs);
	}
}


NTSTATUS
NTAPI
LibraryUnlinkClient(
	PLIBRARY_MODULE LibModule,
	PWDF_BIND_INFO BindInfo
)
{
	BOOLEAN isBindFound;
	PCLIENT_MODULE pClientModule;
	PLIST_ENTRY entry;
	NTSTATUS status;

	isBindFound = FALSE;
	pClientModule = NULL;
	LibraryAcquireClientLock(LibModule);

	for (entry = LibModule->ClientsListHead.Flink; entry != &LibModule->ClientsListHead; entry = entry->Flink)
	{
		pClientModule = CONTAINING_RECORD(entry, CLIENT_MODULE, LibListEntry);
		if (pClientModule->Info == BindInfo)
		{
			isBindFound = TRUE;
			break;
		}
	}
	LibraryReleaseClientLock(LibModule);

	if (isBindFound)
	{
		RemoveEntryList(entry);
		InitializeListHead(entry);
		status = STATUS_SUCCESS;
		ClientCleanupAndFree(pClientModule);
	}
	else
	{
		status = STATUS_UNSUCCESSFUL;
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: LibraryUnlinkClient - ");
			DbgPrint("ERROR: Client module %p, bind %p NOT found\n", LibModule, BindInfo);
		}
	}

	return status;
}
