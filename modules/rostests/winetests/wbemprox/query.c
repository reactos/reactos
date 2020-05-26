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

#define COBJMACROS

#include <stdio.h>
#include "windows.h"
#include "ocidl.h"
#include "sddl.h"
#include "initguid.h"
#include "objidl.h"
#include "wbemcli.h"
#include "wine/test.h"

static const WCHAR wqlW[] = {'w','q','l',0};

static HRESULT exec_query( IWbemServices *services, const WCHAR *str, IEnumWbemClassObject **result )
{
    static const WCHAR captionW[] = {'C','a','p','t','i','o','n',0};
    static const WCHAR descriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
    HRESULT hr;
    IWbemClassObject *obj;
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( str );
    LONG flags = WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY;
    ULONG count;

    hr = IWbemServices_ExecQuery( services, wql, query, flags, NULL, result );
    if (hr == S_OK)
    {
        trace("%s\n", wine_dbgstr_w(str));
        for (;;)
        {
            VARIANT var;
            IWbemQualifierSet *qualifiers;
            SAFEARRAY *names;

            IEnumWbemClassObject_Next( *result, 10000, 1, &obj, &count );
            if (!count) break;

            if (IWbemClassObject_Get( obj, captionW, 0, &var, NULL, NULL ) == WBEM_S_NO_ERROR)
            {
                trace("caption: %s\n", wine_dbgstr_w(V_BSTR(&var)));
                VariantClear( &var );
            }
            if (IWbemClassObject_Get( obj, descriptionW, 0, &var, NULL, NULL ) == WBEM_S_NO_ERROR)
            {
                trace("description: %s\n", wine_dbgstr_w(V_BSTR(&var)));
                VariantClear( &var );
            }

            hr = IWbemClassObject_GetQualifierSet( obj, &qualifiers );
            ok( hr == S_OK, "got %08x\n", hr );

            hr = IWbemQualifierSet_GetNames( qualifiers, 0, &names );
            ok( hr == S_OK, "got %08x\n", hr );

            SafeArrayDestroy( names );
            IWbemQualifierSet_Release( qualifiers );
            IWbemClassObject_Release( obj );
        }
    }
    SysFreeString( wql );
    SysFreeString( query );
    return hr;
}

static void test_select( IWbemServices *services )
{
    static const WCHAR emptyW[] = {0};
    static const WCHAR sqlW[] = {'S','Q','L',0};
    static const WCHAR query1[] =
        {'S','E','L','E','C','T',' ','H','O','T','F','I','X','I','D',' ','F','R','O','M',' ',
         'W','i','n','3','2','_','Q','u','i','c','k','F','i','x','E','n','g','i','n','e','e','r','i','n','g',0};
    static const WCHAR query2[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_','B','I','O','S',0};
    static const WCHAR query3[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k',' ','W','H','E','R','E',' ',
         '\"','N','T','F','S','\"',' ','=',' ','F','i','l','e','S','y','s','t','e','m',0};
    static const WCHAR query4[] =
        {'S','E','L','E','C','T',' ','a',' ','F','R','O','M',' ','b',0};
    static const WCHAR query5[] =
        {'S','E','L','E','C','T',' ','a',' ','F','R','O','M',' ','W','i','n','3','2','_','B','i','o','s',0};
    static const WCHAR query6[] =
        {'S','E','L','E','C','T',' ','D','e','s','c','r','i','p','t','i','o','n',' ','F','R','O','M',' ',
         'W','i','n','3','2','_','B','i','o','s',0};
    static const WCHAR query7[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'P','r','o','c','e','s','s',' ','W','H','E','R','E',' ','C','a','p','t','i','o','n',' ',
         'L','I','K','E',' ','\'','%','%','R','E','G','E','D','I','T','%','\'',0};
    static const WCHAR query8[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'D','i','s','k','D','r','i','v','e',' ','W','H','E','R','E',' ','D','e','v','i','c','e','I','D','=',
         '\"','\\','\\','\\','\\','.','\\','\\','P','H','Y','S','I','C','A','L','D','R','I','V','E','0','\"',0};
    static const WCHAR query9[] =
        {'S','E','L','E','C','T','\n','a','\r','F','R','O','M','\t','b',0};
    static const WCHAR query10[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'P','r','o','c','e','s','s',' ','W','H','E','R','E',' ','C','a','p','t','i','o','n',' ',
         'L','I','K','E',' ','"','%','f','i','r','e','f','o','x','.','e','x','e','"',0};
    static const WCHAR query11[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         'W','i','n','3','2','_','V','i','d','e','o','C','o','n','t','r','o','l','l','e','r',' ','w','h','e','r','e',' ',
         'a','v','a','i','l','a','b','i','l','i','t','y',' ','=',' ','\'','3','\'',0};
    static const WCHAR query12[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_','B','I','O','S',
         ' ','W','H','E','R','E',' ','N','A','M','E',' ','<','>',' ','N','U','L','L', 0};
    static const WCHAR query13[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_','B','I','O','S',
         ' ','W','H','E','R','E',' ','N','U','L','L',' ','=',' ','N','A','M','E', 0};
    static const WCHAR query14[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','T','o','P','a','r','t','i','t','i','o','n',0};
    static const WCHAR query15[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'D','i','s','k','D','r','i','v','e','T','o','D','i','s','k','P','a','r','t','i','t','i','o','n',0};
    static const WCHAR *test[] = { query1, query2, query3, query4, query5, query6, query7, query8, query9, query10,
                                   query11, query12, query13, query14, query15 };
    HRESULT hr;
    IEnumWbemClassObject *result;
    BSTR wql = SysAllocString( wqlW );
    BSTR sql = SysAllocString( sqlW );
    BSTR query = SysAllocString( query1 );
    UINT i;

    hr = IWbemServices_ExecQuery( services, NULL, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, NULL, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, wql, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, sql, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_QUERY_TYPE, "query failed %08x\n", hr );

    hr = IWbemServices_ExecQuery( services, sql, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    SysFreeString( query );
    query = SysAllocString( emptyW );
    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %08x\n", hr );

    for (i = 0; i < ARRAY_SIZE( test ); i++)
    {
        hr = exec_query( services, test[i], &result );
        ok( hr == S_OK, "query %u failed: %08x\n", i, hr );
        if (result) IEnumWbemClassObject_Release( result );
    }

    SysFreeString( wql );
    SysFreeString( sql );
    SysFreeString( query );
}

static void test_associators( IWbemServices *services )
{
    static const WCHAR query1[] =
        {'A','S','S','O','C','I','A','T','O','R','S',' ',' ','O','F','{','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','D','=','"','C',':','"','}',0};
    static const WCHAR query2[] =
        {'A','S','S','O','C','I','A','T','O','R','S',' ','O','F',' ','{','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','D','=','"','C',':','"','}',' ',
         'W','H','E','R','E',' ','A','s','s','o','c','C','l','a','s','s','=','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','T','o','P','a','r','t','i','t','i','o','n',0};
    static const WCHAR query3[] =
        {'A','S','S','O','C','I','A','T','O','R','S',' ',' ','O','F',' ','{','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','D','}',0};
    static const WCHAR query4[] =
        {'A','S','S','O','C','I','A','T','O','R','S',' ',' ','O','F',' ','{','W','i','n','3','2','_',
         'D','i','s','k','D','r','i','v','e','.','D','e','v','i','c','e','I','D','=',
         '\'','\\','\\','.','\\','P','H','Y','S','I','C','A','L','D','R','I','V','E','0','\'','}',0};
    static const WCHAR query5[] =
        {'A','S','S','O','C','I','A','T','O','R','S',' ','O','F',' ','{','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','D','=','"','C',':','"','}',' ',
         'W','H','E','R','E',' ','A','s','s','o','c','C','l','a','s','s','=','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','T','o','P','a','r','t','i','t','i','o','n',' ',
         'C','l','a','s','s','D','e','f','s','O','n','l','y',0};
    static const WCHAR query6[] =
        {'A','S','S','O','C','I','A','T','O','R','S',' ','O','F',' ','{','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','D','=','"','C',':','"','}',' ',
         'W','H','E','R','E',' ','C','l','a','s','s','D','e','f','s','O','n','l','y',0};
    static const WCHAR query7[] =
        {'A','S','S','O','C','I','A','T','O','R','S',' ','O','F',' ','{','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','.','D','e','v','i','c','e','I','D','=','"','C',':','"','}',' ',
         'W','H','E','R','E',' ','C','l','a','s','s','D','e','f','s','O','n','l','y',' ',
         'A','s','s','o','c','C','l','a','s','s',' ','=',' ','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k','T','o','P','a','r','t','i','t','i','o','n',0};
    static const WCHAR *test[] = { query1, query2, query3, query4, query5, query6, query7 };
    HRESULT hr;
    IEnumWbemClassObject *result;
    UINT i;

    for (i = 0; i < ARRAY_SIZE( test ); i++)
    {
        hr = exec_query( services, test[i], &result );
        ok( hr == S_OK, "query %u failed: %08x\n", i, hr );
        if (result) IEnumWbemClassObject_Release( result );
    }
}

static void _check_property( ULONG line, IWbemClassObject *obj, const WCHAR *prop, VARTYPE vartype, CIMTYPE cimtype )
{
    CIMTYPE type = 0xdeadbeef;
    VARIANT val;
    HRESULT hr;

    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, prop, 0, &val, &type, NULL );
    ok( hr == S_OK, "%u: failed to get description %08x\n", line, hr );
    ok( V_VT( &val ) == vartype, "%u: unexpected variant type 0x%x\n", line, V_VT(&val) );
    ok( type == cimtype, "%u: unexpected type 0x%x\n", line, type );
    switch (V_VT(&val))
    {
    case VT_BSTR:
        trace( "%s: %s\n", wine_dbgstr_w(prop), wine_dbgstr_w(V_BSTR(&val)) );
        break;
    case VT_I2:
        trace( "%s: %d\n", wine_dbgstr_w(prop), V_I2(&val) );
        break;
    case VT_I4:
        trace( "%s: %d\n", wine_dbgstr_w(prop), V_I4(&val) );
        break;
    case VT_R4:
        trace( "%s: %f\n", wine_dbgstr_w(prop), V_R4(&val) );
        break;
    default:
        break;
    }
    VariantClear( &val );
}
#define check_property(a,b,c,d) _check_property(__LINE__,a,b,c,d)

static void test_Win32_Service( IWbemServices *services )
{
    static const WCHAR pauseserviceW[] = {'P','a','u','s','e','S','e','r','v','i','c','e',0};
    static const WCHAR processidW[] = {'P','r','o','c','e','s','s','I','D',0};
    static const WCHAR resumeserviceW[] = {'R','e','s','u','m','e','S','e','r','v','i','c','e',0};
    static const WCHAR returnvalueW[] = {'R','e','t','u','r','n','V','a','l','u','e',0};
    static const WCHAR serviceW[] = {'W','i','n','3','2','_','S','e','r','v','i','c','e','.',
        'N','a','m','e','=','"','S','p','o','o','l','e','r','"',0};
    static const WCHAR startserviceW[] = {'S','t','a','r','t','S','e','r','v','i','c','e',0};
    static const WCHAR stateW[] = {'S','t','a','t','e',0};
    static const WCHAR stoppedW[] = {'S','t','o','p','p','e','d',0};
    static const WCHAR stopserviceW[] = {'S','t','o','p','S','e','r','v','i','c','e',0};
    static const WCHAR emptyW[] = {0};
    BSTR class = SysAllocString( serviceW ), empty = SysAllocString( emptyW ), method;
    IWbemClassObject *service, *out;
    VARIANT state, retval;
    CIMTYPE type;
    HRESULT hr;

    hr = IWbemServices_GetObject( services, class, 0, NULL, &service, NULL );
    if (hr != S_OK)
    {
        win_skip( "Win32_Service not available\n" );
        goto out;
    }

    check_property( service, processidW, VT_I4, CIM_UINT32 );
    type = 0xdeadbeef;
    VariantInit( &state );
    hr = IWbemClassObject_Get( service, stateW, 0, &state, &type, NULL );
    ok( hr == S_OK, "failed to get service state %08x\n", hr );
    ok( V_VT( &state ) == VT_BSTR, "unexpected variant type 0x%x\n", V_VT( &state ) );
    ok( type == CIM_STRING, "unexpected type 0x%x\n", type );

    if (!lstrcmpW( V_BSTR( &state ), stoppedW ))
    {
        out = NULL;
        method = SysAllocString( startserviceW );
        hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
        ok( hr == S_OK, "failed to execute method %08x\n", hr );
        SysFreeString( method );

        VariantInit( &retval );
        hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, NULL, NULL );
        ok( hr == S_OK, "failed to get return value %08x\n", hr );
        ok( !V_I4( &retval ), "unexpected error %u\n", V_UI4( &retval ) );
        IWbemClassObject_Release( out );
    }
    out = NULL;
    method = SysAllocString( pauseserviceW );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
    ok( hr == S_OK, "failed to execute method %08x\n", hr );
    SysFreeString( method );

    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, NULL, NULL );
    ok( hr == S_OK, "failed to get return value %08x\n", hr );
    ok( V_I4( &retval ), "unexpected success\n" );
    IWbemClassObject_Release( out );

    out = NULL;
    method = SysAllocString( resumeserviceW );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
    ok( hr == S_OK, "failed to execute method %08x\n", hr );
    SysFreeString( method );

    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, NULL, NULL );
    ok( hr == S_OK, "failed to get return value %08x\n", hr );
    ok( V_I4( &retval ), "unexpected success\n" );
    IWbemClassObject_Release( out );

    if (!lstrcmpW( V_BSTR( &state ), stoppedW ))
    {
        out = NULL;
        method = SysAllocString( stopserviceW );
        hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
        ok( hr == S_OK, "failed to execute method %08x\n", hr );
        SysFreeString( method );

        VariantInit( &retval );
        hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, NULL, NULL );
        ok( hr == S_OK, "failed to get return value %08x\n", hr );
        ok( !V_I4( &retval ), "unexpected error %u\n", V_UI4( &retval ) );
        IWbemClassObject_Release( out );
    }
    VariantClear( &state );
    IWbemClassObject_Release( service );

    service = NULL;
    hr = IWbemServices_GetObject( services, NULL, 0, NULL, &service, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (service) IWbemClassObject_Release( service );

    service = NULL;
    hr = IWbemServices_GetObject( services, empty, 0, NULL, &service, NULL );
    ok( hr == S_OK, "got %08x\n", hr );
    if (service) IWbemClassObject_Release( service );

out:
    SysFreeString( empty );
    SysFreeString( class );
}

static void test_Win32_Bios( IWbemServices *services )
{
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_','B','i','o','s',0};
    static const WCHAR descriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR identificationcodeW[] = {'I','d','e','n','t','i','f','i','c','a','t','i','o','n','C','o','d','e',0};
    static const WCHAR manufacturerW[] = {'M','a','n','u','f','a','c','t','u','r','e','r',0};
    static const WCHAR nameW[] = {'N','a','m','e',0};
    static const WCHAR releasedateW[] = {'R','e','l','e','a','s','e','D','a','t','e',0};
    static const WCHAR serialnumberW[] = {'S','e','r','i','a','l','N','u','m','b','e','r',0};
    static const WCHAR smbiosbiosversionW[] = {'S','M','B','I','O','S','B','I','O','S','V','e','r','s','i','o','n',0};
    static const WCHAR smbiosmajorversionW[] = {'S','M','B','I','O','S','M','a','j','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR smbiosminorversionW[] = {'S','M','B','I','O','S','M','i','n','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR versionW[] = {'V','e','r','s','i','o','n',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    CIMTYPE type;
    ULONG count;
    VARIANT val;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "IWbemServices_ExecQuery failed %08x\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "IEnumWbemClassObject_Next failed %08x\n", hr );

    check_property( obj, descriptionW, VT_BSTR, CIM_STRING );
    check_property( obj, identificationcodeW, VT_NULL, CIM_STRING );
    check_property( obj, manufacturerW, VT_BSTR, CIM_STRING );
    check_property( obj, nameW, VT_BSTR, CIM_STRING );
    check_property( obj, releasedateW, VT_BSTR, CIM_DATETIME );

    type = 0xdeadbeef;
    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, serialnumberW, 0, &val, &type, NULL );
    ok( hr == S_OK, "failed to get serial number %08x\n", hr );
    ok( V_VT( &val ) == VT_BSTR || V_VT( &val ) == VT_NULL /* Testbot VMs */,
        "unexpected variant type 0x%x\n", V_VT( &val ) );
    ok( type == CIM_STRING, "unexpected type 0x%x\n", type );
    VariantClear( &val );

    check_property( obj, smbiosbiosversionW, VT_BSTR, CIM_STRING );
    check_property( obj, smbiosmajorversionW, VT_I4, CIM_UINT16 );
    check_property( obj, smbiosminorversionW, VT_I4, CIM_UINT16 );
    check_property( obj, versionW, VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Baseboard( IWbemServices *services )
{
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_','B','a','s','e','b','o','a','r','d',0};
    static const WCHAR manufacturerW[] = {'M','a','n','u','f','a','c','t','u','r','e','r',0};
    static const WCHAR modelW[] = {'M','o','d','e','l',0};
    static const WCHAR nameW[] = {'N','a','m','e',0};
    static const WCHAR productW[] = {'P','r','o','d','u','c','t',0};
    static const WCHAR tagW[] = {'T','a','g',0};
    static const WCHAR versionW[] = {'V','e','r','s','i','o','n',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    CIMTYPE type;
    ULONG count;
    VARIANT val;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "IWbemServices_ExecQuery failed %08x\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    if (hr != S_OK)
    {
        win_skip( "Win32_Baseboard not available\n" );
        return;
    }
    ok( hr == S_OK, "IEnumWbemClassObject_Next failed %08x\n", hr );

    check_property( obj, manufacturerW, VT_BSTR, CIM_STRING );

    type = 0xdeadbeef;
    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, modelW, 0, &val, &type, NULL );
    ok( hr == S_OK, "failed to get model %08x\n", hr );
    ok( V_VT( &val ) == VT_BSTR || V_VT( &val ) == VT_NULL, "unexpected variant type 0x%x\n", V_VT( &val ) );
    ok( type == CIM_STRING, "unexpected type 0x%x\n", type );
    trace( "model: %s\n", wine_dbgstr_w(V_BSTR(&val)) );
    VariantClear( &val );

    check_property( obj, nameW, VT_BSTR, CIM_STRING );
    check_property( obj, productW, VT_BSTR, CIM_STRING );
    check_property( obj, tagW, VT_BSTR, CIM_STRING );
    check_property( obj, versionW, VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Process( IWbemServices *services, BOOL use_full_path )
{
    static const WCHAR returnvalueW[] = {'R','e','t','u','r','n','V','a','l','u','e',0};
    static const WCHAR getownerW[] = {'G','e','t','O','w','n','e','r',0};
    static const WCHAR userW[] = {'U','s','e','r',0};
    static const WCHAR domainW[] = {'D','o','m','a','i','n',0};
    static const WCHAR processW[] = {'W','i','n','3','2','_','P','r','o','c','e','s','s',0};
    static const WCHAR idW[] = {'I','D',0};
    static const WCHAR fmtW[] = {'W','i','n','3','2','_','P','r','o','c','e','s','s','.',
        'H','a','n','d','l','e','=','"','%','u','"',0};
    static const WCHAR full_path_fmt[] =
        {'\\','\\','%','s','\\','R','O','O','T','\\','C','I','M','V','2',':',0};
    static const LONG expected_flavor = WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
                                        WBEM_FLAVOR_NOT_OVERRIDABLE |
                                        WBEM_FLAVOR_ORIGIN_PROPAGATED;
    WCHAR full_path[MAX_COMPUTERNAME_LENGTH + ARRAY_SIZE(full_path_fmt)];
    BSTR class, method;
    IWbemClassObject *process, *sig_in, *out;
    IWbemQualifierSet *qualifiers;
    VARIANT retval, val;
    DWORD full_path_len = 0;
    LONG flavor;
    CIMTYPE type;
    HRESULT hr;

    if (use_full_path)
    {
        WCHAR server[MAX_COMPUTERNAME_LENGTH+1];

        full_path_len = ARRAY_SIZE(server);
        ok( GetComputerNameW(server, &full_path_len), "GetComputerName failed\n" );
        full_path_len = wsprintfW(full_path, full_path_fmt, server);
    }

    class = SysAllocStringLen( NULL, full_path_len + ARRAY_SIZE( processW ) );
    memcpy( class, full_path, full_path_len * sizeof(WCHAR) );
    memcpy( class + full_path_len, processW, sizeof(processW) );
    hr = IWbemServices_GetObject( services, class, 0, NULL, &process, NULL );
    SysFreeString( class );
    if (hr != S_OK)
    {
        win_skip( "Win32_Process not available\n" );
        return;
    }
    sig_in = (void*)0xdeadbeef;
    hr = IWbemClassObject_GetMethod( process, getownerW, 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get GetOwner method %08x\n", hr );
    ok( !sig_in, "sig_in != NULL\n");
    IWbemClassObject_Release( process );

    out = NULL;
    method = SysAllocString( getownerW );
    class = SysAllocStringLen( NULL, full_path_len + ARRAY_SIZE( fmtW ) + 10 );
    memcpy( class, full_path, full_path_len * sizeof(WCHAR) );
    wsprintfW( class + full_path_len, fmtW, GetCurrentProcessId() );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
    ok( hr == S_OK, "failed to execute method %08x\n", hr );
    SysFreeString( method );
    SysFreeString( class );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %08x\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %u\n", V_I4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );

    check_property( out, userW, VT_BSTR, CIM_STRING );
    check_property( out, domainW, VT_BSTR, CIM_STRING );

    hr = IWbemClassObject_GetPropertyQualifierSet( out, userW, &qualifiers );
    ok( hr == S_OK, "failed to get qualifier set %08x\n", hr );

    flavor = -1;
    V_I4(&val) = -1;
    V_VT(&val) = VT_ERROR;
    hr = IWbemQualifierSet_Get( qualifiers, idW, 0, &val, &flavor );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flavor == expected_flavor, "got %d\n", flavor );
    ok( V_VT(&val) == VT_I4, "got %u\n", V_VT(&val) );
    ok( V_I4(&val) == 0, "got %u\n", V_I4(&val) );
    VariantClear( &val );

    IWbemQualifierSet_Release( qualifiers );
    hr = IWbemClassObject_GetPropertyQualifierSet( out, domainW, &qualifiers );
    ok( hr == S_OK, "failed to get qualifier set %08x\n", hr );

    flavor = -1;
    V_I4(&val) = -1;
    V_VT(&val) = VT_ERROR;
    hr = IWbemQualifierSet_Get( qualifiers, idW, 0, &val, &flavor );
    ok( hr == S_OK, "got %08x\n", hr );
    ok( flavor == expected_flavor, "got %d\n", flavor );
    ok( V_VT(&val) == VT_I4, "got %u\n", V_VT(&val) );
    ok( V_I4(&val) == 1, "got %u\n", V_I4(&val) );
    VariantClear( &val );

    IWbemQualifierSet_Release( qualifiers );
    hr = IWbemClassObject_GetPropertyQualifierSet( out, returnvalueW, &qualifiers );
    ok( hr == S_OK, "failed to get qualifier set %08x\n", hr );

    hr = IWbemQualifierSet_Get( qualifiers, idW, 0, &val, &flavor );
    ok( hr == WBEM_E_NOT_FOUND, "got %08x\n", hr );

    IWbemQualifierSet_Release( qualifiers );
    IWbemClassObject_Release( out );
}

static void test_Win32_ComputerSystem( IWbemServices *services )
{
    static const WCHAR backslashW[] = {'\\',0};
    static const WCHAR memorytypeW[] = {'M','e','m','o','r','y','T','y','p','e',0};
    static const WCHAR modelW[] = {'M','o','d','e','l',0};
    static const WCHAR nameW[] = {'N','a','m','e',0};
    static const WCHAR usernameW[] = {'U','s','e','r','N','a','m','e',0};
    static const WCHAR numprocessorsW[] =
        {'N','u','m','b','e','r','O','f','P','r','o','c','e','s','s','o','r','s',0};
    static const WCHAR numlogicalprocessorsW[] =
        {'N','u','m','b','e','r','O','f','L','o','g','i','c','a','l','P','r','o','c','e','s','s','o','r','s',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'C','o','m','p','u','t','e','r','S','y','s','t','e','m',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    VARIANT value;
    CIMTYPE type;
    HRESULT hr;
    WCHAR compname[MAX_COMPUTERNAME_LENGTH + 1];
    WCHAR username[128];
    DWORD len, count;

    len = ARRAY_SIZE( compname );
    if (!GetComputerNameW( compname, &len ))
        compname[0] = 0;

    lstrcpyW( username, compname );
    lstrcatW( username, backslashW );
    len = ARRAY_SIZE( username ) - lstrlenW( username );
    if (!GetUserNameW( username + lstrlenW( username ), &len ))
        username[0] = 0;

    if (!compname[0] || !username[0])
    {
        skip( "Failed to get user or computer name\n" );
        goto out;
    }

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_ComputerSystem not available\n" );
        goto out;
    }

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "got %08x\n", hr );

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, memorytypeW, 0, &value, &type, NULL );
    ok( hr == WBEM_E_NOT_FOUND, "got %08x\n", hr );

    check_property( obj, modelW, VT_BSTR, CIM_STRING );

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, nameW, 0, &value, &type, NULL );
    ok( hr == S_OK, "failed to get computer name %08x\n", hr );
    ok( V_VT( &value ) == VT_BSTR, "unexpected variant type 0x%x\n", V_VT( &value ) );
    ok( type == CIM_STRING, "unexpected type 0x%x\n", type );
    ok( !lstrcmpiW( V_BSTR( &value ), compname ), "got %s, expected %s\n", wine_dbgstr_w(V_BSTR(&value)), wine_dbgstr_w(compname) );
    VariantClear( &value );

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, numlogicalprocessorsW, 0, &value, &type, NULL );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* win2k3 */, "got %08x\n", hr );
    if (hr == S_OK)
    {
        ok( V_VT( &value ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &value ) );
        ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );
        trace( "numlogicalprocessors %u\n", V_I4( &value ) );
    }

    check_property( obj, numprocessorsW, VT_I4, CIM_UINT32 );

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, usernameW, 0, &value, &type, NULL );
    ok( hr == S_OK, "failed to get computer name %08x\n", hr );
    ok( V_VT( &value ) == VT_BSTR, "unexpected variant type 0x%x\n", V_VT( &value ) );
    ok( type == CIM_STRING, "unexpected type 0x%x\n", type );
    ok( !lstrcmpiW( V_BSTR( &value ), username ), "got %s, expected %s\n", wine_dbgstr_w(V_BSTR(&value)), wine_dbgstr_w(username) );
    VariantClear( &value );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
out:
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_SystemEnclosure( IWbemServices *services )
{
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'S','y','s','t','e','m','E','n','c','l','o','s','u','r','e',0};
    static const WCHAR captionW[] = {'C','a','p','t','i','o','n',0};
    static const WCHAR chassistypesW[] = {'C','h','a','s','s','i','s','T','y','p','e','s',0};
    static const WCHAR descriptionW[] = {'D','e','s','c','r','i','p','t','i','o','n',0};
    static const WCHAR lockpresentW[] = {'L','o','c','k','P','r','e','s','e','n','t',0};
    static const WCHAR manufacturerW[] = {'M','a','n','u','f','a','c','t','u','r','e','r',0};
    static const WCHAR nameW[] = {'N','a','m','e',0};
    static const WCHAR tagW[] = {'T','a','g',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    CIMTYPE type;
    ULONG count;
    VARIANT val;
    DWORD *data;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "IWbemServices_ExecQuery failed %08x\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "IEnumWbemClassObject_Next failed %08x\n", hr );

    check_property( obj, captionW, VT_BSTR, CIM_STRING );

    type = 0xdeadbeef;
    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, chassistypesW, 0, &val, &type, NULL );
    ok( hr == S_OK, "failed to get chassis types %08x\n", hr );
    ok( V_VT( &val ) == (VT_I4|VT_ARRAY), "unexpected variant type 0x%x\n", V_VT( &val ) );
    ok( type == (CIM_UINT16|CIM_FLAG_ARRAY), "unexpected type 0x%x\n", type );
    hr = SafeArrayAccessData( V_ARRAY( &val ), (void **)&data );
    ok( hr == S_OK, "SafeArrayAccessData failed %x\n", hr );
    if (SUCCEEDED(hr))
    {
        LONG i, lower, upper;

        hr = SafeArrayGetLBound( V_ARRAY( &val ), 1, &lower );
        ok( hr == S_OK, "SafeArrayGetLBound failed %x\n", hr );
        hr = SafeArrayGetUBound( V_ARRAY( &val ), 1, &upper );
        ok( hr == S_OK, "SafeArrayGetUBound failed %x\n", hr );
        if (V_VT( &val ) == (VT_I4|VT_ARRAY))
        {
            for (i = 0; i < upper - lower + 1; i++)
                trace( "chassis type: %u\n", data[i] );
        }
        hr = SafeArrayUnaccessData( V_ARRAY( &val ) );
        ok( hr == S_OK, "SafeArrayUnaccessData failed %x\n", hr );
    }
    VariantClear( &val );

    check_property( obj, descriptionW, VT_BSTR, CIM_STRING );
    check_property( obj, lockpresentW, VT_BOOL, CIM_BOOLEAN );
    check_property( obj, manufacturerW, VT_BSTR, CIM_STRING );
    check_property( obj, nameW, VT_BSTR, CIM_STRING );
    check_property( obj, tagW, VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_StdRegProv( IWbemServices *services )
{
    static const WCHAR createkeyW[] = {'C','r','e','a','t','e','K','e','y',0};
    static const WCHAR enumkeyW[] = {'E','n','u','m','K','e','y',0};
    static const WCHAR enumvaluesW[] = {'E','n','u','m','V','a','l','u','e','s',0};
    static const WCHAR getstringvalueW[] = {'G','e','t','S','t','r','i','n','g','V','a','l','u','e',0};
    static const WCHAR stdregprovW[] = {'S','t','d','R','e','g','P','r','o','v',0};
    static const WCHAR defkeyW[] = {'h','D','e','f','K','e','y',0};
    static const WCHAR subkeynameW[] = {'s','S','u','b','K','e','y','N','a','m','e',0};
    static const WCHAR returnvalueW[] = {'R','e','t','u','r','n','V','a','l','u','e',0};
    static const WCHAR namesW[] = {'s','N','a','m','e','s',0};
    static const WCHAR typesW[] = {'T','y','p','e','s',0};
    static const WCHAR valueW[] = {'s','V','a','l','u','e',0};
    static const WCHAR valuenameW[] = {'s','V','a','l','u','e','N','a','m','e',0};
    static const WCHAR programfilesW[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',0};
    static const WCHAR windowsW[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    static const WCHAR regtestW[] =
        {'S','o','f','t','w','a','r','e','\\','S','t','d','R','e','g','P','r','o','v','T','e','s','t',0};
    BSTR class = SysAllocString( stdregprovW ), method, name;
    IWbemClassObject *reg, *sig_in, *sig_out, *in, *out;
    VARIANT defkey, subkey, retval, valuename;
    CIMTYPE type;
    HRESULT hr;
    LONG res;

    hr = IWbemServices_GetObject( services, class, 0, NULL, &reg, NULL );
    if (hr != S_OK)
    {
        win_skip( "StdRegProv not available\n" );
        return;
    }

    hr = IWbemClassObject_BeginMethodEnumeration( reg, 0 );
    ok( hr == S_OK, "got %08x\n", hr );

    while (IWbemClassObject_NextMethod( reg, 0, &name, &sig_in, &sig_out ) == S_OK)
    {
        SysFreeString( name );
        IWbemClassObject_Release( sig_in );
        IWbemClassObject_Release( sig_out );
    }

    hr = IWbemClassObject_EndMethodEnumeration( reg );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWbemClassObject_BeginEnumeration( reg, 0 );
    ok( hr == S_OK, "got %08x\n", hr );

    while (IWbemClassObject_Next( reg, 0, &name, NULL, NULL, NULL ) == S_OK)
        SysFreeString( name );

    hr = IWbemClassObject_EndEnumeration( reg );
    ok( hr == S_OK, "got %08x\n", hr );

    hr = IWbemClassObject_GetMethod( reg, createkeyW, 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get CreateKey method %08x\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %08x\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000001;
    hr = IWbemClassObject_Put( in, defkeyW, 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %08x\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( regtestW );
    hr = IWbemClassObject_Put( in, subkeynameW, 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %08x\n", hr );

    out = NULL;
    method = SysAllocString( createkeyW );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %08x\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %08x\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %u\n", V_UI4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );

    res = RegDeleteKeyW( HKEY_CURRENT_USER, regtestW );
    ok( !res, "got %d\n", res );

    VariantClear( &subkey );
    IWbemClassObject_Release( in );
    IWbemClassObject_Release( out );
    IWbemClassObject_Release( sig_in );

    hr = IWbemClassObject_GetMethod( reg, enumkeyW, 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get EnumKey method %08x\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %08x\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000002;
    hr = IWbemClassObject_Put( in, defkeyW, 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %08x\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( windowsW );
    hr = IWbemClassObject_Put( in, subkeynameW, 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %08x\n", hr );

    out = NULL;
    method = SysAllocString( enumkeyW );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %08x\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %08x\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %u\n", V_UI4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );

    check_property( out, namesW, VT_BSTR|VT_ARRAY, CIM_STRING|CIM_FLAG_ARRAY );

    VariantClear( &subkey );
    IWbemClassObject_Release( in );
    IWbemClassObject_Release( out );
    IWbemClassObject_Release( sig_in );

    hr = IWbemClassObject_GetMethod( reg, enumvaluesW, 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get EnumValues method %08x\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %08x\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000002;
    hr = IWbemClassObject_Put( in, defkeyW, 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %08x\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( windowsW );
    hr = IWbemClassObject_Put( in, subkeynameW, 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %08x\n", hr );

    out = NULL;
    method = SysAllocString( enumvaluesW );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %08x\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %08x\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %u\n", V_I4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );

    check_property( out, namesW, VT_BSTR|VT_ARRAY, CIM_STRING|CIM_FLAG_ARRAY );
    check_property( out, typesW, VT_I4|VT_ARRAY, CIM_SINT32|CIM_FLAG_ARRAY );

    VariantClear( &subkey );
    IWbemClassObject_Release( in );
    IWbemClassObject_Release( out );
    IWbemClassObject_Release( sig_in );

    hr = IWbemClassObject_GetMethod( reg, getstringvalueW, 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get GetStringValue method %08x\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %08x\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000002;
    hr = IWbemClassObject_Put( in, defkeyW, 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %08x\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( windowsW );
    hr = IWbemClassObject_Put( in, subkeynameW, 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %08x\n", hr );

    V_VT( &valuename ) = VT_BSTR;
    V_BSTR( &valuename ) = SysAllocString( programfilesW );
    hr = IWbemClassObject_Put( in, valuenameW, 0, &valuename, 0 );
    ok( hr == S_OK, "failed to set value name %08x\n", hr );

    out = NULL;
    method = SysAllocString( getstringvalueW );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %08x\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %08x\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %u\n", V_I4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );

    check_property( out, valueW, VT_BSTR, CIM_STRING );

    VariantClear( &valuename );
    VariantClear( &subkey );
    IWbemClassObject_Release( in );
    IWbemClassObject_Release( out );
    IWbemClassObject_Release( sig_in );

    IWbemClassObject_Release( reg );
    SysFreeString( class );
}

static HRESULT WINAPI sink_QueryInterface(
    IWbemObjectSink *iface, REFIID riid, void **ppv )
{
    *ppv = NULL;
    if (IsEqualGUID( &IID_IUnknown, riid ) || IsEqualGUID( &IID_IWbemObjectSink, riid ))
    {
        *ppv = iface;
        IWbemObjectSink_AddRef( iface );
        return S_OK;
    }
    return E_NOINTERFACE;
}

static ULONG sink_refs;

static ULONG WINAPI sink_AddRef(
    IWbemObjectSink *iface )
{
    return ++sink_refs;
}

static ULONG WINAPI sink_Release(
    IWbemObjectSink *iface )
{
    return --sink_refs;
}

static HRESULT WINAPI sink_Indicate(
    IWbemObjectSink *iface, LONG count, IWbemClassObject **objects )
{
    trace("Indicate: %d, %p\n", count, objects);
    return S_OK;
}

static HRESULT WINAPI sink_SetStatus(
    IWbemObjectSink *iface, LONG flags, HRESULT hresult, BSTR str_param, IWbemClassObject *obj_param )
{
    trace("SetStatus: %08x, %08x, %s, %p\n", flags, hresult, wine_dbgstr_w(str_param), obj_param);
    return S_OK;
}

static IWbemObjectSinkVtbl sink_vtbl =
{
    sink_QueryInterface,
    sink_AddRef,
    sink_Release,
    sink_Indicate,
    sink_SetStatus
};

static IWbemObjectSink sink = { &sink_vtbl };

static void test_notification_query_async( IWbemServices *services )
{
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'D','e','v','i','c','e','C','h','a','n','g','e','E','v','e','n','t',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    ULONG prev_sink_refs;
    HRESULT hr;

    hr = IWbemServices_ExecNotificationQueryAsync( services, wql, query, 0, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    prev_sink_refs = sink_refs;
    hr = IWbemServices_ExecNotificationQueryAsync( services, wql, query, 0, NULL, &sink );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND), "got %08x\n", hr );
    ok( sink_refs > prev_sink_refs || broken(!sink_refs), "got %u refs\n", sink_refs );

    hr =  IWbemServices_CancelAsyncCall( services, &sink );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND), "got %08x\n", hr );

    SysFreeString( wql );
    SysFreeString( query );
}

static void test_query_async( IWbemServices *services )
{
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'P','r','o','c','e','s','s',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    HRESULT hr;

    hr = IWbemServices_ExecQueryAsync( services, wql, query, 0, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr = IWbemServices_ExecQueryAsync( services, wql, query, 0, NULL, &sink );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND), "got %08x\n", hr );

    hr =  IWbemServices_CancelAsyncCall( services, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %08x\n", hr );

    hr =  IWbemServices_CancelAsyncCall( services, &sink );
    ok( hr == S_OK, "got %08x\n", hr );

    SysFreeString( wql );
    SysFreeString( query );
}

static void test_query_semisync( IWbemServices *services )
{
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_','D','u','m','m','y',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    ULONG count;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
            NULL, &result );
    ok( hr == S_OK, "got %08x\n", hr );

    count = 1;
    obj = (void *)0xdeadbeef;
    hr = IEnumWbemClassObject_Next( result, -1, 1, &obj, &count );
todo_wine
    ok( hr == WBEM_E_INVALID_CLASS, "Unexpected hr %#x.\n", hr );
    ok( count == 0, "Unexpected count %u.\n", count );
    ok( obj == (void *)0xdeadbeef, "Got object %p\n", obj );

    IEnumWbemClassObject_Release( result );

    SysFreeString( wql );
    SysFreeString( query );
}

static void test_GetNames( IWbemServices *services )
{
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %08x\n", hr );

    for (;;)
    {
        IWbemClassObject *obj;
        SAFEARRAY *names;
        ULONG count;
        VARIANT val;

        IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (!count) break;

        VariantInit( &val );
        hr = IWbemClassObject_GetNames( obj, NULL, WBEM_FLAG_NONSYSTEM_ONLY, &val, &names );
        ok( hr == S_OK, "got %08x\n", hr );

        SafeArrayDestroy( names );
        IWbemClassObject_Release( obj );
    }
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_SystemSecurity( IWbemServices *services )
{
    static const WCHAR systemsecurityW[] = {'_','_','S','y','s','t','e','m','S','e','c','u','r','i','t','y',0};
    static const WCHAR getsdW[] = {'G','e','t','S','D',0};
    static const WCHAR returnvalueW[] = {'R','e','t','u','r','n','V','a','l','u','e',0};
    static const WCHAR sdW[] = {'S','D',0};
    BSTR class = SysAllocString( systemsecurityW ), method;
    IWbemClassObject *reg, *out;
    VARIANT retval, var_sd;
    void *data;
    SECURITY_DESCRIPTOR_RELATIVE *sd;
    CIMTYPE type;
    HRESULT hr;
    BYTE sid_admin_buffer[SECURITY_MAX_SID_SIZE];
    SID *sid_admin = (SID*)sid_admin_buffer;
    DWORD sid_size;
    BOOL ret;

    hr = IWbemServices_GetObject( services, class, 0, NULL, &reg, NULL );
    if (hr != S_OK)
    {
        win_skip( "__SystemSecurity not available\n" );
        return;
    }
    IWbemClassObject_Release( reg );

    sid_size = sizeof(sid_admin_buffer);
    ret = CreateWellKnownSid( WinBuiltinAdministratorsSid, NULL, sid_admin, &sid_size );
    ok( ret, "CreateWellKnownSid failed\n" );

    out = NULL;
    method = SysAllocString( getsdW );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
    ok( hr == S_OK || hr == WBEM_E_ACCESS_DENIED, "failed to execute method %08x\n", hr );
    SysFreeString( method );

    if (SUCCEEDED(hr))
    {
        type = 0xdeadbeef;
        VariantInit( &retval );
        hr = IWbemClassObject_Get( out, returnvalueW, 0, &retval, &type, NULL );
        ok( hr == S_OK, "failed to get return value %08x\n", hr );
        ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
        ok( !V_I4( &retval ), "unexpected error %u\n", V_UI4( &retval ) );
        ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );

        type = 0xdeadbeef;
        VariantInit( &var_sd );
        hr = IWbemClassObject_Get( out, sdW, 0, &var_sd, &type, NULL );
        ok( hr == S_OK, "failed to get names %08x\n", hr );
        ok( V_VT( &var_sd ) == (VT_UI1|VT_ARRAY), "unexpected variant type 0x%x\n", V_VT( &var_sd ) );
        ok( type == (CIM_UINT8|CIM_FLAG_ARRAY), "unexpected type 0x%x\n", type );

        hr = SafeArrayAccessData( V_ARRAY( &var_sd ), &data );
        ok( hr == S_OK, "SafeArrayAccessData failed %x\n", hr );
        if (SUCCEEDED(hr))
        {
            sd = data;

            ok( (sd->Control & SE_SELF_RELATIVE) == SE_SELF_RELATIVE, "relative flag unset\n" );
            ok( sd->Owner != 0, "no owner SID\n");
            ok( sd->Group != 0, "no owner SID\n");
            ok( EqualSid( (PSID)((LPBYTE)sd + sd->Owner), sid_admin ), "unexpected owner SID\n" );
            ok( EqualSid( (PSID)((LPBYTE)sd + sd->Group), sid_admin ), "unexpected group SID\n" );

            hr = SafeArrayUnaccessData( V_ARRAY( &var_sd ) );
            ok( hr == S_OK, "SafeArrayUnaccessData failed %x\n", hr );
        }

        VariantClear( &var_sd );
        IWbemClassObject_Release( out );
    }
    else if (hr == WBEM_E_ACCESS_DENIED)
        win_skip( "insufficient privs to test __SystemSecurity\n" );

    SysFreeString( class );
}

static void test_Win32_OperatingSystem( IWbemServices *services )
{
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
    static const WCHAR buildnumberW[] = {'B','u','i','l','d','N','u','m','b','e','r',0};
    static const WCHAR captionW[] = {'C','a','p','t','i','o','n',0};
    static const WCHAR csdversionW[] = {'C','S','D','V','e','r','s','i','o','n',0};
    static const WCHAR csnameW[] = {'C','S','N','a','m','e',0};
    static const WCHAR currenttimezoneW[] = {'C','u','r','r','e','n','t','T','i','m','e','Z','o','n','e',0};
    static const WCHAR freephysicalmemoryW[] = {'F','r','e','e','P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
    static const WCHAR manufacturerW[] = {'M','a','n','u','f','a','c','t','u','r','e','r',0};
    static const WCHAR nameW[] = {'N','a','m','e',0};
    static const WCHAR operatingsystemskuW[] = {'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m','S','K','U',0};
    static const WCHAR osproductsuiteW[] = {'O','S','P','r','o','d','u','c','t','S','u','i','t','e',0};
    static const WCHAR ostypeW[] = {'O','S','T','y','p','e',0};
    static const WCHAR suitemaskW[] = {'S','u','i','t','e','M','a','s','k',0};
    static const WCHAR versionW[] = {'V','e','r','s','i','o','n',0};
    static const WCHAR servicepackmajorW[] =
        {'S','e','r','v','i','c','e','P','a','c','k','M','a','j','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR servicepackminorW[] =
        {'S','e','r','v','i','c','e','P','a','c','k','M','i','n','o','r','V','e','r','s','i','o','n',0};
    static const WCHAR systemdriveW[] =
        {'S','y','s','t','e','m','D','r','i','v','e',0};
    static const WCHAR totalvisiblememorysizeW[] =
        {'T','o','t','a','l','V','i','s','i','b','l','e','M','e','m','o','r','y','S','i','z','e',0};
    static const WCHAR totalvirtualmemorysizeW[] =
        {'T','o','t','a','l','V','i','r','t','u','a','l','M','e','m','o','r','y','S','i','z','e',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    CIMTYPE type;
    ULONG count;
    VARIANT val;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "IWbemServices_ExecQuery failed %08x\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "IEnumWbemClassObject_Next failed %08x\n", hr );

    hr = IWbemClassObject_BeginEnumeration( obj, 0 );
    ok( hr == S_OK, "got %08x\n", hr );

    while (IWbemClassObject_Next( obj, 0, NULL, NULL, NULL, NULL ) == S_OK) {}

    hr = IWbemClassObject_EndEnumeration( obj );
    ok( hr == S_OK, "got %08x\n", hr );

    check_property( obj, buildnumberW, VT_BSTR, CIM_STRING );
    check_property( obj, captionW, VT_BSTR, CIM_STRING );

    type = 0xdeadbeef;
    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, csdversionW, 0, &val, &type, NULL );
    ok( hr == S_OK, "failed to get csdversion %08x\n", hr );
    ok( V_VT( &val ) == VT_BSTR || V_VT( &val ) == VT_NULL, "unexpected variant type 0x%x\n", V_VT( &val ) );
    ok( type == CIM_STRING, "unexpected type 0x%x\n", type );
    trace( "csdversion: %s\n", wine_dbgstr_w(V_BSTR( &val )) );
    VariantClear( &val );

    check_property( obj, freephysicalmemoryW, VT_BSTR, CIM_UINT64 );
    check_property( obj, nameW, VT_BSTR, CIM_STRING );

    type = 0xdeadbeef;
    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, operatingsystemskuW, 0, &val, &type, NULL );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* winxp */, "failed to get operatingsystemsku %08x\n", hr );
    if (hr == S_OK)
    {
        ok( V_VT( &val ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &val ) );
        ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );
        trace( "operatingsystemsku: %08x\n", V_I4( &val ) );
        VariantClear( &val );
    }

    type = 0xdeadbeef;
    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, osproductsuiteW, 0, &val, &type, NULL );
    ok( hr == S_OK, "failed to get osproductsuite %08x\n", hr );
    ok( V_VT( &val ) == VT_I4 || broken(V_VT( &val ) == VT_NULL) /* winxp */, "unexpected variant type 0x%x\n", V_VT( &val ) );
    ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );
    trace( "osproductsuite: %d (%08x)\n", V_I4( &val ), V_I4( &val ) );
    VariantClear( &val );

    check_property( obj, csnameW, VT_BSTR, CIM_STRING );
    check_property( obj, currenttimezoneW, VT_I2, CIM_SINT16 );
    check_property( obj, manufacturerW, VT_BSTR, CIM_STRING );
    check_property( obj, ostypeW, VT_I4, CIM_UINT16 );
    check_property( obj, servicepackmajorW, VT_I4, CIM_UINT16 );
    check_property( obj, servicepackminorW, VT_I4, CIM_UINT16 );
    check_property( obj, suitemaskW, VT_I4, CIM_UINT32 );
    check_property( obj, versionW, VT_BSTR, CIM_STRING );
    check_property( obj, totalvisiblememorysizeW, VT_BSTR, CIM_UINT64 );
    check_property( obj, totalvirtualmemorysizeW, VT_BSTR, CIM_UINT64 );
    check_property( obj, systemdriveW, VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_ComputerSystemProduct( IWbemServices *services )
{
    static const WCHAR identifyingnumberW[] =
        {'I','d','e','n','t','i','f','y','i','n','g','N','u','m','b','e','r',0};
    static const WCHAR nameW[] =
        {'N','a','m','e',0};
    static const WCHAR skunumberW[] =
        {'S','K','U','N','u','m','b','e','r',0};
    static const WCHAR uuidW[] =
        {'U','U','I','D',0};
    static const WCHAR vendorW[] =
        {'V','e','n','d','o','r',0};
    static const WCHAR versionW[] =
        {'V','e','r','s','i','o','n',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'C','o','m','p','u','t','e','r','S','y','s','t','e','m','P','r','o','d','u','c','t',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_ComputerSystemProduct not available\n" );
        return;
    }

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "got %08x\n", hr );

    check_property( obj, identifyingnumberW, VT_BSTR, CIM_STRING );
    check_property( obj, nameW, VT_BSTR, CIM_STRING );
    check_property( obj, skunumberW, VT_NULL, CIM_STRING );
    check_property( obj, uuidW, VT_BSTR, CIM_STRING );
    check_property( obj, vendorW, VT_BSTR, CIM_STRING );
    check_property( obj, versionW, VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_PhysicalMemory( IWbemServices *services )
{
    static const WCHAR capacityW[] = {'C','a','p','a','c','i','t','y',0};
    static const WCHAR memorytypeW[] = {'M','e','m','o','r','y','T','y','p','e',0};
    static const WCHAR devicelocatorW[] = {'D','e','v','i','c','e','L','o','c','a','t','o','r',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_PhysicalMemory not available\n" );
        return;
    }

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "got %08x\n", hr );

    if (count > 0)
    {
        check_property( obj, capacityW, VT_BSTR, CIM_UINT64 );
        check_property( obj, devicelocatorW, VT_BSTR, CIM_STRING );
        check_property( obj, memorytypeW, VT_I4, CIM_UINT16 );
        IWbemClassObject_Release( obj );
    }
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_IP4RouteTable( IWbemServices *services )
{
    static const WCHAR destinationW[] = {'D','e','s','t','i','n','a','t','i','o','n',0};
    static const WCHAR interfaceindexW[] = {'I','n','t','e','r','f','a','c','e','I','n','d','e','x',0};
    static const WCHAR nexthopW[] = {'N','e','x','t','H','o','p',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'I','P','4','R','o','u','t','e','T','a','b','l','e',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_IP4RouteTable not available\n" );
        return;
    }

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, destinationW, VT_BSTR, CIM_STRING );
        check_property( obj, interfaceindexW, VT_I4, CIM_SINT32 );
        check_property( obj, nexthopW, VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Processor( IWbemServices *services )
{
    static const WCHAR architectureW[] =
        {'A','r','c','h','i','t','e','c','t','u','r','e',0};
    static const WCHAR captionW[] =
        {'C','a','p','t','i','o','n',0};
    static const WCHAR cpustatusW[] =
        {'C','p','u','S','t','a','t','u','s',0};
    static const WCHAR familyW[] =
        {'F','a','m','i','l','y',0};
    static const WCHAR levelW[] =
        {'L','e','v','e','l',0};
    static const WCHAR manufacturerW[] =
        {'M','a','n','u','f','a','c','t','u','r','e','r',0};
    static const WCHAR nameW[] =
        {'N','a','m','e',0};
    static const WCHAR numcoresW[] =
        {'N','u','m','b','e','r','O','f','C','o','r','e','s',0};
    static const WCHAR numlogicalprocessorsW[] =
        {'N','u','m','b','e','r','O','f','L','o','g','i','c','a','l','P','r','o','c','e','s','s','o','r','s',0};
    static const WCHAR processoridW[] =
        {'P','r','o','c','e','s','s','o','r','I','d',0};
    static const WCHAR revisionW[] =
        {'R','e','v','i','s','i','o','n',0};
    static const WCHAR versionW[] =
        {'V','e','r','s','i','o','n',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'P','r','o','c','e','s','s','o','r',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    VARIANT val;
    CIMTYPE type;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %08x\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, architectureW, VT_I4, CIM_UINT16 );
        check_property( obj, captionW, VT_BSTR, CIM_STRING );
        check_property( obj, cpustatusW, VT_I4, CIM_UINT16 );
        check_property( obj, familyW, VT_I4, CIM_UINT16 );
        check_property( obj, levelW, VT_I4, CIM_UINT16 );
        check_property( obj, manufacturerW, VT_BSTR, CIM_STRING );
        check_property( obj, nameW, VT_BSTR, CIM_STRING );
        check_property( obj, processoridW, VT_BSTR, CIM_STRING );
        check_property( obj, revisionW, VT_I4, CIM_UINT16 );
        check_property( obj, versionW, VT_BSTR, CIM_STRING );

        type = 0xdeadbeef;
        VariantInit( &val );
        hr = IWbemClassObject_Get( obj, numlogicalprocessorsW, 0, &val, &type, NULL );
        ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* win2k3 */, "got %08x\n", hr );
        if (hr == S_OK)
        {
            ok( V_VT( &val ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &val ) );
            ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );
            trace( "numlogicalprocessors %u\n", V_I4( &val ) );
        }

        type = 0xdeadbeef;
        VariantInit( &val );
        hr = IWbemClassObject_Get( obj, numcoresW, 0, &val, &type, NULL );
        ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* win2k3 */, "got %08x\n", hr );
        if (hr == S_OK)
        {
            ok( V_VT( &val ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &val ) );
            ok( type == CIM_UINT32, "unexpected type 0x%x\n", type );
            trace( "numcores %u\n", V_I4( &val ) );
        }

        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_VideoController( IWbemServices *services )
{
    static const WCHAR availabilityW[] =
        {'A','v','a','i','l','a','b','i','l','i','t','y',0};
    static const WCHAR configmanagererrorcodeW[] =
        {'C','o','n','f','i','g','M','a','n','a','g','e','r','E','r','r','o','r','C','o','d','e',0};
    static const WCHAR driverdateW[] =
        {'D','r','i','v','e','r','D','a','t','e',0};
    static const WCHAR installeddisplaydriversW[]=
        {'I','n','s','t','a','l','l','e','d','D','i','s','p','l','a','y','D','r','i','v','e','r','s',0};
    static const WCHAR statusW[] =
        {'S','t','a','t','u','s',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'V','i','d','e','o','C','o','n','t','r','o','l','l','e','r',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    VARIANT val;
    CIMTYPE type;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_VideoController not available\n" );
        return;
    }

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, availabilityW, VT_I4, CIM_UINT16 );
        check_property( obj, configmanagererrorcodeW, VT_I4, CIM_UINT32 );
        check_property( obj, driverdateW, VT_BSTR, CIM_DATETIME );

        type = 0xdeadbeef;
        VariantInit( &val );
        hr = IWbemClassObject_Get( obj, installeddisplaydriversW, 0, &val, &type, NULL );
        ok( hr == S_OK, "got %08x\n", hr );
        ok( V_VT( &val ) == VT_BSTR || V_VT( &val ) == VT_NULL, "unexpected variant type 0x%x\n", V_VT( &val ) );
        ok( type == CIM_STRING, "unexpected type 0x%x\n", type );
        trace( "installeddisplaydrivers %s\n", wine_dbgstr_w(V_BSTR( &val )) );
        VariantClear( &val );

        check_property( obj, statusW, VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Printer( IWbemServices *services )
{
    static const WCHAR attributesW[] =
        {'A','t','t','r','i','b','u','t','e','s',0};
    static const WCHAR deviceidW[] =
        {'D','e','v','i','c','e','I','d',0};
    static const WCHAR horizontalresolutionW[] =
        {'H','o','r','i','z','o','n','t','a','l','R','e','s','o','l','u','t','i','o','n',0};
    static const WCHAR locationW[] =
        {'L','o','c','a','t','i','o','n',0};
    static const WCHAR portnameW[] =
        {'P','o','r','t','N','a','m','e',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'P','r','i','n','t','e','r',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    VARIANT val;
    CIMTYPE type;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_Printer not available\n" );
        return;
    }

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, attributesW, VT_I4, CIM_UINT32 );
        check_property( obj, deviceidW, VT_BSTR, CIM_STRING );
        check_property( obj, horizontalresolutionW, VT_I4, CIM_UINT32 );

        type = 0xdeadbeef;
        memset( &val, 0, sizeof(val) );
        hr = IWbemClassObject_Get( obj, locationW, 0, &val, &type, NULL );
        ok( hr == S_OK, "got %08x\n", hr );
        ok( V_VT( &val ) == VT_BSTR || V_VT( &val ) == VT_NULL, "unexpected variant type 0x%x\n", V_VT( &val ) );
        ok( type == CIM_STRING, "unexpected type 0x%x\n", type );
        trace( "location %s\n", wine_dbgstr_w(V_BSTR( &val )) );
        VariantClear( &val );

        check_property( obj, portnameW, VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_PnPEntity( IWbemServices *services )
{
    HRESULT hr;
    IEnumWbemClassObject *enm;
    IWbemClassObject *obj;
    VARIANT val;
    CIMTYPE type;
    ULONG count, i;
    BSTR bstr;

    static WCHAR win32_pnpentityW[] = {'W','i','n','3','2','_','P','n','P','E','n','t','i','t','y',0};
    static const WCHAR deviceidW[] = {'D','e','v','i','c','e','I','d',0};

    bstr = SysAllocString( win32_pnpentityW );

    hr = IWbemServices_CreateInstanceEnum( services, bstr, 0, NULL, &enm );
    ok( hr == S_OK, "got %08x\n", hr );

    SysFreeString( bstr );
    bstr = SysAllocString( deviceidW );

    while (1)
    {
        hr = IEnumWbemClassObject_Next( enm, 1000, 1, &obj, &count );
        ok( (count == 1 && (hr == WBEM_S_FALSE || hr == WBEM_S_NO_ERROR)) ||
                (count == 0 && (hr == WBEM_S_FALSE || hr == WBEM_S_TIMEDOUT)),
                "got %08x with %u objects returned\n", hr, count );

        if (count == 0)
            break;

        for (i = 0; i < count; ++i)
        {
            hr = IWbemClassObject_Get( obj, bstr, 0, &val, &type, NULL );
            ok( hr == S_OK, "got %08x\n", hr );

            if (SUCCEEDED( hr ))
            {
                ok( V_VT( &val ) == VT_BSTR, "unexpected variant type 0x%x\n", V_VT( &val ) );
                ok( type == CIM_STRING, "unexpected type 0x%x\n", type );
                VariantClear( &val );
            }
        }
        IWbemClassObject_Release( obj );
    }

    SysFreeString( bstr );

    IEnumWbemClassObject_Release( enm );
}

static void test_Win32_WinSAT( IWbemServices *services )
{
    static const WCHAR cpuscoreW[] =
        {'C','P','U','S','c','o','r','e',0};
    static const WCHAR d3dscoreW[] =
        {'D','3','D','S','c','o','r','e',0};
    static const WCHAR diskscoreW[] =
        {'D','i','s','k','S','c','o','r','e',0};
    static const WCHAR graphicsscoreW[] =
        {'G','r','a','p','h','i','c','s','S','c','o','r','e',0};
    static const WCHAR memoryscoreW[] =
        {'M','e','m','o','r','y','S','c','o','r','e',0};
    static const WCHAR winsatassessmentstateW[] =
        {'W','i','n','S','A','T','A','s','s','e','s','s','m','e','n','t','S','t','a','t','e',0};
    static const WCHAR winsprlevelW[] =
        {'W','i','n','S','P','R','L','e','v','e','l',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_','W','i','n','S','A','T',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK || broken(hr == WBEM_E_INVALID_CLASS) /* win2k8 */, "got %08x\n", hr );
    if (hr == WBEM_E_INVALID_CLASS)
    {
        win_skip( "class not found\n" );
        return;
    }

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, cpuscoreW, VT_R4, CIM_REAL32 );
        check_property( obj, d3dscoreW, VT_R4, CIM_REAL32 );
        check_property( obj, diskscoreW, VT_R4, CIM_REAL32 );
        check_property( obj, graphicsscoreW, VT_R4, CIM_REAL32 );
        check_property( obj, memoryscoreW, VT_R4, CIM_REAL32 );
        check_property( obj, winsatassessmentstateW, VT_I4, CIM_UINT32 );
        check_property( obj, winsprlevelW, VT_R4, CIM_REAL32 );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_DisplayControllerConfiguration( IWbemServices *services )
{
    static const WCHAR bitsperpixelW[] =
        {'B','i','t','s','P','e','r','P','i','x','e','l',0};
    static const WCHAR captionW[] =
        {'C','a','p','t','i','o','n',0};
    static const WCHAR horizontalresolutionW[] =
        {'H','o','r','i','z','o','n','t','a','l','R','e','s','o','l','u','t','i','o','n',0};
    static const WCHAR nameW[] =
        {'N','a','m','e',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'D','i','s','p','l','a','y','C','o','n','t','r','o','l','l','e','r',
         'C','o','n','f','i','g','u','r','a','t','i','o','n',0};
    static const WCHAR verticalresolutionW[] =
        {'V','e','r','t','i','c','a','l','R','e','s','o','l','u','t','i','o','n',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %08x\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, bitsperpixelW, VT_I4, CIM_UINT32 );
        check_property( obj, captionW, VT_BSTR, CIM_STRING );
        check_property( obj, horizontalresolutionW, VT_I4, CIM_UINT32 );
        check_property( obj, nameW, VT_BSTR, CIM_STRING );
        check_property( obj, verticalresolutionW, VT_I4, CIM_UINT32 );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_QuickFixEngineering( IWbemServices *services )
{
    static const WCHAR captionW[] =
        {'C','a','p','t','i','o','n',0};
    static const WCHAR hotfixidW[] =
        {'H','o','t','F','i','x','I','D',0};
    static const WCHAR queryW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'Q','u','i','c','k','F','i','x','E','n','g','i','n','e','e','r','i','n','g',0};
    BSTR wql = SysAllocString( wqlW ), query = SysAllocString( queryW );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count, total = 0;
    VARIANT caption;
    CIMTYPE type;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %08x\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        type = 0xdeadbeef;
        VariantInit( &caption );
        hr = IWbemClassObject_Get( obj, captionW, 0, &caption, &type, NULL );
        ok( hr == S_OK, "failed to get caption %08x\n", hr );
        ok( V_VT( &caption ) == VT_BSTR || V_VT( &caption ) == VT_NULL /* winxp */,
            "unexpected variant type 0x%x\n", V_VT( &caption ) );
        ok( type == CIM_STRING, "unexpected type 0x%x\n", type );

        check_property( obj, hotfixidW, VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
        if (total++ >= 10) break;
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

START_TEST(query)
{
    static const WCHAR cimv2W[] = {'R','O','O','T','\\','C','I','M','V','2',0};
    BSTR path = SysAllocString( cimv2W );
    IWbemLocator *locator;
    IWbemServices *services;
    HRESULT hr;

    CoInitialize( NULL );
    CoInitializeSecurity( NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                          RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL );
    hr = CoCreateInstance( &CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, &IID_IWbemLocator,
                           (void **)&locator );
    if (hr != S_OK)
    {
        win_skip("can't create instance of WbemLocator\n");
        return;
    }
    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %08x\n", hr );

    hr = CoSetProxyBlanket( (IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );
    ok( hr == S_OK, "failed to set proxy blanket %08x\n", hr );

    test_GetNames( services );
    test_associators( services );
    test_notification_query_async( services );
    test_query_async( services );
    test_query_semisync( services );
    test_select( services );

    /* classes */
    test_StdRegProv( services );
    test_SystemSecurity( services );
    test_Win32_Baseboard( services );
    test_Win32_ComputerSystem( services );
    test_Win32_ComputerSystemProduct( services );
    test_Win32_Bios( services );
    test_Win32_DisplayControllerConfiguration( services );
    test_Win32_IP4RouteTable( services );
    test_Win32_OperatingSystem( services );
    test_Win32_PhysicalMemory( services );
    test_Win32_PnPEntity( services );
    test_Win32_Printer( services );
    test_Win32_Process( services, FALSE );
    test_Win32_Process( services, TRUE );
    test_Win32_Processor( services );
    test_Win32_QuickFixEngineering( services );
    test_Win32_Service( services );
    test_Win32_SystemEnclosure( services );
    test_Win32_VideoController( services );
    test_Win32_WinSAT( services );

    SysFreeString( path );
    IWbemServices_Release( services );
    IWbemLocator_Release( locator );
    CoUninitialize();
}
