#include <precomp.h>

#include <locale.h>
#include <internal/wine/msvcrt.h>

size_t CDECL _strxfrm_l( char *dest, const char *src,
        size_t len, _locale_t locale )
{
    MSVCRT_pthreadlocinfo locinfo;
    int ret;

    if(!MSVCRT_CHECK_PMT(src)) return INT_MAX;
    if(!MSVCRT_CHECK_PMT(dest || !len)) return INT_MAX;

    if(len > INT_MAX) {
        FIXME("len > INT_MAX not supported\n");
        len = INT_MAX;
    }

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = ((MSVCRT__locale_t)locale)->locinfo;

    if(!locinfo->lc_handle[MSVCRT_LC_COLLATE]) {
        strncpy(dest, src, len);
        return strlen(src);
    }

    ret = LCMapStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE],
            LCMAP_SORTKEY, src, -1, NULL, 0);
    if(!ret) {
        if(len) dest[0] = 0;
        *_errno() = EILSEQ;
        return INT_MAX;
    }
    if(!len) return ret-1;

    if(ret > len) {
        dest[0] = 0;
        *_errno() = ERANGE;
        return ret-1;
    }

    return LCMapStringA(locinfo->lc_handle[MSVCRT_LC_COLLATE],
            LCMAP_SORTKEY, src, -1, dest, (int)len) - 1;
}

/*********************************************************************
 *		strxfrm (MSVCRT.@)
 */
size_t CDECL strxfrm( char *dest, const char *src, size_t len )
{
    return _strxfrm_l(dest, src, len, NULL);
}
