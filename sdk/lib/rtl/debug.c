/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Run-Time Library
 * FILE:            lib/rtl/debug.c
 * PURPOSE:         Debug Print and Prompt routines
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Royce Mitchel III
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#include <ndk/kdfuncs.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS ********************************************************/

ULONG
NTAPI
DebugPrint(IN PSTRING DebugString,
           IN ULONG ComponentId,
           IN ULONG Level)
{
    /* Call the Debug Service */
    return DebugService(BREAKPOINT_PRINT,
                        DebugString->Buffer,
                        UlongToPtr(DebugString->Length),
                        UlongToPtr(ComponentId),
                        UlongToPtr(Level));
}

ULONG
NTAPI
DebugPrompt(IN PSTRING Output,
            IN PSTRING Input)
{
    /* Call the Debug Service */
    return DebugService(BREAKPOINT_PROMPT,
                        Output->Buffer,
                        UlongToPtr(Output->Length),
                        Input->Buffer,
                        UlongToPtr(Input->MaximumLength));
}

/* FUNCTIONS ****************************************************************/

ULONG
NTAPI
vDbgPrintExWithPrefixInternal(IN PCCH Prefix,
                              IN ULONG ComponentId,
                              IN ULONG Level,
                              IN PCCH Format,
                              IN va_list ap,
                              IN BOOLEAN HandleBreakpoint)
{
    NTSTATUS Status;
    STRING DebugString;
    CHAR Buffer[512];
    SIZE_T Length, PrefixLength;
    EXCEPTION_RECORD ExceptionRecord;

    /* Check if we should print it or not */
    if ((ComponentId != MAXULONG) &&
        (NtQueryDebugFilterState(ComponentId, Level)) != (NTSTATUS)TRUE)
    {
        /* This message is masked */
        return STATUS_SUCCESS;
    }

    /* For user mode, don't recursively DbgPrint */
    if (RtlpSetInDbgPrint()) return STATUS_SUCCESS;

    /* Guard against incorrect pointers */
    _SEH2_TRY
    {
        /* Get the length and normalize it */
        PrefixLength = strlen(Prefix);
        if (PrefixLength > sizeof(Buffer)) PrefixLength = sizeof(Buffer);

        /* Copy it */
        strncpy(Buffer, Prefix, PrefixLength);

        /* Do the printf */
        Length = _vsnprintf(Buffer + PrefixLength,
                            sizeof(Buffer) - PrefixLength,
                            Format,
                            ap);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* In user-mode, clear the InDbgPrint Flag */
        RtlpClearInDbgPrint();
        /* Fail */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check if we went past the buffer */
    if (Length == MAXULONG)
    {
        /* Terminate it if we went over-board */
        Buffer[sizeof(Buffer) - 2] = '\n';
        Buffer[sizeof(Buffer) - 1] = '\0';

        /* Put maximum */
        Length = sizeof(Buffer) - 1;
    }
    else
    {
        /* Add the prefix */
        Length += PrefixLength;
    }

    /* Build the string */
    DebugString.Length = (USHORT)Length;
    DebugString.Buffer = Buffer;

    /* First, let the debugger know as well */
    if (RtlpCheckForActiveDebugger())
    {
        /* Fill out an exception record */
        ExceptionRecord.ExceptionCode = DBG_PRINTEXCEPTION_C;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.NumberParameters = 2;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[0] = DebugString.Length + 1;
        ExceptionRecord.ExceptionInformation[1] = (ULONG_PTR)DebugString.Buffer;

        /* Raise the exception */
        RtlRaiseException(&ExceptionRecord);

        /* In user-mode, clear the InDbgPrint Flag */
        RtlpClearInDbgPrint();
        return STATUS_SUCCESS;
    }

    /* Call the Debug Print routine */
    Status = DebugPrint(&DebugString, ComponentId, Level);

    /* Check if this was with Control-C */
    if (HandleBreakpoint)
    {
        /* Check if we got a breakpoint */
        if (Status == STATUS_BREAKPOINT)
        {
            /* Breakpoint */
            DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
            Status = STATUS_SUCCESS;
        }
    }

    /* In user-mode, clear the InDbgPrint Flag */
    RtlpClearInDbgPrint();

    /* Return */
    return Status;
}

/*
 * @implemented
 */
ULONG
NTAPI
vDbgPrintExWithPrefix(IN PCCH Prefix,
                      IN ULONG ComponentId,
                      IN ULONG Level,
                      IN PCCH Format,
                      IN va_list ap)
{
    /* Call the internal routine that also handles ControlC */
    return vDbgPrintExWithPrefixInternal(Prefix,
                                         ComponentId,
                                         Level,
                                         Format,
                                         ap,
                                         TRUE);
}

/*
 * @implemented
 */
ULONG
NTAPI
vDbgPrintEx(IN ULONG ComponentId,
            IN ULONG Level,
            IN PCCH Format,
            IN va_list ap)
{
    /* Call the internal routine that also handles ControlC */
    return vDbgPrintExWithPrefixInternal("",
                                         ComponentId,
                                         Level,
                                         Format,
                                         ap,
                                         TRUE);
}

/*
 * @implemented
 */
ULONG
__cdecl
DbgPrint(PCCH Format,
         ...)
{
    ULONG Status;
    va_list ap;

    /* Call the internal routine that also handles ControlC */
    va_start(ap, Format);
    Status = vDbgPrintExWithPrefixInternal("",
                                           -1,
                                           DPFLTR_ERROR_LEVEL,
                                           Format,
                                           ap,
                                           TRUE);
    va_end(ap);
    return Status;
}

/*
 * @implemented
 */
ULONG
__cdecl
DbgPrintEx(IN ULONG ComponentId,
           IN ULONG Level,
           IN PCCH Format,
           ...)
{
    ULONG Status;
    va_list ap;

    /* Call the internal routine that also handles ControlC */
    va_start(ap, Format);
    Status = vDbgPrintExWithPrefixInternal("",
                                           ComponentId,
                                           Level,
                                           Format,
                                           ap,
                                           TRUE);
    va_end(ap);
    return Status;
}

/*
 * @implemented
 */
ULONG
__cdecl
DbgPrintReturnControlC(PCCH Format,
                       ...)
{
    ULONG Status;
    va_list ap;

    /* Call the internal routine that also handles ControlC */
    va_start(ap, Format);
    Status = vDbgPrintExWithPrefixInternal("",
                                           -1,
                                           DPFLTR_ERROR_LEVEL,
                                           Format,
                                           ap,
                                           FALSE);
    va_end(ap);
    return Status;
}

/*
 * @implemented
 */
ULONG
NTAPI
DbgPrompt(IN PCCH Prompt,
          OUT PCH Response,
          IN ULONG MaximumResponseLength)
{
    STRING Output;
    STRING Input;

    /* Setup the input string */
    Input.MaximumLength = (USHORT)MaximumResponseLength;
    Input.Buffer = Response;

    /* Setup the output string */
    Output.Length = (USHORT)strlen(Prompt);
    Output.Buffer = (PCHAR)Prompt;

    /* Call the system service */
    return DebugPrompt(&Output, &Input);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgQueryDebugFilterState(IN ULONG ComponentId,
                         IN ULONG Level)
{
    /* Call the Nt routine */
    return NtQueryDebugFilterState(ComponentId, Level);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DbgSetDebugFilterState(IN ULONG ComponentId,
                       IN ULONG Level,
                       IN BOOLEAN State)
{
    /* Call the Nt routine */
    return NtSetDebugFilterState(ComponentId, Level, State);
}

/*
 * @implemented
 */
VOID
NTAPI
DbgLoadImageSymbols(IN PSTRING Name,
                    IN PVOID Base,
                    IN ULONG_PTR ProcessId)
{
    PIMAGE_NT_HEADERS NtHeader;
    KD_SYMBOLS_INFO SymbolInfo;

    /* Setup the symbol data */
    SymbolInfo.BaseOfDll = Base;
    SymbolInfo.ProcessId = ProcessId;

    /* Get NT Headers */
    NtHeader = RtlImageNtHeader(Base);
    if (NtHeader)
    {
        /* Get the rest of the data */
        SymbolInfo.CheckSum = NtHeader->OptionalHeader.CheckSum;
        SymbolInfo.SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
    }
    else
    {
        /* No data available */
        SymbolInfo.CheckSum =
        SymbolInfo.SizeOfImage = 0;
    }

    /* Load the symbols */
    DebugService2(Name, &SymbolInfo, BREAKPOINT_LOAD_SYMBOLS);
}

/*
 * @implemented
 */
VOID
NTAPI
DbgUnLoadImageSymbols(IN PSTRING Name,
                      IN PVOID Base,
                      IN ULONG_PTR ProcessId)
{
    KD_SYMBOLS_INFO SymbolInfo;

    /* Setup the symbol data */
    SymbolInfo.BaseOfDll = Base;
    SymbolInfo.ProcessId = ProcessId;
    SymbolInfo.CheckSum = SymbolInfo.SizeOfImage = 0;

    /* Load the symbols */
    DebugService2(Name, &SymbolInfo, BREAKPOINT_UNLOAD_SYMBOLS);
}

/*
 * @implemented
 */
VOID
NTAPI
DbgCommandString(IN PCCH Name,
                 IN PCCH Command)
{
    STRING NameString, CommandString;

    /* Setup the strings */
    NameString.Buffer = (PCHAR)Name;
    NameString.Length = (USHORT)strlen(Name);
    CommandString.Buffer = (PCHAR)Command;
    CommandString.Length = (USHORT)strlen(Command);

    /* Send them to the debugger */
    DebugService2(&NameString, &CommandString, BREAKPOINT_COMMAND_STRING);
}

/*
* @implemented
*/
VOID
NTAPI
RtlPopFrame(IN PTEB_ACTIVE_FRAME Frame)
{
    /* Restore the previous frame as the active one */
    NtCurrentTeb()->ActiveFrame = Frame->Previous;
}

/*
* @implemented
*/
VOID
NTAPI
RtlPushFrame(IN PTEB_ACTIVE_FRAME Frame)
{
    /* Save the current frame and set the new one as active */
    Frame->Previous = NtCurrentTeb()->ActiveFrame;
    NtCurrentTeb()->ActiveFrame = Frame;
}

PTEB_ACTIVE_FRAME
NTAPI
RtlGetFrame(VOID)
{
    /* Return the frame that's currently active */
    return NtCurrentTeb()->ActiveFrame;
}
