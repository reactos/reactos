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
DWORD STDCALL DisableMediaSense(HANDLE *pHandle,OVERLAPPED *pOverLapped)
{
    UNIMPLEMENTED
    return 0L;
}

/*
 * @unimplemented
 */
DWORD STDCALL RestoreMediaSense(OVERLAPPED* pOverlapped,LPDWORD lpdwEnableCount)
{
    UNIMPLEMENTED
    return 0L;
}

