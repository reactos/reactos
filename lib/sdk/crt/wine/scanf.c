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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <precomp.h>

#include <wchar.h>
#include <ctype.h>

#define NDEBUG
#include <internal/debug.h>

#define WARN DPRINT1

extern FILE _iob[];

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
    if ((c>=L'0') && (c<=L'9') && (c<=L'0'+base-1)) return (c-L'0');
    if (base<=10) return -1;
    if ((c>=L'A') && (c<=L'Z') && (c<=L'A'+base-11)) return (c-L'A'+10);
    if ((c>=L'a') && (c<=L'z') && (c<=L'a'+base-11)) return (c-L'a'+10);
    return -1;
}

/* vfscanf */
#undef WIDE_SCANF
#undef CONSOLE
#undef STRING
#include "scanf.h"

/* vfwscanf */
#define WIDE_SCANF 1
#undef CONSOLE
#undef STRING
#include "scanf.h"

/* vsscanf */
#undef WIDE_SCANF
#undef CONSOLE
#define STRING 1
#include "scanf.h"

/* vswscanf */
#define WIDE_SCANF 1
#undef CONSOLE
#define STRING 1
#include "scanf.h"

/* vcscanf */
#undef WIDE_SCANF
#define CONSOLE 1
#undef STRING
#include "scanf.h"


/*********************************************************************
 *		fscanf (MSVCRT.@)
 */
int fscanf(FILE *file, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfscanf(file, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		scanf (MSVCRT.@)
 */
int scanf(const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfscanf(stdin, format, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fwscanf (MSVCRT.@)
 */
int fwscanf(FILE *file, const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfwscanf(file, format, valist);
    va_end(valist);
    return res;
}


/*********************************************************************
 *		wscanf (MSVCRT.@)
 */
int wscanf(const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfwscanf(stdin, format, valist);
    va_end(valist);
    return res;
}


/*********************************************************************
 *		sscanf (MSVCRT.@)
 */
int crt_sscanf(const char *str, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vsscanf(str, format, valist);
    va_end(valist);
    return res;
}


/*********************************************************************
 *		swscanf (MSVCRT.@)
 */
int swscanf(const wchar_t *str, const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vswscanf(str, format, valist);
    va_end(valist);
    return res;
}


/*********************************************************************
 *		_cscanf (MSVCRT.@)
 */
int _cscanf(/*const*/ char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vcscanf(format, valist);
    va_end(valist);
    return res;
}
