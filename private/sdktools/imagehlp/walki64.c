/*++

Copyright (c) 1996  Intel Corporation
Copyright (c) 1993  Microsoft Corporation

Module Name:

    walki64.c

Abstract:

    This file implements the IA64 stack walking api.

Author:

Environment:

    User Mode

--*/

#define _IMAGEHLP_SOURCE_
#define _IA64REG_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "private.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include "symbols.h"
#include <stdlib.h>

BOOL
WalkIa64Init(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

BOOL
WalkIa64Next(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccessRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

BOOL
GetStackFrameIa64(
    HANDLE                            hProcess,
    PULONG64                          ReturnAddress,
    PULONG64                          FramePointer,
    PULONG64                          BStorePointer,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    );

#define CALLBACK_STACK(f)  (f->KdHelp.ThCallbackStack)
#define CALLBACK_BSTORE(f)  (f->KdHelp.ThCallbackBStore)
#define CALLBACK_NEXT(f)   (f->KdHelp.NextCallback)
#define CALLBACK_FUNC(f)   (f->KdHelp.KiCallUserMode)
#define CALLBACK_THREAD(f) (f->KdHelp.Thread)


ULONGLONG
GetImageBase (
    HANDLE  hProcess,
    ULONG64 ControlPc
    )
{
    PPROCESS_ENTRY  ProcessEntry;
    PMODULE_ENTRY   mi;

    __try {

        ProcessEntry = FindProcessEntry( hProcess );
        if (!ProcessEntry) {
            SetLastError( ERROR_INVALID_HANDLE );
            return (ULONGLONG) NULL;
        }

        mi = GetModuleForPC( ProcessEntry, ControlPc, FALSE );
        if (mi == NULL) {
            SetLastError( ERROR_MOD_NOT_FOUND );
            return (ULONGLONG) NULL;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        return (ULONGLONG) NULL;

    }

    return (mi->BaseOfDll);
}

WalkIa64(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    BOOL rval;

    if (StackFrame->Virtual) {

        rval = WalkIa64Next( hProcess,
                             StackFrame,
                             Context,
                             ReadMemory,
                             FunctionTableAccess,
                             GetModuleBase
                           );

    } else {

        rval = WalkIa64Init( hProcess,
                             StackFrame,
                             Context,
                             ReadMemory,
                             FunctionTableAccess,
                             GetModuleBase
                           );

    }

    return rval;
}

ULONGLONG
VirtualUnwindIa64 (
    HANDLE hProcess,
    ULONGLONG ImageBase,
    DWORD64 ControlPc,
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    PIA64_CONTEXT ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemory
    );


BOOL
GetStackFrameIa64(
    HANDLE                            hProcess,
    PULONG64                          ReturnAddress,
    PULONG64                          FramePointer,
    PULONG64                          BStorePointer,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    ULONGLONG                          ImageBase;
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY rf;
    ULONG64                            dwRa = (ULONG64)Context->BrRp;
    BOOL                               rval = TRUE;


    rf = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY) FunctionTableAccess( hProcess, *ReturnAddress );

    if (rf) {

        //
        // The Rp value coming out of mainCRTStartup is set by some run-time
        // routine to be 0; this serves to cause an error if someone actually
        // does a return from the mainCRTStartup frame.
        //

        ImageBase = GetModuleBase (hProcess, *ReturnAddress);
        dwRa = (ULONG64)VirtualUnwindIa64( hProcess, ImageBase, *ReturnAddress, rf, Context, ReadMemory );
        if (!dwRa) {
            rval = FALSE;
        }

        if ((dwRa == *ReturnAddress) &&
               (*FramePointer == Context->IntSp) &&
               (*(FramePointer+1) == Context->RsBSP)) {
            rval = FALSE;
        }

        *ReturnAddress = dwRa;
        *FramePointer  = Context->IntSp;
        *BStorePointer = Context->RsBSP;

    } else {

        SHORT BsFrameSize;
        SHORT TempFrameSize;

        if ((dwRa == *ReturnAddress) &&
               (*FramePointer == Context->IntSp) &&
               (*(FramePointer+1) == Context->RsBSP)) {
            rval = FALSE;
        }

        *ReturnAddress = Context->BrRp;
        *FramePointer  = Context->IntSp;
        *BStorePointer = Context->RsBSP;
        Context->StIFS = Context->RsPFS;
        BsFrameSize = (SHORT)(Context->StIFS >> IA64_PFS_SIZE_SHIFT) & IA64_PFS_SIZE_MASK;
        TempFrameSize = BsFrameSize - (SHORT)((Context->RsBSP >> 3) & IA64_NAT_BITS_PER_RNAT_REG);
        while (TempFrameSize > 0) {
            BsFrameSize++;
            TempFrameSize -= IA64_NAT_BITS_PER_RNAT_REG;
        }
        Context->RsBSPSTORE = Context->RsBSP -= BsFrameSize * sizeof(ULONGLONG);
    }

    return rval;
}


BOOL
WalkIa64Init(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    IA64_KSWITCH_FRAME SwitchFrame;
    IA64_CONTEXT       ContextSave;
    DWORD64            PcOffset;
    DWORD64            FrameOffset;
    DWORD64            BStoreOffset;
    DWORD              cb;
    ULONG              Index;
    BOOL               Result;


    if (StackFrame->AddrFrame.Offset) {
        if (ReadMemory( hProcess,
                        StackFrame->AddrFrame.Offset+IA64_STACK_SCRATCH_AREA,
                        &SwitchFrame,
                        sizeof(IA64_KSWITCH_FRAME),
                        &cb )) {

            SHORT BsFrameSize;
            SHORT TempFrameSize;

            //
            // successfully read a switch frame from the stack
            //

            Context->IntSp = StackFrame->AddrFrame.Offset;
            Context->Preds = SwitchFrame.SwitchPredicates;
            Context->StIIP = SwitchFrame.SwitchRp;
            Context->StFPSR = SwitchFrame.SwitchFPSR;
            Context->BrRp = SwitchFrame.SwitchRp;
            Context->RsPFS = SwitchFrame.SwitchPFS;
            Context->StIFS = SwitchFrame.SwitchPFS;
            BsFrameSize = (SHORT)(SwitchFrame.SwitchPFS >> IA64_PFS_SIZE_SHIFT) & IA64_PFS_SIZE_MASK;
            TempFrameSize = BsFrameSize - (SHORT)((SwitchFrame.SwitchBsp >> 3) & IA64_NAT_BITS_PER_RNAT_REG);
            while (TempFrameSize > 0) {
                BsFrameSize++;
                TempFrameSize -= IA64_NAT_BITS_PER_RNAT_REG;
            }
            Context->RsBSP = SwitchFrame.SwitchBsp - BsFrameSize * sizeof(ULONGLONG);

            Context->FltS0 = SwitchFrame.SwitchExceptionFrame.FltS0;
            Context->FltS1 = SwitchFrame.SwitchExceptionFrame.FltS1;
            Context->FltS2 = SwitchFrame.SwitchExceptionFrame.FltS2;
            Context->FltS3 = SwitchFrame.SwitchExceptionFrame.FltS3;
            Context->FltS4 = SwitchFrame.SwitchExceptionFrame.FltS4;
            Context->FltS5 = SwitchFrame.SwitchExceptionFrame.FltS5;
            Context->FltS6 = SwitchFrame.SwitchExceptionFrame.FltS6;
            Context->FltS7 = SwitchFrame.SwitchExceptionFrame.FltS7;
            Context->FltS8 = SwitchFrame.SwitchExceptionFrame.FltS8;
            Context->FltS9 = SwitchFrame.SwitchExceptionFrame.FltS9;
            Context->FltS10 = SwitchFrame.SwitchExceptionFrame.FltS10;
            Context->FltS11 = SwitchFrame.SwitchExceptionFrame.FltS11;
            Context->FltS12 = SwitchFrame.SwitchExceptionFrame.FltS12;
            Context->FltS13 = SwitchFrame.SwitchExceptionFrame.FltS13;
            Context->FltS14 = SwitchFrame.SwitchExceptionFrame.FltS14;
            Context->FltS15 = SwitchFrame.SwitchExceptionFrame.FltS15;
            Context->FltS16 = SwitchFrame.SwitchExceptionFrame.FltS16;
            Context->FltS17 = SwitchFrame.SwitchExceptionFrame.FltS17;
            Context->FltS18 = SwitchFrame.SwitchExceptionFrame.FltS18;
            Context->FltS19 = SwitchFrame.SwitchExceptionFrame.FltS19;
            Context->IntS0 = SwitchFrame.SwitchExceptionFrame.IntS0;
            Context->IntS1 = SwitchFrame.SwitchExceptionFrame.IntS1;
            Context->IntS2 = SwitchFrame.SwitchExceptionFrame.IntS2;
            Context->IntS3 = SwitchFrame.SwitchExceptionFrame.IntS3;
            Context->IntNats = SwitchFrame.SwitchExceptionFrame.IntNats;
            Context->BrS0 = SwitchFrame.SwitchExceptionFrame.BrS0;
            Context->BrS1 = SwitchFrame.SwitchExceptionFrame.BrS1;
            Context->BrS2 = SwitchFrame.SwitchExceptionFrame.BrS2;
            Context->BrS3 = SwitchFrame.SwitchExceptionFrame.BrS3;
            Context->BrS4 = SwitchFrame.SwitchExceptionFrame.BrS4;
            Context->ApEC = SwitchFrame.SwitchExceptionFrame.ApEC;
            Context->ApLC = SwitchFrame.SwitchExceptionFrame.ApLC;

        } else {
            return FALSE;
        }
    }

    ZeroMemory( StackFrame, sizeof(*StackFrame) );

    StackFrame->Virtual = TRUE;

    StackFrame->AddrPC.Offset       = Context->StIIP;
    StackFrame->AddrPC.Mode         = AddrModeFlat;

    StackFrame->AddrFrame.Offset    = Context->IntSp;
    StackFrame->AddrFrame.Mode      = AddrModeFlat;

    StackFrame->AddrBStore.Offset    = Context->RsBSP;
    StackFrame->AddrBStore.Mode      = AddrModeFlat;

    ContextSave = *Context;
    PcOffset    = StackFrame->AddrPC.Offset;
    FrameOffset = StackFrame->AddrFrame.Offset;
    BStoreOffset = StackFrame->AddrBStore.Offset;

    if (!GetStackFrameIa64( hProcess,
                        &PcOffset,
                        &FrameOffset,
                        &BStoreOffset,
                        &ContextSave,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase) ) {

        StackFrame->AddrReturn.Offset = Context->BrRp;

    } else {

        StackFrame->AddrReturn.Offset = PcOffset;
    }

    StackFrame->AddrReturn.Mode     = AddrModeFlat;

    //
    // get the arguments to the function
    //

    Index = (ULONG)(ContextSave.RsBSP & 0x1F8) >> 3;
    if (Index > 59) {

        DWORD i, j;
        DWORD64 Params[5];

        Result = ReadMemory (hProcess, ContextSave.RsBSP,
                             Params, 40, &cb);
        if (Result) {
            j = 0;
            for (i = 0; i < 5; i++, Index++) {
                if (Index != 63) {
                    StackFrame->Params[j++] = Params[i];
                }
            }
        }

    } else {
        Result = ReadMemory (hProcess, ContextSave.RsBSP,
                             StackFrame->Params, 32, &cb);
    }

    if (!Result) {
        StackFrame->Params[0] =
        StackFrame->Params[1] =
        StackFrame->Params[2] =
        StackFrame->Params[3] = 0;
    }

    return TRUE;
}


BOOL
WalkIa64Next(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PIA64_CONTEXT                     Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    DWORD           cb;
    IA64_CONTEXT    ContextSave;
    BOOL            rval = TRUE;
    BOOL            Result;
    DWORD64         StackAddress;
    DWORD64         BStoreAddress;
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY rf;
    DWORD64         qw;
    ULONG           Index;


    if (!GetStackFrameIa64( hProcess,
                        &StackFrame->AddrPC.Offset,
                        &StackFrame->AddrFrame.Offset,
                        &StackFrame->AddrBStore.Offset,
                        Context,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase) ) {

        rval = FALSE;

        //
        // If the frame could not be unwound or is terminal, see if
        // there is a callback frame:
        //

        if (AppVersion.Revision >= 4 && CALLBACK_STACK(StackFrame)) {

            if (CALLBACK_STACK(StackFrame) & 0x80000000) {

                //
                // it is the pointer to the stack frame that we want
                //

                StackAddress = CALLBACK_STACK(StackFrame);

            } else {

                //
                // if it is a positive integer, it is the offset to
                // the address in the thread.
                // Look up the pointer:
                //

                rval = ReadMemory(hProcess,
                                  (CALLBACK_THREAD(StackFrame) +
                                                 CALLBACK_STACK(StackFrame)),
                                  &StackAddress,
                                  sizeof(DWORD64),
                                  &cb);

                if (!rval || StackAddress == 0) {
                    StackAddress = (DWORD64)-1;
                    CALLBACK_STACK(StackFrame) = (DWORD)-1;
                }

            }

            if ( (StackAddress == (DWORD64)-1) ||
                !(rf = (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)
                     FunctionTableAccess(hProcess, CALLBACK_FUNC(StackFrame))) ) {

                rval = FALSE;

            } else {

                ReadMemory(hProcess,
                           (StackAddress + CALLBACK_NEXT(StackFrame)),
                           &CALLBACK_STACK(StackFrame),
                           sizeof(DWORD64),
                           &cb);

                StackFrame->AddrPC.Offset = rf->BeginAddress;  // ?????
                StackFrame->AddrFrame.Offset = StackAddress;
                Context->IntSp = StackAddress;

                rval = TRUE;
            }

        }
    }

    //
    // get the return address
    //
    ContextSave = *Context;
    StackFrame->AddrReturn.Offset = StackFrame->AddrPC.Offset;

    if (!GetStackFrameIa64( hProcess,
                        &StackFrame->AddrReturn.Offset,
                        &qw,
                        &qw,
                        &ContextSave,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase) ) {


        StackFrame->AddrReturn.Offset = 0;

    }

    //
    // get the arguments to the function
    //

    Index = (ULONG)(ContextSave.RsBSP & 0x1F8) >> 3;
    if (Index > 59) {

        DWORD i, j;
        DWORD64 Params[5];

        Result = ReadMemory (hProcess, ContextSave.RsBSP,
                             Params, 40, &cb);
        if (Result) {
            j = 0;
            for (i = 0; i < 5; i++, Index++) {
                if (Index != 63) {
                    StackFrame->Params[j++] = Params[i];
                }
            }
        }

    } else {
        Result = ReadMemory (hProcess, ContextSave.RsBSP,
                             StackFrame->Params, 32, &cb);
    }

    if (!Result) {
        StackFrame->Params[0] =
        StackFrame->Params[1] =
        StackFrame->Params[2] =
        StackFrame->Params[3] = 0;
    }

    return rval;
}
