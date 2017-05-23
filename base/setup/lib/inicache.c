/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/inicache.c
 * PURPOSE:         INI file parser that caches contents of INI file in memory
 * PROGRAMMER:      Royce Mitchell III
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "inicache.h"

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS ********************************************************/

static
PINICACHEKEY
IniCacheFreeKey(
    PINICACHEKEY Key)
{
    PINICACHEKEY Next;

    if (Key == NULL)
        return NULL;

    Next = Key->Next;
    if (Key->Name != NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, Key->Name);
        Key->Name = NULL;
    }

    if (Key->Data != NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, Key->Data);
        Key->Data = NULL;
    }

    RtlFreeHeap(ProcessHeap, 0, Key);

    return Next;
}


static
PINICACHESECTION
IniCacheFreeSection(
    PINICACHESECTION Section)
{
    PINICACHESECTION Next;

    if (Section == NULL)
        return NULL;

    Next = Section->Next;
    while (Section->FirstKey != NULL)
    {
        Section->FirstKey = IniCacheFreeKey(Section->FirstKey);
    }
    Section->LastKey = NULL;

    if (Section->Name != NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, Section->Name);
        Section->Name = NULL;
    }

    RtlFreeHeap(ProcessHeap, 0, Section);

    return Next;
}


static
PINICACHEKEY
IniCacheFindKey(
     PINICACHESECTION Section,
     PWCHAR Name,
     ULONG NameLength)
{
    PINICACHEKEY Key;

    Key = Section->FirstKey;
    while (Key != NULL)
    {
        if (NameLength == wcslen(Key->Name))
        {
            if (_wcsnicmp(Key->Name, Name, NameLength) == 0)
                break;
        }

        Key = Key->Next;
    }

    return Key;
}


static
PINICACHEKEY
IniCacheAddKey(
    PINICACHESECTION Section,
    PCHAR Name,
    ULONG NameLength,
    PCHAR Data,
    ULONG DataLength)
{
    PINICACHEKEY Key;
    ULONG i;

    Key = NULL;

    if (Section == NULL ||
        Name == NULL ||
        NameLength == 0 ||
        Data == NULL ||
        DataLength == 0)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    Key = (PINICACHEKEY)RtlAllocateHeap(ProcessHeap,
                                        HEAP_ZERO_MEMORY,
                                        sizeof(INICACHEKEY));
    if (Key == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }

    Key->Name = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                        0,
                                        (NameLength + 1) * sizeof(WCHAR));
    if (Key->Name == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, Key);
        return NULL;
    }

    /* Copy value name */
    for (i = 0; i < NameLength; i++)
    {
        Key->Name[i] = (WCHAR)Name[i];
    }
    Key->Name[NameLength] = 0;

    Key->Data = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                        0,
                                        (DataLength + 1) * sizeof(WCHAR));
    if (Key->Data == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, Key->Name);
        RtlFreeHeap(ProcessHeap, 0, Key);
        return NULL;
    }

    /* Copy value data */
    for (i = 0; i < DataLength; i++)
    {
        Key->Data[i] = (WCHAR)Data[i];
    }
    Key->Data[DataLength] = 0;


    if (Section->FirstKey == NULL)
    {
        Section->FirstKey = Key;
        Section->LastKey = Key;
    }
    else
    {
        Section->LastKey->Next = Key;
        Key->Prev = Section->LastKey;
        Section->LastKey = Key;
    }

  return Key;
}


static
PINICACHESECTION
IniCacheAddSection(
    PINICACHE Cache,
    PCHAR Name,
    ULONG NameLength)
{
    PINICACHESECTION Section = NULL;
    ULONG i;

    if (Cache == NULL || Name == NULL || NameLength == 0)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    Section = (PINICACHESECTION)RtlAllocateHeap(ProcessHeap,
                                                HEAP_ZERO_MEMORY,
                                                sizeof(INICACHESECTION));
    if (Section == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }

    /* Allocate and initialize section name */
    Section->Name = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                            0,
                                            (NameLength + 1) * sizeof(WCHAR));
    if (Section->Name == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, Section);
        return NULL;
    }

    /* Copy section name */
    for (i = 0; i < NameLength; i++)
    {
        Section->Name[i] = (WCHAR)Name[i];
    }
    Section->Name[NameLength] = 0;

    /* Append section */
    if (Cache->FirstSection == NULL)
    {
        Cache->FirstSection = Section;
        Cache->LastSection = Section;
    }
    else
    {
        Cache->LastSection->Next = Section;
        Section->Prev = Cache->LastSection;
        Cache->LastSection = Section;
    }

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
    CHAR Name[256];

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

    strncpy(Name, *NamePtr, Size);
    Name[Size] = 0;

    DPRINT("SectionName: '%s'\n", Name);

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

    while(Ptr && *Ptr)
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

    PINICACHESECTION Section;
    PINICACHEKEY Key;

    PCHAR SectionName;
    ULONG SectionNameSize;

    PCHAR KeyName;
    ULONG KeyNameSize;

    PCHAR KeyValue;
    ULONG KeyValueSize;

    /* Allocate inicache header */
    *Cache = (PINICACHE)RtlAllocateHeap(ProcessHeap,
                                        HEAP_ZERO_MEMORY,
                                        sizeof(INICACHE));
    if (*Cache == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

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

            DPRINT1("[%.*s]\n", SectionNameSize, SectionName);

            Section = IniCacheAddSection(*Cache,
                                         SectionName,
                                         SectionNameSize);
            if (Section == NULL)
            {
                DPRINT("IniCacheAddSection() failed\n");
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

            DPRINT1("'%.*s' = '%.*s'\n", KeyNameSize, KeyName, KeyValueSize, KeyValue);

            Key = IniCacheAddKey(Section,
                                 KeyName,
                                 KeyNameSize,
                                 KeyValue,
                                 KeyValueSize);
            if (Key == NULL)
            {
                DPRINT("IniCacheAddKey() failed\n");
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
IniCacheLoad(
    PINICACHE *Cache,
    PWCHAR FileName,
    BOOLEAN String)
{
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_STANDARD_INFORMATION FileInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    NTSTATUS Status;
    PCHAR FileBuffer;
    ULONG FileLength;
    LARGE_INTEGER FileOffset;

    *Cache = NULL;

    /* Open ini file */
    RtlInitUnicodeString(&Name, FileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
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

    /* Query file size */
    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
        NtClose(FileHandle);
        return Status;
    }

    FileLength = FileInfo.EndOfFile.u.LowPart;

    DPRINT("File size: %lu\n", FileLength);

    /* Allocate file buffer with NULL-terminator */
    FileBuffer = (CHAR*)RtlAllocateHeap(ProcessHeap,
                                        0,
                                        FileLength + 1);
    if (FileBuffer == NULL)
    {
        DPRINT1("RtlAllocateHeap() failed\n");
        NtClose(FileHandle);
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

    NtClose(FileHandle);

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


VOID
IniCacheDestroy(
    PINICACHE Cache)
{
    if (Cache == NULL)
        return;

    while (Cache->FirstSection != NULL)
    {
        Cache->FirstSection = IniCacheFreeSection(Cache->FirstSection);
    }
    Cache->LastSection = NULL;

    RtlFreeHeap(ProcessHeap, 0, Cache);
}


PINICACHESECTION
IniCacheGetSection(
    PINICACHE Cache,
    PWCHAR Name)
{
    PINICACHESECTION Section = NULL;

    if (Cache == NULL || Name == NULL)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    /* Iterate through list of sections */
    Section = Cache->FirstSection;
    while (Section != NULL)
    {
        DPRINT("Comparing '%S' and '%S'\n", Section->Name, Name);

        /* Are the section names the same? */
        if (_wcsicmp(Section->Name, Name) == 0)
            return Section;

        /* Get the next section */
        Section = Section->Next;
    }

    DPRINT("Section not found\n");

    return NULL;
}


NTSTATUS
IniCacheGetKey(
    PINICACHESECTION Section,
    PWCHAR KeyName,
    PWCHAR *KeyData)
{
    PINICACHEKEY Key;

    if (Section == NULL || KeyName == NULL || KeyData == NULL)
    {
        DPRINT("Invalid parameter\n");
        return STATUS_INVALID_PARAMETER;
    }

    *KeyData = NULL;

    Key = IniCacheFindKey(Section, KeyName, wcslen(KeyName));
    if (Key == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *KeyData = Key->Data;

    return STATUS_SUCCESS;
}


PINICACHEITERATOR
IniCacheFindFirstValue(
    PINICACHESECTION Section,
    PWCHAR *KeyName,
    PWCHAR *KeyData)
{
    PINICACHEITERATOR Iterator;
    PINICACHEKEY Key;

    if (Section == NULL || KeyName == NULL || KeyData == NULL)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    Key = Section->FirstKey;
    if (Key == NULL)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    *KeyName = Key->Name;
    *KeyData = Key->Data;

    Iterator = (PINICACHEITERATOR)RtlAllocateHeap(ProcessHeap,
                                                  0,
                                                  sizeof(INICACHEITERATOR));
    if (Iterator == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }

    Iterator->Section = Section;
    Iterator->Key = Key;

    return Iterator;
}


BOOLEAN
IniCacheFindNextValue(
    PINICACHEITERATOR Iterator,
    PWCHAR *KeyName,
    PWCHAR *KeyData)
{
    PINICACHEKEY Key;

    if (Iterator == NULL || KeyName == NULL || KeyData == NULL)
    {
        DPRINT("Invalid parameter\n");
        return FALSE;
    }

    Key = Iterator->Key->Next;
    if (Key == NULL)
    {
        DPRINT("No more entries\n");
        return FALSE;
    }

    *KeyName = Key->Name;
    *KeyData = Key->Data;

    Iterator->Key = Key;

    return TRUE;
}


VOID
IniCacheFindClose(
    PINICACHEITERATOR Iterator)
{
    if (Iterator == NULL)
        return;

    RtlFreeHeap(ProcessHeap, 0, Iterator);
}


PINICACHEKEY
IniCacheInsertKey(
    PINICACHESECTION Section,
    PINICACHEKEY AnchorKey,
    INSERTION_TYPE InsertionType,
    PWCHAR Name,
    PWCHAR Data)
{
    PINICACHEKEY Key;

    Key = NULL;

    if (Section == NULL ||
        Name == NULL ||
        *Name == 0 ||
        Data == NULL ||
        *Data == 0)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    /* Allocate key buffer */
    Key = (PINICACHEKEY)RtlAllocateHeap(ProcessHeap,
                                        HEAP_ZERO_MEMORY,
                                        sizeof(INICACHEKEY));
    if (Key == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }

    /* Allocate name buffer */
    Key->Name = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                        0,
                                        (wcslen(Name) + 1) * sizeof(WCHAR));
    if (Key->Name == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, Key);
        return NULL;
    }

    /* Copy value name */
    wcscpy(Key->Name, Name);

    /* Allocate data buffer */
    Key->Data = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                        0,
                                        (wcslen(Data) + 1) * sizeof(WCHAR));
    if (Key->Data == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, Key->Name);
        RtlFreeHeap(ProcessHeap, 0, Key);
        return NULL;
    }

    /* Copy value data */
    wcscpy(Key->Data, Data);

    /* Insert key into section */
    if (Section->FirstKey == NULL)
    {
        Section->FirstKey = Key;
        Section->LastKey = Key;
    }
    else if ((InsertionType == INSERT_FIRST) ||
             ((InsertionType == INSERT_BEFORE) && ((AnchorKey == NULL) || (AnchorKey == Section->FirstKey))))
    {
        /* Insert at the head of the list */
        Section->FirstKey->Prev = Key;
        Key->Next = Section->FirstKey;
        Section->FirstKey = Key;
    }
    else if ((InsertionType == INSERT_BEFORE) && (AnchorKey != NULL))
    {
        /* Insert before the anchor key */
        Key->Next = AnchorKey;
        Key->Prev = AnchorKey->Prev;
        AnchorKey->Prev->Next = Key;
        AnchorKey->Prev = Key;
    }
    else if ((InsertionType == INSERT_LAST) ||
             ((InsertionType == INSERT_AFTER) && ((AnchorKey == NULL) || (AnchorKey == Section->LastKey))))
    {
        Section->LastKey->Next = Key;
        Key->Prev = Section->LastKey;
        Section->LastKey = Key;
    }
    else if ((InsertionType == INSERT_AFTER) && (AnchorKey != NULL))
    {
        /* Insert after the anchor key */
        Key->Next = AnchorKey->Next;
        Key->Prev = AnchorKey;
        AnchorKey->Next->Prev = Key;
        AnchorKey->Next = Key;
    }

    return Key;
}


PINICACHE
IniCacheCreate(VOID)
{
    PINICACHE Cache;

    /* Allocate inicache header */
    Cache = (PINICACHE)RtlAllocateHeap(ProcessHeap,
                                       HEAP_ZERO_MEMORY,
                                       sizeof(INICACHE));
    if (Cache == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }

    return Cache;
}


NTSTATUS
IniCacheSave(
    PINICACHE Cache,
    PWCHAR FileName)
{
    UNICODE_STRING Name;
    PINICACHESECTION Section;
    PINICACHEKEY Key;
    ULONG BufferSize;
    PCHAR Buffer;
    PCHAR Ptr;
    ULONG Len;
    NTSTATUS Status;

    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER Offset;
    HANDLE FileHandle;


    /* Calculate required buffer size */
    BufferSize = 0;
    Section = Cache->FirstSection;
    while (Section != NULL)
    {
        BufferSize += (Section->Name ? wcslen(Section->Name) : 0)
                       + 4; /* "[]\r\n" */

        Key = Section->FirstKey;
        while (Key != NULL)
        {
            BufferSize += wcslen(Key->Name)
                          + (Key->Data ? wcslen(Key->Data) : 0)
                          + 3; /* "=\r\n" */
            Key = Key->Next;
        }

        Section = Section->Next;
        if (Section != NULL)
            BufferSize += 2; /* Extra "\r\n" at end of each section */
    }

    DPRINT("BufferSize: %lu\n", BufferSize);

    /* Allocate file buffer with NULL-terminator */
    Buffer = (CHAR*)RtlAllocateHeap(ProcessHeap,
                                    HEAP_ZERO_MEMORY,
                                    BufferSize + 1);
    if (Buffer == NULL)
    {
        DPRINT1("RtlAllocateHeap() failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Fill file buffer */
    Ptr = Buffer;
    Section = Cache->FirstSection;
    while (Section != NULL)
    {
        Len = sprintf(Ptr, "[%S]\r\n", Section->Name);
        Ptr += Len;

        Key = Section->FirstKey;
        while (Key != NULL)
        {
            Len = sprintf(Ptr, "%S=%S\r\n", Key->Name, Key->Data);
            Ptr += Len;
            Key = Key->Next;
        }

        Section = Section->Next;
        if (Section != NULL)
        {
            Len = sprintf(Ptr, "\r\n");
            Ptr += Len;
        }
    }

    /* Create ini file */
    RtlInitUnicodeString(&Name, FileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_SUPERSEDE,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtCreateFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, Buffer);
        return Status;
    }

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
      NtClose(FileHandle);
      RtlFreeHeap(ProcessHeap, 0, Buffer);
      return Status;
    }

    NtClose(FileHandle);

    RtlFreeHeap(ProcessHeap, 0, Buffer);

    return STATUS_SUCCESS;
}


PINICACHESECTION
IniCacheAppendSection(
    PINICACHE Cache,
    PWCHAR Name)
{
    PINICACHESECTION Section = NULL;

    if (Cache == NULL || Name == NULL || *Name == 0)
    {
        DPRINT("Invalid parameter\n");
        return NULL;
    }

    Section = (PINICACHESECTION)RtlAllocateHeap(ProcessHeap,
                                                HEAP_ZERO_MEMORY,
                                                sizeof(INICACHESECTION));
    if (Section == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        return NULL;
    }

    /* Allocate and initialize section name */
    Section->Name = (WCHAR*)RtlAllocateHeap(ProcessHeap,
                                            0,
                                            (wcslen(Name) + 1) * sizeof(WCHAR));
    if (Section->Name == NULL)
    {
        DPRINT("RtlAllocateHeap() failed\n");
        RtlFreeHeap(ProcessHeap, 0, Section);
        return NULL;
    }

    /* Copy section name */
    wcscpy(Section->Name, Name);

    /* Append section */
    if (Cache->FirstSection == NULL)
    {
        Cache->FirstSection = Section;
        Cache->LastSection = Section;
    }
    else
    {
        Cache->LastSection->Next = Section;
        Section->Prev = Cache->LastSection;
        Cache->LastSection = Section;
    }

    return Section;
}

/* EOF */
