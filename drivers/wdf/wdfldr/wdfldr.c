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
    _In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DllUnload();
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = &WdfLdrUnload;
    return DllInitialize(RegistryPath);
}


NTSTATUS
NTAPI
WdfLdrOpenRegistryDiagnosticsHandle(
    _Out_ PHANDLE KeyHandle)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING registryPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf\\Kmdf\\Diagnostics");

    ObjectAttributes.RootDirectory = 0;
    ObjectAttributes.SecurityDescriptor = 0;
    ObjectAttributes.SecurityQualityOfService = 0;
    ObjectAttributes.ObjectName = &registryPath;    
    *KeyHandle = NULL;    
    ObjectAttributes.Length = 24;
    ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;
    status = ZwOpenKey(KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
    
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ZwOpenKey (%wZ) failed with Status 0x%x\n", &registryPath, status));
    }

    return status;
}


VOID
NTAPI
WdfLdrCloseRegistryDiagnosticsHandle(
    _In_ PVOID Handle)
{
    ZwClose(Handle);
}


NTSTATUS
NTAPI
WdfLdrDiagnosticsValueByNameAsULONG(
    _In_ PUNICODE_STRING ValueName,
    _Out_ PULONG Value)
{
    HANDLE keyHandle = NULL;
    NTSTATUS status;
    NTSTATUS result = STATUS_SUCCESS;

    if (ValueName == NULL || NULL == Value)
    {
        __DBGPRINT(("ERROR: Invalid Input Parameter\n"));

        status = STATUS_INVALID_PARAMETER;
        goto exit;
    }

    *Value = 0;
    if (KeGetCurrentIrql())
    {
        __DBGPRINT(("Not at PASSIVE_LEVEL\n"));

        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        // open framework diagnostic key
        status = WdfLdrOpenRegistryDiagnosticsHandle(&keyHandle);
            
        if(NT_SUCCESS(status))
        {
            // get value as ulong
            status = FxLdrQueryUlong(keyHandle, ValueName, Value);
                
            __DBGPRINT(("Value 0x%x\n", *Value));
        }
        else
        {
            __DBGPRINT(("ERROR: WdfLdrOpenRegistryDiagnosticsHandle failed with Status 0x%x\n", status));
        }
    }
        
    if (keyHandle != NULL)
    {
        WdfLdrCloseRegistryDiagnosticsHandle(keyHandle);
    }

    __DBGPRINT(("Status 0x%x\n", status));

    result = status;

exit:
    return result;
}


NTSTATUS
NTAPI
DllInitialize(
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    UNICODE_STRING csdVersion = RTL_CONSTANT_STRING(L"DbgPrintOn");
    ULONG ldrDiagnostic;

    UNREFERENCED_PARAMETER(RegistryPath);
        
    if (gFlagInit)
    {
        return STATUS_SUCCESS;
    }

    // initialize library list
    gFlagInit = TRUE;
    InitializeListHead(&gLibList);
    ExInitializeResourceLite(&Resource);    
    
    // get debug print value
    status = WdfLdrDiagnosticsValueByNameAsULONG(&csdVersion, &ldrDiagnostic);
    if (NT_SUCCESS(status) && ldrDiagnostic)
    {
        WdfLdrDiags = TRUE;
    }

    // TODO: move this code to aux_klib.lib
    status = AuxKlibInitialize();
    
    if (NT_SUCCESS(status))
    {
        RtlGetVersion(&gOsVersionInfoW);

        __DBGPRINT(("OsVersion(%d.%d)\n", gOsVersionInfoW.dwMajorVersion, gOsVersionInfoW.dwMinorVersion));
        status = STATUS_SUCCESS;
    }
    else
    {
        __DBGPRINT(("ERROR: AuxKlibInitialize failed with Status 0x%x\n", status));
    }

    return status;
}


PLIST_ENTRY
NTAPI
LibraryUnloadClasses(
    _In_ PLIBRARY_MODULE LibModule)
{
    PLIST_ENTRY classListHead;
    PLIST_ENTRY entry;
    PCLASS_MODULE pClassModule;
    LIST_ENTRY removedList;

    
    InitializeListHead(&removedList);
    classListHead = &LibModule->ClassListHead;

    if (IsListEmpty(classListHead))
    {
        goto exit;
    }

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
            
        __DBGPRINT(("Unload class library %wZ (%p)\n", &pClassModule->Service, pClassModule));

        ClassUnload(pClassModule, 0);
        if (!_InterlockedExchangeAdd(&pClassModule->ClassRefCount, -1))
            ClassCleanupAndFree(pClassModule);
    }
    
exit:
    return classListHead;
}


VOID
NTAPI
DllUnload()
{
    PLIBRARY_MODULE pLibModule;
    LIST_ENTRY entry;
    LIST_ENTRY removeList;

    __DBGPRINT(("enter"));

    if (gUnloaded)
    {
        return;
    }

    InitializeListHead(&removeList);
    entry.Flink = gLibList.Flink;
    gUnloaded = TRUE;

    if (IsListEmpty(&gLibList))
    {
        goto clean;
    }
    
    // copy libs from main list to remove list
    do
    {
        RemoveHeadList(&gLibList);                
        InsertTailList(&removeList, &entry);

        entry.Flink = gLibList.Flink;
    } while (!IsListEmpty(&gLibList));

    // remove list entries and unload libs
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

        __DBGPRINT(("module(%p)\n", pLibModule));

        LibraryUnloadClasses(pLibModule);
        LibraryUnload(pLibModule, 0);

        if (!_InterlockedExchangeAdd(&pLibModule->LibraryRefCount, -1))
            LibraryCleanupAndFree(pLibModule);
    }

clean:    
    ExDeleteResourceLite(&Resource);

    __DBGPRINT(("exit"));
}



NTSTATUS
NTAPI
WdfLdrQueryInterface(
    _In_ PWDF_LOADER_INTERFACE LoaderInterface)
{
    if (LoaderInterface == NULL || LoaderInterface->Header.InterfaceType == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (RtlCompareMemory(LoaderInterface->Header.InterfaceType, &GUID_WDF_LOADER_INTERFACE_STANDARD, 0x10u) == 16)
    {
        if (LoaderInterface->Header.InterfaceSize == 24)
        {
            // fill interface functions addresses
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

    return STATUS_INVALID_PARAMETER;
}

/********************************************
 * 
 * Search module by name in global library list
 * 
 * Params:
 *    ServiceName - searched name
 * 
 * Result:
 *    Finded module pointer
 * 
*********************************************/
PLIBRARY_MODULE
NTAPI
FindModuleByServiceNameLocked(
    _In_ PUNICODE_STRING ServiceName)
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

/********************************************
 * 
 * Register wdf01000 library
 * 
 * Params:
 *    LibraryInfo - information by register lib
 *    ServicePath - service path in registry
 *    LibraryDeviceName - kmdf device name
 * 
 * Result:
 *    Finded module pointer
 * 
*********************************************/
NTSTATUS
NTAPI
WdfRegisterLibrary(
    _In_ PWDF_LIBRARY_INFO LibraryInfo,
    _In_ PUNICODE_STRING ServicePath,
    _In_ PUNICODE_STRING LibraryDeviceName)
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
            
            __DBGPRINT(("ERROR: GetImageBase(%wZ) failed with status 0x%x\n",
                    pLibModule->ImageName,
                    status));

            goto end;
        }
    }
    else
    {
        pLibModule = LibraryCreate(LibraryInfo, ServicePath);
        // add lib to global list
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
        __DBGPRINT(("Module(%p) %wZ\n", pLibModule, &pLibModule->ImageName));
        status = LibraryOpen(pLibModule, LibraryDeviceName);

        if (NT_SUCCESS(status))
        {
            status = pLibModule->LibraryInfo->LibraryCommission();
            if (NT_SUCCESS(status))
            {
                return status;
            }

            __DBGPRINT(("ERROR: WdfRegisterLibrary: LibraryCommissionfailed status 0x%X\n", status));
        }
        else
        {
            __DBGPRINT(("ERROR: LibraryOpen(%wZ) failed with status 0x%x\n", LibraryDeviceName, status));
        }
    }
end:    
    if (!NT_SUCCESS(status) && pLibModule)
        LibraryRemoveFromLibraryList(pLibModule);

    return status;
}

/********************************************
 * 
 * Open framework version registry key
 * 
 * Params:
 *    BindInfo - bind information
 *    HandleRegKey - opened key handle
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
GetVersionRegistryHandle(
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PHANDLE HandleRegKey)
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
        __DBGPRINT(("ERROR: RtlUnicodeStringPrintf failed with Status 0x%x\n", status));
        goto end;
    }

    __DBGPRINT(("Component path %wZ\n", &String));

    ObjectAttributes.ObjectName = &String;
    ObjectAttributes.RootDirectory = NULL;
    ObjectAttributes.SecurityDescriptor = NULL;
    ObjectAttributes.SecurityQualityOfService = NULL;
    ObjectAttributes.Length = 24;
    ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;
    status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
    
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ZwOpenKey (%wZ) failed with Status 0x%x\n", &String, status));
        goto end;
    }

    status = ConvertUlongToWString(BindInfo->Version.Major, &String);
        
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ConvertUlongToWString failed with Status 0x%x\n", status));
        goto end;
    }

    ObjectAttributes.SecurityDescriptor = 0;
    ObjectAttributes.SecurityQualityOfService = 0;
    ObjectAttributes.RootDirectory = KeyHandle;
    ObjectAttributes.ObjectName = &String;
    ObjectAttributes.Length = 24;
    ObjectAttributes.Attributes = OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE;
    status = ZwOpenKey(&handle, 0x20019u, &ObjectAttributes);
        
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ZwOpenKey (%wZ) failed with Status 0x%x\n", &String, status));
    }

end:
    *HandleRegKey = handle;
    
    if (KeyHandle != NULL)
    {
        ZwClose(KeyHandle);
    }

    return status;
}

/********************************************
 * 
 * Create service path by bind info
 * 
 * Params:
 *    BindInfo - bind information
 *    RegistryPath - created path
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
GetDefaultServiceName(
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PUNICODE_STRING RegistryPath
)
{
    PWCHAR buffer;
    NTSTATUS status;
    PUNICODE_STRING defaultServiceName;

    buffer = ExAllocatePoolWithTag(PagedPool, 120, WDFLDR_TAG);
    
    if (buffer == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        __DBGPRINT(("ERROR: ExAllocatePoolWithTag failed with status 0x%x\n", status));
        goto exit;
    }

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
        __DBGPRINT(("Couldn't find control Key -- using default service name %wZ\n", defaultServiceName));
        status = STATUS_SUCCESS;
    }
    else
    {
        __DBGPRINT(("ERROR: RtlUnicodeStringCopyString failed with Status 0x%x\n", status));
        
        ExFreePoolWithTag(buffer, 0);
        defaultServiceName->Length = 0;
        defaultServiceName->Buffer = NULL;
    }
    
exit:
    return status;
}

/********************************************
 * 
 * Create service path by bind info
 * 
 * Params:
 *    BindInfo - bind information
 *    ServicePath - service path in registry
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
GetVersionServicePath(
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PUNICODE_STRING ServicePath)
{
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyVal = NULL;
    HANDLE handleRegKey = NULL;
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"Service");

    status = GetVersionRegistryHandle(BindInfo, &handleRegKey);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: GetVersionRegistryHandle failed with Status 0x%x\n", status));
    }
    else
    {
        // get service name
        status = FxLdrQueryData(handleRegKey, &ValueName, WDFLDR_TAG, &pKeyVal);
        
        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("ERROR: QueryData failed with status 0x%x\n", status));
        }
        else
        {            
            status = BuildServicePath(pKeyVal, WDFLDR_TAG, ServicePath);
        }
    }

    if (!NT_SUCCESS(status))
    {
        status = GetDefaultServiceName(BindInfo, ServicePath);
        
        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("ERROR: GetVersionServicePath failed with Status 0x%x\n", status));
        }
    }
    else
    {
        __DBGPRINT(("(%wZ)\n", ServicePath));
    }

    if (handleRegKey != NULL)
        ZwClose(handleRegKey);
    if (pKeyVal != NULL)
        ExFreePoolWithTag(pKeyVal, 0);

    return status;
}

/********************************************
 * 
 * Increment reference for wdf01000
 * 
 * Params:
 *    BindInfo - bind information
 *    ServicePath - service path in registry
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
ReferenceVersion(
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PLIBRARY_MODULE* LibModule)
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

    // try find module in loaded list
    FxLdrAcquireLoadedModuleLock();
    pLibModule = FindModuleByServiceNameLocked(&driverServiceName);

    // if module finded, increment reference
    if (pLibModule != NULL)
    {
        _InterlockedExchangeAdd(&pLibModule->LibraryRefCount, 1);
    }
    else
    {
        // create library and add it to list
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

    // if new library create, load driver
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
                else
                {
                    __DBGPRINT(("ZwLoadDriver failed and no Libray information was returned: %X\n", status));
                }
            }
            else
            {
                __DBGPRINT(("WARNING: ZwLoadDriver (%wZ) failed with Status 0x%x\n", &driverServiceName, status));
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
    __DBGPRINT(("ERROR: GetVersionServicePath failed, status 0x%x\n", status));

success:
    FreeString(&driverServiceName);

    return status;
}

/********************************************
 * 
 * Bind client driver with framework
 * 
 * Params:
 *    DriverObject - driver object 
 *    RegistryPath - registry path
 *    BindInfo - client driver bind information
 *    ComponentGlobals - client driver global settings
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
WdfVersionBind(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath,
    _Inout_ PWDF_BIND_INFO BindInfo,
    _Out_ PWDF_COMPONENT_GLOBALS *ComponentGlobals)
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

    __DBGPRINT(("enter"));

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
        __DBGPRINT(("ReferenceVersion failed %X\n", status));
        goto clean;
    }

    _InterlockedExchangeAdd(&pLibModule->ClientRefCount, 1);
    status = LibraryLinkInClient(BindInfo->Module, RegistryPath, BindInfo, &clientInfo, &pClientModule);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("LibraryLinkInClient failed %X\n", status));
        goto clean;
    }

    clientInfo.RegistryPath = RegistryPath;
    pclientInfo = &clientInfo;
        
    // Call register function from wdf01000 driver.
    // Framework driver write functions table address in bindInfo
    status = pLibModule->LibraryInfo->LibraryRegisterClient(BindInfo, ComponentGlobals, (PVOID*)&pclientInfo);

    if (NT_SUCCESS(status))
    {
        pClientModule->Globals = *ComponentGlobals;
        pClientModule->Context = &clientInfo;
        goto exit;
    }
    else
    {
        __DBGPRINT(("LibraryLinkInClient failed %X\n", status));
    }

clean:
    if (pLibModule)
    {
        WdfVersionUnbind(RegistryPath, BindInfo, *ComponentGlobals);
    }

exit:
    __DBGPRINT(("Returning with Status 0x%x\n", status));

    return status;
}

/*
* http://redplait.blogspot.com/2013/03/ucxfunctionsidc.html
*/
NTSTATUS
NTAPI
WdfVersionBindClass(
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS Globals,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo)
{
    PCLASS_CLIENT_MODULE pClassClientModule;    
    NTSTATUS status;
    PWDF_CLASS_LIBRARY_INFO pfnBindClient;
    PCLASS_MODULE pClassModule;
    
    pClassModule = NULL;

    __DBGPRINT(("enter\n"));

    pClassClientModule = ClassClientCreate();

    if (pClassClientModule == NULL)
    {
        __DBGPRINT(("Could not create class client struct\n"));
        
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
        __DBGPRINT(("ClassLinkInClient failed 0x%x\n", status));
        goto end;
    }

    pfnBindClient = pClassModule->ClassLibraryInfo;
    pClassClientModule = NULL;
    status = pfnBindClient->ClassLibraryBindClient(ClassBindInfo, Globals);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ClassLibraryBindClient failed, status 0x%x\n", status));
    }

end:
    if (pClassModule != NULL)
        WdfVersionUnbindClass(BindInfo, Globals, ClassBindInfo);
    if (pClassClientModule != NULL)
        ExFreePoolWithTag(pClassClientModule, 0);

    __DBGPRINT(("Returning with Status 0x%x\n", status));

    return status;
}


VOID
NTAPI
WdfVersionUnbindClass(
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS Globals,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo)
{
    DereferenceClassVersion(ClassBindInfo, BindInfo, Globals);
}

/********************************************
 * 
 * Decrement library client count and unregister client driver
 * 
 * Params:
 *    BindInfo - client driver bind information
 *    ComponentGlobals - clien driver global settings
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
DereferenceVersion(
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals)
{
    PLIBRARY_MODULE pLibModule;
    NTSTATUS status;

    pLibModule = BindInfo->Module;
    
    if (ComponentGlobals != NULL)
    {
        // Call wdf01000 function for unregister client
        status = pLibModule->LibraryInfo->LibraryUnregisterClient(BindInfo, ComponentGlobals);
        
        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("LibraryUnregisterClient failed %X\n", status));
        }
    }

    status = LibraryUnlinkClient(pLibModule, BindInfo);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("LibraryUnlinkClient failed %X\n", status));
    }

    LibraryReleaseClientReference(pLibModule);
    BindInfo->Module = NULL;

    return STATUS_SUCCESS;
}

/********************************************
 * 
 * Unbind client driver from framework
 * 
 * Params:
 *    RegistryPath - registry path
 *    BindInfo - client driver bind information
 *    ComponentGlobals - client driver global settings
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
WdfVersionUnbind(
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals)
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);
    PAGED_CODE();

    status = DereferenceVersion(BindInfo, ComponentGlobals);
    
    __DBGPRINT(("exit: %X\n", status));

    return status;
}


/********************************************
 * 
 * http://redplait.blogspot.com/2013/03/ucxfunctionsidc.html
 * 
 * Register extension driver
 * 
 * Params:
 *    ClassBindInfo - client driver bind information
 *    SourceString - 
 *    ObjectName - 
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
WdfRegisterClassLibrary(
    _In_ PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
    _In_ PUNICODE_STRING SourceString,
    _In_ PUNICODE_STRING ObjectName)
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
    
    if (!pClassModule)
    {
        goto exit;
    }

    __DBGPRINT(("Class Library (%p)\n", pClassModule));

    status = ClassOpen(pClassModule, ObjectName);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ClassOpen(%wZ) failed, status 0x%x\n", ObjectName, status));
        goto clean;
    }

    fnClassLibInit = (int (*)(void))pClassModule->ClassLibraryInfo->ClassLibraryInitialize;
    if (fnClassLibInit)
    {
        status = fnClassLibInit();
        if (NT_SUCCESS(status))
            return status;
        
        __DBGPRINT(("ERROR: WdfRegisterClassLibrary: ClassLibraryInitialize failed, status 0x%x\n", status));
    }

clean:
    if (!NT_SUCCESS(status) && pClassModule)
        ClassRemoveFromLibraryList(pClassModule);

exit:
    return status;
}
