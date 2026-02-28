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

static
PPARTENTRY
InsertGptPartition(
    _In_ PDISKENTRY pDisk,
    _In_ ULONGLONG ullSectorSize,
    _In_ const GUID *pPartitionType)
{
    PPARTENTRY PartEntry, NewPartEntry;
    PLIST_ENTRY ListEntry;

    for (ListEntry = pDisk->PrimaryPartListHead.Flink;
         ListEntry != &CurrentDisk->PrimaryPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
        if (PartEntry->IsPartitioned)
            continue;

        if (ullSectorSize == 0ULL)
        {
            DPRINT("Claim whole unused space!\n");
            PartEntry->IsPartitioned = TRUE;
            PartEntry->New = TRUE;
            CopyMemory(&PartEntry->Gpt.PartitionType, pPartitionType, sizeof(GUID));
            CreateGUID(&PartEntry->Gpt.PartitionId);
            PartEntry->Gpt.Attributes = 0ULL;
            PartEntry->PartitionNumber = 0;
            PartEntry->FormatState = Unformatted;
            PartEntry->FileSystemName[0] = L'\0';

            return PartEntry;
        }
        else
        {
            if (ullSectorSize == PartEntry->SectorCount.QuadPart)
            {
                DPRINT("Claim matching unused space!\n");
                PartEntry->IsPartitioned = TRUE;
                PartEntry->New = TRUE;
                CopyMemory(&PartEntry->Gpt.PartitionType, pPartitionType, sizeof(GUID));
                CreateGUID(&PartEntry->Gpt.PartitionId);
                PartEntry->Gpt.Attributes = 0ULL;
                PartEntry->PartitionNumber = 0;
                PartEntry->FormatState = Unformatted;
                PartEntry->FileSystemName[0] = L'\0';

                return PartEntry;
            }
            else if (ullSectorSize < PartEntry->SectorCount.QuadPart)
            {
                DPRINT("Claim part of unused space\n");
                NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PPARTENTRY));
                if (NewPartEntry == NULL)
                {
                    ConPuts(StdOut, L"Memory allocation failed!\n");
                    return NULL;
                }

                NewPartEntry->DiskEntry = PartEntry->DiskEntry;

                NewPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
                NewPartEntry->SectorCount.QuadPart = ullSectorSize;

                NewPartEntry->LogicalPartition = FALSE;
                NewPartEntry->IsPartitioned = TRUE;
                NewPartEntry->New = TRUE;
                CopyMemory(&NewPartEntry->Gpt.PartitionType, pPartitionType, sizeof(GUID));
                CreateGUID(&NewPartEntry->Gpt.PartitionId);
                NewPartEntry->Gpt.Attributes = 0ULL;
                NewPartEntry->PartitionNumber = 0;
                NewPartEntry->FormatState = Unformatted;
                NewPartEntry->FileSystemName[0] = L'\0';

                PartEntry->StartSector.QuadPart += ullSectorSize;
                PartEntry->SectorCount.QuadPart -= ullSectorSize;

                InsertTailList(ListEntry, &NewPartEntry->ListEntry);

                return NewPartEntry;
            }
        }
    }

    return NULL;
}


EXIT_CODE
CreateEfiPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PPARTENTRY PartEntry;
    ULONGLONG ullSize = 0ULL;
    ULONGLONG ullSectorCount;
#if 0
    BOOL bNoErr = FALSE;
#endif
    INT i;
    PWSTR pszSuffix = NULL;
    NTSTATUS Status;

    DPRINT1("CreateEfiPartition()\n");

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle != PARTITION_STYLE_GPT)
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_INVALID_STYLE);
        return EXIT_SUCCESS;
    }

    for (i = 3; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NoErr\n", pszSuffix);
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
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
                return EXIT_SUCCESS;
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
                return EXIT_SUCCESS;
            }
#endif
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr - Already handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }
    }

    DPRINT1("Size: %I64u\n", ullSize);
#if 0
    DPRINT1("Offset: %I64u\n", ullOffset);
#endif

    /* Size */
    if (ullSize != 0)
        ullSectorCount = (ullSize * 1024 * 1024) / CurrentDisk->BytesPerSector;
    else
        ullSectorCount = 0;

    DPRINT1("SectorCount: %I64u\n", ullSectorCount);

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(CurrentDisk);
#endif

    PartEntry = InsertGptPartition(CurrentDisk,
                                   ullSectorCount,
                                   &PARTITION_SYSTEM_GUID);
    if (PartEntry == FALSE)
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return EXIT_SUCCESS;
    }

    CurrentPartition = PartEntry;
    CurrentDisk->Dirty = TRUE;

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(CurrentDisk);
#endif

    UpdateGptDiskLayout(CurrentDisk, FALSE);
    Status = WriteGptPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return EXIT_SUCCESS;
    }

    ConResPuts(StdOut, IDS_CREATE_PARTITION_SUCCESS);

    return EXIT_SUCCESS;
}


EXIT_CODE
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
    NTSTATUS Status;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_INVALID_STYLE);
        return EXIT_SUCCESS;
    }
    else if (CurrentDisk->PartitionStyle == PARTITION_STYLE_RAW)
    {
        CREATE_DISK DiskInfo;
        NTSTATUS Status;

        DiskInfo.PartitionStyle = PARTITION_STYLE_MBR;
        CreateSignature(&DiskInfo.Mbr.Signature);

        Status = CreateDisk(CurrentDisk->DiskNumber, &DiskInfo);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreateDisk() failed!\n");
            return EXIT_SUCCESS;
        }

        CurrentDisk->StartSector.QuadPart = (ULONGLONG)CurrentDisk->SectorAlignment;
        CurrentDisk->EndSector.QuadPart = min(CurrentDisk->SectorCount.QuadPart, 0x100000000) - 1;

        ScanForUnpartitionedMbrDiskSpace(CurrentDisk);
    }

    for (i = 3; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NoErr\n", pszSuffix);
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
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
                return EXIT_SUCCESS;
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
                return EXIT_SUCCESS;
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
            /* noerr - Already handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }
    }

    DPRINT1("Size: %I64u\n", ullSize);
#if 0
    DPRINT1("Offset: %I64u\n", ullOffset);
#endif

    if (GetPrimaryPartitionCount(CurrentDisk) >= 4)
    {
        ConPuts(StdOut, L"No space left for an extended partition!\n");
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->ExtendedPartition != NULL)
    {
        ConPuts(StdOut, L"We already have an extended partition on this disk!\n");
        return EXIT_SUCCESS;
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
        return EXIT_SUCCESS;
    }

    if (ullSectorCount == 0)
    {
        PartEntry->IsPartitioned = TRUE;
        PartEntry->New = TRUE;
        PartEntry->Mbr.PartitionType = PARTITION_EXTENDED;
        PartEntry->FormatState = Unformatted;
        PartEntry->FileSystemName[0] = L'\0';

        CurrentPartition = PartEntry;
        CurrentDisk->Dirty = TRUE;
    }
    else
    {
        if (PartEntry->SectorCount.QuadPart == ullSectorCount)
        {
            PartEntry->IsPartitioned = TRUE;
            PartEntry->New = TRUE;
            PartEntry->Mbr.PartitionType = PARTITION_EXTENDED;
            PartEntry->FormatState = Unformatted;
            PartEntry->FileSystemName[0] = L'\0';

            CurrentPartition = PartEntry;
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
            NewPartEntry->Mbr.PartitionType = PARTITION_EXTENDED;
            NewPartEntry->FormatState = Unformatted;
            NewPartEntry->FileSystemName[0] = L'\0';

            PartEntry->StartSector.QuadPart += ullSectorCount;
            PartEntry->SectorCount.QuadPart -= ullSectorCount;

            InsertTailList(ListEntry, &NewPartEntry->ListEntry);

            CurrentPartition = NewPartEntry;
            CurrentDisk->Dirty = TRUE;
        }
    }

    UpdateMbrDiskLayout(CurrentDisk);
    Status = WriteMbrPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return EXIT_SUCCESS;
    }

    ConResPuts(StdOut, IDS_CREATE_PARTITION_SUCCESS);

    return EXIT_SUCCESS;
}


EXIT_CODE
CreateLogicalPartition(
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
    UCHAR PartitionType = PARTITION_HUGE;
    INT i, length;
    PWSTR pszSuffix = NULL;
    NTSTATUS Status;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle != PARTITION_STYLE_MBR)
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_INVALID_STYLE);
        return EXIT_SUCCESS;
    }

    for (i = 3; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NoErr\n", pszSuffix);
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
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
                return EXIT_SUCCESS;
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
                return EXIT_SUCCESS;
            }
#endif
        }
        else if (HasPrefix(argv[i], L"id=", &pszSuffix))
        {
            /* id=<Byte> */
            DPRINT("Id : %s\n", pszSuffix);

            length = wcslen(pszSuffix);
            if ((length == 1) || (length == 2))
            {
                /* Byte */
                PartitionType = (UCHAR)wcstoul(pszSuffix, NULL, 16);
                if ((PartitionType == 0) && (errno == ERANGE))
                {
                    ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                    return EXIT_SUCCESS;
                }
            }
            else
            {
                ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
                return EXIT_SUCCESS;
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
            /* noerr - Already handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }
    }

    DPRINT1("Size: %I64u\n", ullSize);
#if 0
    DPRINT1("Offset: %I64u\n", ullOffset);
#endif
    DPRINT1("Partition Type: %hx\n", PartitionType);

    if (ullSize != 0)
        ullSectorCount = (ullSize * 1024 * 1024) / CurrentDisk->BytesPerSector;
    else
        ullSectorCount = 0;

    DPRINT1("SectorCount: %I64u\n", ullSectorCount);

    for (ListEntry = CurrentDisk->LogicalPartListHead.Flink;
         ListEntry != &CurrentDisk->LogicalPartListHead;
         ListEntry = ListEntry->Flink)
    {
        PartEntry = CONTAINING_RECORD(ListEntry, PARTENTRY, ListEntry);
        if (PartEntry->IsPartitioned)
            continue;

        if (ullSectorCount == 0)
        {
            PartEntry->IsPartitioned = TRUE;
            PartEntry->New = TRUE;
            PartEntry->Mbr.PartitionType = PartitionType;
            PartEntry->FormatState = Unformatted;
            PartEntry->FileSystemName[0] = L'\0';

            CurrentPartition = PartEntry;
            CurrentDisk->Dirty = TRUE;
            break;
        }
        else
        {
            if (PartEntry->SectorCount.QuadPart == ullSectorCount)
            {
                PartEntry->IsPartitioned = TRUE;
                PartEntry->New = TRUE;
                PartEntry->Mbr.PartitionType = PartitionType;
                PartEntry->FormatState = Unformatted;
                PartEntry->FileSystemName[0] = L'\0';

                CurrentPartition = PartEntry;
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

                NewPartEntry->LogicalPartition = TRUE;
                NewPartEntry->IsPartitioned = TRUE;
                NewPartEntry->New = TRUE;
                NewPartEntry->Mbr.PartitionType = PartitionType;
                NewPartEntry->FormatState = Unformatted;
                NewPartEntry->FileSystemName[0] = L'\0';

                PartEntry->StartSector.QuadPart += ullSectorCount;
                PartEntry->SectorCount.QuadPart -= ullSectorCount;

                InsertTailList(ListEntry, &NewPartEntry->ListEntry);

                CurrentPartition = NewPartEntry;
                CurrentDisk->Dirty = TRUE;
                break;
            }
        }
    }

    UpdateMbrDiskLayout(CurrentDisk);
    Status = WriteMbrPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return EXIT_SUCCESS;
    }

    ConResPuts(StdOut, IDS_CREATE_PARTITION_SUCCESS);

    return EXIT_SUCCESS;
}


EXIT_CODE
CreateMsrPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    PPARTENTRY PartEntry;
    ULONGLONG ullSize = 0ULL;
    ULONGLONG ullSectorSize;
#if 0
    BOOL bNoErr = FALSE;
#endif
    INT i;
    PWSTR pszSuffix = NULL;
    NTSTATUS Status;

    DPRINT1("CreateMsrPartition()\n");

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle != PARTITION_STYLE_GPT)
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_INVALID_STYLE);
        return EXIT_SUCCESS;
    }

    for (i = 3; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NoErr\n", pszSuffix);
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
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
                return EXIT_SUCCESS;
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
                return EXIT_SUCCESS;
            }
#endif
        }
        else if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr - Already handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }
    }

    DPRINT1("Size: %I64u\n", ullSize);
#if 0
    DPRINT1("Offset: %I64u\n", ullOffset);
#endif

    /* Size */
    if (ullSize != 0)
        ullSectorSize = (ullSize * 1024 * 1024) / CurrentDisk->BytesPerSector;
    else
        ullSectorSize = 0;

    DPRINT1("SectorSize: %I64u\n", ullSectorSize);

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(CurrentDisk);
#endif

    PartEntry = InsertGptPartition(CurrentDisk,
                                   ullSectorSize,
                                   &PARTITION_MSFT_RESERVED_GUID);
    if (PartEntry == FALSE)
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return EXIT_SUCCESS;
    }

    CurrentPartition = PartEntry;
    CurrentDisk->Dirty = TRUE;

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(CurrentDisk);
#endif

    UpdateGptDiskLayout(CurrentDisk, FALSE);
    Status = WriteGptPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return EXIT_SUCCESS;
    }

    ConResPuts(StdOut, IDS_CREATE_PARTITION_SUCCESS);

    return EXIT_SUCCESS;
}


static
VOID
CreatePrimaryMbrPartition(
    _In_ ULONGLONG ullSize,
    _In_ PWSTR pszPartitionType)
{
    PPARTENTRY PartEntry, NewPartEntry;
    PLIST_ENTRY ListEntry;
    ULONGLONG ullSectorCount;
    UCHAR PartitionType;
    INT length;
    NTSTATUS Status;

    if (pszPartitionType)
    {
        length = wcslen(pszPartitionType);
        if ((length != 1) && (length != 2))
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return;
        }

        PartitionType = (UCHAR)wcstoul(pszPartitionType, NULL, 16);
        if ((PartitionType == 0) && (errno == ERANGE))
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return;
        }
    }
    else
    {
        PartitionType = PARTITION_HUGE;
    }

    if (GetPrimaryPartitionCount(CurrentDisk) >= 4)
    {
        ConPuts(StdOut, L"No space left for another primary partition!\n");
        return;
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
            PartEntry->Mbr.PartitionType = PartitionType;
            PartEntry->FormatState = Unformatted;
            PartEntry->FileSystemName[0] = L'\0';

            CurrentPartition = PartEntry;
            CurrentDisk->Dirty = TRUE;
            break;
        }
        else
        {
            if (PartEntry->SectorCount.QuadPart == ullSectorCount)
            {
                PartEntry->IsPartitioned = TRUE;
                PartEntry->New = TRUE;
                PartEntry->Mbr.PartitionType = PartitionType;
                PartEntry->FormatState = Unformatted;
                PartEntry->FileSystemName[0] = L'\0';

                CurrentPartition = PartEntry;
                CurrentDisk->Dirty = TRUE;
                break;
            }
            else if (PartEntry->SectorCount.QuadPart > ullSectorCount)
            {
                NewPartEntry = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PPARTENTRY));
                if (NewPartEntry == NULL)
                {
                    ConPuts(StdOut, L"Memory allocation failed!\n");
                    return;
                }

                NewPartEntry->DiskEntry = PartEntry->DiskEntry;

                NewPartEntry->StartSector.QuadPart = PartEntry->StartSector.QuadPart;
                NewPartEntry->SectorCount.QuadPart = ullSectorCount;

                NewPartEntry->LogicalPartition = FALSE;
                NewPartEntry->IsPartitioned = TRUE;
                NewPartEntry->New = TRUE;
                NewPartEntry->Mbr.PartitionType = PartitionType;
                NewPartEntry->FormatState = Unformatted;
                NewPartEntry->FileSystemName[0] = L'\0';

                PartEntry->StartSector.QuadPart += ullSectorCount;
                PartEntry->SectorCount.QuadPart -= ullSectorCount;

                InsertTailList(ListEntry, &NewPartEntry->ListEntry);

                CurrentPartition = NewPartEntry;
                CurrentDisk->Dirty = TRUE;
                break;
            }
        }
    }

    UpdateMbrDiskLayout(CurrentDisk);
    Status = WriteMbrPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return;
    }

    ConResPuts(StdOut, IDS_CREATE_PARTITION_SUCCESS);
}


static
VOID
CreatePrimaryGptPartition(
    _In_ ULONGLONG ullSize,
    _In_ PWSTR pszPartitionType)
{
    PPARTENTRY PartEntry;
    ULONGLONG ullSectorCount;
    GUID guidPartitionType;
    NTSTATUS Status;

    /* Partition Type */
    if (pszPartitionType)
    {
        if (!StringToGUID(&guidPartitionType, pszPartitionType))
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return;
        }
    }
    else
    {
        CopyMemory(&guidPartitionType, &PARTITION_BASIC_DATA_GUID, sizeof(GUID));
    }

    /* Size */
    if (ullSize != 0)
        ullSectorCount = (ullSize * 1024 * 1024) / CurrentDisk->BytesPerSector;
    else
        ullSectorCount = 0;

    DPRINT1("SectorCount: %I64u\n", ullSectorCount);

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(CurrentDisk);
#endif

    PartEntry = InsertGptPartition(CurrentDisk,
                                   ullSectorCount,
                                   &guidPartitionType);
    if (PartEntry == FALSE)
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return;
    }

    CurrentPartition = PartEntry;
    CurrentDisk->Dirty = TRUE;

#ifdef DUMP_PARTITION_LIST
    DumpPartitionList(CurrentDisk);
#endif

    UpdateGptDiskLayout(CurrentDisk, FALSE);
    Status = WriteGptPartitions(CurrentDisk);
    if (!NT_SUCCESS(Status))
    {
        ConResPuts(StdOut, IDS_CREATE_PARTITION_FAIL);
        CurrentPartition = NULL;
        return;
    }

    ConResPuts(StdOut, IDS_CREATE_PARTITION_SUCCESS);
}


EXIT_CODE
CreatePrimaryPartition(
    _In_ INT argc,
    _In_ PWSTR *argv)
{
    ULONGLONG ullSize = 0ULL;
#if 0
    ULONGLONG ullOffset = 0ULL;
    BOOL bNoErr = FALSE;
#endif
    INT i;
    PWSTR pszSuffix = NULL;
    PWSTR pszPartitionType = NULL;

    if (CurrentDisk == NULL)
    {
        ConResPuts(StdOut, IDS_SELECT_NO_DISK);
        return EXIT_SUCCESS;
    }

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_RAW)
    {
        CREATE_DISK DiskInfo;
        NTSTATUS Status;

        DiskInfo.PartitionStyle = PARTITION_STYLE_MBR;
        CreateSignature(&DiskInfo.Mbr.Signature);

        Status = CreateDisk(CurrentDisk->DiskNumber, &DiskInfo);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreateDisk() failed!\n");
            return EXIT_SUCCESS;
        }

        CurrentDisk->StartSector.QuadPart = (ULONGLONG)CurrentDisk->SectorAlignment;
        CurrentDisk->EndSector.QuadPart = min(CurrentDisk->SectorCount.QuadPart, 0x100000000) - 1;

        ScanForUnpartitionedMbrDiskSpace(CurrentDisk);
    }

    for (i = 3; i < argc; i++)
    {
        if (_wcsicmp(argv[i], L"noerr") == 0)
        {
            /* noerr */
            DPRINT("NoErr\n", pszSuffix);
            ConPuts(StdOut, L"The NOERR option is not supported yet!\n");
#if 0
            bNoErr = TRUE;
#endif
        }
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
                return EXIT_SUCCESS;
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
                return EXIT_SUCCESS;
            }
#endif
        }
        else if (HasPrefix(argv[i], L"id=", &pszSuffix))
        {
            /* id=<Byte>|<GUID> */
            DPRINT("Id : %s\n", pszSuffix);
            pszPartitionType = pszSuffix;
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
            /* noerr - Alread handled above */
        }
        else
        {
            ConResPuts(StdErr, IDS_ERROR_INVALID_ARGS);
            return EXIT_SUCCESS;
        }
    }

    DPRINT("Size: %I64u\n", ullSize);
#if 0
    DPRINT1("Offset: %I64u\n", ullOffset);
#endif

    if (CurrentDisk->PartitionStyle == PARTITION_STYLE_MBR)
    {
        DPRINT("Partition Type: %s\n", pszPartitionType);
        CreatePrimaryMbrPartition(ullSize, pszPartitionType);
    }
    else if (CurrentDisk->PartitionStyle == PARTITION_STYLE_GPT)
    {
        CreatePrimaryGptPartition(ullSize, pszPartitionType);
    }

    return EXIT_SUCCESS;
}
