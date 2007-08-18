/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef PCH_H
#define PCH_H

#ifdef _MSC_VER
#pragma warning ( disable : 4786 ) // identifier was truncated to '255' characters in the debug information
#endif//_MSC_VER

#include <string>
#include <vector>
#include <map>
#include <set>

#ifdef WIN32
#include <windows.h>
#ifndef MAX_PATH
#define MAX_PATH _MAX_PATH
#endif//MAX_PATH
#else
#include <unistd.h>

typedef char CHAR, *PCHAR;
typedef unsigned char UCHAR, *PUCHAR;
typedef void VOID, *PVOID;
typedef UCHAR BOOLEAN, *PBOOLEAN;
#include <typedefs64.h>
typedef LONG *PLONG;

#endif//WIN32

#include <stdarg.h>


#ifndef WIN32
#include <string.h>
#include <ctype.h>
#include <wctype.h>
#include <math.h>

inline char * strlwr(char *x)
{
        char  *y=x;

        while (*y) {
                *y=tolower(*y);
                y++;
        }
        return x;
}
              
inline char *strupr(char *x)
{
        char  *y=x;

        while (*y) {
                *y=toupper(*y);
                y++;
        }
        return x;
}

#define _finite __finite
#define _isnan __isnan
#define stricmp strcasecmp
#define MAX_PATH PATH_MAX 
#define _MAX_PATH PATH_MAX
#endif

#endif//PCH_H
