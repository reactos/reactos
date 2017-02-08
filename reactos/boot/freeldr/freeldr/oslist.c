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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#define TAG_STRING  ' rtS'
#define TAG_OS_ITEM 'tISO'

/* FUNCTIONS ******************************************************************/

static PCSTR CopyString(PCSTR Source)
{
    PSTR Dest;

    if (!Source)
        return NULL;

    Dest = FrLdrHeapAlloc(strlen(Source) + 1, TAG_STRING);
    if (Dest)
        strcpy(Dest, Source);

    return Dest;
}

OperatingSystemItem* InitOperatingSystemList(ULONG* OperatingSystemCountPointer)
{
    ULONG Idx;
    CHAR SettingName[260];
    CHAR SettingValue[260];
    ULONG_PTR SectionId;
    PCHAR TitleStart, TitleEnd;
    PCSTR OsLoadOptions;
    ULONG Count;
    OperatingSystemItem* Items;

    /* Open the [FreeLoader] section */
    if (!IniOpenSection("Operating Systems", &SectionId))
        return NULL;

    /* Count number of operating systems in the section */
    Count = IniGetNumSectionItems(SectionId);

    /* Allocate memory to hold operating system lists */
    Items = FrLdrHeapAlloc(Count * sizeof(OperatingSystemItem), TAG_OS_ITEM);
    if (!Items)
        return NULL;

    /* Now loop through and read the operating system section and display names */
    for (Idx = 0; Idx < Count; Idx++)
    {
        IniReadSettingByNumber(SectionId, Idx, SettingName, sizeof(SettingName), SettingValue, sizeof(SettingValue));

        /* Search start and end of the title */
        OsLoadOptions = NULL;
        TitleStart = SettingValue;
        while (*TitleStart == ' ' || *TitleStart == '"')
            TitleStart++;
        TitleEnd = TitleStart;
        if (*TitleEnd != ANSI_NULL)
            TitleEnd++;
        while (*TitleEnd != ANSI_NULL && *TitleEnd != '"')
            TitleEnd++;
        if (*TitleEnd != ANSI_NULL)
        {
            *TitleEnd = ANSI_NULL;
            OsLoadOptions = TitleEnd + 1;
        }

        /* Copy the system partition, identifier and options */
        Items[Idx].SystemPartition = CopyString(SettingName);
        Items[Idx].LoadIdentifier = CopyString(TitleStart);
        Items[Idx].OsLoadOptions = CopyString(OsLoadOptions);
    }

    /* Return success */
    *OperatingSystemCountPointer = Count;
    return Items;
}
