/*
 * Endpoint Mapper Tower Definitions
 *
 * Copyright 2006 Robert Shearman (for CodeWeavers)
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
 *
 */

#include "epm.h"

#define EPM_PROTOCOL_DNET_NSP		0x04
#define EPM_PROTOCOL_OSI_TP4  		0x05
#define EPM_PROTOCOL_OSI_CLNS 		0x06
#define EPM_PROTOCOL_TCP     		0x07
#define EPM_PROTOCOL_UDP     		0x08
#define EPM_PROTOCOL_IP      		0x09
#define EPM_PROTOCOL_NCADG 			0x0a /* Connectionless RPC */
#define EPM_PROTOCOL_NCACN 			0x0b
#define EPM_PROTOCOL_NCALRPC 		0x0c /* Local RPC */
#define EPM_PROTOCOL_UUID 			0x0d
#define EPM_PROTOCOL_IPX  			0x0e
#define EPM_PROTOCOL_SMB     		0x0f
#define EPM_PROTOCOL_PIPE    		0x10
#define EPM_PROTOCOL_NETBIOS 		0x11
#define EPM_PROTOCOL_NETBEUI   		0x12
#define EPM_PROTOCOL_SPX     		0x13
#define EPM_PROTOCOL_NB_IPX  		0x14 /* NetBIOS over IPX */
#define EPM_PROTOCOL_DSP 			0x16 /* AppleTalk Data Stream Protocol */
#define EPM_PROTOCOL_DDP		    0x17 /* AppleTalk Data Datagram Protocol */
#define EPM_PROTOCOL_APPLETALK		0x18 /* AppleTalk */
#define EPM_PROTOCOL_VINES_SPP		0x1a 
#define EPM_PROTOCOL_VINES_IPC		0x1b /* Inter Process Communication */
#define EPM_PROTOCOL_STREETTALK		0x1c /* Vines Streettalk */
#define EPM_PROTOCOL_HTTP    		0x1f
#define EPM_PROTOCOL_UNIX_DS  		0x20 /* Unix domain socket */
#define EPM_PROTOCOL_NULL			0x21

#include <pshpack1.h>

typedef struct
{
    u_int16 count_lhs;
    u_int8 protid;
    GUID uuid;
    u_int16 major_version;
    u_int16 count_rhs;
    u_int16 minor_version;
} twr_uuid_floor_t;

typedef struct
{
    u_int16 count_lhs;
    u_int8 protid;
    u_int16 count_rhs;
    u_int16 port;
} twr_tcp_floor_t;

typedef struct
{
    u_int16 count_lhs;
    u_int8 protid;
    u_int16 count_rhs;
    u_int32 ipv4addr;
} twr_ipv4_floor_t;

typedef struct
{
    u_int16 count_lhs;
    u_int8 protid;
    u_int16 count_rhs;
} twr_empty_floor_t;

#include <poppack.h>
