/*
 * INF file parsing tests
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
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"

#define TMPFILE ".\\tmp.inf"

#define STD_HEADER "[Version]\r\nSignature=\"$CHICAGO$\"\r\n"

/* create a new file with specified contents and open it */
static HINF test_file_contents( const char *data, UINT *err_line )
{
    DWORD res;
    HANDLE handle = CreateFileA( TMPFILE, GENERIC_READ|GENERIC_WRITE,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE) return 0;
    if (!WriteFile( handle, data, strlen(data), &res, NULL )) trace( "write error\n" );
    CloseHandle( handle );
    return SetupOpenInfFileA( TMPFILE, 0, INF_STYLE_WIN4, err_line );
}

static void test_InstallHinfSectionA(void)
{
    char buffer[MAX_INF_STRING_LENGTH];
    UINT err_line;
    HINF hinf;
    DWORD err;
    DWORD len;

    SetLastError( 0xdeadbeef );
    hinf = test_file_contents( STD_HEADER
        "[s]\r\nAddReg=s.Reg\r\n[s.Reg]\r\nHKCU,,Test,,none\r\n"
        "[s.Win]\r\nAddReg=sWin.Reg\r\n[sWin.Reg]\r\nHKCU,,Test,,win\r\n"
        "[s.NT]\r\nAddReg=sNT.Reg\r\n[sNT.Reg]\r\nHKCU,,Test,,nt\r\n"
        , &err_line );
    ok( hinf != INVALID_HANDLE_VALUE, "open failed err %lx", GetLastError() );
    if ( hinf == INVALID_HANDLE_VALUE ) return;

    system( "rundll32.exe setupapi,InstallHinfSection s 128 " TMPFILE );

    len = sizeof( buffer );
    err = RegQueryValueExA( HKEY_CURRENT_USER, "Test", NULL, NULL, (LPBYTE)buffer, &len );
    ok( err == ERROR_SUCCESS, "error %lx", err);

    if (GetVersion() & 0x80000000)
        ok( !strcmp( buffer, "win" ), "bad section %s/win\n", buffer );
    else
        ok( !strcmp( buffer, "nt" ), "bad section %s/nt\n", buffer );

    err = RegDeleteValue( HKEY_CURRENT_USER, "Test" );
    ok( err == ERROR_SUCCESS, "error %lx", err);
}

static void test_SetupInstallFromInfSectionA(void)
{
    char buffer[MAX_INF_STRING_LENGTH];
    UINT err_line;
    HINF hinf;
    DWORD err;
    DWORD len;

    SetLastError( 0xdeadbeef );
    hinf = test_file_contents( STD_HEADER
        "[s]\r\nAddReg=s.Reg\r\n[s.Reg]\r\nHKR,,Test,,none\r\n"
        "[s.Win]\r\nAddReg=sWin.Reg\r\n[sWin.Reg]\r\nHKR,,Test,,win\r\n"
        "[s.NT]\r\nAddReg=sNT.Reg\r\n[sNT.Reg]\r\nHKR,,Test,,nt\r\n"
        , &err_line );
    ok( hinf != INVALID_HANDLE_VALUE, "open failed err %lx", GetLastError() );
    if (hinf == INVALID_HANDLE_VALUE) return;

    SetLastError( 0xdeadbeef );
    ok ( SetupInstallFromInfSectionA( NULL, hinf, "s", SPINST_REGISTRY, HKEY_CURRENT_USER, NULL, 0, NULL, NULL, NULL, NULL ),
        "Error code set to %lx", GetLastError() );

    len = sizeof( buffer );
    err = RegQueryValueExA( HKEY_CURRENT_USER, "Test", NULL, NULL, (LPBYTE)buffer, &len );
    ok( err == ERROR_SUCCESS, "error %lx", err);
    ok( !strcmp( buffer, "none" ), "bad value %s/none", buffer );

    err = RegDeleteValue( HKEY_CURRENT_USER, "Test" );
    ok( err == ERROR_SUCCESS, "error %lx", err);
}

START_TEST(install)
{
    test_InstallHinfSectionA();
    test_SetupInstallFromInfSectionA();
    DeleteFileA( TMPFILE );
}
