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


#define	size_t	unsigned int
#define BOOL	int
#define BOOLEAN	int
#define	NULL	0
#define	TRUE	1
#define FALSE	0
#define BYTE	unsigned char
#define WORD	unsigned short
#define DWORD	unsigned long
#define CHAR	char
#define PCHAR	char *
#define UCHAR	unsigned char
#define PUCHAR	unsigned char *
#define WCHAR	unsigned short
#define PWCHAR	unsigned short *
#define SHORT	short
#define USHORT	unsigned short
#define PUSHORT	unsigned short *
#define LONG	long
#define ULONG	unsigned long
#define PULONG	unsigned long *
#define PDWORD	DWORD *
#define PWORD	WORD *
#define VOID	void
#define PVOID	VOID*
#define INT8	char
#define UINT8	unsigned char
#define INT16	short
#define UINT16	unsigned short
#define INT32	long
#define UINT32	unsigned long
#define PUINT32	UINT32 *
#define INT64	long long
#define UINT64	unsigned long long

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))

#define PACKED __attribute__((packed))

extern ULONG		BootDrive;			// BIOS boot drive, 0-A:, 1-B:, 0x80-C:, 0x81-D:, etc.
extern ULONG		BootPartition;		// Boot Partition, 1-4
extern BOOL			UserInterfaceUp;	// Tells us if the user interface is displayed

void	BootMain(void);

#endif  // defined __FREELDR_H
