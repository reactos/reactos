/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    exceptn.cxx

Abstract:

    Contains exception-handling code for debug version

    Contents:
        SetExceptionHandler
        (WininetExceptionFilter)
        (MapX86ProcessorFlags)

Author:

    Richard L Firth (rfirth) 18-Feb-1997

Revision History:

    18-Feb-1997 rfirth
        Created

--*/

#include <wininetp.h>
#include "rprintf.h"

#if INET_DEBUG

//
// private prototypes
//

PRIVATE
LONG
WininetExceptionFilter(
    IN PEXCEPTION_POINTERS pExPtrs
    );

#if defined(_X86_)

PRIVATE
LPSTR
MapX86ProcessorFlags(
    IN DWORD Flags
    );

#endif // defined(_X86_)

//
// functions
//


VOID
SetExceptionHandler(
    VOID
    )

/*++

Routine Description:

    Just sets the unhandled exception filter for this process

Arguments:

    None.

Return Value:

    None.

--*/

{
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)WininetExceptionFilter);
}


PRIVATE
LONG
WininetExceptionFilter(
    IN PEXCEPTION_POINTERS pExPtrs
    )

/*++

Routine Description:

    We get to look at unhandled exceptions, and dump them to the debug log

Arguments:

    pExPtrs - pointer to exception pointers structure

Return Value:

    LONG

--*/

{
    //
    // don't bother if we are not logging
    //

    if (InternetDebugControlFlags & DBG_NO_DEBUG) {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    LPSTR text;
    LONG disposition = EXCEPTION_EXECUTE_HANDLER;
    DWORD eipOffset = 0;

    switch (pExPtrs->ExceptionRecord->ExceptionCode) {
    case EXCEPTION_ACCESS_VIOLATION:
        text = "Access Violation";
        break;

    case EXCEPTION_DATATYPE_MISALIGNMENT:
        text = "Data Misalignment Exception";
        break;

    case EXCEPTION_BREAKPOINT:
        text = "Breakpoint Exception";
        disposition = EXCEPTION_CONTINUE_EXECUTION;
        eipOffset = 1;
        break;

    case EXCEPTION_SINGLE_STEP:
        text = "Single Step Exception";
        break;

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        text = "Array Bounds Exceeded Exception";
        break;

    case EXCEPTION_FLT_DENORMAL_OPERAND:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INEXACT_RESULT:
    case EXCEPTION_FLT_INVALID_OPERATION:
    case EXCEPTION_FLT_OVERFLOW:
    case EXCEPTION_FLT_STACK_CHECK:
    case EXCEPTION_FLT_UNDERFLOW:
        text = "Floating Point Exception";
        break;

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        text = "Integer Divide-By-Zero Exception";
        break;

    case EXCEPTION_INT_OVERFLOW:
        text = "Integer Overflow Exception";
        break;

    case EXCEPTION_PRIV_INSTRUCTION:
        text = "Privileged Instruction Exception";
        break;

    case EXCEPTION_IN_PAGE_ERROR:
        text = "In-Page Error";
        break;

    case EXCEPTION_ILLEGAL_INSTRUCTION:
        text = "Illegal Instruction";
        break;

    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        text = "Non-Continuable Exception";
        break;

    case EXCEPTION_STACK_OVERFLOW:
        text = "Stack Overflow";
        break;

    case EXCEPTION_INVALID_DISPOSITION:
        text = "Invalid Disposition Exception";
        break;

    case EXCEPTION_GUARD_PAGE:
        text = "Guard Page Exception";
        break;

    case EXCEPTION_INVALID_HANDLE:
        text = "Invalid Handle Exception";
        break;

    case CONTROL_C_EXIT:
        text = "Control-C Exception";
        break;

    default:
        text = "Unknown Exception";
        break;
    }

    InitSymLib();

    
    DWORD dwCodeOffset;
    // BUGBUG: Not 64b compatible
    LPSTR lpszDebugSymbol = GetDebugSymbol(PtrToUlong(pExPtrs->ExceptionRecord->ExceptionAddress),
                                           &dwCodeOffset
                                           );

    char buffer[512];
    int offset;
    BOOL needCrLf = FALSE;

    offset = rsprintf(buffer,
                      "\n"
                      "********************************************************************************\n"
                      "Thread %#x\n"
                      "%s at %#08x",
                      GetCurrentThreadId(),
                      text,
                      pExPtrs->ExceptionRecord->ExceptionAddress
                      );
    if (dwCodeOffset != (DWORD_PTR)pExPtrs->ExceptionRecord->ExceptionAddress) {
        offset += rsprintf(&buffer[offset],
                           " (%s+%#x)\n",
                           lpszDebugSymbol,
                           dwCodeOffset
                           );
    } else {
        buffer[offset++] = ' ';
        needCrLf = TRUE;
    }
    if (pExPtrs->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        rsprintf(&buffer[offset],
                 "%sing %#08x\n",
                 pExPtrs->ExceptionRecord->ExceptionInformation[0]
                    ? "writ"
                    : "read",
                 pExPtrs->ExceptionRecord->ExceptionInformation[1]
                 );
    } else if (needCrLf) {
        buffer[offset++] = '\r';
        buffer[offset++] = '\n';
        buffer[offset] = '\0';
    }
    InternetDebugOut(buffer, FALSE);

#if defined(_X86_)

    if ((pExPtrs->ContextRecord->ContextFlags & CONTEXT_FULL) == CONTEXT_FULL) {
        rsprintf(buffer,
                 "\n"
                 "Processor Context:\n"
                 "eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n"
                 "eip=%08x esp=%08x ebp=%08x iopl=%d         %s\n"
                 "cs=%04x ss=%04x ds=%04x es=%04x fs=%04x gs=%04x                  efl=%08x\n",
                 pExPtrs->ContextRecord->Eax,
                 pExPtrs->ContextRecord->Ebx,
                 pExPtrs->ContextRecord->Ecx,
                 pExPtrs->ContextRecord->Edx,
                 pExPtrs->ContextRecord->Esi,
                 pExPtrs->ContextRecord->Edi,
                 pExPtrs->ContextRecord->Eip,
                 pExPtrs->ContextRecord->Esp,
                 pExPtrs->ContextRecord->Ebp,
                 ((pExPtrs->ContextRecord->EFlags & 0x00003000) >> 12),
                 MapX86ProcessorFlags(pExPtrs->ContextRecord->EFlags),
                 pExPtrs->ContextRecord->SegCs,
                 pExPtrs->ContextRecord->SegSs,
                 pExPtrs->ContextRecord->SegDs,
                 pExPtrs->ContextRecord->SegEs,
                 pExPtrs->ContextRecord->SegFs,
                 pExPtrs->ContextRecord->SegGs,
                 pExPtrs->ContextRecord->EFlags
                 );
        InternetDebugOut(buffer, FALSE);
    }

    //
    // dump out the stack, debug style
    //

    LPBYTE Address = (LPBYTE)pExPtrs->ContextRecord->Esp;

    rsprintf(buffer,
             "\n"
             "256 bytes of process stack at %04x:%08x:\n\n",
             pExPtrs->ContextRecord->SegSs,
             Address
             );
    InternetDebugOut(buffer, FALSE);

    for (DWORD Size = 256; Size; ) {

        DWORD nDumped = InternetDebugDumpFormat(Address, 16, sizeof(DWORD), buffer);

        InternetDebugOut(buffer, FALSE);
        Size -= nDumped;
        Address += nDumped;
    }

    //
    // dump call stack
    //

    LPVOID backtrace[16];

    memset(&backtrace, 0, sizeof(backtrace));

    x86SleazeCallStack((LPVOID *)backtrace,
                       ARRAY_ELEMENTS(backtrace),
                       (LPVOID *)pExPtrs->ContextRecord->Ebp
                       );

    BOOL ok = FALSE;

    for (int i = 0; i < ARRAY_ELEMENTS(backtrace); ++i) {
        if (backtrace[i] != NULL) {
            ok = TRUE;
            break;
        }
    }
    if (ok) {
        rsprintf(buffer,
                 "\n"
                 "Stack back-trace:\n\n"
                 );
        InternetDebugOut(buffer, FALSE);
        for (int i = 0; i < ARRAY_ELEMENTS(backtrace); ++i) {
            if (backtrace[i] == NULL) {
                break;
            }
            lpszDebugSymbol = GetDebugSymbol((DWORD)backtrace[i], &dwCodeOffset);
            rsprintf(buffer,
                     "%08x %s+%#x\n",
                     backtrace[i],
                     lpszDebugSymbol,
                     dwCodeOffset
                     );
            InternetDebugOut(buffer, FALSE);
        }
    }

#endif // defined(_X86_)

    InternetDebugOut("\r\n********************************************************************************\r\n\r\n", FALSE);

    InternetFlushDebugFile();

#if defined(_X86_)

    if (disposition == EXCEPTION_CONTINUE_EXECUTION) {
        pExPtrs->ContextRecord->Eip += eipOffset;
    }

#endif // defined(_X86_)

    return disposition;
}

#if defined(_X86_)


PRIVATE
LPSTR
MapX86ProcessorFlags(
    IN DWORD Flags
    )
{
    //
    // BUGBUG - not re-entrant
    //

    static char buf[32 * 3 + 1];

    rsprintf(buf,
             "%s %s %s %s %s %s %s %s",
             (Flags & 0x00000800) ? "ov" : "nv",    // Overflow:    Overflow or No-overflow
             (Flags & 0x00000400) ? "dn" : "up",    // Direction:   Up or Down
             (Flags & 0x00000200) ? "ei" : "di",    // Interrupts:  Enabled or Disabled
             (Flags & 0x00000080) ? "ng" : "pl",    // Sign:        Negative or Positive
             (Flags & 0x00000040) ? "zr" : "nz",    // Zero:        Zero or Not-zero
             (Flags & 0x00000010) ? "ac" : "na",    // Aux-Carry:   Aux-carry or No-aux-carry
             (Flags & 0x00000004) ? "pe" : "po",    // Parity:      Parity-even or Parity-odd
             (Flags & 0x00000001) ? "cy" : "nc"     // Carry:       Carry or No-carry
             );
    return buf;
}

#endif // defined(_X86_)

#endif // INET_DEBUG
