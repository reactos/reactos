//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* vrsreg.cpp - stub exe to register the virus scanner engine.             *
//*                                                                         *
//* created 10-15-96 inateeg                                                *
//*                                                                         *
//***************************************************************************

//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include <windows.h>
#include <windowsx.h>
#include <olectl.h>
#include <olectlid.h>
#include <initguid.h>
#include "vrsscan.h"
#include "resource.h"
#include "advpub.h"

//***************************************************************************
//* GLOBAL VARIABLES                                                        *
//***************************************************************************
BOOL    g_bRegister;
DWORD   g_dwFlags;
CLSID   g_clsid;
HINSTANCE g_hInst;

#define MAX_STRING      512
#define STR_OLESTR      1

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

// internal use as ANSI string only
#define VENDOR_REG      "CLSID\\%s\\VirusScanner"
#define SCANNER_COOKIE  "Cookie"

// predefined built in INF section name,  ANSI string required
#define SETUP_SECTION1  "InstallOCXs"
#define SETUP_SECTION2  "InstallRegs"
#define SETUP_UNINSTALL "Uninstall"
#define SETUP_TITLE     "Virus Scanner Setup"

// default macfee info
#define MACFEE_CLSID    TEXT("{91B0C1B0-0B71-11d0-8217-00A02474294C}")
#define DEFAULT_ENGINE  TEXT("McAfee")

BOOL Init( HINSTANCE hInstance, LPTSTR lpszCmdLine, INT nCmdShow );
BOOL ParseCmdLine( LPTSTR lpszCmdLine );
LPWSTR MakeWideStrFromAnsi( LPSTR psz, BYTE  bType );
LPSTR MakeAnsiStrFromWide( LPWSTR pwsz,  BYTE  bType );
void SaveRestoreCookie( CLSID clsid, DWORD *pdwCookie, BOOL bSave );
void ErrorMsg( HWND hWnd, UINT idErr );
HRESULT RegInstall( HINSTANCE hInst );
BOOL GetWideString( HINSTANCE hInstance, UINT id, LPWSTR *lplpwsStr );

//***************************************************************************
//*                                                                         *
//* NAME:       WinMain                                                     *
//*                                                                         *
//* SYNOPSIS:   Main entry point for the program.                           *
//*                                                                         *
//* REQUIRES:   hInstance:                                                  *
//*             hPrevInstance:                                              *
//*             lpszCmdLine:                                                *
//*             nCmdShow:                                                   *
//*                                                                         *
//* RETURNS:    int:                                                        *
//*                                                                         *
//***************************************************************************
INT WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPTSTR lpszCmdLine, INT nCmdShow )
{
    HRESULT hr = E_FAIL;
    IRegisterVirusScanEngine *pIRegVirusScanner = NULL;
    DWORD   dwCookie = 0;
    LPWSTR  lpwsDesc = NULL;

    if ( Init( hInstance, lpszCmdLine, nCmdShow ) )
    {
        if ( !GetWideString( hInstance, IDS_VRSENG_DESC, &lpwsDesc ) )
            return hr;  //bail out

        CoInitialize( NULL );

        hr = CoCreateInstance( CLSID_VirusScan, NULL, CLSCTX_INPROC_SERVER, IID_IRegisterVirusScanEngine, (void**) &pIRegVirusScanner );
        if( SUCCEEDED(hr) )
        {
            if ( g_bRegister )
            {
                // Do register OCX and add Reg first
                if ( RegInstall( hInstance ) == S_OK )
                {
                    hr = pIRegVirusScanner->RegisterScanEngine( g_clsid, lpwsDesc, g_dwFlags, 0, &dwCookie );
                    SaveRestoreCookie( g_clsid, &dwCookie, TRUE );
                }
            }
            else
            {
                SaveRestoreCookie( g_clsid, &dwCookie, FALSE );
                pIRegVirusScanner->UnRegisterScanEngine( g_clsid, lpwsDesc, g_dwFlags, 0, dwCookie );
                // Do Unregister OCX and del Reg
                hr = RegInstall( hInstance );
            }
        }
        else
        {
             ErrorMsg( NULL, IDS_ERR_CREATINSTANCE );
        }
        CoUninitialize();
    }

    if ( lpwsDesc )
        CoTaskMemFree( lpwsDesc );

    return hr;
}

//***************************************************************************
//*                                                                         *
//* NAME:       Init                                                        *
//*                                                                         *
//* SYNOPSIS:   Initialization for the program is done here.                *
//*                                                                         *
//* REQUIRES:   hInstance:                                                  *
//*             hPrevInstance:                                              *
//*             lpszCmdLine:                                                *
//*             nCmdShow:                                                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL Init( HINSTANCE hInstance, LPTSTR lpszCmdLine, INT nCmdShow )
{
    TCHAR  szTmpBuf[MAX_PATH];
    LPWSTR pwszClsid;
    g_hInst = hInstance;
    HRESULT hr;

    g_dwFlags = 0;

    if ( !ParseCmdLine(lpszCmdLine) )
    {
        ErrorMsg( NULL, IDS_ERR_BADCMDLINE );
        return FALSE;
    }

    if ( LoadString( hInstance, IDS_VRSENG_CLSID, szTmpBuf, ARRAYSIZE(szTmpBuf) ) )
    {
#ifdef UNICODE
        pwszClsid = szTmpBuf;
#else
        pwszClsid = MakeWideStrFromAnsi( szTmpBuf, STR_OLESTR );
#endif
        hr = CLSIDFromString( pwszClsid, &g_clsid );

#ifndef UNICODE
        CoTaskMemFree( pwszClsid );
#endif
        if ( FAILED( hr) )
            return FALSE;
    }

    if ( lstrcmpi( szTmpBuf, MACFEE_CLSID ) == 0 )
           g_dwFlags |= SFV_DONTDOUI;

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//*  ParseCmdLine()                                                     *
//*                                                                         *
//*  Purpose:    Parses the command line looking for switches               *
//*                                                                         *
//*  Parameters: LPSTR lpszCmdLineOrg - Original command line               *
//*                                                                         *
//*                                                                         *
//*  Return:     (BOOL) TRUE if successful                                  *
//*                     FALSE if an error occurs                            *
//*                                                                         *
//***************************************************************************
BOOL ParseCmdLine( LPTSTR lpszCmdLine )
{
    LPTSTR pArg;
    LPWSTR ;

    if( (!lpszCmdLine) || (lpszCmdLine[0] == 0) )
       return FALSE;

    pArg = strtok( lpszCmdLine, TEXT(" ") );

    while ( pArg )
    {

       if ( lstrcmpi( pArg, TEXT("/U") ) == 0 )
       {
           g_bRegister = FALSE;
       }
       else if ( lstrcmpi( pArg, TEXT("/R") ) == 0 )
       {
           g_bRegister = TRUE;
       }
       else
       {
           return FALSE;
       }
       pArg = strtok( NULL, TEXT(" ") );
    }

    return TRUE;
}

void SaveRestoreCookie( CLSID clsid, DWORD *pdwCookie, BOOL bSave )
{
    char    szBuf[MAX_PATH];
    LPOLESTR  pwsClsid;
    HKEY    hKey;
    LPSTR   pszClsid;
    DWORD   dwSize;

    StringFromCLSID( clsid, &pwsClsid );
    pszClsid = MakeAnsiStrFromWide( pwsClsid, STR_OLESTR );
    CoTaskMemFree( pwsClsid );

    wsprintf( szBuf, VENDOR_REG, pszClsid );
    if ( RegOpenKeyExA( HKEY_CLASSES_ROOT, szBuf, 0, KEY_ALL_ACCESS, &hKey ) == ERROR_SUCCESS )
    {
        if ( bSave )
        {
            RegSetValueExA( hKey, SCANNER_COOKIE, 0, REG_DWORD, (LPBYTE)pdwCookie, sizeof(DWORD) );
        }
        else
        {
            dwSize = sizeof(DWORD);
            *pdwCookie = 0;
            RegQueryValueExA( hKey, SCANNER_COOKIE, NULL, NULL, (LPBYTE)pdwCookie, &dwSize );
        }
        RegCloseKey( hKey );
    }

    CoTaskMemFree( pszClsid );
}

//******************************************************************************
//*  CreateInfFile - Create an INF file from an hmodule
//*
//*  ENTRY
//*      hInst - hmodule that contains the REGINST resource
//*      pszInfFileName - OUT the location to get the INF filename
//*      pszDir - OUT dir directory the INF file
//*
//*  EXIT
//*      HRESULT
//******************************************************************************
HRESULT CreateInfFile( HINSTANCE hInst, LPTSTR pszInfFileName, LPTSTR pszDir )
{
    HRESULT hr = E_FAIL;
    TCHAR szInfFilePath[MAX_PATH];
    LPVOID pvInfData;
    HRSRC hrsrcInfData;
    DWORD cbInfData, cbWritten;
    HANDLE hfileInf = INVALID_HANDLE_VALUE;

    if ( GetTempPath(ARRAYSIZE(szInfFilePath), szInfFilePath) > ARRAYSIZE(szInfFilePath))
    {
        goto Cleanup;
    }
    lstrcpy( pszDir, szInfFilePath );

    if (GetTempFileName(szInfFilePath, TEXT("RGI"), 0, pszInfFileName) == 0)
    {
        goto Cleanup;
    }

    hrsrcInfData = FindResource(hInst, TEXT("REGINST"), TEXT("REGINST"));
    if (hrsrcInfData == NULL)
    {
        goto Cleanup;
    }

    cbInfData = SizeofResource(hInst, hrsrcInfData);

    pvInfData = LockResource(LoadResource(hInst, hrsrcInfData));
    if (pvInfData == NULL)
    {
        goto Cleanup;
    }

    hfileInf = CreateFile(pszInfFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                          FILE_ATTRIBUTE_NORMAL, NULL);
    if (hfileInf == INVALID_HANDLE_VALUE)
    {
        goto Cleanup;
    }

    if ((WriteFile(hfileInf, pvInfData, cbInfData, &cbWritten, NULL) == FALSE) ||
        (cbWritten != cbInfData))
    {
        goto Cleanup;
    }

    hr = S_OK;

Cleanup:
    if (hfileInf != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hfileInf);
    }

    return hr;
}

//********************************************************************************
//*  RegInstall - Install a registry INF
//*
//*
//********************************************************************************

HRESULT RegInstall( HINSTANCE hInst )
{
    HRESULT hr = E_FAIL;
    TCHAR szInfFileName[MAX_PATH];
    HINSTANCE hLib;
    TCHAR szPath[MAX_PATH];
    LPTSTR lpTmp;
    LPSTR  lpInfFile = NULL, lpDir = NULL;
    RUNSETUPCOMMAND pfRunSetupCommand;


    //
    // Load advpack.dll.
    //
    szPath[0] = 0;
    GetSystemDirectory( szPath, ARRAYSIZE(szPath) );
    lpTmp = CharPrev(szPath, szPath+lstrlen(szPath) );
    if ( (lpTmp > szPath) && (*lpTmp != TEXT('\\') ) )
    {
       lstrcat( szPath, TEXT("\\") );
    }
    lstrcat( szPath, TEXT("advpack.dll") );
    hLib = LoadLibrary( szPath );
    if ( !hLib )
    {
       ErrorMsg( NULL, IDS_ERR_LOADADV );
       hr = S_FALSE;
       goto Cleanup;
    }

    pfRunSetupCommand = (RUNSETUPCOMMAND)GetProcAddress( hLib, TEXT("RunSetupCommand") );
    if ( !pfRunSetupCommand )
    {
       ErrorMsg( NULL, IDS_ERR_GETPROCADD );
       hr = S_FALSE;
       goto Cleanup;
    }

    //
    // Create the INF file.
    //
    szInfFileName[0] = TEXT('\0');
    szPath[0] = TEXT('\0');
    hr = CreateInfFile(hInst, szInfFileName, szPath );
    if (FAILED(hr))
    {
        goto Cleanup;
    }

#ifdef UNICODE
    lpInfFile = MakeAnsiStrFromWide( szInfFileName, STR_OLESTR );
    lpDir =  MakeAnsiStrFromWide( szPath, STR_OLESTR );
#else
    lpInfFile = szInfFileName;
    lpDir = szPath;
#endif

    //
    // Execute the INF engine on the INF.
    //
    if ( g_bRegister )
    {
        hr = pfRunSetupCommand( NULL, lpInfFile, SETUP_SECTION1, lpDir,
                                SETUP_TITLE, NULL,
                                RSC_FLAG_INF|RSC_FLAG_QUIET|RSC_FLAG_NGCONV, NULL);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        hr = pfRunSetupCommand( NULL, lpInfFile, SETUP_SECTION2, lpDir,
                                SETUP_TITLE, NULL,
                                RSC_FLAG_INF|RSC_FLAG_QUIET|RSC_FLAG_NGCONV, NULL);
    }
    else
        hr = pfRunSetupCommand( NULL, lpInfFile, SETUP_UNINSTALL, lpDir,
                                SETUP_TITLE, NULL,
                                RSC_FLAG_INF|RSC_FLAG_QUIET|RSC_FLAG_NGCONV, NULL);

    if (FAILED(hr))
    {
        goto Cleanup;
    }
Cleanup:

    //
    // Delete the INF file, Free library.
    //
    if ( hLib )
        FreeLibrary( hLib );

    if (szInfFileName[0] )
    {
        DeleteFile(szInfFileName);
    }

#ifdef UNICODE
    if ( lpInfFile )
    {
        CoTaskMemFree( lpInfFile );     
    }

    if ( lpDir )
    {
        CoTaskMemFree( lpDir );     
    }
#endif

    return hr;
}

BOOL GetWideString( HINSTANCE hInstance, UINT id, LPWSTR *lplpwsStr )
{
    LPTSTR lpStr;
    
    lpStr = (LPTSTR)CoTaskMemAlloc( MAX_STRING );
    if ( !lpStr )
        return FALSE;

    if ( LoadString( hInstance, id, lpStr, MAX_STRING/sizeof(TCHAR) ) )
    {
#ifdef UNICODE
        *lplpwsStr = lpStr;
#else
        *lplpwsStr = MakeWideStrFromAnsi( lpStr, STR_OLESTR );
#endif

#ifndef UNICODE
        CoTaskMemFree( lpStr );
#endif
        if ( *lplpwsStr == 0 )
            return FALSE;
    }
    return TRUE;
}

//=--------------------------------------------------------------------------=
// given a string, make a BSTR out of it.
//
// Parameters:
//    LPSTR         - [in]
//    BYTE          - [in]
//
// Output:
//    LPWSTR        - needs to be cast to final desired result
//
// Notes:
//
LPWSTR MakeWideStrFromAnsi( LPSTR psz, BYTE  bType )
{
    LPWSTR pwsz;
    int i;

    // arg checking.
    //
    if (!psz)
        return NULL;

    // compute the length of the required BSTR
    //
    i =  MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    if (i <= 0) return NULL;

    // allocate the widestr, +1 for terminating null
    //
    switch (bType) {

    case STR_OLESTR:
        pwsz = (LPWSTR) CoTaskMemAlloc(i * sizeof(WCHAR));
        break;
    default:
        return(NULL);
    }

    if (!pwsz) return NULL;
    MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, i);
    pwsz[i - 1] = 0;
    return pwsz;
}


LPSTR MakeAnsiStrFromWide( LPWSTR pwsz,  BYTE  bType )
{
    LPSTR psz;
    int i;

    // arg checking.
    //
    if (!pwsz)
        return NULL;

    // compute the length of the required BSTR
    //
    i =  WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL);
    if (i <= 0) return NULL;

    // allocate the ansistr, +1 for terminating null
    //
    switch (bType) {
    case STR_OLESTR:
        psz = (LPSTR) CoTaskMemAlloc(i * sizeof(CHAR));
        break;
    default:
        return(NULL);
    }

    if (!psz) return NULL;
    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz, i, NULL, NULL);
    psz[i - 1] = 0;
    return psz;
}

void ErrorMsg( HWND hWnd, UINT idErr )
{
    TCHAR szBuf[MAX_STRING];

    if ( !LoadString( g_hInst, idErr, szBuf, ARRAYSIZE(szBuf) ) )
    {
        wsprintf( szBuf, TEXT("LoadString failed on error id: %d"), idErr );
    }

    MessageBox( hWnd, szBuf, TEXT(SETUP_TITLE), MB_ICONSTOP|MB_OK );
}


