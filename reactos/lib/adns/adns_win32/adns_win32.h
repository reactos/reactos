/*
 *
 *  This file is
 *    Copyright (C) 2000, 2002 Jarle (jgaa) Aase <jgaa@jgaa.com>
 *
 *  It is part of adns, which is
 *    Copyright (C) 1997-2000 Ian Jackson <ian@davenant.greenend.org.uk>
 *    Copyright (C) 1999 Tony Finch <dot@dotat.at>
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * 
 *  For the benefit of certain LGPL'd `omnibus' software which provides
 *  a uniform interface to various things including adns, I make the
 *  following additional licence.  I do this because the GPL would
 *  otherwise force either the omnibus software to be GPL'd or for the
 *  adns-using part to be distributed separately.
 *  
 *  So, you may also redistribute and/or modify adns.h (but only the
 *  public header file adns.h and not any other part of adns) under the
 *  terms of the GNU Library General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *  
 *  Note that adns itself is GPL'd.  Authors of adns-using applications
 *  with GPL-incompatible licences, and people who distribute adns with
 *  applications where the whole distribution is not GPL'd, are still
 *  likely to be in violation of the GPL.  Anyone who wants to do this
 *  should contact Ian Jackson.  Please note that to avoid encouraging
 *  people to infringe the GPL as it applies the body of adns, Ian thinks
 *  that if you take advantage of the special exception to redistribute
 *  just adns.h under the LGPL, you should retain this paragraph in its
 *  place in the appropriate copyright statements.
 *
 *
 *  You should have received a copy of the GNU General Public License,
 *  or the GNU Library General Public License, as appropriate, along
 *  with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef ADNS_WIN32_H_INCLUDED
#define ADNS_WIN32_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if defined(ADNS_DLL)
# ifdef ADNS_DLL_EXPORTS
#   define ADNS_API __declspec(dllexport)
# else
#   define ADNS_API __declspec(dllimport)
# endif /* ADNS_EXPORTS */
#else
# define ADNS_API
#endif /* ADNS_DLL */

#if defined (_MSC_VER)
#pragma warning(disable:4003)
#endif /* _MSC_VER */

/* ---------------- START OF C HEADER -------------- */

#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <malloc.h>
#include <signal.h>

#define HAVE_WINSOCK 1
//#define PRINTFFORMAT(si,tc)
#define inline __inline

#define ADNS_SOCKET SOCKET
#define adns_socket_close(sck) closesocket(sck)
#define adns_socket_read(sck, data, len) recv(sck, (char *)data, len, 0)
#define adns_socket_write(sck, data, len) send(sck, (char *)data, len, 0)

/* Win32 does not set errno on Winsock errors(!) 
 * We have to map the winsock errors to errno manually
 * in order to support the original UNIX error hadnlig
 */
#define ADNS_CAPTURE_ERRNO {errno = WSAGetLastError(); WSASetLastError(errno);}
#define ADNS_CLEAR_ERRNO {WSASetLastError(errno = 0);}

#define ENOBUFS WSAENOBUFS 
#define EWOULDBLOCK WSAEWOULDBLOCK
#define EINPROGRESS WSAEINPROGRESS
#define EMSGSIZE WSAEMSGSIZE
#define ENOPROTOOPT WSAENOPROTOOPT
//#define NONRETURNING
//#define NONRETURNPRINTFFORMAT(si,tc) 

 /*
  * UNIX system API for Win32
  * The following is a quick and dirty implementation of
  * some UNIX API calls in Win32. 
  * They are used in the dll, but if your project have
  * it's own implementation of these system calls, simply
  * undefine ADNS_MAP_UNIXAPI.
  */

struct iovec 
{
    char  *iov_base;
    int  iov_len; 
};

struct timezone; /* XXX arty */

/* 
 * Undef ADNS_MAP_UNIXAPI in the calling code to use natve calls 
 */
ADNS_API int adns_gettimeofday(struct timeval *tv, struct timezone *tz);
ADNS_API int adns_writev (int FileDescriptor, const struct iovec * iov, int iovCount);
ADNS_API int adns_inet_aton(const char *cp, struct in_addr *inp);
ADNS_API int adns_getpid();

#ifdef ADNS_DLL
 ADNS_API void *adns_malloc(const size_t bytes);
 ADNS_API void *adns_realloc(void *ptr, const size_t bytes);
 ADNS_API void adns_free(void *ptr);
#endif

#define gettimeofday(tv, tz) adns_gettimeofday(tv, tz)
#define writev(FileDescriptor, iov, iovCount) adns_writev(FileDescriptor, iov, iovCount)
#define inet_aton(ap, inp) adns_inet_aton(ap, inp)
#define getpid() adns_getpid()

#ifdef ADNS_DLL
# define malloc(bytes) adns_malloc(bytes)
# define realloc(ptr, bytes) adns_realloc(ptr, bytes)
# define free(ptr) adns_free(ptr)
#endif

/* ---------------- END OF C HEADER -------------- */
#ifdef __cplusplus 
}
#endif /* __cplusplus */

#include "timercmp.h" /* arty added: mingw headers don't seem to have it */

#endif /* ADNS_WIN32_H_INCLUDED */

