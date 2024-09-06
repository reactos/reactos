/*
 *	IParseDisplayName implementation for DEVENUM.dll
 *
 * Copyright (C) 2002 Robert Shearman
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
 * NOTES ON THIS FILE:
 * - Implements IParseDisplayName interface which creates a moniker
 *   from a string in a special format
 */
#include "devenum_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

static HRESULT WINAPI devenum_parser_QueryInterface(IParseDisplayName *iface, REFIID riid, void **ppv)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (!ppv)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IParseDisplayName))
    {
        *ppv = iface;
        IParseDisplayName_AddRef(iface);
        return S_OK;
    }

    FIXME("- no interface IID: %s\n", debugstr_guid(riid));
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI devenum_parser_AddRef(IParseDisplayName *iface)
{
    TRACE("\n");

    DEVENUM_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI devenum_parser_Release(IParseDisplayName *iface)
{
    TRACE("\n");

    DEVENUM_UnlockModule();

    return 1; /* non-heap based object */
}

static HRESULT WINAPI devenum_parser_ParseDisplayName(IParseDisplayName *iface,
        IBindCtx *pbc, LPOLESTR name, ULONG *eaten, IMoniker **ret)
{
    WCHAR buffer[MAX_PATH];
    enum device_type type;
    struct moniker *mon;
    CLSID class;

    TRACE("(%p, %s, %p, %p)\n", pbc, debugstr_w(name), eaten, ret);

    *ret = NULL;
    if (eaten)
        *eaten = lstrlenW(name);

    name = wcschr(name, ':') + 1;

    if (!wcsncmp(name, swW, 3))
    {
        type = DEVICE_FILTER;
        name += 3;
    }
    else if (!wcsncmp(name, cmW, 3))
    {
        type = DEVICE_CODEC;
        name += 3;
    }
    else if (!wcsncmp(name, dmoW, 4))
    {
        type = DEVICE_DMO;
        name += 4;
    }
    else
    {
        FIXME("unhandled device type %s\n", debugstr_w(name));
        return MK_E_SYNTAX;
    }

    if (!(mon = moniker_create()))
        return E_OUTOFMEMORY;

    if (type == DEVICE_DMO)
    {
        lstrcpynW(buffer, name, CHARS_IN_GUID);
        if (FAILED(CLSIDFromString(buffer, &mon->clsid)))
        {
            IMoniker_Release(&mon->IMoniker_iface);
            return MK_E_SYNTAX;
        }

        lstrcpynW(buffer, name + CHARS_IN_GUID - 1, CHARS_IN_GUID);
        if (FAILED(CLSIDFromString(buffer, &mon->class)))
        {
            IMoniker_Release(&mon->IMoniker_iface);
            return MK_E_SYNTAX;
        }
    }
    else
    {
        lstrcpynW(buffer, name, CHARS_IN_GUID);
        if (CLSIDFromString(buffer, &class) == S_OK)
        {
            mon->has_class = TRUE;
            mon->class = class;
            name += CHARS_IN_GUID;
        }

        if (!(mon->name = CoTaskMemAlloc((lstrlenW(name) + 1) * sizeof(WCHAR))))
        {
            IMoniker_Release(&mon->IMoniker_iface);
            return E_OUTOFMEMORY;
        }
        lstrcpyW(mon->name, name);
    }

    mon->type = type;

    *ret = &mon->IMoniker_iface;

    return S_OK;
}

/**********************************************************************
 * IParseDisplayName_Vtbl
 */
static const IParseDisplayNameVtbl IParseDisplayName_Vtbl =
{
    devenum_parser_QueryInterface,
    devenum_parser_AddRef,
    devenum_parser_Release,
    devenum_parser_ParseDisplayName,
};

/* The one instance of this class */
IParseDisplayName devenum_parser = { &IParseDisplayName_Vtbl };
