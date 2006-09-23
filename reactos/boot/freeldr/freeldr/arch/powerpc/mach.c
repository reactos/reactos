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

extern void BootMain( char * );
extern char *GetFreeLoaderVersionString();
of_proxy ofproxy;
void *PageDirectoryStart, *PageDirectoryEnd;
static int chosen_package, stdin_handle, part_handle = -1;
BOOLEAN AcpiPresent = FALSE;
char BootPath[0x100] = { 0 }, BootPart[0x100] = { 0 }, CmdLine[0x100] = { 0 };
jmp_buf jmp;

void le_swap( const void *start_addr_v, 
              const void *end_addr_v, 
              const void *target_addr_v ) {
    long *start_addr = (long *)ROUND_DOWN((long)start_addr_v,8), 
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

int ofw_finddevice( const char *name ) {
    int ret, len;

    len = strlen(name);
    le_swap( name, name + len, name );
    ret = ofproxy( 0, (char *)name, NULL, NULL, NULL );
    le_swap( name, name + len, name );
    return ret;
}

int ofw_getprop( int package, const char *name, void *buffer, int buflen ) {
    int ret, len = strlen(name);
    le_swap( name, name + len, name );
    le_swap( buffer, (char *)buffer + buflen, buffer );
    ret = ofproxy
        ( 4, (void *)package, (char *)name, buffer, (void *)buflen );
    le_swap( buffer, (char *)buffer + buflen, buffer );
    le_swap( name, name + len, name );
    return ret;
}

/* Since this is from external storage, it doesn't need swapping */
int ofw_write( int handle, const char *data, int len ) {
    int ret;
    le_swap( data, data + len, data );
    ret = ofproxy
        ( 8, (void *)handle, (char *)data, (void *)len, NULL );
    le_swap( data, data + len, data );
    return ret;
}

/* Since this is from external storage, it doesn't need swapping */
int ofw_read( int handle, const char *data, int len ) {
    int ret;

    le_swap( data, data + len, data );
    ret = ofproxy
        ( 12, (void *)handle, (char *)data, (void *)len, NULL );
    le_swap( data, data + len, data );

    return ret;
}

void ofw_exit() {
    ofproxy( 16, NULL, NULL, NULL, NULL );
}

void ofw_dumpregs() {
    ofproxy( 20, NULL, NULL, NULL, NULL );
}

void ofw_print_string( const char *str ) {
    int len = strlen(str);
    le_swap( (char *)str, str + len, (char *)str );
    ofproxy( 24, (void *)str, NULL, NULL, NULL );
    le_swap( (char *)str, str + len, (char *)str );
}

void ofw_print_number( int num ) {
    ofproxy( 28, (void *)num, NULL, NULL, NULL );
}

int ofw_open( const char *name ) {
    int ret, len;

    len = strlen(name);
    le_swap( name, name + len, name );
    ret = ofproxy( 32, (char *)name, NULL, NULL, NULL );
    le_swap( name, name + len, name );
    return ret;
}

int ofw_child( int package ) {
    return ofproxy( 36, (void *)package, NULL, NULL, NULL );
}

int ofw_peer( int package ) {
    return ofproxy( 40, (void *)package, NULL, NULL, NULL );
}

int ofw_seek( int handle, long long location ) {
    return ofproxy( 44, (void *)handle, (void *)(int)(location >> 32), (void *)(int)location, NULL );
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
    return TRUE;
}

int PpcConsGetCh() {
    char buf;
    ofw_read( stdin_handle, &buf, 1 );
    return buf;
}

void PpcVideoClearScreen( UCHAR Attr ) {
    ofw_print_string("ClearScreen\n");
}

VIDEODISPLAYMODE PpcVideoSetDisplayMode( char *DisplayMode, BOOLEAN Init ) {
    printf( "DisplayMode: %s %s\n", DisplayMode, Init ? "true" : "false" );
    return VideoGraphicsMode;
}

/* FIXME: Query */
VOID PpcVideoGetDisplaySize( PULONG Width, PULONG Height, PULONG Depth ) {
    ofw_print_string("GetDisplaySize\n");
    *Width = 640;
    *Height = 480;
    *Depth = 8;
}

ULONG PpcVideoGetBufferSize() {
    ULONG Width, Height, Depth;
    ofw_print_string("PpcVideoGetBufferSize\n");
    PpcVideoGetDisplaySize( &Width, &Height, &Depth );
    return Width * Height * Depth / 8;
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
    printf( "CopyOffScreenBufferToVRAM(%x)\n", Buffer );
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
/* XXX FIXME:
 * According to the linux people (this is backed up by my own experience),
 * the memory object in older ofw does not do getprop right.
 *
 * The "right" way is to probe the pci bridge. *sigh*
 */
ULONG PpcGetMemoryMap( PBIOS_MEMORY_MAP BiosMemoryMap,
                       ULONG MaxMemoryMapSize ) {
    printf("GetMemoryMap(chosen=%x)\n", chosen_package);

    BiosMemoryMap[0].Type = BiosMemoryUsable;
    BiosMemoryMap[0].BaseAddress = 0;
    BiosMemoryMap[0].Length = 32 * 1024 * 1024; /* Assume 32 meg for now */

    printf( "Returning memory map (%dk total)\n", 
            (int)BiosMemoryMap[0].Length / 1024 );

    return 1;
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
    return FALSE;
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

    if( ofw_seek( part_handle, SectorNumber * 512 ) ) {
	printf("Seek to %x failed\n", SectorNumber * 512);
	return FALSE;
    }
    rlen = ofw_read( part_handle, Buffer, SectorCount * 512 );
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
    printf("RTCGeturrentDateTime\n");
}

VOID PpcHwDetect() {
    printf("PpcHwDetect\n");
}

typedef unsigned int uint32_t;

void PpcInit( of_proxy the_ofproxy ) {
    int len;
    ofproxy = the_ofproxy;

    chosen_package = ofw_finddevice( "/chosen" );

    ofw_getprop( chosen_package, "stdin",
                 &stdin_handle, sizeof(stdin_handle) );

    stdin_handle = REV(stdin_handle);

    MachVtbl.ConsPutChar = PpcPutChar;
    MachVtbl.ConsKbHit   = PpcConsKbHit;
    MachVtbl.ConsGetCh   = PpcConsGetCh;
    
    printf( "stdin_handle is %x\n", stdin_handle );

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
