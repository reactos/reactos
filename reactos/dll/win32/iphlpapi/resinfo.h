/*
 * iphlpapi dll implementation -- Resolver information helper function 
 *                                prototypes
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

#ifndef _IPHLP_RES_H
#define _IPHLP_RES_H

typedef struct _IPHLP_RES_INFO {
    DWORD riCount;
    struct sockaddr_in *riAddressList;
} IPHLP_RES_INFO, *PIPHLP_RES_INFO;

/* Get resolver info.  This currently is limited to a list of IP addresses
 * that name our DNS server list. */
PIPHLP_RES_INFO getResInfo();
/* Release any resources used in acquiring the resolver information */
VOID disposeResInfo( PIPHLP_RES_INFO InfoPtr );

#endif/*_IPHLP_RES_H*/
