/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Generic BootCode support functions.
 * COPYRIGHT:   Copyright 2020-2024 Hermès Bélusca-Maïto <hermes.belusca@sfr.fr>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "bootcode.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

NTSTATUS
ReadBootCodeByHandle(
    _Inout_ PBOOTCODE BootCode,
    _In_ HANDLE FileHandle,
    _In_opt_ ULONG Length)
{
    NTSTATUS Status;
    PVOID Buffer;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;

    /* If no bootcode length was provided, get it via the file size */
    if (Length == 0 || Length == (ULONG)-1)
    {
        /* Query the file size */
        FILE_STANDARD_INFORMATION FileInfo;
        Status = NtQueryInformationFile(FileHandle,
                                        &IoStatusBlock,
                                        &FileInfo,
                                        sizeof(FileInfo),
                                        FileStandardInformation);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtQueryInformationFile() failed (Status 0x%08lx)\n", Status);
            FileInfo.EndOfFile.HighPart = 0;
            FileInfo.EndOfFile.LowPart = SECTORSIZE;
        }
        else if (FileInfo.EndOfFile.HighPart != 0)
        {
            DPRINT1("WARNING!! Bootsector file is too large!\n");
            FileInfo.EndOfFile.HighPart = 0;
            FileInfo.EndOfFile.LowPart = SECTORSIZE;
        }

        Length = FileInfo.EndOfFile.LowPart;
        DPRINT("File size: %lu\n", Length);
    }

    /* Allocate a buffer for the bootcode */
    Buffer = RtlAllocateHeap(ProcessHeap, HEAP_ZERO_MEMORY, Length);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Read the bootcode from the file into the buffer */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Buffer,
                        Length,
                        &FileOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, Buffer);
        return Status;
    }

    /* Update the bootcode information */
    if (BootCode->BootCode)
        RtlFreeHeap(ProcessHeap, 0, BootCode->BootCode);
    BootCode->BootCode = Buffer;
    BootCode->Length = Length;

    return STATUS_SUCCESS;
}

NTSTATUS
ReadBootCodeFromFile(
    _Inout_ PBOOTCODE BootCode,
    _In_ PUNICODE_STRING FilePath,
    _In_opt_ ULONG Length)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;

    /* Open the file */
    InitializeObjectAttributes(&ObjectAttributes,
                               FilePath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, // Is FILE_SHARE_WRITE necessary?
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = ReadBootCodeByHandle(BootCode, FileHandle, Length);

    /* Close the file and return */
    NtClose(FileHandle);
    return Status;
}

VOID
FreeBootCode(
    _Inout_ PBOOTCODE BootCode)
{
    /* Free the bootcode information */
    if (BootCode->BootCode)
        RtlFreeHeap(ProcessHeap, 0, BootCode->BootCode);
    BootCode->BootCode = NULL;
    BootCode->Length = 0;
}

BOOLEAN
CompareBootCodes(
    _In_ PBOOTCODE BootCode1,
    _In_ PBOOTCODE BootCode2,
    _In_ PBOOTCODE_REGION ExcludeRegions)
{
    PUCHAR Buffer1, Buffer2;
    PBOOTCODE_REGION Region;
    BOOLEAN AreTheSame;

    /* If they have different lengths, they cannot be the same */
    if (BootCode1->Length != BootCode2->Length)
        return FALSE;

    /* If both are of zero length they are automatically equal */
    if (BootCode1->Length == 0 /* && BootCode2->Length == 0 */)
        return TRUE;

    /* Allocate buffers for bootcode copies */
    Buffer1 = RtlAllocateHeap(ProcessHeap, 0, BootCode1->Length);
    if (!Buffer1)
        return FALSE; // STATUS_INSUFFICIENT_RESOURCES;
    RtlCopyMemory(Buffer1, BootCode1->BootCode, BootCode1->Length);

    Buffer2 = RtlAllocateHeap(ProcessHeap, 0, BootCode2->Length);
    if (!Buffer2)
    {
        RtlFreeHeap(ProcessHeap, 0, Buffer1);
        return FALSE; // STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(Buffer2, BootCode2->BootCode, BootCode2->Length);

    /* Exclude the regions by zeroing them out */
    for (Region = ExcludeRegions;
         Region->Offset != 0 && Region->Length != 0;
         Region++)
    {
        ASSERT(Region->Offset + Region->Length <= BootCode1->Length);
        RtlZeroMemory(Buffer1 + Region->Offset, Region->Length);
        RtlZeroMemory(Buffer2 + Region->Offset, Region->Length);
    }

    /* Compare the bootcodes */
    AreTheSame = RtlEqualMemory(Buffer1, Buffer2, BootCode1->Length);

    /* Free the bootcode copies */
    RtlFreeHeap(ProcessHeap, 0, Buffer2);
    RtlFreeHeap(ProcessHeap, 0, Buffer1);

    return AreTheSame;
}

/**
 * @brief   Finds whether a sub-buffer is present in the given boot code.
 * @note    Similar to GNU memmem().
 **/
PUCHAR
FindInBootCode(
    _In_ PBOOTCODE BootCode,
    _In_ PUCHAR Buffer,
    _In_ SIZE_T Length)
{
    PUCHAR start = (PUCHAR)BootCode->BootCode;
    SIZE_T remain = BootCode->Length;
    const PUCHAR endBuf1 = start + remain - Length + 1;
    const PUCHAR endBuf2 = Buffer + Length;

    /* No wrapping allowed */
    ASSERT(start  <= start + remain);
    ASSERT(Buffer <= endBuf2);

    /* The first occurrence of the empty buffer is deemed
     * to occur at the beginning of the boot code */
    if (Length == 0)
        return start;

    /* If the sub-buffer is larger than the boot code, bail out */
    if (remain < Length)
        return NULL;

    while (start < endBuf1)
    {
        PUCHAR p1 = start, p2 = Buffer;
        while ((p1 < endBuf1) && (p2 < endBuf2) && (*p1 == *p2))
            { ++p1, ++p2; }
        if (p2 == endBuf2)
            return start;
        ++start;
    }
    return NULL;
}

/* EOF */
