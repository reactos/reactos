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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <rtl.h>


BOOL DissectArcPath(char *ArcPath, char *BootPath, U32* BootDrive, U32* BootPartition)
{
	char *p;

	if (strnicmp(ArcPath, "multi(0)disk(0)", 15) != 0)
		return FALSE;

	p = ArcPath + 15;
	if (strnicmp(p, "fdisk(", 6) == 0)
	{
		/*
		 * floppy disk path:
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
	else if (strnicmp(p, "rdisk(", 6) == 0)
	{
		/*
		 * hard disk path:
		 *  multi(0)disk(0)rdisk(x)partition(y)\path
		 */
		p = p + 6;
		*BootDrive = atoi(p) + 0x80;
		p = strchr(p, ')');
		if ((p == NULL) || (strnicmp(p, ")partition(", 11) != 0))
			return FALSE;
		p = p + 11;
		*BootPartition = atoi(p);
		p = strchr(p, ')');
		if ((p == NULL) || (*BootPartition == 0))
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

void ConstructArcPath(PUCHAR ArcPath, PUCHAR SystemFolder, U32 Disk, U32 Partition)
{
	char	tmp[50];

	strcpy(ArcPath, "multi(0)disk(0)");

	if (Disk < 0x80)
	{
		/*
		 * floppy disk path:
		 *  multi(0)disk(0)fdisk(x)\path
		 */
		sprintf(tmp, "fdisk(%d)", (int) Disk);
		strcat(ArcPath, tmp);
	}
	else
	{
		/*
		 * hard disk path:
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

U32 ConvertArcNameToBiosDriveNumber(PUCHAR ArcPath)
{
	char *	p;
	U32		DriveNumber = 0;

	if (strnicmp(ArcPath, "multi(0)disk(0)", 15) != 0)
		return 0;

	p = ArcPath + 15;
	if (strnicmp(p, "fdisk(", 6) == 0)
	{
		/*
		 * floppy disk path:
		 *  multi(0)disk(0)fdisk(x)\path
		 */
		p = p + 6;
		DriveNumber = atoi(p);
	}
	else if (strnicmp(p, "rdisk(", 6) == 0)
	{
		/*
		 * hard disk path:
		 *  multi(0)disk(0)rdisk(x)partition(y)\path
		 */
		p = p + 6;
		DriveNumber = atoi(p) + 0x80;
	}

	return DriveNumber;
}
