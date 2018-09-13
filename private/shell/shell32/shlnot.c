#include "shellprv.h"
#pragma  hdrstop

#include <trayp.h>

TCHAR const c_szTrayClass[] = TEXT(WNDCLASS_TRAYNOTIFY);
BOOL AllowSetForegroundWindow(DWORD dwProcessId);

STDAPI_(BOOL) Shell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATA lpData)
{
    HWND hwndTray;

    SetLastError(0);        // Clean any previous last error (code to help catch another bug)

    hwndTray = FindWindow(c_szTrayClass, NULL);
    if (hwndTray)
    {
        COPYDATASTRUCT cds;
        TRAYNOTIFYDATA tnd = {0};
        DWORD_PTR dwRes = FALSE;
        DWORD dwValidFlags;

        int cbSize = lpData->cbSize;

        if (cbSize == SIZEOF(NOTIFYICONDATA))
        {
            dwValidFlags = NIF_VALID;
        }
        else
        {
            // This will RIP if the app was stupid and passed stack
            // garbage as cbSize.  Apps got away with this on Win95
            // and NT4 because those versions didn't validate cbSize.
            // So if we see a strange cbSize, assume it's the V1 size.
            RIP(cbSize == NOTIFYICONDATA_V1_SIZE);
            cbSize = NOTIFYICONDATA_V1_SIZE;

            dwValidFlags = NIF_VALID_V1;
        }

#ifdef  _WIN64
        // Thunking NOTIFYICONDATA to NOTIFYICONDATA32 is annoying
        // on Win64 due to variations in the size of HWND and HICON
        // We have to copy each field individually.
        tnd.nid.dwWnd            = PtrToUlong(lpData->hWnd);
        tnd.nid.uID              = lpData->uID;
        tnd.nid.uFlags           = lpData->uFlags;
        tnd.nid.uCallbackMessage = lpData->uCallbackMessage;
        tnd.nid.dwIcon           = PtrToUlong(lpData->hIcon);

        // The rest of the fields don't change size between Win32 and
        // Win64, so just block copy them over

        // Toss in an assertion to make sure
        COMPILETIME_ASSERT(
            sizeof(NOTIFYICONDATA  ) - FIELD_OFFSET(NOTIFYICONDATA  , szTip) ==
            sizeof(NOTIFYICONDATA32) - FIELD_OFFSET(NOTIFYICONDATA32, szTip));

        memcpy(&tnd.nid.szTip, &lpData->szTip, cbSize - FIELD_OFFSET(NOTIFYICONDATA, szTip));

#else
        // On Win32, the two structures are the same
        COMPILETIME_ASSERT(sizeof(NOTIFYICONDATA) == sizeof(NOTIFYICONDATA32));
        memcpy(&tnd.nid, lpData, cbSize);
#endif

        tnd.nid.cbSize = sizeof(NOTIFYICONDATA32);

        // This will RIP if the app was really stupid and passed stack
        // garbage as uFlags.
        RIP(!(lpData->uFlags & ~dwValidFlags));
        tnd.nid.uFlags &= dwValidFlags;

        if (dwMessage == NIM_SETFOCUS)
        {
            DWORD dwProcId;
            GetWindowThreadProcessId(hwndTray, &dwProcId);
            AllowSetForegroundWindow(dwProcId);
        }
        
        tnd.dwSignature = NI_SIGNATURE;
        tnd.dwMessage = dwMessage;

        cds.dwData = TCDM_NOTIFY;
        cds.cbData = SIZEOF(tnd);
        cds.lpData = &tnd;

        if (SendMessageTimeout(hwndTray, WM_COPYDATA, (WPARAM)lpData->hWnd, (LPARAM)&cds,
            SMTO_ABORTIFHUNG | SMTO_BLOCK, 4000, &dwRes))
        {
            return (BOOL) dwRes;
        }
    }

    return FALSE;
}

#ifdef UNICODE
STDAPI_(BOOL) Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA lpData)
{
    NOTIFYICONDATAW tndw;
    
    // Clear all fields by default in our local UNICODE copy

    memset(&tndw, 0, SIZEOF(NOTIFYICONDATAW));

    tndw.cbSize           = SIZEOF(NOTIFYICONDATAW);
    tndw.hWnd             = lpData->hWnd;
    tndw.uID              = lpData->uID;
    tndw.uFlags           = lpData->uFlags;
    tndw.uCallbackMessage = lpData->uCallbackMessage;
    tndw.hIcon            = lpData->hIcon;

    // Transfer those fields we are aware of as of this writing
    if (lpData->cbSize == SIZEOF(NOTIFYICONDATAA)) 
    {
        tndw.dwState = lpData->dwState;
        tndw.dwStateMask = lpData->dwStateMask;
        tndw.uTimeout    = lpData->uTimeout;
        tndw.dwInfoFlags = lpData->dwInfoFlags;

        // We defer validating uFlags to Shell_NotifyIconW
    }
    else 
    {
        // This will RIP if the app was stupid and passed stack
        // garbage as cbSize.  Apps got away with this on Win95
        // and NT4 because those versions didn't validate cbSize.
        // So if we see a strange cbSize, assume it's the V1 size.
        RIP(lpData->cbSize == (DWORD)NOTIFYICONDATAA_V1_SIZE);
        tndw.cbSize = NOTIFYICONDATAW_V1_SIZE;

        // This will RIP if the app was really stupid and passed stack
        // garbage as uFlags.  We have to clear out bogus flags to
        // avoid accidentally trying to read from invalid data.
        RIP(!(lpData->uFlags & ~NIF_VALID_V1));
        tndw.uFlags &= NIF_VALID_V1;
    }

    if (tndw.uFlags & NIF_TIP)
        SHAnsiToUnicode(lpData->szTip, tndw.szTip, ARRAYSIZE(tndw.szTip));

    if (tndw.uFlags & NIF_INFO)
    {
        SHAnsiToUnicode(lpData->szInfo, tndw.szInfo, ARRAYSIZE(tndw.szInfo));
        SHAnsiToUnicode(lpData->szInfoTitle, tndw.szInfoTitle, ARRAYSIZE(tndw.szInfoTitle));
    }

    return Shell_NotifyIconW(dwMessage, &tndw);
}
#else
STDAPI_(BOOL) Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW lpData)
{
    return FALSE;   // BUGBUG - BobDay - We should move this into SHUNIMP.C
}
#endif

//***   CopyIn -- copy app data in to shared region (and create shared)
// ENTRY/EXIT
//  return      handle on success, NULL on failure
//  pvData      app buffer
//  cbData      count
//  dwProcId    ...
// NOTES
//  should make it handle pvData=NULL for cases where param is OUT not INOUT.
//
HANDLE CopyIn(void *pvData, int cbData, DWORD dwProcId)
{
    HANDLE hShared;
    void *pvShared;

    hShared = SHAllocShared(NULL, cbData, dwProcId);
    if (hShared) {
        pvShared = SHLockShared(hShared, dwProcId);
        if (pvShared == NULL) {
            SHFreeShared(hShared, dwProcId);
            hShared = NULL;
        }
        else {
            memcpy(pvShared, pvData, cbData);
            SHUnlockShared(pvShared);
        }
    }
    return hShared;
}

//***   CopyOut -- copy out to app data from shared region (and free shared)
// ENTRY/EXIT
//  return      TRUE on success, FALSE on failure.
//  hShared     shared data, freed when done
//  pvData      app buffer
//  cbData      count
BOOL CopyOut(HANDLE hShared, void *pvData, int cbData, DWORD dwProcId)
{
    void *pvShared;

    pvShared = SHLockShared(hShared, dwProcId);
    if (pvShared != NULL) {
        memcpy(pvData, pvShared, cbData);
        SHUnlockShared(pvShared);
    }
    SHFreeShared(hShared, dwProcId);
    return (pvShared != 0);
}

STDAPI_(UINT) SHAppBarMessage(DWORD dwMessage, PAPPBARDATA pabd)
{
    HWND hwndTray;
    COPYDATASTRUCT cds;
    TRAYAPPBARDATA tabd;
    BOOL fret;
    APPBARDATA *pabdShared = NULL;

    hwndTray = FindWindow(c_szTrayClass, NULL);
    if (!hwndTray || (pabd->cbSize > SIZEOF(tabd.abd)))
    {
        return(FALSE);
    }

    tabd.abd = *pabd;
    tabd.dwMessage = dwMessage;
    tabd.hSharedABD = NULL;
    tabd.dwProcId = GetCurrentProcessId();

    cds.dwData = TCDM_APPBAR;
    cds.cbData = SIZEOF(tabd);
    cds.lpData = &tabd;

    switch (dwMessage) {
    case ABM_QUERYPOS:
    case ABM_SETPOS:
    case ABM_GETTASKBARPOS:
        tabd.hSharedABD = CopyIn(pabd, SIZEOF(*pabd), tabd.dwProcId);
        if (tabd.hSharedABD == NULL)
            return FALSE;

        break;
    }

    fret = (BOOL) (SendMessage(hwndTray, WM_COPYDATA, (WPARAM)pabd->hWnd, (LPARAM)&cds));

    if (tabd.hSharedABD) {
        if (!CopyOut(tabd.hSharedABD, pabd, SIZEOF(*pabd), tabd.dwProcId))
            fret = FALSE;
    }
    return fret;
}

STDAPI SHLoadInProc(REFCLSID rclsid)
{
    HWND hwndTray = FindWindow(c_szTrayClass, NULL);
    if (hwndTray)
    {
        COPYDATASTRUCT cds;
        CLSID clsid = *rclsid;

        cds.dwData = TCDM_LOADINPROC;
        cds.cbData = SIZEOF(CLSID);
        cds.lpData = &clsid;

        return (HRESULT)SendMessage(hwndTray, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
    }
    else
    {
        return E_FAIL;
    }
}

// used to implement a per process reference count for the main thread
// the browser msg loop and the proxy desktop use this to let other threads
// extend their lifetime. 
// there is a thread level equivelent of this, shlwapi SHGetThreadRef()/SHSetThreadRef()

IUnknown *g_punkProcessRef = NULL;

STDAPI_(void) SHSetInstanceExplorer(IUnknown *punk)
{
    g_punkProcessRef = punk;
}

// This should be thread safe since we grab the punk locally before
// checking/using it, plus it never gets freed since it is not actually
// alloced in Explorer so we can always use it

STDAPI SHGetInstanceExplorer(IUnknown **ppunk)
{
    *ppunk = g_punkProcessRef;
    if (*ppunk)
    {
        (*ppunk)->lpVtbl->AddRef(*ppunk);
        return NOERROR;
    }
    return E_FAIL;
}
