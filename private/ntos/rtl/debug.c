//      TITLE("Debug Support Functions")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    debug.c
//
// Abstract:
//
//    This module implements functions to support debugging NT.  They call
//    architecture specific routines to do the actual work.
//
// Author:
//
//    Steven R. Wood (stevewo) 8-Nov-1994
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "stdarg.h"
#include "stdio.h"
#include "ntrtlp.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include <ntdbg.h>


ULONG
DbgPrint(
    PCHAR Format,
    ...
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    Format     - printf style format string
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{
    va_list arglist;
    UCHAR Buffer[512];
    int cb;
    STRING Output;
    NTSTATUS Status = STATUS_SUCCESS;
#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)

    if (NtCurrentTeb()->InDbgPrint) {
        return STATUS_SUCCESS;
    }
    NtCurrentTeb()->InDbgPrint = TRUE;
#endif

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);

#if !defined(BLDR_KERNEL_RUNTIME)
    __try {
        cb = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
#else
    cb = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
#endif

    va_end(arglist);

    if (!NT_SUCCESS(Status)) {
#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
        NtCurrentTeb()->InDbgPrint = FALSE;
#endif
        return Status;
    }

    if (cb == -1) {             // detect buffer overflow
        cb = sizeof(Buffer);
        Buffer[sizeof(Buffer) - 1] = '\n';
    }
    Output.Buffer = Buffer;
    Output.Length = (USHORT) cb;

    //
    // If APP is being debugged, raise an exception and the debugger
    // will catch and handle this. Otherwise, kernel debugger service
    // is called.
    //

#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
#if !i386
    //
    // For non-Intel architectures, can't raise exceptions until the PebLock
    // is initialized, since the Function Table lookup code uses the PebLock
    // to serialize access to the loaded module database.  What a crock
    //
    if (NtCurrentPeb()->FastPebLockRoutine != NULL)
#endif  //!i386
    if (NtCurrentPeb()->BeingDebugged) {
        EXCEPTION_RECORD ExceptionRecord;

        //
        // Construct an exception record.
        //

        ExceptionRecord.ExceptionCode = DBG_PRINTEXCEPTION_C;
        ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
        ExceptionRecord.NumberParameters = 2;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[ 0 ] = Output.Length + 1;
        ExceptionRecord.ExceptionInformation[ 1 ] = (ULONG_PTR)(Output.Buffer);
        RtlRaiseException( &ExceptionRecord );
#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
        NtCurrentTeb()->InDbgPrint = FALSE;
#endif
        return STATUS_SUCCESS;
        }
#endif
    Status = DebugPrint( &Output );
    if (Status == STATUS_BREAKPOINT) {
        DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
        Status = STATUS_SUCCESS;
    }
#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
    NtCurrentTeb()->InDbgPrint = FALSE;
#endif
    return Status;
}

ULONG
DbgPrintReturnControlC(
    PCHAR Format,
    ...
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    This routine is exactly the same as DbgPrint except that control-C
//    is NOT handled here.   Instead, status indicating control-C is
//    returned to the caller to do with as they will.
//
// Arguments:
//
//    Format     - printf style format string
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{
    va_list arglist;
    UCHAR Buffer[512];
    int cb;
    STRING Output;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);

    cb = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    if (cb == -1) {             // detect buffer overflow
        cb = sizeof(Buffer);
        Buffer[sizeof(Buffer) - 1] = '\n';
    }
    Output.Buffer = Buffer;
    Output.Length = (USHORT) cb;

    //
    // If APP is being debugged, raise an exception and the debugger
    // will catch and handle this. Otherwise, kernel debugger service
    // is called.
    //

#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
#if !i386
    //
    // For non-Intel architectures, can't raise exceptions until the PebLock
    // is initialized, since the Function Table lookup code uses the PebLock
    // to serialize access to the loaded module database.  What a crock
    //
    if (NtCurrentPeb()->FastPebLockRoutine != NULL)
#endif  //!i386
    if (NtCurrentPeb()->BeingDebugged) {
        EXCEPTION_RECORD ExceptionRecord;

        //
        // Construct an exception record.
        //

        ExceptionRecord.ExceptionCode = DBG_PRINTEXCEPTION_C;
        ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
        ExceptionRecord.NumberParameters = 2;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[ 0 ] = Output.Length + 1;
        ExceptionRecord.ExceptionInformation[ 1 ] = (ULONG_PTR)(Output.Buffer);
        RtlRaiseException( &ExceptionRecord );
        return STATUS_SUCCESS;
        }
#endif
    return DebugPrint( &Output );
}

ULONG
DbgPrompt(
    IN PCHAR Prompt,
    OUT PCHAR Response,
    IN ULONG MaximumResponseLength
    )

//++
//
// Routine Description:
//
//    This function displays the prompt string on the debugging console and
//    then reads a line of text from the debugging console.  The line read
//    is returned in the memory pointed to by the second parameter.  The
//    third parameter specifies the maximum number of characters that can
//    be stored in the response area.
//
// Arguments:
//
//    Prompt - specifies the text to display as the prompt.
//
//    Response - specifies where to store the response read from the
//       debugging console.
//
//    Prompt - specifies the maximum number of characters that can be
//       stored in the Response buffer.
//
// Return Value:
//
//    Number of characters stored in the Response buffer.  Includes the
//    terminating newline character, but not the null character after
//    that.
//
//--

{

    STRING Input;
    STRING Output;

    //
    // Output the prompt string and read input.
    //

    Input.MaximumLength = (USHORT)MaximumResponseLength;
    Input.Buffer = Response;
    Output.Length = (USHORT)strlen( Prompt );
    Output.Buffer = Prompt;
    return DebugPrompt( &Output, &Input );
}

#if defined(NTOS_KERNEL_RUNTIME) || defined(BLDR_KERNEL_RUNTIME)


VOID
DbgLoadImageSymbols(
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG_PTR ProcessId
    )

//++
//
// Routine Description:
//
//    Tells the debugger about newly loaded symbols.
//
// Arguments:
//
// Return Value:
//
//--

{

    PIMAGE_NT_HEADERS NtHeaders;
    KD_SYMBOLS_INFO SymbolInfo;

    SymbolInfo.BaseOfDll = ImageBase;
    SymbolInfo.ProcessId = ProcessId;
    NtHeaders = RtlImageNtHeader( ImageBase );
    if (NtHeaders != NULL) {
        SymbolInfo.CheckSum = (ULONG)NtHeaders->OptionalHeader.CheckSum;
        SymbolInfo.SizeOfImage = (ULONG)NtHeaders->OptionalHeader.SizeOfImage;

    } else {

#if defined(BLDR_KERNEL_RUNTIME)

        //
        // There is only one image loaded in the loader environment that
        // does not have an NT image header. The image is the OS loader
        // and it is loaded by the firmware which strips the file header
        // and the optional ROM header. All the debugger requires is a
        // good guest at the size of the image.
        //

        SymbolInfo.SizeOfImage = 0x100000;

#else

        SymbolInfo.SizeOfImage = 0;

#endif

        SymbolInfo.CheckSum    = 0;
    }

    DebugLoadImageSymbols( FileName, &SymbolInfo);

    return;
}


VOID
DbgUnLoadImageSymbols (
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG_PTR ProcessId
    )

//++
//
// Routine Description:
//
//    Tells the debugger about newly unloaded symbols.
//
// Arguments:
//
// Return Value:
//
//--

{
    KD_SYMBOLS_INFO SymbolInfo;

    SymbolInfo.BaseOfDll = ImageBase;
    SymbolInfo.ProcessId = ProcessId;
    SymbolInfo.CheckSum    = 0;
    SymbolInfo.SizeOfImage = 0;

    DebugUnLoadImageSymbols( FileName, &SymbolInfo );

    return;
}

#endif // defined(NTOS_KERNEL_RUNTIME)
