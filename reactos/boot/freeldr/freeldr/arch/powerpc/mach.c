/*
 *  FreeLoader PowerPC Part
 *  Copyright (C) 2005  Art Yerkes
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
#include "freeldr.h"
#include "machine.h"
#include "ppcmmu/mmu.h"
#include "of.h"
#include "ppcboot.h"
#include "prep.h"
#include "compat.h"

extern void BootMain( LPSTR CmdLine );
extern PCHAR GetFreeLoaderVersionString();
extern ULONG CacheSizeLimit;
of_proxy ofproxy;
void *PageDirectoryStart, *PageDirectoryEnd;
static int chosen_package, stdin_handle, stdout_handle, 
  part_handle = -1, kernel_mem = 0;
int mmu_handle = 0, FixedMemory = 0;
BOOLEAN AcpiPresent = FALSE;
char BootPath[0x100] = { 0 }, BootPart[0x100] = { 0 }, CmdLine[0x100] = { "bootprep" };
jmp_buf jmp;
volatile char *video_mem = 0;
boot_infos_t BootInfo;

void PpcOfwPutChar( int ch ) {
    char buf[3];
    if( ch == 0x0a ) { buf[0] = 0x0d; buf[1] = 0x0a; } 
    else { buf[0] = ch; buf[1] = 0; }
    buf[2] = 0;
    ofw_write(stdout_handle, buf, strlen(buf));
}

int PpcFindDevice( int depth, int parent, char *devname, int *nth ) {
    static char buf[256];
    int next = 0;
    int gotname = 0;
    int match = 0;
    int i;

    next = ofw_child( parent );

    //printf( "next = %x\n", next );

    gotname = ofw_getprop(parent, "name", buf, 256);

    //printf( "gotname = %d\n", gotname );

    match = !strncmp(buf, devname, strlen(devname));

    if( !nth && match ) return parent;

    for( i = 0; i < depth; i++ ) PpcOfwPutChar( ' ' );
    
    if( depth == 1 ) {
	if( gotname > 0 ) {
	    printf( "%c Name: %s\n", match ? '*' : ' ', buf );
	} else {
	    printf( "- No name attribute for %x\n", parent );
	}
    }

    while( !match && next ) {
        i = PpcFindDevice( depth+1, next, devname, nth );
	if( i ) return i;
        next = ofw_peer( next );
    }

    return 0;
}

BOOLEAN PpcConsKbHit() {
    return FALSE;
}

int PpcConsGetCh() {
    char buf;
    ofw_read( stdin_handle, &buf, 1 );
    return buf;
}

void PpcVideoClearScreen( UCHAR Attr ) {
}

VOID PpcVideoGetDisplaySize( PULONG Width, PULONG Height, PULONG Depth ) {
    *Width = 80;
    *Height = 25;
    *Depth = 16;
}

ULONG PpcVideoGetBufferSize() {
    ULONG Width, Height, Depth;
    MachVideoGetDisplaySize( &Width, &Height, &Depth );
    return Width * Height * Depth / 8;
}

VIDEODISPLAYMODE PpcVideoSetDisplayMode( char *DisplayMode, BOOLEAN Init ) {
    //printf( "DisplayMode: %s %s\n", DisplayMode, Init ? "true" : "false" );
    if( Init && !video_mem ) {
	video_mem = MmAllocateMemory( PpcVideoGetBufferSize() );
    }
    return VideoTextMode;
}

VOID PpcVideoSetTextCursorPosition( ULONG X, ULONG Y ) {
    printf("SetTextCursorPosition(%d,%d)\n", X,Y);
}

VOID PpcVideoHideShowTextCursor( BOOLEAN Show ) {
    printf("HideShowTextCursor(%s)\n", Show ? "true" : "false");
}

VOID PpcVideoPutChar( int Ch, UCHAR Attr, unsigned X, unsigned Y ) {
    printf( "\033[%d;%dH%c", Y, X, Ch );
}

VOID PpcVideoCopyOffScreenBufferToVRAM( PVOID Buffer ) {
    int i,j;
    ULONG w,h,d;
    PCHAR ChBuf = Buffer;
    int offset = 0;

    MachVideoGetDisplaySize( &w, &h, &d );

    for( i = 0; i < h; i++ ) {
	for( j = 0; j < w; j++ ) {
	    offset = (j * 2) + (i * w * 2);
	    if( ChBuf[offset] != video_mem[offset] ) {
		video_mem[offset] = ChBuf[offset];
		MachVideoPutChar(ChBuf[offset],0,j+1,i+1);
	    }
	}
    }
}

BOOLEAN PpcVideoIsPaletteFixed() {
    return FALSE;
}

VOID PpcVideoSetPaletteColor( UCHAR Color, 
                              UCHAR Red, UCHAR Green, UCHAR Blue ) {
    printf( "SetPaletteColor(%x,%x,%x,%x)\n", Color, Red, Green, Blue );
}

VOID PpcVideoGetPaletteColor( UCHAR Color, 
                              UCHAR *Red, UCHAR *Green, UCHAR *Blue ) {
    printf( "GetPaletteColor(%x)\n", Color);
}

VOID PpcVideoSync() {
    printf( "Sync\n" );
}

static int prom_next_node(int *nodep)
{
	int node;

	if ((node = *nodep) != 0
	    && (*nodep = ofw_child(node)) != 0)
		return 1;
	if ((*nodep = ofw_peer(node)) != 0)
		return 1;
	for (;;) {
		if ((node = ofw_parent(node)) == 0)
			return 0;
		if ((*nodep = ofw_peer(node)) != 0)
			return 1;
	}
}

/* Appropriated from linux' btext.c
 * author:
 * Benjamin Herrenschmidt <benh@kernel.crashing.org>
 */
VOID PpcVideoPrepareForReactOS(BOOLEAN Setup) {
    int i, j, k, /* display_handle, */ display_package, display_size = 0;
    int node, ret, elts;
    int device_address;
    //pci_reg_property display_regs[8];
    char type[256], path[256], name[256];
    char logo[] = {
	"          "
	"  XXXXXX  "
	" X      X "
	" X X  X X "
	" X      X "
	" X XXXX X "
	" X  XX  X "
	" X      X "
	"  XXXXXX  "
	"          "
    };
    int logo_x = 10, logo_y = 10;
    int logo_scale_x = 8, logo_scale_y = 8;


    for( node = ofw_finddevice("/"); prom_next_node(&node); ) {
	memset(type, 0, sizeof(type));
	memset(path, 0, sizeof(path));
	
	ret = ofw_getprop(node, "name", name, sizeof(name));

	if(ofw_getprop(node, "device_type", type, sizeof(type)) <= 0) {
	    printf("Could not get type for node %x\n", node);
	    continue;
	}

	printf("Node %x ret %d name %s type %s\n", node, ret, name, type);

	if(strcmp(type, "display") == 0) break;
    }

    if(!node) return;

    if(ofw_package_to_path(node, path, sizeof(path)) < 0) {
	printf("could not get path for display package %x\n", node);
	return;
    }

    printf("Opening display package: %s\n", path);
    display_package = ofw_finddevice(path);
    printf("display package %x\n", display_package);

    BootInfo.dispDeviceRect[0] = BootInfo.dispDeviceRect[1] = 0;

    ofw_getprop(display_package, "width", 
		(void *)&BootInfo.dispDeviceRect[2], sizeof(int));
    ofw_getprop(display_package, "height",
		(void *)&BootInfo.dispDeviceRect[3], sizeof(int));
    ofw_getprop(display_package, "depth",
		(void *)&BootInfo.dispDeviceDepth, sizeof(int));
    ofw_getprop(display_package, "linebytes",
		(void *)&BootInfo.dispDeviceRowBytes, sizeof(int));

    BootInfo.dispDeviceRect[2] = BootInfo.dispDeviceRect[2];
    BootInfo.dispDeviceRect[3] = BootInfo.dispDeviceRect[3];
    BootInfo.dispDeviceDepth = BootInfo.dispDeviceDepth;
    BootInfo.dispDeviceRowBytes = BootInfo.dispDeviceRowBytes;

    if(ofw_getprop
       (display_package,
	"address",
	(void *)&device_address,
	sizeof(device_address)) < 1) {
	printf("Could not get device base\n");
	return;
    }

    BootInfo.dispDeviceBase = (PVOID)(device_address);

    display_size = BootInfo.dispDeviceRowBytes * BootInfo.dispDeviceRect[3];

    printf("Display size is %x bytes (%x per row times %x rows)\n",
	   display_size, 
	   BootInfo.dispDeviceRowBytes,
	   BootInfo.dispDeviceRect[3]);

    printf("display is at %x\n", BootInfo.dispDeviceBase);

    for( i = 0; i < logo_y * logo_scale_y; i++ ) {
	for( j = 0; j < logo_x * logo_scale_x; j++ ) {
	    elts = (j/logo_scale_x) + ((i/logo_scale_y) * logo_x);

	    for( k = 0; k < BootInfo.dispDeviceDepth/8; k++ ) {
		SetPhysByte(((ULONG_PTR)BootInfo.dispDeviceBase)+
			    k +
			    ((j * (BootInfo.dispDeviceDepth/8)) + 
			     (i * (BootInfo.dispDeviceRowBytes))),
			    logo[elts] == ' ' ? 0 : 255);
	    }
	}
    }
}

/* 
 * Get memory the proper openfirmware way
 */
ULONG PpcGetMemoryMap( PBIOS_MEMORY_MAP BiosMemoryMap,
                       ULONG MaxMemoryMapSize ) {
    int i, memhandle, returned, total = 0, slots = 0;
    int memdata[0x40];

    printf("PpcGetMemoryMap(%d)\n", MaxMemoryMapSize);

    memhandle = ofw_finddevice("/memory");

    returned = ofw_getprop(memhandle, "available", 
			   (char *)memdata, sizeof(memdata));

    printf("Returned data: %d\n", returned);
    if( returned == -1 ) {
	printf("getprop /memory[@reg] failed\n");
	return 0;
    }

    for( i = 0; i < returned; i++ ) {
	printf("%x ", memdata[i]);
    }
    printf("\n");

    for( i = 0; i < returned / 2; i++ ) {
	BiosMemoryMap[slots].Type = 1/*MEMTYPE_USABLE*/;
	BiosMemoryMap[slots].BaseAddress = memdata[i*2];
	BiosMemoryMap[slots].Length = memdata[i*2+1];
	printf("MemoryMap[%d] = (%x:%x)\n", 
	       i, 
	       (int)BiosMemoryMap[slots].BaseAddress,
	       (int)BiosMemoryMap[slots].Length);

	/* Hack for pearpc */
	if( kernel_mem ) {
	    BiosMemoryMap[slots].Length = kernel_mem * 1024;
	    if( !FixedMemory ) {
		ofw_claim((int)BiosMemoryMap[slots].BaseAddress,
			  (int)BiosMemoryMap[slots].Length,
			  0x1000);
		FixedMemory = BiosMemoryMap[slots].BaseAddress;
	    }
	    total += BiosMemoryMap[slots].Length;
	    slots++;
	    break;
	/* Normal way */
	} else if( BiosMemoryMap[slots].Length &&
		   ofw_claim((int)BiosMemoryMap[slots].BaseAddress,
			     (int)BiosMemoryMap[slots].Length,
			     0x1000) ) {
	    total += BiosMemoryMap[slots].Length;
	    slots++;
	}
    }

    printf( "Returning memory map (%dk total)\n", total / 1024 );

    return slots;
}

/* Strategy:
 *
 * For now, it'll be easy enough to use the boot command line as our boot path.
 * Treat it as the path of a disk partition.  We might even be able to get
 * away with grabbing a partition image by tftp in this scenario.
 */

BOOLEAN PpcDiskGetBootVolume( PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType ) {
    *DriveNumber = 0;
    *StartSector = 0;
    *SectorCount = 0;
    *FsType = FS_FAT;
    return TRUE;
}

BOOLEAN PpcDiskGetSystemVolume( char *SystemPath,
                             char *RemainingPath,
                             PULONG Device,
                             PULONG DriveNumber, 
                             PULONGLONG StartSector, 
                             PULONGLONG SectorCount, 
                             int *FsType ) {
    char *remain = strchr(SystemPath, '\\');
    if( remain ) {
	strcpy( RemainingPath, remain+1 );
    } else {
	RemainingPath[0] = 0;
    }
    *Device = 0;
    return MachDiskGetBootVolume(DriveNumber, StartSector, SectorCount, FsType);
}

BOOLEAN PpcDiskGetBootPath( char *OutBootPath, unsigned Size ) {
    strncpy( OutBootPath, BootPath, Size );
    return TRUE;
}

VOID PpcDiskGetBootDevice( PULONG BootDevice ) {
    BootDevice[0] = BootDevice[1] = 0;
}

BOOLEAN PpcDiskBootingFromFloppy(VOID) {
    return FALSE;
}

BOOLEAN PpcDiskReadLogicalSectors( ULONG DriveNumber, ULONGLONG SectorNumber,
				   ULONG SectorCount, PVOID Buffer ) {
    int rlen = 0;

    if( part_handle == -1 ) {
	part_handle = ofw_open( BootPart );
	
	if( part_handle == -1 ) {
	    printf("Could not open any disk devices we know about\n");
	    return FALSE;
	}
    }

    if( part_handle == -1 ) {
	printf("Got partition handle %x\n", part_handle);
	return FALSE;
    }

    if( ofw_seek( part_handle, 
		   (ULONG)(SectorNumber >> 25), 
		   (ULONG)((SectorNumber * 512) & 0xffffffff) ) ) {
	printf("Seek to %x failed\n", (ULONG)(SectorNumber * 512));
	return FALSE;
    }

    rlen = ofw_read( part_handle, Buffer, (ULONG)(SectorCount * 512) );
    return rlen > 0;
}

BOOLEAN PpcDiskGetPartitionEntry( ULONG DriveNumber, ULONG PartitionNumber,
                               PPARTITION_TABLE_ENTRY PartitionTableEntry ) {
    printf("GetPartitionEntry(%d,%d)\n", DriveNumber, PartitionNumber);
    return FALSE;
}

BOOLEAN PpcDiskGetDriveGeometry( ULONG DriveNumber, PGEOMETRY DriveGeometry ) {
    printf("GetGeometry(%d)\n", DriveNumber);
    DriveGeometry->BytesPerSector = 512;
    DriveGeometry->Heads = 16;
    DriveGeometry->Sectors = 63;
    return TRUE;
}

ULONG PpcDiskGetCacheableBlockCount( ULONG DriveNumber ) {
    printf("GetCacheableBlockCount\n");
    return 1;
}

VOID PpcRTCGetCurrentDateTime( PULONG Hear, PULONG Month, PULONG Day, 
                               PULONG Hour, PULONG Minute, PULONG Second ) {
    //printf("RTCGeturrentDateTime\n");
}

VOID PpcHwDetect() {
    printf("PpcHwDetect\n");
}

BOOLEAN PpcDiskNormalizeSystemPath(char *SystemPath, unsigned Size) {
	CHAR BootPath[256];
	ULONG PartitionNumber;
	ULONG DriveNumber;
	PARTITION_TABLE_ENTRY PartEntry;
	char *p;

	if (!DissectArcPath(SystemPath, BootPath, &DriveNumber, &PartitionNumber))
	{
		return FALSE;
	}

	if (0 != PartitionNumber)
	{
		return TRUE;
	}

	if (! DiskGetActivePartitionEntry(DriveNumber,
	                                  &PartEntry,
	                                  &PartitionNumber) ||
	    PartitionNumber < 1 || 9 < PartitionNumber)
	{
		return FALSE;
	}

	p = SystemPath;
	while ('\0' != *p && 0 != _strnicmp(p, "partition(", 10)) {
		p++;
	}
	p = strchr(p, ')');
	if (NULL == p || '0' != *(p - 1)) {
		return FALSE;
	}
	*(p - 1) = '0' + PartitionNumber;

	return TRUE;
}

extern int _bss;
typedef unsigned int uint32_t;

void PpcOfwInit()
{
    chosen_package = ofw_finddevice( "/chosen" );

    ofw_getprop(chosen_package, "bootargs",
		CmdLine, sizeof(CmdLine));
    ofw_getprop( chosen_package, "stdin",
		 (char *)&stdin_handle, sizeof(stdin_handle) );
    ofw_getprop( chosen_package, "stdout",
		 (char *)&stdout_handle, sizeof(stdout_handle) );
    ofw_getprop( chosen_package, "mmu",
		 (char *)&mmu_handle, sizeof(mmu_handle) );

    MachVtbl.ConsPutChar = PpcOfwPutChar;
    MachVtbl.ConsKbHit   = PpcConsKbHit;
    MachVtbl.ConsGetCh   = PpcConsGetCh;

    printf( "chosen_package %x, stdin_handle is %x\n", 
	    chosen_package, stdin_handle );
    printf("virt2phys (0xe00000,D) -> %x\n", PpcVirt2phys(0xe00000,0));
    printf("virt2phys (0xe01000,D) -> %x\n", PpcVirt2phys(0xe01000,0));

    MachVtbl.VideoClearScreen = PpcVideoClearScreen;
    MachVtbl.VideoSetDisplayMode = PpcVideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = PpcVideoGetDisplaySize;
    MachVtbl.VideoGetBufferSize = PpcVideoGetBufferSize;
    MachVtbl.VideoSetTextCursorPosition = PpcVideoSetTextCursorPosition;
    MachVtbl.VideoHideShowTextCursor = PpcVideoHideShowTextCursor;
    MachVtbl.VideoPutChar = PpcVideoPutChar;
    MachVtbl.VideoCopyOffScreenBufferToVRAM = 
        PpcVideoCopyOffScreenBufferToVRAM;
    MachVtbl.VideoIsPaletteFixed = PpcVideoIsPaletteFixed;
    MachVtbl.VideoSetPaletteColor = PpcVideoSetPaletteColor;
    MachVtbl.VideoGetPaletteColor = PpcVideoGetPaletteColor;
    MachVtbl.VideoSync = PpcVideoSync;
    MachVtbl.VideoPrepareForReactOS = PpcVideoPrepareForReactOS;

    MachVtbl.GetMemoryMap = PpcGetMemoryMap;

    MachVtbl.DiskNormalizeSystemPath = PpcDiskNormalizeSystemPath;
    MachVtbl.DiskGetBootVolume = PpcDiskGetBootVolume;
    MachVtbl.DiskGetSystemVolume = PpcDiskGetSystemVolume;
    MachVtbl.DiskGetBootPath = PpcDiskGetBootPath;
    MachVtbl.DiskGetBootDevice = PpcDiskGetBootDevice;
    MachVtbl.DiskBootingFromFloppy = PpcDiskBootingFromFloppy;
    MachVtbl.DiskReadLogicalSectors = PpcDiskReadLogicalSectors;
    MachVtbl.DiskGetPartitionEntry = PpcDiskGetPartitionEntry;
    MachVtbl.DiskGetDriveGeometry = PpcDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = PpcDiskGetCacheableBlockCount;

    MachVtbl.RTCGetCurrentDateTime = PpcRTCGetCurrentDateTime;

    MachVtbl.HwDetect = PpcHwDetect;

    // Allow forcing prep for broken OFW
    if(!strncmp(CmdLine, "bootprep", 8))
    {
	printf("Going to PREP init...\n");
	PpcPrepInit();
	return;
    }

    printf( "FreeLDR version [%s]\n", GetFreeLoaderVersionString() );

    BootMain( CmdLine );
}

void PpcInit( of_proxy the_ofproxy ) {
    ofproxy = the_ofproxy;
    if(ofproxy) PpcOfwInit();
    else PpcPrepInit();
}

void MachInit(const char *CmdLine) {
    int i, len;
    char *sep;

    BootPart[0] = 0;
    BootPath[0] = 0;

    printf( "Determining boot device: [%s]\n", CmdLine );

    sep = NULL;
    for( i = 0; i < strlen(CmdLine); i++ ) {
	if( strncmp(CmdLine + i, "boot=", 5) == 0) {
	    strcpy(BootPart, CmdLine + i + 5);
	    sep = strchr(BootPart, ',');
	    if( sep )
		*sep = 0;
	    while(CmdLine[i] && CmdLine[i]!=',') i++;
	}
	if( strncmp(CmdLine + i, "mem=", 4) == 0) {
	    kernel_mem = atoi(CmdLine+i+4);
	    printf("Allocate %dk kernel memory\n", kernel_mem);
	    while(CmdLine[i] && CmdLine[i]!=',') i++;
	}
    }

    if( strlen(BootPart) == 0 ) {
	if (ofproxy)
            len = ofw_getprop(chosen_package, "bootpath", 
                              BootPath, sizeof(BootPath));
	else
            len = 0;
	if( len < 0 ) len = 0;
	BootPath[len] = 0;
	printf( "Boot Path: %s\n", BootPath );
	
	sep = strrchr(BootPath, ',');
	
	strcpy(BootPart, BootPath);
	if( sep ) {
	    BootPart[sep - BootPath] = 0;
	}
    }

    printf( "FreeLDR starting (boot partition: %s)\n", BootPart );
}

/* Compatibility functions that don't do much */
void beep() {
}

UCHAR NTAPI READ_PORT_UCHAR(PUCHAR Address) {
    return GetPhysByte(((ULONG)Address)+0x80000000);
}

void WRITE_PORT_UCHAR(PUCHAR Address, UCHAR Value) {
    SetPhysByte(((ULONG)Address)+0x80000000, Value);
}

void DiskStopFloppyMotor() {
}

void BootOldLinuxKernel( unsigned long size ) {
    ofw_exit();
}

void BootNewLinuxKernel() {
    ofw_exit();
}

void ChainLoadBiosBootSectorCode() {
    ofw_exit();
}

void DbgBreakPoint() {
    ofw_exit();
}
