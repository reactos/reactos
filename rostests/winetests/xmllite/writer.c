/*
 * XMLLite IXmlWriter tests
 *
 * Copyright 2011 (C) Alistair Leslie-Hughes
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define CONST_VTABLE
#define COBJMACROS

#include <stdarg.h>
//#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
//#include "ole2.h"
#include <xmllite.h>
#include <wine/test.h>

#include <initguid.h>
DEFINE_GUID(IID_IXmlWriterOutput, 0xc1131708, 0x0f59, 0x477f, 0x93, 0x59, 0x7d, 0x33, 0x24, 0x51, 0xbc, 0x1a);

static HRESULT (WINAPI *pCreateXmlWriter)(REFIID riid, void **ppvObject, IMalloc *pMalloc);
static HRESULT (WINAPI *pCreateXmlWriterOutputWithEncodingName)(IUnknown *stream,
                                                                IMalloc *imalloc,
                                                                LPCWSTR encoding_name,
                                                                IXmlWriterOutput **output);

static HRESULT WINAPI testoutput_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualGUID(riid, &IID_IUnknown)) {
        *obj = iface;
        return S_OK;
    }
    else {
        ok(0, "unknown riid=%s\n", wine_dbgstr_guid(riid));
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI testoutput_AddRef(IUnknown *iface)
{
    return 2;
}

static ULONG WINAPI testoutput_Release(IUnknown *iface)
{
    return 1;
}

static const IUnknownVtbl testoutputvtbl = {
    testoutput_QueryInterface,
    testoutput_AddRef,
    testoutput_Release
};

static IUnknown testoutput = { &testoutputvtbl };

static void test_writer_create(void)
{
    HRESULT hr;
    IXmlWriter *writer;

    /* crashes native */
    if (0)
    {
        pCreateXmlWriter(&IID_IXmlWriter, NULL, NULL);
        pCreateXmlWriter(NULL, (LPVOID*)&writer, NULL);
    }

    hr = pCreateXmlWriter(&IID_IXmlWriter, (LPVOID*)&writer, NULL);
    ok(hr == S_OK, "Expected S_OK, got %08x\n", hr);
    IXmlWriter_Release(writer);
}

static BOOL init_pointers(void)
{
    /* don't free module here, it's to be unloaded on exit */
    HMODULE mod = LoadLibraryA("xmllite.dll");

    if (!mod)
    {
        win_skip("xmllite library not available\n");
        return FALSE;
    }

#define MAKEFUNC(f) if (!(p##f = (void*)GetProcAddress(mod, #f))) return FALSE;
    MAKEFUNC(CreateXmlWriter);
    MAKEFUNC(CreateXmlWriterOutputWithEncodingName);
#undef MAKEFUNC

    return TRUE;
}

static void test_writeroutput(void)
{
    static const WCHAR utf16W[] = {'u','t','f','-','1','6',0};
    IXmlWriterOutput *output;
    IUnknown *unk;
    HRESULT hr;

    hr = pCreateXmlWriterOutputWithEncodingName(&testoutput, NULL, utf16W, &output);
    ok(hr == S_OK, "got %08x\n", hr);
    unk = NULL;
    hr = IUnknown_QueryInterface(output, &IID_IXmlWriterOutput, (void**)&unk);
    ok(hr == S_OK, "got %08x\n", hr);
    ok(unk != NULL, "got %p\n", unk);
    /* releasing 'unk' crashes on native */
    IUnknown_Release(output);
}

START_TEST(writer)
{
    if (!init_pointers())
       return;

    test_writer_create();
    test_writeroutput();
}
