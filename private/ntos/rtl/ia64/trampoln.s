//++
//
// Module Name:
//
//    trampoln.s
//
// Abstract:
//
//    This module implements the trampoline code necessary to dispatch user
//    mode APCs.
//
// Author:
//
//    William K. Cheung  25-Oct-1995
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//    08-Feb-1996    Updated to EAS 2.1
//
//--

#include "ksia64.h"

        .file   "trampoln.s"

        PublicFunction(RtlpCaptureRnats)
        PublicFunction(RtlDispatchException)
        PublicFunction(RtlRaiseException)
        PublicFunction(RtlRaiseStatus)
        PublicFunction(ZwContinue)
        PublicFunction(ZwCallbackReturn)
        PublicFunction(ZwRaiseException)
        PublicFunction(ZwTestAlert)
        .global     Wow64PrepareForException


//++
//
// EXCEPTION_DISPOSITION
// KiUserApcHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//
// Routine Description:
//
//    This function is called when an exception occurs in an APC routine
//    or one of its dynamic descendents and when an unwind through the
//    APC dispatcher is in progress. If an unwind is in progress, then test
//    alert is called to ensure that all currently queued APCs are executed.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    ExceptionContinueSearch is returned as the function value.
//
//--

        NESTED_ENTRY (KiUserApcHandler)

        //
        // register aliases
        //

        pUwnd       = pt1
        pNot        = pt2


        NESTED_SETUP(1, 2, 0, 0)
        add         t0 = ErExceptionFlags, a0
        ;;

        PROLOGUE_END

        ld4         t2 = [t0]                   // get exception flags
        ;;
        and         t2 = EXCEPTION_UNWIND, t2   // check if unwind in progress
        ;;

        cmp4.ne     pUwnd, pNot = zero, t2
        ;;

 (pNot) add         v0 = ExceptionContinueSearch, zero
 (pNot) br.ret.sptk.clr brp                     // return
(pUwnd) br.call.spnt.many brp = ZwTestAlert

//
// restore preserved states and set the disposition value to continue search
//

        add         v0 = ExceptionContinueSearch, zero
        mov         brp = savedbrp              // restore return link
        nop.b       0
     
        nop.m       0
        mov         ar.pfs = savedpfs           // restore pfs
        br.ret.sptk.clr brp                     // return

        NESTED_EXIT (KiUserApcHandler)

//++
//
// VOID
// KiUserApcDispatcher (
//    IN PVOID NormalContext,
//    IN PVOID SystemArgument1,
//    IN PVOID SystemArgument2,
//    IN PKNORMAL_ROUTINE NormalRoutine
//    )
//
// Routine Description:
//
//    This routine is entered on return from kernel mode to deliver an APC
//    in user mode. The stack frame for this routine was built when the
//    APC interrupt was processed and contains the entire machine state of
//    the current thread. The specified APC routine is called and then the
//    machine state is restored and execution is continued.
//
// Arguments:
//
//    t0  - Supplies the normal context parameter that was specified when the
//          APC was initialized.
//
//    t1  - Supplies the first argument that was provied by the executive when
//          the APC was queued.
//
//    t2  - Supplies the second argument that was provided by the executive
//          when the APC was queued.
//
//    t3  - Supplies the address of the plabel for the function 
//          that is to be called.
//
// Return Value:
//
//    None.
//
//
// N.B. On entry, sp points to the stack scratch area at the top of the 
//      memory stack.
//
// N.B. The input arguments of this function are passed from the kernel
//      in scratch registers t0, t1, t2, and t3.
//
//--

        NESTED_ENTRY_EX (KiUserApcDispatch, KiUserApcHandler)
        ALTERNATE_ENTRY (KiUserApcDispatcher)

        .prologue
        .unwabi  @nt,  CONTEXT_FRAME

        .regstk     0, 0, 3, 0

        rBsp        = t10                       // BspStore
        rpCr        = t11                       // pointer to context record
        rpT1        = t12                       // temporary pointer

        alloc       t22 = ar.pfs, 0, 0, 3, 0    // 3 outputs
        add         t10 = STACK_SCRATCH_AREA+ContextFrameLength+24, sp
        add         t11 = STACK_SCRATCH_AREA+ContextFrameLength, sp
        ;;

        PROLOGUE_END

        ld8.nta     t12 = [t10], -8
        movl        s1 = _gp
        ;;

        ld8.nta     out0 = [t11], 8
        ld8.nta     t13 = [t12], PlGlobalPointer-PlEntryPoint
        ;;

        ld8.nta     out1 = [t11], 8
        ld8.nta     out2 = [t10]
        mov         bt0 = t13

        ld8.nta     gp = [t12]
        br.call.sptk.many brp = bt0             // call APC routine
        ;;

//
// On return, setup global pointer and branch register to call ZwContinue.
// Also, flush the RSE to sync up the bsp and bspstore pointers.  The 
// corresponding field in the context record is updated too.
//

        flushrs
        mov         out1 = 1                    // set TestAlert to TRUE
        ;;

        add         out0 = STACK_SCRATCH_AREA, sp   // context record address
        mov         gp = s1                     // restore gp
        br.call.sptk.many brp = ZwContinue
        ;;

//
// if successful, ZwContinue does not return here;
// otherwise, error happened.
//

        mov         gp = s1                     // restore gp
        mov         s0 = v0                     // save the return status
        ;;

Kuad10:
        mov         out0 = s0                   // setup 1st argument
        br.call.sptk.many brp = RtlRaiseStatus
        ;;
        
        nop.m       0
        nop.i       0
        br          Kuad10                      // loop on return

        NESTED_EXIT(KiUserApcDispatch)


//++
//
// VOID
// KiUserCallbackDispatcher (
//    VOID
//    )
//
// Routine Description:
//
//    This routine is entered on a callout from kernel mode to execute a
//    user mode callback function. All arguments for this function have
//    been placed on the stack.
//
// Arguments:
//
//    (sp + 32 + CkApiNumber) - Supplies the API number of the callback 
//                              function that is to be executed.
//
//    (sp + 32 + CkBuffer) - Supplies a pointer to the input buffer.
//
//    (sp + 32 + CkLength) - Supplies the input buffer length.
//
// Return Value:
//
//    This function returns to kernel mode.
//
// N.B. Preserved register s1 is used to save ZwCallbackReturn plabel address.
//      On entry, gp is set to the global pointer value of NTDLL
//
//--

        NESTED_ENTRY(KiUserCallbackDispatch)

        .prologue
        .savesp     rp, STACK_SCRATCH_AREA+CkBrRp
        .savesp     ar.pfs, STACK_SCRATCH_AREA+CkRsPFS
        .vframesp   STACK_SCRATCH_AREA+CkIntSp

        nop.m       0
        nop.m       0
        nop.i       0
        ;;

        PROLOGUE_END

        ALTERNATE_ENTRY(KiUserCallbackDispatcher)


        //
        // register aliases
        //

        rpT0        = t0                        // temporary pointer
        rpT1        = t1                        // temporary pointer
        rT0         = t2                        // temporary value
        rFunc       = t3                        // callback function entry
        rApi        = t4


        alloc       t22 = ar.pfs, 0, 0, 3, 0    // 3 outputs max.
        mov         teb = kteb                  // sanitize teb
        add         rpT0 = STACK_SCRATCH_AREA + CkApiNumber, sp
        movl        gp = _gp
        ;;
  
        ld4         rApi = [rpT0], CkBuffer - CkApiNumber   // get API number
        add         rpT1 = TePeb, teb
        mov         s0 = gp
        ;;

//
// load both input buffer address and length into scratch register t2
// and then deposit them into registers out0 & out1 respectively.
//
// N.B. t0 is 8-byte aligned.
//

        LDPTRINC(out0, rpT0, CkLength-CkBuffer) // input buffer address
        LDPTR(t11, rpT1)                        // get address of PEB
#if defined(_WIN64)
        shl         rApi = rApi, 3              // compute offset to table entry
#else
        shl         rApi = rApi, 2              // compute offset to table entry
#endif
        ;;

        ld4         out1 = [rpT0]               // get input buffer length
        add         t5 = PeKernelCallbackTable, t11
        ;;
        LDPTR(rFunc, t5)                        // address of callback table
        ;;

        add         rFunc = rApi, rFunc         // compute addr of table entry
        ;;
        LDPTR(t6, rFunc)                        // get plabel's address
        ;;

        ld8.nt1     t9 = [t6], PlGlobalPointer-PlEntryPoint  // load entry point address
        ;;

        ld8.nt1     gp = [t6]                   // load callee's GP
        mov         bt0 = t9
        br.call.sptk.many brp = bt0             // invoke the callback func
  
//
// If a return from the callback function occurs, then the output buffer
// address and length are returned as NULL.
//

        mov         out0 = zero                 // NULL output buffer addr
        mov         out1 = zero                 // zero output buffer len

        mov         out2 = v0                   // set completion status
        mov         gp = s0
        br.call.sptk.many brp = ZwCallbackReturn

//
// Unsuccessful completion after attempting to return to kernel mode. Use
// the return status as the exception code, set noncontinuable exception and
// attempt to raise another exception. Note there is no return from raise
// status.
//

        nop.m       0
        mov         gp = s0                     // restore our own GP
        mov         s0 = v0                     // save status value
        ;;
  
Kucd10:
        mov         out0 = s0                   // set status value
        br.call.sptk.many brp = RtlRaiseStatus
        
        nop.m       0
        nop.m       0
        br          Kucd10                      // jump back to Kucd10

        NESTED_EXIT(KiUserCallbackDispatch)

//++
//
// VOID
// KiUserExceptionDispatcher (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PCONTEXT ContextRecord
//    )
//
// Routine Description:
//
//    This routine is entered on return from kernel mode to dispatch a user
//    mode exception. If a frame based handler handles the exception, then
//    the execution is continued. Else last chance processing is performed.
//
// Arguments:
//
//    s0 - Supplies a pointer to an exception record.
//
//    s1 - Supplies a pointer to a context record.
//
// Return Value:
//
//    None.
//
// N.B. preserved register s3 is used to save the current global pointer.
//
//--

        NESTED_ENTRY (KiUserExceptionDispatch)
        ALTERNATE_ENTRY(KiUserExceptionDispatcher)

        .prologue
        .unwabi  @nt,  CONTEXT_FRAME

        alloc       t0 = ar.pfs, 0, 1, 3, 0
        mov         teb = kteb                  // sanitize teb
        mov         s3 = gp                     // save global pointer
        ;;

        PROLOGUE_END

        flushrs                                 // flush the RSE
        ;;
        mov         out0 = s1
        br.call.sptk.many brp = RtlpCaptureRnats
        ;;

        add         t1 = @gprel(Wow64PrepareForException), gp
        ;;
        ld8         t1 = [t1]
        ;;
        cmp.ne      pt1, pt0 = zero, t1         // Wow64PrepareForException != NULL?
        ;;
 (pt1)  ld8         t2 = [t1], PlGlobalPointer - PlEntryPoint
        ;;
 (pt1)  ld8         gp = [t1]
 (pt1)  mov         bt0 = t2
 (pt1)  br.call.spnt.few brp = bt0
        ;;

        mov         gp = s3

        mov         out0 = s0
        mov         out1 = s1
        br.call.sptk.many brp = RtlDispatchException

        cmp4.eq     pt1, pt0 = zero, v0         // result is FALSE ?
        ;;
 (pt1)  mov         out2 = zero
        mov         gp = s3

 (pt0)  add         out0 = 0, s1
 (pt0)  mov         out1 = zero                 // set test alert to FALSE.
 (pt0)  br.call.sptk.many brp = ZwContinue
        ;;

 (pt1)  add         out0 = 0, s0
 (pt1)  mov         out1 = s1
 (pt1)  br.call.sptk.many brp = ZwRaiseException
        ;;

//
// Common code for nonsuccessful completion of the continue or last chance
// processing service.  Use the return status as the exception code, set
// noncontinuable exception and attempt to raise another exception.  Note
// that the stack grows and eventually this loop will end.
//

Kued10:

//
// allocate space for exception record
//

        nop.m       0
        movl        s2 = EXCEPTION_NONCONTINUABLE   // set noncontinuable flag.

        add         sp = -ExceptionRecordLength, sp
        nop.f       0
        mov         gp = s3                     // restore gp
        ;;

        add         out0 = STACK_SCRATCH_AREA, sp   // get except record addr
        add         t2 = ErExceptionFlags+STACK_SCRATCH_AREA, sp
        add         t3 = ErExceptionCode+STACK_SCRATCH_AREA, sp
        ;;

//
// Set exception flags and exception code.
//

        st4         [t2] = s2, ErExceptionRecord - ErExceptionFlags
        st4         [t3] = v0, ErNumberParameters - ErExceptionCode
        ;;

//
// Set exception record and number of parameters.
// Then call RtlRaiseException
//

        st4         [t2] = s0
        st4         [t3] = zero
        br.call.sptk.many brp = RtlRaiseException

        nop.m       0
        nop.m       0
        br          Kued10                      // loop on return

        NESTED_EXIT(KiUserExceptionDispatch)


//++
//
// NTSTATUS
// KiRaiseUserExceptionDispatcher (
//    IN NTSTATUS ExceptionCode
//    )
//
// Routine Description:
//
//    This routine is entered on return from kernel mode to raise a user
//    mode exception.
//
// Arguments:
//
//    v0 - Supplies the status code to be raised.
//
// Return Value:
//
//    ExceptionCode
//
//--

//
// N.B. This function is not called in the typical way. Instead of a normal
// subroutine call to the nested entry point above, the alternate entry point
// address below is stuffed into the Fir address of the trap frame. Thus when
// the kernel returns from the trap, the following code is executed directly.
//


        NESTED_ENTRY(KiRaiseUserExceptionDispatch)

        .prologue
        .savepsp    ar.pfs, -8
        nop.m       0
        .savepsp    rp, 0
        nop.m       0
        nop.i       0
        ;;

        ALTERNATE_ENTRY(KiRaiseUserExceptionDispatcher)

//
// ar.pfs and brp have been saved on the stack in the scratch area.
//

        alloc       t22 = ar.pfs, 0, 1, 1, 0
        ld8.nta     t3 = [sp]
        .fframe     ExceptionRecordLength+STACK_SCRATCH_AREA, tg10
[tg10:] add         sp = -ExceptionRecordLength-STACK_SCRATCH_AREA, sp
        ;;
 
        PROLOGUE_END

        add         t1 = STACK_SCRATCH_AREA+ErExceptionCode, sp
        add         t2 = STACK_SCRATCH_AREA+ErExceptionFlags, sp
        mov         loc0 = v0
        ;;

        //
        // set exception code and exception flags
        //

        st4         [t1] = v0, ErExceptionRecord - ErExceptionCode
        movl        gp = _gp                // setup gp to ntdll's

        st4         [t2] = zero, ErExceptionAddress - ErExceptionFlags
        ;;
        st4         [t1] = zero, ErNumberParameters - ErExceptionRecord
        add         out0 = STACK_SCRATCH_AREA, sp
        ;;

        //
        // set exception record and exception address
        //

        st4         [t1] = zero               // set number of parameters
        STPTR(t2, t3)
        br.call.sptk.many brp = RtlRaiseException

        add         t1 = ExceptionRecordLength+STACK_SCRATCH_AREA, sp
        add         t2 = ExceptionRecordLength+STACK_SCRATCH_AREA+8, sp
        mov         v0 = loc0
        ;;

        ld8.nta     t3 = [t1]
        ld8.nta     t4 = [t2]
        ;;
        mov         brp = t3

        .restore    tg20
[tg20:] add         sp = ExceptionRecordLength, sp
        mov         ar.pfs = t4
        br.ret.sptk.clr brp

        NESTED_EXIT(KiRaiseUserExceptionDispatcher)
