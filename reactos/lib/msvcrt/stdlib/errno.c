/* $Id: errno.c,v 1.7 2002/09/07 15:12:36 chorns Exp $
 *
 */
#include <msvcrti.h>


int* __doserrno(void)
{
  return((int *)&GetThreadData()->tdoserrno);
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
