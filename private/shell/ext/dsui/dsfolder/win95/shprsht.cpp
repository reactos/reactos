//
//  This file contains some shell specific property sheet related code,
// which includes:
//  1. The logic which lets shell extensions add pages.
//  2. The callback function to be called by those shell extensions.
// but does not include:
//  1. The property sheet UI code (should be in COMMCTRL).
//  2. The file system specific property sheet pages.
//
#include "pch.h"
#include "dlshell.h"
#pragma  hdrstop


//
// Stub window stuff lifted from shell32\rundll32 
//


#define HINST_THISDLL GLOBAL_HINSTANCE

const TCHAR c_szPropSheet[]    = STRREG_SHEX_PROPSHEET;
const TCHAR c_szStubWindowClass[] = TEXT("StubWindow32");
const TCHAR c_szNULL[] = TEXT("");

#define MAX_FILE_PROP_PAGES 32

#define SHELL_PROPSHEET_STUB_CLASS 1

#define STUBM_SETDATA       (WM_USER)
#define STUBM_GETDATA       (WM_USER + 1)
#define STUBM_SETICONTITLE  (WM_USER + 2)

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case STUBM_SETICONTITLE:
        if (lParam)
            SetWindowText(hWnd, (LPCTSTR)lParam);
        if (wParam)
            SendMessage(hWnd, WM_SETICON, ICON_BIG, wParam);
        break;

    case STUBM_SETDATA:
        SetWindowLong(hWnd, 0, wParam);
        break;
        
    case STUBM_GETDATA:
        return GetWindowLong(hWnd, 0);
        
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam) ;
    }
    
    return 0;
}


HWND _CreateStubWindow(POINT * ppt)
{
    WNDCLASS wc = { 0 };
    int cx, cy;
    if (!GetClassInfo(HINST_THISDLL, c_szStubWindowClass, &wc))
    {
        wc.lpfnWndProc   = WndProc;
        wc.cbWndExtra    = SIZEOF(DWORD) * 2;
        wc.hInstance     = HINST_THISDLL;
        wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject (WHITE_BRUSH);
        wc.lpszClassName = c_szStubWindowClass;

        RegisterClass(&wc);
    }

    cx = cy = CW_USEDEFAULT;
    if (ppt)
    {
        cx = (int)ppt->x;
        cy = (int)ppt->y;
    }
    // WS_EX_APPWINDOW makes this show up in ALT+TAB, but not the tray.
        
    return CreateWindowEx(WS_EX_APPWINDOW, c_szStubWindowClass, c_szNULL, WS_OVERLAPPED, cx, cy, 0, 0, NULL, NULL, HINST_THISDLL, NULL);
}

//
//  This function is a callback function from property sheet page extensions.
//
BOOL CALLBACK _AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER * ppsh = (PROPSHEETHEADER *)lParam;

    if (ppsh->nPages < MAX_FILE_PROP_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}


//
//  This function enumerates all the property sheet page extensions for
// specified class and let them add pages.
//
//
int DCA_AppendClassSheetInfo(HDCA hdca, HKEY hkeyProgID, LPPROPSHEETHEADER ppsh, LPDATAOBJECT pdtobj)
{
    int i, iStart = -1;
    for (i = 0; i < DCA_GetItemCount(hdca); i++)
    {
        IShellExtInit *psei;
        if (DCA_CreateInstance(hdca, i, IID_IShellExtInit, (void**)&psei) == NOERROR)
        {
            IShellPropSheetExt *pspse;
            if (SUCCEEDED(psei->Initialize(NULL, pdtobj, hkeyProgID))
              && SUCCEEDED(psei->QueryInterface(IID_IShellPropSheetExt, (void**)&pspse)))
            {
                int nPagesSave = ppsh->nPages;
                HRESULT hres = pspse->AddPages(_AddPropSheetPage, (LPARAM)ppsh);
                if (SUCCEEDED(hres) && (ShortFromResult(hres) != 0))
                    iStart = (ShortFromResult(hres) - 1) + nPagesSave;
                pspse->Release();
            }
            psei->Release();
        }
    }
    return iStart;
}


//
// Stub window management stuff
//

HWND FindStubForPidl(LPCITEMIDLIST pidl)
{
    HWND hwnd;

    for (hwnd = FindWindow(c_szStubWindowClass, NULL); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        TCHAR szClass[80];

        // find stub windows only
        GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
        if (lstrcmpi(szClass, c_szStubWindowClass) == 0)
        {
            int iClass;
            HANDLE hClassPidl;
            DWORD dwProcId;

            GetWindowThreadProcessId(hwnd, &dwProcId);

            hClassPidl = (HANDLE)SendMessage(hwnd, STUBM_GETDATA, 0, 0);
            if (hClassPidl)
            {
                LPBYTE lpb;

                lpb = (LPBYTE)SHLockShared(hClassPidl, dwProcId);

                if (lpb)
                {
                    iClass = *(int *)lpb;

                    if (iClass == SHELL_PROPSHEET_STUB_CLASS &&
                        ILIsEqual(pidl, (LPITEMIDLIST)(lpb+SIZEOF(int))) )
                    {
                        SHUnlockShared(lpb);
                        return hwnd;
                    }
                    SHUnlockShared(lpb);
                }
            }
        }
    }
    return NULL;
}

HWND FindOtherStub(HIDA hida)
{
    LPITEMIDLIST pidl;
    HWND hwnd = NULL;

    if (hida && (HIDA_GetCount(hida) == 1) && (NULL != (pidl = HIDA_ILClone(hida, 0)))) {
        hwnd = FindStubForPidl(pidl);
        ILFree(pidl);
    }

    return hwnd;
}

HANDLE StuffStubWindowWithPidl(HWND hwnd, LPITEMIDLIST pidlT)
{
    DWORD dwProcId;
    HANDLE  hSharedClassPidl;
    UINT uidlSize;

    uidlSize = ILGetSize(pidlT);
    GetWindowThreadProcessId(hwnd, &dwProcId);

    hSharedClassPidl = SHAllocShared(NULL, SIZEOF(int)+uidlSize, dwProcId);
    if (hSharedClassPidl)
    {
        LPBYTE lpb = (LPBYTE)SHLockShared(hSharedClassPidl, dwProcId);
        if (lpb)
        {
            *(int *)lpb = SHELL_PROPSHEET_STUB_CLASS;
            memcpy(lpb+SIZEOF(int),pidlT, uidlSize);
            SHUnlockShared(lpb);
            SendMessage(hwnd, STUBM_SETDATA, (WPARAM)hSharedClassPidl, 0);
            return hSharedClassPidl;
        }
        SHFreeShared(hSharedClassPidl, dwProcId);
    }

    return NULL;
}

HANDLE StuffStubWindow(HWND hwnd, HIDA hida)
{
    LPITEMIDLIST pidlT = NULL;
    HANDLE hClassPidl = NULL;

    if (hida && (HIDA_GetCount(hida) == 1) && (NULL != (pidlT = HIDA_ILClone(hida, 0)))) {
        hClassPidl = StuffStubWindowWithPidl(hwnd, pidlT);
        ILFree(pidlT);
    }
    return hClassPidl;
}


BOOL _IsAnyDuplicatedKey(HKEY ahkeys[], UINT ckeys, HKEY hkey)
{
    UINT ikey;
    for (ikey=0; ikey<ckeys; ikey++)
    {
        if (ahkeys[ikey]==hkey) {
            return TRUE;
        }
    }
    return FALSE;
}

EXTERN_C BOOL MySHOpenPropSheet(LPCTSTR pszCaption,
                                HKEY ahkeys[],
                                UINT ckeys,
                                const CLSID * pclsidDef,    OPTIONAL
                                LPDATAOBJECT pdtobj,
                                IShellBrowser * psb,
                                LPCTSTR pStartPage)         OPTIONAL
{
    BOOL fSuccess = FALSE;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpage[MAX_FILE_PROP_PAGES];
    HWND hwnd = NULL;
    STGMEDIUM medium;
    HANDLE hClassPidl = NULL;
    HDCA hdca = NULL;

    ASSERT(IS_VALID_STRING_PTR(pszCaption, -1));
    ASSERT(NULL == pclsidDef || IS_VALID_READ_PTR(pclsidDef, CLSID));
    ASSERT(IS_VALID_CODE_PTR(pdtobj, DATAOBJECT));
    ASSERT(NULL == psb || IS_VALID_CODE_PTR(psb, IShellBrowser));
    ASSERT(NULL == pStartPage || IS_VALID_STRING_PTR(pStartPage, -1));
    
    if (DataObj_GetHIDA(pdtobj, &medium))
    {
        if (NULL != (hwnd = FindOtherStub(medium.hGlobal)))
        {
            SHReleaseStgMedium(&medium);
            SwitchToThisWindow(GetLastActivePopup(hwnd), TRUE);
            return TRUE;
        }
        else 
        {
            POINT pt;
            POINT * ppt = NULL;
            if (SUCCEEDED(DataObj_GetOFFSETs(pdtobj, &pt)))
                ppt = &pt;
            
            if (NULL != (hwnd = _CreateStubWindow(ppt)))
                hClassPidl = StuffStubWindow(hwnd, medium.hGlobal);
        }
    
        HIDA_ReleaseStgMedium(NULL, &medium);
    }

    psh.hwndParent = hwnd;
    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hInstance = HINST_THISDLL;
    psh.pszCaption = pszCaption;
    psh.nPages = 0;     // incremented in callback
    psh.nStartPage = 0;   // set below if specified
    psh.phpage = ahpage;
    if (pStartPage)
    {
        psh.dwFlags |= PSH_USEPSTARTPAGE;
        psh.pStartPage = pStartPage;
    }

    hdca = DCA_Create();
    if (hdca)
    {
        UINT ikey;
        int nStartPage;
        //
        // Always add this default extention at the top, if any.
        //
        if (pclsidDef)
        {
            DCA_AddItem(hdca, *pclsidDef);
        }

        for (ikey = 0; ikey < ckeys; ikey++)
        {
            if (ahkeys[ikey] && !_IsAnyDuplicatedKey(ahkeys, ikey, ahkeys[ikey]))
            {
                DCA_AddItemsFromKey(hdca, ahkeys[ikey], c_szPropSheet);
            }
        }

        // Notes: ahkeys[ckeys-1] as hkeyProgID
        ASSERT(ckeys);
        nStartPage = DCA_AppendClassSheetInfo(hdca, ahkeys[ckeys-1], &psh, pdtobj);
        if (nStartPage >= 0)
        psh.nStartPage = nStartPage;
        DCA_Destroy(hdca);
    }

    // Open the property sheet, only if we have some pages.
    if (psh.nPages > 0)
    {
        _try
        {
            if (PropertySheet(&psh) >= 0)   // IDOK or IDCANCEL (< 0 is error)
                fSuccess = TRUE;
        }
        _except(UnhandledExceptionFilter(GetExceptionInformation()))
        {
            DebugMsg(DM_ERROR, TEXT("PRSHT: Fault in property sheet"));
        }
    }
    else
    {
        TCHAR szTitle[MAX_PATH];
        TCHAR szPrompt[MAX_PATH];

        if ( LoadString(GLOBAL_HINSTANCE, IDS_DESKTOP, szTitle, ARRAYSIZE(szTitle)) &&
                LoadString(GLOBAL_HINSTANCE, IDS_NOPAGE, szPrompt, ARRAYSIZE(szPrompt)) )
        {
            MessageBox(NULL, szPrompt, szTitle, MB_SETFOREGROUND|MB_OK|MB_ICONHAND);
        }
    }

    // clean up the stub window and data
    SHFreeShared(hClassPidl,GetCurrentProcessId());
    if (psh.hwndParent)
        DestroyWindow(psh.hwndParent);

    return fSuccess;
}
