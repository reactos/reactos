#ifndef _FAT_H_INCLUDED
#define _FAT_H_INCLUDED

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 UInt32;
typedef unsigned __int64 u64;
typedef   signed __int8  s8;
typedef   signed __int16 s16;
typedef   signed __int32 s32;
typedef   signed __int64 s64;

#if 0
struct Fat_Bootrecord {
	u8  Jump[3];
	u8  OEMID[8];
	u8  BytesPerSector;
	u8  SectorsPerCluster;		// 1
	u16 ReservedSectors;		// 1
	u8  FATs;
	u16 RootEntries;
	u16 Sectors16;
	u8  MediaDescriptor;		// F0h = 1.44 MB, 3.5", 2-sided, 18-sectors per track
	u16 SectorsPerFAT;
	u16 SectorsPerTrack;		// 18
	u16 Heads;					
	u32 HiddenSectors;
	u32 Sectors32;
	u8  PhysicalDriveNo; // 00h for floppy, 80h for HDD
	u8  CurrentHead;
	u8  NTSignature;		 //for WinNT: 28h or 29h
	u32 SerialNumber;
	u8  VolumeLabel[11];
	u8  SystemID[8];
	u8  BootCode[510 - 62];
	u16 Signature; // 0xAA55
};
#endif

extern char* imageFileName;

#endif//_FAT_H_INCLUDED