/*
 *  FreeLoader - arcname.c
 *
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2001  Eric Kohl
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

BOOLEAN DissectArcPath(CHAR *ArcPath, CHAR *BootPath, UCHAR* BootDrive, ULONG* BootPartition)
{
    char *p;

    /* Detect ramdisk path */
    if (_strnicmp(ArcPath, "ramdisk(0)", 10) == 0)
    {
        /* Magic value for ramdisks */
        *BootDrive = 0x49;
        *BootPartition = 1;

        /* Get the path */
        p = ArcPath + 11;
        strcpy(BootPath, p);
        return TRUE;
    }

    if (_strnicmp(ArcPath, "multi(0)disk(0)", 15) != 0)
        return FALSE;

    p = ArcPath + 15;
    if (_strnicmp(p, "fdisk(", 6) == 0)
    {
        /*
         * Floppy disk path:
         *  multi(0)disk(0)fdisk(x)\path
         */
        p = p + 6;
        *BootDrive = atoi(p);
        p = strchr(p, ')');
        if (p == NULL)
            return FALSE;
        p++;
        *BootPartition = 0;
    }
    else if (_strnicmp(p, "cdrom(", 6) == 0)
    {
        /*
         * Cdrom path:
         *  multi(0)disk(0)cdrom(x)\path
         */
        p = p + 6;
        *BootDrive = atoi(p) + 0x80;
        p = strchr(p, ')');
        if (p == NULL)
            return FALSE;
        p++;
        *BootPartition = 0xff;
    }
    else if (_strnicmp(p, "rdisk(", 6) == 0)
    {
        /*
         * Hard disk path:
         *  multi(0)disk(0)rdisk(x)partition(y)\path
         */
        p = p + 6;
        *BootDrive = atoi(p) + 0x80;
        p = strchr(p, ')');
        if ((p == NULL) || (_strnicmp(p, ")partition(", 11) != 0))
            return FALSE;
        p = p + 11;
        *BootPartition = atoi(p);
        p = strchr(p, ')');
        if (p == NULL)
            return FALSE;
        p++;
    }
    else
    {
        return FALSE;
    }

    strcpy(BootPath, p);

    return TRUE;
}

/* PathSyntax: scsi() = 0, multi() = 1, ramdisk() = 2 */
BOOLEAN
DissectArcPath2(
    IN CHAR* ArcPath,
    OUT ULONG* x,
    OUT ULONG* y,
    OUT ULONG* z,
    OUT ULONG* Partition,
    OUT ULONG *PathSyntax)
{
    /* Detect ramdisk() */
    if (_strnicmp(ArcPath, "ramdisk(0)", 10) == 0)
    {
        *x = *y = *z = 0;
        *Partition = 1;
        *PathSyntax = 2;
        return TRUE;
    }
    /* Detect scsi()disk()rdisk()partition() */
    else if (sscanf(ArcPath, "scsi(%lu)disk(%lu)rdisk(%lu)partition(%lu)", x, y, z, Partition) == 4)
    {
        *PathSyntax = 0;
        return TRUE;
    }
    /* Detect scsi()cdrom()fdisk() */
    else if (sscanf(ArcPath, "scsi(%lu)cdrom(%lu)fdisk(%lu)", x, y, z) == 3)
    {
        *Partition = 0;
        *PathSyntax = 0;
        return TRUE;
    }
    /* Detect multi()disk()rdisk()partition() */
    else if (sscanf(ArcPath, "multi(%lu)disk(%lu)rdisk(%lu)partition(%lu)", x, y, z, Partition) == 4)
    {
        *PathSyntax = 1;
        return TRUE;
    }
    /* Detect multi()disk()cdrom() */
    else if (sscanf(ArcPath, "multi(%lu)disk(%lu)cdrom(%lu)", x, y, z) == 3)
    {
        *Partition = 1;
        *PathSyntax = 1;
        return TRUE;
    }
    /* Detect multi()disk()fdisk() */
    else if (sscanf(ArcPath, "multi(%lu)disk(%lu)fdisk(%lu)", x, y, z) == 3)
    {
        *Partition = 1;
        *PathSyntax = 1;
        return TRUE;
    }

    /* Unknown syntax */
    return FALSE;
}

VOID ConstructArcPath(PCHAR ArcPath, PCHAR SystemFolder, UCHAR Disk, ULONG Partition)
{
    char    tmp[50];

    strcpy(ArcPath, "multi(0)disk(0)");

    if (Disk < 0x80)
    {
        /*
         * Floppy disk path:
         *  multi(0)disk(0)fdisk(x)\path
         */
        sprintf(tmp, "fdisk(%d)", (int) Disk);
        strcat(ArcPath, tmp);
    }
    else
    {
        /*
         * Hard disk path:
         *  multi(0)disk(0)rdisk(x)partition(y)\path
         */
        sprintf(tmp, "rdisk(%d)partition(%d)", (int) (Disk - 0x80), (int) Partition);
        strcat(ArcPath, tmp);
    }

    if (SystemFolder[0] == '\\' || SystemFolder[0] == '/')
    {
        strcat(ArcPath, SystemFolder);
    }
    else
    {
        strcat(ArcPath, "\\");
        strcat(ArcPath, SystemFolder);
    }
}

#if 0
UCHAR ConvertArcNameToBiosDriveNumber(PCHAR ArcPath)
{
    char *    p;
    UCHAR        DriveNumber = 0;

    if (_strnicmp(ArcPath, "multi(0)disk(0)", 15) != 0)
        return 0;

    p = ArcPath + 15;
    if (_strnicmp(p, "fdisk(", 6) == 0)
    {
        /*
         * Floppy disk path:
         *  multi(0)disk(0)fdisk(x)\path
         */
        p = p + 6;
        DriveNumber = atoi(p);
    }
    else if (_strnicmp(p, "rdisk(", 6) == 0)
    {
        /*
         * Hard disk path:
         *  multi(0)disk(0)rdisk(x)partition(y)\path
         */
        p = p + 6;
        DriveNumber = atoi(p) + 0x80;
    }

    return DriveNumber;
}
#endif
