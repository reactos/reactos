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

    return 2; /* non-heap based object */
}

static ULONG WINAPI devenum_parser_Release(IParseDisplayName *iface)
{
    TRACE("\n");

    return 1; /* non-heap based object */
}

static HRESULT WINAPI devenum_parser_ParseDisplayName(IParseDisplayName *iface,
        IBindCtx *pbc, LPOLESTR name, ULONG *eaten, IMoniker **ret)
{
    struct moniker *moniker;
    WCHAR buffer[MAX_PATH];
    enum device_type type;
    GUID class, clsid;

    TRACE("(%p, %s, %p, %p)\n", pbc, debugstr_w(name), eaten, ret);

    *ret = NULL;
    if (eaten)
        *eaten = wcslen(name);

    name = wcschr(name, ':') + 1;

    if (!wcsncmp(name, L"sw:", 3))
    {
        type = DEVICE_FILTER;
        name += 3;
    }
    else if (!wcsncmp(name, L"cm:", 3))
    {
        type = DEVICE_CODEC;
        name += 3;
    }
    else if (!wcsncmp(name, L"dmo:", 4))
    {
        type = DEVICE_DMO;
        name += 4;
    }
    else
    {
        FIXME("unhandled device type %s\n", debugstr_w(name));
        return MK_E_SYNTAX;
    }

    if (type == DEVICE_DMO)
    {
        lstrcpynW(buffer, name, CHARS_IN_GUID);
        if (FAILED(CLSIDFromString(buffer, &clsid)))
            return MK_E_SYNTAX;

        lstrcpynW(buffer, name + CHARS_IN_GUID - 1, CHARS_IN_GUID);
        if (FAILED(CLSIDFromString(buffer, &class)))
            return MK_E_SYNTAX;

        moniker = dmo_moniker_create(class, clsid);
    }
    else
    {
        lstrcpynW(buffer, name, CHARS_IN_GUID);
        if (CLSIDFromString(buffer, &class) == S_OK)
        {
            name += CHARS_IN_GUID;
            if (type == DEVICE_FILTER)
                moniker = filter_moniker_create(&class, name);
            else
                moniker = codec_moniker_create(&class, name);
        }
        else
        {
            if (type == DEVICE_FILTER)
                moniker = filter_moniker_create(NULL, name);
            else
                moniker = codec_moniker_create(NULL, name);
        }
    }

    if (!moniker)
        return E_OUTOFMEMORY;

    *ret = &moniker->IMoniker_iface;
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
