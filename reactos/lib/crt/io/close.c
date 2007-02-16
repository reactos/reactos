#include <precomp.h>

#define NDEBUG
#include <internal/debug.h>

/*
 * @implemented
 */
int _close(int _fd)
{
   TRACE("_close(%i)", _fd);

   if (_fd == -1)
      return(-1);
   if (CloseHandle((HANDLE)_get_osfhandle(_fd)) == FALSE)
      return(-1);
  //return
   free_fd(_fd);
   return(0);
}
