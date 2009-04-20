/*
 * SetupAPI device class-related functions tests
 *
 * Copyright 2006 Hervé Poussineau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "cfgmgr32.h"
#include "setupapi.h"

#include "wine/test.h"

static GUID test_class_guid = { 0x4d36e967, 0xe325, 0x11ce, { 0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18 } };
static char test_class_name[MAX_CLASS_NAME_LEN] = "DiskDrive";

static const char *debugstr_guid(const GUID *guid)
{
    static char guidSTR1[39];
    static char guidSTR2[39];
    char* guidSTR;
    static BOOL index;

    if (!guid) return NULL;

    index = !index;
    guidSTR = index ? guidSTR1 : guidSTR2;

    snprintf(guidSTR, sizeof(guidSTR1),
     "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
     guid->Data1, guid->Data2, guid->Data3,
     guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
     guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return guidSTR;
}

static void test_SetupDiBuildClassInfoList(void)
{
    LPGUID guid_list = NULL;
    DWORD required_size, size;

    SetLastError( 0xdeadbeef );
    ok( !SetupDiBuildClassInfoList( 0, NULL, 0, NULL ),
        "Fail expected" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected error %lx, got %lx", ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok( !SetupDiBuildClassInfoList( 0, NULL, 0, &required_size ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "Expected error %lx, got %lx\n", ERROR_INSUFFICIENT_BUFFER, GetLastError() );

    guid_list = HeapAlloc( GetProcessHeap(), 0, ( required_size + 1 ) * sizeof( GUID ) );
    if ( !guid_list )
        return;

    SetLastError( 0xdeadbeef );
    ok( SetupDiBuildClassInfoList( 0, guid_list, required_size, &size ),
        "Error reported %lx\n", GetLastError() );
    ok( size == required_size, "Expected size %lu, got %lu\n", required_size, size );
    SetLastError( 0xdeadbeef );
    ok( SetupDiBuildClassInfoList( 0, guid_list, required_size + 1, &size ),
        "Error reported %lx\n", GetLastError() );
    ok( size == required_size, "Expected size %lu, got %lu\n", required_size, size );

    if ( size > 0 )
    {
       /* That's better to use the first class found, as we know for sure that it exists */
       memcpy(&test_class_guid, &guid_list[0], sizeof( GUID ) );
       SetupDiClassNameFromGuidA( &test_class_guid, test_class_name, sizeof( test_class_name ), NULL );
    }
    HeapFree( GetProcessHeap(), 0, guid_list );
}

static void test_SetupDiClassGuidsFromNameA(void)
{
    LPGUID guid_list = NULL;
    DWORD required_size, size;

    SetLastError( 0xdeadbeef );
    ok( !SetupDiClassGuidsFromNameA( NULL, NULL, 0, NULL ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected error %lx, got %lx\n", ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok( !SetupDiClassGuidsFromNameA( NULL, NULL, 0, &required_size ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected error %lx, got %lx\n", ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok( SetupDiClassGuidsFromNameA( "", NULL, 0, &required_size ),
        "Error reported %lx\n", GetLastError() );
    ok( required_size == 0, "Expected 0, got %lu\n", required_size );

    SetLastError( 0xdeadbeef );
    ok( !SetupDiClassGuidsFromNameA( test_class_name, NULL, 0, &required_size ),
        "Fail expected\n" );
    SetLastError( 0xdeadbeef );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "Expected error %lx, got %lx\n", ERROR_INSUFFICIENT_BUFFER, GetLastError() );
    ok( required_size > 0, "Expected > 0, got %lu\n", required_size );

    guid_list = HeapAlloc( GetProcessHeap(), 0, ( required_size + 1 ) * sizeof( GUID ) );
    if ( !guid_list )
        return;

    SetLastError( 0xdeadbeef );
    ok( SetupDiClassGuidsFromNameA( test_class_name, guid_list, required_size, &size ),
        "Error reported %lx\n", GetLastError() );
    ok( size == required_size, "Expected size %lu, got %lu\n", required_size, size );
    ok( IsEqualIID( &guid_list[0], &test_class_guid ),
        "Expected %s, got %s\n", debugstr_guid( &test_class_guid ), debugstr_guid( &guid_list[0] ) );
    SetLastError( 0xdeadbeef );
    ok( SetupDiClassGuidsFromNameA( test_class_name, guid_list, required_size + 1, &size ),
        "Error reported %lx\n", GetLastError() );
    ok( size == required_size, "Expected size %lu, got %lu\n", required_size, size );
    ok( IsEqualIID( &guid_list[0], &test_class_guid ),
        "Expected %s, got %s\n", debugstr_guid( &test_class_guid ), debugstr_guid( &guid_list[0] ) );

    HeapFree( GetProcessHeap(), 0, guid_list );
}

static void test_SetupDiClassNameFromGuidA(void)
{
    CHAR* class_name = NULL;
    DWORD required_size, size;

    SetLastError( 0xdeadbeef );
    ok( !SetupDiClassNameFromGuidA( NULL, NULL, 0, NULL ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_CLASS,
        "Expected error %x, got %lx\n", ERROR_INVALID_CLASS, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok( !SetupDiClassNameFromGuidA( NULL, NULL, 0, &required_size ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_CLASS,
        "Expected error %x, got %lx\n", ERROR_INVALID_CLASS, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok( !SetupDiClassNameFromGuidA( &test_class_guid, NULL, 0, &required_size ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "Expected error %lx, got %lx\n", ERROR_INSUFFICIENT_BUFFER, GetLastError() );
    ok( required_size > 0, "Expected > 0, got %lu\n", required_size );
    ok( required_size < MAX_CLASS_NAME_LEN, "Expected < %u, got %lu\n", MAX_CLASS_NAME_LEN, required_size );

    class_name = HeapAlloc( GetProcessHeap(), 0, required_size );
    if ( !class_name )
        return;

    SetLastError( 0xdeadbeef );
    ok( SetupDiClassNameFromGuidA( &test_class_guid, class_name, required_size, &size ),
        "Error reported %lx\n", GetLastError() );
    ok( size == required_size, "Expected size %lu, got %lu\n", required_size, size );
    ok( !strcmp( class_name, test_class_name ),
        "Expected %s, got %s\n", test_class_name, class_name );
    SetLastError( 0xdeadbeef );
    ok( SetupDiClassNameFromGuidA( &test_class_guid, class_name, required_size + 1, &size ),
        "Error reported %lx\n", GetLastError() );
    ok( size == required_size, "Expected size %lu, got %lu\n", required_size, size );
    ok( !strcmp( class_name, test_class_name ),
        "Expected %s, got %s\n", test_class_name, class_name );

    HeapFree( GetProcessHeap(), 0, class_name );
}

static void test_SetupDiGetClassDescriptionA(void)
{
    CHAR* class_desc = NULL;
    DWORD required_size, size;

    SetLastError( 0xdeadbeef );
    ok( !SetupDiGetClassDescriptionA( NULL, NULL, 0, NULL ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected error %lx, got %lx\n", ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok( !SetupDiGetClassDescriptionA( NULL, NULL, 0, &required_size ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected error %lx, got %lx\n", ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    ok( !SetupDiGetClassDescriptionA( &test_class_guid, NULL, 0, &required_size ),
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INSUFFICIENT_BUFFER,
        "Expected error %lx, got %lx\n", ERROR_INSUFFICIENT_BUFFER, GetLastError() );
    ok( required_size > 0, "Expected > 0, got %lu\n", required_size );
    ok( required_size < LINE_LEN, "Expected < %u, got %lu\n", LINE_LEN, required_size );

    class_desc = HeapAlloc( GetProcessHeap(), 0, required_size );
    if ( !class_desc )
        return;

    SetLastError( 0xdeadbeef );
    ok( SetupDiGetClassDescriptionA( &test_class_guid, class_desc, required_size, &size ),
        "Error reported %lx\n", GetLastError() );
    ok( size == required_size, "Expected size %lu, got %lu\n", required_size, size );
    SetLastError( 0xdeadbeef );
    ok( SetupDiGetClassDescriptionA( &test_class_guid, class_desc, required_size + 1, &size ),
        "Error reported %lx\n", GetLastError() );
    ok( size == required_size, "Expected size %lu, got %lu\n", required_size, size );

    HeapFree( GetProcessHeap(), 0, class_desc );
}

static void test_SetupDiGetClassDevsA(void)
{
    HDEVINFO device_info;

    SetLastError( 0xdeadbeef );
    device_info = SetupDiGetClassDevs( NULL, NULL, NULL, 0 );
    ok( device_info == INVALID_HANDLE_VALUE,
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected error %lx, got %lx\n", ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    device_info = SetupDiGetClassDevs( NULL, NULL, NULL, DIGCF_ALLCLASSES );
    ok( device_info != INVALID_HANDLE_VALUE,
        "Error reported %lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ok( SetupDiDestroyDeviceInfoList( device_info ),
        "Error reported %lx\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    device_info = SetupDiGetClassDevs( NULL, NULL, NULL, DIGCF_DEVICEINTERFACE );
    ok( device_info == INVALID_HANDLE_VALUE,
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER,
        "Expected error %lx, got %lx\n", ERROR_INVALID_PARAMETER, GetLastError() );

    SetLastError( 0xdeadbeef );
    device_info = SetupDiGetClassDevs( &test_class_guid, NULL, NULL, 0 );
    ok( device_info != INVALID_HANDLE_VALUE,
        "Error reported %lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ok( SetupDiDestroyDeviceInfoList( device_info ),
        "Error reported %lx\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    device_info = SetupDiGetClassDevs( NULL, "(invalid enumerator)", NULL, DIGCF_ALLCLASSES );
    ok( device_info == INVALID_HANDLE_VALUE,
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_DATA,
        "Expected error %lx, got %lx\n", ERROR_INVALID_DATA, GetLastError() );

    SetLastError( 0xdeadbeef );
    device_info = SetupDiGetClassDevs( NULL, "Root", NULL, DIGCF_ALLCLASSES );
    ok( device_info != INVALID_HANDLE_VALUE,
        "Error reported %lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ok( SetupDiDestroyDeviceInfoList( device_info ),
        "Error reported %lx\n", GetLastError() );
}

static void test_SetupDiOpenClassRegKeyExA(void)
{
    HKEY hkey;
    LONG err;

    SetLastError( 0xdeadbeef );
    hkey = SetupDiOpenClassRegKeyExA( NULL, 0, 0, NULL, NULL );
    ok( hkey == INVALID_HANDLE_VALUE,
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_FLAGS,
        "Expected error %lx, got %lx\n", ERROR_INVALID_FLAGS, GetLastError() );

    SetLastError( 0xdeadbeef );
    hkey = SetupDiOpenClassRegKeyExA( NULL, 0, DIOCR_INSTALLER | DIOCR_INTERFACE, NULL, NULL );
    ok( hkey == INVALID_HANDLE_VALUE,
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_FLAGS,
        "Expected error %lx, got %lx\n", ERROR_INVALID_FLAGS, GetLastError() );

    SetLastError( 0xdeadbeef );
    hkey = SetupDiOpenClassRegKeyExA( NULL, 0, DIOCR_INSTALLER, NULL, NULL );
    ok( hkey == INVALID_HANDLE_VALUE,
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_CLASS,
        "Expected error %x, got %lx\n", ERROR_INVALID_CLASS, GetLastError() );

    SetLastError( 0xdeadbeef );
    hkey = SetupDiOpenClassRegKeyExA( NULL, 0, DIOCR_INTERFACE, NULL, NULL );
    ok( hkey == INVALID_HANDLE_VALUE,
        "Fail expected\n" );
    ok( GetLastError() == ERROR_INVALID_CLASS,
        "Expected error %x, got %lx\n", ERROR_INVALID_CLASS, GetLastError() );

    SetLastError( 0xdeadbeef );
    hkey = SetupDiOpenClassRegKeyExA( NULL, KEY_QUERY_VALUE, DIOCR_INSTALLER, NULL, NULL );
    ok( hkey != INVALID_HANDLE_VALUE, "Got error %lx\n", GetLastError() );
    err = RegCloseKey( hkey );
    ok( err == ERROR_SUCCESS, "Got error %lx\n", err );

    SetLastError( 0xdeadbeef );
    hkey = SetupDiOpenClassRegKeyExA( NULL, KEY_QUERY_VALUE, DIOCR_INTERFACE, NULL, NULL );
    ok( hkey != INVALID_HANDLE_VALUE, "Got error %lx\n", GetLastError() );
    err = RegCloseKey( hkey );
    ok( err == ERROR_SUCCESS, "Got error %lx\n", err );

    SetLastError( 0xdeadbeef );
    hkey = SetupDiOpenClassRegKeyExA( &test_class_guid, KEY_QUERY_VALUE, DIOCR_INSTALLER, NULL, NULL );
    ok( hkey != INVALID_HANDLE_VALUE, "Got error %lx\n", GetLastError() );
    err = RegCloseKey( hkey );
    ok( err == ERROR_SUCCESS, "Got error %lx\n", err );

    err = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Class", 0, KEY_SET_VALUE, &hkey);
    ok( err == ERROR_SUCCESS, "Got error %lx\n", err );
}

START_TEST(devclass)
{
    test_SetupDiBuildClassInfoList();
    test_SetupDiClassGuidsFromNameA();
    test_SetupDiClassNameFromGuidA();
    test_SetupDiGetClassDescriptionA();
    test_SetupDiGetClassDevsA();
    test_SetupDiOpenClassRegKeyExA();
}
