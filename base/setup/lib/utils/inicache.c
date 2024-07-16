/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     INI file parser that caches contents of INI file in memory.
 * COPYRIGHT:   Copyright 2002-2018 Royce Mitchell III
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "inicache.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS ********************************************************/

static VOID
IniCacheFreeKey(
    _In_ PINI_KEYWORD Key)
{
    /* Unlink the key */
    RemoveEntryList(&Key->ListEntry);

    /* Free its data */
    if (Key->Name)
        RtlFreeHeap(ProcessHeap, 0, Key->Name);
    if (Key->Data)
        RtlFreeHeap(ProcessHeap, 0, Key->Data);
    RtlFreeHeap(ProcessHeap, 0, Key);
}

static VOID
IniCacheFreeSection(
    _In_ PINI_SECTION Section)
{
    /* Unlink the section */
    RemoveEntryList(&Section->ListEntry);

    /* Free its data */
    while (!IsListEmpty(&Section->KeyList))
    {
        PLIST_ENTRY Entry = RemoveHeadList(&Section->KeyList);
        PINI_KEYWORD Key = CONTAINING_RECORD(Entry, INI_KEYWORD, ListEntry);
        IniCacheFreeKey(Key);
    }
    if (Section->Name)
        RtlFreeHeap(ProcessHeap, 0, Section->Name);
    RtlFreeHeap(ProcessHeap, 0, Section);
}

static
PINI_SECTION
IniCacheFindSection(
    _In_ PINICACHE Cache,
    _In_ PCWSTR Name)
{
    PLIST_ENTRY Entry;

    for (Entry  =  Cache->SectionList.Flink;
         Entry != &Cache->SectionList;
         Entry  =  Entry->Flink)
    {
        PINI_SECTION Section = CONTAINING_RECORD(Entry, INI_SECTION, ListEntry);
        if (_wcsicmp(Section->Name, Name) == 0)
            return Section;
    }
    return NULL;
}

static
PINI_KEYWORD
IniCacheFindKey(
    _In_ PINI_SECTION Section,
    _In_ PCWSTR Name)
{
    PLIST_ENTRY Entry;

    for (Entry  =  Section->KeyList.Flink;
         Entry != &Section->KeyList;
         Entry  =  Entry->Flink)
    {
        PINI_KEYWORD Key = CONTAINING_RECORD(Entry, INI_KEYWORD, ListEntry);
        if (_wcsicmp(Key->Name, Name) == 0)
            return Key;
    }
    return NULL;
}

static
PINI_KEYWORD
IniCacheAddKeyAorW(
    _In_ PINI_SECTION Section,
    _In_ PINI_KEYWORD AnchorKey,
    _In_ INSERTION_TYPE InsertionType,
    _In_ const VOID* Name,
    _In_ ULONG NameLength,
    _In_ const VOID* Data,
    _In_ ULONG DataLength,
    _In_ BOOLEAN IsUnicode)
{
    PINI_KEYWORD Key;
    PWSTR NameU, DataU;

    if (!Section || !Name || NameLength == 0 || !Data || DataLength == 0)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    /* Allocate the UNICODE key name */
    NameU = (PWSTR)RtlAllocateHeap(ProcessHeap,
                                   0,
                                   (NameLength + 1) * sizeof(WCHAR));
    if (!NameU)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }
    /* Copy the value name (ANSI or UNICODE) */
    if (IsUnicode)
        wcsncpy(NameU, (PCWCH)Name, NameLength);
    else
        _snwprintf(NameU, NameLength, L"%.*S", NameLength, (PCCH)Name);
    NameU[NameLength] = UNICODE_NULL;

    /*
     * Find whether a key with the given name already exists in the section.
     * If so, modify the data and return it; otherwise create a new one.
     */
    Key = IniCacheFindKey(Section, NameU);
    if (Key)
    {
        RtlFreeHeap(ProcessHeap, 0, NameU);

        /* Modify the existing data */

        /* Allocate the UNICODE data buffer */
        DataU = (PWSTR)RtlAllocateHeap(ProcessHeap,
                                       0,
                                       (DataLength + 1) * sizeof(WCHAR));
        if (!DataU)
        {
            DPRINT("RtlAllocateHeap() failed\n");
            return NULL; // We failed, don't modify the original key.
        }
        /* Copy the data (ANSI or UNICODE) */
        if (IsUnicode)
            wcsncpy(DataU, (PCWCH)Data, DataLength);
        else
            _snwprintf(DataU, DataLength, L"%.*S", DataLength, (PCCH)Data);
        DataU[DataLength] = UNICODE_NULL;

        /* Swap the old key data with the new one */
        RtlFreeHeap(ProcessHeap, 0, Key->Data);
        Key->Data = DataU;

        /* Return the modified key */
        return Key;
    }

    /* Allocate the key buffer and name */
    Key = (PINI_KEYWORD)RtlAllocateHeap(ProcessHeap,
                                        HEAP_ZERO_MEMORY,
                                        sizeof(INI_KEYWORD));
    if (!Key)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, NameU);
        return NULL;
    }
    Key->Name = NameU;

    /* Allocate the UNICODE data buffer */
    DataU = (PWSTR)RtlAllocateHeap(ProcessHeap,
                                   0,
                                   (DataLength + 1) * sizeof(WCHAR));
    if (!DataU)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, NameU);
        RtlFreeHeap(ProcessHeap, 0, Key);
        return NULL;
    }
    /* Copy the data (ANSI or UNICODE) */
    if (IsUnicode)
        wcsncpy(DataU, (PCWCH)Data, DataLength);
    else
        _snwprintf(DataU, DataLength, L"%.*S", DataLength, (PCCH)Data);
    DataU[DataLength] = UNICODE_NULL;
    Key->Data = DataU;

    /* Insert the key into section */
    if (IsListEmpty(&Section->KeyList))
    {
        InsertHeadList(&Section->KeyList, &Key->ListEntry);
    }
    else if ((InsertionType == INSERT_FIRST) ||
             ((InsertionType == INSERT_BEFORE) &&
                (!AnchorKey || (&AnchorKey->ListEntry == Section->KeyList.Flink))))
    {
        /* Insert at the head of the list */
        InsertHeadList(&Section->KeyList, &Key->ListEntry);
    }
    else if ((InsertionType == INSERT_BEFORE) && AnchorKey)
    {
        /* Insert before the anchor key */
        InsertTailList(&AnchorKey->ListEntry, &Key->ListEntry);
    }
    else if ((InsertionType == INSERT_LAST) ||
             ((InsertionType == INSERT_AFTER) &&
                (!AnchorKey || (&AnchorKey->ListEntry == Section->KeyList.Blink))))
    {
        /* Insert at the tail of the list */
        InsertTailList(&Section->KeyList, &Key->ListEntry);
    }
    else if ((InsertionType == INSERT_AFTER) && AnchorKey)
    {
        /* Insert after the anchor key */
        InsertHeadList(&AnchorKey->ListEntry, &Key->ListEntry);
    }

    return Key;
}

static
PINI_SECTION
IniCacheAddSectionAorW(
    _In_ PINICACHE Cache,
    _In_ const VOID* Name,
    _In_ ULONG NameLength,
    _In_ BOOLEAN IsUnicode)
{
    PINI_SECTION Section;
    PWSTR NameU;

    if (!Cache || !Name || NameLength == 0)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    /* Allocate the UNICODE section name */
    NameU = (PWSTR)RtlAllocateHeap(ProcessHeap,
                                   0,
                                   (NameLength + 1) * sizeof(WCHAR));
    if (!NameU)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }
    /* Copy the section name (ANSI or UNICODE) */
    if (IsUnicode)
        wcsncpy(NameU, (PCWCH)Name, NameLength);
    else
        _snwprintf(NameU, NameLength, L"%.*S", NameLength, (PCCH)Name);
    NameU[NameLength] = UNICODE_NULL;

    /*
     * Find whether a section with the given name already exists.
     * If so, just return it; otherwise create a new one.
     */
    Section = IniCacheFindSection(Cache, NameU);
    if (Section)
    {
        RtlFreeHeap(ProcessHeap, 0, NameU);
        return Section;
    }

    /* Allocate the section buffer and name */
    Section = (PINI_SECTION)RtlAllocateHeap(ProcessHeap,
                                            HEAP_ZERO_MEMORY,
                                            sizeof(INI_SECTION));
    if (!Section)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, NameU);
        return NULL;
    }
    Section->Name = NameU;
    InitializeListHead(&Section->KeyList);

    /* Append the section */
    InsertTailList(&Cache->SectionList, &Section->ListEntry);

    return Section;
}

static
PCHAR
IniCacheSkipWhitespace(
    PCHAR Ptr)
{
    while (*Ptr != 0 && isspace(*Ptr))
        Ptr++;

    return (*Ptr == 0) ? NULL : Ptr;
}

static
PCHAR
IniCacheSkipToNextSection(
    PCHAR Ptr)
{
    while (*Ptr != 0 && *Ptr != '[')
    {
        while (*Ptr != 0 && *Ptr != L'\n')
        {
            Ptr++;
        }

        Ptr++;
    }

    return (*Ptr == 0) ? NULL : Ptr;
}

static
PCHAR
IniCacheGetSectionName(
    PCHAR Ptr,
    PCHAR *NamePtr,
    PULONG NameSize)
{
    ULONG Size = 0;

    *NamePtr = NULL;
    *NameSize = 0;

    /* Skip whitespace */
    while (*Ptr != 0 && isspace(*Ptr))
    {
        Ptr++;
    }

    *NamePtr = Ptr;

    while (*Ptr != 0 && *Ptr != ']')
    {
        Size++;
        Ptr++;
    }
    Ptr++;

    while (*Ptr != 0 && *Ptr != L'\n')
    {
        Ptr++;
    }
    Ptr++;

    *NameSize = Size;

    DPRINT("SectionName: '%.*s'\n", Size, *NamePtr);

    return Ptr;
}

static
PCHAR
IniCacheGetKeyName(
    PCHAR Ptr,
    PCHAR *NamePtr,
    PULONG NameSize)
{
    ULONG Size = 0;

    *NamePtr = NULL;
    *NameSize = 0;

    while (Ptr && *Ptr)
    {
        *NamePtr = NULL;
        *NameSize = 0;
        Size = 0;

        /* Skip whitespace and empty lines */
        while (isspace(*Ptr) || *Ptr == '\n' || *Ptr == '\r')
        {
            Ptr++;
        }
        if (*Ptr == 0)
        {
            continue;
        }

        *NamePtr = Ptr;

        while (*Ptr != 0 && !isspace(*Ptr) && *Ptr != '=' && *Ptr != ';')
        {
            Size++;
            Ptr++;
        }
        if (*Ptr == ';')
        {
            while (*Ptr != 0 && *Ptr != '\r' && *Ptr != '\n')
            {
                Ptr++;
            }
        }
        else
        {
            *NameSize = Size;
            break;
        }
    }

  return Ptr;
}

static
PCHAR
IniCacheGetKeyValue(
    PCHAR Ptr,
    PCHAR *DataPtr,
    PULONG DataSize,
    BOOLEAN String)
{
    ULONG Size = 0;

    *DataPtr = NULL;
    *DataSize = 0;

    /* Skip whitespace */
    while (*Ptr != 0 && isspace(*Ptr))
    {
        Ptr++;
    }

    /* Check and skip '=' */
    if (*Ptr != '=')
    {
        return NULL;
    }
    Ptr++;

    /* Skip whitespace */
    while (*Ptr != 0 && isspace(*Ptr))
    {
        Ptr++;
    }

    if (*Ptr == '"' && String)
    {
        Ptr++;

        /* Get data */
        *DataPtr = Ptr;
        while (*Ptr != '"')
        {
            Ptr++;
            Size++;
        }
        Ptr++;

        while (*Ptr && *Ptr != '\r' && *Ptr != '\n')
        {
            Ptr++;
        }
    }
    else
    {
        /* Get data */
        *DataPtr = Ptr;
        while (*Ptr != 0 && *Ptr != '\r' && *Ptr != ';')
        {
            Ptr++;
            Size++;
        }
    }

    /* Skip to next line */
    if (*Ptr == '\r')
        Ptr++;
    if (*Ptr == '\n')
        Ptr++;

    *DataSize = Size;

    return Ptr;
}


/* PUBLIC FUNCTIONS *********************************************************/

NTSTATUS
IniCacheLoadFromMemory(
    PINICACHE *Cache,
    PCHAR FileBuffer,
    ULONG FileLength,
    BOOLEAN String)
{
    PCHAR Ptr;

    PINI_SECTION Section;
    PINI_KEYWORD Key;

    PCHAR SectionName;
    ULONG SectionNameSize;

    PCHAR KeyName;
    ULONG KeyNameSize;

    PCHAR KeyValue;
    ULONG KeyValueSize;

    /* Allocate inicache header */
    *Cache = IniCacheCreate();
    if (!*Cache)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Parse ini file */
    Section = NULL;
    Ptr = FileBuffer;
    while (Ptr != NULL && *Ptr != 0)
    {
        Ptr = IniCacheSkipWhitespace(Ptr);
        if (Ptr == NULL)
            continue;

        if (*Ptr == '[')
        {
            Section = NULL;
            Ptr++;

            Ptr = IniCacheGetSectionName(Ptr,
                                         &SectionName,
                                         &SectionNameSize);

            DPRINT("[%.*s]\n", SectionNameSize, SectionName);

            Section = IniCacheAddSectionAorW(*Cache,
                                             SectionName,
                                             SectionNameSize,
                                             FALSE);
            if (Section == NULL)
            {
                DPRINT("IniCacheAddSectionAorW() failed\n");
                Ptr = IniCacheSkipToNextSection(Ptr);
                continue;
            }
        }
        else
        {
            if (Section == NULL)
            {
                Ptr = IniCacheSkipToNextSection(Ptr);
                continue;
            }

            Ptr = IniCacheGetKeyName(Ptr,
                                     &KeyName,
                                     &KeyNameSize);

            Ptr = IniCacheGetKeyValue(Ptr,
                                      &KeyValue,
                                      &KeyValueSize,
                                      String);

            DPRINT("'%.*s' = '%.*s'\n", KeyNameSize, KeyName, KeyValueSize, KeyValue);

            Key = IniCacheAddKeyAorW(Section,
                                     NULL,
                                     INSERT_LAST,
                                     KeyName,
                                     KeyNameSize,
                                     KeyValue,
                                     KeyValueSize,
                                     FALSE);
            if (Key == NULL)
            {
                DPRINT("IniCacheAddKeyAorW() failed\n");
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IniCacheLoadByHandle(
    PINICACHE *Cache,
    HANDLE FileHandle,
    BOOLEAN String)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION FileInfo;
    PCHAR FileBuffer;
    ULONG FileLength;
    LARGE_INTEGER FileOffset;

    *Cache = NULL;

    /* Query file size */
    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
        return Status;
    }

    FileLength = FileInfo.EndOfFile.u.LowPart;

    DPRINT("File size: %lu\n", FileLength);

    /* Allocate file buffer with NULL-terminator */
    FileBuffer = (PCHAR)RtlAllocateHeap(ProcessHeap,
                                        0,
                                        FileLength + 1);
    if (FileBuffer == NULL)
    {
        DPRINT1("RtlAllocateHeap() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Read file */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        FileBuffer,
                        FileLength,
                        &FileOffset,
                        NULL);

    /* Append NULL-terminator */
    FileBuffer[FileLength] = 0;

    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtReadFile() failed (Status %lx)\n", Status);
        goto Quit;
    }

    Status = IniCacheLoadFromMemory(Cache, FileBuffer, FileLength, String);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IniCacheLoadFromMemory() failed (Status %lx)\n", Status);
    }

Quit:
    /* Free the file buffer, and return */
    RtlFreeHeap(ProcessHeap, 0, FileBuffer);
    return Status;
}

NTSTATUS
IniCacheLoad(
    PINICACHE *Cache,
    PWCHAR FileName,
    BOOLEAN String)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;

    *Cache = NULL;

    /* Open the INI file */
    RtlInitUnicodeString(&Name, FileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        FILE_GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtOpenFile() failed (Status %lx)\n", Status);
        return Status;
    }

    DPRINT("NtOpenFile() successful\n");

    Status = IniCacheLoadByHandle(Cache, FileHandle, String);

    /* Close the INI file */
    NtClose(FileHandle);
    return Status;
}

VOID
IniCacheDestroy(
    _In_ PINICACHE Cache)
{
    if (!Cache)
        return;

    while (!IsListEmpty(&Cache->SectionList))
    {
        PLIST_ENTRY Entry = RemoveHeadList(&Cache->SectionList);
        PINI_SECTION Section = CONTAINING_RECORD(Entry, INI_SECTION, ListEntry);
        IniCacheFreeSection(Section);
    }

    RtlFreeHeap(ProcessHeap, 0, Cache);
}


PINI_SECTION
IniGetSection(
    _In_ PINICACHE Cache,
    _In_ PCWSTR Name)
{
    if (!Cache || !Name)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }
    return IniCacheFindSection(Cache, Name);
}

PINI_KEYWORD
IniGetKey(
    _In_ PINI_SECTION Section,
    _In_ PCWSTR KeyName,
    _Out_ PCWSTR* KeyData)
{
    PINI_KEYWORD Key;

    if (!Section || !KeyName || !KeyData)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    *KeyData = NULL;

    Key = IniCacheFindKey(Section, KeyName);
    if (!Key)
        return NULL;

    *KeyData = Key->Data;

    return Key;
}


PINICACHEITERATOR
IniFindFirstValue(
    _In_ PINI_SECTION Section,
    _Out_ PCWSTR* KeyName,
    _Out_ PCWSTR* KeyData)
{
    PINICACHEITERATOR Iterator;
    PLIST_ENTRY Entry;
    PINI_KEYWORD Key;

    if (!Section || !KeyName || !KeyData)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    Entry = Section->KeyList.Flink;
    if (Entry == &Section->KeyList)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }
    Key = CONTAINING_RECORD(Entry, INI_KEYWORD, ListEntry);

    Iterator = (PINICACHEITERATOR)RtlAllocateHeap(ProcessHeap,
                                                  0,
                                                  sizeof(INICACHEITERATOR));
    if (!Iterator)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }
    Iterator->Section = Section;
    Iterator->Key = Key;

    *KeyName = Key->Name;
    *KeyData = Key->Data;

    return Iterator;
}

BOOLEAN
IniFindNextValue(
    _In_ PINICACHEITERATOR Iterator,
    _Out_ PCWSTR* KeyName,
    _Out_ PCWSTR* KeyData)
{
    PLIST_ENTRY Entry;
    PINI_KEYWORD Key;

    if (!Iterator || !KeyName || !KeyData)
    {
        DPRINT("Invalid parameter\n");
        return FALSE;
    }

    Entry = Iterator->Key->ListEntry.Flink;
    if (Entry == &Iterator->Section->KeyList)
    {
        DPRINT("No more entries\n");
        return FALSE;
    }
    Key = CONTAINING_RECORD(Entry, INI_KEYWORD, ListEntry);

    Iterator->Key = Key;

    *KeyName = Key->Name;
    *KeyData = Key->Data;

    return TRUE;
}

VOID
IniFindClose(
    _In_ PINICACHEITERATOR Iterator)
{
    if (!Iterator)
        return;
    RtlFreeHeap(ProcessHeap, 0, Iterator);
}


PINI_SECTION
IniAddSection(
    _In_ PINICACHE Cache,
    _In_ PCWSTR Name)
{
    if (!Cache || !Name || !*Name)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }
    return IniCacheAddSectionAorW(Cache, Name, wcslen(Name), TRUE);
}

VOID
IniRemoveSection(
    _In_ PINI_SECTION Section)
{
    if (!Section)
    {
        DPRINT("Invalid parameter\n");
        return;
    }
    IniCacheFreeSection(Section);
}

PINI_KEYWORD
IniInsertKey(
    _In_ PINI_SECTION Section,
    _In_ PINI_KEYWORD AnchorKey,
    _In_ INSERTION_TYPE InsertionType,
    _In_ PCWSTR Name,
    _In_ PCWSTR Data)
{
    if (!Section || !Name || !*Name || !Data || !*Data)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }
    return IniCacheAddKeyAorW(Section,
                              AnchorKey, InsertionType,
                              Name, wcslen(Name),
                              Data, wcslen(Data),
                              TRUE);
}

PINI_KEYWORD
IniAddKey(
    _In_ PINI_SECTION Section,
    _In_ PCWSTR Name,
    _In_ PCWSTR Data)
{
    return IniInsertKey(Section, NULL, INSERT_LAST, Name, Data);
}

VOID
IniRemoveKeyByName(
    _In_ PINI_SECTION Section,
    _In_ PCWSTR KeyName)
{
    PINI_KEYWORD Key;
    UNREFERENCED_PARAMETER(Section);

    Key = IniCacheFindKey(Section, KeyName);
    if (Key)
        IniCacheFreeKey(Key);
}

VOID
IniRemoveKey(
    _In_ PINI_SECTION Section,
    _In_ PINI_KEYWORD Key)
{
    UNREFERENCED_PARAMETER(Section);
    if (!Key)
    {
        DPRINT("Invalid parameter\n");
        return;
    }
    IniCacheFreeKey(Key);
}

PINICACHE
IniCacheCreate(VOID)
{
    PINICACHE Cache;

    /* Allocate inicache header */
    Cache = (PINICACHE)RtlAllocateHeap(ProcessHeap,
                                       HEAP_ZERO_MEMORY,
                                       sizeof(INICACHE));
    if (!Cache)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }
    InitializeListHead(&Cache->SectionList);

    return Cache;
}

NTSTATUS
IniCacheSaveByHandle(
    PINICACHE Cache,
    HANDLE FileHandle)
{
    NTSTATUS Status;
    PLIST_ENTRY Entry1, Entry2;
    PINI_SECTION Section;
    PINI_KEYWORD Key;
    ULONG BufferSize;
    PCHAR Buffer;
    PCHAR Ptr;
    ULONG Len;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER Offset;

    /* Calculate required buffer size */
    BufferSize = 0;
    Entry1 = Cache->SectionList.Flink;
    while (Entry1 != &Cache->SectionList)
    {
        Section = CONTAINING_RECORD(Entry1, INI_SECTION, ListEntry);
        BufferSize += (Section->Name ? wcslen(Section->Name) : 0)
                       + 4; /* "[]\r\n" */

        Entry2 = Section->KeyList.Flink;
        while (Entry2 != &Section->KeyList)
        {
            Key = CONTAINING_RECORD(Entry2, INI_KEYWORD, ListEntry);
            BufferSize += wcslen(Key->Name)
                          + (Key->Data ? wcslen(Key->Data) : 0)
                          + 3; /* "=\r\n" */
            Entry2 = Entry2->Flink;
        }

        Entry1 = Entry1->Flink;
        if (Entry1 != &Cache->SectionList)
            BufferSize += 2; /* Extra "\r\n" at end of each section */
    }

    DPRINT("BufferSize: %lu\n", BufferSize);

    /* Allocate file buffer with NULL-terminator */
    Buffer = (PCHAR)RtlAllocateHeap(ProcessHeap,
                                    HEAP_ZERO_MEMORY,
                                    BufferSize + 1);
    if (Buffer == NULL)
    {
        DPRINT1("RtlAllocateHeap() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Fill file buffer */
    Ptr = Buffer;
    Entry1 = Cache->SectionList.Flink;
    while (Entry1 != &Cache->SectionList)
    {
        Section = CONTAINING_RECORD(Entry1, INI_SECTION, ListEntry);
        Len = sprintf(Ptr, "[%S]\r\n", Section->Name);
        Ptr += Len;

        Entry2 = Section->KeyList.Flink;
        while (Entry2 != &Section->KeyList)
        {
            Key = CONTAINING_RECORD(Entry2, INI_KEYWORD, ListEntry);
            Len = sprintf(Ptr, "%S=%S\r\n", Key->Name, Key->Data);
            Ptr += Len;
            Entry2 = Entry2->Flink;
        }

        Entry1 = Entry1->Flink;
        if (Entry1 != &Cache->SectionList)
        {
            Len = sprintf(Ptr, "\r\n");
            Ptr += Len;
        }
    }

    /* Write to the INI file */
    Offset.QuadPart = 0LL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Buffer,
                         BufferSize,
                         &Offset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, Buffer);
        return Status;
    }

    RtlFreeHeap(ProcessHeap, 0, Buffer);
    return STATUS_SUCCESS;
}

NTSTATUS
IniCacheSave(
    PINICACHE Cache,
    PWCHAR FileName)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;

    /* Create the INI file */
    RtlInitUnicodeString(&Name, FileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          FILE_GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_SUPERSEDE,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY | FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
        return Status;
    }

    Status = IniCacheSaveByHandle(Cache, FileHandle);

    /* Close the INI file */
    NtClose(FileHandle);
    return Status;
}

/* EOF */
