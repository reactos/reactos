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
#include "fs.h"
#include "rosboot.h"
#include "tui.h"
#include "asmcode.h"
#include "menu.h"
#include "miscboot.h"
#include "linux.h"

// Variable BootDrive moved to asmcode.S
//unsigned int	BootDrive = 0;		// BIOS boot drive, 0-A:, 1-B:, 0x80-C:, 0x81-D:, etc.
unsigned int	BootPartition = 0;	// Boot Partition, 1-4
BOOL			bTUILoaded = FALSE; // Tells us if the user interface is loaded

char			*pFreeldrIni = (char *)(FREELDRINIADDR); // Load address for freeldr.ini
char			*pScreenBuffer = (char *)(SCREENBUFFER); // Save address for screen contents
int				nCursorXPos = 0;	// Cursor's X Position
int				nCursorYPos = 0;	// Cursor's Y Position

OSTYPE			OSList[16];
int				nNumOS = 0;

int				nTimeOut = -1;		// Time to wait for the user before booting

void BootMain(void)
{
	int		i;
	char	name[1024];
	char	value[1024];
	int		nOSToBoot;

	enable_a20();

	SaveScreen(pScreenBuffer);
	nCursorXPos = wherex();
	nCursorYPos = wherey();

	printf("Loading FreeLoader...\n");

	if (!ParseIniFile())
	{
		printf("Press any key to reboot.\n");
		getch();
		return;
	}

	clrscr();
	hidecursor();
	// Draw the backdrop and title box
	DrawBackdrop();
	bTUILoaded = TRUE;

	if (nNumOS == 0)
	{
		DrawStatusText(" Press ENTER to reboot");
		MessageBox("Error: there were no operating systems listed to boot.\nPress ENTER to reboot.");
		clrscr();
		showcursor();
		RestoreScreen(pScreenBuffer);
		return;
	}
	

	DrawStatusText(" Press ENTER to continue");
	// Find all the message box settings and run them
	for (i=1; i<=GetNumSectionItems("FREELOADER"); i++)
	{
		ReadSectionSettingByNumber("FREELOADER", i, name, value);
		if (stricmp(name, "MessageBox") == 0)
			MessageBox(value);
		if (stricmp(name, "MessageLine") == 0)
			MessageLine(value);
	}

	for (;;)
	{
		nOSToBoot = RunMenu();

		switch (OSList[nOSToBoot].nOSType)
		{
		case OSTYPE_REACTOS:
			LoadAndBootReactOS(OSList[nOSToBoot].name);
			break;
		case OSTYPE_LINUX:
			MessageBox("Cannot boot this OS type yet!");
			break;
		case OSTYPE_BOOTSECTOR:
			LoadAndBootBootSector(nOSToBoot);
			break;
		case OSTYPE_PARTITION:
			LoadAndBootPartition(nOSToBoot);
			break;
		case OSTYPE_DRIVE:
			LoadAndBootDrive(nOSToBoot);
			break;
		}
	}

	MessageBox("Press any key to reboot.");
	RestoreScreen(pScreenBuffer);
	showcursor();
	gotoxy(nCursorXPos, nCursorYPos);
}

BOOL ParseIniFile(void)
{
	int		i;
	char	name[1024];
	char	value[1024];
	FILE	Freeldr_Ini;	// File handle for freeldr.ini

	// Open the boot drive for file access
	if(!OpenDiskDrive(BootDrive, 0))
	{
		printf("Error opening boot drive for file access.\n");
		return FALSE;
	}

	// Try to open freeldr.ini or fail
	if(!OpenFile("freeldr.ini", &Freeldr_Ini))
	{
		printf("FREELDR.INI not found.\nYou need to re-install FreeLoader.\n");
		return FALSE;
	}

	// Check and see if freeldr.ini is too big
	// if so display a warning
	if(GetFileSize(&Freeldr_Ini) > 0x4000)
	{
		printf("Warning: FREELDR.INI is bigger than 16k.\n");
		printf("Only 16k of it will be loaded off the disk.\n");
		printf("Press any key to continue.\n");
		getch();
	}

	// Read freeldr.ini off the disk
	ReadFile(&Freeldr_Ini, 0x4000, pFreeldrIni);

	// Make sure the [FREELOADER] section exists
	if(!GetNumSectionItems("FREELOADER"))
	{
		printf("Section [FREELOADER] not found in FREELDR.INI.\nYou need to re-install FreeLoader.\n");
		return FALSE;
	}

	// Validate the settings in the [FREELOADER] section
	for(i=1; i<=GetNumSectionItems("FREELOADER"); i++)
	{
		ReadSectionSettingByNumber("FREELOADER", i, name, value);
		if(!IsValidSetting(name, value))
		{
			printf("Invalid setting in freeldr.ini.\nName: \"%s\", Value: \"%s\"\n", name, value);
			printf("Press any key to continue.\n");
			getch();
		}
		else
			SetSetting(name, value);
	}

	return TRUE;
}

int GetNumSectionItems(char *section)
{
	int		i;
	char	str[1024];
	char	real_section[1024];
	int		num_items = 0;
	int		freeldr_ini_offset;
	BOOL	bFoundSection = FALSE;

	// Get the real section name
	strcpy(real_section, "[");
	strcat(real_section, section);
	strcat(real_section, "]");

	// Get to the beginning of the file
	freeldr_ini_offset = 0;

	// Find the section
	while(freeldr_ini_offset < 0x4000)
	{
		// Read a line 
		for(i=0; i<1024; i++,freeldr_ini_offset++)
		{
			if((freeldr_ini_offset < 0x4000) && (pFreeldrIni[freeldr_ini_offset] != '\n'))
				str[i] = pFreeldrIni[freeldr_ini_offset];
			else
			{
				freeldr_ini_offset++;
				break;
			}
		}
		str[i] = '\0';
		//fgets(str, 1024, &Freeldr_Ini);

		// Get rid of newline & linefeed characters (if any)
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';

		// Skip comments
		if(str[0] == '#')
			continue;

		// Skip blank lines
		if(!strlen(str))
			continue;

		// If it isn't a section header then continue on
		if(str[0] != '[')
			continue;

		// Check and see if we found it
		if(stricmp(str, real_section) == 0)
		{
			bFoundSection = TRUE;
			break;
		}
	}

	// If we didn't find the section then we're outta here
	if(!bFoundSection)
		return 0;

	// Now count how many settings are in this section
	while(freeldr_ini_offset < 0x4000)
	{
		// Read a line 
		for(i=0; i<1024; i++,freeldr_ini_offset++)
		{
			if((freeldr_ini_offset < 0x4000) && (pFreeldrIni[freeldr_ini_offset] != '\n'))
				str[i] = pFreeldrIni[freeldr_ini_offset];
			else
			{
				freeldr_ini_offset++;
				break;
			}
		}
		str[i] = '\0';
		//fgets(str, 1024, &Freeldr_Ini);

		// Get rid of newline & linefeed characters (if any)
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';

		// Skip comments
		if(str[0] == '#')
			continue;

		// Skip blank lines
		if(!strlen(str))
			continue;

		// If we hit a new section then we're done
		if(str[0] == '[')
			break;

		num_items++;
	}

	return num_items;
}

BOOL ReadSectionSettingByNumber(char *section, int num, char *name, char *value)
{
	char	str[1024];
	char	real_section[1024];
	int		num_items = 0;
	int		i;
	int		freeldr_ini_offset;
	BOOL	bFoundSection = FALSE;

	// Get the real section name
	strcpy(real_section, "[");
	strcat(real_section, section);
	strcat(real_section, "]");

	// Get to the beginning of the file
	freeldr_ini_offset = 0;

	// Find the section
	while(freeldr_ini_offset < 0x4000)
	{
		// Read a line 
		for(i=0; i<1024; i++,freeldr_ini_offset++)
		{
			if((freeldr_ini_offset < 0x4000) && (pFreeldrIni[freeldr_ini_offset] != '\n'))
				str[i] = pFreeldrIni[freeldr_ini_offset];
			else
			{
				freeldr_ini_offset++;
				break;
			}
		}
		str[i] = '\0';
		//fgets(str, 1024, &Freeldr_Ini);

		// Get rid of newline & linefeed characters (if any)
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';

		// Skip comments
		if(str[0] == '#')
			continue;

		// Skip blank lines
		if(!strlen(str))
			continue;

		// If it isn't a section header then continue on
		if(str[0] != '[')
			continue;

		// Check and see if we found it
		if(stricmp(str, real_section) == 0)
		{
			bFoundSection = TRUE;
			break;
		}
	}

	// If we didn't find the section then we're outta here
	if(!bFoundSection)
		return FALSE;

	// Now find the setting we are looking for
	while(freeldr_ini_offset < 0x4000)
	{
		// Read a line 
		for(i=0; i<1024; i++,freeldr_ini_offset++)
		{
			if((freeldr_ini_offset < 0x4000) && (pFreeldrIni[freeldr_ini_offset] != '\n'))
				str[i] = pFreeldrIni[freeldr_ini_offset];
			else
			{
				freeldr_ini_offset++;
				break;
			}
		}
		str[i] = '\0';
		//fgets(str, 1024, &Freeldr_Ini);

		// Get rid of newline & linefeed characters (if any)
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';

		// Skip comments
		if(str[0] == '#')
			continue;

		// Skip blank lines
		if(!strlen(str))
			continue;

		// If we hit a new section then we're done
		if(str[0] == '[')
			break;

		// Increment setting number
		num_items++;

		// Check and see if we found the setting
		if(num_items == num)
		{
			for(i=0; i<strlen(str); i++)
			{
				// Check and see if this character is the separator
				if(str[i] == '=')
				{
					name[i] = '\0';

					strcpy(value, str+i+1);

					return TRUE;
				}
				else
					name[i] = str[i];
			}
		}
	}

	return FALSE;
}

BOOL ReadSectionSettingByName(char *section, char *valuename, char *name, char *value)
{
	char	str[1024];
	char	real_section[1024];
	char	temp[1024];
	int		i;
	int		freeldr_ini_offset;
	BOOL	bFoundSection = FALSE;

	// Get the real section name
	strcpy(real_section, "[");
	strcat(real_section, section);
	strcat(real_section, "]");

	// Get to the beginning of the file
	freeldr_ini_offset = 0;

	// Find the section
	while(freeldr_ini_offset < 0x4000)
	{
		// Read a line 
		for(i=0; i<1024; i++,freeldr_ini_offset++)
		{
			if((freeldr_ini_offset < 0x4000) && (pFreeldrIni[freeldr_ini_offset] != '\n'))
				str[i] = pFreeldrIni[freeldr_ini_offset];
			else
			{
				freeldr_ini_offset++;
				break;
			}
		}
		str[i] = '\0';
		//fgets(str, 1024, &Freeldr_Ini);

		// Get rid of newline & linefeed characters (if any)
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';

		// Skip comments
		if(str[0] == '#')
			continue;

		// Skip blank lines
		if(!strlen(str))
			continue;

		// If it isn't a section header then continue on
		if(str[0] != '[')
			continue;

		// Check and see if we found it
		if(stricmp(str, real_section) == 0)
		{
			bFoundSection = TRUE;
			break;
		}
	}

	// If we didn't find the section then we're outta here
	if(!bFoundSection)
		return FALSE;

	// Now find the setting we are looking for
	while(freeldr_ini_offset < 0x4000)
	{
		// Read a line 
		for(i=0; i<1024; i++,freeldr_ini_offset++)
		{
			if((freeldr_ini_offset < 0x4000) && (pFreeldrIni[freeldr_ini_offset] != '\n'))
				str[i] = pFreeldrIni[freeldr_ini_offset];
			else
			{
				freeldr_ini_offset++;
				break;
			}
		}
		str[i] = '\0';
		//fgets(str, 1024, &Freeldr_Ini);

		// Get rid of newline & linefeed characters (if any)
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';
		if((str[strlen(str)-1] == '\n') || (str[strlen(str)-1] == '\r'))
			str[strlen(str)-1] = '\0';

		// Skip comments
		if(str[0] == '#')
			continue;

		// Skip blank lines
		if(!strlen(str))
			continue;

		// If we hit a new section then we're done
		if(str[0] == '[')
			break;

		// Extract the setting name
		for(i=0; i<strlen(str); i++)
		{
			if(str[i] != '=')
				temp[i] = str[i];
			else
			{
				temp[i] = '\0';
				break;
			}
		}

		// Check and see if we found the setting
		if(stricmp(temp, valuename) == 0)
		{
			for(i=0; i<strlen(str); i++)
			{
				// Check and see if this character is the separator
				if(str[i] == '=')
				{
					name[i] = '\0';

					strcpy(value, str+i+1);

					return TRUE;
				}
				else
					name[i] = str[i];
			}
		}
	}

	return FALSE;
}

BOOL IsValidSetting(char *setting, char *value)
{
	if(stricmp(setting, "MessageBox") == 0)
		return TRUE;
	else if(stricmp(setting, "MessageLine") == 0)
		return TRUE;
	else if(stricmp(setting, "TitleText") == 0)
		return TRUE;
	else if(stricmp(setting, "StatusBarColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "StatusBarTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "BackdropTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "BackdropColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "BackdropFillStyle") == 0)
	{
		if(IsValidFillStyle(value))
			return TRUE;
	}
	else if(stricmp(setting, "TitleBoxTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "TitleBoxColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "MessageBoxTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "MessageBoxColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "MenuTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "MenuColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "TextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "SelectedTextColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "SelectedColor") == 0)
	{
		if(IsValidColor(value))
			return TRUE;
	}
	else if(stricmp(setting, "OS") == 0)
		return TRUE;
	else if(stricmp(setting, "TimeOut") == 0)
		return TRUE;
	/*else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;
	else if(stricmp(setting, "") == 0)
		return TRUE;*/

	return FALSE;
}

void SetSetting(char *setting, char *value)
{
	char	name[260];
	char	v[260];

	if(stricmp(setting, "TitleText") == 0)
		strcpy(szTitleBoxTitleText, value);
	else if(stricmp(setting, "StatusBarColor") == 0)
		cStatusBarBgColor = TextToColor(value);
	else if(stricmp(setting, "StatusBarTextColor") == 0)
		cStatusBarFgColor = TextToColor(value);
	else if(stricmp(setting, "BackdropTextColor") == 0)
		cBackdropFgColor = TextToColor(value);
	else if(stricmp(setting, "BackdropColor") == 0)
		cBackdropBgColor = TextToColor(value);
	else if(stricmp(setting, "BackdropFillStyle") == 0)
		cBackdropFillStyle = TextToFillStyle(value);
	else if(stricmp(setting, "TitleBoxTextColor") == 0)
		cTitleBoxFgColor = TextToColor(value);
	else if(stricmp(setting, "TitleBoxColor") == 0)
		cTitleBoxBgColor = TextToColor(value);
	else if(stricmp(setting, "MessageBoxTextColor") == 0)
		cMessageBoxFgColor = TextToColor(value);
	else if(stricmp(setting, "MessageBoxColor") == 0)
		cMessageBoxBgColor = TextToColor(value);
	else if(stricmp(setting, "MenuTextColor") == 0)
		cMenuFgColor = TextToColor(value);
	else if(stricmp(setting, "MenuColor") == 0)
		cMenuBgColor = TextToColor(value);
	else if(stricmp(setting, "TextColor") == 0)
		cTextColor = TextToColor(value);
	else if(stricmp(setting, "SelectedTextColor") == 0)
		cSelectedTextColor = TextToColor(value);
	else if(stricmp(setting, "SelectedColor") == 0)
		cSelectedTextBgColor = TextToColor(value);
	else if(stricmp(setting, "OS") == 0)
	{
		if(nNumOS >= 16)
		{
			printf("Error: you can only boot to at most 16 different operating systems.\n");
			printf("Press any key to continue\n");
			getch();
			return;
		}

		if(!GetNumSectionItems(value))
		{
			printf("Error: OS \"%s\" listed.\n", value);
			printf("It does not have it's own [section], or it is empty.\n");
			printf("Press any key to continue\n");
			getch();
			return;
		}

		strcpy(OSList[nNumOS].name, value);

		if (!ReadSectionSettingByName(value, "BootType", name, v))
		{
			printf("Unknown BootType for OS \"%s\"\n", value);
			printf("Press any key to continue\n");
			getch();
			return;
		}

		if (stricmp(v, "ReactOS") == 0)
			OSList[nNumOS].nOSType = OSTYPE_REACTOS;
		else if (stricmp(v, "Linux") == 0)
			OSList[nNumOS].nOSType = OSTYPE_LINUX;
		else if (stricmp(v, "BootSector") == 0)
			OSList[nNumOS].nOSType = OSTYPE_BOOTSECTOR;
		else if (stricmp(v, "Partition") == 0)
			OSList[nNumOS].nOSType = OSTYPE_PARTITION;
		else if (stricmp(v, "Drive") == 0)
			OSList[nNumOS].nOSType = OSTYPE_DRIVE;
		else
		{
			printf("Unknown BootType for OS \"%s\"\n", value);
			printf("Press any key to continue\n");
			getch();
			return;
		}

		nNumOS++;
	}
	else if(stricmp(setting, "TimeOut") == 0)
		nTimeOut = atoi(value);
}
