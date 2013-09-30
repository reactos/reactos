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

// #define NDEBUG
#include <debug.h>

#include <soft386.h>
#include "common.h"

/* PRIVATE FUNCTIONS **********************************************************/

static inline
ULONG
Soft386GetPageTableEntry(PSOFT386_STATE State,
                         ULONG VirtualAddress)
{
    // TODO: NOT IMPLEMENTED
    UNIMPLEMENTED;

    return 0;
}

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN
Soft386ReadMemory(PSOFT386_STATE State,
                  SOFT386_SEG_REGS SegmentReg,
                  ULONG Offset,
                  BOOLEAN InstFetch,
                  PVOID Buffer,
                  ULONG Size)
{
    ULONG LinearAddress;
    PSOFT386_SEG_REG CachedDescriptor;
    INT Cpl = Soft386GetCurrentPrivLevel(State);

    ASSERT(SegmentReg < SOFT386_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[SegmentReg];

    if ((Offset + Size) > CachedDescriptor->Limit)
    {
        /* Read beyond limit */
        Soft386Exception(State, SOFT386_EXCEPTION_GP);

        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[0] & SOFT386_CR0_PE)
    {
        /* Privilege checks */

        if (!CachedDescriptor->Present)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_NP);
            return FALSE;
        }

        if (GET_SEGMENT_RPL(CachedDescriptor->Selector) > CachedDescriptor->Dpl)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }

        if (InstFetch)
        {
            if (!CachedDescriptor->Executable)
            {
                /* Data segment not executable */

                Soft386Exception(State, SOFT386_EXCEPTION_GP);
                return FALSE;
            }
        }
        else
        {
            if (CachedDescriptor->Executable && (!CachedDescriptor->ReadWrite))
            {
                /* Code segment not readable */

                Soft386Exception(State, SOFT386_EXCEPTION_GP);
                return FALSE;
            }
        }
    }

    /* Find the linear address */
    LinearAddress = CachedDescriptor->Base + Offset;

    /* Check if paging is enabled */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PG)
    {
        ULONG Page;
        SOFT386_PAGE_TABLE TableEntry;

        for (Page = PAGE_ALIGN(LinearAddress);
             Page <= PAGE_ALIGN(LinearAddress + Size - 1);
             Page += PAGE_SIZE)
        {
            ULONG PageOffset = 0, PageLength = PAGE_SIZE;

            /* Get the table entry */
            TableEntry.Value = Soft386GetPageTableEntry(State, Page);

            if (!TableEntry.Present || (!TableEntry.Usermode && (Cpl > 0)))
            {
                /* Exception */
                Soft386ExceptionWithErrorCode(State,
                                              SOFT386_EXCEPTION_PF,
                                              TableEntry.Value & 0x07);
                return FALSE;
            }

            /* Check if this is the first page */
            if (Page == PAGE_ALIGN(LinearAddress))
            {
                /* Start copying from the offset from the beginning of the page */
                PageOffset = PAGE_OFFSET(LinearAddress);
            }

            /* Check if this is the last page */
            if (Page == PAGE_ALIGN(LinearAddress + Size - 1))
            {   
                /* Copy only a part of the page */
                PageLength = PAGE_OFFSET(LinearAddress + Size);
            }

            /* Did the host provide a memory hook? */
            if (State->MemReadCallback)
            {
                /* Yes, call the host */
                State->MemReadCallback(State,
                                       (TableEntry.Address << 12) | PageOffset,
                                       Buffer,
                                       PageLength);
            }
            else
            {
                /* Read the memory directly */
                RtlMoveMemory(Buffer,
                              (PVOID)((TableEntry.Address << 12) | PageOffset),
                              PageLength);
            }
        }
    }
    else
    {
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
    }

    return TRUE;
}

BOOLEAN
Soft386WriteMemory(PSOFT386_STATE State,
                   SOFT386_SEG_REGS SegmentReg,
                   ULONG Offset,
                   PVOID Buffer,
                   ULONG Size)
{
    ULONG LinearAddress;
    PSOFT386_SEG_REG CachedDescriptor;
    INT Cpl = Soft386GetCurrentPrivLevel(State);

    ASSERT(SegmentReg < SOFT386_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[SegmentReg];

    if ((Offset + Size) >= CachedDescriptor->Limit)
    {
        /* Write beyond limit */
        Soft386Exception(State, SOFT386_EXCEPTION_GP);

        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[0] & SOFT386_CR0_PE)
    {
        /* Privilege checks */

        if (!CachedDescriptor->Present)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_NP);
            return FALSE;
        }

        if (GET_SEGMENT_RPL(CachedDescriptor->Selector) > CachedDescriptor->Dpl)
        {
            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }

        if (CachedDescriptor->Executable)
        {
            /* Code segment not writable */

            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }
        else if (!CachedDescriptor->ReadWrite)
        {
            /* Data segment not writeable */

            Soft386Exception(State, SOFT386_EXCEPTION_GP);
            return FALSE;
        }
    }

    /* Find the linear address */
    LinearAddress = CachedDescriptor->Base + Offset;

    /* Check if paging is enabled */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PG)
    {
        ULONG Page;
        SOFT386_PAGE_TABLE TableEntry;

        for (Page = PAGE_ALIGN(LinearAddress);
             Page <= PAGE_ALIGN(LinearAddress + Size - 1);
             Page += PAGE_SIZE)
        {
            ULONG PageOffset = 0, PageLength = PAGE_SIZE;

            /* Get the table entry */
            TableEntry.Value = Soft386GetPageTableEntry(State, Page);

            if ((!TableEntry.Present || (!TableEntry.Usermode && (Cpl > 0)))
                || ((State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_WP)
                && !TableEntry.Writeable))
            {
                /* Exception */
                Soft386ExceptionWithErrorCode(State,
                                              SOFT386_EXCEPTION_PF,
                                              TableEntry.Value & 0x07);
                return FALSE;
            }

            /* Check if this is the first page */
            if (Page == PAGE_ALIGN(LinearAddress))
            {
                /* Start copying from the offset from the beginning of the page */
                PageOffset = PAGE_OFFSET(LinearAddress);
            }

            /* Check if this is the last page */
            if (Page == PAGE_ALIGN(LinearAddress + Size - 1))
            {   
                /* Copy only a part of the page */
                PageLength = PAGE_OFFSET(LinearAddress + Size);
            }

            /* Did the host provide a memory hook? */
            if (State->MemWriteCallback)
            {
                /* Yes, call the host */
                State->MemWriteCallback(State,
                                        (TableEntry.Address << 12) | PageOffset,
                                        Buffer,
                                        PageLength);
            }
            else
            {
                /* Read the memory directly */
                RtlMoveMemory((PVOID)((TableEntry.Address << 12) | PageOffset),
                              Buffer,
                              PageLength);
            }
        }
    }
    else
    {
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
    }

    return TRUE;
}

BOOLEAN
Soft386InterruptInternal(PSOFT386_STATE State,
                         USHORT SegmentSelector,
                         ULONG Offset,
                         BOOLEAN InterruptGate)
{
    /* Check for protected mode */
    if (State->ControlRegisters[SOFT386_REG_CR0] & SOFT386_CR0_PE)
    {
        SOFT386_TSS Tss;
        USHORT OldSs = State->SegmentRegs[SOFT386_REG_SS].Selector;
        ULONG OldEsp = State->GeneralRegs[SOFT386_REG_ESP].Long;

        /* Check if the interrupt handler is more privileged */
        if (Soft386GetCurrentPrivLevel(State) > GET_SEGMENT_RPL(SegmentSelector))
        {
            /* Read the TSS */
            // FIXME: This code is only correct when paging is disabled!!!
            if (State->MemReadCallback)
            {
                State->MemReadCallback(State,
                                       State->Tss.Address,
                                       &Tss,
                                       sizeof(Tss));
            }
            else
            {
                RtlMoveMemory(&Tss, (PVOID)State->Tss.Address, sizeof(Tss));
            }

            /* Check the new (higher) privilege level */
            switch (GET_SEGMENT_RPL(SegmentSelector))
            {
                case 0:
                {
                    if (!Soft386LoadSegment(State, SOFT386_REG_SS, Tss.Ss0))
                    {
                        /* Exception occurred */
                        return FALSE;
                    }
                    State->GeneralRegs[SOFT386_REG_ESP].Long = Tss.Esp0;

                    break;
                }

                case 1:
                {
                    if (!Soft386LoadSegment(State, SOFT386_REG_SS, Tss.Ss1))
                    {
                        /* Exception occurred */
                        return FALSE;
                    }
                    State->GeneralRegs[SOFT386_REG_ESP].Long = Tss.Esp1;

                    break;
                }

                case 2:
                {
                    if (!Soft386LoadSegment(State, SOFT386_REG_SS, Tss.Ss2))
                    {
                        /* Exception occurred */
                        return FALSE;
                    }
                    State->GeneralRegs[SOFT386_REG_ESP].Long = Tss.Esp2;

                    break;
                }

                default:
                {
                    /* Should never reach here! */
                    ASSERT(FALSE);
                }
            }

            /* Push SS selector */
            if (!Soft386StackPush(State, OldSs)) return FALSE;

            /* Push stack pointer */
            if (!Soft386StackPush(State, OldEsp)) return FALSE;
        }
    }

    /* Push EFLAGS */
    if (!Soft386StackPush(State, State->Flags.Long)) return FALSE;

    /* Push CS selector */
    if (!Soft386StackPush(State, State->SegmentRegs[SOFT386_REG_CS].Selector)) return FALSE;

    /* Push the instruction pointer */
    if (!Soft386StackPush(State, State->InstPtr.Long)) return FALSE;

    if (InterruptGate)
    {
        /* Disable interrupts after a jump to an interrupt gate handler */
        State->Flags.If = FALSE;
    }

    /* Load new CS */
    if (!Soft386LoadSegment(State, SOFT386_REG_CS, SegmentSelector))
    {
        /* An exception occurred during the jump */
        return FALSE;
    }

    if (State->SegmentRegs[SOFT386_REG_CS].Size)
    {
        /* 32-bit code segment, use EIP */
        State->InstPtr.Long = Offset;
    }
    else
    {
        /* 16-bit code segment, use IP */
        State->InstPtr.LowWord = LOWORD(Offset);
    }

    return TRUE;
}

VOID
FASTCALL
Soft386ExceptionWithErrorCode(PSOFT386_STATE State,
                              SOFT386_EXCEPTIONS ExceptionCode,
                              ULONG ErrorCode)
{
    SOFT386_IDT_ENTRY IdtEntry;

    /* Increment the exception count */
    State->ExceptionCount++;

    /* Check if the exception occurred more than once */
    if (State->ExceptionCount > 1)
    {
        /* Then this is a double fault */
        ExceptionCode = SOFT386_EXCEPTION_DF;
    }

    /* Check if this is a triple fault */
    if (State->ExceptionCount == 3)
    {
        /* Reset the CPU */
        Soft386Reset(State);
        return;
    }

    if (!Soft386GetIntVector(State, ExceptionCode, &IdtEntry))
    {
        /*
         * If this function failed, that means Soft386Exception
         * was called again, so just return in this case.
         */
        return;
    }

    /* Perform the interrupt */
    if (!Soft386InterruptInternal(State,
                                  IdtEntry.Selector,
                                  MAKELONG(IdtEntry.Offset, IdtEntry.OffsetHigh),
                                  IdtEntry.Type))
    {
        /*
         * If this function failed, that means Soft386Exception
         * was called again, so just return in this case.
         */
        return;
    }

    if (EXCEPTION_HAS_ERROR_CODE(ExceptionCode))
    {
        /* Push the error code */
        Soft386StackPush(State, ErrorCode);
    }
}

/* EOF */
