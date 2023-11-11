/*
 * PROJECT:     ReactOS Tools
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Check Registry Utility I/O hive operation routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "chkreg.h"

/* DEFINES  *****************************************************************/

#define HIVE_DATA_TAG 'aDiH'

/* FUNCTIONS ****************************************************************/

BOOLEAN
NTAPI
CmpFileRead(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    OUT PVOID Buffer,
    IN SIZE_T BufferLength)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    FILE *File = CmHive->FileHandles[HFILE_TYPE_PRIMARY];
    if (fseek(File, *FileOffset, SEEK_SET) != 0)
    {
        return FALSE;
    }

    return (fread(Buffer, 1, BufferLength, File) == BufferLength);
}

BOOLEAN
NTAPI
CmpFileWrite(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    IN PVOID Buffer,
    IN SIZE_T BufferLength)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    FILE *File = CmHive->FileHandles[HFILE_TYPE_PRIMARY];
    if (fseek(File, *FileOffset, SEEK_SET) != 0)
    {
        return FALSE;
    }

    return (fwrite(Buffer, 1, BufferLength, File) == BufferLength);
}

BOOLEAN
NTAPI
CmpFileSetSize(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN ULONG FileSize,
    IN ULONG OldFileSize)
{
    return TRUE;
}

BOOLEAN
NTAPI
CmpFileFlush(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    PLARGE_INTEGER FileOffset,
    ULONG Length)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    FILE *File = CmHive->FileHandles[HFILE_TYPE_PRIMARY];
    return (fflush(File) == 0);
}

BOOLEAN
ChkRegOpenHive(
    IN PCSTR HiveName,
    IN BOOLEAN WriteToHive,
    OUT PVOID *HiveData)
{
    FILE *HiveHandle;
    INT FileSize;
    PVOID Buffer;

    HiveHandle = fopen(HiveName, (WriteToHive == TRUE) ? "rb+" : "rb");
    if (HiveHandle == NULL)
    {
        return FALSE;
    }

    if (fseek(HiveHandle, 0, SEEK_END) != 0)
    {
        fclose(HiveHandle);
        return FALSE;
    }

    FileSize = ftell(HiveHandle);
    if (fseek(HiveHandle, 0, SEEK_SET) != 0)
    {
        fclose(HiveHandle);
        return FALSE;
    }

    Buffer = CmpAllocate(sizeof(CMHIVE) + FileSize,
                         TRUE,
                         HIVE_DATA_TAG);
    if (Buffer == NULL)
    {
        fclose(HiveHandle);
        return FALSE;
    }

    if (fread(Buffer, 1, FileSize, HiveHandle) != FileSize)
    {
        CmpFree(Buffer, 0);
        fclose(HiveHandle);
        return FALSE;
    }

    *HiveData = Buffer;
    fclose(HiveHandle);
    return TRUE;
}

/* EOF */
