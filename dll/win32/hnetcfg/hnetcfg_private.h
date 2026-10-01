/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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

#pragma once

enum type_id
{
    INetFwAuthorizedApplication_tid,
    INetFwAuthorizedApplications_tid,
    INetFwMgr_tid,
    INetFwOpenPort_tid,
    INetFwOpenPorts_tid,
    INetFwPolicy_tid,
    INetFwPolicy2_tid,
    INetFwProfile_tid,
    INetFwRule_tid,
    INetFwRules_tid,
    IUPnPNAT_tid,
    IStaticPortMappingCollection_tid,
    IStaticPortMapping_tid,
    last_tid
};

HRESULT get_typeinfo(enum type_id, ITypeInfo **);
void release_typelib(void);

HRESULT NetFwMgr_create(IUnknown *, LPVOID *);
HRESULT NetFwPolicy_create(IUnknown *, LPVOID *);
HRESULT NetFwPolicy2_create(IUnknown *, void **);
HRESULT NetFwProfile_create(IUnknown *, LPVOID *);
HRESULT NetFwAuthorizedApplication_create(IUnknown *, LPVOID *);
HRESULT NetFwAuthorizedApplications_create(IUnknown *, LPVOID *);
HRESULT NetFwOpenPorts_create(IUnknown *, LPVOID *);
HRESULT NetFwOpenPort_create(IUnknown *, LPVOID *);
HRESULT NetFwServices_create(IUnknown *, LPVOID *);
HRESULT NetFwRule_create(IUnknown *, void **);
HRESULT IUPnPNAT_create(IUnknown *, void **);
