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

#if DBG && defined(_M_IX86) && defined(__GNUC__)
#define RTL_RAW_COM1_BASE 0x3F8
#define RTL_RAW_COM1_LINE_STATUS 5
#define RTL_RAW_COM1_TRANSMIT_EMPTY 0x20

static
UCHAR
RtlRawCom1ReadPortUchar(
    _In_ USHORT Port)
{
    UCHAR Value;

    __asm__ __volatile__("inb %w1, %0" : "=a"(Value) : "Nd"(Port));
    return Value;
}

static
VOID
RtlRawCom1WritePortUchar(
    _In_ USHORT Port,
    _In_ UCHAR Value)
{
    __asm__ __volatile__("outb %0, %w1" : : "a"(Value), "Nd"(Port));
}

static
VOID
RtlRawCom1WriteByte(
    _In_ UCHAR Character)
{
    ULONG SpinCount = 100000;

    while (SpinCount-- != 0)
    {
        if (RtlRawCom1ReadPortUchar(RTL_RAW_COM1_BASE +
                                    RTL_RAW_COM1_LINE_STATUS) &
            RTL_RAW_COM1_TRANSMIT_EMPTY)
            break;
    }

    RtlRawCom1WritePortUchar(RTL_RAW_COM1_BASE, Character);
}

static
VOID
RtlRawCom1WriteString(
    _In_z_ const CHAR *String)
{
    while (*String != ANSI_NULL)
    {
        if (*String == '\n')
            RtlRawCom1WriteByte('\r');

        RtlRawCom1WriteByte(*String++);
    }
}

static
VOID
RtlRawCom1WriteAnsiString(
    _In_opt_ PSTRING String)
{
    USHORT Index;

    if ((String == NULL) || (String->Buffer == NULL))
        return;

    for (Index = 0; Index < String->Length; Index++)
    {
        CHAR Character = String->Buffer[Index];
        RtlRawCom1WriteByte((Character >= 0x20 && Character < 0x7f) ?
                            (UCHAR)Character : (UCHAR)'?');
    }
}

static
VOID
RtlRawCom1WriteHex(
    _In_ ULONG_PTR Value)
{
    ULONG Index;

    for (Index = 0; Index < 8; Index++)
    {
        ULONG Nibble = (Value >> (28 - Index * 4)) & 0xF;

        RtlRawCom1WriteByte((UCHAR)(Nibble < 10 ? ('0' + Nibble) :
                                               ('A' + Nibble - 10)));
    }
}

static
VOID
RtlRawCom1WriteField(
    _In_z_ const CHAR *Name,
    _In_ ULONG_PTR Value)
{
    RtlRawCom1WriteByte(' ');
    RtlRawCom1WriteString(Name);
    RtlRawCom1WriteByte('=');
    RtlRawCom1WriteHex(Value);
}

static
VOID
RtlRawCom1DumpSymbolStage(
    _In_ ULONG Stage,
    _In_opt_ PSTRING Name,
    _In_opt_ PVOID Base,
    _In_ ULONG_PTR Detail)
{
    RtlRawCom1WriteString("\nDbgSym");
    RtlRawCom1WriteField("stage", Stage);
    RtlRawCom1WriteField("base", (ULONG_PTR)Base);
    RtlRawCom1WriteField("detail", Detail);
    RtlRawCom1WriteString(" name=");
    RtlRawCom1WriteAnsiString(Name);
    RtlRawCom1WriteByte('\n');
}
#endif

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

#if DBG && defined(_M_IX86) && defined(__GNUC__)
    RtlRawCom1DumpSymbolStage(0xD201, Name, Base, ProcessId);
#endif

    /* Setup the symbol data */
    SymbolInfo.BaseOfDll = Base;
    SymbolInfo.ProcessId = ProcessId;
#if DBG && defined(_M_IX86) && defined(__GNUC__)
    RtlRawCom1DumpSymbolStage(0xD202, Name, SymbolInfo.BaseOfDll, SymbolInfo.ProcessId);
#endif

    /* Get NT Headers */
#if DBG && defined(_M_IX86) && defined(__GNUC__)
    RtlRawCom1DumpSymbolStage(0xD203, Name, Base, 0);
#endif
    NtHeader = RtlImageNtHeader(Base);
#if DBG && defined(_M_IX86) && defined(__GNUC__)
    RtlRawCom1DumpSymbolStage(0xD204, Name, Base, (ULONG_PTR)NtHeader);
#endif
    if (NtHeader)
    {
        /* Get the rest of the data */
        SymbolInfo.CheckSum = NtHeader->OptionalHeader.CheckSum;
        SymbolInfo.SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
#if DBG && defined(_M_IX86) && defined(__GNUC__)
        RtlRawCom1DumpSymbolStage(0xD205, Name, Base, SymbolInfo.SizeOfImage);
#endif
    }
    else
    {
        /* No data available */
        SymbolInfo.CheckSum =
        SymbolInfo.SizeOfImage = 0;
#if DBG && defined(_M_IX86) && defined(__GNUC__)
        RtlRawCom1DumpSymbolStage(0xD206, Name, Base, 0);
#endif
    }

    /* Load the symbols */
#if DBG && defined(_M_IX86) && defined(__GNUC__)
    RtlRawCom1DumpSymbolStage(0xD207, Name, Base, (ULONG_PTR)&SymbolInfo);
#endif
    DebugService2(Name, &SymbolInfo, BREAKPOINT_LOAD_SYMBOLS);
#if DBG && defined(_M_IX86) && defined(__GNUC__)
    RtlRawCom1DumpSymbolStage(0xD208, Name, Base, SymbolInfo.SizeOfImage);
#endif
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
