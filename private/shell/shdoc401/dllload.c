#include "priv.h"
#include <browseui.h>
#include "uemapp.h"

#include "..\lib\dllload.c"

//
//  To maintain sanity, please list all named imports in alphabetical order.
//  All ordinal imports in numerical order.
//

// --------- SHELL32.DLL ---------------

//
// ----  delay load post win95 shell32 private functions
//

HINSTANCE g_hinstShell32 = NULL;

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, SHSetShellWindowEx, 243,
           (HWND hwnd, HWND hwndChild), (hwnd, hwndChild));

DELAY_LOAD_SHELL(g_hinstShell32, shell32, int, RealDriveTypeFlags, 525,
           (int iDrive, BOOL fOKToHitNet), (iDrive, fOKToHitNet));

DELAY_LOAD_SHELL_VOID(g_hinstShell32, shell32, SHChangeNotifyReceive, 643,
           (LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra), (lEvent, uFlags, pidl, pidlExtra));

DELAY_LOAD_SHELL(g_hinstShell32, shell32, LPSHChangeNotificationLock, SHChangeNotification_Lock, 644,
           (HANDLE hChangeNotification, DWORD dwProcessId, LPITEMIDLIST **pppidl, LONG *plEvent),
           (hChangeNotification, dwProcessId, pppidl,  plEvent));

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, SHChangeNotification_Unlock, 645,
           (LPSHChangeNotificationLock pshcnl), (pshcnl));

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, SHChangeRegistrationReceive, 646,
           (HANDLE hChangeNotification, DWORD dwProcId), (hChangeNotification, dwProcId));

DELAY_LOAD_SHELL_VOID(g_hinstShell32, shell32, SHWaitOp_Operate, 648,
           (HANDLE hWaitOp, DWORD dwProcId), (hWaitOp, dwProcId));

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, __WriteCabinetState, 652,
       (LPCABINETSTATE lpState), (lpState));

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, __ReadCabinetState, 654,
       (LPCABINETSTATE lpState, int cLength), (lpState, cLength));

BOOL WriteCabinetState(LPCABINETSTATE lpState)
{
    BOOL fRc = __WriteCabinetState(lpState);
    if (fRc) {
        // We must do this by hand because the old Shell32 doesn't
        HANDLE hChange = SHGlobalCounterCreate(&GUID_FolderSettingsChange);
        SHGlobalCounterIncrement(hChange);
        SHGlobalCounterDestroy(hChange);
    }
    return fRc;
}

BOOL ReadCabinetState(LPCABINETSTATE lpState, int iSize)
{
    if (!g_fRunningOnNT && WhichPlatform() == PLATFORM_IE3)
    {
        // We at least need decent defaults for this case...
        lpState->cLength = sizeof(CABINETSTATE);
        lpState->fSimpleDefault            = TRUE;
        lpState->fFullPathTitle            = FALSE;
        lpState->fSaveLocalView            = TRUE;
        lpState->fNotShell                 = FALSE;
        lpState->fNewWindowMode            = FALSE; // can't simulate this one, use FALSE
        lpState->fShowCompColor            = FALSE;
        lpState->fDontPrettyNames          = FALSE;
        lpState->fAdminsCreateCommonGroups = TRUE;
        lpState->fUnusedFlags              = 0;
        lpState->fMenuEnumFilter           = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

        // Lie and say we read from the registry,
        // this avoids us calling WriteCabinetState
        return(TRUE);
    }
    else
    {
        return __ReadCabinetState(lpState, iSize);
    }
}

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, FileIconInit, 660,
           ( BOOL fRestoreCache ), ( fRestoreCache ));

DELAY_LOAD_SHELL(g_hinstShell32, shell32, BOOL, IsUserAnAdmin, 680, (), ());

DELAY_LOAD_SHELL_VOID(g_hinstShell32, shell32, CheckWinIniForAssocs, 711, (), ());

// -------- OLEAUT32.DLL --------
HINSTANCE g_hinstOLEAUT32 = NULL;

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, CreateErrorInfo,
    (ICreateErrorInfo **pperrinfo), (pperrinfo));

DELAY_LOAD_INT(g_hinstOLEAUT32, OLEAUT32, DosDateTimeToVariantTime,
    (USHORT wDosDate, USHORT wDosTime, DOUBLE * pvtime), (wDosDate, wDosTime, pvtime));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, LoadRegTypeLib,
    (REFGUID rguid, unsigned short wVerMajor, unsigned short wVerMinor, LCID lcid, ITypeLib **pptlib), 
    (rguid, wVerMajor, wVerMinor, lcid, pptlib));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, LoadTypeLib,
    (const WCHAR *szFile, ITypeLib **pptlib), (szFile, pptlib));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, RegisterTypeLib,
    (ITypeLib *ptlib, WCHAR *szFullPath, WCHAR *szHelpDir),
    (ptlib, szFullPath, szHelpDir));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, SetErrorInfo,
    (DWORD  dwReserved, IErrorInfo  *perrinfo), (dwReserved, perrinfo));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32, BSTR, SysAllocString,
    (const WCHAR *pch), (pch));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32, BSTR, SysAllocStringByteLen,
    (LPCSTR psz, unsigned int len), (psz, len));

DELAY_LOAD(g_hinstOLEAUT32, OLEAUT32, BSTR, SysAllocStringLen,
    (const WCHAR *pch, unsigned int i), (pch, i));

DELAY_LOAD_VOID(g_hinstOLEAUT32, OLEAUT32, SysFreeString,
    (BSTR bs), (bs));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32, SysStringByteLen,
    (BSTR str), (str));

DELAY_LOAD_UINT(g_hinstOLEAUT32, OLEAUT32, SysStringLen,
    (BSTR str), (str));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VariantChangeType,
    (VARIANTARG * pvargDest, VARIANTARG * pvarSrc, USHORT wFlags, VARTYPE vt), 
    (pvargDest, pvarSrc, wFlags, vt));

#undef VariantClear

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VariantClear,
    (VARIANTARG *pvarg), (pvarg));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VariantCopy,
    (VARIANTARG * pvargDest, VARIANTARG * pvargSrc), (pvargDest, pvargSrc));

DELAY_LOAD_HRESULT(g_hinstOLEAUT32, OLEAUT32, VarI4FromStr,
    (OLECHAR FAR * strIn, LCID lcid, DWORD dwFlags, LONG * plOut), (strIn, lcid, dwFlags, plOut));

// --------- CDFVIEW.DLL ---------------

HINSTANCE g_hinstCDFVIEW = NULL;

DELAY_LOAD_HRESULT(g_hinstCDFVIEW, CDFVIEW, ParseDesktopComponent,
           (HWND hwndOwner, LPWSTR wszURL, COMPONENT *pInfo),
           (hwndOwner, wszURL, pInfo));

DELAY_LOAD_HRESULT(g_hinstCDFVIEW, CDFVIEW, SubscribeToCDF,
           (HWND hwndParent, LPCWSTR pwzUrl, DWORD dwCDFTypes),
           (hwndParent, pwzUrl, dwCDFTypes));

//---------- BROWSEUI.DLL --------------

HINSTANCE g_hinstBrowseui = NULL;

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, BOOL, SHOnCWMCommandLine, 127,
                (LPARAM lParam), (lParam));

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, BOOL, SHOpenFolderWindow, 102,
               (IETHREADPARAM* pieiIn),
                (pieiIn));

DELAY_LOAD_IE_ORD_VOID(g_hinstBrowseui, BROWSEUI, SHCreateSavedWindows, 105,
                  (), ());

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, HRESULT,
                  SHCreateBandForPidl, 120,
                  (LPCITEMIDLIST pidl, IUnknown** ppunk, BOOL fAllowBrowserBand),
                  (pidl, ppunk, fAllowBrowserBand));

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, HRESULT,
                  SHPidlFromDataObject, 121,
                  (IDataObject *pdtobj, LPITEMIDLIST * ppidlTarget, LPWSTR pszDisplayName, DWORD cchDisplayName),
                  (pdtobj, ppidlTarget, pszDisplayName, cchDisplayName));

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, DWORD,
                  IDataObject_GetDeskBandState, 122,
                  (IDataObject *pdtobj),
                  (pdtobj));

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, IETHREADPARAM*,
                  SHCreateIETHREADPARAM, 123,
                  (LPCWSTR pszCmdLineIn, int nCmdShowIn, ITravelLog *ptlIn, IEFreeThreadedHandShake* piehsIn),
                  (pszCmdLineIn, nCmdShowIn, ptlIn, piehsIn));

DELAY_LOAD_IE_ORD_VOID(g_hinstBrowseui, BROWSEUI,
                  SHDestroyIETHREADPARAM, 126,
                  (IETHREADPARAM* pieiIn),
                  (pieiIn));

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, BOOL,
                  SHParseIECommandLine, 125,
                  (LPCWSTR * ppszCmdLine, IETHREADPARAM * piei),
                  (ppszCmdLine, piei));

DELAY_LOAD_IE_ORD(g_hinstBrowseui, BROWSEUI, HRESULT,
                  Channel_QuickLaunch, 133, (void),());

// --------- WINMM.DLL ---------------

HINSTANCE g_hinstWINMM = NULL;

#ifdef UNICODE
DELAY_LOAD(g_hinstWINMM, WINMM, BOOL, PlaySoundW,
        (LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound),
        (pszSound, hmod, fdwSound));
#else
DELAY_LOAD(g_hinstWINMM, WINMM, BOOL, PlaySoundA,
        (LPCSTR pszSound, HMODULE hmod, DWORD fdwSound),
        (pszSound, hmod, fdwSound));
#endif

// --------- MPR.DLL ---------------

HMODULE g_hmodMPR = NULL;

#ifdef UNICODE

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetConnectionA,
    (IN LPCSTR lpLocalName,
     OUT LPSTR  lpRemoteName,
     IN OUT LPDWORD  lpnLength),
    (lpLocalName, lpRemoteName, lpnLength));

#else

DELAY_LOAD_WNET(g_hmodMPR, MPR, WNetGetConnectionW,
    (IN LPCWSTR lpLocalName,
     OUT LPWSTR  lpRemoteName,
     IN OUT LPDWORD  lpnLength),
    (lpLocalName, lpRemoteName, lpnLength));

#endif
