/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
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
	
#include "freeldr.h"
#include "stdlib.h"
#include "tui.h"
#include "options.h"
#include "miscboot.h"


void DoOptionsMenu(void)
{
	int		OptionsMenuItemCount = 1; // Count is 1 because we don't show the "Set ReactOS Boot Flags" menu item yet
	char	OptionsMenuItems[2][80] = {	"Boot Wizard", "Set ReactOS Boot Flags" /* i.e. Safe Mode, Last Known Good Configuration */	};
	int		OptionsMenuItemSelected = 0;

	while (OptionsMenuItemSelected != -1)
	{
		OptionsMenuItemSelected = RunOptionsMenu(OptionsMenuItems, OptionsMenuItemCount, OptionsMenuItemSelected, "[Advanced Options]");

		switch (OptionsMenuItemSelected)
		{
		case 0:
			DoDiskOptionsMenu();
			break;
		}
	}
}

void DoDiskOptionsMenu(void)
{
	char	DiskMenuItems[25][80];
	int		DiskMenuItemCount = 0;
	int		FloppyDiskMenuItemCount = 0;
	int		HardDiskMenuItemCount = 0;
	int		DiskMenuItemSelected = 0;

	char	temp[255];
	int		i;

	FloppyDiskMenuItemCount = (int)*((char *)((0x40 * 16) + 0x10)); // Get number of floppy disks from bios data area 40:10
	if (FloppyDiskMenuItemCount & 1)
		FloppyDiskMenuItemCount = (FloppyDiskMenuItemCount >> 6) + 1;
	else
		FloppyDiskMenuItemCount = 0;
	HardDiskMenuItemCount = (int)*((char *)((0x40 * 16) + 0x75)); // Get number of hard disks from bios data area 40:75
	DiskMenuItemCount = FloppyDiskMenuItemCount + HardDiskMenuItemCount;

	for (i=0; i<FloppyDiskMenuItemCount; i++)
	{
		strcpy(DiskMenuItems[i], "Floppy Disk ");
		itoa(i + 1, temp, 10);
		strcat(DiskMenuItems[i], temp);
	}

	for (i=0; i<HardDiskMenuItemCount; i++)
	{
		strcpy(DiskMenuItems[i + FloppyDiskMenuItemCount], "Hard Disk ");
		itoa(i + 1, temp, 10);
		strcat(DiskMenuItems[i + FloppyDiskMenuItemCount], temp);
		strcat(DiskMenuItems[i + FloppyDiskMenuItemCount], " (");
		itoa((get_heads(i+0x80) * get_cylinders(i+0x80) * get_sectors(i+0x80)) / 2048, temp, 10);
		strcat(DiskMenuItems[i + FloppyDiskMenuItemCount], temp);
		strcat(DiskMenuItems[i + FloppyDiskMenuItemCount], " MB)");
	}

	DiskMenuItemSelected = 0;
	while (DiskMenuItemSelected != -1)
	{
		DiskMenuItemSelected = RunOptionsMenu(DiskMenuItems, DiskMenuItemCount, DiskMenuItemSelected, "[Boot Wizard]");

		if (DiskMenuItemSelected != -1)
		{
			if (DiskMenuItemSelected < FloppyDiskMenuItemCount)
				DoBootOptionsMenu(DiskMenuItemSelected, DiskMenuItems[DiskMenuItemSelected]);
			else
				DoBootOptionsMenu((DiskMenuItemSelected - FloppyDiskMenuItemCount) + 0x80, DiskMenuItems[DiskMenuItemSelected]);
		}
	}
}

void DoBootOptionsMenu(int BootDriveNum, char *BootDriveText)
{
	int		BootOptionsMenuItemCount = 2;
	char	BootOptionsMenuItems[2][80] = {	"Boot To ", "Pick A Boot Partition"	};
	int		BootOptionsMenuItemSelected = 0;

	strcat(BootOptionsMenuItems[0], BootDriveText);

	while (BootOptionsMenuItemSelected != -1)
	{
		BootOptionsMenuItemSelected = RunOptionsMenu(BootOptionsMenuItems, BootOptionsMenuItemCount, BootOptionsMenuItemSelected, "[Boot Options]");

		switch (BootOptionsMenuItemSelected)
		{
		case 0:
			BootDrive = BootDriveNum;

			if (!biosdisk(_DISK_READ, BootDrive, 0, 0, 1, 1, (void*)0x7c00))
			{
				MessageBox("Disk Read Error");
				return;
			}

			// Check for validity
			if (*((WORD*)(0x7c00 + 0x1fe)) != 0xaa55)
			{
				MessageBox("Invalid boot sector magic (0xaa55)");
				return;
			}

			RestoreScreen(pScreenBuffer);
			showcursor();
			gotoxy(nCursorXPos, nCursorYPos);

			stop_floppy();
			JumpToBootCode();

			break;
		case 1:
			if (BootDriveNum < 0x80)
			{
				MessageBox("This option is not available for a floppy disk.");
				continue;
			}
			else
				DoBootPartitionOptionsMenu(BootDriveNum);

			break;
		}
	}
}

void DoBootPartitionOptionsMenu(int BootDriveNum)
{
	struct
	{
		int		partition_num;
		int		partition_type;
		int		head, sector, cylinder;
	} BootPartitions[8];
	int		BootOptionsMenuItemCount = 0;
	char	BootOptionsMenuItems[8][80];
	int		BootOptionsMenuItemSelected = 0;
	int		head, sector, cylinder;
	int		offset;
	int		i;
	char	temp[25];


	BootDrive = BootDriveNum;

	if (!biosdisk(_DISK_READ, BootDrive, 0, 0, 1, 1, SectorBuffer))
	{
		MessageBox("Disk Read Error");
		return;
	}

	// Check for validity
	if (*((WORD*)(SectorBuffer + 0x1fe)) != 0xaa55)
	{
		MessageBox("Invalid partition table magic (0xaa55)");
		return;
	}

	offset = 0x1BE;

	for (i=0; i<4; i++)
	{
		// Check for valid partition
		if (SectorBuffer[offset + 4] != 0)
		{
			BootPartitions[BootOptionsMenuItemCount].partition_num = i;
			BootPartitions[BootOptionsMenuItemCount].partition_type = SectorBuffer[offset + 4];

			BootPartitions[BootOptionsMenuItemCount].head = SectorBuffer[offset + 1];
			BootPartitions[BootOptionsMenuItemCount].sector = (SectorBuffer[offset + 2] & 0x3F);
			BootPartitions[BootOptionsMenuItemCount].cylinder = SectorBuffer[offset + 3];
			if (SectorBuffer[offset + 2] & 0x80)
				BootPartitions[BootOptionsMenuItemCount].cylinder += 0x200;
			if (SectorBuffer[offset + 2] & 0x40)
				BootPartitions[BootOptionsMenuItemCount].cylinder += 0x100;

			strcpy(BootOptionsMenuItems[BootOptionsMenuItemCount], "Boot To Partition ");
			itoa(i+1, temp, 10);
			strcat(BootOptionsMenuItems[BootOptionsMenuItemCount], temp);
			strcat(BootOptionsMenuItems[BootOptionsMenuItemCount], " (Type: 0x");
			itoa(BootPartitions[BootOptionsMenuItemCount].partition_type, temp, 16);
			if (strlen(temp) < 2)
				strcat(BootOptionsMenuItems[BootOptionsMenuItemCount], "0");
			strcat(BootOptionsMenuItems[BootOptionsMenuItemCount], temp);
			strcat(BootOptionsMenuItems[BootOptionsMenuItemCount], ")");

			BootOptionsMenuItemCount++;
		}

		offset += 0x10;
	}

	while (BootOptionsMenuItemSelected != -1)
	{
		BootOptionsMenuItemSelected = RunOptionsMenu(BootOptionsMenuItems, BootOptionsMenuItemCount, BootOptionsMenuItemSelected, "[Boot Partition Options]");

		if (BootOptionsMenuItemSelected != -1)
		{
			head = BootPartitions[BootOptionsMenuItemCount].head;
			sector = BootPartitions[BootOptionsMenuItemCount].sector;
			cylinder = BootPartitions[BootOptionsMenuItemCount].cylinder;

			// Read partition boot sector
			if (!biosdisk(_DISK_READ, BootDrive, head, cylinder, sector, 1, (void*)0x7c00))
			{
				MessageBox("Disk Read Error");
				return;
			}

			// Check for validity
			if (*((WORD*)(0x7c00 + 0x1fe)) != 0xaa55)
			{
				MessageBox("Invalid boot sector magic (0xaa55)");
				return;
			}

			RestoreScreen(pScreenBuffer);
			showcursor();
			gotoxy(nCursorXPos, nCursorYPos);

			stop_floppy();
			JumpToBootCode();
		}
	}
}

int RunOptionsMenu(char OptionsMenuItems[][80], int OptionsMenuItemCount, int nOptionSelected, char *OptionsMenuTitle)
{
	int		key;
	int		second;
	BOOL	bDone = FALSE;
	int		nOptionsMenuBoxLeft;
	int		nOptionsMenuBoxRight;
	int		nOptionsMenuBoxTop;
	int		nOptionsMenuBoxBottom;


	// Initialise the menu
	InitOptionsMenu(&nOptionsMenuBoxLeft, &nOptionsMenuBoxTop, &nOptionsMenuBoxRight, &nOptionsMenuBoxBottom, OptionsMenuItemCount);

	DrawBackdrop();

	// Update the menu
	DrawOptionsMenu(OptionsMenuItems, OptionsMenuItemCount, nOptionSelected, OptionsMenuTitle, nOptionsMenuBoxLeft, nOptionsMenuBoxTop, nOptionsMenuBoxRight, nOptionsMenuBoxBottom);

	second = getsecond();

	// Loop
	do
	{
		// Check for a keypress
		if (kbhit())
		{
			// Cancel the timeout
			if (nTimeOut != -1)
			{
				nTimeOut = -1;
				DrawOptionsMenu(OptionsMenuItems, OptionsMenuItemCount, nOptionSelected, OptionsMenuTitle, nOptionsMenuBoxLeft, nOptionsMenuBoxTop, nOptionsMenuBoxRight, nOptionsMenuBoxBottom);
			}

			// Get the key
			key = getch();

			// Is it extended?
			if (key == 0)
				key = getch(); // Yes - so get the extended key

			// Process the key
			switch (key)
			{
			case KEY_UP:
				if (nOptionSelected)
				{
					nOptionSelected--;

					// Update the menu
					DrawOptionsMenu(OptionsMenuItems, OptionsMenuItemCount, nOptionSelected, OptionsMenuTitle, nOptionsMenuBoxLeft, nOptionsMenuBoxTop, nOptionsMenuBoxRight, nOptionsMenuBoxBottom);
				}
				break;
			case KEY_DOWN:
				if (nOptionSelected < (OptionsMenuItemCount - 1))
				{
					nOptionSelected++;

					// Update the menu
					DrawOptionsMenu(OptionsMenuItems, OptionsMenuItemCount, nOptionSelected, OptionsMenuTitle, nOptionsMenuBoxLeft, nOptionsMenuBoxTop, nOptionsMenuBoxRight, nOptionsMenuBoxBottom);
				}
				break;
			case KEY_ENTER:
				//MessageBox("The Advanced Options are still being implemented.");
				bDone = TRUE;
				break;
			case KEY_ESC:
				nOptionSelected = -1;
				bDone = TRUE;
				break;
			}
		}

		// Update the date & time
		UpdateDateTime();

		if (nTimeOut > 0)
		{
			if (getsecond() != second)
			{
				second = getsecond();
				nTimeOut--;

				// Update the menu
				DrawOptionsMenu(OptionsMenuItems, OptionsMenuItemCount, nOptionSelected, OptionsMenuTitle, nOptionsMenuBoxLeft, nOptionsMenuBoxTop, nOptionsMenuBoxRight, nOptionsMenuBoxBottom);
			}
		}

		if (nTimeOut == 0)
			bDone = TRUE;
	}
	while (!bDone);

	return nOptionSelected;
}

void InitOptionsMenu(int *nOptionsMenuBoxLeft, int *nOptionsMenuBoxTop, int *nOptionsMenuBoxRight, int *nOptionsMenuBoxBottom, int OptionsMenuItemCount)
{
	int		height = OptionsMenuItemCount;
	int		width = 20;

	height += 1; // Allow room for top & bottom borders
	width += 18; // Allow room for left & right borders, plus 8 spaces on each side

	// Calculate the OS list box area
	*nOptionsMenuBoxLeft = (nScreenWidth - width) / 2;
	*nOptionsMenuBoxRight = *nOptionsMenuBoxLeft + width;
	*nOptionsMenuBoxTop = (nScreenHeight - height) / 2 + 1;
	*nOptionsMenuBoxBottom = *nOptionsMenuBoxTop + height;
}

void DrawOptionsMenu(char OptionsMenuItems[][80], int OptionsMenuItemCount, int nOptionSelected, char *OptionsMenuTitle, int nOptionsMenuBoxLeft, int nOptionsMenuBoxTop, int nOptionsMenuBoxRight, int nOptionsMenuBoxBottom)
{
	int		i, j;
	char	text[260];
	int		space, space_left, space_right;

	// Update the status bar
	DrawStatusText(" Use \x18\x19 to select, then press ENTER. Press ESC to go back.");

	DrawBox(nOptionsMenuBoxLeft, nOptionsMenuBoxTop, nOptionsMenuBoxRight, nOptionsMenuBoxBottom, D_VERT, D_HORZ, TRUE, TRUE, ATTR(cMenuFgColor, cMenuBgColor));
	DrawText(nOptionsMenuBoxLeft + (((nOptionsMenuBoxRight - nOptionsMenuBoxLeft) - strlen(OptionsMenuTitle)) / 2) + 1, nOptionsMenuBoxTop, OptionsMenuTitle, ATTR(cMenuFgColor, cMenuBgColor));

	for(i=0; i<OptionsMenuItemCount; i++)
	{
		space = (nOptionsMenuBoxRight - nOptionsMenuBoxLeft - 2) - strlen(OptionsMenuItems[i]);
		space_left = (space / 2) + 1;
		space_right = (space - space_left) + 1;

		text[0] = '\0';
		for(j=0; j<space_left; j++)
			strcat(text, " ");
		strcat(text, OptionsMenuItems[i]);
		for(j=0; j<space_right; j++)
			strcat(text, " ");

		if(i == nOptionSelected)
		{
			DrawText(nOptionsMenuBoxLeft+1, nOptionsMenuBoxTop+1+i, text, ATTR(cSelectedTextColor, cSelectedTextBgColor));
		}
		else
		{
			DrawText(nOptionsMenuBoxLeft+1, nOptionsMenuBoxTop+1+i, text, ATTR(cTextColor, cMenuBgColor));
		}
	}
}