/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/setmode.c
 * PURPOSE:     Sets the file translation mode
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */

#include <msvcrt/io.h>
#include <msvcrt/stdio.h>
#include <msvcrt/internal/file.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>


int _setmode(int _fd, int _newmode)
{
  DPRINT("_setmod(fd %d, newmode %x)\n", _fd, _newmode);
  if (_fd >= 0 && _fd < 5)
	  return _O_TEXT;
  return __fileno_setmode(_fd, _newmode);
}
