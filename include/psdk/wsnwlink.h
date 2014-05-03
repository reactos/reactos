/*
 * Copyright (C) 2003,2004 Roderick Colenbrander
 *
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _WSNWLINK_
#define _WSNWLINK_

#define IPX_PTYPE 0x4000
#define IPX_FILTERPTYPE 0x4001
#define IPX_DSTYPE 0x4002
#define IPX_STOPFILTERPTYPE 0x4003
#define IPX_EXTENDED_ADDRESS 0x4004
#define IPX_RECVHDR 0x4005
#define IPX_MAXSIZE 0x4006
#define IPX_ADDRESS 0x4007
#define IPX_GETNETINFO 0x4008
#define IPX_GETNETINFO_NORIP 0x4009
#define IPX_SPXGETCONNECTIONSTATUS 0x400b
#define IPX_ADDRESS_NOTIFY 0x400c
#define IPX_MAX_ADAPTER_NUM 0x400d
#define IPX_RERIPNETNUMBER 0x400e
#define IPX_RECEIVE_BROADCAST 0x400f
#define IPX_IMMEDIATESPXACK 0x4010

typedef struct _IPX_ADDRESS_DATA {
    INT   adapternum;
    UCHAR netnum[4];
    UCHAR nodenum[6];
    BOOLEAN wan;
    BOOLEAN status;
    INT maxpkt;
    ULONG linkspeed;
} IPX_ADDRESS_DATA, *PIPX_ADDRESS_DATA;

#endif /* _WSNWLINK_ */
