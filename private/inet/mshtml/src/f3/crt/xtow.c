/***
*xtow.c - convert integers/longs to wide char string
*
*       Copyright (c) 1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       The module has code to convert integers/longs to wide char strings.
*
*******************************************************************************/


#pragma warning(disable:4201)
#pragma warning(disable:4214)
#pragma warning(disable:4244)
#pragma warning(disable:4514)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

#define INT_SIZE_LENGTH   20
#define LONG_SIZE_LENGTH  40

/***
*wchar_t *_itow, *_ltow, *_ultow(val, buf, radix) - convert binary int to wide
*       char string
*
*Purpose:
*       Converts an int to a wide character string.
*
*Entry:
*       val - number to be converted (int, long or unsigned long)
*       int radix - base to convert into
*       wchar_t *buf - ptr to buffer to place result
*
*Exit:
*       calls ASCII version to convert, converts ASCII to wide char into buf
*       returns a pointer to this buffer
*
*Exceptions:
*
*******************************************************************************/

wchar_t * __cdecl _itow (
        int val,
        wchar_t *buf,
        int radix
        )
{
   char astring[INT_SIZE_LENGTH];

   _itoa (val, astring, radix);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, astring, -1,
                        buf, INT_SIZE_LENGTH);

   return (buf);
}

wchar_t * __cdecl _ltow (
        long val,
        wchar_t *buf,
        int radix
        )
{
   char astring[LONG_SIZE_LENGTH];

   _ltoa (val, astring, radix);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, astring, -1,
                        buf, LONG_SIZE_LENGTH);

   return (buf);
}

wchar_t * __cdecl _ultow (
        unsigned long val,
        wchar_t *buf,
        int radix
        )
{
   char astring[LONG_SIZE_LENGTH];

   _ultoa (val, astring, radix);
   MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, astring, -1,
                        buf, LONG_SIZE_LENGTH);

   return (buf);
}

