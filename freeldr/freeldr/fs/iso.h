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

#ifndef __ISO_H
#define __ISO_H


struct _DIR_RECORD
{
  UCHAR  RecordLength;			// 1
  UCHAR  ExtAttrRecordLength;		// 2
  ULONG  ExtentLocationL;		// 3-6
  ULONG  ExtentLocationM;		// 7-10
  ULONG  DataLengthL;			// 11-14
  ULONG  DataLengthM;			// 15-18
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
  ULONG  VolumeSequenceNumber;		// 29-32
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
  ULONG  VolumeSpaceSizeL;		// 81-84
  ULONG  VolumeSpaceSizeM;		// 85-88
  UCHAR  unused2[32];			// 89-120
  ULONG  VolumeSetSize;			// 121-124
  ULONG  VolumeSequenceNumber;		// 125-128
  ULONG  LogicalBlockSize;		// 129-132
  ULONG  PathTableSizeL;		// 133-136
  ULONG  PathTableSizeM;		// 137-140
  ULONG  LPathTablePos;			// 141-144
  ULONG  LOptPathTablePos;		// 145-148
  ULONG  MPathTablePos;			// 149-152
  ULONG  MOptPathTablePos;		// 153-156
  DIR_RECORD RootDirRecord;		// 157-190
  UCHAR  VolumeSetIdentifier[128];	// 191-318
  UCHAR  PublisherIdentifier[128];	// 319-446

  /* more data ... */

} __attribute__((packed));

typedef struct _PVD PVD, *PPVD;



typedef struct
{
	ULONG	FileStart;		// File start sector
	ULONG	FileSize;		// File size
	ULONG	FilePointer;		// File pointer
	BOOL	Directory;
	ULONG	DriveNumber;
} ISO_FILE_INFO, * PISO_FILE_INFO;


BOOL	IsoOpenVolume(ULONG DriveNumber);
FILE*	IsoOpenFile(PUCHAR FileName);
BOOL	IsoReadFile(FILE *FileHandle, ULONG BytesToRead, PULONG BytesRead, PVOID Buffer);
ULONG	IsoGetFileSize(FILE *FileHandle);
VOID	IsoSetFilePointer(FILE *FileHandle, ULONG NewFilePointer);
ULONG	IsoGetFilePointer(FILE *FileHandle);

#endif // #defined __FAT_H
