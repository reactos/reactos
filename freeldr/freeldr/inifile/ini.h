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

#ifndef __INI_H
#define __INI_H

#include <rtl.h>


#define INI_FILE_COMMENT_CHAR	';'



// This structure describes a single .ini file item
// The item format in the .ini file is:
// Name=Value
typedef struct
{
	LIST_ITEM	ListEntry;
	PUCHAR		ItemName;
	PUCHAR		ItemValue;

} INI_SECTION_ITEM, *PINI_SECTION_ITEM;

// This structure describes a .ini file section
// The section format in the .ini file is:
// [Section Name]
// This structure has a list of section items with
// one INI_SECTION_ITEM for each line in the section
typedef struct
{
	LIST_ITEM			ListEntry;
	PUCHAR				SectionName;
	ULONG				SectionItemCount;
	PINI_SECTION_ITEM	SectionItemList;

} INI_SECTION, *PINI_SECTION;

extern	PINI_SECTION		IniFileSectionListHead;
extern	ULONG				IniFileSectionListCount;

BOOL	IniParseFile(PUCHAR IniFileData, ULONG IniFileSize);
ULONG	IniGetNextLineSize(PUCHAR IniFileData, ULONG IniFileSize, ULONG CurrentOffset);
ULONG	IniGetNextLine(PUCHAR IniFileData, ULONG IniFileSize, PUCHAR Buffer, ULONG BufferSize, ULONG CurrentOffset);
BOOL	IniIsLineEmpty(PUCHAR LineOfText, ULONG TextLength);
BOOL	IniIsCommentLine(PUCHAR LineOfText, ULONG TextLength);
BOOL	IniIsSectionName(PUCHAR LineOfText, ULONG TextLength);
ULONG	IniGetSectionNameSize(PUCHAR SectionNameLine, ULONG LineLength);
VOID	IniExtractSectionName(PUCHAR SectionName, PUCHAR SectionNameLine, ULONG LineLength);
BOOL	IniIsSetting(PUCHAR LineOfText, ULONG TextLength);
ULONG	IniGetSettingNameSize(PUCHAR SettingNameLine, ULONG LineLength);
ULONG	IniGetSettingValueSize(PUCHAR SettingValueLine, ULONG LineLength);
VOID	IniExtractSettingName(PUCHAR SettingName, PUCHAR SettingNameLine, ULONG LineLength);
VOID	IniExtractSettingValue(PUCHAR SettingValue, PUCHAR SettingValueLine, ULONG LineLength);

#endif // defined __INI_H
