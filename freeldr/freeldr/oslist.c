/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
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
#include "oslist.h"

BOOL InitOperatingSystemList(PUCHAR **SectionNamesPointer, PUCHAR **DisplayNamesPointer, PULONG OperatingSystemCountPointer)
{
	ULONG	Idx;
	ULONG	CurrentOperatingSystemIndex;
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	ULONG	OperatingSystemCount;
	ULONG	SectionId;
	ULONG	OperatingSystemSectionId;
	ULONG	SectionSettingCount;
	PUCHAR	*OperatingSystemSectionNames;
	PUCHAR	*OperatingSystemDisplayNames;

	//
	// Open the [FreeLoader] section
	//
	if (!OpenSection("FreeLoader", &SectionId))
	{
		MessageBox("Section [FreeLoader] not found in freeldr.ini.");
		return FALSE;
	}

	SectionSettingCount = GetNumSectionItems(SectionId);
	OperatingSystemCount = CountOperatingSystems(SectionId);

	//
	// Allocate memory to hold operating system lists
	//
	if (!AllocateListMemory(&OperatingSystemSectionNames, &OperatingSystemDisplayNames, OperatingSystemCount))
	{
		return FALSE;
	}

	//
	// Now loop through and read the operating system section names
	//
	CurrentOperatingSystemIndex = 0;
	for (Idx=0; Idx<SectionSettingCount; Idx++)
	{
		ReadSectionSettingByNumber(SectionId, Idx, SettingName, 80, SettingValue, 80);

		if (stricmp(SettingName, "OS") == 0)
		{
			strcpy(OperatingSystemSectionNames[CurrentOperatingSystemIndex], SettingValue);

			CurrentOperatingSystemIndex++;
		}
	}
	
	//
	// Now loop through and read the operating system display names
	//
	for (Idx=0; Idx<OperatingSystemCount; Idx++)
	{
		if (OpenSection(OperatingSystemSectionNames[Idx], &OperatingSystemSectionId))
		{
			if (ReadSectionSettingByName(OperatingSystemSectionId, "Name", SettingValue, 80))
			{
				//
				// Remove any quotes around the string
				//
				RemoveQuotes(SettingValue);
				strcpy(OperatingSystemDisplayNames[Idx], SettingValue);
			}
			else
			{
				sprintf(SettingName, "Operating System '%s' has no Name= line in it's [section].", OperatingSystemSectionNames[Idx]);
				MessageBox(SettingName);
				strcpy(OperatingSystemDisplayNames[Idx], "");
			}
		}
	}

	*OperatingSystemCountPointer = OperatingSystemCount;
	*SectionNamesPointer = OperatingSystemSectionNames;
	*DisplayNamesPointer = OperatingSystemDisplayNames;

	return TRUE;
}

ULONG CountOperatingSystems(ULONG SectionId)
{
	ULONG	Idx;
	UCHAR	SettingName[80];
	UCHAR	SettingValue[80];
	ULONG	OperatingSystemCount = 0;
	ULONG	SectionSettingCount;
	
	//
	// Loop through and count the operating systems
	//
	SectionSettingCount = GetNumSectionItems(SectionId);
	for (Idx=0; Idx<SectionSettingCount; Idx++)
	{
		ReadSectionSettingByNumber(SectionId, Idx, SettingName, 80, SettingValue, 80);

		if (stricmp(SettingName, "OS") == 0)
		{
			if (OpenSection(SettingValue, NULL))
			{
				OperatingSystemCount++;
			}
			else
			{
				sprintf(SettingName, "Operating System '%s' is listed in freeldr.ini but doesn't have a [section].", SettingValue);
				MessageBox(SettingName);
			}
		}
	}

	return OperatingSystemCount;
}

BOOL AllocateListMemory(PUCHAR **SectionNamesPointer, PUCHAR **DisplayNamesPointer, ULONG OperatingSystemCount)
{
	ULONG	Idx;
	PUCHAR	*OperatingSystemSectionNames = NULL;
	PUCHAR	*OperatingSystemDisplayNames = NULL;

	//
	// Allocate memory to hold operating system list arrays
	//
	OperatingSystemSectionNames = (PUCHAR*) AllocateMemory( sizeof(PUCHAR) * OperatingSystemCount);
	OperatingSystemDisplayNames = (PUCHAR*) AllocateMemory( sizeof(PUCHAR) * OperatingSystemCount);
	
	//
	// If either allocation failed then return FALSE
	//
	if ( (OperatingSystemSectionNames == NULL) || (OperatingSystemDisplayNames == NULL) )
	{
		if (OperatingSystemSectionNames != NULL)
		{
			FreeMemory(OperatingSystemSectionNames);
		}

		if (OperatingSystemDisplayNames != NULL)
		{
			FreeMemory(OperatingSystemDisplayNames);
		}

		return FALSE;
	}

	//
	// Clear our newly allocated memory
	//
	memset(OperatingSystemSectionNames, 0, sizeof(PUCHAR) * OperatingSystemCount);
	memset(OperatingSystemDisplayNames, 0, sizeof(PUCHAR) * OperatingSystemCount);

	//
	// Loop through each array element and allocate it's string memory
	//
	for (Idx=0; Idx<OperatingSystemCount; Idx++)
	{
		OperatingSystemSectionNames[Idx] = (PUCHAR) AllocateMemory(80);
		OperatingSystemDisplayNames[Idx] = (PUCHAR) AllocateMemory(80);

		//
		// If it failed then jump to the cleanup code
		//
		if ( (OperatingSystemSectionNames[Idx] == NULL) || (OperatingSystemDisplayNames[Idx] == NULL))
		{
			goto AllocateListMemoryFailed;
		}
	}

	*SectionNamesPointer = OperatingSystemSectionNames;
	*DisplayNamesPointer = OperatingSystemDisplayNames;

	return TRUE;

AllocateListMemoryFailed:

	//
	// Loop through each array element and free it's string memory
	//
	for (Idx=0; Idx<OperatingSystemCount; Idx++)
	{
		if (OperatingSystemSectionNames[Idx] != NULL)
		{
			FreeMemory(OperatingSystemSectionNames[Idx]);
		}

		if (OperatingSystemDisplayNames[Idx] != NULL)
		{
			FreeMemory(OperatingSystemDisplayNames[Idx]);
		}
	}

	//
	// Free operating system list arrays
	//
	FreeMemory(OperatingSystemSectionNames);
	FreeMemory(OperatingSystemDisplayNames);

	return FALSE;
}

BOOL RemoveQuotes(PUCHAR QuotedString)
{
	UCHAR	TempString[200];

	//
	// If this string is not quoted then return FALSE
	//
	if ((QuotedString[0] != '\"') && (QuotedString[strlen(QuotedString)-1] != '\"'))
	{
		return FALSE;
	}

	if (QuotedString[0] == '\"')
	{
		strcpy(TempString, (QuotedString + 1));
	}
	else
	{
		strcpy(TempString, QuotedString);
	}

	if (TempString[strlen(TempString)-1] == '\"')
	{
		TempString[strlen(TempString)-1] = '\0';
	}

	strcpy(QuotedString, TempString);

	return TRUE;
}
