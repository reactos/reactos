/* $Id: read.c,v 1.11 2003/07/11 21:57:54 royce Exp $
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/read.c
 * PURPOSE:     Reads a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/1998: Created
 *              03/05/2002: made _read() non-greedy - it now returns as soon as
 *                          any amount of data has been read. It's the expected
 *                          behavior for line-buffered streams (KJK::Hyperion)
 */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/internal/file.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

/*
 * @implemented
 */
size_t _read(int _fd, void *_buf, size_t _nbyte)
{
   DWORD _rbyte = 0, nbyte = _nbyte;
   char *bufp = (char*)_buf;
   HANDLE hfile;
   int istext, error;

   DPRINT("_read(fd %d, buf %x, nbyte %d)\n", _fd, _buf, _nbyte);

   /* null read */
   if(_nbyte == 0)
      return 0;

   hfile = _get_osfhandle(_fd);
   istext = __fileno_getmode(_fd) & O_TEXT;

   /* read data */
   if (!ReadFile(hfile, bufp, nbyte, &_rbyte, NULL))
   {
      /* failure */
      error = GetLastError();
      if (error == ERROR_BROKEN_PIPE)
      {
	 return 0;
      }
      return -1;
   }
      
   /* text mode */
   if (_rbyte && istext)
   {
      int found_cr = 0;
      int cr = 0;
      DWORD count = _rbyte;

      /* repeat for all bytes in the buffer */
      for(; count; bufp++, count--)
      {
#if 1
          /* carriage return */
          if (*bufp == '\r') {
            found_cr = 1;
            if (cr != 0) {
                *(bufp - cr) = *bufp;
            }
            continue;
          }
          if (found_cr) {
            found_cr = 0;
            if (*bufp == '\n') {
              cr++;
              *(bufp - cr) = *bufp;
            } else {
            }
          } else if (cr != 0) {
            *(bufp - cr) = *bufp;
          }
#else
         /* carriage return */
          if (*bufp == '\r') {
            cr++;
          }
         /* shift characters back, to ignore carriage returns */
          else if (cr != 0) {
            *(bufp - cr) = *bufp;
          }
#endif
      }
      if (found_cr) {
        cr++;
      }
      /* ignore the carriage returns */
      _rbyte -= cr;
   }
   DPRINT("%d\n", _rbyte);
   return _rbyte;
}
