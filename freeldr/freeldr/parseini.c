/*
 *  FreeLoader
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "freeldr.h"
#include "parseini.h"
#include "ui.h"
#include "fs.h"
#include "rtl.h"
#include "mm.h"
#include "debug.h"

PUCHAR	FreeLoaderIniFileData = NULL;
ULONG	FreeLoaderIniFileSize = 0;

BOOL ParseIniFile(VOID)
{
	//int		i;
	//char	name[1024];
	//char	value[1024];
	PFILE	Freeldr_Ini;	// File handle for freeldr.ini

	// Open the boot drive for file access
	if (!OpenDiskDrive(BootDrive, 0))
	{
		printf("Error opening boot drive for file access.\n");
		return FALSE;
	}

	// Try to open freeldr.ini or fail
	Freeldr_Ini = OpenFile("freeldr.ini");
	if (Freeldr_Ini == NULL)
	{
		printf("FREELDR.INI not found.\nYou need to re-install FreeLoader.\n");
		return FALSE;
	}

	// Get the file size & allocate enough memory for it
	FreeLoaderIniFileSize = GetFileSize(Freeldr_Ini);
	FreeLoaderIniFileData = AllocateMemory(FreeLoaderIniFileSize);

	// If we are out of memory then return FALSE
	if (FreeLoaderIniFileData == NULL)
	{
		printf("Out of memory while loading FREELDR.INI.\n");
		CloseFile(Freeldr_Ini);
		return FALSE;
	}

	// Read freeldr.ini off the disk
	ReadFile(Freeldr_Ini, FreeLoaderIniFileSize, NULL, FreeLoaderIniFileData);
	CloseFile(Freeldr_Ini);

	// Make sure the [FREELOADER] section exists
	/*if (OpenSection("FREELOADER", NULL))
	{
		printf("Section [FREELOADER] not found in FREELDR.INI.\nYou need to re-install FreeLoader.\n");
		return FALSE;
	}

	// Validate the settings in the [FREELOADER] section
	for (i=1; i<=GetNumSectionItems("FREELOADER"); i++)
	{
		ReadSectionSettingByNumber("FREELOADER", i, name, value);
		if (!IsValidSetting(name, value))
		{
			printf("Invalid setting in freeldr.ini.\nName: \"%s\", Value: \"%s\"\n", name, value);
			printf("Press any key to continue.\n");
			getch();
		}
		else
			SetSetting(name, value);
	}*/

	return TRUE;
}

ULONG GetNextLineOfFileData(PUCHAR Buffer, ULONG BufferSize, ULONG CurrentOffset)
{
	ULONG	Idx;

	// Loop through grabbing chars until we hit the end of the
	// file or we encounter a new line char
	for (Idx=0; (CurrentOffset < FreeLoaderIniFileSize); CurrentOffset++)
	{
		// If we haven't exceeded our buffer size yet
		// then store another char
		if (Idx < (BufferSize - 1))
		{
			Buffer[Idx++] = FreeLoaderIniFileData[CurrentOffset];
		}

		// Check for new line char
		if (FreeLoaderIniFileData[CurrentOffset] == '\n')
		{
			CurrentOffset++;
			break;
		}
	}

	// Terminate the string
	Buffer[Idx] = '\0';

	// Get rid of newline & linefeed characters (if any)
	if((Buffer[strlen(Buffer)-1] == '\n') || (Buffer[strlen(Buffer)-1] == '\r'))
		Buffer[strlen(Buffer)-1] = '\0';
	if((Buffer[strlen(Buffer)-1] == '\n') || (Buffer[strlen(Buffer)-1] == '\r'))
		Buffer[strlen(Buffer)-1] = '\0';

	// Send back new offset
	return CurrentOffset;
}

BOOL OpenSection(PUCHAR SectionName, PULONG SectionId)
{
	UCHAR	TempString[80];
	UCHAR	RealSectionName[80];
	ULONG	FileOffset;
	BOOL	SectionFound = FALSE;

	//
	// Get the real section name
	//
	strcpy(RealSectionName, "[");
	strcat(RealSectionName, SectionName);
	strcat(RealSectionName, "]");

	//
	// Get to the beginning of the file
	//
	FileOffset = 0;

	//
	// Find the section
	//
	while (FileOffset < FreeLoaderIniFileSize)
	{
		//
		// Read a line
		//
		FileOffset = GetNextLineOfFileData(TempString, 80, FileOffset);

		//
		// If it isn't a section header then continue on
		//
		if (TempString[0] != '[')
			continue;

		//
		// Check and see if we found it
		//
		if (stricmp(TempString, RealSectionName) == 0)
		{
			SectionFound = TRUE;
			break;
		}
	}

	if (SectionId)
	{
		*SectionId = FileOffset;
	}

	return SectionFound;
}

ULONG GetNumSectionItems(ULONG SectionId)
{
	UCHAR	TempString[80];
	ULONG	SectionItemCount = 0;

	// Now count how many settings are in this section
	while (SectionId < FreeLoaderIniFileSize)
	{
		// Read a line
		SectionId = GetNextLineOfFileData(TempString, 80, SectionId);

		// If we hit a new section then we're done
		if (TempString[0] == '[')
			break;

		// Skip comments
		if (TempString[0] == '#')
			continue;

		// Skip blank lines
		if (!strlen(TempString))
			continue;

		SectionItemCount++;
	}

	return SectionItemCount;
}

BOOL ReadSectionSettingByNumber(ULONG SectionId, ULONG SettingNumber, PUCHAR SettingName, ULONG NameSize, PUCHAR SettingValue, ULONG ValueSize)
{
	UCHAR	TempString[1024];
	ULONG	SectionItemCount = 0;
	ULONG	Idx;
	ULONG	FileOffset;

	//
	// Get to the beginning of the section
	//
	FileOffset = SectionId;

	//
	// Now find the setting we are looking for
	//
	do
	{
		// Read a line
		FileOffset = GetNextLineOfFileData(TempString, 1024, FileOffset);

		// Skip comments
		if (TempString[0] == '#')
			continue;

		// Skip blank lines
		if (!strlen(TempString))
			continue;

		// If we hit a new section then we're done
		if (TempString[0] == '[')
			break;

		// Check and see if we found the setting
		if (SectionItemCount == SettingNumber)
		{
			for (Idx=0; Idx<strlen(TempString); Idx++)
			{
				// Check and see if this character is the separator
				if (TempString[Idx] == '=')
				{
					SettingName[Idx] = '\0';

					strncpy(SettingValue, TempString + Idx + 1, ValueSize);

					return TRUE;
				}
				else if (Idx < NameSize)
				{
					SettingName[Idx] = TempString[Idx];
				}
			}
		}

		// Increment setting number
		SectionItemCount++;
	}
	while (FileOffset < FreeLoaderIniFileSize);

	return FALSE;
}

BOOL ReadSectionSettingByName(ULONG SectionId, PUCHAR SettingName, PUCHAR Buffer, ULONG BufferSize)
{
	UCHAR	TempString[1024];
	UCHAR	TempBuffer[80];
	ULONG	Idx;
	ULONG	FileOffset;

	//
	// Get to the beginning of the section
	//
	FileOffset = SectionId;

	//
	// Now find the setting we are looking for
	//
	while (FileOffset < FreeLoaderIniFileSize)
	{
		// Read a line
		FileOffset = GetNextLineOfFileData(TempString, 1024, FileOffset);

		// Skip comments
		if (TempString[0] == '#')
			continue;

		// Skip blank lines
		if (!strlen(TempString))
			continue;

		// If we hit a new section then we're done
		if (TempString[0] == '[')
			break;

		// Extract the setting name
		for (Idx=0; Idx<strlen(TempString); Idx++)
		{
			if (TempString[Idx] != '=')
				TempBuffer[Idx] = TempString[Idx];
			else
			{
				TempBuffer[Idx] = '\0';
				break;
			}
		}

		// Check and see if we found the setting
		if (stricmp(TempBuffer, SettingName) == 0)
		{
			for (Idx=0; Idx<strlen(TempString); Idx++)
			{
				// Check and see if this character is the separator
				if (TempString[Idx] == '=')
				{
					strcpy(Buffer, TempString + Idx + 1);

					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

BOOL IsValidSetting(char *setting, char *value)
{
	if(stricmp(setting, "MessageBox") == 0)
		return TRUE;
	else if(stricmp(setting, "MessageLine") == 0)
		return TRUE;
	else if(stricmp(setting, "TitleText") == 0)
		return TRUE;
	else if(stricmp(setting, "StatusBarColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "StatusBarTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "BackdropTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "BackdropColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "BackdropFillStyle") == 0)
	{
		if(IsValidFillStyle(value))
			return TRUE;
	}
	else if(stricmp(setting, "TitleBoxTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "TitleBoxColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "MessageBoxTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "MessageBoxColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "MenuTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "MenuColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "TextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "SelectedTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "SelectedColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "OS") == 0)
		return TRUE;
	else if(stricmp(setting, "TimeOut") == 0)
		return TRUE;
	/*else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;*/

	return FALSE;
}

/*void SetSetting(char *setting, char *value)
{
	char	v[260];

	if(stricmp(setting, "TitleText") == 0)
		strcpy(szTitleBoxTitleText, value);
	else if(stricmp(setting, "StatusBarColor") == 0)
		cStatusBarBgColor = TextToColor(value);
	else if(stricmp(setting, "StatusBarTextColor") == 0)
		cStatusBarFgColor = TextToColor(value);
	else if(stricmp(setting, "BackdropTextColor") == 0)
		cBackdropFgColor = TextToColor(value);
	else if(stricmp(setting, "BackdropColor") == 0)
		cBackdropBgColor = TextToColor(value);
	else if(stricmp(setting, "BackdropFillStyle") == 0)
		cBackdropFillStyle = TextToFillStyle(value);
	else if(stricmp(setting, "TitleBoxTextColor") == 0)
		cTitleBoxFgColor = TextToColor(value);
	else if(stricmp(setting, "TitleBoxColor") == 0)
		cTitleBoxBgColor = TextToColor(value);
	else if(stricmp(setting, "MessageBoxTextColor") == 0)
		cMessageBoxFgColor = TextToColor(value);
	else if(stricmp(setting, "MessageBoxColor") == 0)
		cMessageBoxBgColor = TextToColor(value);
	else if(stricmp(setting, "MenuTextColor") == 0)
		cMenuFgColor = TextToColor(value);
	else if(stricmp(setting, "MenuColor") == 0)
		cMenuBgColor = TextToColor(value);
	else if(stricmp(setting, "TextColor") == 0)
		cTextColor = TextToColor(value);
	else if(stricmp(setting, "SelectedTextColor") == 0)
		cSelectedTextColor = TextToColor(value);
	else if(stricmp(setting, "SelectedColor") == 0)
		cSelectedTextBgColor = TextToColor(value);
	else if(stricmp(setting, "OS") == 0)
	{
		if(nNumOS >= 16)
		{
			printf("Error: you can only boot to at most 16 different operating systems.\n");
			printf("Press any key to continue\n");
			getch();
			return;
		}

		if(!GetNumSectionItems(value))
		{
			printf("Error: OS \"%s\" listed.\n", value);
			printf("It does not have it's own [section], or it is empty.\n");
			printf("Press any key to continue\n");
			getch();
			return;
		}

		strcpy(OSList[nNumOS].name, value);

		if (!ReadSectionSettingByName(value, "BootType", v))
		{
			printf("Unknown BootType for OS \"%s\"\n", value);
			printf("Press any key to continue\n");
			getch();
			return;
		}

		if (stricmp(v, "ReactOS") == 0)
			OSList[nNumOS].nOSType = OSTYPE_REACTOS;
		else if (stricmp(v, "Linux") == 0)
			OSList[nNumOS].nOSType = OSTYPE_LINUX;
		else if (stricmp(v, "BootSector") == 0)
			OSList[nNumOS].nOSType = OSTYPE_BOOTSECTOR;
		else if (stricmp(v, "Partition") == 0)
			OSList[nNumOS].nOSType = OSTYPE_PARTITION;
		else if (stricmp(v, "Drive") == 0)
			OSList[nNumOS].nOSType = OSTYPE_DRIVE;
		else
		{
			printf("Unknown BootType for OS \"%s\"\n", value);
			printf("Press any key to continue\n");
			getch();
			return;
		}

		nNumOS++;
	}
	else if(stricmp(setting, "TimeOut") == 0)
		nTimeOut = atoi(value);
}*/
