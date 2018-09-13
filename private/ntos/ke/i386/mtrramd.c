#include "ki.h"

#define STATIC

#define IDBG    0

#if DBG
#define DBGMSG(a)   DbgPrint(a)
#else
#define DBGMSG(a)
#endif

//
// Externals.
//

NTSTATUS
KiLoadMTRR (
    PVOID Context
    );

// --- AMD Structure definitions ---

// K6 MTRR hardware register layout.

// Single MTRR control register.

typedef struct _AMDK6_MTRR {
    ULONG       type:2;
    ULONG       mask:15;
    ULONG       base:15;
} AMDK6_MTRR, *PAMDK6_MTRR;

// MSR image, contains two control regs.

typedef struct _AMDK6_MTRR_MSR_IMAGE {
    union {
        struct {
            AMDK6_MTRR    mtrr0;
            AMDK6_MTRR    mtrr1;
        } hw;
        ULONGLONG   QuadPart;
    } u;
} AMDK6_MTRR_MSR_IMAGE, *PAMDK6_MTRR_MSR_IMAGE;

// MTRR reg type field values.

#define AMDK6_MTRR_TYPE_DISABLED    0
#define AMDK6_MTRR_TYPE_UC          1
#define AMDK6_MTRR_TYPE_WC          2
#define AMDK6_MTRR_TYPE_MASK        3

// AMD K6 MTRR MSR Index number

#define AMDK6_MTRR_MSR                0xC0000085

//
// Region table entry - used to track all write combined regions.
//
// Set BaseAddress to AMDK6_REGION_UNUSED for unused entries.
//

typedef struct _AMDK6_MTRR_REGION {
    ULONG                BaseAddress;
    ULONG                Size;
    MEMORY_CACHING_TYPE  RegionType;
    ULONG                RegionFlags;
} AMDK6_MTRR_REGION, *PAMDK6_MTRR_REGION;

#define MAX_K6_REGIONS          2		// Limit the write combined regions to 2 since that's how many MTRRs we have available.

//
// Value to set base address to for unused indication.
//

#define AMDK6_REGION_UNUSED     0xFFFFFFFF

//
// Flag to indicate that this region was set up by the BIOS.    
//

#define AMDK6_REGION_FLAGS_BIOS 0x00000001

//
// Usage count for hardware MTRR registers.
//

#define AMDK6_MAX_MTRR        2

//
// AMD Function Prototypes.
//

VOID
KiAmdK6InitializeMTRR (
    VOID
    );

NTSTATUS
KiAmdK6RestoreMTRR (
    );

NTSTATUS
KiAmdK6MtrrSetMemoryType (
    ULONG BaseAddress,
    ULONG Size,
    MEMORY_CACHING_TYPE Type
    );

BOOLEAN
KiAmdK6AddRegion (
    ULONG BaseAddress,
    ULONG Size,
    MEMORY_CACHING_TYPE Type,
    ULONG Flags
    );

NTSTATUS
KiAmdK6MtrrCommitChanges (
    VOID
    );

NTSTATUS
KiAmdK6HandleWcRegionRequest (
    ULONG BaseAddress,
    ULONG Size
    );

VOID
KiAmdK6MTRRAddRegionFromHW (
    AMDK6_MTRR RegImage
    );

PAMDK6_MTRR_REGION
KiAmdK6FindFreeRegion (
MEMORY_CACHING_TYPE Type
    );

#pragma alloc_text(INIT,KiAmdK6InitializeMTRR)
#pragma alloc_text(PAGELK,KiAmdK6RestoreMTRR)
#pragma alloc_text(PAGELK,KiAmdK6MtrrSetMemoryType)
#pragma alloc_text(PAGELK,KiAmdK6AddRegion)
#pragma alloc_text(PAGELK,KiAmdK6MtrrCommitChanges)
#pragma alloc_text(PAGELK,KiAmdK6HandleWcRegionRequest)
#pragma alloc_text(PAGELK,KiAmdK6MTRRAddRegionFromHW)
#pragma alloc_text(PAGELK,KiAmdK6FindFreeRegion)

// --- AMD Global Variables ---

extern KSPIN_LOCK KiRangeLock;

// AmdK6Regions - Table to track wc regions.

AMDK6_MTRR_REGION AmdK6Regions[MAX_K6_REGIONS];
ULONG AmdK6RegionCount;

// Usage counter for hardware MTRRs.

ULONG AmdMtrrHwUsageCount;

// Global variable image of MTRR MSR.

AMDK6_MTRR_MSR_IMAGE    KiAmdK6Mtrr;

// --- AMD Start of code ---

VOID
KiAmdK6InitializeMTRR (
    VOID
    )
{
    ULONG    i;
    KIRQL    OldIrql;

    DBGMSG("KiAmdK6InitializeMTRR: Initializing K6 MTRR support\n");

    KiAmdK6Mtrr.u.hw.mtrr0.type = AMDK6_MTRR_TYPE_DISABLED;
    KiAmdK6Mtrr.u.hw.mtrr1.type = AMDK6_MTRR_TYPE_DISABLED;
    AmdK6RegionCount = MAX_K6_REGIONS;
    AmdMtrrHwUsageCount = 0;

    //
    // Set all regions to free.
    //

    for (i = 0; i < AmdK6RegionCount; i++) {
        AmdK6Regions[i].BaseAddress = AMDK6_REGION_UNUSED;
        AmdK6Regions[i].RegionFlags = 0;
    }

    //
    // Initialize the spin lock.
    //
    // N.B. Normally this is done by KiInitializeMTRR but that
    // routine is not called in the AMD K6 case.
    //

    KeInitializeSpinLock (&KiRangeLock);

    //
    // Read the MTRR registers to see if the BIOS has set them up.
    // If so, add entries to the region table and adjust the usage
    // count.  Serialize the region table.
    //

    KeAcquireSpinLock (&KiRangeLock, &OldIrql);
                
    KiAmdK6Mtrr.u.QuadPart = RDMSR (AMDK6_MTRR_MSR);

    //
    // Check MTRR0 first.
    //

    KiAmdK6MTRRAddRegionFromHW(KiAmdK6Mtrr.u.hw.mtrr0);

    //
    // Now check MTRR1.
    //

    KiAmdK6MTRRAddRegionFromHW(KiAmdK6Mtrr.u.hw.mtrr1);

    //
    // Release the locks.
    //

    KeReleaseSpinLock (&KiRangeLock, OldIrql);
}

VOID
KiAmdK6MTRRAddRegionFromHW (
    AMDK6_MTRR RegImage
    )
{
    ULONG BaseAddress, Size, TempMask;

    //
    // Check to see if this MTRR is enabled.
    //
        
    if (RegImage.type != AMDK6_MTRR_TYPE_DISABLED) {

        //
        // If this is a write combined region then add an entry to
        // the region table.
        //

        if ((RegImage.type & AMDK6_MTRR_TYPE_UC) == 0) {

            //
            // Create a new resion table entry.
            //

            BaseAddress = RegImage.base << 17;

            //
            // Calculate the size base on the mask value.
            //

            TempMask = RegImage.mask;
            
            //
            // There should never be 4GB WC region!
            //

            ASSERT (TempMask != 0);

            //
            // Start with 128 size and search upward.
            //

            Size = 0x00020000;

            while ((TempMask & 0x00000001) == 0) {
                TempMask >>= 1;
                Size <<= 1;
            }

            //
            // Add the region to the table.
            //
            
            KiAmdK6AddRegion(BaseAddress,
                             Size,
                             MmWriteCombined,
                             AMDK6_REGION_FLAGS_BIOS);

            AmdMtrrHwUsageCount++;
        }
    }
}


NTSTATUS
KiAmdK6MtrrSetMemoryType (
    ULONG BaseAddress,
    ULONG Size,
    MEMORY_CACHING_TYPE Type
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;
    KIRQL       OldIrql;

    switch(Type) {
    case MmWriteCombined:

        //
        // H/W needs updating, lock down the code required to effect
        // the change.
        //

        if (KeGetCurrentIrql() >= DISPATCH_LEVEL) {

            //
            // Code can not be locked down.   Supplying a new range type
            // requires that the caller calls at irql < dispatch_level.
            //

            DBGMSG ("KeAmdK6SetPhysicalCacheTypeRange failed due to calling IRQL == DISPATCH_LEVEL\n");
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Lock the code.
        //

        MmLockPagableSectionByHandle(ExPageLockHandle);
        
        //
        // Serialize the region table.
        //

        KeAcquireSpinLock (&KiRangeLock, &OldIrql);

        Status = KiAmdK6HandleWcRegionRequest(BaseAddress, Size);
        
        //
        // Release the locks.
        //

        KeReleaseSpinLock (&KiRangeLock, OldIrql);
        MmUnlockPagableImageSection(ExPageLockHandle);
        
        break;  // End of WriteCombined case.

    case MmNonCached:

        //
        // Add an entry to the region table.
        //

	// Don't need to add these to the region table.  Non-cached regions are 
	// accessed using a non-caching virtual pointer set up in the page tables.

        break;

    case MmCached:

        //
        // Redundant.  These should be filtered out in
        // KeAmdK6SetPhysicalCacheTypeRange();
        //

        Status = STATUS_NOT_SUPPORTED;
        break;

    default:
        DBGMSG ("KeAmdK6SetPhysicalCacheTypeRange: no such cache type\n");
        Status = STATUS_INVALID_PARAMETER;
        break;
    }
    return Status;
}

NTSTATUS
KiAmdK6HandleWcRegionRequest (
    ULONG BaseAddress,
    ULONG Size
    )
{
    ULONG               i;
    ULONG               AdjustedSize, AdjustedEndAddress, AlignmentMask;
    ULONG               CombinedBase, CombinedSize, CombinedAdjustedSize;
    PAMDK6_MTRR_REGION  pRegion;
    BOOLEAN             bCanCombine, bValidRange;

    //
    // Try and find a region that overlaps or is adjacent to the new one and
    // check to see if the combined region would be a legal mapping.
    //

    for (i = 0; i < AmdK6RegionCount; i++) {
        pRegion = &AmdK6Regions[i];
        if ((pRegion->BaseAddress != AMDK6_REGION_UNUSED) &&
            (pRegion->RegionType == MmWriteCombined)) {

            //
            // Does the new start address overlap or adjoin an
            // existing WC region?
            //

            if (((pRegion->BaseAddress >= BaseAddress) &&
                 (pRegion->BaseAddress <= (BaseAddress + Size))) ||
                 ((BaseAddress <= (pRegion->BaseAddress + pRegion->Size)) &&
                  (BaseAddress >= pRegion->BaseAddress))) {

                //
                // Combine the two regions into one.
                //

                AdjustedEndAddress = BaseAddress + Size;

                if (pRegion->BaseAddress < BaseAddress) {
                    CombinedBase = pRegion->BaseAddress;
                } else {
                    CombinedBase = BaseAddress;
                }

                if ((pRegion->BaseAddress + pRegion->Size) >
                    AdjustedEndAddress) {
                    CombinedSize = (pRegion->BaseAddress + pRegion->Size) -
                           CombinedBase;
                } else {
                    CombinedSize = AdjustedEndAddress - CombinedBase;
                }

                //
                // See if the new region would be a legal mapping.
                //
                //
                // Find the smallest legal size that is equal to the requested range.  Scan
                // all ranges from 128k - 2G. (Start at 2G and work down).
                //
        
                CombinedAdjustedSize = 0x80000000;
                AlignmentMask = 0x7fffffff;
                bCanCombine = FALSE;
                
                while (CombinedAdjustedSize > 0x00010000) {

                    //
                    // Check the size to see if it matches the requested limit.
                    //

                    if (CombinedAdjustedSize == CombinedSize) {

                        //
                        // This one works.
                        // Check to see if the base address conforms to the MTRR restrictions.
                        //

                        if ((CombinedBase & AlignmentMask) == 0) {
                            bCanCombine = TRUE;
                        }

                        break;

                    } else {

                        //
                        // Bump it down to the next range size and try again.
                        //

                        CombinedAdjustedSize >>= 1;
                        AlignmentMask >>= 1;
                    }
                }

                if (bCanCombine) {
                    //
                    // If the resized range is OK, record the change in the region
                    // table and commit the changes to hardware.
                    //
                    
                    pRegion->BaseAddress = CombinedBase;
                    pRegion->Size = CombinedAdjustedSize;
                
                    //
                    // Reset the BIOS flag since we now "own" this region (if we didn't already).
                    //
                
                    pRegion->RegionFlags &= ~AMDK6_REGION_FLAGS_BIOS;

                    return KiAmdK6MtrrCommitChanges();
                }
            }
        }
    }

	// A valid combination could not be found, so try to create a new range for this request.
    //
    // Find the smallest legal size that is less than or equal to the requested range.  Scan
    // all ranges from 128k - 2G. (Start at 2G and work down).
    //
        
    AdjustedSize = 0x80000000;
    AlignmentMask = 0x7fffffff;
    bValidRange = FALSE;

    while (AdjustedSize > 0x00010000) {

        //
        // Check the size to see if it matches the requested limit.
        //

        if (AdjustedSize == Size) {

            //
            // This one works.
            //
            // Check to see if the base address conforms to the MTRR restrictions.
            //

            if ((BaseAddress & AlignmentMask) == 0) {
                bValidRange = TRUE;
            }
            
            //
            // Stop looking.
            //
            
            break;

        } else {

            //
            // Bump it down to the next range size and try again.
            //

            AdjustedSize >>= 1;
            AlignmentMask >>= 1;
        }
    }

    //
    // Couldn't find a legal region that fit.
    //
    
    if (!bValidRange) {
        return STATUS_NOT_SUPPORTED;
    }
    
    
    //
    // If we got this far then this is a new WC region.
    // Create a new region entry for this request.
    //

    if (!KiAmdK6AddRegion(BaseAddress, AdjustedSize, MmWriteCombined, 0)) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Commit the changes to hardware.
    //
        
    return KiAmdK6MtrrCommitChanges();
}

BOOLEAN
KiAmdK6AddRegion (
    ULONG BaseAddress,
    ULONG Size,
    MEMORY_CACHING_TYPE Type,
    ULONG Flags
    )
{
    PAMDK6_MTRR_REGION pRegion;

    if ((pRegion = KiAmdK6FindFreeRegion(Type)) == NULL) {
        return FALSE;
    }
    pRegion->BaseAddress = BaseAddress;
    pRegion->Size = Size;
    pRegion->RegionType = Type;
    pRegion->RegionFlags = Flags;
    
    return TRUE;
}

PAMDK6_MTRR_REGION
KiAmdK6FindFreeRegion (
    MEMORY_CACHING_TYPE Type
    )
{
    ULONG    i;

    //
    // If this is a MmWriteCombined request, limit the number of
    // regions to match the actual hardware support.
    //

    if (Type == MmWriteCombined) {
        if (AmdMtrrHwUsageCount >= AMDK6_MAX_MTRR) {

            //
            // Search the table to see if there are any BIOS entries
            // we can replace.
            //

            for (i = 0; i < AmdK6RegionCount; i++) {
                if (AmdK6Regions[i].RegionFlags & AMDK6_REGION_FLAGS_BIOS) {
                    return &AmdK6Regions[i];
                }
            }

            //
            // No free HW MTRRs and no reusable entries.
            //

            return FALSE;
        }
    }

    //
    // Find the next free region in the table.
    //

    for (i = 0; i < AmdK6RegionCount; i++) {
        if (AmdK6Regions[i].BaseAddress == AMDK6_REGION_UNUSED) {

            if (Type == MmWriteCombined) {
                AmdMtrrHwUsageCount++;
            }
            return &AmdK6Regions[i];
        }
    }


    DBGMSG("AmdK6FindFreeRegion: Region Table is Full!\n");

    return NULL;
}

NTSTATUS
KiAmdK6MtrrCommitChanges (
    VOID
    )

/*++

Routine Description:

    Commits the values in the table to hardware.

    This procedure builds the MTRR images into the KiAmdK6Mtrr variable and
    calls KiLoadMTRR to actually load the register.

Arguments:

   None.

Return Value:

   None.

--*/

{
    ULONG    i, dwWcRangeCount = 0;
    ULONG    RangeTemp, RangeMask;

    //
    // Reset the MTRR image for both MTRRs disabled.
    //

    KiAmdK6Mtrr.u.hw.mtrr0.type = AMDK6_MTRR_TYPE_DISABLED;
    KiAmdK6Mtrr.u.hw.mtrr1.type = AMDK6_MTRR_TYPE_DISABLED;

    //
    // Find the Write Combining Regions, if any and set up the MTRR register.
    //

    for (i = 0; i < AmdK6RegionCount; i++) {

        //
        // Is this a valid region, and is it a write combined type?
        //

        if ((AmdK6Regions[i].BaseAddress != AMDK6_REGION_UNUSED) &&
            (AmdK6Regions[i].RegionType == MmWriteCombined)) {
            
            //
            // Calculate the correct mask for this range size.  The
            // BaseAddress and size were validated and adjusted in
            // AmdK6MtrrSetMemoryType().
            //
            // Start with 128K and scan for all legal range values and
            // build the appropriate range mask at the same time.
            //

            RangeTemp = 0x00020000;
            RangeMask = 0xfffe0000;            

            while (RangeTemp != 0) {
                if (RangeTemp == AmdK6Regions[i].Size) {
                    break;
                }
                RangeTemp <<= 1;
                RangeMask <<= 1;
            }
            if (RangeTemp == 0) {

                //
                // Not a valid range size.  This can never happen!!
                //

                DBGMSG ("AmdK6MtrrCommitChanges: Bad WC range in region table!\n");

                return STATUS_NOT_SUPPORTED;
            }

            //
            // Add the region to the next available register.
            //

            if (dwWcRangeCount == 0)  {

                KiAmdK6Mtrr.u.hw.mtrr0.base = AmdK6Regions[i].BaseAddress >> 17;
                KiAmdK6Mtrr.u.hw.mtrr0.mask = RangeMask >> 17;
                KiAmdK6Mtrr.u.hw.mtrr0.type = AMDK6_MTRR_TYPE_WC;
                dwWcRangeCount++;

            }  else if (dwWcRangeCount == 1) {

                KiAmdK6Mtrr.u.hw.mtrr1.base = AmdK6Regions[i].BaseAddress >> 17;
                KiAmdK6Mtrr.u.hw.mtrr1.mask = RangeMask >> 17;
                KiAmdK6Mtrr.u.hw.mtrr1.type = AMDK6_MTRR_TYPE_WC;
                dwWcRangeCount++;

            } else {

                //
                // Should never happen!  This should have been caught in
                // the calling routine.
                //

                DBGMSG ("AmdK6MtrrCommitChanges: Not enough MTRR registers to satisfy region table!\n");

                return STATUS_NOT_SUPPORTED;
            }
        }
    }

    //
    // Commit the changes to hardware.
    //

    KiLoadMTRR(NULL);

    return STATUS_SUCCESS;
}

VOID
KiAmdK6MtrrWRMSR (
    VOID
    )

/*++

Routine Description:

    Write the AMD K6 MTRRs.

    Note: Access to KiAmdK6Mtrr has been synchronized around this
    call.

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    // Write the MTRRs
    //

    WRMSR (AMDK6_MTRR_MSR, KiAmdK6Mtrr.u.QuadPart);
}


