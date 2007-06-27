#include <precomp.h>
#include <tchar.h>

/*
 * @implemented
 */
_TCHAR* _ttempnam(const _TCHAR* dir,const _TCHAR* prefix)
{
    _TCHAR* TempFileName = malloc(MAX_PATH*sizeof(_TCHAR));
    _TCHAR* d;

    if (dir == NULL)
        d = _tgetenv(_T("TMP"));
    else
        d = (_TCHAR*)dir;

#ifdef _MSVCRT_LIB_    // TODO: check on difference?
    if (GetTempFileName(d, prefix, 1, TempFileName) == 0) {
#else// TODO: FIXME: review which is correct
    if (GetTempFileName(d, prefix, 0, TempFileName) == 0) {
#endif /*_MSVCRT_LIB_*/

        free(TempFileName);
        return NULL;
    }

    return TempFileName;
}
