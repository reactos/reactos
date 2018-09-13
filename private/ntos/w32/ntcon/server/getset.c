/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    getset.c

Abstract:

        This file implements the NT console server console state API

Author:

    Therese Stowell (thereses) 5-Dec-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef i386
VOID
ReverseMousePointer(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PSMALL_RECT Region
    );
#endif

ULONG
SrvGetConsoleMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_MODE_MSG a = (PCONSOLE_MODE_MSG)&m->u.ApiMessageData;
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
                                 CONSOLE_ANY_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {

        //
        // check handle type and access
        //

        if (HandleData->HandleType & CONSOLE_INPUT_HANDLE) {
            a->Mode = HandleData->Buffer.InputBuffer->InputMode;
            if (Console->Flags & CONSOLE_USE_PRIVATE_FLAGS) {
                a->Mode |= ENABLE_PRIVATE_FLAGS;
                if (Console->InsertMode) {
                    a->Mode |= ENABLE_INSERT_MODE;
                }
                if (Console->Flags & CONSOLE_QUICK_EDIT_MODE) {
                    a->Mode |= ENABLE_QUICK_EDIT_MODE;
                }
            }
        } else {
            a->Mode = HandleData->Buffer.ScreenBuffer->OutputMode;
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleNumberOfFonts(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETNUMBEROFFONTS_MSG a = (PCONSOLE_GETNUMBEROFFONTS_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
        Status = STATUS_FULLSCREEN_MODE;
    } else {
        Status = GetNumFonts(&a->NumberOfFonts);
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleNumberOfInputEvents(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETNUMBEROFINPUTEVENTS_MSG a = (PCONSOLE_GETNUMBEROFINPUTEVENTS_MSG)&m->u.ApiMessageData;
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
                                 a->InputHandle,
                                 CONSOLE_INPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        Status = GetNumberOfReadyEvents(HandleData->Buffer.InputBuffer,
                                      &a->ReadyEvents
                                     );
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleScreenBufferInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETSCREENBUFFERINFO_MSG a = (PCONSOLE_GETSCREENBUFFERINFO_MSG)&m->u.ApiMessageData;
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
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        Status = GetScreenBufferInformation(HandleData->Buffer.ScreenBuffer,
                                          &a->Size,
                                          &a->CursorPosition,
                                          &a->ScrollPosition,
                                          &a->Attributes,
                                          &a->CurrentWindowSize,
                                          &a->MaximumWindowSize
                                         );
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleCursorInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETCURSORINFO_MSG a = (PCONSOLE_GETCURSORINFO_MSG)&m->u.ApiMessageData;
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
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        a->CursorSize = HandleData->Buffer.ScreenBuffer->BufferInfo.TextInfo.CursorSize;
        a->Visible = (BOOLEAN) HandleData->Buffer.ScreenBuffer->BufferInfo.TextInfo.CursorVisible;
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleMouseInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETMOUSEINFO_MSG a = (PCONSOLE_GETMOUSEINFO_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = GetMouseButtons(&a->NumButtons);
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleFontInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETFONTINFO_MSG a = (PCONSOLE_GETFONTINFO_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (!CsrValidateMessageBuffer(m, &a->BufPtr, a->NumFonts, sizeof(*a->BufPtr))) {
        UnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        if (HandleData->Buffer.ScreenBuffer->Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
            Status = STATUS_FULLSCREEN_MODE;
        } else {
            Status = GetAvailableFonts(HandleData->Buffer.ScreenBuffer,
                                     a->MaximumWindow,
                                     a->BufPtr,
                                     &a->NumFonts
                                    );
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleFontSize(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETFONTSIZE_MSG a = (PCONSOLE_GETFONTSIZE_MSG)&m->u.ApiMessageData;
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
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        if (HandleData->Buffer.ScreenBuffer->Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
            Status = STATUS_FULLSCREEN_MODE;
        } else {
            Status = GetFontSize(a->FontIndex,
                               &a->FontSize
                              );
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleCurrentFont(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETCURRENTFONT_MSG a = (PCONSOLE_GETCURRENTFONT_MSG)&m->u.ApiMessageData;
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
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        if (HandleData->Buffer.ScreenBuffer->Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
            Status = STATUS_FULLSCREEN_MODE;
        } else {
            Status = GetCurrentFont(HandleData->Buffer.ScreenBuffer,
                                  a->MaximumWindow,
                                  &a->FontIndex,
                                  &a->FontSize
                                 );
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleMode(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_MODE_MSG a = (PCONSOLE_MODE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    try {
        Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                     a->Handle,
                                     CONSOLE_ANY_HANDLE,
                                     GENERIC_WRITE,
                                     &HandleData
                                    );
        if (!NT_SUCCESS(Status)) {
            leave;
        }

        if (HandleData->HandleType & CONSOLE_INPUT_HANDLE) {
            if (a->Mode & ~(INPUT_MODES | PRIVATE_MODES)) {
                Status = STATUS_INVALID_PARAMETER;
                leave;
            }
            if ((a->Mode & (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)) == ENABLE_ECHO_INPUT) {
                Status = STATUS_INVALID_PARAMETER;
                leave;
            }
            if (a->Mode & PRIVATE_MODES) {
                Console->Flags |= CONSOLE_USE_PRIVATE_FLAGS;
                if (a->Mode & ENABLE_QUICK_EDIT_MODE) {
                    Console->Flags |= CONSOLE_QUICK_EDIT_MODE;
                } else {
                    Console->Flags &= ~CONSOLE_QUICK_EDIT_MODE;
                }
                if (a->Mode & ENABLE_INSERT_MODE) {
                    Console->InsertMode = TRUE;
                } else {
                    Console->InsertMode = FALSE;
                }
            } else {
                Console->Flags &= ~CONSOLE_USE_PRIVATE_FLAGS;
            }

#ifdef i386
            if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE &&
                Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER &&
                (a->Mode & ENABLE_MOUSE_INPUT) != (HandleData->Buffer.InputBuffer->InputMode & ENABLE_MOUSE_INPUT)) {
                if (a->Mode & ENABLE_MOUSE_INPUT) {
                    HandleData->Buffer.InputBuffer->InputMode |= ENABLE_MOUSE_INPUT;
                }
#if defined(FE_SB)
                // Korean HBIOS doesn't like to reverse mouse pointer.
                // Because HBIOS will initialize full screen mode again.
                // 949 = Korea WanSung Code Page.
                if (Console->OutputCP != 949) {
                    ReverseMousePointer(Console->CurrentScreenBuffer,
                                        &Console->CurrentScreenBuffer->Window);
                }
#else
                ReverseMousePointer(Console->CurrentScreenBuffer,
                                    &Console->CurrentScreenBuffer->Window);
#endif
            }
#endif
            HandleData->Buffer.InputBuffer->InputMode = a->Mode & ~PRIVATE_MODES;
        }
        else {
            if (a->Mode & ~OUTPUT_MODES) {
                Status = STATUS_INVALID_PARAMETER;
                leave;
            }
            HandleData->Buffer.ScreenBuffer->OutputMode = a->Mode;
        }
    } finally {
        UnlockConsole(Console);
    }
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGenerateConsoleCtrlEvent(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_CTRLEVENT_MSG a = (PCONSOLE_CTRLEVENT_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    try {
        //
        // Make sure the process group id is valid
        //
        if (a->ProcessGroupId) {
            PCONSOLE_PROCESS_HANDLE ProcessHandleRecord;
            PLIST_ENTRY ListHead, ListNext;

            Status = STATUS_INVALID_PARAMETER;
            ListHead = &Console->ProcessHandleList;
            ListNext = ListHead->Flink;
            while (ListNext != ListHead) {
                ProcessHandleRecord = CONTAINING_RECORD( ListNext, CONSOLE_PROCESS_HANDLE, ListLink );
                ListNext = ListNext->Flink;
                if (ProcessHandleRecord->Process->ProcessGroupId == a->ProcessGroupId) {
                    Status = STATUS_SUCCESS;
                    break;
                }
            }
        }
        if (NT_SUCCESS(Status)) {
            Console->LimitingProcessId = a->ProcessGroupId;
            HandleCtrlEvent(Console, a->CtrlEvent);
        }

    } finally {
        UnlockConsole(Console);
    }
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleActiveScreenBuffer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETACTIVESCREENBUFFER_MSG a = (PCONSOLE_SETACTIVESCREENBUFFER_MSG)&m->u.ApiMessageData;
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
                                 a->OutputHandle,
                                 CONSOLE_GRAPHICS_OUTPUT_HANDLE | CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        Status = SetActiveScreenBuffer(HandleData->Buffer.ScreenBuffer);
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


ULONG
SrvFlushConsoleInputBuffer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_FLUSHINPUTBUFFER_MSG a = (PCONSOLE_FLUSHINPUTBUFFER_MSG)&m->u.ApiMessageData;
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
                                 a->InputHandle,
                                 CONSOLE_INPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        Status = FlushInputBuffer(HandleData->Buffer.InputBuffer);
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetLargestConsoleWindowSize(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETLARGESTWINDOWSIZE_MSG a = (PCONSOLE_GETLARGESTWINDOWSIZE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PSCREEN_INFORMATION ScreenInfo;
    WINDOW_LIMITS WindowLimits;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        COORD FontSize;

        ScreenInfo = HandleData->Buffer.ScreenBuffer;
        if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
            a->Size.X = 80;
#if defined(FE_SB)
            a->Size.Y = CONSOLE_IS_DBCS_OUTPUTCP(Console)?25:50;
#else
            a->Size.Y = 50;
#endif
        } else {
            if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                FontSize = SCR_FONTSIZE(ScreenInfo);
            } else {
                FontSize.X = 1;
                FontSize.Y = 1;
            }
            GetWindowLimits(ScreenInfo, &WindowLimits);
            a->Size.X = (SHORT)(WindowLimits.FullScreenSize.X / FontSize.X);
            a->Size.Y = (SHORT)(WindowLimits.FullScreenSize.Y / FontSize.Y);
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleScreenBufferSize(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETSCREENBUFFERSIZE_MSG a = (PCONSOLE_SETSCREENBUFFERSIZE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PSCREEN_INFORMATION ScreenInfo;
    WINDOW_LIMITS WindowLimits;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {

        ScreenInfo = HandleData->Buffer.ScreenBuffer;

        //
        // make sure requested screen buffer size isn't smaller than the window
        //

        GetWindowLimits(ScreenInfo, &WindowLimits);
        if (a->Size.X < CONSOLE_WINDOW_SIZE_X(ScreenInfo) ||
            a->Size.Y < CONSOLE_WINDOW_SIZE_Y(ScreenInfo) ||
            a->Size.Y < WindowLimits.MinimumWindowSize.Y ||
            a->Size.X < WindowLimits.MinimumWindowSize.X) {
            Status = STATUS_INVALID_PARAMETER;
        }
        else if (a->Size.X == ScreenInfo->ScreenBufferSize.X &&
                 a->Size.Y == ScreenInfo->ScreenBufferSize.Y) {
            Status = STATUS_SUCCESS;
        } else {
            Status = ResizeScreenBuffer(ScreenInfo,
                                  a->Size,
                                  TRUE);
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleCursorPosition(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETCURSORPOSITION_MSG a = (PCONSOLE_SETCURSORPOSITION_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    COORD WindowOrigin;
    PSCREEN_INFORMATION ScreenInfo;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {

        ScreenInfo = HandleData->Buffer.ScreenBuffer;

        if (a->CursorPosition.X >= ScreenInfo->ScreenBufferSize.X ||
            a->CursorPosition.Y >= ScreenInfo->ScreenBufferSize.Y ||
            a->CursorPosition.X < 0 ||
            a->CursorPosition.Y < 0) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            Status = SetCursorPosition(ScreenInfo,
                                       a->CursorPosition,
                                       TRUE
                                      );
        }
        if (NT_SUCCESS(Status)) {
#if defined(FE_IME)
            if (ScreenInfo->Console->Flags & CONSOLE_JUST_VDM_UNREGISTERED){
                if( ScreenInfo->Console->InputBuffer.ImeMode.Open ){
                    SHORT ScrollY = 0;
                    AdjustCursorPosition(ScreenInfo,a->CursorPosition,TRUE,&ScrollY);
                    a->CursorPosition.Y += ScrollY;
                }
                Console->Flags &= ~CONSOLE_JUST_VDM_UNREGISTERED;
            }
#endif
            WindowOrigin.X = 0;
            WindowOrigin.Y = 0;
            if (ScreenInfo->Window.Left > a->CursorPosition.X) {
                WindowOrigin.X = a->CursorPosition.X - ScreenInfo->Window.Left;
            }
            else if (ScreenInfo->Window.Right < a->CursorPosition.X) {
                WindowOrigin.X = a->CursorPosition.X - ScreenInfo->Window.Right;
            }
            if (ScreenInfo->Window.Top > a->CursorPosition.Y) {
                WindowOrigin.Y = a->CursorPosition.Y - ScreenInfo->Window.Top;
            }
            else if (ScreenInfo->Window.Bottom < a->CursorPosition.Y) {
                WindowOrigin.Y = a->CursorPosition.Y - ScreenInfo->Window.Bottom;
            }
            Status = SetWindowOrigin(ScreenInfo,
                                     FALSE,
                                     WindowOrigin
                                    );
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleCursorInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETCURSORINFO_MSG a = (PCONSOLE_SETCURSORINFO_MSG)&m->u.ApiMessageData;
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
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        if (a->CursorSize > 100 || a->CursorSize == 0) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            Status = SetCursorInformation(HandleData->Buffer.ScreenBuffer,a->CursorSize,a->Visible);
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleWindowInfo(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETWINDOWINFO_MSG a = (PCONSOLE_SETWINDOWINFO_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PSCREEN_INFORMATION ScreenInfo;
    COORD NewWindowSize;
    WINDOW_LIMITS WindowLimits;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        ScreenInfo = HandleData->Buffer.ScreenBuffer;
        if (!a->Absolute) {
            a->Window.Left += ScreenInfo->Window.Left;
            a->Window.Right += ScreenInfo->Window.Right;
            a->Window.Top += ScreenInfo->Window.Top;
            a->Window.Bottom += ScreenInfo->Window.Bottom;
        }
        if (a->Window.Right < a->Window.Left ||
            a->Window.Bottom < a->Window.Top) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            NewWindowSize.X = (SHORT)(WINDOW_SIZE_X(&a->Window));
            NewWindowSize.Y = (SHORT)(WINDOW_SIZE_Y(&a->Window));
            GetWindowLimits(ScreenInfo, &WindowLimits);
            if ((NewWindowSize.X > WindowLimits.MaximumWindowSize.X ||
                 NewWindowSize.Y > WindowLimits.MaximumWindowSize.Y) &&
                 !(ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
                Status = STATUS_INVALID_PARAMETER;
            } else {
#ifdef i386
                if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
                    COORD NewOrigin;

                    if (NewWindowSize.X != (SHORT)(WINDOW_SIZE_X(&ScreenInfo->Window)) ||
                        NewWindowSize.Y != (SHORT)(WINDOW_SIZE_Y(&ScreenInfo->Window))) {
                        COORD WindowSize;
                        ULONG ModeIndex;

#if defined(FE_SB)
                        ModeIndex = MatchWindowSize(ScreenInfo->Console->OutputCP,NewWindowSize,&WindowSize);
#else
                        ModeIndex = MatchWindowSize(NewWindowSize,&WindowSize);
#endif
                        if (NewWindowSize.X != WindowSize.X ||
                            NewWindowSize.Y != WindowSize.Y ||
                            WindowSize.X > ScreenInfo->ScreenBufferSize.X ||
                            WindowSize.Y > ScreenInfo->ScreenBufferSize.Y) {
                            UnlockConsole(Console);
                            return (ULONG) STATUS_FULLSCREEN_MODE;
                        }
                        ScreenInfo->BufferInfo.TextInfo.ModeIndex = ModeIndex;
                        ResizeWindow(ScreenInfo,
                                     &a->Window,
                                     FALSE
                                    );
                        ScreenInfo->BufferInfo.TextInfo.WindowedWindowSize.X =
                                CONSOLE_WINDOW_SIZE_X(ScreenInfo);
                        ScreenInfo->BufferInfo.TextInfo.WindowedWindowSize.Y =
                                CONSOLE_WINDOW_SIZE_Y(ScreenInfo);
                        if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE &&
                            (!(ScreenInfo->Console->Flags & CONSOLE_VDM_REGISTERED)) ) {
                            SetVideoMode(ScreenInfo);
                            WriteToScreen(ScreenInfo,&ScreenInfo->Window);
                        }
                    } else {
                        NewOrigin.X = a->Window.Left;
                        NewOrigin.Y = a->Window.Top;
                        SetWindowOrigin(ScreenInfo,
                                        TRUE,
                                        NewOrigin
                                       );
                    }
                } else
#endif
                {
                    Status = ResizeWindow(ScreenInfo,
                                          &a->Window,
                                          TRUE
                                         );
                    if (ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
                        SetWindowSize(ScreenInfo);
                        WriteToScreen(ScreenInfo,&ScreenInfo->Window);
                    }
                }
            }
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvScrollConsoleScreenBuffer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SCROLLSCREENBUFFER_MSG a = (PCONSOLE_SCROLLSCREENBUFFER_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PSMALL_RECT ClipRect;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {

        if (a->Clip) {
            ClipRect = &a->ClipRectangle;
        }
        else {
            ClipRect = NULL;
        }
        if (!a->Unicode) {
#if defined(FE_SB)
            a->Fill.Char.UnicodeChar = CharToWchar(Console,
                                                   Console->OutputCP,
                                                   &a->Fill.Char.AsciiChar);
#else
            a->Fill.Char.UnicodeChar = CharToWchar(
                    Console->OutputCP, a->Fill.Char.AsciiChar);
#endif
        } else if ((Console->CurrentScreenBuffer->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                !(Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
            RealUnicodeToFalseUnicode(&a->Fill.Char.UnicodeChar,
                                    1,
                                    Console->OutputCP
                                    );
        }
        Status = ScrollRegion(HandleData->Buffer.ScreenBuffer,
                            &a->ScrollRectangle,
                            ClipRect,
                            a->DestinationOrigin,
                            a->Fill
                           );
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

VOID
UpdatePopups(
    IN PCONSOLE_INFORMATION Console,
    IN WORD NewAttributes,
    IN WORD NewPopupAttributes,
    IN WORD OldAttributes,
    IN WORD OldPopupAttributes
    )

/*

    this routine is called when the user changes the screen/popup
    colors.  it goes through the popup structures and changes
    the saved contents to reflect the new screen/popup colors.

*/

{
    PLIST_ENTRY HistoryListHead, HistoryListNext;
    PLIST_ENTRY PopupListHead, PopupListNext;
    PCOMMAND_HISTORY History;
    PCLE_POPUP Popup;
    SHORT i,j;
    PCHAR_INFO OldContents;
    WORD InvertedOldPopupAttributes,InvertedNewPopupAttributes;

    InvertedOldPopupAttributes = (WORD)(((OldPopupAttributes << 4) & 0xf0) |
                                ((OldPopupAttributes >> 4) & 0x0f));
    InvertedNewPopupAttributes = (WORD)(((NewPopupAttributes << 4) & 0xf0) |
                                ((NewPopupAttributes >> 4) & 0x0f));
    HistoryListHead = &Console->CommandHistoryList;
    HistoryListNext = HistoryListHead->Blink;
    while (HistoryListNext != HistoryListHead) {
        History = CONTAINING_RECORD( HistoryListNext, COMMAND_HISTORY, ListLink );
        HistoryListNext = HistoryListNext->Blink;
        if (History->Flags & CLE_ALLOCATED && !CLE_NO_POPUPS(History)) {
            PopupListHead = &History->PopupList;
            PopupListNext = PopupListHead->Blink;
            while (PopupListNext != PopupListHead) {
                Popup = CONTAINING_RECORD( PopupListNext, CLE_POPUP, ListLink );
                PopupListNext = PopupListNext->Blink;
                OldContents = Popup->OldContents;
                for (i=Popup->Region.Left;i<=Popup->Region.Right;i++) {
                    for (j=Popup->Region.Top;j<=Popup->Region.Bottom;j++) {
                        if (OldContents->Attributes == OldAttributes) {
                            OldContents->Attributes = NewAttributes;
                        } else if (OldContents->Attributes == OldPopupAttributes) {
                            OldContents->Attributes = NewPopupAttributes;
                        } else if (OldContents->Attributes == InvertedOldPopupAttributes) {
                            OldContents->Attributes = InvertedNewPopupAttributes;
                        }
                        OldContents++;
                    }
                }
            }
        }
    }
}


NTSTATUS
SetScreenColors(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN WORD Attributes,
    IN WORD PopupAttributes,
    IN BOOL UpdateWholeScreen
    )
{
    SHORT i,j;
    PROW Row;
    WORD DefaultAttributes,DefaultPopupAttributes;
    PCONSOLE_INFORMATION Console;
    COLORREF rgbBk;
    COLORREF rgbText;

    Console = ScreenInfo->Console;
    rgbBk = ConvertAttrToRGB(Console, LOBYTE(Attributes >> 4));
    rgbBk = GetNearestColor(Console->hDC, rgbBk);
    rgbText = ConvertAttrToRGB(Console, LOBYTE(Attributes));
    rgbText = GetNearestColor(Console->hDC, rgbText);
    if (ACTIVE_SCREEN_BUFFER(ScreenInfo)) {
        SetDCBrushColor(Console->hDC, rgbBk);
        SetTextColor(Console->hDC, rgbText);
        SetBkColor(Console->hDC, rgbBk);
        Console->LastAttributes = Attributes;
        SetConsoleBkColor(Console->hWnd, rgbBk);
    }

    DefaultAttributes = ScreenInfo->Attributes;
    DefaultPopupAttributes = ScreenInfo->PopupAttributes;
    ScreenInfo->Attributes = Attributes;
    ScreenInfo->PopupAttributes = PopupAttributes;
#if defined(FE_IME)
    SetUndetermineAttribute( Console );
#endif

    if (UpdateWholeScreen && ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        WORD InvertedOldPopupAttributes,InvertedNewPopupAttributes;

        InvertedOldPopupAttributes = (WORD)(((DefaultPopupAttributes << 4) & 0xf0) |
                                    ((DefaultPopupAttributes >> 4) & 0x0f));
        InvertedNewPopupAttributes = (WORD)(((PopupAttributes << 4) & 0xf0) |
                                    ((PopupAttributes >> 4) & 0x0f));
        //
        // change all chars with default color
        //

        for (i=0;i<ScreenInfo->ScreenBufferSize.Y;i++) {
            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[i];
            for (j=0;j<Row->AttrRow.Length;j++) {
                if (Row->AttrRow.Attrs[j].Attr == DefaultAttributes) {
                    Row->AttrRow.Attrs[j].Attr = Attributes;
                } else if (Row->AttrRow.Attrs[j].Attr == DefaultPopupAttributes) {
                    Row->AttrRow.Attrs[j].Attr = PopupAttributes;
                } else if (Row->AttrRow.Attrs[j].Attr == InvertedOldPopupAttributes) {
                    Row->AttrRow.Attrs[j].Attr = InvertedNewPopupAttributes;
                }
            }
        }

        if (Console->PopupCount)
            UpdatePopups(Console,
                         Attributes,
                         PopupAttributes,
                         DefaultAttributes,
                         DefaultPopupAttributes
                         );
        // force repaint of entire line
        ScreenInfo->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
        WriteToScreen(ScreenInfo,&ScreenInfo->Window);
        ScreenInfo->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;
    }

    return STATUS_SUCCESS;
}

ULONG
SrvSetConsoleTextAttribute(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETTEXTATTRIBUTE_MSG a = (PCONSOLE_SETTEXTATTRIBUTE_MSG)&m->u.ApiMessageData;
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
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {

        if (a->Attributes & ~VALID_TEXT_ATTRIBUTES) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            Status = SetScreenColors(HandleData->Buffer.ScreenBuffer,
                                     a->Attributes,
                                     HandleData->Buffer.ScreenBuffer->PopupAttributes,
                                     FALSE
                                    );
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleFont(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETFONT_MSG a = (PCONSOLE_SETFONT_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PSCREEN_INFORMATION ScreenInfo;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    try {
        Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                     a->OutputHandle,
                                     CONSOLE_OUTPUT_HANDLE,
                                     GENERIC_WRITE,
                                     &HandleData
                                    );
        if (!NT_SUCCESS(Status)) {
            leave;
        }

        ScreenInfo = HandleData->Buffer.ScreenBuffer;
        if (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN) {
            Status = STATUS_FULLSCREEN_MODE;
        } else {
#if defined(FE_SB)
            Status = SetScreenBufferFont(ScreenInfo,a->FontIndex,ScreenInfo->Console->OutputCP);
#else
            Status = SetScreenBufferFont(ScreenInfo,a->FontIndex);
#endif
        }
    } finally {
        UnlockConsole(Console);
    }
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetConsoleIcon(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETICON_MSG a = (PCONSOLE_SETICON_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    HANDLE hIcon;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (a->hIcon == NULL) {
        hIcon = ghDefaultIcon;
    } else {
        hIcon = CopyIcon(a->hIcon);
    }

    if (hIcon == NULL) {
        Status = STATUS_INVALID_PARAMETER;
    } else if (hIcon != Console->hIcon) {
        PostMessage(Console->hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        if (Console->hIcon != ghDefaultIcon) {
            DestroyIcon(Console->hIcon);
        }
        Console->hIcon = hIcon;

        /*
         * Small icon now
         */
        if (hIcon != ghDefaultIcon) {
            /*
             * The new one is not the default, clean-up
             */
            if (Console->hSmIcon != NULL && Console->hSmIcon != ghDefaultSmIcon) {
                DestroyIcon(Console->hSmIcon);
            }
            Console->hSmIcon = NULL;
            PostMessage(Console->hWnd, WM_SETICON, ICON_SMALL, (LPARAM)NULL);
        } else {
            /*
             * Change to default, so we can use the default small icon
             */
            if (Console->hSmIcon != ghDefaultSmIcon) {
                if (Console->hSmIcon != NULL) {
                    DestroyIcon(Console->hSmIcon);
                }
                Console->hSmIcon = ghDefaultSmIcon;
                PostMessage(Console->hWnd, WM_SETICON, ICON_SMALL, (LPARAM)ghDefaultSmIcon);
            }
        }
    }
    UnlockConsole(Console);

    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


ULONG
SrvSetConsoleCP(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_SETCP_MSG a = (PCONSOLE_SETCP_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    HANDLE hEvent = NULL;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (a->hEvent) {
        Status = NtDuplicateObject(CONSOLE_CLIENTPROCESSHANDLE(),
                       a->hEvent,
                       NtCurrentProcess(),
                       &hEvent,
                       0,
                       FALSE,
                       DUPLICATE_SAME_ACCESS
                       );
        if (!NT_SUCCESS(Status)) {
            goto SrvSetConsoleCPFailure;
        }
    }

    if (!IsValidCodePage(a->wCodePageID)) {
        Status = STATUS_INVALID_PARAMETER;
        goto SrvSetConsoleCPFailure;
    }
    if ( IsAvailableFarEastCodePage( a->wCodePageID ) &&
         OEMCP != a->wCodePageID )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto SrvSetConsoleCPFailure;
    }

    if ( (a->Output && Console->OutputCP != a->wCodePageID) ||
         (!a->Output && Console->CP != a->wCodePageID)         ) {

        UINT CodePage;

        if (a->Output) {

            // Backup old code page
            CodePage = Console->OutputCP;

            // Set new code page
            Console->OutputCP = a->wCodePageID;

            Console->fIsDBCSOutputCP = CONSOLE_IS_DBCS_ENABLED() && IsAvailableFarEastCodePage(Console->OutputCP);

            if (!ReCreateDbcsScreenBuffer(Console, CodePage) ) {
                RIPMSG1(RIP_WARNING, "SrvSetConsoleCP: ReCreateDbcsScreenBuffer failed. Restoring to CP=%d",
                        CodePage);
                Console->OutputCP = CodePage;
                Console->fIsDBCSOutputCP = CONSOLE_IS_DBCS_ENABLED() && IsAvailableFarEastCodePage(CodePage);
                Status = STATUS_NO_MEMORY;
                goto SrvSetConsoleCPFailure;
            }
            SetConsoleCPInfo(Console,a->Output);
            Status = QueueConsoleMessage(Console,
                        CM_SET_IME_CODEPAGE,
                        (WPARAM)hEvent,
                        MAKELPARAM(a->Output,CodePage)
                       );
            if (!NT_SUCCESS(Status)) {
                goto SrvSetConsoleCPFailure;
            }

            // load special ROM font, if necessary

#ifdef i386

            if (Console->FullScreenFlags & CONSOLE_FULLSCREEN_HARDWARE) {
                SetROMFontCodePage(Console->OutputCP,
                                   Console->CurrentScreenBuffer->BufferInfo.TextInfo.ModeIndex);
                WriteRegionToScreenHW(Console->CurrentScreenBuffer,
                        &Console->CurrentScreenBuffer->Window);
            }
#endif


        } else {

            // Backup old code page
            CodePage = Console->CP;

            // Set new code page
            Console->CP = a->wCodePageID;

            Console->fIsDBCSCP = CONSOLE_IS_DBCS_ENABLED() && IsAvailableFarEastCodePage(Console->CP);

            SetConsoleCPInfo(Console,a->Output);
            Status = QueueConsoleMessage(Console,
                        CM_SET_IME_CODEPAGE,
                        (WPARAM)hEvent,
                        MAKELPARAM(a->Output,CodePage)
                       );
            if (!NT_SUCCESS(Status)) {
                goto SrvSetConsoleCPFailure;
            }
        }
    }
    else {
        if (hEvent) {
            NtSetEvent(hEvent, NULL);
            NtClose(hEvent);
        }
    }

    UnlockConsole(Console);
    return STATUS_SUCCESS;

SrvSetConsoleCPFailure:
    if (hEvent) {
        NtSetEvent(hEvent, NULL);
        NtClose(hEvent);
    }
    UnlockConsole(Console);
    return Status;

    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleCP(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETCP_MSG a = (PCONSOLE_GETCP_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (a->Output) {
        a->wCodePageID = Console->OutputCP;
    } else {
        a->wCodePageID = Console->CP;
    }
    UnlockConsole(Console);
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetConsoleKeyboardLayoutName(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETKEYBOARDLAYOUTNAME_MSG a = (PCONSOLE_GETKEYBOARDLAYOUTNAME_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    if (Console->hklActive == NULL) {
        //
        // hklActive is not set yet. Let's initialize it here.
        //
        extern void GetNonBiDiKeyboardLayout(HKL * phklActive);

        RIPMSG1(RIP_WARNING, "SrvGetConsoleKeyboardLayoutName: hklActive is not initialized. pCon=%p", Console);

        SystemParametersInfo(SPI_GETDEFAULTINPUTLANG, 0, &Console->hklActive, FALSE);
        GetNonBiDiKeyboardLayout(&Console->hklActive);
    }
    ActivateKeyboardLayout(Console->hklActive, 0);
    if (a->bAnsi) {
        GetKeyboardLayoutNameA(a->achLayout);
    } else {
        GetKeyboardLayoutNameW(a->awchLayout);
    }
    UnlockConsole(Console);
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


ULONG
SrvGetConsoleWindow(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_GETCONSOLEWINDOW_MSG a = (PCONSOLE_GETCONSOLEWINDOW_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    a->hwnd = Console->hWnd;

    UnlockConsole(Console);
    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


