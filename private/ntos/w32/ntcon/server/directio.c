/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    directio.c

Abstract:

        This file implements the NT console direct I/O API

Author:

    Therese Stowell (thereses) 6-Nov-1990

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#if defined(FE_SB)
#define WWSB_FE
#include "dispatch.h" // get the FE_ prototypes
#undef  WWSB_FE
#pragma alloc_text(FE_TEXT, FE_TranslateInputToOem)
#pragma alloc_text(FE_TEXT, FE_TranslateInputToUnicode)
#pragma alloc_text(FE_TEXT, FE_TranslateOutputToOem)
#pragma alloc_text(FE_TEXT, FE_TranslateOutputToOemUnicode)
#pragma alloc_text(FE_TEXT, FE_TranslateOutputToUnicode)
#pragma alloc_text(FE_TEXT, FE_TranslateOutputToAnsiUnicode)
#endif


#if defined(FE_SB)
ULONG
SB_TranslateInputToOem
#else
NTSTATUS
TranslateInputToOem
#endif
    (
    IN PCONSOLE_INFORMATION Console,
    IN OUT PINPUT_RECORD InputRecords,
    IN ULONG NumRecords
    )
{
    ULONG i;

    DBGCHARS(("TranslateInputToOem\n"));
    for (i=0;i<NumRecords;i++) {
        if (InputRecords[i].EventType == KEY_EVENT) {
            InputRecords[i].Event.KeyEvent.uChar.AsciiChar = WcharToChar(
                    Console->CP, InputRecords[i].Event.KeyEvent.uChar.UnicodeChar);
        }
    }
#if defined(FE_SB)
    return NumRecords;
#else
    return STATUS_SUCCESS;
#endif
}

#if defined(FE_SB)
ULONG
FE_TranslateInputToOem(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PINPUT_RECORD InputRecords,
    IN ULONG NumRecords,    // in : ASCII byte count
    IN ULONG UnicodeLength, // in : Number of events (char count)
    OUT PINPUT_RECORD DbcsLeadInpRec
    )
{
    ULONG i,j;
    PINPUT_RECORD TmpInpRec;
    BYTE AsciiDbcs[2];
    ULONG NumBytes;

    ASSERT(NumRecords >= UnicodeLength);

    TmpInpRec = ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),NumRecords*sizeof(INPUT_RECORD));
    if (TmpInpRec == NULL)
        return 0;

    memcpy(TmpInpRec,InputRecords,NumRecords*sizeof(INPUT_RECORD));
    AsciiDbcs[1] = 0;
    for (i=0,j=0; i<UnicodeLength; i++,j++) {
        if (TmpInpRec[i].EventType == KEY_EVENT) {
            if (IsConsoleFullWidth(Console->hDC,
                                   Console->CP,TmpInpRec[i].Event.KeyEvent.uChar.UnicodeChar)) {
                NumBytes = sizeof(AsciiDbcs);
                ConvertToOem(Console->CP,
                       &TmpInpRec[i].Event.KeyEvent.uChar.UnicodeChar,
                       1,
                       &AsciiDbcs[0],
                       NumBytes
                       );
                if (IsDBCSLeadByteConsole(AsciiDbcs[0],&Console->CPInfo)) {
                    if (j < NumRecords-1) {  // -1 is safe DBCS in buffer
                        InputRecords[j] = TmpInpRec[i];
                        InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = 0;
                        InputRecords[j].Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[0];
                        j++;
                        InputRecords[j] = TmpInpRec[i];
                        InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = 0;
                        InputRecords[j].Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[1];
                        AsciiDbcs[1] = 0;
                    }
                    else if (j == NumRecords-1) {
                        InputRecords[j] = TmpInpRec[i];
                        InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = 0;
                        InputRecords[j].Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[0];
                        j++;
                        break;
                    }
                    else {
                        AsciiDbcs[1] = 0;
                        break;
                    }
                }
                else {
                    InputRecords[j] = TmpInpRec[i];
                    InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = 0;
                    InputRecords[j].Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[0];
                    AsciiDbcs[1] = 0;
                }
            }
            else {
                InputRecords[j] = TmpInpRec[i];
                ConvertToOem(Console->CP,
                       &InputRecords[j].Event.KeyEvent.uChar.UnicodeChar,
                       1,
                       &InputRecords[j].Event.KeyEvent.uChar.AsciiChar,
                       1
                       );
            }
        }
    }
    if (DbcsLeadInpRec) {
        if (AsciiDbcs[1]) {
            *DbcsLeadInpRec = TmpInpRec[i];
            DbcsLeadInpRec->Event.KeyEvent.uChar.AsciiChar = AsciiDbcs[1];
        }
        else {
            RtlZeroMemory(DbcsLeadInpRec,sizeof(INPUT_RECORD));
        }
    }
    ConsoleHeapFree(TmpInpRec);
    return j;
}
#endif



#if defined(FE_SB)
ULONG
SB_TranslateInputToUnicode
#else
NTSTATUS
TranslateInputToUnicode
#endif
    (
    IN PCONSOLE_INFORMATION Console,
    IN OUT PINPUT_RECORD InputRecords,
    IN ULONG NumRecords
    )
{
    ULONG i;
    DBGCHARS(("TranslateInputToUnicode\n"));
    for (i=0;i<NumRecords;i++) {
        if (InputRecords[i].EventType == KEY_EVENT) {
#if defined(FE_SB)
            InputRecords[i].Event.KeyEvent.uChar.UnicodeChar = SB_CharToWchar(
                    Console->CP, InputRecords[i].Event.KeyEvent.uChar.AsciiChar);
#else
            InputRecords[i].Event.KeyEvent.uChar.UnicodeChar = CharToWchar(
                    Console->CP, InputRecords[i].Event.KeyEvent.uChar.AsciiChar);
#endif
        }
    }
#if defined(FE_SB)
    return i;
#else
    return STATUS_SUCCESS;
#endif
}

#if defined(FE_SB)
ULONG
FE_TranslateInputToUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PINPUT_RECORD InputRecords,
    IN ULONG NumRecords,
    IN OUT PINPUT_RECORD DBCSLeadByte
    )
{
    ULONG i,j;
    INPUT_RECORD AsciiDbcs[2];
    CHAR Strings[2];
    WCHAR UnicodeDbcs[2];
    PWCHAR Uni;
    ULONG NumBytes;

    if (DBCSLeadByte->Event.KeyEvent.uChar.AsciiChar) {
        AsciiDbcs[0] = *DBCSLeadByte;
        Strings[0] = DBCSLeadByte->Event.KeyEvent.uChar.AsciiChar;
    }
    else{
        RtlZeroMemory(AsciiDbcs,sizeof(AsciiDbcs));
    }
    for (i=j=0; i<NumRecords; i++) {
        if (InputRecords[i].EventType == KEY_EVENT) {
            if (AsciiDbcs[0].Event.KeyEvent.uChar.AsciiChar) {
                AsciiDbcs[1] = InputRecords[i];
                Strings[1] = InputRecords[i].Event.KeyEvent.uChar.AsciiChar;
                NumBytes = sizeof(Strings);
                NumBytes = ConvertInputToUnicode(Console->CP,
                                                 &Strings[0],
                                                 NumBytes,
                                                 &UnicodeDbcs[0],
                                                 NumBytes);
                Uni = &UnicodeDbcs[0];
                while (NumBytes--) {
                    InputRecords[j] = AsciiDbcs[0];
                    InputRecords[j].Event.KeyEvent.uChar.UnicodeChar = *Uni++;
                    j++;
                }
                RtlZeroMemory(AsciiDbcs,sizeof(AsciiDbcs));
                if (DBCSLeadByte->Event.KeyEvent.uChar.AsciiChar)
                    RtlZeroMemory(DBCSLeadByte,sizeof(INPUT_RECORD));
            }
            else if (IsDBCSLeadByteConsole(InputRecords[i].Event.KeyEvent.uChar.AsciiChar,&Console->CPInfo)) {
                if (i < NumRecords-1) {
                    AsciiDbcs[0] = InputRecords[i];
                    Strings[0] = InputRecords[i].Event.KeyEvent.uChar.AsciiChar;
                }
                else {
                    *DBCSLeadByte = InputRecords[i];
                    break;
                }
            }
            else {
                CHAR c;
                InputRecords[j] = InputRecords[i];
                c = InputRecords[i].Event.KeyEvent.uChar.AsciiChar;
                ConvertInputToUnicode(Console->CP,
                      &c,
                      1,
                      &InputRecords[j].Event.KeyEvent.uChar.UnicodeChar,
                      1);
                j++;
            }
        }
        else {
            InputRecords[j++] = InputRecords[i];
        }
    }
    return j;
}
#endif


BOOLEAN
DirectReadWaitRoutine(
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

    This routine is called to complete a direct read that blocked in
    ReadInputBuffer.  The context of the read was saved in the DirectReadData
    structure.  This routine is called when events have been written to
    the input buffer.  It is called in the context of the writing thread.

Arguments:

    WaitQueue - Pointer to queue containing wait block.

    WaitingThread - Pointer to waiting thread.

    WaitReplyMessage - Pointer to reply message to return to dll when
        read is completed.

    DirectReadData - Context of read.

    SatisfyParameter1 - Unused.

    SatisfyParameter2 - Unused.

    WaitFlags - Flags indicating status of wait.

Return Value:

--*/

{
    PCONSOLE_GETCONSOLEINPUT_MSG a;
    PINPUT_RECORD Buffer;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PDIRECT_READ_DATA DirectReadData;
    PHANDLE_DATA HandleData;
    BOOLEAN RetVal = TRUE;
#if defined(FE_SB)
    BOOLEAN fAddDbcsLead = FALSE;
    PDWORD  nLength;
#endif

    a = (PCONSOLE_GETCONSOLEINPUT_MSG)&WaitReplyMessage->u.ApiMessageData;
    DirectReadData = (PDIRECT_READ_DATA) WaitParameter;

    Status = DereferenceIoHandleNoCheck(DirectReadData->ProcessData,
                                        DirectReadData->HandleIndex,
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

    //
    // if ctrl-c or ctrl-break was seen, ignore it.
    //

    if ((ULONG_PTR)SatisfyParameter2 & (CONSOLE_CTRL_C_SEEN | CONSOLE_CTRL_BREAK_SEEN)) {
        return FALSE;
    }

    Console = DirectReadData->Console;

#if defined(FE_SB)
    if (CONSOLE_IS_DBCS_CP(Console) && !a->Unicode) {
        if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
            if (a->NumRecords == 1) {
                Buffer = &a->Record[0];
                *Buffer = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte;
                if (!(a->Flags & CONSOLE_READ_NOREMOVE))
                    RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
                return TRUE;
            }
        }
    }
#endif

    //
    // this routine should be called by a thread owning the same
    // lock on the same console as we're reading from.
    //

    try {
        LockReadCount(HandleData);
        ASSERT(HandleData->InputReadData->ReadCount);
        HandleData->InputReadData->ReadCount -= 1;
        UnlockReadCount(HandleData);

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

        //
        // if the read buffer is contained within the message, we need to
        // reset the buffer pointer because the message copied from the
        // stack to heap space when the wait block was created.
        //

        if (a->NumRecords <= INPUT_RECORD_BUFFER_SIZE) {
            Buffer = a->Record;
        } else {
            Buffer = a->BufPtr;
        }
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_CP(Console) ) {
            Console->ReadConInpNumBytesUnicode = a->NumRecords;
            if (!a->Unicode) {
                /*
                 * ASCII : a->NumRecords is ASCII byte count
                 */
                if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                    /*
                     * Saved DBCS Traling byte
                     */
                    if (Console->ReadConInpNumBytesUnicode != 1) {
                        Console->ReadConInpNumBytesUnicode--;
                        Buffer++;
                        fAddDbcsLead = TRUE;
                        nLength = &Console->ReadConInpNumBytesUnicode;
                    }
                    else {
                        ASSERT(Console->ReadConInpNumBytesUnicode==1);
                    }
                }
                else {
                    nLength = &Console->ReadConInpNumBytesUnicode;
                }
            }
            else {
                nLength = &a->NumRecords;
            }
        }
        else {
            nLength = &a->NumRecords;
        }
        Status = ReadInputBuffer(DirectReadData->InputInfo,
                                 Buffer,
                                 nLength,
                                 !!(a->Flags & CONSOLE_READ_NOREMOVE),
                                 !(a->Flags & CONSOLE_READ_NOWAIT),
                                 FALSE,
                                 Console,
                                 HandleData,
                                 WaitReplyMessage,
                                 DirectReadWaitRoutine,
                                 &DirectReadData,
                                 sizeof(DirectReadData),
                                 TRUE,
                                 a->Unicode
                                );
#else
        Status = ReadInputBuffer(DirectReadData->InputInfo,
                                 Buffer,
                                 &a->NumRecords,
                                 !!(a->Flags & CONSOLE_READ_NOREMOVE),
                                 !(a->Flags & CONSOLE_READ_NOWAIT),
                                 FALSE,
                                 Console,
                                 HandleData,
                                 WaitReplyMessage,
                                 DirectReadWaitRoutine,
                                 &DirectReadData,
                                 sizeof(DirectReadData),
                                 TRUE
                                );
#endif
        if (Status == CONSOLE_STATUS_WAIT) {
            RetVal = FALSE;
        }
    } finally {

        //
        // if the read was completed (status != wait), free the direct read
        // data.
        //

        if (Status != CONSOLE_STATUS_WAIT) {
            if (Status == STATUS_SUCCESS && !a->Unicode) {
#if defined(FE_SB)
                if (CONSOLE_IS_DBCS_CP(Console) ) {
                    if (fAddDbcsLead &&
                        HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                        a->NumRecords--;
                    }
                    a->NumRecords = FE_TranslateInputToOem(
                                        Console,
                                        Buffer,
                                        a->NumRecords,
                                        Console->ReadConInpNumBytesUnicode,
                                        a->Flags & CONSOLE_READ_NOREMOVE ?
                                            NULL :
                                            &HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte);
                    if (fAddDbcsLead &&
                        HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                        *(Buffer-1) = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte;
                        if (!(a->Flags & CONSOLE_READ_NOREMOVE))
                            RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));
                        a->NumRecords++;
                        Buffer--;
                    }
                }
                else {
                    TranslateInputToOem(Console,
                                         Buffer,
                                         a->NumRecords,
                                         Console->ReadConInpNumBytesUnicode,
                                         a->Flags & CONSOLE_READ_NOREMOVE ?
                                             NULL :
                                             &HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte
                                        );
                }
#else
                TranslateInputToOem(Console,
                                     Buffer,
                                     a->NumRecords
                                    );
#endif
            }
            WaitReplyMessage->ReturnValue = Status;
            ConsoleHeapFree(DirectReadData);
        }
    }

    return RetVal;

    //
    // satisfy the unreferenced parameter warnings.
    //

    UNREFERENCED_PARAMETER(WaitQueue);
    UNREFERENCED_PARAMETER(WaitingThread);
    UNREFERENCED_PARAMETER(SatisfyParameter2);
}


ULONG
SrvGetConsoleInput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine reads or peeks input events.  In both cases, the events
    are copied to the user's buffer.  In the read case they are removed
    from the input buffer and in the peek case they are not.

Arguments:

    m - message containing api parameters

    ReplyStatus - Indicates whether to reply to the dll port.

Return Value:

--*/

{
    PCONSOLE_GETCONSOLEINPUT_MSG a = (PCONSOLE_GETCONSOLEINPUT_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    PINPUT_RECORD Buffer;
    DIRECT_READ_DATA DirectReadData;
#ifdef FE_SB
    BOOLEAN fAddDbcsLead = FALSE;
    PDWORD  nLength;
#endif

    if (a->Flags & ~CONSOLE_READ_VALID) {
        return (ULONG)STATUS_INVALID_PARAMETER;
    }

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
    if (!NT_SUCCESS(Status)) {
        a->NumRecords = 0;
    } else {

        if (a->NumRecords <= INPUT_RECORD_BUFFER_SIZE) {
            Buffer = a->Record;
        } else {
            Buffer = a->BufPtr;
            if (!CsrValidateMessageBuffer(m, &a->BufPtr, a->NumRecords, sizeof(*Buffer))) {
                UnlockConsole(Console);
                return STATUS_INVALID_PARAMETER;
            }
        }

        //
        // if we're reading, wait for data.  if we're peeking, don't.
        //

        DirectReadData.InputInfo = HandleData->Buffer.InputBuffer;
        DirectReadData.Console = Console;
        DirectReadData.ProcessData = CONSOLE_PERPROCESSDATA();
        DirectReadData.HandleIndex = HANDLE_TO_INDEX(a->InputHandle);
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_CP(Console) ) {
            Console->ReadConInpNumBytesUnicode = a->NumRecords;
            if (!a->Unicode) {
                /*
                 * ASCII : a->NumRecords is ASCII byte count
                 */
                if (HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte.Event.KeyEvent.uChar.AsciiChar) {
                    /*
                     * Saved DBCS Traling byte
                     */
                    *Buffer = HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte;
                    if (!(a->Flags & CONSOLE_READ_NOREMOVE))
                        RtlZeroMemory(&HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte,sizeof(INPUT_RECORD));

                    if (Console->ReadConInpNumBytesUnicode == 1) {
                        UnlockConsole(Console);
                        return STATUS_SUCCESS;
                    }
                    else {
                        Console->ReadConInpNumBytesUnicode--;
                        Buffer++;
                        fAddDbcsLead = TRUE;
                        nLength = &Console->ReadConInpNumBytesUnicode;
                    }
                }
                else {
                    nLength = &Console->ReadConInpNumBytesUnicode;
                }
            }
            else {
                nLength = &a->NumRecords;
            }
        }
        else {
            nLength = &a->NumRecords;
        }
        Status = ReadInputBuffer(HandleData->Buffer.InputBuffer,
                                 Buffer,
                                 nLength,
                                 !!(a->Flags & CONSOLE_READ_NOREMOVE),
                                 !(a->Flags & CONSOLE_READ_NOWAIT),
                                 FALSE,
                                 Console,
                                 HandleData,
                                 m,
                                 DirectReadWaitRoutine,
                                 &DirectReadData,
                                 sizeof(DirectReadData),
                                 FALSE,
                                 a->Unicode
                                );
        if (Status == CONSOLE_STATUS_WAIT) {
            *ReplyStatus = CsrReplyPending;
        } else if (!a->Unicode) {
            a->NumRecords = TranslateInputToOem(Console,
                                                Buffer,
                                                fAddDbcsLead ?
                                                    a->NumRecords-1 :
                                                    a->NumRecords,
                                                Console->ReadConInpNumBytesUnicode,
                                                a->Flags & CONSOLE_READ_NOREMOVE ?
                                                    NULL :
                                                    &HandleData->Buffer.InputBuffer->ReadConInpDbcsLeadByte
                                               );
            if (fAddDbcsLead)
            {
                a->NumRecords++;
                Buffer--;
            }
        }
#else
        Status = ReadInputBuffer(HandleData->Buffer.InputBuffer,
                                 Buffer,
                                 &a->NumRecords,
                                 !!(a->Flags & CONSOLE_READ_NOREMOVE),
                                 !(a->Flags & CONSOLE_READ_NOWAIT),
                                 FALSE,
                                 Console,
                                 HandleData,
                                 m,
                                 DirectReadWaitRoutine,
                                 &DirectReadData,
                                 sizeof(DirectReadData),
                                 FALSE
                                );
        if (Status == CONSOLE_STATUS_WAIT) {
            *ReplyStatus = CsrReplyPending;
        } else if (!a->Unicode) {
            TranslateInputToOem(Console,
                                 Buffer,
                                 a->NumRecords
                                );
        }
#endif
    }
    UnlockConsole(Console);
    return Status;
}

ULONG
SrvWriteConsoleInput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_WRITECONSOLEINPUT_MSG a = (PCONSOLE_WRITECONSOLEINPUT_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    PINPUT_RECORD Buffer;

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
    if (!NT_SUCCESS(Status)) {
        a->NumRecords = 0;
    } else {
        if (a->NumRecords <= INPUT_RECORD_BUFFER_SIZE) {
            Buffer = a->Record;
        } else {
            Buffer = a->BufPtr;
            if (!CsrValidateMessageBuffer(m, &a->BufPtr, a->NumRecords, sizeof(*Buffer))) {
                UnlockConsole(Console);
                return STATUS_INVALID_PARAMETER;
            }
        }
        if (!a->Unicode) {
#if defined(FE_SB)
            a->NumRecords = TranslateInputToUnicode(Console,
                                    Buffer,
                                    a->NumRecords,
                                    &HandleData->Buffer.InputBuffer->WriteConInpDbcsLeadByte[0]
                                   );
#else
            TranslateInputToUnicode(Console,
                                    Buffer,
                                    a->NumRecords
                                   );
#endif
        }
        if (a->Append) {
            a->NumRecords = WriteInputBuffer(Console,
                                             HandleData->Buffer.InputBuffer,
                                             Buffer,
                                             a->NumRecords
                                            );
        } else {
            a->NumRecords = PrependInputBuffer(Console,
                                             HandleData->Buffer.InputBuffer,
                                             Buffer,
                                             a->NumRecords
                                            );

        }
    }
    UnlockConsole(Console);
    return((ULONG) Status);
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

NTSTATUS
SB_TranslateOutputToOem
    (
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size
    )
// this is used when the app reads oem from the output buffer
// the app wants real OEM characters.  We have real Unicode or UnicodeOem.
{
    SHORT i,j;
    UINT Codepage;
    DBGCHARS(("SB_TranslateOutputToOem(Console=%lx, OutputBuffer=%lx)\n",
            Console, OutputBuffer));

    j = Size.X * Size.Y;

    if ((Console->CurrentScreenBuffer->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_ENABLED() &&
            Console->OutputCP != WINDOWSCP ) {
                Codepage = USACP;
        }
        else
#endif
        // we have UnicodeOem characters
        Codepage = WINDOWSCP;
    } else {
        // we have real Unicode characters
        Codepage = Console->OutputCP;    // BUG FIX by KazuM Jun.2.97
    }

    for (i=0;i<j;i++,OutputBuffer++) {
        OutputBuffer->Char.AsciiChar = WcharToChar(Codepage,
                OutputBuffer->Char.UnicodeChar);
    }
    return STATUS_SUCCESS;
}

#if defined(FE_SB)
NTSTATUS
FE_TranslateOutputToOem(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size
    )
// this is used when the app reads oem from the output buffer
// the app wants real OEM characters.  We have real Unicode or UnicodeOem.
{
    SHORT i,j;
    UINT Codepage;
    PCHAR_INFO TmpBuffer,SaveBuffer;
    CHAR  AsciiDbcs[2];
    ULONG NumBytes;
    DBGCHARS(("FE_TranslateOutputToOem(Console=%lx, OutputBuffer=%lx)\n",
            Console, OutputBuffer));

    SaveBuffer = TmpBuffer =
        ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ), Size.X * Size.Y * sizeof(CHAR_INFO) * 2);
    if (TmpBuffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    if ((Console->CurrentScreenBuffer->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
        if (CONSOLE_IS_DBCS_ENABLED() &&
            Console->OutputCP != WINDOWSCP ) {
                Codepage = USACP;
        }
        else
            // we have UnicodeOem characters
            Codepage = WINDOWSCP;
    } else {
        // we have real Unicode characters
        Codepage = Console->OutputCP;
    }

    memcpy(TmpBuffer,OutputBuffer,Size.X * Size.Y * sizeof(CHAR_INFO));
    for (i=0; i < Size.Y; i++) {
        for (j=0; j < Size.X; j++) {
            if (TmpBuffer->Attributes & COMMON_LVB_LEADING_BYTE) {
                if (j < Size.X-1) {  // -1 is safe DBCS in buffer
                    j++;
                    NumBytes = sizeof(AsciiDbcs);
                    NumBytes = ConvertOutputToOem(Codepage,
                                   &TmpBuffer->Char.UnicodeChar,
                                   1,
                                   &AsciiDbcs[0],
                                   NumBytes);
                    OutputBuffer->Char.AsciiChar = AsciiDbcs[0];
                    OutputBuffer->Attributes = TmpBuffer->Attributes;
                    OutputBuffer++;
                    TmpBuffer++;
                    OutputBuffer->Char.AsciiChar = AsciiDbcs[1];
                    OutputBuffer->Attributes = TmpBuffer->Attributes;
                    OutputBuffer++;
                    TmpBuffer++;
                }
                else {
                    OutputBuffer->Char.AsciiChar = ' ';
                    OutputBuffer->Attributes = TmpBuffer->Attributes & ~COMMON_LVB_SBCSDBCS;
                    OutputBuffer++;
                    TmpBuffer++;
                }
            }
            else if (!(TmpBuffer->Attributes & COMMON_LVB_SBCSDBCS)){
                ConvertOutputToOem(Codepage,
                    &TmpBuffer->Char.UnicodeChar,
                    1,
                    &OutputBuffer->Char.AsciiChar,
                    1);
                OutputBuffer->Attributes = TmpBuffer->Attributes;
                OutputBuffer++;
                TmpBuffer++;
            }
        }
    }
    ConsoleHeapFree(SaveBuffer);
    return STATUS_SUCCESS;
}
#endif

NTSTATUS
SB_TranslateOutputToOemUnicode
    (
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size
    )
// this is used when the app reads unicode from the output buffer
{
    SHORT i,j;
    DBGCHARS(("SB_TranslateOutputToOemUnicode\n"));

    j = Size.X * Size.Y;

    for (i=0;i<j;i++,OutputBuffer++) {
        FalseUnicodeToRealUnicode(&OutputBuffer->Char.UnicodeChar,
                                1,
                                Console->OutputCP
                                );
    }
    return STATUS_SUCCESS;
}

#if defined(FE_SB)
NTSTATUS
FE_TranslateOutputToOemUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size,
    IN BOOL fRemoveDbcsMark
    )
// this is used when the app reads unicode from the output buffer
{
    SHORT i,j;
    DBGCHARS(("FE_TranslateOutputToOemUnicode\n"));

    j = Size.X * Size.Y;

    if (fRemoveDbcsMark)
        RemoveDbcsMarkCell(OutputBuffer,OutputBuffer,j);

    for (i=0;i<j;i++,OutputBuffer++) {
        FalseUnicodeToRealUnicode(&OutputBuffer->Char.UnicodeChar,
                                1,
                                Console->OutputCP
                                );
    }
    return STATUS_SUCCESS;
}
#endif


NTSTATUS
SB_TranslateOutputToUnicode
    (
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size
    )
// this is used when the app writes oem to the output buffer
// we want UnicodeOem or Unicode in the buffer, depending on font & fullscreen
{
    SHORT i,j;
    UINT Codepage;
    DBGCHARS(("SB_TranslateOutputToUnicode %lx %lx (%lx,%lx)\n",
            Console, OutputBuffer, Size.X, Size.Y));

    j = Size.X * Size.Y;

    if ((Console->CurrentScreenBuffer->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_ENABLED() &&
            (Console->OutputCP != WINDOWSCP) ) {
                Codepage = USACP;
        }
        else
#endif
        // we want UnicodeOem characters
        Codepage = WINDOWSCP;
    } else {
        // we want real Unicode characters
        Codepage = Console->OutputCP;    // BUG FIX by KazuM Jun.2.97
    }
    for (i = 0; i < j; i++, OutputBuffer++) {
#if defined(FE_SB)
        OutputBuffer->Char.UnicodeChar = SB_CharToWchar(
                Codepage, OutputBuffer->Char.AsciiChar);
#else
        OutputBuffer->Char.UnicodeChar = CharToWchar(
                Codepage, OutputBuffer->Char.AsciiChar);
#endif
    }
    return STATUS_SUCCESS;
}

#if defined(FE_SB)
NTSTATUS
FE_TranslateOutputToUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size
    )
// this is used when the app writes oem to the output buffer
// we want UnicodeOem or Unicode in the buffer, depending on font & fullscreen
{
    SHORT i,j;
    UINT Codepage;
    CHAR  AsciiDbcs[2];
    WCHAR UnicodeDbcs[2];
    DBGCHARS(("FE_TranslateOutputToUnicode %lx %lx (%lx,%lx)\n",
            Console, OutputBuffer, Size.X, Size.Y));

    if ((Console->CurrentScreenBuffer->Flags & CONSOLE_OEMFONT_DISPLAY) &&
            ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
        if (CONSOLE_IS_DBCS_ENABLED() &&
            (Console->OutputCP != WINDOWSCP) ) {
                Codepage = USACP;
        }
        else
            // we want UnicodeOem characters
            Codepage = WINDOWSCP;
    } else {
        // we want real Unicode characters
        Codepage = Console->OutputCP;
    }

    for (i=0; i < Size.Y; i++) {
        for (j=0; j < Size.X; j++) {
            OutputBuffer->Attributes &= ~COMMON_LVB_SBCSDBCS;
            if (IsDBCSLeadByteConsole(OutputBuffer->Char.AsciiChar,&Console->OutputCPInfo)) {
                if (j < Size.X-1) {  // -1 is safe DBCS in buffer
                    j++;
                    AsciiDbcs[0] = OutputBuffer->Char.AsciiChar;
                    AsciiDbcs[1] = (OutputBuffer+1)->Char.AsciiChar;
                    ConvertOutputToUnicode(Codepage,
                                           &AsciiDbcs[0],
                                           2,
                                           &UnicodeDbcs[0],
                                           2);
                    OutputBuffer->Char.UnicodeChar = UnicodeDbcs[0];
                    OutputBuffer->Attributes |= COMMON_LVB_LEADING_BYTE;
                    OutputBuffer++;
                    OutputBuffer->Char.UnicodeChar = UNICODE_DBCS_PADDING;
                    OutputBuffer->Attributes &= ~COMMON_LVB_SBCSDBCS;
                    OutputBuffer->Attributes |= COMMON_LVB_TRAILING_BYTE;
                    OutputBuffer++;
                }
                else {
                    OutputBuffer->Char.UnicodeChar = UNICODE_SPACE;
                    OutputBuffer++;
                }
            }
            else {
                CHAR c;
                c=OutputBuffer->Char.AsciiChar;
                ConvertOutputToUnicode(Codepage,
                                       &c,
                                       1,
                                       &OutputBuffer->Char.UnicodeChar,
                                       1);
                OutputBuffer++;
            }
        }
    }
    return STATUS_SUCCESS;
}
#endif


NTSTATUS
SB_TranslateOutputToAnsiUnicode (
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size
    )
// this is used when the app writes unicode to the output buffer
{
    SHORT i,j;
    DBGCHARS(("TranslateOutputToAnsiUnicode\n"));

    j = Size.X * Size.Y;

    for (i=0;i<j;i++,OutputBuffer++) {
        RealUnicodeToFalseUnicode(&OutputBuffer->Char.UnicodeChar,
                                1,
                                Console->OutputCP
                                );
    }
    return STATUS_SUCCESS;
}

NTSTATUS
FE_TranslateOutputToAnsiUnicodeInternal(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size,
    IN OUT PCHAR_INFO OutputBufferR,
    IN BOOL fRealUnicodeToFalseUnicode
    )
// this is used when the app writes unicode to the output buffer
{
    SHORT i,j;
    DBGCHARS(("TranslateOutputToAnsiUnicode\n"));

    for (i=0; i < Size.Y; i++) {
        for (j=0; j < Size.X; j++) {
            WCHAR wch = OutputBuffer->Char.UnicodeChar;

            if (fRealUnicodeToFalseUnicode) {
                RealUnicodeToFalseUnicode(&OutputBuffer->Char.UnicodeChar,
                                          1,
                                          Console->OutputCP
                                         );
            }

            if (OutputBufferR) {
                OutputBufferR->Attributes = OutputBuffer->Attributes & ~COMMON_LVB_SBCSDBCS;
                if (IsConsoleFullWidth(Console->hDC,
                                       Console->OutputCP,OutputBuffer->Char.UnicodeChar)) {
                    if (j == Size.X-1){
                        OutputBufferR->Char.UnicodeChar = UNICODE_SPACE;
                    }
                    else{
                        OutputBufferR->Char.UnicodeChar = OutputBuffer->Char.UnicodeChar;
                        OutputBufferR->Attributes |= COMMON_LVB_LEADING_BYTE;
                        OutputBufferR++;
                        OutputBufferR->Char.UnicodeChar = UNICODE_DBCS_PADDING;
                        OutputBufferR->Attributes = OutputBuffer->Attributes & ~COMMON_LVB_SBCSDBCS;
                        OutputBufferR->Attributes |= COMMON_LVB_TRAILING_BYTE;
                    }
                }
                else{
                    OutputBufferR->Char.UnicodeChar = OutputBuffer->Char.UnicodeChar;
                }
                OutputBufferR++;
            }

            if (IsConsoleFullWidth(Console->hDC,
                                   Console->OutputCP,
                                   wch)) {
                if (j != Size.X-1){
                    j++;
                }
            }
            OutputBuffer++;
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
FE_TranslateOutputToAnsiUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size,
    IN OUT PCHAR_INFO OutputBufferR
    )
// this is used when the app writes unicode to the output buffer
{
    return FE_TranslateOutputToAnsiUnicodeInternal(Console,
                                                   OutputBuffer,
                                                   Size,
                                                   OutputBufferR,
                                                   TRUE
                                                   );
}

NTSTATUS
TranslateOutputToPaddingUnicode(
    IN PCONSOLE_INFORMATION Console,
    IN OUT PCHAR_INFO OutputBuffer,
    IN COORD Size,
    IN OUT PCHAR_INFO OutputBufferR
    )
{
    return FE_TranslateOutputToAnsiUnicodeInternal(Console,
                                                   OutputBuffer,
                                                   Size,
                                                   OutputBufferR,
                                                   FALSE
                                                   );
}

ULONG
SrvReadConsoleOutput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_READCONSOLEOUTPUT_MSG a = (PCONSOLE_READCONSOLEOUTPUT_MSG)&m->u.ApiMessageData;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    PCHAR_INFO Buffer;

    DBGOUTPUT(("SrvReadConsoleOutput\n"));
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
    if (!NT_SUCCESS(Status)) {
        //
        // a region of zero size is indicated by the right and bottom
        // coordinates being less than the left and top.
        //

        a->CharRegion.Right = (USHORT) (a->CharRegion.Left-1);
        a->CharRegion.Bottom = (USHORT) (a->CharRegion.Top-1);
    }
    else {
        COORD BufferSize;

        BufferSize.X = (SHORT)(a->CharRegion.Right - a->CharRegion.Left + 1);
        BufferSize.Y = (SHORT)(a->CharRegion.Bottom - a->CharRegion.Top + 1);

        if ((BufferSize.X == 1) && (BufferSize.Y == 1)) {
            Buffer = &a->Char;
        }
        else {
            Buffer = a->BufPtr;
            if (!CsrValidateMessageBuffer(m, &a->BufPtr, BufferSize.X * BufferSize.Y, sizeof(*Buffer))) {
                UnlockConsole(Console);
                return STATUS_INVALID_PARAMETER;
            }
        }

        Status = ReadScreenBuffer(HandleData->Buffer.ScreenBuffer,
                                  Buffer,
                                  &a->CharRegion
                                 );
        if (!a->Unicode) {
            TranslateOutputToOem(Console,
                                  Buffer,
                                  BufferSize
                                 );
        } else if ((Console->CurrentScreenBuffer->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                !(Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
            TranslateOutputToOemUnicode(Console,
                                        Buffer,
                                        BufferSize
#if defined(FE_SB)
                                        ,
                                        TRUE
#endif
                                       );
        }
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvWriteConsoleOutput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_WRITECONSOLEOUTPUT_MSG a = (PCONSOLE_WRITECONSOLEOUTPUT_MSG)&m->u.ApiMessageData;
    PSCREEN_INFORMATION ScreenBufferInformation;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    PCHAR_INFO Buffer;
#if defined(FE_SB)
    PCHAR_INFO TransBuffer = NULL;
#endif

    DBGOUTPUT(("SrvWriteConsoleOutput\n"));
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
    if (!NT_SUCCESS(Status)) {

        //
        // a region of zero size is indicated by the right and bottom
        // coordinates being less than the left and top.
        //

        a->CharRegion.Right = (USHORT) (a->CharRegion.Left-1);
        a->CharRegion.Bottom = (USHORT) (a->CharRegion.Top-1);
    } else {

        COORD BufferSize;
        ULONG NumBytes;

        BufferSize.X = (SHORT)(a->CharRegion.Right - a->CharRegion.Left + 1);
        BufferSize.Y = (SHORT)(a->CharRegion.Bottom - a->CharRegion.Top + 1);
        NumBytes = BufferSize.X * BufferSize.Y * sizeof(*Buffer);

        if ((BufferSize.X == 1) && (BufferSize.Y == 1)) {
            Buffer = &a->Char;
        } else if (a->ReadVM) {
            Buffer = ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),NumBytes);
            if (Buffer == NULL) {
                UnlockConsole(Console);
                return (ULONG)STATUS_NO_MEMORY;
            }
            Status = NtReadVirtualMemory(CONSOLE_CLIENTPROCESSHANDLE(),
                                         a->BufPtr,
                                         Buffer,
                                         NumBytes,
                                         NULL
                                        );
            if (!NT_SUCCESS(Status)) {
                ConsoleHeapFree(Buffer);
                UnlockConsole(Console);
                return (ULONG)STATUS_NO_MEMORY;
            }
        } else {
            Buffer = a->BufPtr;
            if (!CsrValidateMessageBuffer(m, &a->BufPtr, NumBytes, sizeof(BYTE))) {
                UnlockConsole(Console);
                return STATUS_INVALID_PARAMETER;
            }
        }
        ScreenBufferInformation = HandleData->Buffer.ScreenBuffer;

        if (!a->Unicode) {
            TranslateOutputToUnicode(Console,
                                     Buffer,
                                     BufferSize
                                    );
#if defined(FE_SB)
            Status = WriteScreenBuffer(ScreenBufferInformation,
                                       Buffer,
                                       &a->CharRegion
                                      );
#endif
        } else if ((Console->CurrentScreenBuffer->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_OUTPUTCP(Console)) {
                TransBuffer = (PCHAR_INFO)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),(BufferSize.Y * BufferSize.X) * 2 * sizeof(CHAR_INFO));
                if (TransBuffer == NULL) {
                    UnlockConsole(Console);
                    return (ULONG)STATUS_NO_MEMORY;
                }
                FE_TranslateOutputToAnsiUnicode(Console,
                                            Buffer,
                                            BufferSize,
                                            &TransBuffer[0]
                                           );
                Status = WriteScreenBuffer(ScreenBufferInformation,
                                            &TransBuffer[0],
                                            &a->CharRegion
                                           );
                ConsoleHeapFree(TransBuffer);
            }
            else {
                SB_TranslateOutputToAnsiUnicode(Console,
                                                Buffer,
                                                BufferSize
                                               );
                Status = WriteScreenBuffer(ScreenBufferInformation,
                                            Buffer,
                                            &a->CharRegion
                                           );
            }
#else
            TranslateOutputToAnsiUnicode(Console,
                                        Buffer,
                                        BufferSize
                                       );
#endif
        }
#if defined(FE_SB)
        else
#endif
        Status = WriteScreenBuffer(ScreenBufferInformation,
                                    Buffer,
                                    &a->CharRegion
                                   );

        if (a->ReadVM) {
            ConsoleHeapFree(Buffer);
        }
        if (NT_SUCCESS(Status)) {

            //
            // cause screen to be updated
            //

#if defined(FE_SB)
            if (CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
                Console->Flags & CONSOLE_JUST_VDM_UNREGISTERED ){
                int MouseRec;
                MouseRec = Console->InputBuffer.InputMode;
                Console->InputBuffer.InputMode &= ~ENABLE_MOUSE_INPUT;
                Console->CurrentScreenBuffer->BufferInfo.TextInfo.Flags &= ~TEXT_VALID_HINT;
                WriteToScreen(ScreenBufferInformation,&a->CharRegion );
                Console->CurrentScreenBuffer->BufferInfo.TextInfo.Flags |= TEXT_VALID_HINT;
                Console->InputBuffer.InputMode = MouseRec;
            }
            else
#endif
            WriteToScreen(ScreenBufferInformation,
                          &a->CharRegion
                         );
        }
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


ULONG
SrvReadConsoleOutputString(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PVOID Buffer;
    PCONSOLE_READCONSOLEOUTPUTSTRING_MSG a = (PCONSOLE_READCONSOLEOUTPUTSTRING_MSG)&m->u.ApiMessageData;
    ULONG nSize;

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
    if (!NT_SUCCESS(Status)) {

        //
        // a region of zero size is indicated by the right and bottom
        // coordinates being less than the left and top.
        //

        a->NumRecords = 0;
    } else {
        if (a->StringType == CONSOLE_ASCII)
            nSize = sizeof(CHAR);
        else
            nSize = sizeof(WORD);
        if ((a->NumRecords*nSize) > sizeof(a->String)) {
            Buffer = a->BufPtr;
            if (!CsrValidateMessageBuffer(m, &a->BufPtr, a->NumRecords, nSize)) {
                UnlockConsole(Console);
                return STATUS_INVALID_PARAMETER;
            }
        }
        else {
            Buffer = a->String;
        }
        Status = ReadOutputString(HandleData->Buffer.ScreenBuffer,
                                Buffer,
                                a->ReadCoord,
                                a->StringType,
                                &a->NumRecords
                               );
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvWriteConsoleOutputString(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_WRITECONSOLEOUTPUTSTRING_MSG a = (PCONSOLE_WRITECONSOLEOUTPUTSTRING_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    PHANDLE_DATA HandleData;
    PVOID Buffer;
    ULONG nSize;

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
    if (!NT_SUCCESS(Status)) {
        a->NumRecords = 0;
    } else {
        if (a->WriteCoord.X < 0 ||
            a->WriteCoord.Y < 0) {
            Status = STATUS_INVALID_PARAMETER;
        } else {
            if (a->StringType == CONSOLE_ASCII)
                nSize = sizeof(CHAR);
            else
                nSize = sizeof(WORD);
            if ((a->NumRecords*nSize) > sizeof(a->String)) {
                Buffer = a->BufPtr;
                if (!CsrValidateMessageBuffer(m, &a->BufPtr, a->NumRecords, nSize)) {
                    UnlockConsole(Console);
                    return STATUS_INVALID_PARAMETER;
                }
            }
            else {
                Buffer = a->String;
            }
            Status = WriteOutputString(HandleData->Buffer.ScreenBuffer,
                                     Buffer,
                                     a->WriteCoord,
                                     a->StringType,
                                     &a->NumRecords,
                                     NULL
                                    );
        }
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}

ULONG
SrvFillConsoleOutput(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )
{
    PCONSOLE_FILLCONSOLEOUTPUT_MSG a = (PCONSOLE_FILLCONSOLEOUTPUT_MSG)&m->u.ApiMessageData;
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
    if (!NT_SUCCESS(Status)) {
        a->Length = 0;
    } else {
        Status = FillOutput(HandleData->Buffer.ScreenBuffer,
                          a->Element,
                          a->WriteCoord,
                          a->ElementType,
                          &a->Length
                         );
    }
    UnlockConsole(Console);
    return Status;
    UNREFERENCED_PARAMETER(ReplyStatus);    // get rid of unreferenced parameter warning message
}


ULONG
SrvCreateConsoleScreenBuffer(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    )

/*++

Routine Description:

    This routine creates a screen buffer and returns a handle to it.

Arguments:

    ApiMessageData - Points to parameter structure.

Return Value:

--*/

{
    PCONSOLE_CREATESCREENBUFFER_MSG a = (PCONSOLE_CREATESCREENBUFFER_MSG)&m->u.ApiMessageData;
    NTSTATUS Status;
    PCONSOLE_INFORMATION Console;
    HANDLE Handle;
    PHANDLE_DATA HandleData;
    PCONSOLE_SHARE_ACCESS ShareAccess;
    CHAR_INFO Fill;
    COORD WindowSize;
    PSCREEN_INFORMATION ScreenInfo;
    PCONSOLE_PER_PROCESS_DATA ProcessData;
    ULONG HandleType;
    int FontIndex;

    DBGOUTPUT(("SrvCreateConsoleScreenBuffer\n"));
    Status = ApiPreamble(a->ConsoleHandle,
                         &Console
                        );
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    if (a->Flags & CONSOLE_GRAPHICS_BUFFER) {
        if (!CsrValidateMessageBuffer(m, &a->GraphicsBufferInfo.lpBitMapInfo, a->GraphicsBufferInfo.dwBitMapInfoLength, sizeof(BYTE))) {
            UnlockConsole(Console);
            return STATUS_INVALID_PARAMETER;
        }
    }

    try {
        Handle = (HANDLE) -1;
        ProcessData = CONSOLE_PERPROCESSDATA();
        HandleType = (a->Flags & CONSOLE_GRAPHICS_BUFFER) ?
                      CONSOLE_GRAPHICS_OUTPUT_HANDLE : CONSOLE_OUTPUT_HANDLE;
        if (a->InheritHandle)
            HandleType |= CONSOLE_INHERITABLE;
        Status = AllocateIoHandle(ProcessData,
                                  HandleType,
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

        //
        // create new screen buffer
        //

        Fill.Char.UnicodeChar = (WCHAR)' ';
        Fill.Attributes = Console->CurrentScreenBuffer->Attributes;
        WindowSize.X = (SHORT)CONSOLE_WINDOW_SIZE_X(Console->CurrentScreenBuffer);
        WindowSize.Y = (SHORT)CONSOLE_WINDOW_SIZE_Y(Console->CurrentScreenBuffer);
        FontIndex = FindCreateFont(CON_FAMILY(Console),
                                   CON_FACENAME(Console),
                                   CON_FONTSIZE(Console),
                                   CON_FONTWEIGHT(Console),
                                   CON_FONTCODEPAGE(Console)
                                  );
        Status = CreateScreenBuffer(&ScreenInfo,WindowSize,
                                    FontIndex,
                                    WindowSize,
                                    Fill,Fill,Console,
                                    a->Flags,&a->GraphicsBufferInfo,
                                    &a->lpBitmap,&a->hMutex,
                                    CURSOR_SMALL_SIZE, 
                                    NULL);
        if (!NT_SUCCESS(Status)) {
            leave;
        }
        InitializeOutputHandle(HandleData,ScreenInfo);
        ShareAccess = &ScreenInfo->ShareAccess;

        Status = ConsoleAddShare(a->DesiredAccess,
                                 a->ShareMode,
                                 ShareAccess,
                                 HandleData
                                );
        if (!NT_SUCCESS(Status)) {
            HandleData->Buffer.ScreenBuffer->RefCount--;
            FreeScreenBuffer(ScreenInfo);
            leave;
        }
        InsertScreenBuffer(Console, ScreenInfo);
        a->Handle = INDEX_TO_HANDLE(Handle);
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
