/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/getch.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrt/stdio.h>

#undef putc
#undef putchar

int putchar(int c)
{
  int r = putc(c, stdout);
  if (stdout->_flag & _IOLBF)
     fflush(stdout);
  return r;
}

wint_t putwchar(wint_t c)
{
  wint_t r = putwc(c, stdout);
  if (stdout->_flag & _IOLBF)
     fflush(stdout);
  return r;
}
