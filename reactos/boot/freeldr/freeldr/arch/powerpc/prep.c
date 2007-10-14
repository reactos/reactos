#include "freeldr.h"
#include "machine.h"
#include "ppcmmu/mmu.h"
#include "prep.h"

int prep_serial = 0x800003f8;

void sync() { __asm__("eieio\n\tsync"); }

/* Simple serial */

void PpcPrepPutChar( int ch ) {
    if( ch == 0x0a ) {
	SetPhysByte(prep_serial, 0x0d);
	sync();
    }
    SetPhysByte(prep_serial, ch);
    sync();
}

BOOLEAN PpcPrepDiskReadLogicalSectors
( ULONG DriveNumber, ULONGLONG SectorNumber,
  ULONG SectorCount, PVOID Buffer ) {
    int secct;
    
    for(secct = 0; secct < SectorCount; secct++)
    {
	ide_seek(&ide1_desc, SectorNumber + secct, 0);
	ide_read(&ide1_desc, ((PCHAR)Buffer) + secct * 512, 512);
    }
    /* Never give up! */
    return TRUE;
}

BOOLEAN PpcPrepConsKbHit()
{
    return 1;
    //return GetPhysByte(prep_serial+5) & 1;
}

int PpcPrepConsGetCh() 
{
    while(!PpcPrepConsKbHit());
    return GetPhysByte(prep_serial);
}

void PpcPrepVideoClearScreen(UCHAR Attr)
{
    printf("\033c");
}

VIDEODISPLAYMODE PpcPrepVideoSetDisplayMode( char *DisplayMode, BOOLEAN Init )
{
    return VideoTextMode;
}

void PpcPrepVideoGetDisplaySize( PULONG Width, PULONG Height, PULONG Depth )
{
    *Width = 80;
    *Height = 25;
    *Depth = 16;
}

void PpcPrepVideoPrepareForReactOS(BOOLEAN setup)
{
    pci_setup(&pci1_desc);
}

VOID PpcInitializeMmu(int max);

ULONG PpcPrepGetMemoryMap( PBIOS_MEMORY_MAP BiosMemoryMap,
			   ULONG MaxMemoryMapSize )
{
    // xxx fixme
    BiosMemoryMap[0].Type = 1;
    BiosMemoryMap[0].BaseAddress = 0xe80000;
    BiosMemoryMap[0].Length = (64 * 1024 * 1024) - BiosMemoryMap[0].BaseAddress;
    PpcInitializeMmu(BiosMemoryMap[0].BaseAddress + BiosMemoryMap[0].Length);
    return 1;
}

void PpcPrepInit()
{
    MachVtbl.ConsPutChar = PpcPrepPutChar;

    printf("Serial on\n");

    ide_setup( &ide1_desc );

    MachVtbl.DiskReadLogicalSectors = PpcPrepDiskReadLogicalSectors;

    MachVtbl.ConsKbHit   = PpcPrepConsKbHit;
    MachVtbl.ConsGetCh   = PpcPrepConsGetCh;

    MachVtbl.VideoClearScreen = PpcPrepVideoClearScreen;
    MachVtbl.VideoSetDisplayMode = PpcPrepVideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = PpcPrepVideoGetDisplaySize;

    MachVtbl.VideoPrepareForReactOS = PpcPrepVideoPrepareForReactOS;

    MachVtbl.GetMemoryMap = PpcPrepGetMemoryMap;

    printf( "FreeLDR version [%s]\n", GetFreeLoaderVersionString() );

    BootMain( "" );
}

