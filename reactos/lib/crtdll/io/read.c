/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/read.c
 * PURPOSE:     Reads a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <crtdll/io.h>
#include <windows.h>

size_t	_read(int _fd, void *_buf, size_t _nbyte)
{
   size_t _rbyte;
   
   printf("_read(fd %d, buf %x, _nbyte %d)\n",_fd,_buf,_nbyte);
   
   if (!ReadFile(_get_osfhandle(_fd),_buf,_nbyte,&_rbyte,NULL)) 
     {
	printf("_read() = %d\n",-1);
	return -1;
     }
   printf("_read() = %d\n",_rbyte);
   return _rbyte;
}
