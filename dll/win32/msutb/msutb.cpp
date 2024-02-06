/*
 * PROJECT:     ReactOS msutb.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Language Bar (Tipbar)
 * COPYRIGHT:   Copyright 2023-2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(msutb);

HINSTANCE g_hInst = NULL;
UINT g_wmTaskbarCreated = 0;
UINT g_uACP = CP_ACP;
DWORD g_dwOSInfo = 0;
CRITICAL_SECTION g_cs;
LONG g_DllRefCount = 0;

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

EXTERN_C void __cxa_pure_virtual(void)
{
    ERR("__cxa_pure_virtual\n");
}

class CMsUtbModule : public CComModule
{
};

BEGIN_OBJECT_MAP(ObjectMap)
    //OBJECT_ENTRY(CLSID_MSUTBDeskBand, CDeskBand) // FIXME: Implement this
END_OBJECT_MAP()

CMsUtbModule gModule;

class CCicLibMenuItem;
class CTipbarAccItem;
class CUTBMenuItem;
class CMainIconItem;
class CTrayIconItem;
class CTipbarWnd;
class CButtonIconItem;

CTipbarWnd *g_pTipbarWnd = NULL;

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

void InitSkipRedrawHKLArray(void)
{
    g_prghklSkipRedrawing = new(cicNoThrow) CicArray<HKL>();
    if (!g_prghklSkipRedrawing)
        return;

    g_prghklSkipRedrawing->Add((HKL)UlongToHandle(0xE0010411)); // Japanese IME will be skipped

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
        error = regKey2.QueryDword(TEXT("ShowDeskBand"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_bShowDeskBand = !!dwValue;
        error = regKey2.QueryDword(TEXT("TimerElapseSHOWWDESKBAND"), &dwValue);
        if (error == ERROR_SUCCESS)
            g_uTimerElapseSHOWDESKBAND = dwValue;
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
    STDMETHOD_(void, Initialize)() override;
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
    CUTBMenuWnd *m_pMenuWnd;
    friend class CUTBMenuWnd;

public:
    CUTBMenuItem(CUTBMenuWnd *pMenuWnd);
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

class CLBarItemBase : public ITfLangBarItem
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

    HRESULT AdviseSink(REFIID riid, IUnknown *punk, DWORD *pdwCookie);
    HRESULT UnadviseSink(DWORD dwCookie);

    // ITfLangBarItem methods
    STDMETHOD(GetInfo)(TF_LANGBARITEMINFO *pInfo) override;
    STDMETHOD(GetStatus)(DWORD *pdwStatus) override;
    STDMETHOD(Show)(BOOL fShow) override;
    STDMETHOD(GetTooltipString)(BSTR *pbstrToolTip) override;
};

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
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfMenu))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
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
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IDispatch) ||
        IsEqualIID(riid, IID_IAccessible))
    {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
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

STDMETHODIMP_(void) CUTBMenuWnd::Initialize()
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

CUTBMenuItem::CUTBMenuItem(CUTBMenuWnd *pMenuWnd)
    : CUIFMenuItem(pMenuWnd ? pMenuWnd->GetMenu() : NULL)
{
    m_pMenuWnd = pMenuWnd;
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
    if (!m_pMenuWnd)
        return FALSE;

    m_pMenuWnd->StartDoAccDefaultActionTimer(this);
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
    ::ClientToScreen(m_pMenuWnd->m_hWnd, (LPPOINT)lprc);
    ::ClientToScreen(m_pMenuWnd->m_hWnd, (LPPOINT)&lprc->right);
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

CMainIconItem::CMainIconItem(CTrayIconWnd *pWnd)
    : CButtonIconItem(pWnd, 1)
{
}

BOOL CMainIconItem::Init(HWND hWnd)
{
    return CTrayIconItem::_Init(hWnd, WM_USER, 0, GUID_LBI_TRAYMAIN);
}

/// @unimplemented
STDMETHODIMP_(BOOL) CMainIconItem::OnDelayMsg(UINT uMsg)
{
    if (!CButtonIconItem::OnDelayMsg(uMsg))
        return 0;

    if (uMsg == WM_LBUTTONDBLCLK)
    {
        //FIXME
        //if (g_pTipbarWnd->m_dwUnknown20)
        //    g_pTipbarWnd->m_pLangBarMgr->ShowFloating(TF_SFT_SHOWNORMAL);
    }
    else if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN)
    {
        //FIXME
        //g_pTipbarWnd->ShowContextMenu(m_ptCursor, &m_rcClient, uMsg == WM_RBUTTONDOWN);
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

/// @unimplemented
BOOL CTrayIconWnd::OnIconMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //FIXME
    //if (g_pTipbarWnd)
    //    g_pTipbarWnd->AttachFocusThread();

    for (size_t iItem = 0; iItem < m_Items.size(); ++iItem)
    {
        auto pItem = m_Items[iItem];
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
        m_dwItemStatus &= ~0x1;
    else
        m_dwItemStatus |= 0x1;
    if (bUpdate && (dwOldStatus != m_dwItemStatus))
    {
        if (m_pLangBarItemSink)
            m_pLangBarItemSink->OnUpdate(TF_LBI_STATUS);
    }

    return S_OK;
}

STDMETHODIMP CLBarItemBase::GetInfo(TF_LANGBARITEMINFO *pInfo)
{
    CopyMemory(pInfo, &m_NewUIInfo, sizeof(*pInfo));
    return S_OK;
}

STDMETHODIMP CLBarItemBase::GetStatus(DWORD *pdwStatus)
{
    *pdwStatus = m_dwItemStatus;
    return S_OK;
}

STDMETHODIMP CLBarItemBase::Show(BOOL fShow)
{
    return ShowInternal(fShow, TRUE);
}

STDMETHODIMP CLBarItemBase::GetTooltipString(BSTR *pbstrToolTip)
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

    UINT nCommandId = m_pMenuUI->ShowModalPopup(pWindow, prcExclude, TRUE);
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
 *              GetLibTls (MSUTB.@)
 *
 * @unimplemented
 */
EXTERN_C LPVOID WINAPI
GetLibTls(VOID)
{
    FIXME("stub:()\n");
    return NULL;
}

/***********************************************************************
 *              GetPopupTipbar (MSUTB.@)
 *
 * @unimplemented
 */
EXTERN_C BOOL WINAPI
GetPopupTipbar(HWND hWnd, BOOL fWinLogon)
{
    FIXME("stub:(%p, %d)\n", hWnd, fWinLogon);
    return FALSE;
}

/***********************************************************************
 *              SetRegisterLangBand (MSUTB.@)
 *
 * @unimplemented
 */
EXTERN_C HRESULT WINAPI
SetRegisterLangBand(BOOL bRegister)
{
    FIXME("stub:(%d)\n", bRegister);
    return E_NOTIMPL;
}

/***********************************************************************
 *              ClosePopupTipbar (MSUTB.@)
 *
 * @unimplemented
 */
EXTERN_C VOID WINAPI
ClosePopupTipbar(VOID)
{
    FIXME("stub:()\n");
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
