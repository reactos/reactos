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

#ifndef __STDLIB_H
#define __STDLIB_H

#include "fs.h"

void	putchar(int ch);		// Implemented in asmcode.S
void	clrscr(void);			// Implemented in asmcode.S
int		kbhit(void);			// Implemented in asmcode.S
int		getch(void);			// Implemented in asmcode.S
void	gotoxy(int x, int y);	// Implemented in asmcode.S
int		getyear(void);			// Implemented in asmcode.S
int		getday(void);			// Implemented in asmcode.S
int		getmonth(void);			// Implemented in asmcode.S
int		gethour(void);			// Implemented in asmcode.S
int		getminute(void);		// Implemented in asmcode.S
int		getsecond(void);		// Implemented in asmcode.S
void	hidecursor(void);		// Implemented in asmcode.S
void	showcursor(void);		// Implemented in asmcode.S
int		wherex(void);			// Implemented in asmcode.S
int		wherey(void);			// Implemented in asmcode.S

int		strlen(char *str);
char	*strcpy(char *dest, char *src);
char	*strcat(char *dest, char *src);
int		strcmp(const char *string1, const char *string2);
int		stricmp(const char *string1, const char *string2);
char	*itoa(int value, char *string, int radix);
int		toupper(int c);
int		tolower(int c);
int		memcmp(const void *buf1, const void *buf2, size_t count);
void	*memcpy(void *dest, const void *src, size_t count);
void	*memset(void *dest, int c, size_t count);
char	*fgets(char *string, int n, FILE *stream);
int		atoi(char *string);


void	print(char *str);
void	printf(char *fmt, ...);

int		biosdisk(int cmd, int drive, int head, int track, int sector, int nsects, void *buffer); // Implemented in asmcode.S
void	stop_floppy(void);			// Implemented in asmcode.S
int		get_heads(int drive);		// Implemented in asmcode.S
int		get_cylinders(int drive);	// Implemented in asmcode.S
int		get_sectors(int drive);		// Implemented in asmcode.S

/* Values for biosdisk() */
#define _DISK_RESET		0	// Unimplemented
#define _DISK_STATUS	1	// Unimplemented
#define _DISK_READ		2	// Reads a sector into memory
#define _DISK_WRITE		3	// Unimplemented
#define _DISK_VERIFY	4	// Unimplemented
#define _DISK_FORMAT	5	// Unimplemented


#endif  // defined __STDLIB_H