#include <precomp.h>

/*********************************************************************
 *		_mbstowcs_l (MSVCRT.@)
 */
size_t CDECL _mbstowcs_l(wchar_t *wcstr, const char *mbstr,
        size_t count, _locale_t locale)
{
    MSVCRT_pthreadlocinfo locinfo;
    size_t i, size;

    if(!mbstr) {
        _set_errno(EINVAL);
        return -1;
    }

    if(!locale)
        locinfo = get_locinfo();
    else
        locinfo = ((MSVCRT__locale_t)locale)->locinfo;

    if(!locinfo->lc_codepage) {
        if(!wcstr)
            return strlen(mbstr);

        for(i=0; i<count; i++) {
            wcstr[i] = (unsigned char)mbstr[i];
            if(!wcstr[i]) break;
        }
        return i;
    }

    /* Ignore count parameter */
    if(!wcstr)
        return MultiByteToWideChar(locinfo->lc_codepage, 0, mbstr, -1, NULL, 0)-1;

    for(i=0, size=0; i<count; i++) {
        if(mbstr[size] == '\0')
            break;

        size += (_isleadbyte_l((unsigned char)mbstr[size], locale) ? 2 : 1);
    }

    if(size) {
        size = MultiByteToWideChar(locinfo->lc_codepage, 0,
                                   mbstr, size, wcstr, count);
        if(!size) {
            if(count) wcstr[0] = '\0';
            _set_errno(EILSEQ);
            return -1;
        }
    }

    if(size<count)
        wcstr[size] = '\0';

    return size;
}

/*
 * @implemented
 */
size_t mbstowcs (wchar_t *widechar, const char *multibyte, size_t number)
{
    return _mbstowcs_l(widechar, multibyte, number, NULL);
}
