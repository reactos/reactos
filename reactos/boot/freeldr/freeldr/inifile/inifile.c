/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#include <freeldr.h>
#include <debug.h>

BOOLEAN IniOpenSection(PCSTR SectionName, ULONG* SectionId)
{
	PINI_SECTION	Section;

	DbgPrint((DPRINT_INIFILE, "IniOpenSection() SectionName = %s\n", SectionName));

	if (!IniFileSectionListHead)
		return FALSE;

	// Loop through each section and find the one they want
	Section = (PINI_SECTION)RtlListGetHead((PLIST_ITEM)IniFileSectionListHead);
	while (Section != NULL)
	{
		// Compare against the section name
		if (_stricmp(SectionName, Section->SectionName) == 0)
		{
			// We found it
			*SectionId = (ULONG)Section;
			DbgPrint((DPRINT_INIFILE, "IniOpenSection() Found it! SectionId = 0x%x\n", SectionId));
			return TRUE;
		}

		// Get the next section in the list
		Section = (PINI_SECTION)RtlListGetNext((PLIST_ITEM)Section);
	}

	DbgPrint((DPRINT_INIFILE, "IniOpenSection() Section not found.\n"));

	return FALSE;
}

ULONG IniGetNumSectionItems(ULONG SectionId)
{
	PINI_SECTION	Section = (PINI_SECTION)SectionId;

	DbgPrint((DPRINT_INIFILE, "IniGetNumSectionItems() SectionId = 0x%x\n", SectionId));
	DbgPrint((DPRINT_INIFILE, "IniGetNumSectionItems() Item count = %d\n", Section->SectionItemCount));

	return Section->SectionItemCount;
}

ULONG IniGetSectionSettingNameSize(ULONG SectionId, ULONG SettingIndex)
{
	PINI_SECTION	Section = (PINI_SECTION)SectionId;

	// Return the size of the string plus 1 for the null-terminator
	return (strlen(Section->SectionItemList[SettingIndex].ItemName) + 1);
}

ULONG IniGetSectionSettingValueSize(ULONG SectionId, ULONG SettingIndex)
{
	PINI_SECTION	Section = (PINI_SECTION)SectionId;

	// Return the size of the string plus 1 for the null-terminator
	return (strlen(Section->SectionItemList[SettingIndex].ItemValue) + 1);
}

BOOLEAN IniReadSettingByNumber(ULONG SectionId, ULONG SettingNumber, PCHAR SettingName, ULONG NameSize, PCHAR SettingValue, ULONG ValueSize)
{
	PINI_SECTION		Section = (PINI_SECTION)SectionId;
	PINI_SECTION_ITEM	SectionItem;
	ULONG					RealSettingNumber = SettingNumber;
	DbgPrint((DPRINT_INIFILE, ".001 NameSize = %d ValueSize = %d\n", NameSize, ValueSize));

	DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() SectionId = 0x%x\n", SectionId));

	// Loop through each section item and find the one they want
	DbgPrint((DPRINT_INIFILE, ".01 NameSize = %d ValueSize = %d\n", NameSize, ValueSize));
	SectionItem = (PINI_SECTION_ITEM)RtlListGetHead((PLIST_ITEM)Section->SectionItemList);
	while (SectionItem != NULL)
	{
		DbgPrint((DPRINT_INIFILE, ".1 NameSize = %d ValueSize = %d\n", NameSize, ValueSize));
		// Check to see if this is the setting they want
		if (SettingNumber == 0)
		{
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() Setting number %d found.\n", RealSettingNumber));
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() Setting name = %s\n", SectionItem->ItemName));
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() Setting value = %s\n", SectionItem->ItemValue));

			DbgPrint((DPRINT_INIFILE, "1 NameSize = %d ValueSize = %d\n", NameSize, ValueSize));
			DbgPrint((DPRINT_INIFILE, "2 NameSize = %d ValueSize = %d\n", NameSize, ValueSize));
			strncpy(SettingName, SectionItem->ItemName, NameSize - 1);
			SettingName[NameSize - 1] = '\0';
			DbgPrint((DPRINT_INIFILE, "3 NameSize = %d ValueSize = %d\n", NameSize, ValueSize));
			strncpy(SettingValue, SectionItem->ItemValue, ValueSize - 1);
			SettingValue[ValueSize - 1] = '\0';
			DbgPrint((DPRINT_INIFILE, "4 NameSize = %d ValueSize = %d\n", NameSize, ValueSize));
			DbgDumpBuffer(DPRINT_INIFILE, SettingName, NameSize);
			DbgDumpBuffer(DPRINT_INIFILE, SettingValue, ValueSize);

			return TRUE;
		}

		// Nope, keep going
		SettingNumber--;

		// Get the next section item in the list
		SectionItem = (PINI_SECTION_ITEM)RtlListGetNext((PLIST_ITEM)SectionItem);
	}

	DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() Setting number %d not found.\n", RealSettingNumber));

	return FALSE;
}

BOOLEAN IniReadSettingByName(ULONG SectionId, PCSTR SettingName, PCHAR Buffer, ULONG BufferSize)
{
	PINI_SECTION		Section = (PINI_SECTION)SectionId;
	PINI_SECTION_ITEM	SectionItem;

	DbgPrint((DPRINT_INIFILE, "IniReadSettingByName() SectionId = 0x%x\n", SectionId));

	// Loop through each section item and find the one they want
	SectionItem = (PINI_SECTION_ITEM)RtlListGetHead((PLIST_ITEM)Section->SectionItemList);
	while (SectionItem != NULL)
	{
		// Check to see if this is the setting they want
		if (_stricmp(SettingName, SectionItem->ItemName) == 0)
		{
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByName() Setting \'%s\' found.\n", SettingName));
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByName() Setting value = %s\n", SectionItem->ItemValue));

			strncpy(Buffer, SectionItem->ItemValue, BufferSize - 1);
			Buffer[BufferSize - 1] = '\0';

			return TRUE;
		}

		// Get the next section item in the list
		SectionItem = (PINI_SECTION_ITEM)RtlListGetNext((PLIST_ITEM)SectionItem);
	}

	DbgPrint((DPRINT_INIFILE, "IniReadSettingByName() Setting \'%s\' not found.\n", SettingName));

	return FALSE;
}

BOOLEAN IniAddSection(PCSTR SectionName, ULONG* SectionId)
{
	PINI_SECTION	Section;

	// Allocate a new section structure
	Section = MmAllocateMemory(sizeof(INI_SECTION));
	if (!Section)
	{
		return FALSE;
	}

	RtlZeroMemory(Section, sizeof(INI_SECTION));

	// Allocate the section name buffer
	Section->SectionName = MmAllocateMemory(strlen(SectionName));
	if (!Section->SectionName)
	{
		MmFreeMemory(Section);
		return FALSE;
	}

	// Get the section name
	strcpy(Section->SectionName, SectionName);

	// Add it to the section list head
	IniFileSectionCount++;
	if (IniFileSectionListHead == NULL)
	{
		IniFileSectionListHead = Section;
	}
	else
	{
		RtlListInsertTail((PLIST_ITEM)IniFileSectionListHead, (PLIST_ITEM)Section);
	}

	*SectionId = (ULONG)Section;

	return TRUE;
}

BOOLEAN IniAddSettingValueToSection(ULONG SectionId, PCSTR SettingName, PCSTR SettingValue)
{
	PINI_SECTION		Section = (PINI_SECTION)SectionId;
	PINI_SECTION_ITEM	SectionItem;

	// Allocate a new item structure
	SectionItem = MmAllocateMemory(sizeof(INI_SECTION_ITEM));
	if (!SectionItem)
	{
		return FALSE;
	}

	RtlZeroMemory(SectionItem, sizeof(INI_SECTION_ITEM));

	// Allocate the setting name buffer
	SectionItem->ItemName = MmAllocateMemory(strlen(SettingName) + 1);
	if (!SectionItem->ItemName)
	{
		MmFreeMemory(SectionItem);
		return FALSE;
	}

	// Allocate the setting value buffer
	SectionItem->ItemValue = MmAllocateMemory(strlen(SettingValue) + 1);
	if (!SectionItem->ItemValue)
	{
		MmFreeMemory(SectionItem->ItemName);
		MmFreeMemory(SectionItem);
		return FALSE;
	}

	strcpy(SectionItem->ItemName, SettingName);
	strcpy(SectionItem->ItemValue, SettingValue);

	// Add it to the current section
	Section->SectionItemCount++;
	if (Section->SectionItemList == NULL)
	{
		Section->SectionItemList = SectionItem;
	}
	else
	{
		RtlListInsertTail((PLIST_ITEM)Section->SectionItemList, (PLIST_ITEM)SectionItem);
	}

	return TRUE;
}
