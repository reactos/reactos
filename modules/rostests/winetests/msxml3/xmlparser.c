/*
 * XML Parser implementation
 *
 * Copyright 2011 Alistair Leslie-Hughes
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
#define COBJMACROS
#define CONST_VTABLE

#include <stdio.h>
#include <assert.h>

#include "windows.h"
#include "ole2.h"
#include "xmlparser.h"
#include "wine/test.h"


static HRESULT WINAPI nodefact_QueryInterface(IXMLNodeFactory *iface,
        REFIID riid, void **ppvObject)
{
    *ppvObject = NULL;

    if (IsEqualGUID(riid, &IID_IXMLNodeFactory) ||
        IsEqualGUID(riid, &IID_IUnknown))
        *ppvObject = iface;
    else
        return E_NOINTERFACE;

    return S_OK;
}

static ULONG WINAPI nodefact_AddRef(IXMLNodeFactory *iface)
{
    return 2;
}

static ULONG WINAPI nodefact_Release(IXMLNodeFactory *iface)
{
    return 1;
}

static HRESULT WINAPI nodefact_NotifyEvent(IXMLNodeFactory *iface,
        IXMLNodeSource *pSource, XML_NODEFACTORY_EVENT iEvt)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI nodefact_BeginChildren(IXMLNodeFactory *iface,
        IXMLNodeSource *pSource, XML_NODE_INFO *pNodeInfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI nodefact_EndChildren(IXMLNodeFactory *iface,
        IXMLNodeSource *pSource, BOOL fEmpty, XML_NODE_INFO *pNodeInfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI nodefact_Error(IXMLNodeFactory *iface,
        IXMLNodeSource *pSource, HRESULT hrErrorCode, USHORT cNumRecs,
        XML_NODE_INFO **ppNodeInfo)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI nodefact_CreateNode(IXMLNodeFactory *iface, IXMLNodeSource *pSource,
        PVOID pNodeParent, USHORT cNumRecs, XML_NODE_INFO **ppNodeInfo)
{
    return E_NOTIMPL;
}

static const IXMLNodeFactoryVtbl nodefactoryVtbl =
{
    nodefact_QueryInterface,
    nodefact_AddRef,
    nodefact_Release,
    nodefact_NotifyEvent,
    nodefact_BeginChildren,
    nodefact_EndChildren,
    nodefact_Error,
    nodefact_CreateNode
};

static IXMLNodeFactory thenodefactory = { &nodefactoryVtbl };

static void create_test(void)
{
    HRESULT hr;
    IXMLParser *parser;
    IXMLNodeFactory *nodefactory;
    DWORD flags;

    hr = CoCreateInstance(&CLSID_XMLParser30, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLParser, (void**)&parser);
    if (FAILED(hr))
    {
        win_skip("IXMLParser is not available, hr %#lx.\n", hr);
        return;
    }

    flags = IXMLParser_GetFlags(parser);
    ok(!flags, "Unexpected flags %#lx.\n", flags);

    hr = IXMLParser_SetFlags(parser, XMLFLAG_SAX);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    flags = IXMLParser_GetFlags(parser);
    ok(flags == XMLFLAG_SAX, "Unexpected flags %ld.\n", flags);

    hr = IXMLParser_GetFactory(parser, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLParser_GetFactory(parser, &nodefactory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(nodefactory == NULL, "expected NULL\n");

    hr = IXMLParser_SetFactory(parser, &thenodefactory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLParser_GetFactory(parser, &nodefactory);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    ok(nodefactory == &thenodefactory, "expected NULL\n");

    hr = IXMLParser_SetInput(parser, NULL);
    ok(hr == E_INVALIDARG, "Unexpected hr %#lx.\n", hr);

    hr = IXMLParser_SetFactory(parser, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLParser_SetFlags(parser, 0);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    hr = IXMLParser_GetParserState(parser);
    ok(hr == XMLPARSER_IDLE, "Unexpected hr %#lx.\n", hr);

    IXMLParser_Release(parser);
}

START_TEST(xmlparser)
{
    HRESULT hr;

    hr = CoInitialize( NULL );
    ok( hr == S_OK, "failed to init com\n");
    if (hr != S_OK)
        return;

    create_test();

    CoUninitialize();
}
