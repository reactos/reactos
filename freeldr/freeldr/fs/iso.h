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

#ifndef __ISO_H
#define __ISO_H


struct _DIR_RECORD
{
  UCHAR  RecordLength;			// 1
  UCHAR  ExtAttrRecordLength;		// 2
  U32  ExtentLocationL;		// 3-6
  U32  ExtentLocationM;		// 7-10
  U32  DataLengthL;			// 11-14
  U32  DataLengthM;			// 15-18
  UCHAR  Year;				// 19
  UCHAR  Month;				// 20
  UCHAR  Day;				// 21
  UCHAR  Hour;				// 22
  UCHAR  Minute;			// 23
  UCHAR  Second;			// 24
  UCHAR  TimeZone;			// 25
  UCHAR  FileFlags;			// 26
  UCHAR  FileUnitSize;			// 27
  UCHAR  InterleaveGapSize;		// 28
  U32  VolumeSequenceNumber;		// 29-32
  UCHAR  FileIdLength;			// 33
  UCHAR  FileId[1];			// 34
} __attribute__((packed));

typedef struct _DIR_RECORD DIR_RECORD, *PDIR_RECORD;




/* Volume Descriptor header*/
struct _VD_HEADER
{
  UCHAR  VdType;			// 1
  UCHAR  StandardId[5];			// 2-6
  UCHAR  VdVersion;			// 7
} __attribute__((packed));

typedef struct _VD_HEADER VD_HEADER, *PVD_HEADER;



/* Primary Volume Descriptor */
struct _PVD
{
  UCHAR  VdType;			// 1
  UCHAR  StandardId[5];			// 2-6
  UCHAR  VdVersion;			// 7
  UCHAR  unused0;			// 8
  UCHAR  SystemId[32];			// 9-40
  UCHAR  VolumeId[32];			// 41-72
  UCHAR  unused1[8];			// 73-80
  U32  VolumeSpaceSizeL;		// 81-84
  U32  VolumeSpaceSizeM;		// 85-88
  UCHAR  unused2[32];			// 89-120
  U32  VolumeSetSize;			// 121-124
  U32  VolumeSequenceNumber;		// 125-128
  U32  LogicalBlockSize;		// 129-132
  U32  PathTableSizeL;		// 133-136
  U32  PathTableSizeM;		// 137-140
  U32  LPathTablePos;			// 141-144
  U32  LOptPathTablePos;		// 145-148
  U32  MPathTablePos;			// 149-152
  U32  MOptPathTablePos;		// 153-156
  DIR_RECORD RootDirRecord;		// 157-190
  UCHAR  VolumeSetIdentifier[128];	// 191-318
  UCHAR  PublisherIdentifier[128];	// 319-446

  /* more data ... */

} __attribute__((packed));

typedef struct _PVD PVD, *PPVD;



typedef struct
{
	U32		FileStart;		// File start sector
	U32		FileSize;		// File size
	U32		FilePointer;		// File pointer
	BOOL	Directory;
	U32		DriveNumber;
} ISO_FILE_INFO, * PISO_FILE_INFO;


BOOL	IsIsoFs(U32 DriveNumber);
BOOL	IsoOpenVolume(U32 DriveNumber);
FILE*	IsoOpenFile(PUCHAR FileName);
BOOL	IsoReadFile(FILE *FileHandle, U32 BytesToRead, U32* BytesRead, PVOID Buffer);
U32		IsoGetFileSize(FILE *FileHandle);
VOID	IsoSetFilePointer(FILE *FileHandle, U32 NewFilePointer);
U32		IsoGetFilePointer(FILE *FileHandle);

#endif // #defined __FAT_H
