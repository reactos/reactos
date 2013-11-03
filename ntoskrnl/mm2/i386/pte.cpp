/*
 * PROJECT:         Monstera
 * LICENSE:         
 * FILE:            mm2/i386/pte.cpp
 * PURPOSE:         x86 specific PTE implementation
 * PROGRAMMERS:     Aleksey Bragin <aleksey@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "../memorymanager.hpp"

//#define NDEBUG
#include <debug.h>

const
ULONG_PTR
MmProtectToPteMask[32] =
{
    //
    // These are the base MM_ protection flags
    //
    0,
    PTE_READONLY            | PTE_ENABLE_CACHE,
    PTE_EXECUTE             | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_ENABLE_CACHE,
    PTE_READWRITE           | PTE_ENABLE_CACHE,
    PTE_WRITECOPY           | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_ENABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_ENABLE_CACHE,
    //
    // These OR in the MM_NOCACHE flag
    //
    0,
    PTE_READONLY            | PTE_DISABLE_CACHE,
    PTE_EXECUTE             | PTE_DISABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_DISABLE_CACHE,
    PTE_READWRITE           | PTE_DISABLE_CACHE,
    PTE_WRITECOPY           | PTE_DISABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_DISABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_DISABLE_CACHE,
    //
    // These OR in the MM_DECOMMIT flag, which doesn't seem supported on x86/64/ARM
    //
    0,
    PTE_READONLY            | PTE_ENABLE_CACHE,
    PTE_EXECUTE             | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READ        | PTE_ENABLE_CACHE,
    PTE_READWRITE           | PTE_ENABLE_CACHE,
    PTE_WRITECOPY           | PTE_ENABLE_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_ENABLE_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_ENABLE_CACHE,
    //
    // These OR in the MM_NOACCESS flag, which seems to enable WriteCombining?
    //
    0,
    PTE_READONLY            | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE             | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_READ        | PTE_WRITECOMBINED_CACHE,
    PTE_READWRITE           | PTE_WRITECOMBINED_CACHE,
    PTE_WRITECOPY           | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_READWRITE   | PTE_WRITECOMBINED_CACHE,
    PTE_EXECUTE_WRITECOPY   | PTE_WRITECOMBINED_CACHE,
};

const
ULONG MmProtectToValue[32] =
{
    PAGE_NOACCESS,
    PAGE_READONLY,
    PAGE_EXECUTE,
    PAGE_EXECUTE_READ,
    PAGE_READWRITE,
    PAGE_WRITECOPY,
    PAGE_EXECUTE_READWRITE,
    PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_NOCACHE | PAGE_READONLY,
    PAGE_NOCACHE | PAGE_EXECUTE,
    PAGE_NOCACHE | PAGE_EXECUTE_READ,
    PAGE_NOCACHE | PAGE_READWRITE,
    PAGE_NOCACHE | PAGE_WRITECOPY,
    PAGE_NOCACHE | PAGE_EXECUTE_READWRITE,
    PAGE_NOCACHE | PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_GUARD | PAGE_READONLY,
    PAGE_GUARD | PAGE_EXECUTE,
    PAGE_GUARD | PAGE_EXECUTE_READ,
    PAGE_GUARD | PAGE_READWRITE,
    PAGE_GUARD | PAGE_WRITECOPY,
    PAGE_GUARD | PAGE_EXECUTE_READWRITE,
    PAGE_GUARD | PAGE_EXECUTE_WRITECOPY,
    PAGE_NOACCESS,
    PAGE_WRITECOMBINE | PAGE_READONLY,
    PAGE_WRITECOMBINE | PAGE_EXECUTE,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_READ,
    PAGE_WRITECOMBINE | PAGE_READWRITE,
    PAGE_WRITECOMBINE | PAGE_WRITECOPY,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_READWRITE,
    PAGE_WRITECOMBINE | PAGE_EXECUTE_WRITECOPY
};

PTENTRY *
PTENTRY::
AddressToPde(ULONG_PTR Ptr)
{
    return ((PTENTRY *)(((Ptr >> 22) << 2) + PDE_BASE));
}

PTENTRY *
PTENTRY::
AddressToPte(ULONG_PTR Ptr)
{
    return ((PTENTRY *)(((Ptr >> 12) << 2) + PTE_BASE));
}


PVOID
PTENTRY::
PdeToAddress()
{
    return (PVOID)((ULONG)this << 20);
}

PVOID
PTENTRY::
PteToAddress()
{
    return (PVOID)((ULONG)this << 10);
}

PVOID
PTENTRY::
PteToAddress(PVOID Ptr)
{
    return (PVOID)((ULONG_PTR)Ptr << 10);
}

/*static
PVOID
PTENTRY::
PteToAddress(ULONG_PTR Ptr)
{
    return (PVOID)(Ptr << 10);
}*/


PFN_NUMBER
PTENTRY::
GetPfn()
{
    return this->u.Hard.PageFrameNumber;
}

VOID
PTENTRY::
SetPfn(PFN_NUMBER Number)
{
    this->u.Hard.PageFrameNumber = Number;
}

// Figures out the hardware bits for a PTE
ULONG_PTR
PTENTRY::
DetermineUserGlobalPteMask(PTENTRY *PointerPte)
{
    PTENTRY TempPte;

    // Start fresh
    TempPte.u.Long = 0;

    // Make it valid and accessed
    TempPte.u.Hard.Valid = TRUE;
    TempPte.u.Hard.Accessed = TRUE;

    // Is this for user-mode?
    if (IsUserPde(PointerPte) ||
        IsUserPte(PointerPte))
    {
        // Set the owner bit
        TempPte.u.Hard.Owner = TRUE;
    }
    else if ((PointerPte < PTENTRY::AddressToPte(PTE_BASE)) ||
             (PointerPte >= PTENTRY::AddressToPte((ULONG_PTR)MI_SYSTEM_CACHE_WS_START)))
    {
        // Set the global bit
        TempPte.u.Hard.Global = MemoryManager->IsGlobalSupported();
    }

    // Return the protection
    return TempPte.u.Long;
}

