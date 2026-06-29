/*
 * Copyright (C) 2003 Juan Lang
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
#ifndef __WINE_UDPMIB_H
#define __WINE_UDPMIB_H

#define TCPIP_OWNING_MODULE_SIZE 16


/* UDP table */

typedef struct _MIB_UDPROW
{
    DWORD dwLocalAddr;
    DWORD dwLocalPort;
} MIB_UDPROW, *PMIB_UDPROW;

typedef struct _MIB_UDPTABLE
{
    DWORD      dwNumEntries;
    MIB_UDPROW table[1];
} MIB_UDPTABLE, *PMIB_UDPTABLE;

typedef struct _MIB_UDPROW_OWNER_PID
{
    DWORD dwLocalAddr;
    DWORD dwLocalPort;
    DWORD dwOwningPid;
} MIB_UDPROW_OWNER_PID, *PMIB_UDPROW_OWNER_PID;

typedef struct _MIB_UDPTABLE_OWNER_PID
{
    DWORD                dwNumEntries;
    MIB_UDPROW_OWNER_PID table[1];
} MIB_UDPTABLE_OWNER_PID, *PMIB_UDPTABLE_OWNER_PID;

typedef struct _MIB_UDPROW_OWNER_MODULE
{
    DWORD         dwLocalAddr;
    DWORD         dwLocalPort;
    DWORD         dwOwningPid;
    LARGE_INTEGER liCreateTimestamp;
    __C89_NAMELESS union
    {
        __C89_NAMELESS struct
        {
            int SpecificPortBind:1;
        } __C89_NAMELESSSTRUCTNAME;
        int dwFlags;
    } __C89_NAMELESSUNIONNAME;
    ULONGLONG OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
} MIB_UDPROW_OWNER_MODULE, *PMIB_UDPROW_OWNER_MODULE;

typedef struct _MIB_UDPTABLE_OWNER_MODULE
{
    DWORD                   dwNumEntries;
    MIB_UDPROW_OWNER_MODULE table[1];
} MIB_UDPTABLE_OWNER_MODULE, *PMIB_UDPTABLE_OWNER_MODULE;

typedef struct _MIB_UDP6ROW
{
    IN6_ADDR dwLocalAddr;
    DWORD    dwLocalScopeId;
    DWORD    dwLocalPort;
} MIB_UDP6ROW, *PMIB_UDP6ROW;

typedef struct _MIB_UDP6TABLE
{
    DWORD       dwNumEntries;
    MIB_UDP6ROW table[1];
} MIB_UDP6TABLE, *PMIB_UDP6TABLE;

typedef struct _MIB_UDP6ROW_OWNER_PID
{
    UCHAR ucLocalAddr[16];
    DWORD dwLocalScopeId;
    DWORD dwLocalPort;
    DWORD dwOwningPid;
} MIB_UDP6ROW_OWNER_PID, *PMIB_UDP6ROW_OWNER_PID;

typedef struct _MIB_UDP6TABLE_OWNER_PID
{
    DWORD                 dwNumEntries;
    MIB_UDP6ROW_OWNER_PID table[1];
} MIB_UDP6TABLE_OWNER_PID, *PMIB_UDP6TABLE_OWNER_PID;

typedef struct _MIB_UDP6ROW_OWNER_MODULE
{
    UCHAR         ucLocalAddr[16];
    DWORD         dwLocalScopeId;
    DWORD         dwLocalPort;
    DWORD         dwOwningPid;
    LARGE_INTEGER liCreateTimestamp;
    __C89_NAMELESS union
    {
        __C89_NAMELESS struct
        {
            int SpecificPortBind:1;
        } __C89_NAMELESSSTRUCTNAME;
        int dwFlags;
    } __C89_NAMELESSUNIONNAME;
    ULONGLONG OwningModuleInfo[TCPIP_OWNING_MODULE_SIZE];
} MIB_UDP6ROW_OWNER_MODULE, *PMIB_UDP6ROW_OWNER_MODULE;

typedef struct _MIB_UDP6TABLE_OWNER_MODULE
{
    DWORD                    dwNumEntries;
    MIB_UDP6ROW_OWNER_MODULE table[1];
} MIB_UDP6TABLE_OWNER_MODULE, *PMIB_UDP6TABLE_OWNER_MODULE;

/* UDP statistics */

typedef struct _MIB_UDPSTATS
{
    DWORD dwInDatagrams;
    DWORD dwNoPorts;
    DWORD dwInErrors;
    DWORD dwOutDatagrams;
    DWORD dwNumAddrs;
} MIB_UDPSTATS, *PMIB_UDPSTATS;

#endif /* __WINE_UDPMIB_H */
