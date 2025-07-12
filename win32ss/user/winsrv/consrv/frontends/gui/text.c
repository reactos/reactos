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

#include <consrv.h>

#define NDEBUG
#include <debug.h>

#include "guiterm.h"

/* GLOBALS ********************************************************************/

#define IS_WHITESPACE(c)    ((c) == L'\0' || (c) == L' ' || (c) == L'\t')

/* FUNCTIONS ******************************************************************/

static COLORREF
PaletteRGBFromAttrib(PCONSRV_CONSOLE Console, WORD Attribute)
{
    HPALETTE hPalette = Console->ActiveBuffer->PaletteHandle;
    PALETTEENTRY pe;

    if (hPalette == NULL) return RGBFromAttrib(Console, Attribute);

    GetPaletteEntries(hPalette, Attribute, 1, &pe);
    return PALETTERGB(pe.peRed, pe.peGreen, pe.peBlue);
}

static VOID
CopyBlock(PTEXTMODE_SCREEN_BUFFER Buffer,
          PSMALL_RECT Selection)
{
    /*
     * Pressing the Shift key while copying text, allows us to copy
     * text without newline characters (inline-text copy mode).
     */
    BOOL InlineCopyMode = !!(GetKeyState(VK_SHIFT) & KEY_PRESSED);

    HANDLE hData;
    PCHAR_INFO ptr;
    LPWSTR data, dstPos;
    ULONG selWidth, selHeight;
    ULONG xPos, yPos;
    ULONG size;

    DPRINT("CopyBlock(%u, %u, %u, %u)\n",
           Selection->Left, Selection->Top, Selection->Right, Selection->Bottom);

    /* Prevent against empty blocks */
    if ((Selection == NULL) || ConioIsRectEmpty(Selection))
        return;

    selWidth  = ConioRectWidth(Selection);
    selHeight = ConioRectHeight(Selection);

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
    else
    {
        DPRINT1("This case must never happen, because selHeight is at least == 1\n");
    }

    size++; /* Null-termination */
    size *= sizeof(WCHAR);

    /* Allocate some memory area to be given to the clipboard, so it will not be freed here */
    hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
    if (hData == NULL) return;

    data = GlobalLock(hData);
    if (data == NULL)
    {
        GlobalFree(hData);
        return;
    }

    DPRINT("Copying %dx%d selection\n", selWidth, selHeight);
    dstPos = data;

    for (yPos = 0; yPos < selHeight; yPos++)
    {
        ULONG length = selWidth;

        ptr = ConioCoordToPointer(Buffer,
                                  Selection->Left,
                                  Selection->Top + yPos);

        /* Trim whitespace from the right */
        while (length > 0)
        {
            if (IS_WHITESPACE(ptr[length-1].Char.UnicodeChar))
                --length;
            else
                break;
        }

        /* Copy only the characters, leave attributes alone */
        for (xPos = 0; xPos < length; xPos++)
        {
            /*
             * Sometimes, applications can put NULL chars into the screen-buffer
             * (this behaviour is allowed). Detect this and replace by a space.
             * For full-width characters: copy only the character specified
             * in the leading-byte cell, skipping the trailing-byte cell.
             */
            if (!(ptr[xPos].Attributes & COMMON_LVB_TRAILING_BYTE))
            {
                *dstPos++ = (ptr[xPos].Char.UnicodeChar ? ptr[xPos].Char.UnicodeChar : L' ');
            }
        }

        /* Add newline characters if we are not in inline-text copy mode */
        if (!InlineCopyMode)
        {
            if (yPos != (selHeight - 1))
            {
                wcscat(dstPos, L"\r\n");
                dstPos += 2;
            }
        }
    }

    DPRINT("Setting data <%S> to clipboard\n", data);
    GlobalUnlock(hData);

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hData);
}

static VOID
CopyLines(PTEXTMODE_SCREEN_BUFFER Buffer,
          PCOORD Begin,
          PCOORD End)
{
    HANDLE hData;
    PCHAR_INFO ptr;
    LPWSTR data, dstPos;
    ULONG NumChars, size;
    ULONG xPos, yPos, xBeg, xEnd;

    DPRINT("CopyLines((%u, %u) ; (%u, %u))\n",
           Begin->X, Begin->Y, End->X, End->Y);

    /* Prevent against empty blocks... */
    if (Begin == NULL || End == NULL) return;
    /* ... or malformed blocks */
    if (Begin->Y > End->Y || (Begin->Y == End->Y && Begin->X > End->X)) return;

    /* Compute the number of characters to copy */
    if (End->Y == Begin->Y) // top == bottom
    {
        NumChars = End->X - Begin->X + 1;
    }
    else // if (End->Y > Begin->Y)
    {
        NumChars = Buffer->ScreenBufferSize.X - Begin->X;

        if (End->Y >= Begin->Y + 2)
        {
            NumChars += (End->Y - Begin->Y - 1) * Buffer->ScreenBufferSize.X;
        }

        NumChars += End->X + 1;
    }

    size = (NumChars + 1) * sizeof(WCHAR); /* Null-terminated */

    /* Allocate some memory area to be given to the clipboard, so it will not be freed here */
    hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, size);
    if (hData == NULL) return;

    data = GlobalLock(hData);
    if (data == NULL)
    {
        GlobalFree(hData);
        return;
    }

    DPRINT("Copying %d characters\n", NumChars);
    dstPos = data;

    /*
     * We need to walk per-lines, and not just looping in the big screen-buffer
     * array, because of the way things are stored inside it. The downside is
     * that it makes the code more complicated.
     */
    for (yPos = Begin->Y; (yPos <= (ULONG)End->Y) && (NumChars > 0); yPos++)
    {
        xBeg = (yPos == Begin->Y ? Begin->X : 0);
        xEnd = (yPos ==   End->Y ?   End->X : Buffer->ScreenBufferSize.X - 1);

        ptr = ConioCoordToPointer(Buffer, 0, yPos);

        /* Copy only the characters, leave attributes alone */
        for (xPos = xBeg; (xPos <= xEnd) && (NumChars-- > 0); xPos++)
        {
            /*
             * Sometimes, applications can put NULL chars into the screen-buffer
             * (this behaviour is allowed). Detect this and replace by a space.
             * For full-width characters: copy only the character specified
             * in the leading-byte cell, skipping the trailing-byte cell.
             */
            if (!(ptr[xPos].Attributes & COMMON_LVB_TRAILING_BYTE))
            {
                *dstPos++ = (ptr[xPos].Char.UnicodeChar ? ptr[xPos].Char.UnicodeChar : L' ');
            }
        }
    }

    DPRINT("Setting data <%S> to clipboard\n", data);
    GlobalUnlock(hData);

    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hData);
}


VOID
PasteText(
    IN PCONSRV_CONSOLE Console,
    IN PWCHAR Buffer,
    IN SIZE_T cchSize)
{
    USHORT VkKey; // MAKEWORD(low = vkey_code, high = shift_state);
    INPUT_RECORD er;
    WCHAR CurChar = 0;

    /* Do nothing if we have nothing to paste */
    if (!Buffer || (cchSize <= 0))
        return;

    er.EventType = KEY_EVENT;
    er.Event.KeyEvent.wRepeatCount = 1;
    while (cchSize--)
    {
        /* \r or \n characters. Go to the line only if we get "\r\n" sequence. */
        if (CurChar == L'\r' && *Buffer == L'\n')
        {
            ++Buffer;
            continue;
        }
        CurChar = *Buffer++;

        /* Get the key code (+ shift state) corresponding to the character */
        VkKey = VkKeyScanW(CurChar);
        if (VkKey == 0xFFFF)
        {
            DPRINT1("FIXME: TODO: VkKeyScanW failed - Should simulate the key!\n");
            /*
             * We don't really need the scan/key code because we actually only
             * use the UnicodeChar for output purposes. It may pose few problems
             * later on but it's not of big importance. One trick would be to
             * convert the character to OEM / multibyte and use MapVirtualKey()
             * on each byte (simulating an Alt-0xxx OEM keyboard press).
             */
            er.Event.KeyEvent.wVirtualKeyCode = VK_PACKET;
            er.Event.KeyEvent.wVirtualScanCode = CurChar;
            er.Event.KeyEvent.uChar.UnicodeChar = CurChar;
        }
        else
        {
            /* Pressing the character key, with the control keys maintained pressed */
            er.Event.KeyEvent.wVirtualKeyCode = LOBYTE(VkKey);
            er.Event.KeyEvent.wVirtualScanCode = MapVirtualKeyW(LOBYTE(VkKey), MAPVK_VK_TO_VSC);
            er.Event.KeyEvent.uChar.UnicodeChar = CurChar;
        }

        er.Event.KeyEvent.bKeyDown = TRUE;
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
}

VOID
GetSelectionBeginEnd(PCOORD Begin, PCOORD End,
                     PCOORD SelectionAnchor,
                     PSMALL_RECT SmallRect);

VOID
GuiCopyFromTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                          PGUI_CONSOLE_DATA GuiData)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    BOOL LineSelection = GuiData->LineSelection;

    DPRINT("Selection is (%d|%d) to (%d|%d) in %s mode\n",
           GuiData->Selection.srSelection.Left,
           GuiData->Selection.srSelection.Top,
           GuiData->Selection.srSelection.Right,
           GuiData->Selection.srSelection.Bottom,
           (LineSelection ? "line" : "block"));

    if (!LineSelection)
    {
        CopyBlock(Buffer, &GuiData->Selection.srSelection);
    }
    else
    {
        COORD Begin, End;

        GetSelectionBeginEnd(&Begin, &End,
                             &GuiData->Selection.dwSelectionAnchor,
                             &GuiData->Selection.srSelection);

        CopyLines(Buffer, &Begin, &End);
    }
}

VOID
GuiPasteToTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                         PGUI_CONSOLE_DATA GuiData)
{
    /*
     * This function supposes that the system clipboard was opened.
     */

    PCONSRV_CONSOLE Console = (PCONSRV_CONSOLE)Buffer->Header.Console;

    HANDLE hData;
    LPWSTR pszText;

    hData = GetClipboardData(CF_UNICODETEXT);
    if (hData == NULL) return;

    pszText = GlobalLock(hData);
    if (pszText == NULL) return;

    DPRINT("Got data <%S> from clipboard\n", pszText);
    PasteText(Console, pszText, wcslen(pszText));

    GlobalUnlock(hData);
}

static VOID
GuiPaintCaret(
    PTEXTMODE_SCREEN_BUFFER Buffer,
    PGUI_CONSOLE_DATA GuiData,
    ULONG TopLine,
    ULONG BottomLine,
    ULONG LeftColumn,
    ULONG RightColumn)
{
    PCONSRV_CONSOLE Console = (PCONSRV_CONSOLE)Buffer->Header.Console;

    ULONG CursorX, CursorY, CursorHeight;
    HBRUSH CursorBrush, OldBrush;
    WORD Attribute;

    if (Buffer->CursorInfo.bVisible &&
        Buffer->CursorBlinkOn &&
        !Buffer->ForceCursorOff)
    {
        CursorX = Buffer->CursorPosition.X;
        CursorY = Buffer->CursorPosition.Y;
        if (LeftColumn <= CursorX && CursorX <= RightColumn &&
            TopLine    <= CursorY && CursorY <= BottomLine)
        {
            CursorHeight = ConioEffectiveCursorSize(Console, GuiData->CharHeight);

            Attribute = ConioCoordToPointer(Buffer, Buffer->CursorPosition.X, Buffer->CursorPosition.Y)->Attributes;
            if (Attribute == DEFAULT_SCREEN_ATTRIB)
                Attribute = Buffer->ScreenDefaultAttrib;

            CursorBrush = CreateSolidBrush(PaletteRGBFromAttrib(Console, TextAttribFromAttrib(Attribute)));
            OldBrush    = SelectObject(GuiData->hMemDC, CursorBrush);

            if (Attribute & COMMON_LVB_LEADING_BYTE)
            {
                /* The caret is on the leading byte */
                PatBlt(GuiData->hMemDC,
                       CursorX * GuiData->CharWidth,
                       CursorY * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                       GuiData->CharWidth * 2,
                       CursorHeight,
                       PATCOPY);
            }
            else if (Attribute & COMMON_LVB_TRAILING_BYTE)
            {
                /* The caret is on the trailing byte */
                PatBlt(GuiData->hMemDC,
                       (CursorX - 1) * GuiData->CharWidth,
                       CursorY * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                       GuiData->CharWidth * 2,
                       CursorHeight,
                       PATCOPY);
            }
            else
            {
                PatBlt(GuiData->hMemDC,
                       CursorX * GuiData->CharWidth,
                       CursorY * GuiData->CharHeight + (GuiData->CharHeight - CursorHeight),
                       GuiData->CharWidth,
                       CursorHeight,
                       PATCOPY);
            }

            SelectObject(GuiData->hMemDC, OldBrush);
            DeleteObject(CursorBrush);
        }
    }
}

VOID
GuiPaintTextModeBuffer(PTEXTMODE_SCREEN_BUFFER Buffer,
                       PGUI_CONSOLE_DATA GuiData,
                       PRECT rcView,
                       PRECT rcFramebuffer)
{
    PCONSRV_CONSOLE Console = (PCONSRV_CONSOLE)Buffer->Header.Console;
    ULONG TopLine, BottomLine, LeftColumn, RightColumn;
    ULONG Line, Char, Start;
    PCHAR_INFO From;
    PWCHAR To;
    WORD LastAttribute, Attribute;
    HFONT OldFont, NewFont;
    BOOLEAN IsUnderline;

    // ASSERT(Console == GuiData->Console);

    ConioInitLongRect(rcFramebuffer, 0, 0, 0, 0);

    if (Buffer->Buffer == NULL)
        return;

    if (!ConDrvValidateConsoleUnsafe((PCONSOLE)Console, CONSOLE_RUNNING, TRUE))
        return;

    ConioInitLongRect(rcFramebuffer,
                      Buffer->ViewOrigin.Y * GuiData->CharHeight + rcView->top,
                      Buffer->ViewOrigin.X * GuiData->CharWidth  + rcView->left,
                      Buffer->ViewOrigin.Y * GuiData->CharHeight + rcView->bottom,
                      Buffer->ViewOrigin.X * GuiData->CharWidth  + rcView->right);

    LeftColumn  = rcFramebuffer->left  / GuiData->CharWidth;
    RightColumn = rcFramebuffer->right / GuiData->CharWidth;
    if (RightColumn >= (ULONG)Buffer->ScreenBufferSize.X)
        RightColumn  = Buffer->ScreenBufferSize.X - 1;

    TopLine    = rcFramebuffer->top    / GuiData->CharHeight;
    BottomLine = rcFramebuffer->bottom / GuiData->CharHeight;
    if (BottomLine >= (ULONG)Buffer->ScreenBufferSize.Y)
        BottomLine  = Buffer->ScreenBufferSize.Y - 1;

    LastAttribute = ConioCoordToPointer(Buffer, LeftColumn, TopLine)->Attributes;

    SetTextColor(GuiData->hMemDC, PaletteRGBFromAttrib(Console, TextAttribFromAttrib(LastAttribute)));
    SetBkColor(GuiData->hMemDC, PaletteRGBFromAttrib(Console, BkgdAttribFromAttrib(LastAttribute)));

    /* We use the underscore flag as a underline flag */
    IsUnderline = !!(LastAttribute & COMMON_LVB_UNDERSCORE);
    /* Select the new font */
    NewFont = GuiData->Font[IsUnderline ? FONT_BOLD : FONT_NORMAL];
    OldFont = SelectObject(GuiData->hMemDC, NewFont);

    if (Console->IsCJK)
    {
        for (Line = TopLine; Line <= BottomLine; Line++)
        {
            for (Char = LeftColumn; Char <= RightColumn; Char++)
            {
                From = ConioCoordToPointer(Buffer, Char, Line);
                Attribute = From->Attributes;
                SetTextColor(GuiData->hMemDC, PaletteRGBFromAttrib(Console, TextAttribFromAttrib(Attribute)));
                SetBkColor(GuiData->hMemDC, PaletteRGBFromAttrib(Console, BkgdAttribFromAttrib(Attribute)));

                /* Change underline state if needed */
                if (!!(Attribute & COMMON_LVB_UNDERSCORE) != IsUnderline)
                {
                    IsUnderline = !!(Attribute & COMMON_LVB_UNDERSCORE);

                    /* Select the new font */
                    NewFont = GuiData->Font[IsUnderline ? FONT_BOLD : FONT_NORMAL];
                    SelectObject(GuiData->hMemDC, NewFont);
                }

                if (Attribute & COMMON_LVB_TRAILING_BYTE)
                    continue;

                TextOutW(GuiData->hMemDC,
                         Char * GuiData->CharWidth,
                         Line * GuiData->CharHeight,
                         &From->Char.UnicodeChar, 1);
            }
        }
    }
    else
    {
        for (Line = TopLine; Line <= BottomLine; Line++)
        {
            WCHAR LineBuffer[80];   // Buffer containing a part or all the line to be displayed
            From  = ConioCoordToPointer(Buffer, LeftColumn, Line);  // Get the first code of the line
            Start = LeftColumn;
            To    = LineBuffer;

            for (Char = LeftColumn; Char <= RightColumn; Char++)
            {
                /*
                 * We flush the buffer if the new attribute is different
                 * from the current one, or if the buffer is full.
                 */
                if (From->Attributes != LastAttribute || (Char - Start == sizeof(LineBuffer) / sizeof(WCHAR)))
                {
                    TextOutW(GuiData->hMemDC,
                             Start * GuiData->CharWidth,
                             Line  * GuiData->CharHeight,
                             LineBuffer,
                             Char - Start);
                    Start = Char;
                    To    = LineBuffer;
                    Attribute = From->Attributes;
                    if (Attribute != LastAttribute)
                    {
                        LastAttribute = Attribute;
                        SetTextColor(GuiData->hMemDC, PaletteRGBFromAttrib(Console, TextAttribFromAttrib(LastAttribute)));
                        SetBkColor(GuiData->hMemDC, PaletteRGBFromAttrib(Console, BkgdAttribFromAttrib(LastAttribute)));

                        /* Change underline state if needed */
                        if (!!(LastAttribute & COMMON_LVB_UNDERSCORE) != IsUnderline)
                        {
                            IsUnderline = !!(LastAttribute & COMMON_LVB_UNDERSCORE);
                            /* Select the new font */
                            NewFont = GuiData->Font[IsUnderline ? FONT_BOLD : FONT_NORMAL];
                            SelectObject(GuiData->hMemDC, NewFont);
                        }
                    }
                }

                *(To++) = (From++)->Char.UnicodeChar;
            }

            TextOutW(GuiData->hMemDC,
                     Start * GuiData->CharWidth,
                     Line  * GuiData->CharHeight,
                     LineBuffer,
                     RightColumn - Start + 1);
        }
    }

    /* Restore the old font */
    SelectObject(GuiData->hMemDC, OldFont);

    /* Draw the caret */
    GuiPaintCaret(Buffer, GuiData, TopLine, BottomLine, LeftColumn, RightColumn);

    LeaveCriticalSection(&Console->Lock);
}

/* EOF */
