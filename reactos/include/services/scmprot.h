/* 
 * Service Control Manager - Protocol Header
 *
 * Copyright (C) 2004 Filip Navara
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 * 
 * $Id: scmprot.h,v 1.1 2004/04/12 17:14:26 navaraf Exp $
 */

#ifndef _SCM_H
#define _SCM_H

/*
 * NOTE:
 * This protocol isn't compatible with the Windows (R) one. Since
 * Windows (R) XP (or 2000?) all the communcation goes through RPC.
 * We don't have RPC implemented yet, so it can't be used yet :(
 */

typedef struct
{
    ULONG Length;
    WCHAR Buffer[256];
} SCM_STRING;

#define INIT_SCM_STRING(x, y) x.Length = wcslen(y) * sizeof(WCHAR), RtlCopyMemory(x.Buffer, y, x.Length + sizeof(UNICODE_NULL))

/*
 * Global requests
 */

#define SCM_OPENSERVICE		0x14
#define SCM_CREATESERVICE	0x20

typedef struct _SCM_OPENSERVICE_REQUEST
{
    DWORD RequestCode;
    SCM_STRING ServiceName;
    DWORD dwDesiredAccess;
} SCM_OPENSERVICE_REQUEST, *PSCM_OPENSERVICE_REQUEST;

typedef struct _SCM_OPENSERVICE_REPLY
{
    DWORD ReplyStatus;
    WCHAR PipeName[128];
} SCM_OPENSERVICE_REPLY, *PSCM_OPENSERVICE_REPLY;

typedef struct _SCM_CREATESERVICE_REQUEST
{
    DWORD RequestCode;
    SCM_STRING ServiceName;
    SCM_STRING DisplayName;
    DWORD dwDesiredAccess;
    DWORD dwServiceType;
    DWORD dwStartType;
    DWORD dwErrorControl;
    SCM_STRING BinaryPathName;
    SCM_STRING LoadOrderGroup;
    SCM_STRING Dependencies;
    SCM_STRING ServiceStartName;
    SCM_STRING Password;
} SCM_CREATESERVICE_REQUEST, *PSCM_CREATESERVICE_REQUEST;

typedef struct _SCM_CREATESERVICE_REPLY
{
    DWORD ReplyStatus;
    WCHAR PipeName[128];
} SCM_CREATESERVICE_REPLY, *PSCM_CREATESERVICE_REPLY;

typedef union _SCM_REQUEST
{
    DWORD RequestCode;
    SCM_OPENSERVICE_REQUEST OpenService;
    SCM_CREATESERVICE_REQUEST CreateService;
} SCM_REQUEST, *PSCM_REQUEST;

typedef union _SCM_REPLY
{
    DWORD ReplyStatus;
    SCM_OPENSERVICE_REPLY OpenService;
    SCM_CREATESERVICE_REPLY CreateService;
} SCM_REPLY, *PSCM_REPLY;

/*
 * Per service requests
 */

#define SCM_DELETESERVICE	0x10
#define SCM_STARTSERVICE	0x11

typedef union _SCM_SERVICE_REQUEST
{
    DWORD RequestCode;
} SCM_SERVICE_REQUEST, *PSCM_SERVICE_REQUEST;

typedef union _SCM_SERVICE_REPLY
{
    DWORD ReplyStatus;
} SCM_SERVICE_REPLY, *PSCM_SERVICE_REPLY;

#endif
