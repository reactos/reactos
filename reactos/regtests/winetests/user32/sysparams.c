/* Unit test suite for functions SystemParametersInfo and GetSystemMetrics.
 *
 * Copyright 2002 Andriy Palamarchuk
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#define _WIN32_WINNT 0x0500 /* For SPI_GETMOUSEHOVERWIDTH and more */

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"

#ifndef SPI_GETDESKWALLPAPER
# define SPI_GETDESKWALLPAPER 0x0073
#endif

static int strict;

#define eq(received, expected, label, type) \
        ok((received) == (expected), "%s: got " type " instead of " type "\n", (label),(received),(expected))


#define SPI_SETBEEP_REGKEY                      "Control Panel\\Sound"
#define SPI_SETBEEP_VALNAME                     "Beep"
#define SPI_SETMOUSE_REGKEY                     "Control Panel\\Mouse"
#define SPI_SETMOUSE_VALNAME1                   "MouseThreshold1"
#define SPI_SETMOUSE_VALNAME2                   "MouseThreshold2"
#define SPI_SETMOUSE_VALNAME3                   "MouseSpeed"
#define SPI_SETBORDER_REGKEY                    "Control Panel\\Desktop\\WindowMetrics"
#define SPI_SETBORDER_VALNAME                   "BorderWidth"
#define SPI_SETKEYBOARDSPEED_REGKEY             "Control Panel\\Keyboard"
#define SPI_SETKEYBOARDSPEED_VALNAME            "KeyboardSpeed"
#define SPI_SETSCREENSAVETIMEOUT_REGKEY         "Control Panel\\Desktop"
#define SPI_SETSCREENSAVETIMEOUT_VALNAME        "ScreenSaveTimeOut"
#define SPI_SETSCREENSAVEACTIVE_REGKEY          "Control Panel\\Desktop"
#define SPI_SETSCREENSAVEACTIVE_VALNAME         "ScreenSaveActive"
#define SPI_SETGRIDGRANULARITY_REGKEY           "Control Panel\\Desktop"
#define SPI_SETGRIDGRANULARITY_VALNAME          "GridGranularity"
#define SPI_SETKEYBOARDDELAY_REGKEY             "Control Panel\\Keyboard"
#define SPI_SETKEYBOARDDELAY_VALNAME            "KeyboardDelay"
#define SPI_SETICONTITLEWRAP_REGKEY1            "Control Panel\\Desktop\\WindowMetrics"
#define SPI_SETICONTITLEWRAP_REGKEY2            "Control Panel\\Desktop"
#define SPI_SETICONTITLEWRAP_VALNAME            "IconTitleWrap"
#define SPI_SETMENUDROPALIGNMENT_REGKEY1        "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"
#define SPI_SETMENUDROPALIGNMENT_REGKEY2        "Control Panel\\Desktop"
#define SPI_SETMENUDROPALIGNMENT_VALNAME        "MenuDropAlignment"
#define SPI_SETDOUBLECLKWIDTH_REGKEY1           "Control Panel\\Mouse"
#define SPI_SETDOUBLECLKWIDTH_REGKEY2           "Control Panel\\Desktop"
#define SPI_SETDOUBLECLKWIDTH_VALNAME           "DoubleClickWidth"
#define SPI_SETDOUBLECLKHEIGHT_REGKEY1          "Control Panel\\Mouse"
#define SPI_SETDOUBLECLKHEIGHT_REGKEY2          "Control Panel\\Desktop"
#define SPI_SETDOUBLECLKHEIGHT_VALNAME          "DoubleClickHeight"
#define SPI_SETDOUBLECLICKTIME_REGKEY           "Control Panel\\Mouse"
#define SPI_SETDOUBLECLICKTIME_VALNAME          "DoubleClickSpeed"
#define SPI_SETMOUSEBUTTONSWAP_REGKEY           "Control Panel\\Mouse"
#define SPI_SETMOUSEBUTTONSWAP_VALNAME          "SwapMouseButtons"
#define SPI_SETWORKAREA_REGKEY                  "Control Panel\\Desktop"
#define SPI_SETWORKAREA_VALNAME                 "WINE_WorkArea"
#define SPI_SETSHOWSOUNDS_REGKEY                "Control Panel\\Accessibility\\ShowSounds"
#define SPI_SETSHOWSOUNDS_VALNAME               "On"
#define SPI_SETKEYBOARDPREF_REGKEY              "Control Panel\\Accessibility\\Keyboard Preference"
#define SPI_SETKEYBOARDPREF_VALNAME             "On"
#define SPI_SETKEYBOARDPREF_REGKEY_LEGACY       "Control Panel\\Accessibility"
#define SPI_SETKEYBOARDPREF_VALNAME_LEGACY      "Keyboard Preference"
#define SPI_SETSCREENREADER_REGKEY              "Control Panel\\Accessibility\\Blind Access"
#define SPI_SETSCREENREADER_VALNAME             "On"
#define SPI_SETSCREENREADER_REGKEY_LEGACY       "Control Panel\\Accessibility"
#define SPI_SETSCREENREADER_VALNAME_LEGACY      "Blind Access"
#define SPI_SETLOWPOWERACTIVE_REGKEY            "Control Panel\\Desktop"
#define SPI_SETLOWPOWERACTIVE_VALNAME           "LowPowerActive"
#define SPI_SETPOWEROFFACTIVE_REGKEY            "Control Panel\\Desktop"
#define SPI_SETPOWEROFFACTIVE_VALNAME           "PowerOffActive"
#define SPI_SETDRAGFULLWINDOWS_REGKEY           "Control Panel\\Desktop"
#define SPI_SETDRAGFULLWINDOWS_VALNAME          "DragFullWindows"
#define SPI_SETMOUSEHOVERWIDTH_REGKEY           "Control Panel\\Mouse"
#define SPI_SETMOUSEHOVERWIDTH_VALNAME          "MouseHoverWidth"
#define SPI_SETMOUSEHOVERHEIGHT_REGKEY          "Control Panel\\Mouse"
#define SPI_SETMOUSEHOVERHEIGHT_VALNAME         "MouseHoverHeight"
#define SPI_SETMOUSEHOVERTIME_REGKEY            "Control Panel\\Mouse"
#define SPI_SETMOUSEHOVERTIME_VALNAME           "MouseHoverTime"
#define SPI_SETMOUSESCROLLLINES_REGKEY          "Control Panel\\Desktop"
#define SPI_SETMOUSESCROLLLINES_VALNAME         "WheelScrollLines"
#define SPI_SETMENUSHOWDELAY_REGKEY             "Control Panel\\Desktop"
#define SPI_SETMENUSHOWDELAY_VALNAME            "MenuShowDelay"
#define SPI_SETDESKWALLPAPER_REGKEY             "Control Panel\\Desktop"
#define SPI_SETDESKWALLPAPER_VALNAME            "Wallpaper"

/* volatile registry branch under CURRENT_USER_REGKEY for temporary values storage */
#define WINE_CURRENT_USER_REGKEY     "Wine"

static HWND ghTestWnd;

static DWORD WINAPI SysParamsThreadFunc( LPVOID lpParam );
static LRESULT CALLBACK SysParamsTestWndProc( HWND hWnd, UINT msg, WPARAM wParam,
                                              LPARAM lParam );
static int change_counter;
static int change_last_param;

static LRESULT CALLBACK SysParamsTestWndProc( HWND hWnd, UINT msg, WPARAM wParam,
                                              LPARAM lParam )
{
    switch (msg) {

    case WM_SETTINGCHANGE:
        if (change_counter>0) {
            ok(0,"too many changes counter=%d last change=%d\n",
               change_counter,change_last_param);
        }
        change_counter++;
        change_last_param = wParam;
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    default:
        return( DefWindowProcA( hWnd, msg, wParam, lParam ) );
    }

    return 0;
}

/*
Performs testing for system parameters messages
params:
 - system parameter id
 - supposed value of the registry key
*/
static void test_change_message( int action, int optional )
{
    if (change_counter==0 && optional==1)
        return;
    ok( 1 == change_counter,
        "Missed a message: change_counter=%d\n", change_counter );
    change_counter = 0;
    ok( action == change_last_param,
        "Wrong action got %d expected %d\n", change_last_param, action );
    change_last_param = 0;
}

static BOOL test_error_msg ( int rc, const char *name )
{
    DWORD last_error = GetLastError();

    if (rc==0)
    {
        if (last_error==0xdeadbeef || last_error==ERROR_INVALID_SPI_VALUE)
        {
            trace("%s not supported on this platform. Skipping test\n", name);
        }
        else if (last_error==ERROR_ACCESS_DENIED)
        {
            trace("%s does not have privileges to run. Skipping test\n", name);
        }
        else
        {
            trace("%s failed for reason: %ld. Indicating test failure and skipping remainder of test\n",name,last_error);
            ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,last_error);
        }
        return FALSE;
    }
    else
    {
        ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,last_error);
        return TRUE;
    }
}

/*
 * Tests the HKEY_CURRENT_USER subkey value.
 * The value should contain string value.
 *
 * Params:
 * lpsSubKey - subkey name
 * lpsRegName - registry entry name
 * lpsTestValue - value to test
 */
static void _test_reg_key( LPCSTR subKey1, LPCSTR subKey2, LPCSTR valName1, LPCSTR valName2, LPCSTR testValue )
{
    CHAR  value[MAX_PATH];
    DWORD valueLen;
    DWORD type;
    HKEY hKey;
    LONG rc;
    int found=0;

    *value='\0';
    valueLen=sizeof(value);
    RegOpenKeyA( HKEY_CURRENT_USER, subKey1, &hKey );
    rc=RegQueryValueExA( hKey, valName1, NULL, &type, (LPBYTE)value, &valueLen );
    RegCloseKey( hKey );
    if (rc==ERROR_SUCCESS)
    {
        ok( !strcmp( testValue, value ),
            "Wrong value in registry: subKey=%s, valName=%s, testValue=%s, value=%s\n",
            subKey1, valName1, testValue, value );
        found++;
    }
    else if (strict)
    {
        ok(0,"Missing registry entry: subKey=%s, valName=%s\n",
           subKey1, valName1);
    }
    if (valName2)
    {
        *value='\0';
        valueLen=sizeof(value);
        RegOpenKeyA( HKEY_CURRENT_USER, subKey1, &hKey );
        rc=RegQueryValueExA( hKey, valName2, NULL, &type, (LPBYTE)value, &valueLen );
        RegCloseKey( hKey );
        if (rc==ERROR_SUCCESS)
        {
            ok( !strcmp( testValue, value ),
                "Wrong value in registry: subKey=%s, valName=%s, testValue=%s, value=%s\n",
                subKey1, valName2, testValue, value );
            found++;
        }
        else if (strict)
        {
            ok( 0,"Missing registry entry: subKey=%s, valName=%s\n",
                subKey1, valName2 );
        }
    }
    if (subKey2 && !strict)
    {
        *value='\0';
        valueLen=sizeof(value);
        RegOpenKeyA( HKEY_CURRENT_USER, subKey2, &hKey );
        rc=RegQueryValueExA( hKey, valName1, NULL, &type, (LPBYTE)value, &valueLen );
        RegCloseKey( hKey );
        if (rc==ERROR_SUCCESS)
        {
            ok( !strcmp( testValue, value ),
                "Wrong value in registry: subKey=%s, valName=%s, testValue=%s, value=%s\n",
                subKey2, valName1, testValue, value );
            found++;
        }
        else if (strict)
        {
            ok( 0,"Missing registry entry: subKey=%s, valName=%s\n",
                subKey2, valName1 );
        }
        if (valName2)
        {
            *value='\0';
            valueLen=sizeof(value);
            RegOpenKeyA( HKEY_CURRENT_USER, subKey2, &hKey );
            rc=RegQueryValueExA( hKey, valName2, NULL, &type, (LPBYTE)value, &valueLen );
            RegCloseKey( hKey );
            if (rc==ERROR_SUCCESS)
            {
                ok( !strcmp( testValue, value ),
                    "Wrong value in registry: subKey=%s, valName=%s, testValue=%s, value=%s\n",
                    subKey2, valName2, testValue, value );
                found++;
            }
            else if (strict)
            {
                ok( 0,"Missing registry entry: subKey=%s, valName=%s\n",
                    subKey2, valName2 );
            }
         }
    }
    ok(found,"Missing registry values: %s or %s in keys: %s or %s\n",
       valName1, (valName2?valName2:"<n/a>"), subKey1, (subKey2?subKey2:"<n/a>") );
}

#define test_reg_key( subKey, valName, testValue ) \
    _test_reg_key( subKey, NULL, valName, NULL, testValue )
#define test_reg_key_ex( subKey1, subKey2, valName, testValue ) \
    _test_reg_key( subKey1, subKey2, valName, NULL, testValue )
#define test_reg_key_ex2( subKey1, subKey2, valName1, valName2, testValue ) \
    _test_reg_key( subKey1, subKey2, valName1, valName2, testValue )

static void test_SPI_SETBEEP( void )                   /*      2 */
{
    BOOL rc;
    BOOL old_b;
    BOOL b;
    BOOL curr_val;

    trace("testing SPI_{GET,SET}BEEP\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETBEEP, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}BEEP"))
        return;

    curr_val = TRUE;
    rc=SystemParametersInfoA( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETBEEP, 0 );
    test_reg_key( SPI_SETBEEP_REGKEY,
                  SPI_SETBEEP_VALNAME,
                  curr_val ? "Yes" : "No" );
    rc=SystemParametersInfoA( SPI_GETBEEP, 0, &b, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( b, curr_val, "SPI_{GET,SET}BEEP", "%d" );
    rc=SystemParametersInfoW( SPI_GETBEEP, 0, &b, 0 );
    if (rc!=0 || GetLastError()!=ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(rc!=0,"SystemParametersInfoW: rc=%d err=%ld\n",rc,GetLastError());
        eq( b, curr_val, "SystemParametersInfoW", "%d" );
    }

    /* is a message sent for the second change? */
    rc=SystemParametersInfoA( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETBEEP, 0 );

    curr_val = FALSE;
    rc=SystemParametersInfoW( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    if (rc==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        rc=SystemParametersInfoA( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc!=0,"SystemParametersInfo: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETBEEP, 0 );
    test_reg_key( SPI_SETBEEP_REGKEY,
                  SPI_SETBEEP_VALNAME,
                  curr_val ? "Yes" : "No" );
    rc=SystemParametersInfoA( SPI_GETBEEP, 0, &b, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( b, curr_val, "SPI_{GET,SET}BEEP", "%d" );
    rc=SystemParametersInfoW( SPI_GETBEEP, 0, &b, 0 );
    if (rc!=0 || GetLastError()!=ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(rc!=0,"SystemParametersInfoW: rc=%d err=%ld\n",rc,GetLastError());
        eq( b, curr_val, "SystemParametersInfoW", "%d" );
    }
    ok( MessageBeep( MB_OK ), "Return value of MessageBeep when sound is disabled\n" );

    rc=SystemParametersInfoA( SPI_SETBEEP, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static const char *setmouse_valuenames[3] = {
    SPI_SETMOUSE_VALNAME1,
    SPI_SETMOUSE_VALNAME2,
    SPI_SETMOUSE_VALNAME3
};

/*
 * Runs check for one setting of spi_setmouse.
 */
static void run_spi_setmouse_test( int curr_val[], POINT *req_change, POINT *proj_change,
                                   int nchange )
{
    BOOL rc;
    INT mi[3];
    static int aw_turn = 0;
    static BOOL w_implemented = 1;

    char buf[20];
    int i;

    aw_turn++;
    rc=0;
    if ((aw_turn % 2!=0) && (w_implemented))
    {
        /* call unicode on odd (non even) calls */ 
        SetLastError(0xdeadbeef);
        rc=SystemParametersInfoW( SPI_SETMOUSE, 0, curr_val, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (rc==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        {
            w_implemented = 0;
            trace("SystemParametersInfoW not supported on this platform\n");
        }
    }

    if ((aw_turn % 2==0) || (!w_implemented))
    {
        /* call ascii version on even calls or if unicode is not available */
        rc=SystemParametersInfoA( SPI_SETMOUSE, 0, curr_val, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    }

    ok(rc!=0,"SystemParametersInfo: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETMOUSE, 0 );
    for (i = 0; i < 3; i++)
    {
        sprintf( buf, "%d", curr_val[i] );
        test_reg_key( SPI_SETMOUSE_REGKEY, setmouse_valuenames[i], buf );
    }

    rc=SystemParametersInfoA( SPI_GETMOUSE, 0, mi, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    for (i = 0; i < 3; i++)
    {
        ok(mi[i] == curr_val[i],
           "incorrect value for %d: %d != %d\n", i, mi[i], curr_val[i]);
    }

    if (w_implemented)
    { 
        rc=SystemParametersInfoW( SPI_GETMOUSE, 0, mi, 0 );
        ok(rc!=0,"SystemParametersInfoW: rc=%d err=%ld\n",rc,GetLastError());
        for (i = 0; i < 3; i++)
        {
            ok(mi[i] == curr_val[i],
               "incorrect value for %d: %d != %d\n", i, mi[i], curr_val[i]);
        }
    }

#if 0  /* FIXME: this always fails for me  - AJ */
    for (i = 0; i < nchange; i++)
    {
        POINT mv;
        mouse_event( MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 0, 0, 0, 0 );
        mouse_event( MOUSEEVENTF_MOVE, req_change[i].x, req_change[i].y, 0, 0 );
        GetCursorPos( &mv );
        ok( proj_change[i].x == mv.x, "Projected dx and real dx comparison. May fail under high load.\n" );
        ok( proj_change[i].y == mv.y, "Projected dy equals real dy. May fail under high load.\n" );
    }
#endif
}

static void test_SPI_SETMOUSE( void )                  /*      4 */
{
    BOOL rc;
    INT old_mi[3];

    /* win nt default values - 6, 10, 1 */
    INT curr_val[3] = {6, 10, 1};

    /* requested and projected mouse movements */
    POINT req_change[] =   { {6, 6}, { 7, 6}, { 8, 6}, {10, 10}, {11, 10}, {100, 100} };
    POINT proj_change1[] = { {6, 6}, {14, 6}, {16, 6}, {20, 20}, {22, 20}, {200, 200} };
    POINT proj_change2[] = { {6, 6}, {14, 6}, {16, 6}, {20, 20}, {44, 20}, {400, 400} };
    POINT proj_change3[] = { {6, 6}, {14, 6}, {16, 6}, {20, 20}, {22, 20}, {200, 200} };
    POINT proj_change4[] = { {6, 6}, { 7, 6}, { 8, 6}, {10, 10}, {11, 10}, {100, 100} };
    POINT proj_change5[] = { {6, 6}, { 7, 6}, {16, 6}, {20, 20}, {22, 20}, {200, 200} };
    POINT proj_change6[] = { {6, 6}, {28, 6}, {32, 6}, {40, 40}, {44, 40}, {400, 400} };
    POINT proj_change7[] = { {6, 6}, {14, 6}, {32, 6}, {40, 40}, {44, 40}, {400, 400} };
    POINT proj_change8[] = { {6, 6}, {28, 6}, {32, 6}, {40, 40}, {44, 40}, {400, 400} };

    int nchange = sizeof( req_change ) / sizeof( POINT );

    trace("testing SPI_{GET,SET}MOUSE\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETMOUSE, 0, old_mi, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}MOUSE"))
        return;

    run_spi_setmouse_test( curr_val, req_change, proj_change1, nchange );

    /* acceleration change */
    curr_val[2] = 2;
    run_spi_setmouse_test( curr_val, req_change, proj_change2, nchange );

    /* acceleration change */
    curr_val[2] = 3;
    run_spi_setmouse_test( curr_val, req_change, proj_change3, nchange );

    /* acceleration change */
    curr_val[2] = 0;
    run_spi_setmouse_test( curr_val, req_change, proj_change4, nchange );

    /* threshold change */
    curr_val[2] = 1;
    curr_val[0] = 7;
    run_spi_setmouse_test( curr_val, req_change, proj_change5, nchange );

    /* threshold change */
    curr_val[2] = 2;
    curr_val[0] = 6;
    curr_val[1] = 6;
    run_spi_setmouse_test( curr_val, req_change, proj_change6, nchange );

    /* threshold change */
    curr_val[1] = 7;
    run_spi_setmouse_test( curr_val, req_change, proj_change7, nchange );

    /* threshold change */
    curr_val[1] = 5;
    run_spi_setmouse_test( curr_val, req_change, proj_change8, nchange );

    rc=SystemParametersInfoA( SPI_SETMOUSE, 0, old_mi, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

#if 0
static void test_setborder(UINT curr_val)
{
    BOOL rc;
    UINT border;
    INT frame;
    char buf[10];

    rc=SystemParametersInfoA( SPI_SETBORDER, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETBORDER, 1 );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETBORDER_REGKEY, SPI_SETBORDER_VALNAME, buf );

    if (curr_val == 0)
        curr_val = 1;
    rc=SystemParametersInfoA( SPI_GETBORDER, 0, &border, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( border, curr_val, "SPI_{GET,SET}BORDER", "%d");

    frame = curr_val + GetSystemMetrics( SM_CXDLGFRAME );
    eq( frame, GetSystemMetrics( SM_CXFRAME ), "SM_CXFRAME", "%d" );
    eq( frame, GetSystemMetrics( SM_CYFRAME ), "SM_CYFRAME", "%d" );
    eq( frame, GetSystemMetrics( SM_CXSIZEFRAME ), "SM_CXSIZEFRAME", "%d" );
    eq( frame, GetSystemMetrics( SM_CYSIZEFRAME ), "SM_CYSIZEFRAME", "%d" );
}

static void test_SPI_SETBORDER( void )                 /*      6 */
{
    BOOL rc;
    UINT old_border;

    /* These tests hang when XFree86 4.0 for Windows is running (tested on
     *  WinNT, SP2, Cygwin/XFree 4.1.0. Skip the test when XFree86 is
     * running.
     */
    if (FindWindowA( NULL, "Cygwin/XFree86" ))
        return;

    trace("testing SPI_{GET,SET}BORDER\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETBORDER, 0, &old_border, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}BORDER"))
        return;

    test_setborder(1);
    test_setborder(0);
    test_setborder(7);
    test_setborder(20);

    /* This will restore sane values if the test hang previous run. */
    if ( old_border == 7 || old_border == 20 )
        old_border = -15;

    rc=SystemParametersInfoA( SPI_SETBORDER, old_border, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}
#endif

static void test_SPI_SETKEYBOARDSPEED( void )          /*     10 */
{
    BOOL rc;
    UINT old_speed;
    const UINT vals[]={0,20,31};
    unsigned int i;

    trace("testing SPI_{GET,SET}KEYBOARDSPEED\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETKEYBOARDSPEED, 0, &old_speed, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}KEYBOARDSPEED"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETKEYBOARDSPEED, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETKEYBOARDSPEED, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETKEYBOARDSPEED_REGKEY, SPI_SETKEYBOARDSPEED_VALNAME, buf );

        rc=SystemParametersInfoA( SPI_GETKEYBOARDSPEED, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}KEYBOARDSPEED", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETKEYBOARDSPEED, old_speed, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_ICONHORIZONTALSPACING( void )     /*     13 */
{
    BOOL rc;
    INT old_spacing;
    INT spacing;
    INT curr_val;

    trace("testing SPI_ICONHORIZONTALSPACING\n");
    SetLastError(0xdeadbeef);
    /* default value: 75 */
    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &old_spacing, 0 );
    if (!test_error_msg(rc,"SPI_ICONHORIZONTALSPACING"))
        return;

    /* do not increase the value as it would upset the user's icon layout */
    curr_val = (old_spacing > 32 ? old_spacing-1 : 32);
    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_ICONHORIZONTALSPACING, 0 );
    /* The registry keys depend on the Windows version and the values too
     * => don't test them
     */

    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &spacing, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( spacing, curr_val, "ICONHORIZONTALSPACING", "%d");
    eq( GetSystemMetrics( SM_CXICONSPACING ), curr_val, "SM_CXICONSPACING", "%d" );

    curr_val = 10;
    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    curr_val = 32;      /*min value*/
    test_change_message( SPI_ICONHORIZONTALSPACING, 0 );

    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &spacing, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( spacing, curr_val, "ICONHORIZONTALSPACING", "%d" );
    eq( GetSystemMetrics( SM_CXICONSPACING ), curr_val, "SM_CXICONSPACING", "%d" );

    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, old_spacing, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETSCREENSAVETIMEOUT( void )      /*     14 */
{
    BOOL rc;
    UINT old_timeout;
    const UINT vals[]={0,32767};
    unsigned int i;

    trace("testing SPI_{GET,SET}SCREENSAVETIMEOUT\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETSCREENSAVETIMEOUT, 0, &old_timeout, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}SCREENSAVETIMEOUT"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETSCREENSAVETIMEOUT, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETSCREENSAVETIMEOUT, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETSCREENSAVETIMEOUT_REGKEY,
                      SPI_SETSCREENSAVETIMEOUT_VALNAME, buf );

        SystemParametersInfoA( SPI_GETSCREENSAVETIMEOUT, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}SCREENSAVETIMEOUT", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSCREENSAVETIMEOUT, old_timeout, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETSCREENSAVEACTIVE( void )       /*     17 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}SCREENSAVEACTIVE\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETSCREENSAVEACTIVE, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}SCREENSAVEACTIVE"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETSCREENSAVEACTIVE, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETSCREENSAVEACTIVE, 0 );
        test_reg_key( SPI_SETSCREENSAVEACTIVE_REGKEY,
                      SPI_SETSCREENSAVEACTIVE_VALNAME,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETSCREENSAVEACTIVE, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}SCREENSAVEACTIVE", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSCREENSAVEACTIVE, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETGRIDGRANULARITY( void )        /*     19 */
{
    /* ??? */;
}

static void test_SPI_SETKEYBOARDDELAY( void )          /*     23 */
{
    BOOL rc;
    UINT old_delay;
    const UINT vals[]={0,3};
    unsigned int i;

    trace("testing SPI_{GET,SET}KEYBOARDDELAY\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETKEYBOARDDELAY, 0, &old_delay, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}KEYBOARDDELAY"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT delay;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETKEYBOARDDELAY, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETKEYBOARDDELAY, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETKEYBOARDDELAY_REGKEY,
                      SPI_SETKEYBOARDDELAY_VALNAME, buf );

        rc=SystemParametersInfoA( SPI_GETKEYBOARDDELAY, 0, &delay, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( delay, vals[i], "SPI_{GET,SET}KEYBOARDDELAY", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETKEYBOARDDELAY, old_delay, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_ICONVERTICALSPACING( void )       /*     24 */
{
    BOOL rc;
    INT old_spacing;
    INT spacing;
    INT curr_val;

    trace("testing SPI_ICONVERTICALSPACING\n");
    SetLastError(0xdeadbeef);
    /* default value: 75 */
    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &old_spacing, 0 );
    if (!test_error_msg(rc,"SPI_ICONVERTICALSPACING"))
        return;

    /* do not increase the value as it would upset the user's icon layout */
    curr_val = old_spacing-1;
    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_ICONVERTICALSPACING, 0 );
    /* The registry keys depend on the Windows version and the values too
     * => don't test them
     */

    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &spacing, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( spacing, curr_val, "ICONVERTICALSPACING", "%d" );
    eq( GetSystemMetrics( SM_CYICONSPACING ), curr_val, "SM_CYICONSPACING", "%d" );

    curr_val = 10;
    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    curr_val = 32;      /*min value*/
    test_change_message( SPI_ICONVERTICALSPACING, 0 );

    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &spacing, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( spacing, curr_val, "ICONVERTICALSPACING", "%d" );
    eq( GetSystemMetrics( SM_CYICONSPACING ), curr_val, "SM_CYICONSPACING", "%d" );

    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, old_spacing, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETICONTITLEWRAP( void )          /*     26 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    /* These tests hang when XFree86 4.0 for Windows is running (tested on
     * WinNT, SP2, Cygwin/XFree 4.1.0. Skip the test when XFree86 is
     * running.
     */
    if (FindWindowA( NULL, "Cygwin/XFree86" ))
        return;

    trace("testing SPI_{GET,SET}ICONTITLEWRAP\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}ICONTITLEWRAP"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETICONTITLEWRAP, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETICONTITLEWRAP, 1 );
        test_reg_key_ex( SPI_SETICONTITLEWRAP_REGKEY1,
                         SPI_SETICONTITLEWRAP_REGKEY2,
                         SPI_SETICONTITLEWRAP_VALNAME,
                         vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}ICONTITLEWRAP", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETICONTITLEWRAP, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETMENUDROPALIGNMENT( void )      /*     28 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}MENUDROPALIGNMENT\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETMENUDROPALIGNMENT, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}MENUDROPALIGNMENT"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETMENUDROPALIGNMENT, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETMENUDROPALIGNMENT, 0 );
        test_reg_key_ex( SPI_SETMENUDROPALIGNMENT_REGKEY1,
                         SPI_SETMENUDROPALIGNMENT_REGKEY2,
                         SPI_SETMENUDROPALIGNMENT_VALNAME,
                         vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETMENUDROPALIGNMENT, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MENUDROPALIGNMENT", "%d" );
        eq( GetSystemMetrics( SM_MENUDROPALIGNMENT ), (int)vals[i],
            "SM_MENUDROPALIGNMENT", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMENUDROPALIGNMENT, old_b, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETDOUBLECLKWIDTH( void )         /*     29 */
{
    BOOL rc;
    INT old_width;
    const UINT vals[]={0,10000};
    unsigned int i;

    trace("testing SPI_{GET,SET}DOUBLECLKWIDTH\n");
    old_width = GetSystemMetrics( SM_CXDOUBLECLK );

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        char buf[10];

        SetLastError(0xdeadbeef);
        rc=SystemParametersInfoA( SPI_SETDOUBLECLKWIDTH, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_{GET,SET}DOUBLECLKWIDTH"))
            return;

        test_change_message( SPI_SETDOUBLECLKWIDTH, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key_ex( SPI_SETDOUBLECLKWIDTH_REGKEY1,
                         SPI_SETDOUBLECLKWIDTH_REGKEY2,
                         SPI_SETDOUBLECLKWIDTH_VALNAME, buf );
        eq( GetSystemMetrics( SM_CXDOUBLECLK ), (int)vals[i],
            "SM_CXDOUBLECLK", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETDOUBLECLKWIDTH, old_width, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETDOUBLECLKHEIGHT( void )        /*     30 */
{
    BOOL rc;
    INT old_height;
    const UINT vals[]={0,10000};
    unsigned int i;

    trace("testing SPI_{GET,SET}DOUBLECLKHEIGHT\n");
    old_height = GetSystemMetrics( SM_CYDOUBLECLK );

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        char buf[10];

        SetLastError(0xdeadbeef);
        rc=SystemParametersInfoA( SPI_SETDOUBLECLKHEIGHT, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_{GET,SET}DOUBLECLKHEIGHT"))
            return;

        test_change_message( SPI_SETDOUBLECLKHEIGHT, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key_ex( SPI_SETDOUBLECLKHEIGHT_REGKEY1,
                         SPI_SETDOUBLECLKHEIGHT_REGKEY2,
                         SPI_SETDOUBLECLKHEIGHT_VALNAME, buf );
        eq( GetSystemMetrics( SM_CYDOUBLECLK ), (int)vals[i],
            "SM_CYDOUBLECLK", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETDOUBLECLKHEIGHT, old_height, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETDOUBLECLICKTIME( void )        /*     32 */
{
    BOOL rc;
    UINT curr_val;
    UINT saved_val;
    UINT old_time;
    char buf[10];

    trace("testing SPI_{GET,SET}DOUBLECLICKTIME\n");
    old_time = GetDoubleClickTime();

    curr_val = 0;
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_SETDOUBLECLICKTIME, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    if (!test_error_msg(rc,"SPI_{GET,SET}DOUBLECLICKTIME"))
        return;

    test_change_message( SPI_SETDOUBLECLICKTIME, 0 );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETDOUBLECLICKTIME_REGKEY,
                  SPI_SETDOUBLECLICKTIME_VALNAME, buf );
    curr_val = 500; /* used value for 0 */
    eq( GetDoubleClickTime(), curr_val, "GetDoubleClickTime", "%d" );

    curr_val = 1000;
    rc=SystemParametersInfoA( SPI_SETDOUBLECLICKTIME, curr_val, 0,
                             SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETDOUBLECLICKTIME, 0 );
    sprintf( buf, "%d", curr_val );
    test_reg_key( SPI_SETDOUBLECLICKTIME_REGKEY,
                  SPI_SETDOUBLECLICKTIME_VALNAME, buf );
    eq( GetDoubleClickTime(), curr_val, "GetDoubleClickTime", "%d" );

    saved_val = curr_val;

    curr_val = 0;
    SetDoubleClickTime( curr_val );
    sprintf( buf, "%d", saved_val );
    test_reg_key( SPI_SETDOUBLECLICKTIME_REGKEY,
                  SPI_SETDOUBLECLICKTIME_VALNAME, buf );
    curr_val = 500; /* used value for 0 */
    eq( GetDoubleClickTime(), curr_val, "GetDoubleClickTime", "%d" );

    curr_val = 1000;
    SetDoubleClickTime( curr_val );
    sprintf( buf, "%d", saved_val );
    test_reg_key( SPI_SETDOUBLECLICKTIME_REGKEY,
                  SPI_SETDOUBLECLICKTIME_VALNAME, buf );
    eq( GetDoubleClickTime(), curr_val, "GetDoubleClickTime", "%d" );

    rc=SystemParametersInfoA(SPI_SETDOUBLECLICKTIME, old_time, 0, SPIF_UPDATEINIFILE);
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETMOUSEBUTTONSWAP( void )        /*     33 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}MOUSEBUTTONSWAP\n");
    old_b = GetSystemMetrics( SM_SWAPBUTTON );

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        SetLastError(0xdeadbeef);
        rc=SystemParametersInfoA( SPI_SETMOUSEBUTTONSWAP, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_{GET,SET}MOUSEBUTTONSWAP"))
            return;
            
        test_change_message( SPI_SETMOUSEBUTTONSWAP, 0 );
        test_reg_key( SPI_SETMOUSEBUTTONSWAP_REGKEY,
                      SPI_SETMOUSEBUTTONSWAP_VALNAME,
                      vals[i] ? "1" : "0" );
        eq( GetSystemMetrics( SM_SWAPBUTTON ), (int)vals[i],
            "SM_SWAPBUTTON", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMOUSEBUTTONSWAP, old_b, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETFASTTASKSWITCH( void )         /*     36 */
{
    BOOL rc;
    BOOL v;

    trace("testing SPI_GETFASTTASKSWITCH\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETFASTTASKSWITCH, 0, &v, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}FASTTASKSWITCH"))
        return;

    /* there is not a single Windows platform on which SPI_GETFASTTASKSWITCH
     * works. That sure simplifies testing!
     */
}

static void test_SPI_SETDRAGFULLWINDOWS( void )        /*     37 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}DRAGFULLWINDOWS\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETDRAGFULLWINDOWS, 0, &old_b, 0 );

    /* SPI_{GET,SET}DRAGFULLWINDOWS is not implemented on Win95 */
    if (!test_error_msg(rc,"SPI_{GET,SET}DRAGFULLWINDOWS"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETDRAGFULLWINDOWS, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETDRAGFULLWINDOWS, 0 );
        test_reg_key( SPI_SETDRAGFULLWINDOWS_REGKEY,
                      SPI_SETDRAGFULLWINDOWS_VALNAME,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETDRAGFULLWINDOWS, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}DRAGFULLWINDOWS", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETDRAGFULLWINDOWS, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETMINIMIZEDMETRICS( void )               /*     44 */
{
    BOOL rc;
    MINIMIZEDMETRICS lpMm_orig;
    MINIMIZEDMETRICS lpMm_new;
    MINIMIZEDMETRICS lpMm_cur;

    lpMm_orig.cbSize = sizeof(MINIMIZEDMETRICS);
    lpMm_new.cbSize = sizeof(MINIMIZEDMETRICS);
    lpMm_cur.cbSize = sizeof(MINIMIZEDMETRICS);

    trace("testing SPI_{GET,SET}MINIMIZEDMETRICS\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_orig, FALSE );
    if (!test_error_msg(rc,"SPI_{GET,SET}MINIMIZEDMETRICS"))
        return;

    lpMm_cur.iWidth = 180;
    lpMm_cur.iHorzGap = 1;
    lpMm_cur.iVertGap = 1;
    lpMm_cur.iArrange = 5;
    
    rc=SystemParametersInfoA( SPI_SETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_cur, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());

    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_new, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());

    eq( lpMm_new.iWidth,   lpMm_cur.iWidth,   "iWidth",   "%d" );
    eq( lpMm_new.iHorzGap, lpMm_cur.iHorzGap, "iHorzGap", "%d" );
    eq( lpMm_new.iVertGap, lpMm_cur.iVertGap, "iVertGap", "%d" );
    eq( lpMm_new.iArrange, lpMm_cur.iArrange, "iArrange", "%d" );

    eq( GetSystemMetrics( SM_CXMINIMIZED ) - 6,
        lpMm_new.iWidth,   "iWidth",   "%d" );
    eq( GetSystemMetrics( SM_CXMINSPACING ) - GetSystemMetrics( SM_CXMINIMIZED ),
        lpMm_new.iHorzGap, "iHorzGap", "%d" );
    eq( GetSystemMetrics( SM_CYMINSPACING ) - GetSystemMetrics( SM_CYMINIMIZED ),
        lpMm_new.iVertGap, "iVertGap", "%d" );
    eq( GetSystemMetrics( SM_ARRANGE ),
        lpMm_new.iArrange, "iArrange", "%d" );

    rc=SystemParametersInfoA( SPI_SETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_orig, FALSE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
    
    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_new, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    
    eq( lpMm_new.iWidth,   lpMm_orig.iWidth,   "iWidth",   "%d" );
    eq( lpMm_new.iHorzGap, lpMm_orig.iHorzGap, "iHorzGap", "%d" );
    eq( lpMm_new.iVertGap, lpMm_orig.iVertGap, "iVertGap", "%d" );
    eq( lpMm_new.iArrange, lpMm_orig.iArrange, "iArrange", "%d" );
}

static void test_SPI_SETICONMETRICS( void )               /*     46 */
{
    BOOL rc;
    ICONMETRICSA im_orig;
    ICONMETRICSA im_new;
    ICONMETRICSA im_cur;
        
    im_orig.cbSize = sizeof(ICONMETRICSA);
    im_new.cbSize = sizeof(ICONMETRICSA);
    im_cur.cbSize = sizeof(ICONMETRICSA);

    trace("testing SPI_{GET,SET}ICONMETRICS\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im_orig, FALSE );
    if (!test_error_msg(rc,"SPI_{GET,SET}ICONMETRICS"))
        return;

    im_cur.iHorzSpacing = 65;
    im_cur.iVertSpacing = 65;
    im_cur.iTitleWrap = 0;
    im_cur.lfFont.lfHeight = 1;
    im_cur.lfFont.lfWidth = 1;
    im_cur.lfFont.lfEscapement = 1;
    im_cur.lfFont.lfWeight = 1;
    im_cur.lfFont.lfItalic = 1;
    im_cur.lfFont.lfStrikeOut = 1;
    im_cur.lfFont.lfUnderline = 1;
    im_cur.lfFont.lfCharSet = 1;
    im_cur.lfFont.lfOutPrecision = 1;
    im_cur.lfFont.lfClipPrecision = 1;
    im_cur.lfFont.lfPitchAndFamily = 1;
    im_cur.lfFont.lfQuality = 1;

    rc=SystemParametersInfoA( SPI_SETICONMETRICS, sizeof(ICONMETRICSA), &im_cur, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());

    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im_new, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());

    eq( im_new.iHorzSpacing, im_cur.iHorzSpacing, "iHorzSpacing", "%d" );
    eq( im_new.iVertSpacing, im_cur.iVertSpacing, "iVertSpacing", "%d" );
    eq( im_new.iTitleWrap,   im_cur.iTitleWrap,   "iTitleWrap",   "%d" );

    eq( im_new.lfFont.lfHeight,         im_cur.lfFont.lfHeight,         "lfHeight",         "%ld" );
    eq( im_new.lfFont.lfWidth,          im_cur.lfFont.lfWidth,          "lfWidth",          "%ld" );
    eq( im_new.lfFont.lfEscapement,     im_cur.lfFont.lfEscapement,     "lfEscapement",     "%ld" );
    eq( im_new.lfFont.lfWeight,         im_cur.lfFont.lfWeight,         "lfWeight",         "%ld" );
    eq( im_new.lfFont.lfItalic,         im_cur.lfFont.lfItalic,         "lfItalic",         "%d" );
    eq( im_new.lfFont.lfStrikeOut,      im_cur.lfFont.lfStrikeOut,      "lfStrikeOut",      "%d" );
    eq( im_new.lfFont.lfUnderline,      im_cur.lfFont.lfUnderline,      "lfUnderline",      "%d" );
    eq( im_new.lfFont.lfCharSet,        im_cur.lfFont.lfCharSet,        "lfCharSet",        "%d" );
    eq( im_new.lfFont.lfOutPrecision,   im_cur.lfFont.lfOutPrecision,   "lfOutPrecision",   "%d" );
    eq( im_new.lfFont.lfClipPrecision,  im_cur.lfFont.lfClipPrecision,  "lfClipPrecision",  "%d" );
    eq( im_new.lfFont.lfPitchAndFamily, im_cur.lfFont.lfPitchAndFamily, "lfPitchAndFamily", "%d" );
    eq( im_new.lfFont.lfQuality,        im_cur.lfFont.lfQuality,        "lfQuality",        "%d" );

    eq( GetSystemMetrics( SM_CXICONSPACING ),
        im_new.iHorzSpacing, "iHorzSpacing", "%d" );
    eq( GetSystemMetrics( SM_CYICONSPACING ),
        im_new.iVertSpacing, "iVertSpacing", "%d" );

    rc=SystemParametersInfoA( SPI_SETICONMETRICS, sizeof(ICONMETRICSA), &im_orig, FALSE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
    
    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im_new, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    
    eq( im_new.iHorzSpacing, im_orig.iHorzSpacing, "iHorzSpacing", "%d" );
    eq( im_new.iVertSpacing, im_orig.iVertSpacing, "iVertSpacing", "%d" );
    eq( im_new.iTitleWrap,   im_orig.iTitleWrap,   "iTitleWrap",   "%d" );
}

static void test_SPI_SETWORKAREA( void )               /*     47 */
{
    BOOL rc;
    RECT old_area;
    RECT area;
    RECT curr_val;

    trace("testing SPI_{GET,SET}WORKAREA\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA(SPI_GETWORKAREA, 0, &old_area, 0);
    if (!test_error_msg(rc,"SPI_{GET,SET}WORKAREA"))
        return;

    /* Modify the work area only minimally as this causes the icons that
     * fall outside it to be moved around thus requiring the user to
     * reposition them manually one by one.
     * Changing the work area by just one pixel should make this occurrence
     * reasonably unlikely.
     */
    curr_val.left = old_area.left;
    curr_val.top = old_area.top;
    curr_val.right = old_area.right-1;
    curr_val.bottom = old_area.bottom-1;
    rc=SystemParametersInfoA( SPI_SETWORKAREA, 0, &curr_val,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETWORKAREA, 0 );
    rc=SystemParametersInfoA( SPI_GETWORKAREA, 0, &area, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( area.left,   curr_val.left,   "left",   "%ld" );
    eq( area.top,    curr_val.top,    "top",    "%ld" );
    eq( area.right,  curr_val.right,  "right",  "%ld" );
    eq( area.bottom, curr_val.bottom, "bottom", "%ld" );

    rc=SystemParametersInfoA( SPI_SETWORKAREA, 0, &old_area,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETWORKAREA, 0 );
    rc=SystemParametersInfoA( SPI_GETWORKAREA, 0, &area, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( area.left,   old_area.left,   "left",   "%ld" );
    eq( area.top,    old_area.top,    "top",    "%ld" );
    eq( area.right,  old_area.right,  "right",  "%ld" );
    eq( area.bottom, old_area.bottom, "bottom", "%ld" );
}

static void test_SPI_SETSHOWSOUNDS( void )             /*     57 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}SHOWSOUNDS\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETSHOWSOUNDS, 0, &old_b, 0 );
    /* SPI_{GET,SET}SHOWSOUNDS is completely broken on Win9x */
    if (!test_error_msg(rc,"SPI_{GET,SET}SHOWSOUNDS"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETSHOWSOUNDS, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETSHOWSOUNDS, 0 );
        test_reg_key( SPI_SETSHOWSOUNDS_REGKEY,
                      SPI_SETSHOWSOUNDS_VALNAME,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETSHOWSOUNDS, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_GETSHOWSOUNDS", "%d" );
        eq( GetSystemMetrics( SM_SHOWSOUNDS ), (int)vals[i],
            "SM_SHOWSOUNDS", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSHOWSOUNDS, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETKEYBOARDPREF( void )           /*     69 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}KEYBOARDPREF\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETKEYBOARDPREF, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}KEYBOARDPREF"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        BOOL v;

        rc=SystemParametersInfoA( SPI_SETKEYBOARDPREF, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETKEYBOARDPREF, 0 );
        test_reg_key_ex2( SPI_SETKEYBOARDPREF_REGKEY, SPI_SETKEYBOARDPREF_REGKEY_LEGACY,
                          SPI_SETKEYBOARDPREF_VALNAME, SPI_SETKEYBOARDPREF_VALNAME_LEGACY,
                          vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETKEYBOARDPREF, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_GETKEYBOARDPREF", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETKEYBOARDPREF, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETSCREENREADER( void )           /*     71 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}SCREENREADER\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETSCREENREADER, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}SCREENREADER"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        BOOL v;

        rc=SystemParametersInfoA( SPI_SETSCREENREADER, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETSCREENREADER, 0 );
        test_reg_key_ex2( SPI_SETSCREENREADER_REGKEY, SPI_SETSCREENREADER_REGKEY_LEGACY,
                      SPI_SETSCREENREADER_VALNAME, SPI_SETSCREENREADER_VALNAME_LEGACY,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETSCREENREADER, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_GETSCREENREADER", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSCREENREADER, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETLOWPOWERACTIVE( void )         /*     85 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}LOWPOWERACTIVE\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETLOWPOWERACTIVE, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}LOWPOWERACTIVE"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETLOWPOWERACTIVE, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETLOWPOWERACTIVE, 0 );
        test_reg_key( SPI_SETLOWPOWERACTIVE_REGKEY,
                      SPI_SETLOWPOWERACTIVE_VALNAME,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETLOWPOWERACTIVE, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_GETLOWPOWERACTIVE", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETLOWPOWERACTIVE, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETPOWEROFFACTIVE( void )         /*     86 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}POWEROFFACTIVE\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETPOWEROFFACTIVE, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}POWEROFFACTIVE"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETPOWEROFFACTIVE, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETPOWEROFFACTIVE, 0 );
        test_reg_key( SPI_SETPOWEROFFACTIVE_REGKEY,
                      SPI_SETPOWEROFFACTIVE_VALNAME,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETPOWEROFFACTIVE, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_GETPOWEROFFACTIVE", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETPOWEROFFACTIVE, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETMOUSEHOVERWIDTH( void )      /*     99 */
{
    BOOL rc;
    UINT old_width;
    const UINT vals[]={0,32767};
    unsigned int i;

    trace("testing SPI_{GET,SET}MOUSEHOVERWIDTH\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETMOUSEHOVERWIDTH, 0, &old_width, 0 );
    /* SPI_{GET,SET}MOUSEHOVERWIDTH does not seem to be supported on Win9x despite
    * what MSDN states (Verified on Win98SE)
    */
    if (!test_error_msg(rc,"SPI_{GET,SET}MOUSEHOVERWIDTH"))
        return;
    
    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETMOUSEHOVERWIDTH, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETMOUSEHOVERWIDTH, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSEHOVERWIDTH_REGKEY,
                      SPI_SETMOUSEHOVERWIDTH_VALNAME, buf );

        SystemParametersInfoA( SPI_GETMOUSEHOVERWIDTH, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MOUSEHOVERWIDTH", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMOUSEHOVERWIDTH, old_width, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETMOUSEHOVERHEIGHT( void )      /*     101 */
{
    BOOL rc;
    UINT old_height;
    const UINT vals[]={0,32767};
    unsigned int i;

    trace("testing SPI_{GET,SET}MOUSEHOVERHEIGHT\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETMOUSEHOVERHEIGHT, 0, &old_height, 0 );
    /* SPI_{GET,SET}MOUSEHOVERWIDTH does not seem to be supported on Win9x despite
     * what MSDN states (Verified on Win98SE)
     */
    if (!test_error_msg(rc,"SPI_{GET,SET}MOUSEHOVERHEIGHT"))
        return;
    
    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETMOUSEHOVERHEIGHT, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETMOUSEHOVERHEIGHT, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSEHOVERHEIGHT_REGKEY,
                      SPI_SETMOUSEHOVERHEIGHT_VALNAME, buf );

        SystemParametersInfoA( SPI_GETMOUSEHOVERHEIGHT, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MOUSEHOVERHEIGHT", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMOUSEHOVERHEIGHT, old_height, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETMOUSEHOVERTIME( void )      /*     103 */
{
    BOOL rc;
    UINT old_time;

    /* Windows XP accepts 10 as the minimum hover time. Any value below will be
     * defaulted to a value of 10 automatically.
     */
    const UINT vals[]={10,32767};
    unsigned int i;

    trace("testing SPI_{GET,SET}MOUSEHOVERTIME\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETMOUSEHOVERTIME, 0, &old_time, 0 );
    /* SPI_{GET,SET}MOUSEHOVERWIDTH does not seem to be supported on Win9x despite
     * what MSDN states (Verified on Win98SE)
     */    
    if (!test_error_msg(rc,"SPI_{GET,SET}MOUSEHOVERTIME"))
        return;
    
    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETMOUSEHOVERTIME, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETMOUSEHOVERTIME, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSEHOVERTIME_REGKEY,
                      SPI_SETMOUSEHOVERTIME_VALNAME, buf );

        SystemParametersInfoA( SPI_GETMOUSEHOVERTIME, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MOUSEHOVERTIME", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMOUSEHOVERTIME, old_time, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETWHEELSCROLLLINES( void )      /*     105 */
{
    BOOL rc;
    UINT old_lines;
    const UINT vals[]={0,32767};
    unsigned int i;

    trace("testing SPI_{GET,SET}WHEELSCROLLLINES\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETWHEELSCROLLLINES, 0, &old_lines, 0 );

    /* SPI_{GET,SET}WHEELSCROLLLINES not supported on Windows 95 */
    if (!test_error_msg(rc,"SPI_{GET,SET}WHEELSCROLLLINES"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETWHEELSCROLLLINES, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETWHEELSCROLLLINES, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSESCROLLLINES_REGKEY,
                      SPI_SETMOUSESCROLLLINES_VALNAME, buf );

        SystemParametersInfoA( SPI_GETWHEELSCROLLLINES, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}WHEELSCROLLLINES", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETWHEELSCROLLLINES, old_lines, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETMENUSHOWDELAY( void )      /*     107 */
{
    BOOL rc;
    UINT old_delay;
    const UINT vals[]={0,32767};
    unsigned int i;

    trace("testing SPI_{GET,SET}MENUSHOWDELAY\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETMENUSHOWDELAY, 0, &old_delay, 0 );

    /* SPI_{GET,SET}MENUSHOWDELAY not supported on Windows 95 */
    if (!test_error_msg(rc,"SPI_{GET,SET}MENUSHOWDELAY"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETMENUSHOWDELAY, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETMENUSHOWDELAY, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMENUSHOWDELAY_REGKEY,
                      SPI_SETMENUSHOWDELAY_VALNAME, buf );

        SystemParametersInfoA( SPI_GETMENUSHOWDELAY, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MENUSHOWDELAY", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMENUSHOWDELAY, old_delay, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETWALLPAPER( void )              /*   115 */
{
    BOOL rc;
    char oldval[260];
    char newval[260];

    trace("testing SPI_{GET,SET}DESKWALLPAPER\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA(SPI_GETDESKWALLPAPER, 260, oldval, 0);
    /* SPI_{GET,SET}DESKWALLPAPER is completely broken on Win9x and
     * unimplemented on NT4
     */
    if (!test_error_msg(rc,"SPI_{GET,SET}DESKWALLPAPER"))
        return;

    strcpy(newval, "");
    rc=SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, newval, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message(SPI_SETDESKWALLPAPER, 0);

    rc=SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, oldval, SPIF_UPDATEINIFILE);
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());

    test_reg_key(SPI_SETDESKWALLPAPER_REGKEY, SPI_SETDESKWALLPAPER_VALNAME, oldval);
}

/*
 * Registry entries for the system parameters.
 * Names are created by 'SET' flags names.
 * We assume that corresponding 'GET' entries use the same registry keys.
 */
static DWORD WINAPI SysParamsThreadFunc( LPVOID lpParam )
{
    test_SPI_SETBEEP();                         /*      1 */
    test_SPI_SETMOUSE();                        /*      4 */
    /* Messes up systems settings on Win9x */
    /*test_SPI_SETBORDER();*/                   /*      6 */
    test_SPI_SETKEYBOARDSPEED();                /*     10 */
    test_SPI_ICONHORIZONTALSPACING();           /*     13 */
    test_SPI_SETSCREENSAVETIMEOUT();            /*     14 */
    test_SPI_SETSCREENSAVEACTIVE();             /*     17 */
    test_SPI_SETGRIDGRANULARITY();              /*     19 */
    test_SPI_SETKEYBOARDDELAY();                /*     23 */
    test_SPI_ICONVERTICALSPACING();             /*     24 */
    test_SPI_SETICONTITLEWRAP();                /*     26 */
    test_SPI_SETMENUDROPALIGNMENT();            /*     28 */
    test_SPI_SETDOUBLECLKWIDTH();               /*     29 */
    test_SPI_SETDOUBLECLKHEIGHT();              /*     30 */
    test_SPI_SETDOUBLECLICKTIME();              /*     32 */
    test_SPI_SETMOUSEBUTTONSWAP();              /*     33 */
    test_SPI_SETFASTTASKSWITCH();               /*     36 */
    test_SPI_SETDRAGFULLWINDOWS();              /*     37 */
    test_SPI_SETMINIMIZEDMETRICS();             /*     44 */
    test_SPI_SETICONMETRICS();                  /*     46 */
    test_SPI_SETWORKAREA();                     /*     47 */
    test_SPI_SETSHOWSOUNDS();                   /*     57 */
    test_SPI_SETKEYBOARDPREF();                 /*     69 */
    test_SPI_SETSCREENREADER();                 /*     71 */
    test_SPI_SETLOWPOWERACTIVE();               /*     85 */
    test_SPI_SETPOWEROFFACTIVE();               /*     86 */
    test_SPI_SETMOUSEHOVERWIDTH();              /*     99 */
    test_SPI_SETMOUSEHOVERHEIGHT();             /*    101 */
    test_SPI_SETMOUSEHOVERTIME();               /*    103 */
    test_SPI_SETWHEELSCROLLLINES();             /*    105 */
    test_SPI_SETMENUSHOWDELAY();                /*    107 */
    test_SPI_SETWALLPAPER();                    /*    115 */
    SendMessageA( ghTestWnd, WM_DESTROY, 0, 0 );
    return 0;
}

START_TEST(sysparams)
{
    int argc;
    char** argv;
    WNDCLASSA wc;
    MSG msg;
    HANDLE hThread;
    DWORD dwThreadId;
    HANDLE hInstance = GetModuleHandleA( NULL );

    /* This test requires interactivity, if we don't have it, give up */
    if (!SystemParametersInfoA( SPI_SETBEEP, TRUE, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE ) &&
        GetLastError()==ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION) return;

    argc = winetest_get_mainargs(&argv);
    strict=(argc >= 3 && strcmp(argv[2],"strict")==0);
    trace("strict=%d\n",strict);

    change_counter = 0;
    change_last_param = 0;

    wc.lpszClassName = "SysParamsTestClass";
    wc.lpfnWndProc = SysParamsTestWndProc;
    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA( 0, (LPSTR)IDI_APPLICATION );
    wc.hCursor = LoadCursorA( 0, (LPSTR)IDC_ARROW );
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1 );
    wc.lpszMenuName = 0;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    RegisterClassA( &wc );

    ghTestWnd = CreateWindowA( "SysParamsTestClass", "Test System Parameters Application",
                               WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, 0, 0, hInstance, NULL );

    hThread = CreateThread( NULL, 0, SysParamsThreadFunc, 0, 0, &dwThreadId );
    assert( hThread );
    CloseHandle( hThread );

    while( GetMessageA( &msg, 0, 0, 0 )) {
        TranslateMessage( &msg );
        DispatchMessageA( &msg );
    }
}
