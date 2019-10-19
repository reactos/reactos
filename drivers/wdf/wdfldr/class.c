/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - class functions
 * COPYRIGHT:   Copyright 2019 mrmks04 (mrmks04@yandex.ru)
 */


#include "class.h"

#include <ntstrsafe.h>

BOOLEAN
NTAPI
LibraryAcquireClientLock(
	IN PLIBRARY_MODULE LibModule
)
{
	KeEnterCriticalRegion();
	return ExAcquireResourceExclusiveLite(&LibModule->ClientsListLock, TRUE);
}


VOID
NTAPI
LibraryReleaseClientLock(
	IN PLIBRARY_MODULE LibModule
)
{
	ExReleaseResourceLite(&LibModule->ClientsListLock);
	KeLeaveCriticalRegion();
}


VOID
NTAPI
ClassAcquireClientLock(
	IN PERESOURCE Resource
)
{
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(Resource, TRUE);
}


VOID
NTAPI
ClassReleaseClientLock(
	IN PERESOURCE Resource
)
{
	ExReleaseResourceLite(Resource);
	KeLeaveCriticalRegion();
}


NTSTATUS
NTAPI
ClassOpen(
	IN PCLASS_MODULE ClassModule,
	IN PUNICODE_STRING ObjectName
)
{
	NTSTATUS result;
	PDEVICE_OBJECT deviceObject;

	if (!ObjectName)
		return STATUS_SUCCESS;

	result = IoGetDeviceObjectPointer(ObjectName, 1u, &ClassModule->ClassFileObject, &deviceObject);
	
	if (NT_SUCCESS(result))
		ClassModule->ClassDriverObject = deviceObject->DriverObject;

	return result;
}


VOID
NTAPI
ClassClose(
	IN PCLASS_MODULE ClassModule
)
{
	if (ClassModule->ClassFileObject != NULL)
	{
		ObfDereferenceObject(ClassModule->ClassFileObject);
		ClassModule->ClassFileObject = NULL;
	}
}


VOID
NTAPI
ClassCleanupAndFree(
	IN PCLASS_MODULE ClassModule
)
{
	ClassClose(ClassModule);

	if (ClassModule->Service.Buffer != NULL)
	{
		ExFreePoolWithTag(ClassModule->Service.Buffer, 0);
		ClassModule->Service.Length = 0;
		ClassModule->Service.Buffer = 0;
	}

	if (ClassModule->ImageName.Buffer != NULL)
	{
		ExFreePoolWithTag(ClassModule->ImageName.Buffer, 0);
		ClassModule->ImageName.Length = 0;
		ClassModule->ImageName.Buffer = 0;
	}

	ExDeleteResourceLite(&ClassModule->ClientsListLock);
	ExFreePoolWithTag(ClassModule, 0);
}


PCLASS_MODULE
NTAPI
ClassCreate(
	IN PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
	IN PLIBRARY_MODULE LibModule,
	IN PUNICODE_STRING ServiceName
)
{
	PCLASS_MODULE result;
	NTSTATUS status;

	result = ExAllocatePoolWithTag(NonPagedPool, sizeof(CLASS_MODULE), WDFLDR_TAG);

	if (result != NULL)
	{
		goto exit;
	}

	memset(result, 0, sizeof(CLASS_MODULE));
	if (ClassLibInfo != NULL)
	{
		result->ImplicitlyLoaded = TRUE;
		result->ClassLibraryInfo = ClassLibInfo;
		result->Version.Major = ClassLibInfo->Version.Major;
		result->Version.Minor = ClassLibInfo->Version.Minor;
		result->Version.Build = ClassLibInfo->Version.Build;
	}

	result->ClassRefCount = 1;
	result->Library = LibModule;
	ExInitializeResourceLite(&result->ClientsListLock);
	InitializeListHead(&result->ClientsListHead);
	InitializeListHead(&result->LibraryLinkage);
	result->Service.Buffer = ExAllocatePoolWithTag(PagedPool, ServiceName->MaximumLength, WDFLDR_TAG);

	if (result->Service.Buffer == NULL)
	{
		goto clean;
	}

	result->Service.MaximumLength = ServiceName->MaximumLength;
	memset(result->Service.Buffer, 0, result->Service.MaximumLength);
	RtlCopyUnicodeString(&result->Service, ServiceName);
	status = GetImageName(ServiceName, 0x694C7846u, &result->ImageName);

	if (!NT_SUCCESS(status))
	{
		goto clean;
	}

	result->IsBootDriver = ServiceCheckBootStart(&result->Service);
	status = GetImageBase((PCUNICODE_STRING)& result->ImageName, &result->ImageAddress, &result->ImageSize);

	if (NT_SUCCESS(status))
	{
		return result;
	}
	else if (WdfLdrDiags)
	{
			DbgPrint("WdfLdr: ClassCreate - ");
			DbgPrint("WdfLdr: ClassCreate: GetImageBase failed\n");
	}

clean:
	ClassCleanupAndFree(result);
exit:
	return NULL;
}


PCLASS_CLIENT_MODULE
NTAPI
ClassClientCreate()
{
	PCLASS_CLIENT_MODULE result;

	result = ExAllocatePoolWithTag(NonPagedPool, sizeof(CLASS_CLIENT_MODULE), WDFLDR_TAG);

	if (result != NULL)
	{
		memset(result, 0, sizeof(CLASS_CLIENT_MODULE));
		InitializeListHead(&result->ClassLinkage);
		InitializeListHead(&result->ClientLinkage);
	}

	return result;
}


PCLIENT_MODULE
NTAPI
LibraryFindClientLocked(
	IN PLIBRARY_MODULE LibModule,
	IN PWDF_BIND_INFO BindInfo
)
{
	PCLIENT_MODULE result;

	result = (PCLIENT_MODULE)LibModule->ClientsListHead.Flink;

	for (; result != (PCLIENT_MODULE)&LibModule->ClientsListHead;
		result = (PCLIENT_MODULE)result->LibListEntry.Flink)
	{

		if (result->Info == BindInfo)
			return result;
	}

	return NULL;
}


NTSTATUS
NTAPI
ClassLinkInClient(
	IN PCLASS_MODULE ClassModule,
	IN PWDF_CLASS_BIND_INFO ClassBindInfo,
	IN PWDF_BIND_INFO BindInfo,
	OUT PCLASS_CLIENT_MODULE ClassClientModule
)
{
	PCLIENT_MODULE pClienInfo;
	NTSTATUS status;

	ClassClientModule->Class = ClassModule;
	ClassClientModule->ClientClassBindInfo = ClassBindInfo;
	LibraryAcquireClientLock(ClassModule->Library);
	pClienInfo = LibraryFindClientLocked(ClassModule->Library, BindInfo);

	if (pClienInfo != NULL)
	{
		ClassClientModule->Client = pClienInfo;
		InsertTailList(&pClienInfo->ClassListHead, &ClassClientModule->ClassLinkage);
	}

	LibraryReleaseClientLock(ClassModule->Library);

	if (pClienInfo != NULL)
	{
		ClassAcquireClientLock(&ClassModule->ClientsListLock);
		InsertTailList(&ClassModule->ClientsListHead, &ClassClientModule->ClientLinkage);
		ClassReleaseClientLock(&ClassModule->ClientsListLock);
		status = STATUS_SUCCESS;
	}
	else
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: ClassLinkInClient - ");
			DbgPrint("ERROR: Could not locate client from Info %p, status 0x%x\n", BindInfo, STATUS_INVALID_DEVICE_STATE);
		}
		status = STATUS_INVALID_DEVICE_STATE;
	}

	return status;
}


NTSTATUS
NTAPI
GetClassRegistryHandle(
	IN PWDF_CLASS_BIND_INFO ClassBindInfo,
	IN PWDF_BIND_INFO BindInfo,
	OUT PHANDLE KeyHandle
)
{
	PWCHAR pClassName;
	PWCHAR pClassNameBegin;
	WCHAR currentSym;
	PWDF_BIND_INFO pBindInfo;
	PWDF_CLASS_BIND_INFO pClassBindInfo;
	UNICODE_STRING classVersions;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE rootHandle;
	HANDLE classVersionsHandle;
	HANDLE classVersionHandle;
	NTSTATUS status;
	SIZE_T numOfBytes;
	UNICODE_STRING objectName;
	DECLARE_UNICODE_STRING_SIZE(versionString, 22);
	
	pClassBindInfo = ClassBindInfo;
	pBindInfo = BindInfo;
	classVersions.Length = 0;
	classVersions.Buffer = NULL;
	RtlInitUnicodeString(&objectName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf\\Kmdf");
	ObjectAttributes.ObjectName = &objectName;
	rootHandle = NULL;
	classVersionsHandle = NULL;
	classVersionHandle = 0;

	*KeyHandle = NULL;
	ObjectAttributes.Length = 24;
	ObjectAttributes.RootDirectory = 0;
	ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;//576;
	ObjectAttributes.SecurityDescriptor = 0;
	ObjectAttributes.SecurityQualityOfService = 0;
	status = ZwOpenKey(&rootHandle, KEY_QUERY_VALUE, &ObjectAttributes);

	if (NT_SUCCESS(status))
	{
		pClassName = pClassBindInfo->ClassName;
		pClassNameBegin = pClassName;
		do
		{
			currentSym = *pClassName;
			++pClassName;
		} while ( currentSym != '\0');

		numOfBytes = 2 * (pClassName - pClassNameBegin - 1) + 20;
		classVersions.Buffer = ExAllocatePoolWithTag(PagedPool, numOfBytes, WDFLDR_TAG);

		if (classVersions.Buffer != NULL)
		{
			classVersions.MaximumLength = (USHORT)numOfBytes;
			status = RtlUnicodeStringPrintf(&classVersions, L"%s\\Versions", pClassBindInfo->ClassName);
			if (NT_SUCCESS(status))
			{
				ObjectAttributes.RootDirectory = rootHandle;
				ObjectAttributes.ObjectName = &classVersions;
				ObjectAttributes.Length = 24;
				ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;//576;
				ObjectAttributes.SecurityDescriptor = NULL;
				ObjectAttributes.SecurityQualityOfService = NULL;
				status = ZwOpenKey(&classVersionsHandle, KEY_QUERY_VALUE, &ObjectAttributes);

				if (NT_SUCCESS(status))
				{
					status = ConvertUlongToWString(pClassBindInfo->Version.Major, &versionString);
					if (NT_SUCCESS(status))
					{
						ObjectAttributes.RootDirectory = classVersionsHandle;
						ObjectAttributes.ObjectName = &versionString;
						ObjectAttributes.Length = 24;
						ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;//576;
						ObjectAttributes.SecurityDescriptor = NULL;
						ObjectAttributes.SecurityQualityOfService = NULL;
						status = ZwOpenKey(&classVersionHandle, KEY_QUERY_VALUE, &ObjectAttributes);

						if (NT_SUCCESS(status))
						{
							status = ConvertUlongToWString(pBindInfo->Version.Major, &versionString);
							if (NT_SUCCESS(status))
							{
								ObjectAttributes.RootDirectory = classVersionHandle;
								ObjectAttributes.ObjectName = &versionString;
								ObjectAttributes.Length = 24;
								ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;//576;
								ObjectAttributes.SecurityDescriptor = NULL;
								ObjectAttributes.SecurityQualityOfService = NULL;
								status = ZwOpenKey(KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
								//*keyHandle = tmpKeyHandle;
							}
						}
					}
				}
			}
		}
		else
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}

	if (rootHandle)
		ZwClose(rootHandle);
	if (classVersionsHandle)
		ZwClose(classVersionsHandle);
	if (KeyHandle)
		ZwClose(KeyHandle);

	if (classVersions.Buffer)
	{
		ExFreePoolWithTag(classVersions.Buffer, 0);
		classVersions.Length = 0;
		classVersions.Buffer = NULL;
	}

	return status;
}


NTSTATUS
NTAPI
GetDefaultClassServiceName(
	IN PWDF_CLASS_BIND_INFO ClassBindInfo,
	IN PWDF_BIND_INFO BindInfo,
	OUT PUNICODE_STRING ServiceName
)
{
	PWCHAR pClassName;
	PWCHAR pClassNameBegin;
	WCHAR currentSymbol;
	SIZE_T size;
	NTSTATUS status;

	pClassName = ClassBindInfo->ClassName;
	pClassNameBegin = pClassName;
	do
	{
		currentSymbol = *pClassName;
		++pClassName;
	} while (currentSymbol);

	size = 2 * (pClassName - pClassNameBegin - 1) + 148;
	ServiceName->Buffer = ExAllocatePoolWithTag(PagedPool, size, WDFLDR_TAG);

	if (ServiceName->Buffer == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ServiceName->MaximumLength = (USHORT)size;
	status = RtlUnicodeStringPrintf(
		ServiceName,
		L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%ws%02d%02d",
		ClassBindInfo->ClassName,
		ClassBindInfo->Version.Major,
		BindInfo->Version.Major);

	if (!NT_SUCCESS(status))
	{
		ExFreePoolWithTag(ServiceName->Buffer, 0);
		ServiceName->Length = 0;
		ServiceName->Buffer = NULL;
	}

	return status;
}


NTSTATUS
NTAPI
GetClassServicePath(
	IN PWDF_CLASS_BIND_INFO ClassBindInfo,
	IN PWDF_BIND_INFO BindInfo,
	OUT PUNICODE_STRING ServicePath
)
{
	NTSTATUS status;
	HANDLE Handle;
	PKEY_VALUE_PARTIAL_INFORMATION pKeyValPartial;
	UNICODE_STRING ValueName;

	ServicePath->Length = 0;
	ServicePath->Buffer = NULL;
	Handle = NULL;
	pKeyValPartial = NULL;
	RtlInitUnicodeString(&ValueName, L"Service");
	status = GetClassRegistryHandle(ClassBindInfo, BindInfo, &Handle);

	if (!NT_SUCCESS(status))
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: GetClassServicePath - ");
			DbgPrint("ERROR: GetClassRegistryHandle failed with status 0x%x\n", status);
		}
	}
	else
	{
		status = FxLdrQueryData(Handle, &ValueName, 0x674C7846u, &pKeyValPartial);
		
		if (!NT_SUCCESS(status))
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: GetClassServicePath - ");
				DbgPrint("ERROR: QueryData failed with status 0x%x\n", status);
			}
		}
		else
		{
			status = BuildServicePath(pKeyValPartial, 0x304C7846u, ServicePath);
		}
	}

	if (!NT_SUCCESS(status))
	{
		status = GetDefaultClassServiceName(ClassBindInfo, BindInfo, ServicePath);
		
		if (!NT_SUCCESS(status))
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: GetClassServicePath - ");
				DbgPrint("ERROR: GetDefaultClassServiceName failed, status 0x%x\n", status);
			}
		}
	}

	if (Handle)
		ZwClose(Handle);
	if (pKeyValPartial)
		ExFreePoolWithTag(pKeyValPartial, 0);

	return status;
}


NTSTATUS
NTAPI
ReferenceClassVersion(
	IN PWDF_CLASS_BIND_INFO ClassBindInfo,
	IN PWDF_BIND_INFO BindInfo,
	OUT PCLASS_MODULE* ClassModule
)
{
	PWDF_BIND_INFO pBindInfo;
	PCLASS_MODULE pClassModule;
	UNICODE_STRING driverServiceName;
	PLIBRARY_MODULE pLibModule;
	BOOLEAN created;
	NTSTATUS status;

	driverServiceName.Length = 0;
	driverServiceName.Buffer = NULL;
	*ClassModule = NULL;
	pBindInfo = BindInfo;
	created = FALSE;
	status = GetClassServicePath(ClassBindInfo, BindInfo, &driverServiceName);

	if (!NT_SUCCESS(status))
	{
		FreeString(&driverServiceName);
		return status;
	}
	FxLdrAcquireLoadedModuleLock();
	pClassModule = FindClassByServiceNameLocked(&driverServiceName, &pLibModule);

	if (!pLibModule)
	{
		pLibModule = pBindInfo->Module;
	}

	if (pLibModule == pBindInfo->Module)
	{
		if (pClassModule)
		{
			_InterlockedExchangeAdd(&pClassModule->ClassRefCount, 1);
		}
		else
		{
			pClassModule = ClassCreate(0, pLibModule, &driverServiceName);

			if (pClassModule)
			{
				_InterlockedExchangeAdd(&pClassModule->ClassRefCount, 1);
				LibraryAddToClassListLocked(pLibModule, pClassModule);
				created = TRUE;
			}
			else
			{
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}
	}
	else
	{
		status = STATUS_REVISION_MISMATCH;

		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: ReferenceClassVersion - ");
			DbgPrint(
				"WdfLdr: ReferenceClassVersion class %S bound to library %p,client bound to library %p, status 0x%x\n",
				ClassBindInfo->ClassName,
				pLibModule,
				pBindInfo->Module,
				status);
		}
	}
	FxLdrReleaseLoadedModuleLock();

	if (!created)
	{
		if (!NT_SUCCESS(status))
			goto clean;
		goto done;
	}

	status = ZwLoadDriver(&driverServiceName);
	
	if (!NT_SUCCESS(status))
	{
		if (status == STATUS_IMAGE_ALREADY_LOADED || status == STATUS_OBJECT_NAME_COLLISION)
		{
			if (pClassModule->ClassLibraryInfo)
			{
				status = STATUS_SUCCESS;				
			}
			else if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: ReferenceClassVersion - ");
				DbgPrint(
					"WdfLdr: ReferenceVersion: ZwLoadDriver (%wZ) failed and no Libray information was returned: 0x%x\n",
					&driverServiceName,
					status);
			}
		}
		else
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: ReferenceClassVersion - ");
				DbgPrint("WARNING: ZwLoadDriver (%wZ) failed with Status 0x%x\n", &driverServiceName, status);
			}
			pClassModule->ImageAlreadyLoaded = 1;
		}
		goto clean;
	}

done:
	ClassBindInfo->ClassModule = pClassModule;
	*ClassModule = pClassModule;
clean:
	if (pClassModule && _InterlockedExchangeAdd(&pClassModule->ClassRefCount, -1) <= 0)
	{
		ClassCleanupAndFree(pClassModule);
	}

	FreeString(&driverServiceName);

	return status;
}


PCLASS_MODULE
NTAPI
FindClassByServiceNameLocked(
	IN PUNICODE_STRING Path,
	OUT PLIBRARY_MODULE* LibModule
)
{
	PLIST_ENTRY libEntry;
	PLIBRARY_MODULE pLibModule;
	PLIST_ENTRY classEntry;
	PCLASS_MODULE pClassModule;
	WCHAR tmpName[15];
	WCHAR searchName[15];
	UNICODE_STRING tmp1;
	UNICODE_STRING tmp2;

	tmpName[0] = 0;
	GetNameFromUnicodePath(Path, searchName, sizeof(searchName));

	if (!IsListEmpty(&gLibList))
	{
		for (libEntry = gLibList.Flink; libEntry != &gLibList; libEntry = libEntry->Flink)
		{
			pLibModule = CONTAINING_RECORD(libEntry, LIBRARY_MODULE, LibraryListEntry);

			for (classEntry = pLibModule->ClassListHead.Flink;
				classEntry != &pLibModule->ClassListHead;
				classEntry = classEntry->Flink)
			{

				pClassModule = CONTAINING_RECORD(classEntry, CLASS_MODULE, LibraryLinkage);
				GetNameFromUnicodePath(&pClassModule->Service, tmpName, sizeof(tmpName));
								
				RtlInitUnicodeString(&tmp1, tmpName);				
				RtlInitUnicodeString(&tmp2, searchName);
				
				if (RtlCompareUnicodeString(&tmp1, &tmp2, FALSE) == 0)
				{
					if (LibModule != NULL)
					{
						*LibModule = pLibModule;
					}

					return pClassModule;
				}
			}
		}
	}

	if (LibModule != NULL)
		*LibModule = NULL;

	return NULL;
}


PLIST_ENTRY
NTAPI
LibraryAddToClassListLocked(
	IN PLIBRARY_MODULE LibModule,
	IN PCLASS_MODULE ClassModule
)
{
	PLIST_ENTRY result;

	result = &ClassModule->LibraryLinkage;
	InsertHeadList(&LibModule->ClassListHead, &ClassModule->LibraryLinkage);

	return result;
}


VOID
NTAPI
ClassRemoveFromLibraryList(
	IN PCLASS_MODULE ClassModule
)
{
	PLIST_ENTRY libLinkEntry;
	BOOLEAN removed;

	FxLdrAcquireLoadedModuleLock();
	libLinkEntry = &ClassModule->LibraryLinkage;

	if (IsListEmpty(libLinkEntry))
	{
		removed = FALSE;
	}
	else
	{
		RemoveEntryList(libLinkEntry);
		InitializeListHead(libLinkEntry);
		removed = TRUE;
	}
	FxLdrReleaseLoadedModuleLock();

	if (removed)
	{
		if (!_InterlockedExchangeAdd(&ClassModule->ClassRefCount, -1))
			ClassCleanupAndFree(ClassModule);
	}
}


VOID
NTAPI
ClassUnlinkClient(
	IN PCLASS_MODULE ClassModule,
	IN PWDF_CLASS_BIND_INFO ClassBindInfo
)
{
	PCLASS_CLIENT_MODULE client;
	BOOLEAN unlinked;
	PLIST_ENTRY entry;	

	client = NULL;
	unlinked = FALSE;
	ClassAcquireClientLock(&ClassModule->ClientsListLock);

	for (entry = ClassModule->ClientsListHead.Flink;
		entry != &ClassModule->ClientsListHead;
		entry = entry->Flink)
	{
		client = CONTAINING_RECORD(entry, CLASS_CLIENT_MODULE, ClassLinkage);

		if(CONTAINING_RECORD(entry, CLASS_CLIENT_MODULE, ClassLinkage)->ClientClassBindInfo == ClassBindInfo)
		{
			RemoveEntryList(entry);
			InitializeListHead(entry);
			unlinked = TRUE;
			break;
		}
	}

	ClassReleaseClientLock(&ClassModule->ClientsListLock);

	if (unlinked)
	{
		LibraryAcquireClientLock(ClassModule->Library);
		InitializeListHead(&client->ClientLinkage);
		RemoveEntryList(&client->ClientLinkage);
		LibraryReleaseClientLock(ClassModule->Library);
		ExFreePoolWithTag(client, 0);
	}
}


VOID
NTAPI
ClassUnload(
	IN PCLASS_MODULE ClassModule,
	IN BOOLEAN RemoveFromList
)
{
	PCLASS_MODULE pClassModule;
	PWDF_CLASS_LIBRARY_INFO pClassLibInfo;
	NTSTATUS status;

	pClassModule = ClassModule;
	pClassLibInfo = ClassModule->ClassLibraryInfo;

	if (pClassLibInfo && pClassLibInfo->ClassLibraryDeinitialize)
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: ClassUnload - ");
			DbgPrint(
				"WdfLdr: ClassUnload: calling ClassLibraryDeinitialize (%p)\n",
				ClassModule->ClassLibraryInfo->ClassLibraryDeinitialize);
		}
		ClassModule->ClassLibraryInfo->ClassLibraryDeinitialize();
	}

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: ClassUnload - ");
		DbgPrint("WdfLdr: ClassUnload: Unload class library %wZ\n", &ClassModule->Service);
	}

	ClassClose(ClassModule);
	status = ZwUnloadDriver((PUNICODE_STRING)& ClassModule->Service);

	if (!NT_SUCCESS(status) && WdfLdrDiags)
	{
		DbgPrint("WdfLdr: ClassUnload - ");
		DbgPrint(
			"WdfLdr: ClassUnload: unload of class %wZ returned 0x%x (this may not be a true error if someone else attempted to "
			"stop the service first)\n",
			&pClassModule->Service,
			status);
	}

	if (RemoveFromList)
		ClassRemoveFromLibraryList(pClassModule);
}


VOID
NTAPI
ClassReleaseClientReference(
	IN PCLASS_MODULE ClassModule
)
{
	int refs;

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: ClassReleaseClientReference - ");
		DbgPrint("WdfLdr: ClassReleaseClientReference: Dereference module %wZ\n", &ClassModule->Service);
	}

	refs = _InterlockedDecrement(&ClassModule->ClientRefCount);

	if (refs <= 0)
	{
		ClassUnload(ClassModule, TRUE);
	}
	else if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: ClassReleaseClientReference - ");
		DbgPrint(
			"WdfLdr: ClassReleaseClientReference: Dereference module %wZ still has %d references\n",
			&ClassModule->Service,
			refs);
	}
}


VOID
NTAPI
DereferenceClassVersion(
	PWDF_CLASS_BIND_INFO ClassBindInfo,
	PWDF_BIND_INFO BindInfo,
	PWDF_COMPONENT_GLOBALS Globals
)
{
	PCLASS_MODULE pClassModule;

	UNREFERENCED_PARAMETER(BindInfo);
	pClassModule = ClassBindInfo->ClassModule;

	if (pClassModule)
	{
		pClassModule->ClassLibraryInfo->ClassLibraryUnbindClient(ClassBindInfo, Globals);
		ClassUnlinkClient(pClassModule, ClassBindInfo);
		ClassReleaseClientReference(pClassModule);
		ClassBindInfo->ClassModule = NULL;
	}
}
