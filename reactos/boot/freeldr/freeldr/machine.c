/* $Id$
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

#undef MachConsPutChar
#undef MachConsKbHit
#undef MachConsGetCh
#undef MachVideoClearScreen
#undef MachVideoSetDisplayMode
#undef MachVideoGetDisplaySize
#undef MachVideoGetBufferSize
#undef MachVideoSetTextCursorPosition
#undef MachVideoHideShowTextCursor
#undef MachVideoPutChar
#undef MachVideoCopyOffScreenBufferToVRAM
#undef MachVideoIsPaletteFixed
#undef MachVideoSetPaletteColor
#undef MachVideoGetPaletteColor
#undef MachVideoSync
#undef MachBeep
#undef MachPrepareForReactOS
#undef MachDiskGetBootVolume
#undef MachDiskGetSystemVolume
#undef MachDiskGetBootPath
#undef MachDiskGetBootDevice
#undef MachDiskBootingFromFloppy
#undef MachDiskNormalizeSystemPath
#undef MachDiskReadLogicalSectors
#undef MachDiskGetPartitionEntry
#undef MachDiskGetDriveGeometry
#undef MachDiskGetCacheableBlockCount
#undef MachHwDetect

MACHVTBL MachVtbl;

VOID
MachConsPutChar(int Ch)
{
  MachVtbl.ConsPutChar(Ch);
}

BOOLEAN
MachConsKbHit()
{
  return MachVtbl.ConsKbHit();
}

int
MachConsGetCh()
{
  return MachVtbl.ConsGetCh();
}

VOID
MachVideoClearScreen(UCHAR Attr)
{
  MachVtbl.VideoClearScreen(Attr);
}

VIDEODISPLAYMODE
MachVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
  return MachVtbl.VideoSetDisplayMode(DisplayMode, Init);
}

VOID
MachVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
  return MachVtbl.VideoGetDisplaySize(Width, Height, Depth);
}

ULONG
MachVideoGetBufferSize(VOID)
{
  return MachVtbl.VideoGetBufferSize();
}

VOID
MachVideoSetTextCursorPosition(ULONG X, ULONG Y)
{
  return MachVtbl.VideoSetTextCursorPosition(X, Y);
}

VOID
MachVideoHideShowTextCursor(BOOLEAN Show)
{
  MachVtbl.VideoHideShowTextCursor(Show);
}

VOID
MachVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
  MachVtbl.VideoPutChar(Ch, Attr, X, Y);
}

VOID
MachVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
  MachVtbl.VideoCopyOffScreenBufferToVRAM(Buffer);
}

BOOLEAN
MachVideoIsPaletteFixed(VOID)
{
  return MachVtbl.VideoIsPaletteFixed();
}

VOID
MachVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
  return MachVtbl.VideoSetPaletteColor(Color, Red, Green, Blue);
}

VOID
MachVideoGetPaletteColor(UCHAR Color, UCHAR *Red, UCHAR *Green, UCHAR *Blue)
{
  return MachVtbl.VideoGetPaletteColor(Color, Red, Green, Blue);
}

VOID
MachVideoSync(VOID)
{
  MachVtbl.VideoSync();
}

VOID
MachBeep(VOID)
{
  MachVtbl.Beep();
}

VOID
MachPrepareForReactOS(IN BOOLEAN Setup)
{
  MachVtbl.PrepareForReactOS(Setup);
}

typedef struct
{
    MEMORY_DESCRIPTOR m;
    ULONG Index;
    BOOLEAN GeneratedDescriptor;
} MEMORY_DESCRIPTOR_INT;
static const MEMORY_DESCRIPTOR_INT MemoryDescriptors[] =
{
#if defined (__i386__) || defined (_M_AMD64)
    { { MemoryFirmwarePermanent, 0x00, 1 }, 0, }, // realmode int vectors
    { { MemoryFirmwareTemporary, 0x01, 7 }, 1, }, // freeldr stack + cmdline
    { { MemoryLoadedProgram, 0x08, 0x70 }, 2, }, // freeldr image (roughly max. 0x64 pages)
    { { MemorySpecialMemory, 0x78, 8 }, 3, }, // prot mode stack. BIOSCALLBUFFER
    { { MemoryFirmwareTemporary, 0x80, 0x10 }, 4, }, // File system read buffer. FILESYSBUFFER
    { { MemoryFirmwareTemporary, 0x90, 0x10 }, 5, }, // Disk read buffer for int 13h. DISKREADBUFFER
    { { MemoryFirmwarePermanent, 0xA0, 0x60 }, 6, }, // ROM / Video
    { { MemorySpecialMemory, 0xFFF, 1 }, 7, }, // unusable memory
#elif __arm__ // This needs to be done per-platform specific way
    { { MemoryLoadedProgram, 0x80000, 32 }, 0, }, // X-Loader + OmapLdr
    { { MemoryLoadedProgram, 0x81000, 128 }, 1, }, // FreeLDR
    { { MemoryFirmwareTemporary, 0x80500, 4096 }, 2, }, // Video Buffer
#endif
};
MEMORY_DESCRIPTOR*
ArcGetMemoryDescriptor(MEMORY_DESCRIPTOR* Current)
{
    MEMORY_DESCRIPTOR_INT* CurrentDescriptor;
    BIOS_MEMORY_MAP BiosMemoryMap[32];
    static ULONG BiosMemoryMapEntryCount;
    static MEMORY_DESCRIPTOR_INT BiosMemoryDescriptors[32];
    static BOOLEAN MemoryMapInitialized = FALSE;
    ULONG i, j;

    //
    // Does machine provide an override for this function?
    //
    if (MachVtbl.GetMemoryDescriptor)
    {
        //
        // Yes. Use it instead
        //
        return MachVtbl.GetMemoryDescriptor(Current);
    }

    //
    // Check if it is the first time we're called
    //
    if (!MemoryMapInitialized)
    {
        //
        // Get the machine generated memory map
        //
        RtlZeroMemory(BiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * 32);
        BiosMemoryMapEntryCount = MachVtbl.GetMemoryMap(BiosMemoryMap,
            sizeof(BiosMemoryMap) / sizeof(BIOS_MEMORY_MAP));

        //
        // Copy the entries to our structure
        //
        for (i = 0, j = 0; i < BiosMemoryMapEntryCount; i++)
        {
            //
            // Is it suitable memory?
            //
            if (BiosMemoryMap[i].Type != BiosMemoryUsable)
            {
                //
                // No. Process next descriptor
                //
                continue;
            }

            //
            // Copy this memory descriptor
            //
            BiosMemoryDescriptors[j].m.MemoryType = MemoryFree;
            BiosMemoryDescriptors[j].m.BasePage = BiosMemoryMap[i].BaseAddress / MM_PAGE_SIZE;
            BiosMemoryDescriptors[j].m.PageCount = BiosMemoryMap[i].Length / MM_PAGE_SIZE;
            BiosMemoryDescriptors[j].Index = j;
            BiosMemoryDescriptors[j].GeneratedDescriptor = TRUE;
            j++;
        }

        //
        // Remember how much descriptors we found
        //
        BiosMemoryMapEntryCount = j;

        //
        // Mark memory map as already retrieved and initialized
        //
        MemoryMapInitialized = TRUE;
    }

    CurrentDescriptor = CONTAINING_RECORD(Current, MEMORY_DESCRIPTOR_INT, m);

    if (Current == NULL)
    {
        //
        // First descriptor requested
        //
        if (BiosMemoryMapEntryCount > 0)
        {
            //
            // Return first generated memory descriptor
            //
            return &BiosMemoryDescriptors[0].m;
        }
        else if (sizeof(MemoryDescriptors) > 0)
        {
            //
            // Return first fixed memory descriptor
            //
            return (MEMORY_DESCRIPTOR*)&MemoryDescriptors[0].m;
        }
        else
        {
            //
            // Strange case, we have no memory descriptor
            //
            return NULL;
        }
    }
    else if (CurrentDescriptor->GeneratedDescriptor)
    {
        //
        // Current entry is a generated descriptor
        //
        if (CurrentDescriptor->Index + 1 < BiosMemoryMapEntryCount)
        {
            //
            // Return next generated descriptor
            //
            return &BiosMemoryDescriptors[CurrentDescriptor->Index + 1].m;
        }
        else if (sizeof(MemoryDescriptors) > 0)
        {
            //
            // Return first fixed memory descriptor
            //
            return (MEMORY_DESCRIPTOR*)&MemoryDescriptors[0].m;
        }
        else
        {
            //
            // No fixed memory descriptor; end of memory map
            //
            return NULL;
        }
    }
    else
    {
        //
        // Current entry is a fixed descriptor
        //
        if (CurrentDescriptor->Index + 1 < sizeof(MemoryDescriptors) / sizeof(MemoryDescriptors[0]))
        {
            //
            // Return next fixed descriptor
            //
            return (MEMORY_DESCRIPTOR*)&MemoryDescriptors[CurrentDescriptor->Index + 1].m;
        }
        else
        {
            //
            // No more fixed memory descriptor; end of memory map
            //
            return NULL;
        }
    }
}

BOOLEAN
MachDiskGetBootVolume(PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType)
{
  return MachVtbl.DiskGetBootVolume(DriveNumber, StartSector, SectorCount, FsType);
}

BOOLEAN
MachDiskGetSystemVolume(char *SystemPath,
                        char *RemainingPath,
                        PULONG Device,
                        PULONG DriveNumber,
                        PULONGLONG StartSector,
                        PULONGLONG SectorCount,
                        int *FsType)
{
  return MachVtbl.DiskGetSystemVolume(SystemPath, RemainingPath, Device,
                                      DriveNumber, StartSector, SectorCount,
                                      FsType);
}

BOOLEAN
MachDiskGetBootPath(char *BootPath, unsigned Size)
{
  return MachVtbl.DiskGetBootPath(BootPath, Size);
}

VOID
MachDiskGetBootDevice(PULONG BootDevice)
{
  MachVtbl.DiskGetBootDevice(BootDevice);
}

BOOLEAN
MachDiskBootingFromFloppy()
{
  return MachVtbl.DiskBootingFromFloppy();
}

BOOLEAN
MachDiskNormalizeSystemPath(char *SystemPath, unsigned Size)
{
  return MachVtbl.DiskNormalizeSystemPath(SystemPath, Size);
}

BOOLEAN
MachDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
  return MachVtbl.DiskReadLogicalSectors(DriveNumber, SectorNumber, SectorCount, Buffer);
}

BOOLEAN
MachDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
  return MachVtbl.DiskGetPartitionEntry(DriveNumber, PartitionNumber, PartitionTableEntry);
}

BOOLEAN
MachDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry)
{
  return MachVtbl.DiskGetDriveGeometry(DriveNumber, DriveGeometry);
}

ULONG
MachDiskGetCacheableBlockCount(ULONG DriveNumber)
{
  return MachVtbl.DiskGetCacheableBlockCount(DriveNumber);
}

TIMEINFO*
ArcGetTime(VOID)
{
    return MachVtbl.GetTime();
}

ULONG
ArcGetRelativeTime(VOID)
{
    TIMEINFO* TimeInfo;
    ULONG ret;

    if (MachVtbl.GetRelativeTime)
        return MachVtbl.GetRelativeTime();

    TimeInfo = ArcGetTime();
    ret = ((TimeInfo->Hour * 24) + TimeInfo->Minute) * 60 + TimeInfo->Second;
    return ret;
}

VOID
MachHwDetect(VOID)
{
  MachVtbl.HwDetect();
}

/* EOF */
