/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/stdio/putchar.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <stdio.h>
#include <tchar.h>
#include <internal/file.h>

#undef putc
#undef putchar
#undef putwc
#undef putwchar

/*
 * @implemented
 */
_TINT _puttchar(_TINT c)
{
  _TINT r = _puttc(c, stdout);
  if (stdout->_flag & _IO_LBF)
     fflush(stdout);
  return r;
}

