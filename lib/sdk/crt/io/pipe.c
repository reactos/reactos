/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/pipe.c
 * PURPOSE:     Creates a pipe
 * PROGRAMER:   DJ Delorie
 * UPDATE HISTORY:
 *              28/12/98: Appropriated for Reactos
 */

#include <precomp.h>

#define NDEBUG
#include <internal/debug.h>


/*
 * @implemented
 */
int _pipe(int _fildes[2], unsigned int size, int mode )
{
  HANDLE hReadPipe, hWritePipe;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

  TRACE("_pipe((%i,%i), %ui, %i)", _fildes[0], _fildes[1], size, mode);

  if (mode & O_NOINHERIT)
    sa.bInheritHandle = FALSE;

  if (!CreatePipe(&hReadPipe,&hWritePipe,&sa,size)) {
		_dosmaperr(GetLastError());
      return( -1);
	}

  if ((_fildes[0] = alloc_fd(hReadPipe, split_oflags(mode))) < 0)
  {
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    __set_errno(EMFILE);
    return(-1);
  }

  if ((_fildes[1] = alloc_fd(hWritePipe, split_oflags(mode))) < 0)
  {
    free_fd(_fildes[0]);
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    __set_errno(EMFILE);
    return(-1);
  }
  return(0);
}
