/* $Id: errno.c,v 1.9 2003/07/11 21:58:09 royce Exp $
 *
 */

#include <msvcrt/errno.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/internal/file.h>

/*
 * @implemented
 */
int* __doserrno(void)
{
  return(&GetThreadData()->tdoserrno);
}

/*
 * @implemented
 */
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
