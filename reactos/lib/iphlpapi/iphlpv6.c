#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif

#ifdef __REACTOS__
# include <windows.h>
# include <windef.h>
# include <winbase.h>
# include <net/miniport.h>
# include <winsock2.h>
# include <nspapi.h>
# include <iptypes.h>
# include <ws2tcpip.h>
# include "iphlpapiextra.h"
#else
# include "windef.h"
# include "winbase.h"
# include "winreg.h"
#endif

#include "iphlpapi.h"
#include "ifenum.h"
#include "ipstats.h"
#include "iphlp_res.h"
#include "wine/debug.h"

/*
 * @unimplemented
 */
HANDLE STDCALL  Icmp6CreateFile(
    VOID
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD
STDCALL 
Icmp6SendEcho2(
    HANDLE                   IcmpHandle,
    HANDLE                   Event,
    FARPROC                  ApcRoutine,
    PVOID                    ApcContext,
    struct sockaddr_in6     *SourceAddress,
    struct sockaddr_in6     *DestinationAddress,
    LPVOID                   RequestData,
    WORD                     RequestSize,
    PIP_OPTION_INFORMATION   RequestOptions,
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize,
    DWORD                    Timeout
    )
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
Icmp6ParseReplies(
    LPVOID                   ReplyBuffer,
    DWORD                    ReplySize
    )
{
    UNIMPLEMENTED
    return 0L;
}

