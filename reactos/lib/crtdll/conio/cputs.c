/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/conio/cputs.c
 * PURPOSE:     Writes a character to stdout
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <crtdll/conio.h>
#include <crtdll/string.h>
#include <crtdll/stdio.h>
#include <crtdll/internal/file.h>

int     _cputs(const char *_str)
{
  int len = strlen(_str);
  DWORD written = 0;
  if (!WriteFile(filehnd(stdout->_file),_str,len,&written,NULL)) 
    return -1;
  return 0;
}
