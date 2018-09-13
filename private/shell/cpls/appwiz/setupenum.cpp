//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: setupenum.cpp
//
// The current order of enumeration is whatever order we read from the registry
//
// History:
//         6-11-98  by toddb
//------------------------------------------------------------------------
#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include <shellp.h>     // for IsUserAnAdmin
#include "setupenum.h"
#include "appwizid.h"

#define c_szOCSetupKey  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Setup\\OCManager\\ToDoList")

//-----------------------------------------------------------------------
// OCSetupApp
//-----------------------------------------------------------------------

COCSetupApp::COCSetupApp()
{
    // This must be heap alloced so everything should be zero'ed out.
    // Make sure this wasn't stack alloced using these asserts:
    ASSERT(0 == _szDisplayName[0]);
    ASSERT(0 == _szApp[0]);
    ASSERT(0 == _szArgs[0]);
}

COCSetupApp::~COCSetupApp()
{
}

//-----------------------------------------------------------------------
// GetAppInfo
//
// Fills in the only valid field in our psuedo APPINFODATA structure.

BOOL COCSetupApp::GetAppInfo(PAPPINFODATA pai)
{
    if (pai->cbSize != SIZEOF(APPINFODATA))
        return FALSE;

    DWORD dwInfoFlags = pai->dwMask;
    pai->dwMask = 0;
    
    if (dwInfoFlags & AIM_DISPLAYNAME)
    {
        if (SUCCEEDED(SHStrDup(_szDisplayName, &pai->pszDisplayName)))
            pai->dwMask |= AIM_DISPLAYNAME;
    }
    return TRUE;
}

//-----------------------------------------------------------------------
// ReadFromKey
//
// This function reads the actual data from the given reg key.  It returns
// TRUE if all required fields contained string data.

BOOL COCSetupApp::ReadFromKey( HKEY hkey )
{
    DWORD dwType;
    DWORD dwSize;

    dwSize = sizeof(_szDisplayName);
    if ( ERROR_SUCCESS != RegQueryValueEx( hkey, TEXT("Title"), 0, &dwType, (LPBYTE)_szDisplayName, &dwSize ) ||
         dwType != REG_SZ )
    {
        // DisplayName is required
        return FALSE;
    }

    dwSize = sizeof(_szApp);
    if ( ERROR_SUCCESS == RegQueryValueEx( hkey, TEXT("ConfigCommand"), 0, &dwType, (LPBYTE)_szApp, &dwSize ) &&
         (dwType == REG_SZ || dwType == REG_EXPAND_SZ) )
    {
        if ( dwType == REG_EXPAND_SZ )
        {
            TCHAR szBuf[MAX_PATH];
            ExpandEnvironmentStrings(_szApp, szBuf, ARRAYSIZE(szBuf));
            lstrcpyn(_szApp, szBuf, ARRAYSIZE(_szApp));
        }
    }
    else
    {
        // ConfigCommand is required
        return FALSE;
    }

    dwSize = sizeof(_szArgs);
    if ( ERROR_SUCCESS == RegQueryValueEx( hkey, TEXT("ConfigArgs"), 0, &dwType, (LPBYTE)_szArgs, &dwSize ) &&
         (dwType == REG_SZ || dwType == REG_EXPAND_SZ) )
    {
        if ( dwType == REG_EXPAND_SZ )
        {
            TCHAR szBuf[MAX_PATH];
            ExpandEnvironmentStrings(_szArgs, szBuf, ARRAYSIZE(szBuf));
            lstrcpyn(_szArgs, szBuf, ARRAYSIZE(_szArgs));
        }
    }
    else
    {
        // This is optional so we don't fail.  Instead simply insure that _szArgs is an empty string.
        _szArgs[0] = 0;
    }

    return TRUE;
}

BOOL COCSetupApp::Run()
{
    // BUGBUG: (stephstm, 03/17/99) we should probably wait on a job object in case
    // the spawned process spawns some other process(es) and then exits before them.

    BOOL fRet = FALSE;
    SHELLEXECUTEINFO sei = {0};

    sei.cbSize = sizeof(SHELLEXECUTEINFO);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = GetDesktopWindow();
    sei.lpFile = _szApp;
    sei.lpParameters = _szArgs[0] ? _szArgs : NULL;
    sei.nShow = SW_SHOWDEFAULT; 

    fRet = ShellExecuteEx(&sei);

    if (fRet)
    {
        DWORD dwRet;

        do
        {
            MSG msg;

            // Get and process the messages!
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // MsgWaitForMultipleObjects can fail with -1 being returned!
            dwRet = MsgWaitForMultipleObjects(1, &sei.hProcess, FALSE, INFINITE, QS_ALLINPUT);
        }
        while ((WAIT_OBJECT_0 != dwRet) && (-1 != dwRet));

        // Did MsgWait... failed?
        if (-1 == dwRet)
        {
            // Yes, kill the process
            TerminateProcess(sei.hProcess, 0);

            fRet = FALSE;
        }

        CloseHandle(sei.hProcess);
    }
    else
    {
        ShellMessageBox(HINST_THISDLL, sei.hwnd,  MAKEINTRESOURCE( IDS_CONFIGURE_FAILED ),
                             MAKEINTRESOURCE( IDS_NAME ),
                             MB_OK | MB_ICONEXCLAMATION);
    }

    return fRet;
}


//-----------------------------------------------------------------------
// OCSetupEnum
//-----------------------------------------------------------------------

COCSetupEnum::COCSetupEnum()
{
    _hkeyRoot = 0;
    _iRegEnumIndex = -1;
}

COCSetupEnum::~COCSetupEnum()
{
    if ( _hkeyRoot )
    {
        RegCloseKey( _hkeyRoot );
    }
}

//-----------------------------------------------------------------------
// s_OCSetupNeeded
//
// This checks for the neccessaary conditions to display the OC Setup portion of the ARP.
// This section is only shown if the current user is a member of the administrators group
// AND there are any items listed in the registry that need to be displayed.

BOOL COCSetupEnum::s_OCSetupNeeded()
{
    BOOL fResult = FALSE;
    HKEY hkey;
    // Temporarily open the reg key to see if it exists and has any sub keys
    if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szOCSetupKey, 0, KEY_READ, &hkey ) )
    {
        TCHAR szBuf[MAX_PATH];
        if ( ERROR_SUCCESS == RegEnumKey( hkey, 0, szBuf, sizeof(szBuf) ) )
        {
            // Yes, there are OCSetup items, but is the current user an administrator?
            if ( IsUserAnAdmin() )
            {
                fResult = TRUE;
            }
        }
        RegCloseKey( hkey );
    }
    return fResult;
}

//-----------------------------------------------------------------------
// EnumOCSetupItems
//
// This begins the enumeration by opening the required registry key.  This does
// not attempt to read any of the sub items so there is no garentee that the
// first call to Next() will succeed.

BOOL COCSetupEnum::EnumOCSetupItems()
{
    ASSERT( NULL == _hkeyRoot );
    // Open the reg key, return true if it's open.  We leave the key open until
    // our destructor is called since we need this key to do the enumeration.
    if ( ERROR_SUCCESS == RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            c_szOCSetupKey,
            0,
            KEY_READ,
            &_hkeyRoot ) )
    {
        return TRUE;
    }
    return FALSE;
}

//-----------------------------------------------------------------------
// Next
//
// Reads the data from the next sub key of _hkeyRoot and returns the data in the
// out pointer.  Returns TRUE if the out pointer is a valid COCSetupApp object.

BOOL COCSetupEnum::Next(COCSetupApp **ppocsa)
{
    HKEY hkeySub;
    TCHAR szSubKeyName[MAX_PATH];

    // We open each subkey of the root key and attempt to read an OCSetup item from the subkey.
    if ( ERROR_SUCCESS == RegEnumKey( _hkeyRoot, ++_iRegEnumIndex, szSubKeyName, sizeof(szSubKeyName) ) )
    {
        if ( ERROR_SUCCESS == RegOpenKeyEx( _hkeyRoot, szSubKeyName, 0, KEY_READ, &hkeySub ) )
        {
            *ppocsa = new COCSetupApp();
            if ( *ppocsa )
            {
                if ( (*ppocsa)->ReadFromKey( hkeySub ) )
                {
                    RegCloseKey( hkeySub );
                    return TRUE;
                }

                delete *ppocsa;
            }
            RegCloseKey( hkeySub );
        }
        // fall through
    }

    *ppocsa = NULL;
    return FALSE;
}

#endif //DOWNLEVEL_PLATFORM
