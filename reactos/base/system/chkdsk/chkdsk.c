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
static VOID PrintWin32Error(LPWSTR Message, DWORD ErrorCode)
{
    ConPrintf(StdErr, L"%s: ", Message);
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
    ConPrintf(StdOut,
        L"Usage: %s [drive:] [-F] [-V] [-R] [-C]\n\n"
        L"[drive:]    Specifies the drive to check.\n"
        L"-F          Fixes errors on the disk.\n"
        L"-V          Displays the full path of every file on the disk.\n"
        L"-R          Locates bad sectors and recovers readable information.\n"
        L"-C          Checks the drive only if it is dirty.\n\n",
        ProgramName);
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
    PDWORD      percent;
    PBOOLEAN    status;
    PTEXTOUTPUT output;

    //
    // We get other types of commands,
    // but we don't have to pay attention to them
    //
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
            ConPuts(StdOut, L"VOLUMEINUSE\n");
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
            ConPrintf(StdOut, L"%d percent completed.\r", *percent);
            break;

        case OUTPUT:
            output = (PTEXTOUTPUT)Argument;
            ConPrintf(StdOut, L"%S", output->Output);
            break;

        case DONE:
            status = (PBOOLEAN)Argument;
            if (*status == FALSE)
            {
                ConPuts(StdOut, L"Chkdsk was unable to complete successfully.\n\n");
                Error = TRUE;
            }
            break;
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

    ConPuts(StdOut,
        L"\n"
        L"Chkdskx v1.0.1 by Mark Russinovich\n"
        L"Systems Internals - http://www.sysinternals.com\n"
        L"ReactOS adaptation 1999 by Emanuele Aliberti\n\n");

#ifndef FMIFS_IMPORT_DLL
    //
    // Get function pointers
    //
    if (!LoadFMIFSEntryPoints())
    {
        ConPuts(StdErr, L"Could not located FMIFS entry points.\n\n");
        return -1;
    }
#endif

    //
    // Parse command line
    //
    badArg = ParseCommandLine(argc, argv);
    if (badArg)
    {
        ConPrintf(StdOut, L"Unknown argument: %s\n", argv[badArg]);
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
            PrintWin32Error(L"Could not get current directory", GetLastError());
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
        PrintWin32Error(L"Could not query volume", GetLastError());
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
            ConPuts(StdErr, L"Chkdsk cannot run because the volume is in use by another process.\n\n");
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
    ConPrintf(StdOut, L"The type of file system is %s.\n", fileSystem);
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
