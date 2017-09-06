/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            base/applications/cmdutils/fsinfo.c
 * PURPOSE:         FSutil file systems information
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

/* Add handlers here for subcommands */
static int DrivesMain(int argc, const TCHAR *argv[]);
static int DriveTypeMain(int argc, const TCHAR *argv[]);
static HandlerItem HandlersList[] =
{
    /* Proc, name, help */
    { DrivesMain, _T("drives"), _T("Enumerates the drives") },
    { DriveTypeMain, _T("drivetype"), _T("Provides the type of a drive") },
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
