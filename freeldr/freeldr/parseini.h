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

#ifndef __PARSEINI_H
#define __PARSEINI_H

/*BOOL	ParseIniFile(void);
ULONG	GetNextLineOfFileData(PUCHAR Buffer, ULONG BufferSize, ULONG CurrentOffset); // Gets the next line of text (up to BufferSize) after CurrentOffset and returns the offset of the next line
ULONG	GetOffsetOfFirstLineOfSection(PUCHAR SectionName); // Returns the offset of the first line in the section or zero if the section wasn't found
ULONG	GetNumSectionItems(PUCHAR SectionName); // returns the number of items in a particular section (i.e. [FREELOADER])
BOOL	ReadSectionSettingByNumber(PUCHAR SectionName, ULONG SettingNumber, PUCHAR SettingName, PUCHAR SettingValue); // Reads the num'th value from section
BOOL	ReadSectionSettingByName(PUCHAR SectionName, PUCHAR SettingName, PUCHAR SettingValue); // Reads the value named name from section
BOOL	IsValidSetting(char *setting, char *value);
void	SetSetting(char *setting, char *value);*/

BOOL	ParseIniFile(VOID);
ULONG	GetNextLineOfFileData(PUCHAR Buffer, ULONG BufferSize, ULONG CurrentOffset);
BOOL	OpenSection(PUCHAR SectionName, PULONG SectionId);
ULONG	GetNumSectionItems(ULONG SectionId);
BOOL	ReadSectionSettingByNumber(ULONG SectionId, ULONG SettingNumber, PUCHAR SettingName, ULONG NameSize, PUCHAR SettingValue, ULONG ValueSize);
BOOL	ReadSectionSettingByName(ULONG SectionId, PUCHAR SettingName, PUCHAR Buffer, ULONG BufferSize);
BOOL	IsValidSetting(char *setting, char *value);
void	SetSetting(char *setting, char *value);


#endif // defined __PARSEINI_H
