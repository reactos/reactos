/**************************************************************************\
 * Module Name: multimon.cpp
 *
 * Contains all the code to manage multiple devices
 *
 * Copyright (c) Microsoft Corp.  1995-1998 All Rights Reserved
 *
 * NOTES:
 *
 \**************************************************************************/

#include "precomp.h"
#include "shlobjp.h"
#include "shlwapi.h"
#include "settings.hxx"
#include "ntreg.hxx"
#include <tchar.h>
#include <dbt.h>
#include <oleacc.h>

#if DEBUG
#define new DBG_NEW
#endif

extern "C" {
    LRESULT CALLBACK MonitorWindowProc(HWND hwnd, UINT msg,WPARAM wParam,LPARAM lParam);
    LRESULT CALLBACK SliderSubWndProc (HWND hwndSlider, UINT uMsg, WPARAM wParam, LPARAM lParam, WPARAM uID, ULONG_PTR dwRefData);
    int ComputeNumberOfDisplayDevices();
    //The following are defined in lookdlg.c
    extern  int g_iNewDPI;
    extern  int g_iAppliedNewDPI;
    extern  HWND    g_hDlg;
    extern  void    UpdateUIfontsDueToDPIchange(int iOldDPI, int iNewDPI);    
    extern  void    Look_InitDPIglobals();
}

BOOL MakeMonitorBitmap(int w, int h, LPCTSTR sz, HBITMAP *pBitmap, HBITMAP *pMaskBitmap, int cx, int cy, BOOL fSelected);

#define SELECTION_THICKNESS 4
#define MONITOR_BORDER      1

#define REGSTR_VAL_SAFEBOOT        TEXT("System\\CurrentControlSet\\Control\\SafeBoot\\Option")

// Maximum number of monitors supported.
#define MONITORS_MAX    10

#define PREVIEWAREARATIO 2

#define MM_REDRAWPREVIEW (WM_USER + 1)
#define MM_MONITORMOVED  (WM_USER + 2)

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)

#define ToolTip_Activate(hTT, activate) \
    SendMessage(hTT, TTM_ACTIVATE, (WPARAM) activate, (LPARAM) 0)

#define ToolTip_AddTool(hTT, lpti) \
    SendMessage(hTT, TTM_ADDTOOL, (WPARAM) 0, (LPARAM) (lpti))

#define ToolTip_DelTool(hTT, lpti) \
    SendMessage(hTT, TTM_DELTOOL, (WPARAM) 0, (LPARAM) (lpti))

#define ToolTip_GetCurrentTool(hTT, lpti) \
    SendMessage(hTT, TTM_GETCURRENTTOOL, (WPARAM) 0, (LPARAM) (lpti))

#define ToolTip_RelayEvent(hTT, _msg, h, m, wp, lp)                         \
    _msg.hwnd = h; _msg.message = m; _msg.wParam = wp; _msg.lParam = lp;\
    SendMessage(hTT, TTM_RELAYEVENT, (WPARAM) 0, (LPARAM) &_msg);       

#define ToolTip_SetDelayTime(hTT, d, t) \
    SendMessage(hTT, TTM_SETDELAYTIME, (WPARAM) d, (LPARAM)MAKELONG((t), 0))

#define ToolTip_SetToolInfo(hTT, lpti) \
    SendMessage(hTT, TTM_SETTOOLINFO, (WPARAM) 0, (LPARAM) (lpti))

#define ToolTip_TrackActivate(hTT, bActivate, lpti) \
    SendMessage(hTT, TTM_TRACKACTIVATE, (WPARAM) (bActivate), (LPARAM) (lpti))

#define ToolTip_TrackPosition(hTT, x, y) \
    SendMessage(hTT, TTM_TRACKPOSITION, (WPARAM) 0, (LPARAM) MAKELONG((x), (y)))

#define ToolTip_Update(hTT) \
    SendMessage(hTT, TTM_UPDATE, (WPARAM) 0, (LPARAM) 0)

VOID
CDECL
TRACE(
    PCTSTR pszMsg,
    ...
    ) 
/*++

Outputs a message to the setup log.  Prepends "desk.cpl  " to the strings and 
appends the correct newline chars (\r\n)==

  --*/
{
    TCHAR ach[1024+40];    // Largest path plus extra
    va_list vArgs;

    va_start(vArgs, pszMsg);
    wvsprintf(ach, pszMsg, vArgs);
    va_end(vArgs);

    OutputDebugString(ach);
}

#ifdef _WIN64
//
//  GetDlgItem and GetDlgCtrlID don't support INT_PTR's,
//  so we have to do it manually.
//  Fortunately, GetWindowLongPtr(GWLP_ID) actually returns a full 64-bit
//  value instead of truncating at 32-bits.
//

#define GetDlgCtrlIDP(hwnd)  GetWindowLongPtr(hwnd, GWLP_ID)

HWND GetDlgItemP(HWND hDlg, INT_PTR id)
{
    HWND hwndChild = GetWindow(hDlg, GW_CHILD);
    while (hwndChild && GetDlgCtrlIDP(hwndChild) != id)
        hwndChild = GetWindow(hwndChild, GW_HWNDNEXT);
    return hwndChild;
}

#else
#define GetDlgItemP     GetDlgItem
#define GetDlgCtrlIDP   GetDlgCtrlID
#endif

//
// display devices
//
typedef struct _multimon_device {

    //
    // Main class for settings
    //

    CDeviceSettings * pds;

    //
    // Color and resolution information cache
    // Rebuild when modes are enumerated.
    //

    int            cColors;
    PLONGLONG      ColorList;
    int            cResolutions;
    PPOINT         ResolutionList;


    ULONG          ComboBoxItem;
    DISPLAY_DEVICE DisplayDevice;
    ULONG          DisplayIndex;
    POINT          Snap;
    HDC            hdc;

    //
    // Image information.
    //
    int            w,h;
    HIMAGELIST     himl;
    int            iImage;

    BOOLEAN        bTracking;
    HWND           hwndFlash;  //Flash window.
} MULTIMON_DEVICE, *PMULTIMON_DEVICE;

#define GetDlgCtrlDevice(hwnd) ((PMULTIMON_DEVICE)GetDlgCtrlIDP(hwnd))

BOOL gfFlashWindowRegistered = FALSE;
HWND ghwndToolTipTracking;
HWND ghwndToolTipPopup;
HWND ghwndPropSheet;

void AddTrackingToolTip(PMULTIMON_DEVICE pDevice, HWND hwnd);
void RemoveTrackingToolTip(HWND hwnd);

void AddPopupToolTip(HWND hwndC);
void RemovePopupToolTip(HWND hwndC);

#if QUICK_REFRESH
#define IDC_FREQUENCY_START     2000
HMENU CreateFrequencyMenu(PMULTIMON_DEVICE pDevice);
#endif

extern int AskDynaCDS(HWND hDlg);
extern int GetDisplayCPLPreference(LPCTSTR szRegVal);
extern void SetDisplayCPLPreference(LPCTSTR szRegVal, int val);

// Prototype for CreateStdAccessibleProxy.
// A and W versions are available - pClassName can be ANSI or UNICODE
// string. This is a TCHAR-style prototype, but you can do a A or W
// specific one if desired.
typedef HRESULT (WINAPI *PFNCREATESTDACCESSIBLEPROXY) (
    HWND     hWnd,
    LPTSTR   pClassName,
    LONG     idObject,
    REFIID   riid,
    void **  ppvObject 
    );

// Same for LresultFromObject...
typedef LRESULT (WINAPI *PFNLRESULTFROMOBJECT)(
    REFIID riid,
    WPARAM wParam,
    LPUNKNOWN punk 
    );


PRIVATE PFNCREATESTDACCESSIBLEPROXY s_pfnCreateStdAccessibleProxy = NULL;
PRIVATE PFNLRESULTFROMOBJECT s_pfnLresultFromObject = NULL;

BOOL g_fAttemptedOleAccLoad ;
HMODULE g_hOleAcc;

//-----------------------------------------------------------------------------
static const DWORD sc_MultiMonitorHelpIds[] =
{
   IDC_SCREENSAMPLE,  IDH_DISPLAY_SETTINGS_MONITOR_GRAPHIC,  
   IDC_MULTIMONHELP,  IDH_DISPLAY_SETTINGS_MONITOR_GRAPHIC, 
   IDC_DISPLAYDESK,   IDH_DISPLAY_SETTINGS_MONITOR_GRAPHIC, 

   IDC_DISPLAYLABEL,  IDH_DISPLAY_SETTINGS_DISPLAY_LIST,
   IDC_DISPLAYLIST,   IDH_DISPLAY_SETTINGS_DISPLAY_LIST,
   IDC_DISPLAYTEXT,   IDH_DISPLAY_SETTINGS_DISPLAY_LIST,

   IDC_COLORGROUPBOX, IDH_DISPLAY_SETTINGS_COLORBOX, 
   IDC_COLORBOX,      IDH_DISPLAY_SETTINGS_COLORBOX,
   IDC_COLORSAMPLE,   IDH_DISPLAY_SETTINGS_COLORBOX, 

   IDC_RESGROUPBOX,   IDH_DISPLAY_SETTINGS_SCREENAREA,
   IDC_SCREENSIZE,    IDH_DISPLAY_SETTINGS_SCREENAREA,
   IDC_RES_LESS,      IDH_DISPLAY_SETTINGS_SCREENAREA,
   IDC_RES_MORE,      IDH_DISPLAY_SETTINGS_SCREENAREA,
   IDC_RESXY,         IDH_DISPLAY_SETTINGS_SCREENAREA,

   IDC_DISPLAYUSEME,  IDH_DISPLAY_SETTINGS_EXTEND_DESKTOP_CHECKBOX, 
   IDC_DISPLAYPRIME,  IDH_DISPLAY_SETTINGS_USE_PRIMARY_CHECKBOX,

   IDC_IDENTIFY,          IDH_DISPLAY_SETTINGS_IDENTIFY_BUTTON,
   IDC_TROUBLESHOOT,      IDH_DISPLAY_SETTINGS_TROUBLE_BUTTON,
   IDC_DISPLAYPROPERTIES, IDH_DISPLAY_SETTINGS_ADVANCED_BUTTON,

   0, 0
};

class CAccessibleWrapper: public IAccessible
{
        // We need to do our own refcounting for this wrapper object
        ULONG          m_ref;

        // Need ptr to the IAccessible
        IAccessible *  m_pAcc;
        HWND           m_hwnd;
public:
        CAccessibleWrapper( HWND hwnd, IAccessible * pAcc );
        virtual ~CAccessibleWrapper();

        // IUnknown
        // (We do our own ref counting)
        virtual STDMETHODIMP            QueryInterface(REFIID riid, void** ppv);
        virtual STDMETHODIMP_(ULONG)    AddRef();
        virtual STDMETHODIMP_(ULONG)    Release();
        
        // IDispatch
        virtual STDMETHODIMP            GetTypeInfoCount(UINT* pctinfo);
        virtual STDMETHODIMP            GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo);
        virtual STDMETHODIMP            GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
            LCID lcid, DISPID* rgdispid);
        virtual STDMETHODIMP            Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
            DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
            UINT* puArgErr);

        // IAccessible
        virtual STDMETHODIMP            get_accParent(IDispatch ** ppdispParent);
        virtual STDMETHODIMP            get_accChildCount(long* pChildCount);
        virtual STDMETHODIMP            get_accChild(VARIANT varChild, IDispatch ** ppdispChild);

        virtual STDMETHODIMP            get_accName(VARIANT varChild, BSTR* pszName);
        virtual STDMETHODIMP            get_accValue(VARIANT varChild, BSTR* pszValue);
        virtual STDMETHODIMP            get_accDescription(VARIANT varChild, BSTR* pszDescription);
        virtual STDMETHODIMP            get_accRole(VARIANT varChild, VARIANT *pvarRole);
        virtual STDMETHODIMP            get_accState(VARIANT varChild, VARIANT *pvarState);
        virtual STDMETHODIMP            get_accHelp(VARIANT varChild, BSTR* pszHelp);
        virtual STDMETHODIMP            get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic);
        virtual STDMETHODIMP            get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut);
        virtual STDMETHODIMP            get_accFocus(VARIANT * pvarFocusChild);
        virtual STDMETHODIMP            get_accSelection(VARIANT * pvarSelectedChildren);
        virtual STDMETHODIMP            get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction);

        virtual STDMETHODIMP            accSelect(long flagsSel, VARIANT varChild);
        virtual STDMETHODIMP            accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild);
        virtual STDMETHODIMP            accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt);
        virtual STDMETHODIMP            accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint);
        virtual STDMETHODIMP            accDoDefaultAction(VARIANT varChild);

        virtual STDMETHODIMP            put_accName(VARIANT varChild, BSTR szName);
        virtual STDMETHODIMP            put_accValue(VARIANT varChild, BSTR pszValue);
};

CAccessibleWrapper::CAccessibleWrapper( HWND hwnd, IAccessible * pAcc )
    : m_ref( 1 ),
      m_pAcc( pAcc ),
      m_hwnd( hwnd )
{
    ASSERT( m_pAcc );
    m_pAcc->AddRef();
}

CAccessibleWrapper::~CAccessibleWrapper()
{
    m_pAcc->Release();
}


// IUnknown
// Implement refcounting ourselves
// Also implement QI ourselves, so that we return a ptr back to the wrapper.
STDMETHODIMP  CAccessibleWrapper::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if ((riid == IID_IUnknown)  ||
        (riid == IID_IDispatch) ||
        (riid == IID_IAccessible))
    {
        *ppv = (IAccessible *) this;
    }
    else
        return(E_NOINTERFACE);

    AddRef();
    return(NOERROR);
}


STDMETHODIMP_(ULONG) CAccessibleWrapper::AddRef()
{
    return ++m_ref;
}


STDMETHODIMP_(ULONG) CAccessibleWrapper::Release()
{
    ULONG ulRet = --m_ref;

    if( ulRet == 0 )
        delete this;

    return ulRet;
}

// IDispatch
// - pass all through m_pAcc

STDMETHODIMP  CAccessibleWrapper::GetTypeInfoCount(UINT* pctinfo)
{
    return m_pAcc->GetTypeInfoCount(pctinfo);
}


STDMETHODIMP  CAccessibleWrapper::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
{
    return m_pAcc->GetTypeInfo(itinfo, lcid, pptinfo);
}


STDMETHODIMP  CAccessibleWrapper::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames,
            LCID lcid, DISPID* rgdispid)
{
    return m_pAcc->GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP  CAccessibleWrapper::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
            DISPPARAMS* pdispparams, VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
            UINT* puArgErr)
{
    return m_pAcc->Invoke(dispidMember, riid, lcid, wFlags,
            pdispparams, pvarResult, pexcepinfo,
            puArgErr);
}

// IAccessible
// - pass all through m_pAcc

STDMETHODIMP  CAccessibleWrapper::get_accParent(IDispatch ** ppdispParent)
{
    return m_pAcc->get_accParent(ppdispParent);
}


STDMETHODIMP  CAccessibleWrapper::get_accChildCount(long* pChildCount)
{
    return m_pAcc->get_accChildCount(pChildCount);
}


STDMETHODIMP  CAccessibleWrapper::get_accChild(VARIANT varChild, IDispatch ** ppdispChild)
{
    return m_pAcc->get_accChild(varChild, ppdispChild);
}



STDMETHODIMP  CAccessibleWrapper::get_accName(VARIANT varChild, BSTR* pszName)
{
    return m_pAcc->get_accName(varChild, pszName);
}



STDMETHODIMP  CAccessibleWrapper::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    // varChild.lVal specifies which sub-part of the component
    // is being queried.
    // CHILDID_SELF (0) specifies the overall component - other
    // non-0 values specify a child.

    // In a trackbar, CHILDID_SELF refers to the overall trackbar
    // (which is what we want), whereas other values refer to the
    // sub-components - the actual slider 'thumb', and the 'page
    // up/page down' areas to the left/right of it.
    if( varChild.vt == VT_I4 && varChild.lVal == CHILDID_SELF )
    {
        HWND hDlg;
        TCHAR achRes[120];
#ifndef UNICODE
        WCHAR wszRes[120];
#endif

        hDlg = GetParent( m_hwnd );

        SendDlgItemMessage(hDlg, IDC_RESXY, WM_GETTEXT, 120, (LPARAM)achRes);
#ifdef UNICODE
        *pszValue = SysAllocString( achRes );
#else
        MultiByteToWideChar( CP_ACP, 0, achRes, -1, wszRes, 120 );
        *pszValue = SysAllocString( wszRes );
#endif
        return S_OK;

    }
    else
    {
        // Pass requests about the sub-components to the
        // 'original' IAccessible for us).
        return m_pAcc->get_accValue(varChild, pszValue);
    }
}


STDMETHODIMP  CAccessibleWrapper::get_accDescription(VARIANT varChild, BSTR* pszDescription)
{
    return m_pAcc->get_accDescription(varChild, pszDescription);
}


STDMETHODIMP  CAccessibleWrapper::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    return m_pAcc->get_accRole(varChild, pvarRole);
}


STDMETHODIMP  CAccessibleWrapper::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    return m_pAcc->get_accState(varChild, pvarState);
}


STDMETHODIMP  CAccessibleWrapper::get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
    return m_pAcc->get_accHelp(varChild, pszHelp);
}


STDMETHODIMP  CAccessibleWrapper::get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic)
{
    return m_pAcc->get_accHelpTopic(pszHelpFile, varChild, pidTopic);
}


STDMETHODIMP  CAccessibleWrapper::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut)
{
    return m_pAcc->get_accKeyboardShortcut(varChild, pszKeyboardShortcut);
}


STDMETHODIMP  CAccessibleWrapper::get_accFocus(VARIANT * pvarFocusChild)
{
    return m_pAcc->get_accFocus(pvarFocusChild);
}


STDMETHODIMP  CAccessibleWrapper::get_accSelection(VARIANT * pvarSelectedChildren)
{
    return m_pAcc->get_accSelection(pvarSelectedChildren);
}


STDMETHODIMP  CAccessibleWrapper::get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction)
{
    return m_pAcc->get_accDefaultAction(varChild, pszDefaultAction);
}



STDMETHODIMP  CAccessibleWrapper::accSelect(long flagsSel, VARIANT varChild)
{
    return m_pAcc->accSelect(flagsSel, varChild);
}


STDMETHODIMP  CAccessibleWrapper::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
    return m_pAcc->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
}


STDMETHODIMP  CAccessibleWrapper::accNavigate(long navDir, VARIANT varStart, VARIANT * pvarEndUpAt)
{
    return m_pAcc->accNavigate(navDir, varStart, pvarEndUpAt);
}


STDMETHODIMP  CAccessibleWrapper::accHitTest(long xLeft, long yTop, VARIANT * pvarChildAtPoint)
{
    return m_pAcc->accHitTest(xLeft, yTop, pvarChildAtPoint);
}


STDMETHODIMP  CAccessibleWrapper::accDoDefaultAction(VARIANT varChild)
{
    return m_pAcc->accDoDefaultAction(varChild);
}



STDMETHODIMP  CAccessibleWrapper::put_accName(VARIANT varChild, BSTR szName)
{
    return m_pAcc->put_accName(varChild, szName);
}


STDMETHODIMP  CAccessibleWrapper::put_accValue(VARIANT varChild, BSTR pszValue)
{
    return m_pAcc->put_accValue(varChild, pszValue);
}


class CMultiMon  : public IMultiMonConfig
{
    friend int ComputeNumberOfDisplayDevices();
    friend int DisplaySaveSettings(PVOID pContext, HWND hwnd);

    private:
        // Data Section
        PMULTIMON_DEVICE _pCurDevice;
        PMULTIMON_DEVICE _pPrimaryDevice;

        // HWND for the main window
        HWND _hDlg;
        HWND _hwndDesk;
        HWND _hwndList;

        // union of all monitor RECTs
        RECT _rcDesk;

        // ref count
        UINT _cRef;

        // how to translate to preview size
        int   _DeskScale;
        POINT _DeskOff;
        UINT  _InSetInfo;
        ULONG _NumDevices;
        HBITMAP _hbmScrSample;
        HBITMAP _hbmMonitor;
        HIMAGELIST _himl;


        // UI variables
        int  _iColor;
        int  _iResolution;

        BOOL _bBadDriver         : 1;
        BOOL _bNoAttach          : 1;
        BOOL _bDirty             : 1;

        MULTIMON_DEVICE _Devices[MONITORS_MAX];

        // Private functions
        void _DeskToPreview(LPRECT in, LPRECT out);
        void _OffsetPreviewToDesk(LPRECT in, LPRECT out);
        BOOL _QueryForceSmallFont();
        void _SetPreviewScreenSize(int HRes, int VRes, int iOrgXRes, int iOrgYRes);
        void _CleanupRects(HWND hwndP);
        void _ConfirmPositions();
        void _DoAdvancedSettingsSheet();
        int  _HandleApply();
        BOOL _HandleHScroll(HWND hwndSB, int iCode, int iPos);
        void _RedrawDeskPreviews();
        void _OnAdvancedClicked();

        BOOL _InitDisplaySettings(BOOL bExport);
        int  _EnumerateAllDisplayDevices(); //Enumerates and returns the number of devices.
        void _DestroyMultimonDevice(PMULTIMON_DEVICE pDevice);
        void _DestroyDisplaySettings();

        void _InitUI();
        void _UpdateUI(int FocusToCtrlID = 0);
        LPTSTR _FormatMessageInvoke(LPCTSTR pcszFormat, va_list *argList);
        LPTSTR _FormatMessageWrap(LPCTSTR pcszFormat, ...);
        void _GetDisplayName(PMULTIMON_DEVICE pDevice, LPTSTR pszDisplay, DWORD cchSize);
        int  _SaveDisplaySettings(DWORD dwSet);
        BOOL _RebuildDisplaySettings(BOOL bComplete);
        void _ForwardToChildren(UINT message, WPARAM wParam, LPARAM lParam);

        static BOOL _AnyChange(CDeviceSettings *rgpds[], ULONG numDevices);

        static BOOL _AnyColorChange(CDeviceSettings *rgpds[], ULONG numDevices);

        static BOOL _IsSingleToMultimonChange(CDeviceSettings *rgpds[], 
                                              ULONG numDevices);

        static int _DisplaySaveSettings(CDeviceSettings* rgpds[],
                                        ULONG            numDevices,
                                        HWND             hDlg);

        static int _SaveSettings(CDeviceSettings *rgpds[],
                                 ULONG numDevices,
                                 HWND hDlg,
                                 DWORD dwSet);
        
        // NT specific stuff
#ifdef WINNT
        BOOL _InitMessage();
        void _vPreExecMode();
        void _vPostExecMode();
#endif
    public:
        CMultiMon();

        static BOOL RegisterPreviewWindowClass(WNDPROC pfnWndProc);
        // *** IUnknown methods ***
        STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // *** IMultiMonConfig methods ***
        STDMETHOD ( Initialize ) ( HWND hwndHost, WNDPROC pfnWndProc, DWORD dwReserved);
        STDMETHOD ( GetNumberOfMonitors ) (int * pCMon, DWORD dwReserved);
        STDMETHOD ( GetMonitorData) (int iMonitor, MonitorData * pmd, DWORD dwReserved);
        STDMETHOD ( Paint) (THIS_ int iMonitor, DWORD dwReserved);

        void InitMultiMonitorDlg(HWND hDlg);
        PMULTIMON_DEVICE GetCurDevice(){return _pCurDevice;};

        int  GetNumberOfAttachedDisplays();
        void UpdateActiveDisplay(PMULTIMON_DEVICE pDevice, BOOL bRepaint = TRUE);
        BOOL HandleMonitorChange(HWND hwndP, BOOL bMainDlg, BOOL bRepaint = TRUE);
        void SetDirty(BOOL bDirty=TRUE);
        BOOL SetPrimary(PMULTIMON_DEVICE pDevice);
        BOOL SetMonAttached(PMULTIMON_DEVICE pDevice, BOOL bSetAttached,
                            BOOL bForce, HWND hwnd);

        HWND  GetCurDeviceHwnd() { return GetDlgItemP(_hwndDesk, (INT_PTR) _pCurDevice);};
        ULONG GetNumDevices()    { return _NumDevices;};
        BOOL  QueryNoAttach()    { return _bNoAttach;};
        BOOL  IsDirty()          { return _bDirty;};

        void GetMonitorPosition(PMULTIMON_DEVICE pDevice, HWND hwndP, PPOINT ptPos);

        LRESULT CALLBACK WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#if 0
    TraceMsg(TF_DUMP_CSETTINGS,"\t     cb          = %d",     _DisplayDevice.cb           );
    TraceMsg(TF_DUMP_CSETTINGS,"\t     DeviceName  = %s",     _pstrDisplayDevice   );
    TraceMsg(TF_DUMP_CSETTINGS,"\t     DeviceString= %s",     _DisplayDevice.DeviceString );
    TraceMsg(TF_DUMP_CSETTINGS,"\t     StateFlags  = %08lx",  _DisplayDevice.StateFlags   );
#endif

CMultiMon::CMultiMon() : _cRef(1) {
    ASSERT(_pCurDevice == NULL);
    ASSERT(_pPrimaryDevice == NULL);
    ASSERT(_DeskScale == 0);
    ASSERT(_InSetInfo == 0);
    ASSERT(_NumDevices == 0);
    ASSERT(IsRectEmpty(&_rcDesk));
    ASSERT(_bNoAttach == FALSE);
    ASSERT(_bDirty == FALSE);
};



void CMultiMon::_DestroyMultimonDevice(PMULTIMON_DEVICE pDevice)
{
    ASSERT(pDevice->pds);
    delete pDevice->pds;
    pDevice->pds = NULL;

    if(pDevice->hwndFlash)
    {
        DestroyWindow(pDevice->hwndFlash);
        pDevice->hwndFlash = NULL;
    }

    if (pDevice->hdc) {
        DeleteDC(pDevice->hdc);
        pDevice->hdc = NULL;
    }

    if (pDevice->ResolutionList) {
        LocalFree(pDevice->ResolutionList);
        pDevice->ResolutionList = NULL;
    }

    if (pDevice->ColorList) {
        LocalFree(pDevice->ColorList);
        pDevice->ColorList = NULL;
    }
}

void CMultiMon::_DestroyDisplaySettings()
{
    ULONG iDevice;
    HWND    hwndC;
    ASSERT(_NumDevices);
    TraceMsg(TF_GENERAL, "DestroyDisplaySettings: %d devices", _NumDevices);

    // We are about to destroy the _Devices below. Pointerts to these devices are used as the 
    // CtrlIDs for the monitor windows. So, we need destroy the monitor windows first; 
    // otherwise, if the monitor windows are destroyed later, they try to use these invalid 
    // pDevice in FlashText. (pDevice->hwndFlash will fault).
    while (hwndC = GetWindow(_hwndDesk, GW_CHILD))
    {
        RemoveTrackingToolTip(hwndC);
        RemovePopupToolTip(hwndC);
        DestroyWindow(hwndC);
    }

    // Now, we can destroy the _Devices safely.
    for (iDevice = 0; iDevice < _NumDevices; iDevice++) {
        _DestroyMultimonDevice(_Devices + iDevice);
        // Note: pds is destroyed and set to zero already in the above call.
        //delete _Devices[iDevice].pds;
        //_Devices[iDevice].pds = 0;
    }

    if (_himl) {
        ImageList_Destroy(_himl);
        _himl = NULL;
    }

    DestroyWindow(ghwndToolTipTracking);
    DestroyWindow(ghwndToolTipPopup);

    ghwndToolTipTracking = NULL;
    ghwndToolTipPopup = NULL;

    TraceMsg(TF_GENERAL, "DestroyDisplaySettings: Finished destroying all devices");
}

#ifdef WINNT
//
// deterines if the applet is in detect mode.
//

//
// Called to put up initial messages that need to appear above the dialog
// box
//

BOOL CMultiMon::_InitMessage()
{
    {
        //
        // _bBadDriver will be set when we fail to build the list of modes,
        // or something else failed during initialization.
        //
        // In almost every case, we should already know about this situation
        // based on our boot code.
        // However, if this is a new situation, just report a "bad driver"
        //

        if (_bBadDriver)
        {
            ASSERT(gbExecMode == EXEC_INVALID_MODE);

            gbExecMode = EXEC_INVALID_MODE;
            gbInvalidMode = EXEC_INVALID_DISPLAY_DRIVER;
        }


        if (gbExecMode == EXEC_INVALID_MODE)
        {
            DWORD Mesg;

            switch(gbInvalidMode) {

            case EXEC_INVALID_NEW_DRIVER:
                Mesg = MSG_INVALID_NEW_DRIVER;
                break;
            case EXEC_INVALID_DEFAULT_DISPLAY_MODE:
                Mesg = MSG_INVALID_DEFAULT_DISPLAY_MODE;
                break;
            case EXEC_INVALID_DISPLAY_DRIVER:
                Mesg = MSG_INVALID_DISPLAY_DRIVER;
                break;
            case EXEC_INVALID_OLD_DISPLAY_DRIVER:
                Mesg = MSG_INVALID_OLD_DISPLAY_DRIVER;
                break;
            case EXEC_INVALID_16COLOR_DISPLAY_MODE:
                Mesg = MSG_INVALID_16COLOR_DISPLAY_MODE;
                break;
            case EXEC_INVALID_DISPLAY_MODE:
                Mesg = MSG_INVALID_DISPLAY_MODE;
                {
                    //
                    // If we are in safe mode, then we will get to here when
                    // we initially log in.  We are in forced VGA mode, so there
                    // is no real error here.  Emulate a click on the OK button
                    // and everybody is happy.
                    //
                    HKEY hSafe;

                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                     REGSTR_VAL_SAFEBOOT,
                                     0,
                                     KEY_READ,
                                     &hSafe) == ERROR_SUCCESS) {
                
                        //
                        // If we ever care about the actual safe mode, the value
                        // is nameed "OptionValue"
                        //
                        RegCloseKey(hSafe);
                        PropSheet_PressButton(GetParent(_hDlg), PSBTN_OK);
                        return TRUE;
                    }
                }
                break;
            case EXEC_INVALID_CONFIGURATION:
            default:
                Mesg = MSG_INVALID_CONFIGURATION;
                break;
            }

            FmtMessageBox(_hDlg,
                          MB_ICONEXCLAMATION,
                          MSG_CONFIGURATION_PROBLEM,
                          Mesg);

            //
            // For a bad display driver or old display driver, let's send the
            // user straight to the installation dialog.
            //

            if ((gbInvalidMode == EXEC_INVALID_OLD_DISPLAY_DRIVER) ||
                (gbInvalidMode == EXEC_INVALID_DISPLAY_DRIVER))
            {
                ASSERT(FALSE);
            }
        }
    }

    return TRUE;
}

VOID CMultiMon::_vPreExecMode()
{

    HKEY hkey;
//    DWORD data;

    //
    // This function sets up the execution mode of the applet.
    // There are four vlid modes.
    //
    // EXEC_NORMAL - When the apple is launched from the control panel
    //
    // EXEC_INVALID_MODE is exactly the same as for NORMAL except we will
    //                   not mark the current mode as tested so the user has
    //                   to at least test a mode
    //
    // EXEC_DETECT - When the applet is launched normally, but a detect was
    //               done on the previous boot (the key in the registry is
    //               set)
    //
    // EXEC_SETUP  - When we launch the applet in setup mode from setup (Both
    //               the registry key is set and the setup flag is passed in).
    //

    //
    // These two keys should only be checked \ deleted if the machine has been
    // rebooted and the detect \ new display has actually happened.
    // So we will look for the RebootNecessary key (a volatile key) and if
    // it is not present, then we can delete the key.  Otherwise, the reboot
    // has not happened, and we keep the key
    //

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_REBOOT_NECESSARY,
                     0,
                     KEY_READ | KEY_WRITE,
                     &hkey) != ERROR_SUCCESS) {

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         SZ_DETECT_DISPLAY,
                         0,
                         KEY_READ | KEY_WRITE,
                         &hkey) == ERROR_SUCCESS) {

            //
            // NOTE: This key is also set when EXEC_SETUP is being run.
            //

            if (gbExecMode == EXEC_NORMAL) {

                gbExecMode = EXEC_DETECT;

            } else {

                //
                // If we are in setup mode, we also check the extra values
                // under DetectDisplay that control the unattended installation.
                //

                ASSERT(gbExecMode == EXEC_SETUP);

            }

            RegCloseKey(hkey);
        }

        //
        // Check for a new driver being installed
        //

        if ( (gbExecMode == EXEC_NORMAL) &&
             (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           SZ_NEW_DISPLAY,
                           0,
                           KEY_READ | KEY_WRITE,
                           &hkey) == ERROR_SUCCESS) ) {

            gbExecMode = EXEC_INVALID_MODE;
            gbInvalidMode = EXEC_INVALID_NEW_DRIVER;

            RegCloseKey(hkey);
        }

        RegDeleteKey(HKEY_LOCAL_MACHINE,
                     SZ_DETECT_DISPLAY);

        RegDeleteKey(HKEY_LOCAL_MACHINE,
                     SZ_NEW_DISPLAY);
    }
    {
        LPTSTR psz;
        LPTSTR pszInv;

        switch(gbExecMode) {

            case EXEC_NORMAL:
                psz = TEXT("Normal Execution mode");
                break;
            case EXEC_DETECT:
                psz = TEXT("Detection Execution mode");
                break;
            case EXEC_SETUP:
                psz = TEXT("Setup Execution mode");
                break;
            case EXEC_INVALID_MODE:
                psz = TEXT("Invalid Mode Execution mode");

                switch(gbInvalidMode) {

                    case EXEC_INVALID_NEW_DRIVER:
                        pszInv = TEXT("Invalid new driver");
                        break;
                    default:
                        pszInv = TEXT("*** Invalid *** Invalid mode");
                        break;
                }
                break;
            default:
                psz = TEXT("*** Invalid *** Execution mode");
                break;
        }

        KdPrint(("\n \nDisplay.cpl: The display applet is in : %ws\n", psz));

        if (gbExecMode == EXEC_INVALID_MODE)
        {
            KdPrint(("\t\t sub invalid mode : %ws", pszInv));
        }
        KdPrint(("\n\n", psz));
    }
}


VOID CMultiMon::_vPostExecMode() {

    HKEY hkey;
    DWORD cb;
    DWORD data;

    //
    // Check for various invalid configurations
    //

    if ( (gbExecMode == EXEC_NORMAL) &&
         (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       SZ_INVALID_DISPLAY,
                       0,
                       KEY_READ | KEY_WRITE,
                       &hkey) == ERROR_SUCCESS) ) {

        gbExecMode = EXEC_INVALID_MODE;

        //
        // Check for these fields in increasing order of "badness" or
        // "detail" so that the *worst* error is the one remaining in the
        // gbInvalidMode  variable once all the checks are done.
        //

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("DefaultMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DEFAULT_DISPLAY_MODE;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("BadMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DISPLAY_MODE;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("16ColorMode"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_16COLOR_DISPLAY_MODE;
        }


        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("InvalidConfiguration"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_CONFIGURATION;
        }

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("MissingDisplayDriver"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_DISPLAY_DRIVER;
        }

        //
        // This last case will be set in addition to the previous one in the
        // case where the driver was an old driver linking to winsvr.dll
        // and we can not load it.
        //

        cb = 4;
        if (RegQueryValueEx(hkey,
                            TEXT("OldDisplayDriver"),
                            NULL,
                            NULL,
                            (LPBYTE)(&data),
                            &cb) == ERROR_SUCCESS)
        {
            gbInvalidMode = EXEC_INVALID_OLD_DISPLAY_DRIVER;
        }

        RegCloseKey(hkey);

    }

    //
    // Delete all of these bad configuration keys since we only want the
    // user to see the message once.
    //

    RegDeleteKey(HKEY_LOCAL_MACHINE,
                 SZ_INVALID_DISPLAY);

{
    LPTSTR psz;
    LPTSTR pszInv;

    if (gbExecMode == EXEC_INVALID_MODE)
    {
        switch (gbInvalidMode)
        {
        case EXEC_INVALID_DEFAULT_DISPLAY_MODE:
            pszInv = TEXT("Default mode being used");
            break;
        case EXEC_INVALID_DISPLAY_DRIVER:
            pszInv = TEXT("Invalid Display Driver");
            break;
        case EXEC_INVALID_OLD_DISPLAY_DRIVER:
            pszInv = TEXT("Old Display Driver");
            break;
        case EXEC_INVALID_16COLOR_DISPLAY_MODE:
            pszInv = TEXT("16 color mode not supported");
            break;
        case EXEC_INVALID_DISPLAY_MODE:
            pszInv = TEXT("Invalid display mode");
            break;
        case EXEC_INVALID_CONFIGURATION:
            pszInv = TEXT("Invalid configuration");
            break;
        default:
            psz = TEXT("*** Invalid *** Invalid mode");
            break;
        }

        KdPrint(("\t\t sub invlid mode : %ws", pszInv));
        KdPrint(("\n\n", psz));
    }
}
}

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMultiMon::_DeskToPreview(LPRECT in, LPRECT out)
{
    out->left   = _DeskOff.x + MulDiv(in->left   - _rcDesk.left,_DeskScale,1000);
    out->top    = _DeskOff.y + MulDiv(in->top    - _rcDesk.top, _DeskScale,1000);
    out->right  = _DeskOff.x + MulDiv(in->right  - _rcDesk.left,_DeskScale,1000);
    out->bottom = _DeskOff.y + MulDiv(in->bottom - _rcDesk.top, _DeskScale,1000);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMultiMon::_OffsetPreviewToDesk(LPRECT in, LPRECT out)
{
    int x, y;

    // Scale preview rects back to desk size
    x = _rcDesk.left + MulDiv(in->left - _DeskOff.x,1000,_DeskScale);
    y = _rcDesk.top  + MulDiv(in->top  - _DeskOff.y,1000,_DeskScale);

    // Figure out how much to offset
    x = x - out->left;
    y = y - out->top;

    OffsetRect(out, x, y);
}


//-----------------------------------------------------------------------------
int CMultiMon::_SaveSettings(CDeviceSettings *rgpds[], ULONG numDevices, HWND hDlg, DWORD dwSet)
{
    int     iRet = 0;
    ULONG   iDevice;

    for (iDevice = 0; iDevice < numDevices; iDevice++)
    {
        // BUGBUG - we should only save the settings for devices that have
        // changed.

        int iResult = rgpds[iDevice]->SaveSettings(dwSet);
        if (iResult != DISP_CHANGE_SUCCESSFUL)
        {
            if (iResult == DISP_CHANGE_RESTART)
            {
                iRet = iResult;
                continue;
            }
            else
            {
                FmtMessageBox(hDlg,
                              MB_ICONEXCLAMATION,
                              IDS_CHANGE_SETTINGS,
                              IDS_CHANGESETTINGS_FAILED);

                ASSERT(iResult < 0);
                return iResult;
            }
        }

    }

    return iRet;
}



INT_PTR CALLBACK KeepNewDlgProc(HWND hDlg, UINT message , WPARAM wParam, LPARAM lParam)
{
    UINT_PTR idTimer = 0;
    HICON hicon;
    TCHAR szRevert[100];
    TCHAR szString[120];

    switch(message)
    {
        case WM_INITDIALOG:

            hicon = LoadIcon(NULL, IDI_QUESTION);
            if (hicon)
                SendDlgItemMessage(hDlg, IDC_BIGICON, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);

            LoadString(hInstance, IDS_REVERTBACK, szRevert, SIZEOF(szRevert));
            wsprintf(szString, szRevert, 15);
            SetDlgItemText(hDlg, IDC_COUNTDOWN, szString);
            idTimer = SetTimer(hDlg, 15, 1000, NULL);

            SetFocus(GetDlgItem(hDlg, IDNO));

            // FALSE so that the focus set above is kept
            return FALSE;

        case WM_DESTROY:

            // raymondc - this code is dead; idTimer is initialized to zero
            // fortunately, timers are automatically killed at window destruction
            // if (idTimer)
            //    KillTimer(hDlg, idTimer);
            hicon = (HICON)SendDlgItemMessage(hDlg, IDC_BIGICON, STM_GETIMAGE, IMAGE_ICON, 0);
            if (hicon)
                DestroyIcon(hicon);
            break;

        case WM_TIMER:

            KillTimer(hDlg, wParam);
            LoadString(hInstance, IDS_REVERTBACK, szRevert, SIZEOF(szRevert));
            wsprintf(szString, szRevert, wParam - 1);
            SetDlgItemText(hDlg, IDC_COUNTDOWN, szString);
            idTimer = SetTimer(hDlg, wParam - 1, 1000, NULL);

            if (wParam == 1)
                EndDialog(hDlg, IDNO);

            break;

        case WM_COMMAND:

            EndDialog(hDlg, wParam);
            break;

        default:

            return FALSE;
    }
    return TRUE;
}

BOOL CMultiMon::_RebuildDisplaySettings(BOOL bComplete)
{
    BOOL result = TRUE;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    for (ULONG iDevice = 0; iDevice < _NumDevices; iDevice++)
    {    
        delete _Devices[iDevice].pds;
        _Devices[iDevice].pds = new CDeviceSettings();
    }
    RedrawWindow(_hDlg, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);

    SetCursor(hcur);
    return result;
}

int CMultiMon::GetNumberOfAttachedDisplays()
{
    int nDisplays = 0;

    for (ULONG iDevice = 0; iDevice < _NumDevices; iDevice++)
    {
        if (_Devices[iDevice].pds->IsAttached())
            nDisplays++;
    }
    return nDisplays;
}

BOOL CMultiMon::_AnyColorChange(CDeviceSettings *rgpds[], ULONG numDevices)
{
    for (ULONG iDevice = 0; iDevice < numDevices; iDevice++)
    {
        if (rgpds[iDevice]->IsAttached() && rgpds[iDevice]->IsColorChanged())
            return TRUE;
    }
    return FALSE;
}

/* static */ BOOL CMultiMon::_IsSingleToMultimonChange(CDeviceSettings *rgpds[], 
                                                       ULONG numDevices)
{
    int nAttached = 0;
    int nOrgAttached = 0;
            
    for (ULONG iDevice = 0; 
         (iDevice < numDevices) && (nOrgAttached <= 1); 
         iDevice++)
    {
        if (rgpds[iDevice]->IsOrgAttached())
            nOrgAttached++;
        if (rgpds[iDevice]->IsAttached())
            nAttached++;
    }

    return ((nOrgAttached <= 1) && (nAttached > 1));
}

BOOL CMultiMon::_AnyChange(CDeviceSettings *rgpds[], ULONG numDevices)
{
   for (ULONG iDevice = 0; iDevice < numDevices; iDevice++)
   {
       if (rgpds[iDevice]->IsAttached() && rgpds[iDevice]->bIsModeChanged())
       {
           return TRUE;
       }
   }

   return FALSE;
}

BOOL CMultiMon::_QueryForceSmallFont()
{
    for (ULONG iDevice = 0; iDevice < _NumDevices; iDevice++)
    {
        if ((_Devices[iDevice].pds->IsAttached()) &&
            (!_Devices[iDevice].pds->IsSmallFontNecessary()))
        {
            return FALSE;
        }
    }
    return TRUE;
}

LPTSTR  CMultiMon::_FormatMessageInvoke(LPCTSTR pcszFormat, va_list *argList)

{
    LPTSTR  pszOutput;

    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                      pcszFormat,
                      0, 0,
                      reinterpret_cast<LPTSTR>(&pszOutput), 0,
                      argList) == 0)
    {
        pszOutput = NULL;
    }
    return(pszOutput);
}

LPTSTR  CMultiMon::_FormatMessageWrap(LPCTSTR pcszFormat, ...)

{
    LPTSTR      pszOutput;
    va_list     argList;

    va_start(argList, pcszFormat);
    pszOutput = _FormatMessageInvoke(pcszFormat, &argList);
    va_end(argList);
    return(pszOutput);
}

void CMultiMon::_GetDisplayName(PMULTIMON_DEVICE pDevice, LPTSTR pszDisplay, DWORD cchSize)
{
    LPTSTR  pszFormattedOutput;
    TCHAR   szMonitor[140];
    TCHAR   szDisplayFormat[40];

    LoadString(hInstance, IDS_DISPLAYFORMAT, szDisplayFormat, SIZEOF(szDisplayFormat));

    pDevice->pds->GetMonitorName(szMonitor);

    pszFormattedOutput = _FormatMessageWrap(szDisplayFormat,
                                            pDevice->DisplayIndex,
                                            szMonitor,
                                            pDevice->DisplayDevice.DeviceString);
    lstrcpyn(pszDisplay, pszFormattedOutput, cchSize);
    DirectLocalFree(pszFormattedOutput);
}


void CMultiMon::_OnAdvancedClicked()
{
    BOOL bCanBePruned, bIsPruningReadOnly;
    BOOL bBeforeIsPruningOn, bAfterIsPruningOn;

    _pCurDevice->pds->GetPruningMode(&bCanBePruned, 
                                     &bIsPruningReadOnly, 
                                     &bBeforeIsPruningOn);

    _DoAdvancedSettingsSheet();

    if (bCanBePruned && !bIsPruningReadOnly)
    {
        _pCurDevice->pds->GetPruningMode(&bCanBePruned, 
                                         &bIsPruningReadOnly, 
                                         &bAfterIsPruningOn);
        if (bBeforeIsPruningOn != bAfterIsPruningOn)
        {
            //
            // pruning mode has changed - update the UI
            //
            _InitUI();
            _UpdateUI();
        }
    }
}


//-----------------------------------------------------------------------------
void CMultiMon::_DoAdvancedSettingsSheet()
{
#ifndef WINNT
    HINSTANCE hDesk16 = LoadLibrary16( "DeskCp16.Dll" );
    FARPROC16 pDesk16 = (FARPROC16)( hDesk16?
                        GetProcAddress16( hDesk16, "CplApplet" ) : NULL );
#endif
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETPAGE psp;
    HPSXA hpsxa = NULL;
    HPSXA hpsxaOEM = NULL;
    INT_PTR iResult = 0;
    TCHAR szDisplay[140 + 256 + 20];  //Monitor-name and Adapter Properties.
    TCHAR szMonitor[140];
    TCHAR szDisplayFormat[35];

    // Create the "Monitor-name and Adapter-name properties" string to be used as the title for these
    // property sheets.
    LoadString(hInstance, IDS_ADVDIALOGTITLE, szDisplayFormat, SIZEOF(szDisplayFormat));

    _pCurDevice->pds->GetMonitorName(szMonitor);

    wsprintf(szDisplay,
             szDisplayFormat,
             szMonitor,
             _pCurDevice->DisplayDevice.DeviceString);

    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = GetParent(_hDlg);
    psh.hInstance = hInstance;
    psh.pszCaption = szDisplay;
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = hInstance;

    psp.pfnDlgProc = GeneralPageProc;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_GENERAL);
    psp.lParam = (LPARAM)_QueryForceSmallFont();

    if (rPages[psh.nPages] = CreatePropertySheetPage(&psp))
        psh.nPages++;

#ifndef WINNT
    ATOM AtomDevice = GlobalAddAtom((char *)&_pCurDevice->DisplayDevice.DeviceName);
    if( pDesk16 && CallCPLEntry16( hDesk16, pDesk16, NULL, CPL_INIT, (LPARAM)AtomDevice, 0 ) )
    {
        // or just add the default page
        SHAddPages16( NULL, "DESKCP16.DLL,GetAdapterPage",
                      _AddDisplayPropSheetPage, (LPARAM)&psh );

        //
        // only add the monitor tab iff a monitor exists
        //
        if (_pCurDevice->DisplayDevice.DeviceName[0])
        {
            TCHAR szMonitor[140];

            if (_pCurDevice->pds->GetMonitorName(szMonitor))
            {
                SHAddPages16( NULL, "DESKCP16.DLL,GetMonitorPage",
                          _AddDisplayPropSheetPage, (LPARAM)&psh );
            }
        }

        SHAddPages16( NULL, "DESKCP16.DLL,GetPerformancePage",
                      _AddDisplayPropSheetPage, (LPARAM)&psh );
    }
#endif

    IDataObject * pdo = NULL;
    _pCurDevice->pds->QueryInterface(IID_IDataObject, (LPVOID *) &pdo);

    //
    // load any extensions from the registry
    //

    //
    // load the generic (non hardware specific) extensions
    //
    if( ( hpsxa = SHCreatePropSheetExtArrayEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Device"), 8, pdo) ) != NULL )
    {
        SHAddFromPropSheetExtArray( hpsxa, _AddDisplayPropSheetPage, (LPARAM)&psh );
    }

    //
    // load the hardware-specific extensions
    //
    // NOTE it is very important to load the OEM extensions *after* the
    // generic extensions some HW extensions expect to be the last tabs
    // in the propsheet (right before the settings tab)
    //
    // BUGBUG - we may need a way to NOT load the vendor extensions in case
    // they break our applet.
    //

#ifdef WINNT
    if( ( hpsxaOEM = SHCreatePropSheetExtArrayEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_CONTROLSFOLDER TEXT("\\Display"), 8, pdo) ) != NULL )
    {
        SHAddFromPropSheetExtArray( hpsxaOEM, _AddDisplayPropSheetPage, (LPARAM)&psh );
    }
#else
    if( ( hpsxaOEM = SHCreatePropSheetExtArrayEx( HKEY_LOCAL_MACHINE, _pCurDevice->DisplayDevice.DeviceKey, 8, pdo) ) != NULL )
    {
        SHAddFromPropSheetExtArray( hpsxaOEM, _AddDisplayPropSheetPage, (LPARAM)&psh );
    }
#endif
    //
    // add a fake settings page to fool OEM extensions (must be last)
    //
    if (hpsxa || hpsxaOEM)
    {
        AddFakeSettingsPage(&psh);
    }

    if (psh.nPages)
    {
        iResult = PropertySheet(&psh);
    }

    _GetDisplayName(_pCurDevice, szDisplay, ARRAYSIZE(szDisplay));

    if (_NumDevices == 1)
    {
        //Set the name of the primary in the static text
        //strip the first token off (this is the number we dont want it)
        TCHAR *pch;
        for (pch=szDisplay; *pch && *pch != TEXT(' '); pch++);
        for (;*pch && *pch == TEXT(' '); pch++);
        SetDlgItemText(_hDlg, IDC_DISPLAYTEXT, pch);
    }
    else
    {
        ComboBox_DeleteString(_hwndList, _pCurDevice->ComboBoxItem);
        ComboBox_InsertString(_hwndList, _pCurDevice->ComboBoxItem, szDisplay);
        ComboBox_SetItemData(_hwndList, _pCurDevice->ComboBoxItem, (DWORD_PTR)_pCurDevice);
        ComboBox_SetCurSel(_hwndList, _pCurDevice->ComboBoxItem);
    }

    if( hpsxa )
        SHDestroyPropSheetExtArray( hpsxa );
    if( hpsxaOEM )
        SHDestroyPropSheetExtArray( hpsxaOEM );
    if (pdo)
        pdo->Release();

#ifndef WINNT
    if (pDesk16)
        CallCPLEntry16( hDesk16, pDesk16, NULL, CPL_EXIT, 0, 0 );
    if (AtomDevice)
        GlobalDeleteAtom(AtomDevice);
    if( hDesk16 )
        FreeLibrary16( hDesk16 );
#endif

    if ((iResult == ID_PSRESTARTWINDOWS) || (iResult == ID_PSREBOOTSYSTEM))
    {
        PropSheet_CancelToClose(GetParent(_hDlg));

        if (iResult == ID_PSREBOOTSYSTEM)
            PropSheet_RebootSystem(ghwndPropSheet);
        else
            PropSheet_RestartWindows(ghwndPropSheet);
    }

    //
    // BUGBUG
    // Reset the dirty flag based on what the extensions did.
    //

    //
    // Reset the controls in case someone changed the selected mode.
    //

    UpdateActiveDisplay(NULL);
}

//-----------------------------------------------------------------------------
void CMultiMon::UpdateActiveDisplay(PMULTIMON_DEVICE pDevice, BOOL bRepaint /*=TRUE*/)
{
    HWND hwndC;

    _InSetInfo++;

    if (pDevice == NULL)
        pDevice = (PMULTIMON_DEVICE)ComboBox_GetItemData(_hwndList, ComboBox_GetCurSel(_hwndList));
    else
        ComboBox_SetCurSel(_hwndList, pDevice->ComboBoxItem);

    if (pDevice && pDevice != (PMULTIMON_DEVICE)CB_ERR)
    {
        hwndC = GetCurDeviceHwnd();

        // The Current Device has changed, so, force recreating the bitmap the next time 
        // we paint the monitor on the preview window.
        _pCurDevice->w = pDevice->w = 0;

        _pCurDevice = pDevice;

        if (hwndC)
            RedrawWindow(hwndC, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);

        hwndC = GetCurDeviceHwnd();
        if (hwndC)
            RedrawWindow(hwndC, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);

        if(_NumDevices > 1)
        {
            //
            // Update the two check box windows
            //
#ifdef WINNT
            CheckDlgButton(_hDlg, IDC_DISPLAYPRIME, _pCurDevice->pds->IsPrimary());
            EnableWindow(GetDlgItem(_hDlg, IDC_DISPLAYPRIME),
                     _pCurDevice->pds->IsAttached() &&
                     !_pCurDevice->pds->IsPrimary());
#endif
            CheckDlgButton(_hDlg, IDC_DISPLAYUSEME, _pCurDevice->pds->IsAttached());
            EnableWindow(GetDlgItem(_hDlg, IDC_DISPLAYUSEME),
                     !_bNoAttach && !_pCurDevice->pds->IsPrimary());
        }

        //
        // Reset the values for the list boxes, and then repaint it
        //
        if(bRepaint)
        {
            _InitUI();
            _UpdateUI();
        }
    }
    else
    {
        //
        // No display device !
        //
        TraceMsg(TF_WARNING, "**** UpdateActiveDisplay: No display device!!!!");
        ASSERT(FALSE);
    }

    _InSetInfo--;
}

// ---------------------------------------------------------------------------
// Initialize the resolution and color UI widgets
//

void CMultiMon::_InitUI()
{
    int       i;
    int       Color;

    //
    // Update the Color list
    //

    TraceMsg(TF_FUNC, "_InitUI() -- Color list");

    SendDlgItemMessage(_hDlg, IDC_COLORBOX, CB_RESETCONTENT, 0, 0);

    if (_pCurDevice->ColorList)
    {
        LocalFree(_pCurDevice->ColorList);
        _pCurDevice->ColorList = NULL;
    }
    _pCurDevice->cColors = _pCurDevice->pds->GetColorList(NULL, &_pCurDevice->ColorList);

    for (i = 0; i < _pCurDevice->cColors; i++)
    {
        TCHAR  achColor[50];
        DWORD  idColor;

        Color = (int) *(_pCurDevice->ColorList + i);

        //
        // convert bit count to number of colors and make it a string
        //

        switch (Color)
        {
        case 32: idColor = ID_DSP_TXT_TRUECOLOR32; break;
        case 24: idColor = ID_DSP_TXT_TRUECOLOR24; break;
        case 16: idColor = ID_DSP_TXT_16BIT_COLOR; break;
        case 15: idColor = ID_DSP_TXT_15BIT_COLOR; break;
        case  8: idColor = ID_DSP_TXT_8BIT_COLOR; break;
        case  4: idColor = ID_DSP_TXT_4BIT_COLOR; break;
        default:
            ASSERT(FALSE);
        }

        LoadString(hInstance, idColor, achColor, SIZEOF(achColor));
        SendDlgItemMessage(_hDlg, IDC_COLORBOX, CB_INSERTSTRING, i, (LPARAM)achColor);
    }

    //
    // Update the screen Size List
    //

    TraceMsg(TF_FUNC, "_InitUI() -- Screen Size list");

    if (_pCurDevice->ResolutionList)
    {
        LocalFree(_pCurDevice->ResolutionList);
        _pCurDevice->ResolutionList = NULL;
    }
    _pCurDevice->cResolutions =
        _pCurDevice->pds->GetResolutionList(-1, &_pCurDevice->ResolutionList);

    SendDlgItemMessage(_hDlg, IDC_SCREENSIZE, TBM_SETRANGE, TRUE,
                       MAKELONG(0, _pCurDevice->cResolutions - 1));

    TraceMsg(TF_FUNC, "_InitUI() -- Res MaxRange = %d", _pCurDevice->cResolutions - 1);


    //
    // Reset the indices since they are no longer valid
    //

    _iResolution = -1;
    _iColor = -1;

    //EnableWindow(GetDlgItem(_hDlg, IDC_COLORBOX), _fOrgAttached);
    //EnableWindow(GetDlgItem(_hDlg, IDC_SCREENSIZE), _fOrgAttached);
}

// ---------------------------------------------------------------------------
// Update the resolution and color UI widgets
//
void CMultiMon::_UpdateUI(int FocusToCtrlID)
{
    int  i;
    POINT Res;
    int   Color;
    BOOL bRepaint;

    //
    // Get the current values
    //

    _pCurDevice->pds->GetCurResolution(&Res);
    Color = _pCurDevice->pds->GetCurColor();

    //
    // Update the color listbox
    //

    TraceMsg(TF_FUNC, "_UpdateUI() -- Set Color %d", Color);

    for (i=0; i<_pCurDevice->cColors; i++)
    {
        if (Color == (int) *(_pCurDevice->ColorList + i))
        {
            TraceMsg(TF_FUNC, "_UpdateUI() -- Set Color index %d", i);

            if (_iColor == i)
            {
                TraceMsg(TF_FUNC, "_UpdateUI() -- Set Color index %d - is current", i);
                break;
            }

            HBITMAP hbm, hbmOld;
            int iBitmap = IDB_COLOR4DITHER;
            HDC hdc = GetDC(NULL);
            int bpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);

            SendDlgItemMessage(_hDlg, IDC_COLORBOX, CB_SETCURSEL, i, 0);

            if (Color <= 4)
                iBitmap = IDB_COLOR4;
            else if (bpp >= 16)
            {
                if (Color <= 8)
                    iBitmap = IDB_COLOR8;
                else if (Color <= 16)
                    iBitmap = IDB_COLOR16;
                else
                    iBitmap = IDB_COLOR24;
            }

            ReleaseDC(NULL, hdc);

            hbm = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(iBitmap), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
            if (hbm)
            {
                hbmOld = (HBITMAP) SendDlgItemMessage(_hDlg, IDC_COLORSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm);
                if (hbmOld)
                {
                    DeleteObject(hbmOld);
                }
            }

            _iColor = i;
            break;
        }
    }

    if (i == _pCurDevice->cColors)
    {
        TraceMsg(TF_ERROR, "_UpdateUI -- !!! inconsistent color list !!!");
    }


    TraceMsg(TF_FUNC, "_UpdateUI() -- Set Resolution %d %d", Res.x, Res.y);

    //
    // Update the resolution string
    //

    {
        TCHAR achStr[80];
        TCHAR achRes[120];

        LoadString(hInstance, ID_DSP_TXT_XBYY, achStr, SIZEOF(achStr));
        wsprintf(achRes, achStr, Res.x, Res.y);

        SendDlgItemMessage(_hDlg, IDC_RESXY, WM_SETTEXT, 0, (LPARAM)achRes);
    }

    //
    // Update the resolution slider
    //

    for (i=0; i<_pCurDevice->cResolutions; i++)
    {
        if ( (Res.x == (*(_pCurDevice->ResolutionList + i)).x) &&
             (Res.y == (*(_pCurDevice->ResolutionList + i)).y) )
        {
            TraceMsg(TF_FUNC, "_UpdateUI() -- Set Resolution index %d", i);

            if (_iResolution == i)
            {
                TraceMsg(TF_FUNC, "_UpdateUI() -- Set Resolution index %d - is current", i);
                break;
            }

            SendDlgItemMessage(_hDlg, IDC_SCREENSIZE, TBM_SETPOS, TRUE, i);
            break;
        }
    }

    if (i == _pCurDevice->cResolutions)
    {
        TraceMsg(TF_ERROR, "_UpdateUI -- !!! inconsistent color list !!!");
    }

    bRepaint = (i != _iResolution);
    _iResolution = i;

    //
    // If the resolution has changed, we have to repaint the preview window
    // Set the focus back to the trackbar after the repaint so any further
    // kb events will be send to it rather than the preview window
    //
    if (bRepaint) {
        SendMessage(_hDlg, MM_REDRAWPREVIEW, 0, 0);
    }

    if (FocusToCtrlID != 0) {
        SetFocus(GetDlgItem(_hDlg, FocusToCtrlID));
    }
}

//----------------------------------------------------------------------------
//
//  SetPrimary()
//
//----------------------------------------------------------------------------

BOOL
CMultiMon::SetPrimary(
    PMULTIMON_DEVICE pDevice)
{
    //
    // Check if state is already set.
    //

    if (pDevice == _pPrimaryDevice)
    {
        pDevice->pds->SetPrimary(TRUE);
        return TRUE;
    }

    ASSERT(pDevice->pds->IsAttached());

    //
    // make sure the primary is in a valid mode
    //
    // BUGBUG
    //_VerifyPrimaryMode(FALSE);


    //
    // make sure the primary is in a mode >= 256 color
    //

    if (pDevice->pds->GetCurColor() < 8)
    {
        for (ULONG iDevice = 0; iDevice < _NumDevices; iDevice++)
        {
            _Devices[iDevice].pds->SetAttached(FALSE);
        }

        pDevice->pds->SetAttached(TRUE);

        RedrawWindow(GetDlgItem(_hDlg, IDC_DISPLAYDESK), NULL, NULL,
            RDW_ALLCHILDREN | RDW_ERASE | RDW_INVALIDATE);
    }

    _pPrimaryDevice->pds->SetPrimary(FALSE);
    pDevice->pds->SetPrimary(TRUE);
    _pPrimaryDevice = pDevice;

    SetDirty();

    return TRUE;
}

//----------------------------------------------------------------------------
//
//  SetMonAttached()
//
//----------------------------------------------------------------------------

BOOL
CMultiMon::SetMonAttached(
    PMULTIMON_DEVICE pDevice,
    BOOL bSetAttached,
    BOOL bForce,
    HWND hwnd)
{
    if (pDevice->pds->IsAttached() == bSetAttached)
    {
        return TRUE;
    }

    if (bSetAttached)
    {
        //
        // Make sure this device actually has a rectangle.
        // If it does not (not configured in the registry, then we need
        // to put up a popup and ask the user to configure the device.
        //

        // BUGBUG
        // RECT rcPos;
        // pDevice->pds->GetCurPosition(&rcPos);
        // if (IsRectEmpty(&rcPos))
        //     return FALSE;

        if (hwnd)
        {
            //
            // Check to see if we should ask the user about enabling this device
            //

            if (bForce == FALSE)
            {
                TCHAR szTurnItOn[400];
                TCHAR szTurnOnTitleFormat[30];
                TCHAR szTurnOnTitle[110];
                LPTSTR pstr = szTurnItOn;

                LoadString(hInstance, IDS_TURNONTITLE, szTurnOnTitleFormat, SIZEOF(szTurnOnTitleFormat));
                wsprintf(szTurnOnTitle, szTurnOnTitleFormat, pDevice->DisplayIndex);

                if (GetNumberOfAttachedDisplays() == 1)
                {
                    LoadString(hInstance, IDS_TURNONMSG, szTurnItOn, SIZEOF(szTurnItOn));
                    pstr += lstrlen(szTurnItOn);
                }

                LoadString(hInstance, IDS_TURNITON, pstr, SIZEOF(szTurnItOn));

                if (ShellMessageBox(hInstance, hwnd, szTurnItOn, szTurnOnTitle,
                                    MB_YESNO | MB_ICONINFORMATION) != IDYES)
                {
                   return FALSE;
                }
            }
        }

        pDevice->pds->SetAttached(TRUE);

    }
    else  // (bSetAttached == FALSE)
    {
        //
        // Can't detach if we have only one device or it's the primary.
        // The UI should disable this situation
        //

        if ((GetNumberOfAttachedDisplays() == 1) ||
            pDevice->pds->IsPrimary())
        {
            ASSERT(FALSE);
        }

        pDevice->pds->SetAttached(FALSE);
    }

    SetDirty();

    return TRUE;


#if 0
#ifndef WINNT
        // BUGBUG Memphis stuff
        //else
        //{
        //    _pPrimaryDevice->pds->SetMode(640,480,8);
        //}
#endif
#endif

}

//----------------------------------------------------------------------------
//
//  SetDirty
//
//----------------------------------------------------------------------------
void CMultiMon::SetDirty(BOOL bDirty)
{
    _bDirty = bDirty;

    if (_bDirty)
    {
        PostMessage(GetParent(_hDlg), PSM_CHANGED, (WPARAM)_hDlg, 0L);
    }
}

//-----------------------------------------------------------------------------

void CMultiMon::_CleanupRects(HWND hwndP)
{
    int   n;
    HWND  hwndC;
    DWORD arcDev[MONITORS_MAX];
    RECT arc[MONITORS_MAX];
    DWORD iArcPrimary = 0;

    RECT rc;
    RECT rcU;
    int   i;
    RECT rcPrev;
    int sx,sy;
    int x,y;

    //
    // get the positions of all the windows
    //

    n = 0;

    for (ULONG iDevice = 0; iDevice < _NumDevices; iDevice++)
    {
        PMULTIMON_DEVICE pDevice = &_Devices[iDevice];

        hwndC = GetDlgItemP(hwndP, (INT_PTR) pDevice);

        if (hwndC != NULL)
        {
            RECT rcPos;

            TraceMsg(TF_GENERAL, "_CleanupRects start Device %08lx, Dev = %d, hwnd = %08lx",
                     pDevice, iDevice, hwndC);

            ShowWindow(hwndC, SW_SHOW);

            GetWindowRect(hwndC, &arc[n]);
            MapWindowPoints(NULL, hwndP, (POINT FAR*)&arc[n], 2);

            pDevice->pds->GetCurPosition(&rcPos);

            _OffsetPreviewToDesk(&arc[n], &rcPos);
            arc[n] = rcPos;
            arcDev[n] = iDevice;

            // TEMP
            // For non-atached devices, make sure they end up to the right
            // Eventually, non-attached devices should be showed aligned on the
            // right hand side of the window.

            if (!pDevice->pds->IsAttached())
            {
                OffsetRect(&arc[n], 10000, 0);
            }

            if (pDevice->pds->IsPrimary())
            {
                TraceMsg(TF_GENERAL, "_CleanupRects primary Device %08lx", pDevice);

                iArcPrimary = n;
            }


            n++;
        }
    }

    //
    // cleanup the rects
    //
    AlignRects(arc, n, iArcPrimary, CUDR_NORMAL);

    //
    // Get the union.
    //
    SetRectEmpty(&rcU);
    for (i=0; i<n; i++)
        UnionRect(&rcU, &rcU, &arc[i]);
    GetClientRect(hwndP, &rcPrev);

    //
    // only rescale if the new desk hangs outside the preview area.
    // or is too small
    //

    _DeskToPreview(&rcU, &rc);
    x = ((rcPrev.right  - rcPrev.left)-(rc.right  - rc.left))/2;
    y = ((rcPrev.bottom - rcPrev.top) -(rc.bottom - rc.top))/2;

    if (rcU.left < 0 || rcU.top < 0 || x < 0 || y < 0 ||
        rcU.right > rcPrev.right || rcU.bottom > rcPrev.bottom ||
        (x > (rcPrev.right-rcPrev.left)/8 &&
         y > (rcPrev.bottom-rcPrev.top)/8))
    {
        _rcDesk = rcU;
        sx = MulDiv(rcPrev.right  - rcPrev.left - 16,1000,_rcDesk.right  - _rcDesk.left);
        sy = MulDiv(rcPrev.bottom - rcPrev.top  - 16,1000,_rcDesk.bottom - _rcDesk.top);

        _DeskScale = min(sx,sy) * 2 / 3;
        _DeskToPreview(&_rcDesk, &rc);
        _DeskOff.x = ((rcPrev.right  - rcPrev.left)-(rc.right  - rc.left))/2;
        _DeskOff.y = ((rcPrev.bottom - rcPrev.top) -(rc.bottom - rc.top))/2;
    }

    //
    // Show all the windows and save them all to the devmode.
    //
    for (i=0; i < n; i++)
    {
        RECT rcPos;
        POINT ptPos;

        _Devices[arcDev[i]].pds->GetCurPosition(&rcPos);
        hwndC = GetDlgItemP(hwndP, (INT_PTR) &_Devices[arcDev[i]]);

        _DeskToPreview(&arc[i], &rc);

        rc.right =  MulDiv(RECTWIDTH(rcPos),  _DeskScale, 1000);
        rc.bottom = MulDiv(RECTHEIGHT(rcPos), _DeskScale, 1000);

        TraceMsg(TF_GENERAL, "_CleanupRects set Dev = %d, hwnd = %08lx", arcDev[i], hwndC);
        TraceMsg(TF_GENERAL, "_CleanupRects window pos %d,%d,%d,%d", rc.left, rc.top, rc.right, rc.bottom);

        SetWindowPos(hwndC,
                     NULL,
                     rc.left,
                     rc.top,
                     rc.right,
                     rc.bottom,
                     SWP_NOZORDER);

        ptPos.x = arc[i].left;
        ptPos.y = arc[i].top;

        _Devices[arcDev[i]].pds->SetCurPosition(&ptPos);
    }

    TraceMsg(TF_GENERAL, "");
}

void CMultiMon::_ConfirmPositions()
{
    ASSERT (_NumDevices > 1);

    PMULTIMON_DEVICE pDevice;
    ULONG iDevice;
    
    for (iDevice = 0; iDevice < _NumDevices; iDevice++)
    {
        pDevice = &_Devices[iDevice];
        if (pDevice->pds->IsOrgAttached())
        {
            RECT rcOrg, rcCur;
            
            pDevice->pds->GetCurPosition(&rcCur);
            pDevice->pds->GetOrgPosition(&rcOrg);
            if ((rcCur.left != rcOrg.left) ||
                (rcCur.top != rcOrg.top))
            {
                POINT ptOrg;
    
                ptOrg.x = rcCur.left;
                ptOrg.y = rcCur.top;
                pDevice->pds->SetOrgPosition(&ptOrg);
                SetDirty(TRUE);
            }
        }
    }
}

void CMultiMon::GetMonitorPosition(PMULTIMON_DEVICE pDevice, HWND hwndP, PPOINT ptPos)
{
    int iPrimary;
    HWND hwndC;
    RECT rcPos;
    RECT arc[MONITORS_MAX];
    int i;

    for (ULONG iDevice = 0; iDevice < _NumDevices; iDevice++)
    {
        PMULTIMON_DEVICE pDevice = &_Devices[iDevice];

        hwndC = GetDlgItemP(hwndP, (INT_PTR) pDevice);
        ASSERT(hwndC);

        GetWindowRect(hwndC, &arc[iDevice]);
        MapWindowPoints(NULL, hwndP, (POINT FAR*)&arc[iDevice], 2);

        pDevice->pds->GetCurPosition(&rcPos);

        _OffsetPreviewToDesk(&arc[iDevice], &rcPos);
        arc[iDevice] = rcPos;

        if (pDevice->pds->IsPrimary()) {
            iPrimary = iDevice;
        }
    }

    AlignRects(arc, iDevice, iPrimary, CUDR_NORMAL);

    i = (int)(pDevice - _Devices);
    ptPos->x = arc[i].left;
    ptPos->y = arc[i].top;
}

BOOL CMultiMon::HandleMonitorChange(HWND hwndP, BOOL bMainDlg, BOOL bRepaint /*=TRUE*/)
{
    if (!bMainDlg && _InSetInfo)
        return FALSE;

    SetDirty();

    if (bMainDlg)
        BringWindowToTop(hwndP);
    _CleanupRects(GetParent(hwndP));
    UpdateActiveDisplay(_pCurDevice, bRepaint);
    return TRUE;
}

BOOL CMultiMon::RegisterPreviewWindowClass(WNDPROC pfnWndProc)
{
    TraceMsg(TF_GENERAL, "InitMultiMonitorDlg\n");
    WNDCLASS         cls;

    cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
    cls.hIcon          = NULL;
    cls.lpszMenuName   = NULL;
    cls.lpszClassName  = TEXT("Monitor32");
    cls.hbrBackground  = (HBRUSH)(COLOR_DESKTOP + 1);
    cls.hInstance      = hInstance;
    cls.style          = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    cls.lpfnWndProc    = pfnWndProc;
    cls.cbWndExtra     = SIZEOF(LPVOID);
    cls.cbClsExtra     = 0;

    return RegisterClass(&cls);
}

LRESULT CALLBACK DeskWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwRefData);

// This function is called from desk.c; Hence extern "C".
// This function is needed to determine if we need to use the single monitor's dialog 
// or multi-monitor's dialog template at the time of starting the control panel applet.
int ComputeNumberOfDisplayDevices()
{
    int iNumberOfDevices = 0;
    CMultiMon   *pMultiMon = new CMultiMon;
    int iDevice;

    // Enumerate all display devices to count the number of valid devices.
    iNumberOfDevices = pMultiMon->_EnumerateAllDisplayDevices();

    // Now that we have the number of devices, let's cleanup the device settings we
    // created in the process of enumerating above.
    for (iDevice = 0; iDevice < iNumberOfDevices; iDevice++)
        pMultiMon->_DestroyMultimonDevice(&pMultiMon->_Devices[iDevice]);

    // Let's clean up the MultiMon we allocated earlier.
    delete pMultiMon;
    
    return iNumberOfDevices;
}



BOOL CMultiMon::_InitDisplaySettings(BOOL bExport)
{
    HWND             hwndC;
    int              iItem;
    LONG             iPrimeDevice;
    TCHAR            ach[128];
    PMULTIMON_DEVICE pDevice;
    RECT             rcPrimary;

    HCURSOR hcur;

    _InSetInfo = 1;
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // Reset all the data so we can reinitialize the applet.
    //

    {
        ComboBox_ResetContent(_hwndList);
        SetRectEmpty(&_rcDesk);

        while (hwndC = GetWindow(_hwndDesk, GW_CHILD))
        {
            RemoveTrackingToolTip(hwndC);
            RemovePopupToolTip(hwndC);
            DestroyWindow(hwndC);
        }

        ShowWindow(_hwndDesk, SW_HIDE);

        if (_himl != NULL)
        {
            ImageList_Destroy(_himl);
            _himl = NULL;
        }

        //
        // Clear out all the devices.
        //
        for (ULONG iDevice = 0; iDevice < _NumDevices; iDevice++) {
            pDevice = _Devices + iDevice;
            _DestroyMultimonDevice(pDevice); 
            ZeroMemory(pDevice, sizeof(MULTIMON_DEVICE));
        }

        ZeroMemory(_Devices + _NumDevices,
                   sizeof(_Devices) - sizeof(MULTIMON_DEVICE) * _NumDevices);

        _NumDevices = 0;
    }

    //
    // Enumerate all the devices in the system.
    //
    // Note: This function computes the _NumDevices.

    _EnumerateAllDisplayDevices();
    
    if (_NumDevices == 0)
    {
        ASSERT(0);
        return FALSE;
    }
    //
    // Because we are getting the registry values, the current state of
    // the registry may be inconsistent with that of the system:
    //
    // EmumDisplayDevices will return the active primary in the
    // system, which may be different than the actual primary marked in the
    // registry
    //

    BOOL bTmpDevicePrimary  = FALSE;
    BOOL bBestDevicePrimary = FALSE;
    ULONG iDevice;

    _pPrimaryDevice = NULL;

    for (iDevice = 0; iDevice < _NumDevices; iDevice++)
    {
        //
        // First, we can pick any monitor that is attached as the primary.
        //

        if (_Devices[iDevice].pds->IsAttached())
        {
            if (_pPrimaryDevice == NULL)
            {
                _pPrimaryDevice = &_Devices[iDevice];
                TraceMsg(TF_GENERAL, "InitDisplaySettings: primary found %d\n", iDevice);
            }

            //
            // If the DISPLAY_DEVICE structure tells us this is the primary,
            // Pick this one.
            //

            if (_Devices[iDevice].DisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            {
                if (bTmpDevicePrimary)
                {
                    ASSERT(FALSE);
                }
                else
                {
                    _pPrimaryDevice = &_Devices[iDevice];
                    bTmpDevicePrimary = TRUE;
                    TraceMsg(TF_GENERAL, "InitDisplaySettings: Tmp DEVICE_PRIMARY found %d", iDevice);
                }

                //
                // Check that the position should really be 0,0
                //

                RECT pos;

                _Devices[iDevice].pds->GetCurPosition(&pos);

                if ((pos.left == 0) &&
                    (pos.top == 0))
                {
                    _pPrimaryDevice = &_Devices[iDevice];
                    TraceMsg(TF_GENERAL, "InitDisplaySettings: Best DEVICE_PRIMARY found %d", iDevice);
                }
                else
                {
                    ASSERT(FALSE);
                    TraceMsg(TF_GENERAL, "InitDisplaySettings: PRIMARY is not at 0,0");
                }
            }
        }
    }

    if (_pPrimaryDevice == NULL)
    {
        ASSERT(FALSE);
        TraceMsg(TF_GENERAL, "InitDisplaySettings: NO Attached devices !!!");

        //
        // We must be running setup - pick the first device as the primary.
        //

        _pPrimaryDevice = &_Devices[0];
    }

    _pCurDevice = _pPrimaryDevice;

    //
    // Reset the primary's variables to make sure it is a properly formated
    // primary entry.
    //

    SetMonAttached(_pPrimaryDevice, TRUE, TRUE, NULL);
    SetPrimary(_pPrimaryDevice);
    _pPrimaryDevice->pds->GetCurPosition(&rcPrimary);

#ifndef WINNT
    if (!lstrcmpi((LPTSTR)_pPrimaryDevice->DisplayDevice.DeviceString,TEXT("Standard VGA")))
        _bNoAttach = TRUE;
#endif

    //
    // compute the max image size needed for a monitor bitmap
    //
    // NOTE this must be the max size the images will *ever*
    // be we cant just take the current max size.
    // we use the client window size, a child monitor cant be larger than this.
    //
    RECT rcDesk;
    GetClientRect(_hwndDesk, &rcDesk);
    int cxImage = rcDesk.right;
    int cyImage = rcDesk.bottom;

    //
    // Create a temporary monitor bitmap 
    //
    HBITMAP hbm = NULL;
    MakeMonitorBitmap(cxImage, cyImage, NULL, &hbm, NULL, cxImage, cyImage, FALSE);

    //
    // Go through all the devices one last time to create the windows
    //
    for (iDevice = 0; iDevice < _NumDevices; iDevice++)
    {
        TCHAR szDisplay[256];
        pDevice = &_Devices[iDevice];
        MonitorData md = {0};
        RECT rcPos;
        LPVOID pWindowData = (LPVOID)this;
        pDevice->DisplayIndex = iDevice + 1;
        _GetDisplayName(pDevice, szDisplay, ARRAYSIZE(szDisplay));
        iItem = ComboBox_AddString(_hwndList, szDisplay);

        pDevice->ComboBoxItem = iItem;

        ComboBox_SetItemData(_hwndList,
                             iItem,
                             (DWORD_PTR)pDevice);

        //
        // If the monitor is part of the desktop, show it on the screen
        // otherwise keep it invisible.
        //

        wsprintf(ach, TEXT("%d"), iDevice + 1);

        // BUGBUG - does this need to be done after
        // Set the selection
        //

        if (pDevice == _pPrimaryDevice)
        {
            iPrimeDevice = iDevice;
        }

        if (!pDevice->pds->IsAttached())
        {
            // By default set the unattached monitors to the right of the primary monitor
            POINT ptPos = {rcPrimary.right, rcPrimary.top};
            pDevice->pds->SetCurPosition(&ptPos);
        }

        pDevice->pds->GetCurPosition(&rcPos);

        if (bExport)
        {
            md.dwSize = SIZEOF(MonitorData);
            if ( pDevice->pds->IsPrimary() )
                md.dwStatus |= MD_PRIMARY;
            if ( pDevice->pds->IsAttached() )
                md.dwStatus |= MD_ATTACHED;
            md.rcPos = rcPos;

            pWindowData = &md;
        }

        if (_himl == NULL)
        {
            UINT flags = ILC_COLORDDB | ILC_MASK;
            _himl = ImageList_Create(cxImage, cyImage, flags, _NumDevices, 1);
            ASSERT(_himl);
            ImageList_SetBkColor(_himl, GetSysColor(COLOR_APPWORKSPACE));
        }

        pDevice->w      = -1;
        pDevice->h      = -1;
        pDevice->himl   = _himl;
        pDevice->iImage = ImageList_AddMasked(_himl, hbm, CLR_DEFAULT);

        TraceMsg(TF_GENERAL, "InitDisplaySettings: Creating preview windows %s at %d %d %d %d",
                 ach, rcPos.left, rcPos.top, rcPos.right, rcPos.bottom);

        // HACK! Use pDevice as its own id.  Doesn't work on Win64.
        hwndC = CreateWindowEx(
                               0, // WS_EX_CLIENTEDGE,
                               TEXT("Monitor32"), ach,
                               WS_CLIPSIBLINGS | /* WS_DLGFRAME | */ WS_VISIBLE | WS_CHILD,
                               rcPos.left, rcPos.top, RECTWIDTH(rcPos), RECTHEIGHT(rcPos),
                               _hwndDesk,
                               (HMENU)pDevice,
                               hInstance,
                               pWindowData);

        ASSERT(hwndC);
        AddTrackingToolTip(pDevice, hwndC);
        AddPopupToolTip(hwndC);
    }

    ToolTip_Activate(ghwndToolTipPopup, TRUE);
    ToolTip_SetDelayTime(ghwndToolTipPopup, TTDT_INITIAL, 1000);
    ToolTip_SetDelayTime(ghwndToolTipPopup, TTDT_RESHOW, 1000);

    //  nuke the temp monitor bitmap.
    if (hbm)
        DeleteObject(hbm);

    //
    // Set the primary device as the current device
    //

    ComboBox_SetCurSel(_hwndList, iPrimeDevice);

    // Initialize all the constants and the settings fields
    _DeskScale = 1000;
    _DeskOff.x = 0;
    _DeskOff.y = 0;
    _CleanupRects(_hwndDesk);

    // Now: depends on whether we have a multimon system, change the UI
    if (_NumDevices == 1)
    {
        HWND hwndDisable;

        hwndDisable = GetDlgItem(_hDlg, IDC_MULTIMONHELP);
        ShowWindow(hwndDisable, SW_HIDE);
        ShowWindow(_hwndDesk, SW_HIDE);

        // set up bitmaps for sample screen
        _hbmScrSample = LoadMonitorBitmap( TRUE ); // let them do the desktop
        SendDlgItemMessage(_hDlg, IDC_SCREENSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)_hbmScrSample);

        // get a base copy of the bitmap for when the "internals" change
        _hbmMonitor = LoadMonitorBitmap( FALSE ); // we'll do the desktop

        //Hide the combo box, keep the static text
        ShowWindow(_hwndList, SW_HIDE);

        //Set the name of the primary in the static text
        //strip the first token off (this is the number we dont want it)
        TCHAR *pch, szDisplay[MAX_PATH];
        _GetDisplayName(_pPrimaryDevice, szDisplay, ARRAYSIZE(szDisplay));
        for (pch=szDisplay; *pch && *pch != TEXT(' '); pch++);
        for (;*pch && *pch == TEXT(' '); pch++);
        SetDlgItemText(_hDlg, IDC_DISPLAYTEXT, pch);

        // Hide the check boxes

        // Single monitors use a different dialog template now!
        if(hwndDisable = GetDlgItem(_hDlg, IDC_DISPLAYPRIME))
            ShowWindow(hwndDisable, SW_HIDE);
        if(hwndDisable = GetDlgItem(_hDlg, IDC_DISPLAYUSEME))
            ShowWindow(hwndDisable, SW_HIDE);

    }
    else if (_NumDevices > 0)
    {
        //Hide the static text, keep the combo box
        ShowWindow(GetDlgItem(_hDlg, IDC_DISPLAYTEXT), SW_HIDE);

        // Hide the Multimon version of the preview objects
        ShowWindow(GetDlgItem(_hDlg, IDC_SCREENSAMPLE), SW_HIDE);

        // In case of multiple devices, subclass the _hwndDesk window for key board support
        SetWindowSubclass(_hwndDesk, DeskWndProc, 0, (DWORD_PTR)this);
        ShowWindow(_hwndDesk, SW_SHOW);
    }

    //
    // Paint the UI.
    //

    UpdateActiveDisplay(_pCurDevice);

    //
    // Reset the cursor and leave
    //

    SetCursor(hcur);
    _InSetInfo--;

    return TRUE;
}

//
// This function enumerates all the devices and returns the number of
// devices found in the system.
//

int  CMultiMon::_EnumerateAllDisplayDevices()
{
    PMULTIMON_DEVICE pDevice;
    int iEnum;
    BOOL fSuccess;
    ULONG dwVgaPrimary = 0xFFFFFFFF;
    
    //
    // Enumerate all the devices in the system.
    //

    for (iEnum = 0; _NumDevices < MONITORS_MAX; iEnum++)
    {
        pDevice = &_Devices[_NumDevices];
        ZeroMemory(&(pDevice->DisplayDevice), sizeof(DISPLAY_DEVICE));
        pDevice->DisplayDevice.cb = sizeof(DISPLAY_DEVICE);

        fSuccess = EnumDisplayDevices(NULL, iEnum, &pDevice->DisplayDevice, 0);

        TraceMsg(TF_GENERAL, "Device %d       ", iEnum);
        TraceMsg(TF_GENERAL, "cb %d           ", pDevice->DisplayDevice.cb);
        TraceMsg(TF_GENERAL, "DeviceName %ws  ", pDevice->DisplayDevice.DeviceName);
        TraceMsg(TF_GENERAL, "DeviceString %ws", pDevice->DisplayDevice.DeviceString);
        TraceMsg(TF_GENERAL, "StateFlags %08lx", pDevice->DisplayDevice.StateFlags);

        //
        // ignore device's we cant create a DC for.
        //

        // BUGBUG WINNT - WIN98 incompatibility
#ifndef WINNT
        if (fSuccess)
        {
            pDevice->hdc = CreateDC(NULL,(LPTSTR)pDevice->DisplayDevice.DeviceName,NULL,NULL);
            fSuccess = pDevice->hdc != NULL;
        }
#endif

#ifndef WINNT
        //
        // EnumDisplayDevices is returning NO devices, this is bad
        // invent a fake device.
        //
        if (!fSuccess && iEnum == 0)
        {
            pDevice->DisplayDevice.DeviceName[0] = 0;

            LoadString(hInstance, IDS_UNKNOWNDEVICE,
                (LPTSTR)&pDevice->DisplayDevice.DeviceString[0],
                SIZEOF(pDevice->DisplayDevice.DeviceString));

            pDevice->DisplayDevice.StateFlags = DISPLAY_DEVICE_PRIMARY_DEVICE;

            gbExecMode    = EXEC_INVALID_MODE;
            gbInvalidMode = EXEC_INVALID_DISPLAY_DEVICE;

            fSuccess = TRUE;
        }
#endif

        if (!fSuccess)
        {
            TraceMsg(TF_GENERAL, "End of list\n");
            break;
        }

        //
        // We won't even include the MIRRORING drivers in the list for
        // now.
        //
        if (pDevice->DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
        {
            TraceMsg(TF_GENERAL, "Mirroring driver - skip it\n");
            continue;
        }

        //
        // dump the device software key
        //

        TraceMsg(TF_GENERAL, "DeviceKey %s", pDevice->DisplayDevice.DeviceKey);

        //
        // Create the settings for this device
        //
        pDevice->pds = new CDeviceSettings();

        if (pDevice->pds->InitSettings(&pDevice->DisplayDevice))
        {
#ifdef WINNT
            //
            // Determine if the VGA is the primary.
            // This will only happen for SETUP or BASEVIDEO
            //
            // We want to delete this device later on if we have others.
            //

            if (pDevice->DisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
            {
                CRegistrySettings crv(&pDevice->DisplayDevice.DeviceKey[0]);

                LPTSTR pszMini = crv.GetMiniPort();

                //
                // If VGA is active, then go to pass 2.
                // Otherwise, let's try to use this device
                //

                if (pszMini && (!lstrcmpi(TEXT("vga"), pszMini)))
                {
                    TraceMsg(TF_GENERAL, "EnumDevices - VGA primary\n");
                    dwVgaPrimary = _NumDevices;
                }
            }
#endif

            //
            // Add it to the list.
            //

            _NumDevices++;
        }
        else
        {
            delete pDevice->pds;
        }

    }

#ifdef WINNT
    //
    // If the primary VGA is not needed, remove it.
    //

    if ((dwVgaPrimary != 0xFFFFFFFF) &&
        (_NumDevices >= 2))
    {
        TraceMsg(TF_GENERAL, "REMOVE primary VGA device\n");

        delete _Devices[dwVgaPrimary].pds;

        _NumDevices--;
        _Devices[dwVgaPrimary] = _Devices[_NumDevices];

    }
#endif

    return(_NumDevices);  //Return the number of devices.
}


//-----------------------------------------------------------------------------
void CMultiMon::InitMultiMonitorDlg(HWND hDlg)
{
    HWND hwndSlider;
    BOOL fSucceeded;

    _hDlg = hDlg;
    _hwndDesk = GetDlgItem(_hDlg, IDC_DISPLAYDESK);
    _hwndList = GetDlgItem(_hDlg, IDC_DISPLAYLIST);

    Look_InitDPIglobals();  //Defined in lookdlg.c

    hwndSlider = GetDlgItem(hDlg, IDC_SCREENSIZE);
    ASSERT(hwndSlider != NULL);

    fSucceeded = SetWindowSubclass(hwndSlider, SliderSubWndProc, 0, NULL);
    ASSERT(fSucceeded);


#ifdef WINNT
    //
    // Determine in what mode we are running the applet before getting information
    //
    _vPreExecMode();
#endif

    // Create a tooltip window
    ghwndToolTipTracking = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, TEXT(""),
                                WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
                                hInstance, NULL);

    ghwndToolTipPopup = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, TEXT(""),
                                WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
                                hInstance, NULL);

    RegisterPreviewWindowClass(&MonitorWindowProc);
    _InitDisplaySettings(FALSE);

    if (_NumDevices > 1)
        _ConfirmPositions();
    
    if (GetSystemMetrics(SM_REMOTESESSION)) {
        EnableWindow(GetDlgItem(_hDlg, IDC_DISPLAYPROPERTIES), FALSE);
    }

#ifdef WINNT
    //
    // Determine if any errors showed up during enumerations and initialization
    //
    _vPostExecMode();

    //
    // Now tell the user what we found out during initialization
    // Errors, or what we found during detection
    //

    PostMessage(hDlg, MSG_DSP_SETUP_MESSAGE, 0, 0);
    //
    // Since this could have taken a very long time, just make us visible
    // if another app (like progman) came up.
    //

    ShowWindow(hDlg, SW_SHOW);
#endif
}


LRESULT CALLBACK DeskWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwRefData)
{
    CMultiMon * pcmm = (CMultiMon *)dwRefData;
    HWND hwndC;
    RECT rcPos;
    BOOL bMoved = TRUE;
    int iMonitor;

    switch(message)
    {
        case WM_GETDLGCODE:
             return DLGC_WANTCHARS | DLGC_WANTARROWS;

        case WM_KILLFOCUS:
            RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE);
            break;

        case WM_MOUSEMOVE: {
                MSG mmsg;
                ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, hDlg, message, wParam, lParam);
            }
            break;

        case WM_PAINT:
            if (GetFocus() != hDlg)
                break;
            return(DefSubclassProc(hDlg, message, wParam, lParam));
            break;

        case WM_LBUTTONDOWN:
            SetFocus(hDlg);
            break;

        case WM_KEYDOWN:
#define MONITORMOVEUNIT 3
            hwndC = pcmm->GetCurDeviceHwnd();
            GetWindowRect(hwndC, &rcPos);
            MapWindowRect(NULL, hDlg, &rcPos);
            switch(wParam)
            {
                case VK_LEFT:
                    MoveWindow(hwndC, rcPos.left - MONITORMOVEUNIT, rcPos.top, RECTWIDTH(rcPos), RECTHEIGHT(rcPos), TRUE);
                    break;
                case VK_RIGHT:
                    MoveWindow(hwndC, rcPos.left + MONITORMOVEUNIT, rcPos.top, RECTWIDTH(rcPos), RECTHEIGHT(rcPos), TRUE);
                    break;
                case VK_UP:
                    MoveWindow(hwndC, rcPos.left, rcPos.top - MONITORMOVEUNIT, RECTWIDTH(rcPos), RECTHEIGHT(rcPos), TRUE);
                    break;
                case VK_DOWN:
                    MoveWindow(hwndC, rcPos.left, rcPos.top + MONITORMOVEUNIT, RECTWIDTH(rcPos), RECTHEIGHT(rcPos), TRUE);
                    break;
                default:
                    bMoved = FALSE;
                    break;
            }
#undef MONITORMOVEUNIT

            if (bMoved)
            {
                pcmm->HandleMonitorChange(hwndC, FALSE, FALSE);
                if (IsWindowVisible(ghwndToolTipPopup)) {
                    ToolTip_Update(ghwndToolTipPopup);                    
                }
            }

            break;

        case WM_CHAR:
            
            if (wParam >= TEXT('0') && wParam <= TEXT('9') && pcmm) {
                iMonitor = (TCHAR)wParam - TEXT('0');
                if ((iMonitor == 0) && (pcmm->GetNumDevices() >= 10))
                {
                    iMonitor = 10;
                }

                if ((iMonitor > 0) && ((ULONG)iMonitor <= pcmm->GetNumDevices()))
                {
                    HWND hwndList = GetDlgItem(GetParent(hDlg), IDC_DISPLAYLIST);
                    ComboBox_SetCurSel(hwndList, iMonitor - 1);
                    pcmm->UpdateActiveDisplay(NULL);
                    return 0;
                }
            }
            break;

        case WM_DESTROY:
            RemoveWindowSubclass(hDlg, DeskWndProc, 0);
            break;

        default:
            break;
    }

    return DefSubclassProc(hDlg, message, wParam, lParam);
}


//-----------------------------------------------------------------------------
//
// Callback functions PropertySheet can use
//
INT_PTR CALLBACK
MultiMonitorDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CMultiMon * pcmm = (CMultiMon *) GetWindowLongPtr(hDlg, DWLP_USER);
    switch (message)
    {
        case WM_INITDIALOG:
            ASSERT(!pcmm);
            pcmm = new CMultiMon;
            if (pcmm)
            {
                SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pcmm);
                ghwndPropSheet = GetParent(hDlg);

                SetWindowLong(ghwndPropSheet,
                              GWL_STYLE,
                              GetWindowLong(ghwndPropSheet, GWL_STYLE) | WS_CLIPCHILDREN);

                pcmm->InitMultiMonitorDlg(hDlg);

                //
                // if we have a invalid mode force the user to Apply
                //
                if (gbExecMode == EXEC_INVALID_MODE)
                    pcmm->SetDirty();

                return TRUE;
            }
            break;
        case WM_DESTROY:
            if (pcmm)
            {
                pcmm->WndProc(message, wParam, lParam);
                SetWindowLongPtr(hDlg, DWLP_USER, NULL);
                delete pcmm;
            }
            if(gfFlashWindowRegistered)
            {
                gfFlashWindowRegistered = FALSE;
                UnregisterClass(TEXT("MonitorNumber32"), hInstance);
            }
            break;
        default:
            if (pcmm)
                return pcmm->WndProc(message, wParam, lParam);
            break;
    }

    return FALSE;
}

void CMultiMon::_SetPreviewScreenSize(int HRes, int VRes, int iOrgXRes, int iOrgYRes)
{
    HBITMAP hbmOld;
    HBRUSH hbrOld;
    HDC hdcMem2;

    // stretching the taskbar could get messy, we'll only do the desktop
    int mon_dy = MON_DY - MON_TRAY;

    // init to identical extents
    SIZE dSrc = { MON_DX, mon_dy };
    SIZE dDst = { MON_DX, mon_dy };

    // set up a work area to play in
    if (!_hbmMonitor || !_hbmScrSample)
        return;
    hdcMem2 = CreateCompatibleDC(g_hdcMem);
    if (!hdcMem2)
        return;
    SelectObject(hdcMem2, _hbmScrSample);
    hbmOld = (HBITMAP)SelectObject(g_hdcMem, _hbmMonitor);

    // see if we need to shrink either aspect of the image
    if (HRes > iOrgXRes || VRes > iOrgYRes)
    {
        // make sure the uncovered area will be seamless with the desktop
        RECT rc = { MON_X, MON_Y, MON_X + MON_DX, MON_Y + mon_dy };
        HBRUSH hbr =
                    CreateSolidBrush( GetPixel( g_hdcMem, MON_X + 1, MON_Y + 1 ) );

        FillRect(hdcMem2, &rc, hbr);
        DeleteObject( hbr );
    }

    // stretch the image to reflect the new resolution
    if( HRes > iOrgXRes )
        dDst.cx = MulDiv( MON_DX, iOrgXRes, HRes );
    else if( HRes < iOrgXRes )
        dSrc.cx = MulDiv( MON_DX, HRes, iOrgXRes );

    if( VRes > iOrgYRes )
        dDst.cy = MulDiv( mon_dy, iOrgYRes, VRes );
    else if( VRes < iOrgYRes )
        dSrc.cy = MulDiv( mon_dy, VRes, iOrgYRes );

    SetStretchBltMode( hdcMem2, COLORONCOLOR );
    StretchBlt( hdcMem2, MON_X, MON_Y, dDst.cx, dDst.cy,
                g_hdcMem, MON_X, MON_Y, dSrc.cx, dSrc.cy, SRCCOPY);

    // now fill the new image's desktop with the possibly-dithered brush
    // the top right corner seems least likely to be hit by the stretch...

    hbrOld = (HBRUSH)SelectObject( hdcMem2, GetSysColorBrush( COLOR_DESKTOP ) );
    ExtFloodFill(hdcMem2, MON_X + MON_DX - 2, MON_Y+1,
                 GetPixel(hdcMem2, MON_X + MON_DX - 2, MON_Y+1), FLOODFILLSURFACE);

    // clean up after ourselves
    SelectObject( hdcMem2, hbrOld );
    SelectObject( hdcMem2, g_hbmDefault );
    DeleteObject( hdcMem2 );
    SelectObject( g_hdcMem, hbmOld );
}

void CMultiMon::_RedrawDeskPreviews()
{
    if (_NumDevices > 1)
    {
        _CleanupRects(_hwndDesk);
        RedrawWindow(_hwndDesk, NULL, NULL, RDW_ALLCHILDREN | RDW_ERASE | RDW_INVALIDATE);
    }
    else if (_pCurDevice && _pCurDevice->pds)
    {
        RECT rcPos, rcOrgPos;
        _pCurDevice->pds->GetCurPosition(&rcPos);
        _pCurDevice->pds->GetOrgPosition(&rcOrgPos);
        _SetPreviewScreenSize(RECTWIDTH(rcPos), RECTHEIGHT(rcPos), RECTWIDTH(rcOrgPos), RECTHEIGHT(rcOrgPos));
        // only invalidate the "screen" part of the monitor bitmap
        rcPos.left = MON_X;
        rcPos.top = MON_Y;
        rcPos.right = MON_X + MON_DX + 2;  // fudge (trust me)
        rcPos.bottom = MON_Y + MON_DY + 1; // fudge (trust me)
        InvalidateRect(GetDlgItem(_hDlg, IDC_SCREENSAMPLE), &rcPos, FALSE);
    }
}


int DisplaySaveSettings(PVOID pContext, HWND hwnd)
{
    CDeviceSettings *rgpds[1];
    rgpds[0] = (CDeviceSettings*) pContext;
    if(rgpds[0]->bIsModeChanged())
        return CMultiMon::_DisplaySaveSettings(rgpds, 1, hwnd);
    else 
        return DISP_CHANGE_SUCCESSFUL;
}

int CMultiMon::_DisplaySaveSettings(CDeviceSettings* rgpds[], ULONG numDevices, HWND hDlg)
{
    BOOL  bReboot = FALSE;
    BOOL  bTest = FALSE;
    int   iSave;
    HWND  hwndC;
    ULONG iDevice;
    POINT ptCursorSave;

    //
    // Test the new settings first
    //
    iSave = _SaveSettings(rgpds, numDevices, hDlg, CDS_TEST);

    if (iSave < DISP_CHANGE_SUCCESSFUL)
    {
        FmtMessageBox(hDlg,
                      MB_ICONEXCLAMATION,
                      IDS_CHANGE_SETTINGS,
                      IDS_SETTINGS_INVALID);

        return iSave;
    }
    
    TCHAR szWarnFlicker[510];
    int iDynaResult;
    LoadString(hInstance, IDS_WARNFLICK1, szWarnFlicker, SIZEOF(szWarnFlicker));

    //
    // Ask first and then change the settings.
    //
    if (!bReboot && 
        (_AnyChange(rgpds, numDevices) ||
         _IsSingleToMultimonChange(rgpds, numDevices)))
    {
        iDynaResult = AskDynaCDS(hDlg);
        if (iDynaResult == -1)
        {
            return DISP_CHANGE_NOTUPDATED;
        }
        else if (iDynaResult == 0)
        {
            bReboot = TRUE;
        }
    }

    if (!bReboot && _AnyChange(rgpds, numDevices)) 
    {
        if (ShellMessageBox(hInstance, GetParent(hDlg), szWarnFlicker,
                            NULL, MB_OKCANCEL | MB_ICONINFORMATION) != IDOK)
        {
            return DISP_CHANGE_NOTUPDATED;
        }
        bTest = TRUE;
    }

    //
    // Save the settings to the registry.
    //
    iSave = _SaveSettings(rgpds, numDevices, hDlg, CDS_UPDATEREGISTRY | CDS_NORESET);

    if (iSave < DISP_CHANGE_SUCCESSFUL)
    {
        //
        // BUGBUG
        // If we get NOT_UPDATED, this mean security may be turned on.
        // We could still try to do the dynamic change.
        // This only works in single mon ...
        //

        if (iSave == DISP_CHANGE_NOTUPDATED)
        {
            FmtMessageBox(hDlg,
                          MB_ICONEXCLAMATION,
                          IDS_CHANGE_SETTINGS,
                          IDS_SETTINGS_CANNOT_SAVE);
        }
        else
        {
            FmtMessageBox(hDlg,
                          MB_ICONEXCLAMATION,
                          IDS_CHANGE_SETTINGS,
                          IDS_SETTINGS_FAILED_SAVE);
        }

        //
        // Restore the settings to their original state
        //

        for (iDevice = 0; iDevice < numDevices; iDevice++)
        {
            rgpds[iDevice]->RestoreSettings();
        }

        _SaveSettings(rgpds, numDevices, hDlg, CDS_UPDATEREGISTRY | CDS_NORESET);
        return iSave;
    }


    if (bReboot)
    {
        iSave = DISP_CHANGE_RESTART;
    }

    //
    // Try to change the mode dynamically if it was requested
    //
    GetCursorPos(&ptCursorSave);

    if (iSave == DISP_CHANGE_SUCCESSFUL)
    {
        hwndC = CreateCoverWindow(COVER_NOPAINT);
        
        // If EnumDisplaySettings was called with EDS_RAWMODE, we need CDS_RAWMODE below.
        // Otherwise, it's harmless.
        iSave = ChangeDisplaySettings(NULL, CDS_RAWMODE);
        //We post a message to ourselves to destroy it later.
        if(hwndC)
            PostMessage(hwndC, WM_CLOSE, 0, 0L);

        //
        // Check the return from the dynamic mode switch.
        //

        if (iSave < 0)
        {
            FmtMessageBox(hDlg,
                          MB_ICONEXCLAMATION,
                          IDS_CHANGE_SETTINGS,
                          IDS_DYNAMIC_CHANGESETTINGS_FAILED);
        }
        else if (iSave == DISP_CHANGE_SUCCESSFUL)
        {

            //
            // Set the cursor to where it was before we changed the display
            // (ie, if we changed a 2ndary monitor, the cursor would have been
            // placed on the primary after the application of the change, move
            // it back to the 2ndary monitor.  If the change failed, we are 
            // just placing the cursor back to it orig pos
            //
            SetCursorPos(ptCursorSave.x, ptCursorSave.y);

            //
            // Determine what to do based on the return code.
            //
            if (bTest && (IDYES != DialogBox(hInstance,
                                             MAKEINTRESOURCE(DLG_KEEPNEW),
                                             GetParent(hDlg),
                                             KeepNewDlgProc)))
            {
                iSave = DISP_CHANGE_NOTUPDATED;
            }
        }
    }

    //
    // Determine what to do based on the return code.
    //

    if (iSave >= DISP_CHANGE_SUCCESSFUL)
    {
        //
        // Confirm the settings
        //
        for (iDevice = 0; iDevice < numDevices; iDevice++)
        {
            rgpds[iDevice]->ConfirmChangeSettings();
        }

        #ifndef WINNT
        //
        // Special workaround for Quick Time on Win9x only
        //
        // if (GetNumberOfAttachedDisplays() > 1)
        if (numDevices > 1)
            WritePrivateProfileString(TEXT("Video"), TEXT("Optimize"), TEXT("bmp"), TEXT("qtw.ini"));
        #endif
    }
    else
    {
        //
        // Restore the settings to their original state
        //
        for (iDevice = 0; iDevice < numDevices; iDevice++)
        {
            rgpds[iDevice]->RestoreSettings();
        }

        //DLI: This last function call will actually go refresh the whole desktop
        hwndC = CreateCoverWindow(COVER_NOPAINT);

        // If EnumDisplaySettings was called with EDS_RAWMODE, we need CDS_RAWMODE below.
        // Otherwise, it's harmless.
        ChangeDisplaySettings(NULL, CDS_RAWMODE);

        //We post ourselves a message to destroy it later.
        if(hwndC)
            PostMessage(hwndC, WM_CLOSE, 0, 0L);
    }

    return iSave;
}

int CMultiMon::_HandleApply()
{
#ifndef WINNT   //BUGBUG should we do this on NT???
    //
    // if the user hits apply when we dont have a valid display device
    // force VGA mode in the registry and restart.
    //
    if (gbExecMode    == EXEC_INVALID_MODE &&
        gbInvalidMode == EXEC_INVALID_DISPLAY_DEVICE)
    {
        NukeDisplaySettings();
        PropSheet_RestartWindows(ghwndPropSheet);
        return DISP_CHANGE_SUCCESSFUL;
    }

    //
    // if we are in VGA fallback mode, and the user stays in VGA mode
    // also nuke all settings from the registry.
    //
    if (gbExecMode    == EXEC_INVALID_MODE &&
        gbInvalidMode == EXEC_INVALID_DISPLAY_MODE &&
        _pPrimaryDevice->pds->GetCurColor() == 4)
    {
        NukeDisplaySettings();
        // BUGBUG this does not work because the return value type has changed
        PropSheet_RestartWindows(ghwndPropSheet);
        return DISP_CHANGE_SUCCESSFUL;
    }
#endif

    CDeviceSettings *rgpds[MONITORS_MAX];
    ULONG           iDevice;

    for (iDevice = 0; iDevice < _NumDevices; iDevice++) {
        rgpds[iDevice] = _Devices[iDevice].pds;
    }

    return _DisplaySaveSettings(rgpds, _NumDevices, _hDlg);
}


//-----------------------------------------------------------------------------
//
// Resolution slider
//
CMultiMon::_HandleHScroll(HWND hwndTB, int iCode, int iPos)
{
    int iRes = _iResolution;
    int cRes = (int)SendMessage(hwndTB, TBM_GETRANGEMAX, TRUE, 0);

    TraceMsg(TF_FUNC, "_HandleHScroll: MaxRange = %d", cRes);

    // Message box if something bad is going to happen ?
    //    _VerifyPrimaryMode(TRUE);


    switch(iCode ) {
        case TB_LINEUP:
        case TB_PAGEUP:
            if (iRes != 0)
                iRes--;
            break;

        case TB_LINEDOWN:
        case TB_PAGEDOWN:
            if (++iRes >= cRes)
                iRes = cRes;
            break;

        case TB_BOTTOM:
            iRes = cRes;
            break;

        case TB_TOP:
            iRes = 0;
            break;

        case TB_THUMBTRACK:
        case TB_THUMBPOSITION:
            iRes = iPos;
            break;

        default:
            return FALSE;
    }

    TraceMsg(TF_FUNC, "_HandleHScroll: iRes = %d", iRes);

    _pCurDevice->pds->SetCurResolution(_pCurDevice->ResolutionList + iRes);

    //
    // Repaint the control in case they changed
    //
    _UpdateUI(IDC_SCREENSIZE);

    if ( (gbExecMode == EXEC_NORMAL) ||
         (gbExecMode == EXEC_INVALID_MODE) ||
         (gbExecMode == EXEC_DETECT) ) {

        //
        // Set the apply button if resolution has changed
        //

        if (_pCurDevice->pds->bIsModeChanged())
            SetDirty();

        return 0;
    }

    return TRUE;
}

void CMultiMon::_ForwardToChildren(UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hwndC;
    if ((hwndC = GetDlgItem(_hDlg, IDC_SCREENSIZE)))
        SendMessage(hwndC, message, wParam, lParam);
}

LRESULT CALLBACK CMultiMon::WndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    NMHDR FAR * lpnm;
    HWND hwndC;
    HWND hwndSample;
    HBITMAP hbm;

    switch (message)
    {
    case WM_NOTIFY:

        lpnm = (NMHDR FAR *)lParam;
        switch (lpnm->code)
        {
        case PSN_APPLY:

            //Check to see if there is a change in the DPI value due to "Advanced->General" tab.
            if(g_iNewDPI != g_iAppliedNewDPI)
            {
                //Yup! There is a change. See if the Appearance tab is up and running; if so,
                // it will handle the DPI change gracefully!
                if(!g_hDlg) //If appearance tab running?
                {
                    //No! Appearance tab is not up! So, we need to handle dpi change ourselves;
                    UpdateUIfontsDueToDPIchange(g_iAppliedNewDPI, g_iNewDPI);
                    g_iAppliedNewDPI = g_iNewDPI;
                }
            }

            if (IsDirty())
            {
                int status;

                

                //
                // Apply the settings, and enable\disable the Apply button
                // appropriatly.
                //
                status = _HandleApply();

                SetDirty(status < 0);

                if (status == DISP_CHANGE_RESTART)
                {
                    PropSheet_RestartWindows(ghwndPropSheet);
                }
                else if (status == DISP_CHANGE_SUCCESSFUL)
                {
                    //
                    // if the close flag is false, and we are not rebooting,
                    // rebuild the settings
                    //
                    if (((PSHNOTIFY*)lParam)->lParam == FALSE)
                    {
                        UINT iDevice;
                        TCHAR szDeviceName[32];

                        ASSERT(_pCurDevice);
                        ASSERT(sizeof(szDeviceName) >=
                               sizeof(_pCurDevice->DisplayDevice.DeviceName));
                
                        lstrcpy(szDeviceName, _pCurDevice->DisplayDevice.DeviceName);
                        _InitDisplaySettings(FALSE);
                        for (iDevice = 0; iDevice < _NumDevices; iDevice++)
                        {
                            if (lstrcmp(_Devices[iDevice].DisplayDevice.DeviceName, szDeviceName) == 0)
                            {
                                UpdateActiveDisplay(_Devices + iDevice);
                                break;
                            }
                        }
                    }
                }
                else
                {
                    //
                    // Make sure the dialog stays and redraw
                    //
                    _InitDisplaySettings(FALSE);
                    UpdateActiveDisplay(NULL);
                    SetWindowLongPtr(_hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                }
            }
            break;
        default:
            return FALSE;
        }
        break;

    case WM_CTLCOLORSTATIC:

        if (GetDlgCtrlID((HWND)lParam) == IDC_DISPLAYDESK)
        {
            return (UINT_PTR)GetSysColorBrush(COLOR_APPWORKSPACE);
        }
        return FALSE;


    case WM_COMMAND:

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_DISPLAYPRIME:

            if (!SetPrimary(_pCurDevice))
            {
                return FALSE;
            }

            hwndC = GetCurDeviceHwnd();
            HandleMonitorChange(hwndC, TRUE);

            break;


        case IDC_DISPLAYUSEME:

            //
            // Don't pop up warning dialog box if this display is already attached
            // or if there are already more than 1 display
            //
            if (!SetMonAttached(_pCurDevice,
                                !_pCurDevice->pds->IsAttached(),
                                TRUE,
                                _hDlg))
            {
                return FALSE;
            }

            hwndC = GetCurDeviceHwnd();
            HandleMonitorChange(hwndC, TRUE);

            break;

        case IDC_DISPLAYLIST:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case CBN_DBLCLK:
                goto DoDeviceSettings;

            case CBN_SELCHANGE:
                UpdateActiveDisplay(NULL);
                break;

            default:
                return FALSE;
            }
            break;

        case IDC_DISPLAYPROPERTIES:
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            DoDeviceSettings:
            case BN_CLICKED:
                if (IsWindowEnabled(GetDlgItem(_hDlg, IDC_DISPLAYPROPERTIES)))
                    _OnAdvancedClicked();
                break;

            default:
                return FALSE;
            }
            break;

        case IDC_COLORBOX:
            switch(GET_WM_COMMAND_CMD(wParam, lParam))
            {
                case CBN_SELCHANGE:
                {
                    HWND hwndColorBox = GetDlgItem(_hDlg, IDC_COLORBOX);
                    int iClr = ComboBox_GetCurSel(hwndColorBox);

                    if (iClr != CB_ERR ) {

                        // Message box if something bad is going to happen ?
                        //    _VerifyPrimaryMode(TRUE);

                        _pCurDevice->pds->SetCurColor((int) *(_pCurDevice->ColorList + iClr));

                        //
                        // Repaint the control in case they changed
                        //

                        _UpdateUI(IDC_COLORBOX);
                    }

                    break;
                }
                default:
                    break;
            }

            break;

        case IDC_TROUBLESHOOT:
            // Invokes the trouble shooter for the Settings tab.
            {
                TCHAR szCommand[ MAX_PATH ];
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                
                LoadString(hInstance,IDS_TROUBLESHOOT_EXEC, szCommand, sizeof(szCommand)/sizeof(TCHAR));
                ZeroMemory(&si, sizeof(si));
                si.cb = sizeof(si);
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_NORMAL;
                
                if (CreateProcess(NULL, szCommand, NULL, NULL, FALSE, 0,0, NULL, &si, &pi)) 
                {
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
            }
            break;

        case IDC_IDENTIFY:
            // Flashes the Text on all the monitors simultaneously
            {
                HWND  hwndC;
                
                //Enumerate all the monitors and flash this for each!
                hwndC = GetWindow(_hwndDesk, GW_CHILD);
                while (hwndC)
                {
                    PostMessage(hwndC, WM_COMMAND, MAKEWPARAM(IDC_FLASH, 0), MAKELPARAM(0, 0));
                    hwndC = GetWindow(hwndC, GW_HWNDNEXT);
                }
            }
            break;

        default:
            return FALSE;
        }


        //
        // Enable the apply button only if we are not in setup.
        //

        if ( (gbExecMode == EXEC_NORMAL) ||
             (gbExecMode == EXEC_INVALID_MODE) ||
             (gbExecMode == EXEC_DETECT) ) {

            //
            // Set the apply button if something changed
            //
            if (_pCurDevice->pds->bIsModeChanged())
            {
                SetDirty();
            }
        }
        break;

    case WM_HSCROLL:
        _HandleHScroll((HWND)lParam, (int) LOWORD(wParam), (int) HIWORD(wParam));
        break;

#if QUICK_REFRESH
    case WM_RBUTTONDOWN:
        if (_NumDevices == 1 && GetKeyState(VK_CONTROL) & 0x8000) {
            HMENU hfreq;
            HWND hwnd;
            POINT pt;
            RECT rc;
            DWORD cmd;

            hwnd = GetDlgItem(_hDlg, IDC_SCREENSAMPLE);
            GetWindowRect(hwnd, &rc);
            GetCursorPos(&pt);

            if (!PtInRect(&rc,pt)) {
                break;
            }

            hfreq = CreateFrequencyMenu(_pCurDevice);

            cmd = TrackPopupMenu(hfreq, TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                 pt.x, pt.y, 0, _hDlg, NULL); 
            if (cmd) {
                _pCurDevice->pds->SetCurFrequency(cmd - IDC_FREQUENCY_START);
                SetDirty();
            }

            DestroyMenu(hfreq);
        }
        break;
#endif // QUICK_REFRESH

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("display.hlp"), HELP_WM_HELP,
            (DWORD_PTR)(LPTSTR)sc_MultiMonitorHelpIds);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND)wParam, TEXT("display.hlp"), HELP_CONTEXTMENU,
            (DWORD_PTR)(LPTSTR)sc_MultiMonitorHelpIds);
        break;

    case WM_DISPLAYCHANGE:
    case WM_WININICHANGE:
        _ForwardToChildren(message, wParam, lParam);
        break;

    case WM_SYSCOLORCHANGE:
        if (_himl)
            ImageList_SetBkColor(_himl, GetSysColor(COLOR_APPWORKSPACE));

        //
        // Needs to be passed to all the new common controls so they repaint
        // correctly using the new system colors
        //
        _ForwardToChildren(message, wParam, lParam);

        //
        // Rerender the monitor(s) bitmap(s) to reflect the new colors
        //
        if (_NumDevices == 1) {
            // set up bitmaps for sample screen
            if (_hbmScrSample && (GetObjectType(_hbmScrSample) != 0)) {
                DeleteObject(_hbmScrSample);
                _hbmScrSample = 0;
            }
            _hbmScrSample = LoadMonitorBitmap( TRUE ); // let them do the desktop
            SendDlgItemMessage(_hDlg, IDC_SCREENSAMPLE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)_hbmScrSample);
    
            // get a base copy of the bitmap for when the "internals" change
            if (_hbmMonitor && (GetObjectType(_hbmMonitor) != 0)) {
                DeleteObject(_hbmMonitor);
                _hbmMonitor = 0;
            }
            _hbmMonitor = LoadMonitorBitmap( FALSE ); // we'll do the desktop
        }
        else if (_NumDevices > 0) {
            HBITMAP hbm, hbmMask;
            int cx, cy;
            UINT iDevice;
            PMULTIMON_DEVICE pDevice;
            TCHAR            ach[4]; 

            //
            // replace each monitor bitmap with one with correct colors
            //
            for (iDevice = 0; iDevice < _NumDevices; iDevice++)
            {
                pDevice = &_Devices[iDevice];

                _itot(iDevice+1,ach,10);
                ImageList_GetIconSize(pDevice->himl, &cx, &cy);
                MakeMonitorBitmap(pDevice->w,pDevice->h,ach,&hbm,&hbmMask,cx,cy, (pDevice == _pCurDevice));
                ImageList_Replace(pDevice->himl,pDevice->iImage,hbm,hbmMask);

                DeleteObject(hbm);
                DeleteObject(hbmMask);
            }
        }

        break;

#if 0
    //
    // NOTE:  until video supports device interfaces, we cannot use
    //        WM_DEVICECHANGE to detect video changes.  The default WM_DEVCHANGE
    //        only reports about legacy devices
    //
    case WM_DEVICECHANGE:
        //
        // Rebuild the device list if we are not currently enumerating,
        // because enumerating may cause another device to come on-line
        //
        // We only reenumerate if a new *video* device arrives
        //
        if (!_InSetInfo &&
            (wParam == DBT_DEVICEARRIVAL || wParam == DBT_DEVICEREMOVECOMPLETE))
        {
                        DEV_BROADCAST_HDR *bhdr = (DEV_BROADCAST_HDR *) lParam;

            // check for something else here, most likely the dev interface guid
                        if (bhdr->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) 
            {
                _InitDisplaySettings(FALSE);
            }
        }

        break;
#endif

    case WM_DESTROY:
        TraceMsg(TF_GENERAL, "WndProc:: WM_DESTROY");
        hwndSample = GetDlgItem(_hDlg, IDC_COLORSAMPLE);
        if (hbm = (HBITMAP)SendMessage(hwndSample, STM_SETIMAGE, IMAGE_BITMAP, NULL))
            DeleteObject(hbm);

        if (_NumDevices == 1)
        {
            hwndSample = GetDlgItem(_hDlg, IDC_SCREENSAMPLE);
            if (hbm = (HBITMAP)SendMessage(hwndSample, STM_SETIMAGE, IMAGE_BITMAP, NULL))
                DeleteObject(hbm);

            if (_hbmScrSample && (GetObjectType(_hbmScrSample) != 0))
                DeleteObject(_hbmScrSample);
            if (_hbmMonitor && (GetObjectType(_hbmMonitor) != 0))
                DeleteObject(_hbmMonitor);
        }

        _DestroyDisplaySettings();

        break;

#ifdef WINNT
    case MSG_DSP_SETUP_MESSAGE:
        return _InitMessage();
#endif
        // MultiMonitor CPL specific messages
    case MM_REDRAWPREVIEW:
        _RedrawDeskPreviews();
        break;

    case WM_LBUTTONDBLCLK:
        if (_NumDevices == 1)
        {
            HWND hwndSample = GetDlgItem(_hDlg, IDC_SCREENSAMPLE);
            if(NULL != hwndSample)
            {
                POINT pt;
                RECT rc;

                pt.x = GET_X_LPARAM(lParam);  // horizontal position of cursor 
                pt.y = GET_Y_LPARAM(lParam);  // vertical position of cursor 
                GetWindowRect(hwndSample, &rc);

                if(ClientToScreen(_hDlg, &pt) && PtInRect(&rc, pt)) 
                    PostMessage(_hDlg, WM_COMMAND, MAKEWPARAM(IDC_DISPLAYPROPERTIES, BN_CLICKED), (LPARAM)hwndSample);
            }

            break;
        }
        else
            return FALSE;

    default:
        return FALSE;
    }

    return TRUE;
}


// IUnknown methods
HRESULT CMultiMon::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    // ppvObj must not be NULL
    ASSERT(ppvObj != NULL);

    if (ppvObj == NULL)
        return E_INVALIDARG;

    *ppvObj = NULL;
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = SAFECAST(this, IUnknown *);
    else if (IsEqualIID(riid, IID_IMultiMonConfig))
        *ppvObj = SAFECAST(this, IMultiMonConfig *);
    else
        return E_NOINTERFACE;


    AddRef();
    return S_OK;
}


ULONG CMultiMon::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CMultiMon::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


// IMultiMonConfig methods
HRESULT CMultiMon::Initialize(HWND hwndHost, WNDPROC pfnWndProc, DWORD dwReserved)
{
    WNDCLASS wc = {0};
    HRESULT hr = E_FAIL;

    if (hwndHost && RegisterPreviewWindowClass(pfnWndProc))
    {
        _hwndDesk = hwndHost;
        if (_InitDisplaySettings(TRUE))
            hr = S_OK;
    }
    return hr;
}

HRESULT CMultiMon::GetNumberOfMonitors(int * pCMon, DWORD dwReserved)
{
    if (pCMon)
    {
        *pCMon = _NumDevices;
        return S_OK;
    }

    return E_FAIL;
}

HRESULT CMultiMon::GetMonitorData(int iMonitor, MonitorData * pmd, DWORD dwReserved)
{
    ASSERT(pmd);
    if ((pmd == NULL) || ((ULONG)iMonitor >= _NumDevices))
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);

    PMULTIMON_DEVICE pDevice = &_Devices[iMonitor];

    pmd->dwSize = SIZEOF(MonitorData);
    if ( pDevice->pds->IsPrimary() )
        pmd->dwStatus |= MD_PRIMARY;
    if ( pDevice->pds->IsAttached() )
        pmd->dwStatus |= MD_ATTACHED;
    pDevice->pds->GetCurPosition(&pmd->rcPos);

    return S_OK;
}

HRESULT CMultiMon::Paint(int iMonitor, DWORD dwReserved)
{
    _RedrawDeskPreviews();

    return S_OK;
}


/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/
HFONT GetFont(LPRECT prc)
{
    LOGFONT lf;

    FillMemory(&lf,  SIZEOF(lf), 0);
    lf.lfWeight = FW_EXTRABOLD;
    lf.lfHeight = prc->bottom - prc->top;
    lf.lfWidth  = 0;
    lf.lfPitchAndFamily = FF_SWISS;
    lf.lfOutPrecision = OUT_TT_ONLY_PRECIS;

    return CreateFontIndirect(&lf);
}

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/
#define HANG_TIME 2500

LRESULT CALLBACK BigNumberWindowProc(HWND hwnd, UINT msg,WPARAM wParam,LPARAM lParam)
{
    TCHAR ach[80];
    HFONT hfont;
    RECT  rc;
    HDC   hdc;
    HRGN  hrgnTxtA;
    PAINTSTRUCT ps;
    HGDIOBJ hOldPen;
    HGDIOBJ hNewPen;

    switch (msg)
    {
    case WM_CREATE:
        break;

    case WM_SIZE:
        GetWindowText(hwnd, ach, sizeof(ach));
        GetClientRect(hwnd, &rc);
        hfont = GetFont(&rc);

        hdc = GetDC(hwnd);
        SelectObject(hdc, hfont);

        BeginPath(hdc);
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc,0,0,ach,lstrlen(ach));
        EndPath(hdc);

        hrgnTxtA = PathToRegion(hdc);
        SetWindowRgn(hwnd,hrgnTxtA,TRUE);

        ReleaseDC(hwnd, hdc);
        DeleteObject(hfont);
        break;

    case WM_TIMER:
        DestroyWindow(hwnd);
        return 0;

    case WM_PAINT:
        GetWindowText(hwnd, ach, sizeof(ach));
        GetClientRect(hwnd, &rc);
        hfont = GetFont(&rc);

        hdc = BeginPaint(hwnd, &ps);
        //The following paints the whole region (which is in the shape of the number) black!
        PatBlt(hdc, 0, 0, rc.right, rc.bottom, BLACKNESS | NOMIRRORBITMAP);
        
        SelectObject(hdc, hfont);
        SetTextColor(hdc, 0xFFFFFF);
        //Let's create a path that is the shape of the region by drawing that number.
        BeginPath(hdc);
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc,0,0,ach,lstrlen(ach));
        EndPath(hdc);

        // The above TextOut calljust created the path. Let's now actually draw the number!
        // Note: We are drawing the number in white there by changing whatever painted as black
        // a few moments ago!
        TextOut(hdc,0,0,ach,lstrlen(ach));

        //Let's create a thick black brush to paint the borders of the number we just drew!
        hNewPen = CreatePen(PS_INSIDEFRAME, 4, 0x0); //Black Color
        hOldPen = SelectObject(hdc, hNewPen);

        //Draw the border of the white number with the thick black brush!
        StrokePath(hdc);
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hNewPen);
        
        EndPaint(hwnd, &ps);
        DeleteObject(hfont);
        break;
    }

    return DefWindowProc(hwnd,msg,wParam,lParam);
}

int Bail()
{
    POINT pt;
    POINT pt0;
    DWORD time0;
    DWORD d;

    d     = GetDoubleClickTime();
    time0 = GetMessageTime();
    pt0.x = (int)(short)LOWORD(GetMessagePos());
    pt0.y = (int)(short)HIWORD(GetMessagePos());

    if (GetTickCount()-time0 > d)
        return 2;

    if (!((GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_RBUTTON)) & 0x8000))
        return 1;

    GetCursorPos(&pt);

    if ((pt.y - pt0.y) > 2 || (pt.y - pt0.y) < -2)
        return 1;

    if ((pt.x - pt0.x) > 2 || (pt.x - pt0.x) < -2)
        return 1;

    return 0;
}

void FlashText(HWND hDlg, PMULTIMON_DEVICE pDevice, LPCTSTR sz, LPRECT prc, BOOL fWait)
{
    HFONT hfont;
    SIZE  size;
    HDC   hdc;
    int   i;
    
    if (!pDevice->pds->IsOrgAttached())
        return;

    if (pDevice->hwndFlash && IsWindow(pDevice->hwndFlash))
    {
        DestroyWindow(pDevice->hwndFlash);
        pDevice->hwndFlash = NULL;
    }

    if (sz == NULL)
        return;

    if (fWait)
    {
        while ((i=Bail()) == 0)
            ;

        if (i == 1)
            return;
    }

    hdc = GetDC(NULL);
    hfont = GetFont(prc);
    SelectObject(hdc, hfont);
    GetTextExtentPoint(hdc, sz, lstrlen(sz), &size);
    ReleaseDC(NULL, hdc);
    DeleteObject(hfont);

    if(!gfFlashWindowRegistered)
    {
        WNDCLASS    cls;
        cls.hCursor        = LoadCursor(NULL,IDC_ARROW);
        cls.hIcon          = NULL;
        cls.lpszMenuName   = NULL;
        cls.lpszClassName  = TEXT("MonitorNumber32");
        cls.hbrBackground  = (HBRUSH)(COLOR_DESKTOP + 1);
        cls.hInstance      = hInstance;
        cls.style          = CS_VREDRAW | CS_HREDRAW;
        cls.lpfnWndProc    = BigNumberWindowProc;
        cls.cbWndExtra     = 0;
        cls.cbClsExtra     = 0;

        RegisterClass(&cls);

        gfFlashWindowRegistered = TRUE;
    }

    pDevice->hwndFlash = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW, //WS_BORDER,
        TEXT("MonitorNumber32"), sz,
        WS_POPUP,
        (prc->right  + prc->left - size.cx)/2,
        (prc->bottom + prc->top  - size.cy)/2,
        size.cx,
        size.cy,
        hDlg,   //Set the dialog as the parent sothat we get back the activation after flash window goes away!
        NULL,
        hInstance,
        NULL);

    ShowWindow(pDevice->hwndFlash, SW_SHOW);
    UpdateWindow(pDevice->hwndFlash);
    SetTimer(pDevice->hwndFlash, 1, HANG_TIME, NULL);
}


void DrawMonitorNum(HDC hdc, int w, int h, LPCTSTR sz, BOOL fDrawBackground=TRUE)
{
    HFONT    hfont;
    HFONT    hfontT;
    RECT     rc;
    COLORREF rgb;
    COLORREF rgbDesk;

    SetRect(&rc, 0, 0, w, h);

    rgb     = GetSysColor(COLOR_CAPTIONTEXT);
    rgbDesk = GetSysColor(COLOR_DESKTOP);

    if (fDrawBackground)
        FillRect(hdc, &rc, GetSysColorBrush (COLOR_DESKTOP));

    InflateRect(&rc, -(MON_X*w / MON_W)>> 1, -(MON_Y*h/ MON_H));

    if (rgbDesk == rgb)
        rgb = GetSysColor(COLOR_WINDOWTEXT);

    if (rgbDesk == rgb)
        rgb = rgbDesk ^ 0x00FFFFFF;

    SetTextColor(hdc, rgb);

    hfont = GetFont(&rc);
    hfontT = (HFONT)SelectObject(hdc, hfont);
    SetTextAlign(hdc, TA_CENTER | TA_TOP);
    SetBkMode(hdc, TRANSPARENT);
    ExtTextOut(hdc, (rc.left+rc.right)/2, rc.top, 0, NULL, sz, lstrlen(sz), NULL); 
    SelectObject(hdc, hfontT);
    DeleteObject(hfont);
}

void AddTrackingToolTip(PMULTIMON_DEVICE pDevice, HWND hwnd)
{
    TOOLINFO ti;
    TCHAR location[16];
    RECT rcPos;

    //
    // New tool Tip
    //

    pDevice->pds->GetCurPosition(&rcPos);
    wsprintf(location, TEXT("%d, %d"), rcPos.left, rcPos.top);

    GetWindowRect(hwnd, &rcPos);

    ti.cbSize      = sizeof(TOOLINFO);
    ti.uFlags      = TTF_TRACK; 
    ti.hwnd        = hwnd;
    ti.uId         = (UINT_PTR) pDevice;
    ti.hinst       = hInstance;
    ti.lpszText    = location; 
    ti.rect.left   = rcPos.left + 2;
    ti.rect.top    = rcPos.top + 2;
    ti.rect.right  = rcPos.right - 2;// ti.rect.left + 10;
    ti.rect.bottom = rcPos.bottom - 2; //  ti.rect.top + 10;

    ToolTip_AddTool(ghwndToolTipTracking, &ti);
    pDevice->bTracking = FALSE;

    TraceMsg(TF_GENERAL, "Added TOOLTIP hwnd %08lx, uId %08lx\n", ti.hwnd, ti.uId);
    return;
}

void RemoveTrackingToolTip(HWND hwnd)
{
    TOOLINFO ti;

    ZeroMemory(&ti, sizeof(TOOLINFO));
    ti.cbSize      = sizeof(TOOLINFO);
    ti.hwnd        = hwnd;
    ti.uId         = (UINT_PTR) GetDlgCtrlDevice(hwnd);

    ToolTip_DelTool(ghwndToolTipTracking, &ti);
}

BOOLEAN TrackToolTip(PMULTIMON_DEVICE pDevice, HWND hwnd, BOOL bTrack)
{
    TOOLINFO ti;
    BOOLEAN oldTracking;

    ZeroMemory(&ti, sizeof(TOOLINFO));
    ti.cbSize      = sizeof(TOOLINFO);
    ti.hwnd        = hwnd;
    ti.uId         = (UINT_PTR) pDevice;

    oldTracking = pDevice->bTracking;
    pDevice->bTracking = (BOOLEAN)bTrack;
    ToolTip_TrackActivate(ghwndToolTipTracking, bTrack, &ti);

    TraceMsg(TF_GENERAL, "Track TOOLTIP hwnd %08lx, uId %08lx\n", ti.hwnd, ti.uId);

    return oldTracking;
}

void AddPopupToolTip(HWND hwndC)
{
    TOOLINFO ti;

    //
    // New tool Tip
    //
    ti.cbSize      = sizeof(TOOLINFO);
    ti.uFlags      = TTF_IDISHWND | TTF_SUBCLASS | TTF_CENTERTIP;  
    ti.hwnd        = hwndC;
    ti.uId         = (UINT_PTR) hwndC;
    ti.hinst       = hInstance;
    GetWindowRect(hwndC, &ti.rect);
    ti.lpszText    = LPSTR_TEXTCALLBACK;
    
    ToolTip_AddTool(ghwndToolTipPopup, &ti);
}

void RemovePopupToolTip(HWND hwndC)
{
    TOOLINFO ti;

    ZeroMemory(&ti, sizeof(TOOLINFO));
    ti.cbSize      = sizeof(TOOLINFO);
    ti.hwnd        = hwndC; 
    ti.uId         = (UINT_PTR) hwndC;

    ToolTip_DelTool(ghwndToolTipPopup, &ti);
}

#if QUICK_REFRESH

HMENU CreateFrequencyMenu(PMULTIMON_DEVICE pDevice)
{
    HMENU hfreq;
    MENUITEMINFO mii;

    ZeroMemory(&mii, sizeof(mii));
    mii.cbSize = sizeof(mii);

    if (!(hfreq = CreatePopupMenu())) {
        return NULL;
    }

    PLONGLONG freqs;
    UINT i, iFreq = pDevice->pds->GetFrequencyList(-1, NULL, &freqs);
    int curFreq = pDevice->pds->GetCurFrequency(), freq;
    TCHAR ach[LINE_LEN], achFre[50];

    mii.fMask = MIIM_TYPE | MFT_STRING | MIIM_ID | MIIM_STATE;
    mii.dwTypeData = ach;
    mii.fType = MFT_STRING;
    mii.fState = MFS_ENABLED;

    for (i = 0; i < iFreq; i++ ) {
        freq = (int) freqs[i];

        if (freq == 1) {
            LoadString(hInstance, IDS_DEFFREQ, ach, sizeof(ach));
        }
        else {
            DWORD  idFreq = IDS_FREQ;

            if (freq < 50) {                   
                idFreq = IDS_INTERLACED;
            }

            LoadString(hInstance, idFreq, achFre, sizeof(achFre));
            wsprintf(ach, TEXT("%d %s"), freq, achFre);
        }

        mii.cch = lstrlen(ach); 
        mii.wID = IDC_FREQUENCY_START + freq;

        mii.fState = MFS_ENABLED;
        if (curFreq == freq) 
            mii.fState = MFS_CHECKED;
        InsertMenuItem(hfreq, i, TRUE, &mii);
    }

    LocalFree(freqs);

    return hfreq;
}

#endif

BOOL MakeMonitorBitmap(int w, int h, LPCTSTR sz, HBITMAP *pBitmap, HBITMAP *pMaskBitmap, int cx, int cy, BOOL fSelected)
{
    HBITMAP hbm;        // 128x128 bitmap we will return
    HBITMAP hbmT;       // bitmap loaded from resource
    HBITMAP hbmM = NULL;// mask bitmap
    HDC     hdc;        // work dc
    HDC     hdcS;       // screen dc
    HDC     hdcT;       // another work dc
    HDC     hdcM = NULL;// another work dc
    RECT    rc;

    ASSERT(w <= cx);
    ASSERT(h <= cy);

    hdcS = GetDC(NULL);
    hdc  = CreateCompatibleDC(hdcS);
    hdcT = CreateCompatibleDC(hdcS);
    if(pMaskBitmap)
        hdcM = CreateCompatibleDC(hdcS);

    hbm  = CreateCompatibleBitmap(hdcS, cx, cy);
    hbmT = CreateCompatibleBitmap(hdcS, w, h);
    if(pMaskBitmap)
        hbmM = CreateBitmap(cx,cy,1,1,NULL);
    ReleaseDC(NULL,hdcS);

    SelectObject(hdc, hbm);
    SelectObject(hdcT,hbmT);
    if(pMaskBitmap)
        SelectObject(hdcM,hbmM);

    //
    // Make sure the color of the borders (selection & normal) is different than the background color.
    //
    HBRUSH hbrDiff = NULL;
    BOOL bNeedDiff = ((fSelected && 
                       (GetSysColor(COLOR_APPWORKSPACE) == GetSysColor(COLOR_HIGHLIGHT))) ||
                      (GetSysColor(COLOR_APPWORKSPACE) == GetSysColor(COLOR_BTNHIGHLIGHT)));
    if(bNeedDiff)
    {
        DWORD rgbDiff = ((GetSysColor(COLOR_ACTIVEBORDER) != GetSysColor(COLOR_APPWORKSPACE)) 
                            ? GetSysColor(COLOR_ACTIVEBORDER)
                            : GetSysColor(COLOR_APPWORKSPACE) ^ 0x00FFFFFF);
        hbrDiff = CreateSolidBrush(rgbDiff);
    }

    //
    // Fill it with the selection color or the background color.
    //
    SetRect(&rc, 0, 0, w, h);
    FillRect(hdcT, &rc, 
             (fSelected ? ((GetSysColor(COLOR_APPWORKSPACE) != GetSysColor(COLOR_HIGHLIGHT)) 
                               ? GetSysColorBrush(COLOR_HIGHLIGHT)
                               : hbrDiff)
                        : GetSysColorBrush(COLOR_APPWORKSPACE)));

    InflateRect(&rc, -SELECTION_THICKNESS, -SELECTION_THICKNESS);
    FillRect(hdcT, &rc, 
             ((GetSysColor(COLOR_APPWORKSPACE) != GetSysColor(COLOR_BTNHIGHLIGHT)) 
                  ? GetSysColorBrush(COLOR_BTNHIGHLIGHT)
                  : hbrDiff));

    if(bNeedDiff)
    {
        DeleteObject(hbrDiff);
        hbrDiff = NULL;
    }

    InflateRect(&rc, -MONITOR_BORDER, -MONITOR_BORDER);
    FillRect(hdcT, &rc, GetSysColorBrush(COLOR_DESKTOP));

    //
    // fill bitmap with transparent color
    //
    SetBkColor(hdc,GetSysColor(COLOR_APPWORKSPACE));
    SetRect(&rc, 0, 0, cx, cy);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    //
    // copy bitmap to upper-left of bitmap
    //
    BitBlt(hdc,0,0,w,h,hdcT,0,0,SRCCOPY);

    //
    // draw the monitor number, if provided, in the bitmap (in the right place)
    //
    if(sz)
        DrawMonitorNum(hdc, w, h, sz, FALSE);

    //
    // make mask, if needed.
    //
    if(pMaskBitmap)
        BitBlt(hdcM,0,0,cx,cy,hdc,0,0,SRCCOPY);

    DeleteDC(hdc);
    DeleteDC(hdcT);
    if(pMaskBitmap)
        DeleteDC(hdcM);
    DeleteObject(hbmT);

    *pBitmap     = hbm;
    if(pMaskBitmap)
        *pMaskBitmap = hbmM;
    return TRUE;
}

//
// SnapMonitorRect
//
// called while the user is moving a monitor window (WM_MOVING)
// if the CTRL key is not down we will snap the window rect
// to the edge of one of the other monitors.
//
// this is done so the user can easily align monitors
//
// NOTE pDevice->Snap must be initialized to 0,0 in WM_ENTERSIZEMOVE
//
void SnapMonitorRect(PMULTIMON_DEVICE pDevice, HWND hwnd, RECT *prc)
{
    HWND hwndT;
    int  d;
    RECT rcT;
    RECT rc;

    //
    // allow the user to move the window anywhere when the CTRL key is down
    //
    if (GetKeyState(VK_CONTROL) & 0x8000)
        return;

    //
    // macros to help in alignment
    //
    #define SNAP_DX 6
    #define SNAP_DY 6

    #define SNAPX(f,x) \
        d = rcT.x - rc.f; if (abs(d) <= SNAP_DX) rc.left+=d, rc.right+=d;

    #define SNAPY(f,y) \
        d = rcT.y - rc.f; if (abs(d) <= SNAP_DY) rc.top+=d, rc.bottom+=d;

    //
    // get current rect and offset it by the amount we have corrected
    // it so far (this alignes the rect with the position of the mouse)
    //
    rc = *prc;
    OffsetRect(&rc, pDevice->Snap.x, pDevice->Snap.y);

    //
    // walk all other windows and snap our window to them
    //
    for (hwndT = GetWindow(hwnd,  GW_HWNDFIRST); hwndT;
         hwndT = GetWindow(hwndT, GW_HWNDNEXT))
    {
        if (hwndT == hwnd)
            continue;

        GetWindowRect(hwndT, &rcT);
        InflateRect(&rcT,SNAP_DX,SNAP_DY);

        if (IntersectRect(&rcT, &rcT, &rc))
        {
            GetWindowRect(hwndT, &rcT);
            SNAPX(right,left);  SNAPY(bottom,top);
            SNAPX(right,right); SNAPY(bottom,bottom);
            SNAPX(left,left);   SNAPY(top,top);
            SNAPX(left,right);  SNAPY(top,bottom);
        }
    }

    //
    // adjust the amount we have snap'ed so far, and return the new rect
    //
    pDevice->Snap.x += prc->left - rc.left;
    pDevice->Snap.y += prc->top  - rc.top;
    *prc = rc;
}

WPARAM GetKeyStates()
{
    WPARAM wParam = 0x0;

    if (GetKeyState(VK_CONTROL) & 0x8000) 
        wParam |= MK_CONTROL;
    if (GetKeyState(VK_LBUTTON) & 0x8000) 
        wParam |= MK_LBUTTON;
    if (GetKeyState(VK_MBUTTON) & 0x8000) 
        wParam |= MK_MBUTTON;
    if (GetKeyState(VK_RBUTTON) & 0x8000) 
        wParam |= MK_RBUTTON;
    if (GetKeyState(VK_SHIFT) & 0x8000) 
        wParam |= MK_SHIFT;
        
    return wParam;
}


LRESULT CALLBACK MonitorWindowProc(HWND hwnd, UINT msg,WPARAM wParam,LPARAM lParam)
{
    TOOLINFO ti;
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rc;
    int w,h;
    TCHAR ach[80];
    PMULTIMON_DEVICE pDevice;
    HWND hDlg = GetParent(GetParent(hwnd));
    RECT rcPos;
    MSG mmsg;
    CMultiMon * pcmm = (CMultiMon *) GetWindowLongPtr(hwnd, 0);
    
    switch (msg)
    {
        case WM_CREATE:
            ASSERT(((LPCREATESTRUCT)lParam)->lpCreateParams);
            SetWindowLongPtr(hwnd, 0, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
            break;

        case WM_NCCREATE:
            // turn off RTL_MIRRORED_WINDOW in GWL_EXSTYLE
            SHSetWindowBits(hwnd, GWL_EXSTYLE, RTL_MIRRORED_WINDOW, 0);
            break;
        case WM_NCHITTEST:
            //
            // return HTCAPTION so that we can get the ENTERSIZEMOVE message.
            //
            pDevice = GetDlgCtrlDevice(hwnd);
            // Let disabled monitors move
            if (pDevice) // if (pDevice && pDevice->pds->IsAttached())
                return HTCAPTION;
            break;

        case WM_NCLBUTTONDBLCLK:
            FlashText(hDlg, GetDlgCtrlDevice(hwnd), NULL,NULL,FALSE);
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DISPLAYPROPERTIES, BN_CLICKED), (LPARAM)hwnd );
            break;

        case WM_CHILDACTIVATE:
            if (GetFocus() != GetParent(hwnd)) {
                SetFocus(GetParent(hwnd));
            }
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, TEXT("display.hlp"), HELP_WM_HELP,
                (DWORD_PTR)(LPTSTR)sc_MultiMonitorHelpIds);
            break;

        case WM_CONTEXTMENU:
            WinHelp((HWND)wParam, TEXT("display.hlp"), HELP_CONTEXTMENU,
                (DWORD_PTR)(LPTSTR)sc_MultiMonitorHelpIds);
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_DISPLAYPRIME:
            case IDC_DISPLAYUSEME:
            case IDC_DISPLAYPROPERTIES:
                PostMessage(hDlg, WM_COMMAND, wParam, lParam);
                break;

            case IDC_FLASH:
                pDevice = GetDlgCtrlDevice(hwnd);

                pDevice->pds->GetOrgPosition(&rcPos);

                if (!IsRectEmpty(&rcPos))
                {
                    GetWindowText(hwnd, ach, ARRAYSIZE(ach));
                    FlashText(hDlg, pDevice, ach, &rcPos, FALSE);
                }
                break;

#if QUICK_REFRESH
            default: 
                ASSERT(LOWORD(wParam) - IDC_FREQUENCY_START > 0);

                pDevice = GetDlgCtrlDevice(hwnd);
                pDevice->pds->SetCurFrequency(LOWORD(wParam) - IDC_FREQUENCY_START);
                pcmm->SetDirty();
#endif
                break;
            }

            break;

        case WM_INITMENUPOPUP:
            pDevice = GetDlgCtrlDevice(hwnd);

            CheckMenuItem((HMENU)wParam, IDC_DISPLAYUSEME,
                          pDevice->pds->IsAttached() ? MF_CHECKED : MF_UNCHECKED);
            CheckMenuItem((HMENU)wParam, IDC_DISPLAYPRIME,
                          pDevice->pds->IsPrimary()  ? MF_CHECKED : MF_UNCHECKED);
            // until I figure out how to render on a non attached monitor, just
            // disable the menu item
            EnableMenuItem((HMENU)wParam, IDC_FLASH,
                           pDevice->pds->IsAttached() ? MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, IDC_DISPLAYPROPERTIES,
                           IsWindowEnabled(GetDlgItem(GetParent(GetParent(hwnd)), IDC_DISPLAYPROPERTIES)) ?
                           MF_ENABLED : MF_GRAYED);
            EnableMenuItem((HMENU)wParam, IDC_DISPLAYUSEME,
                           pDevice->pds->IsPrimary() ? MF_GRAYED : MF_ENABLED);

#ifndef WINNT
            EnableMenuItem((HMENU)wParam, IDC_DISPLAYPRIME, MF_GRAYED);
#else
            EnableMenuItem((HMENU)wParam, IDC_DISPLAYPRIME,
                           pDevice->pds->IsPrimary() || !pDevice->pds->IsAttached()
                               ? MF_GRAYED : MF_ENABLED);
#endif

            SetMenuDefaultItem((HMENU)wParam, IDC_DISPLAYPROPERTIES, MF_BYCOMMAND);
            break;

        case WM_NCMOUSEMOVE:
            ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, NULL, WM_MOUSEMOVE, GetKeyStates(), lParam);
            break;

        case WM_NCMBUTTONDOWN:
            ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, NULL, WM_MBUTTONDOWN, GetKeyStates(), lParam);
            break;

        case WM_NCMBUTTONUP:
            ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, NULL, WM_MBUTTONUP, GetKeyStates(), lParam);
            break;

        case WM_NCRBUTTONDOWN:
            ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, NULL, WM_RBUTTONDOWN, GetKeyStates(), lParam);
    
            pDevice = GetDlgCtrlDevice(hwnd);

            if (pDevice && pcmm)
            {
                HMENU hmenu;
                POINT pt;

                hmenu = LoadMenu(hInstance, MAKEINTRESOURCE(MENU_MONITOR));

                if (hmenu)
                {
#if QUICK_REFRESH
                    if (GetKeyState(VK_CONTROL) & 0x8000) {
                        HMENU hsub;
                        MENUITEMINFO mii;
                        TCHAR ach[LINE_LEN];
       
                        LoadString(hInstance, IDS_FREQUENCY, ach, SIZEOF(ach));

                        hsub = GetSubMenu(hmenu, 0);
                        ZeroMemory(&mii, sizeof(mii));
                        mii.cbSize = sizeof(mii);
                        mii.fMask = MIIM_TYPE;
                        mii.fType = MFT_SEPARATOR;
                        InsertMenuItem(hsub, GetMenuItemCount(hsub), TRUE, &mii);

                        mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_STATE;
                        mii.hSubMenu = CreateFrequencyMenu(pDevice); 
                        mii.fType = MFT_STRING;
                        mii.fState = MFS_ENABLED;
                        mii.dwTypeData = ach; 
                        InsertMenuItem(hsub, GetMenuItemCount(hsub), TRUE, &mii);

                        DestroyMenu(mii.hSubMenu);
                    }
#endif // QUICK_REFRESH

                    pcmm->UpdateActiveDisplay(pDevice);
                    GetCursorPos(&pt);
                    TrackPopupMenu(GetSubMenu(hmenu,0), TPM_RIGHTBUTTON,
                        pt.x, pt.y, 0, hwnd, NULL);

                    DestroyMenu(hmenu);
                }
            }
            break;

        case WM_NCRBUTTONUP:
            ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, NULL, WM_RBUTTONUP, GetKeyStates(), lParam);
            break;

        case WM_NCLBUTTONDOWN:
            //TraceMsg(TF_FUNC, "WM_NCLBUTTONDOWN");
            // don't relay the message here because we want to keep the tool tip
            // active until they start moving the monitor.  This click might just
            // be for selection
            // ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, hDlg, WM_LBUTTONDOWN, GetKeyStates(), lParam);

            BringWindowToTop(hwnd);
            pDevice = GetDlgCtrlDevice(hwnd);

            if (pcmm)
                pcmm->UpdateActiveDisplay(pDevice);

            pDevice->pds->GetOrgPosition(&rcPos);

            if (!IsRectEmpty(&rcPos))
            {
                GetWindowText(hwnd, ach, ARRAYSIZE(ach));
                FlashText(hDlg, pDevice, ach, &rcPos, TRUE);
            }

            break;

#if 0
        case WM_NCLBUTTONUP:
            ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, NULL, WM_LBUTTONUP, GetKeyStates(), lParam);
            break;
#endif 

        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code) {
            case TTN_NEEDTEXT: 
                pDevice = GetDlgCtrlDevice(hwnd);
                if (pDevice->pds->IsPrimary()){
                    LoadString(hInstance, IDS_PRIMARY, 
                               ((LPNMTTDISPINFO)lParam)->szText, 
                               SIZEOF(((LPNMTTDISPINFO)lParam)->szText) );
                }
                else if (!pDevice->pds->IsAttached()) {
                    LoadString(hInstance, IDS_NOTATTACHED, 
                               ((LPNMTTDISPINFO)lParam)->szText, 
                               SIZEOF(((LPNMTTDISPINFO)lParam)->szText) );
                }
                else {
                    TCHAR szSecondary[32];
                    LoadString(hInstance, IDS_SECONDARY, szSecondary, SIZEOF(szSecondary));
                    pDevice->pds->GetCurPosition(&rcPos);
                    wsprintf(((LPNMTTDISPINFO)lParam)->szText,
                             TEXT("%s (%d, %d)"), szSecondary, rcPos.left, rcPos.top);
                }
                break;
    
            default:
                break;
            }

            break;

        case WM_ENTERSIZEMOVE:
            //TraceMsg(TF_FUNC, "WM_ENTERSIZEMOVE");
            // relay a mouse up to clean the information tooltip
            ToolTip_RelayEvent(ghwndToolTipPopup, mmsg, NULL, WM_LBUTTONDOWN, GetKeyStates(), lParam);
            pDevice = GetDlgCtrlDevice(hwnd);
            pDevice->Snap.x = 0;
            pDevice->Snap.y = 0;
            FlashText(hDlg, pDevice, NULL,NULL,FALSE);
            break;

        case WM_MOVING:
            //TraceMsg(TF_FUNC, "WM_MOVING");
            pDevice = GetDlgCtrlDevice(hwnd);

            SnapMonitorRect(pDevice, hwnd, (RECT*)lParam);
            ZeroMemory(&ti, sizeof(ti));
            ti.cbSize = sizeof(TOOLINFO);

            if (!pDevice->bTracking) {
                ToolTip_TrackPosition(ghwndToolTipTracking,
                                      ((LPRECT)lParam)->left+2,
                                      ((LPRECT)lParam)->top+2);
                TrackToolTip(pDevice, hwnd, TRUE);
            }

            if (ToolTip_GetCurrentTool(ghwndToolTipTracking, &ti) && pcmm)
            {
                TCHAR location[16];
                POINT pt;

                pcmm->GetMonitorPosition(pDevice, GetParent(hwnd), &pt); 
                wsprintf(location, TEXT("%d, %d"), pt.x, pt.y); 

                ti.lpszText    = location;
                ti.rect.left   = ((RECT*)lParam)->left + 2;
                ti.rect.top    = ((RECT*)lParam)->top + 2;
                ti.rect.right  = ti.rect.left + 10;
                ti.rect.bottom = ti.rect.top + 10;

                ToolTip_SetToolInfo(ghwndToolTipTracking, &ti);
                ToolTip_TrackPosition(ghwndToolTipTracking, ti.rect.left, ti.rect.top);
                // SendMessage(ghwndToolTip, TTM_UPDATE, 0, 0);
            }

            break;

        case WM_EXITSIZEMOVE:
            //TraceMsg(TF_FUNC, "WM_EXITSIZEMOVE");
            pDevice = GetDlgCtrlDevice(hwnd);
            TrackToolTip(pDevice, hwnd, FALSE);

            //
            // We don't want to pop up any dialogs here because the modal size 
            // loop is still active (it eats any mouse movements and the dialogs
            // can't be moved by the user). 
            //
            PostMessage(hwnd, MM_MONITORMOVED, 0, 0);            
            break;

        case MM_MONITORMOVED:
            pDevice = GetDlgCtrlDevice(hwnd);
            if (pcmm)
            {
                //
                // If the user moved the monitor, see if they want to attach it
                //
                if (!pcmm->QueryNoAttach() && pDevice && !pDevice->pds->IsAttached())
                {
                    if (pcmm->SetMonAttached(pDevice, TRUE, FALSE, hwnd))
                    {
                        pcmm->UpdateActiveDisplay(pDevice);
                    }
                }
                pcmm->HandleMonitorChange(hwnd, FALSE);
            }
 
            RedrawWindow(GetParent(hwnd), NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
            return TRUE;

        case WM_DESTROY:
            FlashText(hDlg, GetDlgCtrlDevice(hwnd), NULL,NULL,FALSE);
            SetWindowLong(hwnd, 0, NULL);
            break;

        case WM_ERASEBKGND:
            //GetClientRect(hwnd, &rc);
            //FillRect((HDC)wParam, &rc, GetSysColorBrush(COLOR_APPWORKSPACE));
            return 0L;

        case WM_PAINT:

            hdc = BeginPaint(hwnd,&ps);
            GetWindowText(hwnd, ach, ARRAYSIZE(ach));
            GetClientRect(hwnd, &rc);
            w = rc.right;
            h = rc.bottom;

            pDevice = GetDlgCtrlDevice(hwnd);

            BOOL fSelected = (pcmm ? (BOOL)(pDevice == pcmm->GetCurDevice()) : FALSE);

            if (pDevice->w != w || pDevice->h != h)
            {
                HBITMAP hbm, hbmMask;
                int cx,cy;

                pDevice->w = w;
                pDevice->h = h;

                ImageList_GetIconSize(pDevice->himl, &cx, &cy);
                MakeMonitorBitmap(w,h,ach,&hbm,&hbmMask,cx,cy, fSelected);
                ImageList_Replace(pDevice->himl,pDevice->iImage,hbm,hbmMask);

                DeleteObject(hbm);
                DeleteObject(hbmMask);
            }

            if (!pDevice->pds->IsAttached())
            {
                FillRect(hdc, &rc, GetSysColorBrush(COLOR_APPWORKSPACE));

                if (pcmm && fSelected)
                {
                    ImageList_DrawEx(pDevice->himl,pDevice->iImage,hdc,0,0,w,h,
                        CLR_DEFAULT,CLR_DEFAULT,ILD_BLEND25);
                }
                else
                {
                    ImageList_DrawEx(pDevice->himl,pDevice->iImage,hdc,0,0,w,h,
                        CLR_DEFAULT,CLR_NONE,ILD_BLEND50);
                }
            }
            else
            {
                    ImageList_DrawEx(pDevice->himl,pDevice->iImage,hdc,0,0,w,h,
                        CLR_DEFAULT,CLR_DEFAULT,ILD_IMAGE);
            }

            EndPaint(hwnd,&ps);
            return 0L;
    }

    return DefWindowProc(hwnd,msg,wParam,lParam);
}

LRESULT CALLBACK SliderSubWndProc (HWND hwndSlider, UINT uMsg, WPARAM wParam, LPARAM lParam, WPARAM uID, ULONG_PTR dwRefData)
{
    ASSERT(uID == 0);
    ASSERT(dwRefData == 0);

    switch (uMsg)
    {
        case WM_GETOBJECT:
            if ( lParam == OBJID_CLIENT )
            {       
                // At this point we will try to load oleacc and get the functions
                // we need. 
                if (!g_fAttemptedOleAccLoad)
                {
                    g_fAttemptedOleAccLoad = TRUE;

                    ASSERT(s_pfnCreateStdAccessibleProxy == NULL);
                    ASSERT(s_pfnLresultFromObject == NULL);

                    g_hOleAcc = LoadLibrary(TEXT("OLEACC"));
                    if (g_hOleAcc != NULL)
                    {
#ifdef UNICODE
                        s_pfnCreateStdAccessibleProxy = (PFNCREATESTDACCESSIBLEPROXY)
                                                    GetProcAddress(g_hOleAcc, "CreateStdAccessibleProxyW");
#else
                        s_pfnCreateStdAccessibleProxy = (PFNCREATESTDACCESSIBLEPROXY)
                                                    GetProcAddress(g_hOleAcc, "CreateStdAccessibleProxyA");
#endif
                        s_pfnLresultFromObject = (PFNLRESULTFROMOBJECT)
                                                    GetProcAddress(g_hOleAcc, "LresultFromObject");
                    }
                    if (s_pfnLresultFromObject == NULL || s_pfnCreateStdAccessibleProxy == NULL)
                    {
                        // No point holding on to Oleacc since we can't use it.
                        FreeLibrary(g_hOleAcc);
                        g_hOleAcc = NULL;
                        s_pfnLresultFromObject = NULL;
                        s_pfnCreateStdAccessibleProxy = NULL;
                    }
                }

                
                if (g_hOleAcc && s_pfnCreateStdAccessibleProxy && s_pfnLresultFromObject)
                {
                    IAccessible *pAcc = NULL;
                    HRESULT hr;
                
                    // Create default slider proxy.
                    hr = s_pfnCreateStdAccessibleProxy(
                            hwndSlider,
                            TEXT("msctls_trackbar32"),
                            OBJID_CLIENT,
                            IID_IAccessible,
                            (void **)&pAcc
                            );


                    if (SUCCEEDED(hr) && pAcc)
                    {
                        // now wrap it up in our customized wrapper...
                        IAccessible * pWrapAcc = new CAccessibleWrapper( hwndSlider, pAcc );
                        // Release our ref to proxy (wrapper has its own addref'd ptr)...
                        pAcc->Release();
                    
                        if (pWrapAcc != NULL)
                        {

                            // ...and return the wrapper via LresultFromObject...
                            LRESULT lr = s_pfnLresultFromObject( IID_IAccessible, wParam, pWrapAcc );
                            // Release our interface pointer - OLEACC has its own addref to the object
                            pWrapAcc->Release();

                            // Return the lresult, which 'contains' a reference to our wrapper object.
                            return lr;
                            // All done!
                        }
                    // If it didn't work, fall through to default behavior instead. 
                    }
                }
            }
            break;

        case WM_DESTROY:
            RemoveWindowSubclass(hwndSlider, SliderSubWndProc, uID);
            break;    

    } /* end switch */

    return DefSubclassProc(hwndSlider, uMsg, wParam, lParam);
}
