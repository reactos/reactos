/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/msvcrt/time/strtime.c
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
