/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 */


#include "wdfloader.h"


DEFINE_GUID(GUID_WDF_LOADER_INTERFACE_DIAGNOSTIC, 0x55905BA4, 0x1DD2, 0x45D3, 0xAB, 0xEA, 0xF7, 0xA8, 0x70, 0x11, 0xD6, 0x9F);
DEFINE_GUID(GUID_WDF_LOADER_INTERFACE_CLASS_BIND, 0xFA4838CB, 0x1D08, 0x41E1, 0x8B, 0xA8, 0x71, 0x9C, 0xF8, 0x44, 0xEA, 0x74);

//----- Global vars -----//
BOOLEAN gAlreadyInitialized = FALSE;
BOOLEAN gAlreadyUnloaded = FALSE;

UINT32 WdfLdrDiags;
WDF_LDR_GLOBALS WdfLdrGlobals;
//===== Global vars =====//


VOID
NTAPI
WdfLdrUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    DllUnload();
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = WdfLdrUnload;
    return DllInitialize(RegistryPath);
}

/**
 * @brief Retrieves an ULONG value from KMDF diagnostics registry key
 */
NTSTATUS
NTAPI
WdfLdrDiagnosticsValueByNameAsULONG(
    _In_ PUNICODE_STRING ValueName,
    _Out_ PULONG Value)
{
    HANDLE keyHandle = NULL;
    NTSTATUS status;

    if (ValueName == NULL || Value == NULL)
    {
        __DBGPRINT(("ERROR: Invalid Input Parameter\n"));

        return STATUS_INVALID_PARAMETER;
    }

    *Value = 0; // for compatibility

    if (KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        __DBGPRINT(("Not at PASSIVE_LEVEL\n"));

        return STATUS_INVALID_PARAMETER;
    }

    // open framework diagnostics key
    OBJECT_ATTRIBUTES attributes;
    static UNICODE_STRING diagPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf\\Kmdf\\Diagnostics");

    InitializeObjectAttributes(&attributes,
                               &diagPath,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&keyHandle, KEY_QUERY_VALUE, &attributes);
        
    if (NT_SUCCESS(status))
    {
        status = FxLdrQueryUlong(keyHandle, ValueName, Value);
            
        __DBGPRINT(("Status %x, value 0x%x\n", status, *Value));

        ZwClose(keyHandle);
    }
    else
    {
        __DBGPRINT(("ERROR: ZwOpenKey failed with Status 0x%x\n", status));
    }

    return status;
}

NTSTATUS
NTAPI
DllInitialize(
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    UNICODE_STRING dbgPrintOn = RTL_CONSTANT_STRING(L"DbgPrintOn");

    UNREFERENCED_PARAMETER(RegistryPath);
        
    if (gAlreadyInitialized)
    {
        return STATUS_SUCCESS;
    }

    // Initialize library list
    gAlreadyInitialized = TRUE;
    InitializeListHead(&WdfLdrGlobals.LoadedModulesList);
    ExInitializeResourceLite(&WdfLdrGlobals.LoadedModulesListLock);    
    
    // We try to keep compatibility with MS wdfldr here, so check if diagnostics are enabled
    ULONG diagEnabled;
    status = WdfLdrDiagnosticsValueByNameAsULONG(&dbgPrintOn, &diagEnabled);
    if (NT_SUCCESS(status) && diagEnabled != 0)
    {
        WdfLdrDiags |= DIAGFLAG_ENABLED;
    }

    status = AuxKlibInitialize();
    
    if (NT_SUCCESS(status))
    {
        RtlGetVersion((POSVERSIONINFOW)&WdfLdrGlobals.OsVersion);

        __DBGPRINT(("OsVersion(%d.%d)\n", WdfLdrGlobals.OsVersion.dwMajorVersion, WdfLdrGlobals.OsVersion.dwMinorVersion));
        status = STATUS_SUCCESS;
    }
    else
    {
        __DBGPRINT(("ERROR: AuxKlibInitialize failed with Status %x\n", status));
    }

    return status;
}

VOID
NTAPI
DllUnload()
{
    if (gAlreadyUnloaded)
    {
        return;
    }

    gAlreadyUnloaded = TRUE;

    // Unload all loaded libraries
    while (!IsListEmpty(&WdfLdrGlobals.LoadedModulesList))
    {
        PLIST_ENTRY entry = RemoveHeadList(&WdfLdrGlobals.LoadedModulesList);
        PLIBRARY_MODULE module = CONTAINING_RECORD(&entry, LIBRARY_MODULE, LibraryListEntry);

        __DBGPRINT(("Unloading module(%p)\n", module));

        LibraryUnloadClasses(module);
        LibraryUnload(module);
    }

    ExDeleteResourceLite(&WdfLdrGlobals.LoadedModulesListLock);
}

NTSTATUS
NTAPI
WdfLdrQueryInterface(
    _In_ PWDF_INTERFACE_HEADER LoaderInterface)
{
    if (LoaderInterface == NULL || LoaderInterface->InterfaceType == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (IsEqualGUID(LoaderInterface->InterfaceType, &GUID_WDF_LOADER_INTERFACE_STANDARD))
    {
        if (LoaderInterface->InterfaceSize >= sizeof(PWDF_LOADER_INTERFACE))
        {
            PWDF_LOADER_INTERFACE lInterface = (PWDF_LOADER_INTERFACE)LoaderInterface;

            lInterface->RegisterLibrary = WdfRegisterLibrary;
            lInterface->VersionBind = WdfVersionBind;
            lInterface->VersionUnbind = WdfVersionUnbind;
            lInterface->DiagnosticsValueByNameAsULONG = WdfLdrDiagnosticsValueByNameAsULONG;

            return STATUS_SUCCESS;
        }
    }
    else if (IsEqualGUID(LoaderInterface->InterfaceType, &GUID_WDF_LOADER_INTERFACE_DIAGNOSTIC))
    {
        if (LoaderInterface->InterfaceSize >= sizeof(WDF_LOADER_INTERFACE_DIAGNOSTIC))
        {
            PWDF_LOADER_INTERFACE_DIAGNOSTIC lInterface = 
                (PWDF_LOADER_INTERFACE_DIAGNOSTIC)LoaderInterface;

            lInterface->DiagnosticsValueByNameAsULONG = WdfLdrDiagnosticsValueByNameAsULONG;

            return STATUS_SUCCESS;
        }
    }
    else if (IsEqualGUID(LoaderInterface->InterfaceType, &GUID_WDF_LOADER_INTERFACE_CLASS_BIND))
    {    
        if (LoaderInterface->InterfaceSize >= sizeof(WDF_LOADER_INTERFACE_CLASS_BIND))
        {
            PWDF_LOADER_INTERFACE_CLASS_BIND lInterface =
                (PWDF_LOADER_INTERFACE_CLASS_BIND)LoaderInterface;

            lInterface->ClassBind = WdfVersionBindClass;
            lInterface->ClassUnbind = WdfVersionUnbindClass;

            return STATUS_SUCCESS;
        }
    }
    else
    {
        return STATUS_NOINTERFACE;
    }

    return STATUS_INVALID_PARAMETER;
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
CODE_SEG("PAGE")
NTSTATUS
NTAPI
WdfRegisterLibrary(
    _In_ PWDF_LIBRARY_INFO LibraryInfo,
    _In_ PUNICODE_STRING ServicePath,
    _In_ PCUNICODE_STRING LibraryDeviceName)
{
    NTSTATUS status;
    PLIBRARY_MODULE pLibModule;

    PAGED_CODE();

    FxLdrAcquireLoadedModuleLock();
    pLibModule = FindLibraryByServicePathLocked(ServicePath);

    if (pLibModule != NULL)
    {
        // This happens when a driver which uses KMDF library is loaded before the library itself.
        // In such case, LibraryCreate(NULL, ...) was called in from WdfVersionBind

        ASSERT(pLibModule->ImplicitlyLoaded == FALSE);
        ASSERT(pLibModule->LibraryInfo == NULL);

        pLibModule->Version = LibraryInfo->Version;
        pLibModule->LibraryInfo = LibraryInfo;

        status = GetImageInfo(&pLibModule->ImageName,
                              &pLibModule->ImageAddress,
                              &pLibModule->ImageSize);
        if (!NT_SUCCESS(status))
        {            
            __DBGPRINT(("ERROR: GetImageInfo(%wZ) failed with status 0x%x\n",
                        pLibModule->ImageName, status));
        }
    }
    else
    {
        status = LibraryCreate(LibraryInfo, ServicePath, &pLibModule);
    }

    // Reference the library while we're working on it
    if (pLibModule)
    {
        InterlockedIncrement(&pLibModule->LibraryRefCount);
    }

    FxLdrReleaseLoadedModuleLock();

    if (!pLibModule)
    {
        __DBGPRINT(("WdfRegisterLibrary: LibraryCreate(%wZ) failed, status %X\n",
                    ServicePath, status));
        return status;
    }

    __DBGPRINT(("Registering library(%p) %wZ\n", pLibModule, &pLibModule->ImageName));

    // WdfRegisterLibrary is called from the library itself so we're sure it is loaded
    // Now reference the library's device object
    status = LibraryOpen(pLibModule, LibraryDeviceName);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: LibraryOpen(%wZ) failed with status %x\n", LibraryDeviceName, status));
        goto Failure;
    }

    // Now call the custom library callback
    status = pLibModule->LibraryInfo->LibraryCommission();
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: WdfRegisterLibrary: LibraryCommission failed status 0x%X\n", status));
        goto Failure;
    }

    // The work is finished
    LibraryDereference(pLibModule);
    return status;

Failure:
    // Dereference two times - the library should be cleaned up then
    // Setting LibraryInfo to NULL so ZwUnloadDriver is not called
    pLibModule->LibraryInfo = NULL;
    LibraryDereference(pLibModule);

    // If the library was created in WdfVersionBind, it will be dereferenced there
    if (pLibModule->ImplicitlyLoaded)
        LibraryDereference(pLibModule);
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
CODE_SEG("PAGE")
NTSTATUS
NTAPI
WdfVersionBind(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING ServicePath,
    _Inout_ PWDF_BIND_INFO BindInfo,
    _Out_ PWDF_COMPONENT_GLOBALS *ComponentGlobals)
{
    NTSTATUS status;
    PLIBRARY_MODULE pLibModule;

    UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE();

    __DBGPRINT(("enter"));

    if (ComponentGlobals == NULL || BindInfo == NULL || BindInfo->FuncTable == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    UNICODE_STRING libraryServicePath;
    status = GetVersionServicePath(BindInfo, &libraryServicePath);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // This will load the KMDF library if it is not loaded already. It also references the library
    status = LibraryFindOrLoad(&libraryServicePath, &pLibModule);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("LibraryReferenceAndLoad failed %X\n", status));
        return status;
    }

    PCLIENT_MODULE clientModule;
    CLIENT_INFO clientInfo;
    clientInfo.Size = sizeof(CLIENT_INFO);
    clientInfo.RegistryPath = ServicePath;
    
    // Add a client to the list and increment client reference
    status = LibraryLinkInClient(pLibModule, ServicePath, BindInfo, &clientInfo, &clientModule);
    LibraryDereference(pLibModule);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("LibraryLinkInClient failed %X\n", status));
        LibraryUnload(pLibModule);
        return status;
    }

    BindInfo->Module = pLibModule;
    PVOID context = &clientInfo;
        
    // Register the client in KMDF library. It will write function pointers in BindInfo
    status = pLibModule->LibraryInfo->LibraryRegisterClient(BindInfo, ComponentGlobals, &context);
    if (NT_SUCCESS(status))
    {
        clientModule->Globals = *ComponentGlobals;
        clientModule->Context = context; // Pointer to FX_DRIVER_GLOBALS
        return status;
    }

    __DBGPRINT(("LibraryRegisterClient failed with status %x\n", status));
    WdfVersionUnbind(ServicePath, BindInfo, *ComponentGlobals);

    return status;
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
CODE_SEG("PAGE")
NTSTATUS
NTAPI
WdfVersionUnbind(
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals)
{
    PLIBRARY_MODULE pLibModule = BindInfo->Module;
    NTSTATUS status;

    LibraryReference(pLibModule);
    
    if (ComponentGlobals != NULL)
    {
        status = pLibModule->LibraryInfo->LibraryUnregisterClient(BindInfo, ComponentGlobals);        
        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("LibraryUnregisterClient failed with status %X\n", status));
        }
    }

    if (!LibraryUnlinkClient(pLibModule, BindInfo))
    {
        __DBGPRINT(("LibraryUnlinkClient failed with status %X\n", status));
    }

    LibraryDereference(pLibModule);

    BindInfo->Module = NULL;

    return STATUS_SUCCESS;
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
CODE_SEG("PAGE")
NTSTATUS
NTAPI
WdfRegisterClassLibrary(
    _In_ PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
    _In_ PUNICODE_STRING SourceString,
    _In_ PUNICODE_STRING ObjectName)
{
    NTSTATUS status;
    PCLASS_MODULE pClassModule;
    PFN_CLASS_LIBRARY_INIT fnClassLibInit;
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
    }
    FxLdrReleaseLoadedModuleLock();
    
    if (!pClassModule)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __DBGPRINT(("Class Library (%p)\n", pClassModule));

    status = ClassOpen(pClassModule, ObjectName);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ClassOpen(%wZ) failed, status 0x%x\n", ObjectName, status));
        ClassRemoveFromLibraryList(pClassModule);
        return status;
    }

    fnClassLibInit = pClassModule->ClassLibraryInfo->ClassLibraryInitialize;
    if (fnClassLibInit)
    {
        status = fnClassLibInit();
        if (NT_SUCCESS(status))
            return status;
        
        __DBGPRINT(("ERROR: WdfRegisterClassLibrary: ClassLibraryInitialize failed, status 0x%x\n", status));
    }

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
        ExFreePoolWithTag(pClassClientModule, WDFLDR_TAG);
        return status;
    }
    
    InterlockedExchangeAdd(&pClassModule->ClientRefCount, 1);
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
        ExFreePoolWithTag(pClassClientModule, WDFLDR_TAG);

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
