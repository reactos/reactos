/*
 * PROJECT:     ReactOS msutb.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Language Bar (Tipbar)
 * COPYRIGHT:   Copyright 2023-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msutb);

//#define ENABLE_DESKBAND

typedef struct LANGBARITEMSTATE
{
    CLSID m_clsid;
    DWORD m_dwDemoteLevel;
    UINT_PTR m_nTimerID;
    UINT m_uTimeOut;
    BOOL m_bStartedIntentionally;
    BOOL m_bDisableDemoting;

    BOOL IsShown()
    {
        return m_dwDemoteLevel < 2;
    }
} LANGBARITEMSTATE, *PLANGBARITEMSTATE;

HINSTANCE g_hInst = NULL;
UINT g_wmTaskbarCreated = 0;
UINT g_uACP = CP_ACP;
DWORD g_dwOSInfo = 0;
CRITICAL_SECTION g_cs;
LONG g_DllRefCount = 0;
BOOL g_bWinLogon = FALSE;
BOOL g_fInClosePopupTipbar = FALSE;
HWND g_hwndParent = NULL;
CIC_LIBTHREAD g_libTLS = { NULL, NULL };
#ifdef ENABLE_DESKBAND
BOOL g_bEnableDeskBand = TRUE;
#else
BOOL g_bEnableDeskBand = FALSE;
#endif

BOOL g_bShowTipbar = TRUE;
BOOL g_bShowDebugMenu = FALSE;
BOOL g_bNewLook = TRUE;
BOOL g_bIntelliSense = FALSE;
BOOL g_bShowCloseMenu = FALSE;
UINT g_uTimeOutNonIntentional = 60 * 1000;
UINT g_uTimeOutIntentional = 10 * 60 * 1000;
UINT g_uTimeOutMax = 60 * 60 * 1000;
BOOL g_bShowMinimizedBalloon = TRUE;
POINT g_ptTipbar = { -1, -1 };
BOOL g_bExcludeCaptionButtons = TRUE;
BOOL g_bShowShadow = FALSE;
BOOL g_fTaskbarTheme = TRUE;
BOOL g_fVertical = FALSE;
UINT g_uTimerElapseSTUBSTART = 100;
UINT g_uTimerElapseSTUBEND = 2 * 1000;
UINT g_uTimerElapseBACKTOALPHA = 3 * 1000;
UINT g_uTimerElapseONTHREADITEMCHANGE = 200;
UINT g_uTimerElapseSETWINDOWPOS = 100;
UINT g_uTimerElapseONUPDATECALLED = 50;
UINT g_uTimerElapseSYSCOLORCHANGED = 20;
UINT g_uTimerElapseDISPLAYCHANGE = 20;
UINT g_uTimerElapseUPDATEUI = 70;
UINT g_uTimerElapseSHOWWINDOW = 50;
UINT g_uTimerElapseMOVETOTRAY = 50;
UINT g_uTimerElapseTRAYWNDONDELAYMSG = 50;
UINT g_uTimerElapseDOACCDEFAULTACTION = 200;
UINT g_uTimerElapseENSUREFOCUS = 50;
BOOL g_bShowDeskBand = FALSE;
UINT g_uTimerElapseSHOWDESKBAND = 3 * 1000;
BOOL g_fPolicyDisableCloseButton = FALSE;
BOOL g_fPolicyEnableLanguagebarInFullscreen = FALSE;
DWORD g_dwWndStyle = 0;
DWORD g_dwMenuStyle = 0;
DWORD g_dwChildWndStyle = 0;
BOOL g_fRTL = FALSE;

#define TIMER_ID_DOACCDEFAULTACTION 11

class CMsUtbModule : public CComModule
{
};

CMsUtbModule gModule;

class CCicLibMenuItem;
class CTipbarAccItem;
class CUTBMenuItem;
class CMainIconItem;
class CTrayIconItem;
class CTipbarWnd;
class CButtonIconItem;
class CTrayIconWnd;

CTipbarWnd *g_pTipbarWnd = NULL;
CTrayIconWnd *g_pTrayIconWnd = NULL;

CicArray<HKL> *g_prghklSkipRedrawing = NULL;

BOOL IsSkipRedrawHKL(HKL hSkipKL)
{
    if (LOWORD(hSkipKL) == MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT))
        return FALSE; // Japanese HKL will be skipped
    if (!g_prghklSkipRedrawing)
        return FALSE;

    for (size_t iItem = 0; iItem < g_prghklSkipRedrawing->size(); ++iItem)
    {
        if ((*g_prghklSkipRedrawing)[iItem] == hSkipKL)
            return TRUE; // To be skipped
    }

    return FALSE; // To be not skipped
}

BOOL IsBiDiLocalizedSystem(void)
{
    LOCALESIGNATURE Sig;
    LANGID LangID = ::GetUserDefaultUILanguage();
    if (!LangID)
        return FALSE;

    INT size = sizeof(Sig) / sizeof(WCHAR);
    if (!::GetLocaleInfoW(LangID, LOCALE_FONTSIGNATURE, (LPWSTR)&Sig, size))
        return FALSE;
    return (Sig.lsUsb[3] & 0x8000000) != 0;
}

BOOL GetFontSig(HWND hWnd, HKL hKL)
{
    LOCALESIGNATURE Sig;
    INT size = sizeof(Sig) / sizeof(WCHAR);
    if (!::GetLocaleInfoW(LOWORD(hKL), LOCALE_FONTSIGNATURE, (LPWSTR)&Sig, size))
        return FALSE;

    HDC hDC = ::GetDC(hWnd);
    DWORD CharSet = ::GetTextCharsetInfo(hDC, NULL, 0);
    CHARSETINFO CharSetInfo;
    ::TranslateCharsetInfo((DWORD*)(DWORD_PTR)CharSet, &CharSetInfo, TCI_SRCCHARSET);
    ::ReleaseDC(hWnd, hDC);

    return !!(CharSetInfo.fs.fsCsb[0] & Sig.lsCsbSupported[0]);
}

void InitSkipRedrawHKLArray(void)
{
    g_prghklSkipRedrawing = new(cicNoThrow) CicArray<HKL>();
    if (!g_prghklSkipRedrawing)
        return;

    if (g_bEnableDeskBand && (g_dwOSInfo & CIC_OSINFO_XPPLUS))
    {
        // Japanese IME will be skipped
        g_prghklSkipRedrawing->Add((HKL)UlongToHandle(0xE0010411));
    }

    CicRegKey regKey;
    LSTATUS error = regKey.Open(HKEY_LOCAL_MACHINE,
                                TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\SkipRedrawHKL"));
    if (error != ERROR_SUCCESS)
        return;

    TCHAR szValueName[256];
    for (DWORD dwIndex = 0; ; ++dwIndex)
    {
        error = regKey.EnumValue(dwIndex, szValueName, _countof(szValueName));
        if (error != ERROR_SUCCESS)
            break;

        if (szValueName[0] == TEXT('0') &&
            (szValueName[1] == TEXT('x') || szValueName[1] == TEXT('X')))
        {
            HKL hKL = (HKL)UlongToHandle(_tcstoul(szValueName, NULL, 16));
            g_prghklSkipRedrawing->Add(hKL); // This hKL will be skipped
        }
    }
}

void UninitSkipRedrawHKLArray(void)
{
    if (g_prghklSkipRedrawing)
    {
        delete g_prghklSkipRedrawing;
        g_prghklSkipRedrawing = NULL;
    }
}

HRESULT GetGlobalCompartment(REFGUID rguid, ITfCompartment **ppComp)
{
    ITfCompartmentMgr *pCompMgr = NULL;
    HRESULT hr = TF_GetGlobalCompartment(&pCompMgr);
    if (FAILED(hr))
        return hr;

    if (!pCompMgr)
        return E_FAIL;

    hr = pCompMgr->GetCompartment(rguid, ppComp);
    pCompMgr->Release();
    return hr;
}

HRESULT GetGlobalCompartmentDWORD(REFGUID rguid, LPDWORD pdwValue)
{
    *pdwValue = 0;
    ITfCompartment *pComp;
    HRESULT hr = GetGlobalCompartment(rguid, &pComp);
    if (SUCCEEDED(hr))
    {
        VARIANT vari;
        hr = pComp->GetValue(&vari);
        if (hr == S_OK)
            *pdwValue = V_I4(&vari);
        pComp->Release();
    }
    return hr;
}

HRESULT SetGlobalCompartmentDWORD(REFGUID rguid, DWORD dwValue)
{
    VARIANT vari;
    ITfCompartment *pComp;
    HRESULT hr = GetGlobalCompartment(rguid, &pComp);
    if (SUCCEEDED(hr))
    {
        V_VT(&vari) = VT_I4;
        V_I4(&vari) = dwValue;
        hr = pComp->SetValue(0, &vari);
        pComp->Release();
    }
    return hr;
}

void TurnOffSpeechIfItsOn(void)
{
    DWORD dwValue = 0;
    HRESULT hr = GetGlobalCompartmentDWORD(GUID_COMPARTMENT_SPEECH_OPENCLOSE, &dwValue);
    if (SUCCEEDED(hr) && dwValue)
        SetGlobalCompartmentDWORD(GUID_COMPARTMENT_SPEECH_OPENCLOSE, 0);
}

void DoCloseLangbar(void)
{
    ITfLangBarMgr *pLangBarMgr = NULL;
    HRESULT hr = TF_CreateLangBarMgr(&pLangBarMgr);
    if (FAILED(hr))
        return;

    if (pLangBarMgr)
    {
        hr = pLangBarMgr->ShowFloating(TF_SFT_HIDDEN);
        pLangBarMgr->Release();
    }

    if (SUCCEEDED(hr))
        TurnOffSpeechIfItsOn();

    CicRegKey regKey;
    LSTATUS error = regKey.Open(HKEY_CURRENT_USER,
                                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                                KEY_ALL_ACCESS);
    if (error == ERROR_SUCCESS)
        ::RegDeleteValue(regKey, TEXT("ctfmon.exe"));
}

INT GetIconIndexFromhKL(_In_ HKL hKL)
{
    HKL hGotKL;

    INT iKL, cKLs = TF_MlngInfoCount();
    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        if (TF_GetMlngHKL(iKL, &hGotKL, NULL, 0) && hKL == hGotKL)
            return TF_GetMlngIconIndex(iKL);
    }

    if (!TF_GetMlngHKL(0, &hGotKL, NULL, 0))
        return -1;

    return TF_GetMlngIconIndex(0);
}

BOOL GethKLDesc(_In_ HKL hKL, _Out_ LPWSTR pszDesc, _In_ UINT cchDesc)
{
    HKL hGotKL;

    INT iKL, cKLs = TF_MlngInfoCount();
    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        if (TF_GetMlngHKL(iKL, &hGotKL, pszDesc, cchDesc) && hKL == hGotKL)
            return TRUE;
    }

    return TF_GetMlngHKL(0, &hGotKL, pszDesc, cchDesc);
}

HRESULT
LangBarInsertMenu(
    _In_ ITfMenu *pMenu,
    _In_ UINT uId,
    _In_ LPCWSTR pszText,
    _In_ BOOL bChecked,
    _Inout_opt_ HICON hIcon)
{
    HBITMAP hbmp = NULL, hbmpMask = NULL;
    if (hIcon)
    {
        HICON hIconNew = (HICON)::CopyImage(hIcon, IMAGE_ICON, 16, 16, LR_COPYFROMRESOURCE);
        SIZE iconSize = { 16, 16 };
        if (!hIconNew)
            hIconNew = hIcon;
        if (!cicGetIconBitmaps(hIconNew, &hbmp, &hbmpMask, &iconSize))
            return E_FAIL;
        if (hIconNew)
            ::DestroyIcon(hIconNew);
        ::DestroyIcon(hIcon);
    }

    INT cchText = lstrlenW(pszText);
    DWORD dwFlags = (bChecked ? TF_LBMENUF_CHECKED : 0);
    return pMenu->AddMenuItem(uId, dwFlags, hbmp, hbmpMask, pszText, cchText, NULL);
}

HRESULT LangBarInsertSeparator(_In_ ITfMenu *pMenu)
{
    return pMenu->AddMenuItem(-1, TF_LBMENUF_SEPARATOR, NULL, NULL, NULL, 0, NULL);
}

// Is it a Far-East language ID?
BOOL IsFELangId(LANGID LangID)
{
    switch (LangID)
    {
        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED): // Chinese (Simplified)
        case MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL): // Chinese (Traditional)
        case MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT): // Japanese
        case MAKELANGID(LANG_KOREAN, SUBLANG_DEFAULT): // Korean
            return TRUE;
        default:
            return FALSE;
    }
}

BOOL CheckCloseMenuAvailable(void)
{
    BOOL ret = FALSE;
    ITfInputProcessorProfiles *pProfiles = NULL;
    LANGID *pLangIds = NULL;
    ULONG iItem, cItems;

    if (g_fPolicyDisableCloseButton)
        return FALSE;

    if (g_bShowCloseMenu)
        return TRUE;

    if (SUCCEEDED(TF_CreateInputProcessorProfiles(&pProfiles)) &&
        SUCCEEDED(pProfiles->GetLanguageList(&pLangIds, &cItems)))
    {
        for (iItem = 0; iItem < cItems; ++iItem)
        {
            if (IsFELangId(pLangIds[iItem]))
                break;
        }

        ret = (iItem == cItems);
    }

    if (pLangIds)
        CoTaskMemFree(pLangIds);
    if (pProfiles)
        pProfiles->Release();

    return ret;
}

/// @unimplemented
BOOL IsTransparecyAvailable(void)
{
    return FALSE;
}

static INT CALLBACK
FindEAEnumFontProc(ENUMLOGFONT *pLF, NEWTEXTMETRIC *pTM, INT nFontType, LPARAM lParam)
{
    if ((nFontType != TRUETYPE_FONTTYPE) || (pLF->elfLogFont.lfFaceName[0] != '@'))
        return TRUE;
    *(BOOL*)lParam = TRUE;
    return FALSE;
}

/// Are there East-Asian vertical fonts?
BOOL CheckEAFonts(void)
{
    BOOL bHasVertical = FALSE;
    HDC hDC = ::GetDC(NULL);
    ::EnumFonts(hDC, NULL, (FONTENUMPROC)FindEAEnumFontProc, (LPARAM)&bHasVertical);
    ::ReleaseDC(NULL, hDC);
    return bHasVertical;
}

BOOL IsDeskBandFromReg()
{
    if (!g_bEnableDeskBand || !(g_dwOSInfo & CIC_OSINFO_XPPLUS)) // Desk band is for XP+
        return FALSE;

    CicRegKey regKey;
    if (regKey.Open(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\")))
    {
        DWORD dwValue = 0;
        regKey.QueryDword(TEXT("ShowDeskBand"), &dwValue);
        return !!dwValue;
    }

    return FALSE;
}

void SetDeskBandToReg(BOOL bShow)
{
    CicRegKey regKey;
    if (regKey.Open(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\"), KEY_ALL_ACCESS))
        regKey.SetDword(TEXT("ShowDeskBand"), bShow);
}

BOOL RegisterComCat(REFCLSID rclsid, REFCATID rcatid, BOOL bRegister)
{
    if (FAILED(::CoInitialize(NULL)))
        return FALSE;

    ICatRegister *pCat;
    HRESULT hr = ::CoCreateInstance(CLSID_StdComponentCategoriesMgr, NULL, CLSCTX_INPROC_SERVER,
                                    IID_ICatRegister, (void**)&pCat);
    if (SUCCEEDED(hr))
    {
        if (bRegister)
            hr = pCat->RegisterClassImplCategories(rclsid, 1, const_cast<CATID*>(&rcatid));
        else
            hr = pCat->UnRegisterClassImplCategories(rclsid, 1, const_cast<CATID*>(&rcatid));

        pCat->Release();
    }

    ::CoUninitialize();

    //if (IsIE5())
    //    ::RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Component Categories\\{00021492-0000-0000-C000-000000000046}\\Enum"));

    return SUCCEEDED(hr);
}

BOOL InitFromReg(void)
{
    DWORD dwValue;
    LSTATUS error;

    CicRegKey regKey1;
    error = regKey1.Open(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\"));
    if (error == ERROR_SUCCESS)
    {
        error = regKey1.QueryDword(TEXT("ShowTipbar"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bShowTipbar = !!dwValue;
    }

    CicRegKey regKey2;
    error = regKey2.Open(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\"));
    if (error == ERROR_SUCCESS)
    {
        error = regKey2.QueryDword(TEXT("ShowDebugMenu"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bShowDebugMenu = !!dwValue;
        error = regKey2.QueryDword(TEXT("NewLook"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bNewLook = !!dwValue;
        error = regKey2.QueryDword(TEXT("IntelliSense"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bIntelliSense = !!dwValue;
        error = regKey2.QueryDword(TEXT("ShowCloseMenu"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bShowCloseMenu = !!dwValue;
        error = regKey2.QueryDword(TEXT("TimeOutNonIntentional"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimeOutNonIntentional = 1000 * dwValue;
        error = regKey2.QueryDword(TEXT("TimeOutIntentional"), &dwValue);
        if (error == ERROR_SUCCESS)
        {
            g_uTimeOutIntentional = 1000 * dwValue;
            g_uTimeOutMax = 6000 * dwValue;
        }
        error = regKey2.QueryDword(TEXT("ShowMinimizedBalloon"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bShowMinimizedBalloon = !!dwValue;
        error = regKey2.QueryDword(TEXT("Left"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_ptTipbar.x = dwValue;
        error = regKey2.QueryDword(TEXT("Top"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_ptTipbar.y = dwValue;
        error = regKey2.QueryDword(TEXT("ExcludeCaptionButtons"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bExcludeCaptionButtons = !!dwValue;
        error = regKey2.QueryDword(TEXT("ShowShadow"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bShowShadow = !!dwValue;
        error = regKey2.QueryDword(TEXT("TaskbarTheme"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_fTaskbarTheme = !!dwValue;
        error = regKey2.QueryDword(TEXT("Vertical"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_fVertical = !!dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseSTUBSTART"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseSTUBSTART = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseSTUBEND"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseSTUBEND = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseBACKTOALPHA"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseBACKTOALPHA = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseONTHREADITEMCHANGE"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseONTHREADITEMCHANGE = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseSETWINDOWPOS"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseSETWINDOWPOS = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseONUPDATECALLED"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseONUPDATECALLED = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseSYSCOLORCHANGED"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseSYSCOLORCHANGED = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseDISPLAYCHANGE"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseDISPLAYCHANGE = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseUPDATEUI"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseUPDATEUI = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseSHOWWINDOW"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseSHOWWINDOW = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseMOVETOTRAY"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseMOVETOTRAY = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseTRAYWNDONDELAYMSG"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseTRAYWNDONDELAYMSG = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseDOACCDEFAULTACTION"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseDOACCDEFAULTACTION = dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseENSUREFOCUS"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseENSUREFOCUS = dwValue;
        if (g_bEnableDeskBand && (g_dwOSInfo & CIC_OSINFO_XPPLUS))
        {
            error = regKey2.QueryDword(TEXT("ShowDeskBand"), &dwValue);
            if (error == ERROR_SUCCESS)
                g_bShowDeskBand = !!dwValue;
            error = regKey2.QueryDword(TEXT("TimerElapseSHOWWDESKBAND"), &dwValue);
            if (error == ERROR_SUCCESS)
                g_uTimerElapseSHOWDESKBAND = dwValue;
        }
    }

    CicRegKey regKey3;
    error = regKey3.Open(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Policies\\Microsoft\\MSCTF"));
    if (error == ERROR_SUCCESS)
    {
        error = regKey3.QueryDword(TEXT("DisableCloseButton"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_fPolicyDisableCloseButton = !!dwValue;
    }

    CicRegKey regKey4;
    error = regKey4.Open(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Policies\\Microsoft\\MSCTF"));
    if (error == ERROR_SUCCESS)
    {
        error = regKey4.QueryDword(TEXT("EnableLanguagebarInFullscreen"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_fPolicyEnableLanguagebarInFullscreen = !!dwValue;
    }

    InitSkipRedrawHKLArray();

    if (g_bNewLook)
    {
        g_dwWndStyle = UIF_WINDOW_ENABLETHEMED | UIF_WINDOW_WORKAREA | UIF_WINDOW_TOOLTIP |
                       UIF_WINDOW_TOOLWINDOW | UIF_WINDOW_TOPMOST;
        if (g_bShowShadow)
            g_dwWndStyle |= UIF_WINDOW_SHADOW;
        g_dwMenuStyle = 0x10000000 | UIF_WINDOW_MONITOR | UIF_WINDOW_SHADOW |
                        UIF_WINDOW_TOOLWINDOW | UIF_WINDOW_TOPMOST;
    }
    else
    {
        g_dwWndStyle = UIF_WINDOW_WORKAREA | UIF_WINDOW_TOOLTIP | UIF_WINDOW_DLGFRAME |
                       UIF_WINDOW_TOPMOST;
        g_dwMenuStyle = UIF_WINDOW_MONITOR | UIF_WINDOW_DLGFRAME | UIF_WINDOW_TOPMOST;
    }

    g_dwChildWndStyle =
        UIF_WINDOW_ENABLETHEMED | UIF_WINDOW_NOMOUSEMSG | UIF_WINDOW_TOOLTIP | UIF_WINDOW_CHILD;

    if (IsBiDiLocalizedSystem())
    {
        g_dwWndStyle |= UIF_WINDOW_LAYOUTRTL;
        g_dwChildWndStyle |= UIF_WINDOW_LAYOUTRTL;
        g_dwMenuStyle |= UIF_WINDOW_LAYOUTRTL;
        g_fRTL = TRUE;
    }

    return TRUE;
}

/***********************************************************************/

struct CShellWndThread
{
    HWND m_hTrayWnd = NULL;
    HWND m_hProgmanWnd = NULL;

    HWND GetWndTray()
    {
        if (!m_hTrayWnd || !::IsWindow(m_hTrayWnd))
            m_hTrayWnd = ::FindWindowW(L"Shell_TrayWnd", NULL);
        return m_hTrayWnd;
    }

    HWND GetWndProgman()
    {
        if (!m_hProgmanWnd || !::IsWindow(m_hProgmanWnd))
            m_hProgmanWnd = ::FindWindowW(L"Progman", NULL);
        return m_hProgmanWnd;
    }

    void clear()
    {
        m_hTrayWnd = m_hProgmanWnd = NULL;
    }
};

/***********************************************************************/

class CUTBLangBarDlg
{
protected:
    LPTSTR m_pszDialogName;
    LONG m_cRefs;

public:
    CUTBLangBarDlg() { }
    virtual ~CUTBLangBarDlg() { }

    static CUTBLangBarDlg *GetThis(HWND hDlg);
    static void SetThis(HWND hDlg, CUTBLangBarDlg *pThis);
    static DWORD WINAPI s_ThreadProc(LPVOID pParam);
    static INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL StartThread();
    LONG _Release();

    STDMETHOD_(BOOL, DoModal)(HWND hDlg) = 0;
    STDMETHOD_(BOOL, OnCommand)(HWND hDlg, WPARAM wParam, LPARAM lParam) = 0;
    STDMETHOD_(BOOL, IsDlgShown)() = 0;
    STDMETHOD_(void, SetDlgShown)(BOOL bShown) = 0;
    STDMETHOD_(BOOL, ThreadProc)();
};

/***********************************************************************/

class CUTBCloseLangBarDlg : public CUTBLangBarDlg
{
public:
    CUTBCloseLangBarDlg();

    static BOOL s_bIsDlgShown;

    STDMETHOD_(BOOL, DoModal)(HWND hDlg) override;
    STDMETHOD_(BOOL, OnCommand)(HWND hDlg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(BOOL, IsDlgShown)() override;
    STDMETHOD_(void, SetDlgShown)(BOOL bShown) override;
};

BOOL CUTBCloseLangBarDlg::s_bIsDlgShown = FALSE;

/***********************************************************************/

class CUTBMinimizeLangBarDlg : public CUTBLangBarDlg
{
public:
    CUTBMinimizeLangBarDlg();

    static BOOL s_bIsDlgShown;

    STDMETHOD_(BOOL, DoModal)(HWND hDlg) override;
    STDMETHOD_(BOOL, OnCommand)(HWND hDlg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(BOOL, IsDlgShown)() override;
    STDMETHOD_(void, SetDlgShown)(BOOL bShown) override;
    STDMETHOD_(BOOL, ThreadProc)() override;
};

BOOL CUTBMinimizeLangBarDlg::s_bIsDlgShown = FALSE;

/***********************************************************************/

class CCicLibMenu : public ITfMenu
{
protected:
    CicArray<CCicLibMenuItem*> m_MenuItems;
    LONG m_cRefs;

public:
    CCicLibMenu();
    virtual ~CCicLibMenu();

    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;
    STDMETHOD(AddMenuItem)(
        UINT uId,
        DWORD dwFlags,
        HBITMAP hbmp,
        HBITMAP hbmpMask,
        const WCHAR *pch,
        ULONG cch,
        ITfMenu **ppSubMenu) override;
    STDMETHOD_(CCicLibMenu*, CreateSubMenu)();
    STDMETHOD_(CCicLibMenuItem*, CreateMenuItem)();
};

/***********************************************************************/

class CCicLibMenuItem
{
protected:
    DWORD m_uId;
    DWORD m_dwFlags;
    HBITMAP m_hbmp;
    HBITMAP m_hbmpMask;
    BSTR m_bstrText;
    ITfMenu *m_pMenu;

public:
    CCicLibMenuItem();
    virtual ~CCicLibMenuItem();

    BOOL Init(
        UINT uId,
        DWORD dwFlags,
        HBITMAP hbmp,
        HBITMAP hbmpMask,
        const WCHAR *pch,
        ULONG cch,
        ITfMenu *pMenu);
    HBITMAP CreateBitmap(HANDLE hBitmap);
};

/***********************************************************************/

class CTipbarAccessible : public IAccessible
{
protected:
    LONG m_cRefs;
    HWND m_hWnd;
    IAccessible *m_pStdAccessible;
    ITypeInfo *m_pTypeInfo;
    BOOL m_bInitialized;
    CicArray<CTipbarAccItem*> m_AccItems;
    LONG m_cSelection;
    friend class CUTBMenuWnd;
    friend class CTipbarWnd;

public:
    CTipbarAccessible(CTipbarAccItem *pItem);
    virtual ~CTipbarAccessible();

    HRESULT Initialize();

    BOOL AddAccItem(CTipbarAccItem *pItem);
    HRESULT RemoveAccItem(CTipbarAccItem *pItem);
    void ClearAccItems();
    CTipbarAccItem *AccItemFromID(INT iItem);
    INT GetIDOfItem(CTipbarAccItem *pTarget);

    LONG_PTR CreateRefToAccObj(WPARAM wParam);
    BOOL DoDefaultActionReal(INT nID);
    void NotifyWinEvent(DWORD event, CTipbarAccItem *pItem);
    void SetWindow(HWND hWnd);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IDispatch methods
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(
        UINT iTInfo,
        LCID lcid,
        ITypeInfo **ppTInfo);
    STDMETHOD(GetIDsOfNames)(
        REFIID riid,
        LPOLESTR *rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID *rgDispId);
    STDMETHOD(Invoke)(
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS *pDispParams,
        VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr);

    // IAccessible methods
    STDMETHOD(get_accParent)(IDispatch **ppdispParent);
    STDMETHOD(get_accChildCount)(LONG *pcountChildren);
    STDMETHOD(get_accChild)(VARIANT varChildID, IDispatch **ppdispChild);
    STDMETHOD(get_accName)(VARIANT varID, BSTR *pszName);
    STDMETHOD(get_accValue)(VARIANT varID, BSTR *pszValue);
    STDMETHOD(get_accDescription)(VARIANT varID, BSTR *description);
    STDMETHOD(get_accRole)(VARIANT varID, VARIANT *role);
    STDMETHOD(get_accState)(VARIANT varID, VARIANT *state);
    STDMETHOD(get_accHelp)(VARIANT varID, BSTR *help);
    STDMETHOD(get_accHelpTopic)(BSTR *helpfile, VARIANT varID, LONG *pidTopic);
    STDMETHOD(get_accKeyboardShortcut)(VARIANT varID, BSTR *shortcut);
    STDMETHOD(get_accFocus)(VARIANT *pvarID);
    STDMETHOD(get_accSelection)(VARIANT *pvarID);
    STDMETHOD(get_accDefaultAction)(VARIANT varID, BSTR *action);
    STDMETHOD(accSelect)(LONG flagsSelect, VARIANT varID);
    STDMETHOD(accLocation)(
        LONG *left,
        LONG *top,
        LONG *width,
        LONG *height,
        VARIANT varID);
    STDMETHOD(accNavigate)(LONG dir, VARIANT varStart, VARIANT *pvarEnd);
    STDMETHOD(accHitTest)(LONG left, LONG top, VARIANT *pvarID);
    STDMETHOD(accDoDefaultAction)(VARIANT varID);
    STDMETHOD(put_accName)(VARIANT varID, BSTR name);
    STDMETHOD(put_accValue)(VARIANT varID, BSTR value);
};

/***********************************************************************/

class CTipbarAccItem
{
public:
    CTipbarAccItem() { }
    virtual ~CTipbarAccItem() { }

    STDMETHOD_(BSTR, GetAccName)()
    {
        return SysAllocString(L"");
    }
    STDMETHOD_(BSTR, GetAccValue)()
    {
        return NULL;
    }
    STDMETHOD_(INT, GetAccRole)()
    {
        return 10;
    }
    STDMETHOD_(INT, GetAccState)()
    {
        return 256;
    }
    STDMETHOD_(void, GetAccLocation)(LPRECT lprc)
    {
        *lprc = { 0, 0, 0, 0 };
    }
    STDMETHOD_(BSTR, GetAccDefaultAction)()
    {
        return NULL;
    }
    STDMETHOD_(BOOL, DoAccDefaultAction)()
    {
        return FALSE;
    }
    STDMETHOD_(BOOL, DoAccDefaultActionReal)()
    {
        return FALSE;
    }
};

/***********************************************************************/

class CTipbarCoInitialize
{
public:
    BOOL m_bCoInit;

    CTipbarCoInitialize() : m_bCoInit(FALSE) { }
    ~CTipbarCoInitialize() { CoUninit(); }

    HRESULT EnsureCoInit()
    {
        if (m_bCoInit)
            return S_OK;
        HRESULT hr = ::CoInitialize(NULL);
        if (FAILED(hr))
            return hr;
        m_bCoInit = TRUE;
        return S_OK;
    }

    void CoUninit()
    {
        if (m_bCoInit)
        {
            ::CoUninitialize();
            m_bCoInit = FALSE;
        }
    }
};

/***********************************************************************/

class CUTBMenuWnd : public CTipbarAccItem, public CUIFMenu
{
protected:
    CTipbarCoInitialize m_coInit;
    CTipbarAccessible *m_pAccessible;
    UINT m_nMenuWndID;
    friend class CUTBMenuItem;

public:
    CUTBMenuWnd(HINSTANCE hInst, DWORD style, DWORD dwUnknown14);

    BOOL StartDoAccDefaultActionTimer(CUTBMenuItem *pTarget);

    CTipbarAccItem* GetAccItem()
    {
        return static_cast<CTipbarAccItem*>(this);
    }
    CUIFMenu* GetMenu()
    {
        return static_cast<CUIFMenu*>(this);
    }

    STDMETHOD_(BSTR, GetAccName)() override;
    STDMETHOD_(INT, GetAccRole)() override;
    STDMETHOD_(BOOL, Initialize)() override;
    STDMETHOD_(void, OnCreate)(HWND hWnd) override;
    STDMETHOD_(void, OnDestroy)(HWND hWnd) override;
    STDMETHOD_(HRESULT, OnGetObject)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnShowWindow)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(void, OnTimer)(WPARAM wParam) override;
};

/***********************************************************************/

class CUTBMenuItem : public CTipbarAccItem, public CUIFMenuItem
{
protected:
    CUTBMenuWnd *m_pMenuUI;
    friend class CUTBMenuWnd;

public:
    CUTBMenuItem(CUTBMenuWnd *pMenuUI);
    ~CUTBMenuItem() override;

    CUIFMenuItem* GetMenuItem()
    {
        return static_cast<CUIFMenuItem*>(this);
    }

    STDMETHOD_(BOOL, DoAccDefaultAction)() override;
    STDMETHOD_(BOOL, DoAccDefaultActionReal)() override;
    STDMETHOD_(BSTR, GetAccDefaultAction)() override;
    STDMETHOD_(void, GetAccLocation)(LPRECT lprc) override;
    STDMETHOD_(BSTR, GetAccName)() override;
    STDMETHOD_(INT, GetAccRole)() override;
};

/***********************************************************************/

class CModalMenu
{
public:
    DWORD m_dwUnknown26;
    CUTBMenuWnd *m_pMenuUI;

public:
    CModalMenu() { }
    virtual ~CModalMenu() { }

    CUTBMenuItem *InsertItem(CUTBMenuWnd *pMenuUI, INT nCommandId, INT nStringID);
    void PostKey(BOOL bUp, WPARAM wParam, LPARAM lParam);
    void CancelMenu();
};

/***********************************************************************/

class CTipbarThread;

class CUTBContextMenu : public CModalMenu
{
public:
    CTipbarWnd *m_pTipbarWnd;
    CTipbarThread *m_pTipbarThread;

public:
    CUTBContextMenu(CTipbarWnd *pTipbarWnd);

    BOOL Init();
    CUTBMenuWnd *CreateMenuUI(BOOL bFlag);

    UINT ShowPopup(
        CUIFWindow *pWindow,
        POINT pt,
        LPCRECT prc,
        BOOL bFlag);

    BOOL SelectMenuItem(UINT nCommandId);
};

/***********************************************************************/

class CUTBLBarMenuItem;

class CUTBLBarMenu : public CCicLibMenu
{
protected:
    CUTBMenuWnd *m_pMenuUI;
    HINSTANCE m_hInst;

public:
    CUTBLBarMenu(HINSTANCE hInst);
    ~CUTBLBarMenu() override;

    CUTBMenuWnd *CreateMenuUI();
    INT ShowPopup(CUIFWindow *pWindow, POINT pt, LPCRECT prcExclude);

    STDMETHOD_(CCicLibMenuItem*, CreateMenuItem)() override;
    STDMETHOD_(CCicLibMenu*, CreateSubMenu)() override;
};

/***********************************************************************/

class CUTBLBarMenuItem : public CCicLibMenuItem
{
public:
    CUTBLBarMenu *m_pLBarMenu;

public:
    CUTBLBarMenuItem() { m_pLBarMenu = NULL; }
    BOOL InsertToUI(CUTBMenuWnd *pMenuUI);
};

/***********************************************************************/

class CTipbarGripper : public CUIFGripper
{
protected:
    CTipbarWnd *m_pTipbarWnd;
    BOOL m_bInDebugMenu;
    friend class CTipbarWnd;

public:
    CTipbarGripper(CTipbarWnd *pTipbarWnd, LPCRECT prc, DWORD style);

    STDMETHOD_(void, OnLButtonUp)(LONG x, LONG y) override;
    STDMETHOD_(void, OnRButtonUp)(LONG x, LONG y) override;
    STDMETHOD_(BOOL, OnSetCursor)(UINT uMsg, LONG x, LONG y) override;
};

/***********************************************************************/

class CLangBarItemList : public CicArray<LANGBARITEMSTATE>
{
public:
    BOOL IsStartedIntentionally(REFCLSID rclsid);

    LANGBARITEMSTATE *AddItem(REFCLSID rclsid);
    void Clear();
    BOOL SetDemoteLevel(REFCLSID rclsid, DWORD dwDemoteLevel);

    LANGBARITEMSTATE *FindItem(REFCLSID rclsid);
    LANGBARITEMSTATE *GetItemStateFromTimerId(UINT_PTR nTimerID);

    void Load();
    void SaveItem(CicRegKey *pRegKey, const LANGBARITEMSTATE *pState);

    void StartDemotingTimer(REFCLSID rclsid, BOOL bIntentional);
    UINT_PTR FindDemotingTimerId();
};

/***********************************************************************/

class CTrayIconWnd
{
protected:
    DWORD m_dwUnknown20;
    BOOL m_bBusy;
    UINT m_uCallbackMessage;
    UINT m_uMsg;
    HWND m_hWnd;
    DWORD m_dwUnknown21[2];
    HWND m_hTrayWnd;
    HWND m_hNotifyWnd;
    DWORD m_dwTrayWndThreadId;
    DWORD m_dwUnknown22;
    HWND m_hwndProgman;
    DWORD m_dwProgmanThreadId;
    CMainIconItem *m_pMainIconItem;
    CicArray<CButtonIconItem*> m_Items;
    UINT m_uCallbackMsg;
    UINT m_uNotifyIconID;
    friend class CTipbarWnd;

    static BOOL CALLBACK EnumChildWndProc(HWND hWnd, LPARAM lParam);
    static LRESULT CALLBACK _WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    CTrayIconWnd();
    ~CTrayIconWnd();

    static BOOL RegisterClass();
    static CTrayIconWnd *GetThis(HWND hWnd);
    static void SetThis(HWND hWnd, LPCREATESTRUCT pCS);

    HWND CreateWnd();
    void DestroyWnd();

    BOOL SetMainIcon(HKL hKL);
    BOOL SetIcon(REFGUID rguid, DWORD dwUnknown24, HICON hIcon, LPCWSTR psz);

    void RemoveAllIcon(DWORD dwFlags);
    void RemoveUnusedIcons(int unknown);

    CButtonIconItem *FindIconItem(REFGUID rguid);
    BOOL FindTrayEtc();
    HWND GetNotifyWnd();
    BOOL OnIconMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void CallOnDelayMsg();
};

/***********************************************************************/

class CTrayIconItem
{
protected:
    HWND m_hWnd;
    UINT m_uCallbackMessage;
    UINT m_uNotifyIconID;
    DWORD m_dwIconAddOrModify;
    BOOL m_bIconAdded;
    CTrayIconWnd *m_pTrayIconWnd;
    DWORD m_dwUnknown25;
    GUID m_guid;
    RECT m_rcMenu;
    POINT m_ptCursor;
    friend class CTrayIconWnd;

public:
    CTrayIconItem(CTrayIconWnd *pTrayIconWnd);
    virtual ~CTrayIconItem() { }

    BOOL _Init(HWND hWnd, UINT uCallbackMessage, UINT uNotifyIconID, const GUID& rguid);
    BOOL UpdateMenuRectPoint();
    BOOL RemoveIcon();

    STDMETHOD_(BOOL, SetIcon)(HICON hIcon, LPCWSTR pszTip);
    STDMETHOD_(BOOL, OnMsg)(WPARAM wParam, LPARAM lParam) { return FALSE; };
    STDMETHOD_(BOOL, OnDelayMsg)(UINT uMsg) { return 0; };
};

/***********************************************************************/

class CButtonIconItem : public CTrayIconItem
{
protected:
    DWORD m_dwUnknown24;
    HKL m_hKL;
    friend class CTrayIconWnd;

public:
    CButtonIconItem(CTrayIconWnd *pWnd, DWORD dwUnknown24);

    STDMETHOD_(BOOL, OnMsg)(WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(BOOL, OnDelayMsg)(UINT uMsg) override;
};

/***********************************************************************/

class CMainIconItem : public CButtonIconItem
{
public:
    CMainIconItem(CTrayIconWnd *pWnd);

    BOOL Init(HWND hWnd);
    STDMETHOD_(BOOL, OnDelayMsg)(UINT uMsg) override;
};

/***********************************************************************/

class CLBarItemBase
{
protected:
    DWORD m_dwItemStatus;
    TF_LANGBARITEMINFO m_NewUIInfo;
    WCHAR m_szToolTipText[256];
    LONG m_cRefs;
    ITfLangBarItemSink *m_pLangBarItemSink;

public:
    CLBarItemBase();
    virtual ~CLBarItemBase();

    HRESULT ShowInternal(BOOL bShow, BOOL bUpdate);

    void InitNuiInfo(
        REFIID clsidService,
        REFGUID guidItem,
        DWORD dwStyle,
        DWORD ulSort,
        LPCWSTR Source);

    HRESULT GetInfo(TF_LANGBARITEMINFO *pInfo);
    HRESULT GetStatus(DWORD *pdwStatus);
    HRESULT Show(BOOL fShow);
    HRESULT GetTooltipString(BSTR *pbstrToolTip);

    HRESULT AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    HRESULT UnadviseSink(DWORD dwCookie);
};

/***********************************************************************/

class CLBarItemButtonBase
    : public CLBarItemBase
    , public ITfLangBarItem
    , public ITfLangBarItemButton
    , public ITfSource
{
public:
    HICON m_hIcon;

public:
    CLBarItemButtonBase() { m_hIcon = NULL; }
    ~CLBarItemButtonBase() override;

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) override;
    STDMETHOD_(ULONG, AddRef)() override;
    STDMETHOD_(ULONG, Release)() override;

    // ITfLangBarItem methods
    STDMETHOD(GetInfo)(TF_LANGBARITEMINFO *pInfo) override;
    STDMETHOD(GetStatus)(DWORD *pdwStatus) override;
    STDMETHOD(Show)(BOOL fShow) override;
    STDMETHOD(GetTooltipString)(BSTR *pbstrToolTip) override;

    // ITfLangBarItemButton methods
    STDMETHOD(OnClick)(TfLBIClick click, POINT pt, LPCRECT prc) override;
    STDMETHOD(InitMenu)(ITfMenu *pMenu) override;
    STDMETHOD(OnMenuSelect)(UINT wID) override;
    STDMETHOD(GetIcon)(HICON *phIcon) override;
    STDMETHOD(GetText)(BSTR *pbstr) override;

    // ITfSource methods
    STDMETHOD(AdviseSink)(REFIID riid, IUnknown *punk, DWORD *pdwCookie) override;
    STDMETHOD(UnadviseSink)(DWORD dwCookie) override;
};

/***********************************************************************/

/// Language Bar international item
class CLBarInatItem : public CLBarItemButtonBase
{
protected:
    HKL m_hKL;
    DWORD m_dwThreadId;

public:
    CLBarInatItem(DWORD dwThreadId);

    STDMETHOD(InitMenu)(ITfMenu *pMenu) override;
    STDMETHOD(OnMenuSelect)(INT nCommandId);
    STDMETHOD(GetIcon)(HICON *phIcon) override;
    STDMETHOD(GetText)(BSTR *pbstr) override;
};

/***********************************************************************/

class CTipbarItem;
class CTipbarBalloonItem;

class CTipbarThread
{
protected:
    CTipbarWnd *m_pTipbarWnd;
    ITfLangBarItemMgr *m_pLangBarItemMgr;
    CicArray<CTipbarItem*> m_UIObjects;
    CicArray<CUIFObject*> m_Separators;
    DWORD m_dwUnknown32;
    DWORD m_dwThreadId;
    DWORD m_dwFlags1;
    DWORD m_dwFlags2;
    INT m_cxGrip;
    INT m_cyGrip;
    DWORD m_dwFlags3;
    DWORD m_dwUnknown34;
    LONG m_cRefs;
    friend class CTipbarWnd;
    friend class CTipbarItem;

public:
    CTipbarThread(CTipbarWnd *pTipbarWnd);
    virtual ~CTipbarThread();

    HRESULT Init(DWORD dwThreadId);

    HRESULT InitItemList();
    HRESULT _UninitItemList(BOOL bUnAdvise);

    DWORD IsDirtyItem();
    BOOL IsFocusThread();
    BOOL IsVertical();

    void AddAllSeparators();
    void RemoveAllSeparators();

    void AddUIObjs();
    void RemoveUIObjs();

    CTipbarItem *GetItem(REFCLSID rclsid);
    void GetTextSize(BSTR bstr, LPSIZE pSize);
    void LocateItems();
    void MyMoveWnd(LONG xDelta, LONG yDelta);

    HRESULT _UnadviseItemsSink();
    LONG _AddRef() { return ++m_cRefs; }
    LONG _Release();

    /// @unimplemented
    BOOL SetFocus(CTipbarBalloonItem *pTarget)
    {
        return FALSE;
    }

    /// @unimplemented
    HRESULT CallOnUpdateHandler()
    {
        return E_NOTIMPL;
    }

    //FIXME
};

/***********************************************************************/

class CTipbarItem : public CTipbarAccItem
{
protected:
    DWORD m_dwCookie;
    TF_LANGBARITEMINFO m_ItemInfo;
    DWORD m_dwUnknown16;
    DWORD m_dwUnknown17;
    CTipbarThread *m_pTipbarThread;
    ITfLangBarItem *m_pLangBarItem;
    DWORD m_dwUnknown18[2];
    DWORD m_dwItemFlags;
    DWORD m_dwDirty;
    DWORD m_dwUnknown19[4];
    friend class CTipbarThread;
    friend class CTipbarWnd;

public:
    CTipbarItem(
        CTipbarThread *pThread,
        ITfLangBarItem *pLangBarItem,
        TF_LANGBARITEMINFO *pItemInfo,
        DWORD dwUnknown16);
    ~CTipbarItem() override;

    void _AddedToUI();
    void _RemovedToUI();
    void AddRemoveMeToUI(BOOL bFlag);

    BOOL IsConnected();
    void ClearConnections();

    void StartDemotingTimer(BOOL bStarted);

    void MyClientToScreen(LPPOINT ppt, LPRECT prc);
    void MyClientToScreen(LPRECT prc) { return MyClientToScreen(NULL, prc); }

    STDMETHOD_(BSTR, GetAccName)() override;
    STDMETHOD_(void, GetAccLocation)(LPRECT prc) override;
    STDMETHOD_(BOOL, DoAccDefaultAction)() override;
    STDMETHOD(OnUnknown40)() { return S_OK; }
    STDMETHOD(OnUnknown41)() { return S_OK; }
    STDMETHOD(OnUnknown42)() { return S_OK; }
    STDMETHOD(OnUnknown43)() { return S_OK; }
    STDMETHOD(OnUpdate)(DWORD dwDirty);
    STDMETHOD(OnUnknown44)() { return S_OK; }
    STDMETHOD_(void, OnUnknown45)(DWORD dwDirty, DWORD dwStatus) { }
    STDMETHOD_(void, OnUpdateHandler)(ULONG, ULONG);
    STDMETHOD(OnUnknown46)(CUIFWindow *pWindow) { return S_OK; }
    STDMETHOD(OnUnknown47)(CUIFWindow *pWindow) { return S_OK; }
    STDMETHOD(OnUnknown48)() { return S_OK; }
    STDMETHOD(OnUnknown49)() { return S_OK; }
    STDMETHOD(OnUnknown50)() { return S_OK; }
    STDMETHOD(OnUnknown51)() { return S_OK; }
    STDMETHOD(OnUnknown52)() { return S_OK; }
    STDMETHOD(OnUnknown53)(BSTR bstr) { return S_OK; }
    STDMETHOD_(LPCWSTR, OnUnknown55)() { return NULL; }
    STDMETHOD(OnUnknown56)() { return S_OK; }
    STDMETHOD_(LPCWSTR, GetToolTip)();
    STDMETHOD(OnUnknown57)(LPRECT prc) { return S_OK; }
    STDMETHOD(OnUnknown58)() { return S_OK; }
    STDMETHOD_(void, OnUnknown59)() { }
    STDMETHOD_(void, OnUnknown60)() { }
    STDMETHOD_(void, OnUnknown61)(HWND) { }
    STDMETHOD_(void, OnUnknown62)(HWND) { }
    STDMETHOD(OnUnknown63)() { return S_OK; }
};

/***********************************************************************/

class CTipbarCtrlButtonHolder;
class CDeskBand;

// Flags for m_dwTipbarWndFlags
enum
{
    TIPBAR_ATTACHED = 0x1,
    TIPBAR_CHILD = 0x2,
    TIPBAR_VERTICAL = 0x4,
    TIPBAR_HIGHCONTRAST = 0x10,
    TIPBAR_TRAYICON = 0x20,
    TIPBAR_UPDATING = 0x400,
    TIPBAR_ENSURING = 0x2000,
    TIPBAR_NODESKBAND = 0x4000,
    TIPBAR_TOOLBARENDED = 0x10000,
    TIPBAR_TOPFIT = 0x40000,
    TIPBAR_BOTTOMFIT = 0x80000,
    TIPBAR_RIGHTFIT = 0x100000,
    TIPBAR_LEFTFIT = 0x200000,
};

class CTipbarWnd
    : public ITfLangBarEventSink
    , public ITfLangBarEventSink_P
    , public CTipbarAccItem
    , public CUIFWindow
{
    CTipbarCoInitialize m_coInit;
    DWORD m_dwSinkCookie;
    CModalMenu *m_pModalMenu;
    CTipbarThread *m_pThread;
    CLangBarItemList m_LangBarItemList;
    DWORD m_dwUnknown20;
    CUIFWndFrame *m_pWndFrame;
    CTipbarGripper *m_pTipbarGripper;
    CTipbarThread *m_pFocusThread;
    CicArray<CTipbarThread*> m_Threads;
    CicArray<CTipbarThread*> m_ThreadCreatingList;
    DWORD m_dwAlphaValue;
    DWORD m_dwTipbarWndFlags;
    LONG m_ButtonWidth;
    DWORD m_dwShowType;
    DWORD m_dwUnknown21;
    INT m_cxSmallIcon;
    INT m_cySmallIcon;
    INT m_cxDlgFrameX2;
    INT m_cyDlgFrameX2;
    HFONT m_hMarlettFont;
    HFONT m_hTextFont;
    ITfLangBarMgr_P *m_pLangBarMgr;
    DWORD m_dwUnknown23;
    CTipbarCtrlButtonHolder *m_pTipbarCtrlButtonHolder;
    DWORD m_dwUnknown23_1[8];
    CUIFWindow *m_pBalloon;
    DWORD m_dwChangingThreadId;
    LONG m_bInCallOn;
    LONG m_X;
    LONG m_Y;
    LONG m_CX;
    LONG m_CY;
    CTipbarAccessible *m_pTipbarAccessible;
    INT m_nID;
    MARGINS m_Margins;
    DWORD m_dwUnknown23_5[4];
    CTipbarThread *m_pUnknownThread;
    CDeskBand *m_pDeskBand;
    CShellWndThread m_ShellWndThread;
    LONG m_cRefs;
    friend class CUTBContextMenu;
    friend class CTipbarGripper;
    friend class CTipbarThread;
    friend class CTipbarItem;
    friend class CLBarInatItem;
    friend class CMainIconItem;
    friend VOID WINAPI ClosePopupTipbar(VOID);
    friend BOOL GetTipbarInternal(HWND hWnd, DWORD dwFlags, CDeskBand *pDeskBand);
    friend LONG MyWaitForInputIdle(DWORD dwThreadId, DWORD dwMilliseconds);

public:
    CTipbarWnd(DWORD style);
    ~CTipbarWnd() override;

    CUIFWindow *GetWindow()
    {
        return static_cast<CUIFWindow*>(this);
    }

    CTipbarAccItem *GetAccItem()
    {
        return static_cast<CTipbarAccItem*>(this);
    }

    void Init(BOOL bChild, CDeskBand *pDeskBand);
    void InitHighContrast();
    void InitMetrics();
    void InitThemeMargins();
    void UnInit();

    BOOL IsFullScreenWindow(HWND hWnd);
    BOOL IsHKLToSkipRedrawOnNoItem();
    BOOL IsInItemChangeOrDirty(CTipbarThread *pTarget);

    void AddThreadToThreadCreatingList(CTipbarThread *pThread);
    void RemoveThredFromThreadCreatingList(CTipbarThread *pTarget);

    void MoveToStub(BOOL bFlag);
    void RestoreFromStub();

    INT GetCtrlButtonWidth();
    INT GetGripperWidth();
    INT GetTipbarHeight();
    BOOL AutoAdjustDeskBandSize();
    INT AdjustDeskBandSize(BOOL bFlag);
    void LocateCtrlButtons();
    void AdjustPosOnDisplayChange();
    void SetVertical(BOOL bVertical);
    void UpdatePosFlags();

    void CancelMenu();
    BOOL CheckExcludeCaptionButtonMode(LPRECT prc1, LPCRECT prc2);
    void ClearLBItemList();

    HFONT CreateVerticalFont();
    void UpdateVerticalFont();

    void ShowOverScreenSizeBalloon();
    void DestroyOverScreenSizeBalloon();
    void DestroyWnd();

    HKL GetFocusKeyboardLayout();
    void KillOnTheadItemChangeTimer();

    UINT_PTR SetTimer(UINT_PTR nIDEvent, UINT uElapse);
    BOOL KillTimer(UINT_PTR uIDEvent);

    void MoveToTray();
    void MyClientToScreen(LPPOINT lpPoint, LPRECT prc);
    void SavePosition();
    void SetAlpha(BYTE bAlpha, BOOL bFlag);
    BOOL SetLangBand(BOOL bDeskBand, BOOL bFlag2);
    void SetMoveRect(INT X, INT Y, INT nWidth, INT nHeight);
    void SetShowText(BOOL bShow);
    void SetShowTrayIcon(BOOL bShow);

    void ShowContextMenu(POINT pt, LPCRECT prc, BOOL bFlag);
    void StartBackToAlphaTimer();
    BOOL StartDoAccDefaultActionTimer(CTipbarItem *pTarget);

    void StartModalInput(ITfLangBarEventSink *pSink, DWORD dwThreadId);
    void StopModalInput(DWORD dwThreadId);

    CTipbarThread *_CreateThread(DWORD dwThreadId);
    CTipbarThread *_FindThread(DWORD dwThreadId);
    void EnsureFocusThread();
    HRESULT SetFocusThread(CTipbarThread *pFocusThread);
    HRESULT AttachFocusThread();
    void RestoreLastFocus(DWORD *pdwThreadId, BOOL fPrev);
    void CleanUpThreadPointer(CTipbarThread *pThread, BOOL bRemove);
    void TerminateAllThreads(BOOL bFlag);
    void OnTerminateToolbar();
    HRESULT OnThreadTerminateInternal(DWORD dwThreadId);

    /// @unimplemented
    HRESULT OnThreadItemChangeInternal(DWORD dwThreadId)
    {
        return E_NOTIMPL;
    }

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ITfLangBarEventSink methods
    STDMETHOD(OnSetFocus)(DWORD dwThreadId) override;
    STDMETHOD(OnThreadTerminate)(DWORD dwThreadId) override;
    STDMETHOD(OnThreadItemChange)(DWORD dwThreadId) override;
    STDMETHOD(OnModalInput)(DWORD dwThreadId, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD(ShowFloating)(DWORD dwFlags) override;
    STDMETHOD(GetItemFloatingRect)(DWORD dwThreadId, REFGUID rguid, RECT *prc) override;

    // ITfLangBarEventSink_P methods
    STDMETHOD(OnLangBarUpdate)(TfLBIClick click, BOOL bFlag) override;

    // CTipbarAccItem methods
    STDMETHOD_(BSTR, GetAccName)() override;
    STDMETHOD_(void, GetAccLocation)(LPRECT lprc) override;

    // CUIFWindow methods
    STDMETHOD_(void, PaintObject)(HDC hDC, LPCRECT prc) override;
    STDMETHOD_(DWORD, GetWndStyle)() override;
    STDMETHOD_(void, Move)(INT x, INT y, INT nWidth, INT nHeight) override;
    STDMETHOD_(void, OnMouseOutFromWindow)(LONG x, LONG y) override;
    STDMETHOD_(void, OnCreate)(HWND hWnd) override;
    STDMETHOD_(void, OnDestroy)(HWND hWnd) override;
    STDMETHOD_(void, OnTimer)(WPARAM wParam) override;
    STDMETHOD_(void, OnSysColorChange)() override;
    STDMETHOD_(void, OnEndSession)(HWND hWnd, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(void, OnUser)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnWindowPosChanged)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnWindowPosChanging)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnShowWindow)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnSettingChange)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(LRESULT, OnDisplayChange)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(HRESULT, OnGetObject)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(BOOL, OnEraseBkGnd)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(void, OnThemeChanged)(HWND hWnd, WPARAM wParam, LPARAM lParam) override;
    STDMETHOD_(void, UpdateUI)(LPCRECT prc) override;
    STDMETHOD_(void, HandleMouseMsg)(UINT uMsg, LONG x, LONG y) override;
};

/***********************************************************************/

#ifdef ENABLE_DESKBAND
class CDeskBand
{
public:
    // FIXME: Implement this
};
#endif

/***********************************************************************
 * CUTBLangBarDlg
 */

CUTBLangBarDlg *CUTBLangBarDlg::GetThis(HWND hDlg)
{
    return (CUTBLangBarDlg*)::GetWindowLongPtr(hDlg, DWLP_USER);
}

void CUTBLangBarDlg::SetThis(HWND hDlg, CUTBLangBarDlg *pThis)
{
    ::SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
}

DWORD WINAPI CUTBLangBarDlg::s_ThreadProc(LPVOID pParam)
{
    return ((CUTBLangBarDlg *)pParam)->ThreadProc();
}

BOOL CUTBLangBarDlg::StartThread()
{
    if (IsDlgShown())
        return FALSE;

    SetDlgShown(TRUE);

    DWORD dwThreadId;
    HANDLE hThread = ::CreateThread(NULL, 0, s_ThreadProc, this, 0, &dwThreadId);
    if (!hThread)
    {
        SetDlgShown(FALSE);
        return TRUE;
    }

    ++m_cRefs;
    ::CloseHandle(hThread);
    return TRUE;
}

LONG CUTBLangBarDlg::_Release()
{
    if (--m_cRefs == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

STDMETHODIMP_(BOOL) CUTBLangBarDlg::ThreadProc()
{
    extern HINSTANCE g_hInst;
    ::DialogBoxParam(g_hInst, m_pszDialogName, NULL, DlgProc, (LPARAM)this);
    SetDlgShown(FALSE);
    _Release();
    return TRUE;
}

INT_PTR CALLBACK
CUTBLangBarDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        SetThis(hDlg, (CUTBLangBarDlg *)lParam);
        ::ShowWindow(hDlg, SW_RESTORE);
        ::UpdateWindow(hDlg);
        return TRUE;
    }

    if (uMsg == WM_COMMAND)
    {
        CUTBLangBarDlg *pThis = CUTBLangBarDlg::GetThis(hDlg);
        pThis->OnCommand(hDlg, wParam, lParam);
        return TRUE;
    }

    return FALSE;
}

/***********************************************************************
 * CUTBCloseLangBarDlg
 */

CUTBCloseLangBarDlg::CUTBCloseLangBarDlg()
{
    m_cRefs = 1;

    if (!(g_dwOSInfo & CIC_OSINFO_XPPLUS))
        m_pszDialogName = MAKEINTRESOURCE(IDD_CLOSELANGBARNOBAND);
    else
        m_pszDialogName = MAKEINTRESOURCE(IDD_CLOSELANGBAR);
}

STDMETHODIMP_(BOOL) CUTBCloseLangBarDlg::DoModal(HWND hDlg)
{
    CicRegKey regKey;
    LSTATUS error;
    DWORD dwValue = FALSE;
    error = regKey.Open(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\"));
    if (error == ERROR_SUCCESS)
        regKey.QueryDword(TEXT("DontShowCloseLangBarDlg"), &dwValue);

    if (dwValue)
        return FALSE;

    StartThread();
    return TRUE;
}

STDMETHODIMP_(BOOL) CUTBCloseLangBarDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case IDOK:
            DoCloseLangbar();
            if (::IsDlgButtonChecked(hDlg, IDC_CLOSELANGBAR_CHECK))
            {
                CicRegKey regKey;
                LSTATUS error;
                error = regKey.Create(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\"));
                if (error == ERROR_SUCCESS)
                    regKey.SetDword(TEXT("DontShowCloseLangBarDlg"), TRUE);
            }
            ::EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            ::EndDialog(hDlg, FALSE);
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

STDMETHODIMP_(BOOL) CUTBCloseLangBarDlg::IsDlgShown()
{
    return s_bIsDlgShown;
}

STDMETHODIMP_(void) CUTBCloseLangBarDlg::SetDlgShown(BOOL bShown)
{
    s_bIsDlgShown = bShown;
}

/***********************************************************************
 * CUTBMinimizeLangBarDlg
 */

CUTBMinimizeLangBarDlg::CUTBMinimizeLangBarDlg()
{
    m_cRefs = 1;
    if (!(g_dwOSInfo & CIC_OSINFO_XPPLUS))
        m_pszDialogName = MAKEINTRESOURCE(IDD_MINIMIZELANGBARNOBAND);
    else
        m_pszDialogName = MAKEINTRESOURCE(IDD_MINIMIZELANGBAR);
}

STDMETHODIMP_(BOOL) CUTBMinimizeLangBarDlg::DoModal(HWND hDlg)
{
    CicRegKey regKey;
    LSTATUS error;

    DWORD dwValue = FALSE;
    error = regKey.Open(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\"));
    if (error == ERROR_SUCCESS)
        regKey.QueryDword(TEXT("DontShowMinimizeLangBarDlg"), &dwValue);

    if (dwValue)
        return FALSE;

    StartThread();
    return TRUE;
}

STDMETHODIMP_(BOOL) CUTBMinimizeLangBarDlg::OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
        case IDOK:
            if (::IsDlgButtonChecked(hDlg, IDC_MINIMIZELANGBAR_CHECK))
            {
                LSTATUS error;
                CicRegKey regKey;
                error = regKey.Create(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\"));
                if (error == ERROR_SUCCESS)
                    regKey.SetDword(TEXT("DontShowMinimizeLangBarDlg"), TRUE);
            }
            ::EndDialog(hDlg, TRUE);
            break;
        case IDCANCEL:
            ::EndDialog(hDlg, FALSE);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

STDMETHODIMP_(BOOL) CUTBMinimizeLangBarDlg::IsDlgShown()
{
    return s_bIsDlgShown;
}

STDMETHODIMP_(void) CUTBMinimizeLangBarDlg::SetDlgShown(BOOL bShown)
{
    s_bIsDlgShown = bShown;
}

STDMETHODIMP_(BOOL) CUTBMinimizeLangBarDlg::ThreadProc()
{
    ::Sleep(700);
    return CUTBLangBarDlg::ThreadProc();
}

/***********************************************************************
 * CCicLibMenu
 */

CCicLibMenu::CCicLibMenu() : m_cRefs(1)
{
}

CCicLibMenu::~CCicLibMenu()
{
    for (size_t iItem = 0; iItem < m_MenuItems.size(); ++iItem)
    {
        delete m_MenuItems[iItem];
        m_MenuItems[iItem] = NULL;
    }
}

STDMETHODIMP CCicLibMenu::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CCicLibMenu, ITfMenu),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CCicLibMenu::AddRef()
{
    return ++m_cRefs;
}

STDMETHODIMP_(ULONG) CCicLibMenu::Release()
{
    if (--m_cRefs == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

STDMETHODIMP_(CCicLibMenu*) CCicLibMenu::CreateSubMenu()
{
    return new(cicNoThrow) CCicLibMenu();
}

STDMETHODIMP_(CCicLibMenuItem*) CCicLibMenu::CreateMenuItem()
{
    return new(cicNoThrow) CCicLibMenuItem();
}

STDMETHODIMP CCicLibMenu::AddMenuItem(
    UINT uId,
    DWORD dwFlags,
    HBITMAP hbmp,
    HBITMAP hbmpMask,
    const WCHAR *pch,
    ULONG cch,
    ITfMenu **ppSubMenu)
{
    if (ppSubMenu)
        *ppSubMenu = NULL;

    CCicLibMenu *pSubMenu = NULL;
    if (dwFlags & TF_LBMENUF_SUBMENU)
    {
        if (!ppSubMenu)
            return E_INVALIDARG;
        pSubMenu = CreateSubMenu();
    }

    CCicLibMenuItem *pMenuItem = CreateMenuItem();
    if (!pMenuItem)
        return E_OUTOFMEMORY;

    if (!pMenuItem->Init(uId, dwFlags, hbmp, hbmpMask, pch, cch, pSubMenu))
        return E_FAIL;

    if (ppSubMenu && pSubMenu)
    {
        *ppSubMenu = pSubMenu;
        pSubMenu->AddRef();
    }

    m_MenuItems.Add(pMenuItem);
    return S_OK;
}

/***********************************************************************
 * CCicLibMenuItem
 */

CCicLibMenuItem::CCicLibMenuItem()
{
    m_uId = 0;
    m_dwFlags = 0;
    m_hbmp = NULL;
    m_hbmpMask = NULL;
    m_bstrText = NULL;
    m_pMenu = NULL;
}

CCicLibMenuItem::~CCicLibMenuItem()
{
    if (m_pMenu)
    {
        m_pMenu->Release();
        m_pMenu = NULL;
    }

    if (m_hbmp)
    {
        ::DeleteObject(m_hbmp);
        m_hbmp = NULL;
    }

    if (m_hbmpMask)
    {
        ::DeleteObject(m_hbmpMask);
        m_hbmpMask = NULL;
    }

    ::SysFreeString(m_bstrText);
    m_bstrText = NULL;
}

BOOL CCicLibMenuItem::Init(
    UINT uId,
    DWORD dwFlags,
    HBITMAP hbmp,
    HBITMAP hbmpMask,
    const WCHAR *pch,
    ULONG cch,
    ITfMenu *pMenu)
{
    m_uId = uId;
    m_dwFlags = dwFlags;
    m_bstrText = ::SysAllocStringLen(pch, cch);
    if (!m_bstrText && cch)
        return FALSE;

    m_pMenu = pMenu;
    m_hbmp = CreateBitmap(hbmp);
    m_hbmpMask = CreateBitmap(hbmpMask);
    if (hbmp)
        ::DeleteObject(hbmp);
    if (hbmpMask)
        ::DeleteObject(hbmpMask);

    return TRUE;
}

HBITMAP CCicLibMenuItem::CreateBitmap(HANDLE hBitmap)
{
    if (!hBitmap)
        return NULL;

    HDC hDC = ::CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
    if (!hDC)
        return NULL;

    HBITMAP hbmMem = NULL;

    BITMAP bm;
    ::GetObject(hBitmap, sizeof(bm), &bm);

    HGDIOBJ hbmOld1 = NULL;
    HDC hdcMem1 = ::CreateCompatibleDC(hDC);
    if (hdcMem1)
        hbmOld1 = ::SelectObject(hdcMem1, hBitmap);

    HGDIOBJ hbmOld2 = NULL;
    HDC hdcMem2 = ::CreateCompatibleDC(hDC);
    if (hdcMem2)
    {
        hbmMem = ::CreateCompatibleBitmap(hDC, bm.bmWidth, bm.bmHeight);
        hbmOld2 = ::SelectObject(hdcMem2, hbmMem);
    }

    ::BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);

    if (hbmOld1)
        ::SelectObject(hdcMem1, hbmOld1);
    if (hbmOld2)
        ::SelectObject(hdcMem2, hbmOld2);

    ::DeleteDC(hDC);
    if (hdcMem1)
        ::DeleteDC(hdcMem1);
    if (hdcMem2)
        ::DeleteDC(hdcMem2);

    return hbmMem;
}

/***********************************************************************
 * CTipbarAccessible
 */

CTipbarAccessible::CTipbarAccessible(CTipbarAccItem *pItem)
{
    m_cRefs = 1;
    m_hWnd = NULL;
    m_pTypeInfo = NULL;
    m_pStdAccessible = NULL;
    m_bInitialized = FALSE;
    m_cSelection = 1;
    m_AccItems.Add(pItem);
    ++g_DllRefCount;
}

CTipbarAccessible::~CTipbarAccessible()
{
    m_pTypeInfo = m_pTypeInfo;
    if (m_pTypeInfo)
    {
        m_pTypeInfo->Release();
        m_pTypeInfo = NULL;
    }
    if (m_pStdAccessible)
    {
        m_pStdAccessible->Release();
        m_pStdAccessible = NULL;
    }
    --g_DllRefCount;
}

HRESULT CTipbarAccessible::Initialize()
{
    m_bInitialized = TRUE;

    HRESULT hr = ::CreateStdAccessibleObject(m_hWnd, OBJID_CLIENT, IID_IAccessible,
                                             (void **)&m_pStdAccessible);
    if (FAILED(hr))
        return hr;

    ITypeLib *pTypeLib;
    hr = ::LoadRegTypeLib(LIBID_Accessibility, 1, 0, 0, &pTypeLib);
    if (FAILED(hr))
        hr = ::LoadTypeLib(L"OLEACC.DLL", &pTypeLib);

    if (SUCCEEDED(hr))
    {
        hr = pTypeLib->GetTypeInfoOfGuid(IID_IAccessible, &m_pTypeInfo);
        pTypeLib->Release();
    }

    return hr;
}

BOOL CTipbarAccessible::AddAccItem(CTipbarAccItem *pItem)
{
    return m_AccItems.Add(pItem);
}

HRESULT CTipbarAccessible::RemoveAccItem(CTipbarAccItem *pItem)
{
    for (size_t iItem = 0; iItem < m_AccItems.size(); ++iItem)
    {
        if (m_AccItems[iItem] == pItem)
        {
            m_AccItems.Remove(iItem, 1);
            break;
        }
    }
    return S_OK;
}

void CTipbarAccessible::ClearAccItems()
{
    m_AccItems.clear();
}

CTipbarAccItem *CTipbarAccessible::AccItemFromID(INT iItem)
{
    if (iItem < 0 || (INT)m_AccItems.size() <= iItem)
        return NULL;
    return m_AccItems[iItem];
}

INT CTipbarAccessible::GetIDOfItem(CTipbarAccItem *pTarget)
{
    for (size_t iItem = 0; iItem < m_AccItems.size(); ++iItem)
    {
        if (pTarget == m_AccItems[iItem])
            return (INT)iItem;
    }
    return -1;
}

LONG_PTR CTipbarAccessible::CreateRefToAccObj(WPARAM wParam)
{
    return ::LresultFromObject(IID_IAccessible, wParam, this);
}

BOOL CTipbarAccessible::DoDefaultActionReal(INT nID)
{
    CTipbarAccItem *pItem = AccItemFromID(nID);
    if (!pItem)
        return FALSE;
    return pItem->DoAccDefaultActionReal();
}

void CTipbarAccessible::NotifyWinEvent(DWORD event, CTipbarAccItem *pItem)
{
    INT nID = GetIDOfItem(pItem);
    if (nID < 0)
        return;

    ::NotifyWinEvent(event, m_hWnd, -4, nID);
}

void CTipbarAccessible::SetWindow(HWND hWnd)
{
    m_hWnd = hWnd;
}

STDMETHODIMP CTipbarAccessible::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CTipbarAccessible, IDispatch),
        QITABENT(CTipbarAccessible, IAccessible),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObject);
}

STDMETHODIMP_(ULONG) CTipbarAccessible::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CTipbarAccessible::Release()
{
    LONG count = ::InterlockedDecrement(&m_cRefs);
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

STDMETHODIMP CTipbarAccessible::GetTypeInfoCount(UINT *pctinfo)
{
    if (!pctinfo)
        return E_INVALIDARG;
    *pctinfo = (m_pTypeInfo == NULL);
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::GetTypeInfo(
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo)
{
    if (!ppTInfo)
        return E_INVALIDARG;
    *ppTInfo = NULL;
    if (iTInfo != 0)
        return TYPE_E_ELEMENTNOTFOUND;
    if (!m_pTypeInfo)
        return E_NOTIMPL;
    *ppTInfo = m_pTypeInfo;
    m_pTypeInfo->AddRef();
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::GetIDsOfNames(
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId)
{
    if (!m_pTypeInfo)
        return E_NOTIMPL;
    return m_pTypeInfo->GetIDsOfNames(rgszNames, cNames, rgDispId);
}

STDMETHODIMP CTipbarAccessible::Invoke(
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    if (!m_pTypeInfo)
        return E_NOTIMPL;
    return m_pTypeInfo->Invoke(this,
                               dispIdMember,
                               wFlags,
                               pDispParams,
                               pVarResult,
                               pExcepInfo,
                               puArgErr);
}

STDMETHODIMP CTipbarAccessible::get_accParent(IDispatch **ppdispParent)
{
    return m_pStdAccessible->get_accParent(ppdispParent);
}

STDMETHODIMP CTipbarAccessible::get_accChildCount(LONG *pcountChildren)
{
    if (!pcountChildren)
        return E_INVALIDARG;
    INT cItems = (INT)m_AccItems.size();
    if (!cItems)
        return E_FAIL;
    *pcountChildren = cItems - 1;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accChild(
    VARIANT varChildID,
    IDispatch **ppdispChild)
{
    if (!ppdispChild)
        return E_INVALIDARG;
    *ppdispChild = NULL;
    return S_FALSE;
}

STDMETHODIMP CTipbarAccessible::get_accName(
    VARIANT varID,
    BSTR *pszName)
{
    if (!pszName)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return E_INVALIDARG;
    *pszName = pItem->GetAccName();
    if (!*pszName)
        return DISP_E_MEMBERNOTFOUND;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accValue(
    VARIANT varID,
    BSTR *pszValue)
{
    if (!pszValue)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return E_INVALIDARG;
    *pszValue = pItem->GetAccValue();
    if (!*pszValue)
        return DISP_E_MEMBERNOTFOUND;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accDescription(
    VARIANT varID,
    BSTR *description)
{
    if (!description)
        return E_INVALIDARG;
    return m_pStdAccessible->get_accDescription(varID, description);
}

STDMETHODIMP CTipbarAccessible::get_accRole(
    VARIANT varID,
    VARIANT *role)
{
    if (!role)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return E_INVALIDARG;
    V_VT(role) = VT_I4;
    V_I4(role) = pItem->GetAccRole();
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accState(
    VARIANT varID,
    VARIANT *state)
{
    if (!state)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return E_INVALIDARG;
    V_VT(state) = VT_I4;
    V_I4(state) = pItem->GetAccState();
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accHelp(VARIANT varID, BSTR *help)
{
    return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP CTipbarAccessible::get_accHelpTopic(
    BSTR *helpfile,
    VARIANT varID,
    LONG *pidTopic)
{
    return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP CTipbarAccessible::get_accKeyboardShortcut(VARIANT varID, BSTR *shortcut)
{
    return DISP_E_MEMBERNOTFOUND;
}

STDMETHODIMP CTipbarAccessible::get_accFocus(VARIANT *pvarID)
{
    if (!pvarID)
        return E_INVALIDARG;
    V_VT(pvarID) = VT_EMPTY;
    return S_FALSE;
}

STDMETHODIMP CTipbarAccessible::get_accSelection(VARIANT *pvarID)
{
    if (!pvarID)
        return E_INVALIDARG;

    V_VT(pvarID) = VT_EMPTY;

    INT cItems = (INT)m_AccItems.size();
    if (cItems < m_cSelection)
        return S_FALSE;

    if (cItems > m_cSelection)
    {
        V_VT(pvarID) = VT_I4;
        V_I4(pvarID) = m_cSelection;
    }

    return S_OK;
}

STDMETHODIMP CTipbarAccessible::get_accDefaultAction(
    VARIANT varID,
    BSTR *action)
{
    if (!action)
        return E_INVALIDARG;
    *action = NULL;

    if (V_VT(&varID) != VT_I4)
        return E_INVALIDARG;

    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return DISP_E_MEMBERNOTFOUND;
    *action = pItem->GetAccDefaultAction();
    if (!*action)
        return S_FALSE;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::accSelect(
    LONG flagsSelect,
    VARIANT varID)
{
    if ((flagsSelect & SELFLAG_ADDSELECTION) && (flagsSelect & SELFLAG_REMOVESELECTION))
        return E_INVALIDARG;
    if (flagsSelect & (SELFLAG_TAKEFOCUS | SELFLAG_ADDSELECTION | SELFLAG_EXTENDSELECTION))
        return S_FALSE;
    if (flagsSelect & SELFLAG_REMOVESELECTION)
        return S_OK;
    if (V_VT(&varID) != VT_I4)
        return E_INVALIDARG;
    if (flagsSelect & SELFLAG_TAKESELECTION)
    {
        m_cSelection = V_I4(&varID);
        return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CTipbarAccessible::accLocation(
    LONG *left,
    LONG *top,
    LONG *width,
    LONG *height,
    VARIANT varID)
{
    if (!left || !top || !width || !height)
        return E_INVALIDARG;

    if (!V_I4(&varID))
        return m_pStdAccessible->accLocation(left, top, width, height, varID);

    RECT rc;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    pItem->GetAccLocation(&rc);

    *left = rc.left;
    *top = rc.top;
    *width = rc.right - rc.left;
    *height = rc.bottom - rc.top;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::accNavigate(
    LONG dir,
    VARIANT varStart,
    VARIANT *pvarEnd)
{
    if (m_AccItems.size() <= 1)
    {
        V_VT(pvarEnd) = VT_EMPTY;
        return S_OK;
    }

    switch (dir)
    {
        case NAVDIR_UP:
        case NAVDIR_LEFT:
        case NAVDIR_PREVIOUS:
            V_VT(pvarEnd) = VT_I4;
            V_I4(pvarEnd) = V_I4(&varStart) - 1;
            if (V_I4(&varStart) - 1 <= 0)
                V_I4(pvarEnd) = (INT)(m_AccItems.size() - 1);
            return S_OK;

        case NAVDIR_DOWN:
        case NAVDIR_RIGHT:
        case NAVDIR_NEXT:
            V_VT(pvarEnd) = VT_I4;
            V_I4(pvarEnd) = V_I4(&varStart) + 1;
            if ((INT)m_AccItems.size() <= V_I4(&varStart) + 1)
                V_I4(pvarEnd) = 1;
            return S_OK;

        case NAVDIR_FIRSTCHILD:
            V_VT(pvarEnd) = VT_I4;
            V_I4(pvarEnd) = 1;
            return S_OK;

        case NAVDIR_LASTCHILD:
            V_VT(pvarEnd) = VT_I4;
            V_I4(pvarEnd) = (INT)(m_AccItems.size() - 1);
            return S_OK;

        default:
            break;
    }

    V_VT(pvarEnd) = VT_EMPTY;
    return S_OK;
}

STDMETHODIMP CTipbarAccessible::accHitTest(LONG left, LONG top, VARIANT *pvarID)
{
    if (!pvarID)
        return E_INVALIDARG;
    POINT Point = { left, top };
    RECT Rect;
    ::ScreenToClient(m_hWnd, &Point);
    ::GetClientRect(m_hWnd, &Rect);

    if (!::PtInRect(&Rect, Point))
    {
        V_VT(pvarID) = VT_EMPTY;
        return S_OK;
    }

    V_VT(pvarID) = VT_I4;
    V_I4(pvarID) = 0;

    for (size_t iItem = 1; iItem < m_AccItems.size(); ++iItem)
    {
        CTipbarAccItem *pItem = m_AccItems[iItem];
        if (pItem)
        {
            pItem->GetAccLocation(&Rect);
            if (::PtInRect(&Rect, Point))
            {
                V_I4(pvarID) = iItem;
                break;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CTipbarAccessible::accDoDefaultAction(VARIANT varID)
{
    if (V_VT(&varID) != VT_I4)
        return E_INVALIDARG;
    CTipbarAccItem *pItem = AccItemFromID(V_I4(&varID));
    if (!pItem)
        return DISP_E_MEMBERNOTFOUND;
    return (pItem->DoAccDefaultAction() ? S_OK : S_FALSE);
}

STDMETHODIMP CTipbarAccessible::put_accName(VARIANT varID, BSTR name)
{
    return S_FALSE;
}

STDMETHODIMP CTipbarAccessible::put_accValue(VARIANT varID, BSTR value)
{
    return S_FALSE;
}

/***********************************************************************
 * CUTBMenuWnd
 */

CUTBMenuWnd::CUTBMenuWnd(HINSTANCE hInst, DWORD style, DWORD dwUnknown14)
    : CUIFMenu(hInst, style, dwUnknown14)
{
}

BOOL CUTBMenuWnd::StartDoAccDefaultActionTimer(CUTBMenuItem *pTarget)
{
    if (!m_pAccessible)
        return FALSE;

    m_nMenuWndID = m_pAccessible->GetIDOfItem(pTarget);
    if (!m_nMenuWndID || m_nMenuWndID == (UINT)-1)
        return FALSE;

    if (::IsWindow(m_hWnd))
    {
        ::KillTimer(m_hWnd, TIMER_ID_DOACCDEFAULTACTION);
        ::SetTimer(m_hWnd, TIMER_ID_DOACCDEFAULTACTION, g_uTimerElapseDOACCDEFAULTACTION, NULL);
    }

    return TRUE;
}

STDMETHODIMP_(BSTR) CUTBMenuWnd::GetAccName()
{
    WCHAR szText[64];
    LoadStringW(g_hInst, IDS_MENUWND, szText, _countof(szText));
    return ::SysAllocString(szText);
}

STDMETHODIMP_(INT) CUTBMenuWnd::GetAccRole()
{
    return 9;
}

STDMETHODIMP_(BOOL) CUTBMenuWnd::Initialize()
{
    CTipbarAccessible *pAccessible = new(cicNoThrow) CTipbarAccessible(GetAccItem());
    if (pAccessible)
        m_pAccessible = pAccessible;

    return CUIFObject::Initialize();
}

STDMETHODIMP_(void) CUTBMenuWnd::OnCreate(HWND hWnd)
{
    if (m_pAccessible)
        m_pAccessible->SetWindow(hWnd);
}

STDMETHODIMP_(void) CUTBMenuWnd::OnDestroy(HWND hWnd)
{
    if (m_pAccessible)
    {
        m_pAccessible->NotifyWinEvent(EVENT_OBJECT_DESTROY, GetAccItem());
        m_pAccessible->ClearAccItems();
        m_pAccessible->Release();
        m_pAccessible = NULL;
    }
    m_coInit.CoUninit();
}

STDMETHODIMP_(HRESULT)
CUTBMenuWnd::OnGetObject(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (lParam != -4)
        return S_OK;

    if (!m_pAccessible)
        return E_OUTOFMEMORY;

    if (m_pAccessible->m_bInitialized)
        return m_pAccessible->CreateRefToAccObj(wParam);

    if (SUCCEEDED(m_coInit.EnsureCoInit()))
    {
        HRESULT hr = m_pAccessible->Initialize();
        if (FAILED(hr))
        {
            m_pAccessible->Release();
            m_pAccessible = NULL;
            return hr;
        }

        m_pAccessible->NotifyWinEvent(EVENT_OBJECT_CREATE, GetAccItem());
        return m_pAccessible->CreateRefToAccObj(wParam);
    }

    return S_OK;
}

STDMETHODIMP_(LRESULT)
CUTBMenuWnd::OnShowWindow(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (m_pAccessible)
    {
        if (wParam)
        {
            m_pAccessible->NotifyWinEvent(EVENT_OBJECT_SHOW, GetAccItem());
            m_pAccessible->NotifyWinEvent(EVENT_OBJECT_FOCUS, GetAccItem());
        }
        else
        {
            m_pAccessible->NotifyWinEvent(EVENT_OBJECT_HIDE, GetAccItem());
        }
    }

    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

STDMETHODIMP_(void) CUTBMenuWnd::OnTimer(WPARAM wParam)
{
    if (wParam == TIMER_ID_DOACCDEFAULTACTION)
    {
        ::KillTimer(m_hWnd, TIMER_ID_DOACCDEFAULTACTION);
        if (m_pAccessible && m_nMenuWndID)
        {
            m_pAccessible->DoDefaultActionReal(m_nMenuWndID);
            m_nMenuWndID = 0;
        }
    }
}

/***********************************************************************
 * CUTBMenuItem
 */

CUTBMenuItem::CUTBMenuItem(CUTBMenuWnd *pMenuUI)
    : CUIFMenuItem(pMenuUI ? pMenuUI->GetMenu() : NULL)
{
    m_pMenuUI = pMenuUI;
}

CUTBMenuItem::~CUTBMenuItem()
{
    if (m_hbmColor)
    {
        ::DeleteObject(m_hbmColor);
        m_hbmColor = NULL;
    }
    if (m_hbmMask)
    {
        ::DeleteObject(m_hbmMask);
        m_hbmMask = NULL;
    }
}

STDMETHODIMP_(BOOL) CUTBMenuItem::DoAccDefaultAction()
{
    if (!m_pMenuUI)
        return FALSE;

    m_pMenuUI->StartDoAccDefaultActionTimer(this);
    return TRUE;
}

STDMETHODIMP_(BOOL) CUTBMenuItem::DoAccDefaultActionReal()
{
    if (!m_pSubMenu)
        OnLButtonUp(0, 0);
    else
        ShowSubPopup();
    return TRUE;
}

STDMETHODIMP_(BSTR) CUTBMenuItem::GetAccDefaultAction()
{
    WCHAR szText[64];
    ::LoadStringW(g_hInst, IDS_LEFTCLICK, szText, _countof(szText));
    return ::SysAllocString(szText);
}

STDMETHODIMP_(void) CUTBMenuItem::GetAccLocation(LPRECT lprc)
{
    GetRect(lprc);
    ::ClientToScreen(m_pMenuUI->m_hWnd, (LPPOINT)lprc);
    ::ClientToScreen(m_pMenuUI->m_hWnd, (LPPOINT)&lprc->right);
}

STDMETHODIMP_(BSTR) CUTBMenuItem::GetAccName()
{
    return ::SysAllocString(m_pszMenuItemLeft);
}

/// @unimplemented
STDMETHODIMP_(INT) CUTBMenuItem::GetAccRole()
{
    if (FALSE) //FIXME
        return 21;
    return 12;
}

/***********************************************************************
 * CModalMenu
 */

CUTBMenuItem *
CModalMenu::InsertItem(CUTBMenuWnd *pMenuUI, INT nCommandId, INT nStringID)
{
    CUTBMenuItem *pMenuItem = new(cicNoThrow) CUTBMenuItem(pMenuUI);
    if (!pMenuItem)
        return NULL;

    WCHAR szText[256];
    ::LoadStringW(g_hInst, nStringID, szText, _countof(szText));

    if (pMenuItem->Initialize() &&
        pMenuItem->Init(nCommandId, szText) &&
        pMenuUI->InsertItem(pMenuItem))
    {
        return pMenuItem;
    }

    delete pMenuItem;
    return NULL;
}

void CModalMenu::PostKey(BOOL bUp, WPARAM wParam, LPARAM lParam)
{
    m_pMenuUI->PostKey(bUp, wParam, lParam);
}

void CModalMenu::CancelMenu()
{
    if (m_pMenuUI)
        m_pMenuUI->CancelMenu();
}

/***********************************************************************
 * CUTBContextMenu
 */

CUTBContextMenu::CUTBContextMenu(CTipbarWnd *pTipbarWnd)
{
    m_pTipbarWnd = pTipbarWnd;
}

/// @implemented
BOOL CUTBContextMenu::Init()
{
    m_pTipbarThread = m_pTipbarWnd->m_pFocusThread;
    return !!m_pTipbarThread;
}

/// @unimplemented
CUTBMenuWnd *CUTBContextMenu::CreateMenuUI(BOOL bFlag)
{
    DWORD dwStatus = 0;

    if (FAILED(m_pTipbarWnd->m_pLangBarMgr->GetShowFloatingStatus(&dwStatus)))
        return NULL;

    CUTBMenuWnd *pMenuUI = new (cicNoThrow) CUTBMenuWnd(g_hInst, g_dwMenuStyle, 0);
    if (!pMenuUI)
        return NULL;

    pMenuUI->Initialize();

    if (dwStatus & (TF_SFT_DESKBAND | TF_SFT_MINIMIZED))
    {
        CUTBMenuItem *pRestoreLangBar = InsertItem(pMenuUI, ID_RESTORELANGBAR, IDS_RESTORELANGBAR2);
        if (pRestoreLangBar && !m_pTipbarWnd->m_dwUnknown20)
            pRestoreLangBar->Gray(TRUE);
    }
    else
    {
        InsertItem(pMenuUI, ID_DESKBAND, IDS_MINIMIZE);

        if (bFlag)
        {
            if (IsTransparecyAvailable())
            {
                if (dwStatus & TF_LBI_BALLOON)
                {
                    InsertItem(pMenuUI, ID_TRANS, IDS_TRANSPARENCY);
                }
                else
                {
                    CUTBMenuItem *pTransparency = InsertItem(pMenuUI, ID_NOTRANS, IDS_TRANSPARENCY);
                    if (pTransparency)
                        pTransparency->Check(TRUE);
                }
            }

            if (!(dwStatus & TF_SFT_LABELS))
            {
                InsertItem(pMenuUI, ID_LABELS, IDS_TEXTLABELS);
            }
            else
            {
                CUTBMenuItem *pTextLabels = InsertItem(pMenuUI, ID_NOLABELS, IDS_TEXTLABELS);
                if (pTextLabels)
                    pTextLabels->Check(TRUE);
            }

            CUTBMenuItem *pVertical = InsertItem(pMenuUI, ID_VERTICAL, IDS_VERTICAL);
            if (pVertical)
                pVertical->Check(!!(m_pTipbarWnd->m_dwTipbarWndFlags & TIPBAR_VERTICAL));
        }
    }

    if (bFlag)
    {
        CUTBMenuItem *pExtraIcons = NULL;

        if (dwStatus & TF_SFT_EXTRAICONSONMINIMIZED)
        {
            pExtraIcons = InsertItem(pMenuUI, ID_NOEXTRAICONS, IDS_EXTRAICONS);
            if (pExtraIcons)
                pExtraIcons->Check(TRUE);
        }
        else
        {
            pExtraIcons = CModalMenu::InsertItem(pMenuUI, ID_EXTRAICONS, IDS_EXTRAICONS);
        }

        if (pExtraIcons)
        {
            if (::GetKeyboardLayoutList(0, NULL) == 1)
            {
                pExtraIcons->Check(TRUE);
                pExtraIcons->Gray(TRUE);
            }
            else
            {
                pExtraIcons->Gray(FALSE);
            }
        }

        if (dwStatus & TF_SFT_DESKBAND)
            InsertItem(pMenuUI, ID_ADJUSTDESKBAND, IDS_ADJUSTLANGBAND);

        InsertItem(pMenuUI, ID_SETTINGS, IDS_SETTINGS);

        if (CheckCloseMenuAvailable())
            InsertItem(pMenuUI, ID_CLOSELANGBAR, IDS_CLOSELANGBAR);
    }

    return pMenuUI;
}

UINT
CUTBContextMenu::ShowPopup(
    CUIFWindow *pWindow,
    POINT pt,
    LPCRECT prc,
    BOOL bFlag)
{
    if (g_bWinLogon)
        return 0;

    if (m_pMenuUI)
        return -1;

    m_pMenuUI = CreateMenuUI(bFlag);
    if (!m_pMenuUI)
        return 0;

    UINT nCommandId = m_pMenuUI->ShowModalPopup(pWindow, prc, TRUE);

    if (m_pMenuUI)
    {
        delete m_pMenuUI;
        m_pMenuUI = NULL;
    }

    return nCommandId;
}

/// @unimplemented
BOOL CUTBContextMenu::SelectMenuItem(UINT nCommandId)
{
    switch (nCommandId)
    {
        case ID_TRANS:
            m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_LOWTRANSPARENCY);
            break;

        case ID_NOTRANS:
            m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_NOTRANSPARENCY);
            break;

        case ID_LABELS:
            m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_LABELS);
            break;

        case ID_NOLABELS:
            m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_NOLABELS);
            break;

        case ID_DESKBAND:
        {
            if (!g_bEnableDeskBand || !(g_dwOSInfo & CIC_OSINFO_XPPLUS))
            {
                m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_MINIMIZED);
            }
            else
            {
                DWORD dwStatus;
                m_pTipbarWnd->m_pLangBarMgr->GetShowFloatingStatus(&dwStatus);

                if (dwStatus & TF_SFT_DESKBAND)
                    break;

                m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_DESKBAND);
            }

            CUTBMinimizeLangBarDlg *pDialog = new(cicNoThrow) CUTBMinimizeLangBarDlg();
            if (pDialog)
            {
                pDialog->DoModal(*m_pTipbarWnd->GetWindow());
                pDialog->_Release();
            }
            break;
        }

        case ID_CLOSELANGBAR:
        {
            CUTBCloseLangBarDlg *pDialog = new(cicNoThrow) CUTBCloseLangBarDlg();
            if (pDialog)
            {
                BOOL bOK = pDialog->DoModal(*m_pTipbarWnd->GetWindow());
                pDialog->_Release();
                if (!bOK)
                    DoCloseLangbar();
            }
            break;
        }

        case ID_EXTRAICONS:
            m_pTipbarWnd->m_dwTipbarWndFlags &= ~TIPBAR_NODESKBAND;
            m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_EXTRAICONSONMINIMIZED);
            break;

        case ID_NOEXTRAICONS:
            m_pTipbarWnd->m_dwTipbarWndFlags &= ~TIPBAR_NODESKBAND;
            m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_NOEXTRAICONSONMINIMIZED);
            break;

        case ID_RESTORELANGBAR:
            m_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_LBI_ICON);
            break;

        case ID_VERTICAL:
            m_pTipbarWnd->SetVertical(!!(m_pTipbarWnd->m_dwTipbarWndFlags & TIPBAR_VERTICAL));
            break;

        case ID_ADJUSTDESKBAND:
            m_pTipbarWnd->AdjustDeskBandSize(TRUE);
            break;

        case ID_SETTINGS:
            TF_RunInputCPL();
            break;

        default:
            break;
    }

    return TRUE;
}

/***********************************************************************
 * CTrayIconItem
 */

CTrayIconItem::CTrayIconItem(CTrayIconWnd *pTrayIconWnd)
{
    m_dwIconAddOrModify = NIM_ADD;
    m_pTrayIconWnd = pTrayIconWnd;
}

BOOL
CTrayIconItem::_Init(
    HWND hWnd,
    UINT uCallbackMessage,
    UINT uNotifyIconID,
    const GUID& rguid)
{
    m_hWnd = hWnd;
    m_uCallbackMessage = uCallbackMessage;
    m_uNotifyIconID = uNotifyIconID;
    m_guid = rguid;
    return TRUE;
}

BOOL CTrayIconItem::RemoveIcon()
{
    if (m_dwIconAddOrModify == NIM_MODIFY)
    {
        NOTIFYICONDATAW NotifyIcon = { sizeof(NotifyIcon), m_hWnd, m_uNotifyIconID };
        NotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        NotifyIcon.uCallbackMessage = m_uCallbackMessage;
        ::Shell_NotifyIconW(NIM_DELETE, &NotifyIcon);
    }

    m_dwIconAddOrModify = NIM_ADD;
    m_bIconAdded = TRUE;
    return TRUE;
}

BOOL CTrayIconItem::SetIcon(HICON hIcon, LPCWSTR pszTip)
{
    if (!hIcon)
        return FALSE;

    NOTIFYICONDATAW NotifyIcon = { sizeof(NotifyIcon), m_hWnd, m_uNotifyIconID };
    NotifyIcon.uFlags = NIF_ICON | NIF_MESSAGE;
    NotifyIcon.uCallbackMessage = m_uCallbackMessage;
    NotifyIcon.hIcon = hIcon;
    if (pszTip)
    {
        NotifyIcon.uFlags |= NIF_TIP;
        StringCchCopyW(NotifyIcon.szTip, _countof(NotifyIcon.szTip), pszTip);
    }

    ::Shell_NotifyIconW(m_dwIconAddOrModify, &NotifyIcon);

    m_dwIconAddOrModify = NIM_MODIFY;
    m_bIconAdded = NIM_MODIFY;
    return TRUE;
}

BOOL CTrayIconItem::UpdateMenuRectPoint()
{
    HWND hNotifyWnd = m_pTrayIconWnd->GetNotifyWnd();
    ::GetClientRect(hNotifyWnd, &m_rcMenu);
    ::ClientToScreen(hNotifyWnd, (LPPOINT)&m_rcMenu);
    ::ClientToScreen(hNotifyWnd, (LPPOINT)&m_rcMenu.right);
    ::GetCursorPos(&m_ptCursor);
    return TRUE;
}

/***********************************************************************
 * CButtonIconItem
 */

CButtonIconItem::CButtonIconItem(CTrayIconWnd *pWnd, DWORD dwUnknown24)
    : CTrayIconItem(pWnd)
{
    m_dwUnknown24 = dwUnknown24;
}

/// @unimplemented
STDMETHODIMP_(BOOL) CButtonIconItem::OnMsg(WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
            break;
        default:
            return TRUE;
    }

    //FIXME
    return TRUE;
}

/// @unimplemented
STDMETHODIMP_(BOOL) CButtonIconItem::OnDelayMsg(UINT uMsg)
{
    //FIXME
    return FALSE;
}

/***********************************************************************
 * CMainIconItem
 */

/// @implemented
CMainIconItem::CMainIconItem(CTrayIconWnd *pWnd)
    : CButtonIconItem(pWnd, 1)
{
}

/// @implemented
BOOL CMainIconItem::Init(HWND hWnd)
{
    return CTrayIconItem::_Init(hWnd, WM_USER, 0, GUID_LBI_TRAYMAIN);
}

/// @implemented
STDMETHODIMP_(BOOL) CMainIconItem::OnDelayMsg(UINT uMsg)
{
    if (!CButtonIconItem::OnDelayMsg(uMsg))
        return 0;

    if (uMsg == WM_LBUTTONDBLCLK)
    {
        if (g_pTipbarWnd->m_dwUnknown20)
            g_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_SHOWNORMAL);
    }
    else if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN)
    {
        g_pTipbarWnd->ShowContextMenu(m_ptCursor, &m_rcMenu, uMsg == WM_RBUTTONDOWN);
    }
    return TRUE;
}

/***********************************************************************
 * CTrayIconWnd
 */

CTrayIconWnd::CTrayIconWnd()
{
    m_uCallbackMsg = WM_USER + 0x1000;
    m_uNotifyIconID = 0x1000;
}

CTrayIconWnd::~CTrayIconWnd()
{
    for (size_t iItem = 0; iItem < m_Items.size(); ++iItem)
    {
        auto& pItem = m_Items[iItem];
        if (pItem)
        {
            delete pItem;
            pItem = NULL;
        }
    }
}

void CTrayIconWnd::CallOnDelayMsg()
{
    for (size_t iItem = 0; iItem < m_Items.size(); ++iItem)
    {
        auto pItem = m_Items[iItem];
        if (pItem && m_uCallbackMessage == pItem->m_uCallbackMessage)
        {
            pItem->OnDelayMsg(m_uMsg);
            break;
        }
    }
}

HWND CTrayIconWnd::CreateWnd()
{
    m_hWnd = ::CreateWindowEx(0, TEXT("CTrayIconWndClass"), NULL, WS_DISABLED,
                              0, 0, 0, 0, NULL, NULL, g_hInst, this);
    FindTrayEtc();

    m_pMainIconItem = new(cicNoThrow) CMainIconItem(this);
    if (m_pMainIconItem)
    {
        m_pMainIconItem->Init(m_hWnd);
        m_Items.Add(m_pMainIconItem);
    }

    return m_hWnd;
}

void CTrayIconWnd::DestroyWnd()
{
    ::DestroyWindow(m_hWnd);
    m_hWnd = NULL;
}

BOOL CALLBACK CTrayIconWnd::EnumChildWndProc(HWND hWnd, LPARAM lParam)
{
    CTrayIconWnd *pWnd = (CTrayIconWnd *)lParam;

    TCHAR ClassName[60];
    ::GetClassName(hWnd, ClassName, _countof(ClassName));
    if (lstrcmp(ClassName, TEXT("TrayNotifyWnd")) != 0)
        return TRUE;

    pWnd->m_hNotifyWnd = hWnd;
    return FALSE;
}

CButtonIconItem *CTrayIconWnd::FindIconItem(REFGUID rguid)
{
    for (size_t iItem = 0; iItem < m_Items.size(); ++iItem)
    {
        auto pItem = m_Items[iItem];
        if (IsEqualGUID(rguid, pItem->m_guid))
            return pItem;
    }
    return NULL;
}

BOOL CTrayIconWnd::FindTrayEtc()
{
    m_hTrayWnd = ::FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (!m_hTrayWnd)
        return FALSE;

    ::EnumChildWindows(m_hTrayWnd, EnumChildWndProc, (LPARAM)this);
    if (!m_hNotifyWnd)
        return FALSE;
    m_dwTrayWndThreadId = ::GetWindowThreadProcessId(m_hTrayWnd, NULL);
    m_hwndProgman = FindWindow(TEXT("Progman"), NULL);
    m_dwProgmanThreadId = ::GetWindowThreadProcessId(m_hwndProgman, NULL);
    return TRUE;
}

HWND CTrayIconWnd::GetNotifyWnd()
{
    if (!::IsWindow(m_hNotifyWnd))
        FindTrayEtc();
    return m_hNotifyWnd;
}

/// @implemented
BOOL CTrayIconWnd::OnIconMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (g_pTipbarWnd)
        g_pTipbarWnd->AttachFocusThread();

    for (size_t iItem = 0; iItem < m_Items.size(); ++iItem)
    {
        auto *pItem = m_Items[iItem];
        if (pItem)
        {
            if (uMsg == pItem->m_uCallbackMessage)
            {
                pItem->OnMsg(wParam, lParam);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL CTrayIconWnd::RegisterClass()
{
    WNDCLASSEX wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance = g_hInst;
    wc.hCursor = ::LoadCursor(NULL, (LPCTSTR)IDC_ARROW);
    wc.lpfnWndProc = CTrayIconWnd::_WndProc;
    wc.lpszClassName = TEXT("CTrayIconWndClass");
    ::RegisterClassEx(&wc);
    return TRUE;
}

void CTrayIconWnd::RemoveAllIcon(DWORD dwFlags)
{
    for (size_t iItem = 0; iItem < m_Items.size(); ++iItem)
    {
        auto pItem = m_Items[iItem];
        if (dwFlags & 0x1)
        {
            if (IsEqualGUID(pItem->m_guid, GUID_LBI_INATITEM) ||
                IsEqualGUID(pItem->m_guid, GUID_LBI_CTRL))
            {
                continue;
            }
        }

        if (dwFlags & 0x2)
        {
            if (IsEqualGUID(pItem->m_guid, GUID_TFCAT_TIP_KEYBOARD))
                continue;
        }

        if (pItem->m_uNotifyIconID < 0x1000)
            continue;

        pItem->RemoveIcon();
    }
}

/// @unimplemented
void CTrayIconWnd::RemoveUnusedIcons(int unknown)
{
    //FIXME
}

BOOL CTrayIconWnd::SetIcon(REFGUID rguid, DWORD dwUnknown24, HICON hIcon, LPCWSTR psz)
{
    CButtonIconItem *pItem = FindIconItem(rguid);
    if (!pItem)
    {
        if (!hIcon)
            return FALSE;
        pItem = new(cicNoThrow) CButtonIconItem(this, dwUnknown24);
        if (!pItem)
            return FALSE;

        pItem->_Init(m_hWnd, m_uCallbackMsg, m_uNotifyIconID, rguid);
        m_uCallbackMsg += 2;
        ++m_uNotifyIconID;
        m_Items.Add(pItem);
    }

    if (!hIcon)
        return pItem->RemoveIcon();

    return pItem->SetIcon(hIcon, psz);
}

BOOL CTrayIconWnd::SetMainIcon(HKL hKL)
{
    if (!hKL)
    {
        m_pMainIconItem->RemoveIcon();
        m_pMainIconItem->m_hKL = NULL;
        return TRUE;
    }

    if (hKL != m_pMainIconItem->m_hKL)
    {
        WCHAR szText[64];
        HICON hIcon = TF_GetLangIcon(LOWORD(hKL), szText, _countof(szText));
        if (hIcon)
        {
            m_pMainIconItem->SetIcon(hIcon, szText);
            ::DestroyIcon(hIcon);
        }
        else
        {
            ::LoadStringW(g_hInst, IDS_RESTORELANGBAR, szText, _countof(szText));
            hIcon = ::LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_MAINICON));
            m_pMainIconItem->SetIcon(hIcon, szText);
        }

        m_pMainIconItem->m_hKL = hKL;
    }

    return TRUE;
}

CTrayIconWnd *CTrayIconWnd::GetThis(HWND hWnd)
{
    return (CTrayIconWnd *)::GetWindowLongPtr(hWnd, GWL_USERDATA);
}

void CTrayIconWnd::SetThis(HWND hWnd, LPCREATESTRUCT pCS)
{
    if (pCS)
        ::SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG_PTR)pCS->lpCreateParams);
    else
        ::SetWindowLongPtr(hWnd, GWL_USERDATA, 0);
}

LRESULT CALLBACK
CTrayIconWnd::_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTrayIconWnd *pThis;
    switch (uMsg)
    {
        case WM_CREATE:
            CTrayIconWnd::SetThis(hWnd, (LPCREATESTRUCT)lParam);
            break;
        case WM_DESTROY:
            ::SetWindowLongPtr(hWnd, GWL_USERDATA, 0);
            break;
        case WM_TIMER:
            if (wParam == 100)
            {
                ::KillTimer(hWnd, 100);
                pThis = CTrayIconWnd::GetThis(hWnd);
                if (pThis)
                    pThis->CallOnDelayMsg();
            }
            break;
        default:
        {
            if (uMsg < WM_USER)
                return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
            pThis = CTrayIconWnd::GetThis(hWnd);
            if (pThis && pThis->OnIconMessage(uMsg, wParam, lParam))
                break;
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }
    return 0;
}

/***********************************************************************
 * CLBarItemBase
 */

CLBarItemBase::CLBarItemBase()
{
    m_dwItemStatus = 0;
    m_szToolTipText[0] = 0;
    m_cRefs = 1;
    m_pLangBarItemSink = NULL;
}

CLBarItemBase::~CLBarItemBase()
{
    if (m_pLangBarItemSink)
        m_pLangBarItemSink->Release();
}

HRESULT
CLBarItemBase::AdviseSink(
    REFIID riid,
    IUnknown *punk,
    DWORD *pdwCookie)
{
    if (IsEqualIID(riid, IID_ITfLangBarItemSink) || m_pLangBarItemSink)
        return TF_E_NOOBJECT;

    HRESULT hr = punk->QueryInterface(IID_ITfLangBarItemSink, (void **)&m_pLangBarItemSink);
    if (SUCCEEDED(hr))
        *pdwCookie = 0x80000001;
    return hr;
}

HRESULT CLBarItemBase::UnadviseSink(DWORD dwCookie)
{
    if (dwCookie != 0x80000001)
        return E_FAIL;

    if (!m_pLangBarItemSink)
        return E_UNEXPECTED;

    m_pLangBarItemSink->Release();
    m_pLangBarItemSink = NULL;
    return S_OK;
}

void
CLBarItemBase::InitNuiInfo(
    REFIID clsidService,
    REFGUID guidItem,
    DWORD dwStyle,
    DWORD ulSort,
    LPCWSTR Source)
{
    m_NewUIInfo.clsidService = clsidService;
    m_NewUIInfo.guidItem = guidItem;
    m_NewUIInfo.dwStyle = dwStyle;
    m_NewUIInfo.ulSort = ulSort;
    StringCchCopyW(m_NewUIInfo.szDescription, _countof(m_NewUIInfo.szDescription), Source);
}

HRESULT
CLBarItemBase::ShowInternal(BOOL bShow, BOOL bUpdate)
{
    DWORD dwOldStatus = m_dwItemStatus;

    if (bShow)
        m_dwItemStatus &= ~TF_LBI_STATUS_HIDDEN;
    else
        m_dwItemStatus |= TF_LBI_STATUS_HIDDEN;

    if (bUpdate && (dwOldStatus != m_dwItemStatus))
    {
        if (m_pLangBarItemSink)
            m_pLangBarItemSink->OnUpdate(TF_LBI_STATUS);
    }

    return S_OK;
}

HRESULT CLBarItemBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    CopyMemory(pInfo, &m_NewUIInfo, sizeof(*pInfo));
    return S_OK;
}

HRESULT CLBarItemBase::GetStatus(DWORD *pdwStatus)
{
    *pdwStatus = m_dwItemStatus;
    return S_OK;
}

HRESULT CLBarItemBase::Show(BOOL fShow)
{
    return ShowInternal(fShow, TRUE);
}

HRESULT CLBarItemBase::GetTooltipString(BSTR *pbstrToolTip)
{
    if (!pbstrToolTip)
        return E_INVALIDARG;
    BSTR bstr = ::SysAllocString(m_szToolTipText);
    *pbstrToolTip = bstr;
    return bstr ? S_OK : E_OUTOFMEMORY;
}

/***********************************************************************
 * CUTBLBarMenu
 */

CUTBLBarMenu::CUTBLBarMenu(HINSTANCE hInst) : CCicLibMenu()
{
    m_hInst = hInst;
}

CUTBLBarMenu::~CUTBLBarMenu()
{
}

STDMETHODIMP_(CCicLibMenuItem*) CUTBLBarMenu::CreateMenuItem()
{
    CUTBLBarMenuItem *pItem = new(cicNoThrow) CUTBLBarMenuItem();
    if (!pItem)
        return NULL;
    pItem->m_pLBarMenu = this;
    return pItem;
}

CUTBMenuWnd *CUTBLBarMenu::CreateMenuUI()
{
    CUTBMenuWnd *pMenuUI = new(cicNoThrow) CUTBMenuWnd(m_hInst, g_dwMenuStyle, 0);
    if (!pMenuUI)
        return NULL;

    pMenuUI->Initialize();
    for (size_t iItem = 0; iItem < m_MenuItems.size(); ++iItem)
    {
        CUTBLBarMenuItem *pItem = (CUTBLBarMenuItem *)m_MenuItems[iItem];
        pItem->InsertToUI(pMenuUI);
    }

    return pMenuUI;
}

STDMETHODIMP_(CCicLibMenu*) CUTBLBarMenu::CreateSubMenu()
{
    return new(cicNoThrow) CUTBLBarMenu(m_hInst);
}

INT CUTBLBarMenu::ShowPopup(CUIFWindow *pWindow, POINT pt, LPCRECT prcExclude)
{
    if (m_pMenuUI)
        return 0;

    m_pMenuUI = CreateMenuUI();
    if (!m_pMenuUI)
        return -1;

    INT nCommandId = m_pMenuUI->ShowModalPopup(pWindow, prcExclude, TRUE);

    if (m_pMenuUI)
    {
        delete m_pMenuUI;
        m_pMenuUI = NULL;
    }

    return nCommandId;
}

/***********************************************************************
 * CUTBLBarMenuItem
 */

/// @unimplemented
BOOL CUTBLBarMenuItem::InsertToUI(CUTBMenuWnd *pMenuUI)
{
    if ((m_dwFlags & 4) != 0)
    {
        pMenuUI->InsertSeparator();
        return TRUE;
    }
    if (m_dwFlags & 2)
    {
        //FIXME
    }
    else
    {
        //FIXME
    }
    return FALSE;
}

/***********************************************************************
 * CLBarItemButtonBase
 */

CLBarItemButtonBase::~CLBarItemButtonBase()
{
    if (m_hIcon)
    {
        ::DestroyIcon(m_hIcon);
        m_hIcon = NULL;
    }
}

STDMETHODIMP CLBarItemButtonBase::QueryInterface(REFIID riid, void **ppvObject)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CLBarItemButtonBase, ITfLangBarItem),
        QITABENT(CLBarItemButtonBase, ITfLangBarItemButton),
        QITABENT(CLBarItemButtonBase, ITfSource),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObject);
}

STDMETHODIMP_(ULONG) CLBarItemButtonBase::AddRef()
{
    return ++m_cRefs;
}

STDMETHODIMP_(ULONG) CLBarItemButtonBase::Release()
{
    if (--m_cRefs == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @unimplemented
STDMETHODIMP CLBarItemButtonBase::OnClick(TfLBIClick click, POINT pt, LPCRECT prc)
{
    if (click == TF_LBI_CLK_RIGHT)
    {
        return E_NOTIMPL; //FIXME
    }
    if (click == TF_LBI_CLK_LEFT)
    {
        return E_NOTIMPL; //FIXME
    }
    return E_NOTIMPL;
}

STDMETHODIMP CLBarItemButtonBase::InitMenu(ITfMenu *pMenu)
{
    return E_NOTIMPL;
}

STDMETHODIMP CLBarItemButtonBase::OnMenuSelect(UINT wID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CLBarItemButtonBase::GetIcon(HICON *phIcon)
{
    return E_NOTIMPL;
}

STDMETHODIMP CLBarItemButtonBase::GetText(BSTR *pbstr)
{
    if (!pbstr)
        return E_INVALIDARG;
    *pbstr = ::SysAllocString(m_NewUIInfo.szDescription);
    return (*pbstr ? S_OK : E_OUTOFMEMORY);
}

STDMETHODIMP CLBarItemButtonBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    return CLBarItemBase::GetInfo(pInfo);
}

STDMETHODIMP CLBarItemButtonBase::GetStatus(DWORD *pdwStatus)
{
    return CLBarItemBase::GetStatus(pdwStatus);
}

STDMETHODIMP CLBarItemButtonBase::Show(BOOL fShow)
{
    return CLBarItemBase::Show(fShow);
}

STDMETHODIMP CLBarItemButtonBase::GetTooltipString(BSTR *pbstrToolTip)
{
    return CLBarItemBase::GetTooltipString(pbstrToolTip);
}

STDMETHODIMP CLBarItemButtonBase::AdviseSink(
    REFIID riid,
    IUnknown *punk,
    DWORD *pdwCookie)
{
    return CLBarItemBase::AdviseSink(riid, punk, pdwCookie);
}

STDMETHODIMP CLBarItemButtonBase::UnadviseSink(DWORD dwCookie)
{
    return CLBarItemBase::UnadviseSink(dwCookie);
}

/***********************************************************************
 * CLBarInatItem
 */

CLBarInatItem::CLBarInatItem(DWORD dwThreadId)
{
    WCHAR szText[256];
    ::LoadStringW(g_hInst, IDS_LANGUAGE, szText, _countof(szText));
    InitNuiInfo(CLSID_SYSTEMLANGBARITEM, GUID_LBI_INATITEM, 0x20001, 0, szText);

    ::LoadStringW(g_hInst, IDS_LANGUAGEBUTTON, szText, _countof(szText));
    StringCchCopyW(m_szToolTipText, _countof(m_szToolTipText), szText);
    m_dwThreadId = dwThreadId;
    m_hKL = ::GetKeyboardLayout(m_dwThreadId);

    TF_InitMlngInfo();
    ShowInternal(TF_MlngInfoCount() > 1, 0);
}

STDMETHODIMP CLBarInatItem::GetIcon(HICON *phIcon)
{
    HICON hIcon = NULL;
    INT iIndex = GetIconIndexFromhKL(m_hKL);
    if (iIndex != -1)
        hIcon = TF_InatExtractIcon(iIndex);
    *phIcon = hIcon;
    return S_OK;
}

STDMETHODIMP CLBarInatItem::GetText(BSTR *pbstr)
{
    if (!pbstr)
        return E_INVALIDARG;

    WCHAR szText[256];
    if (!GethKLDesc(m_hKL, szText, _countof(szText)))
        return GetText(pbstr);

    *pbstr = ::SysAllocString(szText);
    return S_OK;
}

STDMETHODIMP CLBarInatItem::InitMenu(ITfMenu *pMenu)
{
    TF_InitMlngInfo();

    INT iKL, cKLs = TF_MlngInfoCount();
    for (iKL = 0; iKL < cKLs; ++iKL)
    {
        HKL hKL;
        WCHAR szDesc[128];
        if (TF_GetMlngHKL(iKL, &hKL, szDesc, _countof(szDesc)))
        {
            HICON hIcon = NULL;
            INT iIndex = GetIconIndexFromhKL(hKL);
            if (iIndex != -1)
                hIcon = TF_InatExtractIcon(iIndex);

            LangBarInsertMenu(pMenu, iKL, szDesc, (hKL == m_hKL), hIcon);
        }
    }

    DWORD dwStatus;
    if (g_pTipbarWnd &&
        g_pTipbarWnd->m_pLangBarMgr &&
        SUCCEEDED(g_pTipbarWnd->m_pLangBarMgr->GetShowFloatingStatus(&dwStatus)) &&
        (dwStatus & (TF_SFT_DESKBAND | TF_SFT_MINIMIZED)))
    {
        LangBarInsertSeparator(pMenu);

        WCHAR szText[256];
        ::LoadStringW(g_hInst, IDS_RESTORELANGBAR2, szText, _countof(szText));
        LangBarInsertMenu(pMenu, 2000, szText, FALSE, NULL);
    }

    return S_OK;
}

STDMETHODIMP CLBarInatItem::OnMenuSelect(INT nCommandId)
{
    HKL hKL;

    if (nCommandId == 2000)
    {
        if (g_pTipbarWnd)
        {
            ITfLangBarMgr *pLangBarMgr = g_pTipbarWnd->m_pLangBarMgr;
            if (pLangBarMgr)
                pLangBarMgr->ShowFloating(TF_SFT_SHOWNORMAL);
        }
    }
    else if (TF_GetMlngHKL(nCommandId, &hKL, NULL, 0))
    {
        g_pTipbarWnd->RestoreLastFocus(NULL, !!(g_pTipbarWnd->m_dwTipbarWndFlags & TIPBAR_CHILD));
        HWND hwndFore = ::GetForegroundWindow();
        if (m_dwThreadId == ::GetWindowThreadProcessId(hwndFore, NULL))
        {
            BOOL FontSig = GetFontSig(hwndFore, hKL);
            ::PostMessage(hwndFore, WM_INPUTLANGCHANGEREQUEST, FontSig, (LPARAM)hKL);
        }
    }

    return S_OK;
}

/***********************************************************************
 * CTipbarGripper
 */

CTipbarGripper::CTipbarGripper(CTipbarWnd *pTipbarWnd, LPCRECT prc, DWORD style)
    : CUIFGripper((pTipbarWnd ? pTipbarWnd->GetWindow() : NULL), prc, style)
{
    m_bInDebugMenu = FALSE;
    m_pTipbarWnd = pTipbarWnd;
}

/// @unimplemented
STDMETHODIMP_(void) CTipbarGripper::OnLButtonUp(LONG x, LONG y)
{
    m_pTipbarWnd->RestoreFromStub();

    if (g_bEnableDeskBand && (g_dwOSInfo & CIC_OSINFO_XPPLUS))
    {
        APPBARDATA AppBar = { sizeof(AppBar) };
        AppBar.hWnd = ::FindWindowW(L"Shell_TrayWnd", NULL);
        if (::SHAppBarMessage(ABM_GETTASKBARPOS, &AppBar))
        {
            RECT rc = AppBar.rc;
            POINT pt;
            ::GetCursorPos(&pt);
            if (g_pTipbarWnd && ::PtInRect(&rc, pt))
                g_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_DESKBAND |
                                                          TF_SFT_EXTRAICONSONMINIMIZED);
        }
    }

    CUIFGripper::OnLButtonUp(x, y);
    m_pTipbarWnd->UpdatePosFlags();
}

/// @unimplemented
STDMETHODIMP_(void) CTipbarGripper::OnRButtonUp(LONG x, LONG y)
{
    if (g_bShowDebugMenu)
    {
        // FIXME: Debugging feature
    }
}

STDMETHODIMP_(BOOL) CTipbarGripper::OnSetCursor(UINT uMsg, LONG x, LONG y)
{
    if (m_bInDebugMenu)
        return FALSE;

    return CUIFGripper::OnSetCursor(uMsg, x, y);
}

/***********************************************************************
 * CLangBarItemList
 */

BOOL CLangBarItemList::IsStartedIntentionally(REFCLSID rclsid)
{
    auto *pItem = FindItem(rclsid);
    if (!pItem)
        return FALSE;
    return pItem->m_bStartedIntentionally;
}

LANGBARITEMSTATE *CLangBarItemList::AddItem(REFCLSID rclsid)
{
    auto *pItem = FindItem(rclsid);
    if (pItem)
        return pItem;

    pItem = Append(1);
    if (!pItem)
        return NULL;

    ZeroMemory(pItem, sizeof(*pItem));
    pItem->m_clsid = rclsid;
    pItem->m_dwDemoteLevel = 0;
    return pItem;
}

void CLangBarItemList::Clear()
{
    clear();

    CicRegKey regKey;
    LSTATUS error;
    error = regKey.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\CTF\\LangBar", KEY_ALL_ACCESS);
    if (error == ERROR_SUCCESS)
        regKey.RecurseDeleteKey(L"ItemState");
}

BOOL CLangBarItemList::SetDemoteLevel(REFCLSID rclsid, DWORD dwDemoteLevel)
{
    auto *pItem = AddItem(rclsid);
    if (!pItem)
        return TRUE;

    pItem->m_dwDemoteLevel = dwDemoteLevel;
    if (!pItem->IsShown())
    {
        if (pItem->m_nTimerID)
        {
            if (g_pTipbarWnd)
                g_pTipbarWnd->KillTimer(pItem->m_nTimerID);
            pItem->m_nTimerID = 0;
            pItem->m_uTimeOut = 0;
        }
        pItem->m_bDisableDemoting = FALSE;
    }

    SaveItem(0, pItem);
    return TRUE;
}

LANGBARITEMSTATE *CLangBarItemList::FindItem(REFCLSID rclsid)
{
    for (size_t iItem = 0; iItem < size(); ++iItem)
    {
        auto& item = (*this)[iItem];
        if (IsEqualCLSID(item.m_clsid, rclsid))
            return &item;
    }
    return NULL;
}

LANGBARITEMSTATE *CLangBarItemList::GetItemStateFromTimerId(UINT_PTR nTimerID)
{
    for (size_t iItem = 0; iItem < size(); ++iItem)
    {
        auto& item = (*this)[iItem];
        if (item.m_nTimerID == nTimerID)
            return &item;
    }
    return NULL;
}

void CLangBarItemList::Load()
{
    CicRegKey regKey;
    LSTATUS error;
    error = regKey.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\CTF\\LangBar\\ItemState");
    if (error != ERROR_SUCCESS)
        return;

    WCHAR szKeyName[MAX_PATH];
    for (DWORD dwIndex = 0; ; ++dwIndex)
    {
        error = ::RegEnumKeyW(regKey, dwIndex, szKeyName, _countof(szKeyName));
        if (error != ERROR_SUCCESS)
            break;

        CLSID clsid;
        if (::CLSIDFromString(szKeyName, &clsid) != S_OK)
            continue;

        CicRegKey regKey2;
        error = regKey2.Open(regKey, szKeyName);
        if (error != ERROR_SUCCESS)
            continue;

        auto *pItem = AddItem(clsid);
        if (!pItem)
            continue;

        DWORD Data = 0;
        regKey2.QueryDword(L"DemoteLevel", &Data);
        pItem->m_dwDemoteLevel = Data;
        regKey2.QueryDword(L"DisableDemoting", &Data);
        pItem->m_bDisableDemoting = !!Data;
    }
}

void CLangBarItemList::SaveItem(CicRegKey *pRegKey, const LANGBARITEMSTATE *pState)
{
    LSTATUS error;
    CicRegKey regKey;

    if (!pRegKey)
    {
        error = regKey.Create(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\CTF\\LangBar\\ItemState");
        if (error != ERROR_SUCCESS)
            return;

        pRegKey = &regKey;
    }

    WCHAR szSubKey[MAX_PATH];
    ::StringFromGUID2(pState->m_clsid, szSubKey, _countof(szSubKey));

    if (pState->m_dwDemoteLevel || pState->m_bDisableDemoting)
    {
        CicRegKey regKey2;
        error = regKey2.Create(*pRegKey, szSubKey);
        if (error == ERROR_SUCCESS)
        {
            DWORD dwDemoteLevel = pState->m_dwDemoteLevel;
            if (dwDemoteLevel)
                regKey2.SetDword(L"DemoteLevel", dwDemoteLevel);
            else
                regKey2.DeleteValue(L"DemoteLevel");

            regKey2.SetDword(L"DisableDemoting", pState->m_bDisableDemoting);
        }
    }
    else
    {
        pRegKey->RecurseDeleteKey(szSubKey);
    }
}

void CLangBarItemList::StartDemotingTimer(REFCLSID rclsid, BOOL bIntentional)
{
    if (!g_bIntelliSense)
        return;

    auto *pItem = AddItem(rclsid);
    if (!pItem || pItem->m_bDisableDemoting)
        return;

    if (pItem->m_nTimerID)
    {
        if (!bIntentional)
            return;

        if (g_pTipbarWnd)
            g_pTipbarWnd->KillTimer(pItem->m_nTimerID);

        pItem->m_nTimerID = 0;
    }

    pItem->m_bStartedIntentionally |= bIntentional;

    UINT uTimeOut = (bIntentional ? g_uTimeOutIntentional : g_uTimeOutNonIntentional);
    pItem->m_uTimeOut += uTimeOut;

    if (pItem->m_uTimeOut < g_uTimeOutMax)
    {
        UINT_PTR uDemotingTimerId = FindDemotingTimerId();
        pItem->m_nTimerID = uDemotingTimerId;
        if (uDemotingTimerId)
        {
            if (g_pTipbarWnd)
                g_pTipbarWnd->SetTimer(uDemotingTimerId, uTimeOut);
        }
    }
    else
    {
        pItem->m_bDisableDemoting = TRUE;
    }
}

UINT_PTR CLangBarItemList::FindDemotingTimerId()
{
    UINT_PTR nTimerID = 10000;

    if (empty())
        return nTimerID;

    for (;;)
    {
        size_t iItem = 0;

        while ((*this)[iItem].m_nTimerID != nTimerID)
        {
            ++iItem;
            if (iItem >= size())
                return nTimerID;
        }

        ++nTimerID;
        if (nTimerID >= 10050)
            return 0;
    }
}

/***********************************************************************
 * CTipbarWnd
 */

/// @unimplemented
CTipbarWnd::CTipbarWnd(DWORD style)
    : CUIFWindow(g_hInst, style)
{
    m_dwUnknown23_1[4] = 0;
    m_dwUnknown23_1[5] = 0;
    m_dwUnknown23_1[6] = 0;
    m_dwUnknown23_1[7] = 0;

    RECT rc;
    cicGetScreenRect(g_ptTipbar, &rc);

    //FIXME: Fix g_ptTipbar

    Move(g_ptTipbar.x, g_ptTipbar.y, 100, 24);
    UpdatePosFlags();

    m_hMarlettFont = ::CreateFontW(8, 8, 0, 0, FW_NORMAL, 0, 0, 0,
                                   SYMBOL_CHARSET, 0, 0, 0, 0, L"Marlett");

    ITfLangBarMgr *pLangBarMgr = NULL;
    if (SUCCEEDED(TF_CreateLangBarMgr(&pLangBarMgr)) && pLangBarMgr)
    {
        pLangBarMgr->QueryInterface(IID_ITfLangBarMgr_P, (void **)&m_pLangBarMgr);
        pLangBarMgr->Release();
    }

    if (style & UIF_WINDOW_ENABLETHEMED)
    {
        if (g_fTaskbarTheme)
        {
            m_iPartId = 1;
            m_iStateId = 1;
            m_pszClassList = L"TASKBAR";
        }
        else
        {
            m_iPartId = 0;
            m_iStateId = 1;
            m_pszClassList = L"REBAR";
        }
    }

    SetVertical(g_fVertical);

    m_cRefs = 1;
}

CTipbarWnd::~CTipbarWnd()
{
    UnInit();

    if (m_hMarlettFont)
        ::DeleteObject(m_hMarlettFont);
    if (m_hTextFont)
        ::DeleteObject(m_hTextFont);

    TFUninitLib_Thread(&g_libTLS);
}

/// @unimplemented
void CTipbarWnd::Init(BOOL bChild, CDeskBand *pDeskBand)
{
    if (bChild)
        m_dwTipbarWndFlags |= TIPBAR_CHILD;
    else
        m_dwTipbarWndFlags &= ~TIPBAR_CHILD;

    if (m_dwTipbarWndFlags & TIPBAR_CHILD)
        m_dwTipbarWndFlags &= TIPBAR_HIGHCONTRAST;

    m_pDeskBand = pDeskBand;

    RECT rc = { 0, 0, 0, 0 };

    if (g_bNewLook && !m_pWndFrame && (m_style & 0x20000000))
    {
        CUIFWndFrame *pWndFrame = new(cicNoThrow) CUIFWndFrame(GetWindow(), &rc, 0);
        if (pWndFrame)
        {
            pWndFrame->Initialize();
            AddUIObj(m_pWndFrame);
        }
    }

    if (!m_pTipbarGripper && !(m_dwTipbarWndFlags & TIPBAR_CHILD))
    {
        m_pTipbarGripper =
            new(cicNoThrow) CTipbarGripper(this, &rc, !!(m_dwTipbarWndFlags & TIPBAR_VERTICAL));
        if (m_pTipbarGripper)
        {
            m_pTipbarGripper->Initialize();
            AddUIObj(m_pTipbarGripper);
        }
    }

    //FIXME: CTipbarCtrlButtonHolder

    if (m_dwTipbarWndFlags & TIPBAR_VERTICAL)
    {
        Move(m_nLeft, m_nTop, GetTipbarHeight(), 0);
    }
    else
    {
        Move(m_nLeft, m_nTop, 0, GetTipbarHeight());
    }
}

void CTipbarWnd::InitHighContrast()
{
    m_dwTipbarWndFlags &= ~TIPBAR_HIGHCONTRAST;

    HIGHCONTRAST HiCon = { sizeof(HiCon) };
    if (::SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HiCon), &HiCon, 0))
    {
        if (HiCon.dwFlags & HCF_HIGHCONTRASTON)
            m_dwTipbarWndFlags |= TIPBAR_HIGHCONTRAST;
    }
}

void CTipbarWnd::InitMetrics()
{
    m_cxSmallIcon = ::GetSystemMetrics(SM_CXSMICON);
    m_cySmallIcon = ::GetSystemMetrics(SM_CYSMICON);

    DWORD_PTR style = ::GetWindowLongPtr(m_hWnd, GWL_STYLE);
    if (style & WS_DLGFRAME)
    {
        m_cxDlgFrameX2 = 2 * ::GetSystemMetrics(SM_CXDLGFRAME);
        m_cyDlgFrameX2 = 2 * ::GetSystemMetrics(SM_CYDLGFRAME);
    }
    else if (style & WS_BORDER)
    {
        m_cxDlgFrameX2 = 2 * ::GetSystemMetrics(SM_CXBORDER);
        m_cyDlgFrameX2 = 2 * ::GetSystemMetrics(SM_CYBORDER);
    }
    else
    {
        m_cxDlgFrameX2 = m_cyDlgFrameX2 = 0;
    }
}

void CTipbarWnd::InitThemeMargins()
{
    ZeroMemory(&m_Margins, sizeof(m_Margins));

    CUIFTheme theme;
    m_dwUnknown23_5[0] = 6;
    m_dwUnknown23_5[1] = 6;
    m_dwUnknown23_5[2] = 0;
    m_ButtonWidth = GetSystemMetrics(SM_CXSIZE);

    theme.m_iPartId = 1;
    theme.m_iStateId = 0;
    theme.m_pszClassList = L"TOOLBAR";
    if (SUCCEEDED(theme.InternalOpenThemeData(m_hWnd)))
    {
        ::GetThemeMargins(theme.m_hTheme, NULL, theme.m_iPartId, 1, 3602, NULL, &m_Margins);
        m_dwUnknown23_5[0] = 4;
        m_dwUnknown23_5[1] = 2;
        m_dwUnknown23_5[2] = 1;
    }
    theme.CloseThemeData();

    theme.m_iPartId = 18;
    theme.m_iStateId = 0;
    theme.m_pszClassList = L"WINDOW";
    if (SUCCEEDED(theme.InternalOpenThemeData(m_hWnd)))
    {
        SIZE partSize;
        ::GetThemePartSize(theme.m_hTheme, NULL, theme.m_iPartId, 1, 0, TS_TRUE, &partSize);
        INT size = ::GetThemeSysSize(theme.m_hTheme, 31);
        m_ButtonWidth = MulDiv(size, partSize.cx, partSize.cy);
    }
    theme.CloseThemeData();
}

void CTipbarWnd::UnInit()
{
    SetFocusThread(NULL);
    for (size_t iItem = 0; iItem < m_Threads.size(); ++iItem)
    {
        CTipbarThread* pThread = m_Threads[iItem];
        if (pThread)
        {
            pThread->_UninitItemList(TRUE);
            pThread->m_pTipbarWnd = NULL;
            pThread->_Release();
        }
    }
    m_Threads.clear();

    if (m_pLangBarMgr)
        m_pLangBarMgr->UnAdviseEventSink(m_dwSinkCookie);

    if (m_pLangBarMgr)
    {
        m_pLangBarMgr->Release();
        m_pLangBarMgr = NULL;
    }
}

BOOL CTipbarWnd::IsFullScreenWindow(HWND hWnd)
{
    if (g_fPolicyEnableLanguagebarInFullscreen)
        return FALSE;

    DWORD_PTR style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
    if (!(style & WS_VISIBLE) || (style & WS_CAPTION))
        return FALSE;

    DWORD_PTR exstyle = ::GetWindowLongPtr(hWnd, GWL_EXSTYLE);
    if (exstyle & WS_EX_LAYERED)
        return FALSE;

    if ((exstyle & WS_EX_TOOLWINDOW) && (hWnd == m_ShellWndThread.GetWndProgman()))
        return FALSE;

    return !!cicIsFullScreenSize(hWnd);
}

BOOL CTipbarWnd::IsHKLToSkipRedrawOnNoItem()
{
    HKL hKL = GetFocusKeyboardLayout();
    return IsSkipRedrawHKL(hKL);
}

BOOL CTipbarWnd::IsInItemChangeOrDirty(CTipbarThread *pTarget)
{
    if (pTarget->m_dwThreadId == m_dwChangingThreadId)
        return TRUE;
    return pTarget->IsDirtyItem();
}

void CTipbarWnd::AddThreadToThreadCreatingList(CTipbarThread *pThread)
{
    m_ThreadCreatingList.Add(pThread);
}

void CTipbarWnd::RemoveThredFromThreadCreatingList(CTipbarThread *pTarget)
{
    ssize_t iItem = m_ThreadCreatingList.Find(pTarget);
    if (iItem >= 0)
        m_ThreadCreatingList.Remove(iItem);
}

void CTipbarWnd::MoveToStub(BOOL bFlag)
{
    m_dwTipbarWndFlags |= 0x40;

    RECT rcWorkArea;
    ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

    if (bFlag)
    {
        m_nLeft = rcWorkArea.right - 38;
        m_dwTipbarWndFlags &= ~0x80;
    }
    else
    {
        RECT Rect;
        ::GetWindowRect(m_hWnd, &Rect);
        m_nLeft = rcWorkArea.right + Rect.left - Rect.right;
        m_dwTipbarWndFlags |= 0x80;
    }

    m_nTop = rcWorkArea.bottom - m_cyDlgFrameX2 - GetTipbarHeight();

    if (m_pFocusThread)
        m_pFocusThread->MyMoveWnd(0, 0);
}

void CTipbarWnd::RestoreFromStub()
{
    m_dwTipbarWndFlags &= 0x3F;
    KillTimer(1);
    KillTimer(2);
}

/// @unimplemented
INT CTipbarWnd::GetCtrlButtonWidth()
{
    return 0;
}

INT CTipbarWnd::GetGripperWidth()
{
    if (m_dwTipbarWndFlags & 2)
        return 0;

    if (!m_pTipbarGripper || FAILED(m_pTipbarGripper->EnsureThemeData(m_hWnd)))
        return 5;

    INT width = -1;
    SIZE partSize;
    HDC hDC = ::GetDC(m_hWnd);
    if (SUCCEEDED(m_pTipbarGripper->GetThemePartSize(hDC, 1, 0, TS_TRUE, &partSize)))
    {
        INT cx = partSize.cx;
        if (m_dwTipbarWndFlags & 4)
            cx = partSize.cy;
        width = cx + 4;
    }
    ::ReleaseDC(m_hWnd, hDC);

    return ((width < 0) ? 5 : width);
}

INT CTipbarWnd::GetTipbarHeight()
{
    SIZE size = { 0, 0 };
    if (m_pWndFrame)
        m_pWndFrame->GetFrameSize(&size);
    INT cy = m_Margins.cyBottomHeight + m_Margins.cyTopHeight + 2;
    if (cy < 6)
        cy = 6;
    return m_cySmallIcon + cy + (2 * size.cy);
}

BOOL CTipbarWnd::AutoAdjustDeskBandSize()
{
    if ((m_dwTipbarWndFlags & TIPBAR_NODESKBAND) ||
        !m_pFocusThread ||
        (m_pFocusThread->m_dwFlags1 & 0x800))
    {
        return FALSE;
    }

    DWORD dwOldWndFlags = m_dwTipbarWndFlags;
    m_dwTipbarWndFlags &= ~0x8000;

    if (!AdjustDeskBandSize(!(dwOldWndFlags & 0x8000)))
        return FALSE;

    m_dwTipbarWndFlags |= TIPBAR_NODESKBAND;
    return TRUE;
}

/// @unimplemented
INT CTipbarWnd::AdjustDeskBandSize(BOOL bFlag)
{
    return 0;
}

/// @unimplemented
void CTipbarWnd::LocateCtrlButtons()
{
}

void CTipbarWnd::AdjustPosOnDisplayChange()
{
    RECT rcWorkArea;
    RECT rc = { m_nLeft, m_nTop, m_nLeft + m_nWidth, m_nTop + m_nHeight };
    if (!GetWorkArea(&rc, &rcWorkArea))
        return;

    INT x = m_nLeft, y = m_nTop;
    if (m_dwTipbarWndFlags & TIPBAR_LEFTFIT)
        x = rcWorkArea.left;
    if (m_dwTipbarWndFlags & TIPBAR_TOPFIT)
        y = rcWorkArea.top;
    if (m_dwTipbarWndFlags & TIPBAR_RIGHTFIT)
        x = rcWorkArea.right - m_nWidth;
    if (m_dwTipbarWndFlags & TIPBAR_BOTTOMFIT)
        y = rcWorkArea.bottom - m_nHeight;
    if (x != m_nLeft || y != m_nTop)
        Move(x, y, m_nWidth, m_nHeight);
}

void CTipbarWnd::SetVertical(BOOL bVertical)
{
    if (bVertical)
        m_dwTipbarWndFlags |= TIPBAR_VERTICAL;
    else
        m_dwTipbarWndFlags &= ~TIPBAR_VERTICAL;

    if (m_pTipbarGripper)
    {
        DWORD style = m_pTipbarGripper->m_style;
        if (bVertical)
            style |= 0x1;
        else
            style &= 0x1;
        m_pTipbarGripper->SetStyle(style);
    }

    if (g_fTaskbarTheme)
        SetActiveTheme(L"TASKBAR", !!(m_dwTipbarWndFlags & TIPBAR_VERTICAL), 1);

    if (!(m_dwTipbarWndFlags & TIPBAR_CHILD))
    {
        if (m_dwTipbarWndFlags & TIPBAR_VERTICAL)
        {
            Move(m_nLeft, m_nTop, GetTipbarHeight(), 0);
        }
        else
        {
            Move(m_nLeft, m_nTop, 0, GetTipbarHeight());
        }
    }

    if (m_hWnd)
    {
        KillTimer(7);
        SetTimer(7, g_uTimerElapseSYSCOLORCHANGED);
    }
}

void CTipbarWnd::UpdatePosFlags()
{
    if (m_dwTipbarWndFlags & TIPBAR_CHILD)
        return;

    RECT rc = { m_nLeft, m_nTop, m_nLeft + m_nWidth, m_nTop + m_nHeight }, rcWorkArea;
    if (!GetWorkArea(&rc, &rcWorkArea))
        return;

    if (rcWorkArea.left + 2 < m_nLeft)
        m_dwTipbarWndFlags &= ~TIPBAR_LEFTFIT;
    else
        m_dwTipbarWndFlags |= TIPBAR_LEFTFIT;

    if (rcWorkArea.top + 2 < m_nTop)
        m_dwTipbarWndFlags &= ~TIPBAR_TOPFIT;
    else
        m_dwTipbarWndFlags |= TIPBAR_TOPFIT;

    if (m_nLeft + m_nWidth < rcWorkArea.right - 2)
        m_dwTipbarWndFlags &= ~TIPBAR_RIGHTFIT;
    else
        m_dwTipbarWndFlags |= TIPBAR_RIGHTFIT;

    if (m_nTop + m_nHeight < rcWorkArea.bottom - 2)
        m_dwTipbarWndFlags &= ~TIPBAR_BOTTOMFIT;
    else
        m_dwTipbarWndFlags |= TIPBAR_BOTTOMFIT;
}

void CTipbarWnd::CancelMenu()
{
    if (!m_pThread)
        return;

    CTipbarWnd *pTipbarWnd = m_pThread->m_pTipbarWnd;
    if (pTipbarWnd)
    {
        if (pTipbarWnd->m_pLangBarMgr)
            pTipbarWnd->StartModalInput(NULL, m_pThread->m_dwThreadId);
    }

    m_pModalMenu->CancelMenu();
    StartBackToAlphaTimer();
}

BOOL CTipbarWnd::CheckExcludeCaptionButtonMode(LPRECT prc1, LPCRECT prc2)
{
    return (prc1->top < prc2->top + 5) && (prc2->right <= prc1->right + (5 * m_ButtonWidth));
}

void CTipbarWnd::ClearLBItemList()
{
    m_LangBarItemList.Clear();
    if (m_pFocusThread)
        OnThreadItemChange(m_pFocusThread->m_dwThreadId);
}

HFONT CTipbarWnd::CreateVerticalFont()
{
    if (!m_hWnd)
        return NULL;

    CUIFTheme theme;
    theme.m_iPartId = 1;
    theme.m_iStateId = 0;
    theme.m_pszClassList = L"TOOLBAR";

    LOGFONTW lf;
    if (FAILED(theme.InternalOpenThemeData(m_hWnd)) ||
        FAILED(::GetThemeFont(theme.m_hTheme, NULL, theme.m_iPartId, 0, 210, &lf)))
    {
        ::GetObject(::GetStockObject(DEFAULT_GUI_FONT), sizeof(lf), &lf);
    }

    lf.lfEscapement = lf.lfOrientation = 2700;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;

    if (CheckEAFonts())
    {
        WCHAR szText[LF_FACESIZE];
        szText[0] = L'@';
        StringCchCopyW(&szText[1], _countof(szText) - 1, lf.lfFaceName);
        StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), szText);
    }

    return ::CreateFontIndirectW(&lf);
}

void CTipbarWnd::UpdateVerticalFont()
{
    if (m_dwTipbarWndFlags & TIPBAR_VERTICAL)
    {
        if (m_hTextFont)
        {
            ::DeleteObject(m_hTextFont);
            SetFontToThis(NULL);
            m_hTextFont = NULL;
        }
        m_hTextFont = CreateVerticalFont();
        SetFontToThis(m_hTextFont);
    }
    else
    {
        SetFontToThis(NULL);
    }
}

/// @unimplemented
void CTipbarWnd::ShowOverScreenSizeBalloon()
{
    //FIXME: CTipbarCtrlButtonHolder
}

void CTipbarWnd::DestroyOverScreenSizeBalloon()
{
    if (m_pBalloon)
    {
        if (::IsWindow(*m_pBalloon))
            ::DestroyWindow(*m_pBalloon);
        delete m_pBalloon;
        m_pBalloon = NULL;
    }
}

void CTipbarWnd::DestroyWnd()
{
    if (::IsWindow(m_hWnd))
        ::DestroyWindow(m_hWnd);
}

HKL CTipbarWnd::GetFocusKeyboardLayout()
{
    DWORD dwThreadId = 0;
    if (m_pFocusThread)
        dwThreadId = m_pFocusThread->m_dwThreadId;
    return ::GetKeyboardLayout(dwThreadId);
}

void CTipbarWnd::KillOnTheadItemChangeTimer()
{
    DWORD dwChangingThreadId = m_dwChangingThreadId;
    m_dwChangingThreadId = 0;
    KillTimer(4);

    if (dwChangingThreadId)
    {
        CTipbarThread *pThread = _FindThread(dwChangingThreadId);
        if (pThread)
            pThread->m_dwUnknown34 |= 0x1;
    }
}

UINT_PTR CTipbarWnd::SetTimer(UINT_PTR nIDEvent, UINT uElapse)
{
    if (::IsWindow(m_hWnd))
        return ::SetTimer(m_hWnd, nIDEvent, uElapse, NULL);
    return 0;
}

BOOL CTipbarWnd::KillTimer(UINT_PTR uIDEvent)
{
    if (::IsWindow(m_hWnd))
        return ::KillTimer(m_hWnd, uIDEvent);
    return FALSE;
}

/// @unimplemented
void CTipbarWnd::MoveToTray()
{
    if (!g_bEnableDeskBand || !(g_dwOSInfo & CIC_OSINFO_XPPLUS))
    {
        //FIXME
    }
}

void CTipbarWnd::MyClientToScreen(LPPOINT lpPoint, LPRECT prc)
{
    if (lpPoint)
        ::ClientToScreen(m_hWnd, lpPoint);

    if (prc)
    {
        ::ClientToScreen(m_hWnd, (LPPOINT)prc);
        ::ClientToScreen(m_hWnd, (LPPOINT)&prc->right);
    }
}

void CTipbarWnd::SavePosition()
{
    CicRegKey regKey;
    if (regKey.Create(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Microsoft\\CTF\\MSUTB\\")))
    {
        POINT pt = { 0, 0 };
        ::ClientToScreen(m_hWnd, &pt);
        regKey.SetDword(TEXT("Left"), pt.x);
        regKey.SetDword(TEXT("Top"), pt.y);
        regKey.SetDword(TEXT("Vertical"), !!(m_dwTipbarWndFlags & TIPBAR_VERTICAL));
    }
}

/// @unimplemented
void CTipbarWnd::SetAlpha(BYTE bAlpha, BOOL bFlag)
{
}

BOOL CTipbarWnd::SetLangBand(BOOL bDeskBand, BOOL bFlag2)
{
    if (bDeskBand == !!(m_dwShowType & TF_SFT_DESKBAND))
        return TRUE;

    BOOL ret = TRUE;
    HWND hwndTray = m_ShellWndThread.GetWndTray();
    if (bFlag2 && hwndTray)
    {
        DWORD_PTR dwResult;
        HWND hImeWnd = ::ImmGetDefaultIMEWnd(hwndTray);
        if (hImeWnd)
            ::SendMessageTimeout(hImeWnd, WM_IME_SYSTEM, 0x24 - bDeskBand, (LPARAM)hwndTray,
                                 (SMTO_BLOCK | SMTO_ABORTIFHUNG), 5000, &dwResult);
        else
            ::SendMessageTimeout(hwndTray, 0x505, 0, bDeskBand,
                                 (SMTO_BLOCK | SMTO_ABORTIFHUNG), 5000, &dwResult);
    }
    else
    {
        ret = FALSE;
    }

    if (!(m_dwTipbarWndFlags & TIPBAR_CHILD) && bDeskBand)
    {
        KillTimer(7);
        SetTimer(7, g_uTimerElapseSYSCOLORCHANGED);
    }

    return ret;
}

void CTipbarWnd::SetMoveRect(INT X, INT Y, INT nWidth, INT nHeight)
{
    if (m_dwTipbarWndFlags & TIPBAR_CHILD)
    {
        m_nWidth = nWidth;
        m_nHeight = nHeight;
        return;
    }

    ++m_bInCallOn;

    m_dwTipbarWndFlags |= TIPBAR_UPDATING;

    m_X = X;
    m_Y = Y;
    m_CX = nWidth;
    m_CY = nHeight;

    RECT rc;
    SIZE size = { 0, 0 };
    if (m_pWndFrame)
    {
        ::SetRect(&rc, 0, 0, nWidth - m_cxDlgFrameX2, nHeight - m_cyDlgFrameX2);
        m_pWndFrame->SetRect(&rc);
        m_pWndFrame->GetFrameSize(&size);
    }

    if (m_pTipbarGripper)
    {
        if (m_dwTipbarWndFlags & TIPBAR_VERTICAL)
        {
            INT GripperWidth = GetGripperWidth();
            ::SetRect(&rc, size.cx, size.cy, nWidth - m_cxDlgFrameX2 - size.cx, size.cy + GripperWidth);
        }
        else
        {
            INT GripperWidth = GetGripperWidth();
            INT y1 = nHeight - m_cyDlgFrameX2 - size.cy;
            ::SetRect(&rc, size.cx, size.cy, size.cx + GripperWidth, y1);
        }
        m_pTipbarGripper->SetRect(&rc);
    }

    --m_bInCallOn;
}

void CTipbarWnd::SetShowText(BOOL bShow)
{
    if (bShow)
        m_dwTipbarWndFlags |= TIPBAR_HIGHCONTRAST;
    else
        m_dwTipbarWndFlags &= ~TIPBAR_HIGHCONTRAST;

    if (m_pFocusThread)
        OnThreadItemChange(m_pFocusThread->m_dwThreadId);

    TerminateAllThreads(FALSE);
}

void CTipbarWnd::SetShowTrayIcon(BOOL bShow)
{
    if (m_dwTipbarWndFlags & TIPBAR_TRAYICON)
        m_dwTipbarWndFlags &= ~TIPBAR_TRAYICON;
    else
        m_dwTipbarWndFlags |= TIPBAR_TRAYICON;

    if ((m_dwTipbarWndFlags & TIPBAR_TRAYICON) && m_pFocusThread)
    {
        KillTimer(10);
        SetTimer(10, g_uTimerElapseMOVETOTRAY);
    }
    else if (g_pTrayIconWnd)
    {
        g_pTrayIconWnd->SetMainIcon(NULL);
        g_pTrayIconWnd->RemoveAllIcon(0);
    }
}

void CTipbarWnd::ShowContextMenu(POINT pt, LPCRECT prc, BOOL bFlag)
{
    AddRef();

    RECT rc;
    if (!prc)
    {
        rc = { pt.x, pt.y, pt.x, pt.y };
        prc = &rc;
    }

    if (m_pFocusThread)
    {
        CUTBContextMenu *pContextMenu = new(cicNoThrow) CUTBContextMenu(this);
        if (pContextMenu)
        {
            if (pContextMenu->Init())
            {
                m_pThread = m_pFocusThread;
                StartModalInput(this, m_pFocusThread->m_dwThreadId);

                m_pModalMenu = pContextMenu;
                DWORD dwCommandId = pContextMenu->ShowPopup(GetWindow(), pt, prc, bFlag);
                m_pModalMenu = NULL;

                if (m_pThread)
                    StopModalInput(m_pThread->m_dwThreadId);

                m_pThread = NULL;

                if (dwCommandId != (DWORD)-1)
                    pContextMenu->SelectMenuItem(dwCommandId);
            }

            delete pContextMenu;
        }
    }

    Release();
}

void CTipbarWnd::StartBackToAlphaTimer()
{
    UINT uTime = ::GetDoubleClickTime();
    ::SetTimer(m_hWnd, 3, 3 * uTime, NULL);
}

BOOL CTipbarWnd::StartDoAccDefaultActionTimer(CTipbarItem *pTarget)
{
    if (!m_pTipbarAccessible)
        return FALSE;
    INT IDOfItem = m_pTipbarAccessible->GetIDOfItem(pTarget);
    m_nID = IDOfItem;
    if (!IDOfItem || IDOfItem == -1)
        return FALSE;
    KillTimer(11);
    SetTimer(11, g_uTimerElapseDOACCDEFAULTACTION);
    return TRUE;
}

void CTipbarWnd::StartModalInput(ITfLangBarEventSink *pSink, DWORD dwThreadId)
{
    if (!m_pLangBarMgr)
        return;

    m_pLangBarMgr->SetModalInput(pSink, dwThreadId, 0);
    if (g_pTrayIconWnd)
        m_pLangBarMgr->SetModalInput(pSink, g_pTrayIconWnd->m_dwTrayWndThreadId, 0);

    DWORD dwCurThreadId = ::GetCurrentThreadId();
    m_pLangBarMgr->SetModalInput(pSink, dwCurThreadId, 1);
}

void CTipbarWnd::StopModalInput(DWORD dwThreadId)
{
    if (!m_pLangBarMgr)
        return;

    m_pLangBarMgr->SetModalInput(NULL, dwThreadId, 0);
    if (g_pTrayIconWnd)
        m_pLangBarMgr->SetModalInput(NULL, g_pTrayIconWnd->m_dwTrayWndThreadId, 0);

    DWORD dwCurThreadId = ::GetCurrentThreadId();
    m_pLangBarMgr->SetModalInput(NULL, dwCurThreadId, 0);
}

LONG MyWaitForInputIdle(DWORD dwThreadId, DWORD dwMilliseconds)
{
    if (g_pTipbarWnd && (g_pTipbarWnd->m_dwShowType & TF_SFT_DESKBAND))
        return 0;

    if (TF_IsInMarshaling(dwThreadId))
        return STATUS_TIMEOUT;

    DWORD dwFlags1 = 0, dwFlags2 = 0;
    if (!TF_GetThreadFlags(dwThreadId, &dwFlags1, &dwFlags2, NULL) && dwFlags2)
        return -1;

    return TF_CheckThreadInputIdle(dwThreadId, dwMilliseconds);
}

CTipbarThread *CTipbarWnd::_CreateThread(DWORD dwThreadId)
{
    CTipbarThread *pTarget = _FindThread(dwThreadId);
    if (pTarget)
        return pTarget;

    MyWaitForInputIdle(dwThreadId, 2000);

    pTarget = new(cicNoThrow) CTipbarThread(this);
    if (!pTarget)
        return NULL;

    AddThreadToThreadCreatingList(pTarget);

    HRESULT hr = pTarget->Init(dwThreadId);

    RemoveThredFromThreadCreatingList(pTarget);

    if (SUCCEEDED(hr) && !m_Threads.Add(pTarget))
    {
        pTarget->_UninitItemList(TRUE);
        pTarget->m_pTipbarWnd = NULL;
        pTarget->_Release();
        return NULL;
    }

    return pTarget;
}

CTipbarThread *CTipbarWnd::_FindThread(DWORD dwThreadId)
{
    if (g_bWinLogon)
        return NULL;

    CTipbarThread *pTarget = NULL;
    for (size_t iItem = 0; iItem < m_Threads.size(); ++iItem)
    {
        CTipbarThread *pThread = m_Threads[iItem];
        if (pThread && pThread->m_dwThreadId == dwThreadId)
        {
            pTarget = pThread;
            break;
        }
    }

    if (!pTarget)
        return NULL;

    DWORD dwFlags1, dwFlags2, dwFlags3;
    TF_GetThreadFlags(dwThreadId, &dwFlags1, &dwFlags2, &dwFlags3);

    if (!dwFlags2 || (dwFlags2 != pTarget->m_dwFlags2) || (dwFlags3 != pTarget->m_dwFlags3))
    {
        OnThreadTerminateInternal(dwThreadId);
        return NULL;
    }

    return pTarget;
}

void CTipbarWnd::EnsureFocusThread()
{
    if (m_pFocusThread || (m_dwTipbarWndFlags & (TIPBAR_TOOLBARENDED | TIPBAR_ENSURING)))
        return;

    m_dwTipbarWndFlags |= TIPBAR_ENSURING;

    HWND hwndFore = ::GetForegroundWindow();
    if (!hwndFore)
        return;

    DWORD dwThreadId = ::GetWindowThreadProcessId(hwndFore, NULL);
    if (dwThreadId)
        OnSetFocus(dwThreadId);

    m_dwTipbarWndFlags &= ~TIPBAR_ENSURING;
}

HRESULT CTipbarWnd::SetFocusThread(CTipbarThread *pFocusThread)
{
    if (pFocusThread == m_pFocusThread)
        return S_OK;

    DWORD dwThreadId = ::GetCurrentThreadId();
    DestroyOverScreenSizeBalloon();

    if (m_pFocusThread)
    {
        m_pFocusThread->SetFocus(NULL);
        ::AttachThreadInput(dwThreadId, m_pFocusThread->m_dwThreadId, FALSE);
    }

    m_dwTipbarWndFlags &= ~TIPBAR_ATTACHED;
    m_pFocusThread = pFocusThread;
    return S_OK;
}

HRESULT CTipbarWnd::AttachFocusThread()
{
    if (m_dwTipbarWndFlags & TIPBAR_ATTACHED)
        return S_FALSE;

    if (m_pFocusThread)
    {
        DWORD dwThreadId = ::GetCurrentThreadId();
        ::AttachThreadInput(dwThreadId, m_pFocusThread->m_dwThreadId, TRUE);
        m_dwTipbarWndFlags |= TIPBAR_ATTACHED;
    }

    return S_OK;
}

void CTipbarWnd::RestoreLastFocus(DWORD *pdwThreadId, BOOL fPrev)
{
    if (m_pLangBarMgr)
        m_pLangBarMgr->RestoreLastFocus(pdwThreadId, fPrev);
}

void CTipbarWnd::CleanUpThreadPointer(CTipbarThread *pThread, BOOL bRemove)
{
    if (bRemove)
    {
        ssize_t iItem = m_Threads.Find(pThread);
        if (iItem >= 0)
            m_Threads.Remove(iItem);
    }

    if (pThread == m_pFocusThread)
        SetFocusThread(NULL);

    if (pThread == m_pThread)
        m_pThread = NULL;

    if (pThread == m_pUnknownThread)
        m_pUnknownThread = NULL;
}

void CTipbarWnd::TerminateAllThreads(BOOL bFlag)
{
    const size_t cItems = m_Threads.size();

    DWORD *pdwThreadIds = new(cicNoThrow) DWORD[cItems];
    if (!pdwThreadIds)
        return;

    for (size_t iItem = 0; iItem < cItems; ++iItem)
    {
        pdwThreadIds[iItem] = 0;
        CTipbarThread* pThread = m_Threads[iItem];
        if (pThread && (bFlag || (pThread != m_pFocusThread)))
        {
            pdwThreadIds[iItem] = pThread->m_dwThreadId;
        }
    }

    for (size_t iItem = 0; iItem < cItems; ++iItem)
    {
        if (pdwThreadIds[iItem])
            OnThreadTerminateInternal(pdwThreadIds[iItem]);
    }

    delete[] pdwThreadIds;
}

STDMETHODIMP CTipbarWnd::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB c_tab[] =
    {
        QITABENT(CTipbarWnd, ITfLangBarEventSink),
        QITABENT(CTipbarWnd, ITfLangBarEventSink_P),
        { NULL }
    };
    return ::QISearch(this, c_tab, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CTipbarWnd::AddRef()
{
    return ++m_cRefs;
}

STDMETHODIMP_(ULONG) CTipbarWnd::Release()
{
    if (--m_cRefs == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @unimplemented
STDMETHODIMP CTipbarWnd::OnSetFocus(DWORD dwThreadId)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTipbarWnd::OnThreadTerminate(DWORD dwThreadId)
{
    HRESULT hr;
    ++m_bInCallOn;
    AddRef();
    {
        hr = OnThreadTerminateInternal(dwThreadId);
        if (!m_pFocusThread)
            EnsureFocusThread();
    }
    --m_bInCallOn;
    Release();
    return hr;
}

HRESULT CTipbarWnd::OnThreadTerminateInternal(DWORD dwThreadId)
{
    for (size_t iItem = 0; iItem < m_Threads.size(); ++iItem)
    {
        CTipbarThread *pThread = m_Threads[iItem];
        if (pThread && pThread->m_dwThreadId == dwThreadId)
        {
            m_Threads.Remove(iItem);
            pThread->RemoveUIObjs();
            CleanUpThreadPointer(pThread, FALSE);
            pThread->_UninitItemList(TRUE);
            pThread->m_pTipbarWnd = NULL;
            pThread->_Release();
            break;
        }
    }

    return S_OK;
}

STDMETHODIMP CTipbarWnd::OnThreadItemChange(DWORD dwThreadId)
{
    if (m_dwTipbarWndFlags & TIPBAR_TOOLBARENDED)
        return S_OK;
    if (!(m_dwTipbarWndFlags & TIPBAR_CHILD) && (m_dwShowType & TF_SFT_DESKBAND))
        return S_OK;

    CTipbarThread *pThread = _FindThread(dwThreadId);
    if (pThread)
    {
        if ((!m_dwUnknown23 || m_dwUnknown23 == dwThreadId) && pThread == m_pFocusThread)
        {
            KillOnTheadItemChangeTimer();
            m_dwChangingThreadId = dwThreadId;
            KillTimer(6);
            SetTimer(4, g_uTimerElapseONTHREADITEMCHANGE);
        }
        else
        {
            pThread->m_dwUnknown34 |= 0x1;
        }
    }
    else
    {
        for (size_t iItem = 0; iItem < m_ThreadCreatingList.size(); ++iItem)
        {
            CTipbarThread *pItem = m_ThreadCreatingList[iItem];
            if (pItem && pItem->m_dwThreadId == dwThreadId)
            {
                pItem->m_dwUnknown34 |= 0x1;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CTipbarWnd::OnModalInput(DWORD dwThreadId, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_NCLBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
        case WM_NCMBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            break;

        case WM_NCLBUTTONUP:
        case WM_NCRBUTTONUP:
        case WM_NCMBUTTONUP:
            if (m_pThread)
            {
                CUTBMenuWnd *pMenuUI = m_pModalMenu->m_pMenuUI;
                if (pMenuUI)
                {
                    HWND hWnd = *pMenuUI;
                    if (hWnd)
                    {
                        POINT pt = { (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam) };
                        ::ScreenToClient(hWnd, &pt);
                        uMsg += WM_LBUTTONUP - WM_NCLBUTTONUP;
                        ::PostMessage(m_hWnd, uMsg, wParam, MAKELPARAM(pt.x, pt.y));
                    }
                }
            }
            break;

        default:
        {
            if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP)
            {
                if (m_pThread)
                    m_pModalMenu->PostKey(uMsg == WM_KEYUP, wParam, lParam);
            }
            else
            {
                CancelMenu();
            }
            break;
        }
    }

    return 0;
}

/// @unimplemented
STDMETHODIMP CTipbarWnd::ShowFloating(DWORD dwFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CTipbarWnd::GetItemFloatingRect(DWORD dwThreadId, REFGUID rguid, RECT *prc)
{
    if (m_dwTipbarWndFlags & TIPBAR_TRAYICON)
        return E_UNEXPECTED;

    if (!m_pFocusThread || (m_pFocusThread->m_dwThreadId != dwThreadId))
        return E_FAIL;

    for (size_t iItem = 0; iItem < m_pFocusThread->m_UIObjects.size(); ++iItem)
    {
        CTipbarItem* pItem = m_pFocusThread->m_UIObjects[iItem];
        if (pItem)
        {
            if ((pItem->m_dwItemFlags & 0x8) && IsEqualGUID(pItem->m_ItemInfo.guidItem, rguid))
            {
                pItem->OnUnknown57(prc);
                return S_OK;
            }
        }
    }

    return E_FAIL;
}

/// @unimplemented
STDMETHODIMP CTipbarWnd::OnLangBarUpdate(TfLBIClick click, BOOL bFlag)
{
    return E_NOTIMPL;
}

STDMETHODIMP_(BSTR) CTipbarWnd::GetAccName()
{
    WCHAR szText[256];
    ::LoadStringW(g_hInst, IDS_LANGUAGEBAR, szText, _countof(szText));
    return ::SysAllocString(szText);
}

STDMETHODIMP_(void) CTipbarWnd::GetAccLocation(LPRECT lprc)
{
    GetRect(lprc);
}

STDMETHODIMP_(void) CTipbarWnd::PaintObject(HDC hDC, LPCRECT prc)
{
    if (m_dwTipbarWndFlags & TIPBAR_UPDATING)
    {
        Move(m_X, m_Y, m_CX, m_CY);
        m_dwTipbarWndFlags &= ~TIPBAR_UPDATING;
    }

    if (!m_pFocusThread || !m_pFocusThread->IsDirtyItem())
    {
        m_pFocusThread->CallOnUpdateHandler();
        if (g_pTipbarWnd)
            CUIFWindow::PaintObject(hDC, prc);
    }
}

STDMETHODIMP_(DWORD) CTipbarWnd::GetWndStyle()
{
    return CUIFWindow::GetWndStyle() & ~WS_BORDER;
}

STDMETHODIMP_(void) CTipbarWnd::Move(INT x, INT y, INT nWidth, INT nHeight)
{
    CUIFWindow::Move(x, y, nWidth, nHeight);
}

STDMETHODIMP_(void) CTipbarWnd::OnMouseOutFromWindow(LONG x, LONG y)
{
    StartBackToAlphaTimer();
    if ((m_dwTipbarWndFlags & 0x40) && (m_dwTipbarWndFlags & 0x80))
        SetTimer(2, g_uTimerElapseSTUBEND);
}

/// @unimplemented
STDMETHODIMP_(void) CTipbarWnd::OnCreate(HWND hWnd)
{
}

STDMETHODIMP_(void) CTipbarWnd::OnDestroy(HWND hWnd)
{
    CancelMenu();

    if (m_pTipbarAccessible)
        m_pTipbarAccessible->NotifyWinEvent(EVENT_OBJECT_DESTROY, GetAccItem());

    OnTerminateToolbar();
    if (m_pTipbarAccessible)
    {
        m_pTipbarAccessible->ClearAccItems();
        m_pTipbarAccessible->Release();
        m_pTipbarAccessible = NULL;
    }

    m_coInit.CoUninit();

    if (m_pLangBarMgr)
        m_pLangBarMgr->UnAdviseEventSink(m_dwSinkCookie);
}

/// @unimplemented
STDMETHODIMP_(void) CTipbarWnd::OnTimer(WPARAM wParam)
{
    AddRef();
    switch (wParam)
    {
        case 1:
            KillTimer(1);
            MoveToStub(FALSE);
            break;
        case 2:
            KillTimer(2);
            MoveToStub(TRUE);
            break;
        case 3:
            KillTimer(3);
            SetAlpha((BYTE)m_dwAlphaValue, TRUE);
            break;
        case 4:
        {
            LONG status = MyWaitForInputIdle(m_dwChangingThreadId, 2000);
            if (status)
            {
                if (status != STATUS_TIMEOUT)
                {
                    KillTimer(4);
                    m_dwChangingThreadId = 0;
                }
            }
            else if (!m_pThread)
            {
                KillTimer(4);
                DWORD dwOldThreadId = m_dwChangingThreadId;
                m_dwChangingThreadId = 0;
                OnThreadItemChangeInternal(dwOldThreadId);
            }
            break;
        }
        case 5:
            KillTimer(5);
            ::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
            break;
        case 6:
            KillTimer(6);
            if (m_pFocusThread)
            {
                if (m_pFocusThread->m_dwThreadId != m_dwChangingThreadId &&
                    !m_pFocusThread->CallOnUpdateHandler())
                {
                    if (m_pFocusThread)
                        OnThreadItemChange(m_pFocusThread->m_dwThreadId);
                }
            }
            break;
        case 7:
        {
            DWORD dwThreadId = 0;
            if (KillTimer(7))
            {
                if (m_pFocusThread)
                    dwThreadId = m_pFocusThread->m_dwThreadId;

                TerminateAllThreads(TRUE);
                UpdateVerticalFont();

                if (dwThreadId)
                    OnSetFocus(dwThreadId);

                InitMetrics();
                // FIXME: CTipbarCtrlButtonHolder

                InitHighContrast();
                SetAlpha(0xFF, TRUE);
                ::RedrawWindow(m_hWnd, NULL, NULL, (RDW_FRAME | RDW_UPDATENOW | RDW_INVALIDATE));
            }
            break;
        }
        case 8:
            KillTimer(8);
            UpdateUI(NULL);
            break;
        case 9:
            KillTimer(9);
            //FIXME
            if (m_pUnknownThread == m_pFocusThread)
                Show(!!(m_dwUnknown23_5[3] & 0x80000000));
            m_pUnknownThread = NULL;
            if ((m_dwUnknown23_5[3] & 0x2))
                ShowOverScreenSizeBalloon();
            break;
        case 10:
            KillTimer(10);
            MoveToTray();
            break;
        case 11:
            KillTimer(11);
            if (m_pTipbarAccessible)
            {
                if (m_nID)
                {
                    m_pTipbarAccessible->DoDefaultActionReal(m_nID);
                    m_nID = 0;
                }
            }
            break;
        case 12:
            KillTimer(12);
            AdjustPosOnDisplayChange();
            break;
        case 13:
#ifdef ENABLE_DESKBAND
            if (!m_pDeskBand || !m_pDeskBand->m_dwUnknown19)
            {
                KillTimer(13);
                if (!m_pFocusThread)
                EnsureFocusThread();
            }
#endif
            break;
        case 14:
            if (SetLangBand(TRUE, TRUE))
            {
                m_dwShowType = TF_SFT_DESKBAND;
                KillTimer(14);
            }
            break;
        default:
        {
            if (10000 <= wParam && wParam < 10050)
            {
                auto *pItem = m_LangBarItemList.GetItemStateFromTimerId(wParam);
                if (pItem)
                {
                    auto& clsid = pItem->m_clsid;
                    m_LangBarItemList.SetDemoteLevel(pItem->m_clsid, 2);
                    if (m_pFocusThread)
                    {
                        auto *pThreadItem = m_pFocusThread->GetItem(clsid);
                        if (pThreadItem)
                            pThreadItem->AddRemoveMeToUI(FALSE);
                    }
                }
            }
            break;
        }
    }

    Release();
}

STDMETHODIMP_(void) CTipbarWnd::OnSysColorChange()
{
    KillTimer(7);
    SetTimer(7, g_uTimerElapseSYSCOLORCHANGED);
}

void CTipbarWnd::OnTerminateToolbar()
{
    m_dwTipbarWndFlags |= TIPBAR_TOOLBARENDED;
    DestroyOverScreenSizeBalloon();
    TerminateAllThreads(TRUE);
    if (!(m_dwTipbarWndFlags & TIPBAR_CHILD))
        SavePosition();
}

STDMETHODIMP_(void) CTipbarWnd::OnEndSession(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    if (!g_bWinLogon)
        OnTerminateToolbar();

    if (wParam) // End session?
    {
        if (lParam & ENDSESSION_LOGOFF)
        {
            KillTimer(9);
            Show(FALSE);
        }
        else
        {
            OnTerminateToolbar();

            AddRef();
            ::DestroyWindow(hWnd);
            Release();
        }
    }
}

STDMETHODIMP_(void) CTipbarWnd::OnUser(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_USER + 1)
    {
        POINT pt = { (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam) };
        ::ClientToScreen(m_hWnd, &pt);
        ShowContextMenu(pt, NULL, TRUE);
    }
    else if (uMsg == g_wmTaskbarCreated)
    {
        m_ShellWndThread.clear();
    }
}

STDMETHODIMP_(LRESULT)
CTipbarWnd::OnWindowPosChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (m_pFocusThread)
    {
        for (size_t iItem = 0; iItem < m_pFocusThread->m_UIObjects.size(); ++iItem)
        {
            CTipbarItem *pItem = m_pFocusThread->m_UIObjects[iItem];
            if (pItem)
                pItem->OnUnknown44();
        }
    }

    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

STDMETHODIMP_(LRESULT)
CTipbarWnd::OnWindowPosChanging(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPWINDOWPOS pWP = (LPWINDOWPOS)lParam;
    if (!(pWP->flags & SWP_NOZORDER))
    {
        if (!m_pThread && (!m_pToolTip || !m_pToolTip->m_bShowToolTip))
            pWP->hwndInsertAfter = NULL;
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

STDMETHODIMP_(LRESULT)
CTipbarWnd::OnShowWindow(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (m_pTipbarAccessible)
    {
        if (wParam) // Show?
        {
            m_pTipbarAccessible->NotifyWinEvent(EVENT_OBJECT_SHOW, GetAccItem());
            m_pTipbarAccessible->NotifyWinEvent(EVENT_OBJECT_FOCUS, GetAccItem());
        }
        else
        {
            m_pTipbarAccessible->NotifyWinEvent(EVENT_OBJECT_HIDE, GetAccItem());
        }
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

STDMETHODIMP_(LRESULT)
CTipbarWnd::OnSettingChange(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!wParam || wParam == SPI_SETNONCLIENTMETRICS || wParam == SPI_SETHIGHCONTRAST)
    {
        KillTimer(7);
        SetTimer(7, g_uTimerElapseSYSCOLORCHANGED);
    }
    return 0;
}

STDMETHODIMP_(LRESULT)
CTipbarWnd::OnDisplayChange(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!(m_dwTipbarWndFlags & TIPBAR_CHILD))
    {
        KillTimer(12);
        SetTimer(12, g_uTimerElapseDISPLAYCHANGE);
    }
    return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

STDMETHODIMP_(HRESULT)
CTipbarWnd::OnGetObject(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (lParam != -4)
        return S_OK;
    if (!m_pTipbarAccessible)
        return E_OUTOFMEMORY;

    if (m_pTipbarAccessible->m_bInitialized)
        return m_pTipbarAccessible->CreateRefToAccObj(wParam);

    HRESULT hr = S_OK;
    if (SUCCEEDED(m_coInit.EnsureCoInit()))
    {
        hr = m_pTipbarAccessible->Initialize();
        if (FAILED(hr))
        {
            m_pTipbarAccessible->Release();
            m_pTipbarAccessible = NULL;
            return hr;
        }

        m_pTipbarAccessible->NotifyWinEvent(EVENT_OBJECT_CREATE, GetAccItem());
        return m_pTipbarAccessible->CreateRefToAccObj(wParam);
    }

    return hr;
}

STDMETHODIMP_(BOOL) CTipbarWnd::OnEraseBkGnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return TRUE;
}

STDMETHODIMP_(void) CTipbarWnd::OnThemeChanged(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    CUIFWindow::OnThemeChanged(hWnd, wParam, lParam);
}

STDMETHODIMP_(void) CTipbarWnd::UpdateUI(LPCRECT prc)
{
    KillTimer(8);

    if (m_dwChangingThreadId || m_bInCallOn || (m_pFocusThread && m_pFocusThread->IsDirtyItem()))
    {
        SetTimer(8, g_uTimerElapseUPDATEUI);
        return;
    }

    if (m_dwTipbarWndFlags & TIPBAR_UPDATING)
    {
        ++m_bInCallOn;
        Move(m_X, m_Y, m_CX, m_CY);
        m_dwTipbarWndFlags &= ~TIPBAR_UPDATING;
        --m_bInCallOn;
    }

    CUIFWindow::UpdateUI(NULL);
}

/// @unimplemented
STDMETHODIMP_(void) CTipbarWnd::HandleMouseMsg(UINT uMsg, LONG x, LONG y)
{
}

/***********************************************************************
 * CTipbarThread
 */

CTipbarThread::CTipbarThread(CTipbarWnd *pTipbarWnd)
{
    m_pTipbarWnd = pTipbarWnd;
    m_dwThreadId = 0;
    m_pLangBarItemMgr = NULL;
    m_cRefs = 1;
}

CTipbarThread::~CTipbarThread()
{
    if (m_pTipbarWnd)
    {
        RemoveUIObjs();
        m_pTipbarWnd->CleanUpThreadPointer(this, 1);
    }

    _UninitItemList(1);

    if (m_pLangBarItemMgr)
    {
        m_pLangBarItemMgr->Release();
        m_pLangBarItemMgr = NULL;
    }
}

HRESULT CTipbarThread::Init(DWORD dwThreadId)
{
    m_dwThreadId = dwThreadId;
    if (!TF_GetThreadFlags(dwThreadId, &m_dwFlags1, &m_dwFlags2, &m_dwFlags3))
        return E_FAIL;
    if (m_dwFlags1 & 0x8)
        return S_OK;
    return m_pTipbarWnd->m_pLangBarMgr->GetThreadLangBarItemMgr(m_dwThreadId,
                                                                &m_pLangBarItemMgr,
                                                                &dwThreadId);
}

/// @unimplemented
HRESULT CTipbarThread::InitItemList()
{
    return E_NOTIMPL;
}

HRESULT CTipbarThread::_UninitItemList(BOOL bUnAdvise)
{
    for (size_t iItem = 0; iItem < m_UIObjects.size(); ++iItem)
    {
        CTipbarItem* pItem = m_UIObjects[iItem];
        if (pItem)
            pItem->m_dwItemFlags |= 0x10;
    }

    HRESULT hr = S_OK;
    if (bUnAdvise)
    {
        if (m_dwThreadId == ::GetCurrentThreadId() || !MyWaitForInputIdle(m_dwThreadId, 2000))
            hr = _UnadviseItemsSink();
    }

    for (size_t iItem = 0; iItem < m_UIObjects.size(); ++iItem)
    {
        CTipbarItem* pItem = m_UIObjects[iItem];
        if (pItem)
        {
            if (m_pTipbarWnd)
                pItem->OnUnknown47(m_pTipbarWnd->GetWindow());

            pItem->ClearConnections();

            if (m_pTipbarWnd)
                pItem->OnUnknown50();
            else
                pItem->OnUnknown51();

            pItem->OnUnknown59();
            pItem->OnUnknown42();
        }
    }

    m_UIObjects.clear();

    RemoveAllSeparators();

    return hr;
}

void CTipbarThread::AddAllSeparators()
{
    for (size_t iItem = 0; iItem < m_Separators.size(); ++iItem)
    {
        CUIFObject *pItem = m_Separators[iItem];
        if (pItem)
            m_pTipbarWnd->AddUIObj(pItem);
    }
}

void CTipbarThread::RemoveAllSeparators()
{
    for (size_t iItem = 0; iItem < m_Separators.size(); ++iItem)
    {
        CUIFObject *pItem = m_Separators[iItem];
        if (pItem)
        {
            if (m_pTipbarWnd)
                m_pTipbarWnd->RemoveUIObj(pItem);
            delete pItem;
        }
    }
    m_Separators.clear();
}

void CTipbarThread::AddUIObjs()
{
    _AddRef();

    for (size_t iItem = 0; iItem < m_UIObjects.size(); ++iItem)
    {
        CTipbarItem* pItem = m_UIObjects[iItem];
        if (pItem && (pItem->m_dwItemFlags & 0x8))
        {
            pItem->OnUnknown46(m_pTipbarWnd ? m_pTipbarWnd->GetWindow() : NULL);
        }
    }

    AddAllSeparators();
    MyMoveWnd(0, 0);

    _Release();
}

void CTipbarThread::RemoveUIObjs()
{
    for (size_t iItem = 0; iItem < m_UIObjects.size(); ++iItem)
    {
        CTipbarItem* pItem = m_UIObjects[iItem];
        if (pItem)
        {
            pItem->OnUnknown47(m_pTipbarWnd ? m_pTipbarWnd->GetWindow() : NULL);
        }
    }
    RemoveAllSeparators();
}

CTipbarItem *CTipbarThread::GetItem(REFCLSID rclsid)
{
    for (size_t iItem = 0; iItem < m_UIObjects.size(); ++iItem)
    {
        auto *pItem = m_UIObjects[iItem];
        if (pItem && IsEqualCLSID(pItem->m_ItemInfo.guidItem, rclsid))
            return pItem;
    }
    return NULL;
}

void CTipbarThread::GetTextSize(BSTR bstr, LPSIZE pSize)
{
    HWND hWnd = *m_pTipbarWnd->GetWindow();

    HGDIOBJ hFontOld = NULL;

    HDC hDC = ::GetDC(hWnd);
    if (FAILED(m_pTipbarWnd->EnsureThemeData(*m_pTipbarWnd->GetWindow())))
    {
        HFONT hFont = m_pTipbarWnd->m_hFont;
        if (hFont)
            hFontOld = ::SelectObject(hDC, hFont);
        INT cch = ::SysStringLen(bstr);
        ::GetTextExtentPoint32W(hDC, bstr, cch, pSize);
        if (hFontOld)
            ::SelectObject(hDC, hFontOld);
    }
    else
    {
        CUIFTheme theme;
        theme.m_iPartId = 1;
        theme.m_iStateId = 0;
        theme.m_pszClassList = L"TOOLBAR";

        HFONT hFont = NULL;

        if (SUCCEEDED(theme.InternalOpenThemeData(hWnd)))
        {
            LOGFONTW lf;
            if (SUCCEEDED(::GetThemeFont(theme.m_hTheme, NULL, theme.m_iPartId, 0, 210, &lf)))
            {
                hFont = ::CreateFontIndirectW(&lf);
                if (hFont)
                    hFontOld = ::SelectObject(hDC, hFont);
            }

            RECT rc;
            INT cch = ::SysStringLen(bstr);
            ::GetThemeTextExtent(theme.m_hTheme, hDC, theme.m_iPartId, 0, bstr, cch, 0, NULL, &rc);

            pSize->cx = rc.right;
            pSize->cy = rc.bottom;
        }

        if (hFontOld)
            ::SelectObject(hDC, hFontOld);
        if (hFont)
            ::DeleteObject(hFont);
    }

    ::ReleaseDC(hWnd, hDC);
}

DWORD CTipbarThread::IsDirtyItem()
{
    DWORD dwDirty = 0;
    for (size_t iItem = 0; iItem < m_UIObjects.size(); ++iItem)
    {
        CTipbarItem* pItem = m_UIObjects[iItem];
        if (pItem)
            dwDirty |= pItem->m_dwDirty;
    }
    return dwDirty;
}

BOOL CTipbarThread::IsFocusThread()
{
    if (!m_pTipbarWnd)
        return FALSE;
    return this == m_pTipbarWnd->m_pFocusThread;
}

BOOL CTipbarThread::IsVertical()
{
    if (!m_pTipbarWnd)
        return FALSE;
    return !!(m_pTipbarWnd->m_dwTipbarWndFlags & TIPBAR_VERTICAL);
}

/// @unimplemented
void CTipbarThread::LocateItems()
{
}

LONG CTipbarThread::_Release()
{
    if (--m_cRefs == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

HRESULT CTipbarThread::_UnadviseItemsSink()
{
    if (!m_pLangBarItemMgr)
        return E_FAIL;

    DWORD *pdwCoolkies = new(cicNoThrow) DWORD[m_UIObjects.size()];
    if (!pdwCoolkies)
        return E_OUTOFMEMORY;

    const size_t cItems = m_UIObjects.size();
    for (size_t iItem = 0; iItem < cItems; ++iItem)
    {
        CTipbarItem* pItem = m_UIObjects[iItem];
        if (pItem)
            pdwCoolkies[iItem] = pItem->m_dwCookie;
    }

    HRESULT hr = m_pLangBarItemMgr->UnadviseItemsSink((LONG)cItems, pdwCoolkies);

    delete[] pdwCoolkies;

    return hr;
}

void CTipbarThread::MyMoveWnd(LONG xDelta, LONG yDelta)
{
    if (!m_pTipbarWnd || (m_pTipbarWnd->m_pFocusThread != this))
        return;

    RECT Rect, rcWorkArea;
    m_pTipbarWnd->GetRect(&Rect);
    POINT pt = { Rect.left, Rect.top };
    cicGetWorkAreaRect(pt, &rcWorkArea);

    ::GetWindowRect(*m_pTipbarWnd->GetWindow(), &Rect);

    LONG X0 = Rect.left + xDelta, Y0 = Rect.top + yDelta;
    if (m_pTipbarWnd->m_dwTipbarWndFlags & 0x1000)
    {
        if (m_pTipbarWnd->CheckExcludeCaptionButtonMode(&Rect, &rcWorkArea))
        {
            X0 = rcWorkArea.right - (3 * m_pTipbarWnd->m_ButtonWidth +
                                     m_pTipbarWnd->m_cxDlgFrameX2 + m_cxGrip);
            Y0 = 0;
        }
        else
        {
            m_pTipbarWnd->m_dwTipbarWndFlags &= ~0x1000;
        }
    }

    if (IsVertical())
    {
        LONG width = m_pTipbarWnd->m_cxDlgFrameX2 + m_pTipbarWnd->GetTipbarHeight();
        LONG height = m_cyGrip + m_pTipbarWnd->m_cyDlgFrameX2;
        m_pTipbarWnd->SetMoveRect(X0, Y0, width, height);
    }
    else
    {
        LONG width = m_cxGrip + m_pTipbarWnd->m_cxDlgFrameX2;
        LONG height = m_pTipbarWnd->m_cyDlgFrameX2 + m_pTipbarWnd->GetTipbarHeight();
        m_pTipbarWnd->SetMoveRect(X0, Y0, width, height);
    }

    SIZE frameSize = { 0, 0 };
    if (m_pTipbarWnd->m_pWndFrame)
        m_pTipbarWnd->m_pWndFrame->GetFrameSize(&frameSize);

    m_pTipbarWnd->LocateCtrlButtons();
    m_pTipbarWnd->AutoAdjustDeskBandSize();
}

/***********************************************************************
 * CTipbarItem
 */

CTipbarItem::CTipbarItem(
    CTipbarThread *pThread,
    ITfLangBarItem *pLangBarItem,
    TF_LANGBARITEMINFO *pItemInfo,
    DWORD dwUnknown16)
{
    m_dwUnknown19[1] = 0;
    m_dwUnknown19[2] = 0;
    m_dwUnknown19[3] = 0;
    m_pTipbarThread = pThread;
    m_ItemInfo = *pItemInfo;
    m_pLangBarItem = pLangBarItem;
    m_pLangBarItem->AddRef();
    m_dwItemFlags = 0;
    m_dwUnknown16 = dwUnknown16;
    m_dwDirty = 0x1001F;
}

CTipbarItem::~CTipbarItem()
{
    if (g_pTipbarWnd)
    {
        if (g_pTipbarWnd->m_pTipbarAccessible)
            g_pTipbarWnd->m_pTipbarAccessible->RemoveAccItem(this);
    }

    if (m_pLangBarItem)
        m_pLangBarItem->Release();
}

void CTipbarItem::_AddedToUI()
{
    if (!IsConnected())
        return;

    OnUnknown41();

    m_dwItemFlags |= 0x2;

    DWORD dwStatus;
    if (m_dwDirty)
    {
        if (m_dwDirty & 0x10000)
            m_pLangBarItem->GetStatus(&dwStatus);
        else
            dwStatus = 0;
        OnUnknown45(m_dwDirty, dwStatus);
        m_dwDirty = 0;
    }

    if (m_pTipbarThread)
    {
        CTipbarWnd *pTipbarWnd = m_pTipbarThread->m_pTipbarWnd;
        if (pTipbarWnd)
        {
            CTipbarAccessible *pTipbarAccessible = pTipbarWnd->m_pTipbarAccessible;
            if (pTipbarAccessible)
                pTipbarAccessible->AddAccItem(this);
        }
    }

    OnUnknown42();
}

void CTipbarItem::_RemovedToUI()
{
    m_dwItemFlags &= ~0x2;

    if (g_pTipbarWnd)
    {
        CTipbarAccessible *pAccessible = g_pTipbarWnd->m_pTipbarAccessible;
        if (pAccessible)
            pAccessible->RemoveAccItem(this);
    }
}

void CTipbarItem::AddRemoveMeToUI(BOOL bFlag)
{
    if (!IsConnected())
        return;

    m_pTipbarThread->LocateItems();

    if (!IsConnected())
        return;

    m_pTipbarThread->AddAllSeparators();

    CTipbarWnd *pTipbarWnd = m_pTipbarThread->m_pTipbarWnd;
    if (bFlag)
        OnUnknown46(pTipbarWnd ? pTipbarWnd->GetWindow() : NULL);
    else
        OnUnknown47(pTipbarWnd ? pTipbarWnd->GetWindow() : NULL);
}

BOOL CTipbarItem::IsConnected()
{
    return (!(m_dwItemFlags & 0x10) && m_pTipbarThread && m_pTipbarThread->m_pTipbarWnd &&
            m_pLangBarItem);
}

void CTipbarItem::ClearConnections()
{
    m_pTipbarThread = NULL;
    if (m_pLangBarItem)
    {
        m_pLangBarItem->Release();
        m_pLangBarItem = NULL;
    }
}

/// @unimplemented
void CTipbarItem::StartDemotingTimer(BOOL bStarted)
{
    if (!g_bIntelliSense)
        return;

    if (!m_pTipbarThread)
        return;

    CTipbarWnd *pTipbarWnd = m_pTipbarThread->m_pTipbarWnd;
    if (!pTipbarWnd)
        return;

    //FIXME
}

STDMETHODIMP_(BOOL) CTipbarItem::DoAccDefaultAction()
{
    if (!m_pTipbarThread)
        return FALSE;
    CTipbarWnd *pTipbarWnd = m_pTipbarThread->m_pTipbarWnd;
    if (!pTipbarWnd)
        return FALSE;
    pTipbarWnd->StartDoAccDefaultActionTimer(this);
    return TRUE;
}

/// @unimplemented
STDMETHODIMP_(void) CTipbarItem::OnUpdateHandler(ULONG, ULONG)
{
}

STDMETHODIMP_(void) CTipbarItem::GetAccLocation(LPRECT prc)
{
    OnUnknown57(prc);
}

STDMETHODIMP_(BSTR) CTipbarItem::GetAccName()
{
    return ::SysAllocString(m_ItemInfo.szDescription);
}

STDMETHODIMP_(LPCWSTR) CTipbarItem::GetToolTip()
{
    OnUnknown41();

    if (!(m_dwItemFlags & 0x1))
    {
        m_dwItemFlags |= 0x1;

        BSTR bstrString;
        if (FAILED(m_pLangBarItem->GetTooltipString(&bstrString)))
            return NULL;

        if (bstrString)
        {
            OnUnknown53(bstrString);
            ::SysFreeString(bstrString);
        }
    }

    LPCWSTR pszToolTip = OnUnknown55();

    OnUnknown42();

    return pszToolTip;
}

HRESULT CTipbarItem::OnUpdate(DWORD dwDirty)
{
    if (!IsConnected())
        return S_OK;

    m_dwDirty |= dwDirty;
    m_dwItemFlags |= 0x20;

    if ((dwDirty & 0x10000) || (m_dwItemFlags & 0x6))
    {
        if (m_pTipbarThread)
        {
            CTipbarWnd *pTipBarWnd = m_pTipbarThread->m_pTipbarWnd;
            if (pTipBarWnd && *pTipBarWnd)
            {
                pTipBarWnd->KillTimer(6);
                pTipBarWnd->SetTimer(6, g_uTimerElapseONUPDATECALLED);
            }
        }
    }

    return S_OK;
}

void CTipbarItem::MyClientToScreen(LPPOINT ppt, LPRECT prc)
{
    if (!m_pTipbarThread)
        return;
    if (m_pTipbarThread->m_pTipbarWnd)
        m_pTipbarThread->m_pTipbarWnd->MyClientToScreen(ppt, prc);
}

/***********************************************************************
 *              GetTipbarInternal
 */
BOOL GetTipbarInternal(HWND hWnd, DWORD dwFlags, CDeskBand *pDeskBand)
{
    BOOL bParent = !!(dwFlags & 0x80000000);
    g_bWinLogon = !!(dwFlags & 0x1);

    InitFromReg();

    if (!g_bShowTipbar)
        return FALSE;

    if (bParent)
    {
        g_pTrayIconWnd = new(cicNoThrow) CTrayIconWnd();
        if (!g_pTrayIconWnd)
            return FALSE;
        g_pTrayIconWnd->CreateWnd();
    }

    g_pTipbarWnd = new(cicNoThrow) CTipbarWnd(bParent ? g_dwWndStyle : g_dwChildWndStyle);
    if (!g_pTipbarWnd || !g_pTipbarWnd->Initialize())
        return FALSE;

    g_pTipbarWnd->Init(!bParent, pDeskBand);
    g_pTipbarWnd->CreateWnd(hWnd);

    ::SetWindowText(*g_pTipbarWnd, TEXT("TF_FloatingLangBar_WndTitle"));

    DWORD dwOldStatus = 0;
    if (!bParent)
    {
        g_pTipbarWnd->m_pLangBarMgr->GetPrevShowFloatingStatus(&dwOldStatus);
        g_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_DESKBAND);
    }

    DWORD dwStatus;
    g_pTipbarWnd->m_pLangBarMgr->GetShowFloatingStatus(&dwStatus);
    g_pTipbarWnd->ShowFloating(dwStatus);

    if (!bParent && (dwOldStatus & TF_SFT_DESKBAND))
        g_pTipbarWnd->m_dwTipbarWndFlags |= TIPBAR_NODESKBAND;

    g_hwndParent = hWnd;
    return TRUE;
}

/***********************************************************************
 *              GetLibTls (MSUTB.@)
 *
 * @implemented
 */
EXTERN_C PCIC_LIBTHREAD WINAPI
GetLibTls(VOID)
{
    TRACE("()\n");
    return &g_libTLS;
}

/***********************************************************************
 *              GetPopupTipbar (MSUTB.@)
 *
 * @implemented
 */
EXTERN_C BOOL WINAPI
GetPopupTipbar(HWND hWnd, BOOL fWinLogon)
{
    TRACE("(%p, %d)\n", hWnd, fWinLogon);

    if (!fWinLogon)
        TurnOffSpeechIfItsOn();

    return GetTipbarInternal(hWnd, fWinLogon | 0x80000000, NULL);
}

/***********************************************************************
 *              SetRegisterLangBand (MSUTB.@)
 *
 * @implemented
 */
EXTERN_C HRESULT WINAPI
SetRegisterLangBand(BOOL bRegister)
{
    TRACE("(%d)\n", bRegister);

    if (!g_bEnableDeskBand || !(g_dwOSInfo & CIC_OSINFO_XPPLUS)) // Desk band is for XP+
        return E_FAIL;

    BOOL bDeskBand = IsDeskBandFromReg();
    if (bDeskBand == bRegister)
        return S_OK;

    SetDeskBandToReg(bRegister);

    if (!RegisterComCat(CLSID_MSUTBDeskBand, CATID_DeskBand, bRegister))
        return TF_E_NOLOCK;

    return S_OK;
}

/***********************************************************************
 *              ClosePopupTipbar (MSUTB.@)
 *
 * @implemented
 */
EXTERN_C VOID WINAPI
ClosePopupTipbar(VOID)
{
    TRACE("()\n");

    if (g_fInClosePopupTipbar)
        return;

    g_fInClosePopupTipbar = TRUE;

    if (g_pTipbarWnd)
    {
        g_pTipbarWnd->m_pDeskBand = NULL;
        g_pTipbarWnd->DestroyWnd();
        g_pTipbarWnd->Release();
        g_pTipbarWnd = NULL;
    }

    if (g_pTrayIconWnd)
    {
        g_pTrayIconWnd->DestroyWnd();
        delete g_pTrayIconWnd;
        g_pTrayIconWnd = NULL;
    }

    UninitSkipRedrawHKLArray();

    g_fInClosePopupTipbar = FALSE;
}

/***********************************************************************
 *              DllRegisterServer (MSUTB.@)
 *
 * @implemented
 */
STDAPI DllRegisterServer(VOID)
{
    TRACE("()\n");
    return gModule.DllRegisterServer(FALSE);
}

/***********************************************************************
 *              DllUnregisterServer (MSUTB.@)
 *
 * @implemented
 */
STDAPI DllUnregisterServer(VOID)
{
    TRACE("()\n");
    return gModule.DllUnregisterServer(FALSE);
}

/***********************************************************************
 *              DllCanUnloadNow (MSUTB.@)
 *
 * @implemented
 */
STDAPI DllCanUnloadNow(VOID)
{
    TRACE("()\n");
    return gModule.DllCanUnloadNow() && (g_DllRefCount == 0);
}

/***********************************************************************
 *              DllGetClassObject (MSUTB.@)
 *
 * @implemented
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    TRACE("()\n");
    return gModule.DllGetClassObject(rclsid, riid, ppv);
}

/// @implemented
HRESULT APIENTRY
MsUtbCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID iid,
    LPVOID *ppv)
{
    if (IsEqualCLSID(rclsid, CLSID_TF_CategoryMgr))
        return TF_CreateCategoryMgr((ITfCategoryMgr**)ppv);
    if (IsEqualCLSID(rclsid, CLSID_TF_DisplayAttributeMgr))
        return TF_CreateDisplayAttributeMgr((ITfDisplayAttributeMgr **)ppv);
    return cicRealCoCreateInstance(rclsid, pUnkOuter, dwClsContext, iid, ppv);
}

BEGIN_OBJECT_MAP(ObjectMap)
#ifdef ENABLE_DESKBAND
    OBJECT_ENTRY(CLSID_MSUTBDeskBand, CDeskBand) // FIXME: Implement this
#endif
END_OBJECT_MAP()

EXTERN_C VOID TFUninitLib(VOID)
{
    // Do nothing
}

/// @implemented
BOOL ProcessAttach(HINSTANCE hinstDLL)
{
    ::InitializeCriticalSectionAndSpinCount(&g_cs, 0);

    g_hInst = hinstDLL;

    cicGetOSInfo(&g_uACP, &g_dwOSInfo);

    TFInitLib(MsUtbCoCreateInstance);
    cicInitUIFLib();

    CTrayIconWnd::RegisterClass();

    g_wmTaskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");

    gModule.Init(ObjectMap, hinstDLL, NULL);
    ::DisableThreadLibraryCalls(hinstDLL);

    return TRUE;
}

/// @implemented
VOID ProcessDetach(HINSTANCE hinstDLL)
{
    cicDoneUIFLib();
    TFUninitLib();
    ::DeleteCriticalSection(&g_cs);
    gModule.Term();
}

/// @implemented
EXTERN_C BOOL WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD dwReason,
    _Inout_opt_ LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            TRACE("(%p, %lu, %p)\n", hinstDLL, dwReason, lpvReserved);
            if (!ProcessAttach(hinstDLL))
            {
                ProcessDetach(hinstDLL);
                return FALSE;
            }
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            ProcessDetach(hinstDLL);
            break;
        }
    }
    return TRUE;
}
