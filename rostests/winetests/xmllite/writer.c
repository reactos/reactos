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

#define COBJMACROS

#include <stdarg.h>
//#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
//#include "ole2.h"
#include <xmllite.h>
#include <wine/test.h>

static HRESULT (WINAPI *pCreateXmlWriter)(REFIID riid, void **ppvObject, IMalloc *pMalloc);

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
    if(hr == S_OK)
    {
        IXmlWriter_Release(writer);
    }
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
#undef MAKEFUNC

    return TRUE;
}

START_TEST(writer)
{
    HRESULT r;

    r = CoInitialize( NULL );
    ok( r == S_OK, "failed to init com\n");

    if (!init_pointers())
    {
       CoUninitialize();
       return;
    }

    test_writer_create();

    CoUninitialize();
}
