//      TITLE("Miscellaneous Exception Handling")
//++
//
// Module Name:
//
//    xcptmisc.s
//
// Abstract:
//
//    This module implements miscellaneous routines that are required to
//    support exception handling. Functions are provided to call an exception
//    handler for an exception, call an exception handler for unwinding, call
//    an exception filter, and call a termination handler.
//
// Author:
//
//    William K. Cheung (wcheung) 15-Jan-1996
//
//    based on the version by David N. Cutler (davec) 12-Sep-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file "xcptmisc.s"

//++
//
// EXCEPTION_DISPOSITION
// RtlpExceptionHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN FRAME_POINTERS EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a nested exception occurs. Its function
//    is to retrieve the establisher frame pointer from its establisher's
//    call frame, store this information in the dispatcher context record,
//    and return a disposition value of nested exception.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1,a2) - Supplies the memory stack and backing store
//       frame pointers of the establisher of this exception handler.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionNestedException is returned if an unwind
//    is not in progress. Otherwise a value of ExceptionContinueSearch is
//    returned.
//
//--

        LEAF_ENTRY(RtlpExceptionHandler)

        //
        // register aliases
        //

        pUwnd       = pt0
        pNot        = pt1


        ARGPTR(a0)
        ARGPTR(a4)

//
// Check if unwind is in progress.
//

        add         t0 = ErExceptionFlags, a0
        ;;
        ld4.nt1     t0 = [t0]
        mov         t1 = EXCEPTION_UNWIND
        ;;

        add         t3 = -8, a1
        and         t0 = t0, t1
        add         t2 = DcEstablisherFrame, a4
        ;;

        ld8.nt1     t4 = [t3]                   // get dispatcher context addr
        cmp4.ne     pUwnd, pNot = zero, t0      // if ne, unwind in progress
        ;;
 (pNot) add         t5 = DcEstablisherFrame, t4
        ;;

//
// If unwind is not in progress - return nested exception disposition.
// And copy the establisher frame pointer structure (i.e. FRAME_POINTERS) 
// to the current dispatcher context.
//
// Otherwise, return continue search disposition
//

 (pNot) ld8.nt1     t6 = [t5], 8
 (pNot) mov         v0 = ExceptionNestedException   // set disposition value
(pUwnd) mov         v0 = ExceptionContinueSearch    // set disposition value
        ;;

 (pNot) ld8.nt1     t7 = [t5]
 (pNot) st8         [t2] = t6, 8
        nop.i       0
        ;;

 (pNot) st8         [t2] = t7
        nop.m       0
        br.ret.sptk.clr brp
        ;;

        LEAF_EXIT(RtlpExceptionHandler)

//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteEmHandlerForException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONGLONG MemoryStack,
//    IN ULONGLONG BackingStore,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
//    IN ULONGLONG GlobalPointer,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//
//    This function stores the establisher's dispatcher context in the stack
//    scratch area, establishes an exception handler, and then calls
//    the specified exception handler as an exception handler. If a nested
//    exception occurs, then the exception handler of this function is called
//    and the establisher frame pointer in the saved dispatcher context
//    is returned to the exception dispatcher via the dispatcher context 
//    parameter of this function's exception handler. If control is returned 
//    to this routine, then the disposition status is returned to the 
//    exception dispatcher.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    MemoryStack (a1) - Supplies the memory stack frame pointer of the 
//       activation record whose exception handler is to be called.
//
//    BackingStore (a2) - Supplies the backing store pointer of the 
//       activation record whose exception handler is to be called.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
//    GlobalPointer (a5) - Supplies the global pointer value of the module
//       to which the function belongs.
//
//    ExceptionRoutine (a6) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        NESTED_ENTRY_EX(RtlpExecuteEmHandlerForException,RtlpExceptionHandler)

        //
        // register aliases
        //

        rpT0        = t8
        rpT1        = t9

        .prologue
        .fframe     32, tg30

        alloc       t1 = ar.pfs, 0, 0, 7, 0
        mov         t0 = brp
        mov         rpT0 = sp

        add         rpT1 = 8, sp
[tg30:] add         sp = -32, sp
        ARGPTR(a6)
        ;;

        .savesp     rp, 32
        st8         [rpT0] = t0, -8             // save brp
        .savesp     ar.pfs, 32+8
        st8         [rpT1] = t1, 8              // save pfs
        ARGPTR(a4)
        ;;

        PROLOGUE_END

//
// Setup global pointer and branch register for the except handler
//

        ld8         t2 = [a6], PlGlobalPointer - PlEntryPoint
        st8.nta     [rpT0] = a4                 // save dispatcher context addr
        ;;

        ld8         gp = [a6]
        mov         bt0 = t2
        br.call.sptk.many brp = bt0             // call except handler

//
// Save swizzled dispatcher context address onto the stack
//

        .restore    tg40
[tg40:] add         sp = 32, sp                 // deallocate stack frame
        ;;
        ld8.nt1     t0 = [sp]
        add         rpT1 = 8, sp
        ;;

        ld8.nt1     t1 = [rpT1]
        nop.f       0
        mov         brp = t0                    // restore return branch
        ;;

        nop.m       0
        mov         ar.pfs = t1                 // restore pfs
        br.ret.sptk.clr brp                     // return

        NESTED_EXIT(RtlpExecuteEmHandlerForException)

//++
//
// EXCEPTION_DISPOSITION
// RtlpEmUnwindHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN FRAME_POINTERS EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a collided unwind occurs. Its function
//    is to retrieve the establisher dispatcher context, copy it to the
//    current dispatcher context, and return a disposition value of nested
//    unwind.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1,a2) - Supplies the memory stack and backing store
//       frame pointers of the establisher of this exception handler.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionCollidedUnwind is returned if an unwind is
//    in progress. Otherwise, a value of ExceptionContinueSearch is returned.
//
//--

        LEAF_ENTRY(RtlpEmUnwindHandler)

        //
        // register aliases
        //

#if 0

        pUwnd       = pt0
        pNot        = pt1

//
// Check if unwind is in progress.
//

        add         t0 = ErExceptionFlags, a0
        ;;
        ld4.nt1     t0 = [t0]
        mov         t1 = EXCEPTION_UNWIND
        ;;
        and         t0 = t0, t1
        ;;

        cmp4.eq     pNot, pUwnd = zero, t0      // if eq, unwind not in progress
 (pNot) br.cond.sptk Ruh10

#endif // 0

        ARGPTR(a4)                              // -> target dispatch context
        add         t2 = -8, a1
        add         t1 = 8, a4                  // -> target dispatch context+8
        ;;

        ld8.nt1     t2 = [t2]                   // -> source dispatch context
        ;;
        add         t3 = 8, t2                  // -> source dispatch context+8
        nop.i       0
        ;;

//
// Copy the establisher dispatcher context (i.e. DISPATCHER_CONTEXT) contents
// to the current dispatcher context.
//

        ld8         t6 = [t2], 16
        ld8         t7 = [t3], 16
        nop.i       0
        ;;

        ld8         t8 = [t2], 16
        ld8         t9 = [t3], 16
        nop.i       0
        ;;
        
        st8         [a4] = t6, 16
        st8         [t1] = t7, 16
        nop.i       0

        LDPTR       (t10, t2)
        LDPTR       (t11, t3)
        nop.i       0
        ;;

        st8         [a4] = t8, 16
        st8         [t1] = t9, 16
        mov         v0 = ExceptionCollidedUnwind    // set disposition value
        ;;

        STPTR       (a4, t10)
        STPTR       (t1, t11)
        br.ret.sptk.clr brp                         // return
        ;;


#if 0

Ruh10:     

//
// If branched to here,
// unwind is not in progress - return continue search disposition.
//

        nop.m       0
(pNot)  mov         v0 = ExceptionContinueSearch    // set disposition value
        br.ret.sptk.clr brp                         // return

#endif // 0

        LEAF_EXIT(RtlpEmUnwindHandler)

//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteEmHandlerForUnwind (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONGLONG MemoryStack,
//    IN ULONGLONG BackingStore,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext,
//    IN ULONGLONG GlobalPointer,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//
//    This function allocates a call frame, stores the establisher frame
//    pointer and the context record address in the frame, establishes an
//    exception handler, and then calls the specified exception handler as
//    an unwind handler. If a collided unwind occurs, then the exception
//    handler of of this function is called and the establisher frame pointer
//    and context record address are returned to the unwind dispatcher via
//    the dispatcher context parameter. If control is returned to this routine,
//    then the frame is deallocated and the disposition status is returned to
//    the unwind dispatcher.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    MemoryStack (a1) - Supplies the memory stack frame pointer of the 
//       activation record whose exception handler is to be called.
//
//    BackingStore (a2) - Supplies the backing store pointer of the 
//       activation record whose exception handler is to be called.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
//    GlobalPointer (a5) - Supplies the global pointer value of the module
//       to which the function belongs.
//
//    ExceptionRoutine (a6) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--


        NESTED_ENTRY_EX(RtlpExecuteEmHandlerForUnwind, RtlpEmUnwindHandler)

        //
        // register aliases
        //

        .prologue
        .fframe     32, tg10

        rpT0        = t8
        rpT1        = t9


        alloc       t1 = ar.pfs, 0, 0, 7, 0
        mov         t0 = brp
        mov         rpT0 = sp

        add         rpT1 = 8, sp
[tg10:] add         sp = -32, sp
        ARGPTR(a6)
        ;;

        .savesp     rp, 32
        st8         [rpT0] = t0, -8             // save brp
        .savesp     ar.pfs, 32+8
        st8         [rpT1] = t1, 8              // save pfs
        ARGPTR(a4)
        ;;

        PROLOGUE_END

//
// Setup global pointer and branch register for the except handler
//

        ld8         t2 = [a6], PlGlobalPointer - PlEntryPoint
        st8.nta     [rpT0] = a4                 // save dispatcher context addr
        ;;

        ld8         gp = [a6]
        mov         bt0 = t2
 (p0)   br.call.sptk.many brp = bt0             // call except handler

//
// Save swizzled dispatcher context address onto the stack
//

        .restore    tg20
[tg20:] add         sp = 32, sp // deallocate stack frame
        ;;
        ld8.nt1     t0 = [sp]
        add         rpT1 = 8, sp
        ;;

        ld8.nt1     t1 = [rpT1]
        nop.f       0
        mov         brp = t0                    // restore return branch
        ;;

        nop.m       0
        mov         ar.pfs = t1                 // restore pfs
        br.ret.sptk.clr brp                     // return
        ;;

        NESTED_EXIT(RtlpExecuteEmHandlerForUnwind)

#if 0
//++
//
// EXCEPTION_DISPOSITION
// RtlpUnwindHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN FRAME_POINTERS EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a collided unwind occurs. Its function
//    is to retrieve the establisher dispatcher context, copy it to the
//    current dispatcher context, and return a disposition value of nested
//    unwind.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1,a2) - Supplies the memory stack and backing store
//       frame pointers of the establisher of this exception handler.
//
//    ContextRecord (a3) - Supplies a pointer to a context record.
//
//    DispatcherContext (a4) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionCollidedUnwind is returned if an unwind is
//    in progress. Otherwise, a value of ExceptionContinueSearch is returned.
//
//--
        LEAF_ENTRY(RtlpUnwindHandler)

        LEAF_RETURN
        LEAF_EXIT(RtlpUnwindHandler)


//
// constants & register aliases for EM-iA transition stubs
//

        rIA32Ptr    = r2                        // IA32 resources pointer
        rTeb        = r3                        // TEB pointer
        rIA32Rsrc   = ar.k7                     // Offset in TEB for iA32 stuff

        rES         = r16
        rCS         = r17
        rSS         = r18
        rDS         = r19
        rFS         = r20
        rGS         = r21
        rLDT        = r22
        rEFLAG      = ar24
        rESD        = r24
        rCSD        = ar25
        rSSD        = ar26
        rDSD        = r27
        rFSD        = r28
        rGSD        = r29
        rLDTD       = r30
        rGDTD       = r31

        isIA        = pt0
        isTIA       = pt1



//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteX86HandlerForException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher 
//       whose exception handler is to be called.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (a4) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        NESTED_ENTRY_EX(RtlpExecuteX86HandlerForException,RtlpExceptionHandler)

        .regstk     5, 6, 0, 0
        .prologue   0xe, loc0

        alloc       loc1 = ar.pfs, 5, 6, 0, 0
        mov         loc2 = brp
        mov         loc0 = sp

        PROLOGUE_END

        mov         rTeb = teb
        add         r13 = 4, sp
        mov         r14 = sp

        mov         loc3 = ar.fpsr
        sxt4        a3 = a3
        sxt4        gp = a4
        ;;

        mov         rIA32Ptr = rIA32Rsrc
        st4         [r14] = a0, 8
        mov         loc4 = pr
        ;;

        st4         [r13] = a1, 8
        st4         [r14] = a2, 16
        mov         loc5 = ar.lc
        ;;

        st4         [r13] = a3
        mov         bt0 = gp
        br.call.sptk brp = _EM_IA_ExecuteHandler_Transition

        mov         ar.fpsr = loc3
        mov         ar.pfs = loc0
        mov         brp = loc1

        add         sp = r0, loc2
        mov         pr = loc4, -1
        mov         ar.lc = loc5

        nop.m       0
        nop.m       0
        br.ret.sptk.clr brp

        NESTED_EXIT(RtlpExecuteX86HandlerForException)


        LEAF_ENTRY(_EM_IA_ExecuteHandler_Transition)

        alloc       loc1 = ar.pfs, 0, 96, 0, 0
        add         rIA32Ptr = rIA32Ptr, rTeb
        mov         rES = _DataSelector

        mov         rESD = rSSD
        mov         rSS = _DataSelector
        mov         rDS = _DataSelector

        mov         rDSD = rSSD
        mov         rCS = _CodeSelector
        mov         rFS = _FsSelector

        ld2         v0 = [gp], 8
        add         sp = -4, sp
        mov         rLDT = _LdtSelector
        ;;

        ld8         rGDTD = [rIA32Ptr], 8
//        movl        r3 = @fptr(_EM_IA_STUBRET_ExecuteX86HandlerStub)
        movl        r3 = artificial_return
        ;;

        ld8         rLDTD = [rIA32Ptr], 8
        movl        loc2 = JMPE_CONST
        ;;

        st4         [sp] = r3
        mov         rGS = _DataSelector
        cmp.eq      isTIA, isIA = loc2, v0
        ;;

        ld8         rFSD = [rIA32Ptr]
(isTIA) ld8         r2 = [gp], 8
        br.ia.sptk  bt0
        ;;

        ld8         gp = [gp]
        mov         bt0 = r2
        br          bt0

        LEAF_EXIT(_EM_IA_ExecuteHandler_Transition)


        LEAF_ENTRY(_EM_IA_STUBRET_ExecuteX86HandlerStub)

//
// Artifical return to the bundle following the call to ReX86Transition
//

artificial_return:
        .text
        data8       0xab80f
        data8       0x0

        nop.m       0
        nop.m       0
        br.ret.sptk brp

        LEAF_EXIT(_EM_IA_STUBRET_ExecuteX86HandlerStub)


//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteX86HandlerForUnwind (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//
//    This function allocates a call frame, stores the establisher frame
//    pointer and the context record address in the frame, establishes an
//    exception handler, and then calls the specified exception handler as
//    an unwind handler. If a collided unwind occurs, then the exception
//    handler of of this function is called and the establisher frame pointer
//    and context record address are returned to the unwind dispatcher via
//    the dispatcher context parameter. If control is returned to this routine,
//    then the frame is deallocated and the disposition status is returned to
//    the unwind dispatcher.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the memory stack and backing store
//       frame pointers of the establisher whose exception handler is to
//       be called.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (a4) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        NESTED_ENTRY_EX(RtlpExecuteX86HandlerForUnwind, RtlpUnwindHandler)

        .regstk     5, 6, 0, 0
        .prologue   0xe, loc0

        alloc       loc1 = ar.pfs, 5, 6, 0, 0
        mov         loc2 = brp
        mov         loc0 = sp

        PROLOGUE_END

        mov         rTeb = teb
        add         r13 = 4, sp
        mov         r14 = sp

        mov         loc3 = ar.fpsr
        sxt4        a3 = a3
        sxt4        gp = a4
        ;;

        mov         rIA32Ptr = rIA32Rsrc
        st4         [r14] = a0, 8
        mov         loc4 = pr
        ;;

        st4         [r13] = a1, 8
        st4         [r14] = a2, 16
        mov         loc5 = ar.lc
        ;;

        st4         [r13] = a3
        mov         bt0 = a4
        br.call.sptk brp = _EM_IA_ExecuteHandler_Transition

        mov         ar.fpsr = loc3
        mov         ar.pfs = loc0
        mov         brp = loc1

        add         sp = r0, loc2
        mov         pr = loc4, -1
        mov         ar.lc = loc5

        nop.m       0
        nop.m       0
        br.ret.sptk.clr brp

        NESTED_EXIT(RtlpExecuteX86HandlerForUnwind)

#endif //0
