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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winreg.h"
#include "winuser.h"
#include "winnls.h"

static BOOL (WINAPI *pIsProcessDPIAware)(void);
static BOOL (WINAPI *pSetProcessDPIAware)(void);
static BOOL (WINAPI *pSetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
static BOOL (WINAPI *pGetProcessDpiAwarenessInternal)(HANDLE,DPI_AWARENESS*);
static BOOL (WINAPI *pSetProcessDpiAwarenessInternal)(DPI_AWARENESS);
static UINT (WINAPI *pGetDpiForSystem)(void);
static UINT (WINAPI *pGetDpiForWindow)(HWND);
static BOOL (WINAPI *pGetDpiForMonitorInternal)(HMONITOR,UINT,UINT*,UINT*);
static DPI_AWARENESS_CONTEXT (WINAPI *pGetThreadDpiAwarenessContext)(void);
static DPI_AWARENESS_CONTEXT (WINAPI *pSetThreadDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
static DPI_AWARENESS_CONTEXT (WINAPI *pGetWindowDpiAwarenessContext)(HWND);
static DPI_AWARENESS (WINAPI *pGetAwarenessFromDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
static BOOL (WINAPI *pIsValidDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
static INT (WINAPI *pGetSystemMetricsForDpi)(INT,UINT);
static BOOL (WINAPI *pSystemParametersInfoForDpi)(UINT,UINT,void*,UINT,UINT);
static BOOL (WINAPI *pAdjustWindowRectExForDpi)(LPRECT,DWORD,BOOL,DWORD,UINT);
static BOOL (WINAPI *pLogicalToPhysicalPointForPerMonitorDPI)(HWND,POINT*);
static BOOL (WINAPI *pPhysicalToLogicalPointForPerMonitorDPI)(HWND,POINT*);
static LONG (WINAPI *pGetAutoRotationState)(PAR_STATE);
static BOOL (WINAPI *pAreDpiAwarenessContextsEqual)(DPI_AWARENESS_CONTEXT,DPI_AWARENESS_CONTEXT);
static LONG (WINAPI *pGetDpiFromDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

static BOOL strict;
static int dpi, real_dpi;

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
#define SPI_SETFONTSMOOTHINGTYPE_VALNAME        "FontSmoothingType"
#define SPI_SETFONTSMOOTHINGCONTRAST_VALNAME    "FontSmoothingGamma"
#define SPI_SETFONTSMOOTHINGORIENTATION_VALNAME "FontSmoothingOrientation"
#define SPI_SETLOWPOWERACTIVE_REGKEY            "Control Panel\\Desktop"
#define SPI_SETLOWPOWERACTIVE_VALNAME           "LowPowerActive"
#define SPI_SETPOWEROFFACTIVE_REGKEY            "Control Panel\\Desktop"
#define SPI_SETPOWEROFFACTIVE_VALNAME           "PowerOffActive"
#define SPI_SETDRAGFULLWINDOWS_REGKEY           "Control Panel\\Desktop"
#define SPI_SETDRAGFULLWINDOWS_VALNAME          "DragFullWindows"
#define SPI_SETSNAPTODEFBUTTON_REGKEY           "Control Panel\\Mouse"
#define SPI_SETSNAPTODEFBUTTON_VALNAME          "SnapToDefaultButton"
#define SPI_SETMOUSEHOVERWIDTH_REGKEY           "Control Panel\\Mouse"
#define SPI_SETMOUSEHOVERWIDTH_VALNAME          "MouseHoverWidth"
#define SPI_SETMOUSEHOVERHEIGHT_REGKEY          "Control Panel\\Mouse"
#define SPI_SETMOUSEHOVERHEIGHT_VALNAME         "MouseHoverHeight"
#define SPI_SETMOUSEHOVERTIME_REGKEY            "Control Panel\\Mouse"
#define SPI_SETMOUSEHOVERTIME_VALNAME           "MouseHoverTime"
#define SPI_SETMOUSESCROLLCHARS_REGKEY          "Control Panel\\Desktop"
#define SPI_SETMOUSESCROLLCHARS_VALNAME         "WheelScrollChars"
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
static int last_bpp;
static BOOL displaychange_ok = FALSE, displaychange_test_active = FALSE;
static HANDLE displaychange_sem = 0;

#define run_in_process( a, b ) run_in_process_( __FILE__, __LINE__, a, b )
static void run_in_process_( const char *file, int line, char **argv, const char *args )
{
    STARTUPINFOA startup = {.cb = sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION info = {0};
    char cmdline[MAX_PATH * 2];
    DWORD ret;

    sprintf( cmdline, "%s %s %s", argv[0], argv[1], args );
    ret = CreateProcessA( NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info );
    ok_(file, line)( ret, "CreateProcessA failed, error %lu\n", GetLastError() );
    if (!ret) return;

    wait_child_process( info.hProcess );
    CloseHandle( info.hThread );
    CloseHandle( info.hProcess );
}

static BOOL get_reg_dword(HKEY base, const char *key_name, const char *value_name, DWORD *value)
{
    HKEY key;
    DWORD type, data, size = sizeof(data);
    BOOL ret = FALSE;

    if (RegOpenKeyA(base, key_name, &key) == ERROR_SUCCESS)
    {
        if (RegQueryValueExA(key, value_name, NULL, &type, (void *)&data, &size) == ERROR_SUCCESS &&
            type == REG_DWORD)
        {
            *value = data;
            ret = TRUE;
        }
        RegCloseKey(key);
    }
    return ret;
}

static DWORD get_real_dpi(void)
{
    DPI_AWARENESS_CONTEXT ctx;
    DWORD dpi;

    if (pSetThreadDpiAwarenessContext)
    {
        ctx = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
        ok( ctx == (DPI_AWARENESS_CONTEXT)0x80006010, "got %p\n", ctx );
        dpi = pGetDpiForSystem();
        ok( dpi, "GetDpiForSystem failed\n" );
        /* restore process-wide DPI awareness context */
        ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80006010 );
        ok( ctx == (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x11 | (dpi << 8)), "got %p\n", ctx );
        return dpi;
    }
    if (get_reg_dword(HKEY_CURRENT_USER, "Control Panel\\Desktop", "LogPixels", &dpi))
        return dpi;
    if (get_reg_dword(HKEY_CURRENT_CONFIG, "Software\\Fonts", "LogPixels", &dpi))
        return dpi;
    return USER_DEFAULT_SCREEN_DPI;
}

static LRESULT CALLBACK SysParamsTestWndProc( HWND hWnd, UINT msg, WPARAM wParam,
                                              LPARAM lParam )
{
    switch (msg) {

    case WM_DISPLAYCHANGE:
        ok(displaychange_ok, "Unexpected WM_DISPLAYCHANGE message\n");
        last_bpp = wParam;
        displaychange_ok = FALSE;
        ReleaseSemaphore(displaychange_sem, 1, 0);
        break;

    case WM_SETTINGCHANGE:
        if (wParam == SPI_SETHANDEDNESS) break; /* ignore */
        if (!change_counter++) change_last_param = 0;
        if  (displaychange_test_active) break;
        if (!change_last_param) change_last_param = wParam;
        else if (change_last_param != wParam)
        {
            switch (wParam)
            {
            /* ignore these messages when caused by other actions */
            case SPI_ICONVERTICALSPACING:
            case SPI_SETWORKAREA:
                break;
            default:
                ok( 0, "too many changes counter=%d last change=%Iu\n", change_counter, wParam );
                break;
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    /* drop through */
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
    SendMessageA( ghTestWnd, WM_NULL, 0, 0 );
    if (change_counter==0 && optional==1)
        return;
    ok( change_counter >= 1, "Missed a message: change_counter=%d\n", change_counter );
    change_counter = 0;
    ok( action == change_last_param, "Wrong action got %d expected %d\n", change_last_param, action );
    change_last_param = 0;
}

static void flush_change_messages(void)
{
    change_counter = 0;
    SendMessageA( ghTestWnd, WM_NULL, 0, 0 );
    change_counter = 0;
}

static BOOL test_error_msg ( int rc, const char *name )
{
    DWORD last_error = GetLastError();

    if (rc==0)
    {
        if (last_error==0xdeadbeef || last_error==ERROR_INVALID_SPI_VALUE || last_error==ERROR_INVALID_PARAMETER)
        {
            skip("%s not supported on this platform\n", name);
        }
        else if (last_error==ERROR_ACCESS_DENIED)
        {
            skip("%s does not have privileges to run\n", name);
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
 */
static void _test_reg_key( LPCSTR subKey1, LPCSTR subKey2, LPCSTR valName1, LPCSTR valName2,
                           const void *exp_value, DWORD exp_type, BOOL optional )
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
        ok( type == exp_type, "wrong type %lu/%lu\n", type, exp_type );
        switch (exp_type)
        {
        case REG_DWORD:
            ok( *(DWORD *)value == *(DWORD *)exp_value,
                "Wrong value in registry: %s %s %08lx/%08lx\n",
                subKey1, valName1, *(DWORD *)value, *(DWORD *)exp_value );
            break;
        case REG_SZ:
            ok( !strcmp( exp_value, value ),
                "Wrong value in registry: %s %s '%s' instead of '%s'\n",
                subKey1, valName1, value, (const char *)exp_value );
            break;
        }
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
            ok( type == exp_type, "wrong type %lu/%lu\n", type, exp_type );
            switch (exp_type)
            {
            case REG_DWORD:
                ok( *(DWORD *)value == *(DWORD *)exp_value,
                    "Wrong value in registry: %s %s %08lx/%08lx\n",
                    subKey1, valName1, *(DWORD *)value, *(DWORD *)exp_value );
                break;
            case REG_SZ:
                ok( !strcmp( exp_value, value ),
                    "Wrong value in registry: %s %s '%s' instead of '%s'\n",
                    subKey1, valName1, value, (const char *)exp_value );
                break;
            }
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
            ok( type == exp_type, "wrong type %lu/%lu\n", type, exp_type );
            switch (exp_type)
            {
            case REG_DWORD:
                ok( *(DWORD *)value == *(DWORD *)exp_value,
                    "Wrong value in registry: %s %s %08lx/%08lx\n",
                    subKey1, valName1, *(DWORD *)value, *(DWORD *)exp_value );
                break;
            case REG_SZ:
                ok( !strcmp( exp_value, value ),
                    "Wrong value in registry: %s %s '%s' instead of '%s'\n",
                    subKey1, valName1, value, (const char *)exp_value );
                break;
            }
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
                ok( type == exp_type, "wrong type %lu/%lu\n", type, exp_type );
                switch (exp_type)
                {
                case REG_DWORD:
                    ok( *(DWORD *)value == *(DWORD *)exp_value,
                        "Wrong value in registry: %s %s %08lx/%08lx\n",
                        subKey1, valName1, *(DWORD *)value, *(DWORD *)exp_value );
                    break;
                case REG_SZ:
                    ok( !strcmp( exp_value, value ),
                        "Wrong value in registry: %s %s '%s' instead of '%s'\n",
                        subKey1, valName1, value, (const char *)exp_value );
                    break;
                }
                found++;
            }
            else if (strict)
            {
                ok( 0,"Missing registry entry: subKey=%s, valName=%s\n",
                    subKey2, valName2 );
            }
         }
    }
    ok(found || optional,
       "Missing registry values: %s or %s in keys: %s or %s\n",
       valName1, (valName2?valName2:"<n/a>"), subKey1, (subKey2?subKey2:"<n/a>") );
}

#define test_reg_key( subKey, valName, testValue ) \
    _test_reg_key( subKey, NULL, valName, NULL, testValue, REG_SZ, FALSE )
#define test_reg_key_optional( subKey, valName, testValue ) \
    _test_reg_key( subKey, NULL, valName, NULL, testValue, REG_SZ, TRUE )
#define test_reg_key_ex( subKey1, subKey2, valName, testValue ) \
    _test_reg_key( subKey1, subKey2, valName, NULL, testValue, REG_SZ, FALSE )
#define test_reg_key_ex2( subKey1, subKey2, valName1, valName2, testValue ) \
    _test_reg_key( subKey1, subKey2, valName1, valName2, testValue, REG_SZ, FALSE )
#define test_reg_key_ex2_optional( subKey1, subKey2, valName1, valName2, testValue ) \
    _test_reg_key( subKey1, subKey2, valName1, valName2, testValue, REG_SZ, TRUE )
#define test_reg_key_dword( subKey, valName, testValue ) \
    _test_reg_key( subKey, NULL, valName, NULL, testValue, REG_DWORD, FALSE )

/* get a metric from the registry. If the value is negative
 * it is assumed to be in twips and converted to pixels */
static UINT metricfromreg( const char *keyname, const char *valname, int dpi)
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
static int lffromreg( const char *keyname, const char *valname, LOGFONTA *plf)
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
    if (!test_error_msg(rc,"SPI_SETBEEP")) return;
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message( SPI_SETBEEP, 0 );
    test_reg_key( SPI_SETBEEP_REGKEY,
                  SPI_SETBEEP_VALNAME,
                  curr_val ? "Yes" : "No" );
    rc=SystemParametersInfoA( SPI_GETBEEP, 0, &b, 0 );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    eq( b, curr_val, "SPI_{GET,SET}BEEP", "%d" );
    rc=SystemParametersInfoW( SPI_GETBEEP, 0, &b, 0 );
    if (rc || GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(rc, "SystemParametersInfoW: rc=%d err=%ld\n", rc, GetLastError());
        eq( b, curr_val, "SystemParametersInfoW", "%d" );
    }

    /* is a message sent for the second change? */
    rc=SystemParametersInfoA( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message( SPI_SETBEEP, 0 );

    curr_val = FALSE;
    rc=SystemParametersInfoW( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    if (rc == FALSE && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        rc=SystemParametersInfoA( SPI_SETBEEP, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc, "SystemParametersInfo: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message( SPI_SETBEEP, 0 );
    test_reg_key( SPI_SETBEEP_REGKEY,
                  SPI_SETBEEP_VALNAME,
                  curr_val ? "Yes" : "No" );
    rc=SystemParametersInfoA( SPI_GETBEEP, 0, &b, 0 );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    eq( b, curr_val, "SPI_{GET,SET}BEEP", "%d" );
    rc=SystemParametersInfoW( SPI_GETBEEP, 0, &b, 0 );
    if (rc || GetLastError() != ERROR_CALL_NOT_IMPLEMENTED)
    {
        ok(rc, "SystemParametersInfoW: rc=%d err=%ld\n", rc, GetLastError());
        eq( b, curr_val, "SystemParametersInfoW", "%d" );
    }
    ok( MessageBeep( MB_OK ), "Return value of MessageBeep when sound is disabled\n" );

    rc=SystemParametersInfoA( SPI_SETBEEP, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
}

static const char *setmouse_valuenames[3] = {
    SPI_SETMOUSE_VALNAME1,
    SPI_SETMOUSE_VALNAME2,
    SPI_SETMOUSE_VALNAME3
};

/*
 * Runs check for one setting of spi_setmouse.
 */
static BOOL run_spi_setmouse_test( int curr_val[], POINT *req_change, POINT *proj_change, int nchange )
{
    BOOL rc;
    INT mi[3];
    static int aw_turn = 0;

    char buf[20];
    int i;

    aw_turn++;
    rc = FALSE;
    SetLastError(0xdeadbeef);
    if (aw_turn % 2)  /* call unicode on odd (non even) calls */
        rc=SystemParametersInfoW( SPI_SETMOUSE, 0, curr_val, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    else
        rc=SystemParametersInfoA( SPI_SETMOUSE, 0, curr_val, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    if (!test_error_msg(rc,"SPI_SETMOUSE")) return FALSE;

    ok(rc, "SystemParametersInfo: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message( SPI_SETMOUSE, 0 );
    for (i = 0; i < 3; i++)
    {
        sprintf( buf, "%d", curr_val[i] );
        test_reg_key( SPI_SETMOUSE_REGKEY, setmouse_valuenames[i], buf );
    }

    rc=SystemParametersInfoA( SPI_GETMOUSE, 0, mi, 0 );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    ok(mi[0] == curr_val[0], "expected X threshold %d, got %d\n", curr_val[0], mi[0]);
    ok(mi[1] == curr_val[1], "expected Y threshold %d, got %d\n", curr_val[1], mi[1]);
    ok(mi[2] == curr_val[2] || broken(mi[2] == 1) /* Win10 1709+ */,
            "expected acceleration %d, got %d\n", curr_val[2], mi[2]);

    rc=SystemParametersInfoW( SPI_GETMOUSE, 0, mi, 0 );
    ok(rc, "SystemParametersInfoW: rc=%d err=%ld\n", rc, GetLastError());
    ok(mi[0] == curr_val[0], "expected X threshold %d, got %d\n", curr_val[0], mi[0]);
    ok(mi[1] == curr_val[1], "expected Y threshold %d, got %d\n", curr_val[1], mi[1]);
    ok(mi[2] == curr_val[2] || broken(mi[2] == 1) /* Win10 1709+ */,
            "expected acceleration %d, got %d\n", curr_val[2], mi[2]);

    if (0)
    {
    /* FIXME: this always fails for me  - AJ */
    for (i = 0; i < nchange; i++)
    {
        POINT mv;
        mouse_event( MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 0, 0, 0, 0 );
        mouse_event( MOUSEEVENTF_MOVE, req_change[i].x, req_change[i].y, 0, 0 );
        GetCursorPos( &mv );
        ok( proj_change[i].x == mv.x, "Projected dx and real dx comparison. May fail under high load.\n" );
        ok( proj_change[i].y == mv.y, "Projected dy equals real dy. May fail under high load.\n" );
    }
    }
    return TRUE;
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

    int nchange = ARRAY_SIZE(req_change);

    trace("testing SPI_{GET,SET}MOUSE\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETMOUSE, 0, old_mi, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}MOUSE"))
        return;

    if (!run_spi_setmouse_test( curr_val, req_change, proj_change1, nchange )) return;

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
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
}

static BOOL test_setborder(UINT curr_val, int usesetborder, int dpi)
{
    BOOL rc;
    UINT border, regval;
    INT frame;
    NONCLIENTMETRICSA ncm;

    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    if( usesetborder) {
        rc=SystemParametersInfoA( SPI_SETBORDER, curr_val, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETBORDER")) return FALSE;
        ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
        test_change_message( SPI_SETBORDER, 1 );
    } else { /* set non client metrics */
        ncm.iBorderWidth = curr_val;
        rc=SystemParametersInfoA( SPI_SETNONCLIENTMETRICS, 0, &ncm, SPIF_UPDATEINIFILE|
                SPIF_SENDCHANGE);
        if (!test_error_msg(rc,"SPI_SETNONCLIENTMETRICS")) return FALSE;
        ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
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
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    eq( (UINT)ncm.iBorderWidth, curr_val, "NonClientMetric.iBorderWidth", "%d");
    /* and from SPI_GETBORDER */ 
    rc=SystemParametersInfoA( SPI_GETBORDER, 0, &border, 0 );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    eq( border, curr_val, "SPI_{GET,SET}BORDER", "%d");
    /* test some SystemMetrics */
    frame = curr_val + GetSystemMetrics( SM_CXDLGFRAME );
    eq( frame, GetSystemMetrics( SM_CXFRAME ), "SM_CXFRAME", "%d" );
    eq( frame, GetSystemMetrics( SM_CYFRAME ), "SM_CYFRAME", "%d" );
    eq( frame, GetSystemMetrics( SM_CXSIZEFRAME ), "SM_CXSIZEFRAME", "%d" );
    eq( frame, GetSystemMetrics( SM_CYSIZEFRAME ), "SM_CYSIZEFRAME", "%d" );
    return TRUE;
}

static void test_SPI_SETBORDER( void )                 /*      6 */
{
    BOOL rc;
    UINT old_border;
    NONCLIENTMETRICSA ncmsave;
    INT CaptionWidth,
        PaddedBorderWidth;

    ncmsave.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, 0, &ncmsave, 0);
    if( !rc) {
        win_skip("SPI_GETNONCLIENTMETRICS is not available\n");
        return;
    }
    /* CaptionWidth from the registry may have different value of iCaptionWidth
     * from the non client metrics (observed on WinXP).
     * Fix this so we can safely restore settings with the nonclientmetrics */
    CaptionWidth = metricfromreg(
            "Control Panel\\Desktop\\WindowMetrics","CaptionWidth", dpi);
    ncmsave.iCaptionWidth = CaptionWidth;

    trace("testing SPI_{GET,SET}BORDER\n");

    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETBORDER, 0, &old_border, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}BORDER"))
        return;
    /* FIXME: include new PaddedBorderWidth parameter */
    PaddedBorderWidth = ncmsave.iBorderWidth - old_border;
    if( PaddedBorderWidth){
        win_skip( "Cannot reliably restore border width yet (PaddedBorderWidth = %d)\n",
                PaddedBorderWidth);
        return;
    }
    /* This will restore sane values if the test hang previous run. */
    if ( old_border == 7 || old_border == 20 )
        old_border = 1;

    /* win2k3 fails if you set the same border twice, or if size is 0 */
    if (!test_setborder(2,  1, dpi)) return;
    test_setborder(1,  1, dpi);
    test_setborder(3,  1, dpi);
    if (!test_setborder(1, 0, dpi)) return;
    test_setborder(0, 0, dpi);
    test_setborder(3, 0, dpi);

    rc=SystemParametersInfoA( SPI_SETNONCLIENTMETRICS, 0, &ncmsave,
            SPIF_UPDATEINIFILE| SPIF_SENDCHANGE);
    test_change_message( SPI_SETNONCLIENTMETRICS, 1 );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETKEYBOARDSPEED, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETKEYBOARDSPEED")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETKEYBOARDSPEED, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETKEYBOARDSPEED_REGKEY, SPI_SETKEYBOARDSPEED_VALNAME, buf );

        rc=SystemParametersInfoA( SPI_GETKEYBOARDSPEED, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}KEYBOARDSPEED", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETKEYBOARDSPEED, old_speed, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
}

/* test_SPI_ICONHORIZONTALSPACING helper */
static BOOL dotest_spi_iconhorizontalspacing( INT curr_val)
{
    BOOL rc;
    INT spacing, regval, min_val = MulDiv( 32, dpi, USER_DEFAULT_SCREEN_DPI );
    ICONMETRICSA im;

    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    if (!test_error_msg(rc,"SPI_ICONHORIZONTALSPACING")) return FALSE;
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message( SPI_ICONHORIZONTALSPACING, 0 );
    curr_val = max( curr_val, min_val );
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
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    eq( spacing, curr_val, "ICONHORIZONTALSPACING", "%d");
    /* and with a system metrics */
    eq( GetSystemMetrics( SM_CXICONSPACING ), curr_val, "SM_CXICONSPACING", "%d" );
    /* and with what SPI_GETICONMETRICS returns */
    im.cbSize = sizeof(ICONMETRICSA);
    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    eq( im.iHorzSpacing, curr_val, "SPI_GETICONMETRICS", "%d" );
    return TRUE;
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
    if (!dotest_spi_iconhorizontalspacing( old_spacing - 1)) return;
    dotest_spi_iconhorizontalspacing( 10); /* minimum is 32 */
    /* restore */
    rc=SystemParametersInfoA( SPI_ICONHORIZONTALSPACING, old_spacing, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETSCREENSAVETIMEOUT, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETSCREENSAVETIMEOUT")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETSCREENSAVETIMEOUT, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETSCREENSAVETIMEOUT_REGKEY,
                      SPI_SETSCREENSAVETIMEOUT_VALNAME, buf );

        rc = SystemParametersInfoA( SPI_GETSCREENSAVETIMEOUT, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}SCREENSAVETIMEOUT", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSCREENSAVETIMEOUT, old_timeout, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETSCREENSAVEACTIVE, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETSCREENSAVEACTIVE")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETSCREENSAVEACTIVE, 0 );
        test_reg_key( SPI_SETSCREENSAVEACTIVE_REGKEY,
                      SPI_SETSCREENSAVEACTIVE_VALNAME,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETSCREENSAVEACTIVE, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        ok(v == vals[i] || broken(! v) /* Win 7 */,
           "SPI_{GET,SET}SCREENSAVEACTIVE: got %d instead of %d\n", v, vals[i]);
    }

    rc=SystemParametersInfoA( SPI_SETSCREENSAVEACTIVE, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT delay;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETKEYBOARDDELAY, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETKEYBOARDDELAY")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETKEYBOARDDELAY, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETKEYBOARDDELAY_REGKEY,
                      SPI_SETKEYBOARDDELAY_VALNAME, buf );

        rc=SystemParametersInfoA( SPI_GETKEYBOARDDELAY, 0, &delay, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( delay, vals[i], "SPI_{GET,SET}KEYBOARDDELAY", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETKEYBOARDDELAY, old_delay, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
}


/* test_SPI_ICONVERTICALSPACING helper */
static BOOL dotest_spi_iconverticalspacing( INT curr_val)
{
    BOOL rc;
    INT spacing, regval, min_val = MulDiv( 32, dpi, USER_DEFAULT_SCREEN_DPI );
    ICONMETRICSA im;

    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, curr_val, 0,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    if (!test_error_msg(rc,"SPI_ICONVERTICALSPACING")) return FALSE;
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message( SPI_ICONVERTICALSPACING, 0 );
    curr_val = max( curr_val, min_val );
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
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    eq( spacing, curr_val, "ICONVERTICALSPACING", "%d" );
    /* and with a system metrics */
    eq( GetSystemMetrics( SM_CYICONSPACING ), curr_val, "SM_CYICONSPACING", "%d" );
    /* and with what SPI_GETICONMETRICS returns */
    im.cbSize = sizeof(ICONMETRICSA);
    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    eq( im.iVertSpacing, curr_val, "SPI_GETICONMETRICS", "%d" );
    return TRUE;
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
    if (!dotest_spi_iconverticalspacing( old_spacing - 1)) return;
    /* same tests with a value less than the minimum 32 */
    dotest_spi_iconverticalspacing( 10);
    /* restore */
    rc=SystemParametersInfoA( SPI_ICONVERTICALSPACING, old_spacing, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
}

static void test_SPI_SETICONTITLEWRAP( void )          /*     26 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;
    ICONMETRICSA im;

    trace("testing SPI_{GET,SET}ICONTITLEWRAP\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}ICONTITLEWRAP"))
        return;

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        UINT regval;

        rc=SystemParametersInfoA( SPI_SETICONTITLEWRAP, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETICONTITLEWRAP")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETICONTITLEWRAP, 1 );
        regval = metricfromreg( SPI_SETICONTITLEWRAP_REGKEY2,
                SPI_SETICONTITLEWRAP_VALNAME, dpi);
        if( regval != vals[i])
            regval = metricfromreg( SPI_SETICONTITLEWRAP_REGKEY1,
                    SPI_SETICONTITLEWRAP_VALNAME, dpi);
        ok( regval == vals[i], "wrong value in registry %d, expected %d\n", regval, vals[i] );

        rc=SystemParametersInfoA( SPI_GETICONTITLEWRAP, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}ICONTITLEWRAP", "%d" );
        /* and test with what SPI_GETICONMETRICS returns */
        im.cbSize = sizeof(ICONMETRICSA);
        rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im, FALSE );
        ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
        eq( im.iTitleWrap, (BOOL)vals[i], "SPI_GETICONMETRICS", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETICONTITLEWRAP, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETMENUDROPALIGNMENT, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETMENUDROPALIGNMENT")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETMENUDROPALIGNMENT, 0 );
        test_reg_key_ex( SPI_SETMENUDROPALIGNMENT_REGKEY1,
                         SPI_SETMENUDROPALIGNMENT_REGKEY2,
                         SPI_SETMENUDROPALIGNMENT_VALNAME,
                         vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETMENUDROPALIGNMENT, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MENUDROPALIGNMENT", "%d" );
        eq( GetSystemMetrics( SM_MENUDROPALIGNMENT ), (int)vals[i],
            "SM_MENUDROPALIGNMENT", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMENUDROPALIGNMENT, old_b, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
}

static void test_SPI_SETDOUBLECLKWIDTH( void )         /*     29 */
{
    BOOL rc;
    INT old_width;
    const UINT vals[]={0,10000};
    unsigned int i;

    trace("testing SPI_{GET,SET}DOUBLECLKWIDTH\n");
    old_width = GetSystemMetrics( SM_CXDOUBLECLK );

    for (i=0;i<ARRAY_SIZE(vals);i++)
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
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
}

static void test_SPI_SETDOUBLECLKHEIGHT( void )        /*     30 */
{
    BOOL rc;
    INT old_height;
    const UINT vals[]={0,10000};
    unsigned int i;

    trace("testing SPI_{GET,SET}DOUBLECLKHEIGHT\n");
    old_height = GetSystemMetrics( SM_CYDOUBLECLK );

    for (i=0;i<ARRAY_SIZE(vals);i++)
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
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
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
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
}

static void test_SPI_SETMOUSEBUTTONSWAP( void )        /*     33 */
{
    BOOL rc;
    BOOL vals[2];
    unsigned int i;

    trace("testing SPI_{GET,SET}MOUSEBUTTONSWAP\n");
    vals[1] = GetSystemMetrics( SM_SWAPBUTTON );
    vals[0] = !vals[1];

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        SetLastError(0xdeadbeef);
        rc=SystemParametersInfoA( SPI_SETMOUSEBUTTONSWAP, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETMOUSEBUTTONSWAP")) break;

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
        test_change_message( SPI_SETMOUSEBUTTONSWAP, 1 );
    }

    rc=SystemParametersInfoA( SPI_SETMOUSEBUTTONSWAP, vals[1], 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETDRAGFULLWINDOWS, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETDRAGFULLWINDOWS")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETDRAGFULLWINDOWS, 0 );
        test_reg_key( SPI_SETDRAGFULLWINDOWS_REGKEY,
                      SPI_SETDRAGFULLWINDOWS_VALNAME,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETDRAGFULLWINDOWS, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}DRAGFULLWINDOWS", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETDRAGFULLWINDOWS, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
}

#define test_reg_metric( KEY, VAL, val) do { \
    INT regval;\
    regval = metricfromreg( KEY, VAL, dpi);\
    ok( regval==val, "wrong value \"%s\" in registry %d, expected %d\n", VAL, regval, val);\
} while(0)

#define test_reg_metric2( KEY1, KEY2, VAL, val) do { \
    INT regval;\
    regval = metricfromreg( KEY1, VAL, dpi);\
    if( regval != val) regval = metricfromreg( KEY2, VAL, dpi);\
    ok( regval==val, "wrong value \"%s\" in registry %d, expected %d\n", VAL, regval, val);\
} while(0)

#define test_reg_font( KEY, VAL, LF) do { \
    LOGFONTA lfreg;\
    lffromreg( KEY, VAL, &lfreg);\
    ok( (lfreg.lfHeight < 0 ? (LF).lfHeight == MulDiv( lfreg.lfHeight, dpi, real_dpi ) : \
                MulDiv( -(LF).lfHeight , 72, dpi) == lfreg.lfHeight )&&\
        (LF).lfWidth == lfreg.lfWidth &&\
        (LF).lfWeight == lfreg.lfWeight &&\
        !strcmp( (LF).lfFaceName, lfreg.lfFaceName)\
        , "wrong value \"%s\" in registry %ld, %ld\n", VAL, (LF).lfHeight, lfreg.lfHeight);\
} while(0)

#define TEST_NONCLIENTMETRICS_REG( ncm) do { \
/*FIXME: test_reg_metric2( SPI_SETBORDER_REGKEY2, SPI_SETBORDER_REGKEY, SPI_SETBORDER_VALNAME, (ncm).iBorderWidth);*/\
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
test_reg_font( SPI_METRIC_REGKEY, SPI_MESSAGEFONT_VALNAME, (ncm).lfMessageFont); } while(0)

/* get text metric height value for the specified logfont */
static int get_tmheight( LOGFONTA *plf, int flag)
{
    TEXTMETRICA tm;
    HDC hdc = GetDC(0);
    HFONT hfont = CreateFontIndirectA( plf);
    hfont = SelectObject( hdc, hfont);
    GetTextMetricsA( hdc, &tm);
    hfont = SelectObject( hdc, hfont);
    ReleaseDC( 0, hdc );
    return tm.tmHeight + (flag ? tm.tmExternalLeading : 0);
}

static int get_tmheightW( LOGFONTW *plf, int flag)
{
    TEXTMETRICW tm;
    HDC hdc = GetDC(0);
    HFONT hfont = CreateFontIndirectW( plf);
    hfont = SelectObject( hdc, hfont);
    GetTextMetricsW( hdc, &tm);
    hfont = SelectObject( hdc, hfont);
    ReleaseDC( 0, hdc );
    return tm.tmHeight + (flag ? tm.tmExternalLeading : 0);
}

static void test_GetSystemMetrics( void);
static UINT smcxsmsize = 999999999;

static void test_SPI_SETNONCLIENTMETRICS( void )               /*     44 */
{
    BOOL rc;
    INT expect;
    NONCLIENTMETRICSA Ncmorig;
    NONCLIENTMETRICSA Ncmnew;
    NONCLIENTMETRICSA Ncmcur;
    NONCLIENTMETRICSA Ncmstart;

    Ncmorig.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);
    Ncmnew.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);
    Ncmcur.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);
    Ncmstart.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);

    trace("testing SPI_{GET,SET}NONCLIENTMETRICS\n");
    change_counter = 0;
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &Ncmorig, FALSE );
    if (!test_error_msg(rc,"SPI_{GET,SET}NONCLIENTMETRICS"))
        return;
    Ncmstart = Ncmorig;
    smcxsmsize = Ncmstart.iSmCaptionWidth;
    /* SPI_GETNONCLIENTMETRICS returns some "cooked" values. For instance if 
       the caption font height is higher than the CaptionHeight field,
       the latter is adjusted accordingly. To be able to restore these setting
       accurately we restore the raw values. */
    Ncmorig.iCaptionWidth = metricfromreg( SPI_METRIC_REGKEY, SPI_CAPTIONWIDTH_VALNAME, real_dpi);
    Ncmorig.iCaptionHeight = metricfromreg( SPI_METRIC_REGKEY, SPI_CAPTIONHEIGHT_VALNAME, dpi);
    Ncmorig.iSmCaptionHeight = metricfromreg( SPI_METRIC_REGKEY, SPI_SMCAPTIONHEIGHT_VALNAME, dpi);
    Ncmorig.iMenuHeight = metricfromreg( SPI_METRIC_REGKEY, SPI_MENUHEIGHT_VALNAME, dpi);
    /* test registry entries */
    TEST_NONCLIENTMETRICS_REG( Ncmorig);

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
    if (!test_error_msg(rc,"SPI_SETNONCLIENTMETRICS")) return;
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message( SPI_SETNONCLIENTMETRICS, 1 );
    /* get them back */
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &Ncmcur, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    /* test registry entries */
    TEST_NONCLIENTMETRICS_REG( Ncmcur );
    /* test the system metrics with these settings */
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
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message( SPI_SETNONCLIENTMETRICS, 1 );
    /* raw values are in registry */
    TEST_NONCLIENTMETRICS_REG( Ncmnew );
    /* get them back */
    rc=SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA), &Ncmcur, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
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

    /* iCaptionWidth depends on a version, could be 8, 12 (Vista, Win7), 13 */
    ok( (Ncmcur.iCaptionWidth >= 8 && Ncmcur.iCaptionWidth <= 13) ||
        Ncmcur.iCaptionWidth == Ncmstart.iCaptionWidth, /* with windows XP theme,  the value never changes */
        "CaptionWidth: %d expected from [8, 13] or %d\n", Ncmcur.iCaptionWidth, Ncmstart.iCaptionWidth);
    ok( Ncmcur.iScrollWidth == 8,
        "ScrollWidth: %d expected 8\n", Ncmcur.iScrollWidth);
    ok( Ncmcur.iScrollHeight == 8,
        "ScrollHeight: %d expected 8\n", Ncmcur.iScrollHeight);
    /* test the system metrics with these settings */
    test_GetSystemMetrics();
    /* restore */
    rc=SystemParametersInfoA( SPI_SETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSA),
        &Ncmorig, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    test_change_message( SPI_SETNONCLIENTMETRICS, 0 );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
    /* test the system metrics with these settings */
    test_GetSystemMetrics();
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
    /* Test registry. Note that it is perfectly valid for some fields to
     * not be set.
     */
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINWIDTH_VALNAME, dpi);
    ok( regval == -1 || regval == lpMm_orig.iWidth, "wrong value in registry %d, expected %d\n",
        regval, lpMm_orig.iWidth);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINHORZGAP_VALNAME, dpi);
    ok( regval == -1 || regval == lpMm_orig.iHorzGap, "wrong value in registry %d, expected %d\n",
        regval, lpMm_orig.iHorzGap);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINVERTGAP_VALNAME, dpi);
    ok( regval == -1 || regval == lpMm_orig.iVertGap, "wrong value in registry %d, expected %d\n",
        regval, lpMm_orig.iVertGap);
    regval = metricfromreg( SPI_MINIMIZEDMETRICS_REGKEY, SPI_MINARRANGE_VALNAME, dpi);
    ok( regval == -1 || regval == lpMm_orig.iArrange, "wrong value in registry %d, expected %d\n",
        regval, lpMm_orig.iArrange);
    /* set some new values */
    lpMm_cur.iWidth = 180;
    lpMm_cur.iHorzGap = 1;
    lpMm_cur.iVertGap = 1;
    lpMm_cur.iArrange = 5;
    rc=SystemParametersInfoA( SPI_SETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS),
        &lpMm_cur, SPIF_UPDATEINIFILE );
    if (!test_error_msg(rc,"SPI_SETMINIMIZEDMETRICS")) return;
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    /* read them back */
    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_new, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
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
    /* now some really invalid settings */
    lpMm_cur.iWidth = -1;
    lpMm_cur.iHorzGap = -1;
    lpMm_cur.iVertGap = -1;
    lpMm_cur.iArrange = - 1;
    rc=SystemParametersInfoA( SPI_SETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS),
        &lpMm_cur, SPIF_UPDATEINIFILE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    /* read back */
    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_new, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    /* the width and H/V gaps have minimum 0, arrange is and'd with 0xf */
    eq( lpMm_new.iWidth,   0,   "iWidth",   "%d" );
    eq( lpMm_new.iHorzGap, 0, "iHorzGap", "%d" );
    eq( lpMm_new.iVertGap, 0, "iVertGap", "%d" );
    eq( lpMm_new.iArrange, 0xf & lpMm_cur.iArrange, "iArrange", "%d" );
    /* test registry */
    if (0)
    {
    /* FIXME: cannot understand the results of this (11, 11, 11, 0) */
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
    }
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
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
    /* check that */
    rc=SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, sizeof(MINIMIZEDMETRICS), &lpMm_new, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
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
    ok( regval==im_orig.iHorzSpacing,
        "wrong value in registry %d, expected %d\n", regval, im_orig.iHorzSpacing);
    regval = metricfromreg( SPI_ICONVERTICALSPACING_REGKEY, SPI_ICONVERTICALSPACING_VALNAME, dpi);
    ok( regval==im_orig.iVertSpacing,
        "wrong value in registry %d, expected %d\n", regval, im_orig.iVertSpacing);
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
    if (!test_error_msg(rc,"SPI_SETICONMETRICS")) return;
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());

    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im_new, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
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
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();

    rc=SystemParametersInfoA( SPI_GETICONMETRICS, sizeof(ICONMETRICSA), &im_new, FALSE );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());

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
    SetRect(&curr_val, old_area.left, old_area.top, old_area.right - 1, old_area.bottom - 1);
    rc=SystemParametersInfoA( SPI_SETWORKAREA, 0, &curr_val,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    if (!test_error_msg(rc,"SPI_SETWORKAREA")) return;
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    rc=SystemParametersInfoA( SPI_GETWORKAREA, 0, &area, 0 );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    if( !EqualRect( &area, &curr_val)) /* no message if rect has not changed */
        test_change_message( SPI_SETWORKAREA, 0);
    eq( area.left,   curr_val.left,   "left",   "%ld" );
    eq( area.top,    curr_val.top,    "top",    "%ld" );
    /* size may be rounded */
    ok( area.right >= curr_val.right - 16 && area.right < curr_val.right + 16,
        "right: got %ld instead of %ld\n", area.right, curr_val.right );
    ok( area.bottom >= curr_val.bottom - 16 && area.bottom < curr_val.bottom + 16,
        "bottom: got %ld instead of %ld\n", area.bottom, curr_val.bottom );
    curr_val = area;
    rc=SystemParametersInfoA( SPI_SETWORKAREA, 0, &old_area,
                              SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    rc=SystemParametersInfoA( SPI_GETWORKAREA, 0, &area, 0 );
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    if( !EqualRect( &area, &curr_val)) /* no message if rect has not changed */
        test_change_message( SPI_SETWORKAREA, 0 );
    eq( area.left,   old_area.left,   "left",   "%ld" );
    eq( area.top,    old_area.top,    "top",    "%ld" );
    /* size may be rounded */
    ok( area.right >= old_area.right - 16 && area.right < old_area.right + 16,
        "right: got %ld instead of %ld\n", area.right, old_area.right );
    ok( area.bottom >= old_area.bottom - 16 && area.bottom < old_area.bottom + 16,
        "bottom: got %ld instead of %ld\n", area.bottom, old_area.bottom );
    flush_change_messages();
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
    if (!test_error_msg(rc,"SPI_{GET,SET}SHOWSOUNDS"))
        return;

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETSHOWSOUNDS, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETSHOWSOUNDS")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETSHOWSOUNDS, 1 );
        test_reg_key( SPI_SETSHOWSOUNDS_REGKEY,
                      SPI_SETSHOWSOUNDS_VALNAME,
                      vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETSHOWSOUNDS, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_GETSHOWSOUNDS", "%d" );
        eq( GetSystemMetrics( SM_SHOWSOUNDS ), (int)vals[i],
            "SM_SHOWSOUNDS", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSHOWSOUNDS, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
}

static void test_SPI_SETKEYBOARDPREF( void )           /*     69 */
{
    BOOL rc;
    BOOL vals[2];
    unsigned int i;

    trace("testing SPI_{GET,SET}KEYBOARDPREF\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETKEYBOARDPREF, 0, &vals[1], 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}KEYBOARDPREF"))
        return;
    vals[0] = !vals[1];

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        BOOL v;

        rc=SystemParametersInfoA( SPI_SETKEYBOARDPREF, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETKEYBOARDPREF")) break;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETKEYBOARDPREF, 1 );
        test_reg_key_ex2( SPI_SETKEYBOARDPREF_REGKEY, SPI_SETKEYBOARDPREF_REGKEY_LEGACY,
                          SPI_SETKEYBOARDPREF_VALNAME, SPI_SETKEYBOARDPREF_VALNAME_LEGACY,
                          vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETKEYBOARDPREF, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, (BOOL)vals[i], "SPI_GETKEYBOARDPREF", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETKEYBOARDPREF, vals[1], 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        BOOL v;

        rc=SystemParametersInfoA( SPI_SETSCREENREADER, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETSCREENREADER")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETSCREENREADER, 1 );
        test_reg_key_ex2_optional( SPI_SETSCREENREADER_REGKEY, SPI_SETSCREENREADER_REGKEY_LEGACY,
                                   SPI_SETSCREENREADER_VALNAME, SPI_SETSCREENREADER_VALNAME_LEGACY,
                                   vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETSCREENREADER, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, (BOOL)vals[i], "SPI_GETSCREENREADER", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSCREENREADER, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
}

static void test_SPI_SETFONTSMOOTHING( void )         /*     75 */
{
    BOOL rc;
    BOOL old_b;
    DWORD old_type, old_contrast, old_orient;
    const UINT vals[]={0xffffffff,0,1,2};
    unsigned int i;

    trace("testing SPI_{GET,SET}FONTSMOOTHING\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETFONTSMOOTHING, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_{GET,SET}FONTSMOOTHING"))
        return;
    SystemParametersInfoA( SPI_GETFONTSMOOTHINGTYPE, 0, &old_type, 0 );
    SystemParametersInfoA( SPI_GETFONTSMOOTHINGCONTRAST, 0, &old_contrast, 0 );
    SystemParametersInfoA( SPI_GETFONTSMOOTHINGORIENTATION, 0, &old_orient, 0 );

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETFONTSMOOTHING, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETFONTSMOOTHING")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETFONTSMOOTHING, 0 );
        test_reg_key( SPI_SETFONTSMOOTHING_REGKEY,
                      SPI_SETFONTSMOOTHING_VALNAME,
                      vals[i] ? "2" : "0" );

        rc=SystemParametersInfoA( SPI_SETFONTSMOOTHINGTYPE, 0, UlongToPtr(vals[i]),
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETFONTSMOOTHINGTYPE")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETFONTSMOOTHINGTYPE, 0 );
        test_reg_key_dword( SPI_SETFONTSMOOTHING_REGKEY,
                            SPI_SETFONTSMOOTHINGTYPE_VALNAME, &vals[i] );

        rc=SystemParametersInfoA( SPI_SETFONTSMOOTHINGCONTRAST, 0, UlongToPtr(vals[i]),
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETFONTSMOOTHINGCONTRAST")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETFONTSMOOTHINGCONTRAST, 0 );
        test_reg_key_dword( SPI_SETFONTSMOOTHING_REGKEY,
                            SPI_SETFONTSMOOTHINGCONTRAST_VALNAME, &vals[i] );

        rc=SystemParametersInfoA( SPI_SETFONTSMOOTHINGORIENTATION, 0, UlongToPtr(vals[i]),
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETFONTSMOOTHINGORIENTATION")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETFONTSMOOTHINGORIENTATION, 0 );
        test_reg_key_dword( SPI_SETFONTSMOOTHING_REGKEY,
                            SPI_SETFONTSMOOTHINGORIENTATION_VALNAME, &vals[i] );

        rc=SystemParametersInfoA( SPI_GETFONTSMOOTHING, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i] ? 1 : 0, "SPI_GETFONTSMOOTHING", "%d" );

        rc=SystemParametersInfoA( SPI_GETFONTSMOOTHINGTYPE, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        ok( v == vals[i], "wrong value %x/%x\n", v, vals[i] );

        rc=SystemParametersInfoA( SPI_GETFONTSMOOTHINGCONTRAST, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        ok( v == vals[i], "wrong value %x/%x\n", v, vals[i] );

        rc=SystemParametersInfoA( SPI_GETFONTSMOOTHINGORIENTATION, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        ok( v == vals[i], "wrong value %x/%x\n", v, vals[i] );
    }

    rc=SystemParametersInfoA( SPI_SETFONTSMOOTHING, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    rc=SystemParametersInfoA( SPI_SETFONTSMOOTHINGTYPE, old_type, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    rc=SystemParametersInfoA( SPI_SETFONTSMOOTHINGCONTRAST, old_contrast, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    rc=SystemParametersInfoA( SPI_SETFONTSMOOTHINGORIENTATION, old_orient, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETLOWPOWERACTIVE, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETLOWPOWERACTIVE")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETLOWPOWERACTIVE, 1 );
        test_reg_key_optional( SPI_SETLOWPOWERACTIVE_REGKEY,
                               SPI_SETLOWPOWERACTIVE_VALNAME,
                               vals[i] ? "1" : "0" );

        /* SPI_SETLOWPOWERACTIVE is not persistent in win2k3 and above */
        v = 0xdeadbeef;
        rc=SystemParametersInfoA( SPI_GETLOWPOWERACTIVE, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        ok(v == vals[i] || v == 0, /* win2k3 */
           "SPI_GETLOWPOWERACTIVE: got %d instead of 0 or %d\n", v, vals[i]);
    }

    rc=SystemParametersInfoA( SPI_SETLOWPOWERACTIVE, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETPOWEROFFACTIVE, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETPOWEROFFACTIVE")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETPOWEROFFACTIVE, 1 );
        test_reg_key_optional( SPI_SETPOWEROFFACTIVE_REGKEY,
                               SPI_SETPOWEROFFACTIVE_VALNAME,
                               vals[i] ? "1" : "0" );

        /* SPI_SETPOWEROFFACTIVE is not persistent in win2k3 and above */
        v = 0xdeadbeef;
        rc=SystemParametersInfoA( SPI_GETPOWEROFFACTIVE, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        ok(v == vals[i] || v == 0, /* win2k3 */
           "SPI_GETPOWEROFFACTIVE: got %d instead of 0 or %d\n", v, vals[i]);
    }

    rc=SystemParametersInfoA( SPI_SETPOWEROFFACTIVE, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
}

static void test_SPI_SETSNAPTODEFBUTTON( void )         /*     95 */
{
    BOOL rc;
    BOOL old_b;
    const UINT vals[]={TRUE,FALSE};
    unsigned int i;

    trace("testing SPI_{GET,SET}SNAPTODEFBUTTON\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETSNAPTODEFBUTTON, 0, &old_b, 0 );
    if (!test_error_msg(rc,"SPI_GETSNAPTODEFBUTTON"))
        return;

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;

        rc=SystemParametersInfoA( SPI_SETSNAPTODEFBUTTON, vals[i], 0,
                                  SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETSNAPTODEFBUTTON")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETSNAPTODEFBUTTON, 0 );
        test_reg_key_optional( SPI_SETSNAPTODEFBUTTON_REGKEY,
                               SPI_SETSNAPTODEFBUTTON_VALNAME,
                               vals[i] ? "1" : "0" );

        rc=SystemParametersInfoA( SPI_GETSNAPTODEFBUTTON, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_GETSNAPTODEFBUTTON", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETSNAPTODEFBUTTON, old_b, 0, SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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
    if (!test_error_msg(rc,"SPI_{GET,SET}MOUSEHOVERWIDTH"))
        return;

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETMOUSEHOVERWIDTH, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETMOUSEHOVERWIDTH")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETMOUSEHOVERWIDTH, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSEHOVERWIDTH_REGKEY,
                      SPI_SETMOUSEHOVERWIDTH_VALNAME, buf );

        SystemParametersInfoA( SPI_GETMOUSEHOVERWIDTH, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MOUSEHOVERWIDTH", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMOUSEHOVERWIDTH, old_width, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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
    if (!test_error_msg(rc,"SPI_{GET,SET}MOUSEHOVERHEIGHT"))
        return;

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETMOUSEHOVERHEIGHT, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETMOUSEHOVERHEIGHT")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETMOUSEHOVERHEIGHT, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSEHOVERHEIGHT_REGKEY,
                      SPI_SETMOUSEHOVERHEIGHT_VALNAME, buf );

        SystemParametersInfoA( SPI_GETMOUSEHOVERHEIGHT, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MOUSEHOVERHEIGHT", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMOUSEHOVERHEIGHT, old_height, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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
    if (!test_error_msg(rc,"SPI_{GET,SET}MOUSEHOVERTIME"))
        return;

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETMOUSEHOVERTIME, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETMOUSEHOVERTIME")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETMOUSEHOVERTIME, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSEHOVERTIME_REGKEY,
                      SPI_SETMOUSEHOVERTIME_VALNAME, buf );

        SystemParametersInfoA( SPI_GETMOUSEHOVERTIME, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MOUSEHOVERTIME", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMOUSEHOVERTIME, old_time, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETWHEELSCROLLLINES, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETWHEELSCROLLLINES")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETWHEELSCROLLLINES, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSESCROLLLINES_REGKEY,
                      SPI_SETMOUSESCROLLLINES_VALNAME, buf );

        SystemParametersInfoA( SPI_GETWHEELSCROLLLINES, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}WHEELSCROLLLINES", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETWHEELSCROLLLINES, old_lines, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
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

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETMENUSHOWDELAY, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETMENUSHOWDELAY")) return;
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        test_change_message( SPI_SETMENUSHOWDELAY, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMENUSHOWDELAY_REGKEY,
                      SPI_SETMENUSHOWDELAY_VALNAME, buf );

        SystemParametersInfoA( SPI_GETMENUSHOWDELAY, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}MENUSHOWDELAY", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETMENUSHOWDELAY, old_delay, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
}

static void test_SPI_SETWHEELSCROLLCHARS( void )      /*     108 */
{
    BOOL rc;
    UINT old_chars;
    const UINT vals[]={32767,0};
    unsigned int i;

    trace("testing SPI_{GET,SET}WHEELSCROLLCHARS\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA( SPI_GETWHEELSCROLLCHARS, 0, &old_chars, 0 );

    /* SPI_{GET,SET}WHEELSCROLLCHARS not supported on Windows 95 */
    if (!test_error_msg(rc,"SPI_{GET,SET}WHEELSCROLLCHARS"))
        return;

    for (i=0;i<ARRAY_SIZE(vals);i++)
    {
        UINT v;
        char buf[10];

        rc=SystemParametersInfoA( SPI_SETWHEELSCROLLCHARS, vals[i], 0,
                               SPIF_UPDATEINIFILE | SPIF_SENDCHANGE );
        if (!test_error_msg(rc,"SPI_SETWHEELSCROLLCHARS")) return;
        test_change_message( SPI_SETWHEELSCROLLCHARS, 0 );
        sprintf( buf, "%d", vals[i] );
        test_reg_key( SPI_SETMOUSESCROLLCHARS_REGKEY,
                      SPI_SETMOUSESCROLLCHARS_VALNAME, buf );

        SystemParametersInfoA( SPI_GETWHEELSCROLLCHARS, 0, &v, 0 );
        ok(rc, "%d: rc=%d err=%ld\n", i, rc, GetLastError());
        eq( v, vals[i], "SPI_{GET,SET}WHEELSCROLLCHARS", "%d" );
    }

    rc=SystemParametersInfoA( SPI_SETWHEELSCROLLCHARS, old_chars, 0,
                              SPIF_UPDATEINIFILE );
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());
    flush_change_messages();
}

static void test_SPI_SETWALLPAPER( void )              /*   115 */
{
    BOOL rc;
    char oldval[260];
    char newval[260];

    trace("testing SPI_{GET,SET}DESKWALLPAPER\n");
    SetLastError(0xdeadbeef);
    rc=SystemParametersInfoA(SPI_GETDESKWALLPAPER, 260, oldval, 0);
    if (!test_error_msg(rc,"SPI_{GET,SET}DESKWALLPAPER"))
        return;

    strcpy(newval, "");
    rc=SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, newval, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    if (!test_error_msg(rc,"SPI_SETDESKWALLPAPER")) return;
    ok(rc, "SystemParametersInfoA: rc=%d err=%ld\n", rc, GetLastError());
    test_change_message(SPI_SETDESKWALLPAPER, 0);

    rc=SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, oldval, SPIF_UPDATEINIFILE);
    ok(rc, "***warning*** failed to restore the original value: rc=%d err=%ld\n", rc, GetLastError());

    test_reg_key(SPI_SETDESKWALLPAPER_REGKEY, SPI_SETDESKWALLPAPER_VALNAME, oldval);
    flush_change_messages();
}

static void test_WM_DISPLAYCHANGE(void)
{
    UINT test_bpps[] = {8, 16, 24}, default_bpp, i;
    DEVMODEW settings;
    DWORD res;
    BOOL ret;

    displaychange_test_active = TRUE;
    displaychange_sem = CreateSemaphoreW( NULL, 0, 1, NULL );

    memset( &settings, 0, sizeof(DEVMODEW) );
    settings.dmSize = sizeof(DEVMODEW);
    ret = EnumDisplaySettingsW( NULL, ENUM_CURRENT_SETTINGS, &settings );
    ok( ret, "EnumDisplaySettingsW failed, error %#lx\n", GetLastError() );

    ok( settings.dmFields & DM_BITSPERPEL, "got dmFields %#lx\n", settings.dmFields );
    ok( settings.dmBitsPerPel == 32, "got dmBitsPerPel %lu\n", settings.dmBitsPerPel );
    default_bpp = settings.dmBitsPerPel;

    /* setting default mode most of the time doesn't send WM_DISPLAYCHANGE,
     * it only does the first time ChangeDisplaySettingsExW is called */

    last_bpp = -1;
    change_counter = 0;
    displaychange_ok = TRUE;
    res = ChangeDisplaySettingsExW( NULL, &settings, NULL, 0, NULL );
    ok( !res, "ChangeDisplaySettingsExW returned %ld\n", res );
    res = WaitForSingleObject( displaychange_sem, 1000 );
    todo_wine
    ok( res == WAIT_TIMEOUT || broken( !res ), "WaitForSingleObject returned %#lx\n", res );
    todo_wine
    ok( last_bpp == -1 || broken( last_bpp == default_bpp ), "got WM_DISPLAYCHANGE bpp %d\n", last_bpp );
    displaychange_ok = FALSE;

    for (i = 0; i < ARRAY_SIZE(test_bpps); i++)
    {
        UINT bpp = test_bpps[i];

        winetest_push_context( "bpp %u", bpp );

        settings.dmBitsPerPel = bpp;
        settings.dmFields |= DM_BITSPERPEL;

        last_bpp = -1;
        change_counter = 0;
        displaychange_ok = TRUE;
        res = ChangeDisplaySettingsExW( NULL, &settings, NULL, 0, NULL );
        if (!res)
        {
            /* Wait quite long for the message, screen setting changes can take some time */
            res = WaitForSingleObject( displaychange_sem, 10000 );
            ok( !res, "WaitForSingleObject returned %#lx\n", res );
            ok( last_bpp == bpp, "got WM_DISPLAYCHANGE bpp %d\n", last_bpp );
        }
        else
        {
            todo_wine
            win_skip( "ChangeDisplaySettingsExW returned %ld\n", res );
            ok( res == DISP_CHANGE_BADMODE || broken( res == DISP_CHANGE_FAILED && bpp == 8 ),
                "ChangeDisplaySettingsExW returned %ld\n", res );
            ok( last_bpp == -1, "got WM_DISPLAYCHANGE bpp %d\n", last_bpp );
        }
        displaychange_ok = FALSE;

        winetest_pop_context();
    }

    /* restoring default mode most of the time doesn't send WM_DISPLAYCHANGE */

    settings.dmBitsPerPel = default_bpp;
    settings.dmFields |= DM_BITSPERPEL;

    last_bpp = -1;
    change_counter = 0;
    displaychange_ok = TRUE;
    res = ChangeDisplaySettingsExW( NULL, &settings, NULL, 0, NULL );
    ok( !res, "ChangeDisplaySettingsExW returned %ld\n", res );
    res = WaitForSingleObject( displaychange_sem, 1000 );
    todo_wine
    ok( res == WAIT_TIMEOUT || broken( !res ), "WaitForSingleObject returned %#lx\n", res );
    todo_wine
    ok( last_bpp == -1 || broken( last_bpp == default_bpp ), "got WM_DISPLAYCHANGE bpp %d\n", last_bpp );
    displaychange_ok = FALSE;

    CloseHandle( displaychange_sem );
    displaychange_sem = 0;
    displaychange_test_active = FALSE;
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
    /* test_WM_DISPLAYCHANGE seems to be somewhat buggy on
     * some versions of Windows (Vista, Win2k8, Win7B) in that
     * not all metrics are properly restored. Problems are
     * SM_CXMAXTRACK, SM_CYMAXTRACK
     * Fortunately setting the Non-Client metrics like in
     * test_SPI_SETNONCLIENTMETRICS will correct this. That is why
     * we do the DISPLAY change now... */
    test_WM_DISPLAYCHANGE();
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
    test_SPI_SETSNAPTODEFBUTTON();              /*     95 */
    test_SPI_SETMOUSEHOVERWIDTH();              /*     99 */
    test_SPI_SETMOUSEHOVERHEIGHT();             /*    101 */
    test_SPI_SETMOUSEHOVERTIME();               /*    103 */
    test_SPI_SETWHEELSCROLLLINES();             /*    105 */
    test_SPI_SETMENUSHOWDELAY();                /*    107 */
    test_SPI_SETWHEELSCROLLCHARS();             /*    108 */
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
static void get_text_metr_size( HDC hdc, LOGFONTA *plf, TEXTMETRICA * ptm, UINT *psz)
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

static INT CALLBACK enum_all_fonts_proc(const LOGFONTA *elf, const TEXTMETRICA *ntm, DWORD type, LPARAM lparam)
{
    return lstrcmpiA(elf->lfFaceName, (const char *)lparam);
}

static BOOL is_font_enumerated(const char *name)
{
    HDC hdc = CreateCompatibleDC(0);
    BOOL ret = FALSE;

    if (!EnumFontFamiliesA(hdc, NULL, enum_all_fonts_proc, (LPARAM)name))
        ret = TRUE;

    DeleteDC(hdc);
    return ret;
}

static int get_cursor_size( int size )
{
    /* only certain sizes are allowed for cursors */
    if (size >= 64) return 64;
    if (size >= 48) return 48;
    return 32;
}

static void test_GetSystemMetrics( void)
{
    TEXTMETRICA tmMenuFont;
    UINT IconSpacing, IconVerticalSpacing;
    BOOL rc;

    HDC hdc = CreateICA( "Display", 0, 0, 0);
    UINT avcwCaption;
    INT CaptionWidthfromreg, width, smicon, broken_val;
    MINIMIZEDMETRICS minim;
    NONCLIENTMETRICSA ncm;
    SIZE screen;

    assert(sizeof(ncm) == 344);

    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);
    rc = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(rc, "SystemParametersInfoA failed\n");

    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth) - 1;
    rc = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(!rc, "SystemParametersInfoA should fail\n");

    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth) + 1;
    SetLastError(0xdeadbeef);
    rc = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(!rc, "SystemParametersInfoA should fail\n");

    ncm.cbSize = sizeof(ncm); /* Vista added padding */
    SetLastError(0xdeadbeef);
    ncm.iPaddedBorderWidth = 0xcccc;
    rc = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(rc || broken(!rc) /* before Vista */, "SystemParametersInfoA failed\n");
    if (rc) ok( ncm.iPaddedBorderWidth == 0, "wrong iPaddedBorderWidth %u\n", ncm.iPaddedBorderWidth );

    minim.cbSize = sizeof( minim);
    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);
    SystemParametersInfoA( SPI_GETMINIMIZEDMETRICS, 0, &minim, 0);
    rc = SystemParametersInfoA( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    if( !rc) {
        win_skip("SPI_GETNONCLIENTMETRICS is not available\n");
        return;
    }

    ok(is_font_enumerated(ncm.lfCaptionFont.lfFaceName), "font %s should be enumerated\n", ncm.lfCaptionFont.lfFaceName);
    ok(is_font_enumerated(ncm.lfSmCaptionFont.lfFaceName), "font %s should be enumerated\n", ncm.lfSmCaptionFont.lfFaceName);
    ok(is_font_enumerated(ncm.lfMenuFont.lfFaceName), "font %s should be enumerated\n", ncm.lfMenuFont.lfFaceName);
    ok(is_font_enumerated(ncm.lfStatusFont.lfFaceName), "font %s should be enumerated\n", ncm.lfStatusFont.lfFaceName);
    ok(is_font_enumerated(ncm.lfMessageFont.lfFaceName), "font %s should be enumerated\n", ncm.lfMessageFont.lfFaceName);

    /* CaptionWidth from the registry may have different value of iCaptionWidth
     * from the non client metrics (observed on WinXP) */
    CaptionWidthfromreg = metricfromreg(
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
    /* These don't depend on the Shell Icon Size registry value */
    ok_gsm( SM_CXICON, MulDiv( 32, dpi, USER_DEFAULT_SCREEN_DPI ) );
    ok_gsm( SM_CYICON, MulDiv( 32, dpi, USER_DEFAULT_SCREEN_DPI ) );
    ok_gsm( SM_CXCURSOR, get_cursor_size( MulDiv( 32, dpi, USER_DEFAULT_SCREEN_DPI )));
    ok_gsm( SM_CYCURSOR, get_cursor_size( MulDiv( 32, dpi, USER_DEFAULT_SCREEN_DPI )));
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
    ok_gsm( SM_CXMIN, 3 * max( CaptionWidthfromreg >= 0 ? CaptionWidthfromreg : ncm.iCaptionWidth, 8) +
            GetSystemMetrics( SM_CYSIZE) + 4 + 4 * avcwCaption + 2 * GetSystemMetrics( SM_CXFRAME));
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
    /* sign-extension for iHorzGap/iVertGap is broken on Win9x */
    ok_gsm( SM_CXMINSPACING, GetSystemMetrics( SM_CXMINIMIZED) + (short)minim.iHorzGap );
    ok_gsm( SM_CYMINSPACING, GetSystemMetrics( SM_CYMINIMIZED) + (short)minim.iVertGap );

    smicon = MulDiv( 16, dpi, USER_DEFAULT_SCREEN_DPI );
    width = CaptionWidthfromreg > 0 ? CaptionWidthfromreg : ncm.iCaptionWidth;
    if (!pIsProcessDPIAware || pIsProcessDPIAware())
        smicon = max( min( smicon, width - 2), 4 ) & ~1;
    todo_wine_if( real_dpi == dpi && smicon != (MulDiv( 16, dpi, USER_DEFAULT_SCREEN_DPI) & ~1) )
    {
        broken_val = (min( ncm.iCaptionHeight, width ) - 2) & ~1;
        broken_val = min( broken_val, 20 );

        if (smicon == 4)
        {
            ok_gsm_2( SM_CXSMICON, smicon, 6 );
            ok_gsm_2( SM_CYSMICON, smicon, 6 );
        }
        else
        {
            ok_gsm_2( SM_CXSMICON, smicon, broken_val );
            ok_gsm_2( SM_CYSMICON, smicon, broken_val );
        }
    }

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
    screen.cx = GetSystemMetrics( SM_CXVIRTUALSCREEN );
    screen.cy = GetSystemMetrics( SM_CYVIRTUALSCREEN );
    if (!screen.cx || !screen.cy)  /* not supported on NT4 */
    {
        screen.cx = GetSystemMetrics( SM_CXSCREEN );
        screen.cy = GetSystemMetrics( SM_CYSCREEN );
    }
    ok_gsm_3( SM_CXMAXTRACK, screen.cx + 4 + 2 * GetSystemMetrics(SM_CXFRAME),
              screen.cx - 4 + 2 * GetSystemMetrics(SM_CXFRAME), /* Vista */
              screen.cx + 2 * GetSystemMetrics(SM_CXFRAME)); /* Win8 */
    ok_gsm_3( SM_CYMAXTRACK, screen.cy + 4 + 2 * GetSystemMetrics(SM_CYFRAME),
              screen.cy - 4 + 2 * GetSystemMetrics(SM_CYFRAME), /* Vista */
              screen.cy + 2 * GetSystemMetrics(SM_CYFRAME)); /* Win8 */
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
        trace( "Captionfontchar width %d  MenuFont %ld,%ld CaptionWidth from registry: %d screen %ld,%ld\n",
                avcwCaption, tmMenuFont.tmHeight, tmMenuFont.tmExternalLeading, CaptionWidthfromreg, screen.cx, screen.cy);
    }

    DeleteDC(hdc);
}

static void compare_font( const LOGFONTW *lf1, const LOGFONTW *lf2, int dpi, int custom_dpi, int line )
{
    ok_(__FILE__,line)( lf2->lfHeight == (dpi == custom_dpi) ? lf1->lfHeight : MulDiv( lf1->lfHeight, custom_dpi, 2 * dpi ),
                        "wrong lfHeight %ld vs %ld\n", lf1->lfHeight, lf2->lfHeight );
    ok_(__FILE__,line)( abs( lf1->lfWidth - MulDiv( lf2->lfWidth, dpi, custom_dpi )) <= 1,
                        "wrong lfWidth %ld vs %ld\n", lf1->lfWidth, lf2->lfWidth );
    ok_(__FILE__,line)( !memcmp( &lf1->lfEscapement, &lf2->lfEscapement,
                                 offsetof( LOGFONTW, lfFaceName ) - offsetof( LOGFONTW, lfEscapement )),
                        "font differs\n" );
    ok_(__FILE__,line)( !lstrcmpW( lf1->lfFaceName, lf2->lfFaceName ), "wrong face name %s vs %s\n",
                        wine_dbgstr_w( lf1->lfFaceName ), wine_dbgstr_w( lf2->lfFaceName ));
}

static void test_metrics_for_dpi( int custom_dpi )
{
    int i, val;
    NONCLIENTMETRICSW ncm1, ncm2;
    ICONMETRICSW im1, im2;
    LOGFONTW lf1, lf2;
    BOOL ret;

    if (!pSystemParametersInfoForDpi)
    {
        win_skip( "custom dpi metrics not supported\n" );
        return;
    }

    ncm1.cbSize = sizeof(ncm1);
    ret = SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, sizeof(ncm1), &ncm1, FALSE );
    ok( ret, "SystemParametersInfoW failed err %lu\n", GetLastError() );
    ncm2.cbSize = sizeof(ncm2);
    ret = pSystemParametersInfoForDpi( SPI_GETNONCLIENTMETRICS, sizeof(ncm2), &ncm2, FALSE, custom_dpi );
    ok( ret, "SystemParametersInfoForDpi failed err %lu\n", GetLastError() );

    for (i = 0; i < 92; i++)
    {
        int ret1 = GetSystemMetrics( i );
        int ret2 = pGetSystemMetricsForDpi( i, custom_dpi );
        switch (i)
        {
        case SM_CXVSCROLL:
        case SM_CYHSCROLL:
        case SM_CYVTHUMB:
        case SM_CXHTHUMB:
        case SM_CXICON:
        case SM_CYICON:
        case SM_CYVSCROLL:
        case SM_CXHSCROLL:
        case SM_CYSIZE:
        case SM_CXICONSPACING:
        case SM_CYICONSPACING:
        case SM_CXSMSIZE:
        case SM_CYSMSIZE:
        case SM_CYMENUSIZE:
            ok( ret1 == MulDiv( ret2, dpi, custom_dpi ), "%u: wrong value %u vs %u\n", i, ret1, ret2 );
            break;
        case SM_CXSIZE:
            ok( ret1 == ncm1.iCaptionWidth && ret2 == ncm2.iCaptionWidth,
                "%u: wrong value %u vs %u caption %u vs %u\n",
                i, ret1, ret2, ncm1.iCaptionWidth, ncm2.iCaptionWidth );
            break;
        case SM_CXCURSOR:
        case SM_CYCURSOR:
            val = MulDiv( 32, custom_dpi, USER_DEFAULT_SCREEN_DPI );
            if (val < 48) val = 32;
            else if (val < 64) val = 48;
            else val = 64;
            ok( val == ret2, "%u: wrong value %u vs %u\n", i, ret1, ret2 );
            break;
        case SM_CYCAPTION:
        case SM_CYSMCAPTION:
        case SM_CYMENU:
            ok( ret1 - 1 == MulDiv( ret2 - 1, dpi, custom_dpi ), "%u: wrong value %u vs %u\n", i, ret1, ret2 );
            break;
        case SM_CXMENUSIZE:
            ok( ret1 / 8 == MulDiv( ret2, dpi, custom_dpi ) / 8, "%u: wrong value %u vs %u\n", i, ret1, ret2 );
            break;
        case SM_CXFRAME:
        case SM_CYFRAME:
            ok( ret1 == ncm1.iBorderWidth + 3 && ret2 == ncm2.iBorderWidth + 3,
                "%u: wrong value %u vs %u borders %u+%u vs %u+%u\n", i, ret1, ret2,
                ncm1.iBorderWidth, ncm1.iPaddedBorderWidth, ncm2.iBorderWidth, ncm2.iPaddedBorderWidth );
            break;
        case SM_CXSMICON:
        case SM_CYSMICON:
            ok( ret1 == (MulDiv( 16, dpi, USER_DEFAULT_SCREEN_DPI ) & ~1) &&
                ret2 == (MulDiv( 16, custom_dpi, USER_DEFAULT_SCREEN_DPI ) & ~1),
                "%u: wrong value %u vs %u\n", i, ret1, ret2 );
            break;
        case SM_CXMENUCHECK:
        case SM_CYMENUCHECK:
            ok( ret1 == ((get_tmheightW( &ncm1.lfMenuFont, 1 ) - 1) | 1) &&
                ret2 == ((get_tmheightW( &ncm2.lfMenuFont, 1 ) - 1) | 1),
                "%u: wrong value %u vs %u font %u vs %u\n", i, ret1, ret2,
                get_tmheightW( &ncm1.lfMenuFont, 1 ), get_tmheightW( &ncm2.lfMenuFont, 1 ));
            break;
        case SM_CYMIN:
        case SM_CYMINTRACK:
            val = pGetSystemMetricsForDpi( SM_CYCAPTION, custom_dpi );
            val += 2 * pGetSystemMetricsForDpi( SM_CYFRAME, custom_dpi );
            val += 2 * ncm2.iPaddedBorderWidth;
            ok( ret1 == ret2 || ret2 == val /* Win10 1709+ */, "%u: expected %u or %u, got %u\n", i, ret1, val, ret2 );
            break;
        case SM_CXMIN:
        case SM_CXMINTRACK:
            val = MulDiv( ret1 - 7, custom_dpi, dpi );
            ok( ret1 == ret2 || (ret2 >= val - 10 && ret2 <= val + 10) /* Win10 1709+ */,
                "%u: expected %u or %u, got %u\n", i, ret1, val, ret2 );
            break;
        default:
            ok( ret1 == ret2, "%u: wrong value %u vs %u\n", i, ret1, ret2 );
            break;
        }
    }
    im1.cbSize = sizeof(im1);
    ret = SystemParametersInfoW( SPI_GETICONMETRICS, sizeof(im1), &im1, FALSE );
    ok( ret, "SystemParametersInfoW failed err %lu\n", GetLastError() );
    im2.cbSize = sizeof(im2);
    ret = pSystemParametersInfoForDpi( SPI_GETICONMETRICS, sizeof(im2), &im2, FALSE, custom_dpi );
    ok( ret, "SystemParametersInfoForDpi failed err %lu\n", GetLastError() );
    ok( im1.iHorzSpacing == MulDiv( im2.iHorzSpacing, dpi, custom_dpi ), "wrong iHorzSpacing %u vs %u\n",
        im1.iHorzSpacing, im2.iHorzSpacing );
    ok( im1.iVertSpacing == MulDiv( im2.iVertSpacing, dpi, custom_dpi ), "wrong iVertSpacing %u vs %u\n",
        im1.iVertSpacing, im2.iVertSpacing );
    ok( im1.iTitleWrap == im2.iTitleWrap, "wrong iTitleWrap %u vs %u\n",
        im1.iTitleWrap, im2.iTitleWrap );
    compare_font( &im1.lfFont, &im2.lfFont, dpi, custom_dpi, __LINE__ );

    ret = SystemParametersInfoW( SPI_GETICONTITLELOGFONT, sizeof(lf1), &lf1, FALSE );
    ok( ret, "SystemParametersInfoW failed err %lu\n", GetLastError() );
    ret = pSystemParametersInfoForDpi( SPI_GETICONTITLELOGFONT, sizeof(lf2), &lf2, FALSE, custom_dpi );
    ok( ret, "SystemParametersInfoForDpi failed err %lu\n", GetLastError() );
    compare_font( &lf1, &lf2, dpi, custom_dpi, __LINE__ );

    /* on high-dpi iPaddedBorderWidth is used in addition to iBorderWidth */
    ok( ncm1.iBorderWidth + ncm1.iPaddedBorderWidth == MulDiv( ncm2.iBorderWidth + ncm2.iPaddedBorderWidth, dpi, custom_dpi ),
        "wrong iBorderWidth %u+%u vs %u+%u\n",
        ncm1.iBorderWidth, ncm1.iPaddedBorderWidth, ncm2.iBorderWidth, ncm2.iPaddedBorderWidth );
    ok( ncm1.iScrollWidth == MulDiv( ncm2.iScrollWidth, dpi, custom_dpi ),
        "wrong iScrollWidth %u vs %u\n", ncm1.iScrollWidth, ncm2.iScrollWidth );
    ok( ncm1.iScrollHeight == MulDiv( ncm2.iScrollHeight, dpi, custom_dpi ),
        "wrong iScrollHeight %u vs %u\n", ncm1.iScrollHeight, ncm2.iScrollHeight );
    ok( ((ncm1.iCaptionWidth + 1) & ~1) == ((MulDiv( ncm2.iCaptionWidth, dpi, custom_dpi ) + 1) & ~1),
        "wrong iCaptionWidth %u vs %u\n", ncm1.iCaptionWidth, ncm2.iCaptionWidth );
    ok( ncm1.iCaptionHeight == MulDiv( ncm2.iCaptionHeight, dpi, custom_dpi ),
        "wrong iCaptionHeight %u vs %u\n", ncm1.iCaptionHeight, ncm2.iCaptionHeight );
    compare_font( &ncm1.lfCaptionFont, &ncm2.lfCaptionFont, dpi, custom_dpi, __LINE__ );
    ok( ncm1.iSmCaptionHeight == MulDiv( ncm2.iSmCaptionHeight, dpi, custom_dpi ),
        "wrong iSmCaptionHeight %u vs %u\n", ncm1.iSmCaptionHeight, ncm2.iSmCaptionHeight );
    compare_font( &ncm1.lfSmCaptionFont, &ncm2.lfSmCaptionFont, dpi, custom_dpi, __LINE__ );
    ok( ncm1.iMenuHeight == MulDiv( ncm2.iMenuHeight, dpi, custom_dpi ),
        "wrong iMenuHeight %u vs %u\n", ncm1.iMenuHeight, ncm2.iMenuHeight );
    /* iSmCaptionWidth and iMenuWidth apparently need to be multiples of 8 */
    ok( ncm1.iSmCaptionWidth / 8 == MulDiv( ncm2.iSmCaptionWidth, dpi, custom_dpi ) / 8,
        "wrong iSmCaptionWidth %u vs %u\n", ncm1.iSmCaptionWidth, ncm2.iSmCaptionWidth );
    ok( ncm1.iMenuWidth / 8 == MulDiv( ncm2.iMenuWidth, dpi, custom_dpi ) / 8,
        "wrong iMenuWidth %u vs %u\n", ncm1.iMenuWidth, ncm2.iMenuWidth );
    compare_font( &ncm1.lfMenuFont, &ncm2.lfMenuFont, dpi, custom_dpi, __LINE__ );
    compare_font( &ncm1.lfStatusFont, &ncm2.lfStatusFont, dpi, custom_dpi, __LINE__ );
    compare_font( &ncm1.lfMessageFont, &ncm2.lfMessageFont, dpi, custom_dpi, __LINE__ );

    for (i = 1; i < 120; i++)
    {
        if (i == SPI_GETICONTITLELOGFONT || i == SPI_GETNONCLIENTMETRICS || i == SPI_GETICONMETRICS)
            continue;
        SetLastError( 0xdeadbeef );
        ret = pSystemParametersInfoForDpi( i, 0, &val, 0, custom_dpi );
        ok( !ret, "%u: SystemParametersInfoForDpi succeeded\n", i );
            ok( GetLastError() == ERROR_INVALID_PARAMETER, "%u: wrong error %lu\n", i, GetLastError() );
    }
}

static BOOL get_primary_adapter_name(CHAR *name)
{
    DISPLAY_DEVICEA dd;
    DWORD adapter;

    dd.cb = sizeof(dd);
    for (adapter = 0; EnumDisplayDevicesA(NULL, adapter, &dd, 0); ++adapter)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            lstrcpyA(name, dd.DeviceName);
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL CALLBACK test_enum_display_settings(HMONITOR hmonitor, HDC hdc, LPRECT rect, LPARAM lparam)
{
    INT width, height;
    MONITORINFOEXA mi;
    DEVMODEA dm;
    BOOL ret;

    memset(&mi, 0, sizeof(mi));
    mi.cbSize = sizeof(mi);
    ret = GetMonitorInfoA(hmonitor, (MONITORINFO *)&mi);
    ok(ret, "GetMonitorInfoA failed, error %#lx\n", GetLastError());

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    ret = EnumDisplaySettingsA(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    ok(ret, "EnumDisplaySettingsA failed, error %#lx\n", GetLastError());

    ok((dm.dmFields & (DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT)) == (DM_POSITION | DM_PELSWIDTH | DM_PELSHEIGHT),
            "Unexpected dmFields %#lx.\n", dm.dmFields);
    ok(dm.dmPosition.x == mi.rcMonitor.left, "Expect dmPosition.x %ld, got %ld\n", mi.rcMonitor.left, dm.dmPosition.x);
    ok(dm.dmPosition.y == mi.rcMonitor.top, "Expect dmPosition.y %ld, got %ld\n", mi.rcMonitor.top, dm.dmPosition.y);
    width = mi.rcMonitor.right - mi.rcMonitor.left;
    ok(dm.dmPelsWidth == width, "Expect dmPelsWidth %d, got %ld\n", width, dm.dmPelsWidth);
    height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    ok(dm.dmPelsHeight == height, "Expect dmPelsHeight %d, got %ld\n", height, dm.dmPelsHeight);

    return TRUE;
}

static void test_EnumDisplaySettings(void)
{
    static const DWORD mode_fields = DM_DISPLAYORIENTATION | DM_BITSPERPEL |
            DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY;
    static const DWORD setting_fields = DM_DISPLAYORIENTATION | DM_BITSPERPEL |
            DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFLAGS | DM_DISPLAYFREQUENCY | DM_POSITION;
    CHAR primary_adapter[CCHDEVICENAME];
    DPI_AWARENESS_CONTEXT ctx = NULL;
    DWORD err, val, device, mode;
    BOOL attached, ret;
    DISPLAY_DEVICEA dd;
    DEVMODEA dm, dm2;
    DEVMODEW dmW;
    HDC hdc;

    /* Test invalid device names */
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    SetLastError(0xdeadbeef);
    ret = EnumDisplaySettingsA("invalid", ENUM_CURRENT_SETTINGS, &dm);
    ok(!ret, "EnumDisplaySettingsA succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "Expect error 0xdeadbeef, got %#lx\n", GetLastError());
    ok(dm.dmFields == 0, "Expect dmFields unchanged, got %#lx\n", dm.dmFields);

    /* Monitor device names are invalid */
    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    SetLastError(0xdeadbeef);
    ret = EnumDisplaySettingsA("\\\\.\\DISPLAY1\\Monitor0", ENUM_CURRENT_SETTINGS, &dm);
    ok(!ret, "EnumDisplaySettingsA succeeded\n");
    ok(GetLastError() == 0xdeadbeef, "Expect error 0xdeadbeef, got %#lx\n", GetLastError());
    ok(dm.dmFields == 0, "Expect dmFields unchanged, got %#lx\n", dm.dmFields);

    /* Test that passing NULL to device name parameter means to use the primary adapter */
    memset(&dm, 0, sizeof(dm));
    memset(&dm2, 0, sizeof(dm2));
    dm.dmSize = sizeof(dm);
    dm2.dmSize = sizeof(dm2);
    ret = get_primary_adapter_name(primary_adapter);
    ok(ret, "get_primary_adapter_name failed\n");
    ret = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(ret, "EnumDisplaySettingsA failed, error %#lx\n", GetLastError());
    ret = EnumDisplaySettingsA(primary_adapter, ENUM_CURRENT_SETTINGS, &dm2);
    ok(ret, "EnumDisplaySettingsA failed, error %#lx\n", GetLastError());
    ok(!memcmp(&dm, &dm2, sizeof(dm)), "Expect NULL device is the primary device.\n");

    /* Test dmSize */
    /* EnumDisplaySettingsA/W modify dmSize and don't check for insufficient dmSize */
    memset(&dm, 0, sizeof(dm));
    ret = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(ret, "EnumDisplaySettingsA failed, error %#lx\n", GetLastError());
    ok(dm.dmSize == FIELD_OFFSET(DEVMODEA, dmICMMethod), "Expect dmSize %lu, got %u\n",
            FIELD_OFFSET(DEVMODEA, dmICMMethod), dm.dmSize);
    ok((dm.dmFields & setting_fields) == setting_fields, "Expect dmFields to contain %#lx, got %#lx\n",
            setting_fields, dm.dmFields);

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    ret = EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    ok(ret, "EnumDisplaySettingsA failed, error %#lx\n", GetLastError());
    ok(dm.dmSize == FIELD_OFFSET(DEVMODEA, dmICMMethod), "Expect dmSize %lu, got %u\n",
            FIELD_OFFSET(DEVMODEA, dmICMMethod), dm.dmSize);
    ok((dm.dmFields & setting_fields) == setting_fields, "Expect dmFields to contain %#lx, got %#lx\n",
            setting_fields, dm.dmFields);

    memset(&dmW, 0, sizeof(dmW));
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dmW);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx\n", GetLastError());
    ok(dmW.dmSize == FIELD_OFFSET(DEVMODEW, dmICMMethod), "Expect dmSize %lu, got %u\n",
            FIELD_OFFSET(DEVMODEW, dmICMMethod), dmW.dmSize);
    ok((dmW.dmFields & setting_fields) == setting_fields, "Expect dmFields to contain %#lx, got %#lx\n",
            setting_fields, dmW.dmFields);

    memset(&dmW, 0, sizeof(dmW));
    dmW.dmSize = sizeof(dmW);
    ret = EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dmW);
    ok(ret, "EnumDisplaySettingsW failed, error %#lx\n", GetLastError());
    ok(dmW.dmSize == FIELD_OFFSET(DEVMODEW, dmICMMethod), "Expect dmSize %lu, got %u\n",
            FIELD_OFFSET(DEVMODEW, dmICMMethod), dmW.dmSize);
    ok((dmW.dmFields & setting_fields) == setting_fields, "Expect dmFields to contain %#lx, got %#lx\n",
            setting_fields, dmW.dmFields);

    /* EnumDisplaySettingsExA/W need dmSize to be at least FIELD_OFFSET(DEVMODEA/W, dmFields) + 1 to have valid dmFields */
    /* Crash on Windows when dmSize is zero */
    if (0)
    {
        memset(&dm, 0, sizeof(dm));
        ret = EnumDisplaySettingsExA(NULL, ENUM_CURRENT_SETTINGS, &dm, 0);
        ok(!ret, "EnumDisplaySettingsExA succeed\n");

        memset(&dmW, 0, sizeof(dmW));
        ret = EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &dmW, 0);
        ok(!ret, "EnumDisplaySettingsExA succeed\n");
    }

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = FIELD_OFFSET(DEVMODEA, dmFields);
    ret = EnumDisplaySettingsExA(NULL, ENUM_CURRENT_SETTINGS, &dm, 0);
    ok(ret, "EnumDisplaySettingsExA failed, error %#lx\n", GetLastError());
    todo_wine ok(dm.dmSize == FIELD_OFFSET(DEVMODEA, dmFields), "Expect dmSize unchanged, got %u\n", dm.dmSize);
    todo_wine ok(dm.dmFields == 0, "Expect dmFields unchanged, got %#lx\n", dm.dmFields);

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = FIELD_OFFSET(DEVMODEA, dmFields) + 1;
    ret = EnumDisplaySettingsExA(NULL, ENUM_CURRENT_SETTINGS, &dm, 0);
    ok(ret, "EnumDisplaySettingsExA failed, error %#lx\n", GetLastError());
    todo_wine ok(dm.dmSize == FIELD_OFFSET(DEVMODEA, dmFields) + 1, "Expect dmSize unchanged, got %u\n", dm.dmSize);
    todo_wine ok((dm.dmFields & setting_fields) == (DM_POSITION | DM_DISPLAYORIENTATION),
            "Expect dmFields to contain %#lx, got %#lx\n", DM_POSITION | DM_DISPLAYORIENTATION, dm.dmFields);
    /* Fields beyond dmSize don't get written */
    todo_wine ok(dm.dmPelsWidth == 0, "Expect dmPelsWidth unwritten\n");

    memset(&dmW, 0, sizeof(dmW));
    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmFields);
    ret = EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &dmW, 0);
    ok(ret, "EnumDisplaySettingsExW failed, error %#lx\n", GetLastError());
    todo_wine ok(dmW.dmSize == FIELD_OFFSET(DEVMODEW, dmFields), "Expect dmSize unchanged, got %u\n", dmW.dmSize);
    todo_wine ok(dmW.dmFields == 0, "Expect dmFields unchanged, got %#lx\n", dmW.dmFields);

    memset(&dmW, 0, sizeof(dmW));
    dmW.dmSize = FIELD_OFFSET(DEVMODEW, dmFields) + 1;
    ret = EnumDisplaySettingsExW(NULL, ENUM_CURRENT_SETTINGS, &dmW, 0);
    ok(ret, "EnumDisplaySettingsExW failed, error %#lx\n", GetLastError());
    todo_wine ok(dmW.dmSize == FIELD_OFFSET(DEVMODEW, dmFields) + 1, "Expect dmSize unchanged, got %u\n", dmW.dmSize);
    todo_wine ok((dmW.dmFields & setting_fields) == (DM_POSITION | DM_DISPLAYORIENTATION),
            "Expect dmFields to contain %#lx, got %#lx\n", DM_POSITION | DM_DISPLAYORIENTATION, dmW.dmFields);
    /* Fields beyond dmSize don't get written */
    todo_wine ok(dmW.dmPelsWidth == 0, "Expect dmPelsWidth unwritten\n");

    /* Test dmBitsPerPel */
    hdc = GetDC(0);
    val = GetDeviceCaps(hdc, BITSPIXEL);

    memset(&dm, 0, sizeof(dm));
    dm.dmSize = sizeof(dm);
    ret = EnumDisplaySettingsExA(NULL, ENUM_CURRENT_SETTINGS, &dm, 0);
    ok(ret, "EnumDisplaySettingsExA failed, error %#lx\n", GetLastError());
    ok((dm.dmFields & setting_fields) == setting_fields, "Expect dmFields to contain %#lx, got %#lx\n",
            setting_fields, dm.dmFields);
    ok(dm.dmBitsPerPel == val, "Expect dmBitsPerPel %ld, got %ld\n", val, dm.dmBitsPerPel);

    val = GetDeviceCaps(hdc, NUMCOLORS);
    if (dm.dmBitsPerPel <= 8)
    {
        ok(val == 256, "Screen bpp is %ld, NUMCOLORS returned %ld\n", dm.dmBitsPerPel, val);
    }
    else
    {
        ok(val == -1, "Screen bpp is %ld, NUMCOLORS returned %ld\n", dm.dmBitsPerPel, val);
    }

    ReleaseDC(0, hdc);

    /* Test dmPosition, dmPelsWidth and dmPelsHeight */
    /* Set DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE so that GetMonitorInfo() returns physical pixels */
    if (pSetThreadDpiAwarenessContext)
        ctx = pSetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    EnumDisplayMonitors(NULL, NULL, test_enum_display_settings, 0);
    if (pSetThreadDpiAwarenessContext && ctx)
        pSetThreadDpiAwarenessContext(ctx);

    /* Test mode enumeration and other fields */
    dd.cb = sizeof(dd);
    for (device = 0; EnumDisplayDevicesA(NULL, device, &dd, 0); ++device)
    {
        INT number;

        /* Skip software devices */
        if (sscanf(dd.DeviceName, "\\\\.\\DISPLAY%d", &number) != 1)
            continue;

        attached = dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;

        memset(&dm, 0, sizeof(dm));
        dm.dmSize = sizeof(dm);
        SetLastError(0xdeadbeef);
        for (mode = ENUM_REGISTRY_SETTINGS; EnumDisplaySettingsA(dd.DeviceName, mode, &dm); ++mode)
        {
            if (mode == ENUM_CURRENT_SETTINGS)
            {
                ok((dm.dmFields & setting_fields) == setting_fields,
                        "Expect dmFields to contain %#lx, got %#lx\n", setting_fields, dm.dmFields);
            }
            else
            {
                ok((dm.dmFields & mode_fields) == mode_fields, "Expect dmFields to contain %#lx, got %#lx\n",
                        mode_fields, dm.dmFields);
            }

            ok(dm.dmDisplayOrientation == DMDO_DEFAULT, "Expect dmDisplayOrientation DMDO_DEFAULT, got %#lx\n",
                    dm.dmDisplayOrientation);
            ok(dm.dmDisplayFlags == 0, "Expect dmDisplayFlags zero\n");

            if (mode == ENUM_CURRENT_SETTINGS && !attached)
            {
                ok(dm.dmBitsPerPel == 0, "Expect dmBitsPerPel zero, got %lu\n", dm.dmBitsPerPel);
                ok(dm.dmPelsWidth == 0, "Expect dmPelsWidth zero, got %lu\n", dm.dmPelsWidth);
                ok(dm.dmPelsHeight == 0, "Expect dmPelsHeight zero, got %lu\n", dm.dmPelsHeight);
                ok(dm.dmDisplayFrequency == 0, "Expect dmDisplayFrequency zero, got %lu\n", dm.dmDisplayFrequency);
            }
            else if (mode != ENUM_REGISTRY_SETTINGS)
            {
                ok(dm.dmBitsPerPel, "Expect dmBitsPerPel not zero\n");
                ok(dm.dmPelsWidth, "Expect dmPelsWidth not zero\n");
                ok(dm.dmPelsHeight, "Expect dmPelsHeight not zero\n");
                ok(dm.dmDisplayFrequency, "Expect dmDisplayFrequency not zero\n");
            }
        }

        ok(mode >= 1, "Expect at least one valid mode gets enumerated.\n");

        err = GetLastError();
        ok(err == ERROR_NO_MORE_FILES ||
                err == ERROR_MOD_NOT_FOUND /* Win8 */ ||
                err == 0xdeadbeef, /* XP, 2003 */
                "Expected ERROR_NO_MORE_FILES, ERROR_MOD_NOT_FOUND or 0xdeadbeef, got %#lx\n", err);
    }
}

static void test_GetSysColorBrush(void)
{
    HBRUSH hbr;

    SetLastError(0xdeadbeef);
    hbr = GetSysColorBrush(-1);
    ok(hbr == NULL, "Expected NULL brush\n");
    ok(GetLastError() == 0xdeadbeef, "Expected last error not set, got %lx\n", GetLastError());
    /* greater than max index */
    hbr = GetSysColorBrush(COLOR_MENUBAR);
    if (hbr)
    {
        SetLastError(0xdeadbeef);
        hbr = GetSysColorBrush(COLOR_MENUBAR + 1);
        ok(hbr == NULL, "Expected NULL brush\n");
        ok(GetLastError() == 0xdeadbeef, "Expected last error not set, got %lx\n", GetLastError());
    }
    else
        win_skip("COLOR_MENUBAR unsupported\n");
}

static void test_dpi_stock_objects( HDC hdc )
{
    DPI_AWARENESS_CONTEXT context;
    HGDIOBJ obj[STOCK_LAST + 1], obj2[STOCK_LAST + 1];
    LOGFONTW lf, lf2;
    UINT i, dpi;

    context = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    dpi = GetDeviceCaps( hdc, LOGPIXELSX );
    ok( dpi == USER_DEFAULT_SCREEN_DPI, "wrong dpi %u\n", dpi );
    ok( !pIsProcessDPIAware(), "not aware\n" );
    for (i = 0; i <= STOCK_LAST; i++) obj[i] = GetStockObject( i );

    pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    dpi = GetDeviceCaps( hdc, LOGPIXELSX );
    ok( dpi == real_dpi, "wrong dpi %u\n", dpi );
    ok( pIsProcessDPIAware(), "not aware\n" );
    for (i = 0; i <= STOCK_LAST; i++) obj2[i] = GetStockObject( i );

    for (i = 0; i <= STOCK_LAST; i++)
    {
        switch (i)
        {
        case OEM_FIXED_FONT:
        case SYSTEM_FIXED_FONT:
            ok( obj[i] != obj2[i], "%u: same object\n", i );
            break;
        case SYSTEM_FONT:
        case DEFAULT_GUI_FONT:
            ok( obj[i] != obj2[i], "%u: same object\n", i );
            GetObjectW( obj[i], sizeof(lf), &lf );
            GetObjectW( obj2[i], sizeof(lf2), &lf2 );
            ok( lf.lfHeight == MulDiv( lf2.lfHeight, USER_DEFAULT_SCREEN_DPI, real_dpi ),
                "%u: wrong height %ld / %ld\n", i, lf.lfHeight, lf2.lfHeight );
            break;
        default:
            ok( obj[i] == obj2[i], "%u: different object\n", i );
            break;
        }
    }

    pSetThreadDpiAwarenessContext( context );
}

static void scale_point_dpi( POINT *pt, UINT src_dpi, UINT target_dpi )
{
    pt->x = MulDiv( pt->x, target_dpi, src_dpi );
    pt->y = MulDiv( pt->y, target_dpi, src_dpi );
}

static void scale_rect_dpi( RECT *rect, UINT src_dpi, UINT target_dpi )
{
    rect->left = MulDiv( rect->left, target_dpi, src_dpi );
    rect->top = MulDiv( rect->top, target_dpi, src_dpi );
    rect->right = MulDiv( rect->right, target_dpi, src_dpi );
    rect->bottom = MulDiv( rect->bottom, target_dpi, src_dpi );
}

static void scale_point_dpi_aware( POINT *pt, DPI_AWARENESS from, DPI_AWARENESS to )
{
    if (from == DPI_AWARENESS_UNAWARE && to != DPI_AWARENESS_UNAWARE)
        scale_point_dpi( pt, USER_DEFAULT_SCREEN_DPI, real_dpi );
    else if (from != DPI_AWARENESS_UNAWARE && to == DPI_AWARENESS_UNAWARE)
        scale_point_dpi( pt, real_dpi, USER_DEFAULT_SCREEN_DPI );
}

static void scale_rect_dpi_aware( RECT *rect, DPI_AWARENESS from, DPI_AWARENESS to )
{
    if (from == DPI_AWARENESS_UNAWARE && to != DPI_AWARENESS_UNAWARE)
        scale_rect_dpi( rect, USER_DEFAULT_SCREEN_DPI, real_dpi );
    else if (from != DPI_AWARENESS_UNAWARE && to == DPI_AWARENESS_UNAWARE)
        scale_rect_dpi( rect, real_dpi, USER_DEFAULT_SCREEN_DPI );
}

static void test_dpi_mapping(void)
{
    HWND hwnd, child;
    HDC hdc;
    UINT win_dpi, units;
    POINT point;
    BOOL ret;
    HRGN rgn, update;
    RECT rect, orig, client, desktop, expect;
    ULONG_PTR i, j, k;
    WINDOWPLACEMENT wpl_orig, wpl;
    HMONITOR monitor;
    INT monitor_count;
    MONITORINFO mon_info;
    DPI_AWARENESS_CONTEXT context;

    if (!pLogicalToPhysicalPointForPerMonitorDPI)
    {
        win_skip( "LogicalToPhysicalPointForPerMonitorDPI not supported\n" );
        return;
    }
    context = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    GetWindowRect( GetDesktopWindow(), &desktop );
    monitor_count = GetSystemMetrics( SM_CMONITORS );
    for (i = DPI_AWARENESS_UNAWARE; i <= DPI_AWARENESS_PER_MONITOR_AWARE; i++)
    {
        pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~i );
        /* test desktop rect */
        GetWindowRect( GetDesktopWindow(), &rect );
        expect = desktop;
        if (i == DPI_AWARENESS_UNAWARE) scale_rect_dpi( &expect, real_dpi, USER_DEFAULT_SCREEN_DPI );
        ok( EqualRect( &expect, &rect ), "%Iu: wrong desktop rect %s expected %s\n",
            i, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
        SetRect( &rect, 0, 0, GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ));
        ok( EqualRect( &expect, &rect ), "%Iu: wrong desktop rect %s expected %s\n",
            i, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
        if (monitor_count < 2)
        {
            SetRect( &rect, 0, 0, GetSystemMetrics( SM_CXVIRTUALSCREEN ), GetSystemMetrics( SM_CYVIRTUALSCREEN ));
            ok( EqualRect( &expect, &rect ), "%Iu: wrong virt desktop rect %s expected %s\n",
                i, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
        }
        SetRect( &rect, 0, 0, 1, 1 );
        monitor = MonitorFromRect( &rect, MONITOR_DEFAULTTOPRIMARY );
        ok( monitor != 0, "failed to get monitor\n" );
        mon_info.cbSize = sizeof(mon_info);
        ok( GetMonitorInfoW( monitor, &mon_info ), "GetMonitorInfoExW failed\n" );
        ok( EqualRect( &expect, &mon_info.rcMonitor ), "%Iu: wrong monitor rect %s expected %s\n",
            i, wine_dbgstr_rect(&mon_info.rcMonitor), wine_dbgstr_rect(&expect) );
        hdc = CreateDCA( "display", NULL, NULL, NULL );
        SetRect( &rect, 0, 0, GetDeviceCaps( hdc, HORZRES ), GetDeviceCaps( hdc, VERTRES ));
        ok( EqualRect( &expect, &rect ), "%Iu: wrong caps desktop rect %s expected %s\n",
            i, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
        SetRect( &rect, 0, 0, GetDeviceCaps( hdc, DESKTOPHORZRES ), GetDeviceCaps( hdc, DESKTOPVERTRES ));
        todo_wine_if(monitor_count > 1)
        ok( EqualRect( &desktop, &rect ), "%Iu: wrong caps virt desktop rect %s expected %s\n",
            i, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&desktop) );
        DeleteDC( hdc );
        /* test message window rect */
        hwnd = CreateWindowA( "SysParamsTestClass", "test", WS_CHILD,
                              10, 10, 20, 20, HWND_MESSAGE, 0, GetModuleHandleA(0), NULL );
        GetWindowRect( GetAncestor( hwnd, GA_PARENT ), &rect );
        SetRect( &expect, 0, 0, 100, 100 );
        if (i == DPI_AWARENESS_UNAWARE) scale_rect_dpi( &expect, real_dpi, USER_DEFAULT_SCREEN_DPI );
        ok( EqualRect( &expect, &rect ), "%Iu: wrong message rect %s expected %s\n",
            i, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
        DestroyWindow( hwnd );
    }
    for (i = DPI_AWARENESS_UNAWARE; i <= DPI_AWARENESS_PER_MONITOR_AWARE; i++)
    {
        pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~i );
        hwnd = CreateWindowA( "SysParamsTestClass", "test", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                              193, 177, 295, 303, 0, 0, GetModuleHandleA(0), NULL );
        ok( hwnd != 0, "creating window failed err %lu\n", GetLastError());
        child = CreateWindowA( "SysParamsTestClass", "child", WS_CHILD | WS_VISIBLE,
                               50, 60, 70, 80, hwnd, 0, GetModuleHandleA(0), NULL );
        ok( child != 0, "creating child failed err %lu\n", GetLastError());
        GetWindowRect( hwnd, &orig );
        SetRect( &rect, 0, 0, 0, 0 );
        pAdjustWindowRectExForDpi( &rect, WS_OVERLAPPEDWINDOW, FALSE, 0, pGetDpiForWindow( hwnd ));
        SetRect( &client, orig.left - rect.left, orig.top - rect.top,
                 orig.right - rect.right, orig.bottom - rect.bottom );
        ShowWindow( hwnd, SW_MINIMIZE );
        ShowWindow( hwnd, SW_RESTORE );
        GetWindowPlacement( hwnd, &wpl_orig );
        units = GetDialogBaseUnits();

        for (j = DPI_AWARENESS_UNAWARE; j <= DPI_AWARENESS_PER_MONITOR_AWARE; j++)
        {
            pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~j );
            /* test window rect */
            GetWindowRect( hwnd, &rect );
            expect = orig;
            scale_rect_dpi_aware( &expect, i, j );
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong window rect %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            /* test client rect */
            GetClientRect( hwnd, &rect );
            expect = client;
            OffsetRect( &expect, -expect.left, -expect.top );
            scale_rect_dpi_aware( &expect, i, j );
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong client rect %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            /* test window placement */
            GetWindowPlacement( hwnd, &wpl );
            point = wpl_orig.ptMinPosition;
            if (point.x != -1 || point.y != -1) scale_point_dpi_aware( &point, i, j );
            ok( wpl.ptMinPosition.x == point.x && wpl.ptMinPosition.y == point.y,
                "%Iu/%Iu: wrong placement min pos %ld,%ld expected %ld,%ld\n", i, j,
                wpl.ptMinPosition.x, wpl.ptMinPosition.y, point.x, point.y );
            point = wpl_orig.ptMaxPosition;
            if (point.x != -1 || point.y != -1) scale_point_dpi_aware( &point, i, j );
            ok( wpl.ptMaxPosition.x == point.x && wpl.ptMaxPosition.y == point.y,
                "%Iu/%Iu: wrong placement max pos %ld,%ld expected %ld,%ld\n", i, j,
                wpl.ptMaxPosition.x, wpl.ptMaxPosition.y, point.x, point.y );
            expect = wpl_orig.rcNormalPosition;
            scale_rect_dpi_aware( &expect, i, j );
            ok( EqualRect( &wpl.rcNormalPosition, &expect ),
                "%Iu/%Iu: wrong placement rect %s expect %s\n", i, j,
                wine_dbgstr_rect(&wpl.rcNormalPosition), wine_dbgstr_rect(&expect));
            /* test DC rect */
            hdc = GetDC( hwnd );
            GetClipBox( hdc, &rect );
            SetRect( &expect, 0, 0, client.right - client.left, client.bottom - client.top );
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong clip box %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            /* test DC resolution */
            SetRect( &rect, 0, 0, GetDeviceCaps( hdc, HORZRES ), GetDeviceCaps( hdc, VERTRES ));
            expect = desktop;
            if (j == DPI_AWARENESS_UNAWARE) scale_rect_dpi( &expect, real_dpi, USER_DEFAULT_SCREEN_DPI );
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong DC resolution %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            SetRect( &rect, 0, 0, GetDeviceCaps( hdc, DESKTOPHORZRES ), GetDeviceCaps( hdc, DESKTOPVERTRES ));
            todo_wine_if(monitor_count > 1)
            ok( EqualRect( &desktop, &rect ), "%Iu/%Iu: wrong desktop resolution %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&desktop) );
            ReleaseDC( hwnd, hdc );
            /* test DC win rect */
            hdc = GetWindowDC( hwnd );
            GetClipBox( hdc, &rect );
            SetRect( &expect, 0, 0, 295, 303 );
            todo_wine
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong clip box win DC %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            ReleaseDC( hwnd, hdc );
            /* test window invalidation */
            UpdateWindow( hwnd );
            update = CreateRectRgn( 0, 0, 0, 0 );
            ret = GetUpdateRgn( hwnd, update, FALSE );
            ok( ret == NULLREGION, "update region not empty\n" );
            rgn = CreateRectRgn( 20, 20, 25, 25 );
            for (k = DPI_AWARENESS_UNAWARE; k <= DPI_AWARENESS_PER_MONITOR_AWARE; k++)
            {
                pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~k );
                RedrawWindow( hwnd, 0, rgn, RDW_INVALIDATE );
                pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~j );
                GetUpdateRgn( hwnd, update, FALSE );
                GetRgnBox( update, &rect );
                SetRect( &expect, 20, 20, 25, 25 );
                ok( EqualRect( &expect, &rect ), "%Iu/%Iu/%Iu: wrong update region %s expected %s\n",
                    i, j, k, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
                GetUpdateRect( hwnd, &rect, FALSE );
                scale_rect_dpi_aware( &expect, i, j );
                ok( EqualRect( &expect, &rect ), "%Iu/%Iu/%Iu: wrong update rect %s expected %s\n",
                    i, j, k, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
                UpdateWindow( hwnd );
            }
            for (k = DPI_AWARENESS_UNAWARE; k <= DPI_AWARENESS_PER_MONITOR_AWARE; k++)
            {
                RedrawWindow( hwnd, 0, rgn, RDW_INVALIDATE );
                pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~k );
                GetUpdateRgn( hwnd, update, FALSE );
                pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~j );
                GetRgnBox( update, &rect );
                SetRect( &expect, 20, 20, 25, 25 );
                ok( EqualRect( &expect, &rect ), "%Iu/%Iu/%Iu: wrong update region %s expected %s\n",
                    i, j, k, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
                GetUpdateRect( hwnd, &rect, FALSE );
                scale_rect_dpi_aware( &expect, i, j );
                ok( EqualRect( &expect, &rect ), "%Iu/%Iu/%Iu: wrong update rect %s expected %s\n",
                    i, j, k, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
                UpdateWindow( hwnd );
            }
            /* test desktop window invalidation */
            pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
            GetClientRect( hwnd, &rect );
            InflateRect( &rect, -50, -50 );
            expect = rect;
            MapWindowPoints( hwnd, 0, (POINT *)&rect, 2 );
            pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~j );
            RedrawWindow( 0, &rect, 0, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN );
            GetUpdateRgn( hwnd, update, TRUE );
            GetRgnBox( update, &rect );
            if (i == DPI_AWARENESS_UNAWARE) scale_rect_dpi( &expect, real_dpi, USER_DEFAULT_SCREEN_DPI );
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong update region %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            GetUpdateRect( hwnd, &rect, FALSE );
            scale_rect_dpi_aware( &expect, i, j );
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong update rect %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            UpdateWindow( hwnd );
            DeleteObject( update );
            /* test dialog units */
            ret = GetDialogBaseUnits();
            point.x = LOWORD( units );
            point.y = HIWORD( units );
            scale_point_dpi_aware( &point, i, j );
            ok( LOWORD(ret) == point.x && HIWORD(ret) == point.y, "%Iu/%Iu: wrong units %d,%d / %ld,%ld\n",
                i, j, LOWORD(ret), HIWORD(ret), point.x, point.y );
            /* test window points mapping */
            SetRect( &rect, 0, 0, 100, 100 );
            rect.right = rect.left + 100;
            rect.bottom = rect.top + 100;
            MapWindowPoints( hwnd, 0, (POINT *)&rect, 2 );
            expect = client;
            scale_rect_dpi_aware( &expect, i, j );
            expect.right = expect.left + 100;
            expect.bottom = expect.top + 100;
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong MapWindowPoints rect %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            SetRect( &rect, 50, 60, 70, 80 );
            scale_rect_dpi_aware( &rect, i, j );
            SetRect( &expect, 40, 30, 60, 80 );
            OffsetRect( &expect, -rect.left, -rect.top );
            SetRect( &rect, 40, 30, 60, 80 );
            MapWindowPoints( hwnd, child, (POINT *)&rect, 2 );
            ok( EqualRect( &expect, &rect ), "%Iu/%Iu: wrong MapWindowPoints child rect %s expected %s\n",
                i, j, wine_dbgstr_rect(&rect), wine_dbgstr_rect(&expect) );
            /* test logical<->physical coords mapping */
            win_dpi = pGetDpiForWindow( hwnd );
            if (i == DPI_AWARENESS_UNAWARE)
                ok( win_dpi == USER_DEFAULT_SCREEN_DPI, "wrong dpi %u\n", win_dpi );
            else if (i == DPI_AWARENESS_SYSTEM_AWARE)
                ok( win_dpi == real_dpi, "wrong dpi %u / %u\n", win_dpi, real_dpi );
            point.x = 373;
            point.y = 377;
            ret = pLogicalToPhysicalPointForPerMonitorDPI( hwnd, &point );
            ok( ret, "%Iu/%Iu: LogicalToPhysicalPointForPerMonitorDPI failed\n", i, j );
            ok( point.x == MulDiv( 373, real_dpi, win_dpi ) &&
                point.y == MulDiv( 377, real_dpi, win_dpi ),
                "%Iu/%Iu: wrong pos %ld,%ld dpi %u\n", i, j, point.x, point.y, win_dpi );
            point.x = 405;
            point.y = 423;
            ret = pPhysicalToLogicalPointForPerMonitorDPI( hwnd, &point );
            ok( ret, "%Iu/%Iu: PhysicalToLogicalPointForPerMonitorDPI failed\n", i, j );
            ok( point.x == MulDiv( 405, win_dpi, real_dpi ) &&
                point.y == MulDiv( 423, win_dpi, real_dpi ),
                "%Iu/%Iu: wrong pos %ld,%ld dpi %u\n", i, j, point.x, point.y, win_dpi );
            /* point outside the window fails, but note that Windows (wrongly) checks against the
             * window rect transformed relative to the thread's awareness */
            GetWindowRect( hwnd, &rect );
            point.x = rect.left - 1;
            point.y = rect.top;
            ret = pLogicalToPhysicalPointForPerMonitorDPI( hwnd, &point );
            ok( !ret, "%Iu/%Iu: LogicalToPhysicalPointForPerMonitorDPI succeeded\n", i, j );
            point.x++;
            point.y--;
            ret = pLogicalToPhysicalPointForPerMonitorDPI( hwnd, &point );
            ok( !ret, "%Iu/%Iu: LogicalToPhysicalPointForPerMonitorDPI succeeded\n", i, j );
            point.y++;
            ret = pLogicalToPhysicalPointForPerMonitorDPI( hwnd, &point );
            ok( ret, "%Iu/%Iu: LogicalToPhysicalPointForPerMonitorDPI failed\n", i, j );
            point.x = rect.right;
            point.y = rect.bottom + 1;
            ret = pLogicalToPhysicalPointForPerMonitorDPI( hwnd, &point );
            ok( !ret, "%Iu/%Iu: LogicalToPhysicalPointForPerMonitorDPI succeeded\n", i, j );
            point.x++;
            point.y--;
            ret = pLogicalToPhysicalPointForPerMonitorDPI( hwnd, &point );
            ok( !ret, "%Iu/%Iu: LogicalToPhysicalPointForPerMonitorDPI succeeded\n", i, j );
            point.x--;
            ret = pLogicalToPhysicalPointForPerMonitorDPI( hwnd, &point );
            ok( ret, "%Iu/%Iu: LogicalToPhysicalPointForPerMonitorDPI failed\n", i, j );
            /* get physical window rect */
            pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
            GetWindowRect( hwnd, &rect );
            pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~j );
            point.x = rect.left - 1;
            point.y = rect.top;
            ret = pPhysicalToLogicalPointForPerMonitorDPI( hwnd, &point );
            ok( !ret, "%Iu/%Iu: PhysicalToLogicalPointForPerMonitorDPI succeeded\n", i, j );
            point.x++;
            point.y--;
            ret = pPhysicalToLogicalPointForPerMonitorDPI( hwnd, &point );
            ok( !ret, "%Iu/%Iu: PhysicalToLogicalPointForPerMonitorDPI succeeded\n", i, j );
            point.y++;
            ret = pPhysicalToLogicalPointForPerMonitorDPI( hwnd, &point );
            ok( ret, "%Iu/%Iu: PhysicalToLogicalPointForPerMonitorDPI failed\n", i, j );
            point.x = rect.right;
            point.y = rect.bottom + 1;
            ret = pPhysicalToLogicalPointForPerMonitorDPI( hwnd, &point );
            ok( !ret, "%Iu/%Iu: PhysicalToLogicalPointForPerMonitorDPI succeeded\n", i, j );
            point.x++;
            point.y--;
            ret = pPhysicalToLogicalPointForPerMonitorDPI( hwnd, &point );
            ok( !ret, "%Iu/%Iu: PhysicalToLogicalPointForPerMonitorDPI succeeded\n", i, j );
            point.x--;
            ret = pPhysicalToLogicalPointForPerMonitorDPI( hwnd, &point );
            ok( ret, "%Iu/%Iu: PhysicalToLogicalPointForPerMonitorDPI failed\n", i, j );
        }
        DestroyWindow( hwnd );
    }
    pSetThreadDpiAwarenessContext( context );
}

static void test_dpi_aware(void)
{
    BOOL ret;

    if (!pIsProcessDPIAware)
    {
        win_skip("IsProcessDPIAware not available\n");
        return;
    }

    ret = pSetProcessDPIAware();
    ok(ret, "got %d\n", ret);

    ret = pIsProcessDPIAware();
    ok(ret, "got %d\n", ret);

    dpi = real_dpi;
    test_GetSystemMetrics();
    test_metrics_for_dpi( 96 );
    test_metrics_for_dpi( 192 );
}

static void test_SetProcessDpiAwarenessContext( ULONG arg )
{
    DPI_AWARENESS_CONTEXT contexts[] =
    {
        (DPI_AWARENESS_CONTEXT)0x6010,
        (DPI_AWARENESS_CONTEXT)0x40006010,
        (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x11 | (real_dpi << 8)),
        (DPI_AWARENESS_CONTEXT)0x12,
        (DPI_AWARENESS_CONTEXT)0x22,
    };
    DPI_AWARENESS_CONTEXT context, ctx, old_ctx, expect_ctx;
    UINT ret, i;

    /* 0x11 is system aware DPI and only works with the current system DPI */
    if (arg == 0x11) context = contexts[2];
    else context = LongToHandle( arg ) /* sign-extend */;

    if (context == DPI_AWARENESS_CONTEXT_UNAWARE) expect_ctx = contexts[0];
    else if (context == DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED) expect_ctx = contexts[1];
    else if (context == DPI_AWARENESS_CONTEXT_SYSTEM_AWARE) expect_ctx = contexts[2];
    else if (context == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE) expect_ctx = contexts[3];
    else if (context == DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) expect_ctx = contexts[4];
    else if ((arg & 0xff000000) == 0x80000000) expect_ctx = ULongToHandle( arg ) /* no sign-extend */;
    else expect_ctx = context;

    winetest_push_context( "%#lx", arg );

    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x6010, "got %p\n", ctx );

    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( 0 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x11 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ACCESS_DENIED, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x21 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x32 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6012 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6022 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x40006011 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x40000012 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x7810 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x1ff11 );
    ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
    todo_wine ok( GetLastError() == ERROR_ACCESS_DENIED, "got %#lx\n", GetLastError() );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x6010, "got %p\n", ctx );

    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( context );
    ok( ret, "SetProcessDpiAwarenessContext failed, error %lu\n", GetLastError() );
    ok( GetLastError() == 0xdeadbeef, "got %#lx\n", GetLastError() );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == expect_ctx, "got %p\n", ctx );

    for (i = 0; i < ARRAY_SIZE(contexts); i++)
    {
        SetLastError( 0xdeadbeef );
        ret = pSetProcessDpiAwarenessContext( contexts[i] );
        ok( !ret, "SetProcessDpiAwarenessContext succeeded\n" );
        ok( GetLastError() == ERROR_ACCESS_DENIED, "got %#lx\n", GetLastError() );
        ctx = pGetThreadDpiAwarenessContext();
        ok( ctx == expect_ctx, "got %p\n", ctx );
    }

    /* thread DPI context overrides process DPI context */
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == expect_ctx, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x12 );
    ok( old_ctx == (DPI_AWARENESS_CONTEXT)ULongToHandle( 0x80000000 | (UINT_PTR)expect_ctx ), "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x12, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x22 );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x22, "got %p\n", ctx );

    /* thread DPI context can be reset to process DPI context with 0x80000000, but needs to be valid */
    SetLastError( 0xdeadbeef );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80000000 );
    ok( !old_ctx, "SetThreadDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80000010 );
    ok( !old_ctx, "SetThreadDpiAwarenessContext succeeded\n" );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %#lx\n", GetLastError() );

    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80006010 );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == expect_ctx, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x80000011 | (real_dpi << 8)) );
    ok( old_ctx == ULongToHandle( 0x80000000 | (UINT_PTR)expect_ctx ), "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == expect_ctx, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80000012 );
    ok( old_ctx == ULongToHandle( 0x80000000 | (UINT_PTR)expect_ctx ), "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == expect_ctx, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80000022 );
    ok( old_ctx == ULongToHandle( 0x80000000 | (UINT_PTR)expect_ctx ), "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == expect_ctx, "got %p\n", ctx );

    winetest_pop_context();
}

static void test_SetThreadDpiAwarenessContext(void)
{
    DPI_AWARENESS_CONTEXT ctx, old_ctx;

    if (!pSetThreadDpiAwarenessContext || !pGetThreadDpiAwarenessContext)
    {
        win_skip( "(Set|Get)ThreadDpiAwarenessContext missing, skipping tests\n" );
        return;
    }

    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x6010, "got %p\n", ctx );

    old_ctx = pSetThreadDpiAwarenessContext( 0 );
    ok( !old_ctx, "SetThreadDpiAwarenessContext succeeded\n" );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x11 );
    ok( !old_ctx, "SetThreadDpiAwarenessContext succeeded\n" );

    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x12 );
    ok( old_ctx == (DPI_AWARENESS_CONTEXT)0x80006010, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x12, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x22 );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x22, "got %p\n", ctx );

    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x7810 );
    ok( !old_ctx, "SetThreadDpiAwarenessContext succeeded\n" );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x6010, "got %p\n", ctx );

    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x11 | ((real_dpi + 1) << 8)) );
    ok( !old_ctx, "SetThreadDpiAwarenessContext succeeded\n" );
    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x11 | (real_dpi << 8)) );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x11 | (real_dpi << 8)), "got %p\n", ctx );

    old_ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x40006010 );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x40006010, "got %p\n", ctx );

    old_ctx = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x6010, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x11 | (real_dpi << 8)), "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x12, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x22, "got %p\n", ctx );
    old_ctx = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED );
    ok( old_ctx == ctx, "got %p\n", old_ctx );
    ctx = pGetThreadDpiAwarenessContext();
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x40006010, "got %p\n", ctx );

    /* restore process-wide DPI awareness context */
    ctx = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80006010 );
    ok( ctx == (DPI_AWARENESS_CONTEXT)0x40006010, "got %p\n", ctx );
}

static void test_IsValidDpiAwarenessContext(void)
{
    UINT_PTR i;
    BOOL ret;

    if (!pIsValidDpiAwarenessContext)
    {
        win_skip( "IsValidDpiAwarenessContext missing, skipping tests\n" );
        return;
    }

    ret = pIsValidDpiAwarenessContext( 0 );
    ok( !ret, "IsValidDpiAwarenessContext returned %u\n", ret );

    /* commonly seen dpi awareness contexts */
    ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x12 );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x22 );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6011 );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x40006010 );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );

    ret = pIsValidDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED );
    ok( ret, "IsValidDpiAwarenessContext returned %u\n", ret );
    ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)-6 );
    ok( !ret, "IsValidDpiAwarenessContext returned %u\n", ret );

    for (i = 1; i < 0xff; i++)
    {
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)i );
        if (i == 0x12 || i == 0x22) ok( ret, "IsValidDpiAwarenessContext %#Ix failed\n", i );
        /* 0x0011 is rejected on win11 */
        else if (i != 0x11) ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", i );

        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)(i | 0x6000) );
        if (i == 0x10 || i == 0x11) ok( ret, "IsValidDpiAwarenessContext %#Ix failed\n", (i | 0x6000) );
        /* 0x6013 is accepted on win11 */
        else if (i != 0x13) ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", (i | 0x6000) );

        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 8) | 0x10) );
        if (i == 0x60) ok( ret, "IsValidDpiAwarenessContext %#Ix failed\n", ((i << 8) | 0x10) );
        else ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 8) | 0x10) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 8) | 0x11) );
        if (i != 0xff) ok( ret, "IsValidDpiAwarenessContext %#Ix failed\n", ((i << 8) | 0x11) );
        else ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 8) | 0x11) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 8) | 0x12) );
        ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 8) | 0x12) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 8) | 0x22) );
        ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 8) | 0x22) );

        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 16) | 0x6010) );
        ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 16) | 0x6010) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 16) | (real_dpi << 8) | 0x11) );
        if (i == 1) ok( ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 16) | (real_dpi << 8) | 0x11) );
        else ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 16) | (real_dpi << 8) | 0x11) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 16) | 0x12) );
        ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 16) | 0x12) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 16) | 0x22) );
        ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 16) | 0x22) );

        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 24) | 0x6010) );
        if (i & 0x1f) ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 24) | 0x6010) );
        /* 0x20000000 is rejected on win11 */
        else if (!(i & 0x20)) ok( ret, "IsValidDpiAwarenessContext %#Ix failed\n", ((i << 24) | 0x6010) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 24) | (real_dpi << 8) | 0x11) );
        if (i & 0x5f) ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 24) | (real_dpi << 8) | 0x11) );
        /* 0x20000000 is rejected on win11 */
        else if (!(i & 0x20)) ok( ret, "IsValidDpiAwarenessContext %#Ix failed\n", ((i << 24) | (real_dpi << 8) | 0x11) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 24) | 0x12) );
        if (i & 0x5f) ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 24) | 0x12) );
        /* 0x20000000 is rejected on win11 */
        else if (!(i & 0x20)) ok( ret, "IsValidDpiAwarenessContext %#Ix failed\n", ((i << 24) | 0x12) );
        ret = pIsValidDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)((i << 24) | 0x22) );
        if (i & 0x5f) ok( !ret, "IsValidDpiAwarenessContext %#Ix succeeded\n", ((i << 24) | 0x22) );
        /* 0x20000000 is rejected on win11 */
        else if (!(i & 0x20)) ok( ret, "IsValidDpiAwarenessContext %#Ix failed\n", ((i << 24) | 0x22) );
    }
}

static void test_GetDpiFromDpiAwarenessContext(void)
{
    UINT ret;

    if (!pGetDpiFromDpiAwarenessContext)
    {
        win_skip( "GetDpiFromDpiAwarenessContext missing, skipping tests\n" );
        return;
    }

    ret = pGetDpiFromDpiAwarenessContext( 0 );
    ok( !ret, "GetDpiFromDpiAwarenessContext returned %u\n", ret );

    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x11 );
    ok( ret == 0, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x12 );
    ok( ret == 0, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x22 );
    ok( ret == 0, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( ret == 96, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6011 );
    ok( ret == 96, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6111 );
    ok( ret == 97, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x7811 );
    ok( ret == 120, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x10011 );
    ok( ret == 256, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x40006010 );
    ok( ret == 96, "GetDpiFromDpiAwarenessContext returned %u\n", ret );

    ret = pGetDpiFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    ok( ret == 96, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( ret == real_dpi, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    ok( ret == 0, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
    ok( ret == 0, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
    ret = pGetDpiFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED );
    ok( ret == 96, "GetDpiFromDpiAwarenessContext returned %u\n", ret );
}

static void test_AreDpiAwarenessContextsEqual(void)
{
    BOOL ret;

    if (!pAreDpiAwarenessContextsEqual)
    {
        win_skip( "AreDpiAwarenessContextsEqual missing, skipping tests\n" );
        return;
    }

    ret = pAreDpiAwarenessContextsEqual( 0, 0 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );

    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x12, (DPI_AWARENESS_CONTEXT)0x12 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x22, (DPI_AWARENESS_CONTEXT)0x22 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x6010, (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x6011, (DPI_AWARENESS_CONTEXT)0x6011 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x6111, (DPI_AWARENESS_CONTEXT)0x6111 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x7811, (DPI_AWARENESS_CONTEXT)0x7811 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x40006010, (DPI_AWARENESS_CONTEXT)0x40006010 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );

    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_UNAWARE, DPI_AWARENESS_CONTEXT_UNAWARE );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );

    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_UNAWARE, (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x6010, DPI_AWARENESS_CONTEXT_UNAWARE );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_UNAWARE, (DPI_AWARENESS_CONTEXT)0x10 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );

    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE, (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x11 | (real_dpi << 8)) );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)((UINT_PTR)0x11 | (real_dpi << 8)), DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE, (DPI_AWARENESS_CONTEXT)0x11 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );

    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE, (DPI_AWARENESS_CONTEXT)0x12 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x12, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2, (DPI_AWARENESS_CONTEXT)0x22 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x22, DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED, (DPI_AWARENESS_CONTEXT)0x40006010 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x40006010, DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED, (DPI_AWARENESS_CONTEXT)0x40000010 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );

    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x10, (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x11, (DPI_AWARENESS_CONTEXT)0x6011 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x12, (DPI_AWARENESS_CONTEXT)0x6012 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x22, (DPI_AWARENESS_CONTEXT)0x6022 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x20006010, (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x40006010, (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( !ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
    ret = pAreDpiAwarenessContextsEqual( (DPI_AWARENESS_CONTEXT)0x80006010, (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( ret, "AreDpiAwarenessContextsEqual returned %u\n", ret );
}

static void test_GetAwarenessFromDpiAwarenessContext(void)
{
    DPI_AWARENESS ret;

    if (!pGetAwarenessFromDpiAwarenessContext)
    {
        win_skip( "GetAwarenessFromDpiAwarenessContext missing, skipping tests\n" );
        return;
    }

    ret = pGetAwarenessFromDpiAwarenessContext( 0 );
    ok( ret == -1, "got %u\n", ret );

    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x12 );
    ok( ret == DPI_AWARENESS_PER_MONITOR_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x22 );
    ok( ret == DPI_AWARENESS_PER_MONITOR_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6010 );
    ok( ret == DPI_AWARENESS_UNAWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6011 );
    ok( ret == DPI_AWARENESS_SYSTEM_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x6111 );
    ok( ret == DPI_AWARENESS_SYSTEM_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x7811 );
    ok( ret == DPI_AWARENESS_SYSTEM_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x10011 );
    ok( ret == DPI_AWARENESS_SYSTEM_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x40006010 );
    ok( ret == DPI_AWARENESS_UNAWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80000012 );
    ok( ret == DPI_AWARENESS_PER_MONITOR_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80006010 );
    ok( ret == DPI_AWARENESS_UNAWARE, "got %u\n", ret );

    ret = pGetAwarenessFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    ok( ret == DPI_AWARENESS_UNAWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( ret == DPI_AWARENESS_SYSTEM_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    ok( ret == DPI_AWARENESS_PER_MONITOR_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 );
    ok( ret == DPI_AWARENESS_PER_MONITOR_AWARE, "got %u\n", ret );
    ret = pGetAwarenessFromDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED );
    ok( ret == DPI_AWARENESS_UNAWARE, "got %u\n", ret );
}

static void test_dpi_context(void)
{
    DPI_AWARENESS awareness;
    DPI_AWARENESS_CONTEXT context;
    BOOL ret;
    UINT dpi;
    HDC hdc = GetDC( 0 );

    context = pGetThreadDpiAwarenessContext();
    ok( context == (DPI_AWARENESS_CONTEXT)0x6010, "wrong context %p\n", context );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_UNAWARE, "wrong awareness %u\n", awareness );
    ok( !pIsProcessDPIAware(), "already aware\n" );
    dpi = pGetDpiForSystem();
    todo_wine_if (real_dpi != USER_DEFAULT_SCREEN_DPI)
        ok( dpi == USER_DEFAULT_SCREEN_DPI, "wrong dpi %u\n", dpi );
    dpi = GetDeviceCaps( hdc, LOGPIXELSX );
    todo_wine_if (real_dpi != USER_DEFAULT_SCREEN_DPI)
        ok( dpi == USER_DEFAULT_SCREEN_DPI, "wrong dpi %u\n", dpi );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( NULL );
    ok( !ret, "got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)-6 );
    ok( !ret, "got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    ret = pSetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( ret, "got %d\n", ret );
    ok( pIsProcessDPIAware(), "not aware\n" );
    real_dpi = pGetDpiForSystem();
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( !ret, "got %d\n", ret );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    ret = pSetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    ok( !ret, "got %d\n", ret );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError() );

    ret = pSetProcessDpiAwarenessInternal( DPI_AWARENESS_INVALID );
    ok( !ret, "got %d\n", ret );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    ret = pSetProcessDpiAwarenessInternal( DPI_AWARENESS_UNAWARE );
    ok( !ret, "got %d\n", ret );
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError() );
    ret = pGetProcessDpiAwarenessInternal( 0, &awareness );
    ok( ret, "got %d\n", ret );
    ok( awareness == DPI_AWARENESS_SYSTEM_AWARE, "wrong value %d\n", awareness );
    ret = pGetProcessDpiAwarenessInternal( GetCurrentProcess(), &awareness );
    ok( ret, "got %d\n", ret );
    ok( awareness == DPI_AWARENESS_SYSTEM_AWARE, "wrong value %d\n", awareness );
    SetLastError(0xdeadbeef);
    ret = pGetProcessDpiAwarenessInternal( (HANDLE)0xdeadbeef, &awareness );
    todo_wine
    ok( !ret || broken(ret) /* <= win10 1709 */, "got %d\n", ret );
    if (!ret)
    {
        ok( GetLastError() == ERROR_INVALID_PARAMETER, "got %lu\n", GetLastError() );
        ok( awareness == DPI_AWARENESS_INVALID, "wrong value %d\n", awareness );
    }
    else ok( awareness == DPI_AWARENESS_UNAWARE, "wrong value %d\n", awareness );

    ret = pIsProcessDPIAware();
    ok(ret, "got %d\n", ret);
    context = pGetThreadDpiAwarenessContext();
    ok( context == (DPI_AWARENESS_CONTEXT)0x6011, "wrong context %p\n", context );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_SYSTEM_AWARE, "wrong awareness %u\n", awareness );
    SetLastError( 0xdeadbeef );
    context = pSetThreadDpiAwarenessContext( 0 );
    ok( !context, "got %p\n", context );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    context = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)-6 );
    ok( !context, "got %p\n", context );
    ok( GetLastError() == ERROR_INVALID_PARAMETER, "wrong error %lu\n", GetLastError() );
    context = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    ok( context == (DPI_AWARENESS_CONTEXT)0x80006011, "wrong context %p\n", context );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_SYSTEM_AWARE, "wrong awareness %u\n", awareness );
    dpi = pGetDpiForSystem();
    ok( dpi == USER_DEFAULT_SCREEN_DPI, "wrong dpi %u\n", dpi );
    dpi = GetDeviceCaps( hdc, LOGPIXELSX );
    ok( dpi == USER_DEFAULT_SCREEN_DPI, "wrong dpi %u\n", dpi );
    ok( !pIsProcessDPIAware(), "still aware\n" );
    context = pGetThreadDpiAwarenessContext();
    ok( context == (DPI_AWARENESS_CONTEXT)0x6010, "wrong context %p\n", context );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_UNAWARE, "wrong awareness %u\n", awareness );
    context = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    ok( context == (DPI_AWARENESS_CONTEXT)0x6010, "wrong context %p\n", context );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_UNAWARE, "wrong awareness %u\n", awareness );
    dpi = pGetDpiForSystem();
    ok( dpi == real_dpi, "wrong dpi %u/%u\n", dpi, real_dpi );
    dpi = GetDeviceCaps( hdc, LOGPIXELSX );
    ok( dpi == real_dpi, "wrong dpi %u\n", dpi );
    context = pGetThreadDpiAwarenessContext();
    ok( context == (DPI_AWARENESS_CONTEXT)0x12, "wrong context %p\n", context );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_PER_MONITOR_AWARE, "wrong awareness %u\n", awareness );
    context = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
    ok( context == (DPI_AWARENESS_CONTEXT)0x12, "wrong context %p\n", context );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_PER_MONITOR_AWARE, "wrong awareness %u\n", awareness );
    dpi = pGetDpiForSystem();
    ok( dpi == real_dpi, "wrong dpi %u/%u\n", dpi, real_dpi );
    dpi = GetDeviceCaps( hdc, LOGPIXELSX );
    ok( dpi == real_dpi, "wrong dpi %u\n", dpi );
    ok( pIsProcessDPIAware(), "not aware\n" );
    context = pGetThreadDpiAwarenessContext();
    ok( context == (DPI_AWARENESS_CONTEXT)0x6011, "wrong context %p\n", context );
    context = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80006010 );
    ok( context == (DPI_AWARENESS_CONTEXT)0x6011, "wrong context %p\n", context );
    context = pGetThreadDpiAwarenessContext();
    ok( context == (DPI_AWARENESS_CONTEXT)0x6011, "wrong context %p\n", context );
    context = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x80006011 );
    ok( context == (DPI_AWARENESS_CONTEXT)0x80006011, "wrong context %p\n", context );
    context = pGetThreadDpiAwarenessContext();
    ok( context == (DPI_AWARENESS_CONTEXT)0x6011, "wrong context %p\n", context );
    context = pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)0x12 );
    ok( context == (DPI_AWARENESS_CONTEXT)0x80006011, "wrong context %p\n", context );
    context = pSetThreadDpiAwarenessContext( context );
    ok( context == (DPI_AWARENESS_CONTEXT)0x12, "wrong context %p\n", context );
    context = pGetThreadDpiAwarenessContext();
    ok( context == (DPI_AWARENESS_CONTEXT)0x6011, "wrong context %p\n", context );

    if (real_dpi != USER_DEFAULT_SCREEN_DPI) test_dpi_stock_objects( hdc );
    ReleaseDC( 0, hdc );
}

static LRESULT CALLBACK dpi_winproc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    DPI_AWARENESS_CONTEXT ctx = pGetWindowDpiAwarenessContext( hwnd );
    DPI_AWARENESS_CONTEXT ctx2 = pGetThreadDpiAwarenessContext();
    DWORD pos, pos2;

    ok( pGetAwarenessFromDpiAwarenessContext( ctx ) == pGetAwarenessFromDpiAwarenessContext( ctx2 ),
        "msg %04x wrong awareness %p / %p\n", msg, ctx, ctx2 );
    pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    pos = GetMessagePos();
    pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    pos2 = GetMessagePos();
    ok( pos == pos2, "wrong pos %08lx / %08lx\n", pos, pos2 );
    pSetThreadDpiAwarenessContext( ctx2 );
    return DefWindowProcA( hwnd, msg, wp, lp );
}

static void test_dpi_window(void)
{
    DPI_AWARENESS_CONTEXT context, orig;
    DPI_AWARENESS awareness;
    ULONG_PTR i, j;
    UINT dpi;
    HWND hwnd, child, ret;
    MSG msg = { 0, WM_USER + 1, 0, 0 };

    if (!pGetWindowDpiAwarenessContext)
    {
        win_skip( "GetWindowDpiAwarenessContext not supported\n" );
        return;
    }
    orig = pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    for (i = DPI_AWARENESS_UNAWARE; i <= DPI_AWARENESS_PER_MONITOR_AWARE; i++)
    {
        pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~i );
        hwnd = CreateWindowA( "DpiTestClass", "Test",
                              WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, 0, 0, GetModuleHandleA(0), NULL );
        ok( hwnd != 0, "failed to create window\n" );
        context = pGetWindowDpiAwarenessContext( hwnd );
        awareness = pGetAwarenessFromDpiAwarenessContext( context );
        ok( awareness == i, "%Iu: wrong awareness %u\n", i, awareness );
        dpi = pGetDpiForWindow( hwnd );
        ok( dpi == (i == DPI_AWARENESS_UNAWARE ? USER_DEFAULT_SCREEN_DPI : real_dpi),
            "%Iu: got %u / %u\n", i, dpi, real_dpi );
        if (pGetDpiForMonitorInternal)
        {
            BOOL res;
            SetLastError( 0xdeadbeef );
            res = pGetDpiForMonitorInternal( MonitorFromWindow( hwnd, 0 ), 0, &dpi, NULL );
            ok( !res, "succeeded\n" );
            ok( GetLastError() == ERROR_INVALID_ADDRESS, "wrong error %lu\n", GetLastError() );
            SetLastError( 0xdeadbeef );
            res = pGetDpiForMonitorInternal( MonitorFromWindow( hwnd, 0 ), 3, &dpi, &dpi );
            ok( !res, "succeeded\n" );
            ok( GetLastError() == ERROR_BAD_ARGUMENTS, "wrong error %lu\n", GetLastError() );
            SetLastError( 0xdeadbeef );
            res = pGetDpiForMonitorInternal( MonitorFromWindow( hwnd, 0 ), 3, &dpi, NULL );
            ok( !res, "succeeded\n" );
            ok( GetLastError() == ERROR_BAD_ARGUMENTS, "wrong error %lu\n", GetLastError() );
            res = pGetDpiForMonitorInternal( MonitorFromWindow( hwnd, 0 ), 0, &dpi, &dpi );
            ok( res, "failed err %lu\n", GetLastError() );
            ok( dpi == (i == DPI_AWARENESS_UNAWARE ? USER_DEFAULT_SCREEN_DPI : real_dpi),
                "%Iu: got %u / %u\n", i, dpi, real_dpi );
        }
        msg.hwnd = hwnd;
        for (j = DPI_AWARENESS_UNAWARE; j <= DPI_AWARENESS_PER_MONITOR_AWARE; j++)
        {
            pSetThreadDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~j );
            SendMessageA( hwnd, WM_USER, 0, 0 );
            DispatchMessageA( &msg );
            CallWindowProcA( dpi_winproc, hwnd, WM_USER + 2, 0, 0 );
            child = CreateWindowA( "DpiTestClass", "Test",
                                   WS_CHILD, 0, 0, 100, 100, hwnd, 0, GetModuleHandleA(0), NULL );
            context = pGetWindowDpiAwarenessContext( child );
            awareness = pGetAwarenessFromDpiAwarenessContext( context );
            ok( awareness == i, "%Iu/%Iu: wrong awareness %u\n", i, j, awareness );
            dpi = pGetDpiForWindow( child );
            ok( dpi == (i == DPI_AWARENESS_UNAWARE ? USER_DEFAULT_SCREEN_DPI : real_dpi),
                "%Iu/%Iu: got %u / %u\n", i, j, dpi, real_dpi );
            ret = SetParent( child, NULL );
            ok( ret != 0, "SetParent failed err %lu\n", GetLastError() );
            context = pGetWindowDpiAwarenessContext( child );
            awareness = pGetAwarenessFromDpiAwarenessContext( context );
            ok( awareness == i, "%Iu/%Iu: wrong awareness %u\n", i, j, awareness );
            dpi = pGetDpiForWindow( child );
            ok( dpi == (i == DPI_AWARENESS_UNAWARE ? USER_DEFAULT_SCREEN_DPI : real_dpi),
                "%Iu/%Iu: got %u / %u\n", i, j, dpi, real_dpi );
            DestroyWindow( child );
            child = CreateWindowA( "DpiTestClass", "Test",
                                   WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, 0, 0, GetModuleHandleA(0), NULL );
            context = pGetWindowDpiAwarenessContext( child );
            awareness = pGetAwarenessFromDpiAwarenessContext( context );
            ok( awareness == j, "%Iu/%Iu: wrong awareness %u\n", i, j, awareness );
            dpi = pGetDpiForWindow( child );
            ok( dpi == (j == DPI_AWARENESS_UNAWARE ? USER_DEFAULT_SCREEN_DPI : real_dpi),
                "%Iu/%Iu: got %u / %u\n", i, j, dpi, real_dpi );
            ret = SetParent( child, hwnd );
            ok( ret != 0 || GetLastError() == ERROR_INVALID_STATE,
                "SetParent failed err %lu\n", GetLastError() );
            context = pGetWindowDpiAwarenessContext( child );
            awareness = pGetAwarenessFromDpiAwarenessContext( context );
            ok( awareness == (ret ? i : j), "%Iu/%Iu: wrong awareness %u\n", i, j, awareness );
            dpi = pGetDpiForWindow( child );
            ok( dpi == (i == DPI_AWARENESS_UNAWARE ? USER_DEFAULT_SCREEN_DPI : real_dpi),
                "%Iu/%Iu: got %u / %u\n", i, j, dpi, real_dpi );
            DestroyWindow( child );
        }
        DestroyWindow( hwnd );
    }

    SetLastError( 0xdeadbeef );
    context = pGetWindowDpiAwarenessContext( (HWND)0xdeadbeef );
    ok( !context, "got %p\n", context );
    ok( GetLastError() == ERROR_INVALID_WINDOW_HANDLE, "wrong error %lu\n", GetLastError() );
    SetLastError( 0xdeadbeef );
    dpi = pGetDpiForWindow( (HWND)0xdeadbeef );
    ok( !dpi, "got %u\n", dpi );
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INVALID_WINDOW_HANDLE,
        "wrong error %lu\n", GetLastError() );

    SetLastError( 0xdeadbeef );
    context = pGetWindowDpiAwarenessContext( GetDesktopWindow() );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_PER_MONITOR_AWARE, "wrong awareness %u\n", awareness );
    dpi = pGetDpiForWindow( GetDesktopWindow() );
    ok( dpi == real_dpi, "got %u / %u\n", dpi, real_dpi );

    pSetThreadDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    SetLastError( 0xdeadbeef );
    context = pGetWindowDpiAwarenessContext( GetDesktopWindow() );
    awareness = pGetAwarenessFromDpiAwarenessContext( context );
    ok( awareness == DPI_AWARENESS_PER_MONITOR_AWARE, "wrong awareness %u\n", awareness );
    dpi = pGetDpiForWindow( GetDesktopWindow() );
    ok( dpi == real_dpi, "got %u / %u\n", dpi, real_dpi );

    pSetThreadDpiAwarenessContext( orig );
}

static void test_GetAutoRotationState(void)
{
    AR_STATE state;
    BOOL ret;

    if (!pGetAutoRotationState)
    {
        win_skip("GetAutoRotationState not supported\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pGetAutoRotationState(NULL);
    ok(!ret, "Expected GetAutoRotationState to fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    state = 0;
    ret = pGetAutoRotationState(&state);
    ok(ret, "Expected GetAutoRotationState to succeed, error %ld\n", GetLastError());
}

static void test_LOGFONT_charset(void)
{
    CHARSETINFO csi;
    LOGFONTA lf;
    NONCLIENTMETRICSA ncm;
    BOOL ret;

    ret = TranslateCharsetInfo(ULongToPtr(GetACP()), &csi, TCI_SRCCODEPAGE);
    ok(ret, "TranslateCharsetInfo(%d) error %lu\n", GetACP(), GetLastError());

    GetObjectA(GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    ok(lf.lfCharSet == csi.ciCharset, "got %d, expected %d\n", lf.lfCharSet, csi.ciCharset);

    ret = SystemParametersInfoA(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, FALSE);
    ok(ret, "SystemParametersInfoW error %lu\n", GetLastError());
    ok(lf.lfCharSet == DEFAULT_CHARSET, "got %d\n", lf.lfCharSet);

    ncm.cbSize = FIELD_OFFSET(NONCLIENTMETRICSA, iPaddedBorderWidth);
    ret = SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
    ok(ret, "SystemParametersInfoW error %lu\n", GetLastError());
    ok(ncm.lfCaptionFont.lfCharSet == DEFAULT_CHARSET, "got %d\n", ncm.lfCaptionFont.lfCharSet);
    ok(ncm.lfSmCaptionFont.lfCharSet == DEFAULT_CHARSET, "got %d\n", ncm.lfSmCaptionFont.lfCharSet);
}

START_TEST(sysparams)
{
    int argc;
    char** argv;
    WNDCLASSA wc;
    MSG msg;
    HDC hdc;
    HANDLE hThread;
    DWORD dwThreadId;
    HANDLE hInstance, hdll;

    hdll = GetModuleHandleA("user32.dll");
    pIsProcessDPIAware = (void*)GetProcAddress(hdll, "IsProcessDPIAware");
    pSetProcessDPIAware = (void*)GetProcAddress(hdll, "SetProcessDPIAware");
    pGetDpiForSystem = (void*)GetProcAddress(hdll, "GetDpiForSystem");
    pGetDpiForWindow = (void*)GetProcAddress(hdll, "GetDpiForWindow");
    pGetDpiForMonitorInternal = (void*)GetProcAddress(hdll, "GetDpiForMonitorInternal");
    pSetProcessDpiAwarenessContext = (void*)GetProcAddress(hdll, "SetProcessDpiAwarenessContext");
    pGetProcessDpiAwarenessInternal = (void*)GetProcAddress(hdll, "GetProcessDpiAwarenessInternal");
    pSetProcessDpiAwarenessInternal = (void*)GetProcAddress(hdll, "SetProcessDpiAwarenessInternal");
    pGetThreadDpiAwarenessContext = (void*)GetProcAddress(hdll, "GetThreadDpiAwarenessContext");
    pSetThreadDpiAwarenessContext = (void*)GetProcAddress(hdll, "SetThreadDpiAwarenessContext");
    pGetWindowDpiAwarenessContext = (void*)GetProcAddress(hdll, "GetWindowDpiAwarenessContext");
    pGetAwarenessFromDpiAwarenessContext = (void*)GetProcAddress(hdll, "GetAwarenessFromDpiAwarenessContext");
    pIsValidDpiAwarenessContext = (void*)GetProcAddress(hdll, "IsValidDpiAwarenessContext");
    pGetSystemMetricsForDpi = (void*)GetProcAddress(hdll, "GetSystemMetricsForDpi");
    pSystemParametersInfoForDpi = (void*)GetProcAddress(hdll, "SystemParametersInfoForDpi");
    pAdjustWindowRectExForDpi = (void*)GetProcAddress(hdll, "AdjustWindowRectExForDpi");
    pLogicalToPhysicalPointForPerMonitorDPI = (void*)GetProcAddress(hdll, "LogicalToPhysicalPointForPerMonitorDPI");
    pPhysicalToLogicalPointForPerMonitorDPI = (void*)GetProcAddress(hdll, "PhysicalToLogicalPointForPerMonitorDPI");
    pGetAutoRotationState = (void*)GetProcAddress(hdll, "GetAutoRotationState");
    pAreDpiAwarenessContextsEqual = (void*)GetProcAddress(hdll, "AreDpiAwarenessContextsEqual");
    pGetDpiFromDpiAwarenessContext = (void*)GetProcAddress(hdll, "GetDpiFromDpiAwarenessContext");

    hInstance = GetModuleHandleA( NULL );
    hdc = GetDC(0);
    dpi = GetDeviceCaps( hdc, LOGPIXELSY);
    real_dpi = get_real_dpi();
    ReleaseDC( 0, hdc);

    /* This test requires interactivity, if we don't have it, give up */
    if (!SystemParametersInfoA( SPI_SETBEEP, TRUE, 0, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE ) &&
        GetLastError()==ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION) return;

    argc = winetest_get_mainargs(&argv);
    strict=(argc >= 3 && strcmp(argv[2],"strict")==0);

    if (argc > 3 && !strcmp( argv[2], "SetProcessDpiAwarenessContext" ))
    {
        test_SetProcessDpiAwarenessContext( strtoul( argv[3], NULL, 16 ) );
        return;
    }

    trace("dpi %d real_dpi %d, strict %d\n", dpi, real_dpi, strict);

    test_LOGFONT_charset();

    trace("testing GetSystemMetrics with your current desktop settings\n");
    test_GetSystemMetrics( );
    test_metrics_for_dpi( 192 );
    test_EnumDisplaySettings( );
    test_GetSysColorBrush( );
    test_GetAutoRotationState( );
    test_SetThreadDpiAwarenessContext();
    test_IsValidDpiAwarenessContext();
    test_GetDpiFromDpiAwarenessContext();
    test_AreDpiAwarenessContextsEqual();
    test_GetAwarenessFromDpiAwarenessContext();

    change_counter = 0;
    change_last_param = 0;

    wc.lpszClassName = "SysParamsTestClass";
    wc.lpfnWndProc = SysParamsTestWndProc;
    wc.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIconA( 0, (LPCSTR)IDI_APPLICATION );
    wc.hCursor = LoadCursorA( 0, (LPCSTR)IDC_ARROW );
    wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wc.lpszMenuName = 0;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    RegisterClassA( &wc );
    wc.lpszClassName = "DpiTestClass";
    wc.lpfnWndProc = dpi_winproc;
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

    if (pSetThreadDpiAwarenessContext)
    {
        test_dpi_context();
        test_dpi_mapping();
        test_dpi_window();
    }
    else win_skip( "SetThreadDpiAwarenessContext not supported\n" );

    test_dpi_aware();

    if (!pSetProcessDpiAwarenessContext || !pSetThreadDpiAwarenessContext || !pGetThreadDpiAwarenessContext)
        win_skip( "SetProcessDpiAwarenessContext not supported\n" );
    else
    {
        run_in_process( argv, "SetProcessDpiAwarenessContext 0x6010" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0x11" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0x12" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0x22" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0x40006010" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0x80006010" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0x80000012" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0x80000022" );

        /* user32 allows abstract DPI contexts to be used */
        run_in_process( argv, "SetProcessDpiAwarenessContext 0xffffffff" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0xfffffffe" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0xfffffffd" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0xfffffffc" );
        run_in_process( argv, "SetProcessDpiAwarenessContext 0xfffffffb" );
    }
}
