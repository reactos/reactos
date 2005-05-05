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
ULONG BootPartition = 0;
ULONG BootDrive = 0;

of_proxy ofproxy;
void *PageDirectoryStart, *PageDirectoryEnd;
static int chosen_package, stdin_handle;
BOOLEAN AcpiPresent = FALSE;
char BootPath[0x100];

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
    le_swap( buffer, buffer + buflen, buffer );
    ret = ofproxy
        ( 4, (void *)package, (char *)name, buffer, (void *)buflen );
    le_swap( buffer, buffer + buflen, buffer );
    le_swap( name, name + len, name );
    return ret;
}

int ofw_write( int handle, const char *data, int len ) {
    int ret;
    le_swap( data, data + len, data );
    ret = ofproxy
        ( 8, (void *)handle, (char *)data, (void *)len, NULL );
    le_swap( data, data + len, data );
    return ret;
}

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

void PpcPutChar( int ch ) {
    char buf[3];
    if( ch == 0x0a ) { buf[0] = 0x0d; buf[1] = 0x0a; } 
    else { buf[0] = ch; buf[1] = 0; }
    buf[2] = 0;
    ofw_print_string( buf );
}

BOOL PpcConsKbHit() {
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

VIDEODISPLAYMODE PpcVideoSetDisplayMode( char *DisplayMode, BOOL Init ) {
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

VOID PpcVideoHideShowTextCursor( BOOL Show ) {
    printf("HideShowTextCursor(%s)\n", Show ? "true" : "false");
}

VOID PpcVideoPutChar( int Ch, UCHAR Attr, unsigned X, unsigned Y ) {
    printf( "\033[%d;%dH%c", Y, X, Ch );
}

VOID PpcVideoCopyOffScreenBufferToVRAM( PVOID Buffer ) {
    printf( "CopyOffScreenBufferToVRAM(%x)\n", Buffer );
}

BOOL PpcVideoIsPaletteFixed() {
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

    BiosMemoryMap[0].Type = MEMTYPE_USABLE;
    BiosMemoryMap[0].BaseAddress = 0;
    BiosMemoryMap[0].Length = 32 * 1024 * 1024; /* Assume 32 meg for now */

    printf( "Returning memory map (%dk total)\n", 
            (int)BiosMemoryMap[0].Length / 1024 );

    return 1;
}

BOOL PpcDiskReadLogicalSectors( ULONG DriveNumber, ULONGLONG SectorNumber,
                                ULONG SectorCount, PVOID Buffer ) {
    printf("DiskReadLogicalSectors\n");
    return FALSE;
}

BOOL PpcDiskGetPartitionEntry( ULONG DriveNumber, ULONG PartitionNumber,
                               PPARTITION_TABLE_ENTRY PartitionTableEntry ) {
    printf("GetPartitionEntry(%d,%d)\n", DriveNumber, PartitionNumber);
    return FALSE;
}

BOOL PpcDiskGetDriveGeometry( ULONG DriveNumber, PGEOMETRY DriveGeometry ) {
    printf("GetGeometry(%d)\n", DriveNumber);
    return FALSE;
}

ULONG PpcDiskGetCacheableBlockCount( ULONG DriveNumber ) {
    printf("GetCacheableBlockCount\n");
    return 0;
}

VOID PpcRTCGetCurrentDateTime( PULONG Hear, PULONG Month, PULONG Day, 
                               PULONG Hour, PULONG Minute, PULONG Second ) {
    printf("RTCGeturrentDateTime\n");
}

VOID PpcHwDetect() {
}

void PpcInit( of_proxy the_ofproxy ) {
    ofproxy = the_ofproxy;
    chosen_package = ofw_finddevice( "/chosen" );

    ofw_getprop( chosen_package, "stdin",
                 &stdin_handle, sizeof(stdin_handle) );

    stdin_handle = REV(stdin_handle);

    MachVtbl.ConsPutChar = PpcPutChar;
    MachVtbl.ConsKbHit   = PpcConsKbHit;
    MachVtbl.ConsGetCh   = PpcConsGetCh;

    printf("chosen_package = %x\n", chosen_package);

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

    MachVtbl.DiskReadLogicalSectors = PpcDiskReadLogicalSectors;
    MachVtbl.DiskGetPartitionEntry = PpcDiskGetPartitionEntry;
    MachVtbl.DiskGetDriveGeometry = PpcDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = PpcDiskGetCacheableBlockCount;

    MachVtbl.RTCGetCurrentDateTime = PpcRTCGetCurrentDateTime;

    MachVtbl.HwDetect = PpcHwDetect;

    printf( "FreeLDR version [%s]\n", GetFreeLoaderVersionString() );
    BootMain("freeldr-ppc");    
}

void MachInit(char *CmdLine) {
    int len;
    printf( "Determining boot device:\n" );
    len = ofw_getprop(chosen_package, "bootpath", 
                      BootPath, sizeof(BootPath));
    printf( "Got %d bytes of path\n", len );
    BootPath[len] = 0;
    printf( "Boot Path: %s\n", BootPath );

    printf( "FreeLDR starting\n" );
}

void FrLdrSetupPageDirectory() {
}

void beep() {
}

UCHAR STDCALL READ_PORT_UCHAR(PUCHAR Address) {
    return 0xff;
}

void WRITE_PORT_UCHAR(PUCHAR Address, UCHAR Value) {
}
