#include <msvcrt/stdio.h>
#include <msvcrt/io.h>
#include <msvcrt/errno.h>
#include <msvcrt/sys/utime.h>
#include <msvcrt/internal/file.h>


/*
 * @implemented
 */
int _utime(const char* filename, struct _utimbuf* buf)
{
    int fn;
    int ret;
  
    fn = _open(filename, _O_RDWR);
    if (fn == -1) {
        __set_errno(EBADF);
        return -1;
    }
    ret = _futime(fn, buf);
    if (_close(fn) < 0)
        return -1;
    return ret;
}
