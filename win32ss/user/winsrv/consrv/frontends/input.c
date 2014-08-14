/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv/frontends/input.c
 * PURPOSE:         Common Front-Ends Input functions
 * PROGRAMMERS:     Jeffrey Morlan
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "include/conio.h"
#include "include/term.h"
#include "coninput.h"

#define NDEBUG
#include <debug.h>


/* PRIVATE FUNCTIONS **********************************************************/

static VOID
ConDrvProcessKey(IN PCONSOLE Console,
                 IN BOOLEAN Down,
                 IN UINT VirtualKeyCode,
                 IN UINT VirtualScanCode,
                 IN WCHAR UnicodeChar,
                 IN ULONG ShiftState,
                 IN BYTE KeyStateCtrl)
{
    INPUT_RECORD er;

    /* process Ctrl-C and Ctrl-Break */
    if ( Console->InputBuffer.Mode & ENABLE_PROCESSED_INPUT &&
         Down && (VirtualKeyCode == VK_PAUSE || VirtualKeyCode == 'C') &&
         (ShiftState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED) || KeyStateCtrl & 0x80) )
    {
        DPRINT1("Console_Api Ctrl-C\n");
        ConSrvConsoleProcessCtrlEvent(Console, 0, CTRL_C_EVENT);

        if (Console->LineBuffer && !Console->LineComplete)
        {
            /* Line input is in progress; end it */
            Console->LinePos = Console->LineSize = 0;
            Console->LineComplete = TRUE;
        }
        return;
    }

    if ( (ShiftState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED)) != 0 &&
         (VK_UP == VirtualKeyCode || VK_DOWN == VirtualKeyCode) )
    {
        if (!Down) return;

        /* scroll up or down */
        if (VK_UP == VirtualKeyCode)
        {
            /* only scroll up if there is room to scroll up into */
            if (Console->ActiveBuffer->CursorPosition.Y != Console->ActiveBuffer->ScreenBufferSize.Y - 1)
            {
                Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY +
                                                   Console->ActiveBuffer->ScreenBufferSize.Y - 1) %
                                                   Console->ActiveBuffer->ScreenBufferSize.Y;
                Console->ActiveBuffer->CursorPosition.Y++;
            }
        }
        else
        {
            /* only scroll down if there is room to scroll down into */
            if (Console->ActiveBuffer->CursorPosition.Y != 0)
            {
                Console->ActiveBuffer->VirtualY = (Console->ActiveBuffer->VirtualY + 1) %
                                                   Console->ActiveBuffer->ScreenBufferSize.Y;
                Console->ActiveBuffer->CursorPosition.Y--;
            }
        }

        ConioDrawConsole(Console);
        return;
    }

    er.EventType                        = KEY_EVENT;
    er.Event.KeyEvent.bKeyDown          = Down;
    er.Event.KeyEvent.wRepeatCount      = 1;
    er.Event.KeyEvent.wVirtualKeyCode   = VirtualKeyCode;
    er.Event.KeyEvent.wVirtualScanCode  = VirtualScanCode;
    er.Event.KeyEvent.uChar.UnicodeChar = UnicodeChar;
    er.Event.KeyEvent.dwControlKeyState = ShiftState;

    ConioProcessInputEvent(Console, &er);
}

static DWORD
ConioGetShiftState(PBYTE KeyState, LPARAM lParam)
{
    DWORD ssOut = 0;

    if (KeyState[VK_CAPITAL] & 0x01)
        ssOut |= CAPSLOCK_ON;

    if (KeyState[VK_NUMLOCK] & 0x01)
        ssOut |= NUMLOCK_ON;

    if (KeyState[VK_SCROLL] & 0x01)
        ssOut |= SCROLLLOCK_ON;

    if (KeyState[VK_SHIFT] & 0x80)
        ssOut |= SHIFT_PRESSED;

    if (KeyState[VK_LCONTROL] & 0x80)
        ssOut |= LEFT_CTRL_PRESSED;
    if (KeyState[VK_RCONTROL] & 0x80)
        ssOut |= RIGHT_CTRL_PRESSED;

    if (KeyState[VK_LMENU] & 0x80)
        ssOut |= LEFT_ALT_PRESSED;
    if (KeyState[VK_RMENU] & 0x80)
        ssOut |= RIGHT_ALT_PRESSED;

    /* See WM_CHAR MSDN documentation for instance */
    if (lParam & 0x01000000)
        ssOut |= ENHANCED_KEY;

    return ssOut;
}

VOID NTAPI
ConioProcessKey(PCONSRV_CONSOLE Console, MSG* msg)
{
    static BYTE KeyState[256] = { 0 };
    /* MSDN mentions that you should use the last virtual key code received
     * when putting a virtual key identity to a WM_CHAR message since multiple
     * or translated keys may be involved. */
    static UINT LastVirtualKey = 0;
    DWORD ShiftState;
    WCHAR UnicodeChar;
    UINT VirtualKeyCode;
    UINT VirtualScanCode;
    BOOL Down = FALSE;
    BOOLEAN Fake;          // synthesized, not a real event
    BOOLEAN NotChar;       // message should not be used to return a character

    if (NULL == Console)
    {
        DPRINT1("No Active Console!\n");
        return;
    }

    VirtualScanCode = HIWORD(msg->lParam) & 0xFF;
    Down = msg->message == WM_KEYDOWN || msg->message == WM_CHAR ||
           msg->message == WM_SYSKEYDOWN || msg->message == WM_SYSCHAR;

    GetKeyboardState(KeyState);
    ShiftState = ConioGetShiftState(KeyState, msg->lParam);

    if (msg->message == WM_CHAR || msg->message == WM_SYSCHAR)
    {
        VirtualKeyCode = LastVirtualKey;
        UnicodeChar = msg->wParam;
    }
    else
    {
        WCHAR Chars[2];
        INT RetChars = 0;

        VirtualKeyCode = msg->wParam;
        RetChars = ToUnicodeEx(VirtualKeyCode,
                               VirtualScanCode,
                               KeyState,
                               Chars,
                               2,
                               0,
                               NULL);
        UnicodeChar = (1 == RetChars ? Chars[0] : 0);
    }

    Fake = UnicodeChar &&
            (msg->message != WM_CHAR && msg->message != WM_SYSCHAR &&
             msg->message != WM_KEYUP && msg->message != WM_SYSKEYUP);
    NotChar = (msg->message != WM_CHAR && msg->message != WM_SYSCHAR);
    if (NotChar) LastVirtualKey = msg->wParam;

    DPRINT("CONSRV: %s %s %s %s %02x %02x '%lc' %04x\n",
           Down ? "down" : "up  ",
           (msg->message == WM_CHAR || msg->message == WM_SYSCHAR) ?
           "char" : "key ",
           Fake ? "fake" : "real",
           NotChar ? "notc" : "char",
           VirtualScanCode,
           VirtualKeyCode,
           (UnicodeChar >= L' ') ? UnicodeChar : L'.',
           ShiftState);

    if (Fake) return;

    /* Send the key press to the console driver */
    ConDrvProcessKey(Console,
                     Down,
                     VirtualKeyCode,
                     VirtualScanCode,
                     UnicodeChar,
                     ShiftState,
                     KeyState[VK_CONTROL]);
}

DWORD
ConioEffectiveCursorSize(PCONSRV_CONSOLE Console, DWORD Scale)
{
    DWORD Size = (Console->ActiveBuffer->CursorInfo.dwSize * Scale + 99) / 100;
    /* If line input in progress, perhaps adjust for insert toggle */
    if (Console->LineBuffer && !Console->LineComplete && (Console->InsertMode ? !Console->LineInsertToggle : Console->LineInsertToggle))
        return (Size * 2 <= Scale) ? (Size * 2) : (Size / 2);
    return Size;
}

/* EOF */
