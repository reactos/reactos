/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/conio/getch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <precomp.h>

/*
 * @implemented
 */
int _getch(void)
{
    DWORD NumberOfCharsRead = 0;
    char c;
    HANDLE ConsoleHandle;
    BOOL RestoreMode;
    DWORD ConsoleMode;

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
        ConsoleHandle = (HANDLE) _get_osfhandle(stdin->_file);
        RestoreMode = GetConsoleMode(ConsoleHandle, &ConsoleMode) &&
                      (0 != (ConsoleMode &
                             (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT)));
        if (RestoreMode) {
            SetConsoleMode(ConsoleHandle,
                           ConsoleMode & (~ (ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT)));
        }
        ReadConsoleA((HANDLE)_get_osfhandle(stdin->_file),
		             &c,
		             1,
		             &NumberOfCharsRead,
		             NULL);
        if (RestoreMode) {
            SetConsoleMode(ConsoleHandle, ConsoleMode);
        }
    }
    if (c == 10)
        c = 13;
    return c;
}

