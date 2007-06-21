/* $Id: machOlpc.c 21339 2006-03-18 22:09:16Z peterw $
 *
 *  FreeLoader
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

#include <freeldr.h>

/* OFW's stdin / stdout */
FILE *stdin_handle;
FILE *stdout_handle;

int chosen_package;
static int part_handle = -1, kernel_mem = 0;

char BootPath[0x100] = { 0 }, BootPart[0x100] = { 0 }, CmdLine[0x100] = { "bootprep" };

int decode_int(UCHAR *p);
VOID OlpcVideoInit();

VOID
XboxRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second);

ULONG_PTR MemMin, MemMax; // OFW can report whole physical memory region

VOID
OlpcMachInit(const char *CmdLine_)
{
  int i, len;
  char *sep;
  /* Initialize our stuff */
  //OlpcMemInit();
  //OlpcVideoInit();

  chosen_package = OFFinddevice( "/chosen" );
  OFGetprop(chosen_package, "bootargs",
		CmdLine, sizeof(CmdLine));

  CmdLineParse(CmdLine);

  /* Initialize the framebuffer */
  OlpcVideoInit();

  /* Setup vtbl */
  MachVtbl.ConsPutChar = OlpcConsPutChar;
  MachVtbl.ConsKbHit = OlpcConsKbHit;
  MachVtbl.ConsGetCh = OlpcConsGetCh;
  MachVtbl.VideoClearScreen = OlpcVideoClearScreen;
  MachVtbl.VideoSetDisplayMode = OlpcVideoSetDisplayMode;
  MachVtbl.VideoGetDisplaySize = OlpcVideoGetDisplaySize;
  MachVtbl.VideoGetBufferSize = OlpcVideoGetBufferSize;
  MachVtbl.VideoHideShowTextCursor = OlpcVideoHideShowTextCursor;
  MachVtbl.VideoPutChar = OlpcVideoPutChar;
  MachVtbl.VideoCopyOffScreenBufferToVRAM = OlpcVideoCopyOffScreenBufferToVRAM;
  MachVtbl.VideoIsPaletteFixed = OlpcVideoIsPaletteFixed;
  MachVtbl.VideoSetPaletteColor = OlpcVideoSetPaletteColor;
  MachVtbl.VideoGetPaletteColor = OlpcVideoGetPaletteColor;
  MachVtbl.VideoSync = OlpcVideoSync;
  MachVtbl.VideoPrepareForReactOS = OlpcVideoPrepareForReactOS;
  MachVtbl.GetMemoryMap = OlpcMemGetMemoryMap;
  MachVtbl.DiskGetBootVolume = OlpcDiskGetBootVolume;
  MachVtbl.DiskGetSystemVolume = OlpcDiskGetSystemVolume;
  MachVtbl.DiskGetBootPath = OlpcDiskGetBootPath;
  MachVtbl.DiskGetBootDevice = OlpcDiskGetBootDevice;
  MachVtbl.DiskBootingFromFloppy = OlpcDiskBootingFromFloppy;
  MachVtbl.DiskNormalizeSystemPath = OlpcDiskNormalizeSystemPath;
  MachVtbl.DiskReadLogicalSectors = OlpcDiskReadLogicalSectors;
  MachVtbl.DiskGetPartitionEntry = OlpcDiskGetPartitionEntry;
  MachVtbl.DiskGetDriveGeometry = OlpcDiskGetDriveGeometry;
  MachVtbl.DiskGetCacheableBlockCount = OlpcDiskGetCacheableBlockCount;
  MachVtbl.RTCGetCurrentDateTime = XboxRTCGetCurrentDateTime;
  MachVtbl.HwDetect = OlpcHwDetect;

  /* Determine boot device */
    BootPart[0] = 0;
    BootPath[0] = 0;

    ofwprintf( "Determining boot device: [%s]\n", CmdLine );

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

  if( strlen(BootPart) == 0 )
  {
	len = OFGetprop(chosen_package, "bootpath", 
			  BootPath, sizeof(BootPath));
	
	if( len < 0 ) len = 0;
	BootPath[len] = 0;
	ofwprintf( "Boot Path: %s\n", BootPath );
	
	sep = strrchr(BootPath, ',') + 9; // HACK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	strcpy(BootPart, BootPath);
	if( sep ) {
	    BootPart[sep - BootPath] = 0;
	}
  }

  ofwprintf( "FreeLDR starting (boot partition: %s)\n", BootPart );

}

void OlpcConsPutChar( int ch )
{
    char buf[3];
    if( ch == 0x0a ) { buf[0] = 0x0d; buf[1] = 0x0a; } 
    else { buf[0] = ch; buf[1] = 0; }
    buf[2] = 0;
    OFWrite(stdout_handle, buf, strlen(buf));
}

BOOLEAN OlpcConsKbHit() {
    return FALSE;
}

int OlpcConsGetCh()
{
    char buf;
ofwprintf("OlpcConsGetCh\n");
	OFRead(stdin_handle, &buf, 1);
    return buf;
}

ULONG OlpcMemGetMemoryMap( PBIOS_MEMORY_MAP BiosMemoryMap,
                       ULONG MaxMemoryMapSize )
{
    int i, memhandle, returned, total = 0, slots = 0;
    int memdata[0x40];

    ofwprintf("OlpcGetMemoryMap(%d)\n", MaxMemoryMapSize);

    memhandle = OFFinddevice("/memory");

	/* Get Max/Min memory boundaries */
    returned = OFGetprop(memhandle, "reg", 
			   (char *)memdata, sizeof(memdata));

	ofwprintf("Returned 'reg' data: %d\n", returned);
	if( returned == -1 )
	{
		ofwprintf("getprop /memory[@reg] failed\n");
		return 0;
	}
	MemMin = decode_int(&memdata[0]);
	MemMax = MemMin + decode_int(&memdata[1]);
	ofwprintf("Memory start: %x, memory end: %x\n", MemMin, MemMax);

	/* Get unclaimed regions */
    returned = OFGetprop(memhandle, "available", 
			   (char *)memdata, sizeof(memdata));

    ofwprintf("Returned 'available' data: %d\n", returned);
    if( returned == -1 ) {
	ofwprintf("getprop /memory[@reg] failed\n");
	return 0;
    }

    for( i = 0; i < returned; i++ ) {
	ofwprintf("%x ", memdata[i]);
    }
    ofwprintf("\n");

	for( i = 0; i < returned / 2; i++ )
	{
		if (decode_int(&memdata[i*2]) > 0x7000000)
			continue;

		BiosMemoryMap[slots].Type = 1/*MEMTYPE_USABLE*/;
		BiosMemoryMap[slots].BaseAddress = decode_int(&memdata[i*2]);
		BiosMemoryMap[slots].Length = decode_int(&memdata[i*2+1]);
		ofwprintf("MemoryMap[%d] = (%x:%x)\n", 
			i, 
			(int)BiosMemoryMap[slots].BaseAddress,
			(int)BiosMemoryMap[slots].Length);

		/* Hack for pearpc */
		if(/* kernel_mem */FALSE) {
			/*BiosMemoryMap[slots].Length = kernel_mem * 1024;
			if( !FixedMemory ) {
			OFClaim((int)BiosMemoryMap[slots].BaseAddress,
			(int)BiosMemoryMap[slots].Length,
			0x1000);
			FixedMemory = BiosMemoryMap[slots].BaseAddress;
			}
			total += BiosMemoryMap[slots].Length;
			slots++;*/
			break;
			/* Normal way */
		} else if( BiosMemoryMap[slots].Length &&
			OFClaim((int)BiosMemoryMap[slots].BaseAddress,
			(int)BiosMemoryMap[slots].Length,
			0x1000) ) {
				total += BiosMemoryMap[slots].Length;
				slots++;
		}
	}

    ofwprintf( "Returning memory map (%dk total)\n", total / 1024 );

    return slots;
}

BOOLEAN OlpcDiskReadLogicalSectors( ULONG DriveNumber, ULONGLONG SectorNumber,
				   ULONG SectorCount, PVOID Buffer )
{
	int rlen = 0;
	int result;
	//char * gethomedir();
	//char *homedir = gethomedir();


	//ofwprintf("OlpcDiskReadLogicalSectors() SN %x, SC %x\n", SectorNumber, SectorCount); //FIXME: incorrect due to SN being ULONGLONG

	if( part_handle == -1 )
	{
		part_handle = OFOpen(BootPart);

		if( part_handle == -1 )
		{
			ofwprintf("Could not open any disk devices we know about\n");
			return FALSE;
		}
	}

	//ofwprintf("Got partition handle %x\n", part_handle);

	if( part_handle == -1 )
	{
		return FALSE;
	}

	result = OFSeek( part_handle, 
		(ULONG)(SectorNumber >> 25), 
		(ULONG)((SectorNumber * 512) & 0xffffffff) );

	if (result == -1)
	{
			ofwprintf("Seek to %x failed\n", (ULONG)(SectorNumber * 512));
			return FALSE;
	}

	rlen = OFRead( part_handle, Buffer, (ULONG)(SectorCount * 512) );
	return rlen > 0;
}

BOOLEAN OlpcDiskGetPartitionEntry( ULONG DriveNumber, ULONG PartitionNumber,
                               PPARTITION_TABLE_ENTRY PartitionTableEntry )
{
    ofwprintf("GetPartitionEntry(%d,%d)\n", DriveNumber, PartitionNumber);
    return FALSE;
}

BOOLEAN OlpcDiskGetDriveGeometry( ULONG DriveNumber, PGEOMETRY DriveGeometry )
{
    //ofwprintf("GetGeometry(%d)\n", DriveNumber);
    DriveGeometry->BytesPerSector = 512;
    DriveGeometry->Heads = 16;
    DriveGeometry->Sectors = 63;
    return TRUE;
}

ULONG OlpcDiskGetCacheableBlockCount( ULONG DriveNumber )
{
    //ofwprintf("GetCacheableBlockCount\n");
    return 1;
}

VOID OlpcHwDetect()
{
    ofwprintf("OlpcHwDetect\n");
}

/* Strategy:
 *
 * For now, it'll be easy enough to use the boot command line as our boot path.
 * Treat it as the path of a disk partition.  We might even be able to get
 * away with grabbing a partition image by tftp in this scenario.
 */

BOOLEAN OlpcDiskGetBootVolume( PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType )
{
    *DriveNumber = 0;
    *StartSector = 0;
    *SectorCount = 0;
    *FsType = FS_FAT;
    return TRUE;
}

BOOLEAN OlpcDiskGetSystemVolume( char *SystemPath,
                             char *RemainingPath,
                             PULONG Device,
                             PULONG DriveNumber, 
                             PULONGLONG StartSector, 
                             PULONGLONG SectorCount, 
                             int *FsType )
{
    char *remain = strchr(SystemPath, '\\');
    if( remain ) {
	strcpy( RemainingPath, remain+1 );
    } else {
	RemainingPath[0] = 0;
    }
    *Device = 0;
    return MachDiskGetBootVolume(DriveNumber, StartSector, SectorCount, FsType);
}

BOOLEAN OlpcDiskGetBootPath( char *OutBootPath, unsigned Size )
{
    strncpy( OutBootPath, BootPath, Size );
    return TRUE;
}

VOID OlpcDiskGetBootDevice( PULONG BootDevice )
{
    BootDevice[0] = BootDevice[1] = 0;
}

BOOLEAN OlpcDiskBootingFromFloppy(VOID)
{
    return FALSE;
}

BOOLEAN OlpcDiskNormalizeSystemPath(char *SystemPath, unsigned Size)
{
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
