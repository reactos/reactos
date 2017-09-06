/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            base/applications/cmdutils/fsinfo.c
 * PURPOSE:         FSutil file systems information
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#include "fsutil.h"

/* Add handlers here for subcommands */
static int DrivesMain(int argc, const TCHAR *argv[]);
static int DriveTypeMain(int argc, const TCHAR *argv[]);
static int VolumeInfoMain(int argc, const TCHAR *argv[]);
static HandlerItem HandlersList[] =
{
    /* Proc, name, help */
    { DrivesMain, _T("drives"), _T("Enumerates the drives") },
    { DriveTypeMain, _T("drivetype"), _T("Provides the type of a drive") },
    { VolumeInfoMain, _T("volumeinfo"), _T("Provides informations about a volume") },
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
        _ftprintf(stderr, _T("Error: %d\n"), GetLastError());
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
        _ftprintf(stderr, _T("Error: %d\n"), GetLastError());
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
