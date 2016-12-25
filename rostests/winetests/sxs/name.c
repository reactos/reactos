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

static const WCHAR wine1W[] =
    {'w','i','n','e',0};
static const WCHAR wine2W[] =
    {'w','i','n','e',',','v','e','r','s','i','o','n','=','\"','1','.','2','.','3','.','4','\"',0};
static const WCHAR wine3W[] =
    {'w','i','n','e',',','v','e','r','s','i','o','n','=','1','.','2','.','3','.','4',0};
static const WCHAR wine4W[] =
    {'w','i','n','e',',',' ','v','e','r','s','i','o','n','=','\"','1','.','2','.','3','.','4','\"',0};
static const WCHAR wine5W[] =
    {'w','i','n','e',',','v','e','r','s','i','o','n',' ','=','\"','1','.','2','.','3','.','4','\"',0};
static const WCHAR wine6W[] =
    {'w','i','n','e',',','v','e','r','s','i','o','n','=',' ','\"','1','.','2','.','3','.','4','\"',0};
static const WCHAR wine7W[] =
    {'w','i','n','e',' ',',','v','e','r','s','i','o','n','=','\"','1','.','2','.','3','.','4','\"',0};
static const WCHAR wine8W[] =
    {'w','i','n','e',',','v','e','r','s','i','o','n',0};
static const WCHAR wine9W[] =
    {'w','i','n','e',',','t','y','p','e','=','\"','\"',0};
static const WCHAR wine10W[] =
    {'w','i','n','e',',','t','y','p','e','=','\"','w','i','n','3','2',0};

static void test_CreateAssemblyNameObject( void )
{
    static const WCHAR emptyW[] = {0};
    IAssemblyName *name;
    HRESULT hr;

    hr = CreateAssemblyNameObject( NULL, wine1W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok(hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr);

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, wine1W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08x\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    hr = CreateAssemblyNameObject( NULL, wine1W, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine1W, CANOF_SET_DEFAULT_VALUES, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    hr = CreateAssemblyNameObject( NULL, wine1W, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine1W, 0, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );

    hr = CreateAssemblyNameObject( NULL, wine1W, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, NULL, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, emptyW, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine1W, CANOF_SET_DEFAULT_VALUES|CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, wine2W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08x\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine3W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine4W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME ),
        "expected ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine5W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == HRESULT_FROM_WIN32( ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME ),
        "expected ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine6W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, wine7W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08x\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine8W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );

    name = NULL;
    hr = CreateAssemblyNameObject( &name, wine9W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == S_OK, "expected S_OK got %08x\n", hr );
    ok( name != NULL, "expected non-NULL name\n" );
    IAssemblyName_Release( name );

    name = (IAssemblyName *)0xdeadbeef;
    hr = CreateAssemblyNameObject( &name, wine10W, CANOF_PARSE_DISPLAY_NAME, NULL );
    ok( hr == E_INVALIDARG, "expected E_INVALIDARG got %08x\n", hr );
    ok( !name, "expected NULL got %p\n", name );
}

START_TEST(name)
{
    test_CreateAssemblyNameObject();
}
