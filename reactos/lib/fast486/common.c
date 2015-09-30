/*
 * Fast486 386/486 CPU Emulation Library
 * common.c
 *
 * Copyright (C) 2015 Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
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
FASTCALL
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

    if (InstFetch || CachedDescriptor->Executable || !CachedDescriptor->DirConf)
    {
        if ((Offset + Size - 1) > CachedDescriptor->Limit)
        {
            /* Read beyond limit */
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }
    }
    else
    {
        if (Offset < CachedDescriptor->Limit)
        {
            /* Read beyond limit */
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }
    }

    /* Check for protected mode */
    if ((State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE) && !State->Flags.Vm)
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

            if ((LinearAddress - State->PrefetchAddress + Size) >= FAST486_CACHE_SIZE)
            {
                /* We can't prefetch without possibly violating page permissions */
                State->PrefetchValid = FALSE;
                return Fast486ReadLinearMemory(State, LinearAddress, Buffer, Size, TRUE);
            }
        }

        /* Prefetch */
        if (Fast486ReadLinearMemory(State,
                                    State->PrefetchAddress,
                                    State->PrefetchCache,
                                    FAST486_CACHE_SIZE,
                                    TRUE))
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
        return Fast486ReadLinearMemory(State, LinearAddress, Buffer, Size, TRUE);
    }
}

BOOLEAN
FASTCALL
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

    if (CachedDescriptor->Executable || !CachedDescriptor->DirConf)
    {
        if ((Offset + Size - 1) > CachedDescriptor->Limit)
        {
            /* Write beyond limit */
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }
    }
    else
    {
        if (Offset < CachedDescriptor->Limit)
        {
            /* Read beyond limit */
            Fast486Exception(State, FAST486_EXCEPTION_GP);
            return FALSE;
        }
    }

    /* Check for protected mode */
    if ((State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE) && !State->Flags.Vm)
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
    return Fast486WriteLinearMemory(State, LinearAddress, Buffer, Size, TRUE);
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
                                     sizeof(*IdtEntry),
                                     FALSE))
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
                         PFAST486_IDT_ENTRY IdtEntry,
                         BOOLEAN PushErrorCode,
                         ULONG ErrorCode)
{
    BOOLEAN GateSize = (IdtEntry->Type == FAST486_IDT_INT_GATE_32) ||
                       (IdtEntry->Type == FAST486_IDT_TRAP_GATE_32);
    USHORT OldCs = State->SegmentRegs[FAST486_REG_CS].Selector;
    ULONG OldEip = State->InstPtr.Long;
    ULONG OldFlags = State->Flags.Long;
    UCHAR OldCpl = State->Cpl;

    /* Check for protected mode */
    if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE)
    {
        USHORT OldSs = State->SegmentRegs[FAST486_REG_SS].Selector;
        ULONG OldEsp = State->GeneralRegs[FAST486_REG_ESP].Long;
        BOOLEAN OldVm = State->Flags.Vm;

        if (IdtEntry->Type == FAST486_TASK_GATE_SIGNATURE)
        {
            /* Task call */
            if (!Fast486TaskSwitch(State, FAST486_TASK_CALL, IdtEntry->Selector))
            {
                /* Exception occurred */
                return FALSE;
            }

            goto Finish;
        }

        /* Check if the interrupt handler is more privileged or if we're in V86 mode */
        if ((OldCpl > GET_SEGMENT_RPL(IdtEntry->Selector)) || State->Flags.Vm)
        {
            FAST486_TSS Tss;
            PFAST486_LEGACY_TSS LegacyTss = (PFAST486_LEGACY_TSS)&Tss;
            USHORT NewSs;
            ULONG NewEsp;

            /* Read the TSS */
            if (!Fast486ReadLinearMemory(State,
                                         State->TaskReg.Base,
                                         &Tss,
                                         State->TaskReg.Modern
                                         ? sizeof(FAST486_TSS) : sizeof(FAST486_LEGACY_TSS),
                                         FALSE))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Switch to the new privilege level */
            State->Cpl = GET_SEGMENT_RPL(IdtEntry->Selector);

            /* Clear the VM flag */
            State->Flags.Vm = FALSE;

            /* Check the new (higher) privilege level */
            switch (State->Cpl)
            {
                case 0:
                {
                    if (State->TaskReg.Modern)
                    {
                        NewSs = Tss.Ss0;
                        NewEsp = Tss.Esp0;
                    }
                    else
                    {
                        NewSs = LegacyTss->Ss0;
                        NewEsp = LegacyTss->Sp0;
                    }

                    break;
                }

                case 1:
                {
                    if (State->TaskReg.Modern)
                    {
                        NewSs = Tss.Ss1;
                        NewEsp = Tss.Esp1;
                    }
                    else
                    {
                        NewSs = LegacyTss->Ss1;
                        NewEsp = LegacyTss->Sp1;
                    }

                    break;
                }

                case 2:
                {
                    if (State->TaskReg.Modern)
                    {
                        NewSs = Tss.Ss2;
                        NewEsp = Tss.Esp2;
                    }
                    else
                    {
                        NewSs = LegacyTss->Ss2;
                        NewEsp = LegacyTss->Sp2;
                    }

                    break;
                }

                default:
                {
                    /* Should never reach here! */
                    ASSERT(FALSE);
                }
            }

            if (!Fast486LoadSegment(State, FAST486_REG_SS, NewSs))
            {
                /* Exception occurred */
                return FALSE;
            }

            State->GeneralRegs[FAST486_REG_ESP].Long = NewEsp;
        }

        /* Load new CS */
        if (!Fast486LoadSegment(State, FAST486_REG_CS, IdtEntry->Selector))
        {
            /* An exception occurred during the jump */
            return FALSE;
        }

        if (GateSize)
        {
            /* 32-bit code segment, use EIP */
            State->InstPtr.Long = MAKELONG(IdtEntry->Offset, IdtEntry->OffsetHigh);
        }
        else
        {
            /* 16-bit code segment, use IP */
            State->InstPtr.LowWord = IdtEntry->Offset;
        }

        /* Clear NT */
        State->Flags.Nt = FALSE;

        if (OldVm)
        {
            /* Push GS, FS, DS and ES */
            if (!Fast486StackPushInternal(State,
                                          GateSize,
                                          State->SegmentRegs[FAST486_REG_GS].Selector))
            {
                return FALSE;
            }
            if (!Fast486StackPushInternal(State,
                                          GateSize,
                                          State->SegmentRegs[FAST486_REG_FS].Selector))
            {
                return FALSE;
            }
            if (!Fast486StackPushInternal(State,
                                          GateSize,
                                          State->SegmentRegs[FAST486_REG_DS].Selector))
            {
                return FALSE;
            }
            if (!Fast486StackPushInternal(State,
                                          GateSize,
                                          State->SegmentRegs[FAST486_REG_ES].Selector))
            {
                return FALSE;
            }

            /* Now load them with NULL selectors, since they are useless in protected mode */
            if (!Fast486LoadSegment(State, FAST486_REG_GS, 0)) return FALSE;
            if (!Fast486LoadSegment(State, FAST486_REG_FS, 0)) return FALSE;
            if (!Fast486LoadSegment(State, FAST486_REG_DS, 0)) return FALSE;
            if (!Fast486LoadSegment(State, FAST486_REG_ES, 0)) return FALSE;
        }

        /* Check if the interrupt handler is more privileged or we're in VM86 mode (again) */
        if ((OldCpl > GET_SEGMENT_RPL(IdtEntry->Selector)) || OldVm)
        {
            /* Push SS selector */
            if (!Fast486StackPushInternal(State, GateSize, OldSs)) return FALSE;

            /* Push the stack pointer */
            if (!Fast486StackPushInternal(State, GateSize, OldEsp)) return FALSE;
        }
    }
    else
    {
        /* Load new CS */
        if (!Fast486LoadSegment(State, FAST486_REG_CS, IdtEntry->Selector))
        {
            /* An exception occurred during the jump */
            return FALSE;
        }

        /* Set the new IP */
        State->InstPtr.LowWord = IdtEntry->Offset;
    }

    /* Push EFLAGS */
    if (!Fast486StackPushInternal(State, GateSize, OldFlags)) return FALSE;

    /* Push CS selector */
    if (!Fast486StackPushInternal(State, GateSize, OldCs)) return FALSE;

    /* Push the instruction pointer */
    if (!Fast486StackPushInternal(State, GateSize, OldEip)) return FALSE;

Finish:

    if (PushErrorCode)
    {
        /* Push the error code */
        if (!Fast486StackPushInternal(State, GateSize, ErrorCode)) return FALSE;
    }

    if ((IdtEntry->Type == FAST486_IDT_INT_GATE)
        || (IdtEntry->Type == FAST486_IDT_INT_GATE_32))
    {
        /* Disable interrupts after a jump to an interrupt gate handler */
        State->Flags.If = FALSE;
    }

    /* Clear TF */
    State->Flags.Tf = FALSE;

    return TRUE;
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
    if (!Fast486InterruptInternal(State, &IdtEntry, FALSE, 0))
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
    FAST486_IDT_ENTRY IdtEntry;

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

    /* Clear the prefix flags */
    State->PrefixFlags = 0;

    /* Restore the IP to the saved IP */
    State->InstPtr = State->SavedInstPtr;

    /* Restore the SP to the saved SP */
    State->GeneralRegs[FAST486_REG_ESP] = State->SavedStackPtr;

    /* Get the interrupt vector */
    if (!Fast486GetIntVector(State, ExceptionCode, &IdtEntry))
    {
        /*
         * If this function failed, that means Fast486Exception
         * was called again, so just return in this case.
         */
        return;
    }

    /* Perform the interrupt */
    if (!Fast486InterruptInternal(State,
                                  &IdtEntry,
                                  EXCEPTION_HAS_ERROR_CODE(ExceptionCode)
                                  && (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE),
                                  ErrorCode))
    {
        /*
         * If this function failed, that means Fast486Exception
         * was called again, so just return in this case.
         */
        return;
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
    FAST486_SYSTEM_DESCRIPTOR NewTssDescriptor;
    FAST486_TSS OldTss;
    PFAST486_LEGACY_TSS OldLegacyTss = (PFAST486_LEGACY_TSS)&OldTss;
    FAST486_TSS NewTss;
    PFAST486_LEGACY_TSS NewLegacyTss = (PFAST486_LEGACY_TSS)&NewTss;
    USHORT NewLdtr, NewEs, NewCs, NewSs, NewDs;

    if (State->TaskReg.Limit < (sizeof(FAST486_TSS) - 1)
        && State->TaskReg.Limit != (sizeof(FAST486_LEGACY_TSS) - 1))
    {
        /* Invalid task register limit */
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, State->TaskReg.Selector);
        return FALSE;
    }

    /* Read the old TSS */
    if (!Fast486ReadLinearMemory(State,
                                 State->TaskReg.Base,
                                 &OldTss,
                                 State->TaskReg.Modern
                                 ? sizeof(FAST486_TSS) : sizeof(FAST486_LEGACY_TSS),
                                 FALSE))
    {
        /* Exception occurred */
        return FALSE;
    }


    /* If this is a task return, use the linked previous selector */
    if (Type == FAST486_TASK_RETURN)
    {
        if (State->TaskReg.Modern) Selector = LOWORD(OldTss.Link);
        else Selector = OldLegacyTss->Link;
    }

    /* Make sure the entry exists in the GDT (not LDT!) */
    if ((GET_SEGMENT_INDEX(Selector) == 0)
        || (Selector & SEGMENT_TABLE_INDICATOR)
        || GET_SEGMENT_INDEX(Selector) >= (State->Gdtr.Size + 1u))
    {
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, Selector);
        return FALSE;
    }

    /* Get the TSS descriptor from the GDT */
    if (!Fast486ReadLinearMemory(State,
                                 State->Gdtr.Address + GET_SEGMENT_INDEX(Selector),
                                 &NewTssDescriptor,
                                 sizeof(NewTssDescriptor),
                                 FALSE))
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

    if (NewTssDescriptor.Granularity)
    {
        NewTssLimit <<= 12;
        NewTssLimit |= 0x00000FFF;
    }

    if (NewTssLimit < (sizeof(FAST486_TSS) - 1)
        && NewTssLimit != (sizeof(FAST486_LEGACY_TSS) - 1))
    {
        /* TSS limit invalid */
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, Selector);
        return FALSE;
    }

    /*
     * The incoming task shouldn't be busy if we're executing it as a
     * new task, and it should be busy if we're returning to it.
     */
    if ((((NewTssDescriptor.Signature != FAST486_TSS_SIGNATURE)
        && (NewTssDescriptor.Signature != FAST486_TSS_16_SIGNATURE))
        || (Type == FAST486_TASK_RETURN))
        && (((NewTssDescriptor.Signature != FAST486_BUSY_TSS_SIGNATURE)
        && (NewTssDescriptor.Signature != FAST486_BUSY_TSS_16_SIGNATURE))
        || (Type != FAST486_TASK_RETURN)))
    {
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
        return FALSE;
    }

    /* Read the new TSS */
    if (!Fast486ReadLinearMemory(State,
                                 NewTssAddress,
                                 &NewTss,
                                 (NewTssDescriptor.Signature == FAST486_TSS_SIGNATURE)
                                 || (NewTssDescriptor.Signature == FAST486_BUSY_TSS_SIGNATURE)
                                 ? sizeof(FAST486_TSS) : sizeof(FAST486_LEGACY_TSS),
                                 FALSE))
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
                                     sizeof(OldTssDescriptor),
                                     FALSE))
        {
            /* Exception occurred */
            return FALSE;
        }

        OldTssDescriptor.Signature = FAST486_TSS_SIGNATURE;

        if (!Fast486WriteLinearMemory(State,
                                      State->Gdtr.Address
                                      + GET_SEGMENT_INDEX(State->TaskReg.Selector),
                                      &OldTssDescriptor,
                                      sizeof(OldTssDescriptor),
                                      FALSE))
        {
            /* Exception occurred */
            return FALSE;
        }
    }
    else
    {
        /* Store the link */
        if ((NewTssDescriptor.Signature == FAST486_TSS_SIGNATURE)
            || (NewTssDescriptor.Signature == FAST486_BUSY_TSS_SIGNATURE))
        {
            NewTss.Link = State->TaskReg.Selector;

            /* Write back the new TSS link */
            if (!Fast486WriteLinearMemory(State,
                                          NewTssAddress,
                                          &NewTss.Link,
                                          sizeof(NewTss.Link),
                                          FALSE))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
        else
        {
            NewLegacyTss->Link = State->TaskReg.Selector;

            /* Write back the new legacy TSS link */
            if (!Fast486WriteLinearMemory(State,
                                          NewTssAddress,
                                          &NewLegacyTss->Link,
                                          sizeof(NewLegacyTss->Link),
                                          FALSE))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    /* Save the current task into the TSS */
    if (State->TaskReg.Modern)
    {
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
    }
    else
    {
        OldLegacyTss->Ip = State->InstPtr.LowWord;
        OldLegacyTss->Flags = State->Flags.LowWord;
        OldLegacyTss->Ax = State->GeneralRegs[FAST486_REG_EAX].LowWord;
        OldLegacyTss->Cx = State->GeneralRegs[FAST486_REG_ECX].LowWord;
        OldLegacyTss->Dx = State->GeneralRegs[FAST486_REG_EDX].LowWord;
        OldLegacyTss->Bx = State->GeneralRegs[FAST486_REG_EBX].LowWord;
        OldLegacyTss->Sp = State->GeneralRegs[FAST486_REG_ESP].LowWord;
        OldLegacyTss->Bp = State->GeneralRegs[FAST486_REG_EBP].LowWord;
        OldLegacyTss->Si = State->GeneralRegs[FAST486_REG_ESI].LowWord;
        OldLegacyTss->Di = State->GeneralRegs[FAST486_REG_EDI].LowWord;
        OldLegacyTss->Es = State->SegmentRegs[FAST486_REG_ES].Selector;
        OldLegacyTss->Cs = State->SegmentRegs[FAST486_REG_CS].Selector;
        OldLegacyTss->Ss = State->SegmentRegs[FAST486_REG_SS].Selector;
        OldLegacyTss->Ds = State->SegmentRegs[FAST486_REG_DS].Selector;
        OldLegacyTss->Ldtr = State->Ldtr.Selector;
    }

    /* Write back the old TSS */
    if (!Fast486WriteLinearMemory(State,
                                  State->TaskReg.Base,
                                  &OldTss,
                                  State->TaskReg.Modern
                                  ? sizeof(FAST486_TSS) : sizeof(FAST486_LEGACY_TSS),
                                  FALSE))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Mark the new task as busy */
    if (NewTssDescriptor.Signature == FAST486_TSS_SIGNATURE
        || NewTssDescriptor.Signature == FAST486_BUSY_TSS_SIGNATURE)
    {
        /* 32-bit TSS */
        NewTssDescriptor.Signature = FAST486_BUSY_TSS_SIGNATURE;
    }
    else
    {
        /* 16-bit TSS */
        NewTssDescriptor.Signature = FAST486_BUSY_TSS_16_SIGNATURE;
    }

    /* Write back the new TSS descriptor */
    if (!Fast486WriteLinearMemory(State,
                                  State->Gdtr.Address + GET_SEGMENT_INDEX(Selector),
                                  &NewTssDescriptor,
                                  sizeof(NewTssDescriptor),
                                  FALSE))
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

    if (NewTssDescriptor.Signature == FAST486_BUSY_TSS_SIGNATURE)
    {
        /* Change the page directory */
        State->ControlRegisters[FAST486_REG_CR3] = NewTss.Cr3;
    }

    /* Flush the TLB */
    Fast486FlushTlb(State);

    /* Update the CPL */
    if (NewTssDescriptor.Signature == FAST486_BUSY_TSS_SIGNATURE)
    {
        State->Cpl = GET_SEGMENT_RPL(NewTss.Cs);
    }
    else
    {
        State->Cpl = GET_SEGMENT_RPL(NewLegacyTss->Cs);
    }

#ifndef FAST486_NO_PREFETCH
    /* Context switching invalidates the prefetch */
    State->PrefetchValid = FALSE;
#endif

    /* Load the registers */
    if (NewTssDescriptor.Signature == FAST486_BUSY_TSS_SIGNATURE)
    {
        State->InstPtr.Long = State->SavedInstPtr.Long = NewTss.Eip;
        State->Flags.Long = NewTss.Eflags;
        State->GeneralRegs[FAST486_REG_EAX].Long = NewTss.Eax;
        State->GeneralRegs[FAST486_REG_ECX].Long = NewTss.Ecx;
        State->GeneralRegs[FAST486_REG_EDX].Long = NewTss.Edx;
        State->GeneralRegs[FAST486_REG_EBX].Long = NewTss.Ebx;
        State->GeneralRegs[FAST486_REG_EBP].Long = NewTss.Ebp;
        State->GeneralRegs[FAST486_REG_ESI].Long = NewTss.Esi;
        State->GeneralRegs[FAST486_REG_EDI].Long = NewTss.Edi;
        NewEs = NewTss.Es;
        NewCs = NewTss.Cs;
        NewDs = NewTss.Ds;
        NewLdtr = NewTss.Ldtr;

        if (Type == FAST486_TASK_CALL && State->Cpl < 3)
        {
            switch (State->Cpl)
            {
                case 0:
                {
                    State->GeneralRegs[FAST486_REG_ESP].Long = NewTss.Esp0;
                    NewSs = NewTss.Ss0;
                    break;
                }

                case 1:
                {
                    State->GeneralRegs[FAST486_REG_ESP].Long = NewTss.Esp1;
                    NewSs = NewTss.Ss1;
                    break;
                }

                case 2:
                {
                    State->GeneralRegs[FAST486_REG_ESP].Long = NewTss.Esp2;
                    NewSs = NewTss.Ss2;
                    break;
                }
            }
        }
        else
        {
            State->GeneralRegs[FAST486_REG_ESP].Long = NewTss.Esp;
            NewSs = NewTss.Ss;
        }
    }
    else
    {
        State->InstPtr.LowWord = State->SavedInstPtr.LowWord = NewLegacyTss->Ip;
        State->Flags.LowWord = NewLegacyTss->Flags;
        State->GeneralRegs[FAST486_REG_EAX].LowWord = NewLegacyTss->Ax;
        State->GeneralRegs[FAST486_REG_ECX].LowWord = NewLegacyTss->Cx;
        State->GeneralRegs[FAST486_REG_EDX].LowWord = NewLegacyTss->Dx;
        State->GeneralRegs[FAST486_REG_EBX].LowWord = NewLegacyTss->Bx;
        State->GeneralRegs[FAST486_REG_EBP].LowWord = NewLegacyTss->Bp;
        State->GeneralRegs[FAST486_REG_ESI].LowWord = NewLegacyTss->Si;
        State->GeneralRegs[FAST486_REG_EDI].LowWord = NewLegacyTss->Di;
        NewEs = NewLegacyTss->Es;
        NewCs = NewLegacyTss->Cs;
        NewDs = NewLegacyTss->Ds;
        NewLdtr = NewLegacyTss->Ldtr;

        if (Type == FAST486_TASK_CALL && State->Cpl < 3)
        {
            switch (State->Cpl)
            {
                case 0:
                {
                    State->GeneralRegs[FAST486_REG_ESP].Long = NewLegacyTss->Sp0;
                    NewSs = NewLegacyTss->Ss0;
                    break;
                }

                case 1:
                {
                    State->GeneralRegs[FAST486_REG_ESP].Long = NewLegacyTss->Sp1;
                    NewSs = NewLegacyTss->Ss1;
                    break;
                }

                case 2:
                {
                    State->GeneralRegs[FAST486_REG_ESP].Long = NewLegacyTss->Sp2;
                    NewSs = NewLegacyTss->Ss2;
                    break;
                }
            }
        }
        else
        {
            State->GeneralRegs[FAST486_REG_ESP].Long = NewLegacyTss->Sp;
            NewSs = NewLegacyTss->Ss;
        }
    }

    /* Set the NT flag if nesting */
    if (Type == FAST486_TASK_CALL) State->Flags.Nt = TRUE;

    if (GET_SEGMENT_INDEX(NewLdtr) != 0)
    {
        BOOLEAN Valid;
        FAST486_SYSTEM_DESCRIPTOR GdtEntry;

        if (NewLdtr & SEGMENT_TABLE_INDICATOR)
        {
            /* This selector doesn't point to the GDT */
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, NewLdtr);
            return FALSE;
        }

        if (!Fast486ReadDescriptorEntry(State, NewLdtr, &Valid, (PFAST486_GDT_ENTRY)&GdtEntry))
        {
            /* Exception occurred */
            return FALSE;
        }

        if (!Valid)
        {
            /* Invalid selector */
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, NewLdtr);
            return FALSE;
        }

        if (GdtEntry.Signature != FAST486_LDT_SIGNATURE)
        {
            /* This is not an LDT descriptor */
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, NewLdtr);
            return FALSE;
        }

        if (!GdtEntry.Present)
        {
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_TS, NewLdtr);
            return FALSE;
        }

        /* Update the LDTR */
        State->Ldtr.Selector = NewLdtr;
        State->Ldtr.Base = GdtEntry.Base | (GdtEntry.BaseMid << 16) | (GdtEntry.BaseHigh << 24);
        State->Ldtr.Limit = GdtEntry.Limit | (GdtEntry.LimitHigh << 16);

        if (GdtEntry.Granularity)
        {
            State->Ldtr.Limit <<= 12;
            State->Ldtr.Limit |= 0x00000FFF;
        }
    }
    else
    {
        /* The LDT of this task is empty */
        RtlZeroMemory(&State->Ldtr, sizeof(State->Ldtr));
    }

    /* Load the new segments */
    if (!Fast486LoadSegmentInternal(State, FAST486_REG_CS, NewCs, FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (!Fast486LoadSegmentInternal(State, FAST486_REG_SS, NewSs, FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (!Fast486LoadSegmentInternal(State, FAST486_REG_ES, NewEs, FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (!Fast486LoadSegmentInternal(State, FAST486_REG_DS, NewDs, FAST486_EXCEPTION_TS))
    {
        return FALSE;
    }

    if (NewTssDescriptor.Signature == FAST486_BUSY_TSS_SIGNATURE)
    {
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
    }

    return TRUE;
}

BOOLEAN
FASTCALL
Fast486CallGate(PFAST486_STATE State,
                PFAST486_CALL_GATE Gate,
                BOOLEAN Call)
{
    BOOLEAN Valid;
    FAST486_GDT_ENTRY NewCodeSegment;
    BOOLEAN GateSize = (Gate->Type == FAST486_CALL_GATE_SIGNATURE);
    FAST486_TSS Tss;
    PFAST486_LEGACY_TSS LegacyTss = (PFAST486_LEGACY_TSS)&Tss;
    USHORT OldCs = State->SegmentRegs[FAST486_REG_CS].Selector;
    ULONG OldEip = State->InstPtr.Long;
    USHORT OldCpl = State->Cpl;
    USHORT OldSs = State->SegmentRegs[FAST486_REG_SS].Selector;
    ULONG OldEsp = State->GeneralRegs[FAST486_REG_ESP].Long;
    ULONG ParamBuffer[32]; /* Maximum possible size - 32 DWORDs */
    PULONG LongParams = (PULONG)ParamBuffer;
    PUSHORT ShortParams = (PUSHORT)ParamBuffer;

    if (!Gate->Selector)
    {
        /* The code segment is NULL */
        Fast486Exception(State, FAST486_EXCEPTION_GP);
        return FALSE;
    }

    if (!Fast486ReadDescriptorEntry(State, Gate->Selector, &Valid, &NewCodeSegment))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Valid || (NewCodeSegment.Dpl > Fast486GetCurrentPrivLevel(State)))
    {
        /* Code segment invalid */
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Gate->Selector);
        return FALSE;
    }

    if (Call && Gate->ParamCount)
    {
        /* Read the parameters */
        if (!Fast486ReadMemory(State,
                               FAST486_REG_SS,
                               OldEsp,
                               FALSE,
                               ParamBuffer,
                               Gate->ParamCount * (GateSize ? sizeof(ULONG) : sizeof(USHORT))))
        {
            /* Exception occurred */
            return FALSE;
        }
    }

    /* Check if the new code segment is more privileged */
    if (NewCodeSegment.Dpl < OldCpl)
    {
        if (Call)
        {
            USHORT NewSs;
            ULONG NewEsp;

            /* Read the TSS */
            if (!Fast486ReadLinearMemory(State,
                                         State->TaskReg.Base,
                                         &Tss,
                                         State->TaskReg.Modern
                                         ? sizeof(FAST486_TSS) : sizeof(FAST486_LEGACY_TSS),
                                         FALSE))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Switch to the new privilege level */
            State->Cpl = NewCodeSegment.Dpl;

            /* Check the new (higher) privilege level */
            switch (State->Cpl)
            {
                case 0:
                {
                    if (State->TaskReg.Modern)
                    {
                        NewSs = Tss.Ss0;
                        NewEsp = Tss.Esp0;
                    }
                    else
                    {
                        NewSs = LegacyTss->Ss0;
                        NewEsp = LegacyTss->Sp0;
                    }

                    break;
                }

                case 1:
                {
                    if (State->TaskReg.Modern)
                    {
                        NewSs = Tss.Ss1;
                        NewEsp = Tss.Esp1;
                    }
                    else
                    {
                        NewSs = LegacyTss->Ss1;
                        NewEsp = LegacyTss->Sp1;
                    }

                    break;
                }

                case 2:
                {
                    if (State->TaskReg.Modern)
                    {
                        NewSs = Tss.Ss2;
                        NewEsp = Tss.Esp2;
                    }
                    else
                    {
                        NewSs = LegacyTss->Ss2;
                        NewEsp = LegacyTss->Sp2;
                    }

                    break;
                }

                default:
                {
                    /* Should never reach here! */
                    ASSERT(FALSE);
                }
            }

            if (!Fast486LoadSegment(State, FAST486_REG_SS, NewSs))
            {
                /* Exception occurred */
                return FALSE;
            }

            State->GeneralRegs[FAST486_REG_ESP].Long = NewEsp;
        }
        else if (!NewCodeSegment.DirConf)
        {
            /* This is not allowed for jumps */
            Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Gate->Selector);
            return FALSE;
        }
    }

    /* Load new CS */
    if (!Fast486LoadSegment(State, FAST486_REG_CS, Gate->Selector))
    {
        /* An exception occurred during the jump */
        return FALSE;
    }

    /* Set the instruction pointer */
    if (GateSize) State->InstPtr.Long = MAKELONG(Gate->Offset, Gate->OffsetHigh);
    else State->InstPtr.Long = Gate->Offset;

    if (Call)
    {
        INT i;

        /* Check if the new code segment is more privileged (again) */
        if (NewCodeSegment.Dpl < OldCpl)
        {
            /* Push SS selector */
            if (!Fast486StackPushInternal(State, GateSize, OldSs)) return FALSE;

            /* Push stack pointer */
            if (!Fast486StackPushInternal(State, GateSize, OldEsp)) return FALSE;
        }

        /* Push the parameters in reverse order */
        for (i = Gate->ParamCount - 1; i >= 0; i--)
        {
            if (!Fast486StackPushInternal(State,
                                          GateSize,
                                          GateSize ? LongParams[i] : ShortParams[i]))
            {
                /* Exception occurred */
                return FALSE;
            }
        }

        /* Push CS selector */
        if (!Fast486StackPushInternal(State, GateSize, OldCs)) return FALSE;

        /* Push the instruction pointer */
        if (!Fast486StackPushInternal(State, GateSize, OldEip)) return FALSE;
    }

    return TRUE;
}

/* EOF */
