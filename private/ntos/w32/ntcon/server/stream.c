/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    stream.c

Abstract:

        This file implements the NT console server stream API

Author:

    Therese Stowell (thereses) 6-Nov-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#define IS_CONTROL_CHAR(wch)  ((wch) < L' ')
#define IS_GLYPH_CHAR(wch)   (((wch) < L' ') || ((wch) == 0x007F))

#define LINE_INPUT_BUFFER_SIZE (256 * sizeof(WCHAR))

#define CONSOLE_CTRL_2 0x0

NTSTATUS
WaitForMoreToRead(
    IN PINPUT_INFORMATION InputInformation,
    IN PCSR_API_MSG Message OPTIONAL,
    IN CSR_WAIT_ROUTINE WaitRoutine OPTIONAL,
    IN PVOID WaitParameter OPTIONAL,
    IN ULONG WaitParameterLength  OPTIONAL,
    IN BOOLEAN WaitBlockExists OPTIONAL
    );

BOOLEAN
WriteConsoleWaitRoutine(
    IN PLIST_ENTRY WaitQueue,
    IN PCSR_THREAD WaitingThread,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2,
    IN ULONG WaitFlags
    );

HANDLE
FindActiveScreenBufferHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PCONSOLE_INFORMATION Console
    )
{
    ULONG i;
    HANDLE ActiveScreenHandle;
    PHANDLE_DATA ActiveScreenHandleData;
    NTSTATUS Status;

    ActiveScreenHandle = INVALID_HANDLE_VALUE;
    for (i=0;i<ProcessData->HandleTableSize;i++) {
        Status = DereferenceIoHandleNoCheck(ProcessData,
                                     (HANDLE) i,
                                     &ActiveScreenHandleData
                                    );
        if (NT_SUCCESS(Status) &&
            Console->CurrentScreenBuffer == ActiveScreenHandleData->Buffer.ScreenBuffer) {
            ASSERT (ActiveScreenHandleData->HandleType & (CONSOLE_OUTPUT_HANDLE | CONSOLE_GRAPHICS_OUTPUT_HANDLE));
            ActiveScreenHandle = (HANDLE) i;
            break;
        }
    }
    return ActiveScreenHandle;
}

ULONG
SrvOpenConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine returns a handle to the input buffer or active screen buffer.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_OPENCONSOLE_MSG a = (PCONSOLE_OPENCONSOLE_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    HANDLE Handle;
    PHANDLE_DATA HandleData;
    PCONSOLE_PER_PROCESS_DATA ProcessData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    try {
        Handle = (HANDLE) -1;
        ProcessData = CONSOLE_PERPROCESSDATA();
        if (a->HandleType == CONSOLE_INPUT_HANDLE) {

            Status = AllocateIoHandle(ProcessData,
                                      a->HandleType,
                                      &Handle
                                     );
            if (!NT_SUCCESS(Status)) {
                leave;
            }
            Status = DereferenceIoHandleNoCheck(ProcessData,
                                         Handle,
                                         &HandleData
                                        );
            ASSERT (NT_SUCCESS(Status));
            if (!NT_SUCCESS(Status)) {
                leave;
            }
            if (!InitializeInputHandle(HandleData,
                                       &Console->InputBuffer)) {
                Status = STATUS_NO_MEMORY;
                leave;
            }
            if (a->InheritHandle) {
                HandleData->HandleType |= CONSOLE_INHERITABLE;
            }
            Status = ConsoleAddShare(a->DesiredAccess,
                                     a->ShareMode,
                                     &HandleData->Buffer.InputBuffer->ShareAccess,
                                     HandleData
                                    );
            if (!NT_SUCCESS(Status)) {
                HandleData->Buffer.InputBuffer->RefCount--;
                leave;
            }
        }
        else if (a->HandleType == CONSOLE_OUTPUT_HANDLE){
            PSCREEN_INFORMATION ScreenInfo;

            //
            // open a handle to the active screen buffer.
            //

            ScreenInfo = Console->CurrentScreenBuffer;
            if (ScreenInfo == NULL) {
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
                leave;
            }
            Status = AllocateIoHandle(ProcessData,
                                      a->HandleType,
                                      &Handle
                                     );
            if (!NT_SUCCESS(Status)) {
                leave;
            }
            Status = DereferenceIoHandleNoCheck(ProcessData,
                                         Handle,
                                         &HandleData
                                        );
            ASSERT (NT_SUCCESS(Status));
            if (!NT_SUCCESS(Status)) {
                leave;
            }
            InitializeOutputHandle(HandleData, ScreenInfo);
            if (a->InheritHandle) {
                HandleData->HandleType |= CONSOLE_INHERITABLE;
            }
            Status = ConsoleAddShare(a->DesiredAccess,
                                     a->ShareMode,
                                     &HandleData->Buffer.ScreenBuffer->ShareAccess,
                                     HandleData
                                    );
            if (!NT_SUCCESS(Status)) {
                HandleData->Buffer.ScreenBuffer->RefCount--;
                leave;
            }
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }
        a->Handle = INDEX_TO_HANDLE(Handle);
        Status = STATUS_SUCCESS;
    } finally {
        if (!NT_SUCCESS(Status) && Handle != (HANDLE)-1) {
            FreeIoHandle(ProcessData,
                         Handle
                        );
        }
        UnlockConsole(Console);
    }
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

/*
 * Convert real Windows NT modifier bit into bizarre Console bits
 */
#define EITHER_CTRL_PRESSED (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)
#define EITHER_ALT_PRESSED (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)
#define MOD_PRESSED (SHIFT_PRESSED | EITHER_CTRL_PRESSED | EITHER_ALT_PRESSED)

DWORD ConsKbdState[] = {
    0,
    SHIFT_PRESSED,
                    EITHER_CTRL_PRESSED,
    SHIFT_PRESSED | EITHER_CTRL_PRESSED,
                                          EITHER_ALT_PRESSED,
    SHIFT_PRESSED |                       EITHER_ALT_PRESSED,
                    EITHER_CTRL_PRESSED | EITHER_ALT_PRESSED,
    SHIFT_PRESSED | EITHER_CTRL_PRESSED | EITHER_ALT_PRESSED
};

#define KEYEVENTSTATE_EQUAL_WINMODS(Event, WinMods)\
    ((Event.Event.KeyEvent.dwControlKeyState & ConsKbdState[WinMods]) && \
    !(Event.Event.KeyEvent.dwControlKeyState & MOD_PRESSED & ~ConsKbdState[WinMods]))

BOOL IsDbcsExemptionForHighAnsi(
    UINT wCodePage,
    WORD wNumpadChar)
{
    UserAssert(HIBYTE(wNumpadChar) == 0);

    if (wCodePage == CP_JAPANESE && IS_JPN_1BYTE_KATAKANA(wNumpadChar)) {
        /*
         * If hkl is JAPANESE and NumpadChar is in KANA range,
         * NumpadChar should be handled by the input locale.
         */
        return FALSE;
    }
    else if (wNumpadChar >= 0x80 && wNumpadChar <= 0xff) {
        /*
         * Otherwise if NumpadChar is in High ANSI range,
         * use 1252 for conversion.
         */
        return TRUE;
    }

    /*
     * None of the above.
     * This case includes the compound Leading Byte and Trailing Byte,
     * which is larger than 0xff.
     */
    return FALSE;
}

NTSTATUS
GetChar(
    IN PINPUT_INFORMATION InputInfo,
    OUT PWCHAR Char,
    IN BOOLEAN Wait,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData,
    IN PCSR_API_MSG Message OPTIONAL,
    IN CSR_WAIT_ROUTINE WaitRoutine OPTIONAL,
    IN PVOID WaitParameter OPTIONAL,
    IN ULONG WaitParameterLength  OPTIONAL,
    IN BOOLEAN WaitBlockExists OPTIONAL,
    OUT PBOOLEAN CommandLineEditingKeys OPTIONAL,
    OUT PBOOLEAN CommandLinePopupKeys OPTIONAL,
    OUT PBOOLEAN EnableScrollMode OPTIONAL,
    OUT PDWORD KeyState OPTIONAL
    )

/*++

Routine Description:

    This routine is used in stream input.  It gets input and filters it
    for unicode characters.

Arguments:

    InputInfo - Pointer to input buffer information.

    Char - Unicode char input.

    Wait - TRUE if the routine shouldn't wait for input.

    Console - Pointer to console buffer information.

    HandleData - Pointer to handle data structure.

    Message - csr api message.

    WaitRoutine - Routine to call when wait is woken up.

    WaitParameter - Parameter to pass to wait routine.

    WaitParameterLength - Length of wait parameter.

    WaitBlockExists - TRUE if wait block has already been created.

    CommandLineEditingKeys - if present, arrow keys will be returned. on
    output, if TRUE, Char contains virtual key code for arrow key.

    CommandLinePopupKeys - if present, arrow keys will be returned. on
    output, if TRUE, Char contains virtual key code for arrow key.

Return Value:

--*/

{
    ULONG NumRead;
    INPUT_RECORD Event;
    NTSTATUS Status;

    if (ARGUMENT_PRESENT(CommandLineEditingKeys)) {
        *CommandLineEditingKeys = FALSE;
    }
    if (ARGUMENT_PRESENT(CommandLinePopupKeys)) {
        *CommandLinePopupKeys = FALSE;
    }
    if (ARGUMENT_PRESENT(EnableScrollMode)) {
        *EnableScrollMode = FALSE;
    }
    if (ARGUMENT_PRESENT(KeyState)) {
        *KeyState = 0;
    }

    NumRead = 1;
    while (TRUE) {
        Status =ReadInputBuffer(InputInfo,
                                 &Event,
                                 &NumRead,
                                 FALSE,
                                 Wait,
                                 TRUE,
                                 Console,
                                 HandleData,
                                 Message,
                                 WaitRoutine,
                                 WaitParameter,
                                 WaitParameterLength,
                                 WaitBlockExists
#if defined(FE_SB)
                                 ,
                                 TRUE
#endif
                                );
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
        if (NumRead == 0) {
            if (Wait) {
                ASSERT (FALSE);
            }
            else {
                return STATUS_UNSUCCESSFUL;
            }
        }
        if (Event.EventType == KEY_EVENT) {
            BOOL fCommandLineEditKey;

            if (ARGUMENT_PRESENT(CommandLineEditingKeys)) {
                fCommandLineEditKey = IsCommandLineEditingKey(&Event.Event.KeyEvent);
            } else if (ARGUMENT_PRESENT(CommandLinePopupKeys)) {
                fCommandLineEditKey = IsCommandLinePopupKey(&Event.Event.KeyEvent);
            } else {
                fCommandLineEditKey = FALSE;
            }

            //
            // Always return keystate if caller asked for it.
            //
            if (ARGUMENT_PRESENT(KeyState)) {
                *KeyState = Event.Event.KeyEvent.dwControlKeyState;
            }

            if (Event.Event.KeyEvent.uChar.UnicodeChar != 0 &&
                    !fCommandLineEditKey) {

                //
                // chars that are generated using alt+numpad
                //
                if (!Event.Event.KeyEvent.bKeyDown &&
                        Event.Event.KeyEvent.wVirtualKeyCode == VK_MENU) {
                    if (Event.Event.KeyEvent.dwControlKeyState & ALTNUMPAD_BIT)
                    {
                        if (CONSOLE_IS_DBCS_CP(Console) && HIBYTE(Event.Event.KeyEvent.uChar.UnicodeChar)) {
                            char chT[2] = {
                                HIBYTE(Event.Event.KeyEvent.uChar.UnicodeChar),
                                LOBYTE(Event.Event.KeyEvent.uChar.UnicodeChar),
                            };
                            *Char = CharToWchar(Console, Console->CP, chT);
                        } else {
                            // Because USER doesn't know our codepage, it gives us the
                            // raw OEM char and we convert it to a Unicode character.
                            char chT = LOBYTE(Event.Event.KeyEvent.uChar.UnicodeChar);
                            UINT uCodePage = Console->CP;

                            //
                            // FarEast hack for High ANSI OEM characters.
                            //
                            if (CONSOLE_IS_DBCS_CP(Console)) {
                                if (IsDbcsExemptionForHighAnsi(uCodePage, chT)) {
                                    /*
                                     * FarEast hack:
                                     * treat characters in High ANSI area as if they are
                                     * the ones of Codepage 1252.
                                     */
                                    uCodePage = 1252;
                                }
                            }
                            *Char = CharToWchar(Console, uCodePage, &chT);
                        }
                    } else {
                        *Char = Event.Event.KeyEvent.uChar.UnicodeChar;
                    }
                    return STATUS_SUCCESS;
                }
                //
                // Ignore Escape and Newline chars
                //
                else if (Event.Event.KeyEvent.bKeyDown &&
                        Event.Event.KeyEvent.wVirtualKeyCode != VK_ESCAPE &&
                        Event.Event.KeyEvent.uChar.UnicodeChar != 0x0a) {

                    *Char = Event.Event.KeyEvent.uChar.UnicodeChar;
                    return STATUS_SUCCESS;
                }
            }

            if (Event.Event.KeyEvent.bKeyDown) {
                SHORT sTmp;
                if (ARGUMENT_PRESENT(CommandLineEditingKeys) &&
                        fCommandLineEditKey) {
                    *CommandLineEditingKeys = TRUE;
                    *Char = (WCHAR) Event.Event.KeyEvent.wVirtualKeyCode;
                    return STATUS_SUCCESS;
                }
                else if (ARGUMENT_PRESENT(CommandLinePopupKeys) &&
                        fCommandLineEditKey) {
                    *CommandLinePopupKeys = TRUE;
                    *Char = (CHAR) Event.Event.KeyEvent.wVirtualKeyCode;
                    return STATUS_SUCCESS;
                }

                sTmp = VkKeyScan(0);

                if ((LOBYTE(sTmp) == Event.Event.KeyEvent.wVirtualKeyCode) &&
                    KEYEVENTSTATE_EQUAL_WINMODS(Event, HIBYTE(sTmp))) {
                    /*
                     * This really is the character 0x0000
                     */
                    *Char = Event.Event.KeyEvent.uChar.UnicodeChar;
                    return STATUS_SUCCESS;
                }
            }
        }
    }
}

BOOLEAN
RawReadWaitRoutine(
    IN PLIST_ENTRY WaitQueue,
    IN PCSR_THREAD WaitingThread,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2,
    IN ULONG WaitFlags
    )

/*++

Routine Description:

    This routine is called to complete a raw read that blocked in
    ReadInputBuffer.  The context of the read was saved in the RawReadData
    structure.  This routine is called when events have been written to
    the input buffer.  It is called in the context of the writing thread.
    ?It will be called at most once per read.?

Arguments:

    WaitQueue - pointer to queue containing wait block

    WaitingThread - pointer to waiting thread

    WaitReplyMessage - Pointer to reply message to return to dll when
        read is completed.

    RawReadData - pointer to data saved in ReadChars

    SatisfyParameter1 - not used

    SatisfyParameter2 - not used

    WaitFlags - Flags indicating status of wait.

Return Value:

--*/

{
    NTSTATUS Status;
    PWCHAR lpBuffer;
    PCONSOLE_READCONSOLE_MSG a;
    PCONSOLE_INFORMATION Console;
    PRAW_READ_DATA RawReadData;
    PHANDLE_DATA HandleData;
    BOOLEAN RetVal = TRUE;
#ifdef FE_SB
    DWORD NumBytes;
    BOOL fAddDbcsLead = FALSE;
#endif

    a = (PCONSOLE_READCONSOLE_MSG)&WaitReplyMessage->u.ApiMessageData;
    RawReadData = (PRAW_READ_DATA)WaitParameter;

    Status = DereferenceIoHandleNoCheck(RawReadData->ProcessData,
                                        RawReadData->HandleIndex,
                                        &HandleData
                                       );
    ASSERT (NT_SUCCESS(Status));

    //
    // see if this routine was called by CloseInputHandle.  if it
    // was, see if this wait block corresponds to the dying handle.
    // if it doesn't, just return.
    //

    if (SatisfyParameter1 != NULL &&
        SatisfyParameter1 != HandleData) {
        return FALSE;
    }
    if ((ULONG_PTR)SatisfyParameter2 & CONSOLE_CTRL_C_SEEN) {
        return FALSE;
    }

    Console = RawReadData->Console;

    //
    // this routine should be called by a thread owning the same
    // lock on the same console as we're reading from.
    //

    a->NumBytes = 0;
#ifdef FE_SB
    NumBytes = 0 ;
#endif
    try {
        LockReadCount(HandleData);
        ASSERT(HandleData->InputReadData->ReadCount);
        HandleData->InputReadData->ReadCount -= 1;
        UnlockReadCount(HandleData);

        //
        // if a ctrl-c is seen, don't terminate read.  if ctrl-break is seen,
        // terminate read.
        //

        if ((ULONG_PTR)SatisfyParameter2 & CONSOLE_CTRL_BREAK_SEEN) {
            WaitReplyMessage->ReturnValue = STATUS_ALERTED;
            leave;
        }

        //
        // see if called by CsrDestroyProcess or CsrDestroyThread
        // via CsrNotifyWaitBlock.   if so, just decrement the ReadCount
        // and return.
        //

        if (WaitFlags & CSR_PROCESS_TERMINATING) {
            Status = STATUS_THREAD_IS_TERMINATING;
            leave;
        }

        //
        // We must see if we were woken up because the handle is being
        // closed.  if so, we decrement the read count.  if it goes to
        // zero, we wake up the close thread.  otherwise, we wake up any
        // other thread waiting for data.
        //

        if (HandleData->InputReadData->InputHandleFlags & HANDLE_CLOSING) {
            ASSERT (SatisfyParameter1 == HandleData);
            Status = STATUS_ALERTED;
            leave;
        }

        //
        // if we get to here, this routine was called either by the input
        // thread or a write routine.  both of these callers grab the
        // current console lock.
        //

        //
        // this routine should be called by a thread owning the same
        // lock on the same console as we're reading from.
        //

        ASSERT (ConsoleLocked(Console));

        if (a->CaptureBufferSize <= BUFFER_SIZE) {
            lpBuffer = a->Buffer;
        }
        else {
            lpBuffer = RawReadData->BufPtr;
        }

        //
        // this call to GetChar may block.
        //

#ifdef FE_SB
        if (!a->Unicode && CONSOLE_IS_DBCS_CP(Console)) {
            if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                fAddDbcsLead = TRUE;
                *lpBuffer = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar;
                RawReadData->BufferSize-=sizeof(WCHAR);
                RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
                Status = STATUS_SUCCESS;
                if (RawReadData->BufferSize == 0) {
                    a->NumBytes = 1;
                    return FALSE;
                }
            }
            else{
                Status = GetChar(RawReadData->InputInfo,
                         lpBuffer,
                         TRUE,
                         Console,
                         HandleData,
                         WaitReplyMessage,
                         RawReadWaitRoutine,
                         RawReadData,
                         sizeof(*RawReadData),
                         TRUE,
                         NULL,
                         NULL,
                         NULL,
                         NULL
                        );
            }
        }
        else
#endif
        Status = GetChar(RawReadData->InputInfo,
                         lpBuffer,
                         TRUE,
                         Console,
                         HandleData,
                         WaitReplyMessage,
                         RawReadWaitRoutine,
                         RawReadData,
                         sizeof(*RawReadData),
                         TRUE,
                         NULL,
                         NULL,
                         NULL,
                         NULL
                        );

        if (!NT_SUCCESS(Status)) {
            if (Status == CONSOLE_STATUS_WAIT) {
                RetVal = FALSE;
            }
            leave;
        }
#ifdef FE_SB
        IsConsoleFullWidth(Console->hDC,
                           Console->CP,*lpBuffer) ? NumBytes+=2 : NumBytes++;
#endif
        lpBuffer++;
        a->NumBytes += sizeof(WCHAR);
        while (a->NumBytes < RawReadData->BufferSize) {

            //
            // this call to GetChar won't block.
            //

            Status = GetChar(RawReadData->InputInfo,lpBuffer,FALSE,NULL,NULL,NULL,NULL,NULL,0,TRUE,NULL,NULL,NULL,NULL);
            if (!NT_SUCCESS(Status)) {
                Status = STATUS_SUCCESS;
                break;
            }
#ifdef FE_SB
            IsConsoleFullWidth(Console->hDC,
                               Console->CP,*lpBuffer) ? NumBytes+=2 : NumBytes++;
#endif
            lpBuffer++;
            a->NumBytes += sizeof(WCHAR);
        }
    } finally {

        //
        // if the read was completed (status != wait), free the raw read
        // data.
        //

        if (Status != CONSOLE_STATUS_WAIT) {
            if (!a->Unicode) {

                //
                // if ansi, translate string.
                //

                PCHAR TransBuffer;

#ifdef FE_SB
                TransBuffer = (PCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),NumBytes);
#else
                TransBuffer = (PCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),a->NumBytes / sizeof(WCHAR));
#endif
                if (TransBuffer == NULL) {
                    return TRUE;
                }

                if (a->CaptureBufferSize <= BUFFER_SIZE) {
                    lpBuffer = a->Buffer;
                }
                else {
                    lpBuffer = RawReadData->BufPtr;
                }

#ifdef FE_SB
                if (CONSOLE_IS_DBCS_CP(Console))
                {
                    a->NumBytes = TranslateUnicodeToOem(Console,
                                                        lpBuffer,
                                                        a->NumBytes / sizeof (WCHAR),
                                                        TransBuffer,
                                                        NumBytes,
                                                        &HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte);
                }
                else
#endif
                a->NumBytes = ConvertToOem(RawReadData->Console->CP,
                                     lpBuffer,
                                     a->NumBytes / sizeof (WCHAR),
                                     TransBuffer,
                                     a->NumBytes / sizeof (WCHAR)
                                     );
                RtlCopyMemory(lpBuffer,TransBuffer,a->NumBytes);
#ifdef FE_SB
                if (fAddDbcsLead)
                    a->NumBytes++;
#endif
                ConsoleHeapFree(TransBuffer);
            }
            WaitReplyMessage->ReturnValue = Status;
            ConsoleHeapFree(RawReadData);
        }
    }
    return RetVal;

    //
    // satisfy the unreferenced parameter warnings.
    //

    UNREFERENCED_PARAMETER(WaitQueue);
    UNREFERENCED_PARAMETER(WaitingThread);
}

ULONG
RetrieveTotalNumberOfSpaces(
    IN SHORT OriginalCursorPositionX,
    IN PWCHAR Buffer,
    IN ULONG CurrentPosition
#if defined(FE_SB)
    ,
    IN PCONSOLE_INFORMATION Console
#endif
    )

/*++

    This routine returns the total number of screen spaces the characters
    up to the specified character take up.

--*/

{
    WCHAR Char;
    ULONG i,NumSpacesForChar,NumSpaces;
    SHORT XPosition;

    XPosition=OriginalCursorPositionX;
    NumSpaces=0;
    for (i=0;i<CurrentPosition;i++) {
        Char = Buffer[i];
        if (Char == UNICODE_TAB) {
            NumSpacesForChar = NUMBER_OF_SPACES_IN_TAB(XPosition);
        } else if (IS_CONTROL_CHAR(Char)) {
            NumSpacesForChar = 2;
#if defined(FE_SB)
        } else if (IsConsoleFullWidth(Console->hDC,Console->CP,Char)) {
            NumSpacesForChar = 2;
#endif
        } else {
            NumSpacesForChar = 1;
        }
        XPosition = (SHORT)(XPosition+NumSpacesForChar);
        NumSpaces += NumSpacesForChar;
    }
    return NumSpaces;
}

ULONG
RetrieveNumberOfSpaces(
    IN SHORT OriginalCursorPositionX,
    IN PWCHAR Buffer,
    IN ULONG CurrentPosition
#if defined(FE_SB)
    ,
    IN PCONSOLE_INFORMATION Console,
    IN DWORD CodePage
#endif
    )

/*++

    This routine returns the number of screen spaces the specified character
    takes up.

--*/

{
    WCHAR Char;
    ULONG i,NumSpaces;
    SHORT XPosition;

    Char = Buffer[CurrentPosition];
    if (Char == UNICODE_TAB) {
        NumSpaces=0;
        XPosition=OriginalCursorPositionX;
        for (i=0;i<=CurrentPosition;i++) {
            Char = Buffer[i];
            if (Char == UNICODE_TAB) {
                NumSpaces = NUMBER_OF_SPACES_IN_TAB(XPosition);
            } else if (IS_CONTROL_CHAR(Char)) {
                NumSpaces = 2;
#if defined(FE_SB)
            } else if (IsConsoleFullWidth(Console->hDC,CodePage,Char)) {
                NumSpaces = 2;
#endif
            } else {
                NumSpaces = 1;
            }
            XPosition = (SHORT)(XPosition+NumSpaces);
        }
        return NumSpaces;
    }
    else if (IS_CONTROL_CHAR(Char)) {
        return 2;
    }
#if defined(FE_SB)
    else if (IsConsoleFullWidth(Console->hDC,CodePage,Char)) {
        return 2;
    }
#endif
    else {
        return 1;
    }
}

BOOL
ProcessCookedReadInput(
    IN PCOOKED_READ_DATA CookedReadData,
    IN WCHAR Char,
    IN DWORD KeyState,
    OUT PNTSTATUS Status
    )

/*++

    Return Value:

        TRUE if read is completed
--*/

{
    DWORD NumSpaces;
    SHORT ScrollY=0;
    ULONG NumToWrite;
    WCHAR wchOrig = Char;
    BOOL fStartFromDelim;

    *Status = STATUS_SUCCESS;
    if (CookedReadData->BytesRead >= (CookedReadData->BufferSize-(2*sizeof(WCHAR))) &&
        Char != UNICODE_CARRIAGERETURN &&
        Char != UNICODE_BACKSPACE) {
        return FALSE;
    }

    if (CookedReadData->CtrlWakeupMask != 0 &&
        Char < L' ' && (CookedReadData->CtrlWakeupMask & (1 << Char))) {
        *CookedReadData->BufPtr = Char;
        CookedReadData->BytesRead += sizeof(WCHAR);
        CookedReadData->BufPtr+=1;
        CookedReadData->CurrentPosition+=1;
        CookedReadData->ControlKeyState = KeyState;
        return TRUE;
    }


    if (Char == EXTKEY_ERASE_PREV_WORD) {
        Char = UNICODE_BACKSPACE;

    }
    if (AT_EOL(CookedReadData)) {

        //
        // if at end of line, processing is relatively simple. just store the
        // character and write it to the screen.
        //


        if (Char == UNICODE_BACKSPACE2) {
            Char = UNICODE_BACKSPACE;
        }
        if (Char != UNICODE_BACKSPACE ||
            CookedReadData->BufPtr != CookedReadData->BackupLimit) {

            fStartFromDelim = gExtendedEditKey && IS_WORD_DELIM(CookedReadData->BufPtr[-1]);

eol_repeat:
            if (CookedReadData->Echo) {
                NumToWrite=sizeof(WCHAR);
                *Status = WriteCharsFromInput(CookedReadData->ScreenInfo,
                                    CookedReadData->BackupLimit,
                                    CookedReadData->BufPtr,
                                    &Char,
                                    &NumToWrite,
                                    (PLONG)&NumSpaces,
                                    CookedReadData->OriginalCursorPosition.X,
                                    WC_DESTRUCTIVE_BACKSPACE |
                                            WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                    &ScrollY
                                    );
                if (NT_SUCCESS(*Status)) {
                    CookedReadData->OriginalCursorPosition.Y += ScrollY;
                } else {
                    RIPMSG1(RIP_WARNING, "WriteCharsFromInput failed %x", *Status);
                }
            }
            CookedReadData->NumberOfVisibleChars += NumSpaces;
            if (Char == UNICODE_BACKSPACE && CookedReadData->Processed) {
                CookedReadData->BytesRead -= sizeof(WCHAR);
                *CookedReadData->BufPtr=(WCHAR)' ';
                CookedReadData->BufPtr-=1;
                CookedReadData->CurrentPosition-=1;

                // Repeat until it hits the word boundary
                if (gExtendedEditKey &&
                        wchOrig == EXTKEY_ERASE_PREV_WORD &&
                        CookedReadData->BufPtr != CookedReadData->BackupLimit &&
                        fStartFromDelim ^ !IS_WORD_DELIM(CookedReadData->BufPtr[-1])) {
                    goto eol_repeat;
                }
            }
            else {
                *CookedReadData->BufPtr = Char;
                CookedReadData->BytesRead += sizeof(WCHAR);
                CookedReadData->BufPtr+=1;
                CookedReadData->CurrentPosition+=1;
            }
        }
    } else {
        BOOL CallWrite=TRUE;

        //
        // processing in the middle of the line is more complex:
        //
        //
        // calculate new cursor position
        // store new char
        // clear the current command line from the screen
        // write the new command line to the screen
        // update the cursor position
        //

        if (Char == UNICODE_BACKSPACE && CookedReadData->Processed) {

            //
            // for backspace, use writechars to calculate the new cursor position.
            // this call also sets the cursor to the right position for the
            // second call to writechars.
            //
            if (CookedReadData->BufPtr != CookedReadData->BackupLimit) {

                fStartFromDelim = gExtendedEditKey && IS_WORD_DELIM(CookedReadData->BufPtr[-1]);

bs_repeat:

                //
                // we call writechar here so that cursor position gets updated
                // correctly.  we also call it later if we're not at eol so
                // that the remainder of the string can be updated correctly.
                //

                if (CookedReadData->Echo) {
                    NumToWrite=sizeof(WCHAR);
                    *Status = WriteCharsFromInput(CookedReadData->ScreenInfo,
                             CookedReadData->BackupLimit,
                             CookedReadData->BufPtr,
                             &Char,
                             &NumToWrite,
                             NULL,
                             CookedReadData->OriginalCursorPosition.X,
                             WC_DESTRUCTIVE_BACKSPACE |
                                     WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                             NULL);

                    if (!NT_SUCCESS(*Status)) {
                        RIPMSG1(RIP_WARNING, "WriteCharsFromInput failed %x", *Status);
                    }
                }
                CookedReadData->BytesRead -= sizeof(WCHAR);
                CookedReadData->BufPtr-=1;
                CookedReadData->CurrentPosition-=1;
                RtlCopyMemory(CookedReadData->BufPtr,
                       CookedReadData->BufPtr+1,
                       CookedReadData->BytesRead - (CookedReadData->CurrentPosition * sizeof(WCHAR))
                      );
#if defined(FE_SB)
                {
                    PWCHAR buf = (PWCHAR)((PBYTE)CookedReadData->BackupLimit +
                                                 CookedReadData->BytesRead    );
                    *buf = (WCHAR)' ';
                }
#endif
                NumSpaces = 0;

                // Repeat until it hits the word boundary
                if (gExtendedEditKey &&
                        wchOrig == EXTKEY_ERASE_PREV_WORD &&
                        CookedReadData->BufPtr != CookedReadData->BackupLimit &&
                        fStartFromDelim ^ !IS_WORD_DELIM(CookedReadData->BufPtr[-1])) {
                    goto bs_repeat;
                }
            } else {
                 CallWrite = FALSE;
            }
        } else {

            //
            // store the char
            //

            if (Char == UNICODE_CARRIAGERETURN) {
                CookedReadData->BufPtr = (PWCHAR)((PBYTE)CookedReadData->BackupLimit + CookedReadData->BytesRead);
                *CookedReadData->BufPtr = Char;
                CookedReadData->BufPtr+=1;
                CookedReadData->BytesRead += sizeof(WCHAR);
                CookedReadData->CurrentPosition += 1;
            } else {
#if defined(FE_SB)
                BOOL fBisect = FALSE;
                if (CookedReadData->Echo) {
                    if (CheckBisectProcessW(CookedReadData->ScreenInfo,
                                            CookedReadData->ScreenInfo->Console->CP,
                                            CookedReadData->BackupLimit,
                                            CookedReadData->CurrentPosition+1,
                                            CookedReadData->ScreenInfo->ScreenBufferSize.X
                                             -CookedReadData->OriginalCursorPosition.X,
                                            CookedReadData->OriginalCursorPosition.X,
                                            TRUE)) {
                        fBisect = TRUE;
                    }
                }
#endif
                if (INSERT_MODE(CookedReadData)) {
                    memmove(CookedReadData->BufPtr+1,
                            CookedReadData->BufPtr,
                            CookedReadData->BytesRead - (CookedReadData->CurrentPosition * sizeof(WCHAR))
                           );
                    CookedReadData->BytesRead += sizeof(WCHAR);
                }
                *CookedReadData->BufPtr = Char;
                CookedReadData->BufPtr+=1;
                CookedReadData->CurrentPosition += 1;

                //
                // calculate new cursor position
                //

                if (CookedReadData->Echo) {
                    NumSpaces = RetrieveNumberOfSpaces(CookedReadData->OriginalCursorPosition.X,
                                                       CookedReadData->BackupLimit,
                                                       CookedReadData->CurrentPosition-1
#if defined(FE_SB)
                                                       ,
                                                       CookedReadData->ScreenInfo->Console,
                                                       CookedReadData->ScreenInfo->Console->CP
#endif
                                                      );
#if defined(FE_SB)
                    if (NumSpaces > 0 && fBisect)
                        NumSpaces--;
#endif
                }
            }
        }

        if (CookedReadData->Echo && CallWrite) {

            COORD CursorPosition;

            //
            // save cursor position
            //

            CursorPosition = CookedReadData->ScreenInfo->BufferInfo.TextInfo.CursorPosition;
            CursorPosition.X = (SHORT)(CursorPosition.X+NumSpaces);

            //
            // clear the current command line from the screen
            //

            DeleteCommandLine(CookedReadData,
                              FALSE);

            //
            // write the new command line to the screen
            //

            NumToWrite = CookedReadData->BytesRead;
            *Status = WriteCharsFromInput(CookedReadData->ScreenInfo,
                                CookedReadData->BackupLimit,
                                CookedReadData->BackupLimit,
                                CookedReadData->BackupLimit,
                                &NumToWrite,
                                (PLONG)&CookedReadData->NumberOfVisibleChars,
                                CookedReadData->OriginalCursorPosition.X,
                                (Char != UNICODE_CARRIAGERETURN) ?
                                     WC_DESTRUCTIVE_BACKSPACE | WC_ECHO :
                                     WC_DESTRUCTIVE_BACKSPACE | WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                                &ScrollY
                                );
            if (!NT_SUCCESS(*Status)) {
                RIPMSG1(RIP_WARNING, "WriteCharsFromInput failed %x", *Status);
                CookedReadData->BytesRead = 0;
                return TRUE;
            }

            //
            // update cursor position
            //

            if (Char != UNICODE_CARRIAGERETURN) {
#if defined(FE_SB)
                if (CheckBisectProcessW(CookedReadData->ScreenInfo,
                                        CookedReadData->ScreenInfo->Console->CP,
                                        CookedReadData->BackupLimit,
                                        CookedReadData->CurrentPosition+1,
                                        CookedReadData->ScreenInfo->ScreenBufferSize.X
                                         -CookedReadData->OriginalCursorPosition.X,
                                        CookedReadData->OriginalCursorPosition.X,
                                        TRUE)) {
                    if (CursorPosition.X == (CookedReadData->ScreenInfo->ScreenBufferSize.X-1))
                        CursorPosition.X++;
                }
#endif

                // adjust cursor position for WriteChars
                CookedReadData->OriginalCursorPosition.Y += ScrollY;
                CursorPosition.Y += ScrollY;
                *Status = AdjustCursorPosition(CookedReadData->ScreenInfo,
                                              CursorPosition,
                                              TRUE,
                                              NULL);
                ASSERT(NT_SUCCESS(*Status));
                if (!NT_SUCCESS(*Status)) {
                    CookedReadData->BytesRead = 0;
                    return TRUE;
                }
            }
        }
    }

    //
    // in cooked mode, enter (carriage return) is converted to
    // carriage return linefeed (0xda).  carriage return is always
    // stored at the end of the buffer.
    //

    if (Char == UNICODE_CARRIAGERETURN) {
        if (CookedReadData->Processed) {
            if (CookedReadData->BytesRead < CookedReadData->BufferSize) {
                *CookedReadData->BufPtr = UNICODE_LINEFEED;
                if (CookedReadData->Echo) {
                    NumToWrite=sizeof(WCHAR);
                    *Status = WriteCharsFromInput(CookedReadData->ScreenInfo,
                             CookedReadData->BackupLimit,
                             CookedReadData->BufPtr,
                             CookedReadData->BufPtr,
                             &NumToWrite,
                             NULL,
                             CookedReadData->OriginalCursorPosition.X,
                             WC_DESTRUCTIVE_BACKSPACE |
                                     WC_KEEP_CURSOR_VISIBLE | WC_ECHO,
                             NULL);

                    if (!NT_SUCCESS(*Status)) {
                        RIPMSG1(RIP_WARNING, "WriteCharsFromInput failed %x", *Status);
                    }
                }
                CookedReadData->BytesRead += sizeof(WCHAR);
                CookedReadData->BufPtr++;
                CookedReadData->CurrentPosition += 1;
            }
        }
        //
        // reset the cursor back to 25% if necessary
        //
        if (CookedReadData->Line) {
            if (CookedReadData->InsertMode != CookedReadData->Console->InsertMode) {
                ProcessCommandLine(CookedReadData,VK_INSERT,0,NULL,NULL,FALSE); // make cursor small
            }
            *Status = STATUS_SUCCESS;
            return TRUE;
        }
    }
    return FALSE;
}

NTSTATUS
CookedRead(
    IN PCOOKED_READ_DATA CookedReadData,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PCSR_THREAD WaitingThread,
    IN BOOLEAN WaitRoutine
    )
{
    WCHAR Char;
    BOOLEAN CommandLineEditingKeys,EnableScrollMode;
    DWORD KeyState;
    NTSTATUS Status=STATUS_SUCCESS;
    PCONSOLE_READCONSOLE_MSG a;
    PHANDLE_DATA HandleData;
#ifdef FE_SB
    DWORD NumBytes;
    ULONG NumToWrite;
    BOOL fAddDbcsLead = FALSE;
#endif

    Status = DereferenceIoHandleNoCheck(CookedReadData->ProcessData,
                                        CookedReadData->HandleIndex,
                                        &HandleData
                                       );
    ASSERT (NT_SUCCESS(Status));
    a = (PCONSOLE_READCONSOLE_MSG)&WaitReplyMessage->u.ApiMessageData;
    while (CookedReadData->BytesRead < CookedReadData->BufferSize) {

        //
        // this call to GetChar may block.
        //

        Status = GetChar(CookedReadData->InputInfo,
                         &Char,
                         TRUE,
                         CookedReadData->Console,
                         HandleData,
                         WaitReplyMessage,
                         CookedReadWaitRoutine,
                         CookedReadData,
                         sizeof(*CookedReadData),
                         WaitRoutine,
                         &CommandLineEditingKeys,
                         NULL,
                         &EnableScrollMode,
                         &KeyState
                        );
        if (!NT_SUCCESS(Status)) {
            if (Status != CONSOLE_STATUS_WAIT) {
                CookedReadData->BytesRead = 0;
            }
            break;
        }

        //
        // we should probably set these up in GetChars, but we set them
        // up here because the debugger is multi-threaded and calls
        // read before outputting the prompt.
        //

        if (CookedReadData->OriginalCursorPosition.X == -1) {
            CookedReadData->OriginalCursorPosition = CookedReadData->ScreenInfo->BufferInfo.TextInfo.CursorPosition;
        }

        if (CommandLineEditingKeys) {
            Status = ProcessCommandLine(CookedReadData,Char,KeyState,WaitReplyMessage,WaitingThread,WaitRoutine);
            if (Status == CONSOLE_STATUS_READ_COMPLETE ||
                Status == CONSOLE_STATUS_WAIT) {
                break;
            }
            if (!NT_SUCCESS(Status)) {
                if (Status == CONSOLE_STATUS_WAIT_NO_BLOCK) {
                    Status = CONSOLE_STATUS_WAIT;
                    if (!WaitRoutine) {
                        //
                        // we have no wait block, so create one.
                        //
                        WaitForMoreToRead(CookedReadData->InputInfo,
                                          WaitReplyMessage,
                                          CookedReadWaitRoutine,
                                          CookedReadData,
                                          sizeof(*CookedReadData),
                                          FALSE
                                         );
                    }
                } else {
                    CookedReadData->BytesRead = 0;
                }
                break;
            }
        } else {
            if (ProcessCookedReadInput(CookedReadData,
                                       Char,
                                       KeyState,
                                       &Status
                                      )) {
                CookedReadData->Console->Flags |= CONSOLE_IGNORE_NEXT_KEYUP;
                break;
            }
        }
    }

    //
    // if the read was completed (status != wait), free the cooked read
    // data.  also, close the temporary output handle that was opened to
    // echo the characters read.
    //

    if (Status != CONSOLE_STATUS_WAIT) {

        DWORD LineCount=1;
        if (CookedReadData->Echo) {
            BOOLEAN FoundCR;
            ULONG i,StringLength;
            PWCHAR StringPtr;

            // figure out where real string ends (at carriage return
            // or end of buffer)

            StringPtr = CookedReadData->BackupLimit;
            StringLength = CookedReadData->BytesRead;
            FoundCR = FALSE;
            for (i=0;i<(CookedReadData->BytesRead/sizeof(WCHAR));i++) {
                if (*StringPtr++ == UNICODE_CARRIAGERETURN) {
                    StringLength = i*sizeof(WCHAR);
                    FoundCR = TRUE;
                    break;
                }
            }

            if (FoundCR) {
                //
                // add to command line recall list
                //

                AddCommand(CookedReadData->CommandHistory,CookedReadData->BackupLimit,(USHORT)StringLength,CookedReadData->Console->Flags & CONSOLE_HISTORY_NODUP);

                //
                // check for alias
                //

                i = CookedReadData->BufferSize;
                if (NT_SUCCESS(MatchandCopyAlias(CookedReadData->Console,
                                                 CookedReadData->BackupLimit,
                                                 (USHORT)StringLength,
                                                 CookedReadData->BackupLimit,
                                                 (PUSHORT)&i,
                                                 CookedReadData->ExeName,
                                                 CookedReadData->ExeNameLength,
                                                 &LineCount
                                                ))) {
                  CookedReadData->BytesRead = i;
                }
            }

            CloseOutputHandle(CONSOLE_FROMTHREADPERPROCESSDATA(WaitingThread),
                              CookedReadData->Console,
                              &CookedReadData->TempHandle,
                              NULL,
                              FALSE
                             );
        }
        WaitReplyMessage->ReturnValue = Status;

        //
        // at this point, a->NumBytes contains the number of bytes in
        // the UNICODE string read.  UserBufferSize contains the converted
        // size of the app's buffer.
        //

        if (CookedReadData->BytesRead > CookedReadData->UserBufferSize || LineCount > 1) {
            if (LineCount > 1) {
                PWSTR Tmp;
                HandleData->InputReadData->InputHandleFlags |= HANDLE_MULTI_LINE_INPUT;
#ifdef FE_SB
                if (!a->Unicode && CONSOLE_IS_DBCS_CP(CookedReadData->Console)) {
                    if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                        fAddDbcsLead = TRUE;
                        *CookedReadData->UserBuffer++ = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar;
                        CookedReadData->UserBufferSize-=sizeof(WCHAR);
                        RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
                    }
                    NumBytes = 0;
                    for (Tmp=CookedReadData->BackupLimit;
                         *Tmp!=UNICODE_LINEFEED && CookedReadData->UserBufferSize/sizeof(WCHAR) > NumBytes;
                         (IsConsoleFullWidth(CookedReadData->Console->hDC,
                                             CookedReadData->Console->CP,*Tmp) ? NumBytes+=2 : NumBytes++),Tmp++) ;
                }
#endif
                for (Tmp=CookedReadData->BackupLimit;*Tmp!=UNICODE_LINEFEED;Tmp++)
                    ASSERT(Tmp<(CookedReadData->BackupLimit+CookedReadData->BytesRead));
                a->NumBytes = (ULONG)(Tmp-CookedReadData->BackupLimit+1)*sizeof(*Tmp);
            } else {
#ifdef FE_SB
                if (!a->Unicode && CONSOLE_IS_DBCS_CP(CookedReadData->Console)) {
                    PWSTR Tmp;

                    if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                        fAddDbcsLead = TRUE;
                        *CookedReadData->UserBuffer++ = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar;
                        CookedReadData->UserBufferSize-=sizeof(WCHAR);
                        RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
                    }
                    NumBytes = 0;
                    NumToWrite = CookedReadData->BytesRead;
                    for (Tmp=CookedReadData->BackupLimit;
                         NumToWrite && CookedReadData->UserBufferSize/sizeof(WCHAR) > NumBytes;
                         (IsConsoleFullWidth(CookedReadData->Console->hDC,
                                             CookedReadData->Console->CP,*Tmp) ? NumBytes+=2 : NumBytes++),Tmp++,NumToWrite-=sizeof(WCHAR)) ;
                }
#endif
                a->NumBytes = CookedReadData->UserBufferSize;
            }
            HandleData->InputReadData->InputHandleFlags |= HANDLE_INPUT_PENDING;
            HandleData->InputReadData->BufPtr = CookedReadData->BackupLimit;
            HandleData->InputReadData->BytesAvailable = CookedReadData->BytesRead - a->NumBytes;
            HandleData->InputReadData->CurrentBufPtr=(PWCHAR)((PBYTE)CookedReadData->BackupLimit+a->NumBytes);
            RtlCopyMemory(CookedReadData->UserBuffer,CookedReadData->BackupLimit,a->NumBytes);
        }
        else {
#ifdef FE_SB
            if (!a->Unicode && CONSOLE_IS_DBCS_CP(CookedReadData->Console)) {
                PWSTR Tmp;

                if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                    fAddDbcsLead = TRUE;
                    *CookedReadData->UserBuffer++ = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar;
                    CookedReadData->UserBufferSize-=sizeof(WCHAR);
                    RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));

                    if (CookedReadData->UserBufferSize == 0) {
                        a->NumBytes = 1;
                        ConsoleHeapFree(CookedReadData->BackupLimit);
                        return STATUS_SUCCESS;
                    }
                }
                NumBytes = 0;
                NumToWrite = CookedReadData->BytesRead;
                for (Tmp=CookedReadData->BackupLimit;
                     NumToWrite && CookedReadData->UserBufferSize/sizeof(WCHAR) > NumBytes;
                     (IsConsoleFullWidth(CookedReadData->Console->hDC,
                                         CookedReadData->Console->CP,*Tmp) ? NumBytes+=2 : NumBytes++),Tmp++,NumToWrite-=sizeof(WCHAR)) ;
            }
#endif
            a->NumBytes = CookedReadData->BytesRead;
            RtlCopyMemory(CookedReadData->UserBuffer,CookedReadData->BackupLimit,a->NumBytes);
            ConsoleHeapFree(CookedReadData->BackupLimit);
        }
        a->ControlKeyState = CookedReadData->ControlKeyState;

        if (!a->Unicode) {

            //
            // if ansi, translate string.
            //

            PCHAR TransBuffer;

#ifdef FE_SB
            if (CONSOLE_IS_DBCS_CP(CookedReadData->Console))
                TransBuffer = (PCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),NumBytes);
            else
#endif
            TransBuffer = (PCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),a->NumBytes / sizeof(WCHAR));
            if (TransBuffer == NULL) {
                return STATUS_NO_MEMORY;
            }

#ifdef FE_SB
            if (CONSOLE_IS_DBCS_CP(CookedReadData->Console))
            {
                a->NumBytes = TranslateUnicodeToOem(CookedReadData->Console,
                                                    CookedReadData->UserBuffer,
                                                    a->NumBytes / sizeof (WCHAR),
                                                    TransBuffer,
                                                    NumBytes,
                                                    &HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte);
            }
            else
#endif
            a->NumBytes = ConvertToOem(CookedReadData->Console->CP,
                                 CookedReadData->UserBuffer,
                                 a->NumBytes / sizeof (WCHAR),
                                 TransBuffer,
                                 a->NumBytes / sizeof (WCHAR)
                                 );
            RtlCopyMemory(CookedReadData->UserBuffer,TransBuffer,a->NumBytes);
#ifdef FE_SB
            if (fAddDbcsLead)
                a->NumBytes++;
#endif
            ConsoleHeapFree(TransBuffer);
        }
        ConsoleHeapFree(CookedReadData->ExeName);
        if (WaitRoutine) {
#ifdef FE_SB
            CookedReadData->Console->lpCookedReadData = NULL;
#endif
            ConsoleHeapFree(CookedReadData);
        }
    }
    return Status;
}

BOOLEAN
CookedReadWaitRoutine(
    IN PLIST_ENTRY WaitQueue,
    IN PCSR_THREAD WaitingThread,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2,
    IN ULONG WaitFlags
    )

/*++

Routine Description:

    This routine is called to complete a cooked read that blocked in
    ReadInputBuffer.  The context of the read was saved in the CookedReadData
    structure.  This routine is called when events have been written to
    the input buffer.  It is called in the context of the writing thread.
    It may be called more than once.

Arguments:

    WaitQueue - pointer to queue containing wait block

    WaitingThread - pointer to waiting thread

    WaitReplyMessage - pointer to reply message

    CookedReadData - pointer to data saved in ReadChars

    SatisfyParameter1 - if this routine was called (indirectly) by
    CloseInputHandle, this argument contains a HandleData pointer of
    the dying handle.  otherwise, it contains NULL.

    SatisfyParameter2 - if this routine is called because a ctrl-c or
    ctrl-break was seen, this argument contains CONSOLE_CTRL_SEEN.
    otherwise it contains NULL.

    WaitFlags - Flags indicating status of wait.

Return Value:

--*/


{
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PCOOKED_READ_DATA CookedReadData;
    PCONSOLE_READCONSOLE_MSG a;
    PHANDLE_DATA HandleData;

    a = (PCONSOLE_READCONSOLE_MSG)&WaitReplyMessage->u.ApiMessageData;
    CookedReadData = (PCOOKED_READ_DATA)WaitParameter;

    Status = DereferenceIoHandleNoCheck(CookedReadData->ProcessData,
                                        CookedReadData->HandleIndex,
                                        &HandleData
                                       );
    ASSERT (NT_SUCCESS(Status));
    ASSERT(!(HandleData->InputReadData->InputHandleFlags & HANDLE_INPUT_PENDING));

    //
    // see if this routine was called by CloseInputHandle.  if it
    // was, see if this wait block corresponds to the dying handle.
    // if it doesn't, just return.
    //

    if (SatisfyParameter1 != NULL &&
        SatisfyParameter1 != HandleData) {
        //DbgPrint("CookedReadWaitRoutine exit 1\n");
        return FALSE;
    }

    Console = CookedReadData->Console;

    //
    // this routine should be called by a thread owning the same
    // lock on the same console as we're reading from.
    //

    LockReadCount(HandleData);
    ASSERT(HandleData->InputReadData->ReadCount);
    HandleData->InputReadData->ReadCount -= 1;
    UnlockReadCount(HandleData);

    //
    // if ctrl-c or ctrl-break was seen, terminate read.
    //

    if ((ULONG_PTR)SatisfyParameter2 & (CONSOLE_CTRL_C_SEEN | CONSOLE_CTRL_BREAK_SEEN)) {
        if (CookedReadData->Echo) {
            CloseOutputHandle(CONSOLE_FROMTHREADPERPROCESSDATA(WaitingThread),
                              CookedReadData->Console,
                              &CookedReadData->TempHandle,
                              NULL,
                              FALSE
                             );
        }
        //DbgPrint("CookedReadWaitRoutine exit 2\n");
        WaitReplyMessage->ReturnValue = STATUS_ALERTED;
        ConsoleHeapFree(CookedReadData->BackupLimit);
        ConsoleHeapFree(CookedReadData->ExeName);
#if defined(FE_SB)
        CookedReadData->Console->lpCookedReadData = NULL;
#endif
        ConsoleHeapFree(CookedReadData);
        return TRUE;
    }

    //
    // see if called by CsrDestroyProcess or CsrDestroyThread
    // via CsrNotifyWaitBlock.   if so, just decrement the ReadCount
    // and return.
    //

    if (WaitFlags & CSR_PROCESS_TERMINATING) {
        if (CookedReadData->Echo) {
            CloseOutputHandle(CONSOLE_FROMTHREADPERPROCESSDATA(WaitingThread),
                              CookedReadData->Console,
                              &CookedReadData->TempHandle,
                              NULL,
                              FALSE
                             );
        }
        //DbgPrint("CookedReadWaitRoutine exit 3\n");
        WaitReplyMessage->ReturnValue = (ULONG)STATUS_THREAD_IS_TERMINATING;

        //
        // clean up popup data structures
        //

        CleanUpPopups(CookedReadData);
        ConsoleHeapFree(CookedReadData->BackupLimit);
        ConsoleHeapFree(CookedReadData->ExeName);
#if defined(FE_SB)
        CookedReadData->Console->lpCookedReadData = NULL;
#endif
        ConsoleHeapFree(CookedReadData);
        return TRUE;
    }

    //
    // We must see if we were woken up because the handle is being
    // closed.  if so, we decrement the read count.  if it goes to
    // zero, we wake up the close thread.  otherwise, we wake up any
    // other thread waiting for data.
    //

    if (HandleData->InputReadData->InputHandleFlags & HANDLE_CLOSING) {
        ASSERT (SatisfyParameter1 == HandleData);
        if (CookedReadData->Echo) {
            CloseOutputHandle(CONSOLE_FROMTHREADPERPROCESSDATA(WaitingThread),
                              CookedReadData->Console,
                              &CookedReadData->TempHandle,
                              NULL,
                              FALSE
                             );
        }
        //DbgPrint("CookedReadWaitRoutine exit 4\n");
        WaitReplyMessage->ReturnValue = STATUS_ALERTED;

        //
        // clean up popup data structures
        //

        CleanUpPopups(CookedReadData);
        ConsoleHeapFree(CookedReadData->BackupLimit);
        ConsoleHeapFree(CookedReadData->ExeName);
#if defined(FE_SB)
        CookedReadData->Console->lpCookedReadData = NULL;
#endif
        ConsoleHeapFree(CookedReadData);
        return TRUE;
    }

    //
    // if we get to here, this routine was called either by the input
    // thread or a write routine.  both of these callers grab the
    // current console lock.
    //

    //
    // this routine should be called by a thread owning the same
    // lock on the same console as we're reading from.
    //

    ASSERT (ConsoleLocked(Console));

    if (CookedReadData->CommandHistory) {
        PCLE_POPUP Popup;
        if (!CLE_NO_POPUPS(CookedReadData->CommandHistory)) {
            Popup = CONTAINING_RECORD( CookedReadData->CommandHistory->PopupList.Flink, CLE_POPUP, ListLink );
            Status = (Popup->PopupInputRoutine)(CookedReadData,
                                                WaitReplyMessage,
                                                WaitingThread,
                                                TRUE);
            if (Status == CONSOLE_STATUS_READ_COMPLETE ||
                (Status != CONSOLE_STATUS_WAIT &&
                 Status != CONSOLE_STATUS_WAIT_NO_BLOCK) ) {
                ConsoleHeapFree(CookedReadData->BackupLimit);
                ConsoleHeapFree(CookedReadData->ExeName);
#if defined(FE_SB)
                CookedReadData->Console->lpCookedReadData = NULL;
#endif
                ConsoleHeapFree(CookedReadData);
                return TRUE;
            }
            return FALSE;
        }
    }
    if (a->CaptureBufferSize <= BUFFER_SIZE &&
        CookedReadData->BytesRead == 0) {
        CookedReadData->UserBuffer = a->Buffer;
    }
    Status = CookedRead(CookedReadData,
                        WaitReplyMessage,
                        WaitingThread,
                        TRUE
                       );

    if (Status != CONSOLE_STATUS_WAIT) {
        return TRUE;
    } else {
        return FALSE;
    }

    //
    // satisfy the unreferenced parameter warnings.
    //

    UNREFERENCED_PARAMETER(WaitQueue);
    UNREFERENCED_PARAMETER(SatisfyParameter2);
}


NTSTATUS
ReadChars(
    IN PINPUT_INFORMATION InputInfo,
    IN PCONSOLE_INFORMATION Console,
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PSCREEN_INFORMATION ScreenInfo,
    IN OUT PWCHAR lpBuffer,
    IN OUT PDWORD NumBytes,
    IN DWORD InitialNumBytes,
    IN DWORD CtrlWakeupMask,
    IN PHANDLE_DATA HandleData,
    IN PCOMMAND_HISTORY CommandHistory,
    IN PCSR_API_MSG Message OPTIONAL,
    IN HANDLE HandleIndex,
    IN USHORT ExeNameLength,
    IN PWCHAR ExeName,
    IN BOOLEAN Unicode
    )

/*++

Routine Description:

    This routine reads in characters for stream input and does the
    required processing based on the input mode (line,char,echo).
    This routine returns UNICODE characters.

Arguments:

    InputInfo - Pointer to input buffer information.

    Console - Pointer to console buffer information.

    ScreenInfo - Pointer to screen buffer information.

    lpBuffer - Pointer to buffer to read into.

    NumBytes - On input, size of buffer.  On output, number of bytes
    read.

    HandleData - Pointer to handle data structure.

Return Value:

--*/

{
    DWORD BufferSize;
    NTSTATUS Status;
    HANDLE_DATA TempHandle;
    BOOLEAN Echo = FALSE;
    ULONG NumToWrite;
#ifdef FE_SB
    PCONSOLE_READCONSOLE_MSG a = (PCONSOLE_READCONSOLE_MSG)&Message->u.ApiMessageData;
    BOOL fAddDbcsLead = FALSE;
    ULONG NumToBytes;
#endif

    BufferSize = *NumBytes;
    *NumBytes = 0;

    if (HandleData->InputReadData->InputHandleFlags & HANDLE_INPUT_PENDING) {

        //
        // if we have leftover input, copy as much fits into the user's
        // buffer and return.  we may have multi line input, if a macro
        // has been defined that contains the $T character.
        //

        if (HandleData->InputReadData->InputHandleFlags & HANDLE_MULTI_LINE_INPUT) {
            PWSTR Tmp;
#ifdef FE_SB
            if (!Unicode && CONSOLE_IS_DBCS_CP(Console)) {

                if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                    fAddDbcsLead = TRUE;
                    *lpBuffer++ = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar;
                    BufferSize--;
                    HandleData->InputReadData->BytesAvailable--;
                    RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
                }
                if (HandleData->InputReadData->BytesAvailable == 0 ||
                    BufferSize == 0) {
                    HandleData->InputReadData->InputHandleFlags &= ~(HANDLE_INPUT_PENDING | HANDLE_MULTI_LINE_INPUT);
                    ConsoleHeapFree(HandleData->InputReadData->BufPtr);
                    *NumBytes = 1;
                    return STATUS_SUCCESS;
                }
                else {
                    for (NumToWrite=0,Tmp=HandleData->InputReadData->CurrentBufPtr,NumToBytes=0;
                         NumToBytes < HandleData->InputReadData->BytesAvailable && NumToBytes < BufferSize/sizeof(WCHAR) && *Tmp!=UNICODE_LINEFEED;
                         (IsConsoleFullWidth(Console->hDC,
                                             Console->CP,*Tmp) ? NumToBytes+=2 : NumToBytes++),Tmp++,NumToWrite+=sizeof(WCHAR)) ;
                }
            }
#endif
            for (NumToWrite=0,Tmp=HandleData->InputReadData->CurrentBufPtr;
                 NumToWrite < HandleData->InputReadData->BytesAvailable && *Tmp!=UNICODE_LINEFEED;
                 Tmp++,NumToWrite+=sizeof(WCHAR)) ;
            NumToWrite += sizeof(WCHAR);
            if (NumToWrite > BufferSize) {
                NumToWrite = BufferSize;
            }
        } else {
#ifdef FE_SB
            if (!Unicode && CONSOLE_IS_DBCS_CP(Console)) {
                PWSTR Tmp;

                if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                    fAddDbcsLead = TRUE;
                    *lpBuffer++ = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar;
                    BufferSize-=sizeof(WCHAR);
                    HandleData->InputReadData->BytesAvailable-=sizeof(WCHAR);
                    RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
                }
                if (HandleData->InputReadData->BytesAvailable == 0) {
                    HandleData->InputReadData->InputHandleFlags &= ~(HANDLE_INPUT_PENDING | HANDLE_MULTI_LINE_INPUT);
                    ConsoleHeapFree(HandleData->InputReadData->BufPtr);
                    *NumBytes = 1;
                    return STATUS_SUCCESS;
                }
                else {
                    for (NumToWrite=0,Tmp=HandleData->InputReadData->CurrentBufPtr,NumToBytes=0;
                         NumToBytes < HandleData->InputReadData->BytesAvailable && NumToBytes < BufferSize/sizeof(WCHAR);
                         (IsConsoleFullWidth(Console->hDC,
                                             Console->CP,*Tmp) ? NumToBytes+=2 : NumToBytes++),Tmp++,NumToWrite+=sizeof(WCHAR)) ;
                }
            }
#endif
            NumToWrite = (BufferSize < HandleData->InputReadData->BytesAvailable) ?
                          BufferSize : HandleData->InputReadData->BytesAvailable;
        }
        RtlCopyMemory(lpBuffer,HandleData->InputReadData->CurrentBufPtr,NumToWrite);
        HandleData->InputReadData->BytesAvailable-= NumToWrite;
        if (HandleData->InputReadData->BytesAvailable == 0) {
            HandleData->InputReadData->InputHandleFlags &= ~(HANDLE_INPUT_PENDING | HANDLE_MULTI_LINE_INPUT);
            ConsoleHeapFree(HandleData->InputReadData->BufPtr);
        }
        else {
            HandleData->InputReadData->CurrentBufPtr=(PWCHAR)((PBYTE)HandleData->InputReadData->CurrentBufPtr+NumToWrite);
        }
        if (!Unicode) {

            //
            // if ansi, translate string.  we allocated the capture buffer large
            // enough to handle the translated string.
            //

            PCHAR TransBuffer;

#ifdef FE_SB
            if (CONSOLE_IS_DBCS_CP(Console))
                TransBuffer = (PCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),NumToBytes);
            else
#endif
            TransBuffer = (PCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),NumToWrite / sizeof(WCHAR));
            if (TransBuffer == NULL) {
                return STATUS_NO_MEMORY;
            }

#ifdef FE_SB
            if (CONSOLE_IS_DBCS_CP(Console))
            {
                NumToWrite = TranslateUnicodeToOem(Console,
                                                   lpBuffer,
                                                   NumToWrite / sizeof (WCHAR),
                                                   TransBuffer,
                                                   NumToBytes,
                                                   &HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte);
            }
            else
#endif
            NumToWrite = ConvertToOem(Console->CP,
                                lpBuffer,
                                NumToWrite / sizeof (WCHAR),
                                TransBuffer,
                                NumToWrite / sizeof (WCHAR)
                                );
            RtlCopyMemory(lpBuffer,TransBuffer,NumToWrite);
#ifdef FE_SB
            if (fAddDbcsLead)
                NumToWrite++;
#endif
            ConsoleHeapFree(TransBuffer);
        }
        *NumBytes = NumToWrite;
        return STATUS_SUCCESS;
    }

    //
    // we need to create a temporary handle to the current screen buffer
    // if echo is on.
    //

    if ((InputInfo->InputMode & (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)) ==
        (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)) {
        HANDLE ActiveScreenHandle;

        Echo = FALSE;
        ActiveScreenHandle = FindActiveScreenBufferHandle(ProcessData,Console);
        if (ActiveScreenHandle != INVALID_HANDLE_VALUE) {
            TempHandle.HandleType = CONSOLE_OUTPUT_HANDLE;
            TempHandle.Buffer.ScreenBuffer = Console->CurrentScreenBuffer;
            if (TempHandle.Buffer.ScreenBuffer != NULL) {
                Status = ConsoleAddShare(GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         &TempHandle.Buffer.ScreenBuffer->ShareAccess,
                                         &TempHandle
                                        );
                if (NT_SUCCESS(Status)) {
                    Echo = TRUE;
                    TempHandle.Buffer.ScreenBuffer->RefCount++;
                }
            }
        }
    }

    if (InputInfo->InputMode & ENABLE_LINE_INPUT) {

        //
        // read in characters until the buffer is full or return is read.
        // since we may wait inside this loop, store all important variables
        // in the read data structure.  if we do wait, a read data structure
        // will be allocated from the heap and its pointer will be stored
        // in the wait block.  the CookedReadData will be copied into the
        // structure.  the data is freed when the read is completed.
        //

        COOKED_READ_DATA CookedReadData;
        ULONG i;
        PWCHAR TempBuffer;
        ULONG TempBufferSize;

        //
        // to emulate OS/2 KbdStringIn, we read into our own big buffer
        // (256 bytes) until the user types enter.  then return as many
        // chars as will fit in the user's buffer.
        //

        TempBufferSize = (BufferSize < LINE_INPUT_BUFFER_SIZE) ? LINE_INPUT_BUFFER_SIZE : BufferSize;
        TempBuffer = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),TempBufferSize);
        if (TempBuffer==NULL) {
            if (Echo) {
                CloseOutputHandle(ProcessData,
                                  Console,
                                  &TempHandle,
                                  NULL,
                                  FALSE
                                 );
            }
            return STATUS_NO_MEMORY;
        }

        //
        // initialize the user's buffer to spaces.  this is done so that
        // moving in the buffer via cursor doesn't do strange things.
        //

        for (i=0;i<TempBufferSize/sizeof(WCHAR);i++) {
            TempBuffer[i] = (WCHAR)' ';
        }

        CookedReadData.InputInfo = InputInfo;
        CookedReadData.ScreenInfo = ScreenInfo;
        CookedReadData.Console = Console;
        CookedReadData.TempHandle.HandleType = TempHandle.HandleType;
        CookedReadData.TempHandle.Buffer.ScreenBuffer = TempHandle.Buffer.ScreenBuffer;
        CookedReadData.BufferSize = TempBufferSize;
        CookedReadData.BytesRead = 0;
        CookedReadData.CurrentPosition = 0;
        CookedReadData.BufPtr = TempBuffer;
        CookedReadData.BackupLimit = TempBuffer;
        CookedReadData.UserBufferSize = BufferSize;
        CookedReadData.UserBuffer = lpBuffer;
        CookedReadData.OriginalCursorPosition.X = -1;
        CookedReadData.OriginalCursorPosition.Y = -1;
        CookedReadData.NumberOfVisibleChars = 0;
        CookedReadData.CtrlWakeupMask = CtrlWakeupMask;
        CookedReadData.CommandHistory = CommandHistory;
        CookedReadData.Echo = Echo;
        CookedReadData.InsertMode = Console->InsertMode;
        CookedReadData.Processed = (InputInfo->InputMode & ENABLE_PROCESSED_INPUT) != 0;
        CookedReadData.Line = (InputInfo->InputMode & ENABLE_LINE_INPUT) != 0;
        CookedReadData.ProcessData = ProcessData;
        CookedReadData.HandleIndex = HandleIndex;
        CookedReadData.ExeName = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( HISTORY_TAG ),ExeNameLength);
        if (InitialNumBytes != 0) {
            RtlCopyMemory(CookedReadData.BufPtr, CookedReadData.UserBuffer, InitialNumBytes);
            CookedReadData.BytesRead += InitialNumBytes;
            CookedReadData.NumberOfVisibleChars = (InitialNumBytes / sizeof(WCHAR));
            CookedReadData.BufPtr += (InitialNumBytes / sizeof(WCHAR));
            CookedReadData.CurrentPosition = (InitialNumBytes / sizeof(WCHAR));
            CookedReadData.OriginalCursorPosition = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
            CookedReadData.OriginalCursorPosition.X -= (SHORT)CookedReadData.CurrentPosition;


            while (CookedReadData.OriginalCursorPosition.X < 0) {
                CookedReadData.OriginalCursorPosition.X += ScreenInfo->ScreenBufferSize.X;
                CookedReadData.OriginalCursorPosition.Y -= 1;
            }
        }
        if (CookedReadData.ExeName) {
            RtlCopyMemory(CookedReadData.ExeName,ExeName,ExeNameLength);
            CookedReadData.ExeNameLength = ExeNameLength;
        }
#ifdef FE_SB
        Console->lpCookedReadData = (PVOID)&CookedReadData;
#endif

        Status = CookedRead(&CookedReadData,
                            Message,
                            CSR_SERVER_QUERYCLIENTTHREAD(),
                            FALSE
                           );
#ifdef FE_SB
        if (Status != CONSOLE_STATUS_WAIT) {
            Console->lpCookedReadData = NULL;
        }
#endif
        return Status;
    }

    //
    // character (raw) mode
    //

    else {

        //
        // read at least one character in.  after one character has been
        // read, get any more available characters and return.  the first
        //  call to GetChar may wait.   if we do wait, a read data structure
        // will be allocated from the heap and its pointer will be stored
        // in the wait block.  the RawReadData will be copied into the
        // structure.  the data is freed when the read is completed.
        //

        RAW_READ_DATA RawReadData;

        RawReadData.InputInfo = InputInfo;
        RawReadData.Console = Console;
        RawReadData.BufferSize = BufferSize;
        RawReadData.BufPtr = lpBuffer;
        RawReadData.ProcessData = ProcessData;
        RawReadData.HandleIndex = HandleIndex;
        if (*NumBytes < BufferSize) {
            PWCHAR pwchT;

#ifdef FE_SB
            PWCHAR lpBufferTmp = lpBuffer;

            NumToWrite = 0;
            if (!Unicode && CONSOLE_IS_DBCS_CP(Console)) {
                if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                    fAddDbcsLead = TRUE;
                    *lpBuffer++ = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar;
                    BufferSize-=sizeof(WCHAR);
                    RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
                    Status = STATUS_SUCCESS;
                    if (BufferSize == 0) {
                        *NumBytes = 1;
                        return STATUS_SUCCESS;
                    }
                }
                else{
                    Status = GetChar(InputInfo,
                             lpBuffer,
                             TRUE,
                             Console,
                             HandleData,
                             Message,
                             RawReadWaitRoutine,
                             &RawReadData,
                             sizeof(RawReadData),
                             FALSE,
                             NULL,
                             NULL,
                             NULL,
                             NULL
                            );
                }
            }
            else
#endif
            Status = GetChar(InputInfo,
                             lpBuffer,
                             TRUE,
                             Console,
                             HandleData,
                             Message,
                             RawReadWaitRoutine,
                             &RawReadData,
                             sizeof(RawReadData),
                             FALSE,
                             NULL,
                             NULL,
                             NULL,
                             NULL
                            );

            if (!NT_SUCCESS(Status)) {
                *NumBytes = 0;
                return Status;
            }
#ifdef FE_SB
            if (! fAddDbcsLead) {
                IsConsoleFullWidth(Console->hDC,
                                   Console->CP,*lpBuffer) ? *NumBytes+=2 : ++*NumBytes;
                NumToWrite+=sizeof(WCHAR);
                lpBuffer++;
            }
            if (CONSOLE_IS_DBCS_CP(Console)) {
                while (NumToWrite < BufferSize) {
                    Status = GetChar(InputInfo,lpBuffer,FALSE,NULL,NULL,NULL,NULL,NULL,0,FALSE,NULL,NULL,NULL,NULL);
                    if (!NT_SUCCESS(Status)) {
                        return STATUS_SUCCESS;
                    }
                    IsConsoleFullWidth(Console->hDC,
                                       Console->CP,*lpBuffer) ? *NumBytes+=2 : ++*NumBytes;
                    lpBuffer++;
                    NumToWrite+=sizeof(WCHAR);
                }
            }
            else{
#endif
            pwchT = lpBuffer + 1;
            *NumBytes += sizeof(WCHAR);
            while (*NumBytes < BufferSize) {
                Status = GetChar(InputInfo,pwchT,FALSE,NULL,NULL,NULL,NULL,NULL,0,FALSE,NULL,NULL,NULL,NULL);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
                pwchT++;
                *NumBytes += sizeof(WCHAR);
            }
#ifdef FE_SB
            }
#endif

            //
            // if ansi, translate string.  we allocated the capture buffer large
            // enough to handle the translated string.
            //

            if (!Unicode) {

                PCHAR TransBuffer;

                TransBuffer = (PCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),*NumBytes / sizeof(WCHAR));
                if (TransBuffer == NULL) {
                    return STATUS_NO_MEMORY;
                }

#ifdef FE_SB
                lpBuffer = lpBufferTmp;
                if (CONSOLE_IS_DBCS_CP(Console))
                {
                    *NumBytes = TranslateUnicodeToOem(Console,
                                                      lpBuffer,
                                                      NumToWrite / sizeof (WCHAR),
                                                      TransBuffer,
                                                      *NumBytes,
                                                      &HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte);
                }
                else
#endif
                *NumBytes = ConvertToOem(Console->CP,
                                   lpBuffer,
                                   *NumBytes / sizeof (WCHAR),
                                   TransBuffer,
                                   *NumBytes / sizeof (WCHAR)
                                   );
                RtlCopyMemory(lpBuffer,TransBuffer,*NumBytes);
#ifdef FE_SB
                if (fAddDbcsLead)
                    ++*NumBytes;
#endif
                ConsoleHeapFree(TransBuffer);
            }
        }
    }
    return STATUS_SUCCESS;
}


ULONG
SrvReadConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine reads characters from the input stream.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_READCONSOLE_MSG a = (PCONSOLE_READCONSOLE_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    PWCHAR Buffer;
    PCONSOLE_PER_PROCESS_DATA ProcessData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    ProcessData = CONSOLE_PERPROCESSDATA();
    Status = DereferenceIoHandle(ProcessData,
                                 a->InputHandle,
                                 CONSOLE_INPUT_HANDLE,
                                 GENERIC_READ,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        a->NumBytes = 0;
    } else {

        if (a->CaptureBufferSize <= BUFFER_SIZE) {
            Buffer = a->Buffer;
        }
        else {
            Buffer = a->BufPtr;
            if (!CsrValidateMessageBuffer(m, &a->BufPtr, a->CaptureBufferSize, sizeof(BYTE))) {
                UnlockConsole(Console);
                return STATUS_INVALID_PARAMETER;
            }
        }

#if defined(FE_SB)
        Console->ReadConInpNumBytesTemp = a->NumBytes / sizeof(WCHAR);
#endif
        Status = ReadChars(HandleData->Buffer.InputBuffer,
                           Console,
                           ProcessData,
                           Console->CurrentScreenBuffer,
                           Buffer,
                           &a->NumBytes,
                           a->InitialNumBytes,
                           a->CtrlWakeupMask,
                           HandleData,
                           FindCommandHistory(Console,CONSOLE_CLIENTPROCESSHANDLE()),
                           m,
                           HANDLE_TO_INDEX(a->InputHandle),
                           a->ExeNameLength,
                           a->Buffer,
                           a->Unicode
                          );
        if (Status == CONSOLE_STATUS_WAIT) {
            *ReplyStatus = CsrReplyPending;
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
}


VOID
MakeCursorVisible(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD CursorPosition
    )
{
    COORD WindowOrigin;
    NTSTATUS Status;

    WindowOrigin.X = 0;
    WindowOrigin.Y = 0;
    if (CursorPosition.X > ScreenInfo->Window.Right) {
        WindowOrigin.X = CursorPosition.X - ScreenInfo->Window.Right;
    }
    else if (CursorPosition.X < ScreenInfo->Window.Left) {
        WindowOrigin.X = CursorPosition.X - ScreenInfo->Window.Left;
    }
    if (CursorPosition.Y > ScreenInfo->Window.Bottom) {
        WindowOrigin.Y = CursorPosition.Y - ScreenInfo->Window.Bottom;
    }
    else if (CursorPosition.Y < ScreenInfo->Window.Top) {
        WindowOrigin.Y = CursorPosition.Y - ScreenInfo->Window.Top;
    }
    if (WindowOrigin.X != 0 || WindowOrigin.Y != 0) {
        Status = SetWindowOrigin(ScreenInfo,
                               FALSE,
                               WindowOrigin
                              );
        if (!NT_SUCCESS(Status)) {
            return;
        }
    }
}

#define WRITE_NO_CR_LF 0
#define WRITE_CR 1
#define WRITE_CR_LF 2
#define WRITE_SPECIAL_CHARS 4
#define WRITE_UNICODE_CRLF 0x000a000d

DWORD
FastStreamWrite(
    IN PWCHAR lpString,
    IN DWORD NumChars
    )

/*++

Routine Description:

    This routine determines whether the text string contains characters
    that require special processing.  If it doesn't,
    unicode characters.  The string is also copied to the input buffer, if
    the output mode is line mode.

Arguments:

    lpString - Pointer to string to write.

    NumChars - Number of chars in buffer.

Return Value:

    WRITE_SPECIAL_CHARS - string contains characters requiring special processing

    WRITE_NO_CR_LF - string contains no special chars and no CRLF

    WRITE_CR_LF - string contains no special chars and is terminated by CRLF

    WRITE_CR - string contains no special chars and is terminated by CR

--*/

{
    DWORD UNALIGNED *Tmp;
    register PWCHAR StrPtr=lpString;
    while (NumChars) {
        if (*StrPtr < UNICODE_SPACE) {
            Tmp = (PDWORD)StrPtr;
            if (NumChars == 2 &&
                *Tmp == WRITE_UNICODE_CRLF) {
                return WRITE_CR_LF;
            } else if (NumChars == 1 &&
                *StrPtr == (WCHAR)'\r') {
                return WRITE_CR;
            } else {
                return WRITE_SPECIAL_CHARS;
            }
        }
        StrPtr++;
        NumChars--;
    }
    return WRITE_NO_CR_LF;
}

VOID UnblockWriteConsole(
    IN PCONSOLE_INFORMATION Console,
    IN DWORD Reason)
{
    Console->Flags &= ~Reason;

    if ((Console->Flags & (CONSOLE_SUSPENDED | CONSOLE_SELECTING | CONSOLE_SCROLLBAR_TRACKING)) == 0) {
        /*
         * no remain reason to suspend output, so unblock it.
         */
        if (CsrNotifyWait(&Console->OutputQueue, TRUE, NULL, NULL)) {
            // #334370 under stress, WaitQueue may already hold the satisfied waits
            ASSERT ((Console->WaitQueue == NULL) ||
                    (Console->WaitQueue == &Console->OutputQueue));
            Console->WaitQueue = &Console->OutputQueue;
        }
    }
}


ULONG
SrvWriteConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine writes characters to the output stream.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    NTSTATUS Status;
    PCONSOLE_WRITECONSOLE_MSG a = (PCONSOLE_WRITECONSOLE_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (!a->BufferInMessage) {
        if (!CsrValidateMessageBuffer(m, &a->BufPtr, a->NumBytes, sizeof(BYTE))) {
            UnlockConsole(Console);
            return STATUS_INVALID_PARAMETER;
        }
    }
    else if (a->NumBytes > sizeof(a->Buffer)) {
        UnlockConsole(Console);
        return STATUS_INVALID_PARAMETER;
    }


    //
    // Make sure we have a valid screen buffer.
    //

    Status = DereferenceIoHandle(CONSOLE_PERPROCESSDATA(),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        UnlockConsole(Console);
        return Status;
    }

    Status = DoSrvWriteConsole(m,ReplyStatus,Console,HandleData);

    UnlockConsole(Console);
    return Status;
}

BOOLEAN
WriteConsoleWaitRoutine(
    IN PLIST_ENTRY WaitQueue,
    IN PCSR_THREAD WaitingThread,
    IN PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2,
    IN ULONG WaitFlags
    )
{
    NTSTATUS Status;
    PCONSOLE_WRITECONSOLE_MSG a = (PCONSOLE_WRITECONSOLE_MSG)&WaitReplyMessage->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;

    if (WaitFlags & CSR_PROCESS_TERMINATING) {
        WaitReplyMessage->ReturnValue = (ULONG)STATUS_THREAD_IS_TERMINATING;
        return TRUE;
    }
    LockConsoleHandleTable();
    Status = DereferenceConsoleHandle(a->ConsoleHandle,
                                      &Console
                                     );
    UnlockConsoleHandleTable();
    ASSERT (NT_SUCCESS(Status));

    //
    // if we get to here, this routine was called by the input
    // thread, which grabs the current console lock.
    //

    //
    // this routine should be called by a thread owning the same
    // lock on the same console as we're reading from.
    //

    ASSERT (ConsoleLocked(Console));

    //
    // if we're unicode, the string may still be in the message buffer.
    // since the message was reallocated and copied when the wait was
    // created, we need to fix up a->TransBuffer here.
    //

    if (a->Unicode && a->BufferInMessage) {
        a->TransBuffer = a->Buffer;
    }

    Status = DoWriteConsole(WaitReplyMessage,Console,WaitingThread);
    if (Status == CONSOLE_STATUS_WAIT) {
        return FALSE;
    }
    if (!a->Unicode) {
#ifdef FE_SB
        if (CONSOLE_IS_DBCS_OUTPUTCP(Console))
        {
            if (a->NumBytes == Console->WriteConOutNumBytesUnicode)
                a->NumBytes = Console->WriteConOutNumBytesTemp;
            else
                a->NumBytes /= sizeof(WCHAR);
            }
        else
#endif
        a->NumBytes /= sizeof(WCHAR);
        ConsoleHeapFree(a->TransBuffer);
    }
    WaitReplyMessage->ReturnValue = Status;
    return TRUE;
    UNREFERENCED_PARAMETER(WaitQueue);
    UNREFERENCED_PARAMETER(WaitParameter);
    UNREFERENCED_PARAMETER(SatisfyParameter1);
    UNREFERENCED_PARAMETER(SatisfyParameter2);
}

ULONG
SrvDuplicateHandle(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine duplicates an input or output handle.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_DUPHANDLE_MSG a = (PCONSOLE_DUPHANDLE_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA SourceHandleData,TargetHandleData;
    PCONSOLE_SHARE_ACCESS ShareAccess;
    NTSTATUS Status;
    PCONSOLE_PER_PROCESS_DATA ProcessData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    ProcessData = CONSOLE_PERPROCESSDATA();
    Status = DereferenceIoHandleNoCheck(ProcessData,
                                 HANDLE_TO_INDEX(a->SourceHandle),
                                 &SourceHandleData
                                );
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }
    if (a->Options & DUPLICATE_SAME_ACCESS) {
        a->DesiredAccess = SourceHandleData->Access;
    }

    //
    // make sure that requested access is a subset of source handle's access
    //

    else if ((a->DesiredAccess & SourceHandleData->Access) != a->DesiredAccess) {
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    Status = AllocateIoHandle(ProcessData,
                              SourceHandleData->HandleType,
                              &a->TargetHandle
                             );
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }

    //
    // it's possible that AllocateIoHandle realloced the handle table,
    // so deference SourceHandle again.
    //

    Status = DereferenceIoHandleNoCheck(ProcessData,
                                 HANDLE_TO_INDEX(a->SourceHandle),
                                 &SourceHandleData
                                );
    ASSERT (NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        goto exit;
    }
    Status = DereferenceIoHandleNoCheck(ProcessData,
                                 a->TargetHandle,
                                 &TargetHandleData
                                );
    ASSERT (NT_SUCCESS(Status));
    if (!NT_SUCCESS(Status)) {
        FreeIoHandle(ProcessData,
                     a->TargetHandle
                    );
        goto exit;
    }
    if (SourceHandleData->HandleType & CONSOLE_INPUT_HANDLE) {
        // grab input lock
        if (!InitializeInputHandle(TargetHandleData,
                                   SourceHandleData->Buffer.InputBuffer)) {
            FreeIoHandle(ProcessData,
                         a->TargetHandle
                        );
            Status = STATUS_NO_MEMORY;
            goto exit;
        }
        ShareAccess = &SourceHandleData->Buffer.InputBuffer->ShareAccess;
    }
    else {
        // grab output lock
        InitializeOutputHandle(TargetHandleData,SourceHandleData->Buffer.ScreenBuffer);
        ShareAccess = &SourceHandleData->Buffer.ScreenBuffer->ShareAccess;
    }
    TargetHandleData->HandleType = SourceHandleData->HandleType;
    if (a->InheritHandle) {
        TargetHandleData->HandleType |= CONSOLE_INHERITABLE;
    } else {
        TargetHandleData->HandleType &= ~CONSOLE_INHERITABLE;
    }

    Status = ConsoleDupShare(a->DesiredAccess,
                             SourceHandleData->ShareAccess,
                             ShareAccess,
                             TargetHandleData
                            );
    if (!NT_SUCCESS(Status)) {
        FreeIoHandle(ProcessData,
                     a->TargetHandle
                    );
        if (SourceHandleData->HandleType & CONSOLE_INPUT_HANDLE) {
            SourceHandleData->Buffer.InputBuffer->RefCount--;
        }
        else {
            SourceHandleData->Buffer.ScreenBuffer->RefCount--;
        }
    }
    else {
        a->TargetHandle = INDEX_TO_HANDLE(a->TargetHandle);
    }

    if (a->Options & DUPLICATE_CLOSE_SOURCE) {
        if (SourceHandleData->HandleType & CONSOLE_INPUT_HANDLE)
            CloseInputHandle(ProcessData,Console,SourceHandleData,HANDLE_TO_INDEX(a->SourceHandle));
        else {
            CloseOutputHandle(ProcessData,Console,SourceHandleData,HANDLE_TO_INDEX(a->SourceHandle),TRUE);
        }
    }

exit:
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvGetHandleInformation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This gets information about an input or output handle.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_GETHANDLEINFORMATION_MSG a = (PCONSOLE_GETHANDLEINFORMATION_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandleNoCheck(CONSOLE_PERPROCESSDATA(),
                                 HANDLE_TO_INDEX(a->Handle),
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        a->Flags = 0;
        if (HandleData->HandleType & CONSOLE_INHERITABLE) {
            a->Flags |= HANDLE_FLAG_INHERIT;
        }
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvSetHandleInformation(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This sets information about an input or output handle.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_SETHANDLEINFORMATION_MSG a = (PCONSOLE_SETHANDLEINFORMATION_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    Status = DereferenceIoHandleNoCheck(CONSOLE_PERPROCESSDATA(),
                                 HANDLE_TO_INDEX(a->Handle),
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        if (a->Mask & HANDLE_FLAG_INHERIT) {
            if (a->Flags & HANDLE_FLAG_INHERIT) {
                HandleData->HandleType |= CONSOLE_INHERITABLE;
            } else {
                HandleData->HandleType &= ~CONSOLE_INHERITABLE;
            }
        }
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

NTSTATUS
CloseInputHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData,
    IN HANDLE Handle
    )

/*++

Routine Description:

    This routine closes an input handle.  It decrements the input buffer's
    reference count.  If it goes to zero, the buffer is reinitialized.
    Otherwise, the handle is removed from sharing.

Arguments:

    ProcessData - Pointer to per process data.

    HandleData - Pointer to handle data structure.

    Handle - Handle to close.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    BOOLEAN WaitSatisfied = FALSE;

    ASSERT(ProcessData->Foo == 0xF00);
    if (HandleData->InputReadData->InputHandleFlags & HANDLE_INPUT_PENDING) {
        HandleData->InputReadData->InputHandleFlags &= ~HANDLE_INPUT_PENDING;
        ConsoleHeapFree(HandleData->InputReadData->BufPtr);
    }

    //
    // see if there are any reads waiting for data via this handle.  if
    // there are, wake them up.  there aren't any other outstanding i/o
    // operations via this handle because the console lock is held.
    //

    LockReadCount(HandleData);
    if (HandleData->InputReadData->ReadCount != 0) {
        UnlockReadCount(HandleData);
        HandleData->InputReadData->InputHandleFlags |= HANDLE_CLOSING;

        WaitSatisfied |= CsrNotifyWait(&HandleData->Buffer.InputBuffer->ReadWaitQueue,
                      TRUE,
                      (PVOID) HandleData,
                      NULL
                     );
        LockReadCount(HandleData);
    }
    if (WaitSatisfied) {
        // #334370 under stress, WaitQueue may already hold the satisfied waits
        ASSERT ((Console->WaitQueue == NULL) ||
                (Console->WaitQueue == &HandleData->Buffer.InputBuffer->ReadWaitQueue));
        Console->WaitQueue = &HandleData->Buffer.InputBuffer->ReadWaitQueue;
    }
    if (HandleData->InputReadData->ReadCount != 0) {
        KdPrint(("ReadCount is %lX\n",HandleData->InputReadData->ReadCount));
    }
    ASSERT (HandleData->InputReadData->ReadCount == 0);
    UnlockReadCount(HandleData);

    ASSERT (HandleData->Buffer.InputBuffer->RefCount);
    HandleData->Buffer.InputBuffer->RefCount--;
    if (HandleData->Buffer.InputBuffer->RefCount == 0) {
        ReinitializeInputBuffer(HandleData->Buffer.InputBuffer);
    }
    else {
        ConsoleRemoveShare(HandleData->Access,
                           HandleData->ShareAccess,
                           &HandleData->Buffer.InputBuffer->ShareAccess
                          );
    }
    return FreeIoHandle(ProcessData,Handle);
}

NTSTATUS
CloseOutputHandle(
    IN PCONSOLE_PER_PROCESS_DATA ProcessData,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData,
    IN HANDLE Handle,
    IN BOOLEAN FreeHandle
    )

/*++

Routine Description:

    This routine closes an output handle.  It decrements the screen buffer's
    reference count.  If it goes to zero, the buffer is freed.  Otherwise,
    the handle is removed from sharing.

Arguments:

    ProcessData - Pointer to per process data.

    Console - Pointer to console information structure.

    HandleData - Pointer to handle data structure.

    Handle - Handle to close.

    FreeHandle - if TRUE, free handle.  used by ReadChars in echo mode
    and by process cleanup.

Return Value:

Note:

    The console lock must be held when calling this routine.

--*/

{
    NTSTATUS Status;

    ASSERT(ProcessData->Foo == 0xF00);
    ASSERT (HandleData->Buffer.ScreenBuffer->RefCount);
    HandleData->Buffer.ScreenBuffer->RefCount--;
    if (HandleData->Buffer.ScreenBuffer->RefCount == 0) {
        RemoveScreenBuffer(Console,HandleData->Buffer.ScreenBuffer);
        if (HandleData->Buffer.ScreenBuffer == Console->CurrentScreenBuffer &&
            Console->ScreenBuffers != Console->CurrentScreenBuffer) {
            if (Console->ScreenBuffers != NULL) {
                SetActiveScreenBuffer(Console->ScreenBuffers);
            } else {
                Console->CurrentScreenBuffer = NULL;
            }
        }
        Status = FreeScreenBuffer(HandleData->Buffer.ScreenBuffer);
    }
    else {
        Status = ConsoleRemoveShare(HandleData->Access,
                                    HandleData->ShareAccess,
                                    &HandleData->Buffer.ScreenBuffer->ShareAccess
                                   );
    }
    if (FreeHandle)
        Status = FreeIoHandle(ProcessData,Handle);
    return Status;
}


ULONG
SrvCloseHandle(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine closes an input or output handle.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_CLOSEHANDLE_MSG a = (PCONSOLE_CLOSEHANDLE_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    PCONSOLE_PER_PROCESS_DATA ProcessData;

    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }
    ProcessData = CONSOLE_PERPROCESSDATA();
    Status = DereferenceIoHandleNoCheck(ProcessData,
                                 HANDLE_TO_INDEX(a->Handle),
                                 &HandleData
                                );
    if (NT_SUCCESS(Status)) {
        if (HandleData->HandleType & CONSOLE_INPUT_HANDLE)
            Status = CloseInputHandle(ProcessData,Console,HandleData,HANDLE_TO_INDEX(a->Handle));
        else {
            Status = CloseOutputHandle(ProcessData,Console,HandleData,HANDLE_TO_INDEX(a->Handle),TRUE);
        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

NTSTATUS
WriteCharsFromInput(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PWCHAR lpBufferBackupLimit,
    IN PWCHAR lpBuffer,
    IN PWCHAR lpString,
    IN OUT PDWORD NumBytes,
    OUT PLONG NumSpaces OPTIONAL,
    IN SHORT OriginalXPosition,
    IN DWORD dwFlags,
    OUT PSHORT ScrollY OPTIONAL
    )

/*++

Routine Description:

    This routine converts chars from their true unicode representation
    to the Unicode representation (UnicodeAnsi) that will generate
    the correct glyph given an OEM font and an ANSI translation by GDI.
    It then calls WriteChars.

Arguments:

    ScreenInfo - Pointer to screen buffer information structure.

    lpBufferBackupLimit - Pointer to beginning of buffer.

    lpBuffer - Pointer to buffer to copy string to.  assumed to be at least
    as long as lpString.  This pointer is updated to point to the next
    position in the buffer.

    lpString - Pointer to string to write.

    NumBytes - On input, number of bytes to write.  On output, number of
    bytes written.

    NumSpaces - On output, the number of spaces consumed by the written characters.

    dwFlags -
      WC_DESTRUCTIVE_BACKSPACE backspace overwrites characters.
      WC_KEEP_CURSOR_VISIBLE   change window origin desirable when hit rt. edge
      WC_ECHO                  if called by Read (echoing characters)
      WC_FALSIFY_UNICODE       if RealUnicodeToFalseUnicode need be called.

Return Value:

Note:

    This routine does not process tabs and backspace properly.  That code
    will be implemented as part of the line editing services.

--*/

{
    DWORD Length,i;

    if (ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER) {
        return STATUS_UNSUCCESSFUL;
    }

    if (!(ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) ||
            (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
        goto SimpleWrite;
    }

    Length = *NumBytes / sizeof(WCHAR);
    for (i=0;i<Length;i++) {
        if (lpString[i] > 0x7f) {
            dwFlags |= WC_FALSIFY_UNICODE;
            break;
        }
    }

SimpleWrite:
    return WriteChars(ScreenInfo,
                    lpBufferBackupLimit,
                    lpBuffer,
                    lpString,
                    NumBytes,
                    NumSpaces,
                    OriginalXPosition,
                    dwFlags,
                    ScrollY
                   );
}

#if defined(FE_SB)

#define WWSB_NOFE
#include "dispatch.h"
#include "_stream.h"
#undef  WWSB_NOFE
#define WWSB_FE
#include "dispatch.h"
#include "_stream.h"
#undef  WWSB_FE

#endif // FE_SB

