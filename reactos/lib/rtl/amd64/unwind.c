/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Exception related functions
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#define UNWIND_HISTORY_TABLE_NONE 0
#define UNWIND_HISTORY_TABLE_GLOBAL 1
#define UNWIND_HISTORY_TABLE_LOCAL 2

#define UWOP_PUSH_NONVOL 0
#define UWOP_ALLOC_LARGE 1
#define UWOP_ALLOC_SMALL 2
#define UWOP_SET_FPREG 3
#define UWOP_SAVE_NONVOL 4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_SAVE_XMM 6
#define UWOP_SAVE_XMM_FAR 7
#define UWOP_SAVE_XMM128 8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME 10

typedef unsigned char UBYTE;

typedef union _UNWIND_CODE
{
    struct
    {
        UBYTE CodeOffset;
        UBYTE UnwindOp:4;
        UBYTE OpInfo:4;
    };
    USHORT FrameOffset;
} UNWIND_CODE, *PUNWIND_CODE;

typedef struct _UNWIND_INFO
{
    UBYTE Version:3;
    UBYTE Flags:5;
    UBYTE SizeOfProlog;
    UBYTE CountOfCodes;
    UBYTE FrameRegister:4;
    UBYTE FrameOffset:4;
    UNWIND_CODE UnwindCode[1];
/*    union {
        OPTIONAL ULONG ExceptionHandler;
        OPTIONAL ULONG FunctionEntry;
    };
    OPTIONAL ULONG ExceptionData[]; 
*/
} UNWIND_INFO, *PUNWIND_INFO;

PVOID
NTAPI
RtlpLookupModuleBase(
    PVOID Address);

/* FUNCTIONS *****************************************************************/

PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionTable(
    IN DWORD64 ControlPc,
    OUT PDWORD64 ImageBase,
    OUT PULONG Length)
{
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_DATA_DIRECTORY Directory;

    /* Find ModuleBase */
    DosHeader = RtlpLookupModuleBase((PVOID)ControlPc);
    if (!DosHeader)
    {
        return NULL;
    }

    /* Locate NT header and check number of directories */
    NtHeader = (PVOID)((ULONG64)DosHeader + DosHeader->e_lfanew);
    if (NtHeader->OptionalHeader.NumberOfRvaAndSizes 
         < IMAGE_DIRECTORY_ENTRY_EXCEPTION)
    {
        return NULL;
    }

    /* Locate the exception directory */
    Directory = &NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    *Length = Directory->Size / sizeof(RUNTIME_FUNCTION);
    *ImageBase = (ULONG64)DosHeader;
    if (!Directory->VirtualAddress)
    {
        return NULL;
    }

    return (PVOID)((ULONG64)DosHeader + Directory->VirtualAddress);
}


// http://msdn.microsoft.com/en-us/library/ms680597(VS.85).aspx
PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionEntry(
    IN DWORD64 ControlPc,
    OUT PDWORD64 ImageBase,
    OUT PUNWIND_HISTORY_TABLE HistoryTable)
{
    PRUNTIME_FUNCTION FunctionTable, FunctionEntry;
    ULONG TableLength;
    ULONG IndexLo, IndexHi, IndexMid;

    /* Find the corresponding table */
    FunctionTable = RtlLookupFunctionTable(ControlPc,
                                           ImageBase,
                                           &TableLength);

    /* Fail, if no table is found */
    if (!FunctionTable)
    {
        return (PVOID)1;
    }

    /* Use relative virtual address */
    ControlPc -= *ImageBase;

    /* Do a binary search */
    IndexLo = 0;
    IndexHi = TableLength;
    while (IndexHi > IndexLo)
    {
        IndexMid = (IndexLo + IndexHi) / 2;
        FunctionEntry = &FunctionTable[IndexMid];

        if ( (ControlPc >= FunctionEntry->BeginAddress) &&
             (ControlPc < FunctionEntry->EndAddress) )
        {
            /* ControlPc is within limits, return entry */
            return FunctionEntry;
        }

        if (ControlPc < FunctionEntry->BeginAddress)
        {
            /* Continue search in lower half */
            IndexHi = IndexMid;
        }
        else
        {
            /* Continue search in upper half */
            IndexLo = IndexMid + 1;
        }
    }

    /* Nothing found, return NULL */
    return NULL;
}

void
FORCEINLINE
SetReg(PCONTEXT Context, UCHAR Reg, ULONG64 Value)
{
    ((PULONG64)(&Context->Rax))[Reg] = Value;
}

PEXCEPTION_ROUTINE
NTAPI
RtlVirtualUnwind(
    IN ULONG HandlerType,
    IN ULONG64 ImageBase,
    IN ULONG64 ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT Context,
    OUT PVOID *HandlerData,
    OUT PULONG64 EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers)
{
    PUNWIND_INFO UnwindInfo;
    ULONG CodeOffset;
    ULONG i;
    UNWIND_CODE UnwindCode;
    UCHAR Reg;

    /* Use relative virtual address */
    ControlPc -= ImageBase;

    /* Sanity checks */
    if ( (ControlPc < FunctionEntry->BeginAddress) ||
         (ControlPc >= FunctionEntry->EndAddress) )
    {
        return NULL;
    }

    /* Get a pointer to the unwind info */
    UnwindInfo = RVA(ImageBase, FunctionEntry->UnwindData);

    /* Calculate relative offset to function start */
    CodeOffset = ControlPc - FunctionEntry->BeginAddress;

    /* Skip all Ops with an offset greater than the current Offset */
    i = 0;
    while (i < UnwindInfo->CountOfCodes &&
           CodeOffset < UnwindInfo->UnwindCode[i].CodeOffset)
    {
        UnwindCode = UnwindInfo->UnwindCode[i];
        switch (UnwindCode.UnwindOp)
        {
            case UWOP_SAVE_NONVOL:
            case UWOP_SAVE_XMM:
            case UWOP_SAVE_XMM128:
                i += 2;
                break;

            case UWOP_SAVE_NONVOL_FAR:
            case UWOP_SAVE_XMM_FAR:
            case UWOP_SAVE_XMM128_FAR:
                i += 3;
                break;

            case UWOP_ALLOC_LARGE:
                i += UnwindCode.OpInfo ? 3 : 2;
                break;

            default:
                i++;
        }
    }

    /* Process the left Ops */
    while (i < UnwindInfo->CountOfCodes)
    {
        UnwindCode = UnwindInfo->UnwindCode[i];
        switch (UnwindCode.UnwindOp)
        {
            case UWOP_PUSH_NONVOL:
                Reg = UnwindCode.OpInfo;
                SetReg(Context, Reg, *(DWORD64*)Context->Rsp);
                Context->Rsp += sizeof(DWORD64);
                i++;
                break;

            case UWOP_ALLOC_LARGE:
                if (UnwindCode.OpInfo)
                {
                    ULONG Offset = *(ULONG*)(&UnwindInfo->UnwindCode[i+1]);
                    Context->Rsp += Offset;
                    i += 3;
                }
                else
                {
                    USHORT Offset = UnwindInfo->UnwindCode[i+1].FrameOffset;
                    Context->Rsp += Offset * 8;
                    i += 2;
                }
                break;

            case UWOP_ALLOC_SMALL:
                Context->Rsp += (UnwindCode.OpInfo + 1) * 8;
                i++;
                break;

            case UWOP_SET_FPREG:
                i++;
                break;

            case UWOP_SAVE_NONVOL:
                i += 2;
                break;

            case UWOP_SAVE_NONVOL_FAR:
                i += 3;
                break;

            case UWOP_SAVE_XMM:
                i += 2;
                break;

            case UWOP_SAVE_XMM_FAR:
                i += 3;
            case UWOP_SAVE_XMM128:
                i += 2;
            case UWOP_SAVE_XMM128_FAR:
                i += 3;
            case UWOP_PUSH_MACHFRAME:
                i += 1;
        }
    }

    /* Unwind is finished, pop new Rip from Stack */
    Context->Rip = *(DWORD64*)Context->Rsp;
    Context->Rsp += sizeof(DWORD64);

    return 0;
}

VOID
NTAPI
RtlUnwindEx(
   IN ULONG64 TargetFrame,
   IN ULONG64 TargetIp,
   IN PEXCEPTION_RECORD ExceptionRecord,
   IN PVOID ReturnValue,
   OUT PCONTEXT OriginalContext,
   IN PUNWIND_HISTORY_TABLE HistoryTable)
{
    UNIMPLEMENTED;
    return;
}

VOID
NTAPI
RtlUnwind(
  IN PVOID TargetFrame,
  IN PVOID TargetIp,
  IN PEXCEPTION_RECORD ExceptionRecord,
  IN PVOID ReturnValue)
{
    UNIMPLEMENTED;
    return;
}

ULONG
NTAPI
RtlWalkFrameChain(OUT PVOID *Callers,
                  IN ULONG Count,
                  IN ULONG Flags)
{
    CONTEXT Context;
    ULONG64 ControlPc, ImageBase, EstablisherFrame;
    ULONG64 StackBegin, StackEnd;
    PVOID HandlerData;
    INT i;
    PRUNTIME_FUNCTION FunctionEntry;
DPRINT1("RtlWalkFrameChain called\n");
    RtlCaptureContext(&Context);

    ControlPc = Context.Rip;

    RtlpGetStackLimits(&StackBegin, &StackEnd);

    /* Check if we want the user-mode stack frame */
    if (Flags == 1)
    {
    }

    /* Loop the frames */
    for (i = 0; i < Count; i++)
    {
        /* Lookup the FunctionEntry for the current ControlPc */
        FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, NULL);

        /* Is this a leaf function? */
        if (!FunctionEntry)
        {
            Context.Rip = *(DWORD64*)Context.Rsp;
            Context.Rsp += sizeof(DWORD64);
            DPRINT("leaf funtion, new Rip = %p, new Rsp = %p\n", (PVOID)Context.Rip, (PVOID)Context.Rsp);
        }
        else
        {
            RtlVirtualUnwind(0,
                             ImageBase,
                             ControlPc,
                             FunctionEntry,
                             &Context,
                             &HandlerData,
                             &EstablisherFrame,
                             NULL);
            DPRINT("normal funtion, new Rip = %p, new Rsp = %p\n", (PVOID)Context.Rip, (PVOID)Context.Rsp);
        }

        ControlPc = Context.Rip;
        /* Save this frame */

        Callers[i] = (PVOID)ControlPc;

    }

    return i;
}

