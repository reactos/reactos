/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    exdsptch.c

Abstract:

    This module implements the dispatching of exceptions and the unwinding of
    procedure call frames.

Author:

    William K. Cheung (wcheung) 23-Dec-1995

    based on the version by David N. Cutler (davec) 11-Sep-1990

Environment:

    Any mode.

Revision History:

    ATM Shafiqul Khalid [askhalid] 8-23-99
    Added RtlAddFunctionTable and RtlDeleteFunctionTable

--*/


#include "ntrtlp.h"

//  the following section should move to ntia64.h under sdk\inc



PRUNTIME_FUNCTION
RtlLookupStaticFunctionEntry(
    IN ULONG_PTR ControlPc,
    OUT PBOOLEAN InImage
    );

PRUNTIME_FUNCTION
RtlLookupDynamicFunctionEntry(
    IN ULONG_PTR ControlPc,
    OUT PULONGLONG ImageBase,
    OUT PULONGLONG TargetGp
    );


LIST_ENTRY DynamicFunctionTable;

VOID
RtlRestoreContext (
    IN PCONTEXT ContextRecord,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL
    );


//
// Define local macros.
//
// Raise noncontinuable exception with associated exception record.
//

#define IS_HANDLER_DEFINED(f, base)                                \
    (f->UnwindInfoAddress &&                                       \
        (((PUNWIND_INFO)(base+f->UnwindInfoAddress))->Flags & 0x3))

#define HANDLER(f, base, target)                                                \
    (PEXCEPTION_ROUTINE)                                                        \
        (*(PULONGLONG) ((LONGLONG)target +                                      \
        (*(PULONGLONG) (base + f->UnwindInfoAddress + sizeof(UNWIND_INFO)  +    \
        (((PUNWIND_INFO) (base + f->UnwindInfoAddress))->DataLength * sizeof(ULONGLONG))))))

#define RAISE_EXCEPTION(Status, ExceptionRecordt) {                \
    EXCEPTION_RECORD ExceptionRecordn;                             \
                                                                   \
    ExceptionRecordn.ExceptionCode = Status;                       \
    ExceptionRecordn.ExceptionFlags = EXCEPTION_NONCONTINUABLE;    \
    ExceptionRecordn.ExceptionRecord = ExceptionRecordt;           \
    ExceptionRecordn.NumberParameters = 0;                         \
    RtlRaiseException(&ExceptionRecordn);                          \
    }

#define IS_SAME_FRAME(Frame1, Frame2)                              \
    ( (Frame1.MemoryStackFp == Frame2.MemoryStackFp) &&            \
      (Frame1.BackingStoreFp == Frame2.BackingStoreFp) )

#define INITIALIZE_FRAME(Frame)                                    \
    Frame.MemoryStackFp = Frame.BackingStoreFp = 0

#define CHECK_MSTACK_FRAME(Establisher, Target)                            \
    ((Establisher.MemoryStackFp < LowStackLimit) ||                        \
     (Establisher.MemoryStackFp > HighStackLimit) ||                       \
     ((Target.MemoryStackFp != 0) &&                                       \
      ((ULONG)Target.MemoryStackFp < (ULONG)Establisher.MemoryStackFp)) || \
     ((Establisher.MemoryStackFp & 0x3) != 0))

#define CHECK_BSTORE_FRAME(Establisher, Target)                               \
    ((Establisher.BackingStoreFp < LowBStoreLimit) ||                         \
     (Establisher.BackingStoreFp > HighBStoreLimit) ||                        \
     ((Target.BackingStoreFp != 0) &&                                         \
      ((ULONG)Target.BackingStoreFp > (ULONG)Establisher.BackingStoreFp)) ||  \
     ((Establisher.BackingStoreFp & 0x7) != 0))

#define IS_EM_SETJMP_REGISTRATION(ExRegistration)                                               \
    (   (ExRegistration != NULL)                                                            &&  \
        (ExRegistration != EXCEPTION_CHAIN_END)                                             &&  \
        (ExRegistration->Next == (struct _EXCEPTION_REGISTRATION_RECORD *)(LONG_PTR) 1)          \
    )


ULONGLONG
RtlpVirtualUnwind (
    IN ULONGLONG ImageBase,
    IN ULONGLONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PFRAME_POINTERS EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    );


PEXCEPTION_REGISTRATION_RECORD
RtlpGetRegistrationHead (
    IN VOID
    )

/*++

Routine Description:

    This function returns the address of the first exception registration
    record for the current context.

Arguments:

    None.

Return Value:

    The address of the first registration record.

--*/

{
    PTEB CurrentTeb = NtCurrentTeb();

    return (CurrentTeb ? CurrentTeb->NtTib.ExceptionList : EXCEPTION_CHAIN_END);
}


VOID
RtlpUnlinkHandler (
    PEXCEPTION_REGISTRATION_RECORD UnlinkPointer
    )

/*++

Routine Description:

    This function removes the specified exception registration record
    (and thus the relevant handler) from the exception traversal chain.

Arguments:

    UnlinkPointer - Address of registration record to unlink.

Return Value:

    None.

--*/

{
    NtCurrentTeb()->NtTib.ExceptionList = UnlinkPointer->Next;
}


PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONGLONG ControlPc,
    OUT PULONGLONG ImageBase,
    OUT PULONGLONG TargetGp
    )

/*++

Routine Description:

    This function searches the currently active function tables for an
    entry that corresponds to the specified PC value.

Arguments:

    ControlPc - Supplies the virtual address of an instruction bundle
        within the specified function.

    ImageBase - Returns the base address of the module to which the
                function belongs.

    TargetGp - Returns the global pointer value of the module.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned.  Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{
    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    ULONG SizeOfExceptionTable;
    ULONG Size;
    LONG High;
    LONG Middle;
    LONG Low;
    USHORT i;

    //
    // Search for the image that includes the specified swizzled PC value.
    //

    *ImageBase = (ULONG_PTR)RtlPcToFileHeader((PVOID)ControlPc, 
                                              (PVOID *)ImageBase);
    

    //
    // If an image is found that includes the specified PC, then locate the
    // function table for the image.
    //

    if ((PVOID)*ImageBase != NULL) {

        *TargetGp = (ULONG_PTR)(RtlImageDirectoryEntryToData(
                               (PVOID)*ImageBase,
                               TRUE,
                               IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                               &Size
                               ));

        FunctionTable = (PRUNTIME_FUNCTION)RtlImageDirectoryEntryToData(
                         (PVOID)*ImageBase, 
                         TRUE, 
                         IMAGE_DIRECTORY_ENTRY_EXCEPTION,
                         &SizeOfExceptionTable);

        //
        // If a function table is located, then search the table for a
        // function table entry for the specified PC.
        //

        if (FunctionTable != NULL) {

            //
            // Initialize search indices.
            //

            Low = 0;
            High = (SizeOfExceptionTable / sizeof(RUNTIME_FUNCTION)) - 1;
            ControlPc = ControlPc - *ImageBase;

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
                FunctionEntry = &FunctionTable[Middle];

                if (ControlPc < FunctionEntry->BeginAddress) {
                    High = Middle - 1;

                } else if (ControlPc >= FunctionEntry->EndAddress) {
                    Low = Middle + 1;

                } else {
                    return FunctionEntry;

                }
            }
        }
    } 
#if !defined(NTOS_KERNEL_RUNTIME)
    
    else     // ImageBase == NULL
        return   RtlLookupDynamicFunctionEntry ( ControlPc, ImageBase, TargetGp );

#endif  // NTOS_KERNEL_RUNTIME

    return NULL;
}


VOID
RtlpRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    )

/*++

Routine Description:

    This function raises a software exception by building a context record
    and calling the raise exception system service.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

Return Value:

    None.

--*/

{
    ULONGLONG ImageBase;
    ULONGLONG TargetGp;
    ULONGLONG ControlPc;
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;
    ULONGLONG NextPc;
    NTSTATUS Status;

    //
    // Capture the current context, virtually unwind to the caller of this
    // routine, set the fault instruction address to that of the caller, and
    // call the raise exception system service.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = RtlIa64InsertIPSlotNumber((ContextRecord.BrRp-16), 2);
    FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, &TargetGp);
    NextPc = RtlVirtualUnwind(ImageBase,
                              ControlPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ContextRecord.StIIP = NextPc + 8;
    ContextRecord.StIPSR &= ~((ULONGLONG) 3 << PSR_RI);
    ExceptionRecord->ExceptionAddress = (PVOID)ContextRecord.StIIP;
    Status = ZwRaiseException(ExceptionRecord, &ContextRecord, TRUE);

    //
    // There should never be a return from this system service unless
    // there is a problem with the argument list itself. Raise another
    // exception specifying the status value returned.
    //

    RtlRaiseStatus(Status);
    return;
}


VOID
RtlRaiseException (
    IN PEXCEPTION_RECORD ExceptionRecord
    )

/*++

Routine Description:

    This function raises a software exception by building a context record
    and calling the raise exception system service.

    N.B. This routine is a shell routine that simply calls another routine
         to do the real work. The reason this is done is to avoid a problem
         in try/finally scopes where the last statement in the scope is a
         call to raise an exception.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

Return Value:

    None.

--*/

{
    RtlpRaiseException(ExceptionRecord);
    return;
}

VOID
RtlpRaiseStatus (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This function raises an exception with the specified status value. The
    exception is marked as noncontinuable with no parameters.

Arguments:

    Status - Supplies the status value to be used as the exception code
        for the exception that is to be raised.

Return Value:

    None.

--*/

{
    ULONGLONG ImageBase;
    ULONGLONG TargetGp;
    ULONGLONG ControlPc;
    ULONGLONG NextPc;
    CONTEXT ContextRecord;
    FRAME_POINTERS EstablisherFrame;
    EXCEPTION_RECORD ExceptionRecord;
    PRUNTIME_FUNCTION FunctionEntry;
    BOOLEAN InFunction;

    //
    // Construct an exception record.
    //

    ExceptionRecord.ExceptionCode = Status;
    ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
    ExceptionRecord.NumberParameters = 0;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

    //
    // Capture the current context, virtually unwind to the caller of this
    // routine, set the fault instruction address to that of the caller, and
    // call the raise exception system service.
    //

    RtlCaptureContext(&ContextRecord);
    ControlPc = RtlIa64InsertIPSlotNumber((ContextRecord.BrRp-16), 2);
    FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, &TargetGp);
    NextPc = RtlVirtualUnwind(ImageBase,
                              ControlPc,
                              FunctionEntry,
                              &ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);
    ContextRecord.StIIP = NextPc + 8;
    ContextRecord.StIPSR &= ~((ULONGLONG) 3 << PSR_RI);
    ExceptionRecord.ExceptionAddress = (PVOID)ContextRecord.StIIP;
    Status = ZwRaiseException(&ExceptionRecord, &ContextRecord, TRUE);

    //
    // There should never be a return from this system service unless
    // there is a problem with the argument list itself. Raise another
    // exception specifying the status value returned.
    //

    RtlRaiseStatus(Status);
    return;
}


VOID
RtlRaiseStatus (
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This function raises an exception with the specified status value. The
    exception is marked as noncontinuable with no parameters.

    N.B. This routine is a shell routine that simply calls another routine
         to do the real work. The reason this is done is to avoid a problem
         in try/finally scopes where the last statement in the scope is a
         call to raise an exception.

Arguments:

    Status - Supplies the status value to be used as the exception code
        for the exception that is to be raised.

Return Value:

    None.

--*/

{
    RtlpRaiseStatus(Status);
    return;
}


VOID
RtlUnwind (
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    )

/*++

Routine Description:

    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetFrame parameter is not specified, then the exit unwind
    flag is also set in the exception flags of the exception record. A backward
    scan through the procedure call frames is then performed to find the target
    of the unwind operation.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

    N.B. This routine is provided for backward compatibility with release 1.

Arguments:

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind. If this parameter is not specified, then an exit
        unwind is performed.

    TargetIp - Supplies an optional instruction address that specifies the
        continuation address of the unwind. This address is ignored if the
        target frame parameter is not specified.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

Return Value:

    None.

--*/

{
    CONTEXT ContextRecord;
    FRAME_POINTERS Frame;
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;

    Frame.MemoryStackFp = (ULONG_PTR)TargetFrame;
    Frame.BackingStoreFp = 0;
    ContextRecord.StIPSR = 0;

    RegistrationPointer = (PEXCEPTION_REGISTRATION_RECORD)TargetFrame;
    if ((RegistrationPointer != NULL) &&
        IS_EM_SETJMP_REGISTRATION(RegistrationPointer)) {

        //
        // iA longjmp to an EM setjmp site, materialize the TargetFrame.
        // resume execution in EM mode.
        //

        PULONGLONG Record = (PULONGLONG)(RegistrationPointer);

        Frame.MemoryStackFp = Record[1];
        Frame.BackingStoreFp = Record[2];
    }

    //
    // Call real unwind routine specifying a context record as an
    // extra argument.
    //

    RtlUnwind2(Frame,
               TargetIp,
               ExceptionRecord,
               ReturnValue,
               &ContextRecord);

    return;
}


VOID
RtlUnwind2 (
    IN FRAME_POINTERS TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetFrame parameter is not specified, then the exit unwind
    flag is also set in the exception flags of the exception record. A backward
    scan through the procedure call frames is then performed to find the target
    of the unwind operation.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

Arguments:

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind. If this parameter is not specified, then an exit
        unwind is performed.

    TargetIp - Supplies an optional instruction address that specifies the
        continuation address of the unwind. This address is ignored if the
        target frame parameter is not specified.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

    ContextRecord - Supplies a pointer to a context record that can be used
        to store context druing the unwind operation.

Return Value:

    None.

--*/

{
    ULONGLONG TargetGp;
    ULONGLONG ImageBase;
    ULONGLONG ControlPc;
    ULONGLONG NextPc;
    ULONG ExceptionFlags;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    FRAME_POINTERS EstablisherFrame;
    EXCEPTION_RECORD ExceptionRecord1;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONGLONG HighStackLimit;
    ULONGLONG LowStackLimit;
    ULONGLONG HighBStoreLimit;
    ULONGLONG LowBStoreLimit;
    ULONG Size;
    BOOLEAN InFunction;

    //
    // Get current memory stack and backing store limits, capture the 
    // current context, virtually unwind to the caller of this routine, 
    // get the initial PC value, and set the unwind target address.
    //
    // N.B. The target gp value is found in the input context record.
    //      The unwinder guarantees that it will not be destroyed
    //      as it is just a scratch register.
    //

    RtlCaptureContext(ContextRecord);

    //
    // Before getting the limits from the TEB, must flush the RSE to have
    // the OS to grow the backing store and update the BStoreLimit.
    //

    Rtlp64GetStackLimits(&LowStackLimit, &HighStackLimit);
    Rtlp64GetBStoreLimits(&LowBStoreLimit, &HighBStoreLimit);

    ControlPc = RtlIa64InsertIPSlotNumber((ContextRecord->BrRp-16), 2);
    FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, &TargetGp);
    NextPc = RtlVirtualUnwind(ImageBase,
                              ControlPc,
                              FunctionEntry,
                              ContextRecord,
                              &InFunction,
                              &EstablisherFrame,
                              NULL);

    ControlPc = NextPc;
    ContextRecord->StIIP = (ULONGLONG)TargetIp;

#if defined(NTOS_KERNEL_RUNTIME)
    ContextRecord->StIPSR = SANITIZE_PSR(ContextRecord->StIPSR, KernelMode);
#else
    ContextRecord->StIPSR = SANITIZE_PSR(ContextRecord->StIPSR, UserMode);
#endif // defined(NTOS_KERNEL_RUNTIME)

    //
    // If an exception record is not specified, then build a local exception
    // record for use in calling exception handlers during the unwind operation.
    //

    if (ARGUMENT_PRESENT(ExceptionRecord) == FALSE) {
        ExceptionRecord = &ExceptionRecord1;
        ExceptionRecord1.ExceptionCode = STATUS_UNWIND;
        ExceptionRecord1.ExceptionRecord = NULL;
        ExceptionRecord1.ExceptionAddress = (PVOID)ControlPc;
        ExceptionRecord1.NumberParameters = 0;
    }

    //
    // If the target frame of the unwind is specified, then a normal unwind
    // is being performed. Otherwise, an exit unwind is being performed.
    //

    ExceptionFlags = EXCEPTION_UNWINDING;
    if (TargetFrame.BackingStoreFp == 0 && TargetFrame.MemoryStackFp == 0) {
        ExceptionRecord->ExceptionFlags |= EXCEPTION_EXIT_UNWIND;
    }

    //
    // Scan backward through the call frame hierarchy and call exception
    // handlers until the target frame of the unwind is reached.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //


        FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, &TargetGp);

        //
        // If there is a function table entry for the routine, then 
        // virtually unwind to the caller of the routine to obtain the 
        // virtual frame pointer of the establisher, but don't update 
        // the context record.
        //

        if (FunctionEntry != NULL) {
            NextPc = RtlpVirtualUnwind(ImageBase,
                                       ControlPc,
                                       FunctionEntry,
                                       ContextRecord,
                                       &InFunction,
                                       &EstablisherFrame,
                                       NULL);

        //
        // If virtual frame is not within the specified limits, unaligned, 
        // or the target frame is below the virtual frame and an exit 
        // unwind is not being performed, then raise the exception 
        // STATUS_BAD_STACK or STATUS_BAD_BSTORE. Otherwise,
        // check to determine if the current routine has an exception
        // handler.
        //

            if (CHECK_MSTACK_FRAME(EstablisherFrame, TargetFrame)) {

                RAISE_EXCEPTION(STATUS_BAD_STACK, ExceptionRecord);

            } else if (CHECK_BSTORE_FRAME(EstablisherFrame, TargetFrame)) {

                RAISE_EXCEPTION(STATUS_BAD_STACK, ExceptionRecord);

            } else if (InFunction && IS_HANDLER_DEFINED(FunctionEntry, ImageBase)) {

                //
                // The frame has an exception handler.
                //
                // The handler (i.e. personality routine) has to be called to
                // execute any termination routines in this frame.
                //
                // The control PC, establisher frame pointer, the address
                // of the function table entry, and the address of the
                // context record are all stored in the dispatcher context.
                // This information is used by the unwind linkage routine
                // and can be used by the exception handler itself.
                //

                DispatcherContext.ControlPc = ControlPc;
                DispatcherContext.FunctionEntry = FunctionEntry;
                DispatcherContext.ImageBase = ImageBase;
                DispatcherContext.ContextRecord = ContextRecord;

                //
                // Call the exception handler.
                //

                do {

                    //
                    // If the establisher frame is the target of the unwind
                    // operation, then set the target unwind flag.
                    //

                    if (IS_SAME_FRAME(TargetFrame, EstablisherFrame)) {
                        ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
                    }

                    ExceptionRecord->ExceptionFlags = ExceptionFlags;

                    //
                    // Set the specified return value in case the exception
                    // handler directly continues execution.
                    //

                    ContextRecord->IntV0 = (ULONGLONG)ReturnValue;

                    //
                    // A linkage routine written in assembler is used to 
                    // actually call the actual exception handler. This is 
                    // required by the exception handler that is associated 
                    // with the linkage routine so it can have access to two 
                    // sets of dispatcher context when it is called.
                    //

                    DispatcherContext.EstablisherFrame = EstablisherFrame;
                    Disposition = RtlpExecuteEmHandlerForUnwind(
                                      ExceptionRecord,
                                      EstablisherFrame.MemoryStackFp,
                                      EstablisherFrame.BackingStoreFp,
                                      ContextRecord,
                                      &DispatcherContext,
                                      TargetGp,
                                      HANDLER(FunctionEntry, ImageBase, TargetGp));

                    //
                    // Clear target unwind and collided unwind flags.
                    //

                    ExceptionFlags &= ~(EXCEPTION_COLLIDED_UNWIND |
                                                EXCEPTION_TARGET_UNWIND);

                    //
                    // Case on the handler disposition.
                    //

                    switch (Disposition) {

                    //
                    // The disposition is to continue the search.
                    //
                    // If the target frame has not been reached, then
                    // virtually unwind to the caller of the current
                    // routine, update the context record, and continue
                    // the search for a handler.
                    //

                    case ExceptionContinueSearch :

                        if (!IS_SAME_FRAME(EstablisherFrame, TargetFrame)) {
                            NextPc = RtlVirtualUnwind(ImageBase,
                                                      ControlPc,
                                                      FunctionEntry,
                                                      ContextRecord,
                                                      &InFunction,
                                                      &EstablisherFrame,
                                                      NULL);
                        }
                        break;

                    //
                    // The disposition is collided unwind.
                    //
                    // Set the target of the current unwind to the context
                    // record of the previous unwind, and reexecute the
                    // exception handler from the collided frame with the
                    // collided unwind flag set in the exception record.
                    //

                    case ExceptionCollidedUnwind :

                        ControlPc = DispatcherContext.ControlPc;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        ImageBase = DispatcherContext.ImageBase;
                        ContextRecord = DispatcherContext.ContextRecord;
                        ContextRecord->StIIP = (ULONGLONG)TargetIp;
                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        EstablisherFrame = DispatcherContext.EstablisherFrame;
                        TargetGp = (ULONG_PTR)(RtlImageDirectoryEntryToData(
                                                   (PVOID)ImageBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                                                   &Size
                                                   ));
                        break;

                    //
                    // All other disposition values are invalid.
                    //
                    // Raise invalid disposition exception.
                    //

                    default :
                        RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                    }

                } while ((ExceptionFlags & EXCEPTION_COLLIDED_UNWIND) != 0);

            } else {

                //
                // If the target frame has not been reached, then virtually 
                // unwind to the caller of the current routine and update 
                // the context record.
                //

                if (!IS_SAME_FRAME(EstablisherFrame, TargetFrame)) {
                    NextPc = RtlVirtualUnwind(ImageBase,
                                              ControlPc,
                                              FunctionEntry,
                                              ContextRecord,
                                              &InFunction,
                                              &EstablisherFrame,
                                              NULL);
                }
            }

        } else {

            //
            // No function table entry was found.  
            //

            SHORT BsFrameSize, TempFrameSize;

            NextPc = RtlIa64InsertIPSlotNumber((ContextRecord->BrRp-16), 2);
            ContextRecord->StIFS = ContextRecord->RsPFS;
            BsFrameSize = (SHORT)(ContextRecord->StIFS >> PFS_SIZE_SHIFT) & PFS_SIZE_MASK;
            TempFrameSize = BsFrameSize - (SHORT)((ContextRecord->RsBSP >> 3) & NAT_BITS_PER_RNAT_REG);
            while (TempFrameSize > 0) {
                BsFrameSize++;
                TempFrameSize -= NAT_BITS_PER_RNAT_REG;
            }
            ContextRecord->RsBSP -= BsFrameSize * sizeof(ULONGLONG);
            ContextRecord->RsBSPSTORE = ContextRecord->RsBSP;
        }

        //
        // Set point at which control left the previous routine.
        //

        ControlPc = NextPc;

    } while (((EstablisherFrame.MemoryStackFp < HighStackLimit) ||
             (EstablisherFrame.BackingStoreFp > LowBStoreLimit)) &&
             !(IS_SAME_FRAME(EstablisherFrame, TargetFrame)));

    //
    // If the establisher stack pointer is equal to the target frame
    // pointer, then continue execution. Otherwise, an exit unwind was
    // performed or the target of the unwind did not exist and the
    // debugger and subsystem are given a second chance to handle the
    // unwind.
    //

    if (IS_SAME_FRAME(EstablisherFrame, TargetFrame)) {
        ContextRecord->IntGp = TargetGp;
        ContextRecord->StIPSR &= ~(0x3i64 << PSR_RI);
        ContextRecord->IntV0 = (ULONGLONG)ReturnValue;
        RtlRestoreContext(ContextRecord, ExceptionRecord);
    } else {
        ZwRaiseException(ExceptionRecord, ContextRecord, FALSE);
    }
}

BOOLEAN
RtlDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function attempts to dispatch an exception to a frame based
    handler by searching backwards through the call frames by unwinding
    the RSE backing store as well as the memory stack.  The search begins 
    with the frame specified in the context record and continues backward 
    until either a handler is found that handles the exception, the stack 
    and/or the backing store is found to be invalid (i.e., out of limits 
    or unaligned), or the end of the call hierarchy is reached.

    As each frame is encountered, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called. If the
    handler does not handle the exception, then the prologue of the routine
    is undone to "unwind" the effect of the prologue and then the next
    frame is examined.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    If the exception is handled by one of the frame based handlers, then
    a value of TRUE is returned. Otherwise a value of FALSE is returned.

--*/

{
    ULONGLONG TargetGp;
    ULONGLONG ImageBase;
    CONTEXT ContextRecordEm;
    ULONGLONG ControlPc;
    ULONGLONG NextPc;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    ULONG ExceptionFlags;
    PRUNTIME_FUNCTION FunctionEntry;
    FRAME_POINTERS EstablisherFrame;
    FRAME_POINTERS TargetFrame;       // to be removed in the future
    ULONGLONG HighStackLimit; 
    ULONGLONG LowStackLimit;
    ULONGLONG HighBStoreLimit; 
    ULONGLONG LowBStoreLimit;
    FRAME_POINTERS NestedFrame;
    FRAME_POINTERS NullFrame;
    ULONG Index;
    ULONG Size;
    BOOLEAN InFunction;

    //
    // Get the current stack limits, make a copy of the context record,
    // get the initial PC value, capture the exception flags, and set
    // the nested exception frame pointer.
    //

    Rtlp64GetStackLimits(&LowStackLimit, &HighStackLimit);
    Rtlp64GetBStoreLimits(&LowBStoreLimit, &HighBStoreLimit);

    RtlMoveMemory(&ContextRecordEm, ContextRecord, sizeof(CONTEXT));

    if ( (ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION) &&
         (ExceptionRecord->NumberParameters == 5) &&
         (ExceptionRecord->ExceptionInformation[4] & (1 << ISR_X)) )
    {
        ControlPc = ExceptionRecord->ExceptionInformation[3];
        ControlPc = RtlIa64InsertIPSlotNumber(ControlPc,
                               ((ContextRecordEm.StIPSR >> PSR_RI) & 0x3));
    } else {
        ControlPc = RtlIa64InsertIPSlotNumber(ContextRecordEm.StIIP,
                               ((ContextRecordEm.StIPSR >> PSR_RI) & 0x3));
    }

    ExceptionFlags = ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE;

    INITIALIZE_FRAME(NestedFrame);
    INITIALIZE_FRAME(NullFrame);

    //
    // Start with the frame specified by the context record and search
    // backwards through the chain of call frames attempting to find an
    // exception handler that will handle the exception.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //


        FunctionEntry = RtlLookupFunctionEntry(ControlPc, &ImageBase, &TargetGp);

        //
        // If there is a function table entry for the routine, then 
        // virtually unwind to the caller of the current routine to 
        // obtain the virtual frame pointer of the establisher and check 
        // if there is an exception handler for the frame.
        //

        if (FunctionEntry != NULL) {
            NextPc = RtlVirtualUnwind(ImageBase,
                                      ControlPc,
                                      FunctionEntry,
                                      &ContextRecordEm,
                                      &InFunction,
                                      &EstablisherFrame,
                                      NULL);

            //
            // If either one or both of the two virtual frame pointers are 
            // not within the specified stack limits or unaligned, 
            // then set the stack invalid flag in the exception record and 
            // return exception not handled.  Otherwise, check if the 
            // current routine has an exception handler.
            //

            if (CHECK_MSTACK_FRAME(EstablisherFrame, NullFrame)) {

                ExceptionFlags |= EXCEPTION_STACK_INVALID;
                break;

            } else if (CHECK_BSTORE_FRAME(EstablisherFrame, NullFrame)) {
    
                ExceptionFlags |= EXCEPTION_STACK_INVALID;
                break;

            } else if ((IS_HANDLER_DEFINED(FunctionEntry, ImageBase) && InFunction)) {

                // 
                // The handler (i.e. personality routine) has to be called 
                // to search for an exception handler in this frame.  The 
                // handler must be executed by calling a stub routine that 
                // is written in assembler.  This is required because up 
                // level addressing of this routine information is required 
                // when a nested exception is encountered.
                //

                DispatcherContext.ControlPc = ControlPc;
                DispatcherContext.FunctionEntry = FunctionEntry;
                DispatcherContext.ImageBase = ImageBase;

                do {

                    ExceptionRecord->ExceptionFlags = ExceptionFlags;

                    if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) {
                        Index = RtlpLogExceptionHandler(
                                        ExceptionRecord,
                                        ContextRecord,
                                        (ULONG)ControlPc,
                                        FunctionEntry,
                                        sizeof(RUNTIME_FUNCTION));
                    }

                    DispatcherContext.EstablisherFrame = EstablisherFrame;
                    DispatcherContext.ContextRecord = ContextRecord;
                    Disposition = RtlpExecuteEmHandlerForException(
                                      ExceptionRecord,
                                      EstablisherFrame.MemoryStackFp,
                                      EstablisherFrame.BackingStoreFp,
                                      ContextRecord,
                                      &DispatcherContext, 
                                      TargetGp,
                                      HANDLER(FunctionEntry, ImageBase, TargetGp)); 

                    if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) {
                        RtlpLogLastExceptionDisposition(Index, Disposition);
                    }

                    ExceptionFlags |=
                        (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

                    ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;

                    //
                    // If the current scan is within a nested context and the
                    // frame just examined is the end of the nested region,
                    // then clear the nested context frame and the nested
                    // exception flag in the exception flags.
                    //

                    if (IS_SAME_FRAME(NestedFrame, EstablisherFrame)) {
                        ExceptionFlags &= (~EXCEPTION_NESTED_CALL);
                        INITIALIZE_FRAME(NestedFrame);
                    }

                    //
                    // Case on the handler disposition.
                    //

                    switch (Disposition) {

                    //
                    // The disposition is to continue execution.
                    //
                    // If the exception is not continuable, then raise the
                    // exception STATUS_NONCONTINUABLE_EXCEPTION.  Otherwise,
                    // return exception handled.
                    //

                    case ExceptionContinueExecution:
                        if ((ExceptionFlags & EXCEPTION_NONCONTINUABLE) != 0) {
                            RAISE_EXCEPTION(STATUS_NONCONTINUABLE_EXCEPTION, 
                                            ExceptionRecord);
                        } else {
                            return TRUE;
                        }

                    //
                    // The disposition is to continue the search.
                    //
                    // Get the next frame address and continue the search.
                    //

                    case ExceptionContinueSearch:
                        break;

                    //
                    // The disposition is nested exception.
                    //
                    // Set the nested context frame to the establisher frame
                    // address and set the nested exception flag in the
                    // exception flags.
                    //
        
                    case ExceptionNestedException:
                        ExceptionFlags |= EXCEPTION_NESTED_CALL;
                        if (DispatcherContext.EstablisherFrame.MemoryStackFp > NestedFrame.MemoryStackFp) {
                            NestedFrame = DispatcherContext.EstablisherFrame;
                        }
                        break;

                    //
                    // The disposition is hitting a frame processed by a
                    // previous unwind.
                    //
                    // Set the target of the current dispatch to the context
                    // record of the previous unwind
                    //

                    case ExceptionCollidedUnwind:
                        ControlPc = DispatcherContext.ControlPc;
                        NextPc = ControlPc;
                        EstablisherFrame = DispatcherContext.EstablisherFrame;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        ImageBase = DispatcherContext.ImageBase;
                        RtlMoveMemory(&ContextRecordEm, 
                                      DispatcherContext.ContextRecord,
                                      sizeof(CONTEXT));
                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        TargetGp = (ULONG_PTR)(RtlImageDirectoryEntryToData(
                                                   (PVOID)ImageBase,
                                                   TRUE,
                                                   IMAGE_DIRECTORY_ENTRY_GLOBALPTR,
                                                   &Size
                                                   ));
                        break;

                    //
                    // All other disposition values are invalid.
                    //
                    // Raise invalid disposition exception.
                    //

                    default:
                        RAISE_EXCEPTION(STATUS_INVALID_DISPOSITION, ExceptionRecord);
                        break;
                    }

                } while ((ExceptionFlags & EXCEPTION_COLLIDED_UNWIND) != 0);

            }

        } else {

            //
            // No function table entry is found.
            //

            SHORT BsFrameSize, TempFrameSize;

            NextPc = RtlIa64InsertIPSlotNumber((ContextRecordEm.BrRp-16), 2);
            ContextRecordEm.StIFS = ContextRecordEm.RsPFS;
            BsFrameSize = (SHORT)(ContextRecordEm.StIFS >> PFS_SIZE_SHIFT) & PFS_SIZE_MASK;
            TempFrameSize = BsFrameSize - (SHORT)((ContextRecordEm.RsBSP >> 3) & NAT_BITS_PER_RNAT_REG);
            while (TempFrameSize > 0) {
                BsFrameSize++;
                TempFrameSize -= NAT_BITS_PER_RNAT_REG;
            }
            ContextRecordEm.RsBSP -= BsFrameSize * sizeof(ULONGLONG);
            ContextRecordEm.RsBSPSTORE = ContextRecordEm.RsBSP;

            if (NextPc == ControlPc) {
                break;
            }
        }

        //
        // Set the point at which control left the previous routine.
        //

        ControlPc = NextPc;

    } while ( (ContextRecordEm.IntSp < HighStackLimit) ||
              (ContextRecordEm.RsBSP > LowBStoreLimit) );

    //
    // Could not handle the exception.
    //
    // Set final exception flags and return exception not handled.
    //

    ExceptionRecord->ExceptionFlags = ExceptionFlags;
    return FALSE;
}


ULONGLONG
RtlpVirtualUnwind (
    IN ULONGLONG ImageBase,
    IN ULONGLONG ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN PCONTEXT ContextRecord,
    OUT PBOOLEAN InFunction,
    OUT PFRAME_POINTERS EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
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
    and a specially coded prologue restores the return address twice. Once
    from the fault instruction address and once from the saved return address
    register. The first restore is returned as the function value and the
    second restore is place in the updated context record.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

    N.B. This function copies the specified context record and only computes
         the establisher frame and whether control is actually in a function.

Arguments:

    ImageBase - Base address of the module to which the function belongs.

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

    CONTEXT LocalContext;

    //
    // Copy the context record so updates will not be reflected in the
    // original copy and then virtually unwind to the caller of the
    // specified control point.
    //

    RtlCopyMemory((PVOID)&LocalContext, ContextRecord, sizeof(CONTEXT));
    return RtlVirtualUnwind(ImageBase,
                            ControlPc,
                            FunctionEntry,
                            &LocalContext,
                            InFunction,
                            EstablisherFrame,
                            ContextPointers);
}

#if !defined(NTOS_KERNEL_RUNTIME)

PLIST_ENTRY
RtlGetFunctionTableListHead (
    VOID
    )

/*++

Routine Description:

    Return the address of the dynamic function table list head.

Return value:

    Address of dynamic function table list head.

--*/
{
    return &DynamicFunctionTable;
}

BOOLEAN
RtlAddFunctionTable(
    IN PRUNTIME_FUNCTION FunctionTable,
    IN ULONG             EntryCount,
    IN ULONGLONG         BaseAddress,
    IN ULONGLONG         TargetGp
    )

/*++

Routine Description:

    Add a dynamic function table to the dynamic function table list. Dynamic
    function tables describe code generated at run-time. The dynamic function
    tables are searched via a call to RtlLookupDynamicFunctionEntry().
    Normally this is only invoked via calls to RtlLookupFunctionEntry().

    The FunctionTable entries need not be sorted in any particular order. The
    list is scanned for a Min and Max address range and whether or not it is
    sorted. If the latter RtlLookupDynamicFunctionEntry() uses a binary
    search, otherwise it uses a linear search.

    The dynamic function entries will be searched only after a search
    through the static function entries associated with all current
    process images has failed.

Arguments:

   FunctionTable       Address of an array of function entries where
                       each element is of type RUNTIME_FUNCTION.

   EntryCount          The number of function entries in the array

   BaseAddress         Base address to calculate the real address  of the function table entry
   TargetGp            return back to RtlLookupFunctionEntry for future query.

Return value:

   TRUE                if RtlAddFunctionTable completed successfully
   FALSE               if RtlAddFunctionTable completed unsuccessfully

--*/
{
    PDYNAMIC_FUNCTION_TABLE pNew;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG i;

    if (EntryCount == 0)
        return FALSE;

    //
    // Make sure the link list is initialized;
    //

    if (DynamicFunctionTable.Flink == NULL) {
       InitializeListHead(&DynamicFunctionTable);
    }

    //
    //  Allocate memory for this link list entry
    //

    pNew = RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(DYNAMIC_FUNCTION_TABLE) );

    if (pNew != NULL) {
        pNew->FunctionTable = FunctionTable;
        pNew->EntryCount = EntryCount;
        NtQuerySystemTime( &pNew->TimeStamp );

        //
        // Scan the function table for Minimum/Maximum and to determine
        // if it is sorted. If the latter, we can perform a binary search.
        //

        FunctionEntry = FunctionTable;
        pNew->MinimumAddress = RF_BEGIN_ADDRESS( BaseAddress, FunctionEntry);
        pNew->MaximumAddress = RF_END_ADDRESS(BaseAddress, FunctionEntry);
        pNew->Sorted = TRUE;
        FunctionEntry++;

        for (i = 1; i < EntryCount; FunctionEntry++, i++) {
            if (pNew->Sorted && FunctionEntry->BeginAddress < FunctionTable[i-1].BeginAddress) {
                pNew->Sorted = FALSE;
            }
            if (RF_BEGIN_ADDRESS(FunctionTable, FunctionEntry) < pNew->MinimumAddress) {
                pNew->MinimumAddress = RF_BEGIN_ADDRESS( BaseAddress, FunctionEntry);
            }
            if (RF_END_ADDRESS( FunctionTable, FunctionEntry) > pNew->MaximumAddress) {
                pNew->MaximumAddress = RF_END_ADDRESS( BaseAddress, FunctionEntry);
            }
        }

        //
        // Insert the new entry in the dynamic function table list.
        // Protect the insertion with the loader lock.
        //

        pNew->BaseAddress = BaseAddress;
        pNew->TargetGp    = TargetGp;

        RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        InsertTailList((PLIST_ENTRY)&DynamicFunctionTable, (PLIST_ENTRY)pNew);
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

        return TRUE;
    } else {
        return FALSE;
    }
}

BOOLEAN
RtlDeleteFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable
    )
{

/*++

Routine Description:

    Remove a dynamic function table from the dynamic function table list.

Arguments:

   FunctionTable       Address of an array of function entries that
                       was passed in a previous call to RtlAddFunctionTable

Return Value

    TRUE - If function completed successfully
    FALSE - If function completed unsuccessfully

--*/

    PDYNAMIC_FUNCTION_TABLE CurrentEntry;
    PLIST_ENTRY Head;
    PLIST_ENTRY Next;
    BOOLEAN Status = FALSE;

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);

    //
    // Search the dynamic function table list for a match on the the function
    // table address.
    //

    Head = &DynamicFunctionTable;
    for (Next = Head->Blink; Next != Head; Next = Next->Blink) {
        CurrentEntry = CONTAINING_RECORD(Next,DYNAMIC_FUNCTION_TABLE,Links);
        if (CurrentEntry->FunctionTable == FunctionTable) {
            RemoveEntryList((PLIST_ENTRY)CurrentEntry);
            RtlFreeHeap( RtlProcessHeap(), 0, CurrentEntry );
            Status = TRUE;
            break;
        }
    }

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    return Status;
}

PRUNTIME_FUNCTION
RtlLookupDynamicFunctionEntry(
    IN ULONG_PTR ControlPc,
    OUT PULONGLONG ImageBase,
    OUT PULONGLONG TargetGp
    )

/*++

Routine Description:

  This function searches through the dynamic function entry
  tables and returns the function entry address that corresponds
  to the specified ControlPc. This routine does NOT perform the
  secondary function entry indirection. That is performed
  by RtlLookupFunctionEntry().

  Argument:

     ControlPc           Supplies a ControlPc.
     ImageBase           OUT Base address for dynamic code

  Return Value

     NULL - No function entry found that contains the ControlPc.

     NON-NULL - Address of the function entry that describes the
                code containing ControlPC.

--*/

{
    PDYNAMIC_FUNCTION_TABLE CurrentEntry;
    PLIST_ENTRY Next,Head;
    PRUNTIME_FUNCTION FunctionTable;
    PRUNTIME_FUNCTION FunctionEntry;
    LONG High;
    LONG Low;
    LONG Middle;
    SIZE_T BaseAddress;


    if (DynamicFunctionTable.Flink == NULL || ImageBase == NULL)
        return NULL;


        //
        //  Search the tree starting from the head, continue until the entry
        //  is found or we reach the end of the list.
        //
    if (RtlTryEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock) ) {

        Head = &DynamicFunctionTable;
        for (Next = Head->Blink; Next != Head; Next = Next->Blink) {
            CurrentEntry = CONTAINING_RECORD(Next,DYNAMIC_FUNCTION_TABLE,Links);
            FunctionTable = CurrentEntry->FunctionTable;

            //
            // Check if the ControlPC is within the range of this function table
            //

            if ((ControlPc >= CurrentEntry->MinimumAddress) &&
                (ControlPc <  CurrentEntry->MaximumAddress) ) {


                // If this function table is sorted do a binary search.

                BaseAddress = CurrentEntry->BaseAddress;
                if (CurrentEntry->Sorted) {

                    //
                    // Perform binary search on the function table for a function table
                    // entry that subsumes the specified PC.
                    //

                    Low = 0;
                    High = CurrentEntry->EntryCount -1 ;
                    
                    while (High >= Low) {

                        //
                        // Compute next probe index and test entry. If the specified PC
                        // is greater than of equal to the beginning address and less
                        // than the ending address of the function table entry, then
                        // return the address of the function table entry. Otherwise,
                        // continue the search.
                        //


                        Middle = (Low + High) >> 1;
                        FunctionEntry = &FunctionTable[Middle];

                        if (ControlPc < RF_BEGIN_ADDRESS( BaseAddress, FunctionEntry)) {
                            High = Middle - 1;

                        } else if (ControlPc >= RF_END_ADDRESS( BaseAddress, FunctionEntry)) {
                            Low = Middle + 1;

                        } else {

                            *ImageBase = CurrentEntry->BaseAddress;

                            if ( TargetGp != NULL )
                                *TargetGp  = CurrentEntry->TargetGp;

                            RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                            return FunctionEntry;
                        }
                    }

                } else {    // Not sorted. Do linear search.

                    PRUNTIME_FUNCTION LastFunctionEntry = &FunctionTable[CurrentEntry->EntryCount];


                    for (FunctionEntry = FunctionTable; FunctionEntry < LastFunctionEntry; FunctionEntry++) {

                        if ((ControlPc >= RF_BEGIN_ADDRESS( BaseAddress, FunctionEntry)) &&
                            (ControlPc <  RF_END_ADDRESS( BaseAddress, FunctionEntry))) {

                            
                            *ImageBase = CurrentEntry->BaseAddress;

                            if ( TargetGp != NULL )
                                *TargetGp  = CurrentEntry->TargetGp;

                            RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                            return FunctionEntry;
                        }
                    }
                } // binary/linear search
            } // if in range
        } // for (... Next != Head ...)
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    } // LoaderLock

    return NULL;
}

#endif
