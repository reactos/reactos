/* $Id: setmode.c,v 1.8 2002/11/24 18:42:22 robd Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/setmode.c
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
    return __fileno_setmode(_fd, _newmode);
}
