/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/io/write.c
 * PURPOSE:     Writes to a file
 * PROGRAMER:   Boudewijn Dekker
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <windows.h>
#include <msvcrt/io.h>
#include <msvcrt/stdlib.h>
#include <msvcrt/internal/file.h>

#define NDEBUG
#include <msvcrt/msvcrtdbg.h>

#define BUFSIZE 4096
/*
void ReportLastError(void)
{
    DWORD error = GetLastError();
    if (error != ERROR_SUCCESS) {
        PTSTR msg;
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
            0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL)) {
            printf("ReportLastError() %d - %s\n", error, msg);
        } else {
            printf("ReportLastError() %d - unknown error\n", error);
        }
        LocalFree(msg);
    }
}
 */
/*
 * @implemented
 */
size_t _write(int _fd, const void* _buf, size_t _nbyte)
{
   char *tmp, *in, *out;
   int result;
   unsigned int count;
   DWORD wbyte;

   DPRINT("_write(fd %d, buf %x, nbyte %d)\n", _fd, _buf, _nbyte);
   if (__fileno_getmode(_fd) & O_TEXT) {
      result = _nbyte; 
      tmp = (char*) malloc(BUFSIZE);
      if (tmp == NULL) {
         return -1;
      }
      count = BUFSIZE;
      out = tmp;
      in = (char*) _buf;
      while (_nbyte--) {
         if (*in == 0x0a) {
            *out++ = 0x0d;
            count--;
            if (count == 0) {
                if (!WriteFile(_get_osfhandle(_fd), tmp, BUFSIZE, &wbyte, NULL)) {
                   //ReportLastError();
                   result = -1;
                   break;
                }
                if (wbyte < BUFSIZE) {
                   result = in - (char*)_buf;
                   break;
                }
                count = BUFSIZE;
                out = tmp;
            }
         }
         *out++ = *in++;
         count--;
         if (count == 0 || _nbyte == 0) {
            int tmp_len_debug = strlen(tmp);
            if (!WriteFile(_get_osfhandle(_fd), tmp, BUFSIZE - count, &wbyte, NULL)) {
               //ReportLastError();
               result = -1; 
               tmp_len_debug = 0;
               break;
            }
            if (wbyte < (BUFSIZE - count)) {
               result = in - (char*)_buf;
               break;
            }
            count = BUFSIZE;
            out = tmp;
         }
      }
      free(tmp);
      return result;
   } else {
      if(!WriteFile(_get_osfhandle(_fd), _buf, _nbyte, &wbyte, NULL)) {
          //ReportLastError();
          return -1;
      }
      return wbyte;
   }
}
