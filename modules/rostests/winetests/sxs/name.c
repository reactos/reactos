/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#include <stdio.h>

#define COBJMACROS

#include <windows.h>
#include <winsxs.h>
#include <corerror.h>

#include "wine/test.h"

static void test_CreateAssemblyNameObject( void )
{
    static const WCHAR emptyW[] = {0};
    IAssemblyName *name;
    HRESULT hr;

    hr = CreateAssemblyNameObject( NULL, L"wine", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr);

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, L"wine", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    hr = CreateAssemblyNameObject( NULL, L"wine", CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine", CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    hr = CreateAssemblyNameObject( NULL, L"wine", 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine", 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );

    hr = CreateAssemblyNameObject( NULL, L"wine", CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine", CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, L"wine,version=\"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,version=1.2.3.4", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine, version=\"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME ),
        "expected ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,version =\"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME ),
        "expected ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,version= \"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, L"wine ,version=\"1.2.3.4\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,version", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, L"wine,type=\"\"", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08lx\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, L"wine,type=\"win32", CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08lx\n", hr );
    ok( !name, "expected NULL got %p\n", name );
}

START_TEST(name)
{
    test_CreateAssemblyNameObject();
}
