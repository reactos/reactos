/* $Id: pipe.c,v 1.5 2002/11/18 03:19:42 robd Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/pipe.c
 * PURPOSE:     Creates a pipe
 * PROGRAMER:   DJ Delorie
 * UPDATE HISTORY:
 *              28/12/98: Appropriated for Reactos
 */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


int _pipe(int _fildes[2], unsigned int size, int mode )
{
  HANDLE hReadPipe, hWritePipe;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

  if (mode & O_NOINHERIT)
    sa.bInheritHandle = FALSE;

  if (!CreatePipe(&hReadPipe,&hWritePipe,&sa,size))
    return -1;

  if ((_fildes[0] = __fileno_alloc(hReadPipe, mode)) < 0)
  {
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    __set_errno(EMFILE);
    return -1;
  }

  if ((_fildes[1] = __fileno_alloc(hWritePipe, mode)) < 0)
  {
    __fileno_close(_fildes[0]);
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    __set_errno(EMFILE);
    return -1;
  }
  return 0;
}
