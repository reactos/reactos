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

#ifndef __FREELDR_H
#define __FREELDR_H


/* just some stuff */
#define VERSION		"FreeLoader v0.8"
#define COPYRIGHT	"Copyright (C) 1999, 2000 Brian Palmer <brianp@sginet.com>"

#define ROSLDR_MAJOR_VERSION	0
#define ROSLDR_MINOR_VERSION	8
#define ROSLDR_PATCH_VERSION	0

#define	size_t	unsigned int
#define BOOL	int
#define	NULL	0
#define	TRUE	1
#define FALSE	0
#define BYTE	unsigned char
#define WORD	unsigned short
#define DWORD	unsigned long
#define CHAR	char
#define WCHAR	unsigned short
#define LONG	long
#define ULONG	unsigned long
#define PULONG	unsigned long *
#define PDWORD	DWORD *
#define PWORD	WORD *

#define	OSTYPE_REACTOS		1
#define OSTYPE_LINUX		2
#define OSTYPE_BOOTSECTOR	3
#define OSTYPE_PARTITION	4
#define OSTYPE_DRIVE		5

typedef struct
{
	char	name[260];
	int		nOSType;			// ReactOS or Linux or a bootsector, etc.
} OSTYPE;

extern unsigned int	BootDrive; // BIOS boot drive, 0-A:, 1-B:, 0x80-C:, 0x81-D:, etc.
extern unsigned int	BootPartition;	// Boot Partition, 1-4
extern BOOL			bTUILoaded; // Tells us if the user interface is loaded

extern char			*pFreeldrIni; // Load address for freeldr.ini
extern char			*pScreenBuffer; // Save address for screen contents
extern int			nCursorXPos;	// Cursor's X Position
extern int			nCursorYPos;	// Cursor's Y Position

extern OSTYPE		OSList[16]; // The OS list
extern int			nNumOS;		// Number of OSes listed

extern int			nTimeOut;		// Time to wait for the user before booting

void	BootMain(void);
BOOL	ParseIniFile(void);
int		GetNumSectionItems(char *section); // returns the number of items in a particular section (i.e. [FREELOADER])
BOOL	ReadSectionSettingByNumber(char *section, int num, char *name, char *value); // Reads the num'th value from section
BOOL	ReadSectionSettingByName(char *section, char *valuename, char *name, char *value); // Reads the value named name from section
BOOL	IsValidSetting(char *setting, char *value);
void	SetSetting(char *setting, char *value);

#endif  // defined __FREELDR_H