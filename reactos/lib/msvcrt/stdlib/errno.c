/* $Id: errno.c,v 1.2 2000/12/03 17:58:38 ekohl Exp $
 *
 */
#include <msvcrt/internal/tls.h>

int _errno (void)
{
   return (GetThreadData()->terrno);
}


int __set_errno (int error)
{
   PTHREADDATA ThreadData;

   ThreadData = GetThreadData();
   if (ThreadData)
     ThreadData->terrno = error;

   return error;
}

/* EOF */
