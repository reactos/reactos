#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
_TCHAR* _ttmpnam(_TCHAR* s)
{
    _TCHAR PathName[MAX_PATH];
    static _TCHAR static_buf[MAX_PATH];

    GetTempPath(MAX_PATH, PathName);
    GetTempFileName(PathName, _T("ARI"), 007, static_buf);
    _tcscpy(s,static_buf);

    return s;
}
