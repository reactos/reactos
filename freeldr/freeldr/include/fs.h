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
BOOL	FsOpenVolume(U32 DriveNumber, U32 PartitionNumber);
PFILE	FsOpenFile(PUCHAR FileName);
VOID	FsCloseFile(PFILE FileHandle);
BOOL	FsReadFile(PFILE FileHandle, U32 BytesToRead, U32* BytesRead, PVOID Buffer);
U32		FsGetFileSize(PFILE FileHandle);
VOID	FsSetFilePointer(PFILE FileHandle, U32 NewFilePointer);
U32		FsGetFilePointer(PFILE FileHandle);
BOOL	FsIsEndOfFile(PFILE FileHandle);
U32		FsGetNumPathParts(PUCHAR Path);
VOID	FsGetFirstNameFromPath(PUCHAR Buffer, PUCHAR Path);

#endif // #defined __FS_H
