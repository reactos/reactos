/* $Id: errno.c,v 1.8 2002/09/08 10:22:57 chorns Exp $
 *
 */

#include <msvcrt/errno.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/internal/file.h>

int* __doserrno(void)
{
  return(&GetThreadData()->tdoserrno);
}

int *_errno(void)
{
  return(&GetThreadData()->terrno);
}


int __set_doserrno(int error)
{
  PTHREADDATA ThreadData;

  ThreadData = GetThreadData();
  if (ThreadData)
    ThreadData->tdoserrno = error;

  return(error);
}

int __set_errno(int error)
{
  PTHREADDATA ThreadData;

  ThreadData = GetThreadData();
  if (ThreadData)
    ThreadData->terrno = error;

  return(error);
}

/* EOF */
