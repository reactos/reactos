/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    clipbrd.c

Abstract:

        This file implements the clipboard functions.

Author:

    Therese Stowell (thereses) Jan-24-1992

--*/

#include "precomp.h"
#pragma hdrstop

/*++

    Here's the pseudocode for various clipboard operations

    init keyboard select (mark)
    ---------------------------
    if (already selecting)
        cancel selection
    init flags
    hidecursor
    createcursor
    init select rect
    set win text

    convert to mouse select (select)
    --------------------------------
    set flags
    destroy cursor
    showcursor
    invert old select rect
    init select rect
    invert select rect
    set win text

    re-init mouse select
    --------------------
    invert old select rect
    init select rect
    invert select rect

    cancel mouse select
    -------------------
    set flags
    reset win text
    invert old select rect

    cancel key select
    -----------------
    set flags
    reset win text
    destroy cursor
    showcursor
    invert old select rect

--*/


BOOL
MyInvert(
    IN PCONSOLE_INFORMATION Console,
    IN PSMALL_RECT SmallRect
    )

/*++

    invert a rect

--*/

{
    RECT Rect;
    PSCREEN_INFORMATION ScreenInfo;
#ifdef FE_SB
    SMALL_RECT SmallRect2;
    COORD TargetPoint;
    SHORT StringLength;
#endif  // FE_SB

    ScreenInfo = Console->CurrentScreenBuffer;
#ifdef FE_SB
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
            ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        for (SmallRect2.Top=SmallRect->Top; SmallRect2.Top <= SmallRect->Bottom;SmallRect2.Top++) {
            SmallRect2.Bottom = SmallRect2.Top;
            SmallRect2.Left = SmallRect->Left;
            SmallRect2.Right = SmallRect->Right;

            TargetPoint.X = SmallRect2.Left;
            TargetPoint.Y = SmallRect2.Top;
            StringLength = SmallRect2.Right - SmallRect2.Left + 1;
            BisectClipbrd(StringLength,TargetPoint,ScreenInfo,&SmallRect2);

            if (SmallRect2.Left <= SmallRect2.Right) {
                Rect.left = SmallRect2.Left-ScreenInfo->Window.Left;
                Rect.top = SmallRect2.Top-ScreenInfo->Window.Top;
                Rect.right = SmallRect2.Right+1-ScreenInfo->Window.Left;
                Rect.bottom = SmallRect2.Bottom+1-ScreenInfo->Window.Top;
                Rect.left *= SCR_FONTSIZE(ScreenInfo).X;
                Rect.top *= SCR_FONTSIZE(ScreenInfo).Y;
                Rect.right *= SCR_FONTSIZE(ScreenInfo).X;
                Rect.bottom *= SCR_FONTSIZE(ScreenInfo).Y;
                PatBlt(Console->hDC,
                       Rect.left,
                       Rect.top,
                       Rect.right  - Rect.left,
                       Rect.bottom - Rect.top,
                       DSTINVERT
                      );
            }
        }
    } else
#endif  // FE_SB
    {
        Rect.left = SmallRect->Left-ScreenInfo->Window.Left;
        Rect.top = SmallRect->Top-ScreenInfo->Window.Top;
        Rect.right = SmallRect->Right+1-ScreenInfo->Window.Left;
        Rect.bottom = SmallRect->Bottom+1-ScreenInfo->Window.Top;
#ifdef FE_SB
        if (!CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
                ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)
#else
        if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)
#endif
        {
            Rect.left *= SCR_FONTSIZE(ScreenInfo).X;
            Rect.top *= SCR_FONTSIZE(ScreenInfo).Y;
            Rect.right *= SCR_FONTSIZE(ScreenInfo).X;
            Rect.bottom *= SCR_FONTSIZE(ScreenInfo).Y;
        }

        PatBlt(Console->hDC,
                  Rect.left,
                  Rect.top,
                  Rect.right  - Rect.left,
                  Rect.bottom - Rect.top,
                  DSTINVERT
                 );
    }

    return(TRUE);
}

VOID
InvertSelection(
    IN PCONSOLE_INFORMATION Console,
    BOOL Inverting
    )
{
    BOOL Inverted;
    if (Console->Flags & CONSOLE_SELECTING &&
        Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY) {
        Inverted = (Console->SelectionFlags & CONSOLE_SELECTION_INVERTED) ? TRUE : FALSE;
        if (Inverting == Inverted) {
            return;
        }
        if (Inverting) {
            Console->SelectionFlags |= CONSOLE_SELECTION_INVERTED;
        } else {
            Console->SelectionFlags &= ~CONSOLE_SELECTION_INVERTED;
        }
        MyInvert(Console,&Console->SelectionRect);
    }

}

VOID
ExtendSelection(
    IN PCONSOLE_INFORMATION Console,
    IN COORD CursorPosition
    )

/*++

    This routine extends a selection region.

--*/

{
    SMALL_RECT OldSelectionRect;
    HRGN OldRegion,NewRegion,CombineRegion;
    COORD FontSize;
    PSCREEN_INFORMATION ScreenInfo = Console->CurrentScreenBuffer;

    if (CursorPosition.X < 0) {
        CursorPosition.X = 0;
    } else if (CursorPosition.X >= ScreenInfo->ScreenBufferSize.X) {
        CursorPosition.X = ScreenInfo->ScreenBufferSize.X-1;
    }

    if (CursorPosition.Y < 0) {
        CursorPosition.Y = 0;
    } else if (CursorPosition.Y >= ScreenInfo->ScreenBufferSize.Y) {
        CursorPosition.Y = ScreenInfo->ScreenBufferSize.Y-1;
    }

    if (!(Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY)) {

        if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
            // scroll if necessary to make cursor visible.
            MakeCursorVisible(ScreenInfo, CursorPosition);
            ASSERT(!(Console->SelectionFlags & CONSOLE_MOUSE_SELECTION));

            //
            // if the selection rect hasn't actually been started,
            // the selection cursor is still blinking.  turn it off.
            //

            ConsoleHideCursor(ScreenInfo);
        }
        Console->SelectionFlags |= CONSOLE_SELECTION_NOT_EMPTY;
        Console->SelectionRect.Left =Console->SelectionRect.Right = Console->SelectionAnchor.X;
        Console->SelectionRect.Top = Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;

        // invert the cursor corner

#ifdef FE_SB
        if (!CONSOLE_IS_DBCS_OUTPUTCP(Console) &&
                ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)
#else
        if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER)
#endif
        {
            MyInvert(Console,&Console->SelectionRect);
        }
    } else {

        if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
            // scroll if necessary to make cursor visible.
            MakeCursorVisible(ScreenInfo,CursorPosition);
        }
#ifdef FE_SB
        //
        // uninvert old selection
        //
        if (CONSOLE_IS_DBCS_OUTPUTCP(Console)) {
            MyInvert(Console, &Console->SelectionRect);
        }
#endif  // FE_SB
    }

    //
    // update selection rect
    //

    OldSelectionRect = Console->SelectionRect;
    if (CursorPosition.X <= Console->SelectionAnchor.X) {
        Console->SelectionRect.Left = CursorPosition.X;
        Console->SelectionRect.Right = Console->SelectionAnchor.X;
    } else if (CursorPosition.X > Console->SelectionAnchor.X) {
        Console->SelectionRect.Right = CursorPosition.X;
        Console->SelectionRect.Left = Console->SelectionAnchor.X;
    }
    if (CursorPosition.Y <= Console->SelectionAnchor.Y) {
        Console->SelectionRect.Top = CursorPosition.Y;
        Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;
    } else if (CursorPosition.Y > Console->SelectionAnchor.Y) {
        Console->SelectionRect.Bottom = CursorPosition.Y;
        Console->SelectionRect.Top = Console->SelectionAnchor.Y;
    }

    //
    // change inverted selection
    //
#ifdef FE_SB
    if (CONSOLE_IS_DBCS_OUTPUTCP(Console)) {
        MyInvert(Console, &Console->SelectionRect);
    } else
#endif
    {
        if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
            FontSize = CON_FONTSIZE(Console);
        } else {
            FontSize.X = 1;
            FontSize.Y = 1;
        }
        CombineRegion = CreateRectRgn(0,0,0,0);
        OldRegion = CreateRectRgn((OldSelectionRect.Left-ScreenInfo->Window.Left)*FontSize.X,
                                  (OldSelectionRect.Top-ScreenInfo->Window.Top)*FontSize.Y,
                                  (OldSelectionRect.Right-ScreenInfo->Window.Left+1)*FontSize.X,
                                  (OldSelectionRect.Bottom-ScreenInfo->Window.Top+1)*FontSize.Y
                                 );
        NewRegion = CreateRectRgn((Console->SelectionRect.Left-ScreenInfo->Window.Left)*FontSize.X,
                                  (Console->SelectionRect.Top-ScreenInfo->Window.Top)*FontSize.Y,
                                  (Console->SelectionRect.Right-ScreenInfo->Window.Left+1)*FontSize.X,
                                  (Console->SelectionRect.Bottom-ScreenInfo->Window.Top+1)*FontSize.Y
                                 );
        CombineRgn(CombineRegion,OldRegion,NewRegion,RGN_XOR);

        InvertRgn(Console->hDC,CombineRegion);
        DeleteObject(OldRegion);
        DeleteObject(NewRegion);
        DeleteObject(CombineRegion);
    }
}

VOID
CancelMouseSelection(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    This routine terminates a mouse selection.

--*/

{
    //
    // turn off selection flag
    //

    Console->Flags &= ~CONSOLE_SELECTING;

    SetWinText(Console,msgSelectMode,FALSE);

    //
    // invert old select rect.  if we're selecting by mouse, we
    // always have a selection rect.
    //

    MyInvert(Console,&Console->SelectionRect);

    ReleaseCapture();
}

VOID
CancelKeySelection(
    IN PCONSOLE_INFORMATION Console,
    IN BOOL JustCursor
    )

/*++

    This routine terminates a key selection.

--*/

{
    PSCREEN_INFORMATION ScreenInfo;

    if (!JustCursor) {

        //
        // turn off selection flag
        //

        Console->Flags &= ~CONSOLE_SELECTING;

        SetWinText(Console,msgMarkMode,FALSE);
    }

    //
    // invert old select rect, if we have one.
    //

    ScreenInfo = Console->CurrentScreenBuffer;
    if (Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY) {
        MyInvert(Console,&Console->SelectionRect);
    } else {
        ConsoleHideCursor(ScreenInfo);
    }

    // restore text cursor

    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
        SetCursorInformation(ScreenInfo,
                             Console->TextCursorSize,
                             Console->TextCursorVisible
                            );
        SetCursorPosition(ScreenInfo,
                          Console->TextCursorPosition,
                          TRUE
                         );
    }
    ConsoleShowCursor(ScreenInfo);
}

VOID
ConvertToMouseSelect(
    IN PCONSOLE_INFORMATION Console,
    IN COORD MousePosition
    )

/*++

    This routine converts to a mouse selection from a key selection.

--*/

{
    Console->SelectionFlags |= CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN;

    //
    // undo key selection
    //

    CancelKeySelection(Console,TRUE);

    Console->SelectionFlags |= CONSOLE_SELECTION_NOT_EMPTY;

    //
    // invert new selection
    //

    Console->SelectionAnchor = MousePosition;
    Console->SelectionRect.Left =Console->SelectionRect.Right = Console->SelectionAnchor.X;
    Console->SelectionRect.Top = Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;
    MyInvert(Console,&Console->SelectionRect);

    //
    // update title bar
    //

    SetWinText(Console,msgMarkMode,FALSE);
    SetWinText(Console,msgSelectMode,TRUE);

    //
    // capture mouse movement
    //

    SetCapture(Console->hWnd);
}


VOID
ClearSelection(
    IN PCONSOLE_INFORMATION Console
    )
{
    if (Console->Flags & CONSOLE_SELECTING) {
        if (Console->SelectionFlags & CONSOLE_MOUSE_SELECTION) {
            CancelMouseSelection(Console);
        } else {
            CancelKeySelection(Console,FALSE);
        }
        UnblockWriteConsole(Console, CONSOLE_SELECTING);
    }
}

VOID
StoreSelection(
    IN PCONSOLE_INFORMATION Console
    )

/*++

 StoreSelection - Store selection (if present) into the Clipboard

--*/

{
    PCHAR_INFO Selection,CurCharInfo;
    COORD SourcePoint;
    COORD TargetSize;
    SMALL_RECT TargetRect;
    PWCHAR CurChar,CharBuf;
    HANDLE ClipboardDataHandle;
    SHORT i,j;
    BOOL Success;
    PSCREEN_INFORMATION ScreenInfo;
    BOOL bFalseUnicode;
    BOOL bMungeData;
#if defined(FE_SB)
    COORD TargetSize2;
    PWCHAR TmpClipboardData;
    SMALL_RECT SmallRect2;
    COORD TargetPoint;
    SHORT StringLength;
    WCHAR wchCARRIAGERETURN;
    WCHAR wchLINEFEED;
    int iExtra = 0;
    int iFeReserve = 1;
#endif

    //
    // See if there is a selection to get
    //

    if (!(Console->SelectionFlags & CONSOLE_SELECTION_NOT_EMPTY)) {
        return;
    }

    //
    // read selection rectangle.  clip it first.
    //

    ScreenInfo = Console->CurrentScreenBuffer;
    if (Console->SelectionRect.Left < 0) {
        Console->SelectionRect.Left = 0;
    }
    if (Console->SelectionRect.Top < 0) {
        Console->SelectionRect.Top = 0;
    }
    if (Console->SelectionRect.Right >= ScreenInfo->ScreenBufferSize.X) {
        Console->SelectionRect.Right = (SHORT)(ScreenInfo->ScreenBufferSize.X-1);
    }
    if (Console->SelectionRect.Bottom >= ScreenInfo->ScreenBufferSize.Y) {
        Console->SelectionRect.Bottom = (SHORT)(ScreenInfo->ScreenBufferSize.Y-1);
    }

    TargetSize.X = WINDOW_SIZE_X(&Console->SelectionRect);
    TargetSize.Y = WINDOW_SIZE_Y(&Console->SelectionRect);
    if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
#if defined(FE_SB)

        if (CONSOLE_IS_DBCS_CP(Console)) {
            iExtra = 4 ;     // 4 is for DBCS lead or tail extra
            iFeReserve = 2 ; // FE does this for safety
            TmpClipboardData = (PWCHAR)ConsoleHeapAlloc(MAKE_TAG( TMP_DBCS_TAG ),(sizeof(WCHAR) * TargetSize.Y * (TargetSize.X + iExtra) + sizeof(WCHAR)));
            if (TmpClipboardData == NULL) {
                return;
            }
        } else {
            TmpClipboardData = NULL;
        }

        Selection = (PCHAR_INFO)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),sizeof(CHAR_INFO) * (TargetSize.X + iExtra) * TargetSize.Y * iFeReserve);
        if (Selection == NULL)
        {
            if (TmpClipboardData)
                ConsoleHeapFree(TmpClipboardData);
            return;
        }
#else
        Selection = (PCHAR_INFO)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),sizeof(CHAR_INFO) * TargetSize.X * TargetSize.Y);
        if (Selection == NULL)
            return;
#endif

#if defined(FE_SB)
    if (!CONSOLE_IS_DBCS_CP(Console)) {
#endif

#ifdef i386
        if ((Console->FullScreenFlags & CONSOLE_FULLSCREEN) &&
            (Console->Flags & CONSOLE_VDM_REGISTERED)) {
            ReadRegionFromScreenHW(ScreenInfo,
                                   &Console->SelectionRect,
                                   Selection);
            CurCharInfo = Selection;
            for (i=0; i<TargetSize.Y; i++) {
                for (j=0; j<TargetSize.X; j++,CurCharInfo++) {
                    CurCharInfo->Char.UnicodeChar = SB_CharToWcharGlyph(Console->OutputCP, CurCharInfo->Char.AsciiChar);
                }
            }
        } else {
#endif
            SourcePoint.X = Console->SelectionRect.Left;
            SourcePoint.Y = Console->SelectionRect.Top;
            TargetRect.Left = TargetRect.Top = 0;
            TargetRect.Right = (SHORT)(TargetSize.X-1);
            TargetRect.Bottom = (SHORT)(TargetSize.Y-1);
            ReadRectFromScreenBuffer(ScreenInfo,
                                     SourcePoint,
                                     Selection,
                                     TargetSize,
                                     &TargetRect);
#ifdef i386
        }
#endif

        // extra 2 per line is for CRLF, extra 1 is for null
        ClipboardDataHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                (TargetSize.Y * (TargetSize.X + 2) + 1) * sizeof(WCHAR));
        if (ClipboardDataHandle == NULL) {
            ConsoleHeapFree(Selection);
            return;
        }
#if defined(FE_SB)
    }
#endif

        //
        // convert to clipboard form
        //

#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_CP(Console)) {
            if ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                    !(Console->FullScreenFlags & CONSOLE_FULLSCREEN)) {
                /*
                 * False Unicode is obtained, so we will have to convert it to
                 * Real Unicode, in which case we can't put CR or LF in now, since
                 * they will be converted into 0x266A and 0x25d9.  Temporarily
                 * mark the CR/LF positions with 0x0000 instead.
                 */
                wchCARRIAGERETURN = 0x0000;
                wchLINEFEED = 0x0000;
            } else {
                wchCARRIAGERETURN = UNICODE_CARRIAGERETURN;
                wchLINEFEED = UNICODE_LINEFEED;
            }

            CurChar = TmpClipboardData;
            bMungeData = (GetKeyState(VK_SHIFT) & KEY_PRESSED) == 0;
            for (i=0;i<TargetSize.Y;i++) {
                PWCHAR pwchLineStart = CurChar;

                SourcePoint.X = Console->SelectionRect.Left;
                SourcePoint.Y = Console->SelectionRect.Top + i;
                TargetSize2.X = TargetSize.X;
                TargetSize2.Y = 1;

                SmallRect2.Left = SourcePoint.X;
                SmallRect2.Top = SourcePoint.Y;
                SmallRect2.Right = SourcePoint.X + TargetSize2.X - 1;
                SmallRect2.Bottom = SourcePoint.Y;
                TargetPoint = SourcePoint;
                StringLength = TargetSize2.X;
                BisectClipbrd(StringLength,TargetPoint,ScreenInfo,&SmallRect2);

                SourcePoint.X = SmallRect2.Left;
                SourcePoint.Y = SmallRect2.Top;
                TargetSize2.X = SmallRect2.Right - SmallRect2.Left + 1;
                TargetSize2.Y = 1;
                TargetRect.Left = TargetRect.Top = TargetRect.Bottom = 0;
                TargetRect.Right = (SHORT)(TargetSize2.X-1);

                ReadRectFromScreenBuffer(ScreenInfo,
                                         SourcePoint,
                                         Selection,
                                         TargetSize2,
                                         &TargetRect);

                CurCharInfo = Selection;
                for (j=0;j<TargetSize2.X;j++,CurCharInfo++) {
                    if (!(CurCharInfo->Attributes & COMMON_LVB_TRAILING_BYTE))
                        *CurChar++ = CurCharInfo->Char.UnicodeChar;
                }
                // trim trailing spaces
                if (bMungeData) {
                    CurChar--;
                    while ((CurChar >= pwchLineStart) && (*CurChar == UNICODE_SPACE))
                        CurChar--;
                    CurChar++;
                    *CurChar++ = wchCARRIAGERETURN;
                    *CurChar++ = wchLINEFEED;
                }
            }
        }
        else {
#endif
        CurCharInfo = Selection;
        CurChar = CharBuf = GlobalLock(ClipboardDataHandle);
        bFalseUnicode = ((ScreenInfo->Flags & CONSOLE_OEMFONT_DISPLAY) &&
                !(Console->FullScreenFlags & CONSOLE_FULLSCREEN));
        bMungeData = (GetKeyState(VK_SHIFT) & KEY_PRESSED) == 0;
        for (i=0;i<TargetSize.Y;i++) {
            PWCHAR pwchLineStart = CurChar;

            for (j=0;j<TargetSize.X;j++,CurCharInfo++,CurChar++) {
                *CurChar = CurCharInfo->Char.UnicodeChar;
                if (*CurChar == 0) {
                    *CurChar = UNICODE_SPACE;
                }
            }
            // trim trailing spaces
            if (bMungeData) {
                CurChar--;
                while ((CurChar >= pwchLineStart) && (*CurChar == UNICODE_SPACE))
                    CurChar--;
                CurChar++;
            }

            if (bFalseUnicode) {
                FalseUnicodeToRealUnicode(pwchLineStart,
                        (ULONG)(CurChar - pwchLineStart), Console->OutputCP);
            }
            if (bMungeData) {
                *CurChar++ = UNICODE_CARRIAGERETURN;
                *CurChar++ = UNICODE_LINEFEED;
            }
        }
#if defined(FE_SB)
        }
#endif
        if (bMungeData) {
            if (TargetSize.Y)
                CurChar -= 2;   // don't put CRLF on last line
        }
        *CurChar = '\0';    // null terminate

#if defined(FE_SB)
        if (CONSOLE_IS_DBCS_CP(Console)) {
            // extra 4 is for CRLF and DBCS Reserved, extra 1 is for null
            ClipboardDataHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                    (sizeof(WCHAR) * TargetSize.Y * (TargetSize.X+(4*sizeof(WCHAR)))) +
                                                        (1*sizeof(WCHAR)));
            if (ClipboardDataHandle == NULL) {
                ConsoleHeapFree(Selection);
                ConsoleHeapFree(TmpClipboardData);
                return;
            }

            CharBuf = GlobalLock(ClipboardDataHandle);
            RtlCopyMemory(CharBuf,TmpClipboardData,ConsoleHeapSize(TmpClipboardData));
            CurChar = CharBuf + (CurChar - TmpClipboardData);

            if (wchCARRIAGERETURN == 0x0000) {
                /*
                 * We have False Unicode, so we temporarily represented CRLFs with
                 * 0x0000s to avoid undesirable conversions (above).
                 * Convert to Real Unicode and restore real CRLFs.
                 */
                PWCHAR pwch;
                FalseUnicodeToRealUnicode(CharBuf,
                                    (ULONG)(CurChar - CharBuf),
                                    Console->OutputCP
                                   );
                for (pwch = CharBuf; pwch < CurChar; pwch++) {
                    if ((*pwch == 0x0000) && (pwch[1] == 0x0000)) {
                        *pwch++ = UNICODE_CARRIAGERETURN;
                        *pwch = UNICODE_LINEFEED;
                    }
                }
            }
        }
#endif

        GlobalUnlock(ClipboardDataHandle);
#if defined(FE_SB)
        if (TmpClipboardData)
            ConsoleHeapFree(TmpClipboardData);
#endif
        ConsoleHeapFree(Selection);
        Success = OpenClipboard(Console->hWnd);
        if (!Success) {
            GlobalFree(ClipboardDataHandle);
            return;
        }

        Success = EmptyClipboard();
        if (!Success) {
            GlobalFree(ClipboardDataHandle);
            return;
        }

        SetClipboardData(CF_UNICODETEXT,ClipboardDataHandle);
        CloseClipboard();   // Close clipboard
    } else {
        HBITMAP hBitmapTarget, hBitmapOld;
        HDC hDCMem;
        HPALETTE hPaletteOld;
        int Height;

        NtWaitForSingleObject(ScreenInfo->BufferInfo.GraphicsInfo.hMutex,
                              FALSE, NULL);

        hDCMem = CreateCompatibleDC(Console->hDC);
        hBitmapTarget = CreateCompatibleBitmap(Console->hDC,
                                                  TargetSize.X,
                                                  TargetSize.Y);
        if (hBitmapTarget) {
            hBitmapOld = SelectObject(hDCMem, hBitmapTarget);
            if (ScreenInfo->hPalette) {
                hPaletteOld = SelectPalette(hDCMem,
                                             ScreenInfo->hPalette,
                                             FALSE);
            }
            MyInvert(Console,&Console->SelectionRect);

            // if (DIB is a top-down)
            //      ySrc = abs(height) - rect.bottom - 1;
            // else
            //      ySrc = rect.Bottom.
            //
            Height = ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo->bmiHeader.biHeight;

            StretchDIBits(hDCMem, 0, 0,
                        TargetSize.X, TargetSize.Y,
                        Console->SelectionRect.Left + ScreenInfo->Window.Left,
                        (Height < 0) ? -Height - (Console->SelectionRect.Bottom + ScreenInfo->Window.Top) -  1
                        : Console->SelectionRect.Bottom + ScreenInfo->Window.Top,
                        TargetSize.X, TargetSize.Y,
                        ScreenInfo->BufferInfo.GraphicsInfo.BitMap,
                        ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo,
                        ScreenInfo->BufferInfo.GraphicsInfo.dwUsage,
                        SRCCOPY);
            MyInvert(Console,&Console->SelectionRect);
            if (ScreenInfo->hPalette) {
                SelectPalette(hDCMem, hPaletteOld, FALSE);
            }
            SelectObject(hDCMem, hBitmapOld);
            OpenClipboard(Console->hWnd);
            EmptyClipboard();
            SetClipboardData(CF_BITMAP,hBitmapTarget);
            CloseClipboard();
        }
        DeleteDC(hDCMem);
        NtReleaseMutant(ScreenInfo->BufferInfo.GraphicsInfo.hMutex, NULL);
    }

}

VOID
DoCopy(
    IN PCONSOLE_INFORMATION Console
    )
{
    StoreSelection(Console);        // store selection in clipboard
    ClearSelection(Console);        // clear selection in console
}

/*++

Routine Description:

    This routine pastes given Unicode string into the console window.

Arguments:
    Console  -   Pointer to CONSOLE_INFORMATION structure
    pwStr    -   Unicode string that is pasted to the console window
    DataSize -   Size of the Unicode String in characters


Return Value:
    None


--*/


VOID
DoStringPaste(
    IN PCONSOLE_INFORMATION Console,
    IN PWCHAR pwStr,
    IN UINT DataSize
    )
{
    PINPUT_RECORD StringData,CurRecord;
    PWCHAR CurChar;
    WCHAR Char;
    DWORD i;
    DWORD ChunkSize,j;
    ULONG EventsWritten;


    if(!pwStr) {
       return;
    }

    if (DataSize > DATA_CHUNK_SIZE) {
        ChunkSize = DATA_CHUNK_SIZE;
    } else {
        ChunkSize = DataSize;
    }

    //
    // allocate space to copy data.
    //

    StringData = (PINPUT_RECORD)ConsoleHeapAlloc(MAKE_TAG( TMP_TAG ),ChunkSize*sizeof(INPUT_RECORD)*8); // 8 is maximum number of events per char
    if (StringData == NULL) {
        return;
    }

    //
    // transfer data to the input buffer in chunks
    //

    CurChar = pwStr;   // LATER remove this
    for (j = 0; j < DataSize; j += ChunkSize) {
        if (ChunkSize > DataSize - j) {
            ChunkSize = DataSize - j;
        }
        CurRecord = StringData;
        for (i = 0, EventsWritten = 0; i < ChunkSize; i++) {
            // filter out LF if not first char and preceded by CR
            Char = *CurChar;
            if (Char != UNICODE_LINEFEED || (i==0 && j==0) || (*(CurChar-1)) != UNICODE_CARRIAGERETURN) {
                SHORT KeyState;
                BYTE KeyFlags;
                BOOL AltGr=FALSE;
                BOOL Shift=FALSE;

                if (Char == 0) {
                    j = DataSize;
                    break;
                }

                KeyState = VkKeyScan(Char);
#if defined(FE_SB)
                if (CONSOLE_IS_DBCS_ENABLED() &&
                    (KeyState == -1)) {
                    WORD CharType;
                    //
                    // Determine DBCS character because these character doesn't know by VkKeyScan.
                    // GetStringTypeW(CT_CTYPE3) & C3_ALPHA can determine all linguistic characters.
                    // However, this is not include symbolic character for DBCS.
                    // IsConsoleFullWidth can help for DBCS symbolic character.
                    //
                    GetStringTypeW(CT_CTYPE3,&Char,1,&CharType);
                    if ((CharType & C3_ALPHA) ||
                        IsConsoleFullWidth(Console->hDC,Console->OutputCP,Char)) {
                        KeyState = 0;
                    }
                }
#endif

                // if VkKeyScanW fails (char is not in kbd layout), we must
                // emulate the key being input through the numpad

                if (KeyState == -1) {
                    CHAR CharString[4];
                    UCHAR OemChar;
                    PCHAR pCharString;

                    ConvertToOem(Console->OutputCP,
                                 &Char,
                                 1,
                                 &OemChar,
                                 1
                                );

                    _itoa(OemChar, CharString, 10);

                    EventsWritten++;
                    LoadKeyEvent(CurRecord,TRUE,0,VK_MENU,0x38,LEFT_ALT_PRESSED);
                    CurRecord++;

                    for (pCharString=CharString;*pCharString;pCharString++) {
                        WORD wVirtualKey, wScancode;
                        EventsWritten++;
                        wVirtualKey = *pCharString-'0'+VK_NUMPAD0;
                        wScancode = (WORD)MapVirtualKey(wVirtualKey, 0);
                        LoadKeyEvent(CurRecord,TRUE,0,wVirtualKey,wScancode,LEFT_ALT_PRESSED);
                        CurRecord++;
                        EventsWritten++;
                        LoadKeyEvent(CurRecord,FALSE,0,wVirtualKey,wScancode,LEFT_ALT_PRESSED);
                        CurRecord++;
                    }

                    EventsWritten++;
                    LoadKeyEvent(CurRecord,FALSE,Char,VK_MENU,0x38,0);
                    CurRecord++;
                } else {
                    KeyFlags = HIBYTE(KeyState);

                    // handle yucky alt-gr keys
                    if ((KeyFlags & 6) == 6) {
                        AltGr=TRUE;
                        EventsWritten++;
                        LoadKeyEvent(CurRecord,TRUE,0,VK_MENU,0x38,ENHANCED_KEY | LEFT_CTRL_PRESSED | RIGHT_ALT_PRESSED);
                        CurRecord++;
                    } else if (KeyFlags & 1) {
                        Shift=TRUE;
                        EventsWritten++;
                        LoadKeyEvent(CurRecord,TRUE,0,VK_SHIFT,0x2a,SHIFT_PRESSED);
                        CurRecord++;
                    }

                    EventsWritten++;
                    LoadKeyEvent(CurRecord,
                                 TRUE,
                                 Char,
                                 LOBYTE(KeyState),
                                 (WORD)MapVirtualKey(CurRecord->Event.KeyEvent.wVirtualKeyCode,0),
                                 0);
                    if (KeyFlags & 1)
                        CurRecord->Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
                    if (KeyFlags & 2)
                        CurRecord->Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
                    if (KeyFlags & 4)
                        CurRecord->Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
                    CurRecord++;

                    EventsWritten++;
                    *CurRecord = *(CurRecord-1);
                    CurRecord->Event.KeyEvent.bKeyDown = FALSE;
                    CurRecord++;

                    // handle yucky alt-gr keys
                    if (AltGr) {
                        EventsWritten++;
                        LoadKeyEvent(CurRecord,FALSE,0,VK_MENU,0x38,ENHANCED_KEY);
                        CurRecord++;
                    } else if (Shift) {
                        EventsWritten++;
                        LoadKeyEvent(CurRecord,FALSE,0,VK_SHIFT,0x2a,0);
                        CurRecord++;
                    }
                }
            }
            CurChar++;
        }
        EventsWritten = WriteInputBuffer(Console,
                                         &Console->InputBuffer,
                                         StringData,
                                         EventsWritten
                                         );
    }
    ConsoleHeapFree(StringData);
    return;
}

VOID
DoPaste(
    IN PCONSOLE_INFORMATION Console
    )

/*++

  Perform paste request into old app by sucking out clipboard
        contents and writing them to the console's input buffer

--*/

{
    BOOL Success;
    HANDLE ClipboardDataHandle;

    if (Console->Flags & CONSOLE_SCROLLING) {
        return;
    }

    //
    // Get paste data from clipboard
    //

    Success = OpenClipboard(Console->hWnd);
    if (!Success)
        return;

    if (Console->CurrentScreenBuffer->Flags & CONSOLE_TEXTMODE_BUFFER) {
        PWCHAR pwstr;

        ClipboardDataHandle = GetClipboardData(CF_UNICODETEXT);
        if (ClipboardDataHandle == NULL) {
            CloseClipboard();   // Close clipboard
            return;
        }
        pwstr = GlobalLock(ClipboardDataHandle);
        DoStringPaste(Console,pwstr,(ULONG)GlobalSize(ClipboardDataHandle)/sizeof(WCHAR));
        GlobalUnlock(ClipboardDataHandle);

    } else {
        HBITMAP hBitmapSource,hBitmapTarget;
        HDC hDCMemSource,hDCMemTarget;
        BITMAP bm;
        PSCREEN_INFORMATION ScreenInfo;

        hBitmapSource = GetClipboardData(CF_BITMAP);
        if (hBitmapSource) {

            ScreenInfo = Console->CurrentScreenBuffer;
            NtWaitForSingleObject(ScreenInfo->BufferInfo.GraphicsInfo.hMutex,
                                  FALSE, NULL);

            hBitmapTarget = CreateDIBitmap(Console->hDC,
                                     &ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo->bmiHeader,
                                     CBM_INIT,
                                     ScreenInfo->BufferInfo.GraphicsInfo.BitMap,
                                     ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo,
                                     ScreenInfo->BufferInfo.GraphicsInfo.dwUsage
                                    );
            if (hBitmapTarget) {
                hDCMemTarget = CreateCompatibleDC ( Console->hDC );
                hDCMemSource = CreateCompatibleDC ( Console->hDC );
                SelectObject( hDCMemTarget, hBitmapTarget );
                SelectObject( hDCMemSource, hBitmapSource );
                GetObjectW(hBitmapSource, sizeof (BITMAP), (LPSTR) &bm);
                BitBlt ( hDCMemTarget, 0, 0, bm.bmWidth, bm.bmHeight,
                     hDCMemSource, 0, 0, SRCCOPY);
                GetObjectW(hBitmapTarget, sizeof (BITMAP), (LPSTR) &bm);

                // copy the bits from the DC to memory

                GetDIBits(hDCMemTarget, hBitmapTarget, 0, bm.bmHeight,
                          ScreenInfo->BufferInfo.GraphicsInfo.BitMap,
                          ScreenInfo->BufferInfo.GraphicsInfo.lpBitMapInfo,
                          ScreenInfo->BufferInfo.GraphicsInfo.dwUsage);
                DeleteDC(hDCMemSource);
                DeleteDC(hDCMemTarget);
                DeleteObject(hBitmapTarget);
                InvalidateRect(Console->hWnd,NULL,FALSE); // force repaint
            }
            NtReleaseMutant(ScreenInfo->BufferInfo.GraphicsInfo.hMutex, NULL);
        }
    }
    CloseClipboard();
    return;
}

VOID
InitSelection(
    IN PCONSOLE_INFORMATION Console
    )

/*++

    This routine initializes the selection process.  It is called
    when the user selects the Mark option from the system menu.

--*/

{
    COORD Position;
    PSCREEN_INFORMATION ScreenInfo;

    //
    // if already selecting, cancel selection.
    //

    if (Console->Flags & CONSOLE_SELECTING) {
        if (Console->SelectionFlags & CONSOLE_MOUSE_SELECTION) {
            CancelMouseSelection(Console);
        } else {
            CancelKeySelection(Console,FALSE);
        }
    }

    //
    // set flags
    //

    Console->Flags |= CONSOLE_SELECTING;
    Console->SelectionFlags = 0;

    //
    // save old cursor position and
    // make console cursor into selection cursor.
    //

    ScreenInfo = Console->CurrentScreenBuffer;
    Console->TextCursorPosition = ScreenInfo->BufferInfo.TextInfo.CursorPosition;
    Console->TextCursorVisible = (BOOLEAN)ScreenInfo->BufferInfo.TextInfo.CursorVisible;
    Console->TextCursorSize =   ScreenInfo->BufferInfo.TextInfo.CursorSize;
    ConsoleHideCursor(ScreenInfo);
    SetCursorInformation(ScreenInfo,
                         100,
                         TRUE
                        );
    Position.X = ScreenInfo->Window.Left;
    Position.Y = ScreenInfo->Window.Top;
    SetCursorPosition(ScreenInfo,
                      Position,
                      TRUE
                     );
    ConsoleShowCursor(ScreenInfo);

    //
    // init select rect
    //

    Console->SelectionAnchor = Position;

    //
    // set win text
    //

    SetWinText(Console,msgMarkMode,TRUE);

}

VOID
DoMark(
    IN PCONSOLE_INFORMATION Console
    )
{
    InitSelection(Console);        // initialize selection
}

VOID
DoSelectAll(
    IN PCONSOLE_INFORMATION Console
    )
{
    COORD Position;
    COORD WindowOrigin;
    PSCREEN_INFORMATION ScreenInfo;

    // clear any old selections
    if (Console->Flags & CONSOLE_SELECTING) {
        ClearSelection(Console);
    }

    // save the old window position
    ScreenInfo = Console->CurrentScreenBuffer;
    WindowOrigin.X = ScreenInfo->Window.Left;
    WindowOrigin.Y = ScreenInfo->Window.Top;

    // initialize selection
    Console->Flags |= CONSOLE_SELECTING;
    Console->SelectionFlags = CONSOLE_MOUSE_SELECTION | CONSOLE_SELECTION_NOT_EMPTY;
    Console->SelectionAnchor.X = Console->SelectionRect.Left = Console->SelectionRect.Right = 0;
    Console->SelectionAnchor.Y = Console->SelectionRect.Top = Console->SelectionRect.Bottom = 0;
    MyInvert(Console,&Console->SelectionRect);
    SetWinText(Console,msgSelectMode,TRUE);

    // extend selection
    Position.X = ScreenInfo->ScreenBufferSize.X - 1;
    Position.Y = ScreenInfo->BufferInfo.TextInfo.CursorPosition.Y;
    ExtendSelection(Console, Position);

    // restore the old window position
    SetWindowOrigin(ScreenInfo, TRUE, WindowOrigin);
}

VOID
DoScroll(
    IN PCONSOLE_INFORMATION Console
    )
{
    if (!(Console->Flags & CONSOLE_SCROLLING)) {
        SetWinText(Console,msgScrollMode,TRUE);
        Console->Flags |= CONSOLE_SCROLLING;
    }
}

VOID
ClearScroll(
    IN PCONSOLE_INFORMATION Console
    )
{
    SetWinText(Console,msgScrollMode,FALSE);
    Console->Flags &= ~CONSOLE_SCROLLING;
}

VOID
ScrollIfNecessary(
    IN PCONSOLE_INFORMATION Console,
    IN PSCREEN_INFORMATION ScreenInfo
    )
{
    POINT CursorPos;
    RECT ClientRect;
    COORD MousePosition;

    if (Console->Flags & CONSOLE_SELECTING &&
        Console->SelectionFlags & CONSOLE_MOUSE_DOWN) {
        if (!GetCursorPos(&CursorPos)) {
            return;
        }
        if (!GetClientRect(Console->hWnd,&ClientRect)) {
            return;
        }
        MapWindowPoints(Console->hWnd,NULL,(LPPOINT)&ClientRect,2);
        if (!(PtInRect(&ClientRect,CursorPos))) {
            ScreenToClient(Console->hWnd,&CursorPos);
            MousePosition.X = (SHORT)CursorPos.x;
            MousePosition.Y = (SHORT)CursorPos.y;
            if (ScreenInfo->Flags & CONSOLE_TEXTMODE_BUFFER) {
                MousePosition.X /= SCR_FONTSIZE(ScreenInfo).X;
                MousePosition.Y /= SCR_FONTSIZE(ScreenInfo).Y;
            }
            MousePosition.X += ScreenInfo->Window.Left;
            MousePosition.Y += ScreenInfo->Window.Top;

            ExtendSelection(Console,
                            MousePosition
                           );
        }
    }
}
