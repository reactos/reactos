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
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING ObjectName)
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
    _In_ PLIBRARY_MODULE LibModule)
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
    _In_ PWDF_LIBRARY_INFO LibInfo,
    _In_ PUNICODE_STRING DriverServiceName)
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
        __DBGPRINT(("GetImageBase failed\n"));
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
    _In_ PLIBRARY_MODULE LibModule,
    _Inout_ PWDF_LIBRARY_INFO LibInfo)
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
    _In_ PLIBRARY_MODULE LibModule)
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

// TODO: move to aux_klib.lib
NTSTATUS
NTAPI
AuxKlibInitialize()
{
    NTSTATUS status;
    RTL_OSVERSIONINFOW osVersion;
    UNICODE_STRING strRtlQueryModuleInformation = RTL_CONSTANT_STRING(L"RtlQueryModuleInformation");

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
    _In_ PLIBRARY_MODULE LibModule)
{
    PLIST_ENTRY result;

    result = &LibModule->LibraryListEntry;
    
    InsertHeadList(&gLibList, result);
        
    return result;
}


VOID
NTAPI
LibraryRemoveFromLibraryList(
    _In_ PLIBRARY_MODULE LibModule)
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
    _In_ PCLIENT_MODULE ClientModule)
{
    if (ClientModule->ImageName.Buffer != NULL)
    {
        __DBGPRINT(("Client Image Name: %wZ\n", &ClientModule->ImageName));

        ExFreePoolWithTag(ClientModule->ImageName.Buffer, WDFLDR_TAG);
        ClientModule->ImageName.Length = 0;
        ClientModule->ImageName.Buffer = NULL;
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
NTAPI
LibraryLinkInClient(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING DriverServiceName,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PVOID Context,
    _Out_ PCLIENT_MODULE* ClientModule)
{
    PCLIENT_MODULE pClientModule;
    NTSTATUS status;

    *ClientModule = NULL;
    pClientModule = ExAllocatePoolWithTag(NonPagedPool, sizeof(CLIENT_MODULE), WDFLDR_TAG);

    if (pClientModule == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;        

        __DBGPRINT(("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", status));

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
        __DBGPRINT(("Client Image Name: %wZ\n", &pClientModule->ImageName));

        status = GetImageBase(&pClientModule->ImageName, &pClientModule->ImageAddr, &pClientModule->ImageSize);

        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("GetImageBase failed with status 0x%x\n", status));

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
    __DBGPRINT(("ERROR: Client module NOT linked\n"));

    return status;
}


VOID
NTAPI
LibraryUnload(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ BOOLEAN RemoveFromList)
{
    PWDF_LIBRARY_INFO LibInfo;
    NTSTATUS status;

    if (LibModule->IsBootDriver == TRUE)
        return;

    LibInfo = LibModule->LibraryInfo;
    
    if (LibInfo)
    {
        status = LibInfo->LibraryDecommission();

        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("LibraryDecommission failed %08X\n", status));
        }
    }
    
    __DBGPRINT(("Unload module %wZ\n", &LibModule->Service));

    LibraryClose(LibModule);
    status = ZwUnloadDriver(&LibModule->Service);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("unload of %wZ returned 0x%x (this may not be a true error if someone else attempted to stop"
            " the service first)\n",
            &LibModule->Service,
            status));
    }

    if (RemoveFromList)
        LibraryRemoveFromLibraryList(LibModule);
}


VOID
NTAPI
LibraryReleaseClientReference(
    _In_ PLIBRARY_MODULE LibModule)
{
    int refs;

    __DBGPRINT(("Dereference module %wZ\n", &LibModule->Service));

    refs = _InterlockedDecrement(&LibModule->ClientRefCount);
    
    // unload library if hasn't clients
    if (refs <= 0)
    {
        LibraryUnload(LibModule, TRUE);
    }
    else
    {
        __DBGPRINT(("Dereference module %wZ still has %d references\n",
            &LibModule->Service,
            refs));
    }
}


NTSTATUS
NTAPI
LibraryUnlinkClient(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PWDF_BIND_INFO BindInfo)
{
    BOOLEAN isBindFound;
    PCLIENT_MODULE pClientModule;
    PLIST_ENTRY entry;
    NTSTATUS status;

    isBindFound = FALSE;
    pClientModule = NULL;
    LibraryAcquireClientLock(LibModule);

    // search in library clients entry to remove
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
        // remove client from list and release resources
        RemoveEntryList(entry);
        InitializeListHead(entry);
        status = STATUS_SUCCESS;
        ClientCleanupAndFree(pClientModule);
    }
    else
    {
        status = STATUS_UNSUCCESSFUL;
        __DBGPRINT(("ERROR: Client module %p, bind %p NOT found\n", LibModule, BindInfo));
    }

    return status;
}
