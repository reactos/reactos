/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    _stream.h

Abstract:

    Performance critical routine for Single Binary

    Each function will be created with two flavors FE and non FE

Author:

    KazuM Jun.09.1997

Revision History:

--*/

#define WWSB_NEUTRAL_FILE 1

#if !defined(FE_SB)
#error This header file should be included with FE_SB
#endif

#if !defined(WWSB_FE) && !defined(WWSB_NOFE)
#error Either WWSB_FE and WWSB_NOFE must be defined.
#endif

#if defined(WWSB_FE) && defined(WWSB_NOFE)
#error Both WWSB_FE and WWSB_NOFE defined.
#endif

#ifdef WWSB_FE
#pragma alloc_text(FE_TEXT, FE_AdjustCursorPosition)
#pragma alloc_text(FE_TEXT, FE_WriteChars)
#pragma alloc_text(FE_TEXT, FE_DoWriteConsole)
#pragma alloc_text(FE_TEXT, FE_DoSrvWriteConsole)
#endif


NTSTATUS
WWSB_AdjustCursorPosition(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN COORD CursorPosition,
    IN BOOL KeepCursorVisible,
    OUT PSHORT ScrollY OPTIONAL
    )

/*++

Routine Description:

    This routine updates the cursor position.  Its input is the non-special
    cased new location of the cursor.  For example, if the cursor were being
    moved one space backwards from the left edge of the screen, the X
    coordinate would be -1.  This routine would set the X coordinate to
    the right edge of the screen and decrement the Y coordinate by one.

Arguments:

    ScreenInfo - Pointer to screen buffer information structure.

    CursorPosition - New location of cursor.

    KeepCursorVisible - TRUE if changing window origin desirable when hit right edge

Return Value:

--*/

{
    COORD WindowOrigin;
    NTSTATUS Status;
#ifdef WWSB_FE
    PCONSOLE_INFORMATION Console = ScreenInfo->Console;

    if (!(ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER))
        return STATUS_SUCCESS;
#endif

    if (CursorPosition.X < 0) {
        if (CursorPosition.Y > 0) {
            CursorPosition.X = (SHORT)(ScreenInfo->ScreenBufferSize.X+CursorPosition.X);
            CursorPosition.Y = (SHORT)(CursorPosition.Y-1);
        }
        else {
            CursorPosition.X = 0;
        }
    }
    else if (CursorPosition.X >= ScreenInfo->ScreenBufferSize.X) {

        //
        // at end of line. if wrap mode, wrap cursor.  otherwise leave it
        // where it is.
        //

        if (ScreenInfo->OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT) {
            CursorPosition.Y += CursorPosition.X / ScreenInfo->ScreenBufferSize.X;
            CursorPosition.X = CursorPosition.X % ScreenInfo->ScreenBufferSize.X;
        }
        else {
            CursorPosition.X = ScreenInfo->BufferInfo.TextInfo.CursorPosition.X;
        }
    }
#ifdef WWSB_FE
    if (CursorPosition.Y >= ScreenInfo->ScreenBufferSize.Y &&
        !(Console->InputBuffer.ImeMode.Open)
       )
#else
    if (CursorPosition.Y >= ScreenInfo->ScreenBufferSize.Y)
#endif
    {

        //
        // at end of buffer.  scroll contents of screen buffer so new
        // position is visible.
        //

        ASSERT (CursorPosition.Y == ScreenInfo->ScreenBufferSize.Y);
        StreamScrollRegion(ScreenInfo);

        if (ARGUMENT_PRESENT(ScrollY)) {
            *ScrollY += (SHORT)(ScreenInfo->ScreenBufferSize.Y - CursorPosition.Y - 1);
        }
        CursorPosition.Y += (SHORT)(ScreenInfo->ScreenBufferSize.Y - CursorPosition.Y - 1);
    }
#ifdef WWSB_FE
    else if (!(Console->InputBuffer.ImeMode.Disable) && Console->InputBuffer.ImeMode.Open)
    {
        if (CursorPosition.Y == (ScreenInfo->ScreenBufferSize.Y-1)) {
            ConsoleImeBottomLineUse(ScreenInfo,2);
            if (ARGUMENT_PRESENT(ScrollY)) {
                *ScrollY += (SHORT)(ScreenInfo->ScreenBufferSize.Y - CursorPosition.Y - 2);
            }
            CursorPosition.Y += (SHORT)(ScreenInfo->ScreenBufferSize.Y - CursorPosition.Y - 2);
            if (!ARGUMENT_PRESENT(ScrollY) && Console->lpCookedReadData) {
                ((PCOOKED_READ_DATA)(Console->lpCookedReadData))->OriginalCursorPosition.Y--;
            }
        }
        else if (CursorPosition.Y == ScreenInfo->Window.Bottom) {
            ;
        }
    }
#endif

    //
    // if at right or bottom edge of window, scroll right or down one char.
    //

#ifdef WWSB_FE
    if (CursorPosition.Y > ScreenInfo->Window.Bottom &&
        !(Console->InputBuffer.ImeMode.Open)
       )
#else
    if (CursorPosition.Y > ScreenInfo->Window.Bottom)
#endif
    {
        WindowOrigin.X = 0;
        WindowOrigin.Y = CursorPosition.Y - ScreenInfo->Window.Bottom;
        Status = SetWindowOrigin(ScreenInfo,
                               FALSE,
                               WindowOrigin
                              );
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }
#ifdef WWSB_FE
    else if (Console->InputBuffer.ImeMode.Open)
    {
        if (CursorPosition.Y >= ScreenInfo->Window.Bottom &&
            CONSOLE_WINDOW_SIZE_Y(ScreenInfo) > 1
           ) {
            WindowOrigin.X = 0;
            WindowOrigin.Y = CursorPosition.Y - ScreenInfo->Window.Bottom + 1;
            Status = SetWindowOrigin(ScreenInfo,
                                        FALSE,
                                        WindowOrigin
                                       );
            if (!NT_SUCCESS(Status)) {
                return Status;
            }
        }
    }
#endif
    if (KeepCursorVisible) {
        MakeCursorVisible(ScreenInfo,CursorPosition);
    }
    Status = SetCursorPosition(ScreenInfo,
                               CursorPosition,
                               KeepCursorVisible
                              );
    return Status;
}

#define LOCAL_BUFFER_SIZE 100

NTSTATUS
WWSB_WriteChars(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PWCHAR lpBufferBackupLimit,
    IN PWCHAR lpBuffer,
    IN PWCHAR lpRealUnicodeString,
    IN OUT PDWORD NumBytes,
    OUT PLONG NumSpaces OPTIONAL,
    IN SHORT OriginalXPosition,
    IN DWORD dwFlags,
    OUT PSHORT ScrollY OPTIONAL
    )

/*++

Routine Description:

    This routine writes a string to the screen, processing any embedded
    unicode characters.  The string is also copied to the input buffer, if
    the output mode is line mode.

Arguments:

    ScreenInfo - Pointer to screen buffer information structure.

    lpBufferBackupLimit - Pointer to beginning of buffer.

    lpBuffer - Pointer to buffer to copy string to.  assumed to be at least
    as long as lpRealUnicodeString.  This pointer is updated to point to the
    next position in the buffer.

    lpRealUnicodeString - Pointer to string to write.

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
    DWORD BufferSize;
    COORD CursorPosition;
    NTSTATUS Status;
    ULONG NumChars;
    static WCHAR Blanks[TAB_SIZE] = { UNICODE_SPACE,
                                      UNICODE_SPACE,
                                      UNICODE_SPACE,
                                      UNICODE_SPACE,
                                      UNICODE_SPACE,
                                      UNICODE_SPACE,
                                      UNICODE_SPACE,
                                      UNICODE_SPACE };
    SHORT XPosition;
    WCHAR LocalBuffer[LOCAL_BUFFER_SIZE];
    PWCHAR LocalBufPtr;
    ULONG i,j;
    SMALL_RECT Region;
    ULONG TabSize;
    DWORD TempNumSpaces;
    WCHAR Char;
    WCHAR RealUnicodeChar;
    WORD Attributes;
    PWCHAR lpString;
    PWCHAR lpAllocatedString;
    BOOL fUnprocessed = ((ScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT) == 0);
#ifdef WWSB_FE
    CHAR LocalBufferA[LOCAL_BUFFER_SIZE];
    PCHAR LocalBufPtrA;
#endif

    ConsoleHideCursor(ScreenInfo);

    Attributes = ScreenInfo->Attributes;
    BufferSize = *NumBytes;
    *NumBytes = 0;
    TempNumSpaces = 0;

    lpAllocatedString = NULL;
    if (dwFlags & WC_FALSIFY_UNICODE) {
        // translation from OEM -> ANSI -> OEM doesn't
        // necessarily yield the same value, so do
        // translation in a separate buffer.

        lpString = ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),BufferSize);
        if (lpString == NULL) {
            Status = STATUS_NO_MEMORY;
            goto ExitWriteChars;
        }

        lpAllocatedString = lpString;
        RtlCopyMemory(lpString, lpRealUnicodeString, BufferSize);
        Status = RealUnicodeToFalseUnicode(lpString,
                                         BufferSize / sizeof(WCHAR),
                                         ScreenInfo->Console->OutputCP
                                        );
        if (!NT_SUCCESS(Status)) {
            goto ExitWriteChars;
        }
    } else {
       lpString = lpRealUnicodeString;
    }

    while (*NumBytes < BufferSize) {

        //
        // as an optimization, collect characters in buffer and
        // print out all at once.
        //

        XPosition = ScreenInfo->BufferInfo.TextInfo.CursorPosition.X;
        i=0;
        LocalBufPtr = LocalBuffer;
#ifdef WWSB_FE
        LocalBufPtrA = LocalBufferA;
#endif
        while (*NumBytes < BufferSize &&
               i < LOCAL_BUFFER_SIZE &&
               XPosition < ScreenInfo->ScreenBufferSize.X) {
            Char = *lpString;
            RealUnicodeChar = *lpRealUnicodeString;
            if (!IS_GLYPH_CHAR(RealUnicodeChar) || fUnprocessed) {
#ifdef WWSB_FE
                if (IsConsoleFullWidth(ScreenInfo->Console->hDC,
                                       ScreenInfo->Console->OutputCP,Char)) {
                    if (i < (LOCAL_BUFFER_SIZE-1) &&
                        XPosition < (ScreenInfo->ScreenBufferSize.X-1)) {
                        *LocalBufPtr++ = Char;
                        *LocalBufPtrA++ = ATTR_LEADING_BYTE;
                        *LocalBufPtr++ = Char;
                        *LocalBufPtrA++ = ATTR_TRAILING_BYTE;
                        XPosition+=2;
                        i+=2;
                        lpBuffer++;
                    }
                    else
                        goto EndWhile;
                }
                else {
#endif
                    *LocalBufPtr = Char;
                    LocalBufPtr++;
                    XPosition++;
                    i++;
                    lpBuffer++;
#ifdef WWSB_FE
                    *LocalBufPtrA++ = 0;
                }
#endif
            } else {
                ASSERT(ScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT);
                switch (RealUnicodeChar) {
                    case UNICODE_BELL:
                        if (dwFlags & WC_ECHO) {
                            goto CtrlChar;
                        } else {
                            SendNotifyMessage(ScreenInfo->Console->hWnd,
                                              CM_BEEP,
                                              0,
                                              0x47474747);
                        }
                        break;
                    case UNICODE_BACKSPACE:

                        // automatically go to EndWhile.  this is because
                        // backspace is not destructive, so "aBkSp" prints
                        // a with the cursor on the "a". we could achieve
                        // this behavior staying in this loop and figuring out
                        // the string that needs to be printed, but it would
                        // be expensive and it's the exceptional case.

                        goto EndWhile;
                        break;
                    case UNICODE_TAB:
                        TabSize = NUMBER_OF_SPACES_IN_TAB(XPosition);
                        XPosition = (SHORT)(XPosition + TabSize);
                        if (XPosition >= ScreenInfo->ScreenBufferSize.X) {
                            goto EndWhile;
                        }
                        for (j=0;j<TabSize && i<LOCAL_BUFFER_SIZE;j++,i++) {
                            *LocalBufPtr = (WCHAR)' ';
                            LocalBufPtr++;
#ifdef WWSB_FE
                            *LocalBufPtrA++ = 0;
#endif
                        }
                        lpBuffer++;
                        break;
                    case UNICODE_LINEFEED:
                    case UNICODE_CARRIAGERETURN:
                        goto EndWhile;
                    default:

                        //
                        // if char is ctrl char, write ^char.
                        //

                        if ((dwFlags & WC_ECHO) && (IS_CONTROL_CHAR(RealUnicodeChar))) {

CtrlChar:                   if (i < (LOCAL_BUFFER_SIZE-1)) {
                                *LocalBufPtr = (WCHAR)'^';
                                LocalBufPtr++;
                                XPosition++;
                                i++;
                                *LocalBufPtr = (WCHAR)(RealUnicodeChar+(WCHAR)'@');
                                LocalBufPtr++;
                                XPosition++;
                                i++;
                                lpBuffer++;
#ifdef WWSB_FE
                                *LocalBufPtrA++ = 0;
                                *LocalBufPtrA++ = 0;
#endif
                            }
                            else {
                                goto EndWhile;
                            }
                        } else {
                            if (!(ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) ||
                                    (ScreenInfo->Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
                                /*
                                 * As a special favor to incompetent apps
                                 * that attempt to display control chars,
                                 * convert to corresponding OEM Glyph Chars
                                 */
#ifdef WWSB_FE
                                WORD CharType;

                                GetStringTypeW(CT_CTYPE1,&RealUnicodeChar,1,&CharType);
                                if (CharType == C1_CNTRL)
                                    ConvertOutputToUnicode(ScreenInfo->Console->OutputCP,
                                                           &(char)RealUnicodeChar,
                                                           1,
                                                           LocalBufPtr,
                                                           1);
                                else
                                    *LocalBufPtr = Char;
#else
                                *LocalBufPtr = SB_CharToWcharGlyph(
                                        ScreenInfo->Console->OutputCP,
                                        (char)RealUnicodeChar);
#endif
                            } else {
                                *LocalBufPtr = Char;
                            }
                            LocalBufPtr++;
                            XPosition++;
                            i++;
                            lpBuffer++;
#ifdef WWSB_FE
                            *LocalBufPtrA++ = 0;
#endif
                        }
                }
            }
            lpString++;
            lpRealUnicodeString++;
            *NumBytes += sizeof(WCHAR);
        }
EndWhile:
        if (i != 0) {

            //
            // Make sure we don't write past the end of the buffer.
            //

            if (i > (ULONG)ScreenInfo->ScreenBufferSize.X - ScreenInfo->BufferInfo.TextInfo.CursorPosition.X) {
                i = (ULONG)ScreenInfo->ScreenBufferSize.X - ScreenInfo->BufferInfo.TextInfo.CursorPosition.X;
            }

#ifdef WWSB_FE
            FE_StreamWriteToScreenBuffer(LocalBuffer,
                                         (SHORT)i,
                                         ScreenInfo,
                                         LocalBufferA
                                        );
#else
            SB_StreamWriteToScreenBuffer(LocalBuffer,
                                         (SHORT)i,
                                         ScreenInfo
                                        );
#endif
            Region.Left = ScreenInfo->BufferInfo.TextInfo.CursorPosition.X;
            Region.Right = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X + i - 1);
            Region.Top = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
            Region.Bottom = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
            WWSB_WriteToScreen(ScreenInfo,&Region);
            TempNumSpaces += i;
            CursorPosition.X = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X + i);
            CursorPosition.Y = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
            Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,
                    dwFlags & WC_KEEP_CURSOR_VISIBLE,ScrollY);
            if (*NumBytes == BufferSize) {
                ConsoleShowCursor(ScreenInfo);
                if (ARGUMENT_PRESENT(NumSpaces)) {
                    *NumSpaces = TempNumSpaces;
                }
                Status = STATUS_SUCCESS;
                goto ExitWriteChars;
            }
            continue;
        } else if (*NumBytes == BufferSize) {

            ASSERT(ScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT);
            // this catches the case where the number of backspaces ==
            // the number of characters.
            if (ARGUMENT_PRESENT(NumSpaces)) {
                *NumSpaces = TempNumSpaces;
            }
            ConsoleShowCursor(ScreenInfo);
            Status = STATUS_SUCCESS;
            goto ExitWriteChars;
        }

        ASSERT(ScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT);
        switch (*lpString) {
            case UNICODE_BACKSPACE:

                //
                // move cursor backwards one space. overwrite current char with blank.
                //
                // we get here because we have to backspace from the beginning of the line

                CursorPosition = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
                TempNumSpaces -= 1;
                if (lpBuffer == lpBufferBackupLimit) {
                    CursorPosition.X-=1;
                }
                else {
                    PWCHAR pBuffer;
                    WCHAR TmpBuffer[LOCAL_BUFFER_SIZE];
                    PWCHAR Tmp,Tmp2;
                    WCHAR LastChar;
                    ULONG i;

                    if (lpBuffer-lpBufferBackupLimit > LOCAL_BUFFER_SIZE) {
                        pBuffer = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),(ULONG)(lpBuffer-lpBufferBackupLimit) * sizeof(WCHAR));
                        if (pBuffer == NULL) {
                            Status = STATUS_NO_MEMORY;
                            goto ExitWriteChars;
                        }
                    } else {
                        pBuffer = TmpBuffer;
                    }

                    for (i=0,Tmp2=pBuffer,Tmp=lpBufferBackupLimit;
                         i<(ULONG)(lpBuffer-lpBufferBackupLimit);
                         i++,Tmp++) {
                        if (*Tmp == UNICODE_BACKSPACE) {
                            if (Tmp2 > pBuffer) {
                                Tmp2--;
                            }
                        } else {
                            ASSERT(Tmp2 >= pBuffer);
                            *Tmp2++ = *Tmp;
                        }

                    }
                    if (Tmp2 == pBuffer) {
                        LastChar = (WCHAR)' ';
                    } else {
                        LastChar = *(Tmp2-1);
                    }
                    if (pBuffer != TmpBuffer) {
                        ConsoleHeapFree(pBuffer);
                    }

                    if (LastChar == UNICODE_TAB) {
                        CursorPosition.X -=
                            (SHORT)(RetrieveNumberOfSpaces(OriginalXPosition,
                                                           lpBufferBackupLimit,
                                                           (ULONG)(lpBuffer - lpBufferBackupLimit - 1),
                                                           ScreenInfo->Console,
                                                           ScreenInfo->Console->OutputCP
                                                          ));
                        if (CursorPosition.X < 0) {
                            CursorPosition.X = (ScreenInfo->ScreenBufferSize.X - 1)/TAB_SIZE;
                            CursorPosition.X *= TAB_SIZE;
                            CursorPosition.X += 1;
                            CursorPosition.Y -= 1;
                        }
                    }
                    else if (IS_CONTROL_CHAR(LastChar)) {
                        CursorPosition.X-=1;
                        TempNumSpaces -= 1;

                        //
                        // overwrite second character of ^x sequence.
                        //

                        if (dwFlags & WC_DESTRUCTIVE_BACKSPACE) {
                            NumChars = 1;
                            Status = WWSB_WriteOutputString(ScreenInfo,
                                Blanks, CursorPosition,
                                CONSOLE_FALSE_UNICODE, // faster than real unicode
                                &NumChars, NULL);
                            Status = WWSB_FillOutput(ScreenInfo,
                                Attributes, CursorPosition,
                                CONSOLE_ATTRIBUTE, &NumChars);
                        }
                        CursorPosition.X-=1;
                    }
#ifdef WWSB_FE
                    else if (IsConsoleFullWidth(ScreenInfo->Console->hDC,
                                                ScreenInfo->Console->OutputCP,LastChar))
                    {
                        CursorPosition.X-=1;
                        TempNumSpaces -= 1;

                        Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,
                                     dwFlags & WC_KEEP_CURSOR_VISIBLE,ScrollY);
                        if (dwFlags & WC_DESTRUCTIVE_BACKSPACE) { // bug 7672
                            NumChars = 1;
                            Status = WWSB_WriteOutputString(ScreenInfo,
                                Blanks, ScreenInfo->BufferInfo.TextInfo.CursorPosition,
                                CONSOLE_FALSE_UNICODE, // faster than real unicode
                                &NumChars, NULL);
                            Status = WWSB_FillOutput(ScreenInfo,
                                Attributes, ScreenInfo->BufferInfo.TextInfo.CursorPosition,
                                CONSOLE_ATTRIBUTE, &NumChars);
                        }
                        CursorPosition.X-=1;
                    }
#endif
                    else {
                        CursorPosition.X--;
                    }
                }
                if ((dwFlags & WC_LIMIT_BACKSPACE) && (CursorPosition.X < 0)) {
                    CursorPosition.X = 0;
                    KdPrint(("CONSRV: Ignoring backspace to previous line\n"));
                }
                Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,
                        (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0,ScrollY);
                if (dwFlags & WC_DESTRUCTIVE_BACKSPACE) {
                    NumChars = 1;
                    Status = WWSB_WriteOutputString(ScreenInfo,
                        Blanks, ScreenInfo->BufferInfo.TextInfo.CursorPosition,
                        CONSOLE_FALSE_UNICODE, //faster than real unicode
                        &NumChars, NULL);
                    Status = WWSB_FillOutput(ScreenInfo,
                        Attributes, ScreenInfo->BufferInfo.TextInfo.CursorPosition,
                        CONSOLE_ATTRIBUTE, &NumChars);
                }
#ifdef WWSB_FE
                if (ScreenInfo->BufferInfo.TextInfo.CursorPosition.X == 0 &&
                    (ScreenInfo->OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT) &&
                    lpBuffer > lpBufferBackupLimit) {
                    if (CheckBisectProcessW(ScreenInfo,
                                            ScreenInfo->Console->OutputCP,
                                            lpBufferBackupLimit,
                                            (ULONG)(lpBuffer+1-lpBufferBackupLimit),
                                            ScreenInfo->ScreenBufferSize.X-OriginalXPosition,
                                            OriginalXPosition,
                                            dwFlags & WC_ECHO)) {
                        CursorPosition.X = ScreenInfo->ScreenBufferSize.X-1;
                        CursorPosition.Y = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y-1);
                        Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,
                                     dwFlags & WC_KEEP_CURSOR_VISIBLE,ScrollY);
                    }
                }
#endif
                break;
            case UNICODE_TAB:
                TabSize = NUMBER_OF_SPACES_IN_TAB(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X);
                CursorPosition.X = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X + TabSize);

                //
                // move cursor forward to next tab stop.  fill space with blanks.
                // we get here when the tab extends beyond the right edge of the
                // window.  if the tab goes wraps the line, set the cursor to the first
                // position in the next line.
                //

                lpBuffer++;

                TempNumSpaces += TabSize;
                if (CursorPosition.X >= ScreenInfo->ScreenBufferSize.X) {
                    NumChars = ScreenInfo->ScreenBufferSize.X - ScreenInfo->BufferInfo.TextInfo.CursorPosition.X;
                    CursorPosition.X = 0;
                    CursorPosition.Y = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y+1;
                }
                else {
                    NumChars = CursorPosition.X - ScreenInfo->BufferInfo.TextInfo.CursorPosition.X;
                    CursorPosition.Y = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
                }
                Status = WWSB_WriteOutputString(ScreenInfo,
                                                Blanks,
                                                ScreenInfo->BufferInfo.TextInfo.CursorPosition,
                                                CONSOLE_FALSE_UNICODE, // faster than real unicode
                                                &NumChars,
                                                NULL);
                Status = WWSB_FillOutput(ScreenInfo,
                                         Attributes, ScreenInfo->BufferInfo.TextInfo.CursorPosition,
                                         CONSOLE_ATTRIBUTE,
                                         &NumChars);
                Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,
                        (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0,ScrollY);
                break;
            case UNICODE_CARRIAGERETURN:

                //
                // Carriage return moves the cursor to the beginning of the line.
                // We don't need to worry about handling cr or lf for
                // backspace because input is sent to the user on cr or lf.
                //

                lpBuffer++;
                CursorPosition.X = 0;
                CursorPosition.Y = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
                Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,
                        (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0,ScrollY);
                break;
            case UNICODE_LINEFEED:

                //
                // move cursor to the beginning of the next line.
                //

                lpBuffer++;
                CursorPosition.X = 0;
                CursorPosition.Y = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y+1);
                Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,
                        (dwFlags & WC_KEEP_CURSOR_VISIBLE) != 0,ScrollY);
                break;
            default:
#ifdef WWSB_FE
                Char = *lpString;
                if (Char >= (WCHAR)' ' &&
                    IsConsoleFullWidth(ScreenInfo->Console->hDC,
                                       ScreenInfo->Console->OutputCP,Char) &&
                    XPosition >= (ScreenInfo->ScreenBufferSize.X-1) &&
                    (ScreenInfo->OutputMode & ENABLE_WRAP_AT_EOL_OUTPUT)) {

                    SHORT RowIndex;
                    PROW Row;
                    PWCHAR Char;
                    COORD TargetPoint;
                    PCHAR AttrP;

                    TargetPoint = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
                    RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow+TargetPoint.Y) % ScreenInfo->ScreenBufferSize.Y;
                    Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
                    Char = &Row->CharRow.Chars[TargetPoint.X];
                    AttrP = &Row->CharRow.KAttrs[TargetPoint.X];

                    if (*AttrP & ATTR_TRAILING_BYTE)
                    {
                        *(Char-1) = UNICODE_SPACE;
                        *Char = UNICODE_SPACE;
                        *AttrP = 0;
                        *(AttrP-1) = 0;

                        Region.Left = ScreenInfo->BufferInfo.TextInfo.CursorPosition.X-1;
                        Region.Right = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X);
                        Region.Top = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
                        Region.Bottom = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
                        WWSB_WriteToScreen(ScreenInfo,&Region);
                    }

                    CursorPosition.X = 0;
                    CursorPosition.Y = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y+1);
                    Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,
                                 dwFlags & WC_KEEP_CURSOR_VISIBLE,ScrollY);
                    continue;
                }
#endif
                break;
        }
        if (!NT_SUCCESS(Status)) {
            ConsoleShowCursor(ScreenInfo);
            goto ExitWriteChars;
        }

       *NumBytes += sizeof(WCHAR);
       lpString++;
       lpRealUnicodeString++;
    }

    if (ARGUMENT_PRESENT(NumSpaces)) {
        *NumSpaces = TempNumSpaces;
    }
    ConsoleShowCursor(ScreenInfo);

    Status = STATUS_SUCCESS;

ExitWriteChars:
    if (lpAllocatedString) {
        ConsoleHeapFree(lpAllocatedString);
    }
    return Status;
}

ULONG
WWSB_DoWriteConsole(
    IN OUT PCSR_API_MSG m,
    IN PCONSOLE_INFORMATION Console,
    IN PCSR_THREAD Thread
    )

//
// NOTE: console lock must be held when calling this routine
//
// string has been translated to unicode at this point
//

{
    PCONSOLE_WRITECONSOLE_MSG a = (PCONSOLE_WRITECONSOLE_MSG)&m->u.ApiMessageData;
    PHANDLE_DATA HandleData;
    NTSTATUS Status;
    PSCREEN_INFORMATION ScreenInfo;
    DWORD NumCharsToWrite;
#ifdef WWSB_FE
    DWORD i;
    SHORT j;
#endif

    if (Console->Flags & (CONSOLE_SUSPENDED | CONSOLE_SELECTING | CONSOLE_SCROLLBAR_TRACKING)) {
        PWCHAR TransBuffer;

        TransBuffer = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),a->NumBytes);
        if (TransBuffer == NULL) {
            return (ULONG)STATUS_NO_MEMORY;
        }
        RtlCopyMemory(TransBuffer,a->TransBuffer,a->NumBytes);
        a->TransBuffer = TransBuffer;
        a->StackBuffer = FALSE;
        if (!CsrCreateWait(&Console->OutputQueue,
                          WriteConsoleWaitRoutine,
                          Thread,
                          m,
                          NULL,
                          NULL
                         )) {
            ConsoleHeapFree(TransBuffer);
            return (ULONG)STATUS_NO_MEMORY;
        }
        return (ULONG)CONSOLE_STATUS_WAIT;
    }

    Status = DereferenceIoHandle(CONSOLE_FROMTHREADPERPROCESSDATA(Thread),
                                 a->OutputHandle,
                                 CONSOLE_OUTPUT_HANDLE,
                                 GENERIC_WRITE,
                                 &HandleData
                                );
    if (!NT_SUCCESS(Status)) {
        a->NumBytes = 0;
        return((ULONG) Status);
    }

    ScreenInfo = HandleData->Buffer.ScreenBuffer;

    //
    // see if we're the typical case - a string containing no special
    // characters, optionally terminated with CRLF.  if so, skip the
    // special processing.
    //

    NumCharsToWrite=a->NumBytes/sizeof(WCHAR);
    if ((ScreenInfo->OutputMode & ENABLE_PROCESSED_OUTPUT) &&
        ((LONG)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X + NumCharsToWrite) <
          ScreenInfo->ScreenBufferSize.X) ) {
        SMALL_RECT Region;
        COORD CursorPosition;

        if (a->Unicode) {
#ifdef WWSB_FE
            a->WriteFlags = WRITE_SPECIAL_CHARS;
#else
            a->WriteFlags = FastStreamWrite(a->TransBuffer,NumCharsToWrite);
#endif
        }
        if (a->WriteFlags == WRITE_SPECIAL_CHARS) {
            goto ProcessedWrite;
        }

        ConsoleHideCursor(ScreenInfo);

        //
        // WriteFlags is designed so that the number of special characters
        // is also the flag value.
        //

        NumCharsToWrite -= a->WriteFlags;

        if (NumCharsToWrite) {
#ifdef WWSB_FE
            PWCHAR TransBuffer,TransBufPtr,String;
            PBYTE TransBufferA,TransBufPtrA;
            BOOL fLocalHeap = FALSE;
            COORD TargetPoint;

            if (NumCharsToWrite > (ULONG)(ScreenInfo->ScreenBufferSize.X * ScreenInfo->ScreenBufferSize.Y)) {

                TransBuffer = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),NumCharsToWrite * 2 * sizeof(WCHAR));
                if (TransBuffer == NULL) {
                    return (ULONG)STATUS_NO_MEMORY;
                }
                TransBufferA = (PCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),NumCharsToWrite * 2 * sizeof(CHAR));
                if (TransBufferA == NULL) {
                    ConsoleHeapFree(TransBuffer);
                    return (ULONG)STATUS_NO_MEMORY;
                }

                fLocalHeap = TRUE;
            }
            else {
                TransBuffer  = ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferCharacter;
                TransBufferA = ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransBufferAttribute;
            }

            String = a->TransBuffer;
            TransBufPtr = TransBuffer;
            TransBufPtrA = TransBufferA;
            for (i = 0 , j = 0 ; i < NumCharsToWrite ; i++,j++){
                if (IsConsoleFullWidth(ScreenInfo->Console->hDC,
                                       ScreenInfo->Console->OutputCP,*String)){
                    *TransBuffer++ = *String ;
                    *TransBufferA++ = ATTR_LEADING_BYTE;
                    *TransBuffer++ = *String++ ;
                    *TransBufferA++ = ATTR_TRAILING_BYTE;
                    j++;
                }
                else{
                    *TransBuffer++ = *String++ ;
                    *TransBufferA++ = 0;
                }
            }
            TargetPoint = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
            BisectWrite(j,TargetPoint,ScreenInfo);
            if (TargetPoint.Y == ScreenInfo->ScreenBufferSize.Y-1 &&
                TargetPoint.X+j >= ScreenInfo->ScreenBufferSize.X &&
                *(TransBufPtrA+j) & ATTR_LEADING_BYTE){
                *(TransBufPtr+ScreenInfo->ScreenBufferSize.X-TargetPoint.X-1) = UNICODE_SPACE;
                *(TransBufPtrA+ScreenInfo->ScreenBufferSize.X-TargetPoint.X-1) = 0;
                if (j > ScreenInfo->ScreenBufferSize.X-TargetPoint.X-1) {
                    *(TransBufPtr+ScreenInfo->ScreenBufferSize.X-TargetPoint.X) = UNICODE_SPACE;
                    *(TransBufPtrA+ScreenInfo->ScreenBufferSize.X-TargetPoint.X) = 0;
                }
            }
            FE_StreamWriteToScreenBuffer(TransBufPtr,
                                         (SHORT)j,
                                         ScreenInfo,
                                         TransBufPtrA
                                        );
            if (fLocalHeap){
                ConsoleHeapFree(TransBufPtr);
                ConsoleHeapFree(TransBufPtrA);
            }
#else
            SB_StreamWriteToScreenBuffer(a->TransBuffer,
                                         (SHORT)NumCharsToWrite,
                                         ScreenInfo
                                        );
#endif
            Region.Left = ScreenInfo->BufferInfo.TextInfo.CursorPosition.X;
#ifdef WWSB_FE
            Region.Right = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X + j - 1);
#else
            Region.Right = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X + NumCharsToWrite - 1);
#endif
            Region.Top = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
            Region.Bottom = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
            ASSERT (Region.Right < ScreenInfo->ScreenBufferSize.X);
            if (ACTIVE_SCREEN_BUFFER(ScreenInfo) &&
                !(ScreenInfo->Console->Flags & CONSOLE_IS_ICONIC && ScreenInfo->Console->FullScreenFlags == 0)) {
                WWSB_WriteRegionToScreen(ScreenInfo,&Region);
            }
        }
        switch (a->WriteFlags) {
            case WRITE_NO_CR_LF:
                CursorPosition.X = (SHORT)(ScreenInfo->BufferInfo.TextInfo.CursorPosition.X + NumCharsToWrite);
                CursorPosition.Y = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
                break;
            case WRITE_CR:
                CursorPosition.X = 0;
                CursorPosition.Y = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
                break;
            case WRITE_CR_LF:
                CursorPosition.X = 0;
                CursorPosition.Y = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y+1;
                break;
            default:
                ASSERT(FALSE);
                break;
        }
        Status = WWSB_AdjustCursorPosition(ScreenInfo,CursorPosition,FALSE,NULL);
        ConsoleShowCursor(ScreenInfo);
        return STATUS_SUCCESS;
    }
ProcessedWrite:
    return WWSB_WriteChars(ScreenInfo,
                      a->TransBuffer,
                      a->TransBuffer,
                      a->TransBuffer,
                      &a->NumBytes,
                      NULL,
                      ScreenInfo->BufferInfo.TextInfo.CursorPosition.X,
                      WC_LIMIT_BACKSPACE,
                      NULL
                     );
}

NTSTATUS
WWSB_DoSrvWriteConsole(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus,
    IN PCONSOLE_INFORMATION Console,
    IN PHANDLE_DATA HandleData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_WRITECONSOLE_MSG a = (PCONSOLE_WRITECONSOLE_MSG)&m->u.ApiMessageData;
    PSCREEN_INFORMATION ScreenInfo;
    WCHAR StackBuffer[STACK_BUFFER_SIZE];
#ifdef WWSB_FE
    BOOL  fLocalHeap = FALSE;
#endif

    ScreenInfo = HandleData->Buffer.ScreenBuffer;

#ifdef WWSB_FE
    // Check code for must CONSOLE_TEXTMODE_BUFFER !!
    ASSERT(!(ScreenInfo->Flags & CONSOLE_GRAPHICS_BUFFER));
#endif

    //
    // if the string was passed in the message, rather than in
    // a capture buffer, adjust the pointer.
    //

    if (a->BufferInMessage) {
        a->BufPtr = a->Buffer;
    }

    //
    // if ansi, translate string.  for speed, we don't allocate a
    // capture buffer if the ansi string was <= 80 chars.  if it's
    // greater than 80 / sizeof(WCHAR), the translated string won't
    // fit into the capture buffer, so reset a->BufPtr to point to
    // a heap buffer and set a->CaptureBufferSize so that we don't
    // think the buffer is in the message.
    //

    if (!a->Unicode) {
        PWCHAR TransBuffer;
        DWORD Length;
        DWORD SpecialChars = 0;
        UINT Codepage;
#ifdef WWSB_FE
        PWCHAR TmpTransBuffer;
        ULONG NumBytes1 = 0;
        ULONG NumBytes2 = 0;
#endif

        if (a->NumBytes <= STACK_BUFFER_SIZE) {
            TransBuffer = StackBuffer;
            a->StackBuffer = TRUE;
#ifdef WWSB_FE
            TmpTransBuffer = TransBuffer;
#endif
        }
#ifdef WWSB_FE
        else if (a->NumBytes > (ULONG)(ScreenInfo->ScreenBufferSize.X * ScreenInfo->ScreenBufferSize.Y)) {
            TransBuffer = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),(a->NumBytes+2) * sizeof(WCHAR));
            if (TransBuffer == NULL) {
                return (ULONG)STATUS_NO_MEMORY;
            }
            TmpTransBuffer = TransBuffer;
            a->StackBuffer = FALSE;
            fLocalHeap = TRUE;
        }
        else {
            TransBuffer = ScreenInfo->BufferInfo.TextInfo.DbcsScreenBuffer.TransWriteConsole;
            TmpTransBuffer = TransBuffer;
        }
#else
        else {
            TransBuffer = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),a->NumBytes * sizeof(WCHAR));
            if (TransBuffer == NULL) {
                return (ULONG)STATUS_NO_MEMORY;
            }
            a->StackBuffer = FALSE;
        }
#endif
        //a->NumBytes = ConvertOutputToUnicode(Console->OutputCP,
        //                        Buffer,
        //                        a->NumBytes,
        //                        TransBuffer,
        //                        a->NumBytes);
        // same as ConvertOutputToUnicode
#ifdef WWSB_FE
        if (! ScreenInfo->WriteConsoleDbcsLeadByte[0]) {
            NumBytes1 = 0;
            NumBytes2 = a->NumBytes;
        }
        else {
            if (*(PUCHAR)a->BufPtr < (UCHAR)' ') {
                NumBytes1 = 0;
                NumBytes2 = a->NumBytes;
            }
            else if (a->NumBytes) {
                ScreenInfo->WriteConsoleDbcsLeadByte[1] = *(PCHAR)a->BufPtr;
                NumBytes1 = sizeof(ScreenInfo->WriteConsoleDbcsLeadByte);
                if (Console->OutputCP == OEMCP) {
                    if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                            ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
                        /*
                         * Translate OEM characters into False Unicode for Window-mode
                         * OEM font. If OutputCP != OEMCP, characters will not appear
                         * correctly, because the OEM fonts are designed to support
                         * only OEMCP (we can't switch fonts in Windowed mode).
                         * Fullscreen or TT "Unicode" fonts should be used for
                         * non-OEMCP output
                         */
                        DBGCHARS(("SrvWriteConsole ACP->U %.*s\n",
                                min(NumBytes1,10), a->BufPtr));
                        Status = RtlConsoleMultiByteToUnicodeN(TransBuffer,
                                NumBytes1 * sizeof(WCHAR), &NumBytes1,
                                ScreenInfo->WriteConsoleDbcsLeadByte, NumBytes1, &SpecialChars);
                    } else {
                        /*
                         * Good! We have Fullscreen or TT "Unicode" fonts, so convert
                         * the OEM characters to real Unicode according to OutputCP.
                         * First find out if any special chars are involved.
                         */
                        DBGCHARS(("SrvWriteConsole %d->U %.*s\n", Console->OutputCP,
                                min(NumBytes1,10), a->BufPtr));
                        NumBytes1 = sizeof(WCHAR) * MultiByteToWideChar(Console->OutputCP,
                                0, ScreenInfo->WriteConsoleDbcsLeadByte, NumBytes1, TransBuffer, NumBytes1);
                        if (NumBytes1 == 0) {
                            Status = STATUS_UNSUCCESSFUL;
                        }
                    }
                }
                else {
                    if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                        !(Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
                        if (Console->OutputCP != WINDOWSCP)
                            Codepage = USACP;
                        else
                            Codepage = WINDOWSCP;
                    } else {
                        Codepage = Console->OutputCP;
                    }

                    if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                            ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
                        NumBytes1 = ConvertOutputToUnicode(Codepage,
                                                           ScreenInfo->WriteConsoleDbcsLeadByte,
                                                           NumBytes1,
                                                           TransBuffer,
                                                           NumBytes1);
                    }
                    else {
                        NumBytes1 = MultiByteToWideChar(Console->OutputCP,
                                0, ScreenInfo->WriteConsoleDbcsLeadByte, NumBytes1, TransBuffer, NumBytes1);
                        if (NumBytes1 == 0) {
                            Status = STATUS_UNSUCCESSFUL;
                        }
                    }
                    NumBytes1 *= sizeof(WCHAR);
                }
                TransBuffer++;
                (PCHAR)a->BufPtr += (NumBytes1 / sizeof(WCHAR));
                NumBytes2 = a->NumBytes - 1;
            }
            else {
                NumBytes2 = 0;
            }
            ScreenInfo->WriteConsoleDbcsLeadByte[0] = 0;
        }

        if (NumBytes2 &&
            CheckBisectStringA(Console->OutputCP,a->BufPtr,NumBytes2,&Console->OutputCPInfo)) {
            ScreenInfo->WriteConsoleDbcsLeadByte[0] = *((PCHAR)a->BufPtr+NumBytes2-1);
            NumBytes2--;
        }

        Length = NumBytes2;
#else
        Length = a->NumBytes;
        if (a->NumBytes >= 2 &&
            ((PCHAR)a->BufPtr)[a->NumBytes-1] == '\n' &&
            ((PCHAR)a->BufPtr)[a->NumBytes-2] == '\r') {
            Length -= 2;
            a->WriteFlags = WRITE_CR_LF;
        } else if (a->NumBytes >= 1 &&
                   ((PCHAR)a->BufPtr)[a->NumBytes-1] == '\r') {
            Length -= 1;
            a->WriteFlags = WRITE_CR;
        } else {
            a->WriteFlags = WRITE_NO_CR_LF;
        }
#endif

        if (Length != 0) {
            if (Console->OutputCP == OEMCP) {
                if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                        ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
                    /*
                     * Translate OEM characters into UnicodeOem for the Window-mode
                     * OEM font. If OutputCP != OEMCP, characters will not appear
                     * correctly, because the OEM fonts are designed to support
                     * only OEMCP (we can't switch fonts in Windowed mode).
                     * Fullscreen or TT "Unicode" fonts should be used for
                     * non-OEMCP output
                     */
                    DBGCHARS(("SrvWriteConsole ACP->U %.*s\n",
                            min(Length,10), a->BufPtr));
                    Status = RtlConsoleMultiByteToUnicodeN(TransBuffer,
                            Length * sizeof(WCHAR), &Length,
                            a->BufPtr, Length, &SpecialChars);
                } else {
                    /*
                     * Good! We have Fullscreen or TT "Unicode" fonts, so convert
                     * the OEM characters to real Unicode according to OutputCP.
                     * First find out if any special chars are involved.
                     */
#ifdef WWSB_NOFE
                    UINT i;
                    for (i = 0; i < Length; i++) {
                        if (((PCHAR)a->BufPtr)[i] < 0x20) {
                            SpecialChars = 1;
                            break;
                        }
                    }
#endif
                    DBGCHARS(("SrvWriteConsole %d->U %.*s\n", Console->OutputCP,
                            min(Length,10), a->BufPtr));
                    Length = sizeof(WCHAR) * MultiByteToWideChar(Console->OutputCP,
                            0, a->BufPtr, Length, TransBuffer, Length);
                    if (Length == 0) {
                        Status = STATUS_UNSUCCESSFUL;
                    }
                }
            }
            else
            {
                if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                    !(Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
                    if (Console->OutputCP != WINDOWSCP)
                        Codepage = USACP;
                    else
                        Codepage = WINDOWSCP;
                } else {
                    Codepage = Console->OutputCP;
                }

                if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                        ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
                    Length = sizeof(WCHAR) * ConvertOutputToUnicode(Codepage,
                                                                    a->BufPtr,
                                                                    Length,
                                                                    TransBuffer,
                                                                    Length);
                }
                else {
                    Length = sizeof(WCHAR) * MultiByteToWideChar(Console->OutputCP,
                            0, a->BufPtr, Length, TransBuffer, Length);
                    if (Length == 0) {
                        Status = STATUS_UNSUCCESSFUL;
                    }
                }

#ifdef WWSB_NOFE
                SpecialChars = 1;
#endif
            }
        }

#ifdef WWSB_FE
        NumBytes2 = Length;

        if ((NumBytes1+NumBytes2) == 0) {
            if (!a->StackBuffer && fLocalHeap) {
                ConsoleHeapFree(a->TransBuffer);
            }
            return Status;
        }
#else
        if (!NT_SUCCESS(Status)) {
            if (!a->StackBuffer) {
                ConsoleHeapFree(TransBuffer);
            }
            return Status;
        }
#endif

#ifdef WWSB_FE
        Console->WriteConOutNumBytesTemp = a->NumBytes;
        a->NumBytes = Console->WriteConOutNumBytesUnicode = NumBytes1 + NumBytes2;
        a->WriteFlags = WRITE_SPECIAL_CHARS;
        a->TransBuffer = TmpTransBuffer;
#else
        DBGOUTPUT(("TransBuffer=%lx, Length = %x(bytes), SpecialChars=%lx\n",
                TransBuffer, Length, SpecialChars));
        a->NumBytes = Length + (a->WriteFlags * sizeof(WCHAR));
        if (a->WriteFlags == WRITE_CR_LF) {
            TransBuffer[(Length+sizeof(WCHAR))/sizeof(WCHAR)] = UNICODE_LINEFEED;
            TransBuffer[Length/sizeof(WCHAR)] = UNICODE_CARRIAGERETURN;
        } else if (a->WriteFlags == WRITE_CR) {
            TransBuffer[Length/sizeof(WCHAR)] = UNICODE_CARRIAGERETURN;
        }
        if (SpecialChars) {
            // CRLF didn't get translated
            a->WriteFlags = WRITE_SPECIAL_CHARS;
        }
        a->TransBuffer = TransBuffer;
#endif
    } else {
        if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                    ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) == 0)) {
            Status = RealUnicodeToFalseUnicode(a->BufPtr,
                    a->NumBytes / sizeof(WCHAR), Console->OutputCP);
            if (!NT_SUCCESS(Status)) {
                return Status;
            }
        }
        a->WriteFlags = (DWORD)-1;
        a->TransBuffer = a->BufPtr;
    }
    Status = WWSB_DoWriteConsole(m,Console,CSR_SERVER_QUERYCLIENTTHREAD());
    if (Status == CONSOLE_STATUS_WAIT) {
        *ReplyStatus = CsrReplyPending;
        return (ULONG)STATUS_SUCCESS;
    } else {
        if (!a->Unicode) {
#ifdef WWSB_FE
            if (a->NumBytes == Console->WriteConOutNumBytesUnicode)
                a->NumBytes = Console->WriteConOutNumBytesTemp;
            else
                a->NumBytes /= sizeof(WCHAR);
            if (!a->StackBuffer && fLocalHeap) {
                ConsoleHeapFree(a->TransBuffer);
            }
#else
            a->NumBytes /= sizeof(WCHAR);
            if (!a->StackBuffer) {
                ConsoleHeapFree(a->TransBuffer);
            }
#endif
        }
    }
    return Status;
}
