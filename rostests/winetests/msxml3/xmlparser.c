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

static void create_test(void)
{
    HRESULT hr;
    IXMLParser *parser;
    DWORD flags;

    hr = CoCreateInstance(&CLSID_XMLParser30, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLParser, (void**)&parser);
    if (FAILED(hr))
    {
        win_skip("IXMLParser is not available (0x%08x)\n", hr);
        return;
    }

    flags = IXMLParser_GetFlags(parser);
    ok(flags == 0, "Expected 0 got %d\n", flags);

    hr = IXMLParser_SetFlags(parser, XMLFLAG_SAX);
    ok(hr == S_OK, "Expected S_OK got 0x%08x\n", hr);

    flags = IXMLParser_GetFlags(parser);
    ok(flags == XMLFLAG_SAX, "Expected 0 got %d\n", flags);

    hr = IXMLParser_SetFlags(parser, 0);
    ok(hr == S_OK, "Expected S_OK got 0x%08x\n", hr);

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
