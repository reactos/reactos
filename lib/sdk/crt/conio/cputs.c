/*
 * COPYRIGHT:   LGPL - See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/conio/cputs.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Aleksey Bragin
 */

#include <precomp.h>

extern FDINFO *fdesc;

/*
 * @implemented
 */
int _cputs(const char *_str)
{
#if 0
  DWORD count;
  int retval = EOF;

  LOCK_CONSOLE;
  if (WriteConsoleA(console_out, str, strlen(str), &count, NULL)
      && count == 1)
    retval = 0;
  UNLOCK_CONSOLE;
  return retval;
#else
  int len = strlen(_str);
  DWORD written = 0;
  if (!WriteFile( fdesc[stdout->_file].hFile ,_str,len,&written,NULL))
    return -1;
  return 0;
#endif
}
