/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id$
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/mkhive.h
 * PURPOSE:         Hive maker
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __MKHIVE_H__
#define __MKHIVE_H__


#define VOID void
typedef void *PVOID;
typedef char CHAR, *PCHAR;
typedef short WCHAR, *PWCHAR;
typedef unsigned char UCHAR, *PUCHAR;
typedef short SHORT, *PSHORT;
typedef unsigned short USHORT, *PUSHORT;
typedef long LONG, *PLONG;
typedef unsigned long ULONG, *PULONG;

typedef unsigned long ULONG_PTR;

typedef int BOOL, *PBOOL;

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif


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

#ifdef WIN32
#define strncasecmp strnicmp
#define strcasecmp stricmp
#endif//WIN32

#ifdef _MSC_VER
#define GCC_PACKED
#define inline
#else//_MSC_VER
#define GCC_PACKED __attribute__((packed))
#endif//_MSC_VER

#endif /* __MKHIVE_H__ */

/* EOF */
