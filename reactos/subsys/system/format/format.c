// Copyright (c) 1998 Mark Russinovich
// Systems Internals
// http://www.sysinternals.com
#include <stdio.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <fmifs.h>

// Globals
BOOL	Error = FALSE;

// switches
BOOL	QuickFormat = FALSE;
DWORD   ClusterSize = 0;
BOOL	CompressDrive = FALSE;
BOOL    GotALabel = FALSE;
PWCHAR  Label = L"";
PWCHAR  Drive = NULL;
PWCHAR  Format = L"FAT";

WCHAR  RootDirectory[MAX_PATH];
WCHAR  LabelString[12];

//
// Size array
//
typedef struct {
	WCHAR  SizeString[16];
	DWORD  ClusterSize;
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
void PrintWin32Error( PWCHAR Message, DWORD ErrorCode )
{
	LPVOID lpMsgBuf;
 
	FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL, ErrorCode, 
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(PWCHAR) &lpMsgBuf, 0, NULL );
	printf("%S: %S\n", Message, lpMsgBuf );
	LocalFree( lpMsgBuf );
}


//----------------------------------------------------------------------
// 
// Usage
//
// Tell the user how to use the program
//
//----------------------------------------------------------------------
VOID Usage( PWCHAR ProgramName )
{
	printf("Usage: %S drive: [-FS:file-system] [-V:label] [-Q] [-A:size] [-C]\n\n", ProgramName);
	printf("  [drive:]         Specifies the drive to format.\n");
	printf("  -FS:file-system  Specifies the type of file system (e.g. FAT).\n");
	printf("  -V:label         Specifies volume label.\n");
	printf("  -Q               Performs a quick format.\n");
	printf("  -A:size          Overrides the default allocation unit size. Default settings\n");
	printf("                   are strongly recommended for general use\n"); 
	printf("                   NTFS supports 512, 1024, 2048, 4096, 8192, 16K, 32K, 64K.\n");
	printf("                   FAT supports 8192, 16K, 32K, 64K, 128K, 256K.\n");
	printf("                   NTFS compression is not supported for allocation unit sizes\n");
	printf("                   above 4096.\n");
	printf("  -C               Files created on the new volume will be compressed by\n");
	printf("                   default.\n");
	printf("\n");
}


//----------------------------------------------------------------------
//
// ParseCommandLine
//
// Get the switches.
//
//----------------------------------------------------------------------
int ParseCommandLine( int argc, WCHAR *argv[] )
{
	int i, j;
	BOOLEAN gotFormat = FALSE;
	BOOLEAN gotQuick = FALSE;
	BOOLEAN gotSize = FALSE;
	BOOLEAN gotLabel = FALSE;
	BOOLEAN gotCompressed = FALSE;


	for( i = 1; i < argc; i++ ) {

		switch( argv[i][0] ) {

		case '-':
		case '/':

			if( !wcsnicmp( &argv[i][1], L"FS:", 3 )) {

				if( gotFormat) return -1;
				Format = &argv[i][4];
				gotFormat = TRUE;


			} else if( !wcsnicmp( &argv[i][1], L"A:", 2 )) {

				if( gotSize ) return -1;
				j = 0; 
				while( LegalSizes[j].ClusterSize &&
					 wcsicmp( LegalSizes[j].SizeString, &argv[i][3] )) j++;

				if( !LegalSizes[j].ClusterSize ) return i;
				ClusterSize = LegalSizes[j].ClusterSize;
				gotSize = TRUE;

			} else if( !wcsnicmp( &argv[i][1], L"V:", 2 )) {

				if( gotLabel ) return -1;
				Label = &argv[i][3];
				gotLabel = TRUE;
				GotALabel = TRUE;

			} else if( !wcsicmp( &argv[i][1], L"Q" )) {

				if( gotQuick ) return -1;
				QuickFormat = TRUE;
				gotQuick = TRUE;

			} else if( !wcsicmp( &argv[i][1], L"C" )) {

				if( gotCompressed ) return -1;
				CompressDrive = TRUE;
				gotCompressed = TRUE;

			} else return i;
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
// FormatExCallback
//
// The file system library will call us back with commands that we
// can interpret. If we wanted to halt the chkdsk we could return FALSE.
//
//----------------------------------------------------------------------
BOOL __stdcall FormatExCallback( CALLBACKCOMMAND Command, DWORD Modifier, PVOID Argument )
{
	PDWORD percent;
	PTEXTOUTPUT output;
	PBOOLEAN status;
	static createStructures = FALSE;

	// 
	// We get other types of commands, but we don't have to pay attention to them
	//
	switch( Command ) {

	case PROGRESS:
		percent = (PDWORD) Argument;
		printf("%d percent completed.\r", *percent);
		break;

	case OUTPUT:
		output = (PTEXTOUTPUT) Argument;
		fprintf(stdout, "%s", output->Output);
		break;

	case DONE:
		status = (PBOOLEAN) Argument;
		if( *status == FALSE ) {

			printf("FormatEx was unable to complete successfully.\n\n");
			Error = TRUE;
		}
		break;
	}
	return TRUE;
}


//----------------------------------------------------------------------
//
// LoadFMIFSEntryPoints
//
// Loads FMIFS.DLL and locates the entry point(s) we are going to use
//
//----------------------------------------------------------------------
BOOLEAN LoadFMIFSEntryPoints()
{
	LoadLibrary( "fmifs.dll" );
	if( !(void*) GetProcAddress( GetModuleHandle( "fmifs.dll"), "FormatEx" ) ) {

		return FALSE;
	}

	if( !((void *) GetProcAddress( GetModuleHandle( "fmifs.dll"),
			"EnableVolumeCompression" )) ) {

		return FALSE;
	}
	return TRUE;
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
int wmain( int argc, WCHAR *argv[] )
{
	int badArg;
	DWORD media;
	DWORD driveType;
	WCHAR fileSystem[1024];
	WCHAR volumeName[1024];
	WCHAR input[1024];
	DWORD serialNumber;
	DWORD flags, maxComponent;
	ULARGE_INTEGER freeBytesAvailableToCaller, totalNumberOfBytes, totalNumberOfFreeBytes;

	//
	// Get function pointers
	//
	if( !LoadFMIFSEntryPoints()) {

		printf("Could not located FMIFS entry points.\n\n");
		return -1;
	}

	//
	// Parse command line
	//
	if( (badArg = ParseCommandLine( argc, argv ))) {

		printf("Unknown argument: %S\n", argv[badArg] );

		Usage(argv[0]);
		return -1;
	}

	// 
	// Get the drive's format
	//
	if( !Drive ) {

		printf("Required drive parameter is missing.\n\n");
		Usage( argv[0] );
		return -1;

	} else {

		wcscpy( RootDirectory, Drive );
	}
	RootDirectory[2] = L'\\';
	RootDirectory[3] = (WCHAR) 0;

	//
	// See if the drive is removable or not
	//
	driveType = GetDriveTypeW( RootDirectory );

	if( driveType == 0 ) {
		PrintWin32Error( L"Could not get drive type", GetLastError());
		return -1;
	}

	if( driveType != DRIVE_FIXED ) {

		printf("Insert a new floppy in drive %C:\nand press Enter when ready...",
			RootDirectory[0] );
		fgetws( input, sizeof(input)/2, stdin );

		media = FMIFS_FLOPPY;


driveType = DRIVE_FIXED;
media = FMIFS_HARDDISK;


	}

	//
	// Determine the drive's file system format
	//
	if( !GetVolumeInformationW( RootDirectory, 
						volumeName, sizeof(volumeName)/2, 
						&serialNumber, &maxComponent, &flags, 
						fileSystem, sizeof(fileSystem)/2)) {

		PrintWin32Error( L"Could not query volume", GetLastError());
		return -1;
	}

	if( !GetDiskFreeSpaceExW( RootDirectory, 
			&freeBytesAvailableToCaller,
			&totalNumberOfBytes,
			&totalNumberOfFreeBytes )) {

		PrintWin32Error( L"Could not query volume size", GetLastError());
		return -1;
	}
	printf("The type of the file system is %S.\n", fileSystem );

	//
	// Make sure they want to do this
	//
	if( driveType == DRIVE_FIXED ) {

		if( volumeName[0] ) {

			while(1 ) {

				printf("Enter current volume label for drive %C: ", RootDirectory[0] );
				fgetws( input, sizeof(input)/2, stdin );
				input[ wcslen( input ) - 1] = 0;
				
				if( !wcsicmp( input, volumeName )) {

					break;
				}
				printf("An incorrect volume label was entered for this drive.\n");
			}
		}

		while( 1 ) {

			printf("\nWARNING, ALL DATA ON NON_REMOVABLE DISK\n");
			printf("DRIVE %C: WILL BE LOST!\n", RootDirectory[0] );
			printf("Proceed with Format (Y/N)? " );
			fgetws( input, sizeof(input)/2, stdin );
		
			if( input[0] == L'Y' || input[0] == L'y' ) break;

			if(	input[0] == L'N' || input[0] == L'n' ) {
				
				printf("\n");
				return 0;
			}
		}
		media = FMIFS_HARDDISK;
	} 

	//
	// Tell the user we're doing a long format if appropriate
	//
	if( !QuickFormat ) {
		
		if( totalNumberOfBytes.QuadPart > 1024*1024*10 ) {
			
			printf("Verifying %dM\n", (DWORD) (totalNumberOfBytes.QuadPart/(1024*1024)));
			
		} else {

			printf("Verifying %.1fM\n", 
				((float)(LONGLONG)totalNumberOfBytes.QuadPart)/(float)(1024.0*1024.0));
		}
	} else  {

		if( totalNumberOfBytes.QuadPart > 1024*1024*10 ) {
			
			printf("QuickFormatting %dM\n", (DWORD) (totalNumberOfBytes.QuadPart/(1024*1024)));
			
		} else {

			printf("QuickFormatting %.2fM\n", 
				((float)(LONGLONG)totalNumberOfBytes.QuadPart)/(float)(1024.0*1024.0));
		}
		printf("Creating file system structures.\n");
	}

	//
	// Format away!
	//			
	FormatEx( RootDirectory, media, Format, Label, QuickFormat,
			ClusterSize, FormatExCallback );
	if( Error ) return -1;
	printf("Format complete.\n");

	//
	// Enable compression if desired
	//
	if( CompressDrive ) {

		if( !EnableVolumeCompression( RootDirectory, TRUE )) {

			printf("Volume does not support compression.\n");
		}
	}

	//
	// Get the label if we don't have it
	//
	if( !GotALabel ) {

		printf("Volume Label (11 characters, Enter for none)? " );
		fgetws( input, sizeof(LabelString)/2, stdin );

		input[ wcslen(input)-1] = 0;
		if( !SetVolumeLabelW( RootDirectory, input )) {

			PrintWin32Error(L"Could not label volume", GetLastError());
			return -1;
		}	
	}

	if( !GetVolumeInformationW( RootDirectory, 
						volumeName, sizeof(volumeName)/2, 
						&serialNumber, &maxComponent, &flags, 
						fileSystem, sizeof(fileSystem)/2)) {

		PrintWin32Error( L"Could not query volume", GetLastError());
		return -1;
	}

	// 
	// Print out some stuff including the formatted size
	//
	if( !GetDiskFreeSpaceExW( RootDirectory, 
			&freeBytesAvailableToCaller,
			&totalNumberOfBytes,
			&totalNumberOfFreeBytes )) {

		PrintWin32Error( L"Could not query volume size", GetLastError());
		return -1;
	}

	printf("\n%I64d bytes total disk space.\n", totalNumberOfBytes.QuadPart );
	printf("%I64d bytes available on disk.\n", totalNumberOfFreeBytes.QuadPart );

	//
	// Get the drive's serial number
	//
	if( !GetVolumeInformationW( RootDirectory, 
						volumeName, sizeof(volumeName)/2, 
						&serialNumber, &maxComponent, &flags, 
						fileSystem, sizeof(fileSystem)/2)) {

		PrintWin32Error( L"Could not query volume", GetLastError());
		return -1;
	}
	printf("\nVolume Serial Number is %04X-%04X\n", serialNumber >> 16,
					serialNumber & 0xFFFF );
			
	return 0;
}

int main(int argc, char* argv[])
{
	UNICODE_STRING warg;
	ANSI_STRING arg;
    NTSTATUS status;
	PWCHAR *wargv;
	int i;

	wargv = (PWCHAR *) RtlAllocateHeap(RtlGetProcessHeap(), 0, argc * sizeof(PWCHAR));

	for (i = 0; i < argc; i++)
	{
		RtlInitAnsiString(&arg, argv[i]);
		status = RtlAnsiStringToUnicodeString(&warg, &arg, TRUE);
		if (!NT_SUCCESS (status))
		{
			printf("Not enough free memory.\n");
			return 1;
		}
		wargv[i] = (PWCHAR) warg.Buffer;
	}

	wmain(argc, wargv);

	for (i = 0; i < argc; i++)
	{
		RtlFreeHeap(RtlGetProcessHeap(), 0, wargv[i]);
	}
	RtlFreeHeap(RtlGetProcessHeap(), 0, wargv);

	return 0;
}
