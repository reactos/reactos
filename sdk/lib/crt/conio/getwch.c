/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/conio/getch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Ariadne
 *              Lee Schroeder
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <precomp.h>

wint_t _getwch(void)
{
    DWORD NumberOfCharsRead = 0;
    wchar_t c;
    HANDLE ConsoleHandle;
    BOOL RestoreMode;
    DWORD ConsoleMode;
	INPUT_RECORD InputRecord;

    if (char_avail) {
        c = ungot_char;
        char_avail = 0;
    } else {
        /*
         * _getch() is documented to NOT echo characters. Testing shows it
         * doesn't wait for a CR either. So we need to switch off
         * ENABLE_ECHO_INPUT and ENABLE_LINE_INPUT if they're currently
         * switched on.
         */
        c = EOF;
        ConsoleHandle = GetStdHandle(STD_INPUT_HANDLE);
        RestoreMode = GetConsoleMode(ConsoleHandle, &ConsoleMode) &&
                      (0 != (ConsoleMode &
                             (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT)));
        if (RestoreMode) {
            SetConsoleMode(ConsoleHandle,
                           ConsoleMode & (~ (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT)));
        }
        for ( ;; )
        {
            if( !ReadConsoleInput(ConsoleHandle,
                     &InputRecord,
                     1,
                     &NumberOfCharsRead) || !NumberOfCharsRead)
            {
                break;
            }
            if (InputRecord.EventType == KEY_EVENT &&
                InputRecord.Event.KeyEvent.bKeyDown)
            {
                if((c = InputRecord.Event.KeyEvent.uChar.UnicodeChar) != 0)
                    break;

                if (InputRecord.Event.KeyEvent.wVirtualScanCode == 0x1d || /* Ctrl */
                    InputRecord.Event.KeyEvent.wVirtualScanCode == 0x2a || /* Left Shift */
                    InputRecord.Event.KeyEvent.wVirtualScanCode == 0x36 || /* Right Shift */
                    InputRecord.Event.KeyEvent.wVirtualScanCode == 0x38 || /* Alt */
                    InputRecord.Event.KeyEvent.wVirtualScanCode == 0x3a || /* Caps Lock */
                    InputRecord.Event.KeyEvent.wVirtualScanCode == 0x45 || /* Num Lock */
                    InputRecord.Event.KeyEvent.wVirtualScanCode == 0x46) /* Scroll Lock */
                    continue;

                if (InputRecord.Event.KeyEvent.wVirtualKeyCode >= 0x70 && /* F1-F10 */
                    InputRecord.Event.KeyEvent.wVirtualKeyCode <= 0x79)
                {
                    c = 0;
                    if(InputRecord.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x19 + 20;
                    else if(InputRecord.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x19 + 10;
                    else if(InputRecord.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x19;
                    else
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode;
                    char_avail = 1;
                }
                else if (InputRecord.Event.KeyEvent.wVirtualKeyCode == 0x7a || /* F11-F12 */
                         InputRecord.Event.KeyEvent.wVirtualKeyCode == 0x7b)
                {
                    c = 0xe0;
                    if(InputRecord.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x2E + 6;
                    else if(InputRecord.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x2E + 4;
                    else if(InputRecord.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x2E + 2;
                    else
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x2E;
                    char_avail = 1;
                }
                else
                {
                    if(InputRecord.Event.KeyEvent.dwControlKeyState & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
                    {
                        c = 0;
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x50;
                    }
                    else if(InputRecord.Event.KeyEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
                    {
                        c = 0xe0;
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode + 0x50;
                    }
                    else
                    {
                        c = 0xe0;
                        ungot_char = InputRecord.Event.KeyEvent.wVirtualScanCode;
                    }
                    char_avail = 1;
                }
                break;
            }
        }
        if (RestoreMode) {
            SetConsoleMode(ConsoleHandle, ConsoleMode);
        }
    }
    if (c == 10)
        c = 13;
    return c;
}