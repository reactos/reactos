/*
 * COPYRIGHT:    See COPYING in the top level directory
 * PROJECT:      ReactOS kernel
 * FILE:         include/disk.h
 * PURPOSE:      Disk related definitions used by all the parts of the system
 * PROGRAMMER:   David Welch <welch@cwcom.net>
 * UPDATE HISTORY: 
 *               27/06/00: Created
 */


#ifndef __INCLUDE_DISK_H
#define __INCLUDE_DISK_H

typedef enum _MEDIA_TYPE { 
  Unknown,                
  F5_1Pt2_512,            
  F3_1Pt44_512,           
  F3_2Pt88_512,           
  F3_20Pt8_512,           
  F3_720_512,             
  F5_360_512,             
  F5_320_512,             
  F5_320_1024,            
  F5_180_512,             
  F5_160_512,             
  RemovableMedia,         
  FixedMedia              
} MEDIA_TYPE; 

typedef struct _PARTITION_INFORMATION {
  BYTE PartitionType;
  BOOLEAN BootIndicator;
  BOOLEAN RecognizedPartition;
  BOOLEAN RewritePartition;
  LARGE_INTEGER StartingOffset;
  LARGE_INTEGER PartitionLength;
  LARGE_INTEGER HiddenSectors;
} PARTITION_INFORMATION;

typedef struct _DRIVE_LAYOUT_INFORMATION { 
  DWORD  PartitionCount; 
  DWORD  Signature; 
  PARTITION_INFORMATION  PartitionEntry[1]; 
} DRIVE_LAYOUT_INFORMATION; 

typedef struct _DISK_GEOMETRY { 
  LARGE_INTEGER  Cylinders; 
  MEDIA_TYPE  MediaType; 
  DWORD  TracksPerCylinder; 
  DWORD  SectorsPerTrack; 
  DWORD  BytesPerSector; 
} DISK_GEOMETRY ; 


#endif /* __INCLUDE_DISK_H */
