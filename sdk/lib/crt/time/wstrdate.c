/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/wstrdate.c
 * PURPOSE:     Fills a buffer with a formatted date representation
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <precomp.h>

/*
 * @implemented
 */
wchar_t* _wstrdate(wchar_t* date)
{
   static const WCHAR format[] = { 'M','M','\'','/','\'','d','d','\'','/','\'','y','y',0 };

   GetDateFormatW(LOCALE_NEUTRAL, 0, NULL, format, (LPWSTR)date, 9);

   return date;

}

int CDECL _wstrdate_s(wchar_t* date, size_t size)
{
    if(date && size)
        date[0] = '\0';

    if(!date) {
        *_errno() = EINVAL;
        return EINVAL;
    }

    if(size < 9) {
        *_errno() = ERANGE;
        return ERANGE;
    }

    _wstrdate(date);
    return 0;
}
