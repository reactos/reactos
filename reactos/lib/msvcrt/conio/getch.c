/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/conio/getch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <msvcrt/conio.h>
#include <msvcrt/stdio.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/console.h>


int _getch(void)
{
  DWORD  NumberOfCharsRead = 0;
  char c;

  if (char_avail)
    {
      c = ungot_char;
      char_avail = 0;
    }
  else
    {
      ReadConsoleA(_get_osfhandle(stdin->_file),
		   &c,
		   1,
		   &NumberOfCharsRead,
		   NULL);
    }
  if (c == 10)
    c = 13;
  putchar(c);

  return c;
}

#if 0
int _getche(void)
{
    int c;

    c = _getch();
    _putch(c);

    return c;
}
#endif
