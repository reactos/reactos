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

#ifndef __FS_H
#define __FS_H


#define	EOF				-1

#define	FS_FAT			1
#define	FS_NTFS			2
#define	FS_EXT2			3
#define FS_REISER		4
#define FS_ISO9660		5

#define FILE			VOID
#define PFILE			FILE *

VOID	FileSystemError(PUCHAR ErrorString);
BOOL	OpenDiskDrive(ULONG DriveNumber, ULONG PartitionNumber);
PFILE	OpenFile(PUCHAR FileName);
VOID	CloseFile(PFILE FileHandle);
BOOL	ReadFile(PFILE FileHandle, ULONG BytesToRead, PULONG BytesRead, PVOID Buffer);
ULONG	GetFileSize(PFILE FileHandle);
VOID	SetFilePointer(PFILE FileHandle, ULONG NewFilePointer);
ULONG	GetFilePointer(PFILE FileHandle);
BOOL	IsEndOfFile(PFILE FileHandle);

#endif // #defined __FS_H
