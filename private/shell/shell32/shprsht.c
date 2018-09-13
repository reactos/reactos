#include "shellprv.h"
#pragma  hdrstop

#include "datautil.h"

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
int DCA_AppendClassSheetInfo(HDCA hdca, HKEY hkeyProgID, LPPROPSHEETHEADER ppsh, IDataObject *pdtobj)
{
    int i, iStart = -1;
    for (i = 0; i < DCA_GetItemCount(hdca); i++)
    {
        IShellExtInit *psei;
        if (DCA_CreateInstance(hdca, i, &IID_IShellExtInit, &psei) == NOERROR)
        {
            IShellPropSheetExt *pspse;
            if (SUCCEEDED(psei->lpVtbl->Initialize(psei, NULL, pdtobj, hkeyProgID))
              && SUCCEEDED(psei->lpVtbl->QueryInterface(psei, &IID_IShellPropSheetExt, &pspse)))
            {
                int nPagesSave = ppsh->nPages;
                HRESULT hres = pspse->lpVtbl->AddPages(pspse, _AddPropSheetPage, (LPARAM)ppsh);
                if (SUCCEEDED(hres) && (ShortFromResult(hres) != 0))
                    iStart = (ShortFromResult(hres) - 1) + nPagesSave;
                pspse->lpVtbl->Release(pspse);
            }
            psei->lpVtbl->Release(psei);
        }
    }
    return iStart;
}

HWND FindStubForPidlClass(LPCITEMIDLIST pidl, int iClass)
{
    HWND hwnd;

    if (!pidl)
        return NULL;

    for (hwnd = FindWindow(c_szStubWindowClass, NULL); hwnd; hwnd = GetWindow(hwnd, GW_HWNDNEXT))
    {
        TCHAR szClass[80];

        // find stub windows only
        GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
        if (lstrcmpi(szClass, c_szStubWindowClass) == 0)
        {
            HANDLE hClassPidl;
            DWORD dwProcId;

            GetWindowThreadProcessId(hwnd, &dwProcId);

            hClassPidl = (HANDLE)SendMessage(hwnd, STUBM_GETDATA, 0, 0);
            if (hClassPidl)
            {
                LPBYTE lpb = (LPBYTE)SHLockShared(hClassPidl, dwProcId);
                if (lpb)
                {
                    int iClassFound = *(int *)lpb;

                    if (iClassFound == iClass &&
                        ILIsEqual(pidl, (LPITEMIDLIST)(lpb + SIZEOF(int))) )
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

HANDLE _StuffStubWindow(HWND hwnd, LPITEMIDLIST pidlT, int iClass)
{
    DWORD dwProcId;
    HANDLE  hSharedClassPidl;
    UINT uidlSize;

    uidlSize = ILGetSize(pidlT);
    GetWindowThreadProcessId(hwnd, &dwProcId);

    hSharedClassPidl = SHAllocShared(NULL, SIZEOF(int)+uidlSize, dwProcId);
    if (hSharedClassPidl)
    {
        LPBYTE lpb = SHLockShared(hSharedClassPidl, dwProcId);
        if (lpb)
        {
            *(int *)lpb = iClass;
            memcpy(lpb+SIZEOF(int),pidlT, uidlSize);
            SHUnlockShared(lpb);
            SendMessage(hwnd, STUBM_SETDATA, (WPARAM)hSharedClassPidl, 0);
            return hSharedClassPidl;
        }
        SHFreeShared(hSharedClassPidl, dwProcId);
    }

    return NULL;
}

//
//  Make sure we are the only stub window for this pidl/class.
//
//  If so, saves information in the UNIQUESTUBINFO structure which keeps
//  track of the uniqueness key.  After you are done, you must pass the
//  UNIQUESTUBINFO structure to FreeUniqueStub() to clean up the uniqueness
//  key and destroy the stub window.  Returns TRUE.
//
//  If a stub window already exists for this pidl/class, then sets focus
//  to the existing window that matches our uniqueness key and returns FALSE.
//
//  In low memory conditions, plays it safe and declares the pidl/class
//  unique.
//
//
//  Example:
//
//      UNIQUESTUBINFO usi;
//      if (EnsureUniqueStub(pidl, STUBCLASS_PROPSHEET, NULL, &usi)) {
//          DoStuff(usi.hwndStub, pidl);
//          FreeUniqueStub(&usi);
//      }
//

STDAPI_(BOOL)
EnsureUniqueStub(LPITEMIDLIST pidl, int iClass, POINT *ppt, UNIQUESTUBINFO *pusi)
{
    HWND hwndOther;

    ZeroMemory(pusi, sizeof(UNIQUESTUBINFO));

    hwndOther = FindStubForPidlClass(pidl, iClass);
    if (hwndOther)
    {
        SwitchToThisWindow(GetLastActivePopup(hwndOther), TRUE);
        return FALSE;
    }
    else
    {   // Tag ourselves as the unique stub for this pidl/class
        pusi->hwndStub = _CreateStubWindow(ppt, NULL);

        // If no pidl, then nothing to tag *with*
        // If no stub window, then nothing to attach the tag *to*
        // But they are both still considered success.

        if (pusi->hwndStub && pidl)
        {
            SHFILEINFO sfi;

            pusi->hClassPidl = _StuffStubWindow(pusi->hwndStub, pidl, iClass);

            if (SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_PIDL)) {
                pusi->hicoStub = sfi.hIcon;

                // Cannot stuff the title because the window might belong to another process
                SendMessage(pusi->hwndStub, STUBM_SETICONTITLE, (WPARAM)pusi->hicoStub, 0);

            }
        }
        return TRUE;
    }
}

STDAPI_(void) FreeUniqueStub(UNIQUESTUBINFO *pusi)
{
    if (pusi->hwndStub)
        DestroyWindow(pusi->hwndStub);
    if (pusi->hClassPidl)
        SHFreeShared(pusi->hClassPidl, GetCurrentProcessId());
    if (pusi->hicoStub)
        DestroyIcon(pusi->hicoStub);
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

STDAPI_(BOOL) SHOpenPropSheet(
    LPCTSTR pszCaption,
    HKEY ahkeys[],
    UINT ckeys,
    const CLSID * pclsidDef,    OPTIONAL
    IDataObject *pdtobj,
    IShellBrowser * psb,
    LPCTSTR pStartPage)         OPTIONAL
{
    BOOL fSuccess = FALSE;
    BOOL fUnique;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpage[MAX_FILE_PROP_PAGES];
    HWND hwndStub = NULL;
    STGMEDIUM medium;
    HDCA hdca = NULL;
    HICON hicoStuff = NULL;
    UNIQUESTUBINFO usi;

    ASSERT(IS_VALID_STRING_PTR(pszCaption, -1));
    ASSERT(NULL == pclsidDef || IS_VALID_READ_PTR(pclsidDef, CLSID));
    ASSERT(IS_VALID_CODE_PTR(pdtobj, DATAOBJECT));
    ASSERT(NULL == psb || IS_VALID_CODE_PTR(psb, IShellBrowser));
    ASSERT(NULL == pStartPage || IS_VALID_STRING_PTR(pStartPage, -1));

    // Create the stub window
    {
        POINT pt;
        POINT * ppt = NULL;
        LPITEMIDLIST pidl = NULL;

        if (SUCCEEDED(DataObj_GetOFFSETs(pdtobj, &pt)))
            ppt = &pt;

        if (DataObj_GetHIDA(pdtobj, &medium))
        {
            HIDA hida = medium.hGlobal;
            if (hida && (HIDA_GetCount(hida) == 1))
            {
                pidl = HIDA_ILClone(hida, 0);
            }
            HIDA_ReleaseStgMedium(NULL, &medium);
        }


        fUnique = EnsureUniqueStub(pidl, STUBCLASS_PROPSHEET, ppt, &usi);
        ILFree(pidl);
    }

    // If there's already a property sheet up for this guy, then our job is done
    if (!fUnique) {
        return TRUE;
    }

    psh.hwndParent = usi.hwndStub;
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
            DCA_AddItem(hdca, pclsidDef);
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
        ShellMessageBox(HINST_THISDLL, NULL,
                        MAKEINTRESOURCE(IDS_NOPAGE),
                        MAKEINTRESOURCE(IDS_DESKTOP),
                        MB_OK|MB_ICONHAND);
    }

    // clean up the stub window and data
    FreeUniqueStub(&usi);

    return fSuccess;
}


#ifdef UNICODE

STDAPI_(BOOL) SHOpenPropSheetA(
    LPCSTR pszCaption,
    HKEY ahkeys[],
    UINT ckeys,
    const CLSID * pclsidDef,
    IDataObject *pdtobj,
    IShellBrowser * psb,
    LPCSTR pszStartPage)       OPTIONAL
{
    BOOL bRet = FALSE;

    if (IS_VALID_STRING_PTRA(pszCaption, MAX_PATH))
    {
        WCHAR wszCaption[MAX_PATH];
        WCHAR wszStartPage[MAX_PATH];

        SHAnsiToUnicode(pszCaption, wszCaption, SIZECHARS(wszCaption));

        if (pszStartPage)
        {
            ASSERT(IS_VALID_STRING_PTRA(pszStartPage, MAX_PATH));

            SHAnsiToUnicode(pszStartPage, wszStartPage, SIZECHARS(wszStartPage));
            pszStartPage = (LPCSTR)wszStartPage;
        }

        bRet = SHOpenPropSheet(wszCaption, ahkeys, ckeys, pclsidDef, pdtobj, psb, (LPCWSTR)pszStartPage);
    }

    return bRet;
}

#else

STDAPI_(BOOL) SHOpenPropSheetW(
    LPCWSTR pszCaption,
    HKEY ahkeys[],
    UINT ckeys,
    const CLSID * pclsidDef,
    IDataObject *pdtobj,
    IShellBrowser * psb,
    LPCWSTR pszStartPage)       OPTIONAL
{
    BOOL bRet = FALSE;

    if (IS_VALID_STRING_PTRW(pszCaption, MAX_PATH))
    {
        char szCaption[MAX_PATH];
        char szStartPage[MAX_PATH];

        SHUnicodeToAnsi(pszCaption, szCaption, SIZECHARS(szCaption));

        if (pszStartPage)
        {
            ASSERT(IS_VALID_STRING_PTRW(pszStartPage, MAX_PATH));

            SHUnicodeToAnsi(pszStartPage, szStartPage, SIZECHARS(szStartPage));
            pszStartPage = (LPCWSTR)szStartPage;
        }

        bRet = SHOpenPropSheet(szCaption, ahkeys, ckeys, pclsidDef, pdtobj, psb, (LPCSTR)pszStartPage);
    }

    return bRet;
}

#endif // UNICODE

//
//  Async version of SHFormatDrive - creates a separate thread to do the
//  format and returns immediately.
//

typedef struct FORMATINFO {
    HWND hwnd;
    UINT drive;
    UINT fmtID;
    UINT options;
} FORMATINFO, *LPFORMATINFO;

STDAPI_(DWORD) _FormatThreadProc(LPVOID lpParam)
{
    LPFORMATINFO pfi = (LPFORMATINFO)lpParam;
    LPITEMIDLIST pidl;
    TCHAR szDrive[4];

    lstrcpy(szDrive, TEXT("A:\\"));
    ASSERT(pfi->drive < 26);
    szDrive[0] += (TCHAR)pfi->drive;

    pidl = ILCreateFromPath(szDrive);
    if (pidl)
    {
        UNIQUESTUBINFO usi;
        LPPOINT ppt = NULL;
        RECT rcWindow;
        if (pfi->hwnd)
        {
            GetWindowRect(pfi->hwnd, &rcWindow);
            ppt = (LPPOINT)&rcWindow;
        }

        if (EnsureUniqueStub(pidl, STUBCLASS_FORMAT, ppt, &usi))
        {
            SHFormatDrive(usi.hwndStub, pfi->drive, pfi->fmtID, pfi->options);
            FreeUniqueStub(&usi);
        }
        ILFree(pidl);
    }
    LocalFree(pfi);
    return 0;
}

STDAPI_(void) SHFormatDriveAsync(
    HWND hwnd,
    UINT drive,
    UINT fmtID,
    UINT options
)
{
    LPFORMATINFO pfi = (LPFORMATINFO)LocalAlloc(LPTR, sizeof(FORMATINFO));
    if (pfi)
    {
        pfi->hwnd = hwnd;
        pfi->drive = drive;
        pfi->fmtID = fmtID;
        pfi->options = options;
        SHCreateThread(_FormatThreadProc, pfi, CTF_INSIST | CTF_PROCESS_REF, NULL);
    }
}
