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

/* Most PReP hardware is in standard locations, based on the corresponding 
 * hardware on PCs. */
VOID PpcPrepHwDetect() {
    PPC_DEVICE_TREE tree;
    PPC_DEVICE_RANGE range;
    int interrupt;

    /* Start the tree */
    if(!PpcDevTreeInitialize
       (&tree,
        PAGE_SIZE, sizeof(long long), 
        (PPC_DEVICE_ALLOC)MmAllocateMemory, 
        (PPC_DEVICE_FREE)MmFreeMemory))
        return;

    /* PCI Bus */
    PpcDevTreeAddDevice(&tree, PPC_DEVICE_PCI_EAGLE, "pci");

    /* Check out the devices on the bus */
    pci_setup(&tree, &pci1_desc);
    
    /* End PCI Bus */
    PpcDevTreeCloseDevice(&tree);

    /* ISA Bus */
    PpcDevTreeAddDevice(&tree, PPC_DEVICE_ISA_BUS, "isa");

    /* Serial port */
    PpcDevTreeAddDevice(&tree, PPC_DEVICE_SERIAL_8250, "com1");
    range.start = (PVOID)0x800003f8;
    range.len = 8;
    range.type = PPC_DEVICE_IO_RANGE;
    interrupt = 4;
    PpcDevTreeAddProperty
        (&tree, PPC_DEVICE_SPACE_RANGE, "reg", (char *)&range, sizeof(range));
    PpcDevTreeAddProperty
        (&tree, PPC_DEVICE_INTERRUPT, "interrupt", 
         (char *)&interrupt, sizeof(interrupt));
    PpcDevTreeCloseDevice(&tree);

    /* We probably have an ISA IDE controller */
    PpcDevTreeAddDevice(&tree, PPC_DEVICE_IDE_DISK, "ide0");
    range.start = (PVOID)0x800001f8;
    range.len = 8;
    range.type = PPC_DEVICE_IO_RANGE;
    interrupt = 14;
    PpcDevTreeAddProperty
        (&tree, PPC_DEVICE_SPACE_RANGE, "reg", (char *)&range, sizeof(range));
    PpcDevTreeAddProperty
        (&tree, PPC_DEVICE_INTERRUPT, "interrupt", 
         (char *)&interrupt, sizeof(interrupt));
    PpcDevTreeCloseDevice(&tree);

    /* Describe VGA */
    PpcDevTreeAddDevice(&tree, PPC_DEVICE_VGA, "vga");
    range.start = (PVOID)0x800003c0;
    range.len = 0x20;
    range.type = PPC_DEVICE_IO_RANGE;
    PpcDevTreeAddProperty
        (&tree, PPC_DEVICE_SPACE_RANGE, "reg", (char *)&range, sizeof(range));
    range.start = BootInfo.dispDeviceBase;
    range.len = BootInfo.dispDeviceRowBytes * BootInfo.dispDeviceRect[3];
    range.type = PPC_DEVICE_MEM_RANGE;
    PpcDevTreeAddProperty
        (&tree, PPC_DEVICE_SPACE_RANGE, "mem", (char *)&range, sizeof(range));
    PpcDevTreeCloseDevice(&tree);

    /* End ISA Bus */
    PpcDevTreeCloseDevice(&tree);

    /* And finish by closing the root node */
    PpcDevTreeCloseDevice(&tree);

    /* Now fish out the root node.  The dev tree is a slab of memory */
    BootInfo.machine = PpcDevTreeGetRootNode(&tree);
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
    MachVtbl.HwDetect = PpcPrepHwDetect;

    printf( "FreeLDR version [%s]\n", GetFreeLoaderVersionString() );

    BootMain( "" );
}

