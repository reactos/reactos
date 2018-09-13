/*++

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    find.c

Abstract:

        This file implements the search functionality.

Author:

    Jerry Shea (jerrysh) 1-May-1997

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


#define SEARCH_STRING_LENGTH    (80)

USHORT
SearchForString(
    IN PSCREEN_INFORMATION ScreenInfo,
    IN PWSTR SearchString,
    IN USHORT StringLength,
    IN BOOLEAN IgnoreCase,
    IN BOOLEAN Reverse,
    OUT PCOORD StringPosition
    )
{
    PCONSOLE_INFORMATION Console;
    COORD MaxPosition;
    COORD EndPosition;
    COORD Position;
    BOOL RecomputeRow;
    SHORT RowIndex;
    PROW Row;
    USHORT ColumnWidth;
    WCHAR SearchString2[SEARCH_STRING_LENGTH * 2 + 1];    // search string buffer
    PWSTR pStr;

    Console = ScreenInfo->Console;

    MaxPosition.X = ScreenInfo->ScreenBufferSize.X - StringLength;
    MaxPosition.Y = ScreenInfo->ScreenBufferSize.Y - 1;

    //
    // calculate starting position
    //

    if (Console->Flags & CONSOLE_SELECTING) {
        Position.X = Console->SelectionAnchor.X;
        Position.Y = Console->SelectionAnchor.Y;
    } else if (Reverse) {
        Position.X = 0;
        Position.Y = 0;
    } else {
        Position.X = MaxPosition.X;
        Position.Y = MaxPosition.Y;
    }

    //
    // prepare search string
    //
    // Raid #113599 CMD:Find(Japanese strings) does not work correctly
    //

    ASSERT(StringLength == wcslen(SearchString) && StringLength < ARRAY_SIZE(SearchString2));

    pStr = SearchString2;
    while (*SearchString) {
        *pStr++ = *SearchString;
#if defined(CON_TB_MARK)
        //
        // On the screen, one FarEast "FullWidth" character occupies two columns (double width),
        // so we have to share two screen buffer elements for one DBCS character.
        // For example, if the screen shows "AB[DBC]CD", the screen buffer will be,
        //   [L'A'] [L'B'] [DBC(Unicode)] [CON_TB_MARK] [L'C'] [L'D']
        //   (DBC:: Double Byte Character)
        // CON_TB_MARK is used to indicate that the column is the trainling byte.
        //
        // Before comparing the string with the screen buffer, we need to modify the search
        // string to match the format of the screen buffer.
        // If we find a FullWidth character in the search string, put CON_TB_MARK
        // right after it so that we're able to use NLS functions.
        //
#else
        //
        // If KAttribute is used, the above example will look like:
        // CharRow.Chars: [L'A'] [L'B'] [DBC(Unicode)] [DBC(Unicode)] [L'C'] [L'D']
        // CharRow.KAttrs:    0      0   LEADING_BYTE  TRAILING_BYTE       0      0
        //
#endif
        if (IsConsoleFullWidth(Console->hDC, Console->CP, *SearchString)) {
#if defined(CON_TB_MARK)
            *pStr++ = CON_TB_MARK;
#else
            *pStr++ = *SearchString;
#endif
        }
        ++SearchString;
    }

    *pStr = L'\0';
    ColumnWidth = (USHORT)(pStr - SearchString2);
    SearchString = SearchString2;

    //
    // set the string length in byte
    //

    StringLength = ColumnWidth * sizeof(WCHAR);

    //
    // search for the string
    //

    RecomputeRow = TRUE;
    EndPosition = Position;
    do {
#if !defined(CON_TB_MARK)
#if DBG
        int nLoop = 0;
#endif
recalc:
#endif
        if (Reverse) {
            if (--Position.X < 0) {
                Position.X = MaxPosition.X;
                if (--Position.Y < 0) {
                    Position.Y = MaxPosition.Y;
                }
                RecomputeRow = TRUE;
            }
        } else {
            if (++Position.X > MaxPosition.X) {
                Position.X = 0;
                if (++Position.Y > MaxPosition.Y) {
                    Position.Y = 0;
                }
                RecomputeRow = TRUE;
            }
        }
        if (RecomputeRow) {
            RowIndex = (ScreenInfo->BufferInfo.TextInfo.FirstRow + Position.Y) % ScreenInfo->ScreenBufferSize.Y;
            Row = &ScreenInfo->BufferInfo.TextInfo.Rows[RowIndex];
            RecomputeRow = FALSE;
        }
#if !defined(CON_TB_MARK)
        ASSERT(nLoop++ < 2);
        if (Row->CharRow.KAttrs && (Row->CharRow.KAttrs[Position.X] & ATTR_TRAILING_BYTE)) {
            goto recalc;
        }
#endif
        if (!MyStringCompareW(SearchString, &Row->CharRow.Chars[Position.X], StringLength, IgnoreCase)) {
            *StringPosition = Position;
            return ColumnWidth;
        }
    } while (!(Position.X == EndPosition.X && Position.Y == EndPosition.Y));

    return 0;   // the string was not found
}

INT_PTR
FindDialogProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCONSOLE_INFORMATION Console;
    PSCREEN_INFORMATION ScreenInfo;
    USHORT StringLength;
    USHORT ColumnWidth;
    WCHAR szBuf[SEARCH_STRING_LENGTH + 1];
    COORD Position;
    BOOLEAN IgnoreCase;
    BOOLEAN Reverse;

    switch (Message) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hWnd, DWLP_USER, lParam);
        SendDlgItemMessage(hWnd, ID_FINDSTR, EM_LIMITTEXT, ARRAY_SIZE(szBuf)-1, 0);
        CheckRadioButton(hWnd, ID_FINDUP, ID_FINDDOWN, ID_FINDDOWN);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            StringLength = (USHORT)GetDlgItemText(hWnd, ID_FINDSTR, szBuf, ARRAY_SIZE(szBuf));
            if (StringLength == 0) {
                break;
            }
            IgnoreCase = IsDlgButtonChecked(hWnd, ID_FINDCASE) == 0;
            Reverse = IsDlgButtonChecked(hWnd, ID_FINDDOWN) == 0;
            Console = (PCONSOLE_INFORMATION)GetWindowLongPtr(hWnd, DWLP_USER);
            ScreenInfo = Console->CurrentScreenBuffer;
            if ((ColumnWidth = SearchForString(ScreenInfo, szBuf, StringLength, IgnoreCase, Reverse, &Position)) != 0) {

                //
                // Clear any old selections
                //

                if (Console->Flags & CONSOLE_SELECTING) {
                    ClearSelection(Console);
                }

                //
                // Make the new selection
                //

                Console->Flags |= CONSOLE_SELECTING;
                Console->SelectionFlags = CONSOLE_MOUSE_SELECTION | CONSOLE_SELECTION_NOT_EMPTY;
                Console->SelectionAnchor = Position;
                Console->SelectionRect.Left = Console->SelectionAnchor.X;
                Console->SelectionRect.Right = Console->SelectionRect.Left + ColumnWidth - 1;
                Console->SelectionRect.Top = Console->SelectionRect.Bottom = Console->SelectionAnchor.Y;
                MyInvert(Console,&Console->SelectionRect);
                SetWinText(Console,msgSelectMode,TRUE);

                //
                // Make sure the hilited text will be visible
                //

                if (Console->SelectionRect.Left < ScreenInfo->Window.Left) {
                    Position.X = Console->SelectionRect.Left;
                } else if (Console->SelectionRect.Right > ScreenInfo->Window.Right) {
                    Position.X = Console->SelectionRect.Right - CONSOLE_WINDOW_SIZE_X(ScreenInfo) + 1;
                } else {
                    Position.X = ScreenInfo->Window.Left;
                }
                if (Console->SelectionRect.Top < ScreenInfo->Window.Top) {
                    Position.Y = Console->SelectionRect.Top;
                } else if (Console->SelectionRect.Bottom > ScreenInfo->Window.Bottom) {
                    Position.Y = Console->SelectionRect.Bottom - CONSOLE_WINDOW_SIZE_Y(ScreenInfo) + 1;
                } else {
                    Position.Y = ScreenInfo->Window.Top;
                }
                SetWindowOrigin(ScreenInfo, TRUE, Position);
                return TRUE;
            } else {

                //
                // The string wasn't found
                //

                Beep(800, 200);
            }
            break;
        case IDCANCEL:
            EndDialog(hWnd, 0);
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

VOID
DoFind(
   IN PCONSOLE_INFORMATION Console
   )
{
    DialogBoxParam(ghInstance,
                   MAKEINTRESOURCE(ID_FINDDLG),
                   Console->hWnd,
                   FindDialogProc,
                   (LPARAM)Console);
}
