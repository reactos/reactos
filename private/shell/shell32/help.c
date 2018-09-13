//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1992
//
// File: help.c
//
// History:
//  6 Apr 94    MikeSh  Created
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop
#include "printer.h"
#include "drives.h" // for ShowMountedVolumeProperties
//
// NOTE: If you change or delete any of these strings, notify the help group
//
static TCHAR szAddPrinter[] = TEXT("AddPrinter");
static TCHAR szConnect[] = TEXT("Connect");
static TCHAR szDisconnect[] = TEXT("Disconnect");
static TCHAR szPrintersFolder[] = TEXT("PrintersFolder");
static TCHAR szFontsFolder[] = TEXT("FontsFolder");

//
// (internal) entry point for Help "Shortcuts".
//
VOID WINAPI SHHelpShortcuts_RunDLL_Common(HWND hwndStub, HINSTANCE hAppInstance, LPCTSTR pszCmdLine, int nCmdShow)
{
    LPITEMIDLIST pidl = NULL;

    if (!lstrcmp(pszCmdLine, szAddPrinter))
    {
        // install a new printer

        pidl = Printers_PrinterSetup(hwndStub, MSP_NEWPRINTER, (LPTSTR)c_szNewObject, NULL);
    }
    else if (!lstrcmp(pszCmdLine, szPrintersFolder))
    {
        // bring up the printers folder

        pidl = SHCloneSpecialIDList(hwndStub, CSIDL_PRINTERS, FALSE);
        goto OpenFolder;
    }
    else if (!lstrcmp(pszCmdLine, szFontsFolder))
    {
        // bring up the printers folder

        pidl = SHCloneSpecialIDList(hwndStub, CSIDL_FONTS, FALSE);
OpenFolder:
        if (pidl)
        {
            CMINVOKECOMMANDINFOEX ici;

            ZeroMemory(&ici, SIZEOF(ici));
            ici.cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
            ici.hwnd = hwndStub;
            ici.nShow = SW_SHOWNORMAL;
            InvokeFolderCommandUsingPidl(&ici, NULL, pidl, NULL, SEE_MASK_FLAG_DDEWAIT);
        }
    }
    else if (!lstrcmp(pszCmdLine, szConnect))
    {
        SHNetConnectionDialog(hwndStub, NULL, RESOURCETYPE_DISK);
        goto FlushDisconnect;
    }
    else if (!lstrcmp(pszCmdLine, szDisconnect))
    {
        WNetDisconnectDialog(hwndStub, RESOURCETYPE_DISK);
FlushDisconnect:
        SHChangeNotifyHandleEvents();   // flush any drive notifications
    }
#ifdef DEBUG
    else if (!StrCmpN(pszCmdLine, TEXT("PrtProp "), 8))
    {
        SHObjectProperties(hwndStub, SHOP_PRINTERNAME, &(pszCmdLine[8]), TEXT("Sharing"));
    }
    else if (!StrCmpN(pszCmdLine, TEXT("FileProp "), 9))
    {
        SHObjectProperties(hwndStub, SHOP_FILEPATH, &(pszCmdLine[9]), TEXT("Sharing"));
    }
#endif
    else
        DebugMsg(DM_TRACE, TEXT("SHHelpShortcuts: unrecognized command '%s'"), pszCmdLine);

    if (pidl)
        ILFree(pidl);
}

VOID WINAPI SHHelpShortcuts_RunDLL(HWND hwndStub, HINSTANCE hAppInstance, LPCSTR lpszCmdLine, int nCmdShow)
{
#ifdef UNICODE
    UINT iLen = lstrlenA(lpszCmdLine)+1;
    LPWSTR  lpwszCmdLine;

    lpwszCmdLine = (LPWSTR)LocalAlloc(LPTR,iLen*SIZEOF(WCHAR));
    if (lpwszCmdLine)
    {
        MultiByteToWideChar(CP_ACP, 0,
                            lpszCmdLine, -1,
                            lpwszCmdLine, iLen);
        SHHelpShortcuts_RunDLL_Common( hwndStub,
                                       hAppInstance,
                                       lpwszCmdLine,
                                       nCmdShow );
        LocalFree(lpwszCmdLine);
    }
#else
    SHHelpShortcuts_RunDLL_Common(hwndStub,hAppInstance,lpszCmdLine,nCmdShow);
#endif
}

VOID WINAPI SHHelpShortcuts_RunDLLW(HWND hwndStub, HINSTANCE hAppInstance, LPCWSTR lpwszCmdLine, int nCmdShow)
{
#ifdef UNICODE
    SHHelpShortcuts_RunDLL_Common(hwndStub,hAppInstance,lpwszCmdLine,nCmdShow);
#else
    UINT iLen = WideCharToMultiByte(CP_ACP, 0,
                            lpwszCmdLine, -1,
                            NULL, 0, NULL, NULL)+1;
    LPSTR  lpszCmdLine;

    lpszCmdLine = (LPSTR)LocalAlloc(LPTR,iLen);
    if (lpszCmdLine)
    {
        WideCharToMultiByte(CP_ACP, 0,
                            lpwszCmdLine, -1,
                            lpszCmdLine, iLen,
                            NULL, NULL);
        SHHelpShortcuts_RunDLL_Common( hwndStub,
                                       hAppInstance,
                                       lpszCmdLine,
                                       nCmdShow );
        LocalFree(lpszCmdLine);
    }
#endif
}

///////////////////////////////////////////////////////
//
// SHObjectProperties is an easy way to call the verb "properties" on an object.
// It's easy because the caller doesn't have to deal with LPITEMIDLISTs.
// Note: SHExecuteEx(SEE_MASK_INVOKEIDLIST) works for the SHOP_FILEPATH case,
// but msshrui needs an easy way to do this for printers. Bummer.
//
BOOL WINAPI SHObjectProperties(HWND hwndOwner, DWORD dwType, LPCTSTR lpObject, LPCTSTR lpPage)
{
    LPITEMIDLIST pidl;

    switch (dwType & SHOP_TYPEMASK)
    {
        case SHOP_PRINTERNAME:
            DebugMsg(DM_TRACE, TEXT("SHObjectProperties(SHOP_PRINTERNAME,%s,%s)"),lpObject,lpPage);
            pidl = Printers_GetPidl(NULL, lpObject);
            break;

        case SHOP_FILEPATH:
            DebugMsg(DM_TRACE, TEXT("SHObjectProperties(SHOP_FILEPATH,%s,%s)"),lpObject,lpPage);
            pidl = ILCreateFromPath(lpObject);
            break;

#ifdef WINNT
        case SHOP_VOLUMEGUID:
            {
                DebugMsg(DM_TRACE, TEXT("SHObjectProperties(SHOP_VOLUMEGUID,%s,%s)"),lpObject,lpPage);

                return ShowMountedVolumeProperties(lpObject, hwndOwner);
            }
            break;
#endif
        default:
            DebugMsg(TF_WARNING, TEXT("illegal SHObjectProperties type for (%s,%s)"),lpObject,lpPage);
            pidl = NULL;
            break;
    }

    if (pidl)
    {
        SHELLEXECUTEINFO sei =
        {
            SIZEOF(SHELLEXECUTEINFO),
            SEE_MASK_INVOKEIDLIST,      // fMask
            hwndOwner,                  // hwnd
            c_szProperties,             // lpVerb
            NULL,                       // lpFile
            lpPage,                     // lpParameters
            NULL,                       // lpDirectory
            SW_SHOWNORMAL,              // nShow
            NULL,                       // hInstApp
            pidl,                       // lpIDList
            NULL,                       // lpClass
            0,                          // hkeyClass
            0,                          // dwHotKey
            NULL                        // hIcon
        };

        BOOL bRet = ShellExecuteEx(&sei);

        ILFree(pidl);

        return bRet;
    }

    return FALSE;
}
