#include "shellprv.h"
#pragma  hdrstop

#include "copy.h"

UINT DefView_CopyHook(const COPYHOOKINFO *pchi);
int PathCopyHookCallback(HWND hwnd, UINT wFunc, LPCTSTR pszSrc, LPCTSTR pszDest);

void _CopyHookTerminate(HDSA hdsaCopyHooks, BOOL fProcessDetach);

typedef struct {
    ICopyHook * pcphk;              // Either ICopyHookA *or LPCOPYHOOK
    BOOL        fAnsiCrossOver;     // TRUE for ICopyHookA *on UNICODE build
} CALLABLECOPYHOOK;

typedef struct
{
    ICopyHook           cphk;
#ifdef UNICODE
    ICopyHookA          cphkA;
#endif
    LONG                cRef;
} CCopyHook;

STDMETHODIMP_(ULONG) CCopyHook_AddRef(ICopyHook *pcphk);	// forward


STDMETHODIMP CCopyHook_QueryInterface(ICopyHook *pcphk, REFIID riid, void **ppvObj)
{
    CCopyHook *this = IToClass(CCopyHook, cphk, pcphk);
    if (IsEqualIID(riid, &IID_IShellCopyHook) || 
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = pcphk;
    }
#ifdef UNICODE
    else if (IsEqualIID(riid, &IID_IShellCopyHookA))
    {
        *ppvObj = &this->cphkA;
    }
#endif
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    CCopyHook_AddRef(&this->cphk);
    return NOERROR;
}

STDMETHODIMP_(ULONG) CCopyHook_AddRef(ICopyHook *pcphk)
{
    CCopyHook *this = IToClass(CCopyHook, cphk, pcphk);
    return InterlockedIncrement(&this->cRef);
}

STDMETHODIMP_(ULONG) CCopyHook_Release(ICopyHook *pcphk)
{
    CCopyHook *this = IToClass(CCopyHook, cphk, pcphk);

    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    LocalFree((HLOCAL)this);
    return 0;
}

STDMETHODIMP_(UINT) CCopyHook_CopyCallback(ICopyHook *pcphk, HWND hwnd, UINT wFunc, UINT wFlags, 
    LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs)
{
    COPYHOOKINFO chi = { hwnd, wFunc, wFlags, pszSrcFile, dwSrcAttribs, pszDestFile, dwDestAttribs };
    
    DebugMsg(DM_TRACE, TEXT("Event = %d, File = %s , %s"), wFunc, pszSrcFile,
        Dbg_SafeStr(pszDestFile));
    
    // check Special Folders first...
    if (PathCopyHookCallback(hwnd, wFunc, pszSrcFile, pszDestFile) == IDNO)
    {
        return IDNO;
    }
    
    if (wFunc != FO_COPY && !(wFlags & FOF_NOCONFIRMATION))
    {
        TCHAR szShortName[MAX_PATH];
        BOOL fInReg = (RLIsPathInList(pszSrcFile) != -1);
        BOOL fInBitBucket = IsFileInBitBucket(pszSrcFile);
        UINT iLength = GetShortPathName(pszSrcFile, szShortName, ARRAYSIZE(szShortName));
        
        // Don't double search for names that are the same (or already found)
        if (iLength != 0 && lstrcmpi(pszSrcFile, szShortName) != 0)
        {
            if (!fInReg)
                fInReg = (RLIsPathInList(szShortName) != -1);
            if (!fInBitBucket)
                fInBitBucket = IsFileInBitBucket(szShortName);
        }
        
        if (fInReg && !fInBitBucket)
        {
            LPCTSTR pszSpec = PathFindFileName(pszSrcFile);
            return ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_RENAMEFILESINREG),
                pszSpec, MB_YESNO | MB_ICONEXCLAMATION, pszSpec);
        }
    }
    return DefView_CopyHook(&chi);
}

ICopyHookVtbl c_CCopyHookVtbl = {
    CCopyHook_QueryInterface, CCopyHook_AddRef, CCopyHook_Release,
    CCopyHook_CopyCallback,
};

#ifdef UNICODE
STDMETHODIMP CCopyHookA_QueryInterface(ICopyHookA *pcphkA, REFIID riid, void **ppvObj)
{
    CCopyHook *this = IToClass(CCopyHook, cphkA, pcphkA);
    return CCopyHook_QueryInterface(&this->cphk,riid,ppvObj);
}

STDMETHODIMP_(ULONG) CCopyHookA_AddRef(ICopyHookA *pcphkA)
{
    CCopyHook *this = IToClass(CCopyHook, cphkA, pcphkA);
    return CCopyHook_AddRef(&this->cphk);
}

STDMETHODIMP_(ULONG) CCopyHookA_Release(ICopyHookA *pcphkA)
{
    CCopyHook *this = IToClass(CCopyHook, cphkA, pcphkA);
    return CCopyHook_Release(&this->cphk);
}

STDMETHODIMP_(UINT) CCopyHookA_CopyCallback(ICopyHookA *pcphkA, HWND hwnd, UINT wFunc, UINT wFlags, 
    LPCSTR pszSrcFile, DWORD dwSrcAttribs, LPCSTR pszDestFile, DWORD dwDestAttribs)
{
    WCHAR szSrcFileW[MAX_PATH];
    WCHAR szDestFileW[MAX_PATH];
    LPWSTR pszSrcFileW = NULL;
    LPWSTR pszDestFileW = NULL;
    CCopyHook *this = IToClass(CCopyHook, cphkA, pcphkA);

    if (pszSrcFile)
    {
        SHAnsiToUnicode(pszSrcFile, szSrcFileW, ARRAYSIZE(szSrcFileW));
        pszSrcFileW = szSrcFileW;
    }

    if (pszDestFile)
    {
        SHAnsiToUnicode(pszDestFile, szDestFileW, ARRAYSIZE(szDestFileW));
        pszDestFileW = szDestFileW;
    }

    return CCopyHook_CopyCallback(&this->cphk, hwnd, wFunc, wFlags,
                                         pszSrcFileW, dwSrcAttribs,
                                         pszDestFileW, dwDestAttribs);
}

ICopyHookAVtbl c_CCopyHookAVtbl = {
    CCopyHookA_QueryInterface, CCopyHookA_AddRef, CCopyHookA_Release,
    CCopyHookA_CopyCallback,
};

#endif

STDAPI SHCreateShellCopyHook(ICopyHook **pcphkOut, REFIID riid)
{
    HRESULT hres = E_OUTOFMEMORY;      // assume error;
    CCopyHook *pcphk = (void*)LocalAlloc(LPTR, SIZEOF(CCopyHook));
    if (pcphk)
    {
        pcphk->cphk.lpVtbl = &c_CCopyHookVtbl;
#ifdef UNICODE
        pcphk->cphkA.lpVtbl = &c_CCopyHookAVtbl;
#endif
        pcphk->cRef = 1;
        hres = CCopyHook_QueryInterface(&pcphk->cphk, riid, pcphkOut);
        CCopyHook_Release(&pcphk->cphk);
    }
    return hres;
}

HRESULT CCopyHook_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return SHCreateShellCopyHook((ICopyHook **)ppv, riid);
}


// create the HDSA of copyhook objects

HDSA CreateCopyHooks(LPCTSTR pszKey)
{
    HDSA hdsaCopyHooks = DSA_Create(SIZEOF(CALLABLECOPYHOOK), 4);
    if (hdsaCopyHooks)
    {
        HKEY hk;

        if (RegOpenKey(HKEY_CLASSES_ROOT, pszKey, &hk) == ERROR_SUCCESS) 
        {
            int i;
            TCHAR szKey[128];

            // iterate through the subkeys
            for (i = 0; RegEnumKey(hk, i, szKey, ARRAYSIZE(szKey)) == ERROR_SUCCESS; ++i) 
            {
                TCHAR szCLSID[128];
                LONG cb = SIZEOF(szCLSID);

                // for each subkey, get the class id and do a cocreateinstance
                if (SHRegQueryValue(hk, szKey, szCLSID, &cb) == ERROR_SUCCESS) 
                {
                    IUnknown *punk;
                    HRESULT hres = SHExtCoCreateInstance(szCLSID, NULL, NULL, &IID_IUnknown, &punk);
                    if (SUCCEEDED(hres)) 
                    {
                        CALLABLECOPYHOOK cc;

                        SHPinDllOfCLSIDStr(szCLSID);

                        cc.pcphk = NULL;
                        cc.fAnsiCrossOver = FALSE;
                        hres = punk->lpVtbl->QueryInterface(punk, &IID_IShellCopyHook, &cc.pcphk);
                        if (SUCCEEDED(hres))
                        {
                            DSA_AppendItem(hdsaCopyHooks, &cc);
                        }
    #ifdef UNICODE
                        else
                        {
                            hres = punk->lpVtbl->QueryInterface(punk, &IID_IShellCopyHookA, &cc.pcphk);
                            if (SUCCEEDED(hres))
                            {
                                cc.fAnsiCrossOver = TRUE;
                                DSA_AppendItem(hdsaCopyHooks, &cc);
                            }
                        }
    #endif
                        punk->lpVtbl->Release(punk);
                    }
                }
            }
            RegCloseKey(hk);
        }
    }
    return hdsaCopyHooks;
}

int CallCopyHooks(HDSA *phdsaHooks, LPCTSTR pszKey, HWND hwnd, UINT wFunc, FILEOP_FLAGS fFlags,
    LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs)
{
    int i;

    if (!*phdsaHooks)
    {
        HDSA hdsaTemp = CreateCopyHooks(pszKey);
        if (hdsaTemp == NULL)
            return IDYES;

        // we don't hold a CritSection when doing the above to avoid deadlocks,
        // now we need to atomicaly store our results. if someone beat us to this
        // we free the hdsa we created. SHInterlockedCompareExchange does this for us
        // letting us know where there is a race condition so we can free the dup copy
        if (SHInterlockedCompareExchange((void **)phdsaHooks, hdsaTemp, 0))
        {
            // some other thread raced with us, blow this away now
            _CopyHookTerminate(hdsaTemp, FALSE);
        }
    }

    for (i = DSA_GetItemCount(*phdsaHooks) - 1; i >= 0; i--) 
    {
        int iReturn;
        CALLABLECOPYHOOK *pcc = (CALLABLECOPYHOOK *)DSA_GetItemPtr(*phdsaHooks, i);
#ifdef UNICODE
        if (!pcc->fAnsiCrossOver)
        {
#endif
            iReturn = pcc->pcphk->lpVtbl->CopyCallback(pcc->pcphk,
                hwnd, wFunc, fFlags, pszSrcFile, dwSrcAttribs, pszDestFile, dwDestAttribs);
#ifdef UNICODE
        }
        else
        {
            CHAR szSrcFileA[MAX_PATH];
            CHAR szDestFileA[MAX_PATH];
            LPSTR pszSrcFileA = NULL;
            LPSTR pszDestFileA = NULL;
            ICopyHookA *pcphkA = (LPCOPYHOOKA)pcc->pcphk;

            if (pszSrcFile)
            {
                SHUnicodeToAnsi(pszSrcFile, szSrcFileA, ARRAYSIZE(szSrcFileA));
                pszSrcFileA = szSrcFileA;
            }
            if (pszDestFile)
            {
                SHUnicodeToAnsi(pszDestFile, szDestFileA, ARRAYSIZE(szDestFileA));
                pszDestFileA = szDestFileA;
            }
            iReturn = pcphkA->lpVtbl->CopyCallback(pcphkA,
                                       hwnd, wFunc, fFlags,
                                       pszSrcFileA, dwSrcAttribs,
                                       pszDestFileA, dwDestAttribs);
        }
#endif
        if (iReturn != IDYES)
            return iReturn;
    }
    return IDYES;
}

// These need to be per-instance since we are storing interfaces pointers
HDSA g_hdsaFileCopyHooks = NULL;
HDSA g_hdsaPrinterCopyHooks = NULL;

int CallFileCopyHooks(HWND hwnd, UINT wFunc, FILEOP_FLAGS fFlags,
    LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs)
{
    return CallCopyHooks(&g_hdsaFileCopyHooks, STRREG_SHEX_COPYHOOK, hwnd, 
        wFunc, fFlags, pszSrcFile, dwSrcAttribs, pszDestFile, dwDestAttribs);
}

int CallPrinterCopyHooks(HWND hwnd, UINT wFunc, PRINTEROP_FLAGS fFlags,
    LPCTSTR pszSrcPrinter, DWORD dwSrcAttribs, LPCTSTR pszDestPrinter, DWORD dwDestAttribs)
{
    return CallCopyHooks(&g_hdsaPrinterCopyHooks, STRREG_SHEX_PRNCOPYHOOK, hwnd, 
        wFunc, fFlags, pszSrcPrinter, dwSrcAttribs, pszDestPrinter, dwDestAttribs);
}

//
// We will only call this on process detach, and these are per-process
// globals, so we do not need a critical section here
//
//  This function is also called from CreateCopyHooks when the second
// thread is cleaning up its local hdsaCopyHoos, which does not require
// a critical section either.
//
void _CopyHookTerminate(HDSA hdsaCopyHooks, BOOL fProcessDetach)
{
    //  Note that we must no call any of virtual functions when we are
    // processing PROCESS_DETACH signal, because the DLL might have been
    // already unloaded before shell32. We just hope that they don't
    // allocate any global thing to be cleaned. USER does the same thing
    // with undestroyed window. It does not send call its window procedure
    // when it is destroying an undestroyed window within its PROCESS_DETACH
    // code. (SatoNa/DavidDS)
    //
    if (!fProcessDetach)
    {
        int i;
        for (i = DSA_GetItemCount(hdsaCopyHooks) - 1; i >= 0; i--) 
        {
            CALLABLECOPYHOOK *pcc = (CALLABLECOPYHOOK *)DSA_GetItemPtr(hdsaCopyHooks, i);
            pcc->pcphk->lpVtbl->Release(pcc->pcphk);
        }
    }

    DSA_Destroy(hdsaCopyHooks);
}


// called from ProcessDetatch
// NOTE: we are seralized at this point, don't need critical sections

void CopyHooksTerminate(void)
{
    ASSERTDLLENTRY;      // does not require a critical section

    if (g_hdsaFileCopyHooks)
    {
        _CopyHookTerminate(g_hdsaFileCopyHooks, TRUE);
        g_hdsaFileCopyHooks = NULL;
    }

    if (g_hdsaPrinterCopyHooks)
    {
        _CopyHookTerminate(g_hdsaPrinterCopyHooks, TRUE);
        g_hdsaPrinterCopyHooks = NULL;
    }
}
