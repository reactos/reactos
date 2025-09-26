/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 *              Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#include "wdfloader.h"

//
// WDF Loader Interface GUIDs
//
DEFINE_GUID(GUID_WDF_LOADER_INTERFACE_DIAGNOSTIC, 0x55905BA4, 0x1DD2, 0x45D3, 0xAB, 0xEA, 0xF7, 0xA8, 0x70, 0x11, 0xD6, 0x9F);
DEFINE_GUID(GUID_WDF_LOADER_INTERFACE_CLASS_BIND, 0xFA4838CB, 0x1D08, 0x41E1, 0x8B, 0xA8, 0x71, 0x9C, 0xF8, 0x44, 0xEA, 0x74);

//
// Global State Variables
//
BOOLEAN gAlreadyInitialized = FALSE;
BOOLEAN gAlreadyUnloaded = FALSE;

//
// Diagnostics and Global State
//
WDFLDR_DIAGS WdfLdrDiags = { 0 };
WDF_LDR_GLOBALS WdfLdrGlobals;


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
    UNICODE_STRING verboseLogging = RTL_CONSTANT_STRING(L"VerboseLogging");
    UNICODE_STRING traceEntry = RTL_CONSTANT_STRING(L"TraceEntry");
    ULONG diagValue = 0;

    UNREFERENCED_PARAMETER(RegistryPath);

    if (gAlreadyInitialized)
    {
        return STATUS_SUCCESS;
    }

    gAlreadyInitialized = TRUE;
    RtlZeroMemory(&WdfLdrGlobals, sizeof(WdfLdrGlobals));
    RtlZeroMemory(&WdfLdrDiags, sizeof(WdfLdrDiags));

    InitializeListHead(&WdfLdrGlobals.LoadedModulesList);
    status = ExInitializeResourceLite(&WdfLdrGlobals.LoadedModulesListLock);
    if (!NT_SUCCESS(status))
    {
        DPRINT_ERROR(("ExInitializeResourceLite failed with Status 0x%x\n", status));
        return status;
    }

#if 0
    /* Force debugging everything */
    WdfLdrDiags.DiagFlags = DIAGFLAG_ENABLED | DIAGFLAG_VERBOSE_LOGGING |
                            DIAGFLAG_TRACE_FUNCTION_ENTRY | DIAGFLAG_TRACE_FUNCTION_EXIT |
                            DIAGFLAG_LOG_ERRORS | DIAGFLAG_LOG_WARNINGS;
#endif
    status = WdfLdrDiagnosticsValueByNameAsULONG(&dbgPrintOn, &diagValue);
    if (NT_SUCCESS(status) && diagValue != 0)
    {
        WdfLdrDiags.DiagFlags |= DIAGFLAG_ENABLED;
    }

    status = WdfLdrDiagnosticsValueByNameAsULONG(&verboseLogging, &diagValue);
    if (NT_SUCCESS(status) && diagValue != 0)
    {
        WdfLdrDiags.DiagFlags |= DIAGFLAG_VERBOSE_LOGGING;
    }

    status = WdfLdrDiagnosticsValueByNameAsULONG(&traceEntry, &diagValue);
    if (NT_SUCCESS(status) && diagValue != 0)
    {
        WdfLdrDiags.DiagFlags |= (DIAGFLAG_TRACE_FUNCTION_ENTRY | DIAGFLAG_TRACE_FUNCTION_EXIT);
    }

    status = AuxKlibInitialize();
    if (NT_SUCCESS(status))
    {
        RtlGetVersion((POSVERSIONINFOW)&WdfLdrGlobals.OsVersion);
        DPRINT(("Initialized WdfLdr - OS Version %d.%d.%d\n",
                WdfLdrGlobals.OsVersion.dwMajorVersion,
                WdfLdrGlobals.OsVersion.dwMinorVersion,
                WdfLdrGlobals.OsVersion.dwBuildNumber));
    }
    else
    {
        DPRINT_ERROR(("AuxKlibInitialize failed with Status 0x%x\n", status));
        ExDeleteResourceLite(&WdfLdrGlobals.LoadedModulesListLock);
        gAlreadyInitialized = FALSE;
        return status;
    }


    DPRINT(("WDF Loader initialization completed successfully\n"));
    return STATUS_SUCCESS;
}


VOID
NTAPI
DllUnload(VOID)
{
    if (gAlreadyUnloaded)
    {
        return;
    }

    DPRINT_TRACE_ENTRY();
    gAlreadyUnloaded = TRUE;

    //
    // Unload all loaded libraries - CRITICAL: proper cleanup sequence
    //
    while (!IsListEmpty(&WdfLdrGlobals.LoadedModulesList))
    {
        PLIST_ENTRY entry = RemoveHeadList(&WdfLdrGlobals.LoadedModulesList);
        PLIBRARY_MODULE module = CONTAINING_RECORD(entry, LIBRARY_MODULE, LibraryListEntry);

        DPRINT(("Unloading module %p (%wZ)\n", module, &module->ServicePath));

        InitializeListHead(&module->LibraryListEntry);
        LibraryUnloadClasses(module);

        if (module->LibraryInfo)
        {
            LibraryUnload(module);
        }
        else
        {
            LibraryClose(module);
            LibraryFree(module);
        }
    }

    ExDeleteResourceLite(&WdfLdrGlobals.LoadedModulesListLock);
    DPRINT_TRACE_EXIT();
}

NTSTATUS
NTAPI
WdfLdrQueryInterface(
    _In_ PWDF_INTERFACE_HEADER LoaderInterface)
{
    DPRINT_TRACE_ENTRY();

    if (LoaderInterface == NULL)
    {
        DPRINT_ERROR(("LoaderInterface is NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    if (LoaderInterface->InterfaceType == NULL)
    {
        DPRINT_ERROR(("InterfaceType is NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    if (IsEqualGUID(LoaderInterface->InterfaceType, &GUID_WDF_LOADER_INTERFACE_STANDARD))
    {
        if (LoaderInterface->InterfaceSize < sizeof(WDF_LOADER_INTERFACE))
        {
            DPRINT_ERROR(("Interface size too small: %u, expected: %u\n",
                         LoaderInterface->InterfaceSize, sizeof(WDF_LOADER_INTERFACE)));
            return STATUS_INVALID_PARAMETER;
        }

        PWDF_LOADER_INTERFACE lInterface = (PWDF_LOADER_INTERFACE)LoaderInterface;

        lInterface->RegisterLibrary = WdfRegisterLibrary;
        lInterface->VersionBind = WdfVersionBind;
        lInterface->VersionUnbind = WdfVersionUnbind;
        lInterface->DiagnosticsValueByNameAsULONG = WdfLdrDiagnosticsValueByNameAsULONG;

        DPRINT_VERBOSE(("Provided standard WDF loader interface\n"));
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUID(LoaderInterface->InterfaceType, &GUID_WDF_LOADER_INTERFACE_DIAGNOSTIC))
    {
        if (LoaderInterface->InterfaceSize < sizeof(WDF_LOADER_INTERFACE_DIAGNOSTIC))
        {
            DPRINT_ERROR(("Diagnostic interface size too small: %u, expected: %u\n",
                         LoaderInterface->InterfaceSize, sizeof(WDF_LOADER_INTERFACE_DIAGNOSTIC)));
            return STATUS_INVALID_PARAMETER;
        }

        PWDF_LOADER_INTERFACE_DIAGNOSTIC lInterface =
            (PWDF_LOADER_INTERFACE_DIAGNOSTIC)LoaderInterface;

        lInterface->DiagnosticsValueByNameAsULONG = WdfLdrDiagnosticsValueByNameAsULONG;

        DPRINT_VERBOSE(("Provided diagnostic interface\n"));
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUID(LoaderInterface->InterfaceType, &GUID_WDF_LOADER_INTERFACE_CLASS_BIND))
    {
        if (LoaderInterface->InterfaceSize < sizeof(WDF_LOADER_INTERFACE_CLASS_BIND))
        {
            DPRINT_ERROR(("Class bind interface size too small: %u, expected: %u\n",
                         LoaderInterface->InterfaceSize, sizeof(WDF_LOADER_INTERFACE_CLASS_BIND)));

            return STATUS_INVALID_PARAMETER;
        }

        PWDF_LOADER_INTERFACE_CLASS_BIND lInterface =
            (PWDF_LOADER_INTERFACE_CLASS_BIND)LoaderInterface;

        lInterface->ClassBind = WdfVersionBindClass;
        lInterface->ClassUnbind = WdfVersionUnbindClass;

        DPRINT_VERBOSE(("Provided class bind interface\n"));
        return STATUS_SUCCESS;
    }

    DPRINT_ERROR(("Unknown interface type requested\n"));
    DPRINT_TRACE_EXIT();
    return STATUS_NOINTERFACE;
}

/**
 * @brief Register wdf01000 library
 *
 * @param LibraryInfo Information about the library being registered
 * @param ServicePath Service path in registry
 * @param LibraryDeviceName KMDF device name
 * @return STATUS_SUCCESS on success, error code otherwise
 */
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

        ASSERT(pLibModule->ImplicitlyLoaded == TRUE);
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

/**
 * @brief Bind client driver with framework
 *
 * @param DriverObject Driver object
 * @param ServicePath Registry service path
 * @param BindInfo Client driver bind information
 * @param ComponentGlobals Client driver global settings
 * @return STATUS_SUCCESS on success, error code otherwise
 */
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
    PLIBRARY_MODULE pLibModule = NULL;
    PCLIENT_MODULE clientModule = NULL;
    UNICODE_STRING libraryServicePath = { 0 };
    CLIENT_INFO clientInfo;
    PVOID context = NULL;

    UNREFERENCED_PARAMETER(DriverObject);
    PAGED_CODE();

    DPRINT_TRACE_ENTRY();

    if (ComponentGlobals == NULL || BindInfo == NULL || BindInfo->FuncTable == NULL)
    {
        DPRINT_ERROR(("Invalid parameters: ComponentGlobals=%p, BindInfo=%p\n",
                     ComponentGlobals, BindInfo));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    *ComponentGlobals = NULL;

    status = ReferenceVersion(BindInfo, &pLibModule);
    if (!NT_SUCCESS(status))
    {
        DPRINT_ERROR(("ReferenceVersion failed with status 0x%x\n", status));
        goto Exit;
    }

    DPRINT_VERBOSE(("Referenced library version for %wZ\n", &pLibModule->ServicePath));


    clientInfo.Size = sizeof(CLIENT_INFO);
    clientInfo.RegistryPath = ServicePath;

    status = LibraryLinkInClient(pLibModule, ServicePath, BindInfo, &clientInfo, &clientModule);
    if (!NT_SUCCESS(status))
    {
        DPRINT_ERROR(("LibraryLinkInClient failed with status 0x%x\n", status));
        LibraryDereference(pLibModule);
        goto Exit;
    }

    /* Dereference the library as LibraryLinkInClient has its own reference */
    LibraryDereference(pLibModule);

    BindInfo->Module = pLibModule;

    if (!pLibModule->LibraryInfo)
    {
        DPRINT_ERROR(("Library not properly initialized - LibraryInfo is NULL\n"));
        status = STATUS_INVALID_DEVICE_STATE;
        goto Cleanup;
    }

    if (!pLibModule->LibraryInfo->LibraryRegisterClient)
    {
        DPRINT_ERROR(("Library missing LibraryRegisterClient function pointer\n"));
        status = STATUS_INVALID_DEVICE_STATE;
        goto Cleanup;
    }

    DPRINT_VERBOSE(("Calling LibraryRegisterClient at %p for library %wZ\n",
                   pLibModule->LibraryInfo->LibraryRegisterClient, &pLibModule->ServicePath));

    status = pLibModule->LibraryInfo->LibraryRegisterClient(BindInfo, ComponentGlobals, &context);
    if (NT_SUCCESS(status))
    {
        clientModule->Globals = *ComponentGlobals;
        clientModule->Context = context; // Pointer to FX_DRIVER_GLOBALS

        DPRINT(("Successfully bound client %wZ to library %wZ\n",
               ServicePath, &libraryServicePath));
        goto Exit;
    }

    DPRINT_ERROR(("LibraryRegisterClient failed with status 0x%x\n", status));

Cleanup:
    WdfVersionUnbind(ServicePath, BindInfo, *ComponentGlobals);

Exit:
    if (libraryServicePath.Buffer)
    {
        RtlFreeUnicodeString(&libraryServicePath);
    }

    DPRINT_TRACE_EXIT();
    return status;
}

/**
 * @brief Unbind client driver from framework
 *
 * @param RegistryPath Registry path
 * @param BindInfo Client driver bind information
 * @param ComponentGlobals Client driver global settings
 * @return STATUS_SUCCESS on success, error code otherwise
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
WdfVersionUnbind(
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS ComponentGlobals)
{
    PLIBRARY_MODULE pLibModule;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS unregisterStatus = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT_TRACE_ENTRY();

    if (!BindInfo || !BindInfo->Module)
    {
        DPRINT_ERROR(("Invalid BindInfo or Module is NULL\n"));
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    pLibModule = BindInfo->Module;

    /* Reference the module while working on it and unregister if globals are valid. */
    LibraryReference(pLibModule);
    if (ComponentGlobals != NULL && pLibModule->LibraryInfo &&
        pLibModule->LibraryInfo->LibraryUnregisterClient)
    {
        unregisterStatus = pLibModule->LibraryInfo->LibraryUnregisterClient(BindInfo, ComponentGlobals);
        if (!NT_SUCCESS(unregisterStatus))
        {
            DPRINT_ERROR(("LibraryUnregisterClient failed with status 0x%x\n", unregisterStatus));
        }
    }

    if (!LibraryUnlinkClient(pLibModule, BindInfo))
    {
        DPRINT_ERROR(("LibraryUnlinkClient failed for BindInfo %p\n", BindInfo));
    }

    LibraryDereference(pLibModule);
    BindInfo->Module = NULL;

    DPRINT_VERBOSE(("Successfully unbound client %wZ\n", RegistryPath));

Exit:
    DPRINT_TRACE_EXIT();
    return NT_SUCCESS(unregisterStatus) ? status : unregisterStatus;
}

/**
 * @brief Reference a WDF library version
 *
 * @param Info Binding information containing version details
 * @param Module Pointer to receive the library module
 * @return NTSTATUS Success or failure status
 */
NTSTATUS
NTAPI
ReferenceVersion(
    _In_ PWDF_BIND_INFO Info,
    _Out_ PLIBRARY_MODULE* Module)
{
    NTSTATUS status;
    UNICODE_STRING libraryServicePath = { 0 };

    DPRINT_TRACE_ENTRY();

    if (!Info || !Module)
    {
        DPRINT_ERROR(("Invalid parameters: Info=%p, Module=%p\n", Info, Module));
        return STATUS_INVALID_PARAMETER;
    }

    *Module = NULL;
    /* Multiple ways to find the correct library - once again UCX stresses this extensively */
    status = GetVersionServicePath(Info, &libraryServicePath);
    if (!NT_SUCCESS(status))
    {
        DPRINT_ERROR(("GetVersionServicePath failed with status 0x%x\n", status));
        goto Exit;
    }

    status = LibraryFindOrLoad(&libraryServicePath, Module);
    if (NT_SUCCESS(status))
    {
        Info->Module = *Module;
        DPRINT_VERBOSE(("Referenced version for library %wZ\n", &libraryServicePath));
    }
    else
    {
        DPRINT_ERROR(("LibraryFindOrLoad failed with status 0x%x\n", status));
    }

Exit:
    if (libraryServicePath.Buffer)
    {
        RtlFreeUnicodeString(&libraryServicePath);
    }

    DPRINT_TRACE_EXIT();
    return status;
}

/**
 * @brief Dereference a WDF library version
 *
 * @param Info Binding information
 * @param Globals Component globals to clean up
 * @return NTSTATUS Success or failure status
 */
NTSTATUS
NTAPI
DereferenceVersion(
    _In_ PWDF_BIND_INFO Info,
    _In_opt_ PWDF_COMPONENT_GLOBALS Globals)
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS unregisterStatus = STATUS_SUCCESS;
    PLIBRARY_MODULE pLibModule;

    DPRINT_TRACE_ENTRY();

    if (!Info || !Info->Module)
    {
        DPRINT_ERROR(("Invalid Info or Module is NULL\n"));
        return STATUS_INVALID_PARAMETER;
    }

    pLibModule = Info->Module;

    // Unregister the client if we have globals
    if (Globals && pLibModule->LibraryInfo &&
        pLibModule->LibraryInfo->LibraryUnregisterClient)
    {
        unregisterStatus = pLibModule->LibraryInfo->LibraryUnregisterClient(Info, Globals);
        if (!NT_SUCCESS(unregisterStatus))
        {
            DPRINT_ERROR(("LibraryUnregisterClient failed with status 0x%x\n", unregisterStatus));
            if (WdfLdrDiags.DiagFlags & DIAGFLAG_ENABLED)
            {
                DPRINT_VERBOSE(("LibraryUnregisterClient failed\n"));
            }
        }
    }

    // Unlink the client
    if (!LibraryUnlinkClient(pLibModule, Info))
    {
        DPRINT_ERROR(("LibraryUnlinkClient failed\n"));
        if (WdfLdrDiags.DiagFlags & DIAGFLAG_ENABLED)
        {
            DPRINT_VERBOSE(("LibraryUnlinkClient failed\n"));
        }
    }

    // Release the reference
    LibraryReleaseReference(pLibModule);

    // Clear the module reference
    Info->Module = NULL;

    DPRINT_TRACE_EXIT();
    return NT_SUCCESS(unregisterStatus) ? status : unregisterStatus;
}

/**
 * @brief Register class extension library (e.g., UCX)
 *
 * @see http://redplait.blogspot.com/2013/03/ucxfunctionsidc.html
 *
 * @param ClassLibInfo Class library information
 * @param SourceString Service name of the class library
 * @param ObjectName Device object name
 * @return STATUS_SUCCESS on success, error code otherwise
 */
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

    // First, try to find an existing class module
    pClassModule = FindClassByServiceNameLocked(SourceString, &libModule);

    if (pClassModule)
    {
        /* Another behavior you can observe by running UCX */
        pClassModule->ClassLibraryInfo = ClassLibInfo;
    }
    else
    {
        status = FindModuleByClientService(SourceString, &libModule);
        if (!NT_SUCCESS(status))
        {
            if (WdfLdrDiags.DiagFlags & DIAGFLAG_ENABLED)
            {
                DPRINT_VERBOSE(("No library found for class module %wZ, status 0x%x\n",
                               SourceString, status));
            }

            /*
             * This ensures class modules are always searchable through library class lists
             * The behavior here can be observed from UCX
             */
            if (!IsListEmpty(&WdfLdrGlobals.LoadedModulesList))
            {
                PLIST_ENTRY entry = WdfLdrGlobals.LoadedModulesList.Flink;
                libModule = CONTAINING_RECORD(entry, LIBRARY_MODULE, LibraryListEntry);
                DPRINT(("Using default WDF library %wZ for class module %wZ\n",
                       &libModule->ServicePath, SourceString));
            }
            else
            {
                libModule = NULL;
            }
        }

        pClassModule = ClassCreate(ClassLibInfo, libModule, SourceString);
        if (pClassModule)
        {
            if (libModule)
                LibraryAddToClassListLocked(libModule, pClassModule);
            ClassAddReference(pClassModule);
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
        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("ERROR: WdfRegisterClassLibrary: ClassLibraryInitialize failed, status 0x%x\n", status));
            pClassModule->ClassLibraryInfo = NULL;
            ClassClose(pClassModule);
        }
    }

    /* Classes always stay loaded after registration - (UCX, NDIS) */
    return status;
}

/*
 * http://redplait.blogspot.com/2013/03/ucxfunctionsidc.html
 */
NTSTATUS
NTAPI
WdfVersionBindClass(
    _In_ PWDF_BIND_INFO BindInfo,
    _Inout_ PWDF_COMPONENT_GLOBALS* ClientGlobals,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo)
{
    PCLASS_CLIENT_MODULE pClassClientModule = NULL;
    NTSTATUS status;
    PCLASS_MODULE pClassModule = NULL;

    DPRINT_TRACE_ENTRY();

    pClassClientModule = ClassClientCreate();
    if (pClassClientModule == NULL)
    {
        DPRINT_ERROR(("Could not create class client struct\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ReferenceClassVersion(ClassBindInfo, BindInfo, &pClassModule);
    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(pClassClientModule, WDFLDR_TAG);
        return status;
    }

    status = ClassLinkInClient(pClassModule, ClassBindInfo, BindInfo, pClassClientModule);
    if (!NT_SUCCESS(status))
    {
        DPRINT_ERROR(("ClassLinkInClient failed 0x%x\n", status));
    }
    else
    {
        // Success - clear pClassClientModule so we don't free it
        pClassClientModule = NULL;
        status = pClassModule->ClassLibraryInfo->ClassLibraryBindClient(ClassBindInfo, ClientGlobals);
        if (NT_SUCCESS(status))
        {
            DPRINT_TRACE_EXIT();
            return status;
        }

        DPRINT_ERROR(("ClassLibraryBindClient failed, status 0x%x\n", status));
    }

    if (pClassModule != NULL)
        DereferenceClassVersion(ClassBindInfo, BindInfo, *ClientGlobals);
    if (pClassClientModule != NULL)
        ExFreePoolWithTag(pClassClientModule, WDFLDR_TAG);

    DPRINT_TRACE_EXIT();
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
