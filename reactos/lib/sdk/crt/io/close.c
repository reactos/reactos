#include <precomp.h>


/*
 * @implemented
 */
int _close(int _fd)
{
   TRACE("_close(%i)", _fd);

   if (_fd == -1)
      return(-1);
   if (!CloseHandle((HANDLE)_get_osfhandle(_fd)))
  {
    WARN(":failed-last error (%d)\n",GetLastError());
    _dosmaperr(GetLastError());
    return -1;
  }
   free_fd(_fd);
   return(0);
}
