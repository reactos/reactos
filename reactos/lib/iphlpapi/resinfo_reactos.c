/*
 * iphlpapi dll implementation -- Auxiliary icmp functions
 *
 * These are stubs for functions that provide a simple ICMP probing API.  They
 * will be operating system specific when implemented.
 *
 * Copyright (C) 2004 Art Yerkes
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include "config.h"
#include "iphlpapi_private.h"

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
#include "resinfo.h"
#include "iphlpapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

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
