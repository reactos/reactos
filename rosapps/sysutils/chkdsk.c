//======================================================================
//
// $Id: chkdsk.c,v 1.3 2000/04/25 23:22:57 ea Exp $
//
// Chkdskx
//
// Copyright (c) 1998 Mark Russinovich
//	Systems Internals
//	http://www.sysinternals.com/
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
// Chkdsk clone that demonstrates the use of the FMIFS file system
// utility library.
//
// 1999 February (Emanuele Aliberti)
// 	Adapted for ReactOS and lcc-win32.
// 	
// 1999 April (Emanuele Aliberti)
// 	Adapted for ReactOS and egcs.
//
//======================================================================
#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <fmifs.h>
#define _UNICODE 1
#include <tchar.h>
#include "config.h"
#include "win32err.h"

//
// Globals
//
BOOL	Error = FALSE;

// switches
BOOL	FixErrors = FALSE;
BOOL	SkipClean = FALSE;
BOOL	ScanSectors = FALSE;
BOOL	Verbose = FALSE;
PWCHAR  Drive = NULL;
WCHAR	CurrentDirectory[1024];

#ifndef FMIFS_IMPORT_DLL
//
// FMIFS function
//
//PCHKDSK   Chkdsk;
#endif /* ndef FMIFS_IMPORT_DLL */


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
CtrlCIntercept( DWORD dwCtrlType )
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
// 19990216 EA Missing printf %s argument
//----------------------------------------------------------------------
VOID
Usage( PWCHAR ProgramName )
{
	_tprintf(
		L"\
Usage: %s [drive:] [-F] [-V] [-R] [-C]\n\n\
  [drive:]    Specifies the drive to check.\n\
  -F          Fixes errors on the disk.\n\
  -V          Displays the full path of every file on the disk.\n\
  -R          Locates bad sectors and recovers readable information.\n\
  -C          Checks the drive only if it is dirty.\n\n",
	  	ProgramName
		);
}


//----------------------------------------------------------------------
//
// ParseCommandLine
//
// Get the switches.
//
//----------------------------------------------------------------------
int
ParseCommandLine(
	int	argc,
	WCHAR	*argv []
	)
{
	int	i;
	BOOLEAN gotFix = FALSE;
	BOOLEAN gotVerbose = FALSE;
	BOOLEAN gotClean = FALSE;
	/*BOOLEAN gotScan = FALSE;*/


	for (	i = 1;
		(i < argc);
		i++
	) {
		switch( argv[i][0] )
		{
		case L'-':
		case L'/':

			switch( argv[i][1] )
			{
			case L'F':
			case L'f':

				if( gotFix ) return i;
				FixErrors = TRUE;
				gotFix = TRUE;
				break;

			case L'V':
			case L'v':

				if( gotVerbose) return i;
				Verbose = TRUE;
				gotVerbose = TRUE;
				break;

			case L'R':
			case L'r':

				if( gotFix ) return i;
				ScanSectors = TRUE;
				gotFix = TRUE;
				break;

			case L'C':
			case L'c':

				if( gotClean ) return i;
				SkipClean = TRUE;
				gotClean = TRUE;
				break;

			default:
				return i;
			}
			break;

		default:

			if( Drive ) return i;
			if( argv[i][1] != L':' ) return i;

			Drive = argv[i];
			break;
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
STDCALL
ChkdskCallback(
	CALLBACKCOMMAND	Command,
	DWORD		Modifier,
	PVOID		Argument
	)
{
	PDWORD		percent;
	PBOOLEAN	status;
	PTEXTOUTPUT	output;

	// 
	// We get other types of commands,
	// but we don't have to pay attention to them
	//
	switch( Command )
	{
	case UNKNOWN2:
		wprintf(L"UNKNOWN2\r");
		break;
		
	case UNKNOWN3:
		wprintf(L"UNKNOWN3\r");
		break;
		
	case UNKNOWN4:
		wprintf(L"UNKNOWN4\r");
		break;
		
	case UNKNOWN5:
		wprintf(L"UNKNOWN5\r");
		break;
		
	case UNKNOWN7:
		wprintf(L"UNKNOWN7\r");
		break;
		
	case UNKNOWN8:
		wprintf(L"UNKNOWN8\r");
		break;
		
	case UNKNOWN9:
		wprintf(L"UNKNOWN9\r");
		break;
		
	case UNKNOWNA:
		wprintf(L"UNKNOWNA\r");
		break;
		
	case UNKNOWNC:
		wprintf(L"UNKNOWNC\r");
		break;
		
	case UNKNOWND:
		wprintf(L"UNKNOWND\r");
		break;
		
	case INSUFFICIENTRIGHTS:
		wprintf(L"INSUFFICIENTRIGHTS\r");
		break;
		
	case STRUCTUREPROGRESS:
		wprintf(L"STRUCTUREPROGRESS\r");
		break;
		
	case DONEWITHSTRUCTURE:
		wprintf(L"DONEWITHSTRUCTURE\r");
		break;
		
	case PROGRESS:
		percent = (PDWORD) Argument;
		wprintf(L"%d percent completed.\r", *percent);
		break;

	case OUTPUT:
		output = (PTEXTOUTPUT) Argument;
		fwprintf(stdout, L"%s", output->Output);
		break;

	case DONE:
		status = (PBOOLEAN) Argument;
		if ( *status == TRUE )
		{
			wprintf(L"Chkdsk was unable to complete successfully.\n\n");
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
// 19990216 EA Used wide functions
//
//----------------------------------------------------------------------
BOOLEAN
LoadFMIFSEntryPoints(VOID)
{
	LoadLibraryW( L"fmifs.dll" );

	if( !(Chkdsk =
		(void *) GetProcAddress(
			GetModuleHandleW(L"fmifs.dll"),
			"Chkdsk" ))
			)
	{
		return FALSE;
	}
	return TRUE;
}
#endif /* ndef FMIFS_IMPORT_DLL */


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
wmain( int argc, WCHAR *argv[] )
{
	int	badArg;
	HANDLE	volumeHandle;
	WCHAR	fileSystem [1024];
	WCHAR	volumeName [1024];
	DWORD	serialNumber;
	DWORD	flags,
		maxComponent;

	wprintf(
		L"\n\
Chkdskx v1.0.1 by Mark Russinovich\n\
Systems Internals - http://www.sysinternals.com/\n\
ReactOS adaptation 1999 by Emanuele Aliberti\n\n"
		);
#ifndef FMIFS_IMPORT_DLL
	//
	// Get function pointers
	//
	if( !LoadFMIFSEntryPoints())
	{
		wprintf(L"Could not located FMIFS entry points.\n\n");
		return -1;
	}
#endif /* ndef FMIFS_IMPORT_DLL */
	//
	// Parse command line
	//
	if( (badArg = ParseCommandLine( argc, argv )))
	{
		wprintf(
			L"Unknown argument: %s\n",
			argv[badArg]
			);

		Usage(argv[0]);
		return -1;
	}

	// 
	// Get the drive's format
	//
	if( !Drive )
	{
		if( !GetCurrentDirectoryW(
			sizeof(CurrentDirectory),
			CurrentDirectory
			)
		) {

			PrintWin32Error(
				L"Could not get current directory",
				GetLastError()
				);
			return -1;
		}

	} else {

		wcscpy( CurrentDirectory, Drive );
	}
	CurrentDirectory[2] = L'\\';
	CurrentDirectory[3] = L'\0';
	Drive = CurrentDirectory;

	//
	// Determine the drive's file system format, which we need to 
	// tell chkdsk
	//
	if( !GetVolumeInformationW(
		Drive, 
		volumeName,
		sizeof volumeName, 
		& serialNumber,
		& maxComponent,
		& flags, 
		fileSystem,
		sizeof fileSystem
		)
	) {
		PrintWin32Error(
			L"Could not query volume",
			GetLastError()
			);
		return -1;
	}

	//
	// If they want to fix, we need to have access to the drive
	//
	if ( FixErrors )
	{
		swprintf(
			volumeName,
			L"\\\\.\\%C:",
			Drive[0]
			);
		volumeHandle = CreateFileW(
				volumeName,
				GENERIC_WRITE, 
				0,
				NULL,
				OPEN_EXISTING, 
				0,
				0 
				);
		if( volumeHandle == INVALID_HANDLE_VALUE )
		{
			wprintf(L"Chdskx cannot run because the volume is in use by another process.\n\n");
			return -1;
		}
		CloseHandle( volumeHandle );

		//
		// Can't let the user break out of a chkdsk that can modify the drive
		//
		SetConsoleCtrlHandler( CtrlCIntercept, TRUE );
	}

	//
	// Just do it
	//
	wprintf(
		L"The type of file system is %s.\n",
		fileSystem
		);
	Chkdsk(
		Drive,
		fileSystem,
		FixErrors,
		Verbose,
		SkipClean,
		ScanSectors,
		NULL,
		NULL,
		ChkdskCallback
		);

	if ( Error ) return -1;
	return 0;
}

/* EOF */
