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
#include "prep.h"
#include "compat.h"

extern void BootMain( LPSTR CmdLine );
extern PCHAR GetFreeLoaderVersionString();
extern ULONG CacheSizeLimit;
of_proxy ofproxy;
void *PageDirectoryStart, *PageDirectoryEnd;
static int chosen_package, stdin_handle, stdout_handle, part_handle = -1;
int mmu_handle = 0;
int claimed[4];
BOOLEAN AcpiPresent = FALSE;
char BootPath[0x100] = { 0 }, BootPart[0x100] = { 0 }, CmdLine[0x100] = { "bootprep" };
jmp_buf jmp;
volatile char *video_mem = 0;

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

int mmu_initialized = 0;
int mem_range_end;
VOID PpcInitializeMmu()
{
    if(!mmu_initialized)
    {
	MmuInit();
	MmuDbgInit(0, 0x800003f8);
        MmuSetMemorySize(mem_range_end);
        //MmuDbgEnter(0x20);
	mmu_initialized = 1;
    }
}

ULONG PpcPrepGetMemoryMap( PBIOS_MEMORY_MAP BiosMemoryMap,
                           ULONG MaxMemoryMapSize );

/*
 * Get memory the proper openfirmware way
 */
ULONG PpcGetMemoryMap( PBIOS_MEMORY_MAP BiosMemoryMap,
                       ULONG MaxMemoryMapSize ) {
    int i, memhandle, total = 0, slots = 0, last = 0x40000, allocstart = 0x1000000;
    int regdata[0x40];

    printf("PpcGetMemoryMap(%d)\n", MaxMemoryMapSize);

    memhandle = ofw_finddevice("/memory");

    ofw_getprop(memhandle, "reg", (char *)regdata, sizeof(regdata));

    /* Try to claim some memory in usable blocks.  Try to get some 8mb bits */
    for( i = 0; i < sizeof(claimed) / sizeof(claimed[0]); ) {
        if (!claimed[i])
            claimed[i] = ofw_claim(allocstart, 8 * 1024 * 1024, 0x1000);

        allocstart += 8 * 1024 * 1024;

        if (claimed[i]) {
            if (last < claimed[i]) {
                BiosMemoryMap[slots].Type = BiosMemoryAcpiReclaim;
                BiosMemoryMap[slots].BaseAddress = last;
                BiosMemoryMap[slots].Length = claimed[i] - last;
                slots++;
            }
            
            BiosMemoryMap[slots].Type = BiosMemoryUsable;
            BiosMemoryMap[slots].BaseAddress = claimed[i];
            BiosMemoryMap[slots].Length = 8 * 1024 * 1024;
            
            total += BiosMemoryMap[slots].Length;
            last = 
                BiosMemoryMap[slots].BaseAddress + 
                BiosMemoryMap[slots].Length;
            slots++;
            i++;
        }
    }

    /* Get the rest until the end of the memory object as we see it */
    if (last < regdata[1]) {
        BiosMemoryMap[slots].Type = BiosMemoryAcpiReclaim;
        BiosMemoryMap[slots].BaseAddress = last;
        BiosMemoryMap[slots].Length = regdata[1] - last;
        slots++;
    }

    for (i = 0; i < slots; i++) {
        printf("MemoryMap[%d] = (%x:%x)\n",
               i,
               (int)BiosMemoryMap[i].BaseAddress,
               (int)BiosMemoryMap[i].Length);
            
    }

    mem_range_end = regdata[1];

    printf( "Returning memory map (%d entries, %dk free, %dk total ram)\n", 
            slots, total / 1024, regdata[1] / 1024 );

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
    // Hack to be a bit easier on ram
    CacheSizeLimit = 64 * 1024;
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

VOID NarrowToWide(WCHAR *wide_name, char *name)
{
    char *copy_name;
    WCHAR *wide_name_ptr;
    for (wide_name_ptr = wide_name, copy_name = name;
         (*wide_name_ptr = *copy_name);
         wide_name_ptr++, copy_name++);
}

/* Recursively copy the device tree into our representation
 * It'll be passed to HAL.
 * 
 * When NT was first done on PPC, it was on PReP hardware, which is very 
 * like PC hardware (really, just a PPC on a PC motherboard).  HAL can guess
 * the addresses of needed resources in this scheme as it can on x86.  
 *
 * Most PPC hardware doesn't assign fixed addresses to hardware, which is
 * the problem that open firmware partially solves.  It allows hardware makers
 * much more leeway in building PPC systems.  Unfortunately, because
 * openfirmware as originally specified neither captures nor standardizes
 * all possible information, and also because of bugs, most OSs use a hybrid
 * configuration scheme that relies both on verification of devices and
 * recording information from openfirmware to be treated as hints.
 */
VOID OfwCopyDeviceTree
(PCONFIGURATION_COMPONENT_DATA ParentKey, 
 char *name, 
 int innode,
 ULONG *BusNumber,
 ULONG *DiskController,
 ULONG *DiskNumber)
{
    int proplen = 0, node = innode;
    char *prev_name, cur_name[64], data[256], *slash, devtype[64];
    wchar_t wide_name[64];
    PCONFIGURATION_COMPONENT_DATA NewKey;

    NarrowToWide(wide_name, name);

    /* Create a key for this device */
    FldrCreateComponentKey
        (ParentKey,
         wide_name,
         0,
         AdapterClass,
         MultiFunctionAdapter,
         &NewKey);

    FldrSetComponentInformation(NewKey, 0, 0, (ULONG)-1);

    /* Add properties */
    for (prev_name = ""; ofw_nextprop(node, prev_name, cur_name) == 1; )
    {
        proplen = ofw_getproplen(node, cur_name);
        if (proplen > 256 || proplen < 0)
        {
            printf("Warning: not getting prop %s (too long: %d)\n", 
                   cur_name, proplen);
            continue;
        }
        ofw_getprop(node, cur_name, data, sizeof(data));

        /* Get device type so we can examine it */
        if (!strcmp(cur_name, "device_type"))
            strcpy(devtype, (char *)data);
        
        NarrowToWide(wide_name, cur_name);
        //RegSetValue(NewKey, wide_name, REG_BINARY, data, proplen);

        strcpy(data, cur_name);
        prev_name = data;
    }

#if 0
    /* Special device handling */
    if (!strcmp(devtype, "ata"))
    {
        OfwHandleDiskController(NewKey, node, *DiskController);
        (*DiskController)++;
        *DiskNumber = 0;
    }
    else if (!strcmp(devtype, "disk"))
    {
        OfwHandleDiskObject(NewKey, node, *DiskController, *DiskNumber);
        (*DiskNumber)++;
    }
#endif

    /* Subdevices */
    for (node = ofw_child(node); node; node = ofw_peer(node))
    {
        ofw_package_to_path(node, data, sizeof(data));
        slash = strrchr(data, '/');
        if (slash) slash++; else continue;
        OfwCopyDeviceTree
            (NewKey, slash, node, BusNumber, DiskController, DiskNumber);
    }
}

PCONFIGURATION_COMPONENT_DATA PpcHwDetect() {
    PCONFIGURATION_COMPONENT_DATA RootKey;
    ULONG BusNumber = 0, DiskController = 0, DiskNumber = 0;
    int node = ofw_finddevice("/");

    FldrCreateSystemKey(&RootKey);

    FldrSetComponentInformation(RootKey, 0, 0, (ULONG)-1);

    OfwCopyDeviceTree(RootKey,"/",node,&BusNumber,&DiskController,&DiskNumber);
    return RootKey;
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

/* Compatibility functions that don't do much */
VOID PpcVideoPrepareForReactOS(BOOLEAN Setup) {
}

void PpcDefaultMachVtbl()
{
    MachVtbl.ConsPutChar = PpcOfwPutChar;
    MachVtbl.ConsKbHit   = PpcConsKbHit;
    MachVtbl.ConsGetCh   = PpcConsGetCh;
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
}

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

    // Allow forcing prep for broken OFW
    if(!strncmp(CmdLine, "bootprep", 8))
    {
	printf("Going to PREP init...\n");
        ofproxy = NULL;
	PpcPrepInit();
	return;
    }

    printf( "FreeLDR version [%s]\n", GetFreeLoaderVersionString() );

    BootMain( CmdLine );
}

void PpcInit( of_proxy the_ofproxy ) {
    ofproxy = the_ofproxy;
    PpcDefaultMachVtbl();
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
    __asm__("twi 31,0,0");
}
