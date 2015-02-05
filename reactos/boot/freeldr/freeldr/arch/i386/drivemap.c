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

#include <freeldr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(WARNING);

BOOLEAN      DriveMapInstalled = FALSE;    // Tells us if we have already installed our drive map int 13h handler code
ULONG        OldInt13HandlerAddress = 0;   // Address of BIOS int 13h handler
ULONG        DriveMapHandlerAddress = 0;   // Linear address of our drive map handler
ULONG        DriveMapHandlerSegOff = 0;    // Segment:offset style address of our drive map handler

#ifndef _MSC_VER
VOID DriveMapMapDrivesInSection(PCSTR SectionName)
{
    CHAR           SettingName[80];
    CHAR           SettingValue[80];
    CHAR           Drive1[80];
    CHAR           Drive2[80];
    ULONG          SectionId;
    ULONG          SectionItemCount;
    ULONG          Index;
    ULONG          Index2;
    DRIVE_MAP_LIST DriveMapList;

    RtlZeroMemory(&DriveMapList, sizeof(DRIVE_MAP_LIST));

    if (!IniOpenSection(SectionName, &SectionId))
    {
        return;
    }

    // Get the number of items in this section
    SectionItemCount = IniGetNumSectionItems(SectionId);

    // Loop through each one and check if its a DriveMap= setting
    for (Index=0; Index<SectionItemCount; Index++)
    {
        // Get the next setting from the .ini file section
        if (IniReadSettingByNumber(SectionId, Index, SettingName, sizeof(SettingName), SettingValue, sizeof(SettingValue)))
        {
            if (_stricmp(SettingName, "DriveMap") == 0)
            {
                // Make sure we haven't exceeded the drive map max count
                if (DriveMapList.DriveMapCount >= 4)
                {
                    UiMessageBox("Max DriveMap count exceeded in section [%s]:\n\n%s=%s", SectionName, SettingName, SettingValue);
                    continue;
                }

                RtlZeroMemory(Drive1, 80);
                RtlZeroMemory(Drive2, 80);

                strcpy(Drive1, SettingValue);

                // Parse the setting value and separate a string "hd0,hd1"
                // into two strings "hd0" and "hd1"
                for (Index2=0; Index2<strlen(Drive1); Index2++)
                {
                    // Check if this character is the separater character (comma - ',')
                    if (Drive1[Index2] == ',')
                    {
                        Drive1[Index2] = '\0';
                        strcpy(Drive2, &Drive1[Index2+1]);
                        break;
                    }
                }

                // Make sure we got good values before we add them to the map
                if (!DriveMapIsValidDriveString(Drive1) || !DriveMapIsValidDriveString(Drive2))
                {
                    UiMessageBox("Error in DriveMap setting in section [%s]:\n\n%s=%s", SectionName, SettingName, SettingValue);
                    continue;
                }

                // Add them to the map
                DriveMapList.DriveMap[(DriveMapList.DriveMapCount * 2)] = DriveMapGetBiosDriveNumber(Drive1);
                DriveMapList.DriveMap[(DriveMapList.DriveMapCount * 2)+1] = DriveMapGetBiosDriveNumber(Drive2);
                DriveMapList.DriveMapCount++;

                TRACE("Mapping BIOS drive 0x%x to drive 0x%x\n", DriveMapGetBiosDriveNumber(Drive1), DriveMapGetBiosDriveNumber(Drive2));
            }
        }
    }

    if (DriveMapList.DriveMapCount)
    {
        TRACE("Installing Int13 drive map for %d drives.\n", DriveMapList.DriveMapCount);
        DriveMapInstallInt13Handler(&DriveMapList);
    }
    else
    {
        TRACE("Removing any previously installed Int13 drive map.\n");
        DriveMapRemoveInt13Handler();
    }
}

BOOLEAN DriveMapIsValidDriveString(PCSTR DriveString)
{
    ULONG        Index;

    // Now verify that the user has given us appropriate strings
    if ((strlen(DriveString) < 3) ||
        ((DriveString[0] != 'f') && (DriveString[0] != 'F') && (DriveString[0] != 'h') && (DriveString[0] != 'H')) ||
        ((DriveString[1] != 'd') && (DriveString[1] != 'D')))
    {
        return FALSE;
    }

    // Now verify that the user has given us appropriate numbers
    // Make sure that only numeric characters were given
    for (Index=2; Index<strlen(DriveString); Index++)
    {
        if (DriveString[Index] < '0' || DriveString[Index] > '9')
        {
            return FALSE;
        }
    }
    // Now make sure that they are not outrageous values (i.e. hd90874)
    if ((atoi(&DriveString[2]) < 0) || (atoi(&DriveString[2]) > 0xff))
    {
        return FALSE;
    }

    return TRUE;
}
#endif

UCHAR DriveMapGetBiosDriveNumber(PCSTR DeviceName)
{
    UCHAR        BiosDriveNumber = 0;

    // If they passed in a number string then just
    // convert it to decimal and return it
    if (DeviceName[0] >= '0' && DeviceName[0] <= '9')
    {
        return atoi(DeviceName);
    }

    // Convert the drive number string into a number
    // 'hd1' = 1
    BiosDriveNumber = atoi(&DeviceName[2]);

    // If it's a hard disk then set the high bit
    if ((DeviceName[0] == 'h' || DeviceName[0] == 'H') &&
        (DeviceName[1] == 'd' || DeviceName[1] == 'D'))
    {
        BiosDriveNumber |= 0x80;
    }

    return BiosDriveNumber;
}

#ifndef _MSC_VER
VOID DriveMapInstallInt13Handler(PDRIVE_MAP_LIST DriveMap)
{
    ULONG*    RealModeIVT = (ULONG*)0x00000000;
    USHORT*    BiosLowMemorySize = (USHORT*)0x00000413;

    if (!DriveMapInstalled)
    {
        // Get the old INT 13h handler address from the vector table
        OldInt13HandlerAddress = RealModeIVT[0x13];

        // Decrease the size of low memory
        (*BiosLowMemorySize)--;

        // Get linear address for drive map handler
        DriveMapHandlerAddress = (ULONG)(*BiosLowMemorySize) << 10;

        // Convert to segment:offset style address
        DriveMapHandlerSegOff = (DriveMapHandlerAddress << 12) & 0xffff0000;
    }

    // Copy the drive map structure to the proper place
    RtlCopyMemory(&DriveMapInt13HandlerMapList, DriveMap, sizeof(DRIVE_MAP_LIST));

    // Set the address of the BIOS INT 13h handler
    DriveMapOldInt13HandlerAddress = OldInt13HandlerAddress;

    // Copy the code to our reserved area
    RtlCopyMemory((PVOID)DriveMapHandlerAddress, &DriveMapInt13HandlerStart, ((ULONG)&DriveMapInt13HandlerEnd - (ULONG)&DriveMapInt13HandlerStart));

    // Update the IVT
    RealModeIVT[0x13] = DriveMapHandlerSegOff;

    CacheInvalidateCacheData();
    DriveMapInstalled = TRUE;
}

VOID DriveMapRemoveInt13Handler(VOID)
{
    ULONG*    RealModeIVT = (ULONG*)0x00000000;
    USHORT*    BiosLowMemorySize = (USHORT*)0x00000413;

    if (DriveMapInstalled)
    {
        // Get the old INT 13h handler address from the vector table
        RealModeIVT[0x13] = OldInt13HandlerAddress;

        // Increase the size of low memory
        (*BiosLowMemorySize)++;

        CacheInvalidateCacheData();
        DriveMapInstalled = FALSE;
    }
}
#endif
