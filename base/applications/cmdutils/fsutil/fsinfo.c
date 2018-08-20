/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            base/applications/cmdutils/fsinfo.c
 * PURPOSE:         FSutil file systems information
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include "fsutil.h"

/* Add handlers here for subcommands */
static HandlerProc DrivesMain;
static HandlerProc DriveTypeMain;
static HandlerProc VolumeInfoMain;
static HandlerProc NtfsInfoMain;
static HandlerProc StatisticsMain;
static HandlerItem HandlersList[] =
{
    /* Proc, name, help */
    { DrivesMain, _T("drives"), _T("Enumerates the drives") },
    { DriveTypeMain, _T("drivetype"), _T("Provides the type of a drive") },
    { VolumeInfoMain, _T("volumeinfo"), _T("Provides informations about a volume") },
    { NtfsInfoMain, _T("ntfsinfo"), _T("Displays informations about a NTFS volume") },
    { StatisticsMain, _T("statistics"), _T("Displays volume statistics") },
};

static int
DrivesMain(int argc, const TCHAR *argv[])
{
    UINT i;
    DWORD Drives;

    /* Get the drives bitmap */
    Drives = GetLogicalDrives();
    if (Drives == 0)
    {
        PrintErrorMessage(GetLastError());
        return 1;
    }

    /* And output any found drive */
    _ftprintf(stdout, _T("Drives:"));
    for (i = 0; i < 26; i++)
    {
        if (Drives & (1 << i))
        {
            _ftprintf(stdout, _T(" %c:\\"), 'A' + i);
        }
    }
    _ftprintf(stdout, _T("\n"));

    return 0;
}

static int
DriveTypeMain(int argc, const TCHAR *argv[])
{
    UINT Type;

    /* We need a volume (letter) */
    if (argc < 2)
    {
        _ftprintf(stderr, _T("Usage: fsutil fsinfo drivetype <volume>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil fsinfo drivetype c:\n"));
        return 1;
    }

    /* Get its drive type and make it readable */
    Type = GetDriveType(argv[1]);
    switch (Type)
    {
        case DRIVE_UNKNOWN:
            _ftprintf(stdout, _T("%s - unknown drive type\n"), argv[1]);
            break;

        case DRIVE_NO_ROOT_DIR:
            _ftprintf(stdout, _T("%s - not a root directory\n"), argv[1]);
            break;

        case DRIVE_REMOVABLE:
            _ftprintf(stdout, _T("%s - removable drive\n"), argv[1]);
            break;

        case DRIVE_FIXED:
            _ftprintf(stdout, _T("%s - fixed drive\n"), argv[1]);
            break;

        case DRIVE_REMOTE:
            _ftprintf(stdout, _T("%s - remote or network drive\n"), argv[1]);
            break;

        case DRIVE_CDROM:
            _ftprintf(stdout, _T("%s - CD-ROM drive\n"), argv[1]);
            break;

        case DRIVE_RAMDISK:
            _ftprintf(stdout, _T("%s - RAM disk drive\n"), argv[1]);
            break;
    }

    return 0;
}

static int
VolumeInfoMain(int argc, const TCHAR *argv[])
{
    DWORD Serial, MaxComponentLen, Flags;
    TCHAR VolumeName[MAX_PATH + 1], FileSystem[MAX_PATH + 1];

#define HANDLE_FLAG(Flags, Flag, Desc) \
    if (Flags & Flag)                  \
        _ftprintf(stdout, Desc)

    /* We need a volume (path) */
    if (argc < 2)
    {
        _ftprintf(stderr, _T("Usage: fsutil fsinfo volumeinfo <volume_path>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil fsinfo volumeinfo c:\\\n"));
        return 1;
    }

    /* Gather information */
    if (!GetVolumeInformation(argv[1], VolumeName,  MAX_PATH + 1, &Serial,
                              &MaxComponentLen, &Flags, FileSystem, MAX_PATH + 1))
    {
        PrintErrorMessage(GetLastError());
        return 1;
    }

    /* Display general information */
    _ftprintf(stdout, _T("Volume name: %s\n"), VolumeName);
    _ftprintf(stdout, _T("Volume serial number: 0x%x\n"), Serial);
    _ftprintf(stdout, _T("Maximum component length: %u\n"), MaxComponentLen);
    _ftprintf(stdout, _T("File system name: %s\n"), FileSystem);

    /* Display specific flags */
    HANDLE_FLAG(Flags, FILE_CASE_SENSITIVE_SEARCH, _T("Supports case-sensitive file names\n"));
    HANDLE_FLAG(Flags, FILE_CASE_PRESERVED_NAMES, _T("Supports preserved case of file names\n"));
    HANDLE_FLAG(Flags, FILE_UNICODE_ON_DISK, _T("Supports unicode file names\n"));
    HANDLE_FLAG(Flags, FILE_PERSISTENT_ACLS, _T("Preserves and applies ACLs\n"));
    HANDLE_FLAG(Flags, FILE_FILE_COMPRESSION, _T("Supports compression per file\n"));
    HANDLE_FLAG(Flags, FILE_VOLUME_QUOTAS, _T("Supports disk quotas\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_SPARSE_FILES, _T("Supports sparse files\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_REPARSE_POINTS, _T("Supports reparse points\n"));
    HANDLE_FLAG(Flags, FILE_VOLUME_IS_COMPRESSED, _T("Is a compressed volume\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_OBJECT_IDS, _T("Supports object identifiers\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_ENCRYPTION, _T("Supports the Encrypted File System (EFS)\n"));
    HANDLE_FLAG(Flags, FILE_NAMED_STREAMS, _T("Supports named streams\n"));
    HANDLE_FLAG(Flags, FILE_READ_ONLY_VOLUME, _T("Is a read-only volume\n"));
    HANDLE_FLAG(Flags, FILE_SEQUENTIAL_WRITE_ONCE, _T("Supports a single sequential write\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_TRANSACTIONS, _T("Supports transactions\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_HARD_LINKS, _T("Supports hard links\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_EXTENDED_ATTRIBUTES, _T("Supports extended attributes\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_OPEN_BY_FILE_ID, _T("Supports opening files per file identifier\n"));
    HANDLE_FLAG(Flags, FILE_SUPPORTS_USN_JOURNAL, _T("Supports Update Sequence Number (USN) journals\n"));
    HANDLE_FLAG(Flags, FILE_DAX_VOLUME, _T("Is a direct access volume\n"));

#undef HANDLE_FLAGS

    return 0;
}

static int
NtfsInfoMain(int argc, const TCHAR *argv[])
{
    HANDLE Volume;
    DWORD BytesRead;
    struct
    {
        NTFS_VOLUME_DATA_BUFFER;
        NTFS_EXTENDED_VOLUME_DATA;
    } Data;

    /* We need a volume (letter or GUID) */
    if (argc < 2)
    {
        _ftprintf(stderr, _T("Usage: fsutil fsinfo ntfsinfo <volume>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil fsinfo ntfsinfo c:\n"));
        return 1;
    }

    /* Get a handle for the volume */
    Volume = OpenVolume(argv[1], FALSE, TRUE);
    if (Volume == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    /* And query the NTFS data */
    if (DeviceIoControl(Volume, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &Data,
                        sizeof(Data), &BytesRead, NULL) == FALSE)
    {
        PrintErrorMessage(GetLastError());
        CloseHandle(Volume);
        return 1;
    }

    /* We no longer need the volume */
    CloseHandle(Volume);

    /* Dump data */
    _ftprintf(stdout, _T("NTFS volume serial number:\t\t0x%0.16I64x\n"), Data.VolumeSerialNumber.QuadPart);
    /* Only print version if extended structure was returned */
    if (BytesRead > sizeof(NTFS_VOLUME_DATA_BUFFER))
    {
        _ftprintf(stdout, _T("Version:\t\t\t\t%u.%u\n"), Data.MajorVersion, Data.MinorVersion);
    }
    _ftprintf(stdout, _T("Number of sectors:\t\t\t0x%0.16I64x\n"), Data.NumberSectors.QuadPart);
    _ftprintf(stdout, _T("Total number of clusters:\t\t0x%0.16I64x\n"), Data.TotalClusters.QuadPart);
    _ftprintf(stdout, _T("Free clusters:\t\t\t\t0x%0.16I64x\n"), Data.FreeClusters.QuadPart);
    _ftprintf(stdout, _T("Total number of reserved clusters:\t0x%0.16I64x\n"), Data.TotalReserved.QuadPart);
    _ftprintf(stdout, _T("Bytes per sector:\t\t\t%d\n"), Data.BytesPerSector);
    _ftprintf(stdout, _T("Bytes per cluster:\t\t\t%d\n"), Data.BytesPerCluster);
    _ftprintf(stdout, _T("Bytes per file record segment:\t\t%d\n"), Data.BytesPerFileRecordSegment);
    _ftprintf(stdout, _T("Clusters per file record segment:\t%d\n"), Data.ClustersPerFileRecordSegment);
    _ftprintf(stdout, _T("MFT valid data length:\t\t\t0x%0.16I64x\n"), Data.MftValidDataLength.QuadPart);
    _ftprintf(stdout, _T("MFT start LCN:\t\t\t\t0x%0.16I64x\n"), Data.MftStartLcn.QuadPart);
    _ftprintf(stdout, _T("MFT2 start LCN:\t\t\t\t0x%0.16I64x\n"), Data.Mft2StartLcn.QuadPart);
    _ftprintf(stdout, _T("MFT zone start:\t\t\t\t0x%0.16I64x\n"), Data.MftZoneStart.QuadPart);
    _ftprintf(stdout, _T("MFT zone end:\t\t\t\t0x%0.16I64x\n"), Data.MftZoneEnd.QuadPart);

    return 0;
}

#define DUMP_VALUE(stats, value) fprintf(stdout, "%s: %lu\n", #value, stats->value)

static void
DumpBase(PFILESYSTEM_STATISTICS Base, TCHAR * Name)
{
    /* Print FS name */
    _ftprintf(stdout, _T("File system type: %s\n\n"), Name);

    /* And then, dump any base stat */
    DUMP_VALUE(Base, UserFileReads);
    DUMP_VALUE(Base, UserFileReadBytes);
    DUMP_VALUE(Base, UserDiskReads);
    DUMP_VALUE(Base, UserFileWrites);
    DUMP_VALUE(Base, UserFileWriteBytes);
    DUMP_VALUE(Base, UserDiskWrites);
    DUMP_VALUE(Base, MetaDataReads);
    DUMP_VALUE(Base, MetaDataReadBytes);
    DUMP_VALUE(Base, MetaDataDiskReads);
    DUMP_VALUE(Base, MetaDataWrites);
    DUMP_VALUE(Base, MetaDataWriteBytes);
    DUMP_VALUE(Base, MetaDataDiskWrites);

    _ftprintf(stdout, _T("\n"));
}

static void
DumpExFat(PVOID Statistics, PVOID Specific)
{
    PEXFAT_STATISTICS ExFat;
    PFILESYSTEM_STATISTICS Base;

    Base = Statistics;
    ExFat = Specific;

    /* First, display the generic stats */
    DumpBase(Base, _T("EXFAT"));

    /* Then, display the EXFAT specific ones */
    DUMP_VALUE(ExFat, CreateHits);
    DUMP_VALUE(ExFat, SuccessfulCreates);
    DUMP_VALUE(ExFat, FailedCreates);
    DUMP_VALUE(ExFat, NonCachedReads);
    DUMP_VALUE(ExFat, NonCachedReadBytes);
    DUMP_VALUE(ExFat, NonCachedWrites);
    DUMP_VALUE(ExFat, NonCachedWriteBytes);
    DUMP_VALUE(ExFat, NonCachedDiskReads);
    DUMP_VALUE(ExFat, NonCachedDiskWrites);
}

static void
DumpFat(PVOID Statistics, PVOID Specific)
{
    PFAT_STATISTICS Fat;
    PFILESYSTEM_STATISTICS Base;

    Base = Statistics;
    Fat = Specific;

    /* First, display the generic stats */
    DumpBase(Base, _T("FAT"));

    /* Then, display the FAT specific ones */
    DUMP_VALUE(Fat, CreateHits);
    DUMP_VALUE(Fat, SuccessfulCreates);
    DUMP_VALUE(Fat, FailedCreates);
    DUMP_VALUE(Fat, NonCachedReads);
    DUMP_VALUE(Fat, NonCachedReadBytes);
    DUMP_VALUE(Fat, NonCachedWrites);
    DUMP_VALUE(Fat, NonCachedWriteBytes);
    DUMP_VALUE(Fat, NonCachedDiskReads);
    DUMP_VALUE(Fat, NonCachedDiskWrites);
}

static void
DumpNtfs(PVOID Statistics, PVOID Specific)
{
    PNTFS_STATISTICS Ntfs;
    PFILESYSTEM_STATISTICS Base;

    Base = Statistics;
    Ntfs = Specific;

    /* First, display the generic stats */
    DumpBase(Base, _T("NTFS"));

    /* Then, display the NTFS specific ones */
    DUMP_VALUE(Ntfs, MftReads);
    DUMP_VALUE(Ntfs, MftReadBytes);
    DUMP_VALUE(Ntfs, MftWrites);
    DUMP_VALUE(Ntfs, MftWriteBytes);
    DUMP_VALUE(Ntfs, Mft2Writes);
    DUMP_VALUE(Ntfs, Mft2WriteBytes);
    DUMP_VALUE(Ntfs, RootIndexReads);
    DUMP_VALUE(Ntfs, RootIndexReadBytes);
    DUMP_VALUE(Ntfs, RootIndexWrites);
    DUMP_VALUE(Ntfs, RootIndexWriteBytes);
    DUMP_VALUE(Ntfs, BitmapReads);
    DUMP_VALUE(Ntfs, BitmapReadBytes);
    DUMP_VALUE(Ntfs, BitmapWrites);
    DUMP_VALUE(Ntfs, BitmapWriteBytes);
    DUMP_VALUE(Ntfs, MftBitmapReads);
    DUMP_VALUE(Ntfs, MftBitmapReadBytes);
    DUMP_VALUE(Ntfs, MftBitmapWrites);
    DUMP_VALUE(Ntfs, MftBitmapWriteBytes);
    DUMP_VALUE(Ntfs, UserIndexReads);
    DUMP_VALUE(Ntfs, UserIndexReadBytes);
    DUMP_VALUE(Ntfs, UserIndexWrites);
    DUMP_VALUE(Ntfs, UserIndexWriteBytes);
    DUMP_VALUE(Ntfs, LogFileReads);
    DUMP_VALUE(Ntfs, LogFileReadBytes);
    DUMP_VALUE(Ntfs, LogFileWrites);
    DUMP_VALUE(Ntfs, LogFileWriteBytes);
}

#define GET_NEXT(stats, length, iter, type) (type)((ULONG_PTR)stats + (length * iter))
#define SUM_VALUE(stats, new, value) stats->value += new->value

inline int
ValidateSizes(ULONG Length, DWORD ProcCount, DWORD BytesRead, DWORD SpecificSize)
{
    /* Check whether we could read our base length for every processor */
    if (Length * ProcCount > BytesRead)
    {
        _ftprintf(stderr, _T("Only performed a partial read: %lu (expected: %lu)\n"), BytesRead, Length * ProcCount);
        return 1;
    }

    /* Check whether this covers a specific entry and a generic entry for every processor */
    if ((sizeof(FILESYSTEM_STATISTICS) + SpecificSize) * ProcCount > BytesRead)
    {
        _ftprintf(stderr, _T("Only performed a partial read: %lu (expected: %Iu)\n"), BytesRead, (sizeof(FILESYSTEM_STATISTICS) + SpecificSize));
        return 1;
    }

    return 0;
}

inline void
SumBase(PFILESYSTEM_STATISTICS Base, PFILESYSTEM_STATISTICS NextBase)
{
    /* Sum any entry in the generic structures */
    SUM_VALUE(Base, NextBase, UserFileReads);
    SUM_VALUE(Base, NextBase, UserFileReadBytes);
    SUM_VALUE(Base, NextBase, UserDiskReads);
    SUM_VALUE(Base, NextBase, UserFileWrites);
    SUM_VALUE(Base, NextBase, UserFileWriteBytes);
    SUM_VALUE(Base, NextBase, UserDiskWrites);
    SUM_VALUE(Base, NextBase, MetaDataReads);
    SUM_VALUE(Base, NextBase, MetaDataReadBytes);
    SUM_VALUE(Base, NextBase, MetaDataDiskReads);
    SUM_VALUE(Base, NextBase, MetaDataWrites);
    SUM_VALUE(Base, NextBase, MetaDataWriteBytes);
    SUM_VALUE(Base, NextBase, MetaDataDiskWrites);
}

static int
SumExFat(PVOID Statistics, PVOID Specific, ULONG Length, DWORD ProcCount, DWORD BytesRead)
{
    DWORD i;
    PEXFAT_STATISTICS ExFat;
    PFILESYSTEM_STATISTICS Base;

    /* First, validate we won't read beyond allocation */
    if (ValidateSizes(Length, ProcCount, BytesRead, sizeof(EXFAT_STATISTICS)))
    {
        return 1;
    }

    Base = Statistics;
    ExFat = Specific;

    /* And for every processor, sum every relevant value in first entry */
    for (i = 1; i < ProcCount; ++i)
    {
        PEXFAT_STATISTICS NextExFat;
        PFILESYSTEM_STATISTICS NextBase;

        NextBase = GET_NEXT(Base, Length, i, PFILESYSTEM_STATISTICS);
        NextExFat = GET_NEXT(ExFat, Length, i, PEXFAT_STATISTICS);

        /* Generic first */
        SumBase(Base, NextBase);
        /* Specific then */
        SUM_VALUE(ExFat, NextExFat, CreateHits);
        SUM_VALUE(ExFat, NextExFat, SuccessfulCreates);
        SUM_VALUE(ExFat, NextExFat, FailedCreates);
        SUM_VALUE(ExFat, NextExFat, NonCachedReads);
        SUM_VALUE(ExFat, NextExFat, NonCachedReadBytes);
        SUM_VALUE(ExFat, NextExFat, NonCachedWrites);
        SUM_VALUE(ExFat, NextExFat, NonCachedWriteBytes);
        SUM_VALUE(ExFat, NextExFat, NonCachedDiskReads);
        SUM_VALUE(ExFat, NextExFat, NonCachedDiskWrites);
    }

    return 0;
}

static int
SumFat(PVOID Statistics, PVOID Specific, ULONG Length, DWORD ProcCount, DWORD BytesRead)
{
    DWORD i;
    PFAT_STATISTICS Fat;
    PFILESYSTEM_STATISTICS Base;

    /* First, validate we won't read beyond allocation */
    if (ValidateSizes(Length, ProcCount, BytesRead, sizeof(FAT_STATISTICS)))
    {
        return 1;
    }

    Base = Statistics;
    Fat = Specific;

    /* And for every processor, sum every relevant value in first entry */
    for (i = 1; i < ProcCount; ++i)
    {
        PFAT_STATISTICS NextFat;
        PFILESYSTEM_STATISTICS NextBase;

        NextBase = GET_NEXT(Base, Length, i, PFILESYSTEM_STATISTICS);
        NextFat = GET_NEXT(Fat, Length, i, PFAT_STATISTICS);

        /* Generic first */
        SumBase(Base, NextBase);
        /* Specific then */
        SUM_VALUE(Fat, NextFat, CreateHits);
        SUM_VALUE(Fat, NextFat, SuccessfulCreates);
        SUM_VALUE(Fat, NextFat, FailedCreates);
        SUM_VALUE(Fat, NextFat, NonCachedReads);
        SUM_VALUE(Fat, NextFat, NonCachedReadBytes);
        SUM_VALUE(Fat, NextFat, NonCachedWrites);
        SUM_VALUE(Fat, NextFat, NonCachedWriteBytes);
        SUM_VALUE(Fat, NextFat, NonCachedDiskReads);
        SUM_VALUE(Fat, NextFat, NonCachedDiskWrites);
    }

    return 0;
}

static int
SumNtfs(PVOID Statistics, PVOID Specific, ULONG Length, DWORD ProcCount, DWORD BytesRead)
{
    DWORD i;
    PNTFS_STATISTICS Ntfs;
    PFILESYSTEM_STATISTICS Base;

    /* First, validate we won't read beyond allocation */
    if (ValidateSizes(Length, ProcCount, BytesRead, sizeof(NTFS_STATISTICS)))
    {
        return 1;
    }

    Base = Statistics;
    Ntfs = Specific;

    /* And for every processor, sum every relevant value in first entry */
    for (i = 1; i < ProcCount; ++i)
    {
        PNTFS_STATISTICS NextNtfs;
        PFILESYSTEM_STATISTICS NextBase;

        NextBase = GET_NEXT(Base, Length, i, PFILESYSTEM_STATISTICS);
        NextNtfs = GET_NEXT(Ntfs, Length, i, PNTFS_STATISTICS);

        /* Generic first */
        SumBase(Base, NextBase);
        /* Specific then */
        SUM_VALUE(Ntfs, NextNtfs, MftReads);
        SUM_VALUE(Ntfs, NextNtfs, MftReadBytes);
        SUM_VALUE(Ntfs, NextNtfs, MftWrites);
        SUM_VALUE(Ntfs, NextNtfs, MftWriteBytes);
        SUM_VALUE(Ntfs, NextNtfs, Mft2Writes);
        SUM_VALUE(Ntfs, NextNtfs, Mft2WriteBytes);
        SUM_VALUE(Ntfs, NextNtfs, RootIndexReads);
        SUM_VALUE(Ntfs, NextNtfs, RootIndexReadBytes);
        SUM_VALUE(Ntfs, NextNtfs, RootIndexWrites);
        SUM_VALUE(Ntfs, NextNtfs, RootIndexWriteBytes);
        SUM_VALUE(Ntfs, NextNtfs, BitmapReads);
        SUM_VALUE(Ntfs, NextNtfs, BitmapReadBytes);
        SUM_VALUE(Ntfs, NextNtfs, BitmapWrites);
        SUM_VALUE(Ntfs, NextNtfs, BitmapWriteBytes);
        SUM_VALUE(Ntfs, NextNtfs, MftBitmapReads);
        SUM_VALUE(Ntfs, NextNtfs, MftBitmapReadBytes);
        SUM_VALUE(Ntfs, NextNtfs, MftBitmapWrites);
        SUM_VALUE(Ntfs, NextNtfs, MftBitmapWriteBytes);
        SUM_VALUE(Ntfs, NextNtfs, UserIndexReads);
        SUM_VALUE(Ntfs, NextNtfs, UserIndexReadBytes);
        SUM_VALUE(Ntfs, NextNtfs, UserIndexWrites);
        SUM_VALUE(Ntfs, NextNtfs, UserIndexWriteBytes);
        SUM_VALUE(Ntfs, NextNtfs, LogFileReads);
        SUM_VALUE(Ntfs, NextNtfs, LogFileReadBytes);
        SUM_VALUE(Ntfs, NextNtfs, LogFileWrites);
        SUM_VALUE(Ntfs, NextNtfs, LogFileWriteBytes);
    }

    return 0;
}

static int
StatisticsMain(int argc, const TCHAR *argv[])
{
    HANDLE Volume;
    SYSTEM_INFO SystemInfo;
    FILESYSTEM_STATISTICS Buffer;
    PFILESYSTEM_STATISTICS Statistics;
    DWORD BytesRead, Length, ProcCount;
    /* +1 because 0 isn't assigned to a filesystem */
    void (*DumpFct[FILESYSTEM_STATISTICS_TYPE_EXFAT + 1])(PVOID, PVOID) = { NULL, DumpNtfs, DumpFat, DumpExFat };
    int (*SumFct[FILESYSTEM_STATISTICS_TYPE_EXFAT + 1])(PVOID, PVOID, ULONG, DWORD, DWORD) = { NULL, SumNtfs, SumFat, SumExFat };

    /* We need a volume (letter or GUID) */
    if (argc < 2)
    {
        _ftprintf(stderr, _T("Usage: fsutil fsinfo statistics <volume>\n"));
        _ftprintf(stderr, _T("\tFor example: fsutil fsinfo statistics c:\n"));
        return 1;
    }

    /* Get a handle for the volume */
    Volume = OpenVolume(argv[1], FALSE, FALSE);
    if (Volume == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    /* And query the statistics status - this call is expected to fail */
    Statistics = &Buffer;
    Length = sizeof(Buffer);
    /* Assume we have a single proc for now */
    ProcCount = 1;
    if (DeviceIoControl(Volume, FSCTL_FILESYSTEM_GET_STATISTICS, NULL, 0, Statistics,
                        Length, &BytesRead, NULL) == FALSE)
    {
        DWORD Error;

        /* Check we failed because we provided a too small buffer */
        Error = GetLastError();
        if (Error == ERROR_MORE_DATA)
        {
            /* Get proc count */
            GetSystemInfo(&SystemInfo);
            ProcCount = SystemInfo.dwNumberOfProcessors;
            /* Get the maximum size to allocate: it's the total size (generic + specific) for every proc */
            Length = Statistics->SizeOfCompleteStructure * ProcCount;

            Statistics = LocalAlloc(LPTR, Length);
            if (Statistics == NULL)
            {
                _ftprintf(stderr, _T("Failed to allocate memory!\n"));
                CloseHandle(Volume);
                return 1;
            }

            /* Reissue the FSCTL, it's supposed to succeed now! */
            if (DeviceIoControl(Volume, FSCTL_FILESYSTEM_GET_STATISTICS, NULL, 0, Statistics,
                                Length, &BytesRead, NULL) == FALSE)
            {
                PrintErrorMessage(GetLastError());
                LocalFree(Statistics);
                CloseHandle(Volume);
                return 1;
            }
        }
        else
        {
            PrintErrorMessage(Error);
            CloseHandle(Volume);
            return 1;
        }
    }

    /* No need to deal with the volume any longer */
    CloseHandle(Volume);

    /* We only support FAT, EXFAT and NTFS for now */
    if (Statistics->FileSystemType > FILESYSTEM_STATISTICS_TYPE_EXFAT || Statistics->FileSystemType < FILESYSTEM_STATISTICS_TYPE_NTFS)
    {
        _ftprintf(stderr, _T("Unrecognized file system type: %d\n"), Statistics->FileSystemType);
        if (Statistics != &Buffer)
        {
            LocalFree(Statistics);
        }

        return 1;
    }

    /* Sum all the statistics (both generic and specific) from all the processors in the first entry */
    if (SumFct[Statistics->FileSystemType](Statistics, (PVOID)((ULONG_PTR)Statistics + sizeof(FILESYSTEM_STATISTICS)),
                                           Statistics->SizeOfCompleteStructure, ProcCount, BytesRead))
    {
        if (Statistics != &Buffer)
        {
            LocalFree(Statistics);
        }

        return 1;
    }

    /* And finally, display the statistics (both generic and specific) */
    DumpFct[Statistics->FileSystemType](Statistics, (PVOID)((ULONG_PTR)Statistics + sizeof(FILESYSTEM_STATISTICS)));

    /* If we allocated memory, release it */
    if (Statistics != &Buffer)
    {
        LocalFree(Statistics);
    }

    return 0;
}

static void
PrintUsage(const TCHAR * Command)
{
    PrintDefaultUsage(_T(" FSINFO "), Command, (HandlerItem *)&HandlersList,
                      (sizeof(HandlersList) / sizeof(HandlersList[0])));
}

int
FsInfoMain(int argc, const TCHAR *argv[])
{
    return FindHandler(argc, argv, (HandlerItem *)&HandlersList,
                       (sizeof(HandlersList) / sizeof(HandlersList[0])),
                       PrintUsage);
}
