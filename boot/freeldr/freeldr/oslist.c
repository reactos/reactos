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

#include <debug.h>
DBG_DEFAULT_CHANNEL(INIFILE);

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

OperatingSystemItem*
InitOperatingSystemList(
    _Out_ PULONG OperatingSystemCount,
    _Out_ PULONG DefaultOperatingSystem)
{
    OperatingSystemItem* Items;
    PCSTR DefaultOSName;
    ULONG DefaultOS = 0;
    ULONG Count;
    ULONG i;
    ULONG_PTR OsSectionId, SectionId;
    PCHAR TitleStart, TitleEnd;
    PCSTR OsLoadOptions;
    BOOLEAN HadSection;
    BOOLEAN HadNoBootType;
    CHAR SettingName[260];
    CHAR SettingValue[260];
    CHAR BootType[80];
    CHAR TempBuffer[_countof(SettingValue)];

    /* Open the [Operating Systems] section */
    if (!IniOpenSection("Operating Systems", &OsSectionId))
        return NULL;

    /* Count the number of operating systems in the section */
    Count = IniGetNumSectionItems(OsSectionId);

    /* Allocate memory to hold operating system lists */
    Items = FrLdrHeapAlloc(Count * sizeof(OperatingSystemItem), TAG_OS_ITEM);
    if (!Items)
        return NULL;

    /* Retrieve the default OS */
    DefaultOSName = GetBootMgrInfo()->DefaultOs;

    /* Now loop through the operating system section and load each item */
    for (i = 0; i < Count; ++i)
    {
        IniReadSettingByNumber(OsSectionId, i,
                               SettingName, sizeof(SettingName),
                               SettingValue, sizeof(SettingValue));
        if (!*SettingName)
        {
            ERR("Invalid OS entry %lu, skipping.\n", i);
            continue;
        }

        /* Retrieve the start and end of the title */
        TitleStart = SettingValue;
        /* Trim any leading whitespace and quotes */
        while (*TitleStart == ' ' || *TitleStart == '\t' || *TitleStart == '"')
            ++TitleStart;
        TitleEnd = TitleStart;
        /* Go up to the first last quote */
        while (*TitleEnd != ANSI_NULL && *TitleEnd != '"')
            ++TitleEnd;

        /* NULL-terminate the title */
        if (*TitleEnd)
            *TitleEnd++ = ANSI_NULL; // Skip the quote too.

        /* Retrieve the options after the quoted title */
        if (*TitleEnd)
        {
            /* Trim any trailing whitespace and quotes */
            while (*TitleEnd == ' ' || *TitleEnd == '\t' || *TitleEnd == '"')
                ++TitleEnd;
        }
        OsLoadOptions = (*TitleEnd ? TitleEnd : NULL);

        // TRACE("\n"
              // "SettingName   = '%s'\n"
              // "TitleStart    = '%s'\n"
              // "OsLoadOptions = '%s'\n",
              // SettingName, TitleStart, OsLoadOptions);

        /* Find the default OS item while we haven't got one */
        if (DefaultOSName && _stricmp(DefaultOSName, SettingName) == 0)
        {
            DefaultOS = i;
            DefaultOSName = NULL; // We have found the first one, don't search for others.
        }

        /*
         * Determine whether this is a legacy operating system entry of the form:
         *
         * [Operating Systems]
         * ArcOsLoadPartition="LoadIdentifier" /List /of /Options
         *
         * and if so, convert it into a new operating system INI entry:
         *
         * [Operating Systems]
         * SectionIdentifier="LoadIdentifier"
         *
         * [SectionIdentifier]
         * BootType=...
         * SystemPath=ArcOsLoadPartition
         * Options=/List /of /Options
         *
         * The "BootType" value is heuristically determined from the form of
         * the ArcOsLoadPartition: if this is an ARC path, the "BootType" value
         * is "Windows", otherwise if this is a DOS path the "BootType" value
         * is "BootSector". This ensures backwards-compatibility with NTLDR.
         */

        /* Try to open the operating system section in the .ini file */
        SectionId = 0;
        HadSection = IniOpenSection(SettingName, &SectionId);
        if (HadSection)
        {
            /* This is a new OS entry: try to read the boot type */
            IniReadSettingByName(SectionId, "BootType", BootType, sizeof(BootType));
        }
        else
        {
            /* This is a legacy OS entry: no explicit BootType specified, we will infer one */
            *BootType = ANSI_NULL;
        }

        /* Check whether we have got a BootType value; if not, try to infer one */
        HadNoBootType = (*BootType == ANSI_NULL);
        if (HadNoBootType)
        {
#ifdef _M_IX86
            ULONG FileId;
            if (ArcOpen(SettingName, OpenReadOnly, &FileId) == ESUCCESS)
            {
                ArcClose(FileId);
                strcpy(BootType, "BootSector");
            }
            else
#endif
            {
                strcpy(BootType, "Windows");
            }
        }

        /* This is a legacy OS entry: convert it into a new OS entry */
        if (!HadSection)
        {
            TIMEINFO* TimeInfo;

            /* Save the system path from the original SettingName (overwritten below) */
            RtlStringCbCopyA(TempBuffer, sizeof(TempBuffer), SettingName);

            /* Generate a unique section name */
            TimeInfo = ArcGetTime();
            if (_stricmp(BootType, "BootSector") == 0)
            {
                RtlStringCbPrintfA(SettingName, sizeof(SettingName),
                                   "BootSectorFile%u%u%u%u%u%u",
                                   TimeInfo->Year, TimeInfo->Day, TimeInfo->Month,
                                   TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);
            }
            else if (_stricmp(BootType, "Windows") == 0)
            {
                RtlStringCbPrintfA(SettingName, sizeof(SettingName),
                                   "Windows%u%u%u%u%u%u",
                                   TimeInfo->Year, TimeInfo->Day, TimeInfo->Month,
                                   TimeInfo->Hour, TimeInfo->Minute, TimeInfo->Second);
            }
            else
            {
                ASSERT(FALSE);
            }

            /* Add the section */
            if (!IniAddSection(SettingName, &SectionId))
            {
                ERR("Could not convert legacy OS entry %lu, skipping.\n", i);
                continue;
            }

            /* Add the system path */
            if (_stricmp(BootType, "BootSector") == 0)
            {
                if (!IniAddSettingValueToSection(SectionId, "BootSectorFile", TempBuffer))
                {
                    ERR("Could not convert legacy OS entry %lu, skipping.\n", i);
                    continue;
                }
            }
            else if (_stricmp(BootType, "Windows") == 0)
            {
                if (!IniAddSettingValueToSection(SectionId, "SystemPath", TempBuffer))
                {
                    ERR("Could not convert legacy OS entry %lu, skipping.\n", i);
                    continue;
                }
            }
            else
            {
                ASSERT(FALSE);
            }

            /* Add the OS options */
            if (OsLoadOptions && !IniAddSettingValueToSection(SectionId, "Options", OsLoadOptions))
            {
                ERR("Could not convert legacy OS entry %lu, skipping.\n", i);
                continue;
            }
        }

        /* Add or modify the BootType if needed */
        if (HadNoBootType && !IniModifySettingValue(SectionId, "BootType", BootType))
        {
            ERR("Could not fixup the BootType entry for OS '%s', ignoring.\n", SettingName);
        }

        /*
         * If this is a new OS entry, but some options were given appended to
         * the OS entry item, append them instead to the "Options=" value.
         */
        if (HadSection && OsLoadOptions && *OsLoadOptions)
        {
            /* Read the original "Options=" value */
            *TempBuffer = ANSI_NULL;
            if (!IniReadSettingByName(SectionId, "Options", TempBuffer, sizeof(TempBuffer)))
                TRACE("No 'Options' value found for OS '%s', ignoring.\n", SettingName);

            /* Concatenate the options together */
            RtlStringCbCatA(TempBuffer, sizeof(TempBuffer), " ");
            RtlStringCbCatA(TempBuffer, sizeof(TempBuffer), OsLoadOptions);

            /* Save them */
            if (!IniModifySettingValue(SectionId, "Options", TempBuffer))
                ERR("Could not modify the options for OS '%s', ignoring.\n", SettingName);
        }

        /* Copy the OS section ID and its identifier */
        Items[i].SectionId = SectionId;
        Items[i].LoadIdentifier = CopyString(TitleStart);
        // TRACE("We did Items[%lu]: SectionName = '%s' (SectionId = 0x%p), LoadIdentifier = '%s'\n",
              // i, SettingName, Items[i].SectionId, Items[i].LoadIdentifier);
    }

    /* Return success */
    *OperatingSystemCount = Count;
    *DefaultOperatingSystem = DefaultOS;
    return Items;
}
