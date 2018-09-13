/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migrate.cpp

Environment:

    WIN32 User Mode

--*/


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tchar.h>

#include "..\common\deskcmmn.h"
#include "..\desknt5\migrate.h"
#include "..\desknt5\settings.h"

// this will change when the .h is moved to a public location
#include "comp.h"

HINSTANCE hInstance;

/****************************************************************************\
*
* LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan )
*
*   If pszScan starts with pszTarget, then the function returns the first
* char of pszScan that follows the pszTarget; other wise it returns pszScan.
*
* eg: SubStrEnd("abc", "abcdefg" ) ==> "defg"
*     SubStrEnd("abc", "abZQRT" ) ==> "abZQRT"
*
\****************************************************************************/

LPTSTR
SubStrEnd(
    LPTSTR pszTarget,
    LPTSTR pszScan
    )
{

    int i;

    for (i = 0; pszScan[i] != TEXT('\0') && pszTarget[i] != TEXT('\0') &&
            CharUpper(CHARTOPSZ(pszScan[i])) ==
            CharUpper(CHARTOPSZ(pszTarget[i])); i++);

    if (pszTarget[i] == TEXT('\0')) {

        // we found the substring
        return pszScan + i;
    }

    return pszScan;
}

void 
SaveDisplaySettings(HKEY hKey)
{
    HDC     hDisplay;
    DWORD   cb;
    int     width, height, val, i;
    DWORD    useVga = FALSE;

    POINT res[] = {
        {  640,  480},
        {  800,  600},
        { 1024,  768},
        { 1152,  900},
        { 1280, 1024},
        { 1600, 1200},
        { 0, 0}         // end of table
        };

    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);

    if (width == 0 || height == 0) {
        //
        // Something went wrong, default to lowest common res
        //
        useVga = TRUE;
    }
    //
    // NT 4.0 multimon via driver vendor, not the OS ... adjust the width and height
    // back to normal values.  Once setup is complete, the second card will come 
    // on line and it will be taken care of.  In both cases, the video area must
    // be rectangular, not like MM on 5.0 where we can have "holes"
    //
    else if (width >= 2 * height) {
        // wide
        for (i = 0; res[i].x != 0; i++) {
            if (res[i].y == height) {
                width = res[i].x;
                break;
            }
        }

        useVga  = (res[i].x == 0); 
    }
    else if (height > width) {
        // tall
        for (i = 0; res[i].x != 0; i++) {
            if (res[i].x == width) {
                height = res[i].y;
                break;
            }
        }
    
        useVga  = (res[i].x == 0); 
    }

    if (useVga) {

        // no match, default to VGA
        // save the orig values first though
        cb = sizeof(width);
        RegSetValueEx(hKey, TEXT("OrigPelsWidth"), 0, REG_DWORD, (PBYTE) &width, cb);
    
        cb = sizeof(height);
        RegSetValueEx(hKey, TEXT("OrigPelsHeight"), 0, REG_DWORD, (PBYTE) &height, cb);

        width = 640;
        height = 480;

        cb  = sizeof(useVga);
        RegSetValueEx(hKey, SZ_UPGRADE_MODIFIED_SETTINGS, 0, REG_DWORD, (PBYTE) &useVga, cb);
    }

    cb = sizeof(width);
    RegSetValueEx(hKey, SZ_UPGRADE_FROM_PELS_WIDTH, 0, REG_DWORD, (PBYTE) &width, cb);

    cb = sizeof(height);
    RegSetValueEx(hKey, SZ_UPGRADE_FROM_PELS_HEIGHT, 0, REG_DWORD, (PBYTE) &height, cb);

    hDisplay = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    val = GetDeviceCaps(hDisplay, BITSPIXEL);
    cb = sizeof(val);
    RegSetValueEx(hKey, SZ_UPGRADE_FROM_BITS_PER_PEL, 0, REG_DWORD, (PBYTE) &val, cb);

    val = GetDeviceCaps(hDisplay, VREFRESH);
    cb = sizeof(val);
    RegSetValueEx(hKey, SZ_UPGRADE_FROM_DISPLAY_FREQ, 0, REG_DWORD, (PBYTE) &val, cb);

    DeleteDC(hDisplay);
}

void
SaveOsInfo(HKEY hKey)
{
    OSVERSIONINFO   os;
    DWORD           cb;

    ZeroMemory(&os, sizeof(OSVERSIONINFO));
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&os)) {
        return;
    }

#if 0
    //
    // If we want to do something particular for each platform we upgrade from,
    // do it here.
    //
    // Note, I think we run a clean install from win9x and win31, but populate
    // an answer file for settings we want to migrate.  We don't deal w/that
    // yet.
    //
    switch (os.dwPlatformId) {
    // upgrade from NT
    case VER_PLATFORM_WIN32_NT:
        break;

    // upgrade from win9X
    case VER_PLATFORM_WIN32_WINDOWS:
        break;

    // upgrade from win3.1
    case VER_PLATFORM_WIN32s:
        break;

    default:
        break;
    }
#endif

    //
    // Can't just dump the struct into the registry b/c of the size difference
    // between CHAR and WCHAR (ie, szCSDVersion)
    //
    cb = sizeof(DWORD);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_PLATFORM,
                  0,
                  REG_DWORD,
                  (PBYTE) &os.dwPlatformId,
                  cb);

    cb = sizeof(DWORD);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_MAJOR_VERSION,
                  0,
                  REG_DWORD,
                  (PBYTE) &os.dwMajorVersion,
                  cb);

    cb = sizeof(DWORD);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_MINOR_VERSION,
                  0,
                  REG_DWORD,
                  (PBYTE) &os.dwMinorVersion,
                  cb);

    cb = sizeof(DWORD);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_BUILD_NUMBER, 
                  0,
                  REG_DWORD,
                  (PBYTE) &os.dwBuildNumber,
                  cb);

    cb = lstrlen(os.szCSDVersion);
    RegSetValueEx(hKey,
                  SZ_UPGRADE_FROM_VERSION_DESC, 
                  0,
                  REG_SZ,
                  (PBYTE) os.szCSDVersion,
                  cb);
}

const TCHAR szWhackDevice[] = TEXT("\\Device");

void 
SaveLegacyDriver(HKEY hKey)
{
    LPTSTR pszEnd;
    HKEY   hKeyMap, hKeyDriver;
    int    i = 0, num = 0;
    TCHAR  szValueName[128], szData[128];
    PTCHAR szPath;
    DWORD  cbValue, cbData, dwType;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_VIDEOMAP,
                     0,
                     KEY_READ,
                     &hKeyMap) !=  ERROR_SUCCESS) {
        return;
    }

    for (cbValue = (sizeof(szValueName) - 1) / sizeof(TCHAR),
         cbData = sizeof(szData) / sizeof(TCHAR) ;

         RegEnumValue(hKeyMap, i++, szValueName, &cbValue, NULL, &dwType,
                      (PBYTE) szData, &cbData) != ERROR_NO_MORE_ITEMS ;

         cbValue = (sizeof(szValueName) - 1) / sizeof(TCHAR),
         cbData = sizeof(szData) / sizeof(TCHAR) ) {

        if (REG_SZ != dwType ||
            _tcsicmp(szData, TEXT("VgaSave")) == 0) {
            continue;
        }

        //
        // Make sure the value's name is \Device\XxxY
        //
        if (cbValue < (DWORD) lstrlen(szWhackDevice) ||
            _tcsnicmp(szValueName, szWhackDevice, lstrlen(szWhackDevice))) {  
            continue;
        }

        szPath = SubStrEnd(SZ_REGISTRYMACHINE, szData);
    
        for (pszEnd = szPath + lstrlen(szPath);
             pszEnd != szPath && *pszEnd != TEXT('\\'); 
             pszEnd--) {
            ; /* nothing */
        }
             
        //
        // Remove the \DeviceX at the end of the path
        //
        *pszEnd = TEXT('\0');
    
        //
        // First check if their is a binary name in there that we should use.
        //
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         szPath,
                         0,
                         KEY_READ,
                         &hKeyDriver) ==  ERROR_SUCCESS) {
    
            //
            // parse the device map and open the registry.
            //
            cbValue = sizeof(szValueName);
            if (RegQueryValueEx(hKeyDriver,
                                TEXT("ImagePath"),
                                NULL,
                                NULL,
                                (LPBYTE) szValueName,
                                &cbValue) == ERROR_SUCCESS) {
                //
                // The is a binary, extract the name, which will be of the form
                // ...\driver.sys
                //
                LPTSTR pszDriver, pszDriverEnd;

                pszDriver = szValueName;
                pszDriverEnd = pszDriver + lstrlen(pszDriver);

                while(pszDriverEnd != pszDriver &&
                      *pszDriverEnd != TEXT('.')) {
                    pszDriverEnd--;
                }

                *pszDriverEnd = UNICODE_NULL;

                while(pszDriverEnd != pszDriver &&
                      *pszDriverEnd != TEXT('\\')) {
                    pszDriverEnd--;
                }

                pszDriverEnd++;

                //
                // If pszDriver and pszDriverEnd are different, we now
                // have the driver name.
                //
                if (pszDriverEnd > pszDriver) {
                    if (_tcsicmp(pszDriverEnd, TEXT("vga")) != 0) {
                        RegCloseKey(hKeyDriver);
                        continue;
                    }

                    wsprintf(szValueName, TEXT("Driver%d"), num);
                    cbValue = lstrlen(pszDriverEnd);
                    RegSetValueEx(hKey,
                                  szValueName,
                                  0,
                                  REG_SZ,
                                  (PBYTE) pszDriverEnd,
                                  cbValue);
                }
            }
    
            RegCloseKey(hKeyDriver);
        }

        //
        // Get the actual service name
        //
        for( ; pszEnd > szPath && *pszEnd != TEXT('\\'); pszEnd--) {
            ;
        }
        pszEnd++;

        //
        // Save the service name
        //
        wsprintf(szValueName, TEXT("Service%d"), num++);
        cbValue = lstrlen(pszEnd);
        RegSetValueEx(hKey,
                      szValueName, 
                      0,
                      REG_SZ,
                      (PBYTE) pszEnd, 
                      cbValue);
    }

    cbValue = sizeof(DWORD);
    RegSetValueEx(hKey, 
                  TEXT("NumDrivers"),
                  0,
                  REG_DWORD,
                  (PBYTE) &num,
                  cbValue);
    
    RegCloseKey(hKeyMap);
}

BOOL
VideoUpgradeCheck(
    PCOMPAIBILITYCALLBACK CompatibilityCallback,
    LPVOID                Context
    )
{
    DWORD disposition;
    HKEY  hKey;

    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                       SZ_UPDATE_SETTINGS,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hKey,
                       &disposition) != ERROR_SUCCESS) {
        //
        // Oh well, guess we can't write it, no big deal
        //
        return FALSE;
    }

    //
    // Get the current device caps and store them away for the display applet
    // to apply later
    //
    SaveDisplaySettings(hKey);
    SaveOsInfo(hKey);
    SaveLegacyDriver(hKey);

    RegCloseKey(hKey);

    return FALSE;
}

extern "C" {

BOOL APIENTRY 
DllMain(HINSTANCE hDll,
        DWORD dwReason, 
        LPVOID lpReserved)
{
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        hInstance = hDll;
        break;

    case DLL_PROCESS_DETACH:
    	break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    default:
    	break;
    }

    return TRUE;
}

}
