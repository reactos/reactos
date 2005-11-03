#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <sys/utime.h>
#include <internal/file.h>


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
