/*
   windows.h

   Include this file if you wish to use the Windows32 API Library

   Copyright (C) 1996 Free Software Foundation

   Author:  Scott Christley <scottc@net-community.com>
   Date: 1996

   This file is part of the Windows32 API Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   If you are interested in a warranty or support for this source code,
   contact Scott Christley <scottc@net-community.com> for more information.

   You should have received a copy of the GNU Library General Public
   License along with this library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _GNU_H_WINDOWS_H
#define _GNU_H_WINDOWS_H

#ifdef __USE_W32API

#include_next <windows.h>

#else /* __USE_W32API */

#ifndef max
#define max(a,b)   (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX max
#endif
#ifndef MIN
#define MIN min
#endif

#ifndef RC_INVOKED
#include <limits.h>
#include <stdarg.h>
#endif

/* Base definitions */
#include <base.h>

/* WIN32 messages */
#ifndef WIN32_LEAN_AND_MEAN
#include <messages.h>
#endif

/* WIN32 definitions */
#include <defines.h>

#ifndef RC_INVOKED

/* WIN32 structures */
#include <structs.h>

/* WIN32 functions */
#ifndef WIN32_LEAN_AND_MEAN
#include <funcs.h>
#endif

/* WIN32 PE file format */
#include <pe.h>

#endif /* ! defined (RC_INVOKED) */

/* WIN32 error codes */
#ifndef WIN32_LEAN_AND_MEAN
#include <errors.h>
#endif

#ifndef RC_INVOKED

/* Windows sockets specification version 1.1 */
#ifdef Win32_Winsock
#ifndef WIN32_LEAN_AND_MEAN
#include <sockets.h>
#endif
#endif

/* There is a conflict with BOOL between Objective-C and Win32,
   so the Windows32 API Library defines and uses WINBOOL.
   However, if we are not using Objective-C then define the normal
   windows BOOL so Win32 programs compile normally.  If you are
   using Objective-C then you must use WINBOOL for Win32 operations.
*/
#ifndef __OBJC__
//typedef WINBOOL BOOL;
#endif /* !__OBJC__ */

/* How do we get the VM page size on NT? */
#ifndef vm_page_size
#define vm_page_size 4096
#endif

#endif /* ! defined (RC_INVOKED) */

#ifdef __GNUC__
#ifndef NONAMELESSUNION
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#define _ANONYMOUS_UNION __extension__
#define _ANONYMOUS_STRUCT __extension__
#else
#if defined(__cplusplus)
#define _ANONYMOUS_UNION __extension__
#endif
#endif /* __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95) */
#endif /* NONAMELESSUNION */
#else
#define _ANONYMOUS_UNION
#define _ANONYMOUS_STRUCT
#endif /* __GNUC__ */

#ifndef _ANONYMOUS_UNION
#define _ANONYMOUS_UNION
#define _UNION_NAME(x) x
#define DUMMYUNIONNAME	u
#define DUMMYUNIONNAME2	u2
#define DUMMYUNIONNAME3	u3
#define DUMMYUNIONNAME4	u4
#define DUMMYUNIONNAME5	u5
#define DUMMYUNIONNAME6	u6
#define DUMMYUNIONNAME7	u7
#define DUMMYUNIONNAME8	u8
#else
#define _UNION_NAME(x)
#define DUMMYUNIONNAME
#define DUMMYUNIONNAME2
#define DUMMYUNIONNAME3
#define DUMMYUNIONNAME4
#define DUMMYUNIONNAME5
#define DUMMYUNIONNAME6
#define DUMMYUNIONNAME7
#define DUMMYUNIONNAME8
#endif
#ifndef _ANONYMOUS_STRUCT
#define _ANONYMOUS_STRUCT
#define _STRUCT_NAME(x) x
#define DUMMYSTRUCTNAME	s
#define DUMMYSTRUCTNAME2 s2
#define DUMMYSTRUCTNAME3 s3
#else
#define _STRUCT_NAME(x)
#define DUMMYSTRUCTNAME
#define DUMMYSTRUCTNAME2
#define DUMMYSTRUCTNAME3
#endif

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif

#endif /* !__USE_W32API */

#endif /* _GNU_H_WINDOWS_H */
