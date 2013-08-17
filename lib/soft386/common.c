/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            common.c
 * PURPOSE:         Common functions used internally by Soft386.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

// #define WIN32_NO_STATUS
// #define _INC_WINDOWS
#include <windef.h>

#include <soft386.h>
#include "common.h"

// #define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

static
inline
INT
Soft386GetCurrentPrivLevel(PSOFT386_STATE State)
{
    return GET_SEGMENT_RPL(State->SegmentRegs[SOFT386_REG_CS].Selector);
}

/* PUBLIC FUNCTIONS ***********************************************************/

inline
BOOLEAN
Soft386ReadMemory(PSOFT386_STATE State,
                  INT SegmentReg,
                  ULONG Offset,
                  BOOLEAN InstFetch,
                  PVOID Buffer,
                  ULONG Size)
{
    ULONG LinearAddress;
    PSOFT386_SEG_REG CachedDescriptor;

    ASSERT(SegmentReg < SOFT386_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[SegmentReg];

    if ((Offset + Size) >= CachedDescriptor->Limit)
    {
        /* Read beyond limit */
        // TODO: Generate exception #GP

        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[0] & SOFT386_CR0_PE)
    {
        /* Privilege checks */

        if (!CachedDescriptor->Present)
        {
            // TODO: Generate exception #NP
            return FALSE;
        }

        if (GET_SEGMENT_RPL(CachedDescriptor->Selector) > CachedDescriptor->Dpl)
        {
            // TODO: Generate exception #GP
            return FALSE;
        }

        if (InstFetch)
        {
            if (!CachedDescriptor->Executable)
            {
                /* Data segment not executable */

                // TODO: Generate exception #GP
                return FALSE;
            }
        }
        else
        {
            if (CachedDescriptor->Executable && (!CachedDescriptor->ReadWrite))
            {
                /* Code segment not readable */

                // TODO: Generate exception #GP
                return FALSE;
            }
        }
    }

    /* Find the linear address */
    LinearAddress = CachedDescriptor->Base + Offset;

    // TODO: Paging support!

    /* Did the host provide a memory hook? */
    if (State->MemReadCallback)
    {
        /* Yes, call the host */
        State->MemReadCallback(State, LinearAddress, Buffer, Size);
    }
    else
    {
        /* Read the memory directly */
        RtlMoveMemory(Buffer, (PVOID)LinearAddress, Size);
    }

    return TRUE;
}

inline
BOOLEAN
Soft386WriteMemory(PSOFT386_STATE State,
                   INT SegmentReg,
                   ULONG Offset,
                   PVOID Buffer,
                   ULONG Size)
{
    ULONG LinearAddress;
    PSOFT386_SEG_REG CachedDescriptor;

    ASSERT(SegmentReg < SOFT386_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[SegmentReg];

    if ((Offset + Size) >= CachedDescriptor->Limit)
    {
        /* Write beyond limit */
        // TODO: Generate exception #GP

        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[0] & SOFT386_CR0_PE)
    {
        /* Privilege checks */

        if (!CachedDescriptor->Present)
        {
            // TODO: Generate exception #NP
            return FALSE;
        }

        if (GET_SEGMENT_RPL(CachedDescriptor->Selector) > CachedDescriptor->Dpl)
        {
            // TODO: Generate exception #GP
            return FALSE;
        }

        if (CachedDescriptor->Executable)
        {
            /* Code segment not writable */

            // TODO: Generate exception #GP
            return FALSE;
        }
        else if (!CachedDescriptor->ReadWrite)
        {
            /* Data segment not writeable */

            // TODO: Generate exception #GP
            return FALSE;
        }
    }

    /* Find the linear address */
    LinearAddress = CachedDescriptor->Base + Offset;

    // TODO: Paging support!

    /* Did the host provide a memory hook? */
    if (State->MemWriteCallback)
    {
        /* Yes, call the host */
        State->MemWriteCallback(State, LinearAddress, Buffer, Size);
    }
    else
    {
        /* Write the memory directly */
        RtlMoveMemory((PVOID)LinearAddress, Buffer, Size);
    }

    return TRUE;
}

inline
BOOLEAN
Soft386StackPush(PSOFT386_STATE State, ULONG Value)
{
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_SS].Size;

    // TODO: Handle OPSIZE prefix.

    if (Size)
    {
        /* 32-bit size */

        /* Check if ESP is between 1 and 3 */
        if (State->GeneralRegs[SOFT386_REG_ESP].Long >= 1
            && State->GeneralRegs[SOFT386_REG_ESP].Long <= 3)
        {
            // TODO: Exception #SS
            return FALSE;
        }

        /* Subtract ESP by 4 */
        State->GeneralRegs[SOFT386_REG_ESP].Long -= 4;

        /* Store the value in SS:ESP */
        return Soft386WriteMemory(State,
                                  SOFT386_REG_SS,
                                  State->GeneralRegs[SOFT386_REG_ESP].Long,
                                  &Value,
                                  sizeof(ULONG));
    }
    else
    {
        /* 16-bit size */
        USHORT ShortValue = LOWORD(Value);

        /* Check if SP is 1 */
        if (State->GeneralRegs[SOFT386_REG_ESP].Long == 1)
        {
            // TODO: Exception #SS
            return FALSE;
        }

        /* Subtract SP by 2 */
        State->GeneralRegs[SOFT386_REG_ESP].LowWord -= 2;

        /* Store the value in SS:SP */
        return Soft386WriteMemory(State,
                                  SOFT386_REG_SS,
                                  State->GeneralRegs[SOFT386_REG_ESP].LowWord,
                                  &ShortValue,
                                  sizeof(USHORT));
    }
}

inline
BOOLEAN
Soft386StackPop(PSOFT386_STATE State, PULONG Value)
{
    ULONG LongValue;
    USHORT ShortValue;
    BOOLEAN Size = State->SegmentRegs[SOFT386_REG_SS].Size;

    // TODO: Handle OPSIZE prefix.

    if (Size)
    {
        /* 32-bit size */

        /* Check if ESP is 0xFFFFFFFF */
        if (State->GeneralRegs[SOFT386_REG_ESP].Long == 0xFFFFFFFF)
        {
            // TODO: Exception #SS
            return FALSE;
        }

        /* Read the value from SS:ESP */
        if (!Soft386ReadMemory(State,
                               SOFT386_REG_SS,
                               State->GeneralRegs[SOFT386_REG_ESP].Long,
                               FALSE,
                               &LongValue,
                               sizeof(LongValue)))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Increment ESP by 4 */
        State->GeneralRegs[SOFT386_REG_ESP].Long += 4;

        /* Store the value in the result */
        *Value = LongValue;
    }
    else
    {
        /* 16-bit size */

        /* Check if SP is 0xFFFF */
        if (State->GeneralRegs[SOFT386_REG_ESP].LowWord == 0xFFFF)
        {
            // TODO: Exception #SS
            return FALSE;
        }

        /* Read the value from SS:SP */
        if (!Soft386ReadMemory(State,
                               SOFT386_REG_SS,
                               State->GeneralRegs[SOFT386_REG_ESP].LowWord,
                               FALSE,
                               &ShortValue,
                               sizeof(ShortValue)))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Increment SP by 2 */
        State->GeneralRegs[SOFT386_REG_ESP].Long += 2;

        /* Store the value in the result */
        *Value = ShortValue;
    }

    return TRUE;
}

inline
BOOLEAN
Soft386LoadSegment(PSOFT386_STATE State, INT Segment, WORD Selector)
{
    PSOFT386_SEG_REG CachedDescriptor;
    SOFT386_GDT_ENTRY GdtEntry;

    ASSERT(Segment < SOFT386_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[Segment];

    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        /* Make sure the GDT contains the entry */
        if (GET_SEGMENT_INDEX(Selector) >= (State->Gdtr.Size + 1))
        {
            // TODO: Exception #GP
            return FALSE;
        }

        /* Read the GDT */
        // FIXME: This code is only correct when paging is disabled!!!
        if (State->MemReadCallback)
        {
            State->MemReadCallback(State,
                                   State->Gdtr.Address
                                   + GET_SEGMENT_INDEX(Selector),
                                   &GdtEntry,
                                   sizeof(GdtEntry));
        }
        else
        {
            RtlMoveMemory(&GdtEntry,
                          (PVOID)(State->Gdtr.Address
                          + GET_SEGMENT_INDEX(Selector)),
                          sizeof(GdtEntry));
        }

        /* Check if we are loading SS */
        if (Segment == SOFT386_REG_SS)
        {
            if (GET_SEGMENT_INDEX(Selector) == 0)
            {
                // TODO: Exception #GP
                return FALSE;
            }

            if (GdtEntry.Executable || !GdtEntry.ReadWrite)
            {
                // TODO: Exception #GP
                return FALSE;
            }

            if ((GET_SEGMENT_RPL(Selector) != Soft386GetCurrentPrivLevel(State))
                || (GET_SEGMENT_RPL(Selector) != GdtEntry.Dpl))
            {
                // TODO: Exception #GP
                return FALSE;
            }

            if (!GdtEntry.Present)
            {
                // TODO: Exception #SS
                return FALSE;
            }
        }
        else
        {
            if ((GET_SEGMENT_RPL(Selector) > GdtEntry.Dpl)
                && (Soft386GetCurrentPrivLevel(State) > GdtEntry.Dpl))
            {
                // TODO: Exception #GP
                return FALSE;
            }

            if (!GdtEntry.Present)
            {
                // TODO: Exception #NP
                return FALSE;
            }
        }

        /* Update the cache entry */
        CachedDescriptor->Selector = Selector;
        CachedDescriptor->Base = GdtEntry.Base | (GdtEntry.BaseHigh << 24);
        CachedDescriptor->Limit = GdtEntry.Limit | (GdtEntry.LimitHigh << 16);
        CachedDescriptor->Accessed = GdtEntry.Accessed;
        CachedDescriptor->ReadWrite = GdtEntry.ReadWrite;
        CachedDescriptor->DirConf = GdtEntry.DirConf;
        CachedDescriptor->Executable = GdtEntry.Executable;
        CachedDescriptor->SystemType = GdtEntry.SystemType;
        CachedDescriptor->Dpl = GdtEntry.Dpl;
        CachedDescriptor->Present = GdtEntry.Present;
        CachedDescriptor->Size = GdtEntry.Size;

        /* Check for page granularity */
        if (GdtEntry.Granularity) CachedDescriptor->Limit <<= 12;
    }
    else
    {
        /* Update the selector and base */
        CachedDescriptor->Selector = Selector;
        CachedDescriptor->Base = Selector << 4;
    }

    return TRUE;
}

/* EOF */
