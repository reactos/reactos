/*
	windows.h - main header file for the Win32 API

	Written by Anders Norlander <anorland@hem2.passagen.se>

	This file is part of a free library for the Win32 API.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

*/
#ifndef _WINDOWS_H
#define _WINDOWS_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

/* translate GCC target defines to MS equivalents. Keep this synchronized
   with winnt.h. */
#if defined(__i686__) && !defined(_M_IX86)
#define _M_IX86 600
#elif defined(__i586__) && !defined(_M_IX86)
#define _M_IX86 500
#elif defined(__i486__) && !defined(_M_IX86)
#define _M_IX86 400
#elif defined(__i386__) && !defined(_M_IX86)
#define _M_IX86 300
#endif
#if defined(_M_IX86) && !defined(_X86_)
#define _X86_
#elif defined(_M_ALPHA) && !defined(_ALPHA_)
#define _ALPHA_
#elif defined(_M_PPC) && !defined(_PPC_)
#define _PPC_
#elif defined(_M_MRX000) && !defined(_MIPS_)
#define _MIPS_
#elif defined(_M_M68K) && !defined(_68K_)
#define _68K_
#endif

#ifdef RC_INVOKED
/* winresrc.h includes the necessary headers */
#include <winresrc.h>
#else

#include <stdarg.h>
#include <windef.h>
#include <wincon.h>
#include <winbase.h>
#if !(defined NOGDI || defined  _WINGDI_H)
#include <wingdi.h>
#endif
#ifndef _WINUSER_H
#include <winuser.h>
#endif
#ifndef _WINNLS_H
#include <winnls.h>
#endif
#ifndef _WINVER_H
#include <winver.h>
#endif
#ifndef _WINNETWK_H
#include <winnetwk.h>
#endif
#ifndef _WINREG_H
#include <winreg.h>
#endif
#ifndef _WINSVC_H
#include <winsvc.h>
#endif

#ifndef WIN32_LEAN_AND_MEAN
#include <cderr.h>
#include <dde.h>
#include <ddeml.h>
#include <dlgs.h>
#include <imm.h>
#include <lzexpand.h>
#include <mmsystem.h>
#include <nb30.h>
#include <rpc.h>
#include <shellapi.h>
#include <winperf.h>
#ifndef NOGDI
#include <commdlg.h>
#include <winspool.h>
#endif
#if defined(Win32_Winsock)
#warning "The  Win32_Winsock macro name is deprecated.\
    Please use __USE_W32_SOCKETS instead"
#ifndef __USE_W32_SOCKETS
#define __USE_W32_SOCKETS
#endif
#endif
#if defined(__USE_W32_SOCKETS) || !(defined(__CYGWIN__) || defined(__MSYS__) || defined(_UWIN))
#if (_WIN32_WINNT >= 0x0400)
#include <winsock2.h>
/*
 * MS likes to include mswsock.h here as well,
 * but that can cause undefined symbols if
 * winsock2.h is included before windows.h
 */
#else
#include <winsock.h>
#endif /*  (_WIN32_WINNT >= 0x0400) */
#endif
#ifndef NOGDI
#if !defined (__OBJC__)
#if (__GNUC__ >= 3) || defined (__WATCOMC__)
#include <ole2.h>
#endif
#endif /* __OBJC__ */
#endif

#endif /* WIN32_LEAN_AND_MEAN */

#endif /* RC_INVOKED */

#ifdef __OBJC__
/* FIXME: Not undefining BOOL here causes all BOOLs to be WINBOOL (int),
   but undefining it causes trouble as well if a file is included after
   windows.h
*/
#undef BOOL
#endif

#endif
