/* $Id: errno.c,v 1.3 2001/01/18 13:23:26 jean Exp $
 *
 */
#include <msvcrt/internal/tls.h>

int *_errno (void)
{
   return (&GetThreadData()->terrno);
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
