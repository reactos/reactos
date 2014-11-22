/*
 * Fast486 386/486 CPU Emulation Library
 * common.inl
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

#include "common.h"
#include "fpu.h"

/* PUBLIC FUNCTIONS ***********************************************************/

#if defined (__GNUC__)
    #define CountLeadingZeros64(x) __builtin_clzll(x)

/*
#elif (_MSC_VER >= 1500) && defined(_WIN64)
    #define CountLeadingZeros64(x) __lzcnt64(x)
#elif (_MSC_VER >= 1500)
    #define CountLeadingZeros64(x) ((x) > 0xFFFFFFFFULL) ? __lzcnt((x) >> 32) \
                                                         : (__lzcnt(x) + 32)
*/

#else
    FORCEINLINE
    ULONG
    CountLeadingZeros64(ULONGLONG Value)
    {
        ULONG Count = 0;
        Value = ~Value;
        while ((LONGLONG)Value < 0)
        {
            Count++;
            Value <<= 1;
        }
        return Count;
    }
#endif

FORCEINLINE
INT
FASTCALL
Fast486GetCurrentPrivLevel(PFAST486_STATE State)
{
    /* Return the CPL, or 3 if we're in virtual 8086 mode */
    return (!State->Flags.Vm) ? State->Cpl : 3;
}

FORCEINLINE
ULONG
FASTCALL
Fast486GetPageTableEntry(PFAST486_STATE State,
                         ULONG VirtualAddress,
                         BOOLEAN MarkAsDirty)
{
    ULONG PdeIndex = GET_ADDR_PDE(VirtualAddress);
    ULONG PteIndex = GET_ADDR_PTE(VirtualAddress);
    FAST486_PAGE_DIR DirectoryEntry;
    FAST486_PAGE_TABLE TableEntry;
    ULONG PageDirectory = State->ControlRegisters[FAST486_REG_CR3];

    if ((State->Tlb != NULL)
        && (State->Tlb[VirtualAddress >> 12] != INVALID_TLB_FIELD))
    {
        /* Return the cached entry */
        return State->Tlb[VirtualAddress >> 12];
    }

    /* Read the directory entry */
    State->MemReadCallback(State,
                           PageDirectory + PdeIndex * sizeof(ULONG),
                           &DirectoryEntry.Value,
                           sizeof(DirectoryEntry));

    /* Make sure it is present */
    if (!DirectoryEntry.Present) return 0;

    /* Was the directory entry accessed before? */
    if (!DirectoryEntry.Accessed)
    {
        /* Well, it is now */
        DirectoryEntry.Accessed = TRUE;

        /* Write back the directory entry */
        State->MemWriteCallback(State,
                                PageDirectory + PdeIndex * sizeof(ULONG),
                                &DirectoryEntry.Value,
                                sizeof(DirectoryEntry));
    }

    /* Read the table entry */
    State->MemReadCallback(State,
                           (DirectoryEntry.TableAddress << 12)
                           + PteIndex * sizeof(ULONG),
                           &TableEntry.Value,
                           sizeof(TableEntry));

    /* Make sure it is present */
    if (!TableEntry.Present) return 0;

    /* Do we need to change any flags? */
    if (!TableEntry.Accessed || (MarkAsDirty && !TableEntry.Dirty))
    {
        /* Mark it as accessed and optionally dirty too */
        TableEntry.Accessed = TRUE;
        if (MarkAsDirty) TableEntry.Dirty = TRUE;

        /* Write back the table entry */
        State->MemWriteCallback(State,
                                (DirectoryEntry.TableAddress << 12)
                                + PteIndex * sizeof(ULONG),
                                &TableEntry.Value,
                                sizeof(TableEntry));
    }

    /*
     * The resulting permissions depend on the permissions
     * in the page directory table too
     */
    TableEntry.Writeable &= DirectoryEntry.Writeable;
    TableEntry.Usermode &= DirectoryEntry.Usermode;

    if (State->Tlb != NULL)
    {
        /* Set the TLB entry */
        State->Tlb[VirtualAddress >> 12] = TableEntry.Value;
    }

    /* Return the table entry */
    return TableEntry.Value;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486ReadLinearMemory(PFAST486_STATE State,
                        ULONG LinearAddress,
                        PVOID Buffer,
                        ULONG Size)
{
    /* Check if paging is enabled */
    if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PG)
    {
        ULONG Page;
        FAST486_PAGE_TABLE TableEntry;
        INT Cpl = Fast486GetCurrentPrivLevel(State);
        ULONG BufferOffset = 0;

        for (Page = PAGE_ALIGN(LinearAddress);
             Page <= PAGE_ALIGN(LinearAddress + Size - 1);
             Page += FAST486_PAGE_SIZE)
        {
            ULONG PageOffset = 0, PageLength = FAST486_PAGE_SIZE;

            /* Get the table entry */
            TableEntry.Value = Fast486GetPageTableEntry(State, Page, FALSE);

            /* Check if this is the first page */
            if (Page == PAGE_ALIGN(LinearAddress))
            {
                /* Start reading from the offset from the beginning of the page */
                PageOffset = PAGE_OFFSET(LinearAddress);
                PageLength -= PageOffset;
            }

            if (!TableEntry.Present || (!TableEntry.Usermode && (Cpl > 0)))
            {
                State->ControlRegisters[FAST486_REG_CR2] = Page + PageOffset;

                /* Exception */
                Fast486ExceptionWithErrorCode(State,
                                              FAST486_EXCEPTION_PF,
                                              TableEntry.Present | (State->Cpl ? 0x04 : 0));
                return FALSE;
            }

            /* Check if this is the last page */
            if (Page == PAGE_ALIGN(LinearAddress + Size - 1))
            {
                /* Read only a part of the page */
                PageLength = PAGE_OFFSET(LinearAddress + Size - 1) - PageOffset + 1;
            }

            /* Read the memory */
            State->MemReadCallback(State,
                                   (TableEntry.Address << 12) | PageOffset,
                                   (PVOID)((ULONG_PTR)Buffer + BufferOffset),
                                   PageLength);

            BufferOffset += PageLength;
        }
    }
    else
    {
        /* Read the memory */
        State->MemReadCallback(State, LinearAddress, Buffer, Size);
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486WriteLinearMemory(PFAST486_STATE State,
                         ULONG LinearAddress,
                         PVOID Buffer,
                         ULONG Size)
{
    /* Check if paging is enabled */
    if (State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PG)
    {
        ULONG Page;
        FAST486_PAGE_TABLE TableEntry;
        INT Cpl = Fast486GetCurrentPrivLevel(State);
        ULONG BufferOffset = 0;

        for (Page = PAGE_ALIGN(LinearAddress);
             Page <= PAGE_ALIGN(LinearAddress + Size - 1);
             Page += FAST486_PAGE_SIZE)
        {
            ULONG PageOffset = 0, PageLength = FAST486_PAGE_SIZE;

            /* Get the table entry */
            TableEntry.Value = Fast486GetPageTableEntry(State, Page, TRUE);

            /* Check if this is the first page */
            if (Page == PAGE_ALIGN(LinearAddress))
            {
                /* Start writing from the offset from the beginning of the page */
                PageOffset = PAGE_OFFSET(LinearAddress);
                PageLength -= PageOffset;
            }

            if ((!TableEntry.Present || (!TableEntry.Usermode && (Cpl > 0)))
                || ((State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_WP)
                && !TableEntry.Writeable))
            {
                State->ControlRegisters[FAST486_REG_CR2] = Page + PageOffset;

                /* Exception */
                Fast486ExceptionWithErrorCode(State,
                                              FAST486_EXCEPTION_PF,
                                              TableEntry.Present | 0x02 | (State->Cpl ? 0x04 : 0));
                return FALSE;
            }

            /* Check if this is the last page */
            if (Page == PAGE_ALIGN(LinearAddress + Size - 1))
            {
                /* Write only a part of the page */
                PageLength = PAGE_OFFSET(LinearAddress + Size - 1) - PageOffset + 1;
            }

            /* Write the memory */
            State->MemWriteCallback(State,
                                    (TableEntry.Address << 12) | PageOffset,
                                    (PVOID)((ULONG_PTR)Buffer + BufferOffset),
                                    PageLength);

            BufferOffset += PageLength;
        }
    }
    else
    {
        /* Write the memory */
        State->MemWriteCallback(State, LinearAddress, Buffer, Size);
    }

    return TRUE;
}

FORCEINLINE
VOID
FASTCALL
Fast486Exception(PFAST486_STATE State,
                 FAST486_EXCEPTIONS ExceptionCode)
{
    /* Call the internal function */
    Fast486ExceptionWithErrorCode(State, ExceptionCode, 0);
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486StackPush(PFAST486_STATE State,
                 ULONG Value)
{
    BOOLEAN Size = State->SegmentRegs[FAST486_REG_CS].Size;

    /* The OPSIZE prefix toggles the size */
    TOGGLE_OPSIZE(Size);

    if (Size)
    {
        /* 32-bit size */

        /* Check if ESP is between 1 and 3 */
        if (State->GeneralRegs[FAST486_REG_ESP].Long >= 1
            && State->GeneralRegs[FAST486_REG_ESP].Long <= 3)
        {
            Fast486Exception(State, FAST486_EXCEPTION_SS);
            return FALSE;
        }

        /* Store the value in SS:[ESP - 4] */
        if (!Fast486WriteMemory(State,
                                FAST486_REG_SS,
                                State->GeneralRegs[FAST486_REG_ESP].Long - sizeof(ULONG),
                                &Value,
                                sizeof(ULONG)))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Subtract ESP by 4 */
        State->GeneralRegs[FAST486_REG_ESP].Long -= sizeof(ULONG);
    }
    else
    {
        /* 16-bit size */
        USHORT ShortValue = LOWORD(Value);

        /* Check if SP is 1 */
        if (State->GeneralRegs[FAST486_REG_ESP].LowWord == 1)
        {
            Fast486Exception(State, FAST486_EXCEPTION_SS);
            return FALSE;
        }

        /* Store the value in SS:[SP - 2] */
        if (!Fast486WriteMemory(State,
                                FAST486_REG_SS,
                                LOWORD(State->GeneralRegs[FAST486_REG_ESP].LowWord - sizeof(USHORT)),
                                &ShortValue,
                                sizeof(USHORT)))
        {
            /* Exception occurred */
            return FALSE;
        }

        /* Subtract SP by 2 */
        State->GeneralRegs[FAST486_REG_ESP].LowWord -= sizeof(USHORT);
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486StackPop(PFAST486_STATE State,
                PULONG Value)
{
    BOOLEAN Size = State->SegmentRegs[FAST486_REG_CS].Size;

    /* The OPSIZE prefix toggles the size */
    TOGGLE_OPSIZE(Size);

    if (Size)
    {
        /* 32-bit size */
        ULONG LongValue;

        /* Check if ESP is 0xFFFFFFFF */
        if (State->GeneralRegs[FAST486_REG_ESP].Long == 0xFFFFFFFF)
        {
            Fast486Exception(State, FAST486_EXCEPTION_SS);
            return FALSE;
        }

        /* Read the value from SS:ESP */
        if (!Fast486ReadMemory(State,
                               FAST486_REG_SS,
                               State->GeneralRegs[FAST486_REG_ESP].Long,
                               FALSE,
                               &LongValue,
                               sizeof(LongValue)))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Increment ESP by 4 */
        State->GeneralRegs[FAST486_REG_ESP].Long += sizeof(ULONG);

        /* Store the value in the result */
        *Value = LongValue;
    }
    else
    {
        /* 16-bit size */
        USHORT ShortValue;

        /* Check if SP is 0xFFFF */
        if (State->GeneralRegs[FAST486_REG_ESP].LowWord == 0xFFFF)
        {
            Fast486Exception(State, FAST486_EXCEPTION_SS);
            return FALSE;
        }

        /* Read the value from SS:SP */
        if (!Fast486ReadMemory(State,
                               FAST486_REG_SS,
                               State->GeneralRegs[FAST486_REG_ESP].LowWord,
                               FALSE,
                               &ShortValue,
                               sizeof(ShortValue)))
        {
            /* An exception occurred */
            return FALSE;
        }

        /* Increment SP by 2 */
        State->GeneralRegs[FAST486_REG_ESP].LowWord += sizeof(USHORT);

        /* Store the value in the result */
        *Value = ShortValue;
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486ReadDescriptorEntry(PFAST486_STATE State,
                           USHORT Selector,
                           PBOOLEAN EntryValid,
                           PFAST486_GDT_ENTRY Entry)
{
    if (!(Selector & SEGMENT_TABLE_INDICATOR))
    {
        /* Make sure the GDT contains the entry */
        if (GET_SEGMENT_INDEX(Selector) >= (State->Gdtr.Size + 1))
        {
            *EntryValid = FALSE;
            return TRUE;
        }

        /* Read the GDT */
        if (!Fast486ReadLinearMemory(State,
                                     State->Gdtr.Address
                                     + GET_SEGMENT_INDEX(Selector),
                                     Entry,
                                     sizeof(*Entry)))
        {
            /* Exception occurred */
            *EntryValid = FALSE;
            return FALSE;
        }
    }
    else
    {
        /* Make sure the LDT contains the entry */
        if (GET_SEGMENT_INDEX(Selector) >= (State->Ldtr.Limit + 1))
        {
            *EntryValid = FALSE;
            return TRUE;
        }

        /* Read the LDT */
        if (!Fast486ReadLinearMemory(State,
                                     State->Ldtr.Base
                                     + GET_SEGMENT_INDEX(Selector),
                                     Entry,
                                     sizeof(*Entry)))
        {
            /* Exception occurred */
            *EntryValid = FALSE;
            return FALSE;
        }
    }

    *EntryValid = TRUE;
    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486LoadSegmentInternal(PFAST486_STATE State,
                           FAST486_SEG_REGS Segment,
                           USHORT Selector,
                           FAST486_EXCEPTIONS Exception)
{
    PFAST486_SEG_REG CachedDescriptor;
    BOOLEAN Valid;
    FAST486_GDT_ENTRY GdtEntry;

    ASSERT(Segment < FAST486_NUM_SEG_REGS);

    /* Get the cached descriptor */
    CachedDescriptor = &State->SegmentRegs[Segment];

    /* Check for protected mode */
    if ((State->ControlRegisters[FAST486_REG_CR0] & FAST486_CR0_PE) && !State->Flags.Vm)
    {
        if (!Fast486ReadDescriptorEntry(State, Selector, &Valid, &GdtEntry))
        {
            /* Exception occurred */
            return FALSE;
        }

        if (!Valid)
        {
            /* Invalid selector */
            Fast486ExceptionWithErrorCode(State, Exception, Selector);
            return FALSE;
        }

        if (Segment == FAST486_REG_SS)
        {
            /* Loading the stack segment */

            if (GET_SEGMENT_INDEX(Selector) == 0)
            {
                Fast486Exception(State, Exception);
                return FALSE;
            }

            if (!GdtEntry.SystemType)
            {
                /* This is a special descriptor */
                Fast486ExceptionWithErrorCode(State, Exception, Selector);
                return FALSE;
            }

            if (GdtEntry.Executable || !GdtEntry.ReadWrite)
            {
                Fast486ExceptionWithErrorCode(State, Exception, Selector);
                return FALSE;
            }

            if ((GET_SEGMENT_RPL(Selector) != Fast486GetCurrentPrivLevel(State))
                || (GET_SEGMENT_RPL(Selector) != GdtEntry.Dpl))
            {
                Fast486ExceptionWithErrorCode(State, Exception, Selector);
                return FALSE;
            }

            if (!GdtEntry.Present)
            {
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_SS, Selector);
                return FALSE;
            }
        }
        else if (Segment == FAST486_REG_CS)
        {
            /* Loading the code segment */

#ifndef FAST486_NO_PREFETCH
            /* Invalidate the prefetch */
            State->PrefetchValid = FALSE;
#endif

            if (GET_SEGMENT_INDEX(Selector) == 0)
            {
                Fast486Exception(State, Exception);
                return FALSE;
            }

            if (!GdtEntry.SystemType)
            {
                /* Must be a segment descriptor */
                Fast486ExceptionWithErrorCode(State, Exception, Selector);
                return FALSE;
            }

            if (!GdtEntry.Present)
            {
                Fast486ExceptionWithErrorCode(State, Exception, Selector);
                return FALSE;
            }

            if (!GdtEntry.Executable)
            {
                Fast486ExceptionWithErrorCode(State, Exception, Selector);
                return FALSE;
            }

            if (GdtEntry.DirConf)
            {
                /* Conforming Code Segment */

                if (GdtEntry.Dpl > Fast486GetCurrentPrivLevel(State))
                {
                    /* Must be accessed from lower-privileged code */
                    Fast486ExceptionWithErrorCode(State, Exception, Selector);
                    return FALSE;
                }
            }
            else
            {
                /* Regular code segment */

                if ((GET_SEGMENT_RPL(Selector) < Fast486GetCurrentPrivLevel(State)))
                {
                    Fast486ExceptionWithErrorCode(State, Exception, Selector);
                    return FALSE;
                }
            }
        }
        else
        {
            /* Loading a data segment */

            if (GET_SEGMENT_INDEX(Selector) != 0)
            {
                if (!GdtEntry.SystemType)
                {
                    /* This is a special descriptor */
                    Fast486ExceptionWithErrorCode(State, Exception, Selector);
                    return FALSE;
                }

                if ((GET_SEGMENT_RPL(Selector) > GdtEntry.Dpl)
                    || (Fast486GetCurrentPrivLevel(State) > GdtEntry.Dpl))
                {
                    Fast486ExceptionWithErrorCode(State, Exception, Selector);
                    return FALSE;
                }

                if (!GdtEntry.Present)
                {
                    Fast486ExceptionWithErrorCode(State, Exception, Selector);
                    return FALSE;
                }
            }
            else
            {
                /* This is a NULL selector */
                RtlZeroMemory(&GdtEntry, sizeof(GdtEntry));
            }
        }

        /* Update the cache entry */
        CachedDescriptor->Selector = Selector;
        CachedDescriptor->Base = GdtEntry.Base | (GdtEntry.BaseMid << 16) | (GdtEntry.BaseHigh << 24);
        CachedDescriptor->Limit = GdtEntry.Limit | (GdtEntry.LimitHigh << 16);
        CachedDescriptor->Accessed = GdtEntry.Accessed;
        CachedDescriptor->ReadWrite = GdtEntry.ReadWrite;
        CachedDescriptor->DirConf = GdtEntry.DirConf;
        CachedDescriptor->Executable = GdtEntry.Executable;
        CachedDescriptor->SystemType = GdtEntry.SystemType;
        CachedDescriptor->Rpl = GET_SEGMENT_RPL(Selector);
        CachedDescriptor->Dpl = GdtEntry.Dpl;
        CachedDescriptor->Present = GdtEntry.Present;
        CachedDescriptor->Size = GdtEntry.Size;

        /* Check for page granularity */
        if (GdtEntry.Granularity)
        {
            CachedDescriptor->Limit <<= 12;
            CachedDescriptor->Limit |= 0x00000FFF;
        }
    }
    else
    {
        /* Update the selector and base */
        CachedDescriptor->Selector = Selector;
        CachedDescriptor->Base = Selector << 4;
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486LoadSegment(PFAST486_STATE State,
                   FAST486_SEG_REGS Segment,
                   USHORT Selector)
{
    return Fast486LoadSegmentInternal(State,
                                      Segment,
                                      Selector,
                                      FAST486_EXCEPTION_GP);
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486ProcessGate(PFAST486_STATE State, USHORT Selector, ULONG Offset, BOOLEAN Call)
{
    BOOLEAN Valid;
    FAST486_SYSTEM_DESCRIPTOR Descriptor;

    if (!Fast486ReadDescriptorEntry(State,
                                    Selector,
                                    &Valid,
                                    (PFAST486_GDT_ENTRY)&Descriptor))
    {
        /* Exception occurred */
        return FALSE;
    }

    if (!Valid)
    {
        /* Invalid selector */
        Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
        return FALSE;
    }

    switch (Descriptor.Signature)
    {
        case FAST486_TASK_GATE_SIGNATURE:
        {
            Fast486TaskSwitch(State,
                              Call ? FAST486_TASK_CALL : FAST486_TASK_JUMP,
                              ((PFAST486_IDT_ENTRY)&Descriptor)->Selector);

            return FALSE;
        }

        case FAST486_TSS_SIGNATURE:
        {
            Fast486TaskSwitch(State,
                              Call ? FAST486_TASK_CALL : FAST486_TASK_JUMP,
                              Selector);

            return FALSE;
        }

        case FAST486_CALL_GATE_SIGNATURE:
        {
            // TODO: NOT IMPLEMENTED
            UNIMPLEMENTED;
        }

        default:
        {
            /* Security check for jumps and calls only */
            if (State->Cpl != Descriptor.Dpl)
            {
                Fast486ExceptionWithErrorCode(State, FAST486_EXCEPTION_GP, Selector);
                return FALSE;
            }

            return TRUE;
        }
    }
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486FetchByte(PFAST486_STATE State,
                 PUCHAR Data)
{
    PFAST486_SEG_REG CachedDescriptor;
    ULONG Offset;
#ifndef FAST486_NO_PREFETCH
    ULONG LinearAddress;
#endif

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[FAST486_REG_CS];

    Offset = (CachedDescriptor->Size) ? State->InstPtr.Long
                                      : State->InstPtr.LowWord;
#ifndef FAST486_NO_PREFETCH
    LinearAddress = CachedDescriptor->Base + Offset;

    if (State->PrefetchValid
        && (LinearAddress >= State->PrefetchAddress)
        && ((LinearAddress + sizeof(UCHAR)) <= (State->PrefetchAddress + FAST486_CACHE_SIZE)))
    {
        *Data = *(PUCHAR)&State->PrefetchCache[LinearAddress - State->PrefetchAddress];
    }
    else
#endif
    {
        /* Read from memory */
        if (!Fast486ReadMemory(State,
                               FAST486_REG_CS,
                               Offset,
                               TRUE,
                               Data,
                               sizeof(UCHAR)))
        {
            /* Exception occurred during instruction fetch */
            return FALSE;
        }
    }

    /* Advance the instruction pointer */
    if (CachedDescriptor->Size) State->InstPtr.Long++;
    else State->InstPtr.LowWord++;

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486FetchWord(PFAST486_STATE State,
                 PUSHORT Data)
{
    PFAST486_SEG_REG CachedDescriptor;
    ULONG Offset;
#ifndef FAST486_NO_PREFETCH
    ULONG LinearAddress;
#endif

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[FAST486_REG_CS];

    Offset = (CachedDescriptor->Size) ? State->InstPtr.Long
                                      : State->InstPtr.LowWord;

#ifndef FAST486_NO_PREFETCH
    LinearAddress = CachedDescriptor->Base + Offset;

    if (State->PrefetchValid
        && (LinearAddress >= State->PrefetchAddress)
        && ((LinearAddress + sizeof(USHORT)) <= (State->PrefetchAddress + FAST486_CACHE_SIZE)))
    {
        *Data = *(PUSHORT)&State->PrefetchCache[LinearAddress - State->PrefetchAddress];
    }
    else
#endif
    {
        /* Read from memory */
        // FIXME: Fix byte order on big-endian machines
        if (!Fast486ReadMemory(State,
                               FAST486_REG_CS,
                               Offset,
                               TRUE,
                               Data,
                               sizeof(USHORT)))
        {
            /* Exception occurred during instruction fetch */
            return FALSE;
        }
    }

    /* Advance the instruction pointer */
    if (CachedDescriptor->Size) State->InstPtr.Long += sizeof(USHORT);
    else State->InstPtr.LowWord += sizeof(USHORT);

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486FetchDword(PFAST486_STATE State,
                  PULONG Data)
{
    PFAST486_SEG_REG CachedDescriptor;
    ULONG Offset;
#ifndef FAST486_NO_PREFETCH
    ULONG LinearAddress;
#endif

    /* Get the cached descriptor of CS */
    CachedDescriptor = &State->SegmentRegs[FAST486_REG_CS];

    Offset = (CachedDescriptor->Size) ? State->InstPtr.Long
                                      : State->InstPtr.LowWord;

#ifndef FAST486_NO_PREFETCH
    LinearAddress = CachedDescriptor->Base + Offset;

    if (State->PrefetchValid
        && (LinearAddress >= State->PrefetchAddress)
        && ((LinearAddress + sizeof(ULONG)) <= (State->PrefetchAddress + FAST486_CACHE_SIZE)))
    {
        *Data = *(PULONG)&State->PrefetchCache[LinearAddress - State->PrefetchAddress];
    }
    else
#endif
    {
        /* Read from memory */
        // FIXME: Fix byte order on big-endian machines
        if (!Fast486ReadMemory(State,
                               FAST486_REG_CS,
                               Offset,
                               TRUE,
                               Data,
                               sizeof(ULONG)))
        {
            /* Exception occurred during instruction fetch */
            return FALSE;
        }
    }

    /* Advance the instruction pointer */
    if (CachedDescriptor->Size) State->InstPtr.Long += sizeof(ULONG);
    else State->InstPtr.LowWord += sizeof(ULONG);

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486CalculateParity(UCHAR Number)
{
    // See http://graphics.stanford.edu/~seander/bithacks.html#ParityLookupTable too...
    return (0x9669 >> ((Number & 0x0F) ^ (Number >> 4))) & 1;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486ParseModRegRm(PFAST486_STATE State,
                     BOOLEAN AddressSize,
                     PFAST486_MOD_REG_RM ModRegRm)
{
    UCHAR ModRmByte, Mode, RegMem;

    /* Fetch the MOD REG R/M byte */
    if (!Fast486FetchByte(State, &ModRmByte))
    {
        /* Exception occurred */
        return FALSE;
    }

    /* Unpack the mode and R/M */
    Mode = ModRmByte >> 6;
    RegMem = ModRmByte & 0x07;

    /* Set the register operand */
    ModRegRm->Register = (ModRmByte >> 3) & 0x07;

    /* Check the mode */
    if (Mode == 3)
    {
        /* The second operand is also a register */
        ModRegRm->Memory = FALSE;
        ModRegRm->SecondRegister = RegMem;

        /* Done parsing */
        return TRUE;
    }

    /* The second operand is memory */
    ModRegRm->Memory = TRUE;

    if (AddressSize)
    {
        if (RegMem == FAST486_REG_ESP)
        {
            UCHAR SibByte;
            ULONG Scale, Index, Base;

            /* Fetch the SIB byte */
            if (!Fast486FetchByte(State, &SibByte))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Unpack the scale, index and base */
            Scale = 1 << (SibByte >> 6);
            Index = (SibByte >> 3) & 0x07;
            if (Index != FAST486_REG_ESP) Index = State->GeneralRegs[Index].Long;
            else Index = 0;

            if (((SibByte & 0x07) != FAST486_REG_EBP) || (Mode != 0))
            {
                /* Use the register a base */
                Base = State->GeneralRegs[SibByte & 0x07].Long;
            }
            else
            {
                /* Fetch the base */
                if (!Fast486FetchDword(State, &Base))
                {
                    /* Exception occurred */
                    return FALSE;
                }
            }

            if ((SibByte & 0x07) == FAST486_REG_ESP)
            {
                /* Check if there is no segment override */
                if (!(State->PrefixFlags & FAST486_PREFIX_SEG))
                {
                    /* Add a SS: prefix */
                    State->PrefixFlags |= FAST486_PREFIX_SEG;
                    State->SegmentOverride = FAST486_REG_SS;
                }
            }

            /* Calculate the address */
            ModRegRm->MemoryAddress = Base + Index * Scale;
        }
        else if (RegMem == FAST486_REG_EBP)
        {
            if (Mode) ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBP].Long;
            else ModRegRm->MemoryAddress = 0;
        }
        else
        {
            /* Get the base from the register */
            ModRegRm->MemoryAddress = State->GeneralRegs[RegMem].Long;
        }

        /* Check if there is no segment override */
        if (!(State->PrefixFlags & FAST486_PREFIX_SEG))
        {
            /* Check if the default segment should be SS */
            if ((RegMem == FAST486_REG_EBP) && Mode)
            {
                /* Add a SS: prefix */
                State->PrefixFlags |= FAST486_PREFIX_SEG;
                State->SegmentOverride = FAST486_REG_SS;
            }
        }

        if (Mode == 1)
        {
            CHAR Offset;

            /* Fetch the byte */
            if (!Fast486FetchByte(State, (PUCHAR)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += (LONG)Offset;
        }
        else if ((Mode == 2) || ((Mode == 0) && (RegMem == FAST486_REG_EBP)))
        {
            LONG Offset;

            /* Fetch the dword */
            if (!Fast486FetchDword(State, (PULONG)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += Offset;
        }
    }
    else
    {
        /* Check the operand */
        switch (RegMem)
        {
            case 0:
            {
                /* [BX + SI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBX].LowWord
                                           + State->GeneralRegs[FAST486_REG_ESI].LowWord;
                break;
            }

            case 1:
            {
                /* [BX + DI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBX].LowWord
                                           + State->GeneralRegs[FAST486_REG_EDI].LowWord;
                break;
            }

            case 2:
            {
                /* SS:[BP + SI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBP].LowWord
                                           + State->GeneralRegs[FAST486_REG_ESI].LowWord;
                break;
            }

            case 3:
            {
                /* SS:[BP + DI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBP].LowWord
                                           + State->GeneralRegs[FAST486_REG_EDI].LowWord;
                break;
            }

            case 4:
            {
                /* [SI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_ESI].LowWord;
                break;
            }

            case 5:
            {
                /* [DI] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EDI].LowWord;
                break;
            }

            case 6:
            {
                if (Mode)
                {
                    /* [BP] */
                    ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBP].LowWord;
                }
                else
                {
                    /* [constant] (added later) */
                    ModRegRm->MemoryAddress = 0;
                }

                break;
            }

            case 7:
            {
                /* [BX] */
                ModRegRm->MemoryAddress = State->GeneralRegs[FAST486_REG_EBX].LowWord;
                break;
            }
        }

        /* Check if there is no segment override */
        if (!(State->PrefixFlags & FAST486_PREFIX_SEG))
        {
            /* Check if the default segment should be SS */
            if ((RegMem == 2) || (RegMem == 3) || ((RegMem == 6) && Mode))
            {
                /* Add a SS: prefix */
                State->PrefixFlags |= FAST486_PREFIX_SEG;
                State->SegmentOverride = FAST486_REG_SS;
            }
        }

        if (Mode == 1)
        {
            CHAR Offset;

            /* Fetch the byte */
            if (!Fast486FetchByte(State, (PUCHAR)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += (LONG)Offset;
        }
        else if ((Mode == 2) || ((Mode == 0) && (RegMem == 6)))
        {
            SHORT Offset;

            /* Fetch the word */
            if (!Fast486FetchWord(State, (PUSHORT)&Offset))
            {
                /* Exception occurred */
                return FALSE;
            }

            /* Add the signed offset to the address */
            ModRegRm->MemoryAddress += (LONG)Offset;
        }

        /* Clear the top 16 bits */
        ModRegRm->MemoryAddress &= 0x0000FFFF;
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486ReadModrmByteOperands(PFAST486_STATE State,
                             PFAST486_MOD_REG_RM ModRegRm,
                             PUCHAR RegValue,
                             PUCHAR RmValue)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (RegValue)
    {
        /* Get the register value */
        if (ModRegRm->Register & 0x04)
        {
            /* AH, CH, DH, BH */
            *RegValue = State->GeneralRegs[ModRegRm->Register & 0x03].HighByte;
        }
        else
        {
            /* AL, CL, DL, BL */
            *RegValue = State->GeneralRegs[ModRegRm->Register & 0x03].LowByte;
        }
    }

    if (RmValue)
    {
        if (!ModRegRm->Memory)
        {
            /* Get the second register value */
            if (ModRegRm->SecondRegister & 0x04)
            {
                /* AH, CH, DH, BH */
                *RmValue = State->GeneralRegs[ModRegRm->SecondRegister & 0x03].HighByte;
            }
            else
            {
                /* AL, CL, DL, BL */
                *RmValue = State->GeneralRegs[ModRegRm->SecondRegister & 0x03].LowByte;
            }
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read memory */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm->MemoryAddress,
                                   FALSE,
                                   RmValue,
                                   sizeof(UCHAR)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486ReadModrmWordOperands(PFAST486_STATE State,
                             PFAST486_MOD_REG_RM ModRegRm,
                             PUSHORT RegValue,
                             PUSHORT RmValue)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (RegValue)
    {
        /* Get the register value */
        *RegValue = State->GeneralRegs[ModRegRm->Register].LowWord;
    }

    if (RmValue)
    {
        if (!ModRegRm->Memory)
        {
            /* Get the second register value */
            *RmValue = State->GeneralRegs[ModRegRm->SecondRegister].LowWord;
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read memory */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm->MemoryAddress,
                                   FALSE,
                                   RmValue,
                                   sizeof(USHORT)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486ReadModrmDwordOperands(PFAST486_STATE State,
                              PFAST486_MOD_REG_RM ModRegRm,
                              PULONG RegValue,
                              PULONG RmValue)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (RegValue)
    {
        /* Get the register value */
        *RegValue = State->GeneralRegs[ModRegRm->Register].Long;
    }

    if (RmValue)
    {
        if (!ModRegRm->Memory)
        {
            /* Get the second register value */
            *RmValue = State->GeneralRegs[ModRegRm->SecondRegister].Long;
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Read memory */
            if (!Fast486ReadMemory(State,
                                   Segment,
                                   ModRegRm->MemoryAddress,
                                   FALSE,
                                   RmValue,
                                   sizeof(ULONG)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486WriteModrmByteOperands(PFAST486_STATE State,
                              PFAST486_MOD_REG_RM ModRegRm,
                              BOOLEAN WriteRegister,
                              UCHAR Value)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (WriteRegister)
    {
        /* Store the value in the register */
        if (ModRegRm->Register & 0x04)
        {
            /* AH, CH, DH, BH */
            State->GeneralRegs[ModRegRm->Register & 0x03].HighByte = Value;
        }
        else
        {
            /* AL, CL, DL, BL */
            State->GeneralRegs[ModRegRm->Register & 0x03].LowByte = Value;
        }
    }
    else
    {
        if (!ModRegRm->Memory)
        {
            /* Store the value in the second register */
            if (ModRegRm->SecondRegister & 0x04)
            {
                /* AH, CH, DH, BH */
                State->GeneralRegs[ModRegRm->SecondRegister & 0x03].HighByte = Value;
            }
            else
            {
                /* AL, CL, DL, BL */
                State->GeneralRegs[ModRegRm->SecondRegister & 0x03].LowByte = Value;
            }
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Fast486WriteMemory(State,
                                    Segment,
                                    ModRegRm->MemoryAddress,
                                    &Value,
                                    sizeof(UCHAR)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486WriteModrmWordOperands(PFAST486_STATE State,
                              PFAST486_MOD_REG_RM ModRegRm,
                              BOOLEAN WriteRegister,
                              USHORT Value)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (WriteRegister)
    {
        /* Store the value in the register */
        State->GeneralRegs[ModRegRm->Register].LowWord = Value;
    }
    else
    {
        if (!ModRegRm->Memory)
        {
            /* Store the value in the second register */
            State->GeneralRegs[ModRegRm->SecondRegister].LowWord = Value;
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Fast486WriteMemory(State,
                                    Segment,
                                    ModRegRm->MemoryAddress,
                                    &Value,
                                    sizeof(USHORT)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
FASTCALL
Fast486WriteModrmDwordOperands(PFAST486_STATE State,
                               PFAST486_MOD_REG_RM ModRegRm,
                               BOOLEAN WriteRegister,
                               ULONG Value)
{
    FAST486_SEG_REGS Segment = FAST486_REG_DS;

    if (WriteRegister)
    {
        /* Store the value in the register */
        State->GeneralRegs[ModRegRm->Register].Long = Value;
    }
    else
    {
        if (!ModRegRm->Memory)
        {
            /* Store the value in the second register */
            State->GeneralRegs[ModRegRm->SecondRegister].Long = Value;
        }
        else
        {
            /* Check for the segment override */
            if (State->PrefixFlags & FAST486_PREFIX_SEG)
            {
                /* Use the override segment instead */
                Segment = State->SegmentOverride;
            }

            /* Write memory */
            if (!Fast486WriteMemory(State,
                                    Segment,
                                    ModRegRm->MemoryAddress,
                                    &Value,
                                    sizeof(ULONG)))
            {
                /* Exception occurred */
                return FALSE;
            }
        }
    }

    return TRUE;
}

#ifndef FAST486_NO_FPU

FORCEINLINE
VOID
FASTCALL
Fast486FpuNormalize(PFAST486_STATE State,
                    PFAST486_FPU_DATA_REG Data)
{
    UINT LeadingZeros;

    if (FPU_IS_NORMALIZED(Data)) return;
    if (FPU_IS_ZERO(Data))
    {
        Data->Exponent = 0;
        return;
    }

    LeadingZeros = CountLeadingZeros64(Data->Mantissa);

    if (LeadingZeros < Data->Exponent)
    {
        Data->Mantissa <<= LeadingZeros;
        Data->Exponent -= LeadingZeros;
    }
    else
    {
        /* Make it denormalized */
        Data->Mantissa <<= Data->Exponent - 1;
        Data->Exponent = 1;

        /* Underflow */
        State->FpuStatus.Ue = TRUE;
    }
}

FORCEINLINE
USHORT
FASTCALL
Fast486GetValueTag(PFAST486_FPU_DATA_REG Data)
{
    if (FPU_IS_ZERO(Data)) return FPU_TAG_ZERO;
    else if (FPU_IS_NAN(Data)) return FPU_TAG_SPECIAL;
    else return FPU_TAG_VALID;
}

FORCEINLINE
VOID
FASTCALL
Fast486FpuPush(PFAST486_STATE State,
               PFAST486_FPU_DATA_REG Data)
{
    State->FpuStatus.Top--;

    if (FPU_GET_TAG(0) == FPU_TAG_EMPTY)
    {
        FPU_ST(0) = *Data;
        FPU_SET_TAG(0, Fast486GetValueTag(Data));
    }
    else State->FpuStatus.Ie = TRUE;
}

FORCEINLINE
VOID
FASTCALL
Fast486FpuPop(PFAST486_STATE State)
{
    if (FPU_GET_TAG(0) != FPU_TAG_EMPTY)
    {
        FPU_SET_TAG(0, FPU_TAG_EMPTY);
        State->FpuStatus.Top++;
    }
    else State->FpuStatus.Ie = TRUE;
}

#endif

/* EOF */
