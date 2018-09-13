/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    constubs.c

Abstract:

Author:

    KazuM Mar.05.1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#if defined(FE_SB)
ULONG
SrvGetConsoleCharType(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine check character type.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    NTSTATUS Status;
    PCONSOLE_CHAR_TYPE_MSG a = (PCONSOLE_CHAR_TYPE_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PSCREEN_INFORMATION ScreenInfo;
    PHANDLE_DATA HandleData;
    SHORT RowIndex;
    PROW Row;
    PWCHAR Char;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->Handle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {

        ScreenInfo = HandleData->Buffer.ScreenBuffer;

#if defined(DBG) && defined(DBG_KATTR)
        BeginKAttrCheck(ScreenInfo);
#endif

        if (a->coordCheck.X >= ScreenInfo->ScreenBufferSize.X ||
            a->coordCheck.Y >= ScreenInfo->ScreenBufferSize.Y) {
            Status = STATUS_INVALID_PARAMETER;
        }
        else {
            RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+a->coordCheck.Y) % ScreenInfo->ScreenBufferSize.Y;
            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
            Char = &Row->CharRow.Chars[a->coordCheck.X];
            if (!CONSOLE_IS_DBCS_OUTPUTCP(ScreenInfo->Console))
                a->dwType = CHAR_TYPE_SBCS;
            else if (Row->CharRow.KAttrs[a->coordCheck.X] & ATTR_TRAILING_BYTE)
                a->dwType = CHAR_TYPE_TRAILING;
            else if (Row->CharRow.KAttrs[a->coordCheck.X] & ATTR_LEADING_BYTE)
                a->dwType = CHAR_TYPE_LEADING;
            else
                a->dwType = CHAR_TYPE_SBCS;
        }
    }

    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleLocalEUDC(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine sets Local EUDC Font.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    PCONSOLE_LOCAL_EUDC_MSG a = (PCONSOLE_LOCAL_EUDC_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    CHAR Source[4];
    WCHAR Target[2];

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->Handle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        UnlockConsole(Console);
        return((ULONG) Status);
    }

    if (!CsrValidateMessageBuffer(m, &a->FontFace, ((a->FontSize.X + 7) / 8), a->FontSize.Y)) {
        UnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    Source[0] = (char)(a->CodePoint >> 8) ;
    Source[1] = (char)(a->CodePoint & 0x00ff) ;
    Source[2] = 0 ;
    ConvertOutputToUnicode(Console->OutputCP,Source,2,Target,1);

    if (IsEudcRange(Console,Target[0]))
    {
        Status = RegisterLocalEUDC(Console,Target[0],a->FontSize,a->FontFace);
        if (NT_SUCCESS(Status)) {
            ((PEUDC_INFORMATION)(Console->EudcInformation))->LocalVDMEudcMode = TRUE;
        }
    }
    else {
        UnlockConsole(Console);
        return (ULONG)STATUS_INVALID_PARAMETER;
    }

    UnlockConsole(Console);
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleCursorMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine sets Cursor Mode.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    PCONSOLE_CURSOR_MODE_MSG a = (PCONSOLE_CURSOR_MODE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->Handle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        UnlockConsole(Console);
        return((ULONG) Status);
    }

    HandleData->Buffer.ScreenBuffer->BufferInfo.TextInfo.CursorBlink = (BOOLEAN)a->Blink ;
    HandleData->Buffer.ScreenBuffer->BufferInfo.TextInfo.CursorDBEnable = (BOOLEAN)a->DBEnable ;

    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


ULONG
SrvGetConsoleCursorMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine gets Cursor Mode.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    PCONSOLE_CURSOR_MODE_MSG a = (PCONSOLE_CURSOR_MODE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->Handle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        UnlockConsole(Console);
        return((ULONG) Status);
    }

    a->Blink = HandleData->Buffer.ScreenBuffer->BufferInfo.TextInfo.CursorBlink ;
    a->DBEnable = HandleData->Buffer.ScreenBuffer->BufferInfo.TextInfo.CursorDBEnable ;

    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvRegisterConsoleOS2(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

  This function calls NEC PC-98 machine's only.

--*/

{
    PCONSOLE_REGISTEROS2_MSG a = (PCONSOLE_REGISTEROS2_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    HANDLE hEvent = NULL;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
#if defined(i386)
    {
        if (!a->fOs2Register) {
            Console->Flags &= ~ CONSOLE_OS2_REGISTERED;
            ResizeWindow(Console->CurrentScreenBuffer, &Console->Os2SavedWindowRect, FALSE);
        }
        else {
            Console->Flags |= CONSOLE_OS2_REGISTERED;
            Console->Os2SavedWindowRect = Console->CurrentScreenBuffer->Window;
        }
    }
#endif // i386

    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleOS2OemFormat(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

  This function calls NEC PC-98 machine's only.

--*/

{
    PCONSOLE_SETOS2OEMFORMAT_MSG a = (PCONSOLE_SETOS2OEMFORMAT_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
#if defined(i386)
    {
        if (!a->fOs2OemFormat) {
            Console->Flags &= ~CONSOLE_OS2_OEM_FORMAT;
        }
        else {
            Console->Flags |= CONSOLE_OS2_OEM_FORMAT;
        }
    }
#endif // i386

    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

#if defined(FE_IME)
ULONG
SrvGetConsoleNlsMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine gets NLS mode for input.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    NTSTATUS Status;
    PCONSOLE_NLS_MODE_MSG a = (PCONSOLE_NLS_MODE_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    HANDLE hEvent = NULL;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->Handle,
                                 CONSOLE_INPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        goto SrvGetConsoleNlsModeFailure;
    }

    Status = NtDuplicateObject(CONSOLE_CLIENTPROCESSHANDLE(),
                       a->hEvent,
                       NtCurrentProcess(),
                       &hEvent,
                       0,
                       FALSE,
                       DUPLICATE_SAME_ACCESS
                       );
    if (!NT_SUCCESS(Status)) {
        goto SrvGetConsoleNlsModeFailure;
    }

    /*
     * Caller should set FALSE on a->Ready.
     */
    if (a->Ready == FALSE)
    {
        a->Ready = HandleData->Buffer.InputBuffer->ImeMode.ReadyConversion;

        if (a->Ready == FALSE)
        {
            /*
             * If not ready ImeMode.Conversion,
             * then get conversion status from ConIME.
             */
            Status = QueueConsoleMessage(Console,
                        CM_GET_NLSMODE,
                        (WPARAM)hEvent,
                        0L
                       );
            if (!NT_SUCCESS(Status)) {
                goto SrvGetConsoleNlsModeFailure;
            }
        }
        else
        {
            if (! HandleData->Buffer.InputBuffer->ImeMode.Disable) {
                a->NlsMode = ImmConversionToConsole(
                                 HandleData->Buffer.InputBuffer->ImeMode.Conversion );
            }
            else {
                a->NlsMode = 0;
            }
            NtSetEvent(hEvent, NULL);
            NtClose(hEvent);
        }
    }
    else
    {
        if (! HandleData->Buffer.InputBuffer->ImeMode.Disable) {
            a->NlsMode = ImmConversionToConsole(
                             HandleData->Buffer.InputBuffer->ImeMode.Conversion );
        }
        else {
            a->NlsMode = 0;
        }
        NtSetEvent(hEvent, NULL);
        NtClose(hEvent);
    }

    UnlockConsole(Console);
    return((ULONG) Status);

SrvGetConsoleNlsModeFailure:
    if (hEvent) {
        NtSetEvent(hEvent, NULL);
        NtClose(hEvent);
    }
    UnlockConsole(Console);
    return((ULONG) Status);

    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleNlsMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine sets NLS mode for input.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    PCONSOLE_NLS_MODE_MSG a = (PCONSOLE_NLS_MODE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    HANDLE hEvent = NULL;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->Handle,
                                 CONSOLE_INPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        goto SrvSetConsoleNlsModeFailure;
    }

    Status = NtDuplicateObject(CONSOLE_CLIENTPROCESSHANDLE(),
                       a->hEvent,
                       NtCurrentProcess(),
                       &hEvent,
                       0,
                       FALSE,
                       DUPLICATE_SAME_ACCESS
                       );
    if (!NT_SUCCESS(Status)) {
        goto SrvSetConsoleNlsModeFailure;
    }

    Status = QueueConsoleMessage(Console,
                CM_SET_NLSMODE,
                (WPARAM)hEvent,
                a->NlsMode
               );
    if (!NT_SUCCESS(Status)) {
        goto SrvSetConsoleNlsModeFailure;
    }

    UnlockConsole(Console);
    return Status;

SrvSetConsoleNlsModeFailure:
    if (hEvent) {
        NtSetEvent(hEvent, NULL);
        NtClose(hEvent);
    }
    UnlockConsole(Console);
    return Status;

    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvRegisterConsoleIME(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine register console IME on the current desktop.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    PCONSOLE_REGISTER_CONSOLEIME_MSG a = (PCONSOLE_REGISTER_CONSOLEIME_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCSR_PROCESS Process;
    HDESK hdesk;
    HWINSTA hwinsta;
    UNICODE_STRING strDesktopName;

    a->dwConsoleThreadId = 0;

    if (!CsrValidateMessageBuffer(m, &a->Desktop, a->DesktopLength, sizeof(BYTE))) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Connect to the windowstation and desktop.
    //

    if (!CsrImpersonateClient(NULL)) {
        return (ULONG)STATUS_BAD_IMPERSONATION_LEVEL;
    }

    Process = (PCSR_PROCESS)(CSR_SERVER_QUERYCLIENTTHREAD()->Process);
    if (a->DesktopLength)
        RtlInitUnicodeString(&strDesktopName, a->Desktop);
    else
        RtlInitUnicodeString(&strDesktopName, L"Default");
    hdesk = NtUserResolveDesktop(
            Process->ProcessHandle,
            &strDesktopName, FALSE, &hwinsta);

    CsrRevertToSelf();

    if (hdesk == NULL) {
        return (ULONG)STATUS_UNSUCCESSFUL;
    }

    Status = ConSrvRegisterConsoleIME(Process,
                                      hdesk,
                                      hwinsta,
                                      a->hWndConsoleIME,
                                      a->dwConsoleIMEThreadId,
                                      REGCONIME_REGISTER,
                                      &a->dwConsoleThreadId
                                     );


    return ((ULONG)Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvUnregisterConsoleIME(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine unregister console IME on the current desktop.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    PCONSOLE_UNREGISTER_CONSOLEIME_MSG a = (PCONSOLE_UNREGISTER_CONSOLEIME_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCSR_PROCESS Process;
    PCONSOLE_PER_PROCESS_DATA ProcessData;

    Process = (PCSR_PROCESS)(CSR_SERVER_QUERYCLIENTTHREAD()->Process);
    ProcessData = CONSOLE_FROMPROCESSPERPROCESSDATA(Process);

    Status = ConSrvRegisterConsoleIME(Process,
                                      ProcessData->hDesk,
                                      ProcessData->hWinSta,
                                      NULL,
                                      a->dwConsoleIMEThreadId,
                                      REGCONIME_UNREGISTER,
                                      NULL
                                     );


    return ((ULONG)Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}
#endif // FE_IME

#endif // FE_SB
