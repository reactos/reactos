/*
 *  FreeLoader - install.c
 *
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
 *
 *  Ported to Linux by Arnav Bhatt <arnavbhatt2004@gmail.com>
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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h> 
#include "install.h"
#include "volume.h"

/*
 * These includes are required to define
 * the fat_data and fat32_data arrays.
 */
#include "fat.h"
#include "fat32.h"

bool BackupBootSector(char* lpszVolumeName);
bool InstallBootSector(char* lpszVolumeType);
int stricmp(const char *a, const char *b);

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("syntax: install x: [fs_type]\nwhere fs_type is fat or fat32\n");
        return -1;
    }

    if (!OpenVolume(argv[1]))
    {
        printf("Exiting program...\n");
        return -1;
    }
    
	if (!InstallBootSector(argv[2]))
    {
        printf("Exiting program...\n");
        return -1;
    }
    
    CloseVolume();

    printf("You must now copy freeldr.sys & freeldr.ini to %s.\n", argv[1]);

    return 0;
}


bool InstallBootSector(char* lpszVolumeType)
{
    unsigned char BootSectorBuffer[512];

    if (!ReadVolumeSector(0, BootSectorBuffer))
    {
        return false;
    }

    if (stricmp(lpszVolumeType, "fat") == 0)
    {
        //
        // Update the BPB in the new boot sector
        //
        memcpy((fat_data+3), (BootSectorBuffer+3), 59 /*fat BPB length*/);

        //
        // Write out new boot sector
        //
        if (!WriteVolumeSector(0, fat_data))
        {
            return false;
        }
    }
    else if (stricmp(lpszVolumeType, "fat32") == 0)
    {
        //
        // Update the BPB in the new boot sector
        //
        memcpy((fat32_data+3), (BootSectorBuffer+3), 87 /*fat32 BPB length*/);

        //
        // Write out new boot sector
        //
        if (!WriteVolumeSector(0, fat32_data))
        {
            return false;
        }

        //
        // Write out new extra sector
        //
        if (!WriteVolumeSector(14, (fat32_data+512)))
        {
            return false;
        }
    }
    else
    {
        printf("%s:%d: ", __FILE__, __LINE__);
        printf("File system type %s unknown.\n", lpszVolumeType);
        return false;
    }

    printf("%s boot sector installed.\n", lpszVolumeType);

    return true;
}

int stricmp(const char *a, const char *b)
{
  int ca, cb;
  do
  {
     ca = (unsigned char) *a++;
     cb = (unsigned char) *b++;
     ca = tolower(toupper(ca));
     cb = tolower(toupper(cb));
   }
   while (ca == cb && ca != '\0');
   
   return ca - cb;
}
