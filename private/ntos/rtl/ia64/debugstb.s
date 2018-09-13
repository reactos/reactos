/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//      TITLE("Debug Support Functions")
//++
//
// Module Name:
//
//    debugstb.s
//
// Abstract:
//
//    This module implements functions to support debugging NT.  Each
//    function executes a break instruction with a special immediate 
//    value.
//
// Author:
//
//    William K. Cheung (wcheung) 17-Jan-1996
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    08-Feb-1996    Updated to EAS 2.1
//
//--

#include "ksia64.h"


//++
//
// VOID
// DbgBreakPoint()
//
// Routine Description:
//
//    This function executes a breakpoint instruction.  Useful for entering
//    the debugger under program control.  This breakpoint will always go to
//    the kernel debugger if one is installed, otherwise it will go to the
//    debug subsystem.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(DbgBreakPoint)

        flushrs
        ;;
        break.i     DEBUG_STOP_BREAKPOINT
        br.ret.sptk.clr brp

        LEAF_EXIT(DbgBreakPoint)

//++
//
// VOID
// DbgBreakPointWithStatus(
//     IN ULONG Status
//     )
//
// Routine Description:
//
//    This function executes a breakpoint instruction.  Useful for entering
//    the debugger under program control.  This breakpoint will always go to
//    the kernel debugger if one is installed, otherwise it will go to the
//    debug subsystem.  This function is identical to DbgBreakPoint, except
//    that it takes an argument which the debugger can see.
//
// Arguments:
//
//    A status code.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(DbgBreakPointWithStatus)

        ALTERNATE_ENTRY(RtlpBreakWithStatusInstruction)

        flushrs
        ;; 
        add         t0 = zero, a0
        break.i     DEBUG_STOP_BREAKPOINT
        br.ret.sptk.clr brp

        LEAF_EXIT(DbgBreakPointWithStatus)

//++
//
// VOID
// DbgUserBreakPoint()
//
// Routine Description:
//
//    This function executes a breakpoint instruction.  Useful for entering
//    the debug subsystem under program control.  The kernel debug will ignore
//    this breakpoint since it will not find the instruction address in its
//    breakpoint table.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(DbgUserBreakPoint)

        flushrs
        ;; 
        break.i     DEBUG_STOP_BREAKPOINT
        br.ret.sptk.clr brp

        LEAF_EXIT(DbgUserBreakPoint)

//++
//
// ULONG
// DebugPrompt(
//     IN PSTRING Output,
//     IN PSTRING Input
//     )
//
// Routine Description:
//
//    This function executes a debug prompt breakpoint.
//
// Arguments:
//
//    Output (a0) - Supplies a pointer to the output string descriptor.
//
//    Input (a1) - Supplies a pointer to the input string descriptor.
//
// Return Value:
//
//    The length of the input string is returned as the function value.
//
//--

#if DEVL

        LEAF_ENTRY(DebugPrompt)
        
        flushrs
        ARGPTR(a0)
        ARGPTR(a1)
        add         a0 = StrBuffer, a0
        add         a1 = StrBuffer, a1
        ;;

//
// Set the following 4 arguments into scratch registers t0 - t3;
// they are passed to the KiDebugRoutine() via the context record.
//
// t0 - address of output string
// t1 - length of output string
// t2 - address of input string
// t3 - maximumm length of input string
//

        LDPTRINC(t0, a0, StrLength - StrBuffer)
        LDPTRINC(t2, a1, StrMaximumLength - StrBuffer)
        nop.i       0
        ;;

        ld2.nta     t1 = [a0]
        ld2.nta     t3 = [a1]
        break.i     DEBUG_PROMPT_BREAKPOINT

        nop.m       0
        nop.m       0
        br.ret.sptk.clr brp

        LEAF_EXIT(DebugPrompt)

#endif


//++
//
// VOID
// DebugLoadImageSymbols(
//     IN PSTRING ImagePathName,
//     IN PKD_SYMBOLS_INFO SymbolInfo
//     )
//
// Routine Description:
//
//    This function calls the kernel debugger to load the symbol
//    table for the specified image.
//
// Arguments:
//
//    ImagePathName - specifies the fully qualified path name of the image
//       file that has been loaded into an NT address space.
//
//    SymbolInfo - information captured from header of image file.
//
// Return Value:
//
//    None.
//
//--

#if DEVL

        LEAF_ENTRY(DebugLoadImageSymbols)

//
// Arguments are passed to the KiDebugRoutine via the context record
// in scratch registers t0 and t1.
//

        flushrs
        ;;
        add         t0 = zero, a0
        add         t1 = zero, a1

        nop.m       0
        break.i     DEBUG_LOAD_SYMBOLS_BREAKPOINT
        br.ret.sptk.clr brp

        LEAF_EXIT(DebugLoadImageSymbols)

#endif

//++
//
// VOID
// DebugUnLoadImageSymbols(
//     IN PSTRING ImagePathName,
//     IN PKD_SYMBOLS_INFO SymbolInfo
//     )
//
// Routine Description:
//
//    This function calls the kernel debugger to unload the symbol
//    table for the specified image.
//
// Arguments:
//
//    ImagePathName - specifies the fully qualified path name of the image
//       file that has been unloaded from an NT address space.
//
//    SymbolInfo - information captured from header of image file.
//
// Return Value:
//
//    None.
//
//--

#if DEVL

        LEAF_ENTRY(DebugUnLoadImageSymbols)

//
// Arguments are passed to the KiDebugRoutine via the context record
// in scratch registers t0 and t1.
//

        flushrs
        ;;
        add         t0 = zero, a0
        add         t1 = zero, a1

        nop.m       0
        break.i     DEBUG_UNLOAD_SYMBOLS_BREAKPOINT
        br.ret.sptk.clr brp

        LEAF_EXIT(DebugUnLoadImageSymbols)

#endif

//++
//
// NTSTATUS
// DebugPrint(
//     IN PSTRING Output
//     )
//
// Routine Description:
//
//    This function executes a debug print breakpoint.
//
// Arguments:
//
//    Output (a0) - Supplies a pointer to the output string descriptor.
//
// Return Value:
//
//    Status code.  STATUS_SUCCESS if debug print happened.
//    STATUS_BREAKPOINT if user typed a Control-C during print.
//    STATUS_DEVICE_NOT_CONNECTED if kernel debugger not present.
//
//--

#if DEVL

        LEAF_ENTRY(DebugPrint)

        flushrs
        ARGPTR(a0)
        add         t5 = StrBuffer, a0
        ;;

        LDPTRINC(t0, t5, StrLength-StrBuffer)    // set address of output string
        ;;
        ld2.nta     t1 = [t5]               // set length of output string
        break.i     DEBUG_PRINT_BREAKPOINT  // execute a debug print breakpoint
        br.ret.sptk.clr brp

        LEAF_EXIT(DebugPrint)

#endif
