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

#include "windef.h"
#include "winbase.h"
#include "winreg.h"

#ifdef __REACTOS__
# include <net/miniport.h> /* ULONGLONG */
# include <winsock2.h>     /* Enables NSPAPI */
# include <nspapi.h>       /* SOCKET_ADDRESS */
# include <iptypes.h>      /* IP_ADAPTER_ADDRESSES */
#endif

#include "iphlpapi.h"
#include "ifenum.h"
#include "ipstats.h"
#include "iphlp_res.h"
#include "wine/debug.h"

PIPHLP_RES_INFO getResInfo() {
    PIPHLP_RES_INFO InfoPtr = 
	(PIPHLP_RES_INFO)HeapAlloc( GetProcessHeap(), 0, 
				    sizeof(PIPHLP_RES_INFO) );
    if( InfoPtr ) {
	InfoPtr->riCount = 0;
	InfoPtr->riAddressList = (LPSOCKADDR)0;
    }

    return InfoPtr;
}

VOID disposeResInfo( PIPHLP_RES_INFO InfoPtr ) {
    HeapFree( GetProcessHeap(), 0, InfoPtr );
}
