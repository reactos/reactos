/* $Id: errno.c,v 1.4 2001/10/03 02:15:34 ekohl Exp $
 *
 */

#include <msvcrt/errno.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/internal/file.h>

int *__doserrno(void)
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
