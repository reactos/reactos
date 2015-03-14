/*
 * general implementation of scanf used by scanf, sscanf, fscanf,
 * _cscanf, wscanf, swscanf and fwscanf
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2002 Daniel Gudbjartsson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <limits.h>

#include <wine/winternl.h>
#include <wine/debug.h>

#include "winesup.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

//extern FILE _iob[];

/* helper function for *scanf.  Returns the value of character c in the
 * given base, or -1 if the given character is not a digit of the base.
 */
static int char2digit(char c, int base) {
    if ((c>='0') && (c<='9') && (c<='0'+base-1)) return (c-'0');
    if (base<=10) return -1;
    if ((c>='A') && (c<='Z') && (c<='A'+base-11)) return (c-'A'+10);
    if ((c>='a') && (c<='z') && (c<='a'+base-11)) return (c-'a'+10);
    return -1;
}

/* helper function for *wscanf.  Returns the value of character c in the
 * given base, or -1 if the given character is not a digit of the base.
 */
static int wchar2digit(wchar_t c, int base) {
    if ((c>='0') && (c<='9') && (c<='0'+base-1)) return (c-'0');
    if (base<=10) return -1;
    if ((c>='A') && (c<='Z') && (c<='A'+base-11)) return (c-'A'+10);
    if ((c>='a') && (c<='z') && (c<='a'+base-11)) return (c-'a'+10);
    return -1;
}

#ifndef _LIBCNT_
/* vfscanf_l */
#undef WIDE_SCANF
#undef CONSOLE
#undef STRING
#undef SECURE
#include "scanf.h"

/* vfwscanf_l */
#define WIDE_SCANF 1
#undef CONSOLE
#undef STRING
#undef SECURE
#include "scanf.h"
#endif /* !_LIBCNT_ */

/* vsscanf_l */
#undef WIDE_SCANF
#undef CONSOLE
#define STRING 1
#undef SECURE
#include "scanf.h"

/* vswscanf_l */
#define WIDE_SCANF 1
#undef CONSOLE
#define STRING 1
#undef SECURE
#include "scanf.h"

#ifndef _LIBCNT_
/* vcscanf_l */
#undef WIDE_SCANF
#define CONSOLE 1
#undef STRING
#undef SECURE
#include "scanf.h"


/*********************************************************************
 *		fscanf (MSVCRT.@)
 */
int CDECL fscanf(FILE *file, const char *format, ...)
{
    __ms_va_list valist;
    int res;

    __ms_va_start(valist, format);
    res = vfscanf_l(file, format, NULL, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		scanf (MSVCRT.@)
 */
int CDECL scanf(const char *format, ...)
{
    __ms_va_list valist;
    int res;

    __ms_va_start(valist, format);
    res = vfscanf_l(stdin, format, NULL, valist);
    __ms_va_end(valist);
    return res;
}

/*********************************************************************
 *		fwscanf (MSVCRT.@)
 */
int CDECL fwscanf(FILE *file, const wchar_t *format, ...)
{
    __ms_va_list valist;
    int res;

    __ms_va_start(valist, format);
    res = vfwscanf_l(file, format, NULL, valist);
    __ms_va_end(valist);
    return res;
}


/*********************************************************************
 *		wscanf (MSVCRT.@)
 */
int CDECL wscanf(const wchar_t *format, ...)
{
    __ms_va_list valist;
    int res;

    __ms_va_start(valist, format);
    res = vfwscanf_l(stdin, format, NULL, valist);
    __ms_va_end(valist);
    return res;
}
#endif /* !_LIBCNT_ */


/*********************************************************************
 *		sscanf (MSVCRT.@)
 */
int CDECL sscanf(const char *str, const char *format, ...)
{
    __ms_va_list valist;
    int res;

    __ms_va_start(valist, format);
    res = vsscanf_l(str, format, NULL, valist);
    __ms_va_end(valist);
    return res;
}


/*********************************************************************
 *		swscanf (MSVCRT.@)
 */
int CDECL swscanf(const wchar_t *str, const wchar_t *format, ...)
{
    __ms_va_list valist;
    int res;

    __ms_va_start(valist, format);
    res = vswscanf_l(str, format, NULL, valist);
    __ms_va_end(valist);
    return res;
}

#ifndef _LIBCNT_
/*********************************************************************
 *		_cscanf (MSVCRT.@)
 */
int CDECL _cscanf(const char *format, ...)
{
    __ms_va_list valist;
    int res;

    __ms_va_start(valist, format);
    res = vcscanf_l(format, NULL, valist);
    __ms_va_end(valist);
    return res;
}
#endif
