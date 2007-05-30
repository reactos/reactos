/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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


//#define	EOF				-1

#define	FS_FAT			1
#define	FS_NTFS			2
#define	FS_EXT2			3
#define FS_REISER		4
#define FS_ISO9660		5

#define FILE			VOID
#define PFILE			FILE *

VOID	FileSystemError(PCSTR ErrorString);
BOOLEAN	FsOpenBootVolume();
BOOLEAN	FsOpenSystemVolume(PCHAR SystemPath, PCHAR RemainingPath, PULONG BootDevice);
PFILE	FsOpenFile(PCSTR FileName);
VOID	FsCloseFile(PFILE FileHandle);
BOOLEAN	FsReadFile(PFILE FileHandle, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer);
ULONG		FsGetFileSize(PFILE FileHandle);
VOID	FsSetFilePointer(PFILE FileHandle, ULONG NewFilePointer);
ULONG		FsGetFilePointer(PFILE FileHandle);
BOOLEAN	FsIsEndOfFile(PFILE FileHandle);
ULONG		FsGetNumPathParts(PCSTR Path);
VOID	FsGetFirstNameFromPath(PCHAR Buffer, PCSTR Path);

#endif // #defined __FS_H
