#include "precomp.h"
#include <io.h>
#include <errno.h>
#include <internal/file.h>


/*
 * @implemented
 */
int _commit(int _fd)
{
   if (! FlushFileBuffers((HANDLE)_get_osfhandle(_fd)) ) {
	__set_errno(EBADF);
	return -1;
     }

   return  0;
}
