/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
#include "ini.h"
#include <ui.h>
#include <rtl.h>
#include <debug.h>

BOOL IniOpenSection(PUCHAR SectionName, U32* SectionId)
{
	PINI_SECTION	Section;

	DbgPrint((DPRINT_INIFILE, "IniOpenSection() SectionName = %s\n", SectionName));

	// Loop through each section and find the one they want
	Section = (PINI_SECTION)RtlListGetHead((PLIST_ITEM)IniFileSectionListHead);
	while (Section != NULL)
	{
		// Compare against the section name
		if (stricmp(SectionName, Section->SectionName) == 0)
		{
			// We found it
			*SectionId = (U32)Section;
			DbgPrint((DPRINT_INIFILE, "IniOpenSection() Found it! SectionId = 0x%x\n", SectionId));
			return TRUE;
		}

		// Get the next section in the list
		Section = (PINI_SECTION)RtlListGetNext((PLIST_ITEM)Section);
	}

	DbgPrint((DPRINT_INIFILE, "IniOpenSection() Section not found.\n"));

	return FALSE;
}

U32 IniGetNumSectionItems(U32 SectionId)
{
	PINI_SECTION	Section = (PINI_SECTION)SectionId;

	DbgPrint((DPRINT_INIFILE, "IniGetNumSectionItems() SectionId = 0x%x\n", SectionId));
	DbgPrint((DPRINT_INIFILE, "IniGetNumSectionItems() Item count = %d\n", Section->SectionItemCount));

	return Section->SectionItemCount;
}

BOOL IniReadSettingByNumber(U32 SectionId, U32 SettingNumber, PUCHAR SettingName, U32 NameSize, PUCHAR SettingValue, U32 ValueSize)
{
	PINI_SECTION		Section = (PINI_SECTION)SectionId;
	PINI_SECTION_ITEM	SectionItem;
#ifdef DEBUG
	U32					RealSettingNumber = SettingNumber;
#endif

	DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() SectionId = 0x%x\n", SectionId));

	// Loop through each section item and find the one they want
	SectionItem = (PINI_SECTION_ITEM)RtlListGetHead((PLIST_ITEM)Section->SectionItemList);
	while (SectionItem != NULL)
	{
		// Check to see if this is the setting they want
		if (SettingNumber == 0)
		{
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() Setting number %d found.\n", RealSettingNumber));
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() Setting name = %s\n", SectionItem->ItemName));
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByNumber() Setting value = %s\n", SectionItem->ItemValue));

			strncpy(SettingName, SectionItem->ItemName, NameSize);
			strncpy(SettingValue, SectionItem->ItemValue, ValueSize);

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

BOOL IniReadSettingByName(U32 SectionId, PUCHAR SettingName, PUCHAR Buffer, U32 BufferSize)
{
	PINI_SECTION		Section = (PINI_SECTION)SectionId;
	PINI_SECTION_ITEM	SectionItem;

	DbgPrint((DPRINT_INIFILE, "IniReadSettingByName() SectionId = 0x%x\n", SectionId));

	// Loop through each section item and find the one they want
	SectionItem = (PINI_SECTION_ITEM)RtlListGetHead((PLIST_ITEM)Section->SectionItemList);
	while (SectionItem != NULL)
	{
		// Check to see if this is the setting they want
		if (stricmp(SettingName, SectionItem->ItemName) == 0)
		{
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByName() Setting \'%s\' found.\n", SettingName));
			DbgPrint((DPRINT_INIFILE, "IniReadSettingByName() Setting value = %s\n", SectionItem->ItemValue));

			strncpy(Buffer, SectionItem->ItemValue, BufferSize);

			return TRUE;
		}

		// Get the next section item in the list
		SectionItem = (PINI_SECTION_ITEM)RtlListGetNext((PLIST_ITEM)SectionItem);
	}

	DbgPrint((DPRINT_INIFILE, "IniReadSettingByName() Setting \'%s\' not found.\n", SettingName));

	return FALSE;
}
