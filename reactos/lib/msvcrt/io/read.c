/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/read.c
 * PURPOSE:     Reads a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

size_t _read(int _fd, void *_buf, size_t _nbyte)
{
   DWORD _rbyte = 0, nbyte = _nbyte, count;
   int cr;
   char *bufp = (char*)_buf;

   DPRINT("_read(fd %d, buf %x, nbyte %d)\n", _fd, _buf, _nbyte);

   while (nbyte)
   {
      if (!ReadFile(_get_osfhandle(_fd), bufp, nbyte, &_rbyte, NULL))
      {
         return -1;
      }
      if (_rbyte == 0)
         break;
      if (__fileno_getmode(_fd) & O_TEXT)
      {
	 cr = 0;
	 count = _rbyte;
         while (count)
	 {
            if (*bufp == '\r')
	       cr++;
	    else if (cr != 0)
	           *(bufp - cr) = *bufp;
            bufp++;
	    count--;
	 }
	 _rbyte -= cr;
         bufp -= cr;
      }
      nbyte -= _rbyte;
   }
   DPRINT("%d\n", _nbyte - nbyte);
   return _nbyte - nbyte;
}
