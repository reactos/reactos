/*
 * Unit tests for the MPEG-1 stream splitter functions
 *
 * Copyright 2015 Anton Baskanov
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

#include "wine/test.h"
#include "dshow.h"

static IUnknown *create_mpeg_splitter(void)
{
    IUnknown *mpeg_splitter = NULL;
    HRESULT result = CoCreateInstance(&CLSID_MPEG1Splitter, NULL, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void **)&mpeg_splitter);
    ok(S_OK == result, "got 0x%08x\n", result);
    return mpeg_splitter;
}

static void test_query_interface(void)
{
    IUnknown *mpeg_splitter = create_mpeg_splitter();

    IAMStreamSelect *stream_select = NULL;
    HRESULT result = IUnknown_QueryInterface(
            mpeg_splitter, &IID_IAMStreamSelect, (void **)&stream_select);
    ok(S_OK == result, "got 0x%08x\n", result);
    if (S_OK == result)
    {
        IAMStreamSelect_Release(stream_select);
    }

    IUnknown_Release(mpeg_splitter);
}

START_TEST(mpegsplit)
{
    CoInitialize(NULL);

    test_query_interface();

    CoUninitialize();
}
