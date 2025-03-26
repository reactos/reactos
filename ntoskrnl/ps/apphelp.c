/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ps/apphelp.c
 * PURPOSE:         SHIM engine caching.
 *                  This caching speeds up checks for the apphelp compatibility layer.
 * PROGRAMMERS:     Mark Jansen
 */

/*
Useful references:
https://github.com/mandiant/ShimCacheParser/blob/master/ShimCacheParser.py
https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-7/dd837644(v=ws.10)
https://learn.microsoft.com/en-us/windows/win32/devnotes/application-compatibility-database
https://www.alex-ionescu.com/secrets-of-the-application-compatilibity-database-sdb-part-4/
https://web.archive.org/web/20170101173150/http://recxltd.blogspot.nl/2012/04/windows-appcompat-research-notes-part-1.html
http://journeyintoir.blogspot.com/2013/12/revealing-recentfilecachebcf-file.html
https://web.archive.org/web/20150926070918/https://dl.mandiant.com/EE/library/Whitepaper_ShimCacheParser.pdf
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static BOOLEAN ApphelpCacheEnabled = FALSE;
static ERESOURCE ApphelpCacheLock;
static RTL_AVL_TABLE ApphelpShimCache;
static LIST_ENTRY ApphelpShimCacheAge;

extern ULONG InitSafeBootMode;

static UNICODE_STRING AppCompatCacheKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCompatCache");
static OBJECT_ATTRIBUTES AppCompatKeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&AppCompatCacheKey, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE);
static UNICODE_STRING AppCompatCacheValue = RTL_CONSTANT_STRING(L"AppCompatCache");

#define EMPTY_SHIM_ENTRY    { { 0 }, { { 0 } }, 0 }
#define MAX_SHIM_ENTRIES    0x200

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE (HANDLE)(-1)
#endif

#include <pshpack1.h>

typedef struct SHIM_PERSISTENT_CACHE_HEADER_52
{
    ULONG Magic;
    ULONG NumEntries;
} SHIM_PERSISTENT_CACHE_HEADER_52, *PSHIM_PERSISTENT_CACHE_HEADER_52;

/* The data that is present in the registry (Win2k3 version) */
typedef struct SHIM_PERSISTENT_CACHE_ENTRY_52
{
    UNICODE_STRING ImageName;
    LARGE_INTEGER DateTime;
    LARGE_INTEGER FileSize;
} SHIM_PERSISTENT_CACHE_ENTRY_52, *PSHIM_PERSISTENT_CACHE_ENTRY_52;

#include <poppack.h>

#define CACHE_MAGIC_NT_52 0xbadc0ffe
#define CACHE_HEADER_SIZE_NT_52 0x8
#define NT52_PERSISTENT_ENTRY_SIZE32 0x18
#define NT52_PERSISTENT_ENTRY_SIZE64 0x20

//#define CACHE_MAGIC_NT_61 0xbadc0fee
//#define CACHE_HEADER_SIZE_NT_61 0x80
//#define NT61_PERSISTENT_ENTRY_SIZE32 0x20
//#define NT61_PERSISTENT_ENTRY_SIZE64 0x30

#define SHIM_CACHE_MAGIC                    CACHE_MAGIC_NT_52
#define SHIM_CACHE_HEADER_SIZE              CACHE_HEADER_SIZE_NT_52
#ifdef _WIN64
#define SHIM_PERSISTENT_CACHE_ENTRY_SIZE    NT52_PERSISTENT_ENTRY_SIZE64
#else
#define SHIM_PERSISTENT_CACHE_ENTRY_SIZE    NT52_PERSISTENT_ENTRY_SIZE32
#endif
#define SHIM_PERSISTENT_CACHE_HEADER        SHIM_PERSISTENT_CACHE_HEADER_52
#define PSHIM_PERSISTENT_CACHE_HEADER      PSHIM_PERSISTENT_CACHE_HEADER_52
#define SHIM_PERSISTENT_CACHE_ENTRY         SHIM_PERSISTENT_CACHE_ENTRY_52
#define PSHIM_PERSISTENT_CACHE_ENTRY       PSHIM_PERSISTENT_CACHE_ENTRY_52

C_ASSERT(sizeof(SHIM_PERSISTENT_CACHE_ENTRY) == SHIM_PERSISTENT_CACHE_ENTRY_SIZE);
C_ASSERT(sizeof(SHIM_PERSISTENT_CACHE_HEADER) == SHIM_CACHE_HEADER_SIZE);

/* The struct we keep in memory */
typedef struct SHIM_CACHE_ENTRY
{
    LIST_ENTRY List;
    SHIM_PERSISTENT_CACHE_ENTRY Persistent;
    ULONG CompatFlags;
} SHIM_CACHE_ENTRY, *PSHIM_CACHE_ENTRY;

/* PRIVATE FUNCTIONS *********************************************************/

PVOID
ApphelpAlloc(
    _In_ ULONG ByteSize)
{
    return ExAllocatePoolWithTag(PagedPool, ByteSize, TAG_SHIM);
}

VOID
ApphelpFree(
    _In_ PVOID Data)
{
    ExFreePoolWithTag(Data, TAG_SHIM);
}

VOID
ApphelpCacheAcquireLock(VOID)
{
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&ApphelpCacheLock, TRUE);
}

BOOLEAN
ApphelpCacheTryAcquireLock(VOID)
{
    KeEnterCriticalRegion();
    if (!ExTryToAcquireResourceExclusiveLite(&ApphelpCacheLock))
    {
        KeLeaveCriticalRegion();
        return FALSE;
    }
    return TRUE;
}

VOID
ApphelpCacheReleaseLock(VOID)
{
    ExReleaseResourceLite(&ApphelpCacheLock);
    KeLeaveCriticalRegion();
}

VOID
ApphelpDuplicateUnicodeString(
    _Out_ PUNICODE_STRING Destination,
    _In_ PCUNICODE_STRING Source)
{
    Destination->Length = Source->Length;
    if (Destination->Length)
    {
        Destination->MaximumLength = Destination->Length + sizeof(WCHAR);
        Destination->Buffer = ApphelpAlloc(Destination->MaximumLength);
        RtlCopyMemory(Destination->Buffer, Source->Buffer, Destination->Length);
        Destination->Buffer[Destination->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        Destination->MaximumLength = 0;
        Destination->Buffer = NULL;
    }
}

VOID
ApphelpFreeUnicodeString(
    _Inout_ PUNICODE_STRING String)
{
    if (String->Buffer)
    {
        ApphelpFree(String->Buffer);
    }
    String->Length = 0;
    String->MaximumLength = 0;
    String->Buffer = NULL;
}

/* Query file info from a handle, storing it in Entry */
NTSTATUS
ApphelpCacheQueryInfo(
    _In_ HANDLE ImageHandle,
    _Out_ PSHIM_CACHE_ENTRY Entry)
{
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileBasic;
    FILE_STANDARD_INFORMATION FileStandard;
    NTSTATUS Status;

    Status = ZwQueryInformationFile(ImageHandle,
                                    &IoStatusBlock,
                                    &FileBasic,
                                    sizeof(FileBasic),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ZwQueryInformationFile(ImageHandle,
                                    &IoStatusBlock,
                                    &FileStandard,
                                    sizeof(FileStandard),
                                    FileStandardInformation);
    if (NT_SUCCESS(Status))
    {
        Entry->Persistent.DateTime = FileBasic.LastWriteTime;
        Entry->Persistent.FileSize = FileStandard.EndOfFile;
    }
    return Status;
}

RTL_GENERIC_COMPARE_RESULTS
NTAPI
ApphelpShimCacheCompareRoutine(
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct)
{
    PSHIM_CACHE_ENTRY FirstEntry = FirstStruct;
    PSHIM_CACHE_ENTRY SecondEntry = SecondStruct;
    LONG Result;

    Result = RtlCompareUnicodeString(&FirstEntry->Persistent.ImageName,
                                     &SecondEntry->Persistent.ImageName,
                                     TRUE);
    if (Result < 0)
    {
        return GenericLessThan;
    }
    else if (Result == 0)
    {
        return GenericEqual;
    }
    return GenericGreaterThan;
}

PVOID
NTAPI
ApphelpShimCacheAllocateRoutine(
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ CLONG ByteSize)
{
    return ApphelpAlloc(ByteSize);
}

VOID
NTAPI
ApphelpShimCacheFreeRoutine(
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ PVOID Buffer)
{
    ApphelpFree(Buffer);
}

NTSTATUS
ApphelpCacheParse(
    _In_reads_(DataLength) PUCHAR Data,
    _In_ ULONG DataLength)
{
    PSHIM_PERSISTENT_CACHE_HEADER Header = (PSHIM_PERSISTENT_CACHE_HEADER)Data;
    ULONG Cur;
    ULONG NumEntries;
    UNICODE_STRING String;
    SHIM_CACHE_ENTRY Entry = EMPTY_SHIM_ENTRY;
    PSHIM_CACHE_ENTRY Result;
    PSHIM_PERSISTENT_CACHE_ENTRY Persistent;

    if (DataLength < CACHE_HEADER_SIZE_NT_52)
    {
        DPRINT1("SHIMS: ApphelpCacheParse not enough data for a minimal header (0x%x)\n", DataLength);
        return STATUS_INVALID_PARAMETER;
    }

    if (Header->Magic != SHIM_CACHE_MAGIC)
    {
        DPRINT1("SHIMS: ApphelpCacheParse found invalid magic (0x%x)\n", Header->Magic);
        return STATUS_INVALID_PARAMETER;
    }

    NumEntries = Header->NumEntries;
    DPRINT("SHIMS: ApphelpCacheParse walking %d entries\n", NumEntries);
    for (Cur = 0; Cur < NumEntries; ++Cur)
    {
        Persistent = (PSHIM_PERSISTENT_CACHE_ENTRY)(Data + SHIM_CACHE_HEADER_SIZE +
                                                    (Cur * SHIM_PERSISTENT_CACHE_ENTRY_SIZE));
        /* The entry in the Persistent storage is not really a UNICODE_STRING,
            so we have to convert the offset into a real pointer before using it. */
        String.Length = Persistent->ImageName.Length;
        String.MaximumLength = Persistent->ImageName.MaximumLength;
        String.Buffer = (PWCHAR)((ULONG_PTR)Persistent->ImageName.Buffer + Data);

        /* Now we copy all data to a local buffer, that can be safely duplicated by RtlInsert */
        Entry.Persistent = *Persistent;
        ApphelpDuplicateUnicodeString(&Entry.Persistent.ImageName, &String);
        Result = RtlInsertElementGenericTableAvl(&ApphelpShimCache,
                                                 &Entry,
                                                 sizeof(Entry),
                                                 NULL);
        if (!Result)
        {
            DPRINT1("SHIMS: ApphelpCacheParse insert failed\n");
            ApphelpFreeUnicodeString(&Entry.Persistent.ImageName);
            return STATUS_INVALID_PARAMETER;
        }
        InsertTailList(&ApphelpShimCacheAge, &Result->List);
    }
    return STATUS_SUCCESS;
}

BOOLEAN
ApphelpCacheRead(VOID)
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    KEY_VALUE_PARTIAL_INFORMATION KeyValueObject;
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation = &KeyValueObject;
    ULONG KeyInfoSize, ResultSize;

    Status = ZwOpenKey(&KeyHandle, KEY_QUERY_VALUE, &AppCompatKeyAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SHIMS: ApphelpCacheRead could not even open Session Manager\\AppCompatCache (0x%x)\n", Status);
        return FALSE;
    }

    Status = ZwQueryValueKey(KeyHandle,
                             &AppCompatCacheValue,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(KeyValueObject),
                             &ResultSize);
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + KeyValueInformation->DataLength;
        KeyValueInformation = ApphelpAlloc(KeyInfoSize);
        if (KeyValueInformation != NULL)
        {
            Status = ZwQueryValueKey(KeyHandle,
                                     &AppCompatCacheValue,
                                     KeyValuePartialInformation,
                                     KeyValueInformation,
                                     KeyInfoSize,
                                     &ResultSize);
        }
    }

    if (NT_SUCCESS(Status) && KeyValueInformation->Type == REG_BINARY)
    {
        Status = ApphelpCacheParse(KeyValueInformation->Data,
                                   KeyValueInformation->DataLength);
    }
    else
    {
        DPRINT1("SHIMS: ApphelpCacheRead not loaded from registry (0x%x)\n", Status);
    }

    if (KeyValueInformation != &KeyValueObject && KeyValueInformation != NULL)
    {
        ApphelpFree(KeyValueInformation);
    }

    ZwClose(KeyHandle);
    return NT_SUCCESS(Status);
}

BOOLEAN
ApphelpCacheWrite(VOID)
{
    ULONG Length = SHIM_CACHE_HEADER_SIZE;
    ULONG NumEntries = 0;
    PLIST_ENTRY ListEntry;
    PUCHAR Buffer, BufferNamePos;
    PSHIM_PERSISTENT_CACHE_HEADER Header;
    PSHIM_PERSISTENT_CACHE_ENTRY WriteEntry;
    HANDLE KeyHandle;
    NTSTATUS Status;

    /* First we have to calculate the required size. */
    ApphelpCacheAcquireLock();
    ListEntry = ApphelpShimCacheAge.Flink;
    while (ListEntry != &ApphelpShimCacheAge)
    {
        PSHIM_CACHE_ENTRY Entry = CONTAINING_RECORD(ListEntry, SHIM_CACHE_ENTRY, List);
        Length += SHIM_PERSISTENT_CACHE_ENTRY_SIZE;
        Length += Entry->Persistent.ImageName.MaximumLength;
        ++NumEntries;
        ListEntry = ListEntry->Flink;
    }
    DPRINT("SHIMS: ApphelpCacheWrite, %d Entries, total size: %d\n", NumEntries, Length);
    Length = ROUND_UP(Length, sizeof(ULONGLONG));
    DPRINT("SHIMS: ApphelpCacheWrite, Rounded to: %d\n", Length);

    /* Now we allocate and prepare some helpers */
    Buffer = ApphelpAlloc(Length);
    BufferNamePos = Buffer + Length;
    Header = (PSHIM_PERSISTENT_CACHE_HEADER)Buffer;
    WriteEntry = (PSHIM_PERSISTENT_CACHE_ENTRY)(Buffer + SHIM_CACHE_HEADER_SIZE);

    Header->Magic = SHIM_CACHE_MAGIC;
    Header->NumEntries = NumEntries;

    ListEntry = ApphelpShimCacheAge.Flink;
    while (ListEntry != &ApphelpShimCacheAge)
    {
        PSHIM_CACHE_ENTRY Entry = CONTAINING_RECORD(ListEntry, SHIM_CACHE_ENTRY, List);
        USHORT ImageNameLen = Entry->Persistent.ImageName.MaximumLength;
        /* Copy the Persistent structure over */
        *WriteEntry = Entry->Persistent;
        BufferNamePos -= ImageNameLen;
        /* Copy the image name over */
        RtlCopyMemory(BufferNamePos, Entry->Persistent.ImageName.Buffer, ImageNameLen);
        /* Fix the Persistent structure, so that Buffer is once again an offset */
        WriteEntry->ImageName.Buffer = (PWCH)(BufferNamePos - Buffer);

        ++WriteEntry;
        ListEntry = ListEntry->Flink;
    }
    ApphelpCacheReleaseLock();

    Status = ZwOpenKey(&KeyHandle, KEY_SET_VALUE, &AppCompatKeyAttributes);
    if (NT_SUCCESS(Status))
    {
        Status = ZwSetValueKey(KeyHandle,
                               &AppCompatCacheValue,
                               0,
                               REG_BINARY,
                               Buffer,
                               Length);
        ZwClose(KeyHandle);
    }
    else
    {
        DPRINT1("SHIMS: ApphelpCacheWrite could not even open Session Manager\\AppCompatCache (0x%x)\n", Status);
    }

    ApphelpFree(Buffer);
    return NT_SUCCESS(Status);
}


CODE_SEG("INIT")
NTSTATUS
NTAPI
ApphelpCacheInitialize(VOID)
{
    DPRINT("SHIMS: ApphelpCacheInitialize\n");
    /* If we are booting in safemode we do not want to use the apphelp cache */
    if (InitSafeBootMode)
    {
        DPRINT1("SHIMS: Safe mode detected, disabling cache.\n");
        ApphelpCacheEnabled = FALSE;
    }
    else
    {
        ExInitializeResourceLite(&ApphelpCacheLock);
        RtlInitializeGenericTableAvl(&ApphelpShimCache,
                                     ApphelpShimCacheCompareRoutine,
                                     ApphelpShimCacheAllocateRoutine,
                                     ApphelpShimCacheFreeRoutine,
                                     NULL);
        InitializeListHead(&ApphelpShimCacheAge);
        ApphelpCacheEnabled = ApphelpCacheRead();
    }
    DPRINT("SHIMS: ApphelpCacheInitialize: %d\n", ApphelpCacheEnabled);
    return STATUS_SUCCESS;
}

VOID
NTAPI
ApphelpCacheShutdown(VOID)
{
    if (ApphelpCacheEnabled)
    {
        ApphelpCacheWrite();
    }
}

NTSTATUS
ApphelpValidateData(
    _In_opt_ PAPPHELP_CACHE_SERVICE_LOOKUP ServiceData,
    _Out_ PUNICODE_STRING ImageName,
    _Out_ PHANDLE ImageHandle)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    if (ServiceData)
    {
        UNICODE_STRING LocalImageName;
        _SEH2_TRY
        {
            ProbeForRead(ServiceData,
                         sizeof(APPHELP_CACHE_SERVICE_LOOKUP),
                         sizeof(ULONG));
            LocalImageName = ServiceData->ImageName;
            *ImageHandle = ServiceData->ImageHandle;
            if (LocalImageName.Length && LocalImageName.Buffer)
            {
                ProbeForRead(LocalImageName.Buffer,
                             LocalImageName.Length * sizeof(WCHAR),
                             1);
                ApphelpDuplicateUnicodeString(ImageName, &LocalImageName);
                Status = STATUS_SUCCESS;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SHIMS: ApphelpValidateData: invalid data passed\n");
    }
    return Status;
}

NTSTATUS
ApphelpCacheRemoveEntryNolock(
    _In_ PSHIM_CACHE_ENTRY Entry)
{
    if (Entry)
    {
        PWSTR Buffer = Entry->Persistent.ImageName.Buffer;
        RemoveEntryList(&Entry->List);
        if (RtlDeleteElementGenericTableAvl(&ApphelpShimCache, Entry))
        {
            ApphelpFree(Buffer);
        }
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_FOUND;
}

NTSTATUS
ApphelpCacheLookupEntry(
    _In_ PUNICODE_STRING ImageName,
    _In_ HANDLE ImageHandle)
{
    NTSTATUS Status = STATUS_NOT_FOUND;
    SHIM_CACHE_ENTRY Lookup = EMPTY_SHIM_ENTRY;
    PSHIM_CACHE_ENTRY Entry;

    if (!ApphelpCacheTryAcquireLock())
    {
        return Status;
    }

    Lookup.Persistent.ImageName = *ImageName;
    Entry = RtlLookupElementGenericTableAvl(&ApphelpShimCache, &Lookup);
    if (Entry == NULL)
    {
        DPRINT("SHIMS: ApphelpCacheLookupEntry: could not find %wZ\n", ImageName);
        goto Cleanup;
    }

    DPRINT("SHIMS: ApphelpCacheLookupEntry: found %wZ\n", ImageName);
    if (ImageHandle == INVALID_HANDLE_VALUE)
    {
        DPRINT("SHIMS: ApphelpCacheLookupEntry: ok\n");
        /* just return if we know it, do not query file info */
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = ApphelpCacheQueryInfo(ImageHandle, &Lookup);
        if (NT_SUCCESS(Status) &&
            Lookup.Persistent.DateTime.QuadPart == Entry->Persistent.DateTime.QuadPart &&
            Lookup.Persistent.FileSize.QuadPart == Entry->Persistent.FileSize.QuadPart)
        {
            DPRINT("SHIMS: ApphelpCacheLookupEntry: found & validated\n");
            Status = STATUS_SUCCESS;
            /* move it to the front to keep it alive */
            RemoveEntryList(&Entry->List);
            InsertHeadList(&ApphelpShimCacheAge, &Entry->List);
        }
        else
        {
            DPRINT1("SHIMS: ApphelpCacheLookupEntry: file info mismatch (%lx)\n", Status);
            Status = STATUS_NOT_FOUND;
            /* Could not read file info, or it did not match, drop it from the cache */
            ApphelpCacheRemoveEntryNolock(Entry);
        }
    }

Cleanup:
    ApphelpCacheReleaseLock();
    return Status;
}

NTSTATUS
ApphelpCacheRemoveEntry(
    _In_ PUNICODE_STRING ImageName)
{
    PSHIM_CACHE_ENTRY Entry;
    NTSTATUS Status;

    ApphelpCacheAcquireLock();
    Entry = RtlLookupElementGenericTableAvl(&ApphelpShimCache, ImageName);
    Status = ApphelpCacheRemoveEntryNolock(Entry);
    ApphelpCacheReleaseLock();
    return Status;
}

/* Validate that we are either called from r0, or from a service-like context */
NTSTATUS
ApphelpCacheAccessCheck(VOID)
{
    if (ExGetPreviousMode() != KernelMode)
    {
        if (!SeSinglePrivilegeCheck(SeTcbPrivilege, UserMode))
        {
            DPRINT1("SHIMS: ApphelpCacheAccessCheck failed\n");
            return STATUS_ACCESS_DENIED;
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
ApphelpCacheUpdateEntry(
    _In_ PUNICODE_STRING ImageName,
    _In_ HANDLE ImageHandle)
{
    NTSTATUS Status = STATUS_SUCCESS;
    SHIM_CACHE_ENTRY Entry = EMPTY_SHIM_ENTRY;
    PSHIM_CACHE_ENTRY Lookup;
    PVOID NodeOrParent;
    TABLE_SEARCH_RESULT SearchResult;

    ApphelpCacheAcquireLock();

    /* If we got a file handle, query it for info */
    if (ImageHandle != INVALID_HANDLE_VALUE)
    {
        Status = ApphelpCacheQueryInfo(ImageHandle, &Entry);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    /* Use ImageName for the lookup, don't actually duplicate it */
    Entry.Persistent.ImageName = *ImageName;
    Lookup = RtlLookupElementGenericTableFullAvl(&ApphelpShimCache,
                                                 &Entry,
                                                 &NodeOrParent, &SearchResult);
    if (Lookup)
    {
        DPRINT("SHIMS: ApphelpCacheUpdateEntry: Entry already exists, reusing it\n");
        /* Unlink the found item, so we can put it back at the front,
            and copy the earlier obtained file info*/
        RemoveEntryList(&Lookup->List);
        Lookup->Persistent.DateTime = Entry.Persistent.DateTime;
        Lookup->Persistent.FileSize = Entry.Persistent.FileSize;
    }
    else
    {
        DPRINT("SHIMS: ApphelpCacheUpdateEntry: Inserting new Entry\n");
        /* Insert a new entry, with its own copy of the ImageName */
        ApphelpDuplicateUnicodeString(&Entry.Persistent.ImageName, ImageName);
        Lookup = RtlInsertElementGenericTableFullAvl(&ApphelpShimCache,
                                                     &Entry,
                                                     sizeof(Entry),
                                                     0,
                                                     NodeOrParent,
                                                     SearchResult);
        if (!Lookup)
        {
            ApphelpFreeUnicodeString(&Entry.Persistent.ImageName);
            Status = STATUS_NO_MEMORY;
        }
    }
    if (Lookup)
    {
        /* Either we re-used an existing item, or we inserted a new one, keep it alive */
        InsertHeadList(&ApphelpShimCacheAge, &Lookup->List);
        if (RtlNumberGenericTableElementsAvl(&ApphelpShimCache) > MAX_SHIM_ENTRIES)
        {
            PSHIM_CACHE_ENTRY Remove;
            DPRINT1("SHIMS: ApphelpCacheUpdateEntry: Cache growing too big, dropping oldest item\n");
            Remove = CONTAINING_RECORD(ApphelpShimCacheAge.Blink, SHIM_CACHE_ENTRY, List);
            Status = ApphelpCacheRemoveEntryNolock(Remove);
        }
    }

Cleanup:
    ApphelpCacheReleaseLock();
    return Status;
}

NTSTATUS
ApphelpCacheFlush(VOID)
{
    PVOID p;

    DPRINT1("SHIMS: ApphelpCacheFlush\n");
    ApphelpCacheAcquireLock();
    while ((p = RtlEnumerateGenericTableAvl(&ApphelpShimCache, TRUE)))
    {
        ApphelpCacheRemoveEntryNolock((PSHIM_CACHE_ENTRY)p);
    }
    ApphelpCacheReleaseLock();
    return STATUS_SUCCESS;
}

NTSTATUS
ApphelpCacheDump(VOID)
{
    PLIST_ENTRY ListEntry;
    PSHIM_CACHE_ENTRY Entry;

    DPRINT1("SHIMS: NtApphelpCacheControl( Dumping entries, newest to oldest )\n");
    ApphelpCacheAcquireLock();
    ListEntry = ApphelpShimCacheAge.Flink;
    while (ListEntry != &ApphelpShimCacheAge)
    {
        Entry = CONTAINING_RECORD(ListEntry, SHIM_CACHE_ENTRY, List);
        DPRINT1("Entry: %wZ\n", &Entry->Persistent.ImageName);
        DPRINT1("DateTime: 0x%I64x\n", Entry->Persistent.DateTime.QuadPart);
        DPRINT1("FileSize: 0x%I64x\n", Entry->Persistent.FileSize.QuadPart);
        DPRINT1("Flags: 0x%x\n", Entry->CompatFlags);
        ListEntry = ListEntry->Flink;
    }
    ApphelpCacheReleaseLock();
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
NtApphelpCacheControl(
    _In_ APPHELPCACHESERVICECLASS Service,
    _In_opt_ PAPPHELP_CACHE_SERVICE_LOOKUP ServiceData)
{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    UNICODE_STRING ImageName = { 0 };
    HANDLE Handle = INVALID_HANDLE_VALUE;

    if (!ApphelpCacheEnabled)
    {
        DPRINT1("NtApphelpCacheControl: ApphelpCacheEnabled == 0\n");
        return Status;
    }
    switch (Service)
    {
        case ApphelpCacheServiceLookup:
            DPRINT("SHIMS: NtApphelpCacheControl( ApphelpCacheServiceLookup )\n");
            Status = ApphelpValidateData(ServiceData, &ImageName, &Handle);
            if (NT_SUCCESS(Status))
                Status = ApphelpCacheLookupEntry(&ImageName, Handle);
            break;
        case ApphelpCacheServiceRemove:
            DPRINT("SHIMS: NtApphelpCacheControl( ApphelpCacheServiceRemove )\n");
            Status = ApphelpValidateData(ServiceData, &ImageName, &Handle);
            if (NT_SUCCESS(Status))
                Status = ApphelpCacheRemoveEntry(&ImageName);
            break;
        case ApphelpCacheServiceUpdate:
            DPRINT("SHIMS: NtApphelpCacheControl( ApphelpCacheServiceUpdate )\n");
            Status = ApphelpCacheAccessCheck();
            if (NT_SUCCESS(Status))
            {
                Status = ApphelpValidateData(ServiceData, &ImageName, &Handle);
                if (NT_SUCCESS(Status))
                    Status = ApphelpCacheUpdateEntry(&ImageName, Handle);
            }
            break;
        case ApphelpCacheServiceFlush:
            /* FIXME: Check for admin or system here. */
            Status = ApphelpCacheFlush();
            break;
        case ApphelpCacheServiceDump:
            Status = ApphelpCacheDump();
            break;
        case ApphelpDBGReadRegistry:
            DPRINT1("SHIMS: NtApphelpCacheControl( ApphelpDBGReadRegistry ): flushing cache.\n");
            ApphelpCacheFlush();
            DPRINT1("SHIMS: NtApphelpCacheControl( ApphelpDBGReadRegistry ): reading cache.\n");
            Status = ApphelpCacheRead() ? STATUS_SUCCESS : STATUS_NOT_FOUND;
            break;
        case ApphelpDBGWriteRegistry:
            DPRINT1("SHIMS: NtApphelpCacheControl( ApphelpDBGWriteRegistry ): writing cache.\n");
            Status = ApphelpCacheWrite() ? STATUS_SUCCESS : STATUS_NOT_FOUND;
            break;
        default:
            DPRINT1("SHIMS: NtApphelpCacheControl( Invalid service requested )\n");
            break;
    }
    if (ImageName.Buffer)
    {
        ApphelpFreeUnicodeString(&ImageName);
    }
    return Status;
}

