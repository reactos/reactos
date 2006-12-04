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

#ifndef __OSLIST_H
#define __OSLIST_H

BOOLEAN	InitOperatingSystemList(PCSTR **SectionNamesPointer, PCSTR **DisplayNamesPointer, ULONG* OperatingSystemCountPointer);
ULONG		CountOperatingSystems(ULONG SectionId);
BOOLEAN	AllocateListMemory(PCHAR **SectionNamesPointer, PCHAR **DisplayNamesPointer, ULONG OperatingSystemCount);
BOOLEAN	RemoveQuotes(PCHAR QuotedString);

#endif // #defined __OSLIST_H
