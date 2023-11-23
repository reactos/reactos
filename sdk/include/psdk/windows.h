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

#ifdef __GNUC__
#include <msvctarget.h>
#endif

#if !defined(_X86_) && !defined(_AMD64_) && !defined(_IA64_) && !defined(_ALPHA_) && \
    !defined(_ARM_) && !defined(_ARM64_) && !defined(_PPC_) && !defined(_MIPS_) && !defined(_68K_) && !defined(_SH_)

#if defined(_M_AMD64) || defined(__x86_64__)
#define _AMD64_
#elif defined(_M_IX86) || defined(__i386__)
#define _X86_
#elif defined(_M_IA64) || defined(__ia64__)
#define _IA64_
#elif defined(_M_ALPHA) || defined(__alpha__)
#define _ALPHA_
#elif defined(_M_ARM) || defined(__arm__)
#define _ARM_
#elif defined(_M_ARM64) || defined(__arm64__) || defined(__aarch64__)
#define _ARM64_
#elif defined(_M_PPC) || defined(__powerpc__)
#define _PPC_
#elif defined(_M_MRX000) || defined(__mips__)
#define _MIPS_
#elif defined(_M_M68K) || defined(__68k__)
#define _68K_
#elif defined(_SHX_) || defined(__sh__)
#define _SH_
#endif

#endif

#ifdef RC_INVOKED
/* winresrc.h includes the necessary headers */
#include <winresrc.h>
#else

#include <excpt.h>
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winuser.h>
#include <wincon.h>
#include <winnls.h>
#include <winver.h>
#include <winnetwk.h>
#include <winreg.h>
#include <winsvc.h>

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
#ifndef NOCRYPT
#include <wincrypt.h>
#include <winefs.h>
#include <winscard.h>
#endif
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
#if (defined(__GNUC__) && (__GNUC__ >= 3)) || defined (__WATCOMC__) || defined(_MSC_VER)
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
