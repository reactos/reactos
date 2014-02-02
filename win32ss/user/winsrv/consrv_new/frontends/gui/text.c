/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/gui/text.c
 * PURPOSE:         GUI Terminal Front-End - Support for text-mode screen-buffers
 * PROGRAMMERS:     Gé van Geldorp
 *                  Johannes Anderwald
 *                  Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/settings.h"
#include "guisettings.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

VOID
GuiCopyFromTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    PCONSOLE Console = Buffer->Header.Console;

    /*
     * Pressing the Shift key while copying text, allows us to copy
     * text without newline characters (inline-text copy mode).
     */
    BOOL InlineCopyMode = (GetKeyState(VK_SHIFT) & 0x8000);

    HANDLE hData;
    PCHAR_INFO ptr;
    LPWSTR data, dstPos;
    ULONG selWidth, selHeight;
    ULONG xPos, yPos, size;

    selWidth  = Console->Selection.srSelection.Right - Console->Selection.srSelection.Left + 1;
    selHeight = Console->Selection.srSelection.Bottom - Console->Selection.srSelection.Top + 1;
    DPRINT("Selection is (%d|%d) to (%d|%d)\n",
           Console->Selection.srSelection.Left,
           Console->Selection.srSelection.Top,
           Console->Selection.srSelection.Right,
           Console->Selection.srSelection.Bottom);

    /* Basic size for one line... */
    size = selWidth;
    /* ... and for the other lines, add newline characters if needed. */
    if (selHeight > 0)
    {
        /*
         * If we are not in inline-text copy mode, each selected line must
         * finish with \r\n . Otherwise, the lines will be just concatenated.
         */
        size += (selWidth + (!InlineCopyMode ? 2 : 0)) * (selHeight - 1);
    }
    size += 1; /* Null-termination */
    size *= sizeof(WCHAR);

    /* Allocate memory, it will be passed to the system and may not be freed here */
    hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
    if (hData == NULL) return;

    data = GlobalLock(hData);
    if (data == NULL) return;

    DPRINT("Copying %dx%d selection\n", selWidth, selHeight);
    dstPos = data;

    for (yPos = 0; yPos < selHeight; yPos++)
    {
        ptr = ConioCoordToPointer(Buffer, 
                                  Console->Selection.srSelection.Left,
                                  yPos + Console->Selection.srSelection.Top);
        /* Copy only the characters, leave attributes alone */
        for (xPos = 0; xPos < selWidth; xPos++)
        {
            /*
             * Sometimes, applications can put NULL chars into the screen-buffer
             * (this behaviour is allowed). Detect this and replace by a space.
             * FIXME - HACK: Improve the way we're doing that (i.e., put spaces
             * instead of NULLs (or even, nothing) only if it exists a non-null
             * char *after* those NULLs, before the end-of-line of the selection.
             * Do the same concerning spaces -- i.e. trailing spaces --).
             */
            dstPos[xPos] = (ptr[xPos].Char.UnicodeChar ? ptr[xPos].Char.UnicodeChar : L' ');
        }
        dstPos += selWidth;

        /* Add newline characters if we are not in inline-text copy mode */
        if (!InlineCopyMode)
        {
            if (yPos != (selHeight - 1))
            {
                wcscat(data, L"\r\n");
                dstPos += 2;
            }
        }
    }

    DPRINT("Setting data <%S> to clipboard\n", data);
    GlobalUnlock(hData);

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hData);
}

VOID
GuiPasteToTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    PCONSOLE Console = Buffer->Header.Console;

    HANDLE hData;
    LPWSTR str;
    WCHAR CurChar = 0;

    SHORT VkKey; // MAKEWORD(low = vkey_code, high = shift_state);
    INPUT_RECORD er;

    hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) return;

    str = GlobalLock(hData);
    if (str == NULL) return;

    DPRINT("Got data <%S> from clipboard\n", str);

    er.EventType = KEY_EVENT;
    er.Event.KeyEvent.wRepeatCount = 1;
    while (*str)
    {
        /* \r or \n characters. Go to the line only if we get "\r\n" sequence. */
        if (CurChar == L'\r' && *str == L'\n')
        {
            str++;
            continue;
        }
        CurChar = *str++;

        /* Get the key code (+ shift state) corresponding to the character */
        VkKey = VkKeyScanW(CurChar);
        if (VkKey == 0xFFFF)
        {
            DPRINT1("VkKeyScanW failed - Should simulate the key...\n");
            continue;
        }

        /* Pressing some control keys */

        /* Pressing the character key, with the control keys maintained pressed */
        er.Event.KeyEvent.bKeyDown = TRUE;
        er.Event.KeyEvent.wVirtualKeyCode = LOBYTE(VkKey);
        er.Event.KeyEvent.wVirtualScanCode = MapVirtualKeyW(LOBYTE(VkKey), MAPVK_VK_TO_CHAR);
        er.Event.KeyEvent.uChar.UnicodeChar = CurChar;
        er.Event.KeyEvent.dwControlKeyState = 0;
        if (HIBYTE(VkKey) & 1)
            er.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
        if (HIBYTE(VkKey) & 2)
            er.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED; // RIGHT_CTRL_PRESSED;
        if (HIBYTE(VkKey) & 4)
            er.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED; // RIGHT_ALT_PRESSED;

        ConioProcessInputEvent(Console, &er);

        /* Up all the character and control keys */
        er.Event.KeyEvent.bKeyDown = FALSE;
        ConioProcessInputEvent(Console, &er);
    }

    GlobalUnlock(hData);
}

VOID
GuiPaintTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       HDC hDC,
                       PRECT rc)
{
    PCONSOLE Console = Buffer->Header.Console;
    // ASSERT(Console == GuiData->Console);

    ULONG TopLine, BottomLine, LeftChar, RightChar;
    ULONG Line, Char, Start;
    PCHAR_INFO From;
    PWCHAR To;
    WORD LastAttribute, Attribute;
    ULONG CursorX, CursorY, CursorHeight;
    HBRUSH CursorBrush, OldBrush;
    HFONT OldFont;

    if (Buffer->Buffer == NULL) return;

    TopLine = rc->top / GuiData->CharHeight + Buffer->ViewOrigin.Y;
    BottomLine = (rc->bottom + (GuiData->CharHeight - 1)) / GuiData->CharHeight - 1 + Buffer->ViewOrigin.Y;
    LeftChar = rc->left / GuiData->CharWidth + Buffer->ViewOrigin.X;
    RightChar = (rc->right + (GuiData->CharWidth - 1)) / GuiData->CharWidth - 1 + Buffer->ViewOrigin.X;

    LastAttribute = ConioCoordToPointer(Buffer, LeftChar, TopLine)->Attributes;

    SetTextColor(hDC, RGBFromAttrib(Console, TextAttribFromAttrib(LastAttribute)));
    SetBkColor(hDC, RGBFromAttrib(Console, BkgdAttribFromAttrib(LastAttribute)));

    if (BottomLine >= Buffer->ScreenBufferSize.Y) BottomLine = Buffer->ScreenBufferSize.Y - 1;
    if (RightChar  >= Buffer->ScreenBufferSize.X) RightChar  = Buffer->ScreenBufferSize.X - 1;

    OldFont = SelectObject(hDC, GuiData->Font);

    for (Line = TopLine; Line <= BottomLine; Line++)
    {
        WCHAR LineBuffer[80];   // Buffer containing a part or all the line to be displayed
        From  = ConioCoordToPointer(Buffer, LeftChar, Line);    // Get the first code of the line
        Start = LeftChar;
        To    = LineBuffer;

        for (Char = LeftChar; Char <= RightChar; Char++)
        {
            /*
             * We flush the buffer if the new attribute is different
             * from the current one, or if the buffer is full.
             */
            if (From->Attributes != LastAttribute || (Char - Start == sizeof(LineBuffer) / sizeof(WCHAR)))
            {
                TextOutW(hDC,
                         (Start - Buffer->ViewOrigin.X) * GuiData->CharWidth ,
                         (Line  - Buffer->ViewOrigin.Y) * GuiData->CharHeight,
                         LineBuffer,
                         Char - Start);
                Start = Char;
                To    = LineBuffer;
                Attribute = From->Attributes;
                if (Attribute != LastAttribute)
                {
                    SetTextColor(hDC, RGBFromAttrib(Console, TextAttribFromAttrib(Attribute)));
                    SetBkColor(hDC, RGBFromAttrib(Console, BkgdAttribFromAttrib(Attribute)));
                    LastAttribute = Attribute;
                }
            }

            *(To++) = (From++)->Char.UnicodeChar;
        }

        TextOutW(hDC,
                 (Start - Buffer->ViewOrigin.X) * GuiData->CharWidth ,
                 (Line  - Buffer->ViewOrigin.Y) * GuiData->CharHeight,
                 LineBuffer,
                 RightChar - Start + 1);
    }

    /*
     * Draw the caret
     */
    if (Buffer->CursorInfo.bVisible &&
        Buffer->CursorBlinkOn &&
        !Buffer->ForceCursorOff)
    {
        CursorX = Buffer->CursorPosition.X;
        CursorY = Buffer->CursorPosition.Y;
        if (LeftChar <= CursorX && CursorX <= RightChar &&
            TopLine  <= CursorY && CursorY <= BottomLine)
        {
            CursorHeight = ConioEffectiveCursorSize(Console, GuiData->CharHeight);
            Attribute = ConioCoordToPointer(Buffer, Buffer->CursorPosition.X, Buffer->CursorPosition.Y)->Attributes;

            if (Attribute != DEFAULT_SCREEN_ATTRIB)
            {
                CursorBrush = CreateSolidBrush(RGBFromAttrib(Console, Attribute));
            }
            else
            {
                CursorBrush = CreateSolidBrush(RGBFromAttrib(Console, Buffer->ScreenDefaultAttrib));
            }

            OldBrush = SelectObject(hDC, CursorBrush);
            PatBlt(hDC,
                   (CursorX - Buffer->ViewOrigin.X) * GuiData->CharWidth,
                   (CursorY - Buffer->ViewOrigin.Y) * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                   GuiData->CharWidth,
                   CursorHeight,
                   PATCOPY);
            SelectObject(hDC, OldBrush);
            DeleteObject(CursorBrush);
        }
    }

    SelectObject(hDC, OldFont);
}

/* EOF */
