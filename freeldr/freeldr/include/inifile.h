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

BOOL	IniFileInitialize(VOID);

BOOL	IniOpenSection(PUCHAR SectionName, U32* SectionId);
U32		IniGetNumSectionItems(U32 SectionId);
BOOL	IniReadSettingByNumber(U32 SectionId, U32 SettingNumber, PUCHAR SettingName, U32 NameSize, PUCHAR SettingValue, U32 ValueSize);
BOOL	IniReadSettingByName(U32 SectionId, PUCHAR SettingName, PUCHAR Buffer, U32 BufferSize);


#endif // defined __PARSEINI_H
