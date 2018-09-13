
/*----------------------------------------------------------
Purpose: Unix-menus related functions.

*/

#include <tchar.h>
#include <process.h>

#include "priv.h"
#pragma hdrstop

#ifdef UNIX

#include "unixstuff.h"
#include "resource.h"

DWORD g_dwTlsInfo = 0xffffffff;

void UnixStuffInit()
{
    g_dwTlsInfo = TlsAlloc();
}

// This is OK since we use Old Nt40 Shell32.

STDAPI_(BOOL) WINAPI SHGetFileClassKey (LPCTSTR szFile, HKEY * phkey, HKEY * phkeyBase);
STDAPI_(VOID) WINAPI SHCloseClassKey   (HKEY hkey);

void InitializeExplorerClass();

STDAPI_(BOOL) FileHasProperAssociation (LPCTSTR path)
{
    BOOL bRet = FALSE;
    HKEY hkClass = NULL, hkBase = NULL;
    if( SHGetFileClassKey(path, &hkClass, &hkBase ) )
    {
        bRet = TRUE;
        if(hkClass) SHCloseClassKey( hkClass );
        if(hkBase ) SHCloseClassKey( hkBase  );
    }
    return bRet;
}

#define WMC_UNIX_NEWWINDOW            (WM_USER + 0x0400)

LRESULT HandleCopyDataUnix(CShellBrowser2* psb, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case WMC_DISPATCH:
    {
        DDENAVIGATESTRUCT *pDDENav;
        LPSTR szURL;
        LRESULT lres = 0;

        szURL = (LPSTR)((COPYDATASTRUCT*)lParam)->lpData;
        pDDENav = new(DDENAVIGATESTRUCT);
        if (pDDENav)
        {
            pDDENav->wszUrl = (LPWSTR)LocalAlloc(LPTR, (lstrlenA(szURL) + 1) * sizeof(WCHAR));
            if (pDDENav->wszUrl)
            {
                pDDENav->transID = 0;
                MultiByteToWideChar(CP_ACP, 0, szURL, lstrlenA(szURL), pDDENav->wszUrl, lstrlenA(szURL));
                lres = psb->WndProcBS(hwnd, WMC_DISPATCH, (WPARAM)DSID_NAVIGATEIEBROWSER, (LPARAM)pDDENav);
                LocalFree(pDDENav->wszUrl);
            }
            delete(pDDENav);
        }
        return lres;
    }
    case WMC_UNIX_NEWWINDOW:
    {
        LPSTR szURL;
        WCHAR wszURL[MAX_URL_STRING];
        LPITEMIDLIST pidl;
        HRESULT hres = S_OK;

        szURL = (LPSTR)((COPYDATASTRUCT*)lParam)->lpData;
        if (szURL[0])
        {
            MultiByteToWideChar(CP_ACP, 0, szURL, lstrlenA(szURL) + 1, wszURL, ARRAYSIZE(wszURL));
        }
        else
        {
            hres = _GetStdLocation(wszURL, ARRAYSIZE(wszURL), DVIDM_GOHOME);
        }
        if (SUCCEEDED(hres)) 
        {
            hres = psb->IEParseDisplayName(CP_ACP, wszURL, &pidl);
            if (SUCCEEDED(hres)) 
                hres = psb->BrowseObject(pidl, SBSP_NEWBROWSER);
        }
        ILFree(pidl);
        return hres;
    }
    default:
        return FALSE;
    }
}

void StoreIEWindowInfo( HWND hwnd )
{
    if (g_dwTlsInfo != 0xffffffff)
    {
        TlsSetValue( g_dwTlsInfo, hwnd );
    }
}

HWND GetIEWindowOnThread( )
{
    if (g_dwTlsInfo != 0xffffffff)
    {
        return (HWND)TlsGetValue( g_dwTlsInfo );
    }

    return (HWND)0;
}


void PrintIEVersion()
{
    CHAR aboutInf[MAX_PATH] = "", *ptr;

    // Get the version information from Shlwapi
    SHAboutInfoA( aboutInf, sizeof(aboutInf) );
    ptr = StrChrA( aboutInf, '~' );
    if( ptr ) *ptr = '\0';
    
    printf("Internet Explorer %s ; Copyright (c) 1995-98 Microsoft Corp.\n", aboutInf);
}

void PrintIEHelp()
{
    CHAR aboutInf[MAX_PATH] = "", *ptr;

    // Get the version information from Shlwapi
    SHAboutInfoA( aboutInf, sizeof(aboutInf) );
    ptr = StrChrA( aboutInf, '~' );
    if( ptr ) *ptr = '\0';
    
    printf("Internet Explorer %s :\n\tUsage : iexplorer [ options ... ] [ URL ]\n\n", aboutInf);
    printf("Valid command line options :\n");
    printf("\t-root    : Opens Internet Explorer, connects to default home page.\n");
    printf("\t-slf     : Opens Internet Explorer, loads home page from cache.\n");
    printf("\t-k       : Kiosk mode.\n");
    printf("\t-version : Prints out the version of Internet Explorer.\n");
    printf("\t-v       : Same as -version.\n\n");
    printf("URL - Name that will be interpreted as either a URL or a file to be loaded.\n");
}

BOOL CheckForInvalidOptions( LPCTSTR inCmdLine )
{
    LPTSTR pszCmdLine = (LPTSTR) inCmdLine;

    TCHAR * knownOptions[] = { TEXT("help"), TEXT("embedding"), TEXT("slf"), TEXT("root"), TEXT("k"), TEXT("version"), TEXT("v"), TEXT("nohome") };

    int nCountOptions = sizeof(knownOptions)/sizeof(TCHAR *);

    while( 1 )
    {
        TCHAR * pos = StrChr( pszCmdLine, TEXT('-') );
        if( pos )
        {
            BOOL bFound = FALSE;
            pos++;
            TCHAR * option = pos;
            while( *pos && *pos != TEXT(' ') ) pos++;

            // Check for empty option or embedded '-'

            if( pos != option && 
              ( (option <= inCmdLine+1) || (*(option-2) == TEXT(' ')) ) )
            {
                for(int i = 0; i<nCountOptions; i++ )
                    if(!StrCmpNI( option, knownOptions[i], 
                                  lstrlen(knownOptions[i]))) 
                    {
                         bFound = TRUE;
                         break;
                    } 

                if( bFound == FALSE )
                {
                    *pos = TCHAR('\0');
                    printf("Unknown option : %s\n\n", option);
                    PrintIEHelp();
                    return bFound;
                }
            }
             
            pszCmdLine = pos;
        }
        else break;
    }

    return TRUE;
}



#ifdef NO_MARSHALLING

// We have multiple windows on a thread.
// So we store the list of psbs in a THREADWINDOWINFO * 
// in each thread (stored in a TLS).

#define DWSTYLE WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN

extern "C" DWORD g_TLSThreadWindowInfo = ~0; 

STDAPI CoMarshalInterfaceDummy( IStream *pStm, REFIID riid, IUnknown *pUnk, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags )
{
    if (pStm)
    {
       HRESULT hres;
       ULONG pcbWrite = 0;
       pStm->Seek(c_li0, STREAM_SEEK_SET, NULL);
       hres = pStm->Write( &pUnk, SIZEOF(pUnk), &pcbWrite );
       pUnk->AddRef(); 
       return hres;
    }
    
    return E_FAIL;
} 


LPCTSTR _GetExplorerClassName(UINT uFlags);

EXTERN_C void IEFrameNewWindowSameThread(IETHREADPARAM* piei)
{
    DWORD uFlags = piei->uFlags;
    THREADWINDOWINFO *lpThreadWindowInfo;
#ifdef NO_MARSHALLING
    BOOL fFirstTime = FALSE;
#endif 

    ASSERT(piei);

    if (!piei)
        return;

    LPWSTR pszCloseEvent = NULL;
    if (uFlags & COF_FIREEVENTONCLOSE)
    {
        ASSERT(piei->szCloseEvent[0]);
        pszCloseEvent = StrDup(piei->szCloseEvent);
    }

    // We shouldn't be the first ones 
    ASSERT(~0 != g_TLSThreadWindowInfo)
    lpThreadWindowInfo = (THREADWINDOWINFO *) TlsGetValue(g_TLSThreadWindowInfo);
    ASSERT(lpThreadWindowInfo); 
    if (!lpThreadWindowInfo)
    {
#ifdef NO_MARSHALLING
        if (!(lpThreadWindowInfo = InitializeThreadInfoStructs()))
            goto Done;
        fFirstTime = TRUE;
#else
        goto Done;
#endif 
    }

#ifdef DEBUG_EXPLORER
    piei->wv.bTree = TRUE;
    piei->TreeSplit = 200; // some random default
#endif

    TraceMsg(TF_COCREATE, "IEFrameNewWindowSameThread calling CreateWindowEx");

#ifdef NO_MARSHALLING
    if (!piei->fOnIEThread)
        InitializeExplorerClass();
#endif 

    LPCTSTR pszClass;
    
    HMENU hmenu = CreateMenu();
    HWND  hwnd  = CreateWindowEx(WS_EX_WINDOWEDGE, 
                   _GetExplorerClassName(piei->uFlags), NULL, DWSTYLE,
                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                   NULL, 
                   hmenu, 
                   HINST_THISDLL, piei);
    if (uFlags & COF_HELPMODE )
    {
        RECT rc;
        GetWindowRect( hwnd, &rc );
        SetWindowPos ( hwnd, 0, rc.left, rc.top, BROWSER_DEFAULT_WIDTH * 3 /4, BROWSER_DEFAULT_HEIGHT * 3 / 4, SWP_NOZORDER );
    }

    if (hwnd)
    {
        // Enable File Manager drag-drop for win16
        DragAcceptFiles(hwnd, TRUE);

        CShellBrowser2* psb = (CShellBrowser2*)GetWindowLong(hwnd, 0);

        if (psb)
        {
            // We are string another refrence to this object in
            // ThreadInfo Stuctures, so AddRef.

            psb->AddRef();

            psb->_AfterWindowCreated(piei);
#ifdef NO_MARSHALLING
            if (fFirstTime)
            {
                // this should happen only if IE is created on a different
                // thread, such as OE dochost
                AddFirstBrowserToList(psb);
            }
            else 
            {
#endif 
                lpThreadWindowInfo->cWindowCount++;
                // tack new window on end of list
                CShellBrowser2** rgpsb = (CShellBrowser2**)
                LocalReAlloc(lpThreadWindowInfo->rgpsb, 
                             lpThreadWindowInfo->cWindowCount * sizeof(CShellBrowser2 *), LMEM_MOVEABLE);
                ASSERT(rgpsb);
                if (rgpsb)
                {
                    lpThreadWindowInfo->rgpsb = rgpsb;
                    rgpsb[lpThreadWindowInfo->cWindowCount-1] = psb;
                    psb->AddRef();
                }
#ifdef NO_MARSHALLING
            }
#endif 
            // The message pump would go here.
            // Let the threadproc handle it.

            psb->Release();
        }
    }
    else
    {
        // Unregister any pending that may be there
        WinList_Revoke(piei->dwRegister);
        TraceMsg(DM_ERROR, "IEFrameNewWindowSameThread CreateWindow failed");
    }

Done:
    if (pszCloseEvent)
    {
        FireEventSz(pszCloseEvent);
    }

    if (piei)
        delete piei;
}

EXTERN_C THREADWINDOWINFO * InitializeThreadInfoStructs()
{
    THREADWINDOWINFO *lpThreadWindowInfo;

    // Allocate a TLS if needed. Will be freed in the WEP.
    if (~0 == g_TLSThreadWindowInfo)
    {
        g_TLSThreadWindowInfo = TlsAlloc();
        ASSERT (~0 != g_TLSThreadWindowInfo);
        lpThreadWindowInfo = NULL;
    }
    else
    {
        lpThreadWindowInfo = (THREADWINDOWINFO *) TlsGetValue(g_TLSThreadWindowInfo);
    }

    if (NULL == lpThreadWindowInfo && ~0 != g_TLSThreadWindowInfo)
    {
        lpThreadWindowInfo = (THREADWINDOWINFO *) LocalAlloc(LPTR, sizeof(*lpThreadWindowInfo));
        TlsSetValue(g_TLSThreadWindowInfo, lpThreadWindowInfo);
    }

    ASSERT(lpThreadWindowInfo);

    return lpThreadWindowInfo;

}

EXTERN_C void FreeThreadInfoStructs()
{
    THREADWINDOWINFO *lpThreadWindowInfo;
    lpThreadWindowInfo = (THREADWINDOWINFO *)TlsGetValue(g_TLSThreadWindowInfo);

    if( lpThreadWindowInfo )
    {
        LocalFree(lpThreadWindowInfo->rgpsb);
        LocalFree(lpThreadWindowInfo);
        TlsSetValue(g_TLSThreadWindowInfo, NULL);
    }
}

EXTERN_C void AddFirstBrowserToList( CShellBrowser2 *psb )
{
    if( psb )
    {
        THREADWINDOWINFO *lpThreadWindowInfo;
        lpThreadWindowInfo = (THREADWINDOWINFO *) TlsGetValue(g_TLSThreadWindowInfo);
        lpThreadWindowInfo->cWindowCount++;
        // tack new window on end of list
        lpThreadWindowInfo->rgpsb = (CShellBrowser2**) LocalAlloc(LPTR, sizeof(CShellBrowser2 *));
        ASSERT(lpThreadWindowInfo->rgpsb); // if this fails we're DEAD
        if( lpThreadWindowInfo->rgpsb )
        {
            lpThreadWindowInfo->rgpsb[0] = psb;
            psb->AddRef();
        }
    }
}

EXTERN_C void RemoveBrowserFromList( CShellBrowser2 *psb )
{
    if (psb->_fReallyClosed)
    {
        int i;
        BOOL fFoundSB = FALSE;
        THREADWINDOWINFO *lpThreadWindowInfo;
        lpThreadWindowInfo = (THREADWINDOWINFO *) TlsGetValue(g_TLSThreadWindowInfo);
        // remove the window from the list. 
        ASSERT(psb->_pbbd->_hwnd == NULL); 

        for (i = 0; i < lpThreadWindowInfo->cWindowCount; i++)
        {
            if( psb == lpThreadWindowInfo->rgpsb[i] ) 
            {
                fFoundSB = TRUE;
                break;
            }
        }

        if ( fFoundSB )
        {  
            memmove(&lpThreadWindowInfo->rgpsb[i], 
                 &lpThreadWindowInfo->rgpsb[i+1], 
                 (--lpThreadWindowInfo->cWindowCount - i) * sizeof(psb));
            psb->Release(); 
        }
    }
}

#endif // NO_MARSHALLING

#endif // UNIX
