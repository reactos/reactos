#include <precomp.h>

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
