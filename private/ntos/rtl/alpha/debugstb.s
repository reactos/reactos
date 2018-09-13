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
//    function executes a specific call pal which is interpreted by the
//    kernel debugger.
//
// Author:
//
//    Steven R. Wood (stevewo) 3-Aug-1989
//    Joe Notarangelo  24-Jun-1992
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

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

	BREAK_DEBUG_STOP		// debug stop breakpoint
	ret	zero, (ra)		// return

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
//    Note: The debugger checks the address of the breakpoint instruction
//    against the address RtlpBreakWithStatusInstruction.  If it matches,
//    we have a breakpoint with status.   A breakpoint is normally issued
//    with the break_debug_stop macro which generates two instructions.
//    We can't use the macro here because of the "label on the breakpoint"
//    requirement.
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

        ldil    v0, DEBUG_STOP_BREAKPOINT       // first half of macro.

        ALTERNATE_ENTRY(RtlpBreakWithStatusInstruction)

	call_pal callkd                         // second half of macro.

        ret     zero, (ra)                      // return

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

	BREAK				// issue user breakpoint
	ret	zero, (ra)		// return

        .end    DbgUserBreakPoint

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

	ldwu	a1, StrLength(a0)	// set length of output string
	LDP	a0, StrBuffer(a0)	// set address of output string
	BREAK_DEBUG_PRINT		// execute a debug print breakpoint
	ret	zero, (ra)		// return
        .end    DebugPrint

#endif

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

        ldwu    a3,StrMaximumLength(a1) // set maximum length of input string
        LDP     a2,StrBuffer(a1)        // set address of input string
        ldwu    a1,StrLength(a0)        // set length of output string
        LDP     a0,StrBuffer(a0)        // set address of output string
        BREAK_DEBUG_PROMPT
	ret	zero, (ra)

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

	BREAK_DEBUG_LOAD_SYMBOLS
	ret	zero, (ra)

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

	BREAK_DEBUG_UNLOAD_SYMBOLS
	ret	zero, (ra)

        .end    DebugUnLoadImageSymbols

#endif
