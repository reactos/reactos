/*
	vfdfat.c

	Virtual Floppy Drive for Windows
	Driver control library
	Formats the image with FAT12

	Copyright (C) 2003-2005 Ken Kato
*/

#ifdef __cplusplus
#pragma message(__FILE__": Compiled as C++ for testing purpose.")
#endif	// __cplusplus

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "vfdtypes.h"
#include "vfdio.h"
#include "vfdlib.h"
#include "vfdver.h"

#pragma pack(1)
//
//	BIOS parameter block
//
typedef struct _DOS_BPB
{
	USHORT	BytesPerSector;
	UCHAR	SectorsPerCluster;
	USHORT	ReservedSectors;
	UCHAR	NumberOfFATs;
	USHORT	RootEntries;
	USHORT	SmallSectors;
	UCHAR	MediaDescriptor;
	USHORT	SectorsPerFAT;
	USHORT	SectorsPerTrack;
	USHORT	NumberOfHeads;
	ULONG	HiddenSectors;
	ULONG	LargeSectors;
}
DOS_BPB, *PDOS_BPB;

//
//	Extended BIOS parameter block for FAT12/16/HUGE
//
typedef struct _EXBPB
{
	UCHAR	PhysicalDriveNumber;
	UCHAR	Reserved;
	UCHAR	BootSignature;
	ULONG	VolumeSerialNumber;
	CHAR	VolumeLabel[11];
	CHAR	FileSystemType[8];
}
EXBPB, *PEXBPB;

//
//	Partition Boot Record
//
typedef struct _DOS_PBR {			//	Partition Boot Record
	UCHAR	jump[3];				//	Jump Instruction (E9 or EB, xx, 90)
	CHAR	oemid[8];				//	OEM ID (OS type)
	DOS_BPB	bpb;					//	BIOS parameter block
	EXBPB	exbpb;					//	Extended BIOS parameter block
}
DOS_PBR, *PDOS_PBR;

#pragma pack()

#define FAT_DIR_ENTRY_SIZE	32

// We need to have the 0xeb and 0x90 in the jump code
// because the file system recognizer checks these values
#define VFD_JUMP_CODE		"\xeb\x3c\x90"
#define VFD_OEM_NAME		"VFD" VFD_DRIVER_VERSION_STR "     "
#define VFD_VOLUME_LABEL	"NO NAME    "
#define VFD_FILESYSTEM		"FAT12   "

//
//	Select DOS BPB parameters from media size
//
static const DOS_BPB *SelectDosBpb(
	USHORT	nSectors)
{
	static const DOS_BPB bpb_tbl[] = {
		// b/s				  s/c r fat root  sec  desc s/f s/t  h
		{VFD_BYTES_PER_SECTOR, 2, 1, 2, 112,  320, 0xFE,  1,  8, 1, 0, 0},	// 160KB 5.25"
		{VFD_BYTES_PER_SECTOR, 2, 1, 2, 112,  360, 0xFC,  1,  9, 1, 0, 0},	// 180KB 5.25"
		{VFD_BYTES_PER_SECTOR, 2, 1, 2, 112,  640, 0xFF,  1,  8, 2, 0, 0},	// 320KB 5.25"
		{VFD_BYTES_PER_SECTOR, 2, 1, 2, 112,  720, 0xFD,  2,  9, 2, 0, 0},	// 360KB 5.25"
		{VFD_BYTES_PER_SECTOR, 2, 1, 2, 112, 1280, 0xFB,  2,  8, 2, 0, 0},	// 640KB 5.25" / 3.5"
		{VFD_BYTES_PER_SECTOR, 2, 1, 2, 112, 1440, 0xF9,  3,  9, 2, 0, 0},	// 720KB 5.25" / 3.5"
		{VFD_BYTES_PER_SECTOR, 2, 1, 2, 112, 1640, 0xF9,  3, 10, 2, 0, 0},	// 820KB 3.5"
		{VFD_BYTES_PER_SECTOR, 1, 1, 2, 224, 2400, 0xF9,  7, 15, 2, 0, 0},	// 1.20MB 5.25" / 3.5"
		{VFD_BYTES_PER_SECTOR, 1, 1, 2, 224, 2880, 0xF0,  9, 18, 2, 0, 0},	// 1.44MB 3.5"
		{VFD_BYTES_PER_SECTOR, 1, 1, 2, 224, 3360, 0xF0, 10, 21, 2, 0, 0},	// 1.68MB 3.5"
		{VFD_BYTES_PER_SECTOR, 1, 1, 2, 224, 3444, 0xF0, 10, 21, 2, 0, 0},	// 1.72MB 3.5"
		{VFD_BYTES_PER_SECTOR, 2, 1, 2, 240, 5760, 0xF0,  9, 36, 2, 0, 0},	// 2.88MB 3.5"
	};

	int i;

	for (i = 0; i < sizeof(bpb_tbl) / sizeof(bpb_tbl[0]); i++) {
		if (nSectors == bpb_tbl[i].SmallSectors) {
			return &bpb_tbl[i];
		}
	}

	return NULL;
}

//
//	Format the buffer with FAT12
//
DWORD FormatBufferFat(
	PUCHAR			pBuffer,
	ULONG			nSectors)
{
	const DOS_BPB	*bpb;	// BIOS Parameter Block
	PDOS_PBR		pbr;	// Partition Boot Record
	PUCHAR			fat;	// File Allocation Table
	USHORT			idx;

	VFDTRACE(0,
		("[VFD] VfdFormatImage - IN\n"));

	//
	//	Select DOS BPB parameters from media size
	//
	bpb = SelectDosBpb((USHORT)nSectors);

	if (!bpb) {
		VFDTRACE(0,
			("[VFD] Unsupported media size %lu\n",
			nSectors));
		return ERROR_INVALID_PARAMETER;
	}

	//
	//	Initialize the whole area with the fill data
	//
	FillMemory(pBuffer,
		VFD_SECTOR_TO_BYTE(nSectors),
		VFD_FORMAT_FILL_DATA);

	//
	//	Make up the FAT boot record
	//
	ZeroMemory(pBuffer, VFD_BYTES_PER_SECTOR);

	pbr = (PDOS_PBR)pBuffer;

	CopyMemory(pbr->jump,	VFD_JUMP_CODE,	sizeof(pbr->jump));
	CopyMemory(pbr->oemid,	VFD_OEM_NAME,	sizeof(pbr->oemid));
	CopyMemory(&pbr->bpb,	bpb,			sizeof(pbr->bpb));

	//	Make up the extended BPB

	pbr->exbpb.BootSignature		= 0x29;

	//	use the tick count as the volume serial number
	pbr->exbpb.VolumeSerialNumber	= GetTickCount();

	CopyMemory(pbr->exbpb.VolumeLabel,
		VFD_VOLUME_LABEL, sizeof(pbr->exbpb.VolumeLabel));

	CopyMemory(pbr->exbpb.FileSystemType,
		VFD_FILESYSTEM, sizeof(pbr->exbpb.FileSystemType));

	//	Set the boot record signature

	*(pBuffer + VFD_BYTES_PER_SECTOR - 2) = 0x55;
	*(pBuffer + VFD_BYTES_PER_SECTOR - 1) = 0xaa;

	//
	//	Clear FAT areas
	//
	fat = pBuffer + VFD_SECTOR_TO_BYTE(bpb->ReservedSectors);

	ZeroMemory(
		fat,
		VFD_SECTOR_TO_BYTE(bpb->SectorsPerFAT * bpb->NumberOfFATs));

	//
	//	Make up FAT entries for the root directory
	//
	for (idx = 0; idx < bpb->NumberOfFATs; idx++) {
		*fat = bpb->MediaDescriptor;
		*(fat + 1) = 0xff;
		*(fat + 2) = 0xff;

		fat += VFD_SECTOR_TO_BYTE(bpb->SectorsPerFAT);
	}

	//
	//	Clear root directory entries
	//
	ZeroMemory(fat, bpb->RootEntries * FAT_DIR_ENTRY_SIZE);

	VFDTRACE(0,
		("[VFD] VfdFormatImage - OUT\n"));

	return ERROR_SUCCESS;
}
