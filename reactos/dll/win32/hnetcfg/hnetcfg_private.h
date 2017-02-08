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

#ifndef _HNETCFG_PRIVATE_H_
#define _HNETCFG_PRIVATE_H_

#include <wine/config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <netfw.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(hnetcfg);

enum type_id
{
    INetFwAuthorizedApplication_tid,
    INetFwAuthorizedApplications_tid,
    INetFwMgr_tid,
    INetFwOpenPort_tid,
    INetFwOpenPorts_tid,
    INetFwPolicy_tid,
    INetFwProfile_tid,
    last_tid
};

HRESULT get_typeinfo(enum type_id, ITypeInfo **) DECLSPEC_HIDDEN;
void release_typelib(void) DECLSPEC_HIDDEN;

HRESULT NetFwMgr_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT NetFwPolicy_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT NetFwProfile_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT NetFwAuthorizedApplication_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT NetFwAuthorizedApplications_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT NetFwOpenPorts_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT NetFwOpenPort_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;
HRESULT NetFwServices_create(IUnknown *, LPVOID *) DECLSPEC_HIDDEN;

#endif /* _HNETCFG_PRIVATE_H_ */
