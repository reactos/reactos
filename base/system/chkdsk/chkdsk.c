//======================================================================
//
// Chkdskx
//
// Copyright (c) 1998 Mark Russinovich
// Systems Internals
// http://www.sysinternals.com
//
// Chkdsk clone that demonstrates the use of the FMIFS file system
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
// 2008 July (Aleksey Bragin)
//      Cleanup, use ReactOS's fmifs.h
//
//======================================================================

#include <stdio.h>

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <wincon.h>

#include <conutils.h>

/* Resource header */
#include "resource.h"

#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* FMIFS Public Header */
#include <fmifs/fmifs.h>

#define FMIFS_IMPORT_DLL

//
// Globals
//
BOOL    Error = FALSE;

// Switches
BOOL    FixErrors = FALSE;
BOOL    SkipClean = FALSE;
BOOL    ScanSectors = FALSE;
BOOL    Verbose = FALSE;
PWCHAR  Drive = NULL;
WCHAR   CurrentDirectory[1024];

#ifndef FMIFS_IMPORT_DLL
//
// Functions in FMIFS.DLL
//
PCHKDSK Chkdsk;
#endif


//----------------------------------------------------------------------
//
// PrintWin32Error
//
// Takes the win32 error code and prints the text version.
//
//----------------------------------------------------------------------
static VOID PrintWin32Error(int Message, DWORD ErrorCode)
{
    ConResPuts(StdErr, Message);
    ConMsgPuts(StdErr, FORMAT_MESSAGE_FROM_SYSTEM,
               NULL, ErrorCode, LANG_USER_DEFAULT);
    ConPuts(StdErr, L"\n");
}


//--------------------------------------------------------------------
//
// CtrlCIntercept
//
// Intercepts Ctrl-C's so that the program can't be quit with the
// disk in an inconsistent state.
//
//--------------------------------------------------------------------
BOOL
WINAPI
CtrlCIntercept(DWORD dwCtrlType)
{
    //
    // Handle the event so that the default handler doesn't
    //
    return TRUE;
}


//----------------------------------------------------------------------
//
// Usage
//
// Tell the user how to use the program
//
//----------------------------------------------------------------------
static VOID
Usage(PWCHAR ProgramName)
{
    ConResPrintf(StdOut, IDS_USAGE, ProgramName);
}


//----------------------------------------------------------------------
//
// ParseCommandLine
//
// Get the switches.
//
//----------------------------------------------------------------------
static int
ParseCommandLine(int argc, WCHAR *argv[])
{
    int i;
    BOOLEAN gotFix = FALSE;
    BOOLEAN gotVerbose = FALSE;
    BOOLEAN gotClean = FALSE;
    // BOOLEAN gotScan = FALSE;

    for (i = 1; i < argc; i++)
    {
        switch (argv[i][0])
        {
            case L'-': case L'/':

                switch (argv[i][1])
                {
                    // case L'?':
                        // Usage(argv[0]);
                        // return i;

                    case L'F': case L'f':
                    {
                        if (gotFix) return i;
                        FixErrors = TRUE;
                        gotFix = TRUE;
                        break;
                    }

                    case L'V': case L'v':
                    {
                        if (gotVerbose) return i;
                        Verbose = TRUE;
                        gotVerbose = TRUE;
                        break;
                    }

                    case L'R': case L'r':
                    {
                        if (gotFix) return i;
                        ScanSectors = TRUE;
                        gotFix = TRUE;
                        break;
                    }

                    case L'C': case L'c':
                    {
                        if (gotClean) return i;
                        SkipClean = TRUE;
                        gotClean = TRUE;
                        break;
                    }

                    default:
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
// ChkdskCallback
//
// The file system library will call us back with commands that we
// can interpret. If we wanted to halt the chkdsk we could return FALSE.
//
//----------------------------------------------------------------------
BOOLEAN
WINAPI
ChkdskCallback(
    CALLBACKCOMMAND Command,
    DWORD       Modifier,
    PVOID       Argument)
{
    BOOLEAN     Ret;
    PDWORD      percent;
    PBOOLEAN    status;
    PTEXTOUTPUT output;

    //
    // We get other types of commands,
    // but we don't have to pay attention to them
    //
    Ret = TRUE;
    switch (Command)
    {
        case UNKNOWN2:
            ConPuts(StdOut, L"UNKNOWN2\r");
            break;

        case UNKNOWN3:
            ConPuts(StdOut, L"UNKNOWN3\n");
            break;

        case UNKNOWN4:
            ConPuts(StdOut, L"UNKNOWN4\n");
            break;

        case UNKNOWN5:
            ConPuts(StdOut, L"UNKNOWN5\n");
            break;

        case FSNOTSUPPORTED:
            ConPuts(StdOut, L"FSNOTSUPPORTED\n");
            break;

        case VOLUMEINUSE:
            ConResPuts(StdOut, IDS_VOLUME_IN_USE);
            Ret = FALSE;
            break;

        case UNKNOWN9:
            ConPuts(StdOut, L"UNKNOWN9\n");
            break;

        case UNKNOWNA:
            ConPuts(StdOut, L"UNKNOWNA\n");
            break;

        case UNKNOWNC:
            ConPuts(StdOut, L"UNKNOWNC\n");
            break;

        case UNKNOWND:
            ConPuts(StdOut, L"UNKNOWND\n");
            break;

        case INSUFFICIENTRIGHTS:
            ConPuts(StdOut, L"INSUFFICIENTRIGHTS\n");
            break;

        case STRUCTUREPROGRESS:
            ConPuts(StdOut, L"STRUCTUREPROGRESS\n");
            break;

        case DONEWITHSTRUCTURE:
            ConPuts(StdOut, L"DONEWITHSTRUCTURE\n");
            break;

        case CLUSTERSIZETOOSMALL:
            ConPuts(StdOut, L"CLUSTERSIZETOOSMALL\n");
            break;

        case PROGRESS:
            percent = (PDWORD)Argument;
            ConResPrintf(StdOut, IDS_PERCENT_COMPL, *percent);
            break;

        case OUTPUT:
            output = (PTEXTOUTPUT)Argument;
            ConPrintf(StdOut, L"%S", output->Output);
            break;

        case DONE:
            status = (PBOOLEAN)Argument;
            if (*status == FALSE)
            {
                ConResPuts(StdOut, IDS_CHKDSK_FAIL);
                Error = TRUE;
            }
            break;
    }
    return Ret;
}

#ifndef FMIFS_IMPORT_DLL
//----------------------------------------------------------------------
//
// LoadFMIFSEntryPoints
//
// Loads FMIFS.DLL and locates the entry point(s) we are going to use
//
//----------------------------------------------------------------------
static BOOLEAN
LoadFMIFSEntryPoints(VOID)
{
    HMODULE hFmifs = LoadLibraryW(L"fmifs.dll");
    if (hFmifs == NULL)
        return FALSE;

    Chkdsk = (PCHKDSK)GetProcAddress(hFmifs, "Chkdsk");
    if (!Chkdsk)
    {
        FreeLibrary(hFmifs);
        return FALSE;
    }

    return TRUE;
}
#endif


//----------------------------------------------------------------------
//
// WMain
//
// Engine. Just get command line switches and fire off a chkdsk. This
// could also be done in a GUI like Explorer does when you select a
// drive and run a check on it.
//
// We do this in UNICODE because the chkdsk command expects PWCHAR
// arguments.
//
//----------------------------------------------------------------------
int
wmain(int argc, WCHAR *argv[])
{
    int badArg;
    HANDLE volumeHandle;
    WCHAR  fileSystem[1024];
    WCHAR  volumeName[1024];
    DWORD  serialNumber;
    DWORD  flags, maxComponent;

    /* Initialize the Console Standard Streams */
    ConInitStdStreams();

    ConResPuts(StdOut, IDS_ABOUT);

#ifndef FMIFS_IMPORT_DLL
    //
    // Get function pointers
    //
    if (!LoadFMIFSEntryPoints())
    {
        ConResPuts(StdErr, IDS_NO_ENTRY_POINT);
        return -1;
    }
#endif

    //
    // Parse command line
    //
    badArg = ParseCommandLine(argc, argv);
    if (badArg)
    {
        ConResPrintf(StdOut, IDS_BAD_ARGUMENT, argv[badArg]);
        Usage(argv[0]);
        return -1;
    }

    //
    // Get the drive's format
    //
    if (!Drive)
    {
        if (!GetCurrentDirectoryW(ARRAYSIZE(CurrentDirectory), CurrentDirectory))
        {
            PrintWin32Error(IDS_NO_CURRENT_DIR, GetLastError());
            return -1;
        }
    }
    else
    {
        wcscpy(CurrentDirectory, Drive);
    }
    CurrentDirectory[2] = L'\\';
    CurrentDirectory[3] = L'\0';
    Drive = CurrentDirectory;

    //
    // Determine the drive's file system format, which we need to
    // tell chkdsk
    //
    if (!GetVolumeInformationW(Drive,
                               volumeName,
                               ARRAYSIZE(volumeName),
                               &serialNumber,
                               &maxComponent,
                               &flags,
                               fileSystem,
                               ARRAYSIZE(fileSystem)))
    {
        PrintWin32Error(IDS_NO_QUERY_VOL, GetLastError());
        return -1;
    }

    //
    // If they want to fix, we need to have access to the drive
    //
    if (FixErrors)
    {
        swprintf(volumeName, L"\\\\.\\%C:", Drive[0]);
        volumeHandle = CreateFileW(volumeName,
                                   GENERIC_WRITE,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   0);
        if (volumeHandle == INVALID_HANDLE_VALUE)
        {
            ConResPuts(StdErr, IDS_VOLUME_IN_USE_PROC);
            return -1;
        }
        CloseHandle(volumeHandle);

        //
        // Can't let the user break out of a chkdsk that can modify the drive
        //
        SetConsoleCtrlHandler(CtrlCIntercept, TRUE);
    }

    //
    // Just do it
    //
    ConResPrintf(StdOut, IDS_FILE_SYSTEM, fileSystem);
    Chkdsk(Drive,
           fileSystem,
           FixErrors,
           Verbose,
           SkipClean,
           ScanSectors,
           NULL,
           NULL,
           ChkdskCallback);

    if (Error) return -1;
    return 0;
}

/* EOF */
