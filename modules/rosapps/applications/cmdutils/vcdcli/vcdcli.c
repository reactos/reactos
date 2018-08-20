/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS FS utility tool
 * FILE:            modules/rosapps/applications/cmdutils/vcdcli/vcdcli.c
 * PURPOSE:         Virtual CD-ROM management application
 * PROGRAMMERS:     Pierre Schweitzer <pierre@reactos.org>
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <winreg.h>
#include <ndk/rtltypes.h>
#include <ndk/rtlfuncs.h>
#include <tchar.h>
#include <stdio.h>

#include <vcdioctl.h>

#define IOCTL_CDROM_BASE FILE_DEVICE_CD_ROM
#define IOCTL_CDROM_EJECT_MEDIA CTL_CODE(IOCTL_CDROM_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)

void
PrintUsage(int type)
{
    if (type == 0)
    {
        _ftprintf(stdout, _T("vcdcli usage:\n"));
        _ftprintf(stdout, _T("\tlist [/a]: list all the virtual drives\n"));
        _ftprintf(stdout, _T("\tcreate: create a virtual drive\n"));
        _ftprintf(stdout, _T("\tmount X path: mount path image on X virtual drive\n"));
        _ftprintf(stdout, _T("\tremount X: remount image on X virtual drive\n"));
        _ftprintf(stdout, _T("\tremount X: remount image on X virtual drive\n"));
        _ftprintf(stdout, _T("\teject X: eject image on X virtual drive\n"));
        _ftprintf(stdout, _T("\tremove X: remove virtual drive X\n"));
    }
    else if (type == 1)
    {
        _ftprintf(stdout, _T("mount usage:\n"));
        _ftprintf(stdout, _T("\tmount <drive letter> <path.iso> [/u] [/j] [/p]\n"));
        _ftprintf(stdout, _T("\tMount the ISO image given in <path.iso> on the previously created virtual drive <drive letter>\n"));
        _ftprintf(stdout, _T("\t\tDo not use colon for drive letter\n"));
        _ftprintf(stdout, _T("\t\tUse /u to make UDF volumes not appear as such\n"));
        _ftprintf(stdout, _T("\t\tUse /j to make Joliet volumes not appear as such\n"));
        _ftprintf(stdout, _T("\t\tUse /p to make the mounting persistent\n"));
    }
    else if (type == 2)
    {
        _ftprintf(stdout, _T("remount usage:\n"));
        _ftprintf(stdout, _T("\tremount <drive letter>\n"));
        _ftprintf(stdout, _T("\tRemount the ISO image that was previously mounted on the virtual drive <drive letter>\n"));
        _ftprintf(stdout, _T("\t\tDo not use colon for drive letter\n"));
    }
    else if (type == 3)
    {
        _ftprintf(stdout, _T("eject usage:\n"));
        _ftprintf(stdout, _T("\teject <drive letter>\n"));
        _ftprintf(stdout, _T("\tEjects the ISO image that is mounted on the virtual drive <drive letter>\n"));
        _ftprintf(stdout, _T("\t\tDo not use colon for drive letter\n"));
    }
    else if (type == 4)
    {
        _ftprintf(stdout, _T("remove usage:\n"));
        _ftprintf(stdout, _T("\tremove <drive letter>\n"));
        _ftprintf(stdout, _T("\tRemoves the virtual drive <drive letter> making it no longer usable\n"));
        _ftprintf(stdout, _T("\t\tDo not use colon for drive letter\n"));
    }
}

HANDLE
OpenLetter(WCHAR Letter)
{
    TCHAR Device[255];

    /* Make name */
    _stprintf(Device, _T("\\\\.\\%c:"), Letter);

    /* And open */
    return CreateFile(Device, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

BOOLEAN
StartDriver(VOID)
{
    SC_HANDLE hMgr, hSvc;

    /* Open the SC manager */
    hMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hMgr == NULL)
    {
        _ftprintf(stderr, _T("Failed opening service manager: %x\n"), GetLastError());
        return FALSE;
    }

    /* Open the service matching our driver */
    hSvc = OpenService(hMgr, _T("Vcdrom"), SERVICE_START);
    if (hSvc == NULL)
    {
        _ftprintf(stderr, _T("Failed opening service: %x\n"), GetLastError());
        CloseServiceHandle(hMgr);
        return FALSE;
    }

    /* Start it */
    /* FIXME: improve */
    StartService(hSvc, 0, NULL);

    /* Cleanup */
    CloseServiceHandle(hSvc);
    CloseServiceHandle(hMgr);

    /* Always return true when service exists
     * We don't care whether it was running or not
     * We just need it
     */
    return TRUE;
}

HANDLE
OpenMaster(VOID)
{
    /* We'll always talk to master first, so we start it here */
    if (!StartDriver())
    {
        return INVALID_HANDLE_VALUE;
    }

    /* And then, open it */
    return CreateFile(_T("\\\\.\\\\VirtualCdRom"), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

BOOLEAN
IsLetterOwned(WCHAR Letter)
{
    HANDLE hDev;
    BOOLEAN Res;
    DRIVES_LIST Drives;
    DWORD i, BytesRead;

    /* We've to deal with driver */
    hDev = OpenMaster();
    if (hDev == INVALID_HANDLE_VALUE)
    {
        _ftprintf(stderr, _T("Failed to open VCD: %x\n"), GetLastError());
        return FALSE;
    }

    /* Get the list of the managed drives */
    Res = DeviceIoControl(hDev, IOCTL_VCDROM_ENUMERATE_DRIVES, NULL, 0, &Drives, sizeof(Drives), &BytesRead, NULL);
    if (!Res)
    {
        _ftprintf(stderr, _T("Failed to enumerate drives: %x\n"), GetLastError());
        CloseHandle(hDev);
        return FALSE;
    }

    /* Don't leak ;-) */
    CloseHandle(hDev);

    /* Do we find our letter in the list? */
    for (i = 0; i < Drives.Count; ++i)
    {
        if (Drives.Drives[i] == Letter)
        {
            break;
        }
    }

    /* No? Fail */
    if (i == Drives.Count)
    {
        _ftprintf(stderr, _T("%c is not a drive owned by VCD\n"), Letter);
        return FALSE;
    }

    /* Otherwise, that's fine! */
    return TRUE;
}

FORCEINLINE
DWORD
Min(DWORD a, DWORD b)
{
    return (a > b ? b : a);
}

int
__cdecl
_tmain(int argc, const TCHAR *argv[])
{
    HANDLE hDev;
    BOOLEAN Res;
    DWORD BytesRead;

    /* We need a command, at least */
    if (argc < 2)
    {
        PrintUsage(0);
        return 1;
    }

    /* List will display all the managed drives */
    if (_tcscmp(argv[1], _T("list")) == 0)
    {
        DWORD i;
        BOOLEAN All;
        DRIVES_LIST Drives;

        /* Open the driver for query */
        hDev = OpenMaster();
        if (hDev == INVALID_HANDLE_VALUE)
        {
            _ftprintf(stderr, _T("Failed to open VCD: %x\n"), GetLastError());
            return 1;
        }

        /* Query the virtual drives */
        Res = DeviceIoControl(hDev, IOCTL_VCDROM_ENUMERATE_DRIVES, NULL, 0, &Drives, sizeof(Drives), &BytesRead, NULL);
        if (!Res)
        {
            _ftprintf(stderr, _T("Failed to create VCD: %x\n"), GetLastError());
            CloseHandle(hDev);
            return 1;
        }

        /* Done with master */
        CloseHandle(hDev);

        /* No drives? Display a pretty message */
        if (Drives.Count == 0)
        {
            _ftprintf(stdout, _T("No virtual drives\n"));
        }
        else
        {
            /* Do we have to display all the information? That's '/a' */
            All = FALSE;
            if (argc > 2)
            {
                if (_tcscmp(argv[2], _T("/a")) == 0)
                {
                    All = TRUE;
                }
            }

            if (All)
            {
                _ftprintf(stdout, _T("Managed drives:\n"));
                /* For each virtual drive... */
                for (i = 0; i < Drives.Count; ++i)
                {
                    HANDLE hLet;
                    IMAGE_PATH Image;

                    /* Display its letter */
                    _ftprintf(stdout, _T("%c: "), Drives.Drives[i]);

                    /* And open it to query more data */
                    hLet = OpenLetter(Drives.Drives[i]);
                    if (hLet != INVALID_HANDLE_VALUE)
                    {
                        /* Get the image path along with mount status */
                        Res = DeviceIoControl(hLet, IOCTL_VCDROM_GET_IMAGE_PATH, NULL, 0, &Image, sizeof(Image), &BytesRead, NULL);
                        /* If it succeed */
                        if (Res)
                        {
                            UNICODE_STRING Path;

                            /* Display image if any, otherwise display "no image" */
                            if (Image.Length != 0)
                            {
                                Path.Length = Image.Length;
                                Path.MaximumLength = Image.Length;
                                Path.Buffer = Image.Path;
                            }
                            else
                            {
                                Path.Length = sizeof(L"no image") - sizeof(UNICODE_NULL);
                                Path.MaximumLength = sizeof(L"no image");
                                Path.Buffer = L"no image";
                            }

                            /* Print everything including mount status */
                            _ftprintf(stdout, _T("%wZ, %s"), &Path, (Image.Mounted == 0 ? _T("not mounted") : _T("mounted")));
                        }

                        /* Close drive and move to the next one */
                        CloseHandle(hLet);
                    }

                    /* EOL! */
                    _ftprintf(stdout, _T("\n"));
                }
            }
            else
            {
                /* Basic display, just display drives on a single line */
                _ftprintf(stdout, _T("Virtual drives:\n"));
                for (i = 0; i < Drives.Count; ++i)
                {
                    _ftprintf(stdout, _T("%c: "), Drives.Drives[i]);
                }
                _ftprintf(stdout, _T("\n"));
            }
        }
    }
    else if (_tcscmp(argv[1], _T("create")) == 0)
    {
        WCHAR Letter;

        /* Open driver */
        hDev = OpenMaster();
        if (hDev == INVALID_HANDLE_VALUE)
        {
            _ftprintf(stderr, _T("Failed to open VCD: %x\n"), GetLastError());
            return 1;
        }

        /* Issue the IOCTL */
        Res = DeviceIoControl(hDev, IOCTL_VCDROM_CREATE_DRIVE, NULL, 0, &Letter, sizeof(WCHAR), &BytesRead, NULL);
        if (!Res)
        {
            _ftprintf(stderr, _T("Failed to create drive: %x\n"), GetLastError());
            CloseHandle(hDev);
            return 1;
        }

        /* And display the create drive letter to the user */
        _ftprintf(stdout, _T("The virtual drive '%c' has been created\n"), Letter);

        CloseHandle(hDev);
    }
    else if (_tcscmp(argv[1], _T("mount")) == 0)
    {
        DWORD i;
        HKEY hKey;
        HANDLE hFile;
        WCHAR Letter;
        BOOLEAN bPersist;
        TCHAR szBuffer[260];
        UNICODE_STRING NtPathName;
        MOUNT_PARAMETERS MountParams;

        /* We need two args */
        if (argc < 4)
        {
            PrintUsage(1);
            return 1;
        }

        /* First, check letter is OK */
        if (!_istalpha(argv[2][0]) || argv[2][1] != 0)
        {
            PrintUsage(1);
            return 1;
        }

        /* Now, check the ISO image is OK and reachable by the user */
        hFile = CreateFile(argv[3], FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            _ftprintf(stderr, _T("Failed to open file: %lu\n"), GetLastError());
            return 1;
        }

        /* Validate the drive is owned by vcdrom */
        Letter = _totupper(argv[2][0]);
        if (!IsLetterOwned(Letter))
        {
            CloseHandle(hFile);
            return 1;
        }

        /* Get NT path for the driver */
        if (!RtlDosPathNameToNtPathName_U(argv[3], &NtPathName, NULL, NULL))
        {
            _ftprintf(stderr, _T("Failed to convert path\n"));
            CloseHandle(hFile);
            return 1;
        }

        /* Copy it in the parameter structure */
        _tcsncpy(MountParams.Path, NtPathName.Buffer, 255);
        MountParams.Length = Min(NtPathName.Length, 255 * sizeof(WCHAR));
        MountParams.Flags = 0;

        /* Do we have to suppress anything? */
        bPersist = FALSE;
        for (i = 4; i < argc; ++i)
        {
            /* Make UDF uneffective */
            if (_tcscmp(argv[i], _T("/u")) == 0)
            {
                MountParams.Flags |= MOUNT_FLAG_SUPP_UDF;
            }
            /* Make Joliet uneffective */
            else if (_tcscmp(argv[i], _T("/j")) == 0)
            {
                MountParams.Flags |= MOUNT_FLAG_SUPP_JOLIET;
            }
            /* Should it be persistent? */
            else if (_tcscmp(argv[i], _T("/p")) == 0)
            {
                bPersist = TRUE;
            }
        }

        /* No longer needed */
        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPathName.Buffer);

        /* Open the drive */
        hDev = OpenLetter(Letter);
        if (hDev == INVALID_HANDLE_VALUE)
        {
            _ftprintf(stderr, _T("Failed to open VCD %c: %x\n"), Letter, GetLastError());
            CloseHandle(hFile);
            return 1;
        }

        /* We have to release image now, the driver will attempt to open it */
        CloseHandle(hFile);

        /* Issue the mount IOCTL */
        Res = DeviceIoControl(hDev, IOCTL_VCDROM_MOUNT_IMAGE, &MountParams, sizeof(MountParams), NULL, 0, &BytesRead, NULL);
        if (!Res)
        {
            _ftprintf(stderr, _T("Failed to mount %s on %c: %x\n"), argv[3], Letter, GetLastError());
            CloseHandle(hDev);
            return 1;
        }

        /* Pretty print in case of a success */
        _ftprintf(stdout, _T("%s mounted on %c\n"), argv[3], Letter);

        CloseHandle(hDev);

        /* Should it persistent? */
        if (bPersist)
        {
            /* Create the registry key Device<Letter> */
            _stprintf(szBuffer, _T("SYSTEM\\CurrentControlSet\\Services\\Vcdrom\\Parameters\\Device%c"), Letter);
            if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
            {
                /* Set the image path */
                _tcsncpy(szBuffer, MountParams.Path, MountParams.Length);
                szBuffer[MountParams.Length / sizeof(TCHAR)] = 0;
                RegSetValueExW(hKey, _T("IMAGE"), 0, REG_SZ, (BYTE *)szBuffer, MountParams.Length);

                /* Set the drive letter */
                szBuffer[0] = Letter;
                szBuffer[1] = _T(':');
                szBuffer[2] = 0;
                RegSetValueExW(hKey, _T("DRIVE"), 0, REG_SZ, (BYTE *)szBuffer, 3 * sizeof(TCHAR));

                RegCloseKey(hKey);
            }
            else
            {
                _ftprintf(stderr, _T("Failed to make mounting persistent: %x\n"), GetLastError());
            }
        }
    }
    else if (_tcscmp(argv[1], _T("remount")) == 0)
    {
        WCHAR Letter;

        /* We need an arg */
        if (argc < 3)
        {
            PrintUsage(2);
            return 1;
        }

        /* First, check letter is OK */
        if (!_istalpha(argv[2][0]) || argv[2][1] != 0)
        {
            PrintUsage(2);
            return 1;
        }

        /* Validate the drive is owned by vcdrom */
        Letter = _totupper(argv[2][0]);
        if (!IsLetterOwned(Letter))
        {
            return 1;
        }

        /* Open the drive */
        hDev = OpenLetter(Letter);
        if (hDev == INVALID_HANDLE_VALUE)
        {
            _ftprintf(stderr, _T("Failed to open VCD %c: %x\n"), Letter, GetLastError());
            return 1;
        }

        /* Issue the remount IOCTL */
        Res = DeviceIoControl(hDev, IOCTL_STORAGE_LOAD_MEDIA, NULL, 0, NULL, 0, &BytesRead, NULL);
        if (!Res)
        {
            _ftprintf(stderr, _T("Failed to remount media on %c: %x\n"), Letter, GetLastError());
            CloseHandle(hDev);
            return 1;
        }

        /* Pretty print in case of a success */
        _ftprintf(stdout, _T("Media remounted on %c\n"), Letter);

        CloseHandle(hDev);
    }
    else if (_tcscmp(argv[1], _T("eject")) == 0)
    {
        WCHAR Letter;

        /* We need an arg */
        if (argc < 3)
        {
            PrintUsage(3);
            return 1;
        }

        /* First, check letter is OK */
        if (!_istalpha(argv[2][0]) || argv[2][1] != 0)
        {
            PrintUsage(3);
            return 1;
        }

        /* Validate the drive is owned by vcdrom */
        Letter = _totupper(argv[2][0]);
        if (!IsLetterOwned(Letter))
        {
            return 1;
        }

        /* Open the drive */
        hDev = OpenLetter(Letter);
        if (hDev == INVALID_HANDLE_VALUE)
        {
            _ftprintf(stderr, _T("Failed to open VCD %c: %x\n"), Letter, GetLastError());
            return 1;
        }

        /* Issue the eject IOCTL */
        Res = DeviceIoControl(hDev, IOCTL_CDROM_EJECT_MEDIA, NULL, 0, NULL, 0, &BytesRead, NULL);
        if (!Res)
        {
            _ftprintf(stderr, _T("Failed to eject media on %c: %x\n"), Letter, GetLastError());
            CloseHandle(hDev);
            return 1;
        }

        /* Pretty print in case of a success */
        _ftprintf(stdout, _T("Media ejected on %c\n"), Letter);

        CloseHandle(hDev);
    }
    else if (_tcscmp(argv[1], _T("remove")) == 0)
    {
        WCHAR Letter;

        /* We need an arg */
        if (argc < 3)
        {
            PrintUsage(4);
            return 1;
        }

        /* First, check letter is OK */
        if (!_istalpha(argv[2][0]) || argv[2][1] != 0)
        {
            PrintUsage(4);
            return 1;
        }

        /* Validate the drive is owned by vcdrom */
        Letter = _totupper(argv[2][0]);
        if (!IsLetterOwned(Letter))
        {
            return 1;
        }

        /* Open the drive */
        hDev = OpenLetter(Letter);
        if (hDev == INVALID_HANDLE_VALUE)
        {
            _ftprintf(stderr, _T("Failed to open VCD %c: %x\n"), Letter, GetLastError());
            return 1;
        }

        /* Issue the remove IOCTL */
        Res = DeviceIoControl(hDev, IOCTL_VCDROM_DELETE_DRIVE, NULL, 0, NULL, 0, &BytesRead, NULL);
        if (!Res)
        {
            _ftprintf(stderr, _T("Failed to remove virtual drive %c: %x\n"), Letter, GetLastError());
            CloseHandle(hDev);
            return 1;
        }

        /* Pretty print in case of a success */
        _ftprintf(stdout, _T("Virtual drive %c removed\n"), Letter);

        CloseHandle(hDev);
    }
    else
    {
        PrintUsage(0);
    }

    return 0;
}
