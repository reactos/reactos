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

#ifndef __PARSEINI_H
#define __PARSEINI_H

BOOL	IniFileInitialize(VOID);

BOOL	IniOpenSection(PCHAR SectionName, ULONG* SectionId);
ULONG		IniGetNumSectionItems(ULONG SectionId);
ULONG		IniGetSectionSettingNameSize(ULONG SectionId, ULONG SettingIndex);
ULONG		IniGetSectionSettingValueSize(ULONG SectionId, ULONG SettingIndex);
BOOL	IniReadSettingByNumber(ULONG SectionId, ULONG SettingNumber, PCHAR SettingName, ULONG NameSize, PCHAR SettingValue, ULONG ValueSize);
BOOL	IniReadSettingByName(ULONG SectionId, PCHAR SettingName, PCHAR Buffer, ULONG BufferSize);
BOOL	IniAddSection(PCHAR SectionName, ULONG* SectionId);
BOOL	IniAddSettingValueToSection(ULONG SectionId, PCHAR SettingName, PCHAR SettingValue);


#endif // defined __PARSEINI_H
