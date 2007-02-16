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

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500 /* For SPI_GETMOUSEHOVERWIDTH and more */

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"
#include "winnls.h"

#ifndef SPI_GETDESKWALLPAPER
# define SPI_GETDESKWALLPAPER 0x0073
#endif

static int strict;
static int dpi;
static int iswin9x;
static HDC hdc;

#define eq(received, expected, label, type) \
        ok((received) == (expected), "%s: got " type " instead of " type "\n", (label),(received),(expected))


#define SPI_SETBEEP_REGKEY                      "Control Panel\\Sound"
#define SPI_SETBEEP_VALNAME                     "Beep"
#define SPI_SETMOUSE_REGKEY                     "Control Panel\\Mouse"
#define SPI_SETMOUSE_VALNAME1                   "MouseThreshold1"
#define SPI_SETMOUSE_VALNAME2                   "MouseThreshold2"
#define SPI_SETMOUSE_VALNAME3                   "MouseSpeed"
#define SPI_SETBORDER_REGKEY                    "Control Panel\\Desktop\\WindowMetrics"
#define SPI_SETBORDER_REGKEY2                   "Control Panel\\Desktop"
#define SPI_SETBORDER_VALNAME                   "BorderWidth"
#define SPI_METRIC_REGKEY                       "Control Panel\\Desktop\\WindowMetrics"
#define SPI_SCROLLWIDTH_VALNAME                 "ScrollWidth"
#define SPI_SCROLLHEIGHT_VALNAME                "ScrollHeight"
#define SPI_CAPTIONWIDTH_VALNAME                "CaptionWidth"
#define SPI_CAPTIONHEIGHT_VALNAME               "CaptionHeight"
#define SPI_CAPTIONFONT_VALNAME                 "CaptionFont"
#define SPI_SMCAPTIONWIDTH_VALNAME              "SmCaptionWidth"
#define SPI_SMCAPTIONHEIGHT_VALNAME             "SmCaptionHeight"
#define SPI_SMCAPTIONFONT_VALNAME               "SmCaptionFont"
#define SPI_MENUWIDTH_VALNAME                   "MenuWidth"
#define SPI_MENUHEIGHT_VALNAME                  "MenuHeight"
#define SPI_MENUFONT_VALNAME                    "MenuFont"
#define SPI_STATUSFONT_VALNAME                  "StatusFont"
#define SPI_MESSAGEFONT_VALNAME                 "MessageFont"

#define SPI_SETKEYBOARDSPEED_REGKEY             "Control Panel\\Keyboard"
#define SPI_SETKEYBOARDSPEED_VALNAME            "KeyboardSpeed"
#define SPI_ICONHORIZONTALSPACING_REGKEY        "Control Panel\\Desktop\\WindowMetrics"
#define SPI_ICONHORIZONTALSPACING_REGKEY2       "Control Panel\\Desktop"
#define SPI_ICONHORIZONTALSPACING_VALNAME       "IconSpacing"
#define SPI_ICONVERTICALSPACING_REGKEY          "Control Panel\\Desktop\\WindowMetrics"
#define SPI_ICONVERTICALSPACING_REGKEY2         "Control Panel\\Desktop"
#define SPI_ICONVERTICALSPACING_VALNAME         "IconVerticalSpacing"
#define SPI_MINIMIZEDMETRICS_REGKEY             "Control Panel\\Desktop\\WindowMetrics"
#define SPI_MINWIDTH_VALNAME                    "MinWidth"
#define SPI_MINHORZGAP_VALNAME                  "MinHorzGap"
#define SPI_MINVERTGAP_VALNAME                  "MinVertGap"
#define SPI_MINARRANGE_VALNAME                  "MinArrange"
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
#define SPI_SETFONTSMOOTHING_REGKEY             "Control Panel\\Desktop"
#define SPI_SETFONTSMOOTHING_VALNAME            "FontSmoothing"
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
            /* ignore these messages caused by resizing of toolbars */
            if( wParam == SPI_SETWORKAREA) break;
            if( change_last_param == SPI_SETWORKAREA) {
                change_last_param = wParam;
                break;
            }
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

/* get a metric from the registry. If the value is negative
 * it is assumed to be in twips and converted to pixels */
static UINT metricfromreg( char *keyname, char *valname, int dpi)
{
    HKEY hkey;
    char buf[64];
    DWORD ret;
    DWORD size, type;
    int value;

    RegOpenKeyA( HKEY_CURRENT_USER, keyname, &hkey );
    size = sizeof(buf);
    ret=RegQueryValueExA( hkey, valname, NULL, &type, (LPBYTE)buf, &size );
    RegCloseKey( hkey );
    if( ret != ERROR_SUCCESS) return -1;
    value = atoi( buf);
    if( value < 0)
        value = ( -value * dpi + 720) / 1440;
    return value;
}

typedef struct
{
    INT16  lfHeight;
    INT16  lfWidth;
    INT16  lfEscapement;
    INT16  lfOrientation;
    INT16  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    CHAR   lfFaceName[LF_FACESIZE];
} LOGFONT16, *LPLOGFONT16;

/* get logfont from the registry */
static int lffromreg( char *keyname,  char *valname, LOGFONTA *plf)
{
    HKEY hkey;
    LOGFONTW lfw;
    DWORD ret, size, type;

    RegOpenKeyA( HKEY_CURRENT_USER, keyname, &hkey ); 
    size = sizeof( lfw);
    ret=RegQueryValueExA( hkey, valname, NULL, &type, (LPBYTE)&lfw, &size );
    RegCloseKey( hkey );
    ok( ret == ERROR_SUCCESS, "Key \"%s\" value \"%s\" not found\n", keyname, valname);
    if( ret != ERROR_SUCCESS) 
        return FALSE;
    if( size <= sizeof( LOGFONT16)) {
        LOGFONT16 *plf16 = (LOGFONT16*) &lfw;
        plf->lfHeight = plf16->lfHeight;
        plf->lfWidth = plf16->lfWidth;
        plf->lfEscapement = plf16->lfEscapement;
        plf->lfOrientation = plf16->lfOrientation;
        plf->lfWeight = plf16->lfWeight;
        plf->lfItalic = plf16->lfItalic;
        plf->lfUnderline = plf16->lfUnderline;
        plf->lfStrikeOut = plf16->lfStrikeOut;
        plf->lfCharSet = plf16->lfCharSet;
        plf->lfOutPrecision = plf16->lfOutPrecision;
        plf->lfClipPrecision = plf16->lfClipPrecision;
        plf->lfQuality = plf16->lfQuality;
        plf->lfPitchAndFamily = plf16->lfPitchAndFamily;
        memcpy( plf->lfFaceName, plf16->lfFaceName, LF_FACESIZE );
    } else if( size <= sizeof( LOGFONTA)) {
        plf = (LOGFONTA*) &lfw;
    } else {
        plf->lfHeight = lfw.lfHeight;
        plf->lfWidth = lfw.lfWidth;
        plf->lfEscapement = lfw.lfEscapement;
        plf->lfOrientation = lfw.lfOrientation;
        plf->lfWeight = lfw.lfWeight;
        plf->lfItalic = lfw.lfItalic;
        plf->lfUnderline = lfw.lfUnderline;
        plf->lfStrikeOut = lfw.lfStrikeOut;
        plf->lfCharSet = lfw.lfCharSet;
        plf->lfOutPrecision = lfw.lfOutPrecision;
        plf->lfClipPrecision = lfw.lfClipPrecision;
        plf->lfQuality = lfw.lfQuality;
        plf->lfPitchAndFamily = lfw.lfPitchAndFamily;
        WideCharToMultiByte( CP_ACP, 0, lfw.lfFaceName, -1, plf->lfFaceName,
            LF_FACESIZE, NULL, NULL);

    }
    return TRUE;
}

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

static void test_setborder(UINT curr_val, int usesetborder, int dpi)
{
    BOOL rc;
    UINT border, regval;
    INT frame;
    NONCLIENTMETRICSA ncm;

    ncm.cbSize = sizeof( ncm);
    rc=SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    if( usesetborder) {
            rc=SystemParametersInfoA( SPI_SETBORDER, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
        test_change_message( SPI_SETBORDER, 1 );
    } else { /* set non client metrics */
        ncm.iBorderWidth = curr_val;
        rc=SystemParametersInfoA( SPI_SETNONCLIENTMETRICS, 0, &ncm, SPIF_UPDATEINIFILE|
                SPIF_SENDCHANGE);
        ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
        test_change_message( SPI_SETNONCLIENTMETRICS, 1 );
    }
    if( curr_val) { /* skip if 0, some windows versions return 0 others 1 */
        regval = metricfromreg( SPI_SETBORDER_REGKEY2, SPI_SETBORDER_VALNAME, dpi);
        if( regval != curr_val)
            regval = metricfromreg( SPI_SETBORDER_REGKEY, SPI_SETBORDER_VALNAME, dpi);
        ok( regval==curr_val, "wrong value in registry %d, expected %d\n", regval, curr_val);
    }
    /* minimum border width is 1 */
    if (curr_val == 0) curr_val = 1;
    /* should be the same as the non client metrics */
    rc=SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( (UINT)ncm.iBorderWidth, curr_val, "NonClientMetric.iBorderWidth", "%d");
    /* and from SPI_GETBORDER */ 
    rc=SystemParametersInfoA( SPI_GETBORDER, 0, &border, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( border, curr_val, "SPI_{GET,SET}BORDER", "%d");
    /* test some SystemMetrics */
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
    NONCLIENTMETRICSA ncmsave;
    INT CaptionWidth;

    ncmsave.cbSize = sizeof( ncmsave);
    rc=SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncmsave, 0);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    /* CaptionWidth from the registry may have different value of iCaptionWidth
     * from the non client metrics (observed on WinXP).
     * Fix this so we can safely restore settings with the nonclientmetrics */
    CaptionWidth = metricfromreg(
            "Control Panel\\Desktop\\WindowMetrics","CaptionWidth", dpi);
    ncmsave.iCaptionWidth = CaptionWidth;

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
    /* This will restore sane values if the test hang previous run. */
    if ( old_border == 7 || old_border == 20 )
        old_border = 1;

    /* The SPI_SETBORDER seems to be buggy on Win9x/ME (looks like you need to
     * do it twice to make the intended change). So skip parts of the tests on
     * those platforms */
    if( !iswin9x) {
        test_setborder(1,  1, dpi);
        test_setborder(0,  1, dpi);
        test_setborder(2,  1, dpi);
    }
    test_setborder(1, 0, dpi);
    test_setborder(0, 0, dpi);
    test_setborder(3, 0, dpi);

    rc=SystemParametersInfoA( SPI_SETNONCLIENTMETRICS, 0, &ncmsave,
            SPIF_UPDATEINIFILE| SPIF_SENDCHANGE);
    test_change_message( SPI_SETNONCLIENTMETRICS, 1 );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",
        rc,GetLastError());
}

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

/* test_SPI_ICONHORIZONTALSPACING helper */
static void dotest_spi_iconhorizontalspacing( INT curr_val)
{
    BOOL rc;
    INT spacing, regval;
    ICONMETRICSA im;

    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_ICONHORIZONTALSPACING, 0 );
    if( curr_val < 32) curr_val = 32;
    /* The registry keys depend on the Windows version and the values too
     * let's test (works on win95,ME,NT4,2k,XP)
     */
    regval = metricfromreg( SPI_ICONHORIZONTALSPACING_REGKEY2, SPI_ICONHORIZONTALSPACING_VALNAME, dpi);
    if( regval != curr_val)
        regval = metricfromreg( SPI_ICONHORIZONTALSPACING_REGKEY, SPI_ICONHORIZONTALSPACING_VALNAME, dpi);
    ok( curr_val == regval,
        "wrong value in registry %d, expected %d\n", regval, curr_val);
    /* compare with what SPI_ICONHORIZONTALSPACING returns */
    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &spacing, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( spacing, curr_val, "ICONHORIZONTALSPACING", "%d");
    /* and with a system metrics */
    eq( GetSystemMetrics( SM_CXICONSPACING ), curr_val, "SM_CXICONSPACING", "%d" );
    /* and with what SPI_GETICONMETRICS returns */
    im.cbSize = sizeof(ICONMETRICSA);
    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( im.iHorzSpacing, curr_val, "SPI_GETICONMETRICS", "%d" );
}

static void test_SPI_ICONHORIZONTALSPACING( void )     /*     13 */
{
    BOOL rc;
    INT old_spacing;

    trace("testing SPI_ICONHORIZONTALSPACING\n");
    SetLastError(0xdeadbeef);
    /* default value: 75 */
    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &old_spacing, 0 );
    if (!test_error_msg(rc,"SPI_ICONHORIZONTALSPACING"))
        return;
    /* do not increase the value as it would upset the user's icon layout */
    dotest_spi_iconhorizontalspacing( old_spacing - 1);
    dotest_spi_iconhorizontalspacing( 10); /* minimum is 32 */
    /* restore */
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


/* test_SPI_ICONVERTICALSPACING helper */
static void dotest_spi_iconverticalspacing( INT curr_val)
{
    BOOL rc;
    INT spacing, regval;
    ICONMETRICSA im;

    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_ICONVERTICALSPACING, 0 );
    if( curr_val < 32) curr_val = 32;
    /* The registry keys depend on the Windows version and the values too
     * let's test (works on win95,ME,NT4,2k,XP)
     */
    regval = metricfromreg( SPI_ICONVERTICALSPACING_REGKEY2, SPI_ICONVERTICALSPACING_VALNAME, dpi);
    if( regval != curr_val)
        regval = metricfromreg( SPI_ICONVERTICALSPACING_REGKEY, SPI_ICONVERTICALSPACING_VALNAME, dpi);
    ok( curr_val == regval,
        "wrong value in registry %d, expected %d\n", regval, curr_val);
    /* compare with what SPI_ICONVERTICALSPACING returns */
    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &spacing, 0 );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( spacing, curr_val, "ICONVERTICALSPACING", "%d" );
    /* and with a system metrics */
    eq( GetSystemMetrics( SM_CYICONSPACING ), curr_val, "SM_CYICONSPACING", "%d" );
    /* and with what SPI_GETICONMETRICS returns */
    im.cbSize = sizeof(ICONMETRICSA);
    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( im.iVertSpacing, curr_val, "SPI_GETICONMETRICS", "%d" );
}

static void test_SPI_ICONVERTICALSPACING( void )       /*     24 */
{
    BOOL rc;
    INT old_spacing;

    trace("testing SPI_ICONVERTICALSPACING\n");
    SetLastError(0xdeadbeef);
    /* default value: 75 */
    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &old_spacing, 0 );
    if (!test_error_msg(rc,"SPI_ICONVERTICALSPACING"))
        return;
    /* do not increase the value as it would upset the user's icon layout */
    dotest_spi_iconverticalspacing( old_spacing - 1);
    /* same tests with a value less than the minimum 32 */
    dotest_spi_iconverticalspacing( 10);
    /* restore */
    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, old_spacing, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",
        rc,GetLastError());
}

static void test_SPI_SETICONTITLEWRAP( void )          /*     26 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;
    ICONMETRICSA im;

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
        UINT regval;

        rc=SystemParametersInfoA( SPI_SETICONTITLEWRAP, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETICONTITLEWRAP, 1 );
        regval = metricfromreg( SPI_SETICONTITLEWRAP_REGKEY2,
                SPI_SETICONTITLEWRAP_VALNAME, dpi);
        if( regval != vals[i])
            regval = metricfromreg( SPI_SETICONTITLEWRAP_REGKEY1,
                    SPI_SETICONTITLEWRAP_VALNAME, dpi);
        ok( regval == vals[i],
                "wrong value in registry %d, expected %d\n", regval, vals[i] );

        rc=SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}ICONTITLEWRAP", "%d" );
        /* and test with what SPI_GETICONMETRICS returns */
        im.cbSize = sizeof(ICONMETRICSA);
        rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im, FALSE );
        ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
        eq( im.iTitleWrap, (BOOL)vals[i], "SPI_GETICONMETRICS", "%d" );
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
            break;
            
        test_change_message( SPI_SETMOUSEBUTTONSWAP, 0 );
        test_reg_key( SPI_SETMOUSEBUTTONSWAP_REGKEY,
                      SPI_SETMOUSEBUTTONSWAP_VALNAME,
                      vals[i] ? "1" : "0" );
        eq( GetSystemMetrics( SM_SWAPBUTTON ), (int)vals[i],
            "SM_SWAPBUTTON", "%d" );
        rc=SwapMouseButton((BOOL)vals[i^1]);
        eq( GetSystemMetrics( SM_SWAPBUTTON ), (int)vals[i^1],
            "SwapMouseButton", "%d" );
        ok( rc==(BOOL)vals[i], "SwapMouseButton does not return previous state (really %d)\n", rc );
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

#define test_reg_metric( KEY, VAL, val) \
{   INT regval;\
    regval = metricfromreg( KEY, VAL, dpi);\
    ok( regval==val, "wrong value \"%s\" in registry %d, expected %d\n", VAL, regval, val);\
}
 
#define test_reg_metric2( KEY1, KEY2, VAL, val) \
{   INT regval;\
    regval = metricfromreg( KEY1, VAL, dpi);\
    if( regval != val) regval = metricfromreg( KEY2, VAL, dpi);\
    ok( regval==val, "wrong value \"%s\" in registry %d, expected %d\n", VAL, regval, val);\
}

#define test_reg_font( KEY, VAL, LF) \
{   LOGFONTA lfreg;\
    lffromreg( KEY, VAL, &lfreg);\
    ok( (lfreg.lfHeight < 0 ? (LF).lfHeight == lfreg.lfHeight :\
                MulDiv( -(LF).lfHeight , 72, dpi) == lfreg.lfHeight )&&\
        (LF).lfWidth == lfreg.lfWidth &&\
        (LF).lfWeight == lfreg.lfWeight &&\
        !strcmp( (LF).lfFaceName, lfreg.lfFaceName)\
        , "wrong value \"%s\" in registry %ld, %ld\n", VAL, (LF).lfHeight, lfreg.lfHeight);\
}

#define TEST_NONCLIENTMETRICS_REG( ncm) \
test_reg_metric2( SPI_SETBORDER_REGKEY2, SPI_SETBORDER_REGKEY, SPI_SETBORDER_VALNAME, (ncm).iBorderWidth);\
test_reg_metric( SPI_METRIC_REGKEY, SPI_SCROLLWIDTH_VALNAME, (ncm).iScrollWidth);\
test_reg_metric( SPI_METRIC_REGKEY, SPI_SCROLLHEIGHT_VALNAME, (ncm).iScrollHeight);\
/*FIXME: test_reg_metric( SPI_METRIC_REGKEY, SPI_CAPTIONWIDTH_VALNAME, (ncm).iCaptionWidth);*/\
test_reg_metric( SPI_METRIC_REGKEY, SPI_CAPTIONHEIGHT_VALNAME, (ncm).iCaptionHeight);\
test_reg_metric( SPI_METRIC_REGKEY, SPI_SMCAPTIONWIDTH_VALNAME, (ncm).iSmCaptionWidth);\
test_reg_metric( SPI_METRIC_REGKEY, SPI_SMCAPTIONHEIGHT_VALNAME, (ncm).iSmCaptionHeight);\
test_reg_metric( SPI_METRIC_REGKEY, SPI_MENUWIDTH_VALNAME, (ncm).iMenuWidth);\
test_reg_metric( SPI_METRIC_REGKEY, SPI_MENUHEIGHT_VALNAME, (ncm).iMenuHeight);\
test_reg_font( SPI_METRIC_REGKEY, SPI_MENUFONT_VALNAME, (ncm).lfMenuFont);\
test_reg_font( SPI_METRIC_REGKEY, SPI_CAPTIONFONT_VALNAME, (ncm).lfCaptionFont);\
test_reg_font( SPI_METRIC_REGKEY, SPI_SMCAPTIONFONT_VALNAME, (ncm).lfSmCaptionFont);\
test_reg_font( SPI_METRIC_REGKEY, SPI_STATUSFONT_VALNAME, (ncm).lfStatusFont);\
test_reg_font( SPI_METRIC_REGKEY, SPI_MESSAGEFONT_VALNAME, (ncm).lfMessageFont);

/* get text metric height value for the specified logfont */
static int get_tmheight( LOGFONTA *plf, int flag)
{
    TEXTMETRICA tm;
    HFONT hfont = CreateFontIndirectA( plf);
    hfont = SelectObject( hdc, hfont);
    GetTextMetricsA( hdc, &tm);
    hfont = SelectObject( hdc, hfont);
    return tm.tmHeight + (flag ? tm.tmExternalLeading : 0);
}

void test_GetSystemMetrics( void);

static void test_SPI_SETNONCLIENTMETRICS( void )               /*     44 */
{
    BOOL rc;
    INT expect;
    NONCLIENTMETRICSA Ncmorig;
    NONCLIENTMETRICSA Ncmnew;
    NONCLIENTMETRICSA Ncmcur;
    NONCLIENTMETRICSA Ncmstart;

    Ncmorig.cbSize = sizeof(NONCLIENTMETRICSA);
    Ncmnew.cbSize = sizeof(NONCLIENTMETRICSA);
    Ncmcur.cbSize = sizeof(NONCLIENTMETRICSA);
    Ncmstart.cbSize = sizeof(NONCLIENTMETRICSA);

    trace("testing SPI_{GET,SET}NONCLIENTMETRICS\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &Ncmorig, FALSE );
    if (!test_error_msg(rc,"SPI_{GET,SET}NONCLIENTMETRICS"))
        return;
    Ncmstart = Ncmorig;
    /* SPI_GETNONCLIENTMETRICS returns some "cooked" values. For instance if 
       the caption font height is higher than the CaptionHeight field,
       the latter is adjusted accordingly. To be able to restore these setting
       accurately be restore the raw values. */
    Ncmorig.iCaptionWidth = metricfromreg( SPI_METRIC_REGKEY, SPI_CAPTIONWIDTH_VALNAME, dpi);
    Ncmorig.iCaptionHeight = metricfromreg( SPI_METRIC_REGKEY, SPI_CAPTIONHEIGHT_VALNAME, dpi);
    Ncmorig.iSmCaptionHeight = metricfromreg( SPI_METRIC_REGKEY, SPI_SMCAPTIONHEIGHT_VALNAME, dpi);
    Ncmorig.iMenuHeight = metricfromreg( SPI_METRIC_REGKEY, SPI_MENUHEIGHT_VALNAME, dpi);
    /* test registry entries */
    TEST_NONCLIENTMETRICS_REG( Ncmorig)
    /* make small changes */
    Ncmnew = Ncmstart;
    Ncmnew.iBorderWidth += 1;
    Ncmnew.iScrollWidth += 1;
    Ncmnew.iScrollHeight -= 1;
    Ncmnew.iCaptionWidth -= 2;
    Ncmnew.iCaptionHeight += 2;
    Ncmnew.lfCaptionFont.lfHeight +=1;
    Ncmnew.lfCaptionFont.lfWidth +=2;
    Ncmnew.lfCaptionFont.lfWeight +=1;
    Ncmnew.iSmCaptionWidth += 1;
    Ncmnew.iSmCaptionHeight += 2;
    Ncmnew.lfSmCaptionFont.lfHeight +=3;
    Ncmnew.lfSmCaptionFont.lfWidth -=1;
    Ncmnew.lfSmCaptionFont.lfWeight +=3;
    Ncmnew.iMenuWidth += 1;
    Ncmnew.iMenuHeight += 2;
    Ncmnew.lfMenuFont.lfHeight +=1;
    Ncmnew.lfMenuFont.lfWidth +=1;
    Ncmnew.lfMenuFont.lfWeight +=2;
    Ncmnew.lfStatusFont.lfHeight -=1;
    Ncmnew.lfStatusFont.lfWidth -=1;
    Ncmnew.lfStatusFont.lfWeight +=3;
    Ncmnew.lfMessageFont.lfHeight -=2;
    Ncmnew.lfMessageFont.lfWidth -=1;
    Ncmnew.lfMessageFont.lfWeight +=4;

    rc=SystemParametersInfoA( SPI_SETNONCLIENTMETRICS, 0, &Ncmnew, SPIF_UPDATEINIFILE|
            SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETNONCLIENTMETRICS, 1 );
    /* get them back */
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &Ncmcur, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    /* test registry entries */
    TEST_NONCLIENTMETRICS_REG( Ncmcur)
    /* test the systemm metrics with these settings */
    test_GetSystemMetrics();
    /* now for something invalid: increase the {menu|caption|smcaption} fonts
       by a large amount will increase the {menu|caption|smcaption} height*/
    Ncmnew = Ncmstart;
    Ncmnew.lfMenuFont.lfHeight -= 8;
    Ncmnew.lfCaptionFont.lfHeight =-4;
    Ncmnew.lfSmCaptionFont.lfHeight -=10;
    /* also show that a few values are lo limited */
    Ncmnew.iCaptionWidth = 0;
    Ncmnew.iCaptionHeight = 0;
    Ncmnew.iScrollHeight = 0;
    Ncmnew.iScrollWidth  = 0;

    rc=SystemParametersInfoA( SPI_SETNONCLIENTMETRICS, 0, &Ncmnew, SPIF_UPDATEINIFILE|
            SPIF_SENDCHANGE);
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    test_change_message( SPI_SETNONCLIENTMETRICS, 1 );
    /* raw values are in registry */
    TEST_NONCLIENTMETRICS_REG( Ncmnew)
    /* get them back */
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &Ncmcur, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    /* cooked values are returned */
    expect = max( Ncmnew.iMenuHeight, 2 + get_tmheight( &Ncmnew.lfMenuFont, 1));
    ok( Ncmcur.iMenuHeight == expect,
        "MenuHeight: %d expected %d\n", Ncmcur.iMenuHeight, expect);
    expect = max( Ncmnew.iCaptionHeight, 2 + get_tmheight(&Ncmnew.lfCaptionFont, 0));
    ok( Ncmcur.iCaptionHeight == expect,
        "CaptionHeight: %d expected %d\n", Ncmcur.iCaptionHeight, expect);
    expect = max( Ncmnew.iSmCaptionHeight, 2 + get_tmheight( &Ncmnew.lfSmCaptionFont, 0));
    ok( Ncmcur.iSmCaptionHeight == expect,
        "SmCaptionHeight: %d expected %d\n", Ncmcur.iSmCaptionHeight, expect);

    ok( Ncmcur.iCaptionWidth == 8 ||
        Ncmcur.iCaptionWidth == Ncmstart.iCaptionWidth, /* with windows XP theme,  the value never changes */
        "CaptionWidth: %d expected 8\n", Ncmcur.iCaptionWidth);
    ok( Ncmcur.iScrollWidth == 8,
        "ScrollWidth: %d expected 8\n", Ncmcur.iScrollWidth);
    ok( Ncmcur.iScrollHeight == 8,
        "ScrollHeight: %d expected 8\n", Ncmcur.iScrollHeight);
    /* test the systemm metrics with these settings */
    test_GetSystemMetrics();
    /* restore */
    rc=SystemParametersInfoA( SPI_SETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS),
        &Ncmorig, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    test_change_message( SPI_SETNONCLIENTMETRICS, 0 );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETMINIMIZEDMETRICS( void )               /*     44 */
{
    BOOL rc;
    INT regval;
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
    /* test registry */
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINWIDTH_VALNAME, dpi);
    ok( regval == lpMm_orig.iWidth, "wrong value in registry %d, expected %d\n",
        regval, lpMm_orig.iWidth);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINHORZGAP_VALNAME, dpi);
    ok( regval == lpMm_orig.iHorzGap, "wrong value in registry %d, expected %d\n",
        regval, lpMm_orig.iHorzGap);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINVERTGAP_VALNAME, dpi);
    ok( regval == lpMm_orig.iVertGap, "wrong value in registry %d, expected %d\n",
        regval, lpMm_orig.iVertGap);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINARRANGE_VALNAME, dpi);
    ok( regval == lpMm_orig.iArrange, "wrong value in registry %d, expected %d\n",
        regval, lpMm_orig.iArrange);
    /* set some new values */
    lpMm_cur.iWidth = 180;
    lpMm_cur.iHorzGap = 1;
    lpMm_cur.iVertGap = 1;
    lpMm_cur.iArrange = 5;
    rc=SystemParametersInfoA( SPI_SETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS),
        &lpMm_cur, SPIF_UPDATEINIFILE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    /* read them back */
    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_new, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    /* and compare */
    eq( lpMm_new.iWidth,   lpMm_cur.iWidth,   "iWidth",   "%d" );
    eq( lpMm_new.iHorzGap, lpMm_cur.iHorzGap, "iHorzGap", "%d" );
    eq( lpMm_new.iVertGap, lpMm_cur.iVertGap, "iVertGap", "%d" );
    eq( lpMm_new.iArrange, lpMm_cur.iArrange, "iArrange", "%d" );
    /* test registry */
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINWIDTH_VALNAME, dpi);
    ok( regval == lpMm_new.iWidth, "wrong value in registry %d, expected %d\n",
        regval, lpMm_new.iWidth);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINHORZGAP_VALNAME, dpi);
    ok( regval == lpMm_new.iHorzGap, "wrong value in registry %d, expected %d\n",
        regval, lpMm_new.iHorzGap);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINVERTGAP_VALNAME, dpi);
    ok( regval == lpMm_new.iVertGap, "wrong value in registry %d, expected %d\n",
        regval, lpMm_new.iVertGap);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINARRANGE_VALNAME, dpi);
    ok( regval == lpMm_new.iArrange, "wrong value in registry %d, expected %d\n",
        regval, lpMm_new.iArrange);
    /* test some system metrics */
    eq( GetSystemMetrics( SM_CXMINIMIZED ) - 6,
        lpMm_new.iWidth,   "iWidth",   "%d" );
    eq( GetSystemMetrics( SM_CXMINSPACING ) - GetSystemMetrics( SM_CXMINIMIZED ),
        lpMm_new.iHorzGap, "iHorzGap", "%d" );
    eq( GetSystemMetrics( SM_CYMINSPACING ) - GetSystemMetrics( SM_CYMINIMIZED ),
        lpMm_new.iVertGap, "iVertGap", "%d" );
    eq( GetSystemMetrics( SM_ARRANGE ),
        lpMm_new.iArrange, "iArrange", "%d" );
    /* now some realy invalid settings */
    lpMm_cur.iWidth = -1;
    lpMm_cur.iHorzGap = -1;
    lpMm_cur.iVertGap = -1;
    lpMm_cur.iArrange = - 1;
    rc=SystemParametersInfoA( SPI_SETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS),
        &lpMm_cur, SPIF_UPDATEINIFILE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    /* read back */
    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_new, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    /* the width and H/V gaps have minimum 0, arrange is and'd with 0xf */
    eq( lpMm_new.iWidth,   0,   "iWidth",   "%d" );
    eq( lpMm_new.iHorzGap, 0, "iHorzGap", "%d" );
    eq( lpMm_new.iVertGap, 0, "iVertGap", "%d" );
    eq( lpMm_new.iArrange, 0xf & lpMm_cur.iArrange, "iArrange", "%d" );
    /* test registry */
#if 0 /* FIXME: cannot understand the results of this (11, 11, 11, 0) */
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINWIDTH_VALNAME, dpi);
    ok( regval == lpMm_new.iWidth, "wrong value in registry %d, expected %d\n",
        regval, lpMm_new.iWidth);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINHORZGAP_VALNAME, dpi);
    ok( regval == lpMm_new.iHorzGap, "wrong value in registry %d, expected %d\n",
        regval, lpMm_new.iHorzGap);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINVERTGAP_VALNAME, dpi);
    ok( regval == lpMm_new.iVertGap, "wrong value in registry %d, expected %d\n",
        regval, lpMm_new.iVertGap);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINARRANGE_VALNAME, dpi);
    ok( regval == lpMm_new.iArrange, "wrong value in registry %d, expected %d\n",
        regval, lpMm_new.iArrange);
#endif
    /* test some system metrics */
    eq( GetSystemMetrics( SM_CXMINIMIZED ) - 6,
        lpMm_new.iWidth,   "iWidth",   "%d" );
    eq( GetSystemMetrics( SM_CXMINSPACING ) - GetSystemMetrics( SM_CXMINIMIZED ),
        lpMm_new.iHorzGap, "iHorzGap", "%d" );
    eq( GetSystemMetrics( SM_CYMINSPACING ) - GetSystemMetrics( SM_CYMINIMIZED ),
        lpMm_new.iVertGap, "iVertGap", "%d" );
    eq( GetSystemMetrics( SM_ARRANGE ),
        lpMm_new.iArrange, "iArrange", "%d" );
    /* restore */
    rc=SystemParametersInfoA( SPI_SETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS),
        &lpMm_orig, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
    /* check that */
    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_new, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    eq( lpMm_new.iWidth,   lpMm_orig.iWidth,   "iWidth",   "%d" );
    eq( lpMm_new.iHorzGap, lpMm_orig.iHorzGap, "iHorzGap", "%d" );
    eq( lpMm_new.iVertGap, lpMm_orig.iVertGap, "iVertGap", "%d" );
    eq( lpMm_new.iArrange, lpMm_orig.iArrange, "iArrange", "%d" );
}

static void test_SPI_SETICONMETRICS( void )               /*     46 */
{
    BOOL rc, wrap;
    INT spacing;
    ICONMETRICSA im_orig;
    ICONMETRICSA im_new;
    ICONMETRICSA im_cur;
    INT regval;
        
    im_orig.cbSize = sizeof(ICONMETRICSA);
    im_new.cbSize = sizeof(ICONMETRICSA);
    im_cur.cbSize = sizeof(ICONMETRICSA);

    trace("testing SPI_{GET,SET}ICONMETRICS\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im_orig, FALSE );
    if (!test_error_msg(rc,"SPI_{GET,SET}ICONMETRICS"))
        return;
   /* check some registry values */ 
    regval = metricfromreg( SPI_ICONHORIZONTALSPACING_REGKEY, SPI_ICONHORIZONTALSPACING_VALNAME, dpi);
    ok( regval==im_orig.iHorzSpacing, "wrong value in registry %d, expected %d\n", regval, im_orig.iHorzSpacing);
    regval = metricfromreg( SPI_ICONVERTICALSPACING_REGKEY, SPI_ICONVERTICALSPACING_VALNAME, dpi);
    ok( regval==im_orig.iVertSpacing, "wrong value in registry %d, expected %d\n", regval, im_orig.iVertSpacing);
    regval = metricfromreg( SPI_SETICONTITLEWRAP_REGKEY2, SPI_SETICONTITLEWRAP_VALNAME, dpi);
    if( regval != im_orig.iTitleWrap)
        regval = metricfromreg( SPI_SETICONTITLEWRAP_REGKEY1, SPI_SETICONTITLEWRAP_VALNAME, dpi);
    ok( regval==im_orig.iTitleWrap, "wrong value in registry %d, expected %d\n", regval, im_orig.iTitleWrap);

    /* change everything without creating something invalid ( Win9x would ignore
     * an invalid font for instance) */
    im_cur = im_orig;
    im_cur.iHorzSpacing += 10;
    im_cur.iVertSpacing += 6;
    im_cur.iTitleWrap = !im_cur.iTitleWrap;
    im_cur.lfFont.lfHeight += 1;
    im_cur.lfFont.lfWidth += 2;
    im_cur.lfFont.lfEscapement = 1;
    im_cur.lfFont.lfWeight = im_cur.lfFont.lfWeight > 100 ? 1 : 314;
    im_cur.lfFont.lfItalic = !im_cur.lfFont.lfItalic;
    im_cur.lfFont.lfStrikeOut = !im_cur.lfFont.lfStrikeOut;
    im_cur.lfFont.lfUnderline = !im_cur.lfFont.lfUnderline;
    im_cur.lfFont.lfCharSet = im_cur.lfFont.lfCharSet ? 0 : 1;
    im_cur.lfFont.lfOutPrecision = im_cur.lfFont.lfOutPrecision == OUT_DEFAULT_PRECIS ?
                                OUT_TT_PRECIS : OUT_DEFAULT_PRECIS;
    im_cur.lfFont.lfClipPrecision ^= CLIP_LH_ANGLES;
    im_cur.lfFont.lfPitchAndFamily = im_cur.lfFont.lfPitchAndFamily ? 0 : 1;
    im_cur.lfFont.lfQuality = im_cur.lfFont.lfQuality == DEFAULT_QUALITY ? 
                                DRAFT_QUALITY : DEFAULT_QUALITY;
    if( strcmp( im_cur.lfFont.lfFaceName, "MS Serif"))
        strcpy( im_cur.lfFont.lfFaceName, "MS Serif");
    else
        strcpy( im_cur.lfFont.lfFaceName, "MS Sans Serif");

    rc=SystemParametersInfoA( SPI_SETICONMETRICS, sizeof(ICONMETRICSA), &im_cur, SPIF_UPDATEINIFILE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());

    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im_new, FALSE );
    ok(rc!=0,"SystemParametersInfoA: rc=%d err=%ld\n",rc,GetLastError());
    /* test GET <-> SETICONMETRICS */ 
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
    ok( !strcmp( im_new.lfFont.lfFaceName, im_cur.lfFont.lfFaceName),
        "wrong facename \"%s\", should be \"%s\"\n", im_new.lfFont.lfFaceName,
        im_cur.lfFont.lfFaceName);
    /* test some system metrics */
    eq( GetSystemMetrics( SM_CXICONSPACING ),
        im_new.iHorzSpacing, "iHorzSpacing", "%d" );
    eq( GetSystemMetrics( SM_CYICONSPACING ),
        im_new.iVertSpacing, "iVertSpacing", "%d" );
   /* check some registry values */ 
    regval = metricfromreg( SPI_ICONHORIZONTALSPACING_REGKEY, SPI_ICONHORIZONTALSPACING_VALNAME, dpi);
    ok( regval==im_cur.iHorzSpacing, "wrong value in registry %d, expected %d\n", regval, im_cur.iHorzSpacing);
    regval = metricfromreg( SPI_ICONVERTICALSPACING_REGKEY, SPI_ICONVERTICALSPACING_VALNAME, dpi);
    ok( regval==im_cur.iVertSpacing, "wrong value in registry %d, expected %d\n", regval, im_cur.iVertSpacing);
    regval = metricfromreg( SPI_SETICONTITLEWRAP_REGKEY2, SPI_SETICONTITLEWRAP_VALNAME, dpi);
    if( regval != im_cur.iTitleWrap)
        regval = metricfromreg( SPI_SETICONTITLEWRAP_REGKEY1, SPI_SETICONTITLEWRAP_VALNAME, dpi);
    ok( regval==im_cur.iTitleWrap, "wrong value in registry %d, expected %d\n", regval, im_cur.iTitleWrap);
    /* test some values from other SPI_GETxxx calls */
    rc = SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &spacing, 0 );
    ok( rc && spacing == im_cur.iHorzSpacing,
        "SystemParametersInfoA( SPI_ICONHORIZONTALSPACING...) failed or returns wrong value %d instead of %d\n",
        spacing, im_cur.iHorzSpacing);
    rc = SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &spacing, 0 );
    ok( rc && spacing == im_cur.iVertSpacing,
        "SystemParametersInfoA( SPI_ICONVERTICALSPACING...) failed or returns wrong value %d instead of %d\n",
        spacing, im_cur.iVertSpacing);
    rc = SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &wrap, 0 );
    ok( rc && wrap == im_cur.iTitleWrap,
        "SystemParametersInfoA( SPI_GETICONTITLEWRAP...) failed or returns wrong value %d instead of %d\n",
        wrap, im_cur.iTitleWrap);
    /* restore old values */
    rc=SystemParametersInfoA( SPI_SETICONMETRICS, sizeof(ICONMETRICSA), &im_orig,SPIF_UPDATEINIFILE );
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
        eq( v, (BOOL)vals[i], "SPI_GETKEYBOARDPREF", "%d" );
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
        eq( v, (BOOL)vals[i], "SPI_GETSCREENREADER", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSCREENREADER, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc!=0,"***warning*** failed to restore the original value: rc=%d err=%ld\n",rc,GetLastError());
}

static void test_SPI_SETFONTSMOOTHING( void )         /*     75 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={0xffffffff,0,1,2};
    unsigned int i;

    trace("testing SPI_{GET,SET}FONTSMOOTHING\n");
    if( iswin9x) return; /* 95/98/ME don't seem to implement this fully */ 
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETFONTSMOOTHING, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}FONTSMOOTHING"))
        return;

    for (i=0;i<sizeof(vals)/sizeof(*vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETFONTSMOOTHING, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        test_change_message( SPI_SETFONTSMOOTHING, 0 );
        test_reg_key( SPI_SETFONTSMOOTHING_REGKEY,
                      SPI_SETFONTSMOOTHING_VALNAME,
                      vals[i] ? "2" : "0" );

        rc=SystemParametersInfoA( SPI_GETFONTSMOOTHING, 0, &v, 0 );
        ok(rc!=0,"%d: rc=%d err=%ld\n",i,rc,GetLastError());
        eq( v, vals[i] ? 1 : 0, "SPI_GETFONTSMOOTHING", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETFONTSMOOTHING, old_b, 0, SPIF_UPDATEINIFILE );
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
    test_SPI_SETBORDER();                       /*      6 */
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
    test_SPI_SETNONCLIENTMETRICS();             /*     42 */
    test_SPI_SETMINIMIZEDMETRICS();             /*     44 */
    test_SPI_SETICONMETRICS();                  /*     46 */
    test_SPI_SETWORKAREA();                     /*     47 */
    test_SPI_SETSHOWSOUNDS();                   /*     57 */
    test_SPI_SETKEYBOARDPREF();                 /*     69 */
    test_SPI_SETSCREENREADER();                 /*     71 */
    test_SPI_SETFONTSMOOTHING();                /*     75 */
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

/* test calculation of GetSystemMetrics values (mostly) from non client metrics,
 * icon metrics and minimized metrics. 
 */

/* copied from wine's GdiGetCharDimensions, which is not available on most
 * windows versions */
static LONG _GdiGetCharDimensions(HDC hdc, LPTEXTMETRICA lptm, LONG *height)
{
    SIZE sz;
    static const CHAR alphabet[] = {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0};

    if(lptm && !GetTextMetricsA(hdc, lptm)) return 0;

    if(!GetTextExtentPointA(hdc, alphabet, 52, &sz)) return 0;

    if (height) *height = sz.cy;
    return (sz.cx / 26 + 1) / 2;
}

/* get text metrics and/or "average" char width of the specified logfont
 * for the specified dc */
void get_text_metr_size( HDC hdc, LOGFONTA *plf, TEXTMETRICA * ptm, UINT *psz)
{
    HFONT hfont, hfontsav;
    TEXTMETRICA tm;
    if( !ptm) ptm = &tm;
    hfont = CreateFontIndirectA( plf);
    if( !hfont || ( hfontsav = SelectObject( hdc, hfont)) == NULL ) {
        ptm->tmHeight = -1;
        if( psz) *psz = 10;
        if( hfont) DeleteObject( hfont);
        return;
    }
    GetTextMetricsA( hdc, ptm);
    if( psz)
        if( !(*psz = _GdiGetCharDimensions( hdc, ptm, NULL)))
            *psz = 10;
    SelectObject( hdc, hfontsav);
    DeleteObject( hfont);
}

static int gsm_error_ctr;
static UINT smcxsmsize = 999999999;

#define ok_gsm( i, e)\
{\
    int exp = (e);\
    int act = GetSystemMetrics( (i));\
    if( exp != act) gsm_error_ctr++;\
    ok( !( exp != act),"GetSystemMetrics(%s): expected %d actual %d\n", #i, exp,act);\
}
#define ok_gsm_2( i, e1, e2)\
{\
    int exp1 = (e1);\
    int exp2 = (e2);\
    int act = GetSystemMetrics( (i));\
    if( exp1 != act && exp2 != act) gsm_error_ctr++;\
    ok( !( exp1 != act && exp2 != act), "GetSystemMetrics(%s): expected %d or %d actual %d\n", #i, exp1, exp2, act);\
}
#define ok_gsm_3( i, e1, e2, e3)\
{\
    int exp1 = (e1);\
    int exp2 = (e2);\
    int exp3 = (e3);\
    int act = GetSystemMetrics( (i));\
    if( exp1 != act && exp2 != act && exp3 != act) gsm_error_ctr++;\
    ok( !( exp1 != act && exp2 != act && exp3 != act),"GetSystemMetrics(%s): expected %d or %d or %d actual %d\n", #i, exp1, exp2, exp3, act);\
}

void test_GetSystemMetrics( void)
{
    TEXTMETRICA tmMenuFont;
    UINT IconSpacing, IconVerticalSpacing;

    HDC hdc = CreateIC( "Display", 0, 0, 0);
    UINT avcwCaption;
    INT CaptionWidth;
    MINIMIZEDMETRICS minim;
    NONCLIENTMETRICS ncm;
    minim.cbSize = sizeof( minim);
    ncm.cbSize = sizeof( ncm);
    SystemParametersInfo( SPI_GETMINIMIZEDMETRICS, 0, &minim, 0);
    SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    /* CaptionWidth from the registry may have different value of iCaptionWidth
     * from the non client metrics (observed on WinXP) */
    CaptionWidth = metricfromreg(
            "Control Panel\\Desktop\\WindowMetrics","CaptionWidth", dpi);
    get_text_metr_size( hdc, &ncm.lfMenuFont, &tmMenuFont, NULL);
    get_text_metr_size( hdc, &ncm.lfCaptionFont, NULL, &avcwCaption);
    /* FIXME: use icon metric */
    if( !SystemParametersInfoA( SPI_ICONVERTICALSPACING, 0, &IconVerticalSpacing, 0))
        IconVerticalSpacing = 0;
    if( !SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, 0, &IconSpacing, 0 ))
        IconSpacing = 0;
    /* reset error counters */
    gsm_error_ctr = 0;

    /* the tests: */

    /* SM_CXSCREEN, cannot test these two */
    /* SM_CYSCREEN */
    ok_gsm( SM_CXVSCROLL,  ncm.iScrollWidth);
    ok_gsm( SM_CYHSCROLL,  ncm.iScrollWidth);
    ok_gsm( SM_CYCAPTION, ncm.iCaptionHeight+1);
    ok_gsm( SM_CXBORDER, 1);
    ok_gsm( SM_CYBORDER, 1);
    ok_gsm( SM_CXDLGFRAME, 3);
    ok_gsm( SM_CYDLGFRAME, 3);
    ok_gsm( SM_CYVTHUMB,  ncm.iScrollHeight);
    ok_gsm( SM_CXHTHUMB,  ncm.iScrollHeight);
    /* SM_CXICON */
    /* SM_CYICON */
    /* SM_CXCURSOR */
    /* SM_CYCURSOR */
    ok_gsm( SM_CYMENU, ncm.iMenuHeight + 1);
    ok_gsm( SM_CXFULLSCREEN,
            GetSystemMetrics( SM_CXMAXIMIZED) - 2 * GetSystemMetrics( SM_CXFRAME));
    ok_gsm( SM_CYFULLSCREEN,
            GetSystemMetrics( SM_CYMAXIMIZED) - GetSystemMetrics( SM_CYMIN));
    /* SM_CYKANJIWINDOW */
    /* SM_MOUSEPRESENT */
    ok_gsm( SM_CYVSCROLL, ncm.iScrollHeight);
    ok_gsm( SM_CXHSCROLL, ncm.iScrollHeight);
    /* SM_DEBUG */
    /* SM_SWAPBUTTON */
    /* SM_RESERVED1 */
    /* SM_RESERVED2 */
    /* SM_RESERVED3 */
    /* SM_RESERVED4 */
    ok_gsm( SM_CXMIN, 3 * max( CaptionWidth, 8) + GetSystemMetrics( SM_CYSIZE) +
            4 + 4 * avcwCaption + 2 * GetSystemMetrics( SM_CXFRAME));
    ok_gsm( SM_CYMIN, GetSystemMetrics( SM_CYCAPTION) +
            2 * GetSystemMetrics( SM_CYFRAME));
    ok_gsm_2( SM_CXSIZE,
        ncm.iCaptionWidth,  /* classic/standard windows style */
        GetSystemMetrics( SM_CYCAPTION) - 1 /* WinXP style */
        );
    ok_gsm( SM_CYSIZE,  ncm.iCaptionHeight);
    ok_gsm( SM_CXFRAME, ncm.iBorderWidth + 3);
    ok_gsm( SM_CYFRAME, ncm.iBorderWidth + 3);
    ok_gsm( SM_CXMINTRACK,  GetSystemMetrics( SM_CXMIN));
    ok_gsm( SM_CYMINTRACK,  GetSystemMetrics( SM_CYMIN));
    /* SM_CXDOUBLECLK */
    /* SM_CYDOUBLECLK */
    if( IconSpacing) ok_gsm( SM_CXICONSPACING, IconSpacing);
    if( IconVerticalSpacing) ok_gsm( SM_CYICONSPACING, IconVerticalSpacing);
    /* SM_MENUDROPALIGNMENT */
    /* SM_PENWINDOWS */
    /* SM_DBCSENABLED */
    /* SM_CMOUSEBUTTONS */
    /* SM_SECURE */
    ok_gsm( SM_CXEDGE, 2);
    ok_gsm( SM_CYEDGE, 2);
    ok_gsm( SM_CXMINSPACING, GetSystemMetrics( SM_CXMINIMIZED) + minim.iHorzGap );
    ok_gsm( SM_CYMINSPACING, GetSystemMetrics( SM_CYMINIMIZED) + minim.iVertGap );
    /* SM_CXSMICON */
    /* SM_CYSMICON */
    ok_gsm( SM_CYSMCAPTION, ncm.iSmCaptionHeight + 1);
    ok_gsm_3( SM_CXSMSIZE,
        ncm.iSmCaptionWidth, /* classic/standard windows style */
        GetSystemMetrics( SM_CYSMCAPTION) - 1, /* WinXP style */
        smcxsmsize /* winXP seems to cache this value: setnonclientmetric
                      does not change it */
        );
    ok_gsm( SM_CYSMSIZE, GetSystemMetrics( SM_CYSMCAPTION) - 1);
    ok_gsm( SM_CXMENUSIZE, ncm.iMenuWidth);
    ok_gsm( SM_CYMENUSIZE, ncm.iMenuHeight);
    /* SM_ARRANGE */
    ok_gsm( SM_CXMINIMIZED, minim.iWidth + 6);
    ok_gsm( SM_CYMINIMIZED, GetSystemMetrics( SM_CYCAPTION) + 5);
    ok_gsm( SM_CXMAXTRACK, GetSystemMetrics( SM_CXSCREEN) +
            4 + 2 * GetSystemMetrics( SM_CXFRAME));
    ok_gsm( SM_CYMAXTRACK, GetSystemMetrics( SM_CYSCREEN) +
            4 + 2 * GetSystemMetrics( SM_CYFRAME));
    /* the next two cannot really be tested as they depend on (application)
     * toolbars */
    /* SM_CXMAXIMIZED */
    /* SM_CYMAXIMIZED */
    /* SM_NETWORK */
    /* */
    /* */
    /* */
    /* SM_CLEANBOOT */
    /* SM_CXDRAG */
    /* SM_CYDRAG */
    /* SM_SHOWSOUNDS */
    ok_gsm( SM_CXMENUCHECK,
            ((tmMenuFont.tmHeight + tmMenuFont.tmExternalLeading+1)/2)*2-1);
    ok_gsm( SM_CYMENUCHECK,
            ((tmMenuFont.tmHeight + tmMenuFont.tmExternalLeading+1)/2)*2-1);
    /* SM_SLOWMACHINE */
    /* SM_MIDEASTENABLED */
    /* SM_MOUSEWHEELPRESENT */
    /* SM_XVIRTUALSCREEN */
    /* SM_YVIRTUALSCREEN */
    /* SM_CXVIRTUALSCREEN */
    /* SM_CYVIRTUALSCREEN */
    /* SM_CMONITORS */
    /* SM_SAMEDISPLAYFORMAT */
    /* SM_IMMENABLED */
    /* SM_CXFOCUSBORDER */
    /* SM_CYFOCUSBORDER */
    /* SM_TABLETPC */
    /* SM_MEDIACENTER */
    /* SM_CMETRICS */
    /* end of tests */
    if( gsm_error_ctr ) { /* if any errors where found */
        trace( "BorderWidth %d CaptionWidth %d CaptionHeight %d IconSpacing %d IconVerticalSpacing %d\n",
                ncm.iBorderWidth, ncm.iCaptionWidth, ncm.iCaptionHeight, IconSpacing, IconVerticalSpacing);
        trace( "MenuHeight %d MenuWidth %d ScrollHeight %d ScrollWidth %d SmCaptionHeight %d SmCaptionWidth %d\n",
                ncm.iMenuHeight, ncm.iMenuWidth, ncm.iScrollHeight, ncm.iScrollWidth, ncm.iSmCaptionHeight, ncm.iSmCaptionWidth);
        trace( "Captionfontchar width %d  MenuFont %ld,%ld CaptionWidth from registry: %d\n",
                avcwCaption, tmMenuFont.tmHeight, tmMenuFont.tmExternalLeading, CaptionWidth);
    }
    ReleaseDC( 0, hdc);
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

    hdc = GetDC(0);
    dpi = GetDeviceCaps( hdc, LOGPIXELSY);
    iswin9x = GetVersion() & 0x80000000;

    /* This test requires interactivity, if we don't have it, give up */
    if (!SystemParametersInfoA( SPI_SETBEEP, TRUE, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE ) &&
        GetLastError()==ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION) return;

    argc = winetest_get_mainargs(&argv);
    strict=(argc >= 3 && strcmp(argv[2],"strict")==0);
    trace("strict=%d\n",strict);

    trace("testing GetSystemMetrics with your current desktop settings\n");
    test_GetSystemMetrics( );

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
    ReleaseDC( 0, hdc);

}
