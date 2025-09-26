/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - library functions
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 *              Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#include "wdfloader.h"

VOID
LibraryFree(
    _In_ PLIBRARY_MODULE LibModule)
{
    DPRINT_TRACE_ENTRY();

    if (!LibModule)
    {
        DPRINT_ERROR(("LibModule is NULL\n"));
        return;
    }

    if (LibModule->ServicePath.Buffer)
    {
        RtlFreeUnicodeString(&LibModule->ServicePath);
        LibModule->ServicePath.Buffer = NULL;
    }

    if (LibModule->ImageName.Buffer)
    {
        RtlFreeUnicodeString(&LibModule->ImageName);
        LibModule->ImageName.Buffer = NULL;
    }

    ExDeleteResourceLite(&LibModule->ClientsListLock);
    RtlZeroMemory(LibModule, sizeof(*LibModule));
    ExFreePoolWithTag(LibModule, WDFLDR_TAG);

    DPRINT_TRACE_EXIT();
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
    PLIBRARY_MODULE LibModule = NULL;

    DPRINT_TRACE_ENTRY();
    if (!ServicePath || !OutLibraryModule)
    {
        DPRINT_ERROR(("Invalid parameters: ServicePath=%p, OutLibraryModule=%p\n",
                     ServicePath, OutLibraryModule));
        return STATUS_INVALID_PARAMETER;
    }

    *OutLibraryModule = NULL;

    LibModule = ExAllocatePoolZero(NonPagedPool, sizeof(*LibModule), WDFLDR_TAG);
    if (LibModule == NULL)
    {
        DPRINT_ERROR(("Failed to allocate library module structure\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LibModule->LibraryRefCount = 1;
    LibModule->ImplicitlyLoaded = (LibraryInfo == NULL);
    LibModule->IsBootDriver = ServiceCheckBootStart((PUNICODE_STRING)ServicePath);
    LibModule->LoaderThread = KeGetCurrentThread();

    InitializeListHead(&LibModule->ClientsListHead);
    InitializeListHead(&LibModule->ClassListHead);

    status = ExInitializeResourceLite(&LibModule->ClientsListLock);
    if (!NT_SUCCESS(status))
    {
        DPRINT_ERROR(("ExInitializeResourceLite failed with status 0x%x\n", status));
        goto Failure;
    }

    KeInitializeEvent(&LibModule->LoaderEvent, SynchronizationEvent, FALSE);

    /* Only set library information if provided */
    if (LibraryInfo)
    {
        LibModule->LibraryInfo = LibraryInfo;
        LibModule->Version = LibraryInfo->Version;
        DPRINT_VERBOSE(("Library info provided: Version %d.%d.%d\n",
                       LibraryInfo->Version.Major,
                       LibraryInfo->Version.Minor,
                       LibraryInfo->Version.Build));
    }

    LibModule->ServicePath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                           ServicePath->MaximumLength,
                                                           WDFLDR_TAG);
    if (!LibModule->ServicePath.Buffer)
    {
        DPRINT_ERROR(("Failed to allocate service path buffer\n"));
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }

    LibModule->ServicePath.MaximumLength = ServicePath->MaximumLength;
    LibModule->ServicePath.Length = ServicePath->Length;
    RtlCopyMemory(LibModule->ServicePath.Buffer, ServicePath->Buffer, ServicePath->Length);

    status = GetImageName(ServicePath, &LibModule->ImageName);
    if (!NT_SUCCESS(status))
    {
        DPRINT_ERROR(("Failed to get image name from service path\n"));
        goto Failure;
    }

    status = GetImageInfo(&LibModule->ImageName,
                          &LibModule->ImageAddress,
                          &LibModule->ImageSize);
    if (NT_SUCCESS(status))
    {
        // Insert into loaded modules list. The LoadedModulesListLock is held here
        InsertHeadList(&WdfLdrGlobals.LoadedModulesList, &LibModule->LibraryListEntry);

        *OutLibraryModule = LibModule;

        DPRINT_VERBOSE(("Successfully created library module %wZ (Image: %wZ, Base: %p, Size: 0x%x)\n",
               &LibModule->ServicePath,
               &LibModule->ImageName,
               LibModule->ImageAddress,
               LibModule->ImageSize));

        DPRINT_TRACE_EXIT();
        return STATUS_SUCCESS;
    }

    DPRINT_ERROR(("GetImageInfo failed with status 0x%x\n", status));

Failure:
    if (LibModule)
    {
        LibraryFree(LibModule);
    }

    DPRINT_TRACE_EXIT();
    return status;
}

/**
 * @brief Opens KMDF library's driver object by its name and fills some library structure data
 *
 * @param LibModule Library module to update
 * @param ObjectName Device object name
 * @return STATUS_SUCCESS on success, error code otherwise
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

/**
 * @brief Release a reference to a library module
 */
VOID
NTAPI
LibraryReleaseReference(
    _In_ PLIBRARY_MODULE LibModule)
{
    LONG refCount;
    DPRINT_TRACE_ENTRY();

    refCount = InterlockedDecrement(&LibModule->LibraryRefCount);
    DPRINT_VERBOSE(("Released reference to library %wZ, RefCount=%d\n",
                   &LibModule->ServicePath, refCount));

    if (refCount <= 0)
    {
        DPRINT(("Library %wZ reference count reached zero, unloading\n", &LibModule->ServicePath));
        LibraryUnload(LibModule);
    }

    DPRINT_TRACE_EXIT();
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

    /* This occurs because of how WDF ClassDrivers behave. */
    if (LibModule->IsBootDriver)
        return;

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

    // Free the library module directly - caller handles the cleanup sequence
    LibraryFree(LibModule);
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

/**
 * @brief Create client module and add it to library client list
 *
 * @param LibModule Library that client is being added to
 * @param ServicePath Client driver service registry path
 * @param BindInfo Bind information
 * @param Context Client context
 * @param OutClientModule Created client module added to library clients list
 * @return STATUS_SUCCESS on success, error code otherwise
 */
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
            __DBGPRINT(("GetImageInfo failed with status 0x%x\n", status));
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

_Requires_lock_held_(WdfLdrGlobals.LoadedModulesListLock)
NTSTATUS
NTAPI
FindModuleByClientService(
    _In_ PUNICODE_STRING RegistryPath,
    _Out_ PLIBRARY_MODULE* Library)
{
    NTSTATUS status;
    UNICODE_STRING imageName = { 0 };
    PLIBRARY_MODULE foundModule = NULL;

    DPRINT_TRACE_ENTRY();

    if (!RegistryPath || !Library)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *Library = NULL;

    status = GetImageName(RegistryPath, &imageName);
    if (!NT_SUCCESS(status))
    {
        if (WdfLdrDiags.DiagFlags & DIAGFLAG_ENABLED)
        {
            DbgPrint("WdfLdr: FindModuleByClientService - ");
            DbgPrint("WdfLdr: FindModuleByClientService: GetImageName for %wZ failed with status 0x%x\n", RegistryPath, status);
        }
        goto Exit;
    }

    /* Search through loaded modules by comparing image names only */
    for (PLIST_ENTRY entry = WdfLdrGlobals.LoadedModulesList.Flink;
         entry != &WdfLdrGlobals.LoadedModulesList;
         entry = entry->Flink)
    {
        PLIBRARY_MODULE currentLib = CONTAINING_RECORD(entry, LIBRARY_MODULE, LibraryListEntry);

        if (RtlEqualUnicodeString(&imageName, &currentLib->ImageName, TRUE))
        {
            foundModule = currentLib;
            break;
        }
    }

    *Library = foundModule;
    status = foundModule ? STATUS_SUCCESS : STATUS_NOT_FOUND;

Exit:
    if (imageName.Buffer)
    {
        RtlFreeUnicodeString(&imageName);
    }

    DPRINT_TRACE_EXIT();
    return status;
}
