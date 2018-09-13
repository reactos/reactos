//      TITLE("Debug Support Functions")
//++
//
// Copyright (c) 1993  IBM Corporation
//
// Module Name:
//
//    debug.s
//
// Abstract:
//
//    This module implements functions to support debugging NT.  Each
//    function executes a trap r31,r29,r0 instruction with a special value in
//    R31.  The simulator decodes this trap instruction and dispatches to the
//    correct piece of code in the simulator based on the value in R31.  See
//    the simscal.c source file in the simulator source directory.
//
// Author:
//
//    Chuck Bauman 12-Aug-1993
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Initial PowerPC port of NT product1 source 12-Aug-1993
//
//--

#include "ksppc.h"

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

        twi     31,0,DEBUG_STOP_BREAKPOINT

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
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(DbgBreakPointWithStatus)

        ALTERNATE_ENTRY(RtlpBreakWithStatusInstruction)
        twi     31,0,DEBUG_STOP_BREAKPOINT

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

        twi     31,0,DEBUG_STOP_BREAKPOINT

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
//    Output (r.3) - Supplies a pointer to the output string descriptor.
//
//    Input (r.4) - Supplies a pointer to the input string descriptor.
//
// Return Value:
//
//    The length of the input string is returned as the function value.
//
//--

#if DEVL

        LEAF_ENTRY(DebugPrompt)

        lhz     r.6,StrMaximumLength(r.4) // set maximum length of input string
        lwz     r.5,StrBuffer(r.4)        // set address of input string
        lhz     r.4,StrLength(r.3)        // set length of output string
        lwz     r.3,StrBuffer(r.3)        // set address of output string
        twi     31,0,DEBUG_PROMPT_BREAKPOINT  // execute a debug prompt breakpoint

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

        twi     31,0,DEBUG_LOAD_SYMBOLS_BREAKPOINT

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

        twi     31,0,DEBUG_UNLOAD_SYMBOLS_BREAKPOINT

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
//    Output (r.3) - Supplies a pointer to the output string descriptor.
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

        lhz     r.4,StrLength(r.3)      // set length of output string
        lwz     r.3,StrBuffer(r.3)      // set address of output string
        twi     31,0,DEBUG_PRINT_BREAKPOINT  // execute a debug print breakpoint

        LEAF_EXIT(DebugPrint)

#endif
