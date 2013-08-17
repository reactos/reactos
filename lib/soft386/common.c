/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         386/486 CPU Emulation Library
 * FILE:            common.c
 * PURPOSE:         Common functions used internally by Soft386.
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#include "common.h"

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

        if (GET_SEGMENT_DPL(CachedDescriptor->Selector) > CachedDescriptor->Dpl)
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
        RtlMoveMemory(Buffer, (LPVOID)LinearAddress, Size);
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

        if (GET_SEGMENT_DPL(CachedDescriptor->Selector) > CachedDescriptor->Dpl)
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
        RtlMoveMemory((LPVOID)LinearAddress, Buffer, Size);
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

/* EOF */
