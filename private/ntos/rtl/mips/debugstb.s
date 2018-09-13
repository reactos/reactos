//      TITLE("Debug Support Functions")
//++
//
// Copyright (c) 1990  Microsoft Corporation
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
//    Steven R. Wood (stevewo) 3-Aug-1989
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksmips.h"

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

        break   DEBUG_STOP_BREAKPOINT
        j       ra

        .end    DbgBreakPoint

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
        break   DEBUG_STOP_BREAKPOINT
        j       ra

        .end    DbgBreakPointWithStatus

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

        break   DEBUG_STOP_BREAKPOINT
        j       ra

        .end    DbgUserBreakPoint

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

        lhu     a3,StrMaximumLength(a1) // set maximum length of input string
        lw      a2,StrBuffer(a1)        // set address of input string
        lhu     a1,StrLength(a0)        // set length of output string
        lw      a0,StrBuffer(a0)        // set address of output string
        break   DEBUG_PROMPT_BREAKPOINT  // execute a debug prompt breakpoint
        j       ra                      // return

        .end    DebugPrompt

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

        break   DEBUG_LOAD_SYMBOLS_BREAKPOINT
        j       ra

        .end    DebugLoadImageSymbols

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

        break   DEBUG_UNLOAD_SYMBOLS_BREAKPOINT
        j       ra

        .end    DebugUnLoadImageSymbols

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

        lhu     a1,StrLength(a0)        // set length of output string
        lw      a0,StrBuffer(a0)        // set address of output string
        break   DEBUG_PRINT_BREAKPOINT  // execute a debug print breakpoint
        j       ra                      // return

        .end    DebugPrint

#endif
