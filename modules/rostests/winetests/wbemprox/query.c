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

static HRESULT exec_query( IWbemServices *services, const WCHAR *str, IEnumWbemClassObject **result )
{
    HRESULT hr;
    IWbemClassObject *obj;
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( str );
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

            if (IWbemClassObject_Get( obj, L"Caption", 0, &var, NULL, NULL ) == WBEM_S_NO_ERROR)
            {
                trace("caption: %s\n", wine_dbgstr_w(V_BSTR(&var)));
                VariantClear( &var );
            }
            if (IWbemClassObject_Get( obj, L"Description", 0, &var, NULL, NULL ) == WBEM_S_NO_ERROR)
            {
                trace("description: %s\n", wine_dbgstr_w(V_BSTR(&var)));
                VariantClear( &var );
            }

            hr = IWbemClassObject_GetQualifierSet( obj, &qualifiers );
            ok( hr == S_OK, "got %#lx\n", hr );

            hr = IWbemQualifierSet_GetNames( qualifiers, 0, &names );
            ok( hr == S_OK, "got %#lx\n", hr );

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
    static const WCHAR *test[] =
    {
        L"SELECT HOTFIXID FROM Win32_QuickFixEngineering",
        L"SELECT * FROM Win32_BIOS",
        L"SELECT * FROM Win32_LogicalDisk WHERE \"NTFS\" = FileSystem",
        L"SELECT a FROM b",
        L"SELECT a FROM Win32_Bios",
        L"SELECT Description FROM Win32_Bios",
        L"SELECT * FROM Win32_Process WHERE Caption LIKE '%%REGEDIT%'",
        L"SELECT * FROM Win32_DiskDrive WHERE DeviceID=\"\\\\\\\\.\\\\PHYSICALDRIVE0\"",
        L"SELECT\na\rFROM\tb",
        L"SELECT * FROM Win32_Process WHERE Caption LIKE \"%firefox.exe\"",
        L"SELECT * FROM Win32_VideoController where availability = '3'",
        L"SELECT * FROM Win32_BIOS WHERE NAME <> NULL",
        L"SELECT * FROM Win32_BIOS WHERE NULL = NAME",
        L"SELECT * FROM Win32_LogicalDiskToPartition",
        L"SELECT * FROM Win32_DiskDriveToDiskPartition",
        L"SELECT \x80 FROM \x80",
        L"SELECT \xC6 FROM \xC6",
        L"SELECT \xFF FROM \xFF",
        L"SELECT \x200C FROM \x200C",
        L"SELECT \xFF21 FROM \xFF21",
        L"SELECT \xFFFD FROM \xFFFD",
    };
    HRESULT hr;
    IEnumWbemClassObject *result;
    BSTR wql = SysAllocString( L"wql" );
    BSTR sql = SysAllocString( L"SQL" );
    BSTR query = SysAllocString( L"SELECT * FROM Win32_BIOS" );
    UINT i;

    hr = IWbemServices_ExecQuery( services, NULL, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %#lx\n", hr );

    hr = IWbemServices_ExecQuery( services, NULL, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %#lx\n", hr );

    hr = IWbemServices_ExecQuery( services, wql, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %#lx\n", hr );

    hr = IWbemServices_ExecQuery( services, sql, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_QUERY_TYPE, "query failed %#lx\n", hr );

    hr = IWbemServices_ExecQuery( services, sql, NULL, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %#lx\n", hr );

    SysFreeString( query );
    query = SysAllocString( L"" );
    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_PARAMETER, "query failed %#lx\n", hr );

    for (i = 0; i < ARRAY_SIZE( test ); i++)
    {
        hr = exec_query( services, test[i], &result );
        ok( hr == S_OK, "query %u failed: %#lx\n", i, hr );
        if (result) IEnumWbemClassObject_Release( result );
    }

    SysFreeString( wql );
    SysFreeString( sql );
    SysFreeString( query );
}

static void check_explorer_like_query( IWbemServices *services, const WCHAR *str, BOOL expect_success)
{
    HRESULT hr;
    IWbemClassObject *obj[2];
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( str );
    LONG flags = WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY;
    ULONG count;
    IEnumWbemClassObject *result;

    hr = IWbemServices_ExecQuery( services, wql, query, flags, NULL, &result );
    if (hr == S_OK)
    {
        VARIANT var;
        IEnumWbemClassObject_Next( result, 10000, 2, obj, &count );

        if (expect_success)
            ok( count >= 1, "expected to get 1 or more results\n" );
        else
            ok( count == 0, "expected to get 0 results\n" );

        if (count)
        {
            BSTR caption;
            hr = IWbemClassObject_Get( obj[0], L"Caption", 0, &var, NULL, NULL );
            ok( hr == WBEM_S_NO_ERROR, "IWbemClassObject_Get failed %#lx", hr);
            caption = V_BSTR(&var);
            ok( !wcscmp( caption, L"explorer.exe" ), "%s is not explorer.exe\n", debugstr_w(caption));
            VariantClear( &var );
        }

        while (count--)
            IWbemClassObject_Release( obj[count] );
    }

    SysFreeString( wql );
    SysFreeString( query );
}


static void test_like_query( IWbemServices *services )
{
    int i;
    WCHAR query[250];

    struct {
        BOOL expect_success;
        const WCHAR *str;
    } queries[] = {
        { TRUE,  L"explorer%" },
        { FALSE, L"xplorer.exe" },
        { FALSE, L"explorer.ex" },
        { TRUE,  L"%explorer%" },
        { TRUE,  L"explorer.exe%" },
        { TRUE,  L"%explorer.exe%" },
        { TRUE,  L"%plorer.exe" },
        { TRUE,  L"%plorer.exe%" },
        { TRUE,  L"__plorer.exe" },
        { TRUE,  L"e_plorer.exe" },
        { FALSE, L"_plorer.exe" },
        { TRUE,  L"%%%plorer.e%" },
        { TRUE,  L"%plorer.e%" },
        { TRUE,  L"%plorer.e_e" },
        { TRUE,  L"%plorer.e_e" },
        { TRUE,  L"explore%exe" },
        { FALSE, L"fancy_explore.exe" },
        { FALSE, L"fancy%xplore%exe" },
        { FALSE, L"%%%f%xplore%exe" },
    };

    for (i = 0; i < ARRAYSIZE(queries); i++)
    {
        wsprintfW( query, L"SELECT * FROM Win32_Process WHERE Caption LIKE '%ls'", queries[i].str );
        trace("%s\n", wine_dbgstr_w(query));
        check_explorer_like_query( services, query, queries[i].expect_success );
    }
}


static void test_IWbemClassObject_Next( IWbemServices *services )
{
    struct
    {
        const WCHAR *name;
        BOOL found;
    }
    system_props[] =
    {
        {L"__GENUS"}, {L"__CLASS"}, {L"__RELPATH"}, {L"__PROPERTY_COUNT"}, {L"__DERIVATION"},
        {L"__SERVER"}, {L"__NAMESPACE"}, {L"__PATH"},
    };

    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_LogicalDisk" );
    BSTR name;
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    unsigned int i, j;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_Volume not available\n" );
        return;
    }

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "got %#lx.\n", hr );

    IWbemClassObject_BeginEnumeration(obj, 0);
    hr = IWbemClassObject_Next( obj, WBEM_FLAG_SYSTEM_ONLY, &name, NULL, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %#lx.\n", hr );
    hr = IWbemClassObject_Next( obj, WBEM_FLAG_NONSYSTEM_ONLY, &name, NULL, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %#lx.\n", hr );

    for (i = 0; !(hr = IWbemClassObject_Next( obj, 0, &name, NULL, NULL, NULL )); ++i)
    {
        ok( hr == S_OK, "got %#lx\n", hr );
        for (j = 0; j < ARRAY_SIZE(system_props); ++j)
        {
            if (!wcscmp(name, system_props[j].name))
            {
                system_props[j].found = TRUE;
                break;
            }
        }
        SysFreeString( name );
    }
    ok( hr == WBEM_S_NO_MORE_DATA, "got %#lx.\n", hr );
    IWbemClassObject_Release( obj );

    for (i = 0; i < ARRAY_SIZE(system_props); ++i)
        ok( system_props[i].found, "%s not found.\n", debugstr_w(system_props[i].name) );

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}


static void test_associators( IWbemServices *services )
{
    static const WCHAR *test[] =
    {
        L"ASSOCIATORS OF{Win32_LogicalDisk.DeviceID=\"C:\"}",
        L"ASSOCIATORS OF {Win32_LogicalDisk.DeviceID=\"C:\"} WHERE AssocClass=Win32_LogicalDiskToPartition",
        L"ASSOCIATORS OF {Win32_LogicalDisk.DeviceID}",
        L"ASSOCIATORS OF {Win32_DiskDrive.DeviceID='\\\\.\\PHYSICALDRIVE0'}",
        L"ASSOCIATORS OF {Win32_LogicalDisk.DeviceID=\"C:\"} WHERE AssocClass=Win32_LogicalDiskToPartition ClassDefsOnly",
        L"ASSOCIATORS OF {Win32_LogicalDisk.DeviceID=\"C:\"} WHERE ClassDefsOnly",
        L"ASSOCIATORS OF {Win32_LogicalDisk.DeviceID=\"C:\"} WHERE ClassDefsOnly AssocClass = Win32_LogicalDiskToPartition",
    };
    HRESULT hr;
    IEnumWbemClassObject *result;
    UINT i;

    for (i = 0; i < ARRAY_SIZE( test ); i++)
    {
        hr = exec_query( services, test[i], &result );
        ok( hr == S_OK, "query %u failed: %#lx\n", i, hr );
        if (result) IEnumWbemClassObject_Release( result );
    }
}

static void test_IEnumWbemClassObject_Next( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_IP4RouteTable" );
    IWbemClassObject **obj, *obj1;
    IEnumWbemClassObject *result;
    DWORD count, num_objects = 0;
    HRESULT hr;
    int i;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 2;
    hr = IEnumWbemClassObject_Next( result, 10000, 1, NULL, &count );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %#lx\n", hr );
    ok( count == 2, "expected 0, got %lu\n", count );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj1, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %#lx\n", hr );

    count = 2;
    hr = IEnumWbemClassObject_Next( result, 10000, 0, &obj1, &count );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 0, "expected 0, got %lu\n", count );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj1, &count );
        if (hr != S_OK) break;
        num_objects++;
        IWbemClassObject_Release(obj1);
    }

    hr = IEnumWbemClassObject_Reset( result );
    ok( hr == S_OK, "got %#lx\n", hr );

    obj = malloc( num_objects * sizeof( IWbemClassObject * ) );

    count = 0;
    hr = IEnumWbemClassObject_Next( result, 10000, num_objects, obj, &count );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == num_objects, "expected %lu, got %lu\n", num_objects, count );

    for (i = 0; i < count; i++)
        IWbemClassObject_Release( obj[i] );

    hr = IEnumWbemClassObject_Reset( result );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 0;
    hr = IEnumWbemClassObject_Next( result, 10000, num_objects + 1, obj, &count );
    ok( hr == S_FALSE, "got %#lx\n", hr );
    ok( count == num_objects, "expected %lu, got %lu\n", num_objects, count );

    for (i = 0; i < count; i++)
        IWbemClassObject_Release( obj[i] );

    free( obj );

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void _check_property( ULONG line, IWbemClassObject *obj, const WCHAR *prop, VARTYPE vartype, CIMTYPE cimtype,
                             BOOL nullable)
{
    CIMTYPE type = 0xdeadbeef;
    VARIANT val;
    HRESULT hr;

    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, prop, 0, &val, &type, NULL );
    ok( hr == S_OK, "%lu: failed to get description %#lx\n", line, hr );
    ok( V_VT( &val ) == vartype || (nullable && V_VT( &val ) == VT_NULL), "%lu: unexpected variant type 0x%x\n",
        line, V_VT(&val) );
    ok( type == cimtype, "%lu: unexpected type %#lx\n", line, type );
    switch (V_VT(&val))
    {
    case VT_BSTR:
        trace( "%s: %s\n", wine_dbgstr_w(prop), wine_dbgstr_w(V_BSTR(&val)) );
        break;
    case VT_I2:
        trace( "%s: %d\n", wine_dbgstr_w(prop), V_I2(&val) );
        break;
    case VT_I4:
        trace( "%s: %ld\n", wine_dbgstr_w(prop), V_I4(&val) );
        break;
    case VT_R4:
        trace( "%s: %f\n", wine_dbgstr_w(prop), V_R4(&val) );
        break;
    case VT_BOOL:
        trace( "%s: %d\n", wine_dbgstr_w(prop), V_BOOL(&val) );
        break;
    default:
        break;
    }
    VariantClear( &val );
}
#define check_property(a,b,c,d) _check_property(__LINE__,a,b,c,d,FALSE)
#define check_property_nullable(a,b,c,d) _check_property(__LINE__,a,b,c,d,TRUE)

static void test_Win32_Service( IWbemServices *services )
{
    BSTR class = SysAllocString( L"Win32_Service.Name=\"Spooler\"" ), empty = SysAllocString( L"" ), method;
    IWbemClassObject *service, *out;
    VARIANT state, retval, classvar;
    CIMTYPE type;
    HRESULT hr;

    hr = IWbemServices_GetObject( services, class, 0, NULL, &service, NULL );
    if (hr != S_OK)
    {
        win_skip( "Win32_Service not available\n" );
        SysFreeString( empty );
        SysFreeString( class );
        return;
    }

    check_property( service, L"ProcessID", VT_I4, CIM_UINT32 );
    type = 0xdeadbeef;
    VariantInit( &state );
    hr = IWbemClassObject_Get( service, L"State", 0, &state, &type, NULL );
    ok( hr == S_OK, "failed to get service state %#lx\n", hr );
    ok( V_VT( &state ) == VT_BSTR, "unexpected variant type 0x%x\n", V_VT( &state ) );
    ok( type == CIM_STRING, "unexpected type %#lx\n", type );

    if (!lstrcmpW( V_BSTR( &state ), L"Stopped" ))
    {
        out = NULL;
        method = SysAllocString( L"StartService" );
        hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
        ok( hr == S_OK, "failed to execute method %#lx\n", hr );
        SysFreeString( method );

        VariantInit( &retval );
        hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, NULL, NULL );
        ok( hr == S_OK, "failed to get return value %#lx\n", hr );
        ok( !V_I4( &retval ), "unexpected error %lu\n", V_UI4( &retval ) );
        IWbemClassObject_Release( out );
    }
    out = NULL;
    method = SysAllocString( L"PauseService" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, NULL, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_I4( &retval ), "unexpected success\n" );
    IWbemClassObject_Release( out );

    out = NULL;
    method = SysAllocString( L"ResumeService" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, NULL, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_I4( &retval ), "unexpected success\n" );
    IWbemClassObject_Release( out );

    if (!lstrcmpW( V_BSTR( &state ), L"Stopped" ))
    {
        out = NULL;
        method = SysAllocString( L"StopService" );
        hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
        ok( hr == S_OK, "failed to execute method %#lx\n", hr );
        SysFreeString( method );

        VariantInit( &retval );
        hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, NULL, NULL );
        ok( hr == S_OK, "failed to get return value %#lx\n", hr );
        ok( !V_I4( &retval ), "unexpected error %lu\n", V_UI4( &retval ) );
        IWbemClassObject_Release( out );
    }
    VariantClear( &state );
    IWbemClassObject_Release( service );

    service = NULL;
    hr = IWbemServices_GetObject( services, NULL, 0, NULL, &service, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !!service, "expected non-NULL service\n" );

    VariantInit(&classvar);
    V_VT(&classvar) = VT_BSTR;
    V_BSTR(&classvar) = SysAllocString(L"MyClass");
    hr = IWbemClassObject_Put(service, L"__CLASS", 0, &classvar, 0);
    ok( hr == S_OK, "got %#lx\n", hr );
    VariantClear(&classvar);
    IWbemClassObject_Release( service );

    service = NULL;
    hr = IWbemServices_GetObject( services, empty, 0, NULL, &service, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( !!service, "expected non-NULL service\n" );
    IWbemClassObject_Release( service );

    SysFreeString( empty );
    SysFreeString( class );

    class = SysAllocString( L"Win32_Service.Name=\"nonexistent\"" );
    service = (IWbemClassObject *)0xdeadbeef;
    hr = IWbemServices_GetObject( services, class, 0, NULL, &service, NULL );
    ok( hr == WBEM_E_NOT_FOUND, "got %#lx\n", hr );
    ok( service == NULL, "expected NULL service, got %p\n", service );
    SysFreeString( class );
}

static void test_Win32_Bios( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_BIOS" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    ULONG count;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "IWbemServices_ExecQuery failed %#lx\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "IEnumWbemClassObject_Next failed %#lx\n", hr );

    check_property( obj, L"Description", VT_BSTR, CIM_STRING );
    check_property( obj, L"IdentificationCode", VT_NULL, CIM_STRING );
    check_property_nullable( obj, L"Manufacturer", VT_BSTR, CIM_STRING );
    check_property( obj, L"Name", VT_BSTR, CIM_STRING );
    check_property_nullable( obj, L"ReleaseDate", VT_BSTR, CIM_DATETIME );
    check_property_nullable( obj, L"SerialNumber", VT_BSTR, CIM_STRING );
    check_property_nullable( obj, L"SMBIOSBIOSVersion", VT_BSTR, CIM_STRING );
    check_property_nullable( obj, L"SMBIOSMajorVersion", VT_I4, CIM_UINT16 );
    check_property_nullable( obj, L"SMBIOSMinorVersion", VT_I4, CIM_UINT16 );
    check_property( obj, L"Status", VT_BSTR, CIM_STRING );
    check_property( obj, L"Version", VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Baseboard( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_Baseboard" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    ULONG count;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "IWbemServices_ExecQuery failed %#lx\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    if (hr != S_OK)
    {
        win_skip( "Win32_Baseboard not available\n" );
        return;
    }
    ok( hr == S_OK, "IEnumWbemClassObject_Next failed %#lx\n", hr );

    check_property( obj, L"Manufacturer", VT_BSTR, CIM_STRING );
    check_property_nullable( obj, L"Model", VT_BSTR, CIM_STRING );
    check_property( obj, L"Name", VT_BSTR, CIM_STRING );
    check_property( obj, L"Product", VT_BSTR, CIM_STRING );
    check_property( obj, L"Tag", VT_BSTR, CIM_STRING );
    check_property( obj, L"Version", VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Process_created_process(void)
{
}

static void test_Win32_Process( IWbemServices *services, BOOL use_full_path )
{
    static const LONG expected_flavor = WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE |
                                        WBEM_FLAVOR_NOT_OVERRIDABLE |
                                        WBEM_FLAVOR_ORIGIN_PROPAGATED;
    WCHAR full_path[MAX_COMPUTERNAME_LENGTH + ARRAY_SIZE( L"\\\\%s\\ROOT\\CIMV2:" )];
    BSTR class, method;
    IWbemClassObject *process, *sig_in, *sig_out, *out, *params;
    WCHAR cmdlineW[MAX_PATH + 64 + 1];
    IWbemQualifierSet *qualifiers;
    WCHAR executable_path[255];
    VARIANT retval, val;
    SAFEARRAY *names;
    LONG bound, i;
    DWORD full_path_len = 0;
    ULONG refcount;
    LONG flavor;
    CIMTYPE type;
    HRESULT hr;
    DWORD ret;
    HANDLE h;

    if (use_full_path)
    {
        WCHAR server[MAX_COMPUTERNAME_LENGTH+1];

        full_path_len = ARRAY_SIZE(server);
        ok( GetComputerNameW(server, &full_path_len), "GetComputerName failed\n" );
        full_path_len = wsprintfW( full_path, L"\\\\%s\\ROOT\\CIMV2:", server );
    }

    class = SysAllocStringLen( NULL, full_path_len + ARRAY_SIZE( L"Win32_Process" ) );
    memcpy( class, full_path, full_path_len * sizeof(WCHAR) );
    memcpy( class + full_path_len, L"Win32_Process", sizeof(L"Win32_Process") );
    hr = IWbemServices_GetObject( services, class, 0, NULL, &process, NULL );
    SysFreeString( class );
    if (hr != S_OK)
    {
        win_skip( "Win32_Process not available\n" );
        return;
    }
    names = NULL;
    hr = IWbemClassObject_GetNames( process, NULL, 0, NULL, &names );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( names != NULL, "names not set\n" );
    hr = SafeArrayGetUBound( names, 1, &bound );
    ok( hr == S_OK, "got %#lx\n", hr );
    for (i = 0; i <= bound; i++)
    {
        BSTR str;
        hr = SafeArrayGetElement( names, &i, &str );
        ok( hr == S_OK, "%ld: got %#lx\n", i, hr );
        SysFreeString( str );
    }
    SafeArrayDestroy( names );

    sig_in = (void *)0xdeadbeef;
    sig_out = (void *)0xdeadbeef;
    hr = IWbemClassObject_GetMethod( process, L"unknown", 0, &sig_in, &sig_out );
    ok( hr == WBEM_E_NOT_FOUND, "Got unexpected hr %#lx\n", hr );
    ok( !sig_in, "Got unexpected sig_in %p.\n", sig_in );
    ok( !sig_out, "Got unexpected sig_out %p.\n", sig_out );

    sig_in = (void *)0xdeadbeef;
    sig_out = (void *)0xdeadbeef;
    hr = IWbemClassObject_GetMethod( process, L"Create", 0, &sig_in, &sig_out );
    ok( hr == S_OK, "Got unexpected hr %#lx\n", hr );
    ok( !!sig_in, "Got unexpected sig_in %p.\n", sig_in );
    ok( !!sig_out, "Got unexpected sig_out %p.\n", sig_out );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &params );
    ok( hr == S_OK, "got %#lx\n", hr );

    out = NULL;
    class = SysAllocString( L"Win32_Process" );
    method = SysAllocString( L"Create" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, params, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );
    SysFreeString( class );

    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( V_I4( &retval ) == 21, "unexpected error %ld\n", V_I4( &retval ) );

    IWbemClassObject_Release( out );

    V_VT( &val ) = VT_BSTR;
    V_BSTR( &val ) = SysAllocString( L"unknown" );
    hr = IWbemClassObject_Put( params, L"CommandLine", 0, &val, 0 );
    ok( hr == S_OK, "got %#lx\n", hr );
    VariantClear( &val );

    out = NULL;
    class = SysAllocString( L"Win32_Process" );
    method = SysAllocString( L"Create" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, params, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );
    SysFreeString( class );

    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( V_I4( &retval ) == 9, "unexpected error %ld\n", V_I4( &retval ) );

    VariantInit( &retval );
    V_VT( &retval ) = VT_I4;
    hr = IWbemClassObject_Get( out, L"ProcessId", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    todo_wine ok( V_VT( &retval ) == VT_NULL && type == CIM_UINT32, "unexpected variant type %#x, type %lu\n",
            V_VT( &retval ), type );

    IWbemClassObject_Release( out );

    ret = GetModuleFileNameW( NULL, cmdlineW, MAX_PATH + 1 );
    ok( ret < MAX_PATH + 1, "got unexpected ret %lu\n", ret );
    lstrcatW( cmdlineW, L" query created_process");
    V_VT( &val ) = VT_BSTR;
    V_BSTR( &val ) = SysAllocString( cmdlineW );
    hr = IWbemClassObject_Put( params, L"CommandLine", 0, &val, 0 );
    ok( hr == S_OK, "got %#lx\n", hr );
    VariantClear( &val );

    out = NULL;
    class = SysAllocString( L"Win32_Process" );
    method = SysAllocString( L"Create" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, params, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );
    SysFreeString( class );

    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %ld\n", V_I4( &retval ) );

    VariantInit( &retval );
    V_VT( &retval ) = VT_I4;
    hr = IWbemClassObject_Get( out, L"ProcessId", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4 && type == CIM_UINT32, "unexpected variant type %#x, type %lu\n",
            V_VT( &retval ), type );
    ok( !!V_UI4( &retval ), "unexpected zero pid\n" );

    IWbemClassObject_Release( out );

    h = OpenProcess( SYNCHRONIZE, FALSE, V_UI4( &retval ));
    ok( !!h, "failed to open process %#lx\n", V_UI4( &retval ));

    ret = WaitForSingleObject( h, INFINITE );
    ok( ret == WAIT_OBJECT_0, "got unexpected ret %lu, GetLastError() %lu\n", ret, GetLastError() );
    CloseHandle( h );

    refcount = IWbemClassObject_Release( params );
    ok( !refcount, "got unexpected refcount %lu\n", refcount );

    refcount = IWbemClassObject_Release( sig_in );
    ok( !refcount, "got unexpected refcount %lu\n", refcount );
    refcount = IWbemClassObject_Release( sig_out );
    ok( !refcount, "got unexpected refcount %lu\n", refcount );

    sig_in = (void*)0xdeadbeef;
    hr = IWbemClassObject_GetMethod( process, L"getowner", 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get GetOwner method %#lx\n", hr );
    ok( !sig_in, "sig_in != NULL\n");
    IWbemClassObject_Release( process );

    out = NULL;
    method = SysAllocString( L"GetOwner" );
    class = SysAllocStringLen( NULL, full_path_len + ARRAY_SIZE( L"Win32_Process.Handle=\"%u\"" ) + 10 );
    memcpy( class, full_path, full_path_len * sizeof(WCHAR) );
    wsprintfW( class + full_path_len, L"Win32_Process.Handle=\"%u\"", GetCurrentProcessId() );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %ld\n", V_I4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type %#lx\n", type );

    check_property( out, L"User", VT_BSTR, CIM_STRING );
    check_property( out, L"Domain", VT_BSTR, CIM_STRING );

    hr = IWbemClassObject_GetPropertyQualifierSet( out, L"User", &qualifiers );
    ok( hr == S_OK, "failed to get qualifier set %#lx\n", hr );

    flavor = -1;
    V_I4(&val) = -1;
    V_VT(&val) = VT_ERROR;
    hr = IWbemQualifierSet_Get( qualifiers, L"ID", 0, &val, &flavor );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( flavor == expected_flavor, "got %ld\n", flavor );
    ok( V_VT(&val) == VT_I4, "got %#x\n", V_VT(&val) );
    ok( V_I4(&val) == 0, "got %ld\n", V_I4(&val) );
    VariantClear( &val );

    IWbemQualifierSet_Release( qualifiers );
    hr = IWbemClassObject_GetPropertyQualifierSet( out, L"Domain", &qualifiers );
    ok( hr == S_OK, "failed to get qualifier set %#lx\n", hr );

    flavor = -1;
    V_I4(&val) = -1;
    V_VT(&val) = VT_ERROR;
    hr = IWbemQualifierSet_Get( qualifiers, L"ID", 0, &val, &flavor );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( flavor == expected_flavor, "got %ld\n", flavor );
    ok( V_VT(&val) == VT_I4, "got %#x\n", V_VT(&val) );
    ok( V_I4(&val) == 1, "got %ld\n", V_I4(&val) );
    VariantClear( &val );

    IWbemQualifierSet_Release( qualifiers );
    hr = IWbemClassObject_GetPropertyQualifierSet( out, L"ReturnValue", &qualifiers );
    ok( hr == S_OK, "failed to get qualifier set %#lx\n", hr );

    hr = IWbemQualifierSet_Get( qualifiers, L"ID", 0, &val, &flavor );
    ok( hr == WBEM_E_NOT_FOUND, "got %#lx\n", hr );

    /* Test instance properties */

    hr = IWbemServices_GetObject( services, class, 0, NULL, &process, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );

    type = 0xdeadbeef;
    VariantInit( &val );
    hr = IWbemClassObject_Get( process, L"ExecutablePath", 0, &val, &type, NULL );
    ok( hr == S_OK, "IWbemClassObject_Get failed with %#lx\n", hr );
    ok( V_VT( &val ) == VT_BSTR, "unexpected variant type 0x%x\n", V_VT( &val ) );
    ok( type == CIM_STRING, "unexpected type %#lx\n", type );
    GetModuleFileNameW( NULL, executable_path, ARRAY_SIZE(executable_path) );
    ok( !lstrcmpiW( V_BSTR( &val ), executable_path ), "got %s, expected %s\n", wine_dbgstr_w(V_BSTR(&val)), wine_dbgstr_w(executable_path) );
    VariantClear( &val );
    IWbemClassObject_Release( process );

    IWbemQualifierSet_Release( qualifiers );
    IWbemClassObject_Release( out );
    SysFreeString( class );
}

static void test_Win32_ComputerSystem( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_ComputerSystem" );
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
    lstrcatW( username, L"\\" );
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
    ok( hr == S_OK, "got %#lx\n", hr );

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, L"MemoryType", 0, &value, &type, NULL );
    ok( hr == WBEM_E_NOT_FOUND, "got %#lx\n", hr );

    check_property( obj, L"Model", VT_BSTR, CIM_STRING );

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, L"Name", 0, &value, &type, NULL );
    ok( hr == S_OK, "failed to get computer name %#lx\n", hr );
    ok( V_VT( &value ) == VT_BSTR, "unexpected variant type 0x%x\n", V_VT( &value ) );
    ok( type == CIM_STRING, "unexpected type %#lx\n", type );
    ok( !lstrcmpiW( V_BSTR( &value ), compname ), "got %s, expected %s\n", wine_dbgstr_w(V_BSTR(&value)), wine_dbgstr_w(compname) );
    VariantClear( &value );

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, L"NumberOfLogicalProcessors", 0, &value, &type, NULL );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* win2k3 */, "got %#lx\n", hr );
    if (hr == S_OK)
    {
        ok( V_VT( &value ) == VT_I4, "unexpected variant type %#x\n", V_VT( &value ) );
        ok( type == CIM_UINT32, "unexpected type %#lx\n", type );
        trace( "numlogicalprocessors %ld\n", V_I4( &value ) );
    }

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, L"HypervisorPresent", 0, &value, &type, NULL );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* win7 testbot */, "got %#lx\n", hr );
    if (hr == S_OK)
    {
        ok( V_VT( &value ) == VT_BOOL, "unexpected variant type %#x\n", V_VT( &value ) );
        ok( type == CIM_BOOLEAN, "unexpected type %#lx\n", type );
        trace( "HypervisorPresent %d\n", V_BOOL( &value ) );
    }

    check_property( obj, L"NumberOfProcessors", VT_I4, CIM_UINT32 );
    check_property( obj, L"SystemType", VT_BSTR, CIM_STRING );

    type = 0xdeadbeef;
    VariantInit( &value );
    hr = IWbemClassObject_Get( obj, L"UserName", 0, &value, &type, NULL );
    ok( hr == S_OK, "failed to get computer name %#lx\n", hr );
    ok( V_VT( &value ) == VT_BSTR, "unexpected variant type 0x%x\n", V_VT( &value ) );
    ok( type == CIM_STRING, "unexpected type %#lx\n", type );
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
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_SystemEnclosure" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    CIMTYPE type;
    ULONG count;
    VARIANT val;
    DWORD *data;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "IWbemServices_ExecQuery failed %#lx\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    if (hr != S_OK) goto done;

    check_property( obj, L"Caption", VT_BSTR, CIM_STRING );

    type = 0xdeadbeef;
    VariantInit( &val );
    hr = IWbemClassObject_Get( obj, L"ChassisTypes", 0, &val, &type, NULL );
    ok( hr == S_OK, "failed to get chassis types %#lx\n", hr );
    ok( V_VT( &val ) == (VT_I4|VT_ARRAY), "unexpected variant type %#x\n", V_VT( &val ) );
    ok( type == (CIM_UINT16|CIM_FLAG_ARRAY), "unexpected type %#lx\n", type );
    hr = SafeArrayAccessData( V_ARRAY( &val ), (void **)&data );
    ok( hr == S_OK, "SafeArrayAccessData failed %#lx\n", hr );
    if (SUCCEEDED(hr))
    {
        LONG i, lower, upper;

        hr = SafeArrayGetLBound( V_ARRAY( &val ), 1, &lower );
        ok( hr == S_OK, "SafeArrayGetLBound failed %#lx\n", hr );
        hr = SafeArrayGetUBound( V_ARRAY( &val ), 1, &upper );
        ok( hr == S_OK, "SafeArrayGetUBound failed %#lx\n", hr );
        if (V_VT( &val ) == (VT_I4|VT_ARRAY))
        {
            for (i = 0; i < upper - lower + 1; i++)
                trace( "chassis type: %lu\n", data[i] );
        }
        hr = SafeArrayUnaccessData( V_ARRAY( &val ) );
        ok( hr == S_OK, "SafeArrayUnaccessData failed %#lx\n", hr );
    }
    VariantClear( &val );

    check_property( obj, L"Description", VT_BSTR, CIM_STRING );
    check_property( obj, L"LockPresent", VT_BOOL, CIM_BOOLEAN );
    check_property( obj, L"Manufacturer", VT_BSTR, CIM_STRING );
    check_property( obj, L"Name", VT_BSTR, CIM_STRING );
    check_property( obj, L"Tag", VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
done:
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_StdRegProv( IWbemServices *services )
{
    BSTR class = SysAllocString( L"StdRegProv" ), method, name;
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
    ok( hr == S_OK, "got %#lx\n", hr );

    while (IWbemClassObject_NextMethod( reg, 0, &name, &sig_in, &sig_out ) == S_OK)
    {
        SysFreeString( name );
        IWbemClassObject_Release( sig_in );
        IWbemClassObject_Release( sig_out );
    }

    hr = IWbemClassObject_EndMethodEnumeration( reg );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = IWbemClassObject_BeginEnumeration( reg, 0 );
    ok( hr == S_OK, "got %#lx\n", hr );

    while (IWbemClassObject_Next( reg, 0, &name, NULL, NULL, NULL ) == S_OK)
        SysFreeString( name );

    hr = IWbemClassObject_EndEnumeration( reg );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = IWbemClassObject_GetMethod( reg, L"CreateKey", 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get CreateKey method %#lx\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %#lx\n", hr );

    hr = IWbemClassObject_BeginEnumeration( in, 0 );
    ok( hr == S_OK, "failed to start enumeration %#lx\n", hr );

    hr = IWbemClassObject_Next( in, 0, &name, NULL, NULL, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    SysFreeString( name );

    hr = IWbemClassObject_EndEnumeration( in );
    ok( hr == S_OK, "failed to end enumeration %#lx\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000001;
    hr = IWbemClassObject_Put( in, L"hDefKey", 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %#lx\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( L"Software\\StdRegProvTest" );
    hr = IWbemClassObject_Put( in, L"sSubKeyName", 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %#lx\n", hr );

    out = NULL;
    method = SysAllocString( L"CreateKey" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type %#x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %ld\n", V_UI4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type %#lx\n", type );

    res = RegDeleteKeyW( HKEY_CURRENT_USER, L"Software\\StdRegProvTest" );
    ok( !res, "got %ld\n", res );

    VariantClear( &subkey );
    IWbemClassObject_Release( in );
    IWbemClassObject_Release( out );
    IWbemClassObject_Release( sig_in );

    hr = IWbemClassObject_GetMethod( reg, L"EnumKey", 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get EnumKey method %#lx\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %#lx\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000002;
    hr = IWbemClassObject_Put( in, L"hDefKey", 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %#lx\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( L"Software\\microsoft\\Windows\\CurrentVersion" );
    hr = IWbemClassObject_Put( in, L"sSubKeyName", 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %#lx\n", hr );

    out = NULL;
    method = SysAllocString( L"EnumKey" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type 0x%x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %lu\n", V_UI4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type %#lx\n", type );

    check_property( out, L"sNames", VT_BSTR|VT_ARRAY, CIM_STRING|CIM_FLAG_ARRAY );

    VariantClear( &subkey );
    IWbemClassObject_Release( in );
    IWbemClassObject_Release( out );
    IWbemClassObject_Release( sig_in );

    hr = IWbemClassObject_GetMethod( reg, L"EnumValues", 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get EnumValues method %#lx\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %#lx\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000002;
    hr = IWbemClassObject_Put( in, L"hDefKey", 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %#lx\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( L"Software\\microsoft\\Windows\\CurrentVersion" );
    hr = IWbemClassObject_Put( in, L"sSubKeyName", 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %#lx\n", hr );

    out = NULL;
    method = SysAllocString( L"EnumValues" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type %#x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %ld\n", V_I4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type %#lx\n", type );

    check_property( out, L"sNames", VT_BSTR|VT_ARRAY, CIM_STRING|CIM_FLAG_ARRAY );
    check_property( out, L"Types", VT_I4|VT_ARRAY, CIM_SINT32|CIM_FLAG_ARRAY );

    VariantClear( &subkey );
    IWbemClassObject_Release( in );
    IWbemClassObject_Release( out );
    IWbemClassObject_Release( sig_in );

    hr = IWbemClassObject_GetMethod( reg, L"GetStringValue", 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get GetStringValue method %#lx\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %#lx\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000002;
    hr = IWbemClassObject_Put( in, L"hDefKey", 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %#lx\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( L"Software\\microsoft\\Windows\\CurrentVersion" );
    hr = IWbemClassObject_Put( in, L"sSubKeyName", 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %#lx\n", hr );

    V_VT( &valuename ) = VT_BSTR;
    V_BSTR( &valuename ) = SysAllocString( L"ProgramFilesDir" );
    hr = IWbemClassObject_Put( in, L"sValueName", 0, &valuename, 0 );
    ok( hr == S_OK, "failed to set value name %#lx\n", hr );

    out = NULL;
    method = SysAllocString( L"GetStringValue" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type %#x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %ld\n", V_I4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type %#lx\n", type );

    check_property( out, L"sValue", VT_BSTR, CIM_STRING );

    VariantClear( &valuename );
    VariantClear( &subkey );
    IWbemClassObject_Release( in );
    IWbemClassObject_Release( out );
    IWbemClassObject_Release( sig_in );

    hr = IWbemClassObject_GetMethod( reg, L"GetBinaryValue", 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get GetStringValue method %#lx\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %#lx\n", hr );

    V_VT( &defkey ) = VT_I4;
    V_I4( &defkey ) = 0x80000001;
    hr = IWbemClassObject_Put( in, L"hDefKey", 0, &defkey, 0 );
    ok( hr == S_OK, "failed to set root %#lx\n", hr );

    V_VT( &subkey ) = VT_BSTR;
    V_BSTR( &subkey ) = SysAllocString( L"Control Panel\\Desktop" );
    hr = IWbemClassObject_Put( in, L"sSubKeyName", 0, &subkey, 0 );
    ok( hr == S_OK, "failed to set subkey %#lx\n", hr );

    V_VT( &valuename ) = VT_BSTR;
    V_BSTR( &valuename ) = SysAllocString( L"UserPreferencesMask" );
    hr = IWbemClassObject_Put( in, L"sValueName", 0, &valuename, 0 );
    ok( hr == S_OK, "failed to set value name %#lx\n", hr );

    out = NULL;
    method = SysAllocString( L"GetBinaryValue" );
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    type = 0xdeadbeef;
    VariantInit( &retval );
    hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
    ok( hr == S_OK, "failed to get return value %#lx\n", hr );
    ok( V_VT( &retval ) == VT_I4, "unexpected variant type %#x\n", V_VT( &retval ) );
    ok( !V_I4( &retval ), "unexpected error %ld\n", V_I4( &retval ) );
    ok( type == CIM_UINT32, "unexpected type %#lx\n", type );

    check_property( out, L"uValue", VT_UI1|VT_ARRAY, CIM_UINT8|CIM_FLAG_ARRAY );

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
    trace( "Indicate: %ld, %p\n", count, objects );
    return S_OK;
}

static HRESULT WINAPI sink_SetStatus(
    IWbemObjectSink *iface, LONG flags, HRESULT hresult, BSTR str_param, IWbemClassObject *obj_param )
{
    trace( "SetStatus: %#lx, %#lx, %s, %p\n", flags, hresult, wine_dbgstr_w(str_param), obj_param );
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
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_DeviceChangeEvent" );
    ULONG prev_sink_refs;
    HRESULT hr;

    hr = IWbemServices_ExecNotificationQueryAsync( services, wql, query, 0, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %#lx\n", hr );

    prev_sink_refs = sink_refs;
    hr = IWbemServices_ExecNotificationQueryAsync( services, wql, query, 0, NULL, &sink );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND), "got %#lx\n", hr );
    ok( sink_refs > prev_sink_refs || broken(!sink_refs), "got %lu refs\n", sink_refs );

    hr =  IWbemServices_CancelAsyncCall( services, &sink );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND), "got %#lx\n", hr );

    SysFreeString( wql );
    SysFreeString( query );
}

static void test_query_async( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_Process" );
    HRESULT hr;

    hr = IWbemServices_ExecQueryAsync( services, wql, query, 0, NULL, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %#lx\n", hr );

    hr = IWbemServices_ExecQueryAsync( services, wql, query, 0, NULL, &sink );
    ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND), "got %#lx\n", hr );

    hr =  IWbemServices_CancelAsyncCall( services, NULL );
    ok( hr == WBEM_E_INVALID_PARAMETER, "got %#lx\n", hr );

    hr =  IWbemServices_CancelAsyncCall( services, &sink );
    ok( hr == S_OK, "got %#lx\n", hr );

    SysFreeString( wql );
    SysFreeString( query );
}

static void test_query_semisync( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_Dummy" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    ULONG count;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
            NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    count = 1;
    obj = (void *)0xdeadbeef;
    hr = IEnumWbemClassObject_Next( result, -1, 1, &obj, &count );
    todo_wine
    ok( hr == WBEM_E_INVALID_CLASS, "unexpected hr %#lx\n", hr );
    ok( count == 0, "unexpected count %lu\n", count );
    ok( obj == (void *)0xdeadbeef, "got object %p\n", obj );

    IEnumWbemClassObject_Release( result );

    SysFreeString( wql );
    SysFreeString( query );
}

static void test_GetNames( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_OperatingSystem" );
    IEnumWbemClassObject *result;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        IWbemClassObject *obj;
        SAFEARRAY *names;
        ULONG count;
        VARIANT val;

        IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (!count) break;

        hr = IWbemClassObject_GetNames( obj, NULL, 0, NULL, NULL );
        ok( hr == WBEM_E_INVALID_PARAMETER, "got %#lx\n", hr );

        hr = IWbemClassObject_GetNames( obj, NULL, 0, NULL, &names );
        ok( hr == S_OK, "got %#lx\n", hr );
        SafeArrayDestroy( names );

        hr = IWbemClassObject_GetNames( obj, NULL, WBEM_FLAG_ALWAYS, NULL, &names );
        ok( hr == S_OK, "got %#lx\n", hr );
        SafeArrayDestroy( names );

        hr = IWbemClassObject_GetNames( obj, NULL, WBEM_FLAG_ALWAYS | WBEM_MASK_CONDITION_ORIGIN, NULL, &names );
        ok( hr == S_OK, "got %#lx\n", hr );
        SafeArrayDestroy( names );

        VariantInit( &val );
        hr = IWbemClassObject_GetNames( obj, NULL, WBEM_FLAG_NONSYSTEM_ONLY, &val, &names );
        ok( hr == S_OK, "got %#lx\n", hr );

        SafeArrayDestroy( names );
        IWbemClassObject_Release( obj );
    }
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_SystemSecurity( IWbemServices *services )
{
    BSTR class = SysAllocString( L"__SystemSecurity" ), method;
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
    method = SysAllocString( L"gETsd" ); /* Also test case insensitivity here */
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, NULL, &out, NULL );
    ok( hr == S_OK || hr == WBEM_E_ACCESS_DENIED, "failed to execute method %#lx\n", hr );
    SysFreeString( method );

    if (SUCCEEDED(hr))
    {
        type = 0xdeadbeef;
        VariantInit( &retval );
        hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &retval, &type, NULL );
        ok( hr == S_OK, "failed to get return value %#lx\n", hr );
        ok( V_VT( &retval ) == VT_I4, "unexpected variant type %#x\n", V_VT( &retval ) );
        ok( !V_I4( &retval ), "unexpected error %lu\n", V_UI4( &retval ) );
        ok( type == CIM_UINT32, "unexpected type %#lx\n", type );

        type = 0xdeadbeef;
        VariantInit( &var_sd );
        hr = IWbemClassObject_Get( out, L"SD", 0, &var_sd, &type, NULL );
        ok( hr == S_OK, "failed to get names %#lx\n", hr );
        ok( V_VT( &var_sd ) == (VT_UI1|VT_ARRAY), "unexpected variant type %#x\n", V_VT( &var_sd ) );
        ok( type == (CIM_UINT8|CIM_FLAG_ARRAY), "unexpected type %#lx\n", type );

        hr = SafeArrayAccessData( V_ARRAY( &var_sd ), &data );
        ok( hr == S_OK, "SafeArrayAccessData failed %#lx\n", hr );
        if (SUCCEEDED(hr))
        {
            sd = data;

            ok( (sd->Control & SE_SELF_RELATIVE) == SE_SELF_RELATIVE, "relative flag unset\n" );
            ok( sd->Owner != 0, "no owner SID\n");
            ok( sd->Group != 0, "no owner SID\n");
            ok( EqualSid( (PSID)((LPBYTE)sd + sd->Owner), sid_admin ), "unexpected owner SID\n" );
            ok( EqualSid( (PSID)((LPBYTE)sd + sd->Group), sid_admin ), "unexpected group SID\n" );

            hr = SafeArrayUnaccessData( V_ARRAY( &var_sd ) );
            ok( hr == S_OK, "SafeArrayUnaccessData failed %#lx\n", hr );
        }

        VariantClear( &var_sd );
        IWbemClassObject_Release( out );
    }
    else if (hr == WBEM_E_ACCESS_DENIED)
        win_skip( "insufficient privs to test __SystemSecurity\n" );

    SysFreeString( class );
}

static void test_Win32_NetworkAdapter( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_NetworkAdapter" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Description", VT_BSTR, CIM_STRING );
        check_property( obj, L"DeviceID", VT_BSTR, CIM_STRING );
        check_property( obj, L"Index", VT_I4, CIM_UINT32 );
        check_property( obj, L"Name", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"ServiceName", VT_BSTR, CIM_STRING );

        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );

    query = SysAllocString( L"SELECT * FROM Win32_NetworkAdapter WHERE PNPDeviceID LIKE \"PCI\\\\%\"" );
    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );
    SysFreeString( query );

    query = SysAllocString( L"SELECT * FROM Win32_NetworkAdapter WHERE PNPDeviceID LIKE \"PCI\\%\"" );
    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    todo_wine ok( hr == WBEM_E_INVALID_QUERY, "got %#lx\n", hr );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_NetworkAdapterConfiguration( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_NetworkAdapterConfiguration" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Description", VT_BSTR, CIM_STRING );
        check_property( obj, L"Index", VT_I4, CIM_UINT32 );
        check_property( obj, L"IPEnabled", VT_BOOL, CIM_BOOLEAN );
        check_property_nullable( obj, L"DNSDomain", VT_BSTR, CIM_STRING );

        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_OperatingSystem( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_OperatingSystem" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    ULONG count;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "IWbemServices_ExecQuery failed %#lx\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "IEnumWbemClassObject_Next failed %#lx\n", hr );

    hr = IWbemClassObject_BeginEnumeration( obj, 0 );
    ok( hr == S_OK, "got %#lx\n", hr );

    while (IWbemClassObject_Next( obj, 0, NULL, NULL, NULL, NULL ) == S_OK) {}

    hr = IWbemClassObject_EndEnumeration( obj );
    ok( hr == S_OK, "got %#lx\n", hr );

    check_property( obj, L"BootDevice", VT_BSTR, CIM_STRING );
    check_property( obj, L"BuildNumber", VT_BSTR, CIM_STRING );
    check_property( obj, L"BuildType", VT_BSTR, CIM_STRING );
    check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
    check_property_nullable( obj, L"CSDVersion", VT_BSTR, CIM_STRING );
    check_property( obj, L"FreePhysicalMemory", VT_BSTR, CIM_UINT64 );
    check_property( obj, L"FreeVirtualMemory", VT_BSTR, CIM_UINT64 );
    check_property( obj, L"Name", VT_BSTR, CIM_STRING );
    check_property( obj, L"OperatingSystemSKU", VT_I4, CIM_UINT32 );
    check_property( obj, L"OSProductSuite", VT_I4, CIM_UINT32 );
    check_property( obj, L"CSName", VT_BSTR, CIM_STRING );
    check_property( obj, L"CurrentTimeZone", VT_I2, CIM_SINT16 );
    check_property( obj, L"Manufacturer", VT_BSTR, CIM_STRING );
    check_property_nullable( obj, L"Organization", VT_BSTR, CIM_STRING );
    check_property( obj, L"OSType", VT_I4, CIM_UINT16 );
    check_property( obj, L"ProductType", VT_I4, CIM_UINT32 );
    check_property_nullable( obj, L"RegisteredUser", VT_BSTR, CIM_STRING );
    check_property( obj, L"ServicePackMajorVersion", VT_I4, CIM_UINT16 );
    check_property( obj, L"ServicePackMinorVersion", VT_I4, CIM_UINT16 );
    check_property( obj, L"SuiteMask", VT_I4, CIM_UINT32 );
    check_property( obj, L"Version", VT_BSTR, CIM_STRING );
    check_property( obj, L"TotalVisibleMemorySize", VT_BSTR, CIM_UINT64 );
    check_property( obj, L"TotalVirtualMemorySize", VT_BSTR, CIM_UINT64 );
    check_property( obj, L"Status", VT_BSTR, CIM_STRING );
    check_property( obj, L"SystemDrive", VT_BSTR, CIM_STRING );
    check_property( obj, L"WindowsDirectory", VT_BSTR, CIM_STRING );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_ComputerSystemProduct( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_ComputerSystemProduct" );
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
    if (hr == S_OK)
    {
        check_property( obj, L"IdentifyingNumber", VT_BSTR, CIM_STRING );
        check_property( obj, L"Name", VT_BSTR, CIM_STRING );
        check_property( obj, L"SKUNumber", VT_NULL, CIM_STRING );
        check_property( obj, L"UUID", VT_BSTR, CIM_STRING );
        check_property( obj, L"Vendor", VT_BSTR, CIM_STRING );
        check_property( obj, L"Version", VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_PageFileUsage( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_PageFileUsage" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK , "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Name", VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_PhysicalMemory( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_PhysicalMemory" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    CIMTYPE type;
    VARIANT val;
    DWORD count;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_PhysicalMemory not available\n" );
        return;
    }

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"BankLabel", VT_BSTR, CIM_STRING );
        check_property( obj, L"Capacity", VT_BSTR, CIM_UINT64 );
        check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
        check_property( obj, L"DeviceLocator", VT_BSTR, CIM_STRING );
        check_property( obj, L"FormFactor", VT_I4, CIM_UINT16 );
        check_property( obj, L"MemoryType", VT_I4, CIM_UINT16 );

        type = 0xdeadbeef;
        VariantInit( &val );
        hr = IWbemClassObject_Get( obj, L"ConfiguredClockSpeed", 0, &val, &type, NULL );
        ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* < win10 */, "got %#lx\n", hr );
        if (hr == S_OK)
        {
            ok( V_VT( &val ) == VT_I4, "unexpected variant type %#x\n", V_VT( &val ) );
            ok( type == CIM_UINT32, "unexpected type %#lx\n", type );
            trace( "ConfiguredClockSpeed %ld\n", V_I4( &val ) );
        }

        check_property_nullable( obj, L"Manufacturer", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"PartNumber", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"SerialNumber", VT_BSTR, CIM_STRING );

        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_PhysicalMemoryArray( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_PhysicalMemoryArray" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    CIMTYPE type;
    VARIANT val;
    DWORD count;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_PhysicalMemoryArray not available\n" );
        return;
    }

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
        check_property( obj, L"MaxCapacity", VT_I4, CIM_UINT32 );
        check_property( obj, L"Model", VT_NULL, CIM_STRING );
        check_property( obj, L"Name", VT_BSTR, CIM_STRING );
        check_property( obj, L"Status", VT_NULL, CIM_STRING );

        type = 0xdeadbeef;
        VariantInit( &val );
        hr = IWbemClassObject_Get( obj, L"MaxCapacityEx", 0, &val, &type, NULL );
        ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* < win10 */, "got %#lx\n", hr );
        if (hr == S_OK)
        {
            ok( V_VT( &val ) == VT_BSTR, "unexpected variant type %#x\n", V_VT( &val ) );
            ok( type == CIM_UINT64, "unexpected type %#lx\n", type );
            VariantClear( &val );
        }

        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_IP4RouteTable( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_IP4RouteTable" );
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

        check_property( obj, L"Destination", VT_BSTR, CIM_STRING );
        check_property( obj, L"InterfaceIndex", VT_I4, CIM_SINT32 );
        check_property( obj, L"NextHop", VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Processor( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_Processor" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    VARIANT val;
    CIMTYPE type;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Architecture", VT_I4, CIM_UINT16 );
        check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
        check_property( obj, L"CpuStatus", VT_I4, CIM_UINT16 );
        check_property( obj, L"Family", VT_I4, CIM_UINT16 );
        check_property( obj, L"Level", VT_I4, CIM_UINT16 );
        check_property( obj, L"Manufacturer", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"Name", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"ProcessorId", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"Revision", VT_I4, CIM_UINT16 );
        check_property( obj, L"Version", VT_BSTR, CIM_STRING );

        type = 0xdeadbeef;
        VariantInit( &val );
        hr = IWbemClassObject_Get( obj, L"NumberOfLogicalProcessors", 0, &val, &type, NULL );
        ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* win2k3 */, "got %#lx\n", hr );
        if (hr == S_OK)
        {
            ok( V_VT( &val ) == VT_I4, "unexpected variant type %#x\n", V_VT( &val ) );
            ok( type == CIM_UINT32, "unexpected type %#lx\n", type );
            trace( "numlogicalprocessors %ld\n", V_I4( &val ) );
        }

        type = 0xdeadbeef;
        VariantInit( &val );
        hr = IWbemClassObject_Get( obj, L"NumberOfCores", 0, &val, &type, NULL );
        ok( hr == S_OK || broken(hr == WBEM_E_NOT_FOUND) /* win2k3 */, "got %#lx\n", hr );
        if (hr == S_OK)
        {
            ok( V_VT( &val ) == VT_I4, "unexpected variant type %#x\n", V_VT( &val ) );
            ok( type == CIM_UINT32, "unexpected type %#lx\n", type );
            trace( "numcores %ld\n", V_I4( &val ) );
        }

        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_CacheMemory( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_CacheMemory" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    DWORD count;
    HRESULT hr;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"BlockSize", VT_BSTR, CIM_UINT64 );
        check_property( obj, L"CacheSpeed", VT_I4, CIM_UINT32 );
        check_property( obj, L"CacheType", VT_I4, CIM_UINT16 );
        check_property( obj, L"DeviceID", VT_BSTR, CIM_STRING );
        check_property( obj, L"InstalledSize", VT_I4, CIM_UINT32 );
        check_property( obj, L"Level", VT_I4, CIM_UINT16 );
        check_property( obj, L"MaxCacheSize", VT_I4, CIM_UINT32 );
        check_property( obj, L"NumberOfBlocks", VT_BSTR, CIM_UINT64 );
        check_property( obj, L"Status", VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_VideoController( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_VideoController" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
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

        check_property( obj, L"__CLASS", VT_BSTR, CIM_STRING );
        check_property( obj, L"__DERIVATION", VT_BSTR | VT_ARRAY, CIM_STRING | CIM_FLAG_ARRAY );
        check_property( obj, L"__GENUS", VT_I4, CIM_SINT32 );
        check_property( obj, L"__NAMESPACE", VT_BSTR, CIM_STRING );
        check_property( obj, L"__PATH", VT_BSTR, CIM_STRING );
        check_property( obj, L"__PROPERTY_COUNT", VT_I4, CIM_SINT32 );
        check_property( obj, L"__RELPATH", VT_BSTR, CIM_STRING );
        check_property( obj, L"__SERVER", VT_BSTR, CIM_STRING );
        check_property( obj, L"AdapterCompatibility", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"AdapterDACType", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"AdapterRAM", VT_I4, CIM_UINT32 );
        check_property( obj, L"Availability", VT_I4, CIM_UINT16 );
        check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
        check_property( obj, L"ConfigManagerErrorCode", VT_I4, CIM_UINT32 );
        check_property( obj, L"DriverDate", VT_BSTR, CIM_DATETIME );
        check_property( obj, L"DriverVersion", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"InstalledDisplayDrivers", VT_BSTR, CIM_STRING );
        check_property( obj, L"Status", VT_BSTR, CIM_STRING );

        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );

    query = SysAllocString( L"SELECT AdapterRAM FROM Win32_VideoController" );
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
        check_property( obj, L"__CLASS", VT_BSTR, CIM_STRING );
        check_property( obj, L"__GENUS", VT_I4, CIM_SINT32 );
        check_property( obj, L"__NAMESPACE", VT_NULL, CIM_STRING );
        check_property( obj, L"__PATH", VT_NULL, CIM_STRING );
        check_property( obj, L"__PROPERTY_COUNT", VT_I4, CIM_SINT32 );
        check_property( obj, L"__RELPATH", VT_NULL, CIM_STRING );
        check_property( obj, L"__SERVER", VT_NULL, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Volume( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_Volume" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    if (hr != S_OK)
    {
        win_skip( "Win32_Volume not available\n" );
        return;
    }

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"DeviceID", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"DriveLetter", VT_BSTR, CIM_STRING );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_Printer( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_Printer" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
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

        check_property( obj, L"Attributes", VT_I4, CIM_UINT32 );
        check_property( obj, L"DeviceId", VT_BSTR, CIM_STRING );
        check_property( obj, L"HorizontalResolution", VT_I4, CIM_UINT32 );
        check_property_nullable( obj, L"Location", VT_BSTR, CIM_STRING );
        check_property( obj, L"PortName", VT_BSTR, CIM_STRING );
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

    bstr = SysAllocString( L"Win32_PnPEntity" );
    hr = IWbemServices_CreateInstanceEnum( services, bstr, 0, NULL, &enm );
    ok( hr == S_OK, "got %#lx\n", hr );
    SysFreeString( bstr );

    bstr = SysAllocString( L"DeviceId" );
    while (1)
    {
        hr = IEnumWbemClassObject_Next( enm, 1000, 1, &obj, &count );
        ok( (count == 1 && (hr == WBEM_S_FALSE || hr == WBEM_S_NO_ERROR)) ||
                (count == 0 && (hr == WBEM_S_FALSE || hr == WBEM_S_TIMEDOUT)),
                "got %#lx with %lu objects returned\n", hr, count );

        if (count == 0)
            break;

        for (i = 0; i < count; ++i)
        {
            hr = IWbemClassObject_Get( obj, bstr, 0, &val, &type, NULL );
            ok( hr == S_OK, "got %#lx\n", hr );

            if (SUCCEEDED( hr ))
            {
                ok( V_VT( &val ) == VT_BSTR, "unexpected variant type %#x\n", V_VT( &val ) );
                ok( type == CIM_STRING, "unexpected type %#lx\n", type );
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
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_WinSAT" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK || broken(hr == WBEM_E_INVALID_CLASS) /* win2k8 */, "got %#lx\n", hr );
    if (hr == WBEM_E_INVALID_CLASS)
    {
        win_skip( "class not found\n" );
        return;
    }

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"CPUScore", VT_R4, CIM_REAL32 );
        check_property( obj, L"D3DScore", VT_R4, CIM_REAL32 );
        check_property( obj, L"DiskScore", VT_R4, CIM_REAL32 );
        check_property( obj, L"GraphicsScore", VT_R4, CIM_REAL32 );
        check_property( obj, L"MemoryScore", VT_R4, CIM_REAL32 );
        check_property( obj, L"WinSATAssessmentState", VT_I4, CIM_UINT32 );
        check_property( obj, L"WinSPRLevel", VT_R4, CIM_REAL32 );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_SoftwareLicensingProduct( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM SoftwareLicensingProduct" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK , "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;
        check_property( obj, L"ApplicationId", VT_BSTR, CIM_STRING );
        check_property( obj, L"LicenseIsAddon", VT_BOOL, CIM_BOOLEAN );
        check_property( obj, L"LicenseStatus", VT_I4, CIM_UINT32 );
        check_property_nullable( obj, L"PartialProductKey", VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_DesktopMonitor( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_DesktopMonitor" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Name", VT_BSTR, CIM_STRING );
        check_property( obj, L"PixelsPerXlogicalInch", VT_I4, CIM_UINT32 );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_DiskDrive( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_DiskDrive" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"DeviceID", VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_DisplayControllerConfiguration( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" );
    BSTR query = SysAllocString( L"SELECT * FROM Win32_DisplayControllerConfiguration" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"BitsPerPixel", VT_I4, CIM_UINT32 );
        check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
        check_property( obj, L"HorizontalResolution", VT_I4, CIM_UINT32 );
        check_property( obj, L"Name", VT_BSTR, CIM_STRING );
        check_property( obj, L"VerticalResolution", VT_I4, CIM_UINT32 );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_QuickFixEngineering( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_QuickFixEngineering" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count, total = 0;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
        check_property( obj, L"Description", VT_BSTR, CIM_STRING );
        check_property( obj, L"HotFixID", VT_BSTR, CIM_STRING );
        check_property( obj, L"InstalledBy", VT_BSTR, CIM_STRING );
        check_property( obj, L"InstalledOn", VT_BSTR, CIM_STRING );

        IWbemClassObject_Release( obj );
        if (total++ >= 10) break;
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_SoundDevice( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" );
    BSTR query = SysAllocString( L"SELECT * FROM Win32_SoundDevice" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
        check_property( obj, L"DeviceID", VT_BSTR, CIM_STRING );
        check_property( obj, L"Manufacturer", VT_BSTR, CIM_STRING );
        check_property( obj, L"Name", VT_BSTR, CIM_STRING );
        check_property( obj, L"PNPDeviceID", VT_BSTR, CIM_STRING );
        check_property( obj, L"ProductName", VT_BSTR, CIM_STRING );
        check_property( obj, L"Status", VT_BSTR, CIM_STRING );
        check_property( obj, L"StatusInfo", VT_I4, CIM_UINT16 );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_SystemRestore( IWbemServices *services )
{
    WCHAR path[MAX_PATH];
    BSTR class, method;
    IWbemClassObject *service, *sig_in, *in, *out;
    VARIANT var;
    HRESULT hr;

    class = SysAllocString( L"SystemRestore" );
    hr = IWbemServices_GetObject( services, class, 0, NULL, &service, NULL );
    if (hr != S_OK)
    {
        win_skip( "SystemRestore not available\n" );
        SysFreeString( class );
        return;
    }

    check_property( service, L"CreationTime", VT_NULL, CIM_STRING );
    check_property( service, L"Description", VT_NULL, CIM_STRING );
    if (0) /* FIXME */
    {
    check_property( service, L"EventType", VT_NULL, CIM_UINT32 );
    check_property( service, L"RestorePointType", VT_NULL, CIM_UINT32 );
    check_property( service, L"SequenceNumber", VT_NULL, CIM_UINT32 );
    }

    method = SysAllocString( L"Enable" );
    sig_in = NULL;
    hr = IWbemClassObject_GetMethod( service, method, 0, &sig_in, NULL );
    ok( hr == S_OK, "failed to get Enable method %#lx\n", hr );

    hr = IWbemClassObject_SpawnInstance( sig_in, 0, &in );
    ok( hr == S_OK, "failed to spawn instance %#lx\n", hr );

    GetWindowsDirectoryW(path, ARRAY_SIZE(path));
    V_VT( &var ) = VT_BSTR;
    V_BSTR( &var ) = SysAllocString( path );
    hr = IWbemClassObject_Put( in, L"Drive", 0, &var, 0 );
    ok( hr == S_OK, "failed to set Drive %#lx\n", hr );
    SysFreeString( V_BSTR( &var ) );

    out = NULL;
    hr = IWbemServices_ExecMethod( services, class, method, 0, NULL, in, &out, NULL );
    ok( hr == S_OK || hr == WBEM_E_ACCESS_DENIED, "failed to execute method %#lx\n", hr );
    if (hr == S_OK)
    {
        VariantInit( &var );
        hr = IWbemClassObject_Get( out, L"ReturnValue", 0, &var, NULL, NULL );
        ok( hr == S_OK, "failed to get return value %#lx\n", hr );
        ok( V_I4( &var ) == ERROR_SUCCESS, "unexpected error %ld\n", V_UI4( &var ) );

        IWbemClassObject_Release( out );
    }
    else if (hr == WBEM_E_ACCESS_DENIED)
        win_skip( "insufficient privs to test SystemRestore\n" );

    IWbemClassObject_Release( in );
    IWbemClassObject_Release( sig_in );
    IWbemClassObject_Release( service );
    SysFreeString( method );
    SysFreeString( class );
}

static void test_Win32_LocalTime( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_LocalTime" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "got %#lx\n", hr );

    check_property( obj, L"Day", VT_I4, CIM_UINT32 );
    check_property( obj, L"DayOfWeek", VT_I4, CIM_UINT32 );
    check_property( obj, L"Month", VT_I4, CIM_UINT32 );
    check_property( obj, L"Quarter", VT_I4, CIM_UINT32 );
    check_property( obj, L"WeekInMonth", VT_I4, CIM_UINT32 );
    check_property( obj, L"Year", VT_I4, CIM_UINT32 );

    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_Win32_LogicalDisk( IWbemServices *services )
{
    BSTR wql = SysAllocString( L"wql" ), query = SysAllocString( L"SELECT * FROM Win32_LogicalDisk" );
    IEnumWbemClassObject *result;
    IWbemClassObject *obj;
    HRESULT hr;
    DWORD count;

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        check_property( obj, L"Caption", VT_BSTR, CIM_STRING );
        check_property( obj, L"Name", VT_BSTR, CIM_STRING );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    SysFreeString( query );

    query = SysAllocString( L"SELECT * FROM Win32_LogicalDisk WHERE DeviceID > 'b:' AND DeviceID < 'd:'" );
    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );
    count = 0;
    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 1, "got %lu\n", count );
    IWbemClassObject_Release( obj );
    SysFreeString( query );

    query = SysAllocString( L"SELECT * FROM Win32_LogicalDisk WHERE DeviceID = 'C:'" );
    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );
    count = 0;
    hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
    ok( hr == S_OK, "got %#lx\n", hr );
    ok( count == 1, "got %lu\n", count );
    IWbemClassObject_Release( obj );
    IEnumWbemClassObject_Release( result );
    SysFreeString( query );

    query = SysAllocString( L"Win32_LogicalDisk = \"C:\"" );
    hr = IWbemServices_GetObject( services, query, 0, NULL, &obj, NULL );
    ok( hr == WBEM_E_INVALID_OBJECT_PATH, "got %#lx\n", hr );
    SysFreeString( query );

    query = SysAllocString( L"Win32_LogicalDisk=\"C:\"" );
    hr = IWbemServices_GetObject( services, query, 0, NULL, &obj, NULL );
    ok( hr == S_OK, "got %#lx\n", hr );
    IWbemClassObject_Release( obj );
    SysFreeString( query );
    SysFreeString( wql );
}

static void test_empty_namespace( IWbemLocator *locator )
{
    BSTR path = SysAllocString( L"ROOT" );
    BSTR wql = SysAllocString( L"wql" );
    IEnumWbemClassObject *result;
    IWbemServices *services;
    BSTR query;
    HRESULT hr;

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %#lx\n", hr );

    query = SysAllocString( L"SELECT * FROM __ASSOCIATORS" );
    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_CLASS, "Query failed: %#lx\n", hr );
    SysFreeString( query );

    query = SysAllocString( L"SELECT * FROM Win32_OperatingSystem" );
    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == WBEM_E_INVALID_CLASS, "got %#lx\n", hr );
    SysFreeString( query );

    SysFreeString( wql );
    SysFreeString( path );
    IWbemServices_Release( services );
}

static void test_MSSMBios_RawSMBiosTables( IWbemLocator *locator )
{
    BSTR path = SysAllocString( L"ROOT\\WMI" );
    BSTR bios = SysAllocString( L"MSSMBios_RawSMBiosTables" );
    IWbemServices *services;
    IEnumWbemClassObject *iter;
    IWbemClassObject *obj;
    ULONG count;
    HRESULT hr;

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %#lx\n", hr );

    hr = IWbemServices_CreateInstanceEnum( services, bios, 0, NULL, &iter );
    ok( hr == S_OK, "got %#lx\n", hr );

    hr = IEnumWbemClassObject_Next( iter, WBEM_INFINITE, 1, &obj, &count );
    if (hr != S_OK) goto done;

    check_property( obj, L"Active", VT_BOOL, CIM_BOOLEAN );
    check_property( obj, L"DmiRevision", VT_UI1, CIM_UINT8 );
    check_property( obj, L"InstanceName", VT_BSTR, CIM_STRING );
    check_property( obj, L"Size", VT_I4, CIM_UINT32 );
    check_property( obj, L"SMBiosData", VT_ARRAY | VT_UI1, CIM_FLAG_ARRAY | CIM_UINT8 );
    check_property( obj, L"SmbiosMajorVersion", VT_UI1, CIM_UINT8 );
    check_property( obj, L"SmbiosMinorVersion", VT_UI1, CIM_UINT8 );
    check_property( obj, L"Used20CallingMethod", VT_BOOL, CIM_BOOLEAN );

    IWbemClassObject_Release( obj );
done:
    IEnumWbemClassObject_Release( iter );
    IWbemServices_Release( services );
    SysFreeString( path );
    SysFreeString( bios );
}

static void test_MSFT_PhysicalDisk( IWbemLocator *locator )
{
    BSTR path = SysAllocString( L"ROOT\\Microsoft\\Windows\\Storage" );
    BSTR query = SysAllocString( L"SELECT * FROM MSFT_PhysicalDisk" );
    BSTR wql = SysAllocString( L"wql" );
    IEnumWbemClassObject *result;
    IWbemServices *services;
    IWbemClassObject *obj;
    ULONG count;
    HRESULT hr;

    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %#lx\n", hr );

    hr = IWbemServices_ExecQuery( services, wql, query, 0, NULL, &result );
    ok( hr == S_OK, "got %#lx\n", hr );

    for (;;)
    {
        hr = IEnumWbemClassObject_Next( result, 10000, 1, &obj, &count );
        if (hr != S_OK) break;

        /* Properties not checked with 'if (0)' are absent on older Windows. */
        if (0) check_property_nullable( obj, L"AdapterSerialNumber", VT_BSTR, CIM_STRING );
        check_property( obj, L"AllocatedSize", VT_BSTR, CIM_UINT64 );
        check_property( obj, L"BusType", VT_I4, CIM_UINT16 );
        check_property_nullable( obj, L"CannotPoolReason", VT_ARRAY | VT_I4, CIM_FLAG_ARRAY | CIM_UINT16 );
        check_property( obj, L"CanPool", VT_BOOL, CIM_BOOLEAN );
        check_property_nullable( obj, L"Description", VT_BSTR, CIM_STRING );
        check_property( obj, L"DeviceID", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"EnclosureNumber", VT_I4, CIM_UINT16 );
        check_property( obj, L"FirmwareVersion", VT_BSTR, CIM_STRING );
        check_property( obj, L"FriendlyName", VT_BSTR, CIM_STRING );
        if (0) check_property_nullable( obj, L"FruId", VT_BSTR, CIM_STRING );
        check_property( obj, L"HealthStatus", VT_I4, CIM_UINT16 );
        check_property_nullable( obj, L"IsIndicationEnabled", VT_BOOL, CIM_BOOLEAN );
        check_property( obj, L"IsPartial", VT_BOOL, CIM_BOOLEAN );
        check_property( obj, L"LogicalSectorSize", VT_BSTR, CIM_UINT64 );
        check_property_nullable( obj, L"Manufacturer", VT_BSTR, CIM_STRING );
        check_property( obj, L"MediaType", VT_I4, CIM_UINT16 );
        check_property( obj, L"Model", VT_BSTR, CIM_STRING );
        if (0) check_property_nullable( obj, L"OperationalDetails", VT_ARRAY | VT_BSTR, CIM_FLAG_ARRAY | CIM_STRING );
        check_property( obj, L"OperationalStatus", VT_ARRAY | VT_I4, CIM_FLAG_ARRAY | CIM_UINT16 );
        check_property_nullable( obj, L"OtherCannotPoolReasonDescription", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"PartNumber", VT_BSTR, CIM_STRING );
        check_property_nullable( obj, L"PhysicalLocation", VT_BSTR, CIM_STRING );
        check_property( obj, L"PhysicalSectorSize", VT_BSTR, CIM_UINT64 );
        check_property_nullable( obj, L"SerialNumber", VT_BSTR, CIM_STRING );
        check_property( obj, L"Size", VT_BSTR, CIM_UINT64 );
        check_property_nullable( obj, L"SlotNumber", VT_I4, CIM_UINT16 );
        check_property_nullable( obj, L"SoftwareVersion", VT_BSTR, CIM_STRING );
        check_property( obj, L"SpindleSpeed", VT_I4, CIM_UINT32 );
        check_property( obj, L"SupportedUsages", VT_ARRAY | VT_I4, CIM_FLAG_ARRAY | CIM_UINT16 );
        check_property( obj, L"UniqueId", VT_BSTR, CIM_STRING );
        if (0) check_property( obj, L"UniqueIdFormat", VT_I4, CIM_UINT16 );
        check_property( obj, L"Usage", VT_I4, CIM_UINT16 );
        if (0) check_property( obj, L"VirtualDiskFootprint", VT_BSTR, CIM_UINT64 );
        IWbemClassObject_Release( obj );
    }

    IEnumWbemClassObject_Release( result );
    IWbemServices_Release( services );
    SysFreeString( wql );
    SysFreeString( path );
    SysFreeString( query );
}

START_TEST(query)
{
    BSTR path = SysAllocString( L"ROOT\\CIMV2" );
    IWbemLocator *locator;
    IWbemServices *services;
    DWORD authn_svc;
    char **argv;
    HRESULT hr;
    int argc;

    argc = winetest_get_mainargs( &argv );
    if (argc >= 3)
    {
        if (!strcmp( argv[2], "created_process" ))
            test_Win32_Process_created_process();
        return;
    }

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
    ok( hr == S_OK, "failed to get IWbemServices interface %#lx\n", hr );

    hr = CoQueryProxyBlanket( (IUnknown *)services, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "failed to query proxy blanket %#lx\n", hr );

    hr = CoQueryProxyBlanket( (IUnknown *)services, &authn_svc, NULL, NULL, NULL, NULL, NULL, NULL );
    ok( hr == S_OK, "failed to query proxy blanket %#lx\n", hr );

    hr = CoSetProxyBlanket( (IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );
    ok( hr == S_OK, "failed to set proxy blanket %#lx\n", hr );

    test_GetNames( services );
    test_IEnumWbemClassObject_Next( services );
    test_associators( services );
    test_notification_query_async( services );
    test_query_async( services );
    test_query_semisync( services );
    test_select( services );
    test_like_query( services );
    test_IWbemClassObject_Next( services );

    /* classes */
    test_SoftwareLicensingProduct( services );
    test_StdRegProv( services );
    test_SystemSecurity( services );
    test_Win32_Baseboard( services );
    test_Win32_CacheMemory( services );
    test_Win32_ComputerSystem( services );
    test_Win32_ComputerSystemProduct( services );
    test_Win32_Bios( services );
    test_Win32_DesktopMonitor( services );
    test_Win32_DiskDrive( services );
    test_Win32_DisplayControllerConfiguration( services );
    test_Win32_IP4RouteTable( services );
    test_Win32_LocalTime( services );
    test_Win32_LogicalDisk( services );
    test_Win32_NetworkAdapter( services );
    test_Win32_NetworkAdapterConfiguration( services );
    test_Win32_OperatingSystem( services );
    test_Win32_PageFileUsage( services );
    test_Win32_PhysicalMemory( services );
    test_Win32_PhysicalMemoryArray( services );
    test_Win32_PnPEntity( services );
    test_Win32_Printer( services );
    test_Win32_Process( services, FALSE );
    test_Win32_Process( services, TRUE );
    test_Win32_Processor( services );
    test_Win32_QuickFixEngineering( services );
    test_Win32_Service( services );
    test_Win32_SoundDevice( services );
    test_Win32_SystemEnclosure( services );
    test_Win32_VideoController( services );
    test_Win32_Volume( services );
    test_Win32_WinSAT( services );
    test_SystemRestore( services );
    test_empty_namespace( locator );
    test_MSSMBios_RawSMBiosTables( locator );
    test_MSFT_PhysicalDisk( locator );

    SysFreeString( path );
    IWbemServices_Release( services );

    /* Some tests need other connection point */
    path = SysAllocString( L"ROOT\\DEFAULT" );
    hr = IWbemLocator_ConnectServer( locator, path, NULL, NULL, NULL, 0, NULL, NULL, &services );
    ok( hr == S_OK, "failed to get IWbemServices interface %#lx\n", hr );
    hr = CoSetProxyBlanket( (IUnknown *)services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                            RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE );
    ok( hr == S_OK, "failed to set proxy blanket %#lx\n", hr );

    test_StdRegProv( services );
    test_SystemRestore( services );

    SysFreeString( path );
    IWbemServices_Release( services );
    IWbemLocator_Release( locator );
    CoUninitialize();
}
