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

#ifndef __INI_H
#define __INI_H

#include <rtl.h>
#include <fs.h>


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
	U32					SectionItemCount;
	PINI_SECTION_ITEM	SectionItemList;

} INI_SECTION, *PINI_SECTION;

extern	PINI_SECTION		IniFileSectionListHead;
extern	U32					IniFileSectionCount;
extern	U32					IniFileSettingCount;

PFILE	IniOpenIniFile(U8 BootDriveNumber, U8 BootPartitionNumber);

BOOL	IniParseFile(PUCHAR IniFileData, U32 IniFileSize);
U32		IniGetNextLineSize(PUCHAR IniFileData, U32 IniFileSize, U32 CurrentOffset);
U32		IniGetNextLine(PUCHAR IniFileData, U32 IniFileSize, PUCHAR Buffer, U32 BufferSize, U32 CurrentOffset);
BOOL	IniIsLineEmpty(PUCHAR LineOfText, U32 TextLength);
BOOL	IniIsCommentLine(PUCHAR LineOfText, U32 TextLength);
BOOL	IniIsSectionName(PUCHAR LineOfText, U32 TextLength);
U32		IniGetSectionNameSize(PUCHAR SectionNameLine, U32 LineLength);
VOID	IniExtractSectionName(PUCHAR SectionName, PUCHAR SectionNameLine, U32 LineLength);
BOOL	IniIsSetting(PUCHAR LineOfText, U32 TextLength);
U32		IniGetSettingNameSize(PUCHAR SettingNameLine, U32 LineLength);
U32		IniGetSettingValueSize(PUCHAR SettingValueLine, U32 LineLength);
VOID	IniExtractSettingName(PUCHAR SettingName, PUCHAR SettingNameLine, U32 LineLength);
VOID	IniExtractSettingValue(PUCHAR SettingValue, PUCHAR SettingValueLine, U32 LineLength);

#endif // defined __INI_H
