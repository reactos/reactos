/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/conio/cputs.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <precomp.h>

/*
 * @implemented
 */
int _cputs(const char *_str)
{
  int len = strlen(_str);
  DWORD written = 0;
  if (!WriteFile( fdinfo(stdout->_file)->hFile ,_str,len,&written,NULL))
    return -1;
  return 0;
}
