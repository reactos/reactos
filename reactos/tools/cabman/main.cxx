/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/main.cpp
 * PURPOSE:     Main program
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Colin Finck <mail@colinfinck.de>
 * REVISIONS:
 *   CSH 21/03-2001 Created
 *   CSH 15/08-2003 Made it portable
 *   CF  04/05-2007 Reformatted the code to be more consistent and use TABs instead of spaces
 *   CF  04/05-2007 Made it compatible with 64-bit operating systems
 *   CF  18/08-2007 Use typedefs64.h and the Windows types for compatibility with 64-bit operating systems
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "cabman.h"


#ifdef DBG

ULONG DebugTraceLevel = MIN_TRACE;
//ULONG DebugTraceLevel = MID_TRACE;
//ULONG DebugTraceLevel = MAX_TRACE;

#endif /* DBG */


#define CM_VERSION  "0.9"


char* Pad(char* Str, char PadChar, ULONG Length)
/*
 * FUNCTION: Pads a string with a character to make a given length
 * ARGUMENTS:
 *     Str     = Pointer to string to pad
 *     PadChar = Character to pad with
 *     Length  = Disired length of string
 * RETURNS:
 *     Pointer to string
 * NOTES:
 *     Str must be at least Length + 1 bytes
 */
{
	ULONG Len;

	Len = (ULONG)strlen(Str);

	if (Len < Length)
	{
		memcpy(&Str[Length - Len], Str, Len + 1);
		memset(Str, PadChar, Length - Len);
	}
	return Str;
}


char* Date2Str(char* Str, USHORT Date)
/*
 * FUNCTION: Converts a DOS style date to a string
 * ARGUMENTS:
 *     Str  = Pointer to destination string
 *     Date = DOS style date
 * RETURNS:
 *     Pointer to string
 */
{
	ULONG dw;

	/* Month */
	Str[0] = (char)('0' + ((Date & 0x01E0) >> 5) / 10);
	Str[1] = (char)('0' + ((Date & 0x01E0) >> 5) % 10);
	Str[2] = '-';
	/* Day */
	Str[3] = (char)('0' + (Date & 0x001F) / 10);
	Str[4] = (char)('0' + (Date & 0x001F) % 10);
	Str[5] = '-';
	/* Year */
	dw = 1980 + ((Date & 0xFE00) >> 9);
	Str[6] = (char)('0' + dw / 1000); dw %= 1000;
	Str[7] = (char)('0' + dw / 100);  dw %= 100;
	Str[8] = (char)('0' + dw / 10);   dw %= 10;
	Str[9] = (char)('0' + dw % 10);
	Str[10] = '\0';
	return Str;
}


char* Time2Str(char* Str, USHORT Time)
/*
 * FUNCTION: Converts a DOS style time to a string
 * ARGUMENTS:
 *     Str  = Pointer to destination string
 *     Time = DOS style time
 * RETURNS:
 *     Pointer to string
 */
{
	bool PM;
	ULONG Hour;
	ULONG dw;

	Hour = ((Time & 0xF800) >> 11);
	PM = (Hour >= 12);
	Hour %= 12;
	if (Hour == 0)
		Hour = 12;

	if (Hour >= 10)
		Str[0] = (char)('0' + Hour / 10);
	else Str[0] = ' ';
	Str[1] = (char)('0' + Hour % 10);
	Str[2] = ':';
	/* Minute */
	Str[3] = (char)('0' + ((Time & 0x07E0) >> 5) / 10);
	Str[4] = (char)('0' + ((Time & 0x07E0) >> 5) % 10);
	Str[5] = ':';
	/* Second */
	dw = 2 * (Time & 0x001F);
	Str[6] = (char)('0' + dw / 10);
	Str[7] = (char)('0' + dw % 10);

	Str[8] = PM? 'p' : 'a';
	Str[9] = '\0';
	return Str;
}


char* Attr2Str(char* Str, USHORT Attr)
/*
 * FUNCTION: Converts attributes to a string
 * ARGUMENTS:
 *     Str  = Pointer to destination string
 *     Attr = Attributes
 * RETURNS:
 *     Pointer to string
 */
{
	/* Archive */
	if (Attr & CAB_ATTRIB_ARCHIVE)
		Str[0] = 'A';
	else
		Str[0] = '-';

	/* Hidden */
	if (Attr & CAB_ATTRIB_HIDDEN)
		Str[1] = 'H';
	else
		Str[1] = '-';

	/* Read only */
	if (Attr & CAB_ATTRIB_READONLY)
		Str[2] = 'R';
	else
		Str[2] = '-';

	/* System */
	if (Attr & CAB_ATTRIB_SYSTEM)
		Str[3] = 'S';
	else
		Str[3] = '-';

	Str[4] = '\0';
	return Str;
}


/* CCABManager */

CCABManager::CCABManager()
/*
 * FUNCTION: Default constructor
 */
{
	ProcessAll = false;
	InfFileOnly = false;
	Mode = CM_MODE_DISPLAY;
}


CCABManager::~CCABManager()
/*
 * FUNCTION: Default destructor
 */
{
}


void CCABManager::Usage()
/*
 * FUNCTION: Display usage information on screen
 */
{
	printf("ReactOS Cabinet Manager - Version %s\n\n", CM_VERSION);
	printf("CABMAN [-D | -E] [-A] [-L dir] cabinet [filename ...]\n");
	printf("CABMAN -C dirfile [-I] [-RC file] [-P dir]\n");
	printf("CABMAN -S cabinet filename\n");
	printf("  cabinet   Cabinet file.\n");
	printf("  filename  Name of the file to extract from the cabinet.\n");
	printf("            Wild cards and multiple filenames\n");
	printf("            (separated by blanks) may be used.\n\n");

	printf("  dirfile   Name of the directive file to use.\n");

	printf("  -A        Process ALL cabinets. Follows cabinet chain\n");
	printf("            starting in first cabinet mentioned.\n");
	printf("  -C        Create cabinet.\n");
	printf("  -D        Display cabinet directory.\n");
	printf("  -E        Extract files from cabinet.\n");
	printf("  -I        Don't create the cabinet, only the .inf file.\n");
	printf("  -L dir    Location to place extracted or generated files\n");
	printf("            (default is current directory).\n");
	printf("  -N        Don't create the .inf file, only the cabinet.\n");
	printf("  -RC       Specify file to put in cabinet reserved area\n");
	printf("            (size must be less than 64KB).\n");
	printf("  -S        Create simple cabinet.\n");
	printf("  -P dir    Files in the .dff are relative to this directory\n");
}

bool CCABManager::ParseCmdline(int argc, char* argv[])
/*
 * FUNCTION: Parse command line arguments
 * ARGUMENTS:
 *     argc = Number of arguments on command line
 *     argv = Pointer to list of command line arguments
 * RETURNS:
 *    true if command line arguments was successfully parsed, false if not
 */
{
	int i;
	bool ShowUsage;
	bool FoundCabinet = false;

	ShowUsage = (argc < 2);

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			switch (argv[i][1])
			{
				case 'a':
				case 'A':
					ProcessAll = true;
					break;

				case 'c':
				case 'C':
					Mode = CM_MODE_CREATE;
					break;

				case 'd':
				case 'D':
					Mode = CM_MODE_DISPLAY;
					break;

				case 'e':
				case 'E':
					Mode = CM_MODE_EXTRACT;
					break;

				case 'i':
				case 'I':
					InfFileOnly = true;
					break;

				case 'l':
				case 'L':
					if (argv[i][2] == 0)
					{
						i++;
						SetDestinationPath((char*)&argv[i][0]);
					}
					else
						SetDestinationPath((char*)&argv[i][1]);

					break;

				case 'n':
				case 'N':
					DontGenerateInf = true;
					break;

				case 'R':
					switch (argv[i][2])
					{
						case 'C': /* File to put in cabinet reserved area */
							if (argv[i][3] == 0)
							{
								i++;
								if (!SetCabinetReservedFile((char*)&argv[i][0]))
								{
									printf("Cannot open cabinet reserved area file.\n");
									return false;
								}
							}
							else
							{
								if (!SetCabinetReservedFile((char*)&argv[i][3]))
								{
									printf("Cannot open cabinet reserved area file.\n");
									return false;
								}
							}
							break;

						default:
							printf("Bad parameter %s.\n", argv[i]);
							return false;
					}
					break;

				case 's':
				case 'S':
					Mode = CM_MODE_CREATE_SIMPLE;
					break;

				case 'P':
					if (argv[i][2] == 0)
					{
						i++;
						SetFileRelativePath((char*)&argv[i][0]);
					}
					else
						SetFileRelativePath((char*)&argv[i][1]);

					break;

				default:
					printf("Bad parameter %s.\n", argv[i]);
					return false;
			}
		}
		else
		{
			if ((FoundCabinet) || (Mode == CM_MODE_CREATE))
			{
				/* FIXME: There may be many of these if Mode != CM_MODE_CREATE */
				strcpy((char*)FileName, argv[i]);
			}
			else
			{
				SetCabinetName(argv[i]);
				FoundCabinet = true;
			}
		}
	}

	if (ShowUsage)
	{
	  Usage();
	  return false;
	}

	/* FIXME */
	SelectCodec(CAB_CODEC_MSZIP);

	return true;
}


bool CCABManager::CreateCabinet()
/*
 * FUNCTION: Create cabinet
 */
{
	ULONG Status;

	Status = Load((char*)&FileName);
	if (Status != CAB_STATUS_SUCCESS)
	{
		printf("Specified directive file could not be found: %s.\n", (char*)&FileName);
		return false;
	}

	Status = Parse();

	return (Status == CAB_STATUS_SUCCESS ? true : false);
}


bool CCABManager::CreateSimpleCabinet()
/*
 * FUNCTION: Create cabinet
 */
{
	ULONG Status;

	Status = NewCabinet();
	if (Status != CAB_STATUS_SUCCESS)
	{
		DPRINT(MIN_TRACE, ("Cannot create cabinet (%d).\n", (ULONG)Status));
		return false;
	}

	Status = AddFile(FileName);
	if (Status != CAB_STATUS_SUCCESS)
	{
		DPRINT(MIN_TRACE, ("Cannot add file to cabinet (%d).\n", (ULONG)Status));
		return false;
	}

	Status = WriteDisk(false);
	if (Status == CAB_STATUS_SUCCESS)
		Status = CloseDisk();
	if (Status != CAB_STATUS_SUCCESS)
	{
		DPRINT(MIN_TRACE, ("Cannot write disk (%d).\n", (ULONG)Status));
		return false;
	}

	CloseCabinet();

	return true;
}


bool CCABManager::DisplayCabinet()
/*
 * FUNCTION: Display cabinet contents
 */
{
	CAB_SEARCH Search;
	char Str[20];
	ULONG FileCount = 0;
	ULONG ByteCount = 0;

	if (Open() == CAB_STATUS_SUCCESS)
	{
		printf("Cabinet %s\n\n", GetCabinetName());

		if (FindFirst("", &Search) == CAB_STATUS_SUCCESS)
		{
			do
			{
				if (Search.File->FileControlID != CAB_FILE_CONTINUED)
				{
					printf("%s ", Date2Str((char*)&Str, Search.File->FileDate));
					printf("%s ", Time2Str((char*)&Str, Search.File->FileTime));
					printf("%s ", Attr2Str((char*)&Str, Search.File->Attributes));
					sprintf(Str, "%lu", Search.File->FileSize);
					printf("%s ", Pad(Str, ' ', 13));
					printf("%s\n", Search.FileName);

					FileCount++;
					ByteCount += Search.File->FileSize;
				}
			} while (FindNext(&Search) == CAB_STATUS_SUCCESS);
		}

		if (FileCount > 0) {
			if (FileCount == 1)
				printf("                 1 file    ");
			else
			{
				sprintf(Str, "%lu", FileCount);
				printf("      %s files   ", Pad(Str, ' ', 12));
			}

			if (ByteCount == 1)
				printf("           1 byte\n");
			else
			{
				sprintf(Str, "%lu", ByteCount);
				printf("%s bytes\n", Pad(Str, ' ', 12));
			}
		}
		else
		{
			/* There should be at least one file in a cabinet */
			printf("No files in cabinet.");
		}
		return true;
	}
	else
		printf("Cannot open file: %s\n", GetCabinetName());

	return false;
}


bool CCABManager::ExtractFromCabinet()
/*
 * FUNCTION: Extract file(s) from cabinet
 */
{
	CAB_SEARCH Search;
	ULONG Status;

	if (Open() == CAB_STATUS_SUCCESS)
	{
		printf("Cabinet %s\n\n", GetCabinetName());

		if (FindFirst("", &Search) == CAB_STATUS_SUCCESS)
		{
			do
			{
				switch (Status = ExtractFile(Search.FileName)) {
					case CAB_STATUS_SUCCESS:
						break;

					case CAB_STATUS_INVALID_CAB:
						printf("Cabinet contains errors.\n");
						return false;

					case CAB_STATUS_UNSUPPCOMP:
						printf("Cabinet uses unsupported compression type.\n");
						return false;

					case CAB_STATUS_CANNOT_WRITE:
						printf("You've run out of free space on the destination volume or the volume is damaged.\n");
						return false;

					default:
						printf("Unspecified error code (%d).\n", (ULONG)Status);
						return false;
				}
			} while (FindNext(&Search) == CAB_STATUS_SUCCESS);
		}
		return true;
	} else
		printf("Cannot open file: %s.\n", GetCabinetName());

	return false;
}


bool CCABManager::Run()
/*
 * FUNCTION: Process cabinet
 */
{
	printf("ReactOS Cabinet Manager - Version %s\n\n", CM_VERSION);

	switch (Mode)
	{
		case CM_MODE_CREATE:
			return CreateCabinet();
			break;

		case CM_MODE_DISPLAY:
			return DisplayCabinet();
			break;

		case CM_MODE_EXTRACT:
			return ExtractFromCabinet();
			break;

		case CM_MODE_CREATE_SIMPLE:
			return CreateSimpleCabinet();
			break;

		default:
			break;
	}
	return false;
}


/* Event handlers */

bool CCABManager::OnOverwrite(PCFFILE File,
                              char* FileName)
/*
 * FUNCTION: Called when extracting a file and it already exists
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     Filename = Pointer to buffer with name of file (full path)
 * RETURNS
 *     true if the file should be overwritten, false if not
 */
{
	if (Mode == CM_MODE_CREATE)
		return true;

	/* Always overwrite */
	return true;
}


void CCABManager::OnExtract(PCFFILE File,
                            char* FileName)
/*
 * FUNCTION: Called just before extracting a file
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
	printf("Extracting %s\n", GetFileName(FileName));
}



void CCABManager::OnDiskChange(char* CabinetName,
                               char* DiskLabel)
/*
 * FUNCTION: Called when a new disk is to be processed
 * ARGUMENTS:
 *     CabinetName = Pointer to buffer with name of cabinet
 *     DiskLabel   = Pointer to buffer with label of disk
 */
{
	printf("\nChanging to cabinet %s - %s\n\n", CabinetName, DiskLabel);
}


void CCABManager::OnAdd(PCFFILE File,
                        char* FileName)
/*
 * FUNCTION: Called just before adding a file to a cabinet
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being added
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
	printf("Adding %s\n", GetFileName(FileName));
}


int main(int argc, char * argv[])
/*
 * FUNCTION: Main entry point
 * ARGUMENTS:
 *     argc = Number of arguments on command line
 *     argv = Pointer to list of command line arguments
 */
{
	CCABManager CABMgr;
	bool status = false;

	if (CABMgr.ParseCmdline(argc, argv))
		status = CABMgr.Run();

	return (status ? 0 : 1);
}

/* EOF */
