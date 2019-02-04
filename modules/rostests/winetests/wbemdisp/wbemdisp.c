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
#include "wbemcli.h"
#include "wine/test.h"

DEFINE_GUID(CLSID_WINMGMTS,0x172bddf8,0xceea,0x11d1,0x8b,0x05,0x00,0x60,0x08,0x06,0xd9,0xb6);
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static const LCID english = MAKELCID(MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US),SORT_DEFAULT);

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
    static const WCHAR stdregprovW[] =
        {'w','i','n','m','g','m','t','s',':','\\','\\','.','\\','r','o','o','t','\\','d','e','f','a','u','l','t',':',
         'S','t','d','R','e','g','P','r','o','v',0};
    static const WCHAR getstringvalueW[] =
        {'G','e','t','S','t','r','i','n','g','V','a','l','u','e',0};
    static const struct
    {
        const WCHAR *name;
        HRESULT      hr;
        REFIID       iid;
        ULONG        eaten;
    } tests[] =
    {
        { name1, S_OK, &IID_ISWbemServices, ARRAY_SIZE( name1 ) - 1 },
        { name2, S_OK, &IID_ISWbemServices, ARRAY_SIZE( name2 ) - 1 },
        { name3, S_OK, &IID_ISWbemObject, ARRAY_SIZE( name3 ) - 1 },
        { name4, S_OK, &IID_ISWbemObject, ARRAY_SIZE( name4 ) - 1 }
    };
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

    for (i =0; i < ARRAY_SIZE( tests ); i++)
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

    hr = CreateBindCtx( 0, &ctx );
    ok( hr == S_OK, "got %x\n", hr );

    str = SysAllocString( stdregprovW );
    hr = IParseDisplayName_ParseDisplayName( displayname, NULL, str, &eaten, &moniker );
    ok( hr == S_OK, "got %x\n", hr );
    SysFreeString( str );

    if (moniker)
    {
        ISWbemObject *sobj = NULL;
        hr = IMoniker_BindToObject( moniker, ctx, NULL, &IID_ISWbemObject, (void **)&sobj );
        ok( hr == S_OK, "got %x\n",hr );
        if (sobj)
        {
            DISPID dispid = 0xdeadbeef;

            str = SysAllocString( getstringvalueW );
            hr = ISWbemObject_GetIDsOfNames( sobj, &IID_NULL, &str, 1, english, &dispid );
            ok( hr == S_OK, "got %x\n", hr );
            ok( dispid == 0x1000001, "got %x\n", dispid );

            ISWbemObject_Release( sobj );
            SysFreeString( str );
        }
        IMoniker_Release( moniker );
    }

    IBindCtx_Release(ctx);
    IParseDisplayName_Release( displayname );
}

static const WCHAR localhost[] = {'l','o','c','a','l','h','o','s','t',0};
static const WCHAR root[] = {'r','o','o','t','\\','C','I','M','V','2',0};
static const WCHAR query[] = {'S','e','l','e','c','t',' ','P','r','o','c','e','s','s','o','r','I','d',' ','f','r','o','m',
                              ' ','W','i','n','3','2','_','P','r','o','c','e','s','s','o','r',0};
static const WCHAR lang[] = {'W','Q','L',0};
static const WCHAR props[] = {'P','r','o','p','e','r','t','i','e','s','_',0};
static const WCHAR procid[] = {'P','r','o','c','e','s','s','o','r','I','d',0};

static void test_locator(void)
{
    HRESULT hr;
    DISPID id;
    BSTR host_bstr, root_bstr, query_bstr, lang_bstr, props_bstr, procid_bstr;
    ISWbemLocator *locator;
    ISWbemServices *services;
    ISWbemObjectSet *object_set;
    IEnumVARIANT *enum_var;
    ISWbemObject *object;
    ISWbemPropertySet *prop_set;
    ISWbemProperty *prop;
    ISWbemSecurity *security;
    VARIANT var;
    LONG count;
    WbemImpersonationLevelEnum imp_level;
    WbemAuthenticationLevelEnum auth_level;

    hr = CoCreateInstance( &CLSID_SWbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_ISWbemLocator, (void **)&locator );
    ok( hr == S_OK, "got %x\n", hr );

    host_bstr = SysAllocString(localhost);
    root_bstr = SysAllocString(root);
    hr = ISWbemLocator_ConnectServer( locator, host_bstr, root_bstr, NULL, NULL, NULL, NULL, 0, NULL, &services);
    ok( hr == S_OK, "got %x\n", hr );
    SysFreeString( root_bstr );
    SysFreeString( host_bstr );

    query_bstr = SysAllocString(query);
    lang_bstr = SysAllocString(lang);
    hr = ISWbemServices_ExecQuery( services, query_bstr, lang_bstr, wbemFlagForwardOnly, NULL, &object_set);
    ok( hr == S_OK, "got %x\n", hr );
    SysFreeString( lang_bstr );
    SysFreeString( query_bstr );

    hr = ISWbemLocator_get_Security_( locator, &security );
    ok( hr == S_OK, "got %x\n", hr );
    imp_level = 0xdeadbeef;
    hr = ISWbemSecurity_get_ImpersonationLevel( security, &imp_level );
    ok( hr == S_OK, "got %x\n", hr );
    ok( imp_level == wbemImpersonationLevelImpersonate, "got %u\n", imp_level );
    hr = ISWbemSecurity_put_ImpersonationLevel( security, wbemImpersonationLevelAnonymous );
    ok( hr == S_OK, "got %x\n", hr );
    imp_level = 0xdeadbeef;
    hr = ISWbemSecurity_get_ImpersonationLevel( security, &imp_level );
    ok( hr == S_OK, "got %x\n", hr );
    ok( imp_level == wbemImpersonationLevelAnonymous, "got %u\n", imp_level );

    auth_level = 0xdeadbeef;
    hr = ISWbemSecurity_get_AuthenticationLevel( security, &auth_level );
    todo_wine {
    ok( hr == WBEM_E_FAILED, "got %x\n", hr );
    ok( auth_level == 0xdeadbeef, "got %u\n", auth_level );
    }
    hr = ISWbemSecurity_put_AuthenticationLevel( security, wbemAuthenticationLevelNone );
    ok( hr == S_OK, "got %x\n", hr );
    auth_level = 0xdeadbeef;
    hr = ISWbemSecurity_get_AuthenticationLevel( security, &auth_level );
    ok( hr == S_OK, "got %x\n", hr );
    ok( auth_level == wbemAuthenticationLevelNone, "got %u\n", auth_level );
    ISWbemSecurity_Release( security );
    security = NULL;

    hr = ISWbemObjectSet_get__NewEnum( object_set, (IUnknown**)&enum_var );
    ok( hr == S_OK, "got %x\n", hr );

    VariantInit( &var );
    hr = IEnumVARIANT_Next( enum_var, 1, &var, NULL );
    ok( hr == S_OK, "got %x\n", hr );
    ok( V_VT(&var) == VT_DISPATCH, "got %x\n", V_VT(&var));

    props_bstr = SysAllocString( props );
    hr = IDispatch_GetIDsOfNames( V_DISPATCH(&var), &IID_NULL, &props_bstr, 1, english, &id );
    ok( hr == S_OK, "got %x\n", hr );
    ok( id == 21, "got %d\n", id );

    hr = IDispatch_QueryInterface( V_DISPATCH(&var), &IID_ISWbemObject, (void**)&object );
    ok( hr == S_OK, "got %x\n", hr );
    VariantClear( &var );

    hr = ISWbemObject_get_Properties_( object, &prop_set );
    ok( hr == S_OK, "got %x\n", hr );

    hr = ISWbemPropertySet_Item( prop_set, props_bstr, 0, &prop );
    ok( hr == WBEM_E_NOT_FOUND, "got %x\n", hr );
    SysFreeString( props_bstr );

    procid_bstr = SysAllocString( procid );
    hr = ISWbemPropertySet_Item( prop_set, procid_bstr, 0, &prop );
    ok( hr == S_OK, "got %x\n", hr );
    SysFreeString( procid_bstr );

    count = 0;
    hr = ISWbemPropertySet_get_Count( prop_set, &count );
    ok( hr == S_OK, "got %x\n", hr );
    ok( count > 0, "got %d\n", count );

    hr = ISWbemProperty_get_Value( prop, &var );
    ok( hr == S_OK, "got %x\n", hr );
    ok( V_VT(&var) == VT_BSTR, "got %x\n", V_VT(&var) );
    VariantClear( &var );

    hr = ISWbemServices_get_Security_( services, &security );
    ok( hr == S_OK, "got %x\n", hr );
    imp_level = 0xdeadbeef;
    hr = ISWbemSecurity_get_ImpersonationLevel( security, &imp_level );
    ok( hr == S_OK, "got %x\n", hr );
    ok( imp_level == wbemImpersonationLevelImpersonate, "got %u\n", imp_level );
    auth_level = 0xdeadbeef;
    hr = ISWbemSecurity_get_AuthenticationLevel( security, &auth_level );
    ok( hr == S_OK, "got %x\n", hr );
    ok( auth_level == wbemAuthenticationLevelPktPrivacy, "got %u\n", auth_level );

    ISWbemSecurity_Release(security);
    ISWbemProperty_Release( prop );
    ISWbemPropertySet_Release( prop_set );
    ISWbemObject_Release( object );
    IEnumVARIANT_Release( enum_var );
    ISWbemObjectSet_Release( object_set );
    ISWbemServices_Release( services );
    ISWbemLocator_Release( locator );
}

START_TEST(wbemdisp)
{
    CoInitialize( NULL );

    test_ParseDisplayName();
    test_locator();

    CoUninitialize();
}
