/**
***  Copyright  (C) 1996-1999 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
***
****************************************************************************
***
*** WARNING: ntos\rtl\ia64\vunwind.c and sdktools\imagehlp\vwndia64.c are
***          identical. For sake of maintenance and for debug purposes, 
**           please keep them as this. Thank you.
***
****************************************************************************
**/

#define _CROSS_PLATFORM_
#define _IA64REG_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "private.h"
#include "ia64inst.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include <stdlib.h>

#ifdef _IMAGEHLP_SOURCE_

#define NOT_IMAGEHLP(E)
#define FUNCTION_ENTRY_IS_IMAGE_STYLE
#define RtlVirtualUnwind VirtualUnwindIa64
#define VUW_DEBUG_PRINT OutputDebugString

#else  // !_IMAGEHLP_SOURCE_

#define NOT_IMAGEHLP(E) E
#define VUW_DEBUG_PRINT DbgPrint

#endif // !_IMAGEHLP_SOURCE_

#ifdef MASK
#undef MASK
#endif // MASK
#define MASK(bp,value)  (value << bp)

//
// ABI values
//

#define SVR4_ABI      0
#define HPUX_ABI      1
#define NT_ABI        2


#ifdef KERNEL_DEBUGGER
#define FUNCTION_ENTRY_IS_IMAGE_STYLE
#define RtlVirtualUnwind VirtualUnwind
#endif

#define STATE_RECORD_STACK_SIZE 32

#define SPILLSIZE_OF_FLOAT128_IN_DWORDS   4
#define SPILLSIZE_OF_ULONGLONG_IN_DWORDS  2

#define REGISTER_SIZE                sizeof(ULONGLONG)
#define STATIC_REGISTER_SET_SIZE     32
#define SLOTS_PER_BUNDLE             3

#define R1_MASK              0xC0
#define R1_PREFIX            0x0
#define R1_REGION_TYPE_MASK  0x20
#define R1_LENGTH_MASK       0x1F

#define R2_MASK              0xE0
#define R2_PREFIX            0x40

#define R3_MASK              0xE0
#define R3_PREFIX            0x60
#define R3_REGION_TYPE_MASK  0x3

#define P1_MASK              0xE0
#define P1_PREFIX            0x80
#define P2_MASK              0xF0
#define P2_PREFIX            0xA0
#define P3_MASK              0xF8
#define P3_PREFIX            0xB0
#define P4_MASK              0xFF
#define P4_PREFIX            0xB8
#define P5_MASK              0xFF
#define P5_PREFIX            0xB9
#define P6_MASK              0xE0
#define P6_PREFIX            0xC0
#define P7_MASK              0xF0
#define P7_PREFIX            0xE0
#define P8_MASK              0xFF
#define P8_PREFIX            0xF0
#define P9_MASK              0xFF
#define P9_PREFIX            0xF1
#define P10_MASK             0xFF
#define P10_PREFIX           0xFF

#define B1_MASK              0xC0
#define B1_PREFIX            0x80
#define B1_TYPE_MASK         0x20
#define B1_LABEL_MASK        0x1F
#define B2_MASK              0xE0
#define B2_PREFIX            0xC0
#define B2_ECOUNT_MASK       0x1F
#define B3_MASK              0xF0
#define B3_PREFIX            0xE0
#define B4_MASK              0xF0
#define B4_PREFIX            0xF0
#define B4_TYPE_MASK         0x08

//
// P3 descriptor type
//

#define PSP_GR               0
#define RP_GR                1
#define PFS_GR               2
#define PREDS_GR             3
#define UNAT_GR              4
#define LC_GR                5
#define RP_BR                6
#define RNAT_GR              7
#define BSP_GR               8
#define BSPSTORE_GR          9
#define FPSR_GR              10
#define PRIUNAT_GR           11

//
// P7 descriptor type
//

#define MEM_STACK_F          0
#define MEM_STACK_V          1
#define SPILL_BASE           2
#define PSP_SPREL            3
#define RP_WHEN              4
#define RP_PSPREL            5
#define PFS_WHEN             6
#define PFS_PSPREL           7
#define PREDS_WHEN           8
#define PREDS_PSPREL         9
#define LC_WHEN              10
#define LC_PSPREL            11
#define UNAT_WHEN            12
#define UNAT_PSPREL          13
#define FPSR_WHEN            14
#define FPSR_PSPREL          15

//
// P8 descriptor type
//

#define PSP_PSPREL           0
#define RP_SPREL             1
#define PFS_SPREL            2
#define PREDS_SPREL          3
#define LC_SPREL             4
#define UNAT_SPREL           5
#define FPSR_SPREL           6
#define BSP_WHEN             7
#define BSP_PSPREL           8
#define BSP_SPREL            9
#define BSPSTORE_WHEN        10
#define BSPSTORE_PSPREL      11
#define BSPSTORE_SPREL       12
#define RNAT_WHEN            13
#define RNAT_PSPREL          14
#define RNAT_SPREL           15
#define PRIUNAT_WHEN         16
#define PRIUNAT_PSPREL       17
#define PRIUNAT_SPREL        18


#define STACK_POINTER_GR     12

#define FIRST_PRESERVED_GR                4
#define LAST_PRESERVED_GR                 7
#define NUMBER_OF_PRESERVED_GR            4

#define FIRST_LOW_PRESERVED_FR            2
#define LAST_LOW_PRESERVED_FR             5
#define NUMBER_OF_LOW_PRESERVED_FR        4

#define FIRST_HIGH_PRESERVED_FR           16
#define LAST_HIGH_PRESERVED_FR            31
#define NUMBER_OF_HIGH_PRESERVED_FR       16
#define NUMBER_OF_PRESERVED_FR            NUMBER_OF_LOW_PRESERVED_FR+NUMBER_OF_HIGH_PRESERVED_FR

#define FIRST_PRESERVED_BR                1
#define LAST_PRESERVED_BR                 5
#define NUMBER_OF_PRESERVED_BR            5

#define NUMBER_OF_PRESERVED_MISC          7

#define NUMBER_OF_PRESERVED_REGISTERS     12


#define REG_MISC_BASE        0
#define REG_PREDS            REG_MISC_BASE+0
#define REG_SP               REG_MISC_BASE+1
#define REG_PFS              REG_MISC_BASE+2
#define REG_RP               REG_MISC_BASE+3
#define REG_UNAT             REG_MISC_BASE+4
#define REG_LC               REG_MISC_BASE+5
#define REG_NATS             REG_MISC_BASE+6

#define REG_BR_BASE          REG_MISC_BASE+NUMBER_OF_PRESERVED_MISC

#define REG_FPSR             0xff // REG_MISC_BASE+7
#define REG_BSP              0xff // REG_MISC_BASE+8
#define REG_BSPSTORE         0xff // REG_MISC_BASE+9
#define REG_RNAT             0xff // REG_MISC_BASE+10

//
// Where is a preserved register saved?
//
//     1. stack general register
//     2. memory stack (pspoff)
//     3. memory stack (spoff)
//     4. branch register
//

#define GENERAL_REG          0
#define PSP_RELATIVE         1
#define SP_RELATIVE          2
#define BRANCH_REG           3


#define ADD_STATE_RECORD(States, RegionLength, DescBeginIndex)       \
    States.Top++;                                                    \
    States.Top->IsTarget = FALSE;                                    \
    States.Top->MiscMask = 0;                                        \
    States.Top->FrMask = 0;                                          \
    States.Top->GrMask = 0;                                          \
    States.Top->Label = (LABEL)0;                                    \
    States.Top->Ecount = 0;                                          \
    States.Top->RegionLen = RegionLength;                            \
    States.Top->RegionBegin = UnwindContext.SlotCount;               \
    States.Top->SpWhen = 0;                                          \
    States.Top->SpAdjustment = 0;                                    \
    States.Top->SpillBase = (States.Top-1)->SpillPtr;                \
    States.Top->SpillPtr = (States.Top-1)->SpillPtr;                 \
    States.Top->Previous = States.Current;                           \
    States.Top->DescBegin = DescBeginIndex;                          \
    States.Current = States.Top


#define VALID_LABEL_BIT_POSITION    15

#define LABEL_REGION(Region, Label)                                    \
    Region->Label = Label;                                             \
    Region->MiscMask |= (1 << VALID_LABEL_BIT_POSITION)

#define IS_REGION_LABELED(Region)  \
    (Region->MiscMask & (1 << VALID_LABEL_BIT_POSITION))

#define CHECK_LABEL(State, Label) \
    ( (IS_REGION_LABELED(State)) && (Label == State->Label) )


#define EXTRACT_NAT_FROM_UNAT(NatBit)  \
    NatBit = (UCHAR)((IntNats >> (((ULONG_PTR)Source & 0x1F8) >> 3)) & 0x1);


#if DBG
int UnwindDebugLevel = 0;
# ifdef _IMAGEHLP_SOURCE_
#  define UW_DEBUG(x) if (UnwindDebugLevel) dbPrint##x
# else
#  define UW_DEBUG(x) if (UnwindDebugLevel) DbgPrint##x
# endif
#else
# define UW_DEBUG(x)
#endif // DBG



typedef struct _REGISTER_RECORD {
    ULONG Where : 2;                  // 2-bit field
    ULONG SaveOffset : 30;            // 30 bits for offset, big enough?
    ULONG When;                       // slot offset relative to region
} REGISTER_RECORD, *PREGISTER_RECORD;

typedef ULONG LABEL;

typedef struct _STATE_RECORD {
    struct _STATE_RECORD *Previous;   // pointer to outer nested prologue
    BOOLEAN IsTarget;       // TRUE if the control pc is in this prologue
    UCHAR GrMask;           // Mask that specifies which GRs to be restored
    USHORT MiscMask;        // Mask that specifies which BRs and misc. registers
                            // are to be restored.
                            // N.B. MSBit indicates Label is valid or not.
    ULONG FrMask;           // Mask that specifies which FRs to be restored
    ULONG SpAdjustment;     // size of stack frame allocated in the prologue
    ULONG SpWhen;           // slot offset relative to region
    ULONG SpillPtr;         // current spill location
    ULONG SpillBase;        // spill base of the region
    ULONG RegionBegin;      // first slot of region relative to function entry
    ULONG RegionLen;        // number of slots in the region
    LABEL Label;            // label that identifies a post-prologue state
    ULONG Ecount;           // number of prologue regions to pop
    ULONG DescBegin;        // first prologue descriptor for the region
    ULONG DescEnd;          // last prologue descriptor for the region
} STATE_RECORD, *PSTATE_RECORD;

typedef struct _UNWIND_CONTEXT {
    REGISTER_RECORD MiscRegs[NUMBER_OF_PRESERVED_REGISTERS];
    REGISTER_RECORD Float[NUMBER_OF_PRESERVED_FR];
    REGISTER_RECORD Integer[NUMBER_OF_PRESERVED_GR];
    BOOLEAN ActiveRegionFound;
    USHORT Version;
    PUCHAR Descriptors;               // beginning of descriptor data
    ULONG Size;                       // total size of all descriptors
    ULONG DescCount;                  // number of descriptor bytes processed
    ULONG TargetSlot;
    ULONG SlotCount;
} UNWIND_CONTEXT, *PUNWIND_CONTEXT;

typedef struct _STATE_RECORD_STACK {
    ULONG Size;
    PSTATE_RECORD Current;
    PSTATE_RECORD Top;
    PSTATE_RECORD Base;
} STATE_RECORD_STACK, *PSTATE_RECORD_STACK;

#define OFFSET(type, field) ((ULONG_PTR)(&((type *)0)->field))

static USHORT MiscContextOffset[NUMBER_OF_PRESERVED_REGISTERS] = {
    OFFSET(IA64_CONTEXT, Preds),
    OFFSET(IA64_CONTEXT, IntSp),
    OFFSET(IA64_CONTEXT, RsPFS),
    OFFSET(IA64_CONTEXT, BrRp),
    OFFSET(IA64_CONTEXT, ApUNAT),
    OFFSET(IA64_CONTEXT, ApLC),
    0,
    OFFSET(IA64_CONTEXT, BrS0),
    OFFSET(IA64_CONTEXT, BrS1),
    OFFSET(IA64_CONTEXT, BrS2),
    OFFSET(IA64_CONTEXT, BrS3),
    OFFSET(IA64_CONTEXT, BrS4)
};

static USHORT MiscContextPointersOffset[NUMBER_OF_PRESERVED_REGISTERS] = {
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, Preds),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, IntSp),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, RsPFS),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, BrRp),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, ApUNAT),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, ApLC),
    0,
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, BrS0),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, BrS1),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, BrS2),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, BrS3),
    OFFSET(IA64_KNONVOLATILE_CONTEXT_POINTERS, BrS4)
};

static UCHAR P3RecordTypeToRegisterIndex[] =
    {REG_SP, REG_RP, REG_PFS, REG_PREDS, REG_UNAT, REG_LC, REG_RP};

static UCHAR P7RecordTypeToRegisterIndex[] =
    {0, REG_SP, 0, REG_SP, REG_RP, REG_RP, REG_PFS, REG_PFS, REG_PREDS,
     REG_PREDS, REG_LC, REG_LC, REG_UNAT, REG_UNAT, REG_FPSR, REG_FPSR};

static UCHAR P8RecordTypeToRegisterIndex[] =
    {REG_SP, REG_RP, REG_PFS, REG_PREDS, REG_LC, REG_UNAT, REG_FPSR,
     REG_BSP, REG_BSP, REG_BSP, REG_BSPSTORE, REG_BSPSTORE, REG_BSPSTORE,
     REG_RNAT, REG_RNAT, REG_RNAT, REG_NATS, REG_NATS, REG_NATS};

UCHAR
NewParsePrologueRegionPhase0 (
    IN PUNWIND_CONTEXT UwContext,
    IN PSTATE_RECORD StateRecord,
    IN OUT PUCHAR AbiImmContext
    );

VOID
NewParsePrologueRegionPhase1 (
    IN PUNWIND_CONTEXT UwContext,
    IN PSTATE_RECORD StateRecord
    );


VOID
SrInitialize (
    IN PSTATE_RECORD_STACK StateTable,
    IN PSTATE_RECORD StateRecord,
    IN ULONG Size
    )
{
    StateTable->Size = Size;
    StateTable->Base = StateRecord;
    StateTable->Top = StateRecord;
    StateTable->Current = StateRecord;
    RtlZeroMemory(StateTable->Top, sizeof(STATE_RECORD));
}


ULONG
ReadLEB128 (
    IN PUCHAR Descriptors,
    IN OUT PULONG CurrentDescIndex
    )
{
    PUCHAR Buffer;
    ULONG Value;
    ULONG ShiftCount = 7;
    ULONG Count;

    Buffer = Descriptors + *CurrentDescIndex;
    Count = 1;

    Value = Buffer[0] & 0x7F;
    if (Buffer[0] & 0x80) {
        while (TRUE) {
            Value += ((Buffer[Count] & 0x7F) << ShiftCount);
            if (Buffer[Count++] & 0x80) {
                ShiftCount += 7;
            } else {
                break;
            }
        }
    }

    *CurrentDescIndex += Count;

    return Value;
}


ULONGLONG
RestorePreservedRegisterFromGR (
    IN PIA64_CONTEXT Context,
    IN SHORT BsFrameSize,
    IN SHORT RNatSaveIndex,
    IN SHORT GrNumber,
#ifdef _IMAGEHLP_SOURCE_
    IN HANDLE hProcess,
    IN PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
    OUT BOOL *Succeed,
#else
    OUT ULONG64 *SourceAddress,
#endif // _IMAGEHLP_SOURCE_
    OUT PUCHAR Nat OPTIONAL
    )
{
    ULONGLONG Result;
    SHORT Offset;
    SHORT Temp;
#ifdef _IMAGEHLP_SOURCE_
    ULONG Size;
#endif // _IMAGEHLP_SOURCE_

#ifdef _IMAGEHLP_SOURCE_
    *Succeed = FALSE;
#endif // _IMAGEHLP_SOURCE_

    if (GrNumber >= STATIC_REGISTER_SET_SIZE) {

        Offset = GrNumber - STATIC_REGISTER_SET_SIZE;
        if ( Offset < BsFrameSize ) {

            Temp = Offset + RNatSaveIndex - IA64_NAT_BITS_PER_RNAT_REG;
            while (Temp >= 0) {
                Offset++;
                Temp -= IA64_NAT_BITS_PER_RNAT_REG;
            }
            Offset = Offset * sizeof(ULONGLONG);

#ifdef _IMAGEHLP_SOURCE_
            *Succeed = ReadMemory(hProcess, Context->RsBSP + Offset,
                                  &Result, sizeof(ULONGLONG), &Size);
#else
            *SourceAddress = (ULONG64)(Context->RsBSP + Offset);
            Result = *(PULONGLONG)(Context->RsBSP + Offset);
#endif // _IMAGEHLP_SOURCE_

        } else {

            UW_DEBUG(("ERROR: Invalid GR!\n"));
        }

    } else {

        if (GrNumber == 0 || GrNumber == 12) {

            //
            // Invalid GR number -> Invalid Unwind Descriptor
            //

            UW_DEBUG(("ERROR: Invalid GR!\n"));

        } else {

            UW_DEBUG(("WARNING: Target register is not a stacked GR!\n"));
            Offset = GrNumber - 1;
            NOT_IMAGEHLP(*SourceAddress = (ULONG64)(&Context->IntGp + Offset));
            Result = *(&Context->IntGp + Offset);

#ifdef _IMAGEHLP_SOURCE_
            *Succeed = TRUE;
#endif // _IMAGEHLP_SOURCE_

        }
    }

    if (ARGUMENT_PRESENT(Nat)) {

        //
        // TBD: Pick up the corresponding Nat bit
        //

        *Nat = (UCHAR) 0;

    }

    return (Result);
}


UCHAR
ParseBodyRegionDescriptors (
    IN PUNWIND_CONTEXT UnwindContext,
    IN PSTATE_RECORD_STACK StateTable,
    IN ULONG RegionLen
    )
{
    LABEL Label;
    UCHAR FirstByte;
    BOOLEAN EcountDefined;
    BOOLEAN CopyLabel;
    ULONG Ecount;
    ULONG SlotOffset;
    PSTATE_RECORD StateTablePtr;
    PUCHAR Descriptors;

    CopyLabel = EcountDefined = FALSE;
    Descriptors = UnwindContext->Descriptors;

    while (UnwindContext->DescCount < UnwindContext->Size) {

        FirstByte = Descriptors[UnwindContext->DescCount++];

        if ( (FirstByte & B1_MASK) == B1_PREFIX ) {

            Label = (LABEL)(FirstByte & B1_LABEL_MASK);
            if (FirstByte & B1_TYPE_MASK) {

                //
                // copy the entry state
                //

                CopyLabel = TRUE;

            } else {

                //
                // label the entry state
                //

                LABEL_REGION(StateTable->Top, Label);
            }

            UW_DEBUG(("Body region desc B1: copy=%d, label_num=%d\n",
                     FirstByte & B1_TYPE_MASK ? TRUE : FALSE, Label));

        } else if ( (FirstByte & B2_MASK) == B2_PREFIX ) {

            Ecount = FirstByte & B2_ECOUNT_MASK;
            SlotOffset = ReadLEB128(Descriptors, &UnwindContext->DescCount);
            EcountDefined = TRUE;

            UW_DEBUG(("Epilog desc B2: ecount=%d, LEB128(slot)=%d\n",
                      Ecount, SlotOffset));

        } else if ( (FirstByte & B3_MASK) == B3_PREFIX ) {

            SlotOffset = ReadLEB128(Descriptors, &UnwindContext->DescCount);
            Ecount = ReadLEB128(Descriptors, &UnwindContext->DescCount);
            EcountDefined = TRUE;

            UW_DEBUG(("Epilog desc B3: ecount=%d, LEB128 val=%d\n",
                      Ecount, SlotOffset));

        } else if ( (FirstByte & B4_MASK) == B4_PREFIX ) {

            Label = ReadLEB128(Descriptors, &UnwindContext->DescCount);

            if (FirstByte & B4_TYPE_MASK) {

                //
                // copy the entry state
                //

                CopyLabel = TRUE;

            } else {

                //
                // label the current top of stack
                //

                LABEL_REGION(StateTable->Top, Label);
            }

            UW_DEBUG(("Body region desc B4: copy=%d, label_num=%d\n",
                     FirstByte & B4_TYPE_MASK, Label));

        } else {

            //
            // Encounter another region header record
            //

            break;
        }
    }

    if (CopyLabel) {
        StateTablePtr = StateTable->Top;
        while (TRUE) {
            if (CHECK_LABEL(StateTablePtr, Label)) {
                StateTable->Current = StateTablePtr;
                break;
            } else if ((StateTablePtr == StateTable->Base)) {
                UW_DEBUG(("Undefined Label %d\n", Label));
                break;
            }
            StateTablePtr--;
        }
    }

    if (EcountDefined) {

        Ecount++;    // Ecount specifies additional level of prologue
                     // regions to undo (i.e. a value of 0 implies 1
                     // prologue region)

        if (UnwindContext->ActiveRegionFound == FALSE) {
            while (Ecount-- > 0) {
                if (StateTable->Current->Previous) {
                    StateTable->Current = StateTable->Current->Previous;
                }

#if DBG
                else {
                    UW_DEBUG(("WARNING: Ecount is greater than the # of active prologues!\n"));
                }
#endif // DBG

            }
        } else {

            //
            // control PC is in this body/epilog region
            //

            if ((UnwindContext->SlotCount + RegionLen - SlotOffset)
                    <= UnwindContext->TargetSlot)
            {
                PSTATE_RECORD SrPointer;

                StateTable->Current->Ecount = Ecount;
                SrPointer = StateTable->Current;
                while (Ecount > 0) {

                    if (SrPointer->Previous) {
                        SrPointer->Ecount = Ecount;
                        SrPointer->SpWhen = 0;
                        SrPointer->SpAdjustment = 0;
                        SrPointer = SrPointer->Previous;
                    }

#if DBG
                    else {
                        UW_DEBUG(("WARNING: Ecount is greater than the # of active prologues!\n"));
                    }
#endif // DBG
                    Ecount--;

                }
            }
        }
    }

    return FirstByte;
}


ULONGLONG
ProcessInterruptRegion (
#ifdef _IMAGEHLP_SOURCE_
    IN HANDLE hProcess,
    IN PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory,
#else
    IN PIA64_KNONVOLATILE_CONTEXT_POINTERS ContextPointers,
#endif _IMAGEHLP_SOURCE_
    IN PUNWIND_CONTEXT UnwindContext,
    IN PIA64_CONTEXT Context,
    IN SHORT BsFrameSize,
    IN SHORT RNatSaveIndex,
    IN UCHAR AbiImmContext
    )
{
    //
    // no prologue descriptor in interrupt region.
    //

    PIA64_CONTEXT PrevContext;
    ULONGLONG NextPc;
    ULONG Index;
    SHORT TempFrameSize;
    BOOLEAN Success;
#ifdef _IMAGEHLP_SOURCE_
    ULONG Size;
#else
    PVOID *Source;
    PVOID Address;
#endif _IMAGEHLP_SOURCE_


    if (AbiImmContext != IA64_CONTEXT_FRAME) {

        PIA64_KTRAP_FRAME TrapFrame;
        PIA64_KEXCEPTION_FRAME ExFrame;
#ifdef _IMAGEHLP_SOURCE_
        IA64_KTRAP_FRAME TF;
        IA64_KEXCEPTION_FRAME ExF;
#endif // _IMAGEHLP_SOURCE_

        TrapFrame = (PIA64_KTRAP_FRAME) Context->IntSp;
#ifdef _IMAGEHLP_SOURCE_
        if (!ReadMemory(hProcess, Context->IntSp, &TF, sizeof(IA64_KTRAP_FRAME), &Size))
        {
            return 0;
        }
        TrapFrame = &TF;
#endif // _IMAGEHLP_SOURCE_

        Context->ApDCR = TrapFrame->ApDCR;
        Context->ApUNAT = TrapFrame->ApUNAT;
        Context->StFPSR = TrapFrame->StFPSR;
        Context->Preds = TrapFrame->Preds;
        Context->IntSp = TrapFrame->IntSp;
        Context->StIPSR = TrapFrame->StIPSR;
        Context->StIFS = TrapFrame->StIFS;
        Context->BrRp = TrapFrame->BrRp;
        Context->RsPFS = TrapFrame->RsPFS;

#ifndef _IMAGEHLP_SOURCE_
        if (ARGUMENT_PRESENT(ContextPointers)) {
            ContextPointers->ApUNAT = &TrapFrame->ApUNAT;
            ContextPointers->IntSp = &TrapFrame->IntSp;
            ContextPointers->BrRp = &TrapFrame->BrRp;
            ContextPointers->RsPFS = &TrapFrame->RsPFS;
            ContextPointers->Preds = &TrapFrame->Preds;
        }
#endif // _IMAGEHLP_SOURCE_

        switch (AbiImmContext) {

        case IA64_SYSCALL_FRAME:

            //
            // System Call Handler Frame
            //

            BsFrameSize = (SHORT)(TrapFrame->StIFS >> IA64_PFS_SIZE_SHIFT);
            BsFrameSize &= IA64_PFS_SIZE_MASK;
            break;

        case IA64_INTERRUPT_FRAME:
        case IA64_EXCEPTION_FRAME:

            //
            // External Interrupt Frame / Exception Frame
            //

            BsFrameSize = (SHORT)TrapFrame->StIFS & IA64_PFS_SIZE_MASK;
            break;

        default:

            break;
        }

        RNatSaveIndex = (SHORT)(TrapFrame->RsBSP >> 3) & IA64_NAT_BITS_PER_RNAT_REG;
        TempFrameSize = BsFrameSize - RNatSaveIndex;
        while (TempFrameSize > 0) {
            BsFrameSize++;
            TempFrameSize -= IA64_NAT_BITS_PER_RNAT_REG;
        }

        Context->RsBSP = TrapFrame->RsBSP - BsFrameSize * sizeof(ULONGLONG);
        Context->RsBSPSTORE = Context->RsBSP;
        Context->RsRNAT = TrapFrame->RsRNAT;

        NextPc = Ia64InsertIPSlotNumber(TrapFrame->StIIP,
                     ((TrapFrame->StIPSR >> PSR_RI) & 0x3));

        return (NextPc);
    }

    //
    // Kernel-to-User thunk, context of the previous frame can be
    // found on the user stack (i.e. context's address = sp+SCRATCH_AREA)
    //

    PrevContext = (PIA64_CONTEXT)(Context->IntSp + IA64_STACK_SCRATCH_AREA);
#ifdef _IMAGEHLP_SOURCE_
    if (!ReadMemory(hProcess, (DWORD64)PrevContext, Context, sizeof(IA64_CONTEXT), &Size))
    {
        return 0;
    }
    NextPc = Ia64InsertIPSlotNumber(Context->StIIP,
                                       ((Context->StIPSR >> PSR_RI) & 0x3));
#else

    RtlCopyMemory(&Context->BrRp, &PrevContext->BrRp,
                  (NUMBER_OF_PRESERVED_BR+1) * sizeof(ULONGLONG));
    RtlCopyMemory(&Context->FltS0, &PrevContext->FltS0,
                  NUMBER_OF_LOW_PRESERVED_FR * sizeof(FLOAT128));
    RtlCopyMemory(&Context->FltS4, &PrevContext->FltS4,
                  NUMBER_OF_HIGH_PRESERVED_FR * sizeof(FLOAT128));
    RtlCopyMemory(&Context->IntS0, &PrevContext->IntS0,
                  NUMBER_OF_PRESERVED_GR * sizeof(ULONGLONG));

    Context->IntSp = PrevContext->IntSp;
    Context->IntNats = PrevContext->IntNats;
    Context->ApUNAT = PrevContext->ApUNAT;
    Context->ApLC = PrevContext->ApLC;
    Context->ApEC = PrevContext->ApEC;
    Context->Preds = PrevContext->Preds;
    Context->RsPFS = PrevContext->RsPFS;
    Context->RsBSP = PrevContext->RsBSP;
    Context->RsBSPSTORE = PrevContext->RsBSPSTORE;
    Context->RsRSC = PrevContext->RsRSC;
    Context->RsRNAT = PrevContext->RsRNAT;
    Context->StIFS = PrevContext->StIFS;
    Context->StIPSR = PrevContext->StIPSR;
    NextPc = Ia64InsertIPSlotNumber(PrevContext->StIIP,
                 ((PrevContext->StIPSR >> PSR_RI) & 0x3));

#endif // _IMAGEHLP_SOURCE_

    return(NextPc);
}


ULONGLONG
RtlVirtualUnwind (
#ifdef _IMAGEHLP_SOURCE_
    HANDLE hProcess,
    ULONGLONG ImageBase,
    ULONGLONG ControlPc,
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    PIA64_CONTEXT ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory
#define ContextPointers ((PIA64_KNONVOLATILE_CONTEXT_POINTERS)0)
#else
    IN ULONGLONG ImageBase,
    IN ULONGLONG ControlPc,
    IN PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    IN OUT PIA64_CONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PFRAME_POINTERS EstablisherFrame,
    IN OUT PIA64_KNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
#endif
    )

/*++

Routine Description:

    This function virtually unwinds the specfified function by executing its
    prologue code backwards.

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and an especially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is placed in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

Arguments:

    ImageBase - Supplies the base address of the module to which the
        function belongs.

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    InFunction - Supplies a pointer to a variable that receives whether the
        control PC is within the current function.

    EstablisherFrame - Supplies a pointer to a variable that receives the
        the establisher frame pointer value.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:

    The address where control left the previous frame is returned as the
    function value.

--*/

{
#ifdef _IMAGEHLP_SOURCE_
    BOOL Succeed;
#endif // _IMAGEHLP_SOURCE_
    PUCHAR Descriptors = NULL;
    UCHAR AbiImmContext = 0xFF;
    ULONG Mask;
    ULONGLONG NextPc;
    ULONG RegionLen;
    UCHAR FirstByte;
    UCHAR Nat;
    SHORT BsFrameSize;                  // in 8-byte units
    SHORT LocalFrameSize;                  // in 8-byte units
    SHORT TempFrameSize;                // in 8-byte units
    SHORT RNatSaveIndex;
    ULONG i;
    PULONG Buffer;
    BOOLEAN IsPrologueRegion;
    BOOLEAN PspRestored;
    ULONGLONG PreviousIntSp;
    PVOID Destination;
    ULONG64 Source;
    ULONG64 *CtxPtr;
    ULONG64 *NatCtxPtr;
    ULONG64 IntNatsSource;
    ULONG64 IntNats;
    ULONG Size;
    ULONGLONG OldTopRnat;
    ULONGLONG NewTopRnat;
    IA64_UNWIND_INFO UnwindInfo;
    ULONG64 UnwindInfoPtr;
    UNWIND_CONTEXT UnwindContext;
    PSTATE_RECORD SrPointer;
    STATE_RECORD_STACK StateTable;
    STATE_RECORD StateRecords[STATE_RECORD_STACK_SIZE];


    BsFrameSize = (SHORT)ContextRecord->StIFS & IA64_PFS_SIZE_MASK;
    RNatSaveIndex = (SHORT)(ContextRecord->RsBSP >> 3) & IA64_NAT_BITS_PER_RNAT_REG;
    TempFrameSize = RNatSaveIndex + BsFrameSize - IA64_NAT_BITS_PER_RNAT_REG;
    while (TempFrameSize >= 0) {
        BsFrameSize++;
        TempFrameSize -= IA64_NAT_BITS_PER_RNAT_REG;
    }

    UnwindInfoPtr = ImageBase + FunctionEntry->UnwindInfoAddress;
#ifdef _IMAGEHLP_SOURCE_
    if (!ReadMemory( hProcess, (ULONG64)UnwindInfoPtr,
                     &UnwindInfo, sizeof(IA64_UNWIND_INFO), &Size))
    {
        return 0;
    }

    UnwindContext.Version = UnwindInfo.Version;
    Size = UnwindInfo.DataLength * sizeof(ULONGLONG);
    if (Size) {
        Descriptors = (PUCHAR) MemAlloc (Size);
        if (!ReadMemory(hProcess,(ULONG64)(UnwindInfoPtr+sizeof(IA64_UNWIND_INFO)), Descriptors, Size, &Size)) {
            return 0;
        }
    }
#else
    UnwindContext.Version = ((PIA64_UNWIND_INFO)UnwindInfoPtr)->Version;
    Size = ((PIA64_UNWIND_INFO)UnwindInfoPtr)->DataLength * sizeof(ULONGLONG);
    Descriptors = (PUCHAR)UnwindInfoPtr + sizeof(IA64_UNWIND_INFO);
#endif // _IMAGEHLP_SOURCE_

    UnwindContext.Size = Size;
    UnwindContext.ActiveRegionFound = FALSE;
    UnwindContext.DescCount = 0;
    UnwindContext.SlotCount = 0;
    UnwindContext.TargetSlot = (ULONG)(((ControlPc - FunctionEntry->BeginAddress - ImageBase) >> 4) * SLOTS_PER_BUNDLE + ((ControlPc >> 2) & 0x3));
    UnwindContext.Descriptors = Descriptors;

    SrInitialize(&StateTable, StateRecords, STATE_RECORD_STACK_SIZE);

    if (Size) {
        FirstByte = Descriptors[UnwindContext.DescCount++];
    }


    while ( (UnwindContext.DescCount < UnwindContext.Size) &&
            (!UnwindContext.ActiveRegionFound) )
    {

        //
        // Assume a prologue region but not an interrupt region.
        //

        IsPrologueRegion = TRUE;

        //
        // Based on the type of region header, dispatch
        // to the corresponding routine that processes
        // the succeeding descriptors until the next
        // region header record.
        //

        if ((FirstByte & R1_MASK) == R1_PREFIX) {

            //
            // region header record in short format
            //

            RegionLen = FirstByte & R1_LENGTH_MASK;

            if (FirstByte & R1_REGION_TYPE_MASK) {
                IsPrologueRegion = FALSE;
            } else {
                ADD_STATE_RECORD(StateTable, RegionLen, UnwindContext.DescCount);
            }

            UW_DEBUG(("Region R1 format: body=%x, length=%d\n",
                     IsPrologueRegion ? 0 : 1, RegionLen));

        } else if ((FirstByte & R2_MASK) == R2_PREFIX) {

            //
            // general prologue region header
            // N.B. Skip the 2nd byte of the header and proceed to read
            //      the region length; the header descriptors will be
            //      processed again in phase 1.
            //

            ULONG R2DescIndex;

            R2DescIndex = UnwindContext.DescCount - 1;
            UnwindContext.DescCount++;
            RegionLen = ReadLEB128(Descriptors, &UnwindContext.DescCount);
            ADD_STATE_RECORD(StateTable, RegionLen, R2DescIndex);
            UW_DEBUG(("Region R2: body=0, length=%d\n", RegionLen));

        } else if ((FirstByte & R3_MASK) == R3_PREFIX) {

            //
            // region header record in long format
            //

            RegionLen = ReadLEB128(Descriptors, &UnwindContext.DescCount);

            switch (FirstByte & R3_REGION_TYPE_MASK) {

            case 0:      // prologue region header

                ADD_STATE_RECORD(StateTable, RegionLen, UnwindContext.DescCount);
                break;

            case 1:      // body region header

                IsPrologueRegion = FALSE;
                break;

            }

            UW_DEBUG(("Region R3: body=%x, length=%d\n",
                      IsPrologueRegion ? 0 : 1, RegionLen));

        } else {

            //
            // Not a region header record -> Invalid unwind descriptor.
            //

            UW_DEBUG(("Invalid unwind descriptor!\n"));

        }

        if (UnwindContext.TargetSlot < (UnwindContext.SlotCount + RegionLen)) {
            UnwindContext.ActiveRegionFound = TRUE;
            StateTable.Current->IsTarget = IsPrologueRegion;
        }

        if (IsPrologueRegion) {
            FirstByte = NewParsePrologueRegionPhase0(&UnwindContext,
                                                     StateTable.Current,
                                                     &AbiImmContext);
        } else {
            FirstByte = ParseBodyRegionDescriptors(&UnwindContext,
                                                   &StateTable,
                                                   RegionLen);
        }

        UnwindContext.SlotCount += RegionLen;
    }

    //
    // Restore the value of psp and save the current NatCr.
    // N.B. If the value is restored from stack/bstore, turn off the
    //      corresponding sp bit in the saved mask associated with the
    //      prologue region in which psp is saved.
    //

    if (ARGUMENT_PRESENT(ContextPointers)) {
        IntNatsSource = (ULONG64)ContextPointers->ApUNAT;
    } 
    IntNats = ContextRecord->ApUNAT;
    PreviousIntSp = ContextRecord->IntSp;
    PspRestored = FALSE;

    SrPointer = StateTable.Current;
    while (SrPointer != StateTable.Base) {
        NewParsePrologueRegionPhase1(&UnwindContext, SrPointer);

        if (SrPointer->MiscMask & (1 << REG_SP)) {
            if (UnwindContext.MiscRegs[REG_SP].Where == GENERAL_REG) {
                PreviousIntSp = RestorePreservedRegisterFromGR (
                                    ContextRecord,
                                    BsFrameSize,
                                    RNatSaveIndex,
                                    (SHORT)UnwindContext.MiscRegs[REG_SP].SaveOffset,
#ifdef _IMAGEHLP_SOURCE_
                                    hProcess,
                                    ReadMemory,
                                    &Succeed,
#else
                                    &Source,
#endif // _IMAGEHLP_SOURCE_
                                    &Nat
                                    );
#ifdef _IMAGEHLP_SOURCE_
                if (!Succeed) {
                    return 0;
                }
#endif // _IMAGEHLP_SOURCE_

            } else {

                Source = ContextRecord->IntSp + UnwindContext.MiscRegs[REG_SP].SaveOffset*4;
#ifdef _IMAGEHLP_SOURCE_
                if (!ReadMemory(hProcess, (ULONG64)(Source), &PreviousIntSp, sizeof(ULONGLONG), &Size)) {
                    return 0;
                }
#else
                PreviousIntSp = *(PULONGLONG)Source;
#endif // _IMAGEHLP_SOURCE_
                EXTRACT_NAT_FROM_UNAT(Nat);

            }
            ContextRecord->IntNats &= ~(0x1 << STACK_POINTER_GR);
            ContextRecord->IntNats |= (Nat << STACK_POINTER_GR);
            SrPointer->MiscMask &= ~(1 << REG_SP);
            if (ARGUMENT_PRESENT(ContextPointers)) {
                CtxPtr = (ULONG64 *)((ULONG_PTR)ContextPointers +
                                   MiscContextPointersOffset[REG_SP]);
                *CtxPtr = Source;
            }
            PspRestored = TRUE;
        }
        if (PspRestored == FALSE) {
            PreviousIntSp += SrPointer->SpAdjustment * 4;
        }
        SrPointer = SrPointer->Previous;
    }

    if (AbiImmContext != 0xFF) {

        ContextRecord->IntSp = PreviousIntSp;  // trap/context frame address
        NextPc = ProcessInterruptRegion(
#ifdef _IMAGEHLP_SOURCE_
                     hProcess,
                     ReadMemory,
#else
                     ContextPointers,
#endif _IMAGEHLP_SOURCE_
                     &UnwindContext,
                     ContextRecord,
                     BsFrameSize,
                     RNatSaveIndex,
                     AbiImmContext);

        goto FastExit;
    }

    //
    // Restore the contents of any preserved registers saved in this frame.
    //

    SrPointer = StateTable.Current;
    while (SrPointer != StateTable.Base) {

        Mask = SrPointer->MiscMask;
        UW_DEBUG(("MiscMask = 0x%x\n", Mask));

        for (i = 0; i < NUMBER_OF_PRESERVED_REGISTERS; i++) {
            Destination = (PVOID)((ULONG_PTR)ContextRecord + MiscContextOffset[i]);
            if (Mask & 0x1) {

                if (ARGUMENT_PRESENT(ContextPointers)) {
                    CtxPtr = (ULONG64 *)((ULONG_PTR)ContextPointers +
                                       MiscContextPointersOffset[i]);
                    Source = *CtxPtr;
                }

                if (UnwindContext.MiscRegs[i].Where == GENERAL_REG) {

                    *(PULONGLONG)Destination =
                        RestorePreservedRegisterFromGR (
                            ContextRecord,
                            BsFrameSize,
                            RNatSaveIndex,
                            (SHORT)UnwindContext.MiscRegs[i].SaveOffset,
#ifdef _IMAGEHLP_SOURCE_
                            hProcess,
                            ReadMemory,
                            &Succeed,
#else
                            &Source,
#endif // _IMAGEHLP_SOURCE_
                            NULL
                            );
#ifdef _IMAGEHLP_SOURCE_
                    if (!Succeed) {
                        return 0;
                    }
#endif // _IMAGEHLP_SOURCE_

                } else if (UnwindContext.MiscRegs[i].Where == BRANCH_REG) {

                    //
                    // restore return pointer from branch register
                    //

                    USHORT Offset;

                    Offset = (USHORT)UnwindContext.MiscRegs[i].SaveOffset-FIRST_PRESERVED_BR;
                    Source = (ULONG64)(&ContextRecord->BrS0 + Offset);
#ifdef _IMAGEHLP_SOURCE_
                    if (!ReadMemory(hProcess, (ULONG64)(Source), Destination, sizeof(ULONGLONG), &Size)) {
                        return 0;
                    }
#else
                    *(PULONGLONG)Destination = *(PULONGLONG)(Source);
#endif // _IMAGEHLP_SOURCE_

                } else if (UnwindContext.MiscRegs[i].Where == PSP_RELATIVE) {

                    if ((SrPointer->Ecount == 0) || (UnwindContext.MiscRegs[i].SaveOffset <= (IA64_STACK_SCRATCH_AREA/sizeof(ULONG)))) {
                        Source = PreviousIntSp + IA64_STACK_SCRATCH_AREA
                                     - UnwindContext.MiscRegs[i].SaveOffset*4;

                        if (i == REG_NATS) {
                            Destination = (PVOID)&IntNats;
                            IntNatsSource = Source;
                        }

#ifdef _IMAGEHLP_SOURCE_
                        if (!ReadMemory(hProcess, (ULONG64)(Source), Destination, sizeof(ULONGLONG), &Size)) {
                            return 0;
                        }
#else
                        *(PULONGLONG)Destination = *(PULONGLONG)(Source);
#endif // _IMAGEHLP_SOURCE_
                    }

                } else if (UnwindContext.MiscRegs[i].Where == SP_RELATIVE) {

                    //
                    // Make the necessary adjustment depending on whether
                    // the preserved register is saved before or after the
                    // stack pointer has been adjusted in this prologue.
                    //

                    if (UnwindContext.MiscRegs[i].When >= SrPointer->SpWhen)
                        Source = ContextRecord->IntSp
                                     + UnwindContext.MiscRegs[i].SaveOffset*4;
                    else
                        Source = ContextRecord->IntSp+SrPointer->SpAdjustment*4
                                     + UnwindContext.MiscRegs[i].SaveOffset*4;

                        if (i == REG_NATS) {
                            Destination = (PVOID)&IntNats;
                            IntNatsSource = Source;
                        }

#ifdef _IMAGEHLP_SOURCE_
                    if (!ReadMemory(hProcess, (ULONG64)(Source), Destination, sizeof(ULONGLONG), &Size)) {
                        return 0;
                    }
#else
                    *(PULONGLONG)Destination = *(PULONGLONG)(Source);
#endif // _IMAGEHLP_SOURCE_
                }

                if (ARGUMENT_PRESENT(ContextPointers) && (i != REG_NATS)) {
                    *CtxPtr = Source;
                }

            } else if (Mask == 0) {

                //
                // No more registers to restore
                //

                break;
            }

            Mask = Mask >> 1;
        }

        //
        // Restore preserved FRs (f2 - f5, f16 - f31)
        //

        Mask = SrPointer->FrMask;
        Destination = (PVOID)&ContextRecord->FltS0;
        CtxPtr = (ULONG64 *)&ContextPointers->FltS0;

        UW_DEBUG(("FrMask = 0x%x\n", Mask));
        for (i = 0; i < NUMBER_OF_PRESERVED_FR; i++) {
            if (Mask & 0x1) {

                if ((SrPointer->Ecount == 0) || (UnwindContext.Float[i].SaveOffset <= (IA64_STACK_SCRATCH_AREA/sizeof(ULONG)))) {
                    Source = PreviousIntSp + IA64_STACK_SCRATCH_AREA
                                 - UnwindContext.Float[i].SaveOffset*4;
#ifdef _IMAGEHLP_SOURCE_
                    if (!ReadMemory(hProcess, (ULONG64)(Source), Destination, sizeof(FLOAT128), &Size)) {
                        return 0;
                    }
#else
                    *(FLOAT128 *)Destination = *(FLOAT128 *)Source;
#endif // _IMAGEHLP_SOURCE_

                    if (ARGUMENT_PRESENT(ContextPointers)) {
                        *CtxPtr = Source;
                    }
                }

            } else if (Mask == 0) {
                break;
            }

            Mask = Mask >> 1;

            if (i == (NUMBER_OF_LOW_PRESERVED_FR - 1)) {
                Destination = (PVOID)&ContextRecord->FltS4;
                CtxPtr = (ULONG64 *)(&ContextPointers->FltS4);
            } else {
                Destination = (PVOID)((FLOAT128 *)Destination+1);
                CtxPtr++;
            }
        }

        //
        // Restore preserved GRs (r4 - r7)
        //

        Mask = SrPointer->GrMask;
        Destination = (PVOID)&ContextRecord->IntS0;
        CtxPtr = (ULONG64 *)&ContextPointers->IntS0;
        NatCtxPtr = (ULONG64 *)&ContextPointers->IntS0Nat;

        UW_DEBUG(("GrMask = 0x%x\n", Mask));
        for (i = 0; i < NUMBER_OF_PRESERVED_GR; i++)
        {
            if (Mask & 0x1) {

                if ((SrPointer->Ecount == 0) || (UnwindContext.Integer[i].SaveOffset <= (IA64_STACK_SCRATCH_AREA/sizeof(ULONG)))) {
                    Source = PreviousIntSp + IA64_STACK_SCRATCH_AREA
                                 - UnwindContext.Integer[i].SaveOffset*4;

#ifdef _IMAGEHLP_SOURCE_
                    if (!ReadMemory(hProcess, (ULONG64)(Source), Destination, sizeof(ULONGLONG), &Size)) {
                        return 0;
                    }
#else
                    *(PULONGLONG)Destination = *(PULONGLONG)Source;
#endif // _IMAGEHLP_SOURCE_
                    EXTRACT_NAT_FROM_UNAT(Nat);
                    Nat = (UCHAR)((IntNats >> (((ULONG_PTR)Source & 0x1F8) >> 3)) & 0x1);
                    ContextRecord->IntNats &= ~(0x1 << (i+FIRST_PRESERVED_GR));
                    ContextRecord->IntNats |= (Nat << (i+FIRST_PRESERVED_GR));

#ifndef _IMAGEHLP_SOURCE_
                    if (ARGUMENT_PRESENT(ContextPointers)) {
                        *CtxPtr = Source;
                        *NatCtxPtr = IntNatsSource;
                    }
#endif
                }

            } else if (Mask == 0) {
                break;
            }

            Mask = Mask >> 1;
            Destination = (PVOID)((PULONGLONG)Destination+1);
            CtxPtr++;
            NatCtxPtr++;
        }

        ContextRecord->IntSp += SrPointer->SpAdjustment * 4;
        SrPointer = SrPointer->Previous;
    }

    ContextRecord->IntSp = PreviousIntSp;

    //
    // Restore the value of the epilogue count from the PFS
    //

    ContextRecord->ApEC = (ContextRecord->RsPFS >> IA64_PFS_EC_SHIFT) &
                               ~(((ULONGLONG)1 << IA64_PFS_EC_SIZE) - 1);
    if (ARGUMENT_PRESENT(ContextPointers)) {
        ContextPointers->ApEC = ContextPointers->RsPFS;
    }


FastExit:

    NOT_IMAGEHLP(*InFunction = TRUE);
    NOT_IMAGEHLP(EstablisherFrame->MemoryStackFp = ContextRecord->IntSp);
    NOT_IMAGEHLP(EstablisherFrame->BackingStoreFp = ContextRecord->RsBSP);

#ifdef _IMAGEHLP_SOURCE_
    if (Descriptors)
        MemFree(Descriptors);
#endif // _IMAGEHLP_SOURCE_

    if (AbiImmContext == 0xFF) {

#ifdef _IMAGEHLP_SOURCE_
        NextPc = ContextRecord->BrRp;
#else
        NextPc = Ia64InsertIPSlotNumber((ContextRecord->BrRp-16), 2);
#endif // _IMAGEHLP_SOURCE_

        //
        // determine the local frame size of previous frame and compute
        // the new bsp.
        //

        OldTopRnat = (ContextRecord->RsBSP+(BsFrameSize-1)*8) | IA64_RNAT_ALIGNMENT;

        ContextRecord->StIFS = MASK(IA64_IFS_V, (ULONGLONG)1) | ContextRecord->RsPFS;
        BsFrameSize = (SHORT)ContextRecord->StIFS & IA64_PFS_SIZE_MASK;
        LocalFrameSize = (SHORT)(ContextRecord->StIFS >> IA64_PFS_SIZE_SHIFT) & IA64_PFS_SIZE_MASK;
        TempFrameSize = LocalFrameSize - RNatSaveIndex;
        while (TempFrameSize > 0) {
            LocalFrameSize++;
            BsFrameSize++;
            TempFrameSize -= IA64_NAT_BITS_PER_RNAT_REG;
        }
        ContextRecord->RsBSP -= LocalFrameSize * 8;
        ContextRecord->RsBSPSTORE = ContextRecord->RsBSP;

        //
        // determine if the RNAT field needs to be updated.
        //

        NewTopRnat = (ContextRecord->RsBSP+(BsFrameSize-1)*8) | IA64_RNAT_ALIGNMENT;

        if (NewTopRnat < OldTopRnat) {

#ifdef _IMAGEHLP_SOURCE_
            Destination = &ContextRecord->RsRNAT;
            Source = NewTopRnat;
            if (!ReadMemory(hProcess, (ULONG64)Source, Destination, 8, &Size)) {
                return 0;
            }
#else
            ContextRecord->RsRNAT = *(PULONGLONG)(NewTopRnat);
#endif // _IMAGEHLP_SOURCE_

        }
    }

#ifdef _IMAGEHLP_SOURCE_
    UW_DEBUG(("NextPc = 0x%lx, PSP = 0x%lx, BSP = 0x%lx\n",
               (ULONGLONG)NextPc,
               (ULONGLONG)ContextRecord->IntSp,
               (ULONGLONG)ContextRecord->RsBSP));
#else
    UW_DEBUG(("NextPc = 0x%lx, PSP = 0x%lx, BSP = 0x%lx\n",
               (ULONGLONG)NextPc,
               EstablisherFrame->MemoryStackFp,
               EstablisherFrame->BackingStoreFp));
#endif // _IMAGEHLP_SOURCE_
    return (NextPc);
}


UCHAR
NewParsePrologueRegionPhase0 (
    IN PUNWIND_CONTEXT UwContext,
    IN PSTATE_RECORD State,
    IN OUT PUCHAR AbiImmContext
    )
{
    PUCHAR Desc = UwContext->Descriptors;
    ULONG Offset;
    ULONG FrameSize;
    ULONG Index;
    UCHAR RecType;
    UCHAR FirstByte;
    UCHAR SecondByte;
    ULONG GrSave;
    ULONG TempMask;
    ULONG i;

    while (UwContext->DescCount < UwContext->Size) {

        FirstByte = Desc[UwContext->DescCount++];

        if ( (FirstByte & P1_MASK) == P1_PREFIX) {

            continue;

        } else if ( (FirstByte & P2_MASK) == P2_PREFIX ) {

            UwContext->DescCount++;

        } else if ( (FirstByte & P3_MASK) == P3_PREFIX ) {

            UwContext->DescCount++;

        } else if ( (FirstByte & P4_MASK) == P4_PREFIX ) {

            UwContext->DescCount += ((State->RegionLen+3) >> 2);

        } else if ( (FirstByte & P5_MASK) == P5_PREFIX ) {

            UwContext->DescCount += 3;

        } else if ( (FirstByte & P6_MASK) == P6_PREFIX ) {

            continue;

        } else if ( (FirstByte & P7_MASK) == P7_PREFIX ) {

            RecType = FirstByte & ~P7_MASK;

            switch (RecType) {

            case MEM_STACK_F:

                Offset = ReadLEB128(Desc, &UwContext->DescCount);
                FrameSize = ReadLEB128(Desc, &UwContext->DescCount);

                if (UwContext->TargetSlot > (UwContext->SlotCount+Offset))
                {
                    State->SpAdjustment += FrameSize*4;
                    State->SpWhen = Offset;
                }
                break;

            case SPILL_BASE:

                State->SpillBase = ReadLEB128(Desc, &UwContext->DescCount);
                State->SpillPtr = State->SpillBase;
                break;

            case MEM_STACK_V:
            case RP_WHEN:
            case PFS_WHEN:
            case PREDS_WHEN:
            case LC_WHEN:
            case UNAT_WHEN:
            case FPSR_WHEN:

                Offset = ReadLEB128(Desc, &UwContext->DescCount);
                if ((State->IsTarget) &&
                    (UwContext->TargetSlot > (UwContext->SlotCount+Offset)))
                {
                    Index = P7RecordTypeToRegisterIndex[RecType];
                    if (!(State->MiscMask & (1 << Index))) {
                        State->MiscMask |= MASK(Index,1);
                        UwContext->MiscRegs[Index].When = Offset;
                    } else {
                        UW_DEBUG(("Duplicate descriptors,"));
                        UW_DEBUG(("unwinder may produce incorrect result!\n"));
                    }
                }
                UW_DEBUG(("Prolog P7: type=%d slot= %d\n", RecType, Offset));
                break;

            case PSP_SPREL:
            case RP_PSPREL:
            case PFS_PSPREL:
            case PREDS_PSPREL:
            case LC_PSPREL:
            case UNAT_PSPREL:
            case FPSR_PSPREL:

                Offset = ReadLEB128(Desc, &UwContext->DescCount);
                break;

            default:

                UW_DEBUG(("Invalid record type for descriptor P7!\n"));

            }

        } else if ( (FirstByte & P8_MASK) == P8_PREFIX ) {

            RecType = Desc[UwContext->DescCount++];

            switch (RecType) {

            case PSP_PSPREL:
            case RP_SPREL:
            case PFS_SPREL:
            case PREDS_SPREL:
            case LC_SPREL:
            case UNAT_SPREL:
            case FPSR_SPREL:
            case BSP_PSPREL:
            case BSP_SPREL:
            case BSPSTORE_PSPREL:
            case BSPSTORE_SPREL:
            case RNAT_PSPREL:
            case RNAT_SPREL:
            case PRIUNAT_PSPREL:
            case PRIUNAT_SPREL:

                Offset = ReadLEB128(Desc, &UwContext->DescCount);
                UW_DEBUG(("Prolog P8: type=%d slot= %d\n", RecType, Offset));
                break;

            case BSP_WHEN:
            case BSPSTORE_WHEN:
            case RNAT_WHEN:
            case PRIUNAT_WHEN:

                Offset = ReadLEB128(Desc, &UwContext->DescCount);
                if ((State->IsTarget) &&
                    (UwContext->TargetSlot > (UwContext->SlotCount+Offset)))
                {
                    Index = P7RecordTypeToRegisterIndex[RecType];
                    if (!(State->MiscMask & (1 << Index))) {
                        State->MiscMask |= MASK(Index,1);
                        UwContext->MiscRegs[Index].When = Offset;
                    } else {
                        UW_DEBUG(("Duplicate descriptors,"));
                        UW_DEBUG(("unwinder may produce incorrect result!\n"));
                    }
                }
                UW_DEBUG(("Prolog P8: type=%d slot= %d\n", RecType, Offset));
                break;

            default:

                UW_DEBUG(("Invalid record type for descriptor P8!\n"));

            }

        } else if ( (FirstByte & P9_MASK) == P9_PREFIX ) {

            UwContext->DescCount += 2;
            VUW_DEBUG_PRINT("Format P9 not supported yet!\n");

        } else if ( (FirstByte & P10_MASK) == P10_PREFIX ) {

            UCHAR Abi = Desc[UwContext->DescCount++];
            UCHAR Context = Desc[UwContext->DescCount++];

            *AbiImmContext = Context;

            if (Abi != NT_ABI) {
                VUW_DEBUG_PRINT("Unknown ABI unwind descriptor\n");
            }

        } else {

            //
            // Encounter another region header record
            //

            break;
        }
    }

    State->DescEnd = UwContext->DescCount - 2;

    return FirstByte;
}

VOID
NewParsePrologueRegionPhase1 (
    IN PUNWIND_CONTEXT UwContext,
    IN PSTATE_RECORD State
    )
{
    ULONG FrameSize;
    ULONG Offset;
    ULONG GrSave;
    ULONG BrBase;
    ULONG Index;
    ULONG Count;
    UCHAR RecType;
    UCHAR FirstByte, SecondByte;   // 1st & 2nd bytes of a region header record
    ULONG DescIndex;
    ULONG ImaskBegin;
    UCHAR NextBr, NextGr, NextFr;
    USHORT MiscMask;
    ULONG TempMask;
    ULONG FrMask = 0;
    UCHAR BrMask = 0;
    UCHAR GrMask = 0;
    PUCHAR Desc = UwContext->Descriptors;
    BOOLEAN SpillMaskOmitted = TRUE;

    DescIndex = State->DescBegin;

    FirstByte = Desc[DescIndex];

    if ((FirstByte & R2_MASK) == R2_PREFIX) {

        //
        // general prologue region header; need to process it first
        //

        ULONG GrSave, Count;
        UCHAR MiscMask;
        UCHAR SecondByte;
        USHORT i;

        DescIndex++;
        SecondByte = Desc[DescIndex++];
        MiscMask = ((FirstByte & 0x7) << 1) | ((SecondByte & 0x80) >> 7);
        GrSave = SecondByte & 0x7F;
        ReadLEB128(Desc, &DescIndex);    // advance the descriptor index

        if (GrSave < STATIC_REGISTER_SET_SIZE) {
            UW_DEBUG(("Invalid unwind descriptor!\n"));
        }

        UW_DEBUG(("Region R2: rmask=%x,grsave=%d,length=%d\n",
                  MiscMask, GrSave, State->RegionLen));

        Count = 0;
        for (Index = REG_PREDS; Index <= REG_RP; Index++) {
            if (MiscMask & 0x1) {
                if (!(State->IsTarget) ||
                    (State->MiscMask & MASK(Index,1)))
                {
                    UwContext->MiscRegs[Index].Where = GENERAL_REG;
                    UwContext->MiscRegs[Index].SaveOffset = GrSave+Count;
                    UwContext->MiscRegs[Index].When = 0;
                    State->MiscMask |= MASK(Index,1);
                }
                Count++;
            }
            MiscMask = MiscMask >> 1;
        }
    }

    while (DescIndex <= State->DescEnd) {

        FirstByte = Desc[DescIndex++];

        if ( (FirstByte & P1_MASK) == P1_PREFIX) {

            BrMask = FirstByte & ~P1_MASK;
            State->MiscMask |= (BrMask << REG_BR_BASE);

            UW_DEBUG(("Prolog P1: brmask=%x\n", BrMask));

            for (Count = REG_BR_BASE;
                 Count < REG_BR_BASE+NUMBER_OF_PRESERVED_BR;
                 Count++)
            {
                if (BrMask & 0x1) {
                    UwContext->MiscRegs[Count].Where = PSP_RELATIVE;
                    UwContext->MiscRegs[Count].When = State->RegionLen;
                }
                BrMask = BrMask >> 1;
            }

        } else if ( (FirstByte & P2_MASK) == P2_PREFIX ) {

            SecondByte = Desc[DescIndex++];
            GrSave = SecondByte & 0x7F;
            BrMask = ((FirstByte & ~P2_MASK) << 1) | ((SecondByte & 0x80) >> 7);
            UW_DEBUG(("Prolog P2: brmask=%x reg base=%d\n", BrMask, GrSave));

            State->MiscMask |= (BrMask << REG_BR_BASE);

            for (Count = REG_BR_BASE;
                 Count < REG_BR_BASE+NUMBER_OF_PRESERVED_BR;
                 Count++)
            {
                if (BrMask & 0x1) {
                    UwContext->MiscRegs[Count].Where = GENERAL_REG;
                    UwContext->MiscRegs[Count].SaveOffset = GrSave++;
                }
                BrMask = BrMask >> 1;
            }

        } else if ( (FirstByte & P3_MASK) == P3_PREFIX ) {

            SecondByte = Desc[DescIndex++];
            RecType = ((SecondByte & 0x80) >> 7) | ((FirstByte & 0x7) << 1);
            Index = P3RecordTypeToRegisterIndex[RecType];

            if (!(State->IsTarget) ||
                (State->MiscMask & MASK(Index,1)))
            {
                if (RecType == RP_BR) {
                    UwContext->MiscRegs[Index].Where = BRANCH_REG;
                } else {
                    UwContext->MiscRegs[Index].Where = GENERAL_REG;
                }
                UwContext->MiscRegs[Index].SaveOffset = SecondByte & 0x7F;
                UwContext->MiscRegs[Index].When = 0;
                State->MiscMask |= MASK(Index,1);

                UW_DEBUG(("Prolog P3: type=%d reg=%d\n",
                          RecType, UwContext->MiscRegs[Index].SaveOffset));
            }

        } else if ( (FirstByte & P4_MASK) == P4_PREFIX ) {

            SpillMaskOmitted = FALSE;
            ImaskBegin = DescIndex;
            DescIndex += ((State->RegionLen+3) >> 2);

        } else if ( (FirstByte & P5_MASK) == P5_PREFIX ) {

            GrMask = (Desc[DescIndex] & 0xF0) >> 4;
            FrMask = ((ULONG)(Desc[DescIndex] & 0xF) << 16) |
                         ((ULONG)Desc[DescIndex+1] << 8) |
                         ((ULONG)Desc[DescIndex+2]);

            DescIndex += 3;    // increment the descriptor index

            State->GrMask |= GrMask;
            State->FrMask |= FrMask;

            UW_DEBUG(("Prolog P5: grmask = %x, frmask = %x\n",
                      State->GrMask, State->FrMask));

        } else if ( (FirstByte & P6_MASK) == P6_PREFIX ) {

            if (FirstByte & 0x10) {

                GrMask = FirstByte & 0xF;
                State->GrMask |= GrMask;

            } else {

                FrMask = FirstByte & 0xF;
                State->FrMask |= FrMask;

            }

            UW_DEBUG(("Prolog P6: is_gr = %d, mask = %x\n",
                      (FirstByte & 0x10) ? 1 : 0,
                      (FirstByte & 0x10) ? State->GrMask : State->FrMask));

        } else if ( (FirstByte & P7_MASK) == P7_PREFIX ) {

            RecType = FirstByte & ~P7_MASK;

            switch (RecType) {

            case PSP_SPREL:

                //
                // sp-relative location
                //

                Index = P7RecordTypeToRegisterIndex[RecType];
                Offset = ReadLEB128(Desc, &DescIndex);
                if (!(State->IsTarget) || (State->MiscMask & MASK(Index,1)))
                {
                    UwContext->MiscRegs[Index].Where = SP_RELATIVE;
                    UwContext->MiscRegs[Index].SaveOffset = Offset;
                    if (!(State->MiscMask & MASK(Index,1))) {
                        UwContext->MiscRegs[Index].When = State->RegionLen;
                        State->MiscMask |= MASK(Index,1);
                    }
                }
                UW_DEBUG(("Prolog P7: type=%d spoff = %d\n", RecType, Offset));
                break;


            case RP_PSPREL:
            case PFS_PSPREL:
            case PREDS_PSPREL:
            case LC_PSPREL:
            case UNAT_PSPREL:
            case FPSR_PSPREL:

                //
                // psp-relative location
                //

                Index = P7RecordTypeToRegisterIndex[RecType];
                Offset = ReadLEB128(Desc, &DescIndex);
                if (!(State->IsTarget) || (State->MiscMask & MASK(Index,1)))
                {
                    UwContext->MiscRegs[Index].Where = PSP_RELATIVE;
                    UwContext->MiscRegs[Index].SaveOffset = Offset;
                    UwContext->MiscRegs[Index].When = 0;
                    State->MiscMask |= MASK(Index,1);
                }
                UW_DEBUG(("Prolog P7: type=%d pspoff= %d\n", RecType, Offset));
                break;

            case MEM_STACK_V:
            case RP_WHEN:
            case PFS_WHEN:
            case PREDS_WHEN:
            case LC_WHEN:
            case UNAT_WHEN:
            case FPSR_WHEN:

                //
                // Nevermind processing these descriptors because they
                // have been taken care of in phase 0
                //

                Offset = ReadLEB128(Desc, &DescIndex);
                break;

            case MEM_STACK_F:

                Offset = ReadLEB128(Desc, &DescIndex);
                FrameSize = ReadLEB128(Desc, &DescIndex);

                UW_DEBUG(("Prolog P7: type=%d Slot=%d FrameSize=%d\n",
                          RecType, Offset, FrameSize));
                break;

            case SPILL_BASE:

                State->SpillBase = ReadLEB128(Desc, &DescIndex);
                State->SpillPtr = State->SpillBase;
                UW_DEBUG(("Prolog P7: type=%d, spillbase=%d\n",
                          RecType, State->SpillBase));
                break;

            default:

                UW_DEBUG(("invalid unwind descriptors\n"));

            }

        } else if ( (FirstByte & P8_MASK) == P8_PREFIX ) {

            RecType = Desc[DescIndex++];

            switch (RecType) {

            case PSP_PSPREL:
                 VUW_DEBUG_PRINT("Unsupported Unwind Descriptor!\n");
                 break;

            case RP_SPREL:
            case PFS_SPREL:
            case PREDS_SPREL:
            case LC_SPREL:
            case UNAT_SPREL:
            case FPSR_SPREL:
            case BSP_SPREL:
            case BSPSTORE_SPREL:
            case RNAT_SPREL:
            case PRIUNAT_SPREL:

                //
                // sp-relative location
                //

                Index = P8RecordTypeToRegisterIndex[RecType];
                Offset = ReadLEB128(Desc, &DescIndex);
                if (!(State->IsTarget) || (State->MiscMask & MASK(Index,1)))
                {
                    UwContext->MiscRegs[Index].Where = SP_RELATIVE;
                    UwContext->MiscRegs[Index].SaveOffset = Offset;
                    if (!(State->MiscMask & MASK(Index,1))) {
                        UwContext->MiscRegs[Index].When=State->RegionLen;
                        State->MiscMask |= MASK(Index,1);
                    }
                }
                UW_DEBUG(("Prolog P8: type=%d spoff= %d\n", RecType, Offset));
                break;

            case BSP_PSPREL:
            case BSPSTORE_PSPREL:
            case RNAT_PSPREL:
            case PRIUNAT_PSPREL:

                //
                // psp-relative location
                //

                Index = P8RecordTypeToRegisterIndex[RecType];
                Offset = ReadLEB128(Desc, &DescIndex);
                if (!(State->IsTarget) || (State->MiscMask & MASK(Index,1)))
                {
                    UwContext->MiscRegs[Index].Where = PSP_RELATIVE;
                    UwContext->MiscRegs[Index].SaveOffset = Offset;
                    UwContext->MiscRegs[Index].When = 0;
                    State->MiscMask |= MASK(Index,1);
                }
                UW_DEBUG(("Prolog P8: type=%d pspoff= %d\n", RecType, Offset));
                break;

            case BSP_WHEN:
            case BSPSTORE_WHEN:
            case RNAT_WHEN:
            case PRIUNAT_WHEN:

                //
                // Nevermind processing these descriptors because they
                // have been taken care of in phase 0
                //

                Offset = ReadLEB128(Desc, &DescIndex);
                break;

            default:

                UW_DEBUG(("Invalid record type for descriptor P8!\n"));

            }

        } else if ( (FirstByte & P9_MASK) == P9_PREFIX ) {

            DescIndex += 2;
            VUW_DEBUG_PRINT("Format P9 not supported yet!\n");

        } else if ( (FirstByte & P10_MASK) == P10_PREFIX ) {

            UCHAR Abi = Desc[DescIndex++];
            UCHAR Context = Desc[DescIndex++];

        } else {

            UW_DEBUG(("Invalid descriptor!\n"));

        }
    }

    GrMask = State->GrMask;
    FrMask = State->FrMask;
    BrMask = State->MiscMask >> REG_BR_BASE;

    if (!(GrMask | FrMask | BrMask)) {

        return;

    } else if (SpillMaskOmitted && !(State->IsTarget)) {

        //
        // When spillmask is omitted, floating point registers, general
        // registers, and then branch regisers are spilled in order.
        // They are not modified in the prologue region; therefore, there
        // is no need to restore their contents when the control ip is
        // in this prologue region.
        //

        // 1. floating point registers

        State->SpillPtr &= ~(SPILLSIZE_OF_FLOAT128_IN_DWORDS - 1);
        NextFr = NUMBER_OF_PRESERVED_FR - 1;
        while (FrMask & 0xFFFFF) {
            if (FrMask & 0x80000) {
                State->SpillPtr += SPILLSIZE_OF_FLOAT128_IN_DWORDS;
                UwContext->Float[NextFr].SaveOffset = State->SpillPtr;
            }
            FrMask = FrMask << 1;
            NextFr--;
        }

        // 2. branch registers

        NextBr = REG_BR_BASE + NUMBER_OF_PRESERVED_BR - 1;
        while (BrMask & 0x1F) {
            if (BrMask & 0x10) {
                if (UwContext->MiscRegs[Index].Where == PSP_RELATIVE) {
                    State->SpillPtr += SPILLSIZE_OF_ULONGLONG_IN_DWORDS;
                    UwContext->MiscRegs[NextBr].SaveOffset = State->SpillPtr;
                }
            }
            BrMask = BrMask << 1;
            NextBr--;
        }

        // 3. general registers

        NextGr = NUMBER_OF_PRESERVED_GR - 1;
        while (GrMask & 0xF) {
            if (GrMask & 0x8) {
                State->SpillPtr += SPILLSIZE_OF_ULONGLONG_IN_DWORDS;
                UwContext->Integer[NextGr].SaveOffset = State->SpillPtr;
            }
            GrMask = GrMask << 1;
            NextGr--;
        }

    } else if (SpillMaskOmitted && State->IsTarget) {

        State->GrMask = 0;
        State->FrMask = 0;
        State->MiscMask &= MASK(REG_BR_BASE, 1) - 1;

    } else if (SpillMaskOmitted == FALSE) {

        ULONG Length;

        if (State->IsTarget) {

            //
            // control ip is in the prologue region; clear the masks
            // and then process the imask to determine which preserved
            // Gr/Fr/Br have been saved and set the corresponding bits.
            //

            State->GrMask = 0;
            State->FrMask = 0;
            State->MiscMask &= MASK(REG_BR_BASE, 1) - 1;
            Length = UwContext->TargetSlot - State->RegionBegin;
        } else {
            Length = State->RegionLen;
        }

        NextGr = NUMBER_OF_PRESERVED_GR - 1;
        NextBr = NUMBER_OF_PRESERVED_BR - 1;
        NextFr = NUMBER_OF_PRESERVED_FR - 1;
        for (Count = 0; Count < Length; Count++) {

            if ((Count % 4) == 0) {
                FirstByte = Desc[ImaskBegin++];
            } else {
                FirstByte = FirstByte << 2;
            }

            switch (FirstByte & 0xC0) {

            case 0x40:                  // 0x01 - save next fr

                while ( !(FrMask & 0x80000) && (NextFr > 0) ) {
                    NextFr--;
                    FrMask = FrMask << 1;
                }

                UW_DEBUG(("spilled register FS%lx\n", (ULONG)NextFr));

                State->FrMask |= MASK(NextFr,1);
                UwContext->Float[NextFr].When = Count;
                State->SpillPtr += SPILLSIZE_OF_ULONGLONG_IN_DWORDS;
                State->SpillPtr &= ~(SPILLSIZE_OF_FLOAT128_IN_DWORDS - 1);
                State->SpillPtr += SPILLSIZE_OF_FLOAT128_IN_DWORDS;
                UwContext->Float[NextFr].SaveOffset = State->SpillPtr;

                NextFr--;
                FrMask = FrMask << 1;
                break;

            case 0x80:                  // 0x10 - save next gr

                while ( !(GrMask & 0x8) && (NextGr > 0) ) {
                    NextGr--;
                    GrMask = GrMask << 1;
                }

                UW_DEBUG(("spilled register S%lx\n", (ULONG)NextGr));

                State->GrMask |= MASK(NextGr,1);
                UwContext->Integer[NextGr].When = Count;
                State->SpillPtr += SPILLSIZE_OF_ULONGLONG_IN_DWORDS;
                UwContext->Integer[NextGr].SaveOffset = State->SpillPtr;

                NextGr--;
                GrMask = GrMask << 1;
                break;

            case 0xC0:                  // 0x11 - save next br

                while ( !(BrMask & 0x10) && (NextBr > 0) ) {
                    NextBr--;
                    BrMask = BrMask << 1;
                }

                UW_DEBUG(("spilled register BS%lx\n", (ULONG)NextBr));

                Index = REG_BR_BASE + NextBr;
                State->MiscMask |= MASK(Index,1);
                UwContext->MiscRegs[Index].When = Count;
                if (UwContext->MiscRegs[Index].Where == PSP_RELATIVE) {
                    State->SpillPtr += SPILLSIZE_OF_ULONGLONG_IN_DWORDS;
                    UwContext->MiscRegs[Index].SaveOffset = State->SpillPtr;
                }

                NextBr--;
                BrMask = BrMask << 1;
                break;

            default:                    // 0x00 - save no register
                break;

            }
        }
    }
}
