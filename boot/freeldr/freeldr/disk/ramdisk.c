/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/i386/ramdisk.c
 * PURPOSE:         Implements routines to support booting from a RAM Disk
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PVOID gRamDiskBase;
ULONG gRamDiskSize;
extern BOOLEAN gCacheEnabled;

/* FUNCTIONS ******************************************************************/

FORCEINLINE
PVOID
RamDiskGetDataAtOffset(IN PVOID Offset)
{
    //
    // Return data from our RAM Disk
    //
    ASSERT(((ULONG_PTR)gRamDiskBase + (ULONG_PTR)Offset) <
           ((ULONG_PTR)gRamDiskBase + (ULONG_PTR)gRamDiskSize));
    return (PVOID)((ULONG_PTR)gRamDiskBase + (ULONG_PTR)(Offset));
}

ULONG
RamDiskGetCacheableBlockCount(IN ULONG Reserved)
{
    //
    // Allow 32KB transfers (64 sectors), emulating BIOS LBA
    //
    ASSERT(Reserved == 0x49);
    return 64;
}

BOOLEAN
RamDiskGetDriveGeometry(IN ULONG Reserved,
                        OUT PGEOMETRY Geometry)
{
    //
    // Should never be called when the caller expects valid Geometry!
    //
    ASSERT(Reserved == 0x49);
    return TRUE;
}

BOOLEAN
RamDiskReadLogicalSectors(IN ULONG Reserved,
                          IN ULONGLONG SectorNumber,
                          IN ULONG SectorCount,
                          IN PVOID Buffer)
{
    PVOID StartAddress;
    ULONG Length;
    ASSERT(Reserved == 0x49);

    //
    // Get actual pointers and lengths
    //
    StartAddress = (PVOID)((ULONG_PTR)SectorNumber * 512);
    Length = SectorCount * 512;
    
    //
    // Don't allow reads past our image
    //
    if (((ULONG_PTR)StartAddress + Length) > gRamDiskSize) return FALSE;

    //
    // Do the read
    //
    RtlCopyMemory(Buffer, RamDiskGetDataAtOffset(StartAddress), Length);
    return TRUE;
}

VOID
NTAPI
RamDiskLoadVirtualFile(IN PCHAR FileName)
{
    PFILE RamFile;
    ULONG TotalRead, ChunkSize;
	PCHAR MsgBuffer = "Loading ramdisk...";
    ULONG PercentPerChunk, Percent;

    //
    // Display progress
    //
    UiDrawProgressBarCenter(1, 100, MsgBuffer);

    //
    // Try opening the ramdisk file (this assumes the boot volume was opened)
    //    
    RamFile = FsOpenFile(FileName);
    if (RamFile)
    {
        //
        // Get the file size
        //
        gRamDiskSize = FsGetFileSize(RamFile);
        if (!gRamDiskSize) return;
        
        //
        // Allocate memory for it
        //
        ChunkSize = 8 * 1024 * 1024;
        Percent = PercentPerChunk = 100 / (gRamDiskSize / ChunkSize);
        gRamDiskBase = MmAllocateMemory(gRamDiskSize);
        if (!gRamDiskBase) return;
                
        //
        // Read it in chunks
        //
        for (TotalRead = 0; TotalRead < gRamDiskSize; TotalRead += ChunkSize)
        {
            //
            // Check if we're at the last chunk
            //
            if ((gRamDiskSize - TotalRead) < ChunkSize)
            {
                //
                // Only need the actual data required
                //
                ChunkSize = gRamDiskSize - TotalRead;
            }

            //
            // Draw progress
            //
            UiDrawProgressBarCenter(Percent, 100, MsgBuffer);
            Percent += PercentPerChunk;

            //
            // Copy the contents
            //
            
            if (!FsReadFile(RamFile,
                            ChunkSize,
                            NULL,
                            (PVOID)((ULONG_PTR)gRamDiskBase + TotalRead)))
            {
                //
                // Fail
                //
                UiMessageBox("Failed to read ramdisk\n");
            }
        }
    }
}

VOID
NTAPI
RamDiskSwitchFromBios(VOID)
{
    extern ULONG BootDrive, BootPartition;

    //
    // Check if we have a ramdisk, in which case we need to switch routines
    //
    if (gRamDiskBase)
    {
        //
        // Don't use the BIOS for reads anymore
        //
        MachVtbl.DiskReadLogicalSectors = RamDiskReadLogicalSectors;
        MachVtbl.DiskGetDriveGeometry = RamDiskGetDriveGeometry;
        MachVtbl.DiskGetCacheableBlockCount = RamDiskGetCacheableBlockCount;
        
        //
        // Also disable cached FAT reads
        //
        gCacheEnabled = FALSE;
        
        //
        // Switch to ramdisk boot partition
        //
        BootDrive = 0x49;
        BootPartition = 0;
    }
}
