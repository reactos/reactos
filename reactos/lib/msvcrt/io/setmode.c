/* $Id: setmode.c,v 1.9 2003/07/11 21:57:54 royce Exp $
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


/*
 * @implemented
 */
int _setmode(int _fd, int _newmode)
{
    DPRINT("_setmod(fd %d, newmode %x)\n", _fd, _newmode);
    return __fileno_setmode(_fd, _newmode);
}
