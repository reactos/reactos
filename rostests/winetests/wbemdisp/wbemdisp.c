/*
 * Copyright 2015 Hans Leidekker for CodeWeavers
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
#include "initguid.h"
#include "objidl.h"
#include "wbemdisp.h"
#include "wine/test.h"

DEFINE_GUID(CLSID_WINMGMTS,0x172bddf8,0xceea,0x11d1,0x8b,0x05,0x00,0x60,0x08,0x06,0xd9,0xb6);
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static void test_ParseDisplayName(void)
{
    static const WCHAR biosW[] = {'W','i','n','3','2','_','B','i','o','s',0};
    static const WCHAR manufacturerW[] = {'M','a','n','u','f','a','c','t','u','r','e','r',0};
    static const WCHAR versionW[] = {'v','e','r','s','i','o','n',0};
    static const WCHAR nosuchW[] = {'N','o','S','u','c','h',0};
    static const WCHAR name1[] =
        {'w','i','n','m','g','m','t','s',':',0};
    static const WCHAR name2[] =
        {'w','i','n','m','g','m','t','s',':','\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',0};
    static const WCHAR name3[] =
        {'w','i','n','m','g','m','t','s',':','\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',':',
         'W','i','n','3','2','_','L','o','g','i','c','a','l','D','i','s','k','.',
         'D','e','v','i','c','e','I','D','=','\'','C',':','\'',0};
    static const WCHAR name4[] =
        {'w','i','n','m','g','m','t','s',':','\\','\\','.','\\','r','o','o','t','\\','c','i','m','v','2',':',
         'W','i','n','3','2','_','S','e','r','v','i','c','e',0};
    static const struct
    {
        const WCHAR *name;
        HRESULT      hr;
        REFIID       iid;
        ULONG        eaten;
    } tests[] =
    {
        { name1, S_OK, &IID_ISWbemServices, sizeof(name1)/sizeof(name1[0]) - 1 },
        { name2, S_OK, &IID_ISWbemServices, sizeof(name2)/sizeof(name2[0]) - 1 },
        { name3, S_OK, &IID_ISWbemObject, sizeof(name3)/sizeof(name3[0]) - 1 },
        { name4, S_OK, &IID_ISWbemObject, sizeof(name4)/sizeof(name4[0]) - 1 }
    };
    LCID english = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);
    IParseDisplayName *displayname;
    IBindCtx *ctx;
    IMoniker *moniker;
    IUnknown *obj;
    BSTR str;
    ULONG i, eaten, count;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_WINMGMTS, NULL, CLSCTX_INPROC_SERVER, &IID_IParseDisplayName, (void **)&displayname );
    if (hr != S_OK)
    {
        win_skip( "can't create instance of WINMGMTS\n" );
        return;
    }

    hr = CreateBindCtx( 0, &ctx );
    ok( hr == S_OK, "got %x\n", hr );

    for (i =0; i < sizeof(tests)/sizeof(tests[0]); i++)
    {
        str = SysAllocString( tests[i].name );
        eaten = 0xdeadbeef;
        moniker = NULL;
        hr = IParseDisplayName_ParseDisplayName( displayname, NULL, str, &eaten, &moniker );
        SysFreeString( str );
        ok( hr == tests[i].hr, "%u: got %x\n", i, hr );
        ok( eaten == tests[i].eaten, "%u: got %u\n", i, eaten );
        if (moniker)
        {
            obj = NULL;
            hr = IMoniker_BindToObject( moniker, ctx, NULL, tests[i].iid, (void **)&obj );
            ok( hr == S_OK, "%u: got %x\n", i, hr );
            if (obj) IUnknown_Release( obj );
            IMoniker_Release( moniker );
        }
    }

    str = SysAllocString( name1 );
    eaten = 0xdeadbeef;
    moniker = NULL;
    hr = IParseDisplayName_ParseDisplayName( displayname, NULL, str, &eaten, &moniker );
    SysFreeString( str );
    ok( hr == S_OK, "got %x\n", hr );
    ok( eaten == lstrlenW(name1), "got %u\n", eaten );
    if (moniker)
    {
        ISWbemServices *services = NULL;

        hr = IMoniker_BindToObject( moniker, ctx, NULL, &IID_IUnknown, (void **)&services );
        ok( hr == S_OK, "got %x\n", hr );
        if (services)
        {
            ISWbemObjectSet *objectset = NULL;

            str = SysAllocString( biosW );
            hr = ISWbemServices_InstancesOf( services, str, 0, NULL, &objectset );
            SysFreeString( str );
            ok( hr == S_OK, "got %x\n", hr );
            if (objectset)
            {
                hr = ISWbemObjectSet_get__NewEnum( objectset, &obj );
                ok( hr == S_OK, "got %x\n", hr );
                if (obj)
                {
                    IEnumVARIANT *enumvar = NULL;

                    hr = IUnknown_QueryInterface( obj, &IID_IEnumVARIANT, (void **)&enumvar );
                    ok( hr == S_OK, "got %x\n", hr );

                    if (enumvar)
                    {
                        VARIANT var, res;
                        ULONG fetched;
                        IDispatch *dispatch = NULL;
                        DISPID dispid;
                        DISPPARAMS params;
                        UINT arg_err;

                        fetched = 0xdeadbeef;
                        hr = IEnumVARIANT_Next( enumvar, 0, &var, &fetched );
                        ok( hr == S_OK, "got %x\n", hr );
                        ok( !fetched, "got %u\n", fetched );

                        fetched = 0xdeadbeef;
                        V_VT( &var ) = VT_ERROR;
                        V_ERROR( &var ) = 0xdeadbeef;
                        hr = IEnumVARIANT_Next( enumvar, 1, &var, &fetched );
                        ok( hr == S_OK, "got %x\n", hr );
                        ok( fetched == 1, "got %u\n", fetched );
                        ok( V_VT( &var ) == VT_DISPATCH, "got %u\n", V_VT( &var ) );
                        ok( V_DISPATCH( &var ) != (IDispatch *)0xdeadbeef, "got %u\n", V_VT( &var ) );

                        dispatch = V_DISPATCH( &var );
                        count = 0;
                        hr = IDispatch_GetTypeInfoCount( dispatch, &count );
                        ok( hr == S_OK, "got %x\n", hr );
                        ok( count == 1, "got %u\n", count );

                        str = SysAllocString( manufacturerW );
                        dispid = 0xdeadbeef;
                        hr = IDispatch_GetIDsOfNames( dispatch, &IID_NULL, &str, 1, english, &dispid );
                        SysFreeString( str );
                        ok( hr == S_OK, "got %x\n", hr );
                        ok( dispid == 0x1800001 || dispid == 0x10b /* win2k */, "got %x\n", dispid );

                        str = SysAllocString( versionW );
                        dispid = 0xdeadbeef;
                        hr = IDispatch_GetIDsOfNames( dispatch, &IID_NULL, &str, 1, english, &dispid );
                        SysFreeString( str );
                        ok( hr == S_OK, "got %x\n", hr );
                        ok( dispid == 0x1800002 || dispid == 0x119 /* win2k */, "got %x\n", dispid );

                        str = SysAllocString( nosuchW );
                        dispid = 0xdeadbeef;
                        hr = IDispatch_GetIDsOfNames( dispatch, &IID_NULL, &str, 1, english, &dispid );
                        SysFreeString( str );
                        ok( hr == DISP_E_UNKNOWNNAME, "got %x\n", hr );
                        ok( dispid == DISPID_UNKNOWN, "got %x\n", dispid );

                        str = SysAllocString( manufacturerW );
                        dispid = 0xdeadbeef;
                        hr = IDispatch_GetIDsOfNames( dispatch, &IID_NULL, &str, 1, english, &dispid );
                        SysFreeString( str );
                        ok( hr == S_OK, "got %x\n", hr );
                        ok( dispid == 0x1800001 || dispid == 0x10b /* win2k */, "got %x\n", dispid );

                        if (dispid == 0x1800001) /* crashes on win2k */
                        {
                            V_VT( &res ) = VT_ERROR;
                            V_BSTR( &res ) = (BSTR)0xdeadbeef;
                            params.rgvarg = (VARIANTARG *)0xdeadbeef;
                            params.rgdispidNamedArgs = (DISPID *)0xdeadbeef;
                            params.cArgs = params.cNamedArgs = 0xdeadbeef;
                            arg_err = 0xdeadbeef;
                            hr = IDispatch_Invoke( dispatch, DISPID_UNKNOWN, &IID_NULL, english,
                                                   DISPATCH_METHOD|DISPATCH_PROPERTYGET,
                                                   &params, &res, NULL, &arg_err );
                            ok( hr == DISP_E_MEMBERNOTFOUND || hr == S_OK /* winxp */, "got %x\n", hr );
                            ok( params.rgvarg == (VARIANTARG *)0xdeadbeef, "got %p\n", params.rgvarg );
                            ok( params.rgdispidNamedArgs == (DISPID *)0xdeadbeef, "got %p\n", params.rgdispidNamedArgs );
                            ok( params.cArgs == 0xdeadbeef, "got %u\n", params.cArgs );
                            ok( params.cNamedArgs == 0xdeadbeef, "got %u\n", params.cNamedArgs );
                            ok( V_VT( &res ) == VT_ERROR, "got %u\n", V_VT( &res ) );
                            ok( V_ERROR( &res ) == 0xdeadbeef, "got %u\n", V_VT( &res ) );
                            ok( arg_err == 0xdeadbeef, "got %u\n", arg_err );
                            if (hr == S_OK) VariantClear( &res );
                        }

                        V_VT( &res ) = VT_ERROR;
                        V_BSTR( &res ) = (BSTR)0xdeadbeef;
                        memset( &params, 0, sizeof(params) );
                        hr = IDispatch_Invoke( dispatch, dispid, &IID_NULL, english,
                                               DISPATCH_METHOD|DISPATCH_PROPERTYGET,
                                               &params, &res, NULL, NULL );
                        ok( hr == S_OK, "got %x\n", hr );
                        ok( params.rgvarg == NULL, "got %p\n", params.rgvarg );
                        ok( params.rgdispidNamedArgs == NULL, "got %p\n", params.rgdispidNamedArgs );
                        ok( !params.cArgs, "got %u\n", params.cArgs );
                        ok( !params.cNamedArgs, "got %u\n", params.cNamedArgs );
                        ok( V_VT( &res ) == VT_BSTR, "got %u\n", V_VT( &res ) );
                        ok( V_BSTR( &res ) != (BSTR)0xdeadbeef, "got %u\n", V_VT( &res ) );
                        VariantClear( &res );
                        VariantClear( &var );

                        fetched = 0xdeadbeef;
                        hr = IEnumVARIANT_Next( enumvar, 1, &var, &fetched );
                        ok( hr == S_FALSE, "got %x\n", hr );
                        ok( !fetched, "got %u\n", fetched );

                        IEnumVARIANT_Release( enumvar );
                    }
                    IUnknown_Release( obj );
                }
                ISWbemObjectSet_Release( objectset );
            }
            IUnknown_Release( services );
        }
        IMoniker_Release( moniker );
    }

    IBindCtx_Release( ctx );
    IParseDisplayName_Release( displayname );
}

START_TEST(wbemdisp)
{
    CoInitialize( NULL );
    test_ParseDisplayName();
    CoUninitialize();
}
