//======================================================================
//
// Formatx
//
// Copyright (c) 1998 Mark Russinovich
// Systems Internals
// http://www.sysinternals.com
//
// Format clone that demonstrates the use of the FMIFS file system
// utility library.
//
// --------------------------------------------------------------------
//
// This software is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this software; see the file COPYING.LIB. If
// not, write to the Free Software Foundation, Inc., 675 Mass Ave,
// Cambridge, MA 02139, USA.
//
// --------------------------------------------------------------------
//
// 1999 February (Emanuele Aliberti)
//      Adapted for ReactOS and lcc-win32.
//
// 1999 April (Emanuele Aliberti)
//      Adapted for ReactOS and egcs.
//
// 2003 April (Casper S. Hornstrup)
//      Reintegration.
//
//======================================================================

#include <stdio.h>
#include <tchar.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>

#include <conutils.h>

#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>

/* FMIFS Public Header */
#include <fmifs/fmifs.h>

#include "resource.h"

#define FMIFS_IMPORT_DLL

// Globals
BOOL    Error = FALSE;

// Switches
BOOL    QuickFormat = FALSE;
DWORD   ClusterSize = 0;
BOOL    CompressDrive = FALSE;
BOOL    GotALabel = FALSE;
PWCHAR  Label = L"";
PWCHAR  Drive = NULL;
PWCHAR  FileSystem = L"FAT";

WCHAR   RootDirectory[MAX_PATH];
WCHAR   LabelString[12];

#ifndef FMIFS_IMPORT_DLL
//
// Functions in FMIFS.DLL
//
PFORMATEX FormatEx;
PENABLEVOLUMECOMPRESSION EnableVolumeCompression;
PQUERYAVAILABLEFILESYSTEMFORMAT QueryAvailableFileSystemFormat;
#endif


//
// Size array
//
typedef struct {
    WCHAR SizeString[16];
    DWORD ClusterSize;
} SIZEDEFINITION, *PSIZEDEFINITION;

SIZEDEFINITION LegalSizes[] = {
    { L"512", 512 },
    { L"1024", 1024 },
    { L"2048", 2048 },
    { L"4096", 4096 },
    { L"8192", 8192 },
    { L"16K", 16384 },
    { L"32K", 32768 },
    { L"64K", 65536 },
    { L"128K", 65536 * 2 },
    { L"256K", 65536 * 4 },
    { L"", 0 },
};


//----------------------------------------------------------------------
//
// PrintWin32Error
//
// Takes the win32 error code and prints the text version.
//
//----------------------------------------------------------------------
static VOID PrintWin32Error(LPWSTR Message, DWORD ErrorCode)
{
    ConPrintf(StdErr, L"%s: ", Message);
    ConMsgPuts(StdErr, FORMAT_MESSAGE_FROM_SYSTEM,
               NULL, ErrorCode, LANG_USER_DEFAULT);
    ConPuts(StdErr, L"\n");
}


//----------------------------------------------------------------------
//
// ParseCommandLine
//
// Get the switches.
//
//----------------------------------------------------------------------
static int ParseCommandLine(int argc, WCHAR *argv[])
{
    int i, j;
    BOOLEAN gotFormat = FALSE;
    BOOLEAN gotQuick = FALSE;
    BOOLEAN gotSize = FALSE;
    BOOLEAN gotLabel = FALSE;
    BOOLEAN gotCompressed = FALSE;

    for (i = 1; i < argc; i++)
    {
        switch (argv[i][0])
        {
            case L'-': case L'/':

                if (!_wcsnicmp(&argv[i][1], L"FS:", 3))
                {
                    if (gotFormat) return -1;
                    FileSystem = &argv[i][4];
                    gotFormat = TRUE;
                }
                else if (!_wcsnicmp(&argv[i][1], L"A:", 2))
                {
                    if (gotSize) return -1;
                    j = 0;
                    while (LegalSizes[j].ClusterSize &&
                           wcsicmp(LegalSizes[j].SizeString, &argv[i][3]))
                    {
                        j++;
                    }

                    if (!LegalSizes[j].ClusterSize) return i;
                    ClusterSize = LegalSizes[j].ClusterSize;
                    gotSize = TRUE;
                }
                else if (!_wcsnicmp(&argv[i][1], L"V:", 2))
                {
                    if (gotLabel) return -1;
                    Label = &argv[i][3];
                    gotLabel = TRUE;
                    GotALabel = TRUE;
                }
                else if (!wcsicmp(&argv[i][1], L"Q"))
                {
                    if (gotQuick) return -1;
                    QuickFormat = TRUE;
                    gotQuick = TRUE;
                }
                else if (!wcsicmp(&argv[i][1], L"C"))
                {
                    if (gotCompressed) return -1;
                    CompressDrive = TRUE;
                    gotCompressed = TRUE;
                }
                else
                {
                    return i;
                }
                break;

            default:
            {
                if (Drive) return i;
                if (argv[i][1] != L':') return i;

                Drive = argv[i];
                break;
            }
        }
    }
    return 0;
}

//----------------------------------------------------------------------
//
// FormatExCallback
//
// The file system library will call us back with commands that we
// can interpret. If we wanted to halt the chkdsk we could return FALSE.
//
//----------------------------------------------------------------------
BOOLEAN WINAPI
FormatExCallback(
    CALLBACKCOMMAND Command,
    ULONG Modifier,
    PVOID Argument)
{
    PDWORD      percent;
    PTEXTOUTPUT output;
    PBOOLEAN    status;

    //
    // We get other types of commands, but we don't have to pay attention to them
    //
    switch (Command)
    {
        case PROGRESS:
            percent = (PDWORD)Argument;
            ConResPrintf(StdOut, STRING_COMPLETE, *percent);
            break;

        case OUTPUT:
            output = (PTEXTOUTPUT)Argument;
            ConPrintf(StdOut, L"%S\n", output->Output);
            break;

        case DONE:
            status = (PBOOLEAN)Argument;
            if (*status == FALSE)
            {
                ConResPuts(StdOut, STRING_FORMAT_FAIL);
                Error = TRUE;
            }
            break;

        case DONEWITHSTRUCTURE:
        case UNKNOWN2:
        case UNKNOWN3:
        case UNKNOWN4:
        case UNKNOWN5:
        case INSUFFICIENTRIGHTS:
        case FSNOTSUPPORTED:
        case VOLUMEINUSE:
        case UNKNOWN9:
        case UNKNOWNA:
        case UNKNOWNC:
        case UNKNOWND:
        case STRUCTUREPROGRESS:
        case CLUSTERSIZETOOSMALL:
            ConResPuts(StdOut, STRING_NO_SUPPORT);
            return FALSE;
    }
    return TRUE;
}

#ifndef FMIFS_IMPORT_DLL
//----------------------------------------------------------------------
//
// LoadFMIFSEntryPoints
//
// Loads FMIFS.DLL and locates the entry point(s) we are going to use
//
//----------------------------------------------------------------------
static BOOLEAN LoadFMIFSEntryPoints(VOID)
{
    HMODULE hFmifs = LoadLibraryW( L"fmifs.dll");
    if (hFmifs == NULL)
        return FALSE;

    FormatEx = (PFORMATEX)GetProcAddress(hFmifs, "FormatEx");
    if (!FormatEx)
    {
        FreeLibrary(hFmifs);
        return FALSE;
    }

    EnableVolumeCompression = (PENABLEVOLUMECOMPRESSION)GetProcAddress(hFmifs, "EnableVolumeCompression");
    if (!EnableVolumeCompression)
    {
        FreeLibrary(hFmifs);
        return FALSE;
    }

    QueryAvailableFileSystemFormat = (PQUERYAVAILABLEFILESYSTEMFORMAT)GetProcAddress(hFmifs, "QueryAvailableFileSystemFormat");
    if (!QueryAvailableFileSystemFormat)
    {
        FreeLibrary(hFmifs);
        return FALSE;
    }

    return TRUE;
}
#endif


//----------------------------------------------------------------------
//
// Usage
//
// Tell the user how to use the program
//
//----------------------------------------------------------------------
static VOID Usage(LPWSTR ProgramName)
{
    WCHAR szMsg[RC_STRING_MAX_SIZE];
    WCHAR szFormats[MAX_PATH];
    WCHAR szFormatW[MAX_PATH];
    DWORD Index = 0;
    BYTE dummy;
    BOOLEAN latestVersion;

    K32LoadStringW(GetModuleHandle(NULL), STRING_HELP, szMsg, ARRAYSIZE(szMsg));

#ifndef FMIFS_IMPORT_DLL
    if (!LoadFMIFSEntryPoints())
    {
        ConPrintf(StdOut, szMsg, ProgramName, L"");
        return;
    }
#endif

    szFormats[0] = 0;
    while (QueryAvailableFileSystemFormat(Index++, szFormatW, &dummy, &dummy, &latestVersion))
    {
        if (!latestVersion)
            continue;
        if (szFormats[0])
            wcscat(szFormats, L", ");

        wcscat(szFormats, szFormatW);
    }
    ConPrintf(StdOut, szMsg, ProgramName, szFormats);
}


//----------------------------------------------------------------------
//
// WMain
//
// Engine. Just get command line switches and fire off a format. This
// could also be done in a GUI like Explorer does when you select a
// drive and run a check on it.
//
// We do this in UNICODE because the chkdsk command expects PWCHAR
// arguments.
//
//----------------------------------------------------------------------
int wmain(int argc, WCHAR *argv[])
{
    int badArg;
    DEVICE_INFORMATION DeviceInformation = {0};
    FMIFS_MEDIA_FLAG media = FMIFS_HARDDISK;
    DWORD driveType;
    WCHAR fileSystem[1024];
    WCHAR volumeName[1024] = {0};
    WCHAR input[1024];
    DWORD serialNumber;
    DWORD flags, maxComponent;
    ULARGE_INTEGER freeBytesAvailableToCaller, totalNumberOfBytes, totalNumberOfFreeBytes;
    WCHAR szMsg[RC_STRING_MAX_SIZE];

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    ConPuts(StdOut,
        L"\n"
        L"Formatx v1.0 by Mark Russinovich\n"
        L"Systems Internals - http://www.sysinternals.com\n"
        L"ReactOS adaptation 1999 by Emanuele Aliberti\n\n");

#ifndef FMIFS_IMPORT_DLL
    //
    // Get function pointers
    //
    if (!LoadFMIFSEntryPoints())
    {
        ConResPuts(StdErr, STRING_FMIFS_FAIL);
        return -1;
    }
#endif

    //
    // Parse command line
    //
    badArg = ParseCommandLine(argc, argv);
    if (badArg)
    {
        ConResPrintf(StdErr, STRING_UNKNOW_ARG, argv[badArg]);
        Usage(argv[0]);
        return -1;
    }

    //
    // Get the drive's format
    //
    if (!Drive)
    {
        ConResPuts(StdErr, STRING_DRIVE_PARM);
        Usage(argv[0]);
        return -1;
    }
    else
    {
        wcscpy(RootDirectory, Drive);
    }
    RootDirectory[2] = L'\\';
    RootDirectory[3] = L'\0';

    //
    // See if the drive is removable or not
    //
    driveType = GetDriveTypeW(RootDirectory);
    switch (driveType)
    {
        case DRIVE_UNKNOWN :
            K32LoadStringW(GetModuleHandle(NULL), STRING_ERROR_DRIVE_TYPE, szMsg, ARRAYSIZE(szMsg));
            PrintWin32Error(szMsg, GetLastError());
            return -1;

        case DRIVE_REMOTE:
        case DRIVE_CDROM:
            ConResPuts(StdOut, STRING_NO_SUPPORT);
            return -1;

        case DRIVE_NO_ROOT_DIR:
            K32LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME, szMsg, ARRAYSIZE(szMsg));
            PrintWin32Error(szMsg, GetLastError());
            return -1;

        case DRIVE_REMOVABLE:
            ConResPrintf(StdOut, STRING_INSERT_DISK, RootDirectory[0]);
            fgetws(input, ARRAYSIZE(input), stdin);
            media = FMIFS_FLOPPY;
            break;

        case DRIVE_FIXED:
        case DRIVE_RAMDISK:
            media = FMIFS_HARDDISK;
            break;
    }

    // Reject attempts to format the system drive
    {
        WCHAR path[MAX_PATH + 1];
        UINT rc;
        rc = GetWindowsDirectoryW(path, MAX_PATH);
        if (rc == 0 || rc > MAX_PATH)
            // todo: Report "Unable to query system directory"
            return -1;
        if (towlower(path[0]) == towlower(Drive[0]))
        {
            // todo: report "Cannot format system drive"
            ConResPuts(StdOut, STRING_NO_SUPPORT);
            return -1;
        }
    }

    //
    // Determine the drive's file system format
    //
    if (!GetVolumeInformationW(RootDirectory,
                               volumeName, ARRAYSIZE(volumeName),
                               &serialNumber, &maxComponent, &flags,
                               fileSystem, ARRAYSIZE(fileSystem)))
    {
        if (GetLastError() == ERROR_UNRECOGNIZED_VOLUME)
        {
            wcscpy(fileSystem, L"RAW");
        }
        else
        {
            K32LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME, szMsg, ARRAYSIZE(szMsg));
            PrintWin32Error(szMsg, GetLastError());
            return -1;
        }
    }

    if (QueryDeviceInformation(RootDirectory,
                               &DeviceInformation,
                               sizeof(DeviceInformation)))
    {
        totalNumberOfBytes.QuadPart = DeviceInformation.SectorSize *
                                      DeviceInformation.SectorCount.QuadPart;
    }

    /* QueryDeviceInformation returns more accurate volume length and works with
     * unformatted volumes, however it will NOT return volume length on XP/2003.
     * Fallback to GetFreeDiskSpaceExW if we did not get any volume length. */
    if (totalNumberOfBytes.QuadPart == 0 &&
        !GetDiskFreeSpaceExW(RootDirectory,
                             &freeBytesAvailableToCaller,
                             &totalNumberOfBytes,
                             &totalNumberOfFreeBytes))
    {
        K32LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME_SIZE, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }
    ConResPrintf(StdOut, STRING_FILESYSTEM, fileSystem);

    //
    // Make sure they want to do this
    //
    if (driveType == DRIVE_FIXED)
    {
        if (volumeName[0])
        {
            while (TRUE)
            {
                ConResPrintf(StdOut, STRING_LABEL_NAME_EDIT, RootDirectory[0]);
                fgetws(input, ARRAYSIZE(input), stdin);
                input[wcslen(input) - 1] = 0;

                if (!wcsicmp(input, volumeName))
                    break;

                ConResPuts(StdOut, STRING_ERROR_LABEL);
            }
        }

        ConResPrintf(StdOut, STRING_YN_FORMAT, RootDirectory[0]);

        K32LoadStringW(GetModuleHandle(NULL), STRING_YES_NO_FAQ, szMsg, ARRAYSIZE(szMsg));
        while (TRUE)
        {
            fgetws(input, ARRAYSIZE(input), stdin);
            if (_wcsnicmp(&input[0], &szMsg[0], 1) == 0) break;
            if (_wcsnicmp(&input[0], &szMsg[1], 1) == 0)
            {
                ConPuts(StdOut, L"\n");
                return 0;
            }
        }
    }

    //
    // Tell the user we're doing a long format if appropriate
    //
    if (!QuickFormat)
    {
        K32LoadStringW(GetModuleHandle(NULL), STRING_VERIFYING, szMsg, ARRAYSIZE(szMsg));
        if (totalNumberOfBytes.QuadPart > 1024*1024*10)
        {
            ConPrintf(StdOut, L"%s %luM\n", szMsg, (DWORD)(totalNumberOfBytes.QuadPart/(1024*1024)));
        }
        else
        {
            ConPrintf(StdOut, L"%s %.1fM\n", szMsg,
                ((float)(LONGLONG)totalNumberOfBytes.QuadPart)/(float)(1024.0*1024.0));
        }
    }
    else
    {
        K32LoadStringW(GetModuleHandle(NULL), STRING_FAST_FMT, szMsg, ARRAYSIZE(szMsg));
        if (totalNumberOfBytes.QuadPart > 1024*1024*10)
        {
            ConPrintf(StdOut, L"%s %luM\n", szMsg, (DWORD)(totalNumberOfBytes.QuadPart/(1024*1024)));
        }
        else
        {
            ConPrintf(StdOut, L"%s %.2fM\n", szMsg,
                ((float)(LONGLONG)totalNumberOfBytes.QuadPart)/(float)(1024.0*1024.0));
        }
        ConResPuts(StdOut, STRING_CREATE_FSYS);
    }

    //
    // Format away!
    //
    FormatEx(RootDirectory, media, FileSystem, Label, QuickFormat,
             ClusterSize, FormatExCallback);
    if (Error) return -1;
    ConPuts(StdOut, L"\n");
    ConResPuts(StdOut, STRING_FMT_COMPLETE);

    //
    // Enable compression if desired
    //
    if (CompressDrive)
    {
        if (!EnableVolumeCompression(RootDirectory, TRUE))
            ConResPuts(StdOut, STRING_VOL_COMPRESS);
    }

    //
    // Get the label if we don't have it
    //
    if (!GotALabel)
    {
        ConResPuts(StdOut, STRING_ENTER_LABEL);
        fgetws(input, ARRAYSIZE(LabelString), stdin);

        input[wcslen(input) - 1] = 0;
        if (!SetVolumeLabelW(RootDirectory, input))
        {
            K32LoadStringW(GetModuleHandle(NULL), STRING_NO_LABEL, szMsg, ARRAYSIZE(szMsg));
            PrintWin32Error(szMsg, GetLastError());
            return -1;
        }
    }

    if (!GetVolumeInformationW(RootDirectory,
                               volumeName, ARRAYSIZE(volumeName),
                               &serialNumber, &maxComponent, &flags,
                               fileSystem, ARRAYSIZE(fileSystem)))
    {
        K32LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }

    //
    // Print out some stuff including the formatted size
    //
    if (!GetDiskFreeSpaceExW(RootDirectory,
                             &freeBytesAvailableToCaller,
                             &totalNumberOfBytes,
                             &totalNumberOfFreeBytes))
    {
        K32LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME_SIZE, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }

    ConResPrintf(StdOut, STRING_FREE_SPACE, totalNumberOfBytes.QuadPart,
                                            totalNumberOfFreeBytes.QuadPart);

    //
    // Get the drive's serial number
    //
    if (!GetVolumeInformationW(RootDirectory,
                               volumeName, ARRAYSIZE(volumeName),
                               &serialNumber, &maxComponent, &flags,
                               fileSystem, ARRAYSIZE(fileSystem)))
    {
        K32LoadStringW(GetModuleHandle(NULL), STRING_NO_VOLUME, szMsg, ARRAYSIZE(szMsg));
        PrintWin32Error(szMsg, GetLastError());
        return -1;
    }
    ConResPrintf(StdOut, STRING_SERIAL_NUMBER,
                         (unsigned int)(serialNumber >> 16),
                         (unsigned int)(serialNumber & 0xFFFF));

    return 0;
}

/* EOF */
