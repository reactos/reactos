/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver
 * COPYRIGHT:   Copyright 2019 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdfldr.h"
#include "ntddk_ex.h"
#include "library.h"
#include "class.h"

#include <ntintsafe.h>
#include <ntstrsafe.h>
#include <initguid.h>


DEFINE_GUID(GUID_WDF_LOADER_INTERFACE_STANDARD, 0x49215dff, 0xf5ac, 0x4901, 0x85, 0x88, 0xab, 0x3d, 0x54, 0xf, 0x60, 0x21);
//TODO: Set guids
DEFINE_GUID(GUID_WDF_LOADER_INTERFACE_DIAGNOSTIC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
DEFINE_GUID(GUID_WDF_LOADER_INTERFACE_CLASS_BIND, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);


//----- Global vars -----//
BOOLEAN gFlagInit;
BOOLEAN gUnloaded;
OSVERSIONINFOW gOsVersionInfoW;
//===== Global vars =====//

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, WdfRegisterLibrary)
#pragma alloc_text (PAGE, WdfVersionBind)
#pragma alloc_text (PAGE, WdfVersionUnbind)
#pragma alloc_text (PAGE, WdfRegisterClassLibrary)
#endif

VOID
NTAPI
WdfLdrUnload(
	IN PDRIVER_OBJECT DriverObject
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	DllUnload();
}

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	DriverObject->DriverUnload = &WdfLdrUnload;
	return DllInitialize(RegistryPath);
}


NTSTATUS
NTAPI
WdfLdrOpenRegistryDiagnosticsHandle(
	OUT PHANDLE KeyHandle
)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING registryPath;	

	RtlInitUnicodeString(&registryPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf\\Kmdf\\Diagnostics");
	ObjectAttributes.RootDirectory = 0;
	ObjectAttributes.SecurityDescriptor = 0;
	ObjectAttributes.SecurityQualityOfService = 0;
	ObjectAttributes.ObjectName = &registryPath;	
	*KeyHandle = NULL;	
	ObjectAttributes.Length = 24;
	ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;//576;
	status = ZwOpenKey(KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
	
	if (!NT_SUCCESS(status) && WdfLdrDiags)
	{
		DbgPrint("WdfLdr: WdfLdrOpenRegistryDiagnosticsHandle - ");
		DbgPrint("ERROR: ZwOpenKey (%wZ) failed with Status 0x%x\n", &registryPath, status);
	}

	return status;
}


VOID
NTAPI
WdfLdrCloseRegistryDiagnosticsHandle(
	IN PVOID Handle
)
{
	ZwClose(Handle);
}


NTSTATUS
NTAPI
WdfLdrDiagnosticsValueByNameAsULONG(
	IN PUNICODE_STRING ValueName,
	OUT PULONG Value
)
{
	HANDLE keyHandle = NULL;
	NTSTATUS status;
	NTSTATUS result = STATUS_SUCCESS;

	if (ValueName != NULL && NULL != Value)
	{
		*Value = 0;
		if (KeGetCurrentIrql())
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: WdfLdrDiagnosticsValueByNameAsULONG - ");
				DbgPrint("ERROR: Not at PASSIVE_LEVEL\n");
			}

			status = STATUS_INVALID_PARAMETER;
		}
		else
		{
			status = WdfLdrOpenRegistryDiagnosticsHandle(&keyHandle);
			
			if(NT_SUCCESS(status))
			{				
				status = FxLdrQueryUlong(keyHandle, ValueName, Value);
				if (WdfLdrDiags)
				{
					DbgPrint("WdfLdr: WdfLdrDiagnosticsValueByNameAsULONG - ");
					DbgPrint("Value 0x%x\n", *Value);
				}
			}
			else if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: WdfLdrDiagnosticsValueByNameAsULONG - ");
				DbgPrint("ERROR: WdfLdrOpenRegistryDiagnosticsHandle failed with Status 0x%x\n", status);
			}
		}
		
		if (keyHandle != NULL)
		{
			WdfLdrCloseRegistryDiagnosticsHandle(keyHandle);
		}
		
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfLdrDiagnosticsValueByNameAsULONG - ");
			DbgPrint("Status 0x%x\n", status);
		}

		result = status;
	}
	else
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfLdrDiagnosticsValueByNameAsULONG - ");
			DbgPrint("ERROR: Invalid Input Parameter\n");
		}

		status = STATUS_INVALID_PARAMETER;
	}

	return result;
}


NTSTATUS
NTAPI
DllInitialize(
	IN PUNICODE_STRING RegistryPath
)
{
	NTSTATUS status;
	UNICODE_STRING csdVersion;
	ULONG ldrDiagnostic;

	UNREFERENCED_PARAMETER(RegistryPath);
	RtlInitUnicodeString(&csdVersion, L"DbgPrintOn");
		
	if (gFlagInit)
	{
		return STATUS_SUCCESS;
	}

	gFlagInit = TRUE;
	InitializeListHead(&gLibList);
	ExInitializeResourceLite(&Resource);	
	
	status = WdfLdrDiagnosticsValueByNameAsULONG(&csdVersion, &ldrDiagnostic);
	if ( NT_SUCCESS(status) && ldrDiagnostic)
	{
		WdfLdrDiags = TRUE;
	}

	status = AuxKlibInitialize();
	
	if(NT_SUCCESS(status))
	{
		RtlGetVersion(&gOsVersionInfoW);

		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: FxDllInitialize - ");			
			DbgPrint("OsVersion(%d.%d)\n", gOsVersionInfoW.dwMajorVersion, gOsVersionInfoW.dwMinorVersion);
		}
		status = STATUS_SUCCESS;
	}
	else
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: FxDllInitialize - ");
			DbgPrint("ERROR: AuxKlibInitialize failed with Status 0x%x\n", status);
		}
	}

	return status;
}


PLIST_ENTRY
NTAPI
LibraryUnloadClasses(
	IN PLIBRARY_MODULE LibModule
)
{
	PLIST_ENTRY classListHead;
	PLIST_ENTRY entry;
	PCLASS_MODULE pClassModule;
	LIST_ENTRY removedList;

	
	InitializeListHead(&removedList);
	classListHead = &LibModule->ClassListHead;

	if (!IsListEmpty(classListHead))
	{
		do
		{
			entry = classListHead->Flink;
			RemoveHeadList(classListHead);
			InsertTailList(&removedList, entry);
		} while (!IsListEmpty(classListHead));

		for(;;)
		{
			classListHead = removedList.Flink;

			if (IsListEmpty(&removedList))
			{
				break;
			}

			RemoveHeadList(&removedList);
			InitializeListHead(classListHead);
			pClassModule = CONTAINING_RECORD(classListHead, CLASS_MODULE, LibraryLinkage);

			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: LibraryUnloadClasses - ");
				DbgPrint("WdfLdr: LibraryUnloadClasses: unload class library %wZ (%p)\n", &pClassModule->Service, pClassModule);
			}

			ClassUnload(pClassModule, 0);
			if (!_InterlockedExchangeAdd(&pClassModule->ClassRefCount, -1))
				ClassCleanupAndFree(pClassModule);
		}
	}

	return classListHead;
}


VOID
NTAPI
DllUnload()
{
	PLIBRARY_MODULE pLibModule;
	LIST_ENTRY entry;
	LIST_ENTRY removeList;

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: DllUnload - ");
		DbgPrint("WdfLdr: DllUnload: enter\n");
	}

	if (!gUnloaded)
	{
		InitializeListHead(&removeList);
		entry.Flink = gLibList.Flink;
		gUnloaded = TRUE;

		if (!IsListEmpty(&gLibList))
		{
			do
			{
				RemoveHeadList(&gLibList);				
				InsertTailList(&removeList, &entry);

				entry.Flink = gLibList.Flink;
			} while (!IsListEmpty(&gLibList));

			for(;;)
			{
				entry = removeList;
				if (IsListEmpty(&removeList))
				{
					break;
				}
				
				RemoveHeadList(&removeList);
				InitializeListHead(entry.Flink);
				pLibModule = CONTAINING_RECORD(&entry, LIBRARY_MODULE, LibraryListEntry);

				if (WdfLdrDiags)
				{
					DbgPrint("WdfLdr: DllUnload - ");
					DbgPrint("WdfLdr: DllUnload: module(%p)\n", pLibModule);
				}

				LibraryUnloadClasses(pLibModule);
				LibraryUnload(pLibModule, 0);
				if (!_InterlockedExchangeAdd(&pLibModule->LibraryRefCount, -1))
					LibraryCleanupAndFree(pLibModule);
			}
		}
		ExDeleteResourceLite(&Resource);

		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: DllUnload - ");
			DbgPrint("WdfLdr: DllUnload: exit\n");
		}
	}
}



NTSTATUS
NTAPI
WdfLdrQueryInterface(
	IN PWDF_LOADER_INTERFACE LoaderInterface
)
{
	if (LoaderInterface && LoaderInterface->Header.InterfaceType)
	{
		if (RtlCompareMemory(LoaderInterface->Header.InterfaceType, &GUID_WDF_LOADER_INTERFACE_STANDARD, 0x10u) == 16)
		{
			if (LoaderInterface->Header.InterfaceSize == 24)
			{
				LoaderInterface->RegisterLibrary = (int(__stdcall*)(PWDF_LIBRARY_INFO, PUNICODE_STRING, PUNICODE_STRING))WdfRegisterLibrary;
				LoaderInterface->VersionBind = (int(__stdcall*)(PDRIVER_OBJECT, PUNICODE_STRING, PWDF_BIND_INFO, void***))WdfVersionBind;
				LoaderInterface->VersionUnbind = WdfVersionUnbind;
				LoaderInterface->DiagnosticsValueByNameAsULONG = WdfLdrDiagnosticsValueByNameAsULONG;
				return STATUS_SUCCESS;
			}
		}
		else if (RtlCompareMemory(LoaderInterface->Header.InterfaceType, &GUID_WDF_LOADER_INTERFACE_DIAGNOSTIC, 0x10u) == 16)
		{
			if (LoaderInterface->Header.InterfaceSize == 12)
			{
				LoaderInterface->RegisterLibrary = (int(__stdcall*)(PWDF_LIBRARY_INFO, PUNICODE_STRING, PUNICODE_STRING))WdfLdrDiagnosticsValueByNameAsULONG;
				return STATUS_SUCCESS;
			}
		}
		else
		{
			if (RtlCompareMemory(LoaderInterface->Header.InterfaceType, &GUID_WDF_LOADER_INTERFACE_CLASS_BIND, 0x10u) != 16)
				return STATUS_NOINTERFACE;
			if (LoaderInterface->Header.InterfaceSize == 16)
			{
				LoaderInterface->RegisterLibrary = (int(__stdcall*)(PWDF_LIBRARY_INFO, PUNICODE_STRING, PUNICODE_STRING))WdfVersionBindClass;
				LoaderInterface->VersionBind = (int(__stdcall*)(PDRIVER_OBJECT, PUNICODE_STRING, PWDF_BIND_INFO, void***))WdfVersionUnbindClass;
				return STATUS_SUCCESS;
			}
		}
	}
	
	return STATUS_INVALID_PARAMETER;
}


PLIBRARY_MODULE
NTAPI
FindModuleByServiceNameLocked(
	IN PUNICODE_STRING ServiceName
)
{
	PLIBRARY_MODULE pLibModule;
	PLIST_ENTRY currentLib;
	WCHAR name[30];
	WCHAR searchedServiceName[30];
	size_t length;
	UNICODE_STRING nameString;
	UNICODE_STRING searchedNameString;
	
	name[0] = 0;
	searchedServiceName[0] = 0;
	GetNameFromUnicodePath(ServiceName, searchedServiceName, sizeof(searchedServiceName));
	RtlStringCchLengthW(searchedServiceName, sizeof(searchedServiceName), &length);
	
	for (currentLib = gLibList.Flink; ; currentLib = currentLib->Flink)
	{
		if (currentLib == &gLibList)
		{
			return NULL;
		}

		pLibModule = CONTAINING_RECORD(currentLib, LIBRARY_MODULE, LibraryListEntry);
		GetNameFromUnicodePath(&pLibModule->Service, name, sizeof(name));
		RtlInitUnicodeString(&nameString, name);
		RtlInitUnicodeString(&searchedNameString, searchedServiceName);

		if (RtlEqualUnicodeString(&nameString, &searchedNameString, TRUE))
		{
			break;
		}
	}

	return pLibModule;
}


NTSTATUS
NTAPI
WdfRegisterLibrary(
	IN PWDF_LIBRARY_INFO LibraryInfo,
	IN PUNICODE_STRING ServicePath,
	IN PUNICODE_STRING LibraryDeviceName
)
{
	NTSTATUS status;
	PLIBRARY_MODULE pLibModule;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	FxLdrAcquireLoadedModuleLock();
	pLibModule = FindModuleByServiceNameLocked(ServicePath);

	if (pLibModule != NULL)
	{
		LibraryCopyInfo(pLibModule, LibraryInfo);
		status = GetImageBase(&pLibModule->ImageName, &pLibModule->ImageAddress, &pLibModule->ImageSize);
		
		if (!NT_SUCCESS(status))
		{
			FxLdrReleaseLoadedModuleLock();
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: WdfRegisterLibrary - ");
				DbgPrint("ERROR: GetImageBase(%wZ) failed with status 0x%x\n",
					pLibModule->ImageName,
					status);
			}
			goto end;
		}
	}
	else
	{
		pLibModule = LibraryCreate(LibraryInfo, ServicePath);
		if (pLibModule)
		{
			LibraryAddToLibraryListLocked(pLibModule);
		}
		else
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}

	FxLdrReleaseLoadedModuleLock();
	if (pLibModule != NULL)
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfRegisterLibrary - ");
			DbgPrint("Module(%p)\n", pLibModule);
		}
		status = LibraryOpen(pLibModule, LibraryDeviceName);

		if (NT_SUCCESS(status))
		{
			status = pLibModule->LibraryInfo->LibraryCommission();
			if (NT_SUCCESS(status))
			{
				return status;
			}
			
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: WdfRegisterLibrary - ");
				DbgPrint("ERROR: WdfRegisterLibrary: LibraryCommissionfailed status 0x%X\n", status);
			}
		}
		else if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfRegisterLibrary - ");
			DbgPrint("ERROR: LibraryOpen(%wZ) failed with status 0x%x\n", LibraryDeviceName, status);
		}
	}
end:	
	if (!NT_SUCCESS(status) && pLibModule)
		LibraryRemoveFromLibraryList(pLibModule);

	return status;
}


NTSTATUS
NTAPI
GetVersionRegistryHandle(
	IN PWDF_BIND_INFO BindInfo,
	OUT PHANDLE HandleRegKey
)
{
	NTSTATUS status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE handle;
	HANDLE KeyHandle;
	DECLARE_UNICODE_STRING_SIZE(String, 520);
		
	KeyHandle = NULL;
	handle = NULL;	
	status = RtlUnicodeStringPrintf(&String,
		L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf\\Kmdf\\%s\\Versions",
		BindInfo->Component);

	if (!NT_SUCCESS(status))
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: GetVersionRegistryHandle - ");
			DbgPrint("ERROR: RtlUnicodeStringPrintf failed with Status 0x%x\n", status);
		}
		goto end;
	}

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: GetVersionRegistryHandle - ");
		DbgPrint("Component path %wZ\n", &String);
	}

	ObjectAttributes.ObjectName = &String;
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;
	ObjectAttributes.Length = 24;
	ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;//576;
	status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
	
	if (NT_SUCCESS(status))
	{
		status = ConvertUlongToWString(BindInfo->Version.Major, &String);
		if (!NT_SUCCESS(status))
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: GetVersionRegistryHandle - ");
				DbgPrint("ERROR: ConvertUlongToWString failed with Status 0x%x\n", status);
			}
			goto end;
		}
		ObjectAttributes.SecurityDescriptor = 0;
		ObjectAttributes.SecurityQualityOfService = 0;
		ObjectAttributes.RootDirectory = KeyHandle;
		ObjectAttributes.ObjectName = &String;
		ObjectAttributes.Length = 24;
		ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;// 576;
		status = ZwOpenKey(&handle, 0x20019u, &ObjectAttributes);
		
		if (NT_SUCCESS(status))
			goto end;
	}

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: GetVersionRegistryHandle - ");
		DbgPrint("ERROR: ZwOpenKey (%wZ) failed with Status 0x%x\n", &String, status);
	}

end:
	*HandleRegKey = handle;
	
	if (KeyHandle != NULL)
	{
		ZwClose(KeyHandle);
	}

	return status;
}


NTSTATUS
NTAPI
GetDefaultServiceName(
	IN PWDF_BIND_INFO BindInfo,
	OUT PUNICODE_STRING RegistryPath
)
{
	PWCHAR buffer;
	NTSTATUS status;
	PUNICODE_STRING defaultServiceName;

	buffer = ExAllocatePoolWithTag(PagedPool, 120, WDFLDR_TAG);
	
	if (buffer != NULL)
	{
		defaultServiceName = RegistryPath;
		RegistryPath->Length = 0;
		RegistryPath->Buffer = NULL;
		RegistryPath->Length = 0;
		RegistryPath->MaximumLength = 120;
		RegistryPath->Buffer = buffer;
		status = RtlUnicodeStringPrintf(RegistryPath,
			L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Wdf%02d000",
			BindInfo->Version.Major);

		if (NT_SUCCESS(status))
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: GetDefaultServiceName - ");
				DbgPrint("Couldn't find control Key -- using default service name %wZ\n", defaultServiceName);
			}
			status = STATUS_SUCCESS;
		}
		else
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: GetDefaultServiceName - ");
				DbgPrint("ERROR: RtlUnicodeStringCopyString failed with Status 0x%x\n", status);
			}
			ExFreePoolWithTag(buffer, 0);
			defaultServiceName->Length = 0;
			defaultServiceName->Buffer = NULL;
		}
	}
	else
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: GetDefaultServiceName - ");
			DbgPrint("ERROR: ExAllocatePoolWithTag failed with status 0x%x\n", STATUS_INSUFFICIENT_RESOURCES);
		}
		status = STATUS_INSUFFICIENT_RESOURCES;
	}

	return status;
}


NTSTATUS
NTAPI
GetVersionServicePath(
	IN PWDF_BIND_INFO BindInfo,
	IN PUNICODE_STRING ServiceName
)
{
	NTSTATUS status;
	PKEY_VALUE_PARTIAL_INFORMATION pKeyVal;
	HANDLE handleRegKey;
	UNICODE_STRING ValueName;


	handleRegKey = NULL;
	pKeyVal = NULL;
	RtlInitUnicodeString(&ValueName, L"Service");
	status = GetVersionRegistryHandle(BindInfo, &handleRegKey);

	if (!NT_SUCCESS(status))
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: GetVersionServicePath - ");
			DbgPrint("ERROR: GetVersionRegistryHandle failed with Status 0x%x\n", status);
		}
	}
	else
	{
		status = FxLdrQueryData(handleRegKey, &ValueName, WDFLDR_TAG, &pKeyVal);
		if (!NT_SUCCESS(status))
		{
			if (WdfLdrDiags)
			{
				DbgPrint("WdfLdr: GetVersionServicePath - ");
				DbgPrint("ERROR: QueryData failed with status 0x%x\n", status);
			}
		}
		else
		{			
			status = BuildServicePath(pKeyVal, WDFLDR_TAG, ServiceName);
		}
	}

	if (!NT_SUCCESS(status))
	{
		status = GetDefaultServiceName(BindInfo, ServiceName);
		if (!NT_SUCCESS(status) && WdfLdrDiags)
		{
			DbgPrint("WdfLdr: GetVersionServicePath - ");
			DbgPrint("ERROR: GetVersionServicePath failed with Status 0x%x\n", status);
		}
	}
	else if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: GetVersionServicePath - ");
		DbgPrint("GetVersionServicePath (%wZ)\n", ServiceName);
	}

	if (handleRegKey != NULL)
		ZwClose(handleRegKey);
	if (pKeyVal != NULL)
		ExFreePoolWithTag(pKeyVal, 0);

	return status;
}


NTSTATUS
NTAPI
ReferenceVersion(
	IN PWDF_BIND_INFO BindInfo,
	OUT PLIBRARY_MODULE* LibModule
)
{
	BOOLEAN newCreated;
	NTSTATUS status;
	PLIBRARY_MODULE pLibModule;
	UNICODE_STRING driverServiceName;

	*LibModule = NULL;
	newCreated = FALSE;
	status = GetVersionServicePath(BindInfo, &driverServiceName);

	if (!NT_SUCCESS(status))
	{
		goto error;
	}


	FxLdrAcquireLoadedModuleLock();
	pLibModule = FindModuleByServiceNameLocked(&driverServiceName);

	if (pLibModule != NULL)
	{
		_InterlockedExchangeAdd(&pLibModule->LibraryRefCount, 1);
	}
	else
	{
		pLibModule = LibraryCreate(0, &driverServiceName);
		if (pLibModule != NULL)
		{
			_InterlockedExchangeAdd(&pLibModule->LibraryRefCount, 1);
			LibraryAddToLibraryListLocked(pLibModule);
			newCreated = TRUE;
		}
		else
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}

	FxLdrReleaseLoadedModuleLock();

	if (newCreated)
	{
		status = ZwLoadDriver(&driverServiceName);

		if (!NT_SUCCESS(status))
		{
			if (status == STATUS_IMAGE_ALREADY_LOADED ||
				status == STATUS_OBJECT_NAME_COLLISION)
			{

				if (pLibModule->LibraryInfo)
				{
					status = STATUS_SUCCESS;
				}
				else if (WdfLdrDiags)
				{
					DbgPrint("WdfLdr: ReferenceVersion - ");
					DbgPrint(
						"WdfLdr: ReferenceVersion: ZwLoadDriver failed and no Libray information was returned: %X\n",
						status);
				}
			}
			else
			{
				if (WdfLdrDiags)
				{
					DbgPrint("WdfLdr: ReferenceVersion - ");
					DbgPrint("WARNING: ZwLoadDriver (%wZ) failed with Status 0x%x\n", &driverServiceName, status);
				}
				pLibModule->ImageAlreadyLoaded = TRUE;
			}
		}
	}

	if (pLibModule != NULL &&
		_InterlockedExchangeAdd(&pLibModule->LibraryRefCount, -1) == 0)
	{
		LibraryCleanupAndFree(pLibModule);
	}

	if (NT_SUCCESS(status))
	{
		BindInfo->Module = pLibModule;
		*LibModule = pLibModule;
	}

	goto success;

error:
	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: ReferenceVersion - ");
		DbgPrint("ERROR: GetVersionServicePath failed, status 0x%x\n", status);
	}

success:
	FreeString(&driverServiceName);

	return status;
}


NTSTATUS
NTAPI
WdfVersionBind(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath,
	IN OUT PWDF_BIND_INFO BindInfo,
	OUT PWDF_COMPONENT_GLOBALS *ComponentGlobals
)
{
	NTSTATUS status;
	PLIBRARY_MODULE pLibModule;
	CLIENT_INFO clientInfo;
	PCLIENT_INFO pclientInfo;
	PCLIENT_MODULE  pClientModule;

	UNREFERENCED_PARAMETER(DriverObject);
	PAGED_CODE();

	pLibModule = NULL;
	clientInfo.Size = 8;

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: WdfVersionBind - ");
		DbgPrint("WdfLdr: WdfVersionBind: enter\n");
	}

	pClientModule = NULL;
	if (ComponentGlobals == NULL ||
		BindInfo == NULL ||
		BindInfo->FuncTable == NULL)
	{
		return STATUS_INVALID_PARAMETER;
	}

	status = ReferenceVersion(BindInfo, &pLibModule);

	if (!NT_SUCCESS(status))
	{
		goto clean;
	}
	
	_InterlockedExchangeAdd(&pLibModule->ClientRefCount, 1);
	status = LibraryLinkInClient(BindInfo->Module, RegistryPath, BindInfo, &clientInfo, &pClientModule);

	if (NT_SUCCESS(status))
	{
		clientInfo.RegistryPath = RegistryPath;
		pclientInfo = &clientInfo;
		status = pLibModule->LibraryInfo->LibraryRegisterClient(BindInfo, ComponentGlobals, (PVOID*)&pclientInfo);

		if (NT_SUCCESS(status))
		{
			pClientModule->Globals = *ComponentGlobals;
			pClientModule->Context = &clientInfo;
		}
		else if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfVersionBind - ");
			DbgPrint("WdfLdr: libraryRegisterClient: LibraryLinkInClient failed %X\n", status);
		}
	}
	else
	{
	clean:
		if (pLibModule)
		{
			WdfVersionUnbind(RegistryPath, BindInfo, *ComponentGlobals);
		}
		
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfVersionBind - ");
			DbgPrint("WdfLdr: DereferenceVersion: LibraryLinkInClient failed %X\n", status);
		}
	}

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: WdfVersionBind - ");
		DbgPrint("Returning with Status 0x%x\n", status);
	}

	return status;
}


NTSTATUS
NTAPI
WdfVersionBindClass(
	IN PWDF_BIND_INFO BindInfo,
	IN PWDF_COMPONENT_GLOBALS Globals,
	IN PWDF_CLASS_BIND_INFO ClassBindInfo
)
{
	PCLASS_CLIENT_MODULE pClassClientModule;	
	NTSTATUS status;
	PWDF_CLASS_LIBRARY_INFO pfnBindClient;
	PCLASS_MODULE pClassModule;
	
	pClassModule = NULL;

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: WdfVersionBindClass - ");
		DbgPrint("WdfLdr: WdfVersionBindClass: enter\n");
	}

	pClassClientModule = ClassClientCreate();

	if (pClassClientModule == NULL)
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfVersionBindClass - ");
			DbgPrint("WdfLdr: Could not create class client struct\n");
		}
		
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = ReferenceClassVersion(ClassBindInfo, BindInfo, &pClassModule);

	if (!NT_SUCCESS(status))
	{
		ExFreePoolWithTag(pClassClientModule, 0);
		return status;
	}
	
	_InterlockedExchangeAdd(&pClassModule->ClientRefCount, 1);
	status = ClassLinkInClient(pClassModule, ClassBindInfo, BindInfo, pClassClientModule);
	
	if (!NT_SUCCESS(status))
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfVersionBindClass - ");
			DbgPrint("WdfLdr: ClassLinkInClient failed 0x%x\n", status);
		}
		goto end;
	}

	pfnBindClient = pClassModule->ClassLibraryInfo;
	pClassClientModule = NULL;
	status = pfnBindClient->ClassLibraryBindClient(ClassBindInfo, Globals);

	if (!NT_SUCCESS(status) && WdfLdrDiags)
	{
		DbgPrint("WdfLdr: WdfVersionBindClass - ");
		DbgPrint("WdfLdr: ClassLibraryBindClient failed, status 0x%x\n", status);
	}

end:
	if (pClassModule != NULL)
		WdfVersionUnbindClass(BindInfo, Globals, ClassBindInfo);
	if (pClassClientModule != NULL)
		ExFreePoolWithTag(pClassClientModule, 0);

	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: WdfVersionBindClass - ");
		DbgPrint("Returning with Status 0x%x\n", status);
	}

	return status;
}


VOID
NTAPI
WdfVersionUnbindClass(
	IN PWDF_BIND_INFO BindInfo,
	IN PWDF_COMPONENT_GLOBALS Globals,
	IN PWDF_CLASS_BIND_INFO ClassBindInfo
)
{
	DereferenceClassVersion(ClassBindInfo, BindInfo, Globals);
}


NTSTATUS
NTAPI
DereferenceVersion(
	IN PWDF_BIND_INFO BindInfo,
	IN PWDF_COMPONENT_GLOBALS ComponentGlobals
)
{
	PLIBRARY_MODULE pLibModule;
	NTSTATUS status;

	pLibModule = BindInfo->Module;
	
	if (ComponentGlobals != NULL)
	{
		status = pLibModule->LibraryInfo->LibraryUnregisterClient(BindInfo, ComponentGlobals);
		
		if (!NT_SUCCESS(status) && WdfLdrDiags)
		{
			DbgPrint("WdfLdr: DereferenceVersion - ");
			DbgPrint("WdfLdr: DereferenceVersion: LibraryUnregisterClient failed %X\n", status);

		}
	}

	status = LibraryUnlinkClient(pLibModule, BindInfo);

	if (!NT_SUCCESS(status) && WdfLdrDiags)
	{
		DbgPrint("WdfLdr: DereferenceVersion - ");
		DbgPrint("WdfLdr: DereferenceVersion: LibraryUnlinkClient failed %X\n", status);
	}

	LibraryReleaseClientReference(pLibModule);
	BindInfo->Module = NULL;

	return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
WdfVersionUnbind(
	IN PUNICODE_STRING RegistryPath,
	IN PWDF_BIND_INFO BindInfo,
	IN PWDF_COMPONENT_GLOBALS ComponentGlobals
)
{
	NTSTATUS status;

	UNREFERENCED_PARAMETER(RegistryPath);
	PAGED_CODE();

	status = DereferenceVersion(BindInfo, ComponentGlobals);
	
	if (WdfLdrDiags)
	{
		DbgPrint("WdfLdr: WdfVersionUnbind - ");
		DbgPrint("WdfLdr: WdfVersionUnbind: exit: %X\n", status);
	}

	return status;
}


NTSTATUS
NTAPI
WdfRegisterClassLibrary(
	IN PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
	IN PUNICODE_STRING SourceString,
	IN PUNICODE_STRING ObjectName
)
{
	NTSTATUS status;
	PCLASS_MODULE pClassModule;
	int (*fnClassLibInit)(void);
	PLIBRARY_MODULE libModule;

	PAGED_CODE();

	status = STATUS_SUCCESS;
	FxLdrAcquireLoadedModuleLock();
	pClassModule = FindClassByServiceNameLocked(SourceString, &libModule);

	if (pClassModule)
	{
		pClassModule->ClassLibraryInfo = ClassLibInfo;
	}
	else
	{
		pClassModule = ClassCreate(ClassLibInfo, libModule, SourceString);		
		if (pClassModule)
		{
			LibraryAddToClassListLocked(libModule, pClassModule);
		}
		else
		{			
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}
	FxLdrReleaseLoadedModuleLock();
	
	if (pClassModule)
	{
		if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfRegisterClassLibrary - ");
			DbgPrint("Class Library (%p)\n", pClassModule);
		}
		status = ClassOpen(pClassModule, ObjectName);

		if (NT_SUCCESS(status))
		{
			fnClassLibInit = (int (*)(void))pClassModule->ClassLibraryInfo->ClassLibraryInitialize;
			if (fnClassLibInit)
			{
				status = fnClassLibInit();
				if (NT_SUCCESS(status))
					return status;
				
				if (WdfLdrDiags)
				{
					DbgPrint("WdfLdr: WdfRegisterClassLibrary - ");
					DbgPrint("ERROR: WdfRegisterClassLibrary: ClassLibraryInitialize failed, status 0x%x\n", status);
				}
			}
		}
		else if (WdfLdrDiags)
		{
			DbgPrint("WdfLdr: WdfRegisterClassLibrary - ");
			DbgPrint("ERROR: ClassOpen(%wZ) failed, status 0x%x\n", ObjectName, status);
		}
	}

	if (!NT_SUCCESS(status) && pClassModule)
		ClassRemoveFromLibraryList(pClassModule);

	return status;
}
