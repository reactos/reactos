/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/pte.hpp
 * PURPOSE:         PTE class header
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

#pragma once

//
// Specific PTE Definitions that map to the Memory Manager's Protection Mask Bits
// The Memory Manager's definition define the attributes that must be preserved
// and these PTE definitions describe the attributes in the hardware sense. This
// helps deal with hardware differences between the actual boolean expression of
// the argument.
//
// For example, in the logical attributes, we want to express read-only as a flag
// but on x86, it is writability that must be set. On the other hand, on x86, just
// like in the kernel, it is disabling the caches that requires a special flag,
// while on certain architectures such as ARM, it is enabling the cache which
// requires a flag.
//
#if defined(_M_IX86) || defined(_M_AMD64)
//
// Access Flags
//
#define PTE_READONLY            0 // Doesn't exist on x86
#define PTE_EXECUTE             0 // Not worrying about NX yet
#define PTE_EXECUTE_READ        0 // Not worrying about NX yet
#define PTE_READWRITE           0x2
#define PTE_WRITECOPY           0x200
#define PTE_EXECUTE_READWRITE   0x2 // Not worrying about NX yet
#define PTE_EXECUTE_WRITECOPY   0x200
#define PTE_PROTOTYPE           0x400

//
// State Flags
//
#define PTE_VALID               0x1
#define PTE_ACCESSED            0x20
#define PTE_DIRTY               0x40

//
// Cache flags
//
#define PTE_ENABLE_CACHE        0
#define PTE_DISABLE_CACHE       0x10
#define PTE_WRITECOMBINED_CACHE 0x10
#elif defined(_M_ARM)
#define PTE_READONLY            0x200
#define PTE_EXECUTE             0 // Not worrying about NX yet
#define PTE_EXECUTE_READ        0 // Not worrying about NX yet
#define PTE_READWRITE           0 // Doesn't exist on ARM
#define PTE_WRITECOPY           0 // Doesn't exist on ARM
#define PTE_EXECUTE_READWRITE   0 // Not worrying about NX yet
#define PTE_EXECUTE_WRITECOPY   0 // Not worrying about NX yet
#define PTE_PROTOTYPE           0x400 // Using the Shared bit
//
// Cache flags
//
#define PTE_ENABLE_CACHE        0
#define PTE_DISABLE_CACHE       0x10
#define PTE_WRITECOMBINED_CACHE 0x10
#else
#error Define these please!
#endif

// Corresponds to MMPTE_SOFTWARE.Protection
#ifdef _M_IX86
#define MM_PTE_SOFTWARE_PROTECTION_BITS   5
#elif _M_ARM
#define MM_PTE_SOFTWARE_PROTECTION_BITS   6
#elif _M_AMD64
#define MM_PTE_SOFTWARE_PROTECTION_BITS   5
#else
#error Define these please!
#endif

// FIXME: Move inside PTENTRY ?
extern const ULONG_PTR MmProtectToPteMask[32];
extern const ULONG MmProtectToValue[32];

// FIXME: Should be called MMPTE
// FIXME: Should be totally in an arch specific dir?
// Must be plain old data!
class PTENTRY
{
public:
    union
    {
        ULONG_PTR Long;
        HARDWARE_PTE Flush;
        MMPTE_HARDWARE Hard;
        MMPTE_PROTOTYPE Proto;
        MMPTE_SOFTWARE Soft;
        MMPTE_TRANSITION Trans;
        MMPTE_SUBSECTION Subsect;
        MMPTE_LIST List;
    } u;

    static PTENTRY *AddressToPde(ULONG_PTR Address);
    static PTENTRY *AddressToPte(ULONG_PTR Address);
    PVOID PdeToAddress();
    PVOID PteToAddress();
    static PVOID PteToAddress(PVOID Ptr);

    PFN_NUMBER GetPfn();
    VOID SetPfn(PFN_NUMBER Number);
    ULONG_PTR DetermineUserGlobalPteMask(PTENTRY *PointerPte);

    VOID WriteValidPte(PTENTRY *PteToWrite)
    {
        /* Write the valid PTE */
        ASSERT(this->u.Hard.Valid == 0);
        ASSERT(PteToWrite->u.Hard.Valid == 1);
        this->u.Long = PteToWrite->u.Long;
    }

    VOID MakeValidPte(PFN_NUMBER Number, ULONG Protection, PTENTRY *MaskPte)
    {
        // Start fresh
        this->u.Long = 0;

        // Set page frame number
        SetPfn(Number);

        // Set protection
        this->u.Long |= MmProtectToPteMask[Protection];

        // Add global mask
        this->u.Long |= DetermineUserGlobalPteMask(MaskPte);
    }

    BOOLEAN IsUserPde(PVOID Address)
    {
        return ((Address >= (PVOID)AddressToPde(NULL)) &&
                (Address <= (PVOID)AddressToPde((ULONG_PTR)MmHighestUserAddress)));
    }

    BOOLEAN IsUserPte(PVOID Address)
    {
        return (Address <= (PVOID)AddressToPte((ULONG_PTR)MmHighestUserAddress));
    }


    BOOLEAN IsValid()
    {
        return (this->u.Hard.Valid != 0);
    }

    BOOLEAN IsLargePage()
    {
        return (this->u.Hard.LargePage != 0);
    }

    // Universal
    VOID SetZero()
    {
        u.Long = 0;
    }

    VOID SetDirty()
    {
        u.Hard.Dirty = 1;
    }

    VOID SetEmpty()
    {
        u.Hard.PageFrameNumber = MM_EMPTY_PTE_LIST;
    }

    BOOLEAN IsCachingDisabled()
    {
        if (u.Hard.CacheDisable == 1)
            return TRUE;
        else
            return FALSE;
    }

    // List
    VOID SetNextEntryEmpty()
    {
        u.List.NextEntry = MM_EMPTY_PTE_LIST;
    }

    BOOLEAN IsNextEntryEmpty()
    {
        return (u.List.NextEntry == MM_EMPTY_PTE_LIST);
    }

    VOID SetNextEntry(ULONG Entry)
    {
        u.List.NextEntry = Entry;
    }

    ULONG GetNextEntry()
    {
        return u.List.NextEntry;
    }
};

C_ASSERT(sizeof(PTENTRY) == sizeof(MMPTE));
