#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#include <msvcrt/internal/file.h>


int _commit(int _fd)
{
   if (! FlushFileBuffers(_get_osfhandle(_fd)) ) {
	__set_errno(EBADF);
	return -1;
     }

   return  0;
}
