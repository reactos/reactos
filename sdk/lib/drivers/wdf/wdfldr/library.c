/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - library functions
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

#include "wdfloader.h"

static
VOID
LibraryFree(
    _In_ PLIBRARY_MODULE LibModule)
{
    if (LibModule->ServicePath.Buffer)
    {
        RtlFreeUnicodeString(&LibModule->ServicePath);
    }

    if (LibModule->ImageName.Buffer)
    {
        RtlFreeUnicodeString(&LibModule->ImageName);
    }

    ExDeleteResourceLite(&LibModule->ClientsListLock);
    ExFreePoolWithTag(LibModule, WDFLDR_TAG);
}

/**
 * @brief      Creates a new KMDF library (wdf01000)
 *
 * @param[in]  LibraryInfo  The library information (version and functons)
 * @param[in]  ServicePath  The registry path for the KMDF driver
 *
 * @return     The Pointer to the created library or NULL.
 */
_Requires_exclusive_lock_held_(WdfLdrGlobals.LoadedModulesListLock)
NTSTATUS
LibraryCreate(
    _In_opt_ PWDF_LIBRARY_INFO LibraryInfo,
    _In_ PCUNICODE_STRING ServicePath,
    _Out_ PLIBRARY_MODULE* OutLibraryModule)
{
    NTSTATUS status;
    PLIBRARY_MODULE pLibModule;

    pLibModule = ExAllocatePoolWithTag(NonPagedPool, sizeof(*pLibModule), WDFLDR_TAG);
    
    if (pLibModule == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *pLibModule = (LIBRARY_MODULE){
        .LibraryRefCount = 1,
        .ImplicitlyLoaded = (_Bool)LibraryInfo,
        .IsBootDriver = ServiceCheckBootStart((PUNICODE_STRING)ServicePath),
        .LoaderThread = KeGetCurrentThread(),
    };

    InitializeListHead(&pLibModule->ClientsListHead);
    InitializeListHead(&pLibModule->ClassListHead);
    ExInitializeResourceLite(&pLibModule->ClientsListLock);
    KeInitializeEvent(&pLibModule->LoaderEvent, SynchronizationEvent, FALSE);

    if (LibraryInfo)
    {
        pLibModule->LibraryInfo = LibraryInfo;
        pLibModule->Version = LibraryInfo->Version;
    }

    // Initialize service's registry path
    pLibModule->ServicePath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                           ServicePath->MaximumLength,
                                                           WDFLDR_TAG);
    if (!pLibModule->ServicePath.Buffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }

    pLibModule->ServicePath.MaximumLength = ServicePath->MaximumLength;
    pLibModule->ServicePath.Length = ServicePath->Length;
    RtlCopyMemory(pLibModule->ServicePath.Buffer, ServicePath->Buffer, ServicePath->MaximumLength);
    
    status = GetImageName(ServicePath, &pLibModule->ImageName);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("LibraryCreate: GetImageName failed with status %x\n", status));
        goto Failure;
    }

    status = GetImageInfo(&pLibModule->ImageName,
                          &pLibModule->ImageAddress,
                          &pLibModule->ImageSize);
    if (NT_SUCCESS(status))
    {
        // Insert into loaded modules list. The LoadedModulesListLock is held here
        InsertHeadList(&WdfLdrGlobals.LoadedModulesList, &pLibModule->LibraryListEntry);
        
        *OutLibraryModule = pLibModule;
        return status;
    }

    __DBGPRINT(("LibraryCreate: GetImageInfo failed with status %x\n", status));

Failure:
    LibraryFree(pLibModule);
    return status;
}

/**
 * @brief Opens KMDF library's driver object by its name and fills some library structure data
 */
NTSTATUS
LibraryOpen(
    _Inout_ PLIBRARY_MODULE LibModule,
    _In_ PCUNICODE_STRING ObjectName)
{
    PDEVICE_OBJECT deviceObject;

    if (ObjectName == NULL)
    {
        return STATUS_SUCCESS;
    }

    NTSTATUS status = IoGetDeviceObjectPointer((PUNICODE_STRING)ObjectName,
                                               FILE_READ_DATA,
                                               &LibModule->LibraryFileObject,
                                               &deviceObject);

    if (NT_SUCCESS(status))
    {
        LibModule->LibraryDriverObject = deviceObject->DriverObject;
    }

    return status;
}

/**
 * @brief Dereferences KMDF library's device object
 */
VOID
LibraryClose(
    _Inout_ PLIBRARY_MODULE LibModule)
{
    if (LibModule->LibraryFileObject != NULL)
    {
        ObDereferenceObject(LibModule->LibraryFileObject); 
        LibModule->LibraryFileObject = NULL;
    }
}

NTSTATUS
LibraryFindOrLoad(
    _In_ PCUNICODE_STRING ServicePath,
    _Out_ PLIBRARY_MODULE* LibModule)
{
    BOOLEAN loadNeeded = FALSE;
    NTSTATUS status;

    FxLdrAcquireLoadedModuleLock();
    PLIBRARY_MODULE pLibModule = FindLibraryByServicePathLocked(ServicePath);

    if (pLibModule == NULL)
    {
        // Client driver is loaded before the library, we need to create it here
        status = LibraryCreate(NULL, ServicePath, &pLibModule);
        if (NT_SUCCESS(status))
        {
            loadNeeded = TRUE;
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    // Reference the library
    if (pLibModule)
        InterlockedIncrement(&pLibModule->LibraryRefCount);

    FxLdrReleaseLoadedModuleLock();

    // We've just created a library, time to load the actual driver
    if (loadNeeded)
    {
        // This will call WdfRegisterLibrary (as part of KMDF's DriverEntry)
        status = ZwLoadDriver((PUNICODE_STRING)ServicePath);
        if (NT_SUCCESS(status) && !pLibModule->LibraryInfo)
        {
            __DBGPRINT(("ZwLoadDriver(%wZ) succeeded but Libray information was returned\n",
                        ServicePath));
            status = STATUS_NOT_SAFE_MODE_DRIVER;
        }
        else if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("ZwLoadDriver(%wZ) failed with status %x\n", ServicePath, status));
        }

        if (!NT_SUCCESS(status))
        {
            LibraryDereference(pLibModule);
            // Cleanup created library
            ASSERT(pLibModule->LibraryInfo == NULL);
            LibraryDereference(pLibModule);
            pLibModule = NULL;
        }
    }

    *LibModule = pLibModule;
    return status;
}

VOID
LibraryReference(
    _In_ PLIBRARY_MODULE LibModule)
{
    FxLdrAcquireLoadedModuleLock();
    InterlockedIncrement(&LibModule->LibraryRefCount);
    FxLdrReleaseLoadedModuleLock();
}

VOID
LibraryDereference(
    _In_ PLIBRARY_MODULE LibModule)
{
    BOOLEAN cleanupNeeded = FALSE;

    FxLdrAcquireLoadedModuleLock();
    if (InterlockedDecrement(&LibModule->LibraryRefCount) == 0)
    {
        if (!IsListEmpty(&LibModule->LibraryListEntry))
        {
            RemoveEntryList(&LibModule->LibraryListEntry);
            InitializeListHead(&LibModule->LibraryListEntry); // Indicate removal process
            cleanupNeeded = TRUE;
        }
    }
    FxLdrReleaseLoadedModuleLock();

    if (cleanupNeeded)
    {
        if (LibModule->LibraryInfo)
            LibraryUnload(LibModule);
        else
            LibraryClose(LibModule);
        LibraryFree(LibModule);
    }
}

VOID
LibraryUnload(
    _In_ PLIBRARY_MODULE LibModule)
{
    NTSTATUS status;

    // TODO: Reference wdfldr.sys has this check. Research the reason
    // if (LibModule->IsBootDriver)
    //     return;
    
    ASSERT(LibModule->LibraryInfo);

    status = LibModule->LibraryInfo->LibraryDecommission();
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("LibraryDecommission failed %X\n", status));
    }

    LibModule->LibraryInfo = NULL;
    
    __DBGPRINT(("Unloading library %wZ\n", &LibModule->ServicePath));

    LibraryClose(LibModule);

    status = ZwUnloadDriver(&LibModule->ServicePath);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("unload of %wZ returned 0x%x (this may not be a true error if someone else attempted to stop"
            " the service first)\n",
            &LibModule->ServicePath,
            status));
    }

    // That should free the library entry
    LibraryDereference(LibModule);
}

BOOLEAN
LibraryAcquireClientLock(
    _In_ PLIBRARY_MODULE LibModule)
{
    KeEnterCriticalRegion();
    return ExAcquireResourceExclusiveLite(&LibModule->ClientsListLock, TRUE);
}

VOID
LibraryReleaseClientLock(
    _In_ PLIBRARY_MODULE LibModule)
{
    ExReleaseResourceLite(&LibModule->ClientsListLock);
    KeLeaveCriticalRegion();
}

static
VOID
ClientFree(
    _In_ PCLIENT_MODULE ClientModule)
{
    if (ClientModule->ImageName.Buffer)
    {
        RtlFreeUnicodeString(&ClientModule->ImageName);
    }

    ExFreePoolWithTag(ClientModule, WDFLDR_TAG);
}

/********************************************
 * 
 * Create client module and add it to client list in library
 * 
 * Params:
 *    LibModule - library that client being added
 *    DriverServiceName - client driver service registry path
 *    BindInfo - bind information
 *    Context - 
 *    ClientModule - client added to library clients list
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
LibraryLinkInClient(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING ServicePath,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PVOID Context,
    _Out_ PCLIENT_MODULE* OutClientModule)
{
    PCLIENT_MODULE clientModule;
    NTSTATUS status;

    clientModule = ExAllocatePoolWithTag(NonPagedPool, sizeof(*clientModule), WDFLDR_TAG);

    if (clientModule == NULL)
    {
        __DBGPRINT(("ERROR: ExAllocatePoolWithTag failed\n"));
        __DBGPRINT(("ERROR: Client module NOT linked\n"));
        
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *clientModule = (CLIENT_MODULE){
        .Context = Context,
        .Info = BindInfo
    };

    InitializeListHead(&clientModule->ClassListHead);
    InitializeListHead(&clientModule->LibListEntry);

    status = GetImageName(ServicePath, &clientModule->ImageName);
    if (NT_SUCCESS(status))
    {
        __DBGPRINT(("Client Image Name: %wZ\n", &clientModule->ImageName));

        status = GetImageInfo(&clientModule->ImageName,
                              &clientModule->ImageAddr,
                              &clientModule->ImageSize);
        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("GetImageBase failed with status 0x%x\n", status));            
            __DBGPRINT(("ERROR: Client module NOT linked\n"));
            ClientFree(clientModule);

            return status;
        }
    }

    LibraryAcquireClientLock(LibModule);
    InsertHeadList(&LibModule->ClientsListHead, &clientModule->LibListEntry);
    InterlockedIncrement(&LibModule->ClientRefCount);
    LibraryReleaseClientLock(LibModule);

    *OutClientModule = clientModule;
    return STATUS_SUCCESS;
}

static
VOID
LibraryClientDereference(
    _In_ PLIBRARY_MODULE LibModule)
{
    LONG refs = InterlockedDecrement(&LibModule->ClientRefCount);

    __DBGPRINT(("Dereference module %wZ, %d references left\n", &LibModule->ServicePath, refs));

    if (refs <= 0)
    {
        LibraryUnload(LibModule);
    }
}

BOOLEAN
LibraryUnlinkClient(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PWDF_BIND_INFO BindInfo)
{
    LibraryAcquireClientLock(LibModule);

    // search in library clients entry to remove
    BOOLEAN found = FALSE;
    PCLIENT_MODULE clientModule;
    for (PLIST_ENTRY entry = LibModule->ClientsListHead.Flink;
         entry != &LibModule->ClientsListHead;
         entry = entry->Flink)
    {
        clientModule = CONTAINING_RECORD(entry, CLIENT_MODULE, LibListEntry);
        if (clientModule->Info == BindInfo)
        {
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        RemoveEntryList(&clientModule->LibListEntry);
        LibraryClientDereference(LibModule);
    }

    LibraryReleaseClientLock(LibModule);

    if (found)
    {
        InitializeListHead(&clientModule->LibListEntry);
        ClientFree(clientModule);
        return TRUE;
    }

    __DBGPRINT(("ERROR: Client module %p, bind %p NOT found\n", LibModule, BindInfo));
    return FALSE;
}

/**
 * @brief Finds a library module in WdfLdrGlobals.LoadedModulesList
 *        by service path in the registry. Compares only the service name part
 */
_Requires_lock_held_(WdfLdrGlobals.LoadedModulesListLock)
PLIBRARY_MODULE
FindLibraryByServicePathLocked(
    _In_ PCUNICODE_STRING ServicePath)
{
    PLIBRARY_MODULE foundModule = NULL;
    UNICODE_STRING needleName;

    GetNameFromPath(ServicePath, &needleName);
    
    for (PLIST_ENTRY entry = WdfLdrGlobals.LoadedModulesList.Flink;
         entry != &WdfLdrGlobals.LoadedModulesList;
         entry = entry->Flink)
    {
        UNICODE_STRING haystackName;
        PLIBRARY_MODULE currentLib = CONTAINING_RECORD(entry, LIBRARY_MODULE, LibraryListEntry);
        GetNameFromPath(&currentLib->ServicePath, &haystackName);

        if (RtlEqualUnicodeString(&needleName, &haystackName, TRUE))
        {
            foundModule = currentLib;
            break;
        }
    }

    return foundModule;
}
