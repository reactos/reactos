/* $Id: errno.c,v 1.13 2004/01/11 10:58:45 hbirr Exp $
 *
 */
#ifdef __USE_W32API
#undef __USE_W32API
#endif 

#include <msvcrt/errno.h>
#include <msvcrt/internal/tls.h>
#include <msvcrt/internal/file.h>

#include "doserrmap.h"

/*
 * @implemented
 */
int* __doserrno(void)
{
  return (int*)(&GetThreadData()->tdoserrno);
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

/*
 * This function sets both doserrno to the passed in OS error code
 * and also maps this to an appropriate errno code.  The mapping
 * has been deduced automagically by running this functions, which
 * exists in MSVCRT but is undocumented, on all the error codes in
 * winerror.h.
 */
void _dosmaperr(unsigned long oserror)
{
	int pos, base, lim;
	
	__set_doserrno(oserror);
	
	/* Use binary chop to find the corresponding errno code */
	for (base=0, lim=sizeof(doserrmap)/sizeof(doserrmap[0]); lim; lim >>= 1) {
		pos = base+(lim >> 1);
		if (doserrmap[pos].winerr == oserror) {
			__set_errno(doserrmap[pos].en);
			return;
		} else if (doserrmap[pos].winerr < oserror) {
			base = pos + 1;
			--lim;
		}
	}
	/* EINVAL appears to be the default */
	__set_errno(EINVAL);
}

/* EOF */
