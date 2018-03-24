
#ifndef _WININET_PRECOMP_H_
#define _WININET_PRECOMP_H_

#include <wine/config.h>

#include <assert.h>
#include <stdio.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <wininet.h>
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_GDI
#include <shlwapi.h>

#include <wine/debug.h>

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <sys/types.h>
# include <netinet/in.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#else
#define closesocket close
#define ioctlsocket ioctl
#endif /* __MINGW32__ */

#include "internet.h"
#include "resource.h"

#endif /* !_WININET_PRECOMP_H_ */
