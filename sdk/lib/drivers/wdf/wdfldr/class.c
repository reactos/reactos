/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - class functions
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 *              Copyright 2024 Justin Miller <justin.miller@reactos.org>
 */

#include "wdfloader.h"

VOID
ClassAcquireClientLock(
    _In_ PERESOURCE Resource)
{
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(Resource, TRUE);
}

VOID
ClassReleaseClientLock(
    _In_ PERESOURCE Resource)
{
    ExReleaseResourceLite(Resource);
    KeLeaveCriticalRegion();
}

NTSTATUS
ClassOpen(
    _Inout_ PCLASS_MODULE ClassModule,
    _In_ PUNICODE_STRING ObjectName)
{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;

    if (!ObjectName)
        return STATUS_SUCCESS;

    status = IoGetDeviceObjectPointer(ObjectName, FILE_READ_DATA, &ClassModule->ClassFileObject, &deviceObject);

    if (NT_SUCCESS(status))
        ClassModule->ClassDriverObject = deviceObject->DriverObject;

    return status;
}

VOID
ClassClose(
    _In_ PCLASS_MODULE ClassModule)
{
    if (ClassModule->ClassFileObject != NULL)
    {
        ObDereferenceObject(ClassModule->ClassFileObject);
        ClassModule->ClassFileObject = NULL;
    }
}

VOID
ClassCleanupAndFree(
    _In_ PCLASS_MODULE ClassModule)
{
    ClassClose(ClassModule);

    if (ClassModule->Service.Buffer != NULL)
        ExFreePoolWithTag(ClassModule->Service.Buffer, WDFLDR_TAG);

    if (ClassModule->ImageName.Buffer != NULL)
        ExFreePoolWithTag(ClassModule->ImageName.Buffer, WDFLDR_TAG);

    ExDeleteResourceLite(&ClassModule->ClientsListLock);
    ExFreePoolWithTag(ClassModule, WDFLDR_TAG);
}

PCLASS_MODULE
ClassCreate(
    _In_ PWDF_CLASS_LIBRARY_INFO ClassLibInfo,
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PUNICODE_STRING ServiceName)
{
    PCLASS_MODULE pNewClassModule;
    NTSTATUS status;

    pNewClassModule = ExAllocatePoolZero(NonPagedPool, sizeof(*pNewClassModule), WDFLDR_TAG);
    if (pNewClassModule == NULL)
        return NULL;

    if (ClassLibInfo != NULL)
    {
        pNewClassModule->ImplicitlyLoaded = TRUE;
        pNewClassModule->ClassLibraryInfo = ClassLibInfo;
        pNewClassModule->Version.Major = ClassLibInfo->Version.Major;
        pNewClassModule->Version.Minor = ClassLibInfo->Version.Minor;
        pNewClassModule->Version.Build = ClassLibInfo->Version.Build;
    }

    pNewClassModule->ClassRefCount = 1;
    pNewClassModule->Library = LibModule;
    ExInitializeResourceLite(&pNewClassModule->ClientsListLock);
    InitializeListHead(&pNewClassModule->ClientsListHead);
    InitializeListHead(&pNewClassModule->LibraryLinkage);

    pNewClassModule->Service.Buffer = ExAllocatePoolZero(PagedPool, ServiceName->MaximumLength, WDFLDR_TAG);
    if (pNewClassModule->Service.Buffer == NULL)
    {
        goto clean;
    }
    pNewClassModule->Service.MaximumLength = ServiceName->MaximumLength;
    RtlCopyUnicodeString(&pNewClassModule->Service, ServiceName);
    status = GetImageName(ServiceName, &pNewClassModule->ImageName);

    if (!NT_SUCCESS(status))
    {
        goto clean;
    }

    pNewClassModule->IsBootDriver = ServiceCheckBootStart(&pNewClassModule->Service);
    status = GetImageInfo(&pNewClassModule->ImageName,
                          &pNewClassModule->ImageAddress,
                          &pNewClassModule->ImageSize);

    if (NT_SUCCESS(status))
    {
        return pNewClassModule;
    }
    else
    {
        __DBGPRINT(("GetImageBase failed\n"));
    }

clean:
    ClassCleanupAndFree(pNewClassModule);

    return NULL;
}

PCLASS_CLIENT_MODULE
ClassClientCreate()
{
    PCLASS_CLIENT_MODULE pNewClassClientModule;

    pNewClassClientModule = ExAllocatePoolZero(NonPagedPool, sizeof(*pNewClassClientModule), WDFLDR_TAG);
    if (pNewClassClientModule != NULL)
    {
        InitializeListHead(&pNewClassClientModule->ClassLinkage);
        InitializeListHead(&pNewClassClientModule->ClientLinkage);
    }

    return pNewClassClientModule;
}

PCLIENT_MODULE
LibraryFindClientLocked(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PWDF_BIND_INFO BindInfo)
{
    PCLIENT_MODULE pFoundClient;
    PLIST_ENTRY entry;

    for (entry = LibModule->ClientsListHead.Flink; entry != &LibModule->ClientsListHead; entry = entry->Flink)
    {
        pFoundClient = CONTAINING_RECORD(entry, CLIENT_MODULE, LibListEntry);
        if (pFoundClient->Info == BindInfo)
            return pFoundClient;
    }

    return NULL;
}

NTSTATUS
ClassLinkInClient(
    _In_ PCLASS_MODULE ClassModule,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PCLASS_CLIENT_MODULE ClassClientModule)
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
        status = STATUS_INVALID_DEVICE_STATE;
        __DBGPRINT(("ERROR: Could not locate client from Info %p, status 0x%x\n", BindInfo, status));
    }

    return status;
}

NTSTATUS
GetClassRegistryHandle(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PHANDLE KeyHandle)
{
    UNICODE_STRING classVersions;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE rootHandle = NULL;
    HANDLE classVersionsHandle = NULL;
    HANDLE classVersionHandle = NULL;
    NTSTATUS status;
    SIZE_T classNameLength;
    SIZE_T bufferSize;
    UNICODE_STRING objectName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf\\Kmdf");
    DECLARE_UNICODE_STRING_SIZE(versionString, 11);

    classVersions.Length = 0;
    classVersions.Buffer = NULL;
    *KeyHandle = NULL;

    /* Open root registry key */
    InitializeObjectAttributes(&objectAttributes,
                               &objectName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = ZwOpenKey(&rootHandle, KEY_QUERY_VALUE, &objectAttributes);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ZwOpenKey failed for root key, status 0x%08X\n", status));
        goto cleanup;
    }

    classNameLength = wcslen(ClassBindInfo->ClassName);
    bufferSize = (classNameLength * sizeof(WCHAR)) + sizeof(L"\\Versions");

    classVersions.Buffer = ExAllocatePoolWithTag(PagedPool, bufferSize, WDFLDR_TAG);
    if (classVersions.Buffer == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    classVersions.MaximumLength = (USHORT)bufferSize;
    status = RtlUnicodeStringPrintf(&classVersions, L"%s\\Versions", ClassBindInfo->ClassName);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("RtlUnicodeStringPrintf failed, status 0x%08X\n", status));
        goto cleanup;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &classVersions,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               rootHandle,
                               NULL);

    status = ZwOpenKey(&classVersionsHandle, KEY_QUERY_VALUE, &objectAttributes);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ZwOpenKey failed for class versions, status 0x%08X\n", status));
        goto cleanup;
    }

    status = RtlIntegerToUnicodeString(ClassBindInfo->Version.Major, 10, &versionString);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("RtlIntegerToUnicodeString failed for class version, status 0x%08X\n", status));
        goto cleanup;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &versionString,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               classVersionsHandle,
                               NULL);

    status = ZwOpenKey(&classVersionHandle, KEY_QUERY_VALUE, &objectAttributes);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ZwOpenKey failed for class version, status 0x%08X\n", status));
        goto cleanup;
    }

    status = RtlIntegerToUnicodeString(BindInfo->Version.Major, 10, &versionString);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("RtlIntegerToUnicodeString failed for bind version, status 0x%08X\n", status));
        goto cleanup;
    }

    InitializeObjectAttributes(&objectAttributes,
                               &versionString,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               classVersionHandle,
                               NULL);

    status = ZwOpenKey(KeyHandle, KEY_QUERY_VALUE, &objectAttributes);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ZwOpenKey failed for bind version, status 0x%08X\n", status));
        goto cleanup;
    }

cleanup:
    if (classVersionHandle)
        ZwClose(classVersionHandle);
    if (classVersionsHandle)
        ZwClose(classVersionsHandle);
    if (rootHandle)
        ZwClose(rootHandle);

    if (classVersions.Buffer)
    {
        ExFreePoolWithTag(classVersions.Buffer, WDFLDR_TAG);
    }

    return status;
}

NTSTATUS
GetDefaultClassServiceName(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PUNICODE_STRING ServiceName)
{
    SIZE_T classNameLength;
    SIZE_T bufferSize;
    NTSTATUS status;

    /*
     * version numbers are 2 digits each (4 chars)
     */
    classNameLength = wcslen(ClassBindInfo->ClassName);
    bufferSize = (classNameLength * sizeof(WCHAR)) +
                 (sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") +
                  (4 * sizeof(WCHAR)) + sizeof(UNICODE_NULL));

    ServiceName->Buffer = ExAllocatePoolWithTag(PagedPool, bufferSize, WDFLDR_TAG);
    if (ServiceName->Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ServiceName->MaximumLength = (USHORT)bufferSize;
    status = RtlUnicodeStringPrintf(
        ServiceName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%ws%02d%02d",
        ClassBindInfo->ClassName,
        ClassBindInfo->Version.Major,
        BindInfo->Version.Major);

    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(ServiceName->Buffer, WDFLDR_TAG);
    }

    return status;
}

NTSTATUS
GetClassServicePath(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PUNICODE_STRING ServicePath)
{
    NTSTATUS status;
    HANDLE Handle = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyValPartial = NULL;
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"Service");

    ServicePath->Length = 0;
    ServicePath->Buffer = NULL;

    status = GetClassRegistryHandle(ClassBindInfo, BindInfo, &Handle);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: GetClassRegistryHandle failed with status 0x%x\n", status));
    }
    else
    {
        status = FxLdrQueryData(Handle, &ValueName, WDFLDR_TAG, &pKeyValPartial);

        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("ERROR: QueryData failed with status 0x%x\n", status));
        }
        else
        {
            status = BuildServicePath(pKeyValPartial, ServicePath);
        }
    }

    if (!NT_SUCCESS(status))
    {
        status = GetDefaultClassServiceName(ClassBindInfo, BindInfo, ServicePath);

        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("ERROR: GetDefaultClassServiceName failed, status 0x%x\n", status));
        }
    }

    if (Handle)
        ZwClose(Handle);
    if (pKeyValPartial)
        ExFreePoolWithTag(pKeyValPartial, WDFLDR_TAG);

    return status;
}

NTSTATUS
ReferenceClassVersion(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PCLASS_MODULE* ClassModule)
{
    PWDF_BIND_INFO pBindInfo;
    PCLASS_MODULE pClassModule;
    UNICODE_STRING driverServiceName;
    PLIBRARY_MODULE pLibModule;
    BOOLEAN created;
    NTSTATUS status;

    *ClassModule = NULL;
    pBindInfo = BindInfo;
    created = FALSE;
    status = GetClassServicePath(ClassBindInfo, BindInfo, &driverServiceName);

    if (!NT_SUCCESS(status))
    {
        RtlFreeUnicodeString(&driverServiceName);
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
            InterlockedExchangeAdd(&pClassModule->ClassRefCount, 1);
        }
        else
        {
            pClassModule = ClassCreate(NULL, pLibModule, &driverServiceName);

            if (pClassModule)
            {
                InterlockedExchangeAdd(&pClassModule->ClassRefCount, 1);
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

        __DBGPRINT(("class %S bound to library %p,client bound to library %p, status 0x%x\n",
                ClassBindInfo->ClassName,
                pLibModule,
                pBindInfo->Module,
                status));
    }
    FxLdrReleaseLoadedModuleLock();

    if (!created)
    {
        if (!NT_SUCCESS(status))
            goto clean;

        ClassBindInfo->ClassModule = pClassModule;
        *ClassModule = pClassModule;
    }

    /* Class logic: Only call ZwLoadDriver if:
     * 1. Class module was newly created, OR
     * 2. Class module exists but has no ClassLibraryInfo
     */
    if (created || (pClassModule && !pClassModule->ClassLibraryInfo))
    {

        status = ZwLoadDriver(&driverServiceName);
        if (!NT_SUCCESS(status))
        {

            if (status == STATUS_OBJECT_PATH_NOT_FOUND ||
                status == STATUS_OBJECT_NAME_NOT_FOUND ||
                status == STATUS_IMAGE_ALREADY_LOADED)
            {
                /* Driver might already be loaded as boot driver - UCX*/
                status = STATUS_SUCCESS;
            }
        }
        else if (pClassModule && !pClassModule->ClassLibraryInfo)
        {
            status = STATUS_DRIVER_INTERNAL_ERROR;
        }
    }
    else
    {
        /* Class module exists and has ClassLibraryInfo - skip ZwLoadDriver */
        status = STATUS_SUCCESS;
    }

    /* Always set the class module regardless of ZwLoadDriver result (Again.. UCX) */
    if (pClassModule && NT_SUCCESS(status))
    {
        ClassBindInfo->ClassModule = pClassModule;
        *ClassModule = pClassModule;

        InterlockedExchangeAdd(&pClassModule->ClientRefCount, 1);
        goto exit_success;
    }

clean:
    if (pClassModule && InterlockedExchangeAdd(&pClassModule->ClassRefCount, -1) == 1)
        ClassCleanupAndFree(pClassModule);

exit_success:

    RtlFreeUnicodeString(&driverServiceName);

    return status;
}

VOID
ClassUnload(
    _In_ PCLASS_MODULE ClassModule,
    _In_ BOOLEAN RemoveFromList)
{
    PCLASS_MODULE pClassModule;
    PWDF_CLASS_LIBRARY_INFO pClassLibInfo;
    NTSTATUS status;

    pClassModule = ClassModule;
    pClassLibInfo = ClassModule->ClassLibraryInfo;

    if (pClassLibInfo && pClassLibInfo->ClassLibraryDeinitialize)
    {
        __DBGPRINT(("calling ClassLibraryDeinitialize (%p)\n",
                ClassModule->ClassLibraryInfo->ClassLibraryDeinitialize));

        ClassModule->ClassLibraryInfo->ClassLibraryDeinitialize();
    }

    __DBGPRINT(("Unload class library %wZ\n", &ClassModule->Service));

    ClassClose(ClassModule);
    status = ZwUnloadDriver(&ClassModule->Service);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("unload of class %wZ returned 0x%x (this may not be a true error if someone else attempted to "
            "stop the service first)\n",
            &pClassModule->Service,
            status));
    }

    if (RemoveFromList)
        ClassRemoveFromLibraryList(pClassModule);
}

PCLASS_MODULE
FindClassByServiceNameLocked(
    _In_ PUNICODE_STRING ServicePath,
    _Out_ PLIBRARY_MODULE* LibModule)
{
    PLIST_ENTRY libEntry;
    PLIBRARY_MODULE pLibModule;
    PLIST_ENTRY classEntry;
    PCLASS_MODULE pClassModule;
    UNICODE_STRING needleName;

    GetNameFromPath(ServicePath, &needleName);

    if (IsListEmpty(&WdfLdrGlobals.LoadedModulesList))
    {
        goto end;
    }

    for (libEntry = WdfLdrGlobals.LoadedModulesList.Flink;
         libEntry != &WdfLdrGlobals.LoadedModulesList;
         libEntry = libEntry->Flink)
    {
        pLibModule = CONTAINING_RECORD(libEntry, LIBRARY_MODULE, LibraryListEntry);

        for (classEntry = pLibModule->ClassListHead.Flink;
             classEntry != &pLibModule->ClassListHead;
             classEntry = classEntry->Flink)
        {
            UNICODE_STRING haystackName;
            pClassModule = CONTAINING_RECORD(classEntry, CLASS_MODULE, LibraryLinkage);
            GetNameFromPath(&pClassModule->Service, &haystackName);

            if (RtlEqualUnicodeString(&needleName, &haystackName, TRUE))
            {
                if (LibModule != NULL)
                {
                    *LibModule = pLibModule;
                }

                return pClassModule;
            }
        }
    }

end:
    if (LibModule != NULL)
        *LibModule = NULL;

    return NULL;
}

PLIST_ENTRY
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
        return classListHead;

    while (!IsListEmpty(classListHead))
    {
        entry = RemoveHeadList(classListHead);
        InsertTailList(&removedList, entry);
    }

    while (!IsListEmpty(&removedList))
    {
        entry = RemoveHeadList(&removedList);
        InitializeListHead(entry);
        pClassModule = CONTAINING_RECORD(entry, CLASS_MODULE, LibraryLinkage);

        __DBGPRINT(("Unload class library %wZ (%p)\n", &pClassModule->Service, pClassModule));

        ClassUnload(pClassModule, 0);
        if (InterlockedExchangeAdd(&pClassModule->ClassRefCount, -1) == 1)
            ClassCleanupAndFree(pClassModule);
    }

    return classListHead;
}

PLIST_ENTRY
LibraryAddToClassListLocked(
    _In_ PLIBRARY_MODULE LibModule,
    _In_ PCLASS_MODULE ClassModule)
{
    PLIST_ENTRY result;

    if (!LibModule || !ClassModule)
        return NULL;

    result = &ClassModule->LibraryLinkage;
    InsertHeadList(&LibModule->ClassListHead, &ClassModule->LibraryLinkage);

    DPRINT_VERBOSE(("Added class %wZ to library %wZ\n",
                   &ClassModule->Service, &LibModule->ServicePath));

    return result;
}

VOID
ClassRemoveFromLibraryList(
    _In_ PCLASS_MODULE ClassModule)
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
        if (InterlockedExchangeAdd(&ClassModule->ClassRefCount, -1) == 1)
            ClassCleanupAndFree(ClassModule);
    }
}

VOID
ClassUnlinkClient(
    _In_ PCLASS_MODULE ClassModule,
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo)
{
    PCLASS_CLIENT_MODULE client;
    BOOLEAN isUnlinked;
    PLIST_ENTRY entry;

    client = NULL;
    isUnlinked = FALSE;
    ClassAcquireClientLock(&ClassModule->ClientsListLock);

    for (entry = ClassModule->ClientsListHead.Flink;
        entry != &ClassModule->ClientsListHead;
        entry = entry->Flink)
    {
        client = CONTAINING_RECORD(entry, CLASS_CLIENT_MODULE, ClassLinkage);
        if (CONTAINING_RECORD(entry, CLASS_CLIENT_MODULE, ClassLinkage)->ClientClassBindInfo == ClassBindInfo)
        {
            isUnlinked = TRUE;
            break;
        }
    }

    ClassReleaseClientLock(&ClassModule->ClientsListLock);

    if (isUnlinked)
    {
        RemoveEntryList(entry);
        InitializeListHead(entry);
        LibraryAcquireClientLock(ClassModule->Library);
        InitializeListHead(&client->ClientLinkage);
        RemoveEntryList(&client->ClientLinkage);
        LibraryReleaseClientLock(ClassModule->Library);
        ExFreePoolWithTag(client, WDFLDR_TAG);
    }
}

VOID
NTAPI
ClassAddReference(
    _In_ PCLASS_MODULE ClassModule)
{
    InterlockedIncrement(&ClassModule->ClassRefCount);
    DPRINT_VERBOSE(("Added reference to class %wZ, RefCount=%d\n",
                   &ClassModule->Service, ClassModule->ClassRefCount));
}

VOID
ClassReleaseClientReference(
    _In_ PCLASS_MODULE ClassModule)
{
    int refs;

    __DBGPRINT(("Dereference module %wZ\n", &ClassModule->Service));
    refs = InterlockedDecrement(&ClassModule->ClientRefCount);

    if (refs <= 0)
    {
        ClassUnload(ClassModule, TRUE);
    }
    else
    {
        __DBGPRINT(("Dereference module %wZ still has %d references\n",
                   &ClassModule->Service, refs));
    }
}

VOID
DereferenceClassVersion(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _In_ PWDF_COMPONENT_GLOBALS Globals)
{
    PCLASS_MODULE pClassModule;

    UNREFERENCED_PARAMETER(BindInfo);
    pClassModule = ClassBindInfo->ClassModule;

    if (pClassModule)
    {
        if (pClassModule->ClassLibraryInfo &&
            pClassModule->ClassLibraryInfo->ClassLibraryUnbindClient)
        {
            pClassModule->ClassLibraryInfo->ClassLibraryUnbindClient(ClassBindInfo, &Globals);
        }
        ClassUnlinkClient(pClassModule, ClassBindInfo);
        ClassReleaseClientReference(pClassModule);
        ClassBindInfo->ClassModule = NULL;
    }
}
