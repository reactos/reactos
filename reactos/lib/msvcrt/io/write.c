/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crtdll/io/write.c
 * PURPOSE:     Writes to a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <crtdll/io.h>
#include <windows.h>



size_t _write(int _fd, const void *_buf, size_t _nbyte)
{
   DWORD _wbyte;
   
   if (!WriteFile(_get_osfhandle(_fd),_buf,_nbyte,&_wbyte,NULL))
   {
      return -1;
   }
   return (size_t)_wbyte;
}
