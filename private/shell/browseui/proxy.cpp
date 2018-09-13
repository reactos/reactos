
#include "priv.h"
#include <multimon.h>
#include "hnfblock.h"
#include <trayp.h>
#include "desktop.h"
#include "shbrows2.h"
#include "resource.h"
#include "onetree.h"
#include "apithk.h"
#include <regitemp.h>

#include "mluisupp.h"

BOOL _RootsEqual(HANDLE hCR, DWORD dwProcId, LPCITEMIDLIST pidlRoot)
{
    BOOL bSame = FALSE;
    if (hCR)
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)SHLockShared(hCR, dwProcId);
        if (pidl)
        {
            bSame = ILIsEqualRoot(pidlRoot, pidl);
            SHUnlockShared(pidl);
        }
    }
    return bSame;
}


// NOTE: this export is new to IE5, so it can move to browseui
// along with the rest of this proxy desktop code
BOOL SHOnCWMCommandLine(LPARAM lParam)
{
    HNFBLOCK hnf = (HNFBLOCK)lParam;
    IETHREADPARAM *piei = ConvertHNFBLOCKtoNFI(hnf);
    if (piei)
        return SHOpenFolderWindow(piei);

    // bad params passed, normal failure case
    return FALSE;
}


//---------------------------------------------------------------------------
// This proxy desktop window procedure is used when we are run and we
// are not the shell.  We are a hidden window which will simply respond
// to messages like the ones that create threads for folder windows.
// This window procedure will close after all of the open windows
// associated with it go away.
class CProxyDesktop
{
private:
    static LRESULT CALLBACK ProxyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    friend CProxyDesktop *CreateProxyDesktop(IETHREADPARAM *piei);
    friend BOOL SHCreateFromDesktop(PNEWFOLDERINFO pfi);

    CProxyDesktop() {};
    ~CProxyDesktop();

    HWND            _hwnd;
    LPITEMIDLIST    _pidlRoot;
};

CProxyDesktop::~CProxyDesktop()
{
    ILFree(_pidlRoot);
}

LRESULT CALLBACK CProxyDesktop::ProxyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CProxyDesktop *pproxy = (CProxyDesktop *)GetWindowPtr0(hwnd);

    switch (msg)
    {
    case WM_CREATE:
        pproxy = (CProxyDesktop *)((CREATESTRUCT *)lParam)->lpCreateParams;
        SetWindowPtr0(hwnd, pproxy);

        pproxy->_hwnd = hwnd;
        return 0;   // success

    case WM_DESTROY:
        if (pproxy)
            pproxy->_hwnd = NULL;
        return 0;

    case CWM_COMMANDLINE:
        SHOnCWMCommandLine(lParam);
        break;

    case CWM_COMPAREROOT:
        return _RootsEqual((HANDLE)lParam, (DWORD)wParam, pproxy->_pidlRoot);

    default:
        return DefWindowProcWrap(hwnd, msg, wParam, lParam);
    }
    return 0;
}

CProxyDesktop *CreateProxyDesktop(IETHREADPARAM *piei)
{
    CProxyDesktop *pproxy = new CProxyDesktop();
    if (pproxy)
    {
        WNDCLASS wc = {0};
        wc.lpfnWndProc = CProxyDesktop::ProxyWndProc;
        wc.cbWndExtra = SIZEOF(CProxyDesktop *);
        wc.hInstance = HINST_THISDLL;
        wc.hbrBackground = (HBRUSH)(COLOR_DESKTOP + 1);
        wc.lpszClassName = DESKTOPPROXYCLASS;

        SHRegisterClass(&wc);

        if (CreateWindowEx(WS_EX_TOOLWINDOW, DESKTOPPROXYCLASS, DESKTOPPROXYCLASS,
            WS_POPUP, 0, 0, 0, 0, NULL, NULL, HINST_THISDLL, pproxy))
        {
            if (ILIsRooted(piei->pidl))
            {
                pproxy->_pidlRoot = ILCloneFirst(piei->pidl);
                if (pproxy->_pidlRoot == NULL)
                {
                    DestroyWindow(pproxy->_hwnd);
                    pproxy = NULL;
                }
            }
        }
        else
        {
            delete pproxy;
            pproxy = NULL;
        }
    }
    return pproxy;
}


// REVIEW: maybe just check (hwnd == GetShellWindow())

STDAPI_(BOOL) IsDesktopWindow(HWND hwnd)
{
    TCHAR szName[80];

    GetClassName(hwnd, szName, ARRAYSIZE(szName));
    if (!lstrcmp(szName, DESKTOPCLASS))
    {
        GetWindowText(hwnd, szName, ARRAYSIZE(szName));
        return !lstrcmp(szName, PROGMAN);
    }
    return FALSE;
}

typedef struct
{
    HWND hwndDesktop;
    HANDLE hCR;
    DWORD dwProcId;
    HWND hwndResult;
} FRDSTRUCT;

BOOL CALLBACK FindRootEnumProc(HWND hwnd, LPARAM lParam)
{
    FRDSTRUCT *pfrds = (FRDSTRUCT *)lParam;
    TCHAR szClassName[40];

    GetClassName(hwnd, szClassName, ARRAYSIZE(szClassName));
    if (lstrcmpi(szClassName, DESKTOPPROXYCLASS) == 0)
    {
        ASSERT(hwnd != pfrds->hwndDesktop);

        if (SendMessage(hwnd, CWM_COMPAREROOT, (WPARAM)pfrds->dwProcId, (LPARAM)pfrds->hCR))
        {
            // Found it, so stop enumerating
            pfrds->hwndResult = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

BOOL RunSeparateDesktop()
{
    DWORD bSeparate = FALSE;

    if (SHRestricted(REST_SEPARATEDESKTOPPROCESS))
        bSeparate = TRUE;
    else
    {
        SHELLSTATE ss;

        SHGetSetSettings(&ss, SSF_SEPPROCESS, FALSE);
        bSeparate = ss.fSepProcess;

        if (!bSeparate)
        {
            DWORD cbData = SIZEOF(bSeparate);
            SHGetValue(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("DesktopProcess"), NULL, &bSeparate, &cbData);
        }
    }
    return bSeparate;

}

//  if we need to force some legacy rootet explorers into their own process, implement this.
//#define _RootRunSeparateProcess(pidlRoot)  ILIsRooted(pidlRoot)  OLD BEHAVIOR
#define _RootRunSeparateProcess(pidlRoot)  FALSE

HWND FindRootedDesktop(LPCITEMIDLIST pidlRoot)
{
    HWND hwndDesktop = GetShellWindow();    // This is the "normal" desktop

    if (!RunSeparateDesktop() && !_RootRunSeparateProcess(pidlRoot) && hwndDesktop)
    {
        ASSERT(IsDesktopWindow(hwndDesktop));
        return hwndDesktop;
    }

    FRDSTRUCT frds;
    frds.hwndDesktop = hwndDesktop;
    frds.hwndResult = NULL;     // Initalize to no matching rooted expl
    frds.dwProcId = GetCurrentProcessId();
    frds.hCR = SHAllocShared(pidlRoot, ILGetSize(pidlRoot), frds.dwProcId);
    if (frds.hCR)
    {
        EnumWindows(FindRootEnumProc, (LPARAM)&frds);
        SHFreeShared(frds.hCR, frds.dwProcId);
    }

    return frds.hwndResult;
}


UINT _GetProcessHotkey(void)
{
    STARTUPINFO si = {SIZEOF(si)};
    GetStartupInfo(&si);
    return (UINT)(DWORD_PTR)si.hStdInput;
}

void FolderInfoToIEThreadParam(PNEWFOLDERINFO pfi, IETHREADPARAM *piei)
{
    piei->uFlags = pfi->uFlags;
    piei->nCmdShow = pfi->nShow;
    piei->wHotkey = _GetProcessHotkey();
    
    ASSERT(pfi->pszRoot == NULL);       // explorer always converts to a PIDL for us

    //  we no longer support rooted explorers this way
    //  it should have been filtered out above us.
    ASSERT(!pfi->pidlRoot);
    ASSERT(!(pfi->uFlags & (COF_ROOTCLASS | COF_NEWROOT)));
    ASSERT(IsEqualGUID(pfi->clsid, CLSID_NULL));

    if (pfi->pidl) 
    {
        piei->pidl = ILClone(pfi->pidl);
    } 
    //  COF_PARSEPATH means that we should defer the parsing of the pszPath
    else if (!(pfi->uFlags & COF_PARSEPATH) && pfi->pszPath && pfi->pszPath[0])
    {
        //  maybe should use IECreateFromPath??
        //  or maybe we should parse relative to the root??
        piei->pidl = ILCreateFromPathA(pfi->pszPath);
    }
}

// IE4 Integrated delay loads CreateFromDesktop from SHDOCVW.DLL
// So we need to keep this function here. Forward to the correct
// implementation in SHELL32 (if integrated) or SHDOC41 (if not)
BOOL SHCreateFromDesktop(PNEWFOLDERINFO pfi)
{
    IETHREADPARAM *piei = SHCreateIETHREADPARAM(NULL, 0, NULL, NULL);
    if (piei)
    {
        //  ASSUMING UNICODE COMPILE!
        LPCTSTR pszPath = NULL;
        
        if (pfi->uFlags & COF_PARSEPATH)
        {
            ASSERT(!pfi->pidl);
            pszPath = (LPCTSTR) pfi->pszPath;
        }

        FolderInfoToIEThreadParam(pfi, piei);

        HWND hwndDesktop = FindRootedDesktop(piei->pidl);
        if (hwndDesktop)
        {
            DWORD dwProcId;
            DWORD dwThreadId = GetWindowThreadProcessId(hwndDesktop, &dwProcId);
            AllowSetForegroundWindow(dwProcId);
            HNFBLOCK hBlock = ConvertNFItoHNFBLOCK(piei, pszPath, dwProcId);
            if (hBlock)
            {
                PostMessage(hwndDesktop, CWM_COMMANDLINE, 0, (LPARAM)hBlock);

                HANDLE hExplorer = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, dwProcId );
                if ( hExplorer )
                {
                    // wait for input idle 10 seconds.
                    WaitForInputIdle( hExplorer, 10000 );
                    CloseHandle( hExplorer );
                }
            }
        }
        else
        {
            CoInitialize(0);

            CProxyDesktop *pproxy = CreateProxyDesktop(piei);
            if (pproxy)
            {
                // CRefThread controls this processes reference count. browser windows use this
                // to keep this process (window) around and this also lets thrid parties hold 
                // references to our process, MSN uses this for example

                LONG cRefMsgLoop;
                IUnknown *punkRefMsgLoop;
                if (SUCCEEDED(SHCreateThreadRef(&cRefMsgLoop, &punkRefMsgLoop)))
                {
                    SHSetInstanceExplorer(punkRefMsgLoop);

                    //  we needed to wait for this for the CoInit()
                    if (pszPath)
                        piei->pidl = ILCreateFromPath(pszPath);

                    SHOpenFolderWindow(piei);
                    piei = NULL;                // OpenFolderWindow() takes ownership of this
                    punkRefMsgLoop->Release();  // we now depend on the browser window to keep our msg loop
                }

                MSG msg;
                while (GetMessage(&msg, NULL, 0, 0))
                {
                    if (cRefMsgLoop == 0)
                        break; // no more refs on this thread, done

                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                delete pproxy;
            }

            CoUninitialize();
        }

        if (piei)
            SHDestroyIETHREADPARAM(piei);
    }
    return TRUE;        // no one pays attention to this
}
        

HNFBLOCK ConvertNFItoHNFBLOCK(IETHREADPARAM* pInfo, LPCTSTR pszPath, DWORD dwProcId)
{
    UINT    uSize;
    UINT    uPidl;
    UINT    uPidlSelect;
    UINT    uPidlRoot;
    UINT    upszPath;
    PNEWFOLDERBLOCK pnfb;
    LPBYTE  lpb;
    HNFBLOCK hBlock;
    LPVOID pidlRootOrMonitor = NULL; // pidlRoot or &hMonitor

    uSize = SIZEOF(NEWFOLDERBLOCK);
    if (pInfo->pidl)
    {
        uPidl = ILGetSize(pInfo->pidl);
        uSize += uPidl;
    }
    if (pInfo->pidlSelect)
    {
        uPidlSelect = ILGetSize(pInfo->pidlSelect);
        uSize += uPidlSelect;
    }

    if (pInfo->uFlags & COF_HASHMONITOR)
    {
        pidlRootOrMonitor = &pInfo->pidlRoot;
        uPidlRoot = sizeof(HMONITOR);
        uSize += uPidlRoot;
    }
    else if (pInfo->pidlRoot)
    {
        pidlRootOrMonitor = pInfo->pidlRoot;
        uPidlRoot = ILGetSize(pInfo->pidlRoot);
        uSize += uPidlRoot;
    }

    if (pszPath) {
        upszPath = CbFromCch(lstrlen(pszPath) + 1);
        uSize += upszPath;
    }

    hBlock = (HNFBLOCK)SHAllocShared(NULL, uSize, dwProcId);
    if (hBlock == NULL)
        return NULL;

    pnfb = (PNEWFOLDERBLOCK)SHLockShared(hBlock, dwProcId);
    if (pnfb == NULL)
    {
        SHFreeShared(hBlock, dwProcId);
        return NULL;
    }

    pnfb->dwSize      = uSize;
    pnfb->uFlags      = pInfo->uFlags;
    pnfb->nShow       = pInfo->nCmdShow;
    pnfb->hwndCaller  = pInfo->hwndCaller;
    pnfb->dwHotKey    = pInfo->wHotkey;
    pnfb->clsid       = pInfo->clsid;
    pnfb->clsidInProc = pInfo->clsidInProc;
    pnfb->oidl        = 0;
    pnfb->oidlSelect  = 0;
    pnfb->oidlRoot    = 0;
    pnfb->opszPath    = 0;

    lpb = (LPBYTE)(pnfb+1);     // Point just past the structure

    if (pInfo->pidl)
    {
        memcpy(lpb,pInfo->pidl,uPidl);
        pnfb->oidl = (int)(lpb-(LPBYTE)pnfb);
        lpb += uPidl;
    }
    if (pInfo->pidlSelect)
    {
        memcpy(lpb,pInfo->pidlSelect,uPidlSelect);
        pnfb->oidlSelect = (int)(lpb-(LPBYTE)pnfb);
        lpb += uPidlSelect;
    }

    if (pidlRootOrMonitor)
    {
        memcpy(lpb, pidlRootOrMonitor, uPidlRoot);
        pnfb->oidlRoot = (int)(lpb-(LPBYTE)pnfb);
        lpb += uPidlRoot;
    }

    if (pszPath)
    {
        memcpy(lpb, pszPath, upszPath);
        pnfb->opszPath = (int)(lpb-(LPBYTE)pnfb);
        lpb += upszPath;
    }
    SHUnlockShared(pnfb);
    return hBlock;
}

IETHREADPARAM* ConvertHNFBLOCKtoNFI(HNFBLOCK hBlock)
{
    BOOL fFailure = FALSE;
    IETHREADPARAM* piei = NULL;
    if (hBlock)
    {
        DWORD dwProcId = GetCurrentProcessId();
        PNEWFOLDERBLOCK pnfb = (PNEWFOLDERBLOCK)SHLockShared(hBlock, dwProcId);
        if (pnfb)
        {
            if (pnfb->dwSize >= SIZEOF(NEWFOLDERBLOCK))
            {
                piei = SHCreateIETHREADPARAM(NULL, pnfb->nShow, NULL, NULL);
                if (piei)
                {
                    LPITEMIDLIST pidl = NULL;
                    piei->uFlags      = pnfb->uFlags;
                    piei->hwndCaller  = pnfb->hwndCaller;
                    piei->wHotkey     = pnfb->dwHotKey;
                    piei->clsid       = pnfb->clsid;
                    piei->clsidInProc = pnfb->clsidInProc;

                    if (pnfb->oidlSelect)
                        piei->pidlSelect = ILClone((LPITEMIDLIST)((LPBYTE)pnfb+pnfb->oidlSelect));

                    if (pnfb->oidlRoot)
                    {
                        LPITEMIDLIST pidlRoot = (LPITEMIDLIST)((LPBYTE)pnfb+pnfb->oidlRoot);
                        if (pnfb->uFlags & COF_HASHMONITOR)
                        {
                            piei->pidlRoot = (LPITEMIDLIST)*(UNALIGNED HMONITOR *)pidlRoot;
                        }
                        else
                        {
                            piei->pidlRoot = ILClone(pidl);
                        }
                    }

                    if (pnfb->oidl)
                        pidl = ILClone((LPITEMIDLIST)((LPBYTE)pnfb+pnfb->oidl));

                    if (pidl) 
                    {
                        piei->pidl = pidl;
                    } 
                    
                    // we pass this string through because msn fails the cocreateinstane of
                    // their desktop if another one is up and running, so we can't convert
                    // this from path to pidl except in the current process context
                    if (pnfb->opszPath) 
                    {
                        LPTSTR pszPath = (LPTSTR)((LPBYTE)pnfb+pnfb->opszPath);
                        HRESULT hr = E_FAIL;
                        
                        if (ILIsRooted(pidl))
                        {
                            //  let the root handle the parsing.
                            IShellFolder *psf;
                            if (SUCCEEDED(IEBindToObject(pidl, &psf)))
                            {
                                hr = IShellFolder_ParseDisplayName(psf, NULL, NULL, pszPath, NULL, &(piei->pidl), NULL);
                                psf->Release();
                            }
                        }
                        else
                            IECreateFromPath(pszPath, &(piei->pidl));

                        // APP COMPAT: these two specific return result codes are the two we ignored for win95.
                        // APP COMPAT: MSN 1.3 Classic accidentally on purpose returns one of these...
                        if ( !piei->pidl )
                        {
                            // failed, report the error to the user ... (will only fail for paths)
                            ASSERT( !PathIsURL( pszPath))

                            if (! (piei->uFlags & COF_NOTUSERDRIVEN) && ( hr != E_OUTOFMEMORY ) && ( hr != HRESULT_FROM_WIN32( ERROR_CANCELLED )))
                            {
                                MLShellMessageBox(
                                                  NULL,
                                                  MAKEINTRESOURCE( IDS_NOTADIR ),
                                                  MAKEINTRESOURCE( IDS_CABINET ),
                                                  MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND,
                                                  pszPath);
                            }
                            fFailure = TRUE;
                        }
                    }

                }
            }
            SHUnlockShared(pnfb);
        }
        SHFreeShared(hBlock, dwProcId);
    }

    // if we really failed somewhere, return NULL
    if (fFailure)
    {
        SHDestroyIETHREADPARAM(piei);
        piei = NULL;
    }
    return piei;
}


// Check the registry for a shell root under this CLSID.
BOOL GetRootFromRootClass(LPCTSTR pszGUID, LPTSTR pszPath, int cchPath)
{
    TCHAR szClass[MAX_PATH];

    wnsprintf(szClass, ARRAYSIZE(szClass), TEXT("CLSID\\%s\\ShellExplorerRoot"), pszGUID);

    DWORD cbPath = cchPath * sizeof(TCHAR);

    return SHGetValueGoodBoot(HKEY_CLASSES_ROOT, szClass, NULL, NULL, (BYTE *)pszPath, &cbPath) == ERROR_SUCCESS;
}

// format is ":<hMem>:<hProcess>"

LPITEMIDLIST IDListFromCmdLine(LPCTSTR pszCmdLine, int i)
{
    LPITEMIDLIST pidl = NULL;
    TCHAR szField[80];

    if (ParseField(pszCmdLine, i, szField, ARRAYSIZE(szField)) && szField[0] == TEXT(':'))
    {
        // Convert the string of format ":<hmem>:<hprocess>" into a pointer
        HANDLE hMem = (HANDLE)StrToLong(szField + 1);
        LPTSTR pszNextColon = StrChr(szField + 1, TEXT(':'));
        if (pszNextColon)
        {
            DWORD dwProcId = (DWORD)StrToLong(pszNextColon + 1);
            LPITEMIDLIST pidlGlobal = (LPITEMIDLIST) SHLockShared(hMem, dwProcId);
            if (pidlGlobal)
            {
                if (!IsBadReadPtr(pidlGlobal, 1))
                    pidl = ILClone(pidlGlobal);

                SHUnlockShared(pidlGlobal);
                SHFreeShared(hMem, dwProcId);
            }
        }
    }
    return pidl;
}

#define MYDOCS_CLSID TEXT("{450d8fba-ad25-11d0-98a8-0800361b1103}") // CLSID_MyDocuments

LPITEMIDLIST MyDocsIDList(void)
{
    LPITEMIDLIST pidl = NULL;
    IShellFolder *psf;
    HRESULT hres = SHGetDesktopFolder(&psf);
    if (SUCCEEDED(hres))
    {
        WCHAR wszName[128];

        wszName[0] = wszName[1] = TEXT(':');
        SHTCharToUnicode(MYDOCS_CLSID, wszName + 2, ARRAYSIZE(wszName) - 2);

        hres = psf->ParseDisplayName(NULL, NULL, wszName, NULL, &pidl, NULL);
        psf->Release();
    }

    // Win95/NT4 case, go for the real MyDocs folder
    if (FAILED(hres))
    {
        hres = SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl);
    }
    return SUCCEEDED(hres) ? pidl : NULL;
}


BOOL SHExplorerParseCmdLine(PNEWFOLDERINFO pfi)
{
    int i;
    TCHAR szField[MAX_PATH];

    LPCTSTR pszCmdLine = GetCommandLine();
    pszCmdLine = PathGetArgs(pszCmdLine);

    // empty command line -> explorer My Docs
    if (*pszCmdLine == 0)
    {
        pfi->uFlags = COF_CREATENEWWINDOW | COF_EXPLORE;

        // try MyDocs first?
        pfi->pidl = MyDocsIDList();
        if (pfi->pidl == NULL)
        {
            TCHAR szPath[MAX_PATH];
            GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
            PathStripToRoot(szPath);
            pfi->pidl = ILCreateFromPath(szPath);
        }

        return BOOLFROMPTR(pfi->pidl);
    }

    // Arguments must be separated by '=' or ','
    for (i = 1; ParseField(pszCmdLine, i, szField, ARRAYSIZE(szField)); i++)
    {
        if (lstrcmpi(szField, TEXT("/N")) == 0)
        {
            pfi->uFlags |= COF_CREATENEWWINDOW | COF_NOFINDWINDOW;
        }
        else if (lstrcmpi(szField, TEXT("/S")) == 0)
        {
            pfi->uFlags |= COF_USEOPENSETTINGS;
        }
        else if (lstrcmpi(szField, TEXT("/E")) == 0)
        {
            pfi->uFlags |= COF_EXPLORE;
        }
        else if (lstrcmpi(szField, TEXT("/ROOT")) == 0)
        {
            LPITEMIDLIST pidlRoot = NULL;
            CLSID *pclsidRoot = NULL;
            CLSID clsid;

            RIPMSG(!pfi->pidl, "SHExplorerParseCommandLine: (/ROOT) caller passed bad params");

            // of the form:
            //     /ROOT,{clsid}[,<path>]
            //     /ROOT,/IDLIST,:<hmem>:<hprocess>
            //     /ROOT,<path>

            if (!ParseField(pszCmdLine, ++i, szField, ARRAYSIZE(szField)))
                return FALSE;

            // {clsid}
            if (GUIDFromString(szField, &clsid))
            {
                TCHAR szGUID[GUIDSTR_MAX];
                StrCpyN(szGUID, szField, SIZECHARS(szGUID));

                // {clsid} case, if not path compute from the registry
                if (!ParseField(pszCmdLine, ++i, szField, ARRAYSIZE(szField)))
                {
                    // path must come from the registry now
                    if (!GetRootFromRootClass(szGUID, szField, ARRAYSIZE(szField)))
                    {
                        return FALSE;   // bad command line
                    }
                }

                IECreateFromPath(szField, &pidlRoot);
                pclsidRoot = &clsid;

            }
            else if (lstrcmpi(szField, TEXT("/IDLIST")) == 0)
            {
                // /IDLIST
                pidlRoot = IDListFromCmdLine(pszCmdLine, ++i);
            }
            else
            {
                // <path>
                IECreateFromPath(szField, &pidlRoot);
            }

            // fix up bad cmd line "explorer.exe /root," case
            if (pidlRoot == NULL)
                SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pfi->pidlRoot);

            if (pidlRoot)
            {
                pfi->pidl = ILRootedCreateIDList(pclsidRoot, pidlRoot);
                ILFree(pidlRoot);
            }
        }
        else if (lstrcmpi(szField, TEXT("/INPROC")) == 0)
        {
            // Parse and skip the next arg or 2
            if (!ParseField(pszCmdLine, ++i, szField, ARRAYSIZE(szField)))
            {
                return FALSE;
            }

            // The next arg must be a GUID
            if (!GUIDFromString(szField, &pfi->clsidInProc))
            {
                return FALSE;
            }

            pfi->uFlags |= COF_INPROC;
        }
        else if (lstrcmpi(szField, TEXT("/SELECT")) == 0)
        {
            pfi->uFlags |= COF_SELECT;
        }
        else if (lstrcmpi(szField, TEXT("/NOUI")) == 0)
        {
            pfi->uFlags |= COF_NOUI;
        }
        else if (lstrcmpi(szField, TEXT("-embedding")) == 0)
        {
            pfi->uFlags |= COF_AUTOMATION;
        }
        else  if (lstrcmpi(szField, TEXT("/IDLIST")) == 0)
        {
            LPITEMIDLIST pidl = IDListFromCmdLine(pszCmdLine, ++i);

            if (pidl)
            {
                if (pfi->pidl)
                {
                    // again, this is kind of bogus (see comment below). If we already have a
                    // pidl, free it and use the new one.
                    ILFree(pfi->pidl);
                }

                pfi->pidl = pidl;
            }
            else if (pfi->pidl == NULL)
            {
                // if we didn't have a pidl before and we dont have one now, we are screwed, so bail
                return FALSE;
            }
        }
        else
        {
            LPITEMIDLIST pidl = ILCreateFromPath(szField);
            if (!pidl)
            {
                //
                //  LEGACY - if this is unparseable, then guess it is relative path
                //  this catches "explorer ." as opening the current directory
                //
                TCHAR szDir[MAX_PATH];
                TCHAR szCombined[MAX_PATH];
                GetCurrentDirectory(SIZECHARS(szDir), szDir);

                PathCombine(szCombined, szDir, szField);

                pidl = ILCreateFromPath(szCombined);
            }

            // this is kind of bogus: we have traditionally passed both the idlist (/idlist,:580:1612) and the path
            // (C:\Winnt\Profiles\reinerf\Desktop) as the default command string to explorer (see HKCR\Folder\shell
            // \open\command). Since we have both a /idlist and a path, we have always used the latter so that is what
            // we continue to do here.
            if (pfi->pidl)
            {
                // free the /idlist pidl and use the one from the path
                ILFree(pfi->pidl);
            }

            if (pidl)  
            {
                pfi->pidl = pidl;
                pfi->uFlags |= COF_NOTRANSLATE;     // pidl is abosolute from the desktop
            }
            else
            {
                pfi->pszPath = (LPSTR) StrDup(szField);
                if (pfi->pszPath)
                {
                    pfi->uFlags |= COF_PARSEPATH;
                }
            }
        }
    }
    return TRUE;
}
