/*
 *  ReactOS kernel
 *  Copyright (C) 2003, 2006 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/mkhive.h
 * PURPOSE:         Hive maker
 * PROGRAMMER:      Eric Kohl
 *                  Hervé Poussineau
 */

#ifndef __MKHIVE_H__
#define __MKHIVE_H__

#include <stdio.h>
#include <stdlib.h>

/* We have to do this because psdk/windef.h will _always_ define _WIN32... */
#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_HOST
#endif

#define NTOS_MODE_USER
#define WIN32_NO_STATUS

#ifdef MKHIVE_HOST
#include <typedefs64.h>
#endif

#include <ntddk.h>
#include <cmlib.h>
#include <infhost.h>
#include "reginf.h"
#include "cmi.h"
#include "registry.h"
#include "binhive.h"

#define HIVE_NO_FILE 2
#define VERIFY_REGISTRY_HIVE(hive)
extern LIST_ENTRY CmiHiveListHead;
#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))

#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif


/* Debugging macros */

#ifdef _MSC_VER
#include <stdio.h>
#include <stdarg.h>
static void DPRINT1(const char* fmt, ... )
{
	va_list args;
	va_start ( args, fmt );
	vprintf ( fmt, args );
	va_end ( args );
}
static void DPRINT ( const char* fmt, ... )
{
}
#else
#define DPRINT1(args...) do { printf("(%s:%d) ",__FILE__,__LINE__); printf(args); } while(0);
#define DPRINT(args...)
#endif//_MSC_VER
#define CHECKPOINT1 do { printf("%s:%d\n",__FILE__,__LINE__); } while(0);

#define CHECKPOINT

#ifdef WINDOWS_HOST
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#else
#include <string.h>
#endif//_WIN32

#ifdef _MSC_VER
#define GCC_PACKED
#define inline
#else//_MSC_VER
#define GCC_PACKED __attribute__((packed))
#endif//_MSC_VER

/* rtl.c */
PWSTR
xwcschr(
   PWSTR String,
   WCHAR Char
);

#endif /* __MKHIVE_H__ */

/* EOF */
