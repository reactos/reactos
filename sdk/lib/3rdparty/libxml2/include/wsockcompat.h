/* include/wsockcompat.h
 * Windows -> Berkeley Sockets compatibility things.
 */

#if !defined __XML_WSOCKCOMPAT_H__
#define __XML_WSOCKCOMPAT_H__

#ifdef _WIN32_WCE
#include <winsock.h>
#else
#include <errno.h>
#include <winsock2.h>

/* the following is a workaround a problem for 'inline' keyword in said
   header when compiled with Borland C++ 6 */
#if defined(__BORLANDC__) && !defined(__cplusplus)
#define inline __inline
#define _inline __inline
#endif

#include <ws2tcpip.h>

/* Check if ws2tcpip.h is a recent version which provides getaddrinfo() */
#if defined(GetAddrInfo)
#include <wspiapi.h>
#define HAVE_GETADDRINFO
#endif
#endif

#undef XML_SOCKLEN_T
#define XML_SOCKLEN_T int

#ifndef ECONNRESET
#define ECONNRESET WSAECONNRESET
#endif
#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif
#ifndef EINTR
#define EINTR WSAEINTR
#endif
#ifndef ESHUTDOWN
#define ESHUTDOWN WSAESHUTDOWN
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#endif /* __XML_WSOCKCOMPAT_H__ */
