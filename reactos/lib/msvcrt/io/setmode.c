/* $Id: setmode.c,v 1.5 2002/09/07 15:12:32 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/setmode.c
 * PURPOSE:     Sets the file translation mode
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <msvcrti.h>

#define NDEBUG
#include <msvcrtdbg.h>


int _setmode(int _fd, int _newmode)
{
  DPRINT("_setmod(fd %d, newmode %x)\n", _fd, _newmode);
  return __fileno_setmode(_fd, _newmode);
}
