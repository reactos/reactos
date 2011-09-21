/*
 * PROJECT:         ReactOS Boot Loader (FreeLDR)
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/arcemul/mm.c
 * PURPOSE:         Routines for ARC Memory Management
 * PROGRAMMERS:     Hervé Poussineau  <hpoussin@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

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

#endif
};

static
VOID MmFixupSystemMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG* MapCount)
{
	int		Index;
	int		Index2;
	ULONGLONG BaseAddressOffset;

	// Loop through each entry in the array
	for (Index=0; Index<*MapCount; Index++)
	{
		// Correct all the addresses to be aligned on page boundaries
		BaseAddressOffset = ROUND_UP(BiosMemoryMap[Index].BaseAddress, MM_PAGE_SIZE) - BiosMemoryMap[Index].BaseAddress;
		BiosMemoryMap[Index].BaseAddress += BaseAddressOffset;
		if (BiosMemoryMap[Index].Length < BaseAddressOffset)
		{
			BiosMemoryMap[Index].Length = 0;
		}
		else
		{
			BiosMemoryMap[Index].Length -= BaseAddressOffset;
		}
		BiosMemoryMap[Index].Length = ROUND_DOWN(BiosMemoryMap[Index].Length, MM_PAGE_SIZE);

		// If the entry type isn't usable then remove
		// it from the memory map (this will help reduce
		// the size of our lookup table)
		// If the length is less than a full page then
		// get rid of it also.
		if (BiosMemoryMap[Index].Type != BiosMemoryUsable ||
			BiosMemoryMap[Index].Length < MM_PAGE_SIZE)
		{
			// Slide every entry after this down one
			for (Index2=Index; Index2<(*MapCount - 1); Index2++)
			{
				BiosMemoryMap[Index2] = BiosMemoryMap[Index2 + 1];
			}
			(*MapCount)--;
			Index--;
		}
	}
}

const MEMORY_DESCRIPTOR*
ArcGetMemoryDescriptor(const MEMORY_DESCRIPTOR* Current)
{
    MEMORY_DESCRIPTOR_INT* CurrentDescriptor;
    BIOS_MEMORY_MAP BiosMemoryMap[32];
    static ULONG BiosMemoryMapEntryCount;
    static MEMORY_DESCRIPTOR_INT BiosMemoryDescriptors[32];
    static BOOLEAN MemoryMapInitialized = FALSE;
    ULONG i, j;

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
                                                        sizeof(BiosMemoryMap) /
                                                        sizeof(BIOS_MEMORY_MAP));

        //
        // Fix entries that are not page aligned
        //
        MmFixupSystemMemoryMap(BiosMemoryMap, &BiosMemoryMapEntryCount);

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
            BiosMemoryDescriptors[j].m.BasePage = (ULONG)(BiosMemoryMap[i].BaseAddress / MM_PAGE_SIZE);
            BiosMemoryDescriptors[j].m.PageCount = (ULONG)(BiosMemoryMap[i].Length / MM_PAGE_SIZE);
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
            return &MemoryDescriptors[0].m;
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
            return &MemoryDescriptors[0].m;
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
            return &MemoryDescriptors[CurrentDescriptor->Index + 1].m;
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
