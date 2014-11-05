/*
 * Fast486 386/486 CPU Emulation Library
 * common.c
 *
 * Copyright (C) 2014 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* INCLUDES *******************************************************************/

#include <windef.h>

// #define NDEBUG
#include <debug.h>

#include <fast486.h>
#include "common.h"

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN
Fast486ReadMemory(PFAST486_STATE State,
                  FAST486_SEG_REGS SegmentReg,
                  ULONG Offset,
                  BOOLEAN InstFetch,
                  PVOID Buffer,
                  ULONG Size)
{
    ULONG LinearAddress;
    PFAST486_SEG_REG CachedDescriptor;

    ASSERT(SegmentReg < FAST486_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[SegmentReg];

    if ((Offset + Size - 1) > CachedDescriptor->Limit)
    {
        /* Read beyond limit */
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
    {
        /* Privilege checks */

        if (!CachedDescriptor->Present)
        {
            Fast486Exception(State, FAST486_EXCEPTION_NP);
            return FALSE;
        }

        if ((!InstFetch && (CachedDescriptor->Rpl > CachedDescriptor->Dpl))
            || (Fast486GetCurrentPrivLevel(State) > CachedDescriptor->Dpl))
        {
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }

        if (InstFetch)
        {
            if (!CachedDescriptor->Executable)
            {
                /* Data segment not executable */
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return FALSE;
            }
        }
        else
        {
            if (CachedDescriptor->Executable && (!CachedDescriptor->ReadWrite))
            {
                /* Code segment not readable */
                Fast486Exception(State, FAST486_EXCEPTION_GP);
                return FALSE;
            }
        }
    }

    /* Find the linear address */
    LinearAddress = CachedDescriptor->Base + Offset;

#ifndef FAST486_NO_PREFETCH
    if (InstFetch && ((Offset + FAST486_CACHE_SIZE - 1) <= CachedDescriptor->Limit))
    {
        State->PrefetchAddress = LinearAddress;

        if ((State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PG)
            && (PAGE_OFFSET(State->PrefetchAddress) > (FAST486_PAGE_SIZE - FAST486_CACHE_SIZE)))
        {
            /* We mustn't prefetch across a page boundary */
            State->PrefetchAddress = PAGE_ALIGN(State->PrefetchAddress)
                                     | (FAST486_PAGE_SIZE - FAST486_CACHE_SIZE);
        }

        /* Prefetch */
        if (Fast486ReadLinearMemory(State,
                                    State->PrefetchAddress,
                                    State->PrefetchCache,
                                    FAST486_CACHE_SIZE))
        {
            State->PrefetchValid = TRUE;

            RtlMoveMemory(Buffer,
                          &State->PrefetchCache[LinearAddress - State->PrefetchAddress],
                          Size);
            return TRUE;
        }
        else
        {
            State->PrefetchValid = FALSE;
            return FALSE;
        }
    }
    else
#endif
    {
        /* Read from the linear address */
        return Fast486ReadLinearMemory(State, LinearAddress, Buffer, Size);
    }
}

BOOLEAN
Fast486WriteMemory(PFAST486_STATE State,
                   FAST486_SEG_REGS SegmentReg,
                   ULONG Offset,
                   PVOID Buffer,
                   ULONG Size)
{
    ULONG LinearAddress;
    PFAST486_SEG_REG CachedDescriptor;

    ASSERT(SegmentReg < FAST486_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[SegmentReg];

    if ((Offset + Size - 1) > CachedDescriptor->Limit)
    {
        /* Write beyond limit */
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return FALSE;
    }

    /* Check for protected mode */
    if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
    {
        /* Privilege checks */

        if (!CachedDescriptor->Present)
        {
            Fast486Exception(State, FAST486_EXCEPTION_NP);
            return FALSE;
        }

        if ((CachedDescriptor->Rpl > CachedDescriptor->Dpl)
            || (Fast486GetCurrentPrivLevel(State) > CachedDescriptor->Dpl))
        {
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }

        if (CachedDescriptor->Executable)
        {
            /* Code segment not writable */
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }
        else if (!CachedDescriptor->ReadWrite)
        {
            /* Data segment not writeable */
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }
    }

    /* Find the linear address */
    LinearAddress = CachedDescriptor->Base + Offset;

#ifndef FAST486_NO_PREFETCH
    if (State->PrefetchValid
        && (LinearAddress >= State->PrefetchAddress)
        && ((LinearAddress + Size) <= (State->PrefetchAddress + FAST486_CACHE_SIZE)))
    {
        /* Update the prefetch */
        RtlMoveMemory(&State->PrefetchCache[LinearAddress - State->PrefetchAddress],
                      Buffer,
                      min(Size, FAST486_CACHE_SIZE + State->PrefetchAddress - LinearAddress));
    }
#endif

    /* Write to the linear address */
    return Fast486WriteLinearMemory(State, LinearAddress, Buffer, Size);
}

static inline BOOLEAN
FASTCALL
Fast486GetIntVector(PFAST486_STATE State,
                    UCHAR Number,
                    PFAST486_IDT_ENTRY IdtEntry)
{
    /* Check for protected mode */
    if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
    {
        /* Read from the IDT */
        if (!Fast486ReadLinearMemory(State,
                                     State->Idtr.Address
                                     + Number * sizeof(*IdtEntry),
                                     IdtEntry,
                                     sizeof(*IdtEntry)))
        {
            /* Exception occurred */
            return FALSE;
        }
    }
    else
    {
        /* Read from the real-mode IVT */
        ULONG FarPointer;

        /* Paging is always disabled in real mode */
        State->MemReadCallback(State,
                               State->Idtr.Address
                               + Number * sizeof(FarPointer),
                               &FarPointer,
                               sizeof(FarPointer));

        /* Fill a fake IDT entry */
        IdtEntry->Offset = LOWORD(FarPointer);
        IdtEntry->Selector = HIWORD(FarPointer);
        IdtEntry->Zero = 0;
        IdtEntry->Type = FAST486_IDT_INT_GATE;
        IdtEntry->Storage = FALSE;
        IdtEntry->Dpl = 0;
        IdtEntry->Present = TRUE;
        IdtEntry->OffsetHigh = 0;
    }

    return TRUE;
}

static inline BOOLEAN
FASTCALL
Fast486InterruptInternal(PFAST486_STATE State,
                         PFAST486_IDT_ENTRY IdtEntry)
{
    USHORT SegmentSelector = IdtEntry->Selector;
    ULONG  Offset          = MAKELONG(IdtEntry->Offset, IdtEntry->OffsetHigh);
    ULONG  GateType        = IdtEntry->Type;
    BOOLEAN GateSize = (GateType == FAST486_IDT_INT_GATE_32) ||
                       (GateType == FAST486_IDT_TRAP_GATE_32);

    BOOLEAN Success = FALSE;
    ULONG OldPrefixFlags = State->PrefixFlags;

    /* Check for protected mode */
    if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
    {
        FAST486_TSS Tss;
        USHORT OldSs = State->SegmentRegs[FAST486_REG_SS].Selector;
        ULONG OldEsp = State->GeneralRegs[FAST486_REG_ESP].Long;

        if (GateType == FAST486_TASK_GATE_SIGNATURE)
        {
            /* Task call */
            return Fast486TaskSwitch(State, FAST486_TASK_CALL, IdtEntry->Selector);
        }
        
        if (GateSize != (State->SegmentRegs[FAST486_REG_CS].Size))
        {
            /*
             * The gate size doesn't match the current operand size, so toggle
             * the OPSIZE flag.
             */
            State->PrefixFlags ^= FAST486_PREFIX_OPSIZE;
        }

        /* Check if the interrupt handler is more privileged */
        if (Fast486GetCurrentPrivLevel(State) > GET_SEGMENT_RPL(SegmentSelector))
        {
            /* Read the TSS */
            if (!Fast486ReadLinearMemory(State,
                                         State->TaskReg.Base,
                                         &Tss,
                                         sizeof(Tss)))
            {
                /* Exception occurred */
                goto Cleanup;
            }

            /* Check the new (higher) privilege level */
            switch (GET_SEGMENT_RPL(SegmentSelector))
            {
                case 0:
                {
                    if (!Fast486LoadSegment(State, FAST486_REG_SS, Tss.Ss0))
                    {
                        /* Exception occurred */
                        goto Cleanup;
                    }
                    State->GeneralRegs[FAST486_REG_ESP].Long = Tss.Esp0;

                    break;
                }

                case 1:
                {
                    if (!Fast486LoadSegment(State, FAST486_REG_SS, Tss.Ss1))
                    {
                        /* Exception occurred */
                        goto Cleanup;
                    }
                    State->GeneralRegs[FAST486_REG_ESP].Long = Tss.Esp1;

                    break;
                }

                case 2:
                {
                    if (!Fast486LoadSegment(State, FAST486_REG_SS, Tss.Ss2))
                    {
                        /* Exception occurred */
                        goto Cleanup;
                    }
                    State->GeneralRegs[FAST486_REG_ESP].Long = Tss.Esp2;

                    break;
                }

                default:
                {
                    /* Should never reach here! */
                    ASSERT(FALSE);
                }
            }

            /* Push SS selector */
            if (!Fast486StackPush(State, OldSs)) goto Cleanup;

            /* Push stack pointer */
            if (!Fast486StackPush(State, OldEsp)) goto Cleanup;
        }
    }
    else
    {
        if (State->SegmentRegs[FAST486_REG_CS].Size)
        {
            /* Set OPSIZE, because INT always pushes 16-bit values in real mode */
            State->PrefixFlags |= FAST486_PREFIX_OPSIZE;
        }
    }

    /* Push EFLAGS */
    if (!Fast486StackPush(State, State->Flags.Long)) goto Cleanup;

    /* Push CS selector */
    if (!Fast486StackPush(State, State->SegmentRegs[FAST486_REG_CS].Selector)) goto Cleanup;

    /* Push the instruction pointer */
    if (!Fast486StackPush(State, State->InstPtr.Long)) goto Cleanup;

    if ((GateType == FAST486_IDT_INT_GATE) || (GateType == FAST486_IDT_INT_GATE_32))
    {
        /* Disable interrupts after a jump to an interrupt gate handler */
        State->Flags.If = FALSE;
    }

    /* Load new CS */
    if (!Fast486LoadSegment(State, FAST486_REG_CS, SegmentSelector))
    {
        /* An exception occurred during the jump */
        goto Cleanup;
    }

    if (GateSize)
    {
        /* 32-bit code segment, use EIP */
        State->InstPtr.Long = Offset;
    }
    else
    {
        /* 16-bit code segment, use IP */
        State->InstPtr.LowWord = LOWORD(Offset);
    }

    Success = TRUE;

Cleanup:
    /* Restore the prefix flags */
    State->PrefixFlags = OldPrefixFlags;

    return Success;
}

BOOLEAN
FASTCALL
Fast486PerformInterrupt(PFAST486_STATE State,
                        UCHAR Number)
{
    FAST486_IDT_ENTRY IdtEntry;

    /* Get the interrupt vector */
    if (!Fast486GetIntVector(State, Number, &IdtEntry))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Perform the interrupt */
    if (!Fast486InterruptInternal(State, &IdtEntry))
    {
        /* Exception occurred */
        return FALSE;
    }

    return TRUE;
}

VOID
FASTCALL
Fast486ExceptionWithErrorCode(PFAST486_STATE State,
                              FAST486_EXCEPTIONS ExceptionCode,
                              ULONG ErrorCode)
{
    /* Increment the exception count */
    State->ExceptionCount++;

    /* Check if the exception occurred more than once */
    if (State->ExceptionCount > 1)
    {
        /* Then this is a double fault */
        ExceptionCode = FAST486_EXCEPTION_DF;
    }

    /* Check if this is a triple fault */
    if (State->ExceptionCount == 3)
    {
        DPRINT("Fast486ExceptionWithErrorCode(%04X:%08X) -- Triple fault\n",
               State->SegmentRegs[FAST486_REG_CS].Selector,
               State->InstPtr.Long);

        /* Reset the CPU */
        Fast486Reset(State);
        return;
    }

    /* Restore the IP to the saved IP */
    State->InstPtr = State->SavedInstPtr;

    /* Perform the interrupt */
    if (!Fast486PerformInterrupt(State, ExceptionCode))
    {
        /*
         * If this function failed, that means Fast486Exception
         * was called again, so just return in this case.
         */
        return;
    }

    if (EXCEPTION_HAS_ERROR_CODE(ExceptionCode)
        && (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE))
    {
        /* Push the error code */
        if (!Fast486StackPush(State, ErrorCode))
        {
            /*
             * If this function failed, that means Fast486Exception
             * was called again, so just return in this case.
             */
            return;
        }
    }

    /* Reset the exception count */
    State->ExceptionCount = 0;
}

BOOLEAN
FASTCALL
Fast486TaskSwitch(PFAST486_STATE State, FAST486_TASK_SWITCH_TYPE Type, USHORT Selector)
{
    ULONG NewTssAddress;
    ULONG NewTssLimit;
    FAST486_TSS OldTss;
    FAST486_TSS NewTss;
    FAST486_SYSTEM_DESCRIPTOR NewTssDescriptor;

    /* Read the old TSS */
    if (!Fast486ReadLinearMemory(State,
                                 State->TaskReg.Base,
                                 &OldTss,
                                 sizeof(OldTss)))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* If this is a task return, use the linked previous selector */
    if (Type == FAST486_TASK_RETURN) Selector = LOWORD(OldTss.Link);

    /* Make sure the entry exists in the GDT (not LDT!) */
    if ((GET_SEGMENT_INDEX(Selector) == 0)
        || (Selector & SEGMENT_TABLE_INDICATOR)
        || GET_SEGMENT_INDEX(Selector) >= (State->Gdtr.Size + 1))
    {
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, Selector);
        return FALSE;
    }

    /* Get the TSS descriptor from the GDT */
    if (!Fast486ReadLinearMemory(State,
                                 State->Gdtr.Address + GET_SEGMENT_INDEX(Selector),
                                 &NewTssDescriptor,
                                 sizeof(NewTssDescriptor)))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!NewTssDescriptor.Present)
    {
        /* Incoming task TSS not present */
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_NP, Selector);
        return FALSE;
    }

    /* Calculate the linear address of the new TSS */
    NewTssAddress = NewTssDescriptor.Base;
    NewTssAddress |= NewTssDescriptor.BaseMid << 16;
    NewTssAddress |= NewTssDescriptor.BaseHigh << 24;

    /* Calculate the limit of the new TSS */
    NewTssLimit = NewTssDescriptor.Limit | (NewTssDescriptor.LimitHigh << 16);
    if (NewTssDescriptor.Granularity) NewTssLimit <<= 12;

    if (NewTssLimit < sizeof(FAST486_TSS))
    {
        /* TSS limit too small */
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, Selector); 
    }

    /*
     * The incoming task shouldn't be busy if we're executing it as a
     * new task, and it should be busy if we're returning to it.
     */
    if (((NewTssDescriptor.Signature != FAST486_TSS_SIGNATURE)
        || (Type == FAST486_TASK_RETURN))
        && ((NewTssDescriptor.Signature != FAST486_BUSY_TSS_SIGNATURE)
        || (Type != FAST486_TASK_RETURN)))
    {
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector); 
        return FALSE;
    }

    /* Read the new TSS */
    if (!Fast486ReadLinearMemory(State,
                                 NewTssAddress,
                                 &NewTss,
                                 sizeof(NewTss)))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (Type != FAST486_TASK_CALL)
    {
        /* Clear the busy bit of the outgoing task */
        FAST486_SYSTEM_DESCRIPTOR OldTssDescriptor;

        if (!Fast486ReadLinearMemory(State,
                                     State->Gdtr.Address
                                     + GET_SEGMENT_INDEX(State->TaskReg.Selector),
                                     &OldTssDescriptor,
                                     sizeof(OldTssDescriptor)))
        {
            /* Exception occurred */
            return FALSE;
        }

        OldTssDescriptor.Signature = FAST486_TSS_SIGNATURE;

        if (!Fast486WriteLinearMemory(State,
                                      State->Gdtr.Address
                                      + GET_SEGMENT_INDEX(State->TaskReg.Selector),
                                      &OldTssDescriptor,
                                      sizeof(OldTssDescriptor)))
        {
            /* Exception occurred */
            return FALSE;
        }
    }
    else
    {
        /* Store the link */
        NewTss.Link = State->TaskReg.Selector;
    }

    /* Save the current task into the TSS */
    OldTss.Cr3 = State->ControlRegisters[FAST486_REG_CR3];
    OldTss.Eip = State->InstPtr.Long;
    OldTss.Eflags = State->Flags.Long;
    OldTss.Eax = State->GeneralRegs[FAST486_REG_EAX].Long;
    OldTss.Ecx = State->GeneralRegs[FAST486_REG_ECX].Long;
    OldTss.Edx = State->GeneralRegs[FAST486_REG_EDX].Long;
    OldTss.Ebx = State->GeneralRegs[FAST486_REG_EBX].Long;
    OldTss.Esp = State->GeneralRegs[FAST486_REG_ESP].Long;
    OldTss.Ebp = State->GeneralRegs[FAST486_REG_EBP].Long;
    OldTss.Esi = State->GeneralRegs[FAST486_REG_ESI].Long;
    OldTss.Edi = State->GeneralRegs[FAST486_REG_EDI].Long;
    OldTss.Es = State->SegmentRegs[FAST486_REG_ES].Selector;
    OldTss.Cs = State->SegmentRegs[FAST486_REG_CS].Selector;
    OldTss.Ss = State->SegmentRegs[FAST486_REG_SS].Selector;
    OldTss.Ds = State->SegmentRegs[FAST486_REG_DS].Selector;
    OldTss.Fs = State->SegmentRegs[FAST486_REG_FS].Selector;
    OldTss.Gs = State->SegmentRegs[FAST486_REG_GS].Selector;
    OldTss.Ldtr = State->Ldtr.Selector;

    /* Write back the old TSS */
    if (!Fast486WriteLinearMemory(State,
                                  State->TaskReg.Base,
                                  &OldTss,
                                  sizeof(OldTss)))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Mark the new task as busy */
    NewTssDescriptor.Signature = FAST486_BUSY_TSS_SIGNATURE;

    /* Write back the new TSS descriptor */
    if (!Fast486WriteLinearMemory(State,
                                  State->Gdtr.Address + GET_SEGMENT_INDEX(Selector),
                                  &NewTssDescriptor,
                                  sizeof(NewTssDescriptor)))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Set the task switch bit */
    State->ControlRegisters[FAST486_REG_CR0] |= FAST486_CR0_TS;

    /* Load the task register with the new values */
    State->TaskReg.Selector = Selector;
    State->TaskReg.Base = NewTssAddress;
    State->TaskReg.Limit = NewTssLimit;

    /* Change the page directory */
    State->ControlRegisters[FAST486_REG_CR3] = NewTss.Cr3;

    /* Flush the TLB */
    if (State->Tlb) RtlZeroMemory(State->Tlb, NUM_TLB_ENTRIES * sizeof(ULONG));

#ifndef FAST486_NO_PREFETCH
    /* Context switching invalidates the prefetch */
    State->PrefetchValid = FALSE;
#endif

    /* Load the registers */
    State->InstPtr.Long = State->SavedInstPtr.Long = NewTss.Eip;
    State->Flags.Long = NewTss.Eflags;
    State->GeneralRegs[FAST486_REG_EAX].Long = NewTss.Eax;
    State->GeneralRegs[FAST486_REG_ECX].Long = NewTss.Ecx;
    State->GeneralRegs[FAST486_REG_EDX].Long = NewTss.Edx;
    State->GeneralRegs[FAST486_REG_EBX].Long = NewTss.Ebx;
    State->GeneralRegs[FAST486_REG_ESP].Long = NewTss.Esp;
    State->GeneralRegs[FAST486_REG_EBP].Long = NewTss.Ebp;
    State->GeneralRegs[FAST486_REG_ESI].Long = NewTss.Esi;
    State->GeneralRegs[FAST486_REG_EDI].Long = NewTss.Edi;

    /* Set the NT flag if nesting */
    if (Type == FAST486_TASK_CALL) State->Flags.Nt = TRUE;

    if (GET_SEGMENT_INDEX(NewTss.Ldtr) != 0)
    {
        BOOLEAN Valid;
        FAST486_SYSTEM_DESCRIPTOR GdtEntry;

        if (NewTss.Ldtr & SEGMENT_TABLE_INDICATOR)
        {
            /* This selector doesn't point to the GDT */
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, NewTss.Ldtr);
            return FALSE;
        }

        if (!Fast486ReadDescriptorEntry(State,
                                        NewTss.Ldtr,
                                        &Valid,
                                        (PFAST486_GDT_ENTRY)&GdtEntry))
        {
            /* Exception occurred */
            return FALSE;
        }

        if (!Valid)
        {
            /* Invalid selector */
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, NewTss.Ldtr);
            return FALSE;
        }

        if (GdtEntry.Signature != FAST486_LDT_SIGNATURE)
        {
            /* This is not an LDT descriptor */
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, NewTss.Ldtr);
            return FALSE;
        }

        if (!GdtEntry.Present)
        {
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, NewTss.Ldtr);
            return FALSE;
        }

        /* Update the LDTR */
        State->Ldtr.Selector = NewTss.Ldtr;
        State->Ldtr.Base = GdtEntry.Base | (GdtEntry.BaseMid << 16) | (GdtEntry.BaseHigh << 24);
        State->Ldtr.Limit = GdtEntry.Limit | (GdtEntry.LimitHigh << 16);
        if (GdtEntry.Granularity) State->Ldtr.Limit <<= 12;
    }
    else
    {
        /* The LDT of this task is empty */
        RtlZeroMemory(&State->Ldtr, sizeof(State->Ldtr));
    }

    /* Load the new segments */
    if (!Fast486LoadSegmentInternal(State,
                                    FAST486_REG_CS,
                                    NewTss.Cs,
                                    FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (!Fast486LoadSegmentInternal(State,
                                    FAST486_REG_SS,
                                    NewTss.Ss,
                                    FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (!Fast486LoadSegmentInternal(State,
                                    FAST486_REG_ES,
                                    NewTss.Es,
                                    FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (!Fast486LoadSegmentInternal(State,
                                    FAST486_REG_DS,
                                    NewTss.Ds,
                                    FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (!Fast486LoadSegmentInternal(State,
                                    FAST486_REG_FS,
                                    NewTss.Fs,
                                    FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (!Fast486LoadSegmentInternal(State,
                                    FAST486_REG_GS,
                                    NewTss.Gs,
                                    FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    return TRUE;
}

/* EOF */
