/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    walkx86.c

Abstract:

    This file implements the Intel x86 stack walking api.  This api allows for
    the presence of "real mode" stack frames.  This means that you can trace
    into WOW code.

Author:

    Wesley Witt (wesw) 1-Oct-1993

Environment:

    User Mode

--*/

#define _IMAGEHLP_SOURCE_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "private.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include <objbase.h>
#include <wx86dll.h>
#include <symbols.h>



#define SAVE_EBP(f)        (f->Reserved[0])
#define TRAP_TSS(f)        (f->Reserved[1])
#define TRAP_EDITED(f)     (f->Reserved[1])
#define SAVE_TRAP(f)       (f->Reserved[2])
#define CALLBACK_STACK(f)  (f->KdHelp.ThCallbackStack)
#define CALLBACK_NEXT(f)   (f->KdHelp.NextCallback)
#define CALLBACK_FUNC(f)   (f->KdHelp.KiCallUserMode)
#define CALLBACK_THREAD(f) (f->KdHelp.Thread)
#define CALLBACK_FP(f)     (f->KdHelp.FramePointer)
#define CALLBACK_DISPATCHER(f) (f->KdHelp.KeUserCallbackDispatcher)
#define SYSTEM_RANGE_START(f) (f->KdHelp.SystemRangeStart)

#define STACK_SIZE         (sizeof(DWORD))
#define FRAME_SIZE         (STACK_SIZE * 2)

#define STACK_SIZE16       (sizeof(WORD))
#define FRAME_SIZE16       (STACK_SIZE16 * 2)
#define FRAME_SIZE1632     (STACK_SIZE16 * 3)

#define MAX_STACK_SEARCH   64   // in STACK_SIZE units
#define MAX_JMP_CHAIN      64   // in STACK_SIZE units
#define MAX_CALL           7    // in bytes
#define MIN_CALL           2    // in bytes

#define PUSHBP             0x55
#define MOVBPSP            0xEC8B


#define DoMemoryRead(addr,buf,sz,br) \
    ReadMemoryInternal( hProcess, hThread, addr, buf, sz, \
                        br, ReadMemory, TranslateAddress )


BOOL
WalkX86Init(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
WalkX86Next(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
ReadMemoryInternal(
    HANDLE                          hProcess,
    HANDLE                          hThread,
    LPADDRESS64                     lpBaseAddress,
    LPVOID                          lpBuffer,
    DWORD                           nSize,
    LPDWORD                         lpNumberOfBytesRead,
    PREAD_PROCESS_MEMORY_ROUTINE64  ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64    TranslateAddress
    );

BOOL
IsFarCall(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    BOOL                              *Ok,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    );

BOOL
ReadTrapFrame(
    HANDLE                            hProcess,
    DWORD64                           TrapFrameAddress,
    PX86_KTRAP_FRAME                  TrapFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    );

BOOL
TaskGate2TrapFrame(
    HANDLE                            hProcess,
    USHORT                            TaskRegister,
    PX86_KTRAP_FRAME                  TrapFrame,
    PULONG64                          off,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    );



BOOL
WalkX86(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    StackFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL rval;

    if (StackFrame->Virtual) {

        rval = WalkX86Next( hProcess,
                            hThread,
                            StackFrame,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress
                          );

    } else {

        rval = WalkX86Init( hProcess,
                            hThread,
                            StackFrame,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress
                          );

    }

    return rval;
}

BOOL
ReadMemoryInternal(
    HANDLE                          hProcess,
    HANDLE                          hThread,
    LPADDRESS64                     lpBaseAddress,
    LPVOID                          lpBuffer,
    DWORD                           nSize,
    LPDWORD                         lpNumberOfBytesRead,
    PREAD_PROCESS_MEMORY_ROUTINE64  ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64    TranslateAddress
    )
{
    ADDRESS64 addr;

    addr = *lpBaseAddress;
    if (addr.Mode != AddrModeFlat) {
        TranslateAddress( hProcess, hThread, &addr );
    }
    return ReadMemory( hProcess,
                       addr.Offset,
                       lpBuffer,
                       nSize,
                       lpNumberOfBytesRead
                       );
}

DWORD64
SearchForReturnAddress(
    HANDLE                            hProcess,
    DWORD64                           uoffStack,
    DWORD64                           funcAddr,
    DWORD                             funcSize,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    BOOL                              AcceptUnreadableCallSite
    )
{
    DWORD64        uoffRet;
    DWORD64        uoffBestGuess = 0;
    DWORD          cdwIndex;
    DWORD          cdwIndexMax;
    INT            cbIndex;
    INT            cbLimit;
    DWORD          cBytes;
    DWORD          cJmpChain = 0;
    DWORD64        uoffT;
    DWORD          cb;
    BYTE           jmpBuffer[ sizeof(WORD) + sizeof(DWORD) ];
    LPWORD         lpwJmp = (LPWORD)&jmpBuffer[0];
    BYTE           code[MAX_CALL];
    DWORD          stack [ MAX_STACK_SEARCH ];
    BOPINSTR BopInstr;

    //
    // this function is necessary for 4 reasons:
    //
    //      1) random compiler bugs where regs are saved on the
    //         stack but the fpo data does not account for them
    //
    //      2) inline asm code that does a push
    //
    //      3) any random code that does a push and it isn't
    //         accounted for in the fpo data
    //
    //      4) non-void non-fpo functions
    //         *** This case is not neccessary when the compiler
    //          emits FPO records for non-FPO funtions.  Unfortunately
    //          only the NT group uses this feature.
    //

    if (!ReadMemory(hProcess,
                    uoffStack,
                    stack,
                    sizeof(stack),
                    &cb)) {
        return 0;
    }


    cdwIndexMax = cb / STACK_SIZE;

    if ( !cdwIndexMax ) {
        return 0;
    }

    for ( cdwIndex=0; cdwIndex<cdwIndexMax; cdwIndex++,uoffStack+=STACK_SIZE ) {

        uoffRet = (DWORD64)(LONG64)(LONG)stack[cdwIndex];

        //
        // Don't try looking for Code in the first 64K of an NT app.
        //
        if ( uoffRet < 0x00010000 ) {
            continue;
        }

        //
        // if it isn't part of any known address space it must be bogus
        //

        if (GetModuleBase( hProcess, uoffRet ) == 0) {
            continue;
        }

        //
        // Check for a BOP instruction.
        //
        if (ReadMemory(hProcess,
                       uoffRet - sizeof(BOPINSTR),
                       &BopInstr,
                       sizeof(BOPINSTR),
                       &cb)) {

            if (cb == sizeof(BOPINSTR) &&
                BopInstr.Instr1 == 0xc4 && BopInstr.Instr2 == 0xc4) {
                return uoffStack;
            }
        }

        //
        // Read the maximum number of bytes a call could be from the istream
        //
        cBytes = MAX_CALL;
        if (!ReadMemory(hProcess,
                        uoffRet - cBytes,
                        code,
                        cBytes,
                        &cb)) {

            //
            // if page is not present, we will ALWAYS screw up by
            // continuing to search.  If alloca was used also, we
            // are toast.  Too Bad.
            //
            if (cdwIndex == 0 && AcceptUnreadableCallSite) {
                return uoffStack;
            } else {
                continue;
            }
        }



        //
        // With 32bit code that isn't FAR:32 we don't have to worry about
        // intersegment calls.  Check here to see if we had a call within
        // segment.  If it is we can later check it's full diplacement if
        // necessary and see if it calls the FPO function.  We will also have
        // to check for thunks and see if maybe it called a JMP indirect which
        // called the FPO function. We will fail to find the caller if it was
        // a case of tail recursion where one function doesn't actually call
        // another but rather jumps to it.  This will only happen when a
        // function who's parameter list is void calls another function who's
        // parameter list is void and the call is made as the last statement
        // in the first function.  If the call to the first function was an
        // 0xE8 call we will fail to find it here because it didn't call the
        // FPO function but rather the FPO functions caller.  If we don't get
        // specific about our 0xE8 checks we will potentially see things that
        // look like return addresses but aren't.
        //

        if (( cBytes >= 5 ) && ( ( code[ 2 ] == 0xE8 ) || ( code[ 2 ] == 0xE9 ) )) {

            // We do math on 32 bit so we can ignore carry, and then sign extended
            uoffT = (ULONG64)(LONG64)(LONG)((DWORD)uoffRet + *( (UNALIGNED DWORD *) &code[3] ));

            //
            // See if it calls the function directly, or into the function
            //
            if (( uoffT >= funcAddr) && ( uoffT < (funcAddr + funcSize) ) ) {
                return uoffStack;
            }


            while ( cJmpChain < MAX_JMP_CHAIN ) {

                if (!ReadMemory(hProcess,
                                uoffT,
                                jmpBuffer,
                                sizeof(jmpBuffer),
                                &cb)) {
                    break;
                }

                if (cb != sizeof(jmpBuffer)) {
                    break;
                }

                //
                // Now we are going to check if it is a call to a JMP, that may
                // jump to the function
                //
                // If it is a relative JMP then calculate the destination
                // and save it in uoffT.  If it is an indirect JMP then read
                // the destination from where the JMP is inderecting through.
                //
                if ( *(LPBYTE)lpwJmp == 0xE9 ) {

                    // We do math on 32 bit so we can ignore carry, and then
                    // sign extended
                    uoffT = (ULONG64)(LONG64)(LONG) ((ULONG)uoffT +
                            *(UNALIGNED DWORD *)( jmpBuffer + sizeof(BYTE) ) + 5);

                } else if ( *lpwJmp == 0x25FF ) {

                    if ((!ReadMemory(hProcess,
                                     (ULONG64)(LONG64)(LONG) (
                                         *(UNALIGNED DWORD *)
                                         ((LPBYTE)lpwJmp+sizeof(WORD))),
                                     &uoffT,
                                     sizeof(DWORD),
                                     &cb)) || (cb != sizeof(DWORD))) {
                        uoffT = 0;
                        break;
                    }
                    uoffT =  (DWORD64)(LONG64)(LONG)uoffT;

                } else {
                    break;
                }

                //
                // If the destination is to the FPO function then we have
                // found the return address and thus the vEBP
                //
                if ( uoffT == funcAddr ) {
                    return uoffStack;
                }

                cJmpChain++;
            }

            //
            // We cache away the first 0xE8 call or 0xE9 jmp that we find in
            // the event we cant find anything else that looks like a return
            // address.  This is meant to protect us in the tail recursion case.
            //
            if ( !uoffBestGuess ) {
                uoffBestGuess = uoffStack;
            }
        }


        //
        // Now loop backward through the bytes read checking for a multi
        // byte call type from Grp5.  If we find an 0xFF then we need to
        // check the byte after that to make sure that the nnn bits of
        // the mod/rm byte tell us that it is a call.  It it is a call
        // then we will assume that this one called us because we can
        // no longer accurately determine for sure whether this did
        // in fact call the FPO function.  Since 0xFF calls are a guess
        // as well we will not check them if we already have an earlier guess.
        // It is more likely that the first 0xE8 called the function than
        // something higher up the stack that might be an 0xFF call.
        //
        if ( !uoffBestGuess && cBytes >= MIN_CALL ) {

            cbLimit = MAX_CALL - (INT)cBytes;

            for (cbIndex = MAX_CALL - MIN_CALL;
                 cbIndex >= cbLimit;  //MAX_CALL - (INT)cBytes;
                 cbIndex--) {

                if ( ( code [ cbIndex ] == 0xFF ) &&
                    ( ( code [ cbIndex + 1 ] & 0x30 ) == 0x10 )){

                    return uoffStack;

                }
            }
        }
    }

    //
    // we found nothing that was 100% definite so we'll return the best guess
    //
    return uoffBestGuess;
}


BOOL
GetFpoFrameBase(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    lpstkfrm,
    PFPO_DATA                         pFpoData,
    BOOL                              fFirstFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase
    )
{
    DWORD          Addr32;
    X86_KTRAP_FRAME    TrapFrame;
    DWORD64        OldFrameAddr;
    DWORD64        FrameAddr;
    DWORD64        StackAddr;
    DWORD64        ModuleBase;
    DWORD64        FuncAddr;
    DWORD          cb;
    DWORD64        StoredEbp;
    PFPO_DATA      PreviousFpoData = (PFPO_DATA)lpstkfrm->FuncTableEntry;

    //
    // calculate the address of the beginning of the function
    //
    ModuleBase = GetModuleBase( hProcess, lpstkfrm->AddrPC.Offset );
    if (!ModuleBase) {
        return FALSE;
    }

    FuncAddr = ModuleBase+pFpoData->ulOffStart;

    //
    // If this isn't the first/current frame then we can add back the count
    // bytes of locals and register pushed before beginning to search for
    // vEBP.  If we are beyond prolog we can add back the count bytes of locals
    // and registers pushed as well.  If it is the first frame and EIP is
    // greater than the address of the function then the SUB for locals has
    // been done so we can add them back before beginning the search.  If we
    // are right on the function then we will need to start our search at ESP.
    //

    if ( !fFirstFrame ) {

        OldFrameAddr = lpstkfrm->AddrFrame.Offset;
        FrameAddr = 0;

        //
        // if this is a non-fpo or trap frame, get the frame base now:
        //

        if (pFpoData->cbFrame != FRAME_FPO) {

            if (!PreviousFpoData || PreviousFpoData->cbFrame == FRAME_NONFPO) {

                //
                // previous frame base is ebp and points to this frame's ebp
                //
                ReadMemory(hProcess,
                           OldFrameAddr,
                           &Addr32,
                           sizeof(DWORD),
                           &cb);

                FrameAddr = (DWORD64)(LONG64)(LONG)Addr32;
            }

            //
            // if that didn't work, try for a saved ebp
            //
            if (!FrameAddr && SAVE_EBP(lpstkfrm)) {

                FrameAddr = SAVE_EBP(lpstkfrm);

            }

            //
            // this is not an FPO frame, so the saved EBP can only have come
            // from this or a lower frame.
            //

            SAVE_EBP(lpstkfrm) = 0;
        }

        //
        // still no frame base - either this frame is fpo, or we couldn't
        // follow the ebp chain.
        //

        if (FrameAddr == 0) {
            FrameAddr = OldFrameAddr;

            //
            // skip over return address from prev frame
            //
            FrameAddr += FRAME_SIZE;

            //
            // skip over this frame's locals and saved regs
            //
            FrameAddr += ( pFpoData->cdwLocals * STACK_SIZE );
            FrameAddr += ( pFpoData->cbRegs * STACK_SIZE );

            if (PreviousFpoData) {
                //
                // if the previous frame had an fpo record, we can account
                // for its parameters
                //
                FrameAddr += PreviousFpoData->cdwParams * STACK_SIZE;

            }
        }

        //
        // if this is an FPO frame
        // and the previous frame was non-fpo,
        // and this frame passed the inherited ebp to the previous frame,
        //  save its ebp
        //
        // (if this frame used ebp, SAVE_EBP will be set after verifying
        // the frame base)
        //
        if (pFpoData->cbFrame == FRAME_FPO &&
            (!PreviousFpoData || PreviousFpoData->cbFrame == FRAME_NONFPO) &&
            !pFpoData->fUseBP) {

            SAVE_EBP(lpstkfrm) = 0;

            if (ReadMemory(hProcess,
                           OldFrameAddr,
                           &Addr32,
                           sizeof(DWORD),
                           &cb)) {

                SAVE_EBP(lpstkfrm) = (DWORD64)(LONG64)(LONG)Addr32;
            }
        }


    } else {

        OldFrameAddr = lpstkfrm->AddrFrame.Offset;
        if (pFpoData->cbFrame == FRAME_FPO && !pFpoData->fUseBP) {
            //
            // this frame didn't use EBP, so it actually belongs
            // to a non-FPO frame further up the stack.  Stash
            // it in the save area for the next frame.
            //
            SAVE_EBP(lpstkfrm) = lpstkfrm->AddrFrame.Offset;
        }

        if (pFpoData->cbFrame == FRAME_TRAP ||
            pFpoData->cbFrame == FRAME_TSS) {

            FrameAddr = lpstkfrm->AddrFrame.Offset;

        } else if (lpstkfrm->AddrPC.Offset == FuncAddr) {

            FrameAddr = lpstkfrm->AddrStack.Offset;

        } else if (lpstkfrm->AddrPC.Offset >= FuncAddr+pFpoData->cbProlog) {

            FrameAddr = lpstkfrm->AddrStack.Offset +
                        ( pFpoData->cdwLocals * STACK_SIZE ) +
                        ( pFpoData->cbRegs * STACK_SIZE );

        } else {

            FrameAddr = lpstkfrm->AddrStack.Offset +
                        ( pFpoData->cdwLocals * STACK_SIZE );

        }

    }


    if (pFpoData->cbFrame == FRAME_TRAP) {

        //
        // read a kernel mode trap frame from the stack
        //

        if (!ReadTrapFrame( hProcess,
                            FrameAddr,
                            &TrapFrame,
                            ReadMemory )) {
            return FALSE;
        }

        SAVE_TRAP(lpstkfrm) = FrameAddr;
        TRAP_EDITED(lpstkfrm) = TrapFrame.SegCs & X86_FRAME_EDITED;

        lpstkfrm->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.Eip);
        lpstkfrm->AddrReturn.Mode = AddrModeFlat;
        lpstkfrm->AddrReturn.Segment = 0;

        return TRUE;
    }

    if (pFpoData->cbFrame == FRAME_TSS) {

        //
        // translate a tss to a kernel mode trap frame
        //

        StackAddr = FrameAddr;

        TaskGate2TrapFrame( hProcess, X86_KGDT_TSS, &TrapFrame, &StackAddr, ReadMemory );

        TRAP_TSS(lpstkfrm) = X86_KGDT_TSS;
        SAVE_TRAP(lpstkfrm) = StackAddr;

        lpstkfrm->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.Eip);
        lpstkfrm->AddrReturn.Mode = AddrModeFlat;
        lpstkfrm->AddrReturn.Segment = 0;

        return TRUE;
    }

    if ((pFpoData->cbFrame != FRAME_FPO) &&
        (pFpoData->cbFrame != FRAME_NONFPO) ) {
        //
        // we either have a compiler or linker problem, or possibly
        // just simple data corruption.
        //
        return FALSE;
    }

    //
    // go look for a return address.  this is done because, eventhough
    // we have subtracted all that we can from the frame pointer it is
    // possible that there is other unknown data on the stack.  by
    // searching for the return address we are able to find the base of
    // the fpo frame.
    //
    FrameAddr = SearchForReturnAddress( hProcess,
                                        FrameAddr,
                                        FuncAddr,
                                        pFpoData->cbProcSize,
                                        ReadMemory,
                                        GetModuleBase,
                                        PreviousFpoData != NULL
                                        );
    if (!FrameAddr) {
        return FALSE;
    }

    if (pFpoData->fUseBP && pFpoData->cbFrame == FRAME_FPO) {

        //
        // this function used ebp as a general purpose register, but
        // before doing so it saved ebp on the stack.  the prolog code
        // always saves ebp last so it is always at the top of the stack.
        //
        // we must retrieve this ebp and save it for possible later
        // use if we encounter a non-fpo frame
        //

        if (fFirstFrame && lpstkfrm->AddrPC.Offset < FuncAddr+pFpoData->cbProlog) {

            SAVE_EBP(lpstkfrm) = OldFrameAddr;

        } else {

            StackAddr = FrameAddr -
                ( ( pFpoData->cbRegs + pFpoData->cdwLocals ) * STACK_SIZE );

            SAVE_EBP(lpstkfrm) = 0;
            if (ReadMemory(hProcess,
                           StackAddr,
                           &Addr32,
                           sizeof(DWORD),
                           &cb)) {

                SAVE_EBP(lpstkfrm) = (DWORD64)(LONG64)(LONG)Addr32;
            }

        }
    }

    //
    // subtract the size for an ebp register if one had
    // been pushed.  this is done because the frames that
    // are virtualized need to appear as close to a real frame
    // as possible.
    //

    lpstkfrm->AddrFrame.Offset = FrameAddr - STACK_SIZE;

    return TRUE;
}


BOOL
ReadTrapFrame(
    HANDLE                            hProcess,
    DWORD64                           TrapFrameAddress,
    PX86_KTRAP_FRAME                  TrapFrame,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    DWORD cb;

    if (!ReadMemory(hProcess,
                    TrapFrameAddress,
                    TrapFrame,
                    sizeof(*TrapFrame),
                    &cb)) {
        return FALSE;
    }

    if (cb < sizeof(*TrapFrame)) {
        if (cb < sizeof(*TrapFrame) - 20) {
            //
            // shorter then the smallest possible frame type
            //
            return FALSE;
        }

        if ((TrapFrame->SegCs & 1) &&  cb < sizeof(*TrapFrame) - 16 ) {
            //
            // too small for inter-ring frame
            //
            return FALSE;
        }

        if (TrapFrame->EFlags & X86_EFLAGS_V86_MASK) {
            //
            // too small for V86 frame
            //
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
GetSelector(
    HANDLE                            hProcess,
    USHORT                            Processor,
    PX86_DESCRIPTOR_TABLE_ENTRY       pDescriptorTableEntry,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    ULONG_PTR   Address;
    PVOID       TableBase;
    USHORT      TableLimit;
    ULONG       Index;
    X86_LDT_ENTRY   Descriptor;
    ULONG       bytesread;


    //
    // Fetch the address and limit of the GDT
    //
    Address = (ULONG_PTR)&(((PX86_KSPECIAL_REGISTERS)0)->Gdtr.Base);
    ReadMemory( hProcess, Address, &TableBase, sizeof(TableBase), (LPDWORD)-1  );
    Address = (ULONG_PTR)&(((PX86_KSPECIAL_REGISTERS)0)->Gdtr.Limit);
    ReadMemory( hProcess, Address, &TableLimit, sizeof(TableLimit),  (LPDWORD)-1  );

    //
    // Find out whether this is a GDT or LDT selector
    //
    if (pDescriptorTableEntry->Selector & 0x4) {

        //
        // This is an LDT selector, so we reload the TableBase and TableLimit
        // with the LDT's Base & Limit by loading the descriptor for the
        // LDT selector.
        //

        if (!ReadMemory(hProcess,
                        (ULONG64)TableBase+X86_KGDT_LDT,
                        &Descriptor,
                        sizeof(Descriptor),
                        &bytesread)) {
            return FALSE;
        }

        TableBase = (PVOID)(DWORD_PTR)((ULONG)Descriptor.BaseLow +    // Sundown: zero-extension from ULONG to PVOID.
                    ((ULONG)Descriptor.HighWord.Bits.BaseMid << 16) +
                    ((ULONG)Descriptor.HighWord.Bytes.BaseHi << 24));

        TableLimit = Descriptor.LimitLow;  // LDT can't be > 64k

        if(Descriptor.HighWord.Bits.Granularity) {

            //
            //  I suppose it's possible, although silly, to have an
            //  LDT with page granularity.
            //
            TableLimit <<= X86_PAGE_SHIFT;
        }
    }

    Index = (USHORT)(pDescriptorTableEntry->Selector) & ~0x7;
                                                    // Irrelevant bits
    //
    // Check to make sure that the selector is within the table bounds
    //
    if (Index >= TableLimit) {

        //
        // Selector is out of table's bounds
        //

        return FALSE;
    }

    if (!ReadMemory(hProcess,
                    (ULONG64)TableBase+Index,
                    &(pDescriptorTableEntry->Descriptor),
                    sizeof(pDescriptorTableEntry->Descriptor),
                    &bytesread)) {
        return FALSE;
    }

    return TRUE;
}


BOOL
TaskGate2TrapFrame(
    HANDLE                            hProcess,
    USHORT                            TaskRegister,
    PX86_KTRAP_FRAME                  TrapFrame,
    PULONG64                          off,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory
    )
{
    X86_DESCRIPTOR_TABLE_ENTRY desc;
    ULONG                    bytesread;
    struct  {
        ULONG   r1[8];
        ULONG   Eip;
        ULONG   EFlags;
        ULONG   Eax;
        ULONG   Ecx;
        ULONG   Edx;
        ULONG   Ebx;
        ULONG   Esp;
        ULONG   Ebp;
        ULONG   Esi;
        ULONG   Edi;
        ULONG   Es;
        ULONG   Cs;
        ULONG   Ss;
        ULONG   Ds;
        ULONG   Fs;
        ULONG   Gs;
    } TaskState;


    //
    // Get the task register
    //

    desc.Selector = TaskRegister;
    if (!GetSelector(hProcess, 0, &desc, ReadMemory)) {
        return FALSE;
    }

    if (desc.Descriptor.HighWord.Bits.Type != 9  &&
        desc.Descriptor.HighWord.Bits.Type != 0xb) {
        //
        // not a 32bit task descriptor
        //
        return FALSE;
    }

    //
    // Read in Task State Segment
    //

    *off = ((ULONG)desc.Descriptor.BaseLow +
           ((ULONG)desc.Descriptor.HighWord.Bytes.BaseMid << 16) +
           ((ULONG)desc.Descriptor.HighWord.Bytes.BaseHi  << 24) );

    if (!ReadMemory(hProcess,
                    (ULONG64)(LONG64)(LONG)(*off),
                    &TaskState,
                    sizeof(TaskState),
                    &bytesread)) {
        return FALSE;
    }

    //
    // Move fields from Task State Segment to TrapFrame
    //

    ZeroMemory( TrapFrame, sizeof(*TrapFrame) );

    TrapFrame->Eip    = TaskState.Eip;
    TrapFrame->EFlags = TaskState.EFlags;
    TrapFrame->Eax    = TaskState.Eax;
    TrapFrame->Ecx    = TaskState.Ecx;
    TrapFrame->Edx    = TaskState.Edx;
    TrapFrame->Ebx    = TaskState.Ebx;
    TrapFrame->Ebp    = TaskState.Ebp;
    TrapFrame->Esi    = TaskState.Esi;
    TrapFrame->Edi    = TaskState.Edi;
    TrapFrame->SegEs  = TaskState.Es;
    TrapFrame->SegCs  = TaskState.Cs;
    TrapFrame->SegDs  = TaskState.Ds;
    TrapFrame->SegFs  = TaskState.Fs;
    TrapFrame->SegGs  = TaskState.Gs;
    TrapFrame->HardwareEsp = TaskState.Esp;
    TrapFrame->HardwareSegSs = TaskState.Ss;

    return TRUE;
}

BOOL
ProcessTrapFrame(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    lpstkfrm,
    PFPO_DATA                         pFpoData,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess
    )
{
    X86_KTRAP_FRAME TrapFrame;
    DWORD64         StackAddr;

    if (((PFPO_DATA)lpstkfrm->FuncTableEntry)->cbFrame == FRAME_TSS) {
        StackAddr = SAVE_TRAP(lpstkfrm);
        TaskGate2TrapFrame( hProcess, X86_KGDT_TSS, &TrapFrame, &StackAddr, ReadMemory );
    } else {
        if (!ReadTrapFrame( hProcess,
                            SAVE_TRAP(lpstkfrm),
                            &TrapFrame,
                            ReadMemory)) {
            SAVE_TRAP(lpstkfrm) = 0;
            return FALSE;
        }
    }

    pFpoData = (PFPO_DATA)
               FunctionTableAccess(hProcess,
                                   (DWORD64)(LONG64)(LONG)TrapFrame.Eip);

    if (!pFpoData) {
        lpstkfrm->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.Ebp;
        SAVE_EBP(lpstkfrm) = 0;
    } else {
        if ((TrapFrame.SegCs & X86_MODE_MASK) ||
            (TrapFrame.EFlags & X86_EFLAGS_V86_MASK)) {
            //
            // User-mode frame, real value of Esp is in HardwareEsp
            //
            lpstkfrm->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(TrapFrame.HardwareEsp - STACK_SIZE);
            lpstkfrm->AddrStack.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.HardwareEsp;

        } else {
            //
            // We ignore if Esp has been edited for now, and we will print a
            // separate line indicating this later.
            //
            // Calculate kernel Esp
            //

            if (((PFPO_DATA)lpstkfrm->FuncTableEntry)->cbFrame == FRAME_TRAP) {
                //
                // plain trap frame
                //
                if ((TrapFrame.SegCs & X86_FRAME_EDITED) == 0) {
                    lpstkfrm->AddrStack.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.TempEsp;
                } else {
                    lpstkfrm->AddrStack.Offset = (ULONG64)(LONG64)(LONG_PTR)
                        (& (((PX86_KTRAP_FRAME)SAVE_TRAP(lpstkfrm))->HardwareEsp) );
                }
            } else {
                //
                // tss converted to trap frame
                //
                lpstkfrm->AddrStack.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.HardwareEsp;
            }
        }
    }

    lpstkfrm->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.Ebp;
    lpstkfrm->AddrPC.Offset = (DWORD64)(LONG64)(LONG)TrapFrame.Eip;

    SAVE_TRAP(lpstkfrm) = 0;
    lpstkfrm->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
IsFarCall(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    BOOL                              *Ok,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL       fFar = FALSE;
    ULONG      cb;
    ADDRESS64  Addr;

    *Ok = TRUE;

    if (lpstkfrm->AddrFrame.Mode == AddrModeFlat) {
        DWORD      dwStk[ 3 ];
        //
        // If we are working with 32 bit offset stack pointers, we
        //      will say that the return address if far if the address
        //      treated as a FAR pointer makes any sense,  if not then
        //      it must be a near return
        //

        if (lpstkfrm->AddrFrame.Offset &&
            DoMemoryRead( &lpstkfrm->AddrFrame, dwStk, sizeof(dwStk), &cb )) {
            //
            //  See if segment makes sense
            //

            Addr.Offset   = (DWORD64)(LONG64)(LONG)(dwStk[1]);
            Addr.Segment  = (WORD)dwStk[2];
            Addr.Mode = AddrModeFlat;

            if (TranslateAddress( hProcess, hThread, &Addr ) && Addr.Offset) {
                fFar = TRUE;
            }
        } else {
            *Ok = FALSE;
        }
    } else {
        WORD       wStk[ 3 ];
        //
        // For 16 bit (i.e. windows WOW code) we do the following tests
        //      to check to see if an address is a far return value.
        //
        //      1.  if the saved BP register is odd then it is a far
        //              return values
        //      2.  if the address treated as a far return value makes sense
        //              then it is a far return value
        //      3.  else it is a near return value
        //

        if (lpstkfrm->AddrFrame.Offset &&
            DoMemoryRead( &lpstkfrm->AddrFrame, wStk, 6, &cb )) {

            if ( wStk[0] & 0x0001 ) {
                fFar = TRUE;
            } else {

                //
                //  See if segment makes sense
                //

                Addr.Offset   = wStk[1];
                Addr.Segment  = wStk[2];
                Addr.Mode = AddrModeFlat;

                if (TranslateAddress( hProcess, hThread, &Addr  ) && Addr.Offset) {
                    fFar = TRUE;
                }
            }
        } else {
            *Ok = FALSE;
        }
    }
    return fFar;
}


BOOL
SetNonOff32FrameAddress(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL    fFar;
    WORD    Stk[ 3 ];
    ULONG   cb;
    BOOL    Ok;

    fFar = IsFarCall( hProcess, hThread, lpstkfrm, &Ok, ReadMemory, TranslateAddress );

    if (!Ok) {
        return FALSE;
    }

    if (!DoMemoryRead( &lpstkfrm->AddrFrame, Stk, fFar ? FRAME_SIZE1632 : FRAME_SIZE16, &cb )) {
        return FALSE;
    }

    if (SAVE_EBP(lpstkfrm) > 0) {
        lpstkfrm->AddrFrame.Offset = SAVE_EBP(lpstkfrm) & 0xffff;
        lpstkfrm->AddrPC.Offset = Stk[1];
        if (fFar) {
            lpstkfrm->AddrPC.Segment = Stk[2];
        }
        SAVE_EBP(lpstkfrm) = 0;
    } else {
        if (Stk[1] == 0) {
            return FALSE;
        } else {
            lpstkfrm->AddrFrame.Offset = Stk[0];
            lpstkfrm->AddrFrame.Offset &= 0xFFFFFFFE;
            lpstkfrm->AddrPC.Offset = Stk[1];
            if (fFar) {
                lpstkfrm->AddrPC.Segment = Stk[2];
            }
        }
    }

    return TRUE;
}

VOID
GetFunctionParameters(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL                Ok;
    DWORD               cb;
    ADDRESS64           ParmsAddr;
    DWORD               Params[4];


    ParmsAddr = lpstkfrm->AddrFrame;

    //
    // calculate the frame size
    //
    if (lpstkfrm->AddrPC.Mode == AddrModeFlat) {

        ParmsAddr.Offset += FRAME_SIZE;

    } else
    if ( IsFarCall( hProcess, hThread, lpstkfrm, &Ok,
                    ReadMemory, TranslateAddress ) ) {

        lpstkfrm->Far = TRUE;
        ParmsAddr.Offset += FRAME_SIZE1632;

    } else {

        lpstkfrm->Far = FALSE;
        ParmsAddr.Offset += STACK_SIZE;

    }

    //
    // read the memory
    //
    if (DoMemoryRead( &ParmsAddr, Params, STACK_SIZE*4, &cb )) {
        lpstkfrm->Params[0] = (DWORD64)(LONG64)(LONG)(Params[0]);
        lpstkfrm->Params[1] = (DWORD64)(LONG64)(LONG)(Params[1]);
        lpstkfrm->Params[2] = (DWORD64)(LONG64)(LONG)(Params[2]);
        lpstkfrm->Params[3] = (DWORD64)(LONG64)(LONG)(Params[3]);
    } else {
        lpstkfrm->Params[0] =
        lpstkfrm->Params[1] =
        lpstkfrm->Params[2] =
        lpstkfrm->Params[3] = 0;
    }
}

VOID
GetReturnAddress(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    ULONG               cb;
    DWORD               stack[2];


    if (SAVE_TRAP(lpstkfrm)) {
        //
        // if a trap frame was encountered then
        // the return address was already calculated
        //
        return;
    }

    if (lpstkfrm->AddrPC.Mode == AddrModeFlat) {

        //
        // read the frame from the process's memory
        //
        if (!DoMemoryRead( &lpstkfrm->AddrFrame, stack, FRAME_SIZE, &cb )) {
            //
            // if we could not read the memory then set
            // the return address to zero so that the stack trace
            // will terminate
            //

            stack[1] = 0;

        }

        lpstkfrm->AddrReturn.Offset = (DWORD64)(LONG64)(LONG)(stack[1]);

    } else {

        lpstkfrm->AddrReturn.Offset = lpstkfrm->AddrPC.Offset;
        lpstkfrm->AddrReturn.Segment = lpstkfrm->AddrPC.Segment;

    }
}

BOOL
WalkX86_Fpo_Fpo(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    PFPO_DATA                         pFpoData,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL rval;

    rval = GetFpoFrameBase( hProcess,
                            lpstkfrm,
                            pFpoData,
                            FALSE,
                            ReadMemory,
                            GetModuleBase );

    lpstkfrm->FuncTableEntry = pFpoData;

    return rval;
}

BOOL
WalkX86_Fpo_NonFpo(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    PFPO_DATA                         pFpoData,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    DWORD       stack[FRAME_SIZE+STACK_SIZE];
    DWORD       cb;
    DWORD64     FrameAddr;
    DWORD64     FuncAddr;
    DWORD       FuncSize;
    BOOL        AcceptUnreadableCallsite = FALSE;

    //
    // if the previous frame was an seh frame then we must
    // retrieve the "real" frame pointer for this frame.
    // the seh function pushed the frame pointer last.
    //

    if (((PFPO_DATA)lpstkfrm->FuncTableEntry)->fHasSEH) {

        if (DoMemoryRead( &lpstkfrm->AddrFrame, stack, FRAME_SIZE+STACK_SIZE, &cb )) {

            lpstkfrm->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(stack[2]);
            lpstkfrm->AddrStack.Offset = (DWORD64)(LONG64)(LONG)(stack[2]);
            WalkX86Init(hProcess,
                        hThread,
                        lpstkfrm,
                        ReadMemory,
                        FunctionTableAccess,
                        GetModuleBase,
                        TranslateAddress);

            return TRUE;
        }
    }

    //
    // If a prior frame has stored this frame's EBP, just use it.
    //

    if (SAVE_EBP(lpstkfrm)) {

        lpstkfrm->AddrFrame.Offset = SAVE_EBP(lpstkfrm);
        FrameAddr = lpstkfrm->AddrFrame.Offset + 4;
        AcceptUnreadableCallsite = TRUE;

    } else {

        //
        // Skip past the FPO frame base and parameters.
        //
        lpstkfrm->AddrFrame.Offset +=
            (FRAME_SIZE + (((PFPO_DATA)lpstkfrm->FuncTableEntry)->cdwParams * 4));

        //
        // Now this is pointing to the bottom of the non-FPO frame.
        // If the frame has an fpo record, use it:
        //

        if (pFpoData) {
            FrameAddr = lpstkfrm->AddrFrame.Offset +
                            4* (pFpoData->cbRegs + pFpoData->cdwLocals);
            AcceptUnreadableCallsite = TRUE;
        } else {
            //
            // We don't know if the non-fpo frame has any locals, but
            // skip past the EBP anyway.
            //
            FrameAddr = lpstkfrm->AddrFrame.Offset + 4;
        }
    }

    //
    // at this point we may not be sitting at the base of the frame
    // so we now search for the return address and then subtract the
    // size of the frame pointer and use that address as the new base.
    //

    if (pFpoData) {
        FuncAddr = GetModuleBase(hProcess,lpstkfrm->AddrPC.Offset) + pFpoData->ulOffStart;
        FuncSize = pFpoData->cbProcSize;

    } else {
        FuncAddr = lpstkfrm->AddrPC.Offset - MAX_CALL;
        FuncSize = MAX_CALL;
    }



    FrameAddr = SearchForReturnAddress( hProcess,
                                        FrameAddr,
                                        FuncAddr,
                                        FuncSize,
                                        ReadMemory,
                                        GetModuleBase,
                                        AcceptUnreadableCallsite
                                        );
    if (FrameAddr) {
        lpstkfrm->AddrFrame.Offset = FrameAddr - STACK_SIZE;
    }

    if (!DoMemoryRead( &lpstkfrm->AddrFrame, stack, FRAME_SIZE, &cb )) {
        //
        // a failure means that we likely have a bad address.
        // returning zero will terminate that stack trace.
        //
        stack[0] = 0;
    }

    SAVE_EBP(lpstkfrm) = (DWORD64)(LONG64)(LONG)(stack[0]);

    lpstkfrm->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
WalkX86_NonFpo_Fpo(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    PFPO_DATA                         pFpoData,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL           rval;

    rval = GetFpoFrameBase( hProcess,
                            lpstkfrm,
                            pFpoData,
                            FALSE,
                            ReadMemory,
                            GetModuleBase );

    lpstkfrm->FuncTableEntry = pFpoData;

    return rval;
}

BOOL
WalkX86_NonFpo_NonFpo(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    PFPO_DATA                         pFpoData,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    DWORD       stack[FRAME_SIZE*4];
    DWORD       cb;

    //
    // a previous function in the call stack was a fpo function that used ebp as
    // a general purpose register.  ul contains the ebp value that was good  before
    // that function executed.  it is that ebp that we want, not what was just read
    // from the stack.  what was just read from the stack is totally bogus.
    //
    if (SAVE_EBP(lpstkfrm)) {

        lpstkfrm->AddrFrame.Offset = SAVE_EBP(lpstkfrm);
        SAVE_EBP(lpstkfrm) = 0;

    } else {

        //
        // read the first 2 dwords off the stack
        //
        if (!DoMemoryRead( &lpstkfrm->AddrFrame, stack, FRAME_SIZE, &cb )) {
            return FALSE;
        }

        lpstkfrm->AddrFrame.Offset = (DWORD64)(LONG64)(LONG)(stack[0]);
    }

    lpstkfrm->FuncTableEntry = pFpoData;

    return TRUE;
}

BOOL
WalkX86Next(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    PFPO_DATA      pFpoData = NULL;
    BOOL           rVal = TRUE;
    DWORD64        Address;
    DWORD          cb;
    DWORD64        ThisPC;
    DWORD64        ModuleBase;
    DWORD64        SystemRangeStart;


    if (AppVersion.Revision >= 6) {
        SystemRangeStart = (ULONG64)(LONG64)(LONG_PTR)(SYSTEM_RANGE_START(lpstkfrm));
    } else {
        //
        // This might not really work right with old debuggers, but it keeps
        // us from looking off the end of the structure anyway.
        //
        SystemRangeStart = 0xFFFFFFFF80000000;
    }


    ThisPC = lpstkfrm->AddrPC.Offset;

    //
    // the previous frame's return address is this frame's pc
    //
    lpstkfrm->AddrPC = lpstkfrm->AddrReturn;

    if (lpstkfrm->AddrPC.Mode != AddrModeFlat) {
        //
        // the call stack is from either WOW or a DOS app
        //
        SetNonOff32FrameAddress( hProcess,
                                 hThread,
                                 lpstkfrm,
                                 ReadMemory,
                                 FunctionTableAccess,
                                 GetModuleBase,
                                 TranslateAddress
                               );
        goto exit;
    }

    //
    // if the last frame was the usermode callback dispatcher,
    // switch over to the kernel stack:
    //

    ModuleBase = GetModuleBase(hProcess, ThisPC);

    if ((AppVersion.Revision >= 4) &&
        (CALLBACK_STACK(lpstkfrm) != 0) &&
        (pFpoData = (PFPO_DATA)lpstkfrm->FuncTableEntry) &&
        (CALLBACK_DISPATCHER(lpstkfrm) == ModuleBase + pFpoData->ulOffStart) )  {


      NextCallback:

        rVal = FALSE;

        //
        // find callout frame
        //

        if ((ULONG64)(LONG64)(LONG_PTR)(CALLBACK_STACK(lpstkfrm)) >= SystemRangeStart) {

            //
            // it is the pointer to the stack frame that we want,
            // or -1.

            Address = (ULONG64)(LONG64)(LONG) CALLBACK_STACK(lpstkfrm);

        } else {

            //
            // if it is below SystemRangeStart, it is the offset to
            // the address in the thread.
            // Look up the pointer:
            //

            rVal = ReadMemory(hProcess,
                              (CALLBACK_THREAD(lpstkfrm) +
                                 CALLBACK_STACK(lpstkfrm)),
                              &Address,
                              sizeof(DWORD),
                              &cb);

            Address = (ULONG64)(LONG64)(LONG)Address;

            if (!rVal || Address == 0) {
                Address = 0xffffffff;
                CALLBACK_STACK(lpstkfrm) = 0xffffffff;
            }

        }

        if ((Address == 0xffffffff) ||
            !(pFpoData = (PFPO_DATA) FunctionTableAccess( hProcess,
                                                 CALLBACK_FUNC(lpstkfrm))) ) {
            rVal = FALSE;

        } else {

            lpstkfrm->FuncTableEntry = pFpoData;

            lpstkfrm->AddrPC.Offset = CALLBACK_FUNC(lpstkfrm) +
                                                    pFpoData->cbProlog;

            lpstkfrm->AddrStack.Offset = Address;

            ReadMemory(hProcess,
                       Address + CALLBACK_FP(lpstkfrm),
                       &lpstkfrm->AddrFrame.Offset,
                       sizeof(DWORD),
                       &cb);

            lpstkfrm->AddrFrame.Offset = (ULONG64)(LONG64)(LONG)
                                         lpstkfrm->AddrFrame.Offset;

            ReadMemory(hProcess,
                       Address + CALLBACK_NEXT(lpstkfrm),
                       &CALLBACK_STACK(lpstkfrm),
                       sizeof(DWORD),
                       &cb);

            SAVE_TRAP(lpstkfrm) = 0;

            rVal = WalkX86Init(
                hProcess,
                hThread,
                lpstkfrm,
                ReadMemory,
                FunctionTableAccess,
                GetModuleBase,
                TranslateAddress
                );

        }

        return rVal;

    }

    //
    // if there is a trap frame then handle it
    //
    if (SAVE_TRAP(lpstkfrm)) {
        rVal = ProcessTrapFrame(
            hProcess,
            lpstkfrm,
            pFpoData,
            ReadMemory,
            FunctionTableAccess
            );
        if (!rVal) {
            return rVal;
        }
        rVal = WalkX86Init(
            hProcess,
            hThread,
            lpstkfrm,
            ReadMemory,
            FunctionTableAccess,
            GetModuleBase,
            TranslateAddress
            );
        return rVal;
    }

    //
    // if the PC address is zero then we're at the end of the stack
    //
    //if (GetModuleBase(hProcess, lpstkfrm->AddrPC.Offset) == 0)

    if (lpstkfrm->AddrPC.Offset < 65536) {

        //
        // if we ran out of stack, check to see if there is
        // a callback stack chain
        //
        if (AppVersion.Revision >= 4 && CALLBACK_STACK(lpstkfrm) != 0) {
            goto NextCallback;
        }

        return FALSE;
    }


    //
    // check to see if the current frame is an fpo frame
    //
    pFpoData = (PFPO_DATA) FunctionTableAccess(hProcess, lpstkfrm->AddrPC.Offset);


    if (pFpoData && pFpoData->cbFrame != FRAME_NONFPO) {

        if (lpstkfrm->FuncTableEntry && ((PFPO_DATA)lpstkfrm->FuncTableEntry)->cbFrame != FRAME_NONFPO) {

            rVal = WalkX86_Fpo_Fpo( hProcess,
                                  hThread,
                                  pFpoData,
                                  lpstkfrm,
                                  ReadMemory,
                                  FunctionTableAccess,
                                  GetModuleBase,
                                  TranslateAddress
                                );

        } else {

            rVal = WalkX86_NonFpo_Fpo( hProcess,
                                     hThread,
                                     pFpoData,
                                     lpstkfrm,
                                     ReadMemory,
                                     FunctionTableAccess,
                                     GetModuleBase,
                                     TranslateAddress
                                   );

        }
    } else {
        if (lpstkfrm->FuncTableEntry && ((PFPO_DATA)lpstkfrm->FuncTableEntry)->cbFrame != FRAME_NONFPO) {

            rVal = WalkX86_Fpo_NonFpo( hProcess,
                                     hThread,
                                     pFpoData,
                                     lpstkfrm,
                                     ReadMemory,
                                     FunctionTableAccess,
                                     GetModuleBase,
                                     TranslateAddress
                                   );

        } else {

            rVal = WalkX86_NonFpo_NonFpo( hProcess,
                                        hThread,
                                        pFpoData,
                                        lpstkfrm,
                                        ReadMemory,
                                        FunctionTableAccess,
                                        GetModuleBase,
                                        TranslateAddress
                                      );

        }
    }

exit:
    lpstkfrm->AddrFrame.Mode = lpstkfrm->AddrPC.Mode;
    lpstkfrm->AddrReturn.Mode = lpstkfrm->AddrPC.Mode;

    GetFunctionParameters( hProcess, hThread, lpstkfrm,
                           ReadMemory, GetModuleBase, TranslateAddress );

    GetReturnAddress( hProcess, hThread, lpstkfrm,
                      ReadMemory, GetModuleBase, TranslateAddress );

    return rVal;
}

BOOL
WalkX86Init(
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    lpstkfrm,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    UCHAR               code[3];
    DWORD               stack[FRAME_SIZE*4];
    PFPO_DATA           pFpoData = NULL;
    ULONG               cb;

    lpstkfrm->Virtual = TRUE;
    lpstkfrm->Reserved[0] =
    lpstkfrm->Reserved[1] =
    lpstkfrm->Reserved[2] = 0;
    lpstkfrm->AddrReturn = lpstkfrm->AddrPC;

    if (lpstkfrm->AddrPC.Mode != AddrModeFlat) {
        goto exit;
    }

    lpstkfrm->FuncTableEntry = pFpoData = (PFPO_DATA)
        FunctionTableAccess(hProcess, lpstkfrm->AddrPC.Offset);

    if (pFpoData && pFpoData->cbFrame != FRAME_NONFPO) {

        GetFpoFrameBase( hProcess,
                         lpstkfrm,
                         pFpoData,
                         TRUE,
                         ReadMemory,
                         GetModuleBase );

    } else {

        //
        // this code determines whether eip is in the function prolog
        //
        if (!DoMemoryRead( &lpstkfrm->AddrPC, code, 3, &cb )) {
            //
            // assume a call to a bad address if the memory read fails
            //
            code[0] = PUSHBP;
        }
        if ((code[0] == PUSHBP) || (*(LPWORD)&code[0] == MOVBPSP)) {
            SAVE_EBP(lpstkfrm) = lpstkfrm->AddrFrame.Offset;
            lpstkfrm->AddrFrame.Offset = lpstkfrm->AddrStack.Offset;
            if (lpstkfrm->AddrPC.Mode != AddrModeFlat) {
                lpstkfrm->AddrFrame.Offset &= 0xffff;
            }
            if (code[0] == PUSHBP) {
                if (lpstkfrm->AddrPC.Mode == AddrModeFlat) {
                    lpstkfrm->AddrFrame.Offset -= STACK_SIZE;
                } else {
                    lpstkfrm->AddrFrame.Offset -= STACK_SIZE16;
                }
            }
        } else {
            //
            // read the first 2 dwords off the stack
            //
            if (DoMemoryRead( &lpstkfrm->AddrFrame, stack, FRAME_SIZE, &cb )) {

                SAVE_EBP(lpstkfrm) = (ULONG64)(LONG64)(LONG)stack[0];

            }

            if (lpstkfrm->AddrPC.Mode != AddrModeFlat) {
                lpstkfrm->AddrFrame.Offset &= 0x0000FFFF;
            }
        }

    }

exit:
    lpstkfrm->AddrFrame.Mode = lpstkfrm->AddrPC.Mode;

    GetFunctionParameters( hProcess, hThread, lpstkfrm,
                           ReadMemory, GetModuleBase, TranslateAddress );

    GetReturnAddress( hProcess, hThread, lpstkfrm,
                      ReadMemory, GetModuleBase, TranslateAddress );

    return TRUE;
}
