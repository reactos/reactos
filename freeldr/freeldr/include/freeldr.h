/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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

#define	NULL	0
#define	TRUE	1
#define FALSE	0

#define BOOL	int
#define BOOLEAN	int

#define CHAR	char
#define PCHAR	char *
#define UCHAR	unsigned char
#define PUCHAR	unsigned char *
#define WCHAR	unsigned short
#define PWCHAR	unsigned short *

#define VOID	void
#define PVOID	VOID*

#ifdef __i386__

#define	size_t	unsigned int

typedef unsigned char		U8;
typedef char				S8;
typedef unsigned short		U16;
typedef short				S16;
typedef unsigned long		U32;
typedef long				S32;
typedef unsigned long long	U64;
typedef long long			S64;

typedef U8					__u8;
typedef S8					__s8;
typedef U16					__u16;
typedef S16					__s16;
typedef U32					__u32;
typedef S32					__s32;
typedef U64					__u64;
typedef S64					__s64;

#endif // __i386__

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

#define PACKED __attribute__((packed))

extern U32			BootDrive;			// BIOS boot drive, 0-A:, 1-B:, 0x80-C:, 0x81-D:, etc.
extern U32			BootPartition;		// Boot Partition, 1-4
extern BOOL			UserInterfaceUp;	// Tells us if the user interface is displayed

void	BootMain(void);
VOID	RunLoader(VOID);

#endif  // defined __FREELDR_H
