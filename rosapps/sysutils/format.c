//======================================================================
//
// $Id: format.c,v 1.2 2000/02/29 23:57:46 ea Exp $
//
// Formatx
//
// Copyright (c) 1998 Mark Russinovich
//	Systems Internals
//	http://www.sysinternals.com/
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
// ---------------------------------------------------------------------
// Format clone that demonstrates the use of the FMIFS file system
// utility library.
//
// 1999 February (Emanuele Aliberti)
// 	Adapted for ReactOS and lcc-win32.
// 	
// 1999 April (Emanuele Aliberti)
// 	Adapted for ReactOS and egcs.
//
//======================================================================
#define UNICODE 1
#define _UNICODE 1
#include <windows.h>
#include <stdio.h>
#include "../../reactos/include/fmifs.h"
//#include <tchar.h>
#include "win32err.h"
#include "config.h"

#define WBUFSIZE(b) (sizeof(b) / sizeof (wchar_t))


//
// Globals
//
BOOL	Error = FALSE;

// switches
BOOL	QuickFormat = FALSE;
DWORD   ClusterSize = 0;
BOOL	CompressDrive = FALSE;
BOOL    GotALabel = FALSE;
PWCHAR  Label = L"";
PWCHAR  Drive = NULL;
PWCHAR  Format = L"FAT";

WCHAR  RootDirectory [MAX_PATH];
WCHAR  LabelString [12];

#ifndef FMIFS_IMPORT_DLL
//
// Functions in FMIFS.DLL
//
PFORMATEX			FormatEx;
PENABLEVOLUMECOMPRESSION	EnableVolumeCompression;
#endif /* ndef FMIFS_IMPORT_DLL */

//
// Size array
//
typedef
struct
{
	WCHAR  SizeString [16];
	DWORD  ClusterSize;
	
} SIZEDEFINITION, *PSIZEDEFINITION;


SIZEDEFINITION
LegalSizes [] =
{
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
// Usage
//
// Tell the user how to use the program
//
// 1990218 EA ProgramName missing in wprintf arg list
//
//----------------------------------------------------------------------
VOID
Usage( PWCHAR ProgramName )
{
	wprintf(
		L"\
Usage: %s drive: [-FS:file-system] [-V:label] [-Q] [-A:size] [-C]\n\n\
  [drive:]         Specifies the drive to format.\n\
  -FS:file-system  Specifies the type of file system (e.g. FAT).\n\
  -V:label         Specifies volume label.\n\
  -Q               Performs a quick format.\n\
  -A:size          Overrides the default allocation unit size. Default settings\n\
                   are strongly recommended for general use\n\
                   NTFS supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K.\n\
                   FAT supports 8192, 16K, 32K, 64K, 128K, 256K.\n\
                   NTFS compression is not supported for allocation unit sizes\n\
                   above 4096.\n\
  -C               Files created on the new volume will be compressed by\n\
                   default.\n\n",
		ProgramName
		);
}


//----------------------------------------------------------------------
//
// ParseCommandLine
//
// Get the switches.
//
// 19990218 EA switch characters '-' and '/' are wide
//
//----------------------------------------------------------------------
int
ParseCommandLine(
	int	argc,
	WCHAR	*argv []
	)
{
	int	i,
		j;
	BOOLEAN gotFormat = FALSE;
	BOOLEAN gotQuick = FALSE;
	BOOLEAN gotSize = FALSE;
	BOOLEAN gotLabel = FALSE;
	BOOLEAN gotCompressed = FALSE;


	for (
		i = 1;
		(i < argc);
		i++
	) {
		switch ( argv[i][0] )
		{

		case L'-':
		case L'/':

			if( !wcsnicmp( & argv[i][1], L"FS:", 3 ))
			{
				if( gotFormat) return -1;
				Format = & argv[i][4];
				gotFormat = TRUE;
			}
			else if( !wcsnicmp( & argv[i][1], L"A:", 2 ))
			{
				if ( gotSize ) return -1;
				j = 0; 
				while (	LegalSizes[j].ClusterSize &&
					wcsicmp(
						LegalSizes[j].SizeString,
						& argv[i][3]
						)
				) {
					j++;
				}
				if( !LegalSizes[j].ClusterSize ) return i;
				ClusterSize = LegalSizes[j].ClusterSize;
				gotSize = TRUE;
			}
			else if( !wcsnicmp( & argv[i][1], L"V:", 2 ))
			{
				if( gotLabel ) return -1;
				Label = & argv[i][3];
				gotLabel = TRUE;
				GotALabel = TRUE;
			}
			else if ( !wcsicmp( & argv[i][1], L"Q" ))
			{
				if( gotQuick ) return -1;
				QuickFormat = TRUE;
				gotQuick = TRUE;
			}
			else if ( !wcsicmp( & argv[i][1], L"C" ))
			{
				if( gotCompressed ) return -1;
				CompressDrive = TRUE;
				gotCompressed = TRUE;
			}
			else
			{
				return i;
			}
			break;

		default:

			if ( Drive ) return i;
			if ( argv[i][1] != L':' ) return i;

			Drive = argv[i];
			break;
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
BOOLEAN
__stdcall
FormatExCallback(
	CALLBACKCOMMAND Command,
	DWORD		Modifier,
	PVOID		Argument
	)
{
	PDWORD		percent;
	PTEXTOUTPUT	output;
	PBOOLEAN	status;
	//static createStructures = FALSE;

	// 
	// We get other types of commands, but we don't have to pay attention to them
	//
	switch ( Command )
	{
	case PROGRESS:
		percent = (PDWORD) Argument;
		wprintf(L"%d percent completed.\r", *percent);
		break;

	case OUTPUT:
		output = (PTEXTOUTPUT) Argument;
		fprintf(stdout, "%s", output->Output);
		break;

	case DONE:
		status = (PBOOLEAN) Argument;
		if ( *status == FALSE )
		{
			wprintf(L"FormatEx was unable to complete successfully.\n\n");
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
// 19990216 EA	ANSI strings in LoadFMIFSEntryPoints should be
//		wide strings
//
//----------------------------------------------------------------------
BOOLEAN
LoadFMIFSEntryPoints(VOID)
{
	LoadLibraryW( L"fmifs.dll" );

	if ( !(FormatEx =
		(void *) GetProcAddress(
			GetModuleHandleW( L"fmifs.dll"),
			"FormatEx"
			)
		)
	) {
		return FALSE;
	}
	if ( !(EnableVolumeCompression =
		(void *) GetProcAddress(
				GetModuleHandleW( L"fmifs.dll"),
				"EnableVolumeCompression"
				)
		)
	) {
		return FALSE;
	}
	return TRUE;
}
#endif /* ndef FMIFS_IMPORT_DLL */


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
int
wmain( int argc, WCHAR *argv[] )
{
	int		badArg;
	
	DWORD		media;
	DWORD		driveType;
	
	WCHAR		fileSystem [1024];
	WCHAR		volumeName [1024];
	WCHAR		input [1024];
	
	DWORD		serialNumber;
	DWORD		flags,
			maxComponent;
			
	ULARGE_INTEGER	freeBytesAvailableToCaller,
			totalNumberOfBytes,
			totalNumberOfFreeBytes;

			
	wprintf( L"\
\nFormatx v1.0 by Mark Russinovich\n\
Systems Internals - http://www.sysinternals.com\n\
ReactOs adaptation 1999 by Emanuele Aliberti\n\n"
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
		wprintf(L"Required drive parameter is missing.\n\n");
		Usage( argv[0] );
		return -1;
	}
	else
	{
		wcscpy( RootDirectory, Drive );
	}
	RootDirectory[2] = L'\\';
	RootDirectory[3] = L'\0';
	//
	// See if the drive is removable or not
	//
	driveType = GetDriveTypeW( RootDirectory );
	if ( driveType != DRIVE_FIXED )
	{
		wprintf(
			L"Insert a new floppy in drive %C:\nand press Enter when ready...",
			RootDirectory[0]
			);
		fgetws(
			input,
			WBUFSIZE(input),
			stdin
			);
		media = FMIFS_FLOPPY;
	}
	//
	// Determine the drive's file system format
	//
	if ( !GetVolumeInformationW(
		RootDirectory, 
		volumeName,
		WBUFSIZE(volumeName), 
		& serialNumber,
		& maxComponent,
		& flags, 
		fileSystem,
		WBUFSIZE(fileSystem) )
	) {
		PrintWin32Error(
			L"Could not query volume",
			GetLastError()
			);
		return -1;
	}
	if( !GetDiskFreeSpaceExW(
			RootDirectory, 
			& freeBytesAvailableToCaller,
			& totalNumberOfBytes,
			& totalNumberOfFreeBytes )
			)
	{
		PrintWin32Error(
			L"Could not query volume size",
			GetLastError()
			);
		return -1;
	}
	wprintf(
		L"The type of the file system is %s.\n",
		fileSystem
		);
	//
	// Make sure they want to do this
	//
	if ( driveType == DRIVE_FIXED )
	{
		if ( volumeName[0] )
		{
			while (1)
			{
				wprintf(
					L"Enter current volume label for drive %C: ",
					RootDirectory[0]
					);
				fgetws(
					input,
					WBUFSIZE(input),
					stdin
					);
				input[ wcslen( input ) - 1 ] = 0;
				
				if ( !wcsicmp( input, volumeName ))
				{
					break;
				}
				wprintf(L"An incorrect volume label was entered for this drive.\n");
			}
		}
		while ( 1 )
		{
			wprintf(L"\nWARNING, ALL DATA ON NON_REMOVABLE DISK\n");
			wprintf(L"DRIVE %C: WILL BE LOST!\n", RootDirectory[0] );
			wprintf(L"Proceed with Format (Y/N)? " );
			fgetws(
				input,
				WBUFSIZE(input),
				stdin
				);
			if ( (input[0] == L'Y') || (input[0] == L'y') ) break;

			if ( (input[0] == L'N') || (input[0] == L'n') )
			{	
				wprintf(L"\n");
				return 0;
			}
		}
		media = FMIFS_HARDDISK;
	} 
	//
	// Tell the user we're doing a long format if appropriate
	//
	if ( !QuickFormat )
	{	
		if ( totalNumberOfBytes.QuadPart > 1024*1024*10 )
		{
			
			wprintf(
				L"Verifying %dM\n",
				(DWORD) (totalNumberOfBytes.QuadPart/(1024*1024))
				);	
		}
		else
		{
			wprintf(
				L"Verifying %.1fM\n", 
				((float)(LONGLONG)totalNumberOfBytes.QuadPart)/(float)(1024.0*1024.0)
				);
		}
	}
	else
	{
		if ( totalNumberOfBytes.QuadPart > 1024*1024*10 )
		{	
			wprintf(
				L"QuickFormatting %dM\n",
				(DWORD) (totalNumberOfBytes.QuadPart / (1024 * 1024))
				);	
		}
		else
		{
			wprintf(
				L"QuickFormatting %.2fM\n", 
				((float)(LONGLONG)totalNumberOfBytes.QuadPart) / (float)(1024.0*1024.0)
				);
		}
		wprintf(L"Creating file system structures.\n");
	}
	//
	// Format away!
	//			
	FormatEx(
		RootDirectory,
		media,
		Format,
		Label,
		QuickFormat,
		ClusterSize,
		FormatExCallback
		);
	if ( Error ) return -1;
	wprintf(L"Format complete.\n");
	//
	// Enable compression if desired
	//
	if ( CompressDrive )
	{
		if( !EnableVolumeCompression( RootDirectory, TRUE ))
		{
			wprintf(L"Volume does not support compression.\n");
		}
	}
	//
	// Get the label if we don't have it
	//
	if( !GotALabel )
	{
		wprintf(L"Volume Label (11 characters, Enter for none)? " );
		fgetws(
			input,
			WBUFSIZE(LabelString),
			stdin
			);

		input[ wcslen(input) - 1 ] = 0;
		if( !SetVolumeLabelW( RootDirectory, input ))
		{
			PrintWin32Error(
				L"Could not label volume",
				GetLastError()
				);
			return -1;
		}	
	}
	if ( !GetVolumeInformationW(
		RootDirectory, 
		volumeName,
		WBUFSIZE(volumeName), 
		& serialNumber,
		& maxComponent,
		& flags, 
		fileSystem,
		WBUFSIZE(fileSystem) )
	) {
		PrintWin32Error(
			L"Could not query volume",
			GetLastError()
			);
		return -1;
	}
	// 
	// Print out some stuff including the formatted size
	//
	if ( !GetDiskFreeSpaceExW(
		RootDirectory, 
		& freeBytesAvailableToCaller,
		& totalNumberOfBytes,
		& totalNumberOfFreeBytes )
	) {
		PrintWin32Error( 
			L"Could not query volume size",
			GetLastError()
			);
		return -1;
	}
	wprintf(
		L"\n%I64d bytes total disk space.\n",
		totalNumberOfBytes.QuadPart
		);
	wprintf(
		L"%I64d bytes available on disk.\n",
		totalNumberOfFreeBytes.QuadPart
		);
	//
	// Get the drive's serial number
	//
	if ( !GetVolumeInformationW(
		RootDirectory, 
		volumeName,
		WBUFSIZE(volumeName), 
		& serialNumber,
		& maxComponent,
		& flags, 
		fileSystem,
		WBUFSIZE(fileSystem) )
	) {
		PrintWin32Error( 
			L"Could not query volume",
			GetLastError()
			);
		return -1;
	}
	wprintf(
		L"\nVolume Serial Number is %04X-%04X\n",
		serialNumber >> 16,
		serialNumber & 0xFFFF
		);
	return 0;
}


/* EOF */
