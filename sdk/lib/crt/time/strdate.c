/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/strdate.c
 * PURPOSE:     Fills a buffer with a formatted date representation
 * PROGRAMER:   Ariadne
 * UPDATE HISTORY:
 *              28/12/98: Created
 */
#include <precomp.h>

/*
 * @implemented
 */
char* _strdate(char* date)
{
   static const char format[] = "MM'/'dd'/'yy";

   GetDateFormatA(LOCALE_NEUTRAL, 0, NULL, format, date, 9);

   return date;

}

/*
 * @implemented
 */
int CDECL _strdate_s(char* date, size_t size)
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

    _strdate(date);
    return 0;
}
