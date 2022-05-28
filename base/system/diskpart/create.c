/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/create.c
 * PURPOSE:         Manages all the partitions of the OS in an interactive way.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

#define NDEBUG
#include <debug.h>


BOOL
CreateExtendedPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PPARTENTRY PartEntry, NewPartEntry;
    PLIST_ENTRY ListEntry;
    ULONGLONG ullSize = 0ULL;
    ULONGLONG ullSectorCount;
#if 0
    ULONGLONG ullOffset = 0ULL;
    BOOL bNoErr = FALSE;
#endif
    INT i;
    PWSTR pszSuffix = NULL;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    for (i = 3; i < argc; i++)
    {
        if (HasPrefix(argv[i], L"size=", &pszSuffix))
        {
            /* size=<N> (MB) */
            DPRINT("Size : %s\n", pszSuffix);

            ullSize = _wcstoui64(pszSuffix, NULL, 10);
            if ((ullSize == 0) && (errno == ERANGE))
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE;
            }
        }
        else if (HasPrefix(argv[i], L"offset=", &pszSuffix))
        {
            /* offset=<N> (KB) */
            DPRINT("Offset : %s\n", pszSuffix);
            ConPuts(StdOut, L"The OFFSET option is not supported yet!\n");
#if 0
            ullOffset = _wcstoui64(pszSuffix, NULL, 10);
            if ((ullOffset == 0) && (errno == ERANGE))
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE;
            }
#endif
        }
        else if (HasPrefix(argv[i], L"align=", &pszSuffix))
        {
            /* align=<N> */
            DPRINT("Align : %s\n", pszSuffix);
            ConPuts(StdOut, L"The ALIGN option is not supported yet!\n");
#if 0
            bAlign = TRUE;
#endif
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NoErr\n", pszSuffix);
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }
    }

    DPRINT1("Size: %I64u\n", ullSize);
#if 0
    DPRINT1("Offset: %I64u\n", ullOffset);
#endif

    if (GetPrimaryPartitionCount(CurrentDisk) >= 4)
    {
        ConPuts(StdOut, L"No space left for an extended partition!\n");
        return TRUE;
    }

    if (CurrentDisk->ExtendedPartition != NULL)
    {
        ConPuts(StdOut, L"We already have an extended partition on this disk!\n");
        return TRUE;
    }

    if (ullSize != 0)
        ullSectorCount = (ullSize * 1024 * 1024) / CurrentDisk->BytesPerSector;
    else
        ullSectorCount = 0;

    DPRINT1("SectorCount: %I64u\n", ullSectorCount);

    ListEntry = CurrentDisk->PrimaryPartListHead.Blink;

    PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
    if (PartEntry->IsPartitioned)
    {
        ConPuts(StdOut, L"No disk space left for an extended partition!\n");
        return TRUE;
    }

    if (ullSectorCount == 0)
    {
        PartEntry->IsPartitioned = TRUE;
        PartEntry->New = TRUE;
        PartEntry->PartitionType = PARTITION_EXTENDED;
        PartEntry->FormatState = Unformatted;
        PartEntry->FileSystemName[0] = L'\0';

        CurrentDisk->Dirty = TRUE;
    }
    else
    {
        if (PartEntry->SectorCount.QuadPart == ullSectorCount)
        {
            PartEntry->IsPartitioned = TRUE;
            PartEntry->New = TRUE;
            PartEntry->PartitionType = PARTITION_EXTENDED;
            PartEntry->FormatState = Unformatted;
            PartEntry->FileSystemName[0] = L'\0';

            CurrentDisk->Dirty = TRUE;
        }
        else if (PartEntry->SectorCount.QuadPart > ullSectorCount)
        {
            NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PPARTENTRY));
            if (NewPartEntry == NULL)
            {
                ConPuts(StdOut, L"Memory allocation failed!\n");
                return TRUE;
            }

            NewPartEntry->DiskEntry = PartEntry->DiskEntry;

            NewPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
            NewPartEntry->SectorCount.QuadPart = ullSectorCount;

            NewPartEntry->LogicalPartition = FALSE;
            NewPartEntry->IsPartitioned = TRUE;
            NewPartEntry->New = TRUE;
            NewPartEntry->PartitionType = PARTITION_EXTENDED;
            NewPartEntry->FormatState = Unformatted;
            NewPartEntry->FileSystemName[0] = L'\0';

            PartEntry->StartSector.QuadPart += ullSectorCount;
            PartEntry->SectorCount.QuadPart -= ullSectorCount;

            InsertTailList(ListEntry, &NewPartEntry->ListEntry);

            CurrentDisk->Dirty = TRUE;
        }
    }

    UpdateDiskLayout(CurrentDisk);
    WritePartitions(CurrentDisk);

    return TRUE;
}


BOOL
CreateLogicalPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    ConPrintf(StdOut, L"Not implemented yet!\n");

    return TRUE;
}


BOOL
CreatePrimaryPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PPARTENTRY PartEntry, NewPartEntry;
    PLIST_ENTRY ListEntry;
    ULONGLONG ullSize = 0ULL;
    ULONGLONG ullSectorCount;
#if 0
    ULONGLONG ullOffset = 0ULL;
    BOOL bNoErr = FALSE;
#endif
    UCHAR PartitionType = 6;
    INT i, length;
    PWSTR pszSuffix = NULL;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return TRUE;
    }

    for (i = 3; i < argc; i++)
    {
        if (HasPrefix(argv[i], L"size=", &pszSuffix))
        {
            /* size=<N> (MB) */
            DPRINT("Size : %s\n", pszSuffix);

            ullSize = _wcstoui64(pszSuffix, NULL, 10);
            if ((ullSize == 0) && (errno == ERANGE))
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE;
            }
        }
        else if (HasPrefix(argv[i], L"offset=", &pszSuffix))
        {
            /* offset=<N> (KB) */
            DPRINT("Offset : %s\n", pszSuffix);
            ConPuts(StdOut, L"The OFFSET option is not supported yet!\n");
#if 0
            ullOffset = _wcstoui64(pszSuffix, NULL, 10);
            if ((ullOffset == 0) && (errno == ERANGE))
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE;
            }
#endif
        }
        else if (HasPrefix(argv[i], L"id=", &pszSuffix))
        {
            /* id=<Byte>|<GUID> */
            DPRINT("Id : %s\n", pszSuffix);

            length = wcslen(pszSuffix);
            if ((length == 1) || (length == 2))
            {
                /* Byte */
                PartitionType = (UCHAR)wcstoul(pszSuffix, NULL, 16);
                if ((PartitionType == 0) && (errno == ERANGE))
                {
                    ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                    return TRUE;
                }
            }
#if 0
            else if ()
            {
                /* GUID */
            }
#endif
            else
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return TRUE; 
            }
        }
        else if (HasPrefix(argv[i], L"align=", &pszSuffix))
        {
            /* align=<N> */
            DPRINT("Align : %s\n", pszSuffix);
            ConPuts(StdOut, L"The ALIGN option is not supported yet!\n");
#if 0
            bAlign = TRUE;
#endif
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NoErr\n", pszSuffix);
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return TRUE;
        }
    }

    DPRINT1("Size: %I64u\n", ullSize);
#if 0
    DPRINT1("Offset: %I64u\n", ullOffset);
#endif
    DPRINT1("Partition Type: %hx\n", PartitionType);

    if (GetPrimaryPartitionCount(CurrentDisk) >= 4)
    {
        ConPuts(StdOut, L"No space left for another primary partition!\n");
        return TRUE;
    }

    if (ullSize != 0)
        ullSectorCount = (ullSize * 1024 * 1024) / CurrentDisk->BytesPerSector;
    else
        ullSectorCount = 0;

    DPRINT1("SectorCount: %I64u\n", ullSectorCount);

    for (ListEntry = CurrentDisk->PrimaryPartListHead.Flink;
         ListEntry != &CurrentDisk->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
        if (PartEntry->IsPartitioned)
            continue;

        if (ullSectorCount == 0)
        {
            PartEntry->IsPartitioned = TRUE;
            PartEntry->New = TRUE;
            PartEntry->PartitionType = PartitionType;
            PartEntry->FormatState = Unformatted;
            PartEntry->FileSystemName[0] = L'\0';

            CurrentDisk->Dirty = TRUE;
            break;
        }
        else
        {
            if (PartEntry->SectorCount.QuadPart == ullSectorCount)
            {
                PartEntry->IsPartitioned = TRUE;
                PartEntry->New = TRUE;
                PartEntry->PartitionType = PartitionType;
                PartEntry->FormatState = Unformatted;
                PartEntry->FileSystemName[0] = L'\0';

                CurrentDisk->Dirty = TRUE;
                break;
            }
            else if (PartEntry->SectorCount.QuadPart > ullSectorCount)
            {
                NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PPARTENTRY));
                if (NewPartEntry == NULL)
                {
                    ConPuts(StdOut, L"Memory allocation failed!\n");
                    return TRUE;
                }

                NewPartEntry->DiskEntry = PartEntry->DiskEntry;

                NewPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
                NewPartEntry->SectorCount.QuadPart = ullSectorCount;

                NewPartEntry->LogicalPartition = FALSE;
                NewPartEntry->IsPartitioned = TRUE;
                NewPartEntry->New = TRUE;
                NewPartEntry->PartitionType = PartitionType;
                NewPartEntry->FormatState = Unformatted;
                NewPartEntry->FileSystemName[0] = L'\0';

                PartEntry->StartSector.QuadPart += ullSectorCount;
                PartEntry->SectorCount.QuadPart -= ullSectorCount;

                InsertTailList(ListEntry, &NewPartEntry->ListEntry);

                CurrentDisk->Dirty = TRUE;
                break;
            }
        }
    }

    UpdateDiskLayout(CurrentDisk);
    WritePartitions(CurrentDisk);

    return TRUE;
}
