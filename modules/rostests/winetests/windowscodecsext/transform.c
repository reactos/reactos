/*
 * Copyright 2013 Hans Leidekker for CodeWeavers
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

#include <stdio.h>
#include "windows.h"
#include "objbase.h"
#include "wincodec.h"
#include "wine/test.h"

HRESULT WINAPI WICCreateColorTransform_Proxy(IWICColorTransform**);

static void test_WICCreateColorTransform_Proxy(void)
{
    HRESULT hr;
    IWICColorTransform *transform;

    hr = WICCreateColorTransform_Proxy( NULL );
    ok( hr == E_INVALIDARG, "got %08lx\n", hr );

    transform = NULL;
    hr = WICCreateColorTransform_Proxy( &transform );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (transform) IWICColorTransform_Release( transform );

    hr = CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );
    ok( hr == S_OK, "got %08lx\n", hr );

    transform = NULL;
    hr = WICCreateColorTransform_Proxy( &transform );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (transform) IWICColorTransform_Release( transform );
    CoUninitialize();

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    ok( hr == S_OK, "got %08lx\n", hr );

    transform = NULL;
    hr = WICCreateColorTransform_Proxy( &transform );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (transform) IWICColorTransform_Release( transform );
    CoUninitialize();
}

START_TEST(transform)
{
    test_WICCreateColorTransform_Proxy();
}
