/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/crt/??????
 * PURPOSE:     Unknown
 * PROGRAMER:   Unknown
 * UPDATE HISTORY:
 *              25/11/05: Created
 */

#include <precomp.h>
#include <sys/utime.h>

/*
 * @implemented
 */
int _wutime(const wchar_t* filename, struct _utimbuf* buf)
{
    int fn;
    int ret;

    fn = _wopen(filename, _O_RDWR);
    if (fn == -1) {
        __set_errno(EBADF);
        return -1;
    }
    ret = _futime(fn, buf);
    if (_close(fn) < 0)
        return -1;
    return ret;
}

