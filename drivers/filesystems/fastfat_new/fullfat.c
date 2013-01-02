/*
 * PROJECT:         ReactOS FAT file system driver
 * LICENSE:         GNU GPLv3 as published by the Free Software Foundation
 * FILE:            drivers/filesystems/fastfat/fullfat.c
 * PURPOSE:         FullFAT integration routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "fastfat.h"

/* GLOBALS ******************************************************************/

#define TAG_FULLFAT 'FLUF'

/* FUNCTIONS ****************************************************************/

VOID *
FF_Malloc(FF_T_UINT32 allocSize)
{
    return ExAllocatePoolWithTag(PagedPool, allocSize, TAG_FULLFAT);
}

VOID
FF_Free(VOID *pBuffer)
{
    ExFreePoolWithTag(pBuffer, TAG_FULLFAT);
}

FF_T_SINT32
FatWriteBlocks(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam)
{
    DPRINT1("FatWriteBlocks %p %d %d %p\n", pBuffer, SectorAddress, Count, pParam);

    return 0;
}

FF_T_SINT32
FatReadBlocks(FF_T_UINT8 *DestBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam)
{
    LARGE_INTEGER Offset;
    //PVOID Buffer;
    PVCB Vcb = (PVCB)pParam;
    //PBCB Bcb;
    ULONG SectorSize = 512; // FIXME: hardcoding 512 is bad
    IO_STATUS_BLOCK IoSb;

    DPRINT("FatReadBlocks %p %d %d %p\n", DestBuffer, SectorAddress, Count, pParam);

    /* Calculate the offset */
    Offset.QuadPart = Int32x32To64(SectorAddress, SectorSize);
#if 0
    if (!CcMapData(Vcb->StreamFileObject,
                  &Offset,
                  Count * SectorSize,
                  TRUE,
                  &Bcb,
                  &Buffer))
    {
        ASSERT(FALSE);
        /* Mapping failed */
        return 0;
    }

    /* Copy data to the buffer */
    RtlCopyMemory(DestBuffer, Buffer, Count * SectorSize);

    /* Unpin unneeded data */
    CcUnpinData(Bcb);
#else
    CcCopyRead(Vcb->StreamFileObject, &Offset, Count * SectorSize, TRUE, DestBuffer, &IoSb);
#endif

    /* Return amount of read data in sectors */
    return Count;
}

FF_FILE *FF_OpenW(FF_IOMAN *pIoman, PUNICODE_STRING pathW, FF_T_UINT8 Mode, FF_ERROR *pError)
{
    OEM_STRING AnsiName;
    CHAR AnsiNameBuf[512];
    NTSTATUS Status;

    /* Convert the name to ANSI */
    AnsiName.Buffer = AnsiNameBuf;
    AnsiName.Length = 0;
    AnsiName.MaximumLength = sizeof(AnsiNameBuf);
    RtlZeroMemory(AnsiNameBuf, sizeof(AnsiNameBuf));
    Status = RtlUpcaseUnicodeStringToCountedOemString(&AnsiName, pathW, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
    }

    DPRINT1("Opening '%s'\n", AnsiName.Buffer);

    /* Call FullFAT's handler */
    return FF_Open(pIoman, AnsiName.Buffer, Mode, pError);
}

FORCEINLINE
VOID
FatDateTimeToSystemTime(OUT PLARGE_INTEGER SystemTime,
                        IN PFAT_DATETIME FatDateTime,
                        IN UCHAR TenMs OPTIONAL)
{
    TIME_FIELDS TimeFields;

    /* Setup time fields */
    TimeFields.Year = FatDateTime->Date.Year + 1980;
    TimeFields.Month = FatDateTime->Date.Month;
    TimeFields.Day = FatDateTime->Date.Day;
    TimeFields.Hour = FatDateTime->Time.Hour;
    TimeFields.Minute = FatDateTime->Time.Minute;
    TimeFields.Second = (FatDateTime->Time.DoubleSeconds << 1);

    /* Adjust up to 10 milliseconds
     * if the parameter was supplied
     */
    if (ARGUMENT_PRESENT(TenMs))
    {
        TimeFields.Second += TenMs / 100;
        TimeFields.Milliseconds = (TenMs % 100) * 10;
    }
    else
    {
        TimeFields.Milliseconds = 0;
    }

    /* Fix seconds value that might get beyoud the bound */
    if (TimeFields.Second > 59) TimeFields.Second = 0;

    /* Perform ceonversion to system time if possible */
    if (RtlTimeFieldsToTime(&TimeFields, SystemTime))
    {
        /* Convert to system time */
        ExLocalTimeToSystemTime(SystemTime, SystemTime);
    }
    else
    {
        /* Set to default time if conversion failed */
        *SystemTime = FatGlobalData.DefaultFileTime;
    }
}

// TODO: Make it a helper around FullFAT library
VOID
NTAPI
FatQueryFileTimes(OUT PLARGE_INTEGER FileTimes,
                  IN PDIR_ENTRY Dirent)
{
    /* Convert LastWriteTime */
    FatDateTimeToSystemTime(&FileTimes[FileLastWriteTime],
                            &Dirent->LastWriteDateTime,
                            0);
    /* All other time fileds are valid (according to MS)
     * only if Win31 compatability mode is set.
     */
    if (FatGlobalData.Win31FileSystem)
    {
       /* We can avoid calling conversion routine
        * if time in dirent is 0 or equals to already
        * known time (LastWriteTime).
        */
        if (Dirent->CreationDateTime.Value == 0)
        {
            /* Set it to default time */
            FileTimes[FileCreationTime] = FatGlobalData.DefaultFileTime;
        }
        else if (Dirent->CreationDateTime.Value
            == Dirent->LastWriteDateTime.Value)
        {
            /* Assign the already known time */
            FileTimes[FileCreationTime] = FileTimes[FileLastWriteTime];
            /* Adjust milliseconds from extra dirent field */
            FileTimes[FileCreationTime].QuadPart
                += (ULONG) Dirent->CreationTimeTenMs * 100000;
        }
        else
        {
            /* Perform conversion */
            FatDateTimeToSystemTime(&FileTimes[FileCreationTime],
                                    &Dirent->CreationDateTime,
                                    Dirent->CreationTimeTenMs);
        }
        if (Dirent->LastAccessDate.Value == 0)
        {
            /* Set it to default time */
            FileTimes[FileLastAccessTime] = FatGlobalData.DefaultFileTime;
        }
        else if (Dirent->LastAccessDate.Value
                == Dirent->LastWriteDateTime.Date.Value)
        {
            /* Assign the already known time */
            FileTimes[FileLastAccessTime] = FileTimes[FileLastWriteTime];
        }
        else
        {
            /* Perform conversion */
            FAT_DATETIME LastAccessDateTime;

            LastAccessDateTime.Date.Value = Dirent->LastAccessDate.Value;
            LastAccessDateTime.Time.Value = 0;
            FatDateTimeToSystemTime(&FileTimes[FileLastAccessTime],
                                    &LastAccessDateTime,
                                    0);
        }
    }
}

/* EOF */
