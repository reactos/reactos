#include <msvcrti.h>


#define _IOCOMMIT 0x008000

int _commode = _IOCOMMIT;


int *__p__commode(void)
{
   return &_commode;
}

int _commit(int _fd)
{
   if (! FlushFileBuffers((HANDLE)_get_osfhandle(_fd)) )
     {
	__set_errno(EBADF);
	return -1;
     }

   return  0;
}
