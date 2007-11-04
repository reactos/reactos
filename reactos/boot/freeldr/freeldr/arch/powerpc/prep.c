#include "freeldr.h"
#include "machine.h"
#include "ppcboot.h"
#include "ppcmmu/mmu.h"
#include "prep.h"

extern boot_infos_t BootInfo;
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
    /* Prep boxen are PCI */
    BootInfo.machineType = PCIBus;
    pci_setup(&pci1_desc);
}

VOID PpcInitializeMmu(int max);

ULONG PpcPrepGetMemoryMap( PBIOS_MEMORY_MAP BiosMemoryMap,
			   ULONG MaxMemoryMapSize )
{
    // Probe memory
    paddr_t physAddr;
    register int oldStore = 0, newStore = 0, change = 0, oldmsr;

    __asm__("mfmsr %0\n" : "=r" (oldmsr));
    change = oldmsr & 0x6fff;
    __asm__("mtmsr %0\n" : : "r" (change));

    // Find the last ram address in physical space ... this bypasses mapping
    // but could run into non-ram objects right above ram.  Usually systems
    // aren't designed like that though.
    for (physAddr = 0x30000, change = newStore; 
         (physAddr < 0x80000000) && (change == newStore); 
         physAddr += 1 << 12)
    {
        oldStore = GetPhys(physAddr);
        newStore = (physAddr & 0x1000) ? 0x55aa55aa : 0xaa55aa55;
        SetPhys(physAddr, newStore);
        change = GetPhys(physAddr);
        SetPhys(physAddr, oldStore);
    }
    // Back off by one page
    physAddr -= 0x1000;
    BiosMemoryMap[0].BaseAddress = 0x30000; // End of ppcmmu
    BiosMemoryMap[0].Type = BiosMemoryUsable;
    BiosMemoryMap[0].Length = physAddr - BiosMemoryMap[0].BaseAddress;

    __asm__("mtmsr %0\n" : : "r" (oldmsr));

    printf("Actual RAM: %d Mb\n", physAddr >> 20);
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

