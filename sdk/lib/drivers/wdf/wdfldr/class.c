/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - class functions
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
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
    {
        ExFreePoolWithTag(ClassModule->Service.Buffer, WDFLDR_TAG);
        ClassModule->Service.Length = 0;
        ClassModule->Service.Buffer = NULL;
    }

    if (ClassModule->ImageName.Buffer != NULL)
    {
        ExFreePoolWithTag(ClassModule->ImageName.Buffer, WDFLDR_TAG);
        ClassModule->ImageName.Length = 0;
        ClassModule->ImageName.Buffer = NULL;
    }

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

    pNewClassModule = ExAllocatePoolWithTag(NonPagedPool, sizeof(CLASS_MODULE), WDFLDR_TAG);

    if (pNewClassModule == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(pNewClassModule, sizeof(CLASS_MODULE));
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
    pNewClassModule->Service.Buffer = ExAllocatePoolWithTag(PagedPool, ServiceName->MaximumLength, WDFLDR_TAG);

    if (pNewClassModule->Service.Buffer == NULL)
    {
        goto clean;
    }

    pNewClassModule->Service.MaximumLength = ServiceName->MaximumLength;
    RtlZeroMemory(pNewClassModule->Service.Buffer, pNewClassModule->Service.MaximumLength);
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

    pNewClassClientModule = ExAllocatePoolWithTag(NonPagedPool, sizeof(CLASS_CLIENT_MODULE), WDFLDR_TAG);

    if (pNewClassClientModule != NULL)
    {
        RtlZeroMemory(pNewClassClientModule, sizeof(CLASS_CLIENT_MODULE));
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
    PWCHAR pClassName;
    PWCHAR pClassNameBegin;
    WCHAR currentSym;
    PWDF_BIND_INFO pBindInfo;
    PWDF_CLASS_BIND_INFO pClassBindInfo;
    UNICODE_STRING classVersions;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE rootHandle = NULL;
    HANDLE classVersionsHandle = NULL;
    HANDLE classVersionHandle = NULL;
    NTSTATUS status;
    SIZE_T numOfBytes;
    UNICODE_STRING objectName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf\\Kmdf");
    DECLARE_UNICODE_STRING_SIZE(versionString, 11);
    
    pClassBindInfo = ClassBindInfo;
    pBindInfo = BindInfo;
    classVersions.Length = 0;
    classVersions.Buffer = NULL;
    *KeyHandle = NULL;

    InitializeObjectAttributes(&objectAttributes, &objectName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenKey(&rootHandle, KEY_QUERY_VALUE, &objectAttributes);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ZwOpenKey status 0x%08X\n", status));
        goto clean;
    }

    pClassName = pClassBindInfo->ClassName;
    pClassNameBegin = pClassName;
    do
    {
        currentSym = *pClassName;
        ++pClassName;
    } while ( currentSym != '\0');

    numOfBytes = sizeof(WCHAR) * (pClassName - pClassNameBegin - 1) + sizeof(L"\\Versions");
    classVersions.Buffer = ExAllocatePoolWithTag(PagedPool, numOfBytes, WDFLDR_TAG);

    if (classVersions.Buffer == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto clean;
    }
        
    classVersions.MaximumLength = (USHORT)numOfBytes;
    status = RtlUnicodeStringPrintf(&classVersions, L"%s\\Versions", pClassBindInfo->ClassName);
    
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("RtlUnicodeStringPrintf status 0x%08x\n", status));
        goto clean;
    }

    InitializeObjectAttributes(&objectAttributes, &classVersions, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, rootHandle, NULL);
    status = ZwOpenKey(&classVersionsHandle, KEY_QUERY_VALUE, &objectAttributes);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ZwOpenKey status 0x%08X\n", status));
        goto clean;
    }

    status = RtlIntegerToUnicodeString(pClassBindInfo->Version.Major, 10, &versionString);
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ConvertUlongToWString status 0x%08X\n", status));
        goto clean;
    }

    InitializeObjectAttributes(&objectAttributes, &versionString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, classVersionsHandle, NULL);
    status = ZwOpenKey(&classVersionHandle, KEY_QUERY_VALUE, &objectAttributes);

    if (NT_SUCCESS(status))
    {
        status = RtlIntegerToUnicodeString(pBindInfo->Version.Major, 10, &versionString);
        if (NT_SUCCESS(status))
        {
            InitializeObjectAttributes(&objectAttributes, &versionString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, classVersionHandle, NULL);
            status = ZwOpenKey(KeyHandle, KEY_QUERY_VALUE, &objectAttributes);
        }
    }

clean:
    if (rootHandle)
        ZwClose(rootHandle);
    if (classVersionsHandle)
        ZwClose(classVersionsHandle);
    if (KeyHandle)
        ZwClose(KeyHandle);

    if (classVersions.Buffer)
    {
        ExFreePoolWithTag(classVersions.Buffer, WDFLDR_TAG);
        classVersions.Length = 0;
        classVersions.Buffer = NULL;
    }

    return status;
}

NTSTATUS
GetDefaultClassServiceName(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PUNICODE_STRING ServiceName)
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

    size = sizeof(WCHAR) * (pClassName - pClassNameBegin - 1) + 148;
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
        ExFreePoolWithTag(ServiceName->Buffer, WDFLDR_TAG);
        ServiceName->Length = 0;
        ServiceName->Buffer = NULL;
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

    status = ZwLoadDriver(&driverServiceName);
    
    if (NT_SUCCESS(status))
    {
        ClassBindInfo->ClassModule = pClassModule;
        *ClassModule = pClassModule;
    }
        
    if (status == STATUS_IMAGE_ALREADY_LOADED || status == STATUS_OBJECT_NAME_COLLISION)
    {
        if (pClassModule->ClassLibraryInfo)
        {
            status = STATUS_SUCCESS;                
        }
        else
        {
            __DBGPRINT(("ZwLoadDriver (%wZ) failed and no Libray information was returned: 0x%x\n",
                &driverServiceName,
                status));
        }
    }
    else
    {
        __DBGPRINT(("WARNING: ZwLoadDriver (%wZ) failed with Status 0x%x\n", &driverServiceName, status));
        pClassModule->ImageAlreadyLoaded = 1;
    }

clean:
    if (pClassModule && InterlockedExchangeAdd(&pClassModule->ClassRefCount, -1) <= 0)
    {
        ClassCleanupAndFree(pClassModule);
    }

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
    {
        return classListHead;
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
        if (!InterlockedExchangeAdd(&pClassModule->ClassRefCount, -1))
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

    result = &ClassModule->LibraryLinkage;
    InsertHeadList(&LibModule->ClassListHead, &ClassModule->LibraryLinkage);

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
        if (!InterlockedExchangeAdd(&ClassModule->ClassRefCount, -1))
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
            &ClassModule->Service,
            refs));
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
        pClassModule->ClassLibraryInfo->ClassLibraryUnbindClient(ClassBindInfo, Globals);
        ClassUnlinkClient(pClassModule, ClassBindInfo);
        ClassReleaseClientReference(pClassModule);
        ClassBindInfo->ClassModule = NULL;
    }
}
