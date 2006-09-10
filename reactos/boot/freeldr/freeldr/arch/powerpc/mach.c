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
#include "of.h"
#include "mmu.h"

#define TOTAL_HEAP_NEEDED (48 * 1024 * 1024) /* 48 megs */

extern void BootMain( LPSTR CmdLine );
extern PCHAR GetFreeLoaderVersionString();
extern ULONG CacheSizeLimit;
of_proxy ofproxy;
void *PageDirectoryStart, *PageDirectoryEnd, *mem_base = 0;
static int chosen_package, stdin_handle, part_handle = -1;
BOOLEAN AcpiPresent = FALSE;
char BootPath[0x100] = { 0 }, BootPart[0x100] = { 0 }, CmdLine[0x100] = { 0 };
jmp_buf jmp;
volatile char *video_mem = 0;

void le_swap( void *start_addr_v, 
              void *end_addr_v, 
              void *target_addr_v ) {
    long 
	*start_addr = (long *)ROUND_DOWN((long)start_addr_v,8), 
        *end_addr = (long *)ROUND_UP((long)end_addr_v,8), 
        *target_addr = (long *)ROUND_DOWN((long)target_addr_v,8);
    long tmp;
    while( start_addr <= end_addr ) {
        tmp = start_addr[0];
        target_addr[0] = REV(start_addr[1]);
        target_addr[1] = REV(tmp);
        start_addr += 2;
        target_addr += 2;
    }
}

void PpcPutChar( int ch ) {
    char buf[3];
    if( ch == 0x0a ) { buf[0] = 0x0d; buf[1] = 0x0a; } 
    else { buf[0] = ch; buf[1] = 0; }
    buf[2] = 0;
    ofw_print_string( buf );
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

    for( i = 0; i < depth; i++ ) PpcPutChar( ' ' );
    
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
    ofw_print_string("ClearScreen\n");
}

VOID PpcVideoGetDisplaySize( PULONG Width, PULONG Height, PULONG Depth ) {
    //ofw_print_string("GetDisplaySize\n");
    *Width = 80;
    *Height = 25;
    *Depth = 16;
    //printf("GetDisplaySize(%d,%d,%d)\n", *Width, *Height, *Depth);
}

ULONG PpcVideoGetBufferSize() {
    ULONG Width, Height, Depth;
    //ofw_print_string("PpcVideoGetBufferSize\n");
    PpcVideoGetDisplaySize( &Width, &Height, &Depth );
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

    PpcVideoGetDisplaySize( &w, &h, &d );

    for( i = 0; i < h; i++ ) {
	for( j = 0; j < w; j++ ) {
	    offset = (j * 2) + (i * w * 2);
	    if( ChBuf[offset] != video_mem[offset] ) {
		video_mem[offset] = ChBuf[offset];
		PpcVideoPutChar(ChBuf[offset],0,j+1,i+1);
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

VOID PpcVideoPrepareForReactOS() {
    printf( "PrepareForReactOS\n");
}
/* 
 * Get memory the proper openfirmware way
 */
ULONG PpcGetMemoryMap( PBIOS_MEMORY_MAP BiosMemoryMap,
                       ULONG MaxMemoryMapSize ) {
    int i, memhandle, mmuhandle, returned, total = 0, num_mem = 0;
    int memdata[256];

    printf("PpcGetMemoryMap(%d)\n", MaxMemoryMapSize);

    if( mem_base ) {
	BiosMemoryMap[0].Type = MEMTYPE_USABLE;
	BiosMemoryMap[0].BaseAddress = (ULONG)mem_base;
	BiosMemoryMap[0].Length = TOTAL_HEAP_NEEDED;
	printf("[cached] returning 1 element\n");
	return 1;
    }

    ofw_getprop(chosen_package, "memory", 
		(char *)&memhandle, sizeof(memhandle));
    ofw_getprop(chosen_package, "mmu",
		(char *)&mmuhandle, sizeof(mmuhandle));

    returned = ofw_getprop(memhandle, "available", 
			   (char *)memdata, sizeof(memdata));

    /* We need to leave some for open firmware.  Let's claim up to 16 megs 
     * for now */

    for( i = 0; i < returned / sizeof(int) && !num_mem; i += 2 ) {
	BiosMemoryMap[num_mem].Type = MEMTYPE_USABLE;
	BiosMemoryMap[num_mem].BaseAddress = memdata[i];
	mem_base = (void *)memdata[i];
	BiosMemoryMap[num_mem].Length = memdata[i+1];
	if( BiosMemoryMap[num_mem].Length >= TOTAL_HEAP_NEEDED && 
	    total < TOTAL_HEAP_NEEDED ) {
	    BiosMemoryMap[num_mem].Length = TOTAL_HEAP_NEEDED;	     
	    ofw_claim(BiosMemoryMap[num_mem].BaseAddress, 
		      BiosMemoryMap[num_mem].Length, 0x1000); /* claim it */
	    total += BiosMemoryMap[0].Length;
	    num_mem++;
	}
    }

    printf( "Returning memory map (%dk total)\n", total / 1024 );

    return num_mem;
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
    return PpcDiskGetBootVolume(DriveNumber, StartSector, SectorCount, FsType);
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

typedef unsigned int uint32_t;

void PpcInit( of_proxy the_ofproxy ) {
    int len, stdin_handle_chosen;
    ofproxy = the_ofproxy;

    ofw_print_string("Freeldr PowerPC Init\n");

    chosen_package = ofw_finddevice( "/chosen" );

    ofw_print_string("Freeldr: chosen_package is ");
    ofw_print_number(chosen_package);
    ofw_print_string("\n");

    ofw_getprop( chosen_package, "stdin",
                 (char *)&stdin_handle_chosen, sizeof(stdin_handle_chosen) );

    ofw_print_string("Freeldr: stdin_handle is ");
    ofw_print_number(stdin_handle_chosen);
    ofw_print_string("\n");

    stdin_handle = stdin_handle_chosen;

    /* stdin_handle = REV(stdin_handle); */

    MachVtbl.ConsPutChar = PpcPutChar;
    MachVtbl.ConsKbHit   = PpcConsKbHit;
    MachVtbl.ConsGetCh   = PpcConsGetCh;

    printf( "stdin_handle is %x\n", stdin_handle );
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

    printf( "FreeLDR version [%s]\n", GetFreeLoaderVersionString() );

    len = ofw_getprop(chosen_package, "bootargs",
		      CmdLine, sizeof(CmdLine));

    if( len < 0 ) len = 0;
    CmdLine[len] = 0;

    BootMain( CmdLine );
}

void MachInit(const char *CmdLine) {
    int len, i;
    char *sep;

    BootPart[0] = 0;
    BootPath[0] = 0;

    printf( "Determining boot device: [%s]\n", CmdLine );

    printf( "Boot Args: %s\n", CmdLine );
    sep = NULL;
    for( i = 0; i < strlen(CmdLine); i++ ) {
	if( strncmp(CmdLine + i, "boot=", 5) == 0) {
	    strcpy(BootPart, CmdLine + i + 5);
	    sep = strchr(BootPart, ' ');
	    if( sep )
		*sep = 0;
	    break;
	}
    }

    if( strlen(BootPart) == 0 ) {
	len = ofw_getprop(chosen_package, "bootpath", 
			  BootPath, sizeof(BootPath));
	
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
    return 0xff;
}

void WRITE_PORT_UCHAR(PUCHAR Address, UCHAR Value) {
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
