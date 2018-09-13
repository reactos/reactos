/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    walkalpha.c

Abstract:

    This file implements the ALPHA stack walking api.

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
#include "symbols.h"
#include "alphaops.h"
#include <stdlib.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
#endif

BOOL
WalkAlphaInit(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PALPHA_NT5_CONTEXT                Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64
    );

BOOL
WalkAlphaNext(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PALPHA_NT5_CONTEXT                Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemoryRoutine,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64
    );

BOOL
WalkAlphaGetStackFrame(
    HANDLE                            hProcess,
    PULONG64                          ReturnAddress,
    PULONG64                          FramePointer,
    PALPHA_NT5_CONTEXT                Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    PKDHELP64                         KdHelp,
    BOOL                              Use64
    );

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
LookupDirectFunctionEntry (
    HANDLE                            hProcess,
    ULONG64                           ControlPc,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64
    );

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
LookupStaticFunctionEntry(
    HANDLE                            hProcess,
    ULONG64                           ControlPc,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64,
    PBOOLEAN                          InImage
    );

VOID
GetUnwindFunctionEntry(
    HANDLE                                hProcess,
    ULONG64                               ControlPc,
    PREAD_PROCESS_MEMORY_ROUTINE64        ReadMemory,
    PGET_MODULE_BASE_ROUTINE64            GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64      GetFunctionEntry,
    BOOL                                  Use64,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY UnwindFunctionEntry,
    PULONG                                StackAdjust,
    PULONG64                              FixedReturn
    );

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
FetchFunctionEntry (
    HANDLE                            hProcess,
    ULONG64                           Address,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    BOOL                              Use64
    );

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
PromoteFunctionEntry (
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY Address
    );

ULONG64
FunctionTableBase(
    HANDLE                            hProcess,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    BOOL                              Use64,
    ULONG64                           Base,
    PULONG                            Size
    );

#if DBG
void
ShowRuntimeFunction(
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    PSTR Label
    );

#define MAXENTRYTYPE 2
char *EntryTypeName[] = {
    "ALPHA_RF_NOT_CONTIGUOUS", // 0
    "ALPHA_RF_ALT_ENT_PROLOG", // 1
    "ALPHA_RF_NULL_CONTEXT",   // 2
    "***INVALID***"
};

BOOLEAN DebugFunctionEntries = 0;
#endif

#define ZERO 0x0                /* integer register 0 */
#define SP 0x1d                 /* integer register 29 */
#define RA 0x1f                 /* integer register 31 */
#define SAVED_FLOATING_MASK 0xfff00000 /* saved floating registers */
#define SAVED_INTEGER_MASK 0xf3ffff02 /* saved integer registers */
#define IS_FLOATING_SAVED(Register) ((SAVED_FLOATING_MASK >> Register) & 1L)
#define IS_INTEGER_SAVED(Register) ((SAVED_INTEGER_MASK >> Register) & 1L)
#define CALLBACK_STACK(f)  (f->KdHelp.ThCallbackStack)
#define CALLBACK_NEXT(f)   (f->KdHelp.NextCallback)
#define CALLBACK_FUNC(f)   (f->KdHelp.KiCallUserMode)
#define CALLBACK_THREAD(f) (f->KdHelp.Thread)
#define CALLBACK_FP(f)     (f->KdHelp.FramePointer)
#define CALLBACK_DISPATCHER(f) (f->KdHelp.KeUserCallbackDispatcher)
#define SYSTEM_RANGE_START(f) (f->KdHelp.SystemRangeStart)

// The function entry algorithms use the address of the function entry. This structure keeps
// both the function entry's address in the target process and its contents.

typedef struct {
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY  Value;        // copy of function entry
    ULONG64 Address;                                    // address of this function entry in target process
    HANDLE  Process;                                    // process id from which this function entry came
#if DBG
    PCHAR   Description;                                // description of address
#endif
} LOCAL_FUNCTION_ENTRY, *PLOCAL_FUNCTION_ENTRY;

// This array caches a small number of function entries to speed up the function entry binary search
#define LFE_MAX 60                                      // total number of cached function entries
#define LFE_NEW 40                                      // once the cache is full new entries will
                                                        // will be read into the part of the table
                                                        // beyond this index.
LOCAL_FUNCTION_ENTRY LocalFunctionEntry[LFE_MAX];
ULONG LocalFunctionEntryCount = 0;
ULONG LocalFunctionEntryNext  = 0;

// These two entries save the most recent primary and secondary function entry. This is a big performance
// improvement because the function entries used to get the return address and params from one call to
// WalkAlp are used to get the return PC in the next call (each WalkAlp() call does two virtual unwinds).

LOCAL_FUNCTION_ENTRY LastPrimaryFunctionEntry;          // Primary function entry from last call to walkalp
LOCAL_FUNCTION_ENTRY LastSecondaryFunctionEntry;        // Secondary function entry from last call to walkalp
LOCAL_FUNCTION_ENTRY LastDynamicFunctionEntry;          // Dynamic function entry from last registered FunctionEntryCallback
LOCAL_FUNCTION_ENTRY LastUserFunctionEntry;             // Function entry from last stack walk FunctionEntryCallback

// Macro to retrieve address of a fetched function entry.
// Dependent on RUNTIME_FUNCTION being the first member of LOCAL_FUNCTION_ENTRY.

#define FUNCTION_ENTRY_ADDRESS(pfe) ((pfe) != NULL? ((LOCAL_FUNCTION_ENTRY *)pfe)->Address : 0)
#define FUNCTION_ENTRY_DESCRIPTION(pfe) ((pfe) != NULL? ((LOCAL_FUNCTION_ENTRY *)pfe)->Description: "" )

#define IS_HANDLER_DEFINED(FunctionEntry) \
    (RF_EXCEPTION_HANDLER(FunctionEntry) != 0)

// These modifications of the RF_ macros are required because of the need for an explicit ULONG64 result

#define FIXED_RETURN(RF) (((ULONG64)(RF)->ExceptionHandler) & (~3))
#define ALT_PROLOG(RF)   (((ULONG64)(RF)->ExceptionHandler) & (~3))

BOOL
WalkAlpha(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PALPHA_NT5_CONTEXT                Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64
    )
{
    BOOL rval;

    if (StackFrame->Virtual) {

        rval = WalkAlphaNext( hProcess,
                              StackFrame,
                              Context,
                              ReadMemory,
                              GetModuleBase,
                              GetFunctionEntry,
                              Use64
                            );

    } else {

        rval = WalkAlphaInit( hProcess,
                              StackFrame,
                              Context,
                              ReadMemory,
                              GetModuleBase,
                              GetFunctionEntry,
                              Use64
                            );

    }

    return rval;
}


ULONG64
VirtualUnwind (
    HANDLE                                  hProcess,
    ULONG64                                 ControlPc,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY   FunctionEntry,
    ULONG                                   StackAdjust,
    ULONG64                                 FixedReturn,
    PALPHA_NT5_CONTEXT                      Context,
    PREAD_PROCESS_MEMORY_ROUTINE64          ReadMemory
//    PKNONVOLATILE_CONTEXT_POINTERS          ContextPointers OPTIONAL
    )

/*++

Routine Description:

    This function virtually unwinds the specified function by executing its
    prologue code backwards. Given the current context and the instructions
    that preserve registers in the prologue, it is possible to recreate the
    nonvolatile context at the point the function was called.

    If the function is a leaf function, then the address where control left
    the previous frame is obtained from the context record. If the function
    is a nested function, but not an exception or interrupt frame, then the
    prologue code is executed backwards and the address where control left
    the previous frame is obtained from the updated context record.

    Otherwise, an exception or interrupt entry to the system is being unwound
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is placed in the updated context record.

    During the unwind, the virtual and real frame pointers for the function
    are calculated and returned in the given frame pointers structure.

    If a context pointers record is specified, then the address where each
    register is restored from is recorded in the appropriate element of the
    context pointers record.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:

    The address where control left the previous frame is returned as the
    function value.

Implementation Notes:

    N.B. "where control left" is not the "return address" of the call in the
    previous frame. For normal frames, NextPc points to the last instruction
    that completed in the previous frame (the JSR/BSR). The difference between
    NextPc and NextPc + 4 (return address) is important for correct behavior
    in boundary cases of exception addresses and scope tables.

    For exception and interrupt frames, NextPc is obtained from the trap frame
    contination address (Fir). For faults and synchronous traps, NextPc is both
    the last instruction to execute in the previous frame and the next
    instruction to execute if the function were to return. For asynchronous
    traps, NextPc is the continuation address. It is the responsibility of the
    compiler to insert TRAPB instructions to insure asynchronous traps do not
    occur outside the scope from the instruction(s) that caused them.

    N.B. in this and other files where RtlVirtualUnwind is used, the variable
    named NextPc is perhaps more accurately, LastPc - the last PC value in
    the previous frame, or CallPc - the address of the call instruction, or
    ControlPc - the address where control left the previous frame. Instead
    think of NextPc as the next PC to use in another call to virtual unwind.

    The Alpha version of virtual unwind is similar in design, but slightly
    more complex than the Mips version. This is because Alpha compilers
    are given more flexibility to optimize generated code and instruction
    sequences, including within procedure prologues. And also because of
    compiler design issues, the function must manage both virtual and real
    frame pointers.

Version Information:  This version was taken from exdspatch.c@v37 (Feb 1993)

--*/

{
    ALPHA_INSTRUCTION FollowingInstruction;
    ALPHA_INSTRUCTION Instruction;
    ULONGLONG         Address;
    ULONG             DecrementOffset;
    ULONG             DecrementRegister;
    PULONGLONG        FloatingRegister;
    ULONG             FrameSize;
    ULONG             Function;
    PULONGLONG        IntegerRegister;
    ULONG             Literal8;
    ULONGLONG         NextPc;
    LONG              Offset16;
    ULONG             Opcode;
    ULONG             Ra;
    ULONG             Rb;
    ULONG             Rc;
    BOOLEAN           RestoredRa;
    BOOLEAN           RestoredSp;
    DWORD             cb;
    PVOID             Prolog;


    //
    // perf hack: fill cache with prolog
    // skip it if this is a secondary function entry
    //

    if (FunctionEntry &&
        (ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry) > ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) &&
        (ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry) < ALPHA_RF_END_ADDRESS(FunctionEntry)) ) {

        cb = (ULONG)(ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry) - ALPHA_RF_BEGIN_ADDRESS(FunctionEntry));

        //
        // If the function is a leaf it doesn't have a prolog, skip this
        // optimization.
        //

        if (cb != 0) {
            Prolog = (PVOID) MemAlloc( cb );
            if (!ReadMemory( hProcess,
                             ALPHA_RF_BEGIN_ADDRESS(FunctionEntry),
                             Prolog,
                             cb,
                             &cb )) {
                return 0;
            }
            MemFree(Prolog);
        }
    }

    //
    // Set the base address of the integer and floating register arrays within
    // the context record. Each set of 32 registers is known to be contiguous.
    //

    // assuming that quad values are together in context.

    IntegerRegister      = &Context->IntV0;
    FloatingRegister     = &Context->FltF0;

    //
    // Handle the epilogue case where the next instruction is a return.
    //
    // Exception handlers cannot be called if the ControlPc is within the
    // epilogue because exception handlers expect to operate with a current
    // stack frame. The value of SP is not current within the epilogue.
    //

    if (!ReadMemory(hProcess, ControlPc, &Instruction.Long, 4, &cb))  {
        return(0);
    }

    if (IS_RETURN_0001_INSTRUCTION(Instruction.Long)) {
        Rb = Instruction.Jump.Rb;
        NextPc = IntegerRegister[Rb] - 4;

        //
        // The instruction at the point where control left the specified
        // function is a return, so any saved registers have already been
        // restored, and the stack pointer has already been adjusted. The
        // stack does not need to be unwound in this case and the saved
        // return address register is returned as the function value.
        //
        // In fact, reverse execution of the prologue is not possible in
        // this case: the stack pointer has already been incremented and
        // so, for this frame, neither a valid stack pointer nor frame
        // pointer exists from which to begin reverse execution of the
        // prologue. In addition, the integrity of any data on the stack
        // below the stack pointer is never guaranteed (due to interrupts
        // and exceptions).
        //
        // The epilogue instruction sequence is:
        //
        // ==>  ret   zero, (Ra), 1     // return
        // or
        //
        //      mov   ra, Rx            // save return address
        //      ...
        // ==>  ret   zero, (Rx), 1     // return
        //

        return NextPc;
    }

    //
    // Handle the epilogue case where the next two instructions are a stack
    // frame deallocation and a return.
    //

    if (!ReadMemory(hProcess,(ControlPc+4),&FollowingInstruction.Long,4,&cb)) {
        return 0;
    }

    if (IS_RETURN_0001_INSTRUCTION(FollowingInstruction.Long)) {
        Rb = FollowingInstruction.Jump.Rb;
        NextPc = IntegerRegister[Rb] - 4;

        //
        // The second instruction following the point where control
        // left the specified function is a return. If the instruction
        // before the return is a stack increment instruction, then all
        // saved registers have already been restored except for SP.
        // The value of the stack pointer register cannot be recovered
        // through reverse execution of the prologue because in order
        // to begin reverse execution either the stack pointer or the
        // frame pointer (if any) must still be valid.
        //
        // Instead, the effect that the stack increment instruction
        // would have had on the context is manually applied to the
        // current context. This is forward execution of the epilogue
        // rather than reverse execution of the prologue.
        //
        // In an epilogue, as in a prologue, the stack pointer is always
        // adjusted with a single instruction: either an immediate-value
        // (lda) or a register-value (addq) add instruction.
        //

        Function = Instruction.OpReg.Function;
        Offset16 = Instruction.Memory.MemDisp;
        Opcode = Instruction.OpReg.Opcode;
        Ra = Instruction.OpReg.Ra;
        Rb = Instruction.OpReg.Rb;
        Rc = Instruction.OpReg.Rc;

        if ((Opcode == LDA_OP) && (Ra == SP_REG)) {

            //
            // Load Address instruction.
            //
            // Since the destination (Ra) register is SP, an immediate-
            // value stack deallocation operation is being performed. The
            // displacement value should be added to SP. The displacement
            // value is assumed to be positive. The amount of stack
            // deallocation possible using this instruction ranges from
            // 16 to 32752 (32768 - 16) bytes. The base register (Rb) is
            // usually SP, but may be another register.
            //
            // The epilogue instruction sequence is:
            //
            // ==>  lda   sp, +N(sp)        // deallocate stack frame
            //      ret   zero, (ra)        // return
            // or
            //
            // ==>  lda   sp, +N(Rx)        // restore SP and deallocate frame
            //      ret   zero, (ra)        // return
            //

            Context->IntSp = Offset16 + IntegerRegister[Rb];
            return NextPc;

        } else if ((Opcode == ARITH_OP) && (Function == ADDQ_FUNC) &&
                   (Rc == SP_REG) &&
                   (Instruction.OpReg.RbvType == RBV_REGISTER_FORMAT)) {

            //
            // Add Quadword instruction.
            //
            // Since both source operands are registers, and the
            // destination register is SP, a register-value stack
            // deallocation is being performed. The value of the two
            // source registers should be added and this is the new
            // value of SP. One of the source registers is usually SP,
            // but may be another register.
            //
            // The epilogue instruction sequence is:
            //
            //      ldiq  Rx, N             // set [large] frame size
            //      ...
            // ==>  addq  sp, Rx, sp        // deallocate stack frame
            //      ret   zero, (ra)        // return
            // or
            //
            // ==>  addq  Rx, Ry, sp        // restore SP and deallocate frame
            //      ret   zero, (ra)        // return
            //

            Context->IntSp = IntegerRegister[Ra] + IntegerRegister[Rb];
            return NextPc;
        }
    }

    //
    // By default set the frame pointers to the current value of SP.
    //
    // When a procedure is called, the value of SP before the stack
    // allocation instruction is the virtual frame pointer. When reverse
    // executing instructions in the prologue, the value of SP before the
    // stack allocation instruction is encountered is the real frame
    // pointer. This is the current value of SP unless the procedure uses
    // a frame pointer (e.g., FP_REG).
    //

    //
    // If the address where control left the specified function is beyond
    // the end of the prologue, then the control PC is considered to be
    // within the function and the control address is set to the end of
    // the prologue. Otherwise, the control PC is not considered to be
    // within the function (i.e., the prologue).
    //
    // N.B. PrologEndAddress is equal to BeginAddress for a leaf function.
    //
    // The low-order two bits of PrologEndAddress are reserved for the IEEE
    // exception mode and so must be masked out.
    //

    if ((ControlPc < ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) ||
        (ControlPc >= ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry))) {
        ControlPc = ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry);
    }

    //
    // Scan backward through the prologue to reload callee saved registers
    // that were stored or copied and to increment the stack pointer if it
    // was decremented.
    //

    DecrementRegister = ZERO_REG;
    NextPc = Context->IntRa - 4;
    RestoredRa = FALSE;
    RestoredSp = FALSE;
    while (ControlPc > ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) {

        //
        // Get instruction value, decode fields, case on opcode value, and
        // reverse register store and stack decrement operations.
        // N.B. The location of Opcode, Ra, Rb, and Rc is the same across
        // all opcode formats. The same is not true for Function.
        //

        ControlPc -= 4;
        if (!ReadMemory(hProcess, ControlPc, &Instruction.Long, 4, &cb)) {
             return 0;
        }
        Function = Instruction.OpReg.Function;
        Literal8 = Instruction.OpLit.Literal;
        Offset16 = Instruction.Memory.MemDisp;
        Opcode = Instruction.OpReg.Opcode;
        Ra = Instruction.OpReg.Ra;
        Rb = Instruction.OpReg.Rb;
        Rc = Instruction.OpReg.Rc;

        //
        // Compare against each instruction type that will affect the context
        // and that is allowed in a prologue. Any other instructions found
        // in the prologue will be ignored since they are assumed to have no
        // effect on the context.
        //

        switch (Opcode) {

        case STQ_OP :

            //
            // Store Quad instruction.
            //
            // If the base register is SP, then reload the source register
            // value from the value stored on the stack.
            //
            // The prologue instruction sequence is:
            //
            // ==>  stq   Rx, N(sp)         // save integer register Rx
            //

            if ((Rb == SP_REG) && (Ra != ZERO_REG)) {

                //
                // Reload the register by retrieving the value previously
                // stored on the stack.
                //

                Address = (Offset16 + Context->IntSp);
                if (!ReadMemory(hProcess, Address, &IntegerRegister[Ra], 8L, &cb)) {
                    return 0;
                }

                //
                // If the destination register is RA and this is the first
                // time that RA is being restored, then set the address of
                // where control left the previous frame. Otherwise, if this
                // is the second time RA is being restored, then the first
                // one was an interrupt or exception address and the return
                // PC should not have been biased by 4.
                //

                if (Ra == RA_REG) {
                    if (RestoredRa == FALSE) {
                        NextPc = Context->IntRa - 4;
                        RestoredRa = TRUE;

                    } else {
                        NextPc += 4;
                    }

                //
                // Otherwise, if the destination register is SP and this is
                // the first time that SP is being restored, then set the
                // establisher frame pointers.
                //

                } else if ((Ra == SP_REG) && (RestoredSp == FALSE)) {
                    RestoredSp = TRUE;
                }

                //
                // If a context pointer record is specified, then record
                // the address where the destination register contents
                // are stored.
                //

                //if (ContextPointers != (PKNONVOLATILE_CONTEXT_POINTERS) NULL) {
                //    ContextPointers->IntegerContext[Ra] = (PULONGLONG)Address;
                //}
            }
            break;

        case LDAH_OP :
            Offset16 <<= 16;

        case LDA_OP :

            //
            // Load Address High, Load Address instruction.
            //
            // There are several cases where the lda and/or ldah instructions
            // are used: one to decrement the stack pointer directly, and the
            // others to load immediate values into another register and that
            // register is then used to decrement the stack pointer.
            //
            // In the examples below, as a single instructions or as a pair,
            // a lda may be substituted for a ldah and visa-versa.
            //

            if (Ra == SP_REG) {
                if (Rb == SP_REG) {

                    //
                    // If both the destination (Ra) and base (Rb) registers
                    // are SP, then a standard stack allocation was performed
                    // and the negated displacement value is the stack frame
                    // size. The amount of stack allocation possible using
                    // the lda instruction ranges from 16 to 32768 bytes and
                    // the amount of stack allocation possible using the ldah
                    // instruction ranges from 65536 to 2GB in multiples of
                    // 65536 bytes. It is rare for the ldah instruction to be
                    // used in this manner.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  lda   sp, -N(sp)    // allocate stack frame
                    //

                    FrameSize = -Offset16;
                    goto StackAllocation;

                } else {

                    //
                    // The destination register is SP and the base register
                    // is not SP, so this instruction must be the second
                    // half of an instruction pair to allocate a large size
                    // (>32768 bytes) stack frame. Save the displacement value
                    // as the partial decrement value and postpone adjusting
                    // the value of SP until the first instruction of the pair
                    // is encountered.
                    //
                    // The prologue instruction sequence is:
                    //
                    //      ldah  Rx, -N(sp)    // prepare new SP (upper)
                    // ==>  lda   sp, sN(Rx)    // allocate stack frame
                    //

                    DecrementRegister = Rb;
                    DecrementOffset = Offset16;
                }

            } else if (Ra == DecrementRegister) {
                if (Rb == DecrementRegister) {

                    //
                    // Both the destination and base registers are the
                    // decrement register, so this instruction exists as the
                    // second half of a two instruction pair to load a
                    // 31-bit immediate value into the decrement register.
                    // Save the displacement value as the partial decrement
                    // value.
                    //
                    // The prologue instruction sequence is:
                    //
                    //      ldah  Rx, +N(zero)      // set frame size (upper)
                    // ==>  lda   Rx, sN(Rx)        // set frame size (+lower)
                    //      ...
                    //      subq  sp, Rx, sp        // allocate stack frame
                    //

                    DecrementOffset += Offset16;

                } else if (Rb == ZERO_REG) {

                    //
                    // The destination register is the decrement register and
                    // the base register is zero, so this instruction exists
                    // to load an immediate value into the decrement register.
                    // The stack frame size is the new displacement value added
                    // to the previous displacement value, if any.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  lda   Rx, +N(zero)      // set frame size
                    //      ...
                    //      subq  sp, Rx, sp        // allocate stack frame
                    // or
                    //
                    // ==>  ldah  Rx, +N(zero)      // set frame size (upper)
                    //      lda   Rx, sN(Rx)        // set frame size (+lower)
                    //      ...
                    //      subq  sp, Rx, sp        // allocate stack frame
                    //

                    FrameSize = (Offset16 + DecrementOffset);
                    goto StackAllocation;

                } else if (Rb == SP_REG) {

                    //
                    // The destination (Ra) register is SP and the base (Rb)
                    // register is the decrement register, so a two
                    // instruction, large size (>32768 bytes) stack frame
                    // allocation was performed. Add the new displacement
                    // value to the previous displacement value. The negated
                    // displacement value is the stack frame size.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  ldah  Rx, -N(sp)    // prepare new SP (upper)
                    //      lda   sp, sN(Rx)    // allocate stack frame
                    //

                    FrameSize = -(Offset16 + (LONG)DecrementOffset);
                    goto StackAllocation;
                }
            }
            break;

        case ARITH_OP :

            if ((Function == ADDQ_FUNC) &&
                (Instruction.OpReg.RbvType != RBV_REGISTER_FORMAT)) {

                //
                // Add Quadword (immediate) instruction.
                //
                // If the first source register is zero, and the second
                // operand is a literal, and the destination register is
                // the decrement register, then the instruction exists
                // to load an unsigned immediate value less than 256 into
                // the decrement register. The immediate value is the stack
                // frame size.
                //
                // The prologue instruction sequence is:
                //
                // ==>  addq  zero, N, Rx       // set frame size
                //      ...
                //      subq  sp, Rx, sp        // allocate stack frame
                //

                if ((Ra == ZERO_REG) && (Rc == DecrementRegister)) {
                    FrameSize = Literal8;
                    goto StackAllocation;
                }

            } else if ((Function == SUBQ_FUNC) &&
                       (Instruction.OpReg.RbvType == RBV_REGISTER_FORMAT)) {

                //
                // Subtract Quadword (register) instruction.
                //
                // If both source operands are registers and the first
                // source (minuend) register and the destination
                // (difference) register are both SP, then a register value
                // stack allocation was performed and the second source
                // (subtrahend) register value will be added to SP when its
                // value is known. Until that time save the register number of
                // this decrement register.
                //
                // The prologue instruction sequence is:
                //
                //      ldiq  Rx, N             // set frame size
                //      ...
                // ==>  subq  sp, Rx, sp        // allocate stack frame
                //

                if ((Ra == SP_REG) && (Rc == SP_REG)) {
                    DecrementRegister = Rb;
                    DecrementOffset = 0;
                }
            }
            break;

        case BIT_OP :

            //
            // If the second operand is a register the bit set instruction
            // may be a register move instruction, otherwise if the second
            // operand is a literal, the bit set instruction may be a load
            // immediate value instruction.
            //

            if ((Function == BIS_FUNC) && (Rc != ZERO_REG)) {
                if (Instruction.OpReg.RbvType == RBV_REGISTER_FORMAT) {

                    //
                    // Bit Set (register move) instruction.
                    //
                    // If both source registers are the same register, or
                    // one of the source registers is zero, then this is a
                    // register move operation. Restore the value of the
                    // source register by copying the current destination
                    // register value back to the source register.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  bis   Rx, Rx, Ry        // copy register Rx
                    // or
                    //
                    // ==>  bis   Rx, zero, Ry      // copy register Rx
                    // or
                    //
                    // ==>  bis   zero, Rx, Ry      // copy register Rx
                    //

                    if (Ra == ZERO_REG) {

                        //
                        // Map the third case above to the first case.
                        //

                        Ra = Rb;

                    } else if (Rb == ZERO_REG) {

                        //
                        // Map the second case above to the first case.
                        //

                        Rb = Ra;
                    }

                    if ((Ra == Rb) && (Ra != ZERO_REG)) {
                        IntegerRegister[Ra] = IntegerRegister[Rc];


                        //
                        // If the destination register is RA and this is the
                        // first time that RA is being restored, then set the
                        // address of where control left the previous frame.
                        // Otherwise, if this is the second time RA is being
                        // restored, then the first one was an interrupt or
                        // exception address and the return PC should not
                        // have been biased by 4.
                        //

                        if (Ra == RA_REG) {
                            if (RestoredRa == FALSE) {
                                NextPc = Context->IntRa - 4;
                                RestoredRa = TRUE;

                            } else {
                                NextPc += 4;
                            }
                        }

                        //
                        // If the source register is SP and this is the first
                        // time SP is set, then this is a frame pointer set
                        // instruction. Reset the frame pointers to this new
                        // value of SP.
                        //

                        if ((Ra == SP_REG) && (RestoredSp == FALSE)) {
                            RestoredSp = TRUE;
                        }
                    }

                } else {

                    //
                    // Bit Set (load immediate) instruction.
                    //
                    // If the first source register is zero, and the second
                    // operand is a literal, and the destination register is
                    // the decrement register, then this instruction exists
                    // to load an unsigned immediate value less than 256 into
                    // the decrement register. The decrement register value is
                    // the stack frame size.
                    //
                    // The prologue instruction sequence is:
                    //
                    // ==>  bis   zero, N, Rx       // set frame size
                    //      ...
                    //      subq  sp, Rx, sp        // allocate stack frame
                    //

                    if ((Ra == ZERO_REG) && (Rc == DecrementRegister)) {
                        FrameSize = Literal8;
StackAllocation:
                        //
                        // Add the frame size to SP to reverse the stack frame
                        // allocation, leave the real frame pointer as is, set
                        // the virtual frame pointer with the updated SP value,
                        // and clear the decrement register.
                        //

                        Context->IntSp += FrameSize;
                        DecrementRegister = ZERO_REG;
                    }
                }
            }
            break;

        case STT_OP :

            //
            // Store T-Floating (quadword integer) instruction.
            //
            // If the base register is SP, then reload the source register
            // value from the value stored on the stack.
            //
            // The prologue instruction sequence is:
            //
            // ==>  stt   Fx, N(sp)         // save floating register Fx
            //

            if ((Rb == SP_REG) && (Ra != FZERO_REG)) {

                //
                // Reload the register by retrieving the value previously
                // stored on the stack.
                //

                Address = (Offset16 + Context->IntSp);
                if (!ReadMemory(hProcess, Address, &FloatingRegister[Ra], 8L, &cb)) {
                    return 0;
                }

                //
                // If a context pointer record is specified, then record
                // the address where the destination register contents are
                // stored.
                //

                //if (ContextPointers != (PKNONVOLATILE_CONTEXT_POINTERS) NULL) {
                //    ContextPointers->FloatingContext[Ra] = (PULONGLONG)Address;
                //}
            }
            break;


        case STS_OP :

            //
            // Store T-Floating (dword integer) instruction.
            //
            // If the base register is SP, then reload the source register
            // value from the value stored on the stack.
            //
            // The prologue instruction sequence is:
            //
            // ==>  stt   Fx, N(sp)         // save floating register Fx
            //

            if ((Rb == SP_REG) && (Ra != FZERO_REG)) {

                //
                // Reload the register by retrieving the value previously
                // stored on the stack.
                //

                float f;

                Address = (Offset16 + Context->IntSp);
                if (!ReadMemory(hProcess, Address, &f, sizeof(float), &cb)) {
                    return 0;
                }

                //
                // value was stored as a float.  Do a conversion to a
                // double, since registers are Always read as doubles
                //
                FloatingRegister[Ra] = (ULONGLONG)(double)f;

                //
                // If a context pointer record is specified, then record
                // the address where the destination register contents are
                // stored.
                //

                //if (ContextPointers != (PKNONVOLATILE_CONTEXT_POINTERS) NULL) {
                //    ContextPointers->FloatingContext[Ra] = (PULONGLONG)Address;
                //}
            }
            break;

        case FPOP_OP :

            //
            // N.B. The floating operate function field is not the same as
            // the integer operate nor the jump function fields.
            //

            if (Instruction.FpOp.Function == CPYS_FUNC) {

                //
                // Copy Sign (floating-point move) instruction.
                //
                // If both source registers are the same register, then this is
                // a floating-point register move operation. Restore the value
                // of the source register by copying the current destination
                // register value to the source register.
                //
                // The prologue instruction sequence is:
                //
                // ==>  cpys  Fx, Fx, Fy        // copy floating register Fx
                //

                if ((Ra == Rb) && (Ra != FZERO_REG)) {
                    FloatingRegister[Ra] = FloatingRegister[Rc];
                }
            }

        default :
            break;
        }
    }

    if (StackAdjust) {
        // Check for exlicit stack adjust amount

        Context->IntSp += StackAdjust;
    }

    if (FixedReturn != 0) {
        NextPc = FixedReturn;
    }

    return NextPc;
}

void
ConvertAlphaRf32To64(
    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY rf32,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY rf64
    )
{
    rf64->BeginAddress     = (ULONG64)(LONG64)(LONG)rf32->BeginAddress;
    rf64->EndAddress       = (ULONG64)(LONG64)(LONG)rf32->EndAddress;
    rf64->ExceptionHandler = (ULONG64)(LONG64)(LONG)rf32->ExceptionHandler;
    rf64->HandlerData      = (ULONG64)(LONG64)(LONG)rf32->HandlerData;
    rf64->PrologEndAddress = (ULONG64)(LONG64)(LONG)rf32->PrologEndAddress;
}

BOOL
WalkAlphaGetStackFrame(
    HANDLE                            hProcess,
    PULONG64                          ReturnAddress,
    PULONG64                          FramePointer,
    PALPHA_NT5_CONTEXT                Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    PKDHELP64                         KdHelp,
    BOOL                              Use64
    )
{
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY rf64;
    LOCAL_FUNCTION_ENTRY             UnwindFunctionEntry;
    ULONG64                          NextPc = Context->IntRa;
    BOOL                             rval = TRUE;
    ULONG                            cb;
    ULONG                            StackAdjust;
    ULONG64                          FixedReturn;


    if (*ReturnAddress == 0) {
        return FALSE;
    }

    __try {
        rf64 = LookupFunctionEntry( hProcess, *ReturnAddress, ReadMemory, GetModuleBase, GetFunctionEntry, Use64 );

        if (rf64) {

            // Construct a function entry suitable for unwinding from ControlPc

            UnwindFunctionEntry.Address = 0;
            UnwindFunctionEntry.Process = 0;

            GetUnwindFunctionEntry( hProcess, *ReturnAddress, ReadMemory, GetModuleBase, GetFunctionEntry, Use64, rf64,
                                    &UnwindFunctionEntry.Value, &StackAdjust, &FixedReturn );
#if DBG
            UnwindFunctionEntry.Description = "from UnwindFunctionEntry";
            ShowRuntimeFunction(&UnwindFunctionEntry.Value, "VirtualUnwind: unwind function entry");
            if (DebugFunctionEntries) {
                dbPrint("    FixedReturn      = %16.8I64x\n", FixedReturn );
                dbPrint("    StackAdjust      = %16x\n", StackAdjust );
            }
#endif

            NextPc = VirtualUnwind( hProcess, *ReturnAddress, &UnwindFunctionEntry.Value, StackAdjust, FixedReturn, Context, ReadMemory);
#if DBG
            if (DebugFunctionEntries) {
                dbPrint("NextPc = %.8I64x\n", NextPc );
            }
#endif
            if (!NextPc) {
                rval = FALSE;
            }

            //
            // The Ra value coming out of mainCRTStartup is set by some RTL
            // routines to be "1"; return out of mainCRTStartup is actually
            // done through Jump/Unwind, so this serves to cause an error if
            // someone actually does a return.  That's why we check here for
            // NextPc == 1 - this happens when in the frame for CRTStartup.
            //
            // We test for (0-4) and (1-4) because on ALPHA, the value returned by
            // VirtualUnwind is the value to be passed to the next call to
            // VirtualUnwind, which is NOT the same as the Ra - it's sometimes
            // decremented by four - this gives the faulting instruction -
            // in particular, we want the fault instruction so we can get the
            // correct scope in the case of an exception.
            //
            if ((NextPc == 1) || (NextPc == 4) || (NextPc == (0-4)) || (NextPc == (1-4)) ) {
                NextPc = 0;
            }
            if ( !NextPc || (NextPc == *ReturnAddress && *FramePointer == Context->IntSp) ) {
                rval = FALSE;
            }

            *ReturnAddress = NextPc;
            *FramePointer  = Context->IntSp;

        } else {

            if ( (NextPc == *ReturnAddress && *FramePointer == Context->IntSp) ||
                 (NextPc == 1) || (NextPc == 0) || (NextPc == (-4)) ) {
                rval = FALSE;
            }

            *ReturnAddress = Context->IntRa;
            *FramePointer  = Context->IntSp;

        }

    } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                    EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        rval = FALSE;
    }

    return rval;
}


BOOL
WalkAlphaInit(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PALPHA_NT5_CONTEXT                Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64
    )
{
    ALPHA_NT5_CONTEXT  ContextSave;
    ULONG64            PcOffset;
    ULONG64            FrameOffset;
    DWORD              cb;
    ALPHA_KEXCEPTION_FRAME   ExceptionFrame;
    PALPHA_KEXCEPTION_FRAME  pef = &ExceptionFrame;
    DWORD              Result;

    if (StackFrame->AddrFrame.Offset) {
        if (ReadMemory( hProcess,
                        StackFrame->AddrFrame.Offset,
                        &ExceptionFrame,
                        sizeof(ALPHA_KEXCEPTION_FRAME),
                        &cb )) {
            //
            // successfully read an exception frame from the stack
            //
            Context->IntSp  = StackFrame->AddrFrame.Offset;
            Context->Fir    = pef->SwapReturn;
            Context->IntRa  = pef->SwapReturn;
            Context->IntS0  = pef->IntS0;
            Context->IntS1  = pef->IntS1;
            Context->IntS2  = pef->IntS2;
            Context->IntS3  = pef->IntS3;
            Context->IntS4  = pef->IntS4;
            Context->IntS5  = pef->IntS5;
            Context->Psr    = pef->Psr;
        } else {
            return FALSE;
        }
    }

    ZeroMemory( StackFrame, FIELD_OFFSET( STACKFRAME64, KdHelp.ThCallbackBStore) );

    StackFrame->Virtual = TRUE;

    StackFrame->AddrPC.Offset       = Context->Fir;
    StackFrame->AddrPC.Mode         = AddrModeFlat;

    StackFrame->AddrFrame.Offset    = Context->IntSp;
    StackFrame->AddrFrame.Mode      = AddrModeFlat;

    ContextSave = *Context;
    PcOffset    = StackFrame->AddrPC.Offset;
    FrameOffset = StackFrame->AddrFrame.Offset;

    if (!WalkAlphaGetStackFrame( hProcess,
                        &PcOffset,
                        &FrameOffset,
                        &ContextSave,
                        ReadMemory,
                        GetModuleBase,
                        GetFunctionEntry,
                        &StackFrame->KdHelp,
                        Use64) ) {

        StackFrame->AddrReturn.Offset = Context->IntRa;

    } else {

        StackFrame->AddrReturn.Offset = PcOffset;
    }

    StackFrame->AddrReturn.Mode     = AddrModeFlat;

    //
    // get the arguments to the function
    //
    StackFrame->Params[0] = Context->IntA0;
    StackFrame->Params[1] = Context->IntA1;
    StackFrame->Params[2] = Context->IntA2;
    StackFrame->Params[3] = Context->IntA3;

    return TRUE;
}


BOOL
WalkAlphaNext(
    HANDLE                            hProcess,
    LPSTACKFRAME64                    StackFrame,
    PALPHA_NT5_CONTEXT                Context,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64
    )
{
    DWORD              cb;
    ALPHA_NT5_CONTEXT  ContextSave;
    BOOL               rval = TRUE;
    ULONG64            Address;
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY  rf64;
    ULONG64            SystemRangeStart;
    DWORD              dw;
    ULONG64            qw;


    if (!WalkAlphaGetStackFrame( hProcess,
                        &StackFrame->AddrPC.Offset,
                        &StackFrame->AddrFrame.Offset,
                        Context,
                        ReadMemory,
                        GetModuleBase,
                        GetFunctionEntry,
                        &StackFrame->KdHelp,
                        Use64) ) {

        rval = FALSE;

        //
        // If the frame could not be unwound or is terminal, see if
        // there is a callback frame:
        //

        if (AppVersion.Revision >= 4 && CALLBACK_STACK(StackFrame)) {

            if (AppVersion.Revision >= 6) {
                SystemRangeStart = SYSTEM_RANGE_START(StackFrame);
            } else {
                //
                // This might not really work right with old debuggers, but it keeps
                // us from looking off the end of the structure anyway.
                //
                SystemRangeStart = 0x80000000;
            }

           if (CALLBACK_STACK(StackFrame) >= SystemRangeStart) {

                //
                // it is the pointer to the stack frame that we want,
                // or -1.

                Address = CALLBACK_STACK(StackFrame);

            } else {

                //
                // if it is a positive integer, it is the offset to
                // the address in the thread.
                // Look up the pointer:
                //

                if (Use64) {
                    rval = ReadMemory(hProcess,
                                      (CALLBACK_THREAD(StackFrame) +
                                                     CALLBACK_STACK(StackFrame)),
                                      &Address,
                                      sizeof(ULONG64),
                                      &cb);
                } else {
                    rval = ReadMemory(hProcess,
                                      (CALLBACK_THREAD(StackFrame) +
                                                     CALLBACK_STACK(StackFrame)),
                                      &dw,
                                      sizeof(DWORD),
                                      &cb);
                    Address = (ULONG64)(LONG64)(LONG)dw;
                }

                if (!rval || Address == 0) {
                    Address = (ULONG64)-1;
                    CALLBACK_STACK(StackFrame) = (DWORD)-1;
                }

            }

            if ( (Address == (ULONG64)-1) ||
                !(rf64 = LookupFunctionEntry(hProcess, CALLBACK_FUNC(StackFrame), ReadMemory, GetModuleBase, GetFunctionEntry, Use64 )) ) {

                rval = FALSE;

            } else {

                if (Use64) {
                    ReadMemory(hProcess,
                               (Address + CALLBACK_NEXT(StackFrame)),
                               &CALLBACK_STACK(StackFrame),
                               sizeof(ULONG64),
                               &cb);
                    StackFrame->AddrPC.Offset = ALPHA_RF_PROLOG_END_ADDRESS(rf64);
                } else {
                    ReadMemory(hProcess,
                               (Address + CALLBACK_NEXT(StackFrame)),
                               &dw,
                               sizeof(DWORD),
                               &cb);
                    CALLBACK_STACK(StackFrame) = dw;
                    StackFrame->AddrPC.Offset = ALPHA_RF_PROLOG_END_ADDRESS((PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)rf64);
                }

                StackFrame->AddrFrame.Offset = Address;
                Context->IntSp = Address;

                rval = TRUE;
            }
        }
    }

    //
    // get the return address
    //
    ContextSave = *Context;
    StackFrame->AddrReturn.Offset = StackFrame->AddrPC.Offset;

    if (!WalkAlphaGetStackFrame( hProcess,
                        &StackFrame->AddrReturn.Offset,
                        &qw,
                        &ContextSave,
                        ReadMemory,
                        GetModuleBase,
                        GetFunctionEntry,
                        &StackFrame->KdHelp,
                        Use64) ) {

        StackFrame->AddrReturn.Offset = 0;

    }

    //
    // get the arguments to the function
    //
    StackFrame->Params[0] = ContextSave.IntA0;
    StackFrame->Params[1] = ContextSave.IntA1;
    StackFrame->Params[2] = ContextSave.IntA2;
    StackFrame->Params[3] = ContextSave.IntA3;

    return rval;
}


PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
LookupFunctionEntry(
    HANDLE                            hProcess,
    ULONG64                           ControlPc,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64
    )
/*++

Routine Description:

    This function searches the currently active function tables (static and dynamic)
    for an entry that corresponds to the specified Address.

    This function mirrors RtlLookupFunctionEntry with changes required to make it work
    from a separate process and capable of searching 32- or 64-bit function tables

Arguments:

    hProcess - Proces handle

    ControlPc - Address of an instruction within the specified function.

    ReadMemory - Routine to read the target process memory

    Use64 - TRUE:  Target process is 64 bit
            FALSE: Target process is 32 bit

Return Value:

    If there is no entry in the function table for the specified ControlPc, then
    NULL is returned. Otherwise, the address of the primary function table
    entry that corresponds to the specified PC is returned.

    This function always returns a 64-bit function entry. 32-bit function entries
    are converted to the 64-bit equivalent.

--*/

{
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionEntry;

#if DBG
    if (DebugFunctionEntries) {
        dbPrint("\nLookupFunctionEntry(ControlPc=%.8I64x,Use64=%d)\n", ControlPc, Use64 );
    }
#endif

    // Look for a static or dynamic function entry

    FunctionEntry = LookupDirectFunctionEntry( hProcess, ControlPc, ReadMemory, GetModuleBase, GetFunctionEntry, Use64 );

    if (FunctionEntry != NULL) {

        //
        // The capability exists for more than one function entry
        // to map to the same function. This permits a function to
        // have discontiguous code segments described by separate
        // function table entries. If the ending prologue address
        // is not within the limits of the begining and ending
        // address of the function able entry, then the prologue
        // ending address is the address of the primary function
        // table entry that accurately describes the ending prologue
        // address.
        //

        if ((ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry) <  ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) ||
            (ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry) >= ALPHA_RF_END_ADDRESS(FunctionEntry))) {
#if DBG
            ShowRuntimeFunction(FunctionEntry, "LookupFunctionEntry: secondary entry");
#endif
            // Officially the PrologEndAddress field in secondary function entries
            // doesn't have the exception mode bits there have been some versions
            // of alpha tools that put them there. Strip them off to be safe.

            FunctionEntry = FetchFunctionEntry( hProcess, ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry), ReadMemory, Use64 );
            if (!FunctionEntry) {
                return NULL;
            }
            FunctionEntry = PromoteFunctionEntry( FunctionEntry );

        } else if (ALPHA_RF_IS_FIXED_RETURN(FunctionEntry)) {
            ULONG64 FixedReturn = FIXED_RETURN(FunctionEntry);

#if DBG
            ShowRuntimeFunction(FunctionEntry, "LookupFunctionEntry: fixed return entry");
#endif
            // Recursively call LookupFunctionEntry to ensure we get a primary function entry here.
            // Check for incorrectly formed function entry where the fixed return points to itself.

            if ((FixedReturn <  ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) ||
                (FixedReturn >= ALPHA_RF_END_ADDRESS(FunctionEntry))) {
                FunctionEntry = LookupFunctionEntry( hProcess, FIXED_RETURN(FunctionEntry), ReadMemory, GetModuleBase, GetFunctionEntry, Use64 );
            }
        }
#if DBG
        ShowRuntimeFunction(FunctionEntry, "LookupFunctionEntry: primary entry");
#endif
    }

#if DBG
    if (DebugFunctionEntries) {
        if (FUNCTION_ENTRY_ADDRESS(FunctionEntry)) {
            dbPrint("LookupFunctionEntry: returning FunctionEntry=%.8I64x %s\n",
                FUNCTION_ENTRY_ADDRESS(FunctionEntry),
                FUNCTION_ENTRY_DESCRIPTION(FunctionEntry) );
        } else {
            dbPrint("LookupFunctionEntry: returning FunctionEntry=%.8I64x %s\n",
                (ULONG64)(LONG64)(LONG_PTR)FunctionEntry,
                FUNCTION_ENTRY_DESCRIPTION(FunctionEntry) );
        }
    }
#endif
    return FunctionEntry;
}


PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
LookupDirectFunctionEntry (
    HANDLE                            hProcess,
    ULONG64                           ControlPc,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64
    )
/*++

Routine Description:

    This function searches the currently active function tables (static and dynamic)
    for an entry that corresponds to the specified PC value.

    This function mirrors RtlLookupDirectFunctionEntry with changes required to make it work
    from a separate process and capable of searching 32- or 64-bit function tables

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionEntry;
    BOOLEAN InImage;

    //
    // look for function entry in static function tables
    //

    FunctionEntry = LookupStaticFunctionEntry( hProcess, ControlPc, ReadMemory, GetModuleBase, GetFunctionEntry, Use64, &InImage );

    //
    // If not in static image range and no static function entry
    // found use FunctionEntryCallback routine (if present) for
    // dynamic function entry or some other source of pdata (e.g.
    // saved pdata information for ROM images)
    //

    if (FunctionEntry == NULL) {
        PPROCESS_ENTRY  ProcessEntry = FindProcessEntry( hProcess );
        if (ProcessEntry) {
            if (!InImage) {
                if (ProcessEntry->pFunctionEntryCallback32) {
                    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY FunctionEntry32;

                    FunctionEntry32 = ProcessEntry->pFunctionEntryCallback32(
                                            hProcess,
                                            (ULONG)ControlPc,
                                            (PVOID)ProcessEntry->FunctionEntryUserContext
                                            );
                    if (FunctionEntry32) {
                        LastDynamicFunctionEntry.Process = hProcess;
                        LastDynamicFunctionEntry.Address = 0;
#if DBG
                        LastDynamicFunctionEntry.Description = "from FunctionEntryCallback32";
#endif
                        ConvertAlphaRf32To64( FunctionEntry32, &LastDynamicFunctionEntry.Value );
                        FunctionEntry = &LastDynamicFunctionEntry.Value;
                    }

                } else if (ProcessEntry->pFunctionEntryCallback64) {
                    FunctionEntry = ProcessEntry->pFunctionEntryCallback64(
                                            hProcess,
                                            ControlPc,
                                            ProcessEntry->FunctionEntryUserContext
                                            );
                    if (FunctionEntry) {
                        LastDynamicFunctionEntry.Process = hProcess;
                        LastDynamicFunctionEntry.Address = 0;
#if DBG
                        LastDynamicFunctionEntry.Description = "from FunctionEntryCallback64";
#endif
                        LastDynamicFunctionEntry.Value = *FunctionEntry;
                        FunctionEntry = &LastDynamicFunctionEntry.Value;
                    }
                }

                if (FunctionEntry) {
#if DBG
                    if (DebugFunctionEntries) dbPrint("  LookupDirectFunctionEntry: got dynamic entry\n");
#endif
                } else if (GetFunctionEntry != NULL) {
                    // VC 6 didn't supply a GetModuleBase callback so this code is to make
                    // stack walking backward compatible.
                    //
                    // If we don't have a function by now, use the old-style function entry
                    // callback and let VC give it to us. Note that MSDN documentation indicates
                    // that this callback should return a 3-field IMAGE_FUNCTION_ENTRY structure,
                    // but VC 6 actually returns the 5-field IMAGE_RUNTIME_FUNCTION_ENTRY. Since
                    // the purpose of this hack is to make VC 6 work just go with the
                    // way VC 6 does it rather than what MSDN says.


                    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY UserFunctionEntry;
                    UserFunctionEntry = GetFunctionEntry( hProcess, ControlPc );

                    if (UserFunctionEntry) {
                        LastUserFunctionEntry.Process = hProcess;
                        LastUserFunctionEntry.Address = 0;
                        LastUserFunctionEntry.Value = *UserFunctionEntry;
                        FunctionEntry = &LastUserFunctionEntry.Value;
#if DBG
                        LastUserFunctionEntry.Description = "from LookupDirectFunctionEntry";
                        if (DebugFunctionEntries) dbPrint("  LookupDirectFunctionEntry: got user callback entry\n");
#endif
                    }
                }
            } else {
                // Nothing has turned up a function entry but we do have a module base address.
                // One possibility is that this is the kernel debugger and the pdata section is not
                // paged in. The last ditch attempt for a function entry will be an internal dbghelp
                // call to get the pdata entry from the debug info. This is not great because the data
                // in the debug section is incomplete and potentially out of date, but in most cases it
                // works and makes it possible to get user-mode stack traces in the kernel debugger.

                PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY UserFunctionEntry;
                UserFunctionEntry = GetAlphaFunctionEntryFromDebugInfo( ProcessEntry, ControlPc );

                if (UserFunctionEntry) {
                    LastUserFunctionEntry.Process = hProcess;
                    LastUserFunctionEntry.Address = 0;
                    LastUserFunctionEntry.Value = *UserFunctionEntry;
                    FunctionEntry = &LastUserFunctionEntry.Value;
#if DBG
                    LastUserFunctionEntry.Description = "from GetAlphaFunctionEntryFromDebugInfo";
                    if (DebugFunctionEntries) dbPrint("  LookupDirectFunctionEntry: got function entry from symbol file\n");
#endif
                }
            }
        }
    }

    return FunctionEntry;
}


PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
LookupStaticFunctionEntry(
    HANDLE                            hProcess,
    ULONG64                           ControlPc,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  GetFunctionEntry,
    BOOL                              Use64,
    PBOOLEAN                          InImage
    )

/*++

Routine Description:

    This function searches the currently active static function tables for an
    entry that corresponds to the specified PC value.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

    InImage - Address to recieve a flag indicating whether the ControlPc
        was in the range of a static function table

Return Value:

    If there is no entry in the static function tables for the specified PC,
    then NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{
    ULONG64 FunctionTable;
    ULONG64 NextFunctionTableEntry;
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionEntry;
    ULONG SizeOfExceptionTable;
    LONG High;
    ULONG64 ImageBase;
    LONG Low;
    LONG Middle;
    ULONG Index;

    // First check the most recent primary and secondary function entry cache

    if (LastPrimaryFunctionEntry.Process == hProcess &&
        ControlPc >= LastPrimaryFunctionEntry.Value.BeginAddress &&
        ControlPc <  ALPHA_RF_END_ADDRESS(&LastPrimaryFunctionEntry.Value)) {
        return &LastPrimaryFunctionEntry.Value;
    }

    if (LastSecondaryFunctionEntry.Process == hProcess &&
        ControlPc >= LastSecondaryFunctionEntry.Value.BeginAddress &&
        ControlPc <  ALPHA_RF_END_ADDRESS(&LastSecondaryFunctionEntry.Value)) {
        return &LastSecondaryFunctionEntry.Value;
    }

    // Next check the array of recently fetched function entries

    for (Index = 0; Index < LocalFunctionEntryCount; Index++) {
        if (LocalFunctionEntry[Index].Process == hProcess &&
            ControlPc >= LocalFunctionEntry[Index].Value.BeginAddress &&
            ControlPc <  ALPHA_RF_END_ADDRESS(&LocalFunctionEntry[Index].Value)) {

            return &LocalFunctionEntry[Index].Value;
        }
    }

    //
    // Search for the image that includes the specified PC value.
    //

    ImageBase = GetModuleBase( hProcess, ControlPc );

#if DBG
    if (DebugFunctionEntries) {
        dbPrint("  LookupStaticEntry(ControlPc=%.8I64x) ImageBase = %.8I64x\n",
                 ControlPc, ImageBase);
    }
#endif

    //
    // If an image is found that includes the specified PC, then locate the
    // function table for the image.
    //

    *InImage = (ImageBase != 0);
    FunctionEntry = NULL;
    if (ImageBase) {
        FunctionTable = FunctionTableBase( hProcess, ReadMemory, Use64, ImageBase, &SizeOfExceptionTable );

        //
        // If a function table is located, then search the function table
        // for a function table entry for the specified PC.
        //

        if (FunctionTable) {

            //
            // Initialize search indicies.
            //

            Low = 0;
            if (Use64) {
                High = (SizeOfExceptionTable / sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY)) - 1;
            } else {
                High = (SizeOfExceptionTable / sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)) - 1;
            }

            //
            // Perform binary search on the function table for a function table
            // entry that subsumes the specified PC.
            //

            while (High >= Low) {

                //
                // Compute next probe index and test entry. If the specified PC
                // is greater than of equal to the beginning address and less
                // than the ending address of the function table entry, then
                // return the address of the function table entry. Otherwise,
                // continue the search.
                //

                Middle = (Low + High) >> 1;

                if (Use64) {
                    NextFunctionTableEntry = FunctionTable + Middle*sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY);
                } else {
                    NextFunctionTableEntry = FunctionTable + Middle*sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY);
                }

                //
                // Fetch the function entry and bail if there is an error reading it
                //
                FunctionEntry = FetchFunctionEntry( hProcess, NextFunctionTableEntry, ReadMemory, Use64 );
                if (!FunctionEntry) {
                    return NULL;
                }
                if (ControlPc < ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) {
                    High = Middle - 1;

                } else if (ControlPc >= ALPHA_RF_END_ADDRESS(FunctionEntry)) {
                    Low = Middle + 1;

                } else {
                    return PromoteFunctionEntry( FunctionEntry );
                }
            } // while (High >= Low)
        } // FunctionTable != NULL
    } // ImageBase != NULL
    return NULL;
}

VOID
GetUnwindFunctionEntry(
    HANDLE                                hProcess,
    ULONG64                               ControlPc,
    PREAD_PROCESS_MEMORY_ROUTINE64        ReadMemory,
    PGET_MODULE_BASE_ROUTINE64            GetModuleBase,
    PFUNCTION_TABLE_ACCESS_ROUTINE64      GetFunctionEntry,
    BOOL                                  Use64,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY UnwindFunctionEntry,
    PULONG                                StackAdjust,
    PULONG64                              FixedReturn
    )
/*++

Routine Description:

    This function returns a function entry (RUNTIME_FUNCTION) suitable
    for unwinding from ControlPc. It encapsulates the handling of primary
    and secondary function entries so that this processing is not duplicated
    in VirtualUnwind and other similar functions.

Arguments:

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    UnwindFunctionEntry - Supplies the address of a function table entry which
        will be setup with appropriate fields for unwinding from ControlPc

Return Value:

    None.

--*/

{
    ULONG EntryType = 0;
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY SecondaryFunctionEntry = NULL;
    ULONG64 AlternateProlog;

    *FixedReturn = 0;
    *StackAdjust = 0;

#if DBG
    if (ControlPc & 0x3) {
        dbPrint("GetUnwindFunctionEntry: invalid PC for unwinding (low bits set): %16.8I64x.\n", ControlPc);
    }
#endif

    // FunctionEntry should never be null, but if it is create one that
    // looks like a leaf entry for ControlPc

    if (FunctionEntry == NULL) {
#if DBG
        dbPrint("\nGetUnwindFunctionEntry: Null function table entry for unwinding.\n");
#endif
        UnwindFunctionEntry->BeginAddress     = ControlPc;
        UnwindFunctionEntry->EndAddress       = ControlPc+4;
        UnwindFunctionEntry->ExceptionHandler = 0;
        UnwindFunctionEntry->HandlerData      = 0;
        UnwindFunctionEntry->PrologEndAddress = ControlPc;
        return;
    }

    // Save the most recent primary function entry

    LastPrimaryFunctionEntry = *(PLOCAL_FUNCTION_ENTRY)FunctionEntry;

    // Reference the copy of the function entry passed in so FetchFunctionEntry's
    // buffered function entries don't overwrite the primary

    FunctionEntry = &LastPrimaryFunctionEntry.Value;

    //
    // Because of the secondary-to-primary function entry indirection applied by
    // LookupFunctionEntry() ControlPc may not be within the range described
    // by the supplied function entry. Call LookupDirectFunctionEntry()
    // to recover the actual (secondary) function entry.  If we don't get a
    // valid associated function entry then process the unwind with the one
    // supplied, trusting that the caller has supplied the given entry intentionally.
    //
    // A secondary function entry is a RUNTIME_FUNCTION entry where
    // PrologEndAddress is not in the range of BeginAddress to EndAddress.
    // There are three types of secondary function entries. They are
    // distinquished by the Entry Type field (2 bits):
    //
    // ALPHA_RF_NOT_CONTIGUOUS - discontiguous code
    // ALPHA_RF_ALT_ENT_PROLOG - alternate entry point prologue
    // ALPHA_RF_NULL_CONTEXT   - null-context code
    //

    if ((ControlPc <  ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) ||
        (ControlPc >= ALPHA_RF_END_ADDRESS(FunctionEntry))) {

        // ControlPC is not in the range of the supplied function entry.
        // Get the actual function entry which is expected to be the
        // associated secondary function entry or a fixed return primary function.
#if DBG
        if (DebugFunctionEntries) {
            dbPrint("\nGetUnwindFunctionEntry:LookupDirectFunctionEntry(ControlPc=%.8I64x,Use64=%d)\n", ControlPc, Use64 );
        }
#endif
        SecondaryFunctionEntry = LookupDirectFunctionEntry( hProcess, ControlPc, ReadMemory, GetModuleBase, GetFunctionEntry, Use64 );

        if (SecondaryFunctionEntry) {

#if DBG
            ShowRuntimeFunction(SecondaryFunctionEntry, "GetUnwindFunctionEntry: LookupDirectFunctionEntry");
#endif

            // If this is a null-context tail region then unwind with a null-context-like descriptor

            if ((ControlPc >= ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry)-(ALPHA_RF_NULL_CONTEXT_COUNT(SecondaryFunctionEntry)*4)) &&
                (ControlPc <  ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry))) {

                // Use the secondary function entry with PrologEndAddress = BeginAddress.
                // This ensures that the prologue is not reverse executed.

                UnwindFunctionEntry->BeginAddress     = ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
                UnwindFunctionEntry->EndAddress       = ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry);
                UnwindFunctionEntry->ExceptionHandler = 0;
                UnwindFunctionEntry->HandlerData      = 0;
                UnwindFunctionEntry->PrologEndAddress = ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
                return;
            }

            if ((SecondaryFunctionEntry->PrologEndAddress < ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry)) ||
                (SecondaryFunctionEntry->PrologEndAddress > ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry))) {

                // Got a secondary function entry as expected. But if indirection doesn't point
                // to FunctionEntry then ignore it and use the caller supplied FunctionEntry.

                if (ALPHA_RF_PROLOG_END_ADDRESS(SecondaryFunctionEntry) != FUNCTION_ENTRY_ADDRESS(FunctionEntry)) {
    #if DBG
                    ShowRuntimeFunction(SecondaryFunctionEntry,
                                        "GetUnwindFunctionEntry: unexpected secondary function entry from LookupDirectFunctionEntry");
    #endif
                    SecondaryFunctionEntry = NULL;
                }
            } else if (ALPHA_RF_IS_FIXED_RETURN(SecondaryFunctionEntry)) {
                // Got a fixed return entry. Switch to using the fixed return entry as the primary.

                    FunctionEntry = SecondaryFunctionEntry;
                    SecondaryFunctionEntry = NULL;

            } else {

                // Got a primary function entry. Ignore it and use caller supplied FunctionEntry.
    #if DBG
                ShowRuntimeFunction(SecondaryFunctionEntry,
                                    "GetUnwindFunctionEntry: unexpected primary function entry from LookupDirectFunctionEntry");
    #endif
                SecondaryFunctionEntry = NULL;
            }
#if DBG
        } else {
            ShowRuntimeFunction(SecondaryFunctionEntry, "GetUnwindFunctionEntry: LookupDirectFunctionEntry returned NULL");
#endif
        }
    } else {

        // ControlPC is in the range of the supplied function entry.

        // If this is a null-context tail region then unwind with a null-context-like descriptor

        if ((ControlPc >= ALPHA_RF_END_ADDRESS(FunctionEntry)-(ALPHA_RF_NULL_CONTEXT_COUNT(FunctionEntry)*4)) &&
            (ControlPc <  ALPHA_RF_END_ADDRESS(FunctionEntry))) {

            // Use the secondary function entry with PrologEndAddress = BeginAddress.
            // This ensures that the prologue is not reverse executed.

            UnwindFunctionEntry->BeginAddress     = ALPHA_RF_BEGIN_ADDRESS(FunctionEntry);
            UnwindFunctionEntry->EndAddress       = ALPHA_RF_END_ADDRESS(FunctionEntry);
            UnwindFunctionEntry->ExceptionHandler = 0;
            UnwindFunctionEntry->HandlerData      = 0;
            UnwindFunctionEntry->PrologEndAddress = ALPHA_RF_BEGIN_ADDRESS(FunctionEntry);
            return;
        }

        // Check if it is a secondary function entry. This shouldn't happen because
        // LookupFunctionEntry is always supposed to return a primary function entry.
        // But if we get passed a secondary, then switch to it's primary. However note
        // that we've gone through this pass

        if ((FunctionEntry->PrologEndAddress < ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) ||
            (FunctionEntry->PrologEndAddress > ALPHA_RF_END_ADDRESS(FunctionEntry))) {

            SecondaryFunctionEntry = FunctionEntry;
            FunctionEntry = FetchFunctionEntry( hProcess, ALPHA_RF_PROLOG_END_ADDRESS(SecondaryFunctionEntry), ReadMemory, Use64 );
#if DBG
            ShowRuntimeFunction(SecondaryFunctionEntry, "GetUnwindFunctionEntry: received secondary function entry");
#endif
        }
    }

    // FunctionEntry is now the primary function entry and if SecondaryFunctionEntry is
    // not NULL then it is the secondary function entry that contains the ControlPC. Setup a
    // copy of the FunctionEntry suitable for unwinding. By default use the supplied FunctionEntry.

    if (SecondaryFunctionEntry) {

        // Save the most secondary function entry

        LastSecondaryFunctionEntry = *(PLOCAL_FUNCTION_ENTRY)SecondaryFunctionEntry;

        // Extract the secondary function entry type.

        EntryType = ALPHA_RF_ENTRY_TYPE(SecondaryFunctionEntry);

        if (EntryType == ALPHA_RF_NOT_CONTIGUOUS) {
            // The exception happened in the body of the procedure but in a non-contiguous
            // section of code. Regardless of what entry point was used, it is normally valid
            // to unwind using the primary entry point prologue. The only exception is when an
            // alternate prologue is specified However, there may be an
            // alternate prologue end addresss specified in which case unwind using this
            // block as though it were the primary.

            AlternateProlog = ALT_PROLOG(SecondaryFunctionEntry);

            if ((AlternateProlog >= ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry)) &&
                (AlternateProlog <  ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry))) {

                // If the control PC is in the alternate prologue, use the secondary.
                // The control Pc is not in procedure context.

                if ((ControlPc >= ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry)) &&
                    (ControlPc <  ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry))) {

                    UnwindFunctionEntry->BeginAddress     = ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
                    UnwindFunctionEntry->EndAddress       = ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry);
                    UnwindFunctionEntry->ExceptionHandler = 0;
                    UnwindFunctionEntry->HandlerData      = 0;
                    UnwindFunctionEntry->PrologEndAddress = AlternateProlog;
                    return;
                }
            }

            // Fall out of the if statement to pick up the primary function entry below.
            // This code is in-procedure-context and subject to the primary's prologue
            // and exception handlers.

        } else if (EntryType == ALPHA_RF_ALT_ENT_PROLOG) {
            // Exception occured in an alternate entry point prologue.
            // Use the secondary function entry with a fixed-up PrologEndAddress.

            UnwindFunctionEntry->BeginAddress     = ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
            UnwindFunctionEntry->EndAddress       = ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry);
            UnwindFunctionEntry->ExceptionHandler = 0;
            UnwindFunctionEntry->HandlerData      = 0;
            UnwindFunctionEntry->PrologEndAddress = ALPHA_RF_END_ADDRESS(UnwindFunctionEntry);

            // Check for an alternate prologue.

            AlternateProlog = ALT_PROLOG(SecondaryFunctionEntry);
            if (AlternateProlog >= UnwindFunctionEntry->BeginAddress &&
                AlternateProlog <  UnwindFunctionEntry->EndAddress ) {
                // The prologue is only part of the procedure
                UnwindFunctionEntry->PrologEndAddress = AlternateProlog;
            }

            return;

        } else if (EntryType == ALPHA_RF_NULL_CONTEXT) {

            // Exception occured in null-context code associated with a primary function.
            // Use the secondary function entry with a PrologEndAddress = BeginAddress.
            // There is no prologue for null-context code.

            *StackAdjust = ALPHA_RF_STACK_ADJUST(SecondaryFunctionEntry);
            UnwindFunctionEntry->BeginAddress     = ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
            UnwindFunctionEntry->EndAddress       = ALPHA_RF_END_ADDRESS(SecondaryFunctionEntry);
            UnwindFunctionEntry->ExceptionHandler = 0;
            UnwindFunctionEntry->HandlerData      = 0;
            UnwindFunctionEntry->PrologEndAddress = ALPHA_RF_BEGIN_ADDRESS(SecondaryFunctionEntry);
            return;
        }
    }

    // FunctionEntry is only null if there was an error fetching it from a passed in
    // secondary function entry.

    if (FunctionEntry == NULL) {
#if DBG
        dbPrint("\nGetUnwindFunctionEntry: Error in FetchFunctionEntry.\n");
#endif
        UnwindFunctionEntry->BeginAddress     = ControlPc;
        UnwindFunctionEntry->EndAddress       = ControlPc+4;
        UnwindFunctionEntry->ExceptionHandler = 0;
        UnwindFunctionEntry->HandlerData      = 0;
        UnwindFunctionEntry->PrologEndAddress = ControlPc;
        return;
    }

#if DBG
    if (ALPHA_RF_BEGIN_ADDRESS(FunctionEntry) >= ALPHA_RF_END_ADDRESS(FunctionEntry)) {
        ShowRuntimeFunction(FunctionEntry, "GetUnwindFunctionEntry: Warning - BeginAddress < EndAddress.");
    } else if (FunctionEntry->PrologEndAddress < ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) {
        ShowRuntimeFunction(FunctionEntry, "GetUnwindFunctionEntry: Warning - PrologEndAddress < BeginAddress.");
    } else if (FunctionEntry->PrologEndAddress > ALPHA_RF_END_ADDRESS(FunctionEntry)) {
        ShowRuntimeFunction(FunctionEntry, "GetUnwindFunctionEntry: Warning - PrologEndAddress > EndAddress.");
    }
#endif

    // Use the primary function entry

    *UnwindFunctionEntry = *FunctionEntry;
    UnwindFunctionEntry->EndAddress = ALPHA_RF_END_ADDRESS(UnwindFunctionEntry);  // Remove null-context count

    // If the primary has a fixed return address, pull that out now.

    if (ALPHA_RF_IS_FIXED_RETURN(FunctionEntry)) {
        *FixedReturn = FIXED_RETURN(FunctionEntry);
        UnwindFunctionEntry->ExceptionHandler = 0;
        UnwindFunctionEntry->HandlerData      = 0;
    }
}

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
FetchFunctionEntry (
    HANDLE                            hProcess,
    ULONG64                           Address,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    BOOL                              Use64
    )
/*++

Routine Description:

    Read a function entry from the target process.

--*/
{
    PLOCAL_FUNCTION_ENTRY CachedFunctionEntry;
    IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY FunctionEntry32;
    ULONG Index;
    DWORD cb;
    BOOL bReadMemory;

    // First check the most recent primary and secondary function entry cache

    if (LastPrimaryFunctionEntry.Process == hProcess &&
        LastPrimaryFunctionEntry.Address == Address ) {
        return &LastPrimaryFunctionEntry.Value;
    }

    if (LastSecondaryFunctionEntry.Process == hProcess &&
        LastSecondaryFunctionEntry.Address == Address ) {
        return &LastSecondaryFunctionEntry.Value;
    }

    // Next check the array of recently fetched function entries

    for (Index = 0; Index < LocalFunctionEntryCount; Index++) {
        if (LocalFunctionEntry[Index].Process == hProcess &&
            LocalFunctionEntry[Index].Address == Address ) {

            return &LocalFunctionEntry[Index].Value;
        }
    }

    // If not in the cache, replace the the entry that LocalFunctionEntryNext
    // points to. LocalFunctionEntryNext cycles through the last part of the
    // table and function entries we want to keep are promoted to the first
    // part of the table so they don't get overwritten by new ones being read
    // as part of the binary search through function entry tables.

    if (LocalFunctionEntryCount < LFE_MAX) {
        LocalFunctionEntryCount++;
        LocalFunctionEntryNext = LocalFunctionEntryCount;
    } else {
        LocalFunctionEntryNext++;
        if (LocalFunctionEntryNext >= LFE_MAX) {
            LocalFunctionEntryNext = LFE_NEW;
        }
    }

    CachedFunctionEntry = &LocalFunctionEntry[LocalFunctionEntryNext-1];

    if (Use64) {
        bReadMemory = ReadMemory(hProcess, Address, &CachedFunctionEntry->Value, sizeof(IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY), &cb);
    } else {
        if (bReadMemory = ReadMemory(hProcess, Address, &FunctionEntry32, sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY), &cb)) {
            ConvertAlphaRf32To64( &FunctionEntry32, &CachedFunctionEntry->Value );
        }
    }

    // If successful, update cache entry and return its pointer
    // if failure, invalidate cache and return null
    if (bReadMemory) {
        CachedFunctionEntry->Address = Address;
        CachedFunctionEntry->Process = hProcess;
#if DBG
        CachedFunctionEntry->Description = "from target process";
#endif
        return &CachedFunctionEntry->Value;
    } else {
        CachedFunctionEntry->Address = 0;
        CachedFunctionEntry->Process = NULL;
#if DBG
        dbPrint("Can't read function entry from target process at: %I64x\n", Address);
#endif
        return NULL;
    }
}

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
PromoteFunctionEntry (
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY Address
    )
{
    ULONG Index;
    ULONG Move;
    PLOCAL_FUNCTION_ENTRY Entry = (PLOCAL_FUNCTION_ENTRY)Address;

    if (Entry >= &LocalFunctionEntry[1] &&
        Entry <  &LocalFunctionEntry[LocalFunctionEntryCount]) {

        Index = Entry - LocalFunctionEntry;

        // make sure it's promoted out of the new entry area
        if (Index >= LFE_NEW) {
            Move = Index - (LFE_NEW - 3);
        } else {
            Move = Index >= 3? 3 : 1;
        }

        if (Index > Move) {
            LOCAL_FUNCTION_ENTRY Temp = LocalFunctionEntry[Index];
            LocalFunctionEntry[Index] = LocalFunctionEntry[Index-Move];
            LocalFunctionEntry[Index-Move] = Temp;
            Index -= Move;
        }
        return &LocalFunctionEntry[Index].Value;
    }
    return Address;
}

ULONG64
FunctionTableBase(
    HANDLE                            hProcess,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    BOOL                              Use64,
    ULONG64                           Base,
    PULONG                            Size
    )
/*++

Routine Description:

    Return the address of an image's function entry table given the base
    address of the module.

--*/
{
    ULONG64 NtHeaders;
    ULONG64 ExceptionDirectoryEntryAddress;
    IMAGE_DATA_DIRECTORY ExceptionData;
    IMAGE_DOS_HEADER DosHeaderData;
    DWORD cb;

    // Read DOS header to calculate the address of the NT header.

    if (!ReadMemory( hProcess, Base, &DosHeaderData, sizeof(DosHeaderData), &cb ))
        return 0;
    if (DosHeaderData.e_magic != IMAGE_DOS_SIGNATURE)
        return 0;

    NtHeaders = Base + DosHeaderData.e_lfanew;

    if (Use64) {
        ExceptionDirectoryEntryAddress = NtHeaders +
               offsetof(IMAGE_NT_HEADERS64,OptionalHeader) +
               offsetof(IMAGE_OPTIONAL_HEADER64,DataDirectory) +
               IMAGE_DIRECTORY_ENTRY_EXCEPTION * sizeof(IMAGE_DATA_DIRECTORY);
    } else {
        ExceptionDirectoryEntryAddress = NtHeaders +
               offsetof(IMAGE_NT_HEADERS32,OptionalHeader) +
               offsetof(IMAGE_OPTIONAL_HEADER32,DataDirectory) +
               IMAGE_DIRECTORY_ENTRY_EXCEPTION * sizeof(IMAGE_DATA_DIRECTORY);
    }

    // Read NT header to get the image data directory.

    if (!ReadMemory( hProcess, ExceptionDirectoryEntryAddress, &ExceptionData, sizeof(IMAGE_DATA_DIRECTORY), &cb ))
        return 0;

    *Size = ExceptionData.Size;
    return Base + ExceptionData.VirtualAddress;
}

#if DBG
void
ShowRuntimeFunction(
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionEntry,
    PSTR Label
    )
{
    BOOL Secondary = FALSE;
    BOOL FixedReturn = FALSE;
    ULONG EntryType = 0;
    ULONG NullCount = 0;

    if (DebugFunctionEntries) {
        if (FUNCTION_ENTRY_ADDRESS(FunctionEntry))
            dbPrint("    %.8I64x: ", FUNCTION_ENTRY_ADDRESS(FunctionEntry) );
        else
            dbPrint("    ");
        if (Label) dbPrint(Label);
        dbPrint("\n", FunctionEntry );
        if (FunctionEntry) {
            if ((ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry) < ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) ||
                (ALPHA_RF_PROLOG_END_ADDRESS(FunctionEntry) > ALPHA_RF_END_ADDRESS(FunctionEntry))) {
                Secondary = TRUE;
                EntryType = ALPHA_RF_ENTRY_TYPE(FunctionEntry);
                if (EntryType > MAXENTRYTYPE) EntryType = MAXENTRYTYPE;
            } else if (ALPHA_RF_IS_FIXED_RETURN(FunctionEntry)) {
                FixedReturn = TRUE;
            }
            NullCount = ALPHA_RF_NULL_CONTEXT_COUNT(FunctionEntry);

            dbPrint("    BeginAddress     = %16.8I64x\n", FunctionEntry->BeginAddress);
            dbPrint("    EndAddress       = %16.8I64x", FunctionEntry->EndAddress);
            if (NullCount) {
                dbPrint(" %d null-context instructions", NullCount);
            }
            dbPrint("\n");
            dbPrint("    ExceptionHandler = %16.8I64x", FunctionEntry->ExceptionHandler);
            if (FunctionEntry->ExceptionHandler != 0) {
                if (Secondary) {
                    ULONG64 AlternateProlog = ALT_PROLOG(FunctionEntry);

                    switch( EntryType ) {
                    case ALPHA_RF_NOT_CONTIGUOUS:
                    case ALPHA_RF_ALT_ENT_PROLOG:

                        if ((AlternateProlog >= ALPHA_RF_BEGIN_ADDRESS(FunctionEntry)) &&
                            (AlternateProlog <= FunctionEntry->EndAddress)) {
                                dbPrint(" alternate PrologEndAddress");
                        }
                        break;
                    case ALPHA_RF_NULL_CONTEXT:
                        dbPrint(" stack adjustment");
                    }
                } else if (FixedReturn) {
                    dbPrint(" fixed return address");
                }
            }
            dbPrint("\n");
            dbPrint("    HandlerData      = %16.8I64x", FunctionEntry->HandlerData);
            if (Secondary) {
                dbPrint(" type %d: %s", EntryType, EntryTypeName[EntryType] );
            }
            dbPrint("\n");
            dbPrint("    PrologEndAddress = %16.8I64x\n",   FunctionEntry->PrologEndAddress );
        }
    }
}
#endif
