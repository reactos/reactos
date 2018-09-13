////////////////////////////////////////////////////////////////////////////
// SSWND.CPP
//
// Implementation of CScreenSaverWindow
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
// jaym     01/24/97    Added password verification
// jaym     04/15/97    Toolbar UI updates
// jaym     05/05/97    Removed Toolbar
// jaym     05/19/97    Re-added Toolbar
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include <regstr.h>
#include "regini.h"
#include "resource.h"
#include "sswnd.h"

#define TF_DEBUGQI          0
#define TF_DEBUGQS          0
#define TF_DEBUGREFCOUNT    0
#define TF_NAVIGATION       0
#define TF_NOTIFY           0
#define TF_TIMER            0
#define TF_FUNCENTRY        0
#define TF_CLOSE            TF_ALWAYS

#define WM_LOADSCREENSAVER  (WM_USER+550)

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
BOOL                g_bPasswordEnabled;
extern BOOL         g_bPlatformNT;
extern TCHAR        g_szRegSubKey[];
extern IMalloc *    g_pMalloc;
extern IMMASSOCPROC g_pfnIMMProc;

/////////////////////////////////////////////////////////////////////////////
// Module variables
/////////////////////////////////////////////////////////////////////////////
static CScreenSaverWindow * s_pThis = NULL;
static HHOOK                s_hKeyboardHook = NULL;
static HHOOK                s_hMouseHook = NULL;

#pragma data_seg(DATASEG_READONLY)
static const TCHAR      s_szScreenSaverKey[] = REGSTR_PATH_SCREENSAVE;
static const TCHAR      s_szPasswordActiveValue[] = REGSTR_VALUE_USESCRPASSWORD;
static const TCHAR      s_szPasswordActiveOnNTValue[] = TEXT("ScreenSaverIsSecure");
static const TCHAR      s_szPasswordValue[] = REGSTR_VALUE_SCRPASSWORD;
static const TCHAR      s_szPasswordDLL[] = TEXT("PASSWORD.CPL");
static const TCHAR      s_szPasswordFnName[] = TEXT("VerifyScreenSavePwd");
    // Password

static const TCHAR      s_szRegLastURL[]        = TEXT("LastURL");
static const TCHAR      s_szRegLastNavURL[]     = TEXT("LastNavURL");
static const TCHAR      s_szSETermEvent[]       = TEXT("ActSaverSEEvent");
static const TCHAR      s_szTrayUIWindow[]      = TEXT("MS_WebcheckMonitor");
static const TCHAR      s_szRES[]               = TEXT("res://");
static const TCHAR      s_szSSEmptyFile[]       = TEXT("/ssempty.htm");
static const OLECHAR    s_szYes[]               = OLESTR("YES");
static const OLECHAR    s_szNo[]                = OLESTR("NO");
static const OLECHAR    s_szDefTransition[]     = OLESTR("revealTrans(Duration=5.0, Transition=15)");
    // Misc.
#pragma data_seg()

/////////////////////////////////////////////////////////////////////////////
// Design constants
/////////////////////////////////////////////////////////////////////////////
#define MOUSEMOVE_THRESHOLD         10  // In pixels
    // Number of pixels that the mouse must move to determine a
    // 'real' mouse move event (for closing the Screen Saver).

#define ID_CHANNELCHANGE_TIMER      0x0100
    // Channel change/update timer ID

#define ID_CONTROLS_TIMER           0x0101
    // Scrollbar/Toolbar show/hide timer ID

#define ID_CLICKCHECK_TIMER         0x0102
    // Hotlink click check timer ID

#define ID_RESTART_TIMER            0x0103
    // Restart timer ID

#define TIME_CONTROLS_TIMEOUT       3000    // ms (3 seconds)
    // Scrollbar/Toolbar show/hide timeout

#define TIME_CLICKCHECK_TIMEOUT     1000    // ms (1 second)
    // Hotlink click check timeout

#define TIME_NAVIGATE_TIMEOUT       10000   // ms (10 seconds)
    // WebBrowser Navigate timeout

#define CX_TOOLBAR                  51
#define CY_TOOLBAR                  29
    // Toolbar dimensions

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow
/////////////////////////////////////////////////////////////////////////////
CScreenSaverWindow::CScreenSaverWindow
(
)
{
    m_cRef = 0;

    m_pToolbar = NULL;

    m_bModelessEnabled = TRUE;
    m_bMouseClicked = FALSE;
    m_bScrollbarVisible = TRUE;

    m_pScreenSaver = NULL;
    m_pSSConfig = NULL;

    m_idChangeTimer = 0;
    m_idControlsTimer = 0;
    m_idClickCheckTimer = 0;
    m_idReloadTimer = 0;

    m_pPIDLList = NULL;
    m_pidlDefault = NULL;
    m_lCurrChannel = CURRENT_CHANNEL_NONE;

    m_pUnkBrowser = NULL;
    m_pWebBrowser = NULL;
    m_pOleObject = NULL;
    m_pHlinkFrame = NULL;

    m_pHTMLDocument = NULL;

    m_dwWebBrowserEvents = 0L;
    m_dwWebBrowserEvents2 = 0L;
    m_dwHTMLDocumentEvents = 0L;

    m_pBindStatusCallback = NULL;

    m_hPasswordDLL = NULL;
    m_pfnVerifyPassword = NULL;

    m_hPrevBrowserIMC = 0;

    m_lVerifingPassword = FALSE;
}

CScreenSaverWindow::~CScreenSaverWindow
(
)
{
    // All clean-up is done in OnDestroy and OnNCDestroy.
}

//
// Helper routine to check appropriate registry value to see if screensaver
// is password protected.
//
BOOL IsScreenSaverPasswordProtected()
{
    BOOL bRetValue = FALSE;
    HKEY hKey;

    if (RegOpenKey( HKEY_CURRENT_USER,
                    s_szScreenSaverKey,
                    &hKey) == ERROR_SUCCESS)
    {
        //
        // Registry key string is different on Win95 & NT !!!
        //
        if (g_bPlatformNT)
        {
            CHAR buf[4];
            DWORD dwSize = sizeof(buf);

            if (RegQueryValueEx(hKey, s_szPasswordActiveOnNTValue,
                                NULL,
                                NULL,
                                (BYTE *)buf,
                                &dwSize) == ERROR_SUCCESS)
            {
                //
                // check if "1" is returned in buf
                //
                if (dwSize == 2 && buf[0] == '1')
                {
                    bRetValue = TRUE;
                }
            }
        }
        else
        {
            DWORD dwVal;
            DWORD dwSize = sizeof(DWORD);

            if  (
                (RegQueryValueEx(   hKey,
                                    s_szPasswordActiveValue,
                                    NULL,
                                    NULL,
                                    (BYTE *)&dwVal,
                                    &dwSize) == ERROR_SUCCESS)
                &&
                (dwVal != 0)
                )
            {
                bRetValue = TRUE;
            }
        }
        RegCloseKey(hKey);
    }

    return bRetValue;
}



/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::Create
/////////////////////////////////////////////////////////////////////////////
BOOL CScreenSaverWindow::Create
(
    const RECT &    rect,
    HWND            hwndParent,
    IScreenSaver *  pScreenSaver
)
{
    HRESULT hrResult;

    ASSERT(pScreenSaver != NULL);
    TraceMsg(TF_ALWAYS, "CScreenSaverWindow::Create(hwndParent=%x)", hwndParent);

    //
    // Set the g_bPasswordEnabled flag
    //
    g_bPasswordEnabled = IsScreenSaverPasswordProtected();

    for (;;)
    {
        // Save this pointer for Hook functions.
        s_pThis = this;

        // Save IScreenSaver interface pointer.
        m_pScreenSaver = pScreenSaver;
        m_pScreenSaver->AddRef();

        // Get the Configuration interface pointer.
        if (FAILED(hrResult = m_pScreenSaver->QueryInterface(IID_IScreenSaverConfig,
                                                             (void **)&m_pSSConfig)))
            break;

        DWORD dwFeatureFlags;
        EVAL(SUCCEEDED(m_pSSConfig->get_Features(&dwFeatureFlags)));

        // Create the actual window.
        if (!CWindow::CreateEx( NULL,
                                ((dwFeatureFlags & FEATURE_NOT_TOPMOST)
                                    ? 0
                                    : (WS_EX_TOOLWINDOW | WS_EX_TOPMOST)),
                                WS_POPUP,
                                rect,
                                hwndParent,
                                0))
        {
            hrResult = E_OUTOFMEMORY;
            break;
        }

        TraceMsg(TF_ALWAYS, "CScreenSaverWindow::Create(m_hWnd=%x)", m_hWnd );

        // If we aren't on NT, load the password verification DLL.
        if (!g_bPlatformNT)
            LoadPasswordDLL();

        // Create PIDL for default HTML page
        char szModuleName[MAX_PATH];
        GetModuleFileName(_pModule->GetModuleInstance(), szModuleName, MAX_PATH);

        CString strURLDefault;
        strURLDefault = s_szRES;
        strURLDefault += PathFindFileName(szModuleName);
        strURLDefault += s_szSSEmptyFile;

        EVAL(CreatePIDLFromPath(strURLDefault, &m_pidlDefault));

        // Create the toolbar.
        RECT rectToolbar;
        rectToolbar.left    = rect.right - CX_TOOLBAR;
        rectToolbar.top     = 0;
        rectToolbar.right   = rectToolbar.left + CX_TOOLBAR;
        rectToolbar.bottom  = rectToolbar.top + CY_TOOLBAR;

        m_pToolbar = new CToolbarWindow;

        if (NULL == m_pToolbar)
        {
            hrResult = E_OUTOFMEMORY;
            break;
        }

        if (!m_pToolbar->Create(rectToolbar, this))
        {
            delete m_pToolbar;
            m_pToolbar = NULL;

            hrResult = E_OUTOFMEMORY;
            break;
        }

        // Initialize the channel view list.
        InitChannelList();

        //  TODO: Darrenmi add online/offline registration here.

        // Initialize the web browser.
        if (!CreateWebBrowser(rect))
        {
            CString strCaption;
            CString strText;

            // Display error message.
            strCaption.LoadString(IDS_SCREENSAVER_DESC);
            strText.LoadString(IDS_ERROR_REQUIREMENTSNOTMET);
            MessageBox(hwndParent, strText, strCaption, MB_OK);

            hrResult = E_OUTOFMEMORY;
            break;
        }

        // Setup the Keyboard and Mouse hooks.
        SetHooks();

        // Show the first channel.
        DisplayCurrentChannel();

        // Start the reload timer.
        if (!g_bPlatformNT)
        {
            DWORD dwRestartTime;
            EVAL(SUCCEEDED(m_pSSConfig->get_RestartTime(&dwRestartTime)));
            EVAL((m_idReloadTimer = SetTimer(   ID_RESTART_TIMER,
                                                dwRestartTime,
                                                NULL)) != 0);
        }

        hrResult = S_OK;
        break;
    }

    return SUCCEEDED(hrResult);
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::ShowControls
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::ShowControls
(
    BOOL bShow
)
{
    TraceMsg(   TF_FUNCENTRY,
                "ShowControls(%d): m_bScrollbarVisible = %d, Toolbar = %d",
                bShow, m_bScrollbarVisible, m_pToolbar->IsWindowVisible());

    // Kill the current 'controls timer' and restart if needed.
    if (m_idControlsTimer != 0)
    {
        EVAL(KillTimer(m_idControlsTimer));
        m_idControlsTimer = 0;
    }

    // Screen saver does not need the IME.                
    if (g_pfnIMMProc != NULL)
    {
        if (m_hPrevBrowserIMC != 0)
        {
            g_pfnIMMProc(m_hwndContainer, m_hPrevBrowserIMC);
            m_hPrevBrowserIMC = 0;
        }

        ASSERT(m_hwndContainer != NULL)
        m_hPrevBrowserIMC = g_pfnIMMProc(m_hwndContainer, (HIMC)NULL);
    }

    IHTMLBodyElement * pHTMLBodyElement = NULL;
    BOOL               bLongContent = FALSE;

    RECT rectClient;
    GetClientRect(&rectClient);

    // Check to see if the content is long enough to warrant scrollbars.
    if  (
        SUCCEEDED(GetHTMLBodyElement(&pHTMLBodyElement))
        &&
        (pHTMLBodyElement != NULL)
        )
    {
        IHTMLTextContainer * pTxtEdit = NULL;

        if  (
            (SUCCEEDED(pHTMLBodyElement->QueryInterface(IID_IHTMLTextContainer,
                                                        (void **)&pTxtEdit)))
            &&
            (pTxtEdit != NULL)
            )
        {
            long lHeight;
            EVAL(SUCCEEDED(pTxtEdit->get_scrollHeight(&lHeight)));

            bLongContent = (lHeight > (rectClient.bottom - rectClient.top));

            pTxtEdit->Release();
        }
    }

    // Don't show scrollbar if we don't need them.
    if (pHTMLBodyElement != NULL)
    {
        if (m_bScrollbarVisible != bShow)
        {
            BSTR bstrScroll = SysAllocString(((bShow && bLongContent)? s_szYes : s_szNo));

            if (bstrScroll)
            {
                pHTMLBodyElement->put_scroll(bstrScroll);
                SysFreeString(bstrScroll);
                m_bScrollbarVisible = bShow;
            }
        }

        pHTMLBodyElement->Release();
    }

    // Show the toolbar, too.
    if ((m_pToolbar != NULL) && (m_pToolbar->IsWindowVisible() != bShow))
    {
        RECT rectToolbar;

        m_pToolbar->GetWindowRect(&rectToolbar);

        // Position toolbar appropriately, depending
        // on visibility of the scrollbar.
        if (bLongContent)
        {
            if (rectToolbar.right == rectClient.right)
            {
                OffsetRect(&rectToolbar, -GetSystemMetrics(SM_CXVSCROLL), 0);

                m_pToolbar->SetWindowPos(   NULL,
                                            &rectToolbar,
                                            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
            }
        }
        else
        {
            if (rectToolbar.right != rectClient.right)
            {
                OffsetRect(&rectToolbar, GetSystemMetrics(SM_CXVSCROLL), 0);

                m_pToolbar->SetWindowPos(   NULL,
                                            &rectToolbar,
                                            SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
            }
        }

        m_pToolbar->ShowToolbar(bShow);
    }

    if  (
        m_bScrollbarVisible
        ||
        ((m_pToolbar != NULL) && m_pToolbar->IsWindowVisible())
        )
    {
        EVAL((m_idControlsTimer = SetTimer( ID_CONTROLS_TIMER,
                                            TIME_CONTROLS_TIMEOUT,
                                            NULL)) != 0);
    }

    // REVIEW: Should use UIActivate? [jaym]
    POINT pt = { 0 };
    m_hwndContainer = WindowFromPoint(pt);
    SetFocus(m_hwndContainer);
}


/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::ShowPropertiesDlg
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::ShowPropertiesDlg
(
    HWND hwndParent
)
{
    // Save the 'current' channel.
    SaveState();            

    EnableModeless(FALSE);
    m_pSSConfig->ShowDialog(hwndParent);
    EnableModeless(TRUE);

    // Reinitialize the internal channel list.
    InitChannelList();

    // Show the first channel in the new list.
    DisplayCurrentChannel();
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnCreate
/////////////////////////////////////////////////////////////////////////////
BOOL CScreenSaverWindow::OnCreate
(
    CREATESTRUCT * pcs
)
{
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnDestroy
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::OnDestroy
(
)
{
    TraceMsg(TF_CLOSE, "CScreenSaverWindow::OnDestroy() BEGIN");

    ReleaseHooks();

    if (m_idChangeTimer != 0)
        EVAL(KillTimer(m_idChangeTimer));

    if (m_idControlsTimer != 0)
        EVAL(KillTimer(m_idControlsTimer));

    if (m_idClickCheckTimer != 0)
        EVAL(KillTimer(m_idClickCheckTimer));

    if (m_idReloadTimer != 0)
        EVAL(KillTimer(m_idReloadTimer));

    if (m_pToolbar != NULL)
        delete m_pToolbar;

    if (m_hPrevBrowserIMC != 0)
    {
        ASSERT(m_hwndContainer != NULL);
        g_pfnIMMProc(m_hwndContainer, m_hPrevBrowserIMC);
    }

    //  TODO: Darrenmi add online/offline deregistration here.

    ConnectToConnectionPoint(   SAFECAST(this, IDispatch *),
                                DIID_DWebBrowserEvents,
                                FALSE,
                                m_pUnkBrowser,
                                &m_dwWebBrowserEvents,
                                NULL);

    ConnectToConnectionPoint(   SAFECAST(this, IDispatch *),
                                DIID_DWebBrowserEvents2,
                                FALSE,
                                m_pUnkBrowser,
                                &m_dwWebBrowserEvents2,
                                NULL);

    if (m_pHTMLDocument != NULL)
    {
        // Disconnect current connection to HTMLDocumentEvents
        ConnectToConnectionPoint(   SAFECAST(this, IDispatch *),
                                    DIID_HTMLDocumentEvents,
                                    FALSE,
                                    m_pHTMLDocument,
                                    &m_dwHTMLDocumentEvents,
                                    NULL);
        m_pHTMLDocument->Release();
    }

    OleSetContainedObject((IUnknown *)m_pOleObject, FALSE);

    if (m_pHlinkFrame != NULL)
        m_pHlinkFrame->Release();

    if (m_pOleObject != NULL)
    {
        m_pOleObject->DoVerb(   OLEIVERB_HIDE,
                                NULL,
                                SAFECAST(this, IOleClientSite *),
                                0,
                                m_hWnd,
                                NULL);

        m_pOleObject->SetClientSite(NULL);
        m_pOleObject->Close(OLECLOSE_NOSAVE);

        m_pOleObject->Release();
    }

    if (m_pWebBrowser != NULL)
    {
        m_pWebBrowser->Stop();
        m_pWebBrowser->Release();
    }

    if (m_pBindStatusCallback != NULL)
        EVAL(m_pBindStatusCallback->Release() == 0);

    if (m_hPasswordDLL != NULL)
        UnloadPasswordDLL();

    if (m_pPIDLList != NULL)
        delete m_pPIDLList;

    if (m_pidlDefault != NULL)
        g_pMalloc->Free(m_pidlDefault);

    if (m_pUnkBrowser != NULL)
        m_pUnkBrowser->Release();

    if (m_pSSConfig != NULL)
        m_pSSConfig->Release();

    m_pScreenSaver->put_Running(FALSE);

    if (m_pScreenSaver != NULL)
        m_pScreenSaver->Release();

    CWindow::OnDestroy();

    PostQuitMessage(0);

    TraceMsg(TF_CLOSE, "CScreenSaverWindow::OnDestroy() END");
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnEraseBkgnd
/////////////////////////////////////////////////////////////////////////////
BOOL CScreenSaverWindow::OnEraseBkgnd
(
    HDC hDC
)
{
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnTimer
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::OnTimer
(
    UINT nIDTimer
)
{
    if  (
        // Ignore if there is a dialog up or we are disabled.
        !m_bModelessEnabled
        ||
        // Ignore if the user is selecting or scrolling.
        (GetAsyncKeyState((GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON)) & 0x8000)
        )
    {
        return;
    }

    switch (nIDTimer)
    {
        case ID_CONTROLS_TIMER:
        {
            TraceMsg(TF_TIMER, "CScreenSaverWindow::OnTimer(ID_CONTROLS_TIMER)");

            ShowControls(FALSE);
            break;
        }

        case ID_CHANNELCHANGE_TIMER:
        {
            TraceMsg(TF_TIMER, "CScreenSaverWindow::OnTimer(ID_CHANNELCHANGE_TIMER)");

            OnChannelChangeTimer();
            break;
        }

        case ID_CLICKCHECK_TIMER:
        {
            TraceMsg(TF_TIMER, "CScreenSaverWindow::OnTimer(ID_CLICKCHECK_TIMER)");

            ASSERT(m_idClickCheckTimer != 0);
            ASSERT(m_bMouseClicked);

            EVAL(KillTimer(m_idClickCheckTimer));
            m_idClickCheckTimer = 0;

            Quit();

            break;
        }

        case ID_RESTART_TIMER:
        {
            HWND hwndTrayUI;

            TraceMsg(TF_TIMER, "CScreenSaverWindow::OnTimer(ID_RELOAD_TIMER)");

            // Send message which will be handled in
            // WebCheck to restart the screen saver.
            if ((hwndTrayUI = FindWindow(s_szTrayUIWindow, s_szTrayUIWindow)) != NULL)
            {
                ::PostMessage(hwndTrayUI, WM_LOADSCREENSAVER, 0, 0);

                EVAL(KillTimer(m_idReloadTimer));
                m_idReloadTimer = 0;

                Quit(TRUE);
            }

            break;
        }

        default:
            ASSERT(FALSE);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnUserMessage
/////////////////////////////////////////////////////////////////////////////
LRESULT CScreenSaverWindow::OnUserMessage
(
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    switch (uMsg)
    {
        case WM_ABORTNAVIGATE:
            return NavigateToDefault();

        case WM_DEATHTOSAVER:
            DestroyWindow(m_hWnd);
            break;
            
        default:
            break;
    }

    return -1;
}

////////////////////////////////////////////////////////////////////////////
//
//
#define PARAM_PROCESSED     0
#define PARAM_HEADERS       1
#define PARAM_POST_DATA     2
#define PARAM_FRAME_NAME    3
#define PARAM_FLAGS         4
#define PARAM_URL           5
//
//
//
////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnBeforeNavigate
/////////////////////////////////////////////////////////////////////////////
#define NUM_PARAM_NEWWINDOW 6



HRESULT CScreenSaverWindow::OnNewWindow
(
    DISPPARAMS *    pDispParams,
    VARIANT *       pVarResult
)
{
    HRESULT hrResult;

    for (;;)
    {
        if (pDispParams == NULL)
        {
            hrResult = DISP_E_PARAMNOTOPTIONAL;
            break;
        }

        if (pDispParams->cArgs != NUM_PARAM_NEWWINDOW)
        {
            hrResult = DISP_E_BADPARAMCOUNT;
            break;
        }


            // Check for password.
        if (VerifyPassword())
        {
            CString strNavigateURL;

            ASSERT(pDispParams->rgvarg[PARAM_URL].vt == VT_BSTR);
            strNavigateURL = pDispParams->rgvarg[PARAM_URL].bstrVal;

            TraceMsg(TF_NAVIGATION, "Navigate URL = '%s'\r\n", (LPCTSTR)strNavigateURL);

            if (LaunchBrowser(strNavigateURL))
            {
                // Save our state and quit the Screen Saver
                // if we successfully launched the browser.
                TraceMsg(TF_CLOSE, "Attempting to close screen saver");

                Quit(TRUE);
            }
            else
                MessageBeep(MB_ICONEXCLAMATION);
        }

        // Set the cancel flag to TRUE to disallow
        // shdocvw from launching a new window
        ASSERT(pDispParams->rgvarg[PARAM_PROCESSED].vt == (VT_BYREF | VT_BOOL));
        *(pDispParams->rgvarg[PARAM_PROCESSED].pboolVal) = VARIANT_TRUE;

        hrResult = S_OK;
        break;
    }

    return hrResult;
}




/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnBeforeNavigate
/////////////////////////////////////////////////////////////////////////////
#define NUM_PARAM_BEFORENAV 6

#define PARAM_PROCESSED     0
#define PARAM_HEADERS       1
#define PARAM_POST_DATA     2
#define PARAM_FRAME_NAME    3
#define PARAM_FLAGS         4
#define PARAM_URL           5

HRESULT CScreenSaverWindow::OnBeforeNavigate
(
    DISPPARAMS *    pDispParams,
    VARIANT *       pVarResult
)
{
    HRESULT hrResult;

    for (;;)
    {
        if (pDispParams == NULL)
        {
            hrResult = DISP_E_PARAMNOTOPTIONAL;
            break;
        }

        if (pDispParams->cArgs != NUM_PARAM_BEFORENAV)
        {
            hrResult = DISP_E_BADPARAMCOUNT;
            break;
        }

        DWORD dwFeatureFlags;
        EVAL(SUCCEEDED(m_pSSConfig->get_Features(&dwFeatureFlags)));

        // We'll skip everything until a mouse click happens
        if (m_bMouseClicked && !(dwFeatureFlags & FEATURE_IGNORE_MOUSE_CLICKS))
        {
            ASSERT(m_idClickCheckTimer != 0);

            EVAL(KillTimer(m_idClickCheckTimer));
            m_idClickCheckTimer = 0;

            // Check for password.
            if (VerifyPassword())
            {
                CString strNavigateURL;

                ASSERT(pDispParams->rgvarg[PARAM_URL].vt == VT_BSTR);
                strNavigateURL = pDispParams->rgvarg[PARAM_URL].bstrVal;

                TraceMsg(TF_NAVIGATION, "Navigate URL = '%s'\r\n", (LPCTSTR)strNavigateURL);

                if (LaunchBrowser(strNavigateURL))
                {
                    // Save our state and quit the Screen Saver
                    // if we successfully launched the browser.
                    TraceMsg(TF_CLOSE, "Attempting to close screen saver");

                    Quit(TRUE);
                }
                else
                    MessageBeep(MB_ICONEXCLAMATION);
            }

            // Set the cancel flag to TRUE to disallow
            // navigation in the Screen Saver itself.
            ASSERT(pDispParams->rgvarg[PARAM_PROCESSED].vt == (VT_BYREF | VT_BOOL));
            *(pDispParams->rgvarg[PARAM_PROCESSED].pboolVal) = VARIANT_TRUE;
        }

        hrResult = S_OK;
        break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnDocumentComplete
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::OnDocumentComplete
(
    BSTR bstrURL
)
{
    if (m_pHTMLDocument != NULL)
    {
        // Disconnect current connection to HTMLDocumentEvents
        ConnectToConnectionPoint(   SAFECAST(this, IDispatch *),
                                    DIID_HTMLDocumentEvents,
                                    FALSE,
                                    m_pHTMLDocument,
                                    &m_dwHTMLDocumentEvents,
                                    NULL);
        m_pHTMLDocument->Release();
        m_pHTMLDocument = NULL;
    }

    // Connect to new document HTMLDocumentEvents connection
    if  (
        SUCCEEDED(m_pWebBrowser->get_Document(&m_pHTMLDocument))
        &&
        (m_pHTMLDocument != NULL)
        )
    {
        ConnectToConnectionPoint(   SAFECAST(this, IDispatch *),
                                    DIID_HTMLDocumentEvents,
                                    TRUE,
                                    m_pHTMLDocument,
                                    &m_dwHTMLDocumentEvents,
                                    NULL);
    }
    else
        TraceMsg(TF_NAVIGATION, "OnDocumentComplete: Unable to get_Document()!");

    // Set the current URL property.
    SUCCEEDED(m_pScreenSaver->put_CurrentURL(bstrURL));

#ifdef _DEBUG
    CString strURL = bstrURL;
    TraceMsg(TF_NAVIGATION, "OnDocumentComplete: Current URL is \"%s\"", (LPCTSTR)strURL);
#endif  // _DEBUG

    // Hide the scrollbar and any other controls
    ShowControls(FALSE);

    // Start the channel change timer again.
    ResetChangeTimer();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnTitleChange
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::OnTitleChange
(
    BSTR bstrTitle
)
{
    TraceMsg(TF_FUNCENTRY, "OnTitleChange()");

    CString strTitle = bstrTitle;
    SetWindowText(strTitle);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::CreateWebBrowser
/////////////////////////////////////////////////////////////////////////////
BOOL CScreenSaverWindow::CreateWebBrowser
(
    const RECT & rect
)
{
    HRESULT hrResult;

    ASSERT(m_hWnd);

    for (;;)
    {
        CLSID clsidBrowser;

        if  (
            FAILED(CLSIDFromProgID(OLESTR("Shell.Explorer"), &clsidBrowser))
            ||
            FAILED(hrResult = CoCreateInstance( clsidBrowser,
                                                NULL,
                                                CLSCTX_INPROC_SERVER
                                                    | CLSCTX_LOCAL_SERVER,
                                                IID_IUnknown,
                                                (void **)&m_pUnkBrowser))
            )
        {
            TraceMsg(TF_ERROR, "Failed to create Web Browser object.");
            break;
        }

        if (FAILED(hrResult = m_pUnkBrowser->QueryInterface(IID_IWebBrowser2,
                                                            (void **)&m_pWebBrowser)))
        {
            TraceMsg(TF_ERROR, "Web browser doesn't support IWebBrowser2!");
            break;
        }

        if  (
            FAILED(hrResult = m_pUnkBrowser->QueryInterface(IID_IOleObject, (void **)&m_pOleObject))
            ||
            FAILED(hrResult = m_pOleObject->SetClientSite(SAFECAST(this, IOleClientSite *)))
            )
        {
            TraceMsg(TF_ERROR, "Failure attempting to setup IOleObject!");
            break;
        }

        IPersistStreamInit * pPersistStreamInit;
        if (SUCCEEDED(hrResult = m_pOleObject->QueryInterface(IID_IPersistStreamInit, (void **)&pPersistStreamInit)))
        {
            pPersistStreamInit->InitNew();
            pPersistStreamInit->Release();
        }

        OleSetContainedObject((IUnknown *)m_pOleObject, TRUE);

        ConnectToConnectionPoint(   SAFECAST(this, IDispatch *),
                                    DIID_DWebBrowserEvents,
                                    TRUE,
                                    m_pUnkBrowser,
                                    &m_dwWebBrowserEvents,
                                    NULL);

        ConnectToConnectionPoint(   SAFECAST(this, IDispatch *),
                                    DIID_DWebBrowserEvents2,
                                    TRUE,
                                    m_pUnkBrowser,
                                    &m_dwWebBrowserEvents2,
                                    NULL);

        // Make sure we are known as the browser
        EVAL(SUCCEEDED(m_pWebBrowser->put_RegisterAsBrowser(VARIANT_TRUE)));

        // We don't need to handle drops in the screen saver.
        EVAL(SUCCEEDED(m_pWebBrowser->put_RegisterAsDropTarget(VARIANT_FALSE)));

        // Make sure no dialogs are displayed for error conditions.
        EVAL(SUCCEEDED(m_pWebBrowser->put_Silent(VARIANT_TRUE)));

        // Make sure we get benefits of running theater mode.
        // (in particular the flat scrollbar)
        EVAL(SUCCEEDED(m_pWebBrowser->put_TheaterMode(VARIANT_TRUE)));

        // Get the Target frame and don't allow,
        // resizing, and turn off the 3D border.
        ITargetEmbedding * pTargetEmbedding;
        if (SUCCEEDED(hrResult = m_pUnkBrowser->QueryInterface( IID_ITargetEmbedding,
                                                                (void **)&pTargetEmbedding)))
        {
            ITargetFrame * pTargetFrame;

            // Save the ITargetFrame interface pointer.
            if (SUCCEEDED(hrResult = pTargetEmbedding->GetTargetFrame(&pTargetFrame)))
            {
                DWORD dwFrameOpts;
                if (SUCCEEDED(pTargetFrame->GetFrameOptions(&dwFrameOpts)))
                {
                    dwFrameOpts |= (FRAMEOPTIONS_NORESIZE
                                        | FRAMEOPTIONS_NO3DBORDER);

                    EVAL(SUCCEEDED(pTargetFrame->SetFrameOptions(dwFrameOpts)));
                }

                // Save the HLINK frame for navigation and setup purposes.
                hrResult = pTargetFrame->QueryInterface(IID_IHlinkFrame,
                                                        (void **)&m_pHlinkFrame);

                pTargetFrame->Release();
            }

            pTargetEmbedding->Release();
        }

        if (FAILED(hrResult))
        {
            TraceMsg(TF_ERROR, "Error attempting to initialize TargetEmbedding.");
            break;
        }

        // Make sure we are running in the current mode.
        DWORD dwFlags;
        VARIANT_BOOL bOffline = (InternetGetConnectedState(&dwFlags, 0)
                                    ? VARIANT_FALSE
                                    : VARIANT_TRUE);

#ifdef _DEBUG
        TraceMsg(TF_NAVIGATION, "Currently %s", ((bOffline == VARIANT_TRUE)
                                                    ? TEXT("OFFLINE")
                                                    : TEXT("ONLINE")));
#endif  // _DEBUG

        EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(bOffline)));

        m_pBindStatusCallback = new CSSNavigateBSC( SAFECAST(this, CScreenSaverWindow *),
                                                    TIME_NAVIGATE_TIMEOUT);

        if (NULL == m_pBindStatusCallback)
        {
            hrResult = E_OUTOFMEMORY;
            break;
        }

        // Display the Web Browser OC.
        ShowWindow(SW_SHOWNORMAL);
        m_pOleObject->DoVerb(   OLEIVERB_SHOW,
                                NULL,
                                SAFECAST(this, IOleClientSite *),
                                0,
                                m_hWnd,
                                &rect);

        break;
    }

    return SUCCEEDED(hrResult);
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetHTMLBodyElement
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::GetHTMLBodyElement
(
    IHTMLBodyElement ** ppHTMLBodyElement
)
{
    IDispatch *         pDispatch = NULL;
    IHTMLDocument2 *    pHTMLDocument = NULL;
    IHTMLElement *      pHTMLElement = NULL;
    HRESULT             hrResult;

    for (;;)
    {
        *ppHTMLBodyElement = NULL;

        if  (
            FAILED(m_pWebBrowser->get_Document(&pDispatch))
            ||
            (pDispatch == NULL)
            )
        {
            hrResult = E_FAIL;
            break;
        }

        if (FAILED(hrResult = pDispatch->QueryInterface( IID_IHTMLDocument2,
                                                         (void **)&pHTMLDocument)))
        {
            break;
        }

        if  (
            SUCCEEDED(hrResult = pHTMLDocument->get_body(&pHTMLElement))
            &&
            (pHTMLElement != NULL)
            )
        {
            hrResult = pHTMLElement->QueryInterface(IID_IHTMLBodyElement,
                                                    (void **)ppHTMLBodyElement);
        }

        break;
    }

    if (pHTMLElement != NULL)
        pHTMLElement->Release();

    if (pHTMLDocument != NULL)
        pHTMLDocument->Release();

    if (pDispatch != NULL)
        pDispatch->Release();

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::DisplayCurrentChannel
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::DisplayCurrentChannel
(
)
{
    HRESULT hrResult;

    if (m_lCurrChannel == CURRENT_CHANNEL_NONE)
        hrResult = DisplayNextChannel();
    else
    {
        if (FAILED(hrResult = DisplayChannel(m_lCurrChannel)))
        {
            TraceMsg(TF_NAVIGATION, "Unable to display channel, trying next one.");
            hrResult = DisplayNextChannel();
        }
    }

    // If everything blows up, show the default page.
    if (FAILED(hrResult))
    {
        TraceMsg(TF_NAVIGATION, "No channels are able to display. Using default.");
        hrResult = NavigateToDefault();
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::DisplayNextChannel
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::DisplayNextChannel
(
)
{
    long    lStartChannel = m_lCurrChannel;
    long    lNextChannel = m_lCurrChannel;
    HRESULT hrResult;

    for (;;)
    {
        lNextChannel = FindNextChannel(lNextChannel);

        // Bail if we have gone all the way around the list.
        if (lNextChannel == lStartChannel)
        {
            TraceMsg(TF_NAVIGATION, "No new channel available...Continuing current channel.");

            hrResult = E_FAIL;
            break;
        }

        if (lNextChannel == CURRENT_CHANNEL_NONE)
            continue;

        // Continue until we loop around or have navigated successfully.
        if (SUCCEEDED(hrResult = DisplayChannel(lNextChannel)))
            break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::DisplayChannel
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::DisplayChannel
(
    long lChannel
)
{
    LPITEMIDLIST pidlChannel = m_pPIDLList->Item(lChannel);

    ASSERT(pidlChannel != NULL);

    m_bMouseClicked = FALSE;

    HRESULT hrResult = NavigateToPIDL(pidlChannel);

    m_lCurrChannel = lChannel;

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::NavigateToPIDL
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::NavigateToPIDL
(
    LPITEMIDLIST pidl
)
{
    HRESULT hrResult = E_FAIL;

    for (;;)
    {
        CString strURL;
        TCHAR * pszURL = strURL.GetBuffer(INTERNET_MAX_URL_LENGTH);
        BOOL bResult = GetPIDLDisplayName(  NULL,
                                            pidl,
                                            pszURL,
                                            INTERNET_MAX_URL_LENGTH,
                                            SHGDN_FORPARSING);
        strURL.ReleaseBuffer();

        if (!bResult)
            break;

        BSTR bstrURL;
        if ((bstrURL = strURL.AllocSysString()) != NULL)
        {
            EVAL(SUCCEEDED(hrResult = NavigateToBSTR(bstrURL)));
            SysFreeString(bstrURL);
        }

        break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::NavigateToBSTR
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::NavigateToBSTR
(
    BSTR bstrURL
)
{

    IMoniker *  pURLMoniker = NULL;
    IBindCtx *  pBindCtx = NULL;
    IHlink *    pHlink = NULL;
    HRESULT     hrResult;

    for (;;)
    {
        if (bstrURL == NULL)
        {
            hrResult = E_INVALIDARG;
            break;
        }

        DWORD dwFeatureFlags;
        EVAL(SUCCEEDED(m_pSSConfig->get_Features(&dwFeatureFlags)));
        if (dwFeatureFlags & FEATURE_USE_CACHED_CONTENT_FIRST)
        {
            if (FAILED(hrResult = SetConnectedState(bstrURL)))
                break;
        }
        else
        {
            if (FAILED(hrResult = IsURLAvailable(bstrURL)))
                break;
        }

        if  (
            FAILED(hrResult = CreateURLMoniker(NULL, bstrURL, &pURLMoniker))
            ||
            FAILED(hrResult = CreateBindCtx(0, &pBindCtx))
            ||
            FAILED(hrResult = HlinkCreateFromMoniker(   pURLMoniker,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        0,
                                                        NULL,
                                                        IID_IHlink,
                                                        (void **)&pHlink))
            ||
            FAILED(hrResult = m_pHlinkFrame->Navigate(  HLNF_CREATENOHISTORY,
                                                        pBindCtx,
                                                        SAFECAST(m_pBindStatusCallback, IBindStatusCallback *),
                                                        pHlink))
            )
        {
            break;
        }

#ifdef _DEBUG
        VARIANT_BOOL bOffline;
        EVAL(SUCCEEDED(m_pWebBrowser->get_Offline(&bOffline)));

        CString strURL = bstrURL;

        TraceMsg(TF_NAVIGATION, "%s: Started navigation to '%s'", (bOffline ? TEXT("OFFLINE") : TEXT("ONLINE")), (LPCTSTR)strURL);
#endif  // _DEBUG

        break;
    }

    // Cleanup
    if (pHlink != NULL)
        pHlink->Release();

    if (pBindCtx != NULL)
        pBindCtx->Release();

    if (pURLMoniker != NULL)
        pURLMoniker->Release();

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::NavigateToDefault
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::NavigateToDefault
(
)
{
    HRESULT hrResult = E_FAIL;

    for (;;)
    {
        BSTR bstrCurrentURL;

        if  (
            FAILED(m_pScreenSaver->get_CurrentURL(&bstrCurrentURL))
            ||
            (bstrCurrentURL == NULL)
            )
        {
            break;
        }

        CString strDefaultURL;
        TCHAR * pszURL = strDefaultURL.GetBuffer(INTERNET_MAX_URL_LENGTH);
        EVAL(GetPIDLDisplayName(NULL, m_pidlDefault, pszURL, INTERNET_MAX_URL_LENGTH, SHGDN_FORPARSING));
        strDefaultURL.ReleaseBuffer();

        CString strCurrentURL = bstrCurrentURL;

        if (StrCmpI(strCurrentURL, strDefaultURL) == 0)
        {
            ResetChangeTimer();
            hrResult = S_OK;
        }

        SysFreeString(bstrCurrentURL);

        break;
    }

    if (FAILED(hrResult))
        hrResult = NavigateToPIDL(m_pidlDefault);

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::SetConnectedState
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::SetConnectedState
(
    BSTR bstrURL
)
{
    if (bstrURL == NULL)
        return E_INVALIDARG;

    CString strURL = bstrURL;
    if ((LPTSTR)strURL == NULL)
        return E_OUTOFMEMORY;

    // Check for 'res://' URL
    BOOL bResURL = (StrCmpNI(strURL, s_szRES, lstrlen(s_szRES)) == 0);

    long lMode;
    EVAL(SUCCEEDED(m_pScreenSaver->get_Mode(&lMode)));

    HRESULT hrResult;

    // Check the cache for the URL first. Assume /u URLs want to hit the net.
    if  (
        (lMode != SSMODE_SINGLEURL)
        &&
        // Ignore cache check for 'res://' URLs
        !bResURL
        &&
        GetUrlCacheEntryInfoEx(strURL, NULL, NULL, NULL, NULL, NULL, 0)
        )
    {
        // Go offline and use cache content.
        TraceMsg(TF_NAVIGATION, "Going offline");
        EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(VARIANT_TRUE)));
        hrResult = S_OK;
    }
    else
    {
        DWORD dwConnectState;

        // Try the internet if we have a direct connection. Ignore dialup.
        if  (
            InternetGetConnectedState(&dwConnectState, 0)
            &&
            (
                (dwConnectState & INTERNET_CONNECTION_LAN)
                ||
                (dwConnectState & INTERNET_CONNECTION_PROXY)
            )
            )
        {
            if (!bResURL)
            {
                TraceMsg(   TF_NAVIGATION,
                            "Content for '%s' not in cache! Going online",
                            (LPCTSTR)strURL);

                EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(VARIANT_FALSE)));
            }
            else
            {
                TraceMsg(TF_NAVIGATION, "Going offline for res:// URL");

                // Go offline for res:// URLs they should be local.
                EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(VARIANT_TRUE)));
            }

            hrResult = S_OK;
        }
        else
            hrResult = E_FAIL;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::IsURLAvailable
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::IsURLAvailable
(
    BSTR bstrURL
)
{
    if (bstrURL == NULL)
        return E_INVALIDARG;

    CString strURL = bstrURL;
    if ((LPTSTR)strURL == NULL)
        return E_OUTOFMEMORY;

    HRESULT hrResult = S_OK;

    for (;;)
    {
        // Ignore cache check for 'res://' URLs
        if (StrCmpNI(strURL, s_szRES, lstrlen(s_szRES)) == 0)
            break;
        
        long lMode;
        EVAL(SUCCEEDED(m_pScreenSaver->get_Mode(&lMode)));

        DWORD dwConnectState;
              
        if  (
            // Assume /u URLs want to hit the net.
            (lMode == SSMODE_SINGLEURL)
            ||
            // If we are online it should be available, too.
            InternetGetConnectedState(&dwConnectState, 0)
            )
        {
            TraceMsg(TF_NAVIGATION, "Going ONLINE for internet content");

            EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(VARIANT_FALSE)));
            break;
        }

        // Check the cache for the URL if it is there, go offline.
        if (GetUrlCacheEntryInfoEx(strURL, NULL, NULL, NULL, NULL, NULL, 0))
        {
            TraceMsg(TF_NAVIGATION, "Going OFFLINE for cached content");

            EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(VARIANT_TRUE)));
            break;
        }

        hrResult = E_FAIL;
        break;
    }

    return hrResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::ResetChangeTimer
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::ResetChangeTimer
(
    BOOL bExtendedTime
)
{
    if (m_idChangeTimer != 0)
    {
        EVAL(KillTimer(m_idChangeTimer));
        m_idChangeTimer = 0;
    }

    int iChannelTime;
    EVAL(SUCCEEDED(m_pSSConfig->get_ChannelTime(&iChannelTime)));

    EVAL((m_idChangeTimer = SetTimer(   ID_CHANNELCHANGE_TIMER,
                                        (iChannelTime * 1000)
                                            + (bExtendedTime ? TIME_NAVIGATE_TIMEOUT : 0),
                                        NULL)) != 0);
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::FindNextChannel
/////////////////////////////////////////////////////////////////////////////
long CScreenSaverWindow::FindNextChannel
(
    long lChannelStart
)
{
    if (m_pPIDLList->Count() > 0)
    {
        if (lChannelStart == CURRENT_CHANNEL_NONE)
            lChannelStart = 0;
        else
        {
            lChannelStart++;
            if (lChannelStart >= m_pPIDLList->Count())
                lChannelStart = CURRENT_CHANNEL_NONE;
        }
    }

    return lChannelStart;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::LaunchBrowser
/////////////////////////////////////////////////////////////////////////////
BOOL CScreenSaverWindow::LaunchBrowser
(
    CString & strURL
)
{
    BOOL bResult = FALSE;

    for (;;)
    {
        PARSEDURL pu = { 0 };
        pu.cbSize = sizeof(PARSEDURL);
        if (FAILED(ParseURL(strURL, &pu)))
            break;

        if  (
            (pu.nScheme == URL_SCHEME_INVALID)
            ||
            (pu.nScheme == URL_SCHEME_UNKNOWN)
            ||
            (pu.nScheme == URL_SCHEME_VBSCRIPT)
            ||
            (pu.nScheme == URL_SCHEME_JAVASCRIPT)
            )
        {
            // Don't allow navigation to scripts.
            // They may need page context.
            TraceMsg(TF_NAVIGATION, "Ignoring %s -- Can't launch", (LPCTSTR)strURL);
            break;
        }

        //
        // If we are running on NT and password protection is enabled then
        // we disable launching browsers. This is because there is no way
        // to detect on the other side that the password verification succeeded
        //
        if (g_bPlatformNT && g_bPasswordEnabled)
        {
            bResult = TRUE;
            break;
        }

        if (pu.nScheme == URL_SCHEME_FILE)
        {
            // Make sure that URL is completely escaped.
            char * szURL = strURL.GetBuffer(INTERNET_MAX_URL_LENGTH);
            DWORD cchSize = INTERNET_MAX_URL_LENGTH;
            UrlEscape(szURL, szURL, &cchSize, 0);
            strURL.ReleaseBuffer();
        } 

        if (g_bPlatformNT)
        {
            // Write URL to registry and trigger event.
            WriteRegValue(  HKEY_CURRENT_USER,
                            g_szRegSubKey,
                            s_szRegLastNavURL,
                            REG_SZ,
                            (LPBYTE)(char *)strURL,
                            strURL.GetLength());

            HANDLE hTermEvent;
            if ((hTermEvent = OpenEvent(EVENT_MODIFY_STATE,
                                        FALSE,
                                        s_szSETermEvent)) != NULL)
            {
                bResult = SetEvent(hTermEvent);
                CloseHandle(hTermEvent);
            }
        }
        else
        {
            BSTR bstrURL = strURL.AllocSysString();
            if (bstrURL != NULL)
            {
                bResult = SUCCEEDED(HlinkNavigateString(NULL, bstrURL));
                SysFreeString(bstrURL);
            }
        }

        break;
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::UserIsInteractive
/////////////////////////////////////////////////////////////////////////////
BOOL CScreenSaverWindow::UserIsInteractive
(
)
{
    if (g_bPlatformNT)
        return TRUE;

    // REVIEW: This is a big hack. We need to
    // find a better way to do this [jaym]
    HWND hwndCurr = ::GetWindow(m_hWnd, GW_HWNDFIRST);
    while (hwndCurr != NULL)
    {
        if (::GetWindowTextLength(hwndCurr) > 0)
        {
            TCHAR szWindowText[40];
            ::GetWindowText(hwndCurr, szWindowText, ARRAYSIZE(szWindowText));

            TraceMsg(TF_ALWAYS, "Window '%s'", szWindowText);

            if (StrCmpI(szWindowText, TEXT("DDE Server Window")) == 0)
                return TRUE;
        }

        hwndCurr = ::GetWindow(hwndCurr, GW_HWNDNEXT);
    }

    TraceMsg(TF_ALWAYS, "USER IS NOT INTERACTIVE!");

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnChannelChangeTimer
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::OnChannelChangeTimer
(
)
{
    ASSERT(m_idChangeTimer != 0);

    EVAL(KillTimer(m_idChangeTimer));
    m_idChangeTimer = 0;

    if (FAILED(DisplayNextChannel()))
        ResetChangeTimer();
    else
    {
        // Just in case we don't get OnDocumentComplete()...
        ResetChangeTimer(TRUE);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::VerifyPassword
/////////////////////////////////////////////////////////////////////////////
BOOL CScreenSaverWindow::VerifyPassword
(
)
{
    BOOL fOK = FALSE;

    //  Check to see if we are already in here.  If m_lVerifyingPassword
    //  is zero on entry then it will be set to 1 and the return from
    //  InterlockedExchange will be 0.  When we are done we call
    //  InterlockedExchange to reset the value to 0.
    if (InterlockedExchange(&m_lVerifingPassword, TRUE) == FALSE)
    {
        EnableModeless(FALSE);
        if (g_bPlatformNT)
        {
            // REVIEW: Verify password for NT users. Or, alternatively
            // figure out how to lock the workstation before the screensaver
            // activates.

            // LogonUser

            fOK = TRUE;
        }
        else
        {
            // Also disable the browser window
            HWND hBrowserWnd = NULL;
            ASSERT(m_pWebBrowser);
            if(m_pWebBrowser)
            {
                IOleWindow *pOleWindow = NULL;
                if(SUCCEEDED(m_pWebBrowser->QueryInterface(IID_IOleWindow, (void**)&pOleWindow)))
                {
                    ASSERT(pOleWindow);
                    pOleWindow->GetWindow(&hBrowserWnd);
                    pOleWindow->Release();
                }

            }
            if (m_pfnVerifyPassword != NULL)
                fOK = m_pfnVerifyPassword(hBrowserWnd ? hBrowserWnd : m_hWnd);
            else
            {
                // Passwords disabled or unable to load
                // handler DLL always allow exit.
                fOK = TRUE;
            }
        }

        EnableModeless(TRUE);
        InterlockedExchange(&m_lVerifingPassword, FALSE);
    }

    return fOK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::InitChannelList
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::InitChannelList
(
)
{
    if (m_pPIDLList == NULL)
        m_pPIDLList = new CPIDLList;

    if (m_pPIDLList == NULL)
        return;

    if (UserIsInteractive())
    {
        long lMode;

        EVAL(SUCCEEDED(m_pScreenSaver->get_Mode(&lMode)));
        if (lMode == SSMODE_SINGLEURL)
        {
            BSTR bstrURL;

            if  (
                SUCCEEDED(m_pScreenSaver->get_CurrentURL(&bstrURL))
                &&
                (bstrURL != NULL)
                &&
                (SysStringLen(bstrURL) != 0)
                )
            {
                CString strURL = bstrURL;
                m_pPIDLList->Add(strURL);
            }

            SysFreeString(bstrURL);
        }
        else
        {
            DWORD dwFeatureFlags;
            EVAL(SUCCEEDED(m_pSSConfig->get_Features(&dwFeatureFlags)));
            m_pPIDLList->Build(m_pSSConfig, (dwFeatureFlags & FEATURE_USE_CDF_TOPLEVEL_URL));
        }
    }
    else
        TraceMsg(TF_WARNING, "USER IS NOT LOGGED ON!");

    // If there are no current Screen Saver channels, show the
    // info. HTML message about setting up Screen Saver channels.
    if (m_pPIDLList->Count() == 0)
    {
        TraceMsg(TF_NAVIGATION, "No URLs in list -- adding default HTML");
        EVAL(m_pPIDLList->Add(CopyPIDL(m_pidlDefault)));
    }
    else
    {
        ASSERT(m_pPIDLList->Count() > 0);

        LoadState();
    }

    TraceMsg(TF_NAVIGATION, "%d URLs in list", m_pPIDLList->Count());
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::LoadState
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::LoadState
(
)
{
    CString strLastURL;

    ASSERT(m_pPIDLList != NULL);

    if (m_pPIDLList->Count() > 0)
    {
        char * pszLastURL = strLastURL.GetBuffer(INTERNET_MAX_URL_LENGTH);
        ReadRegString(  HKEY_CURRENT_USER,
                        g_szRegSubKey,
                        s_szRegLastURL,
                        TEXT(""),
                        pszLastURL,
                        INTERNET_MAX_URL_LENGTH);
        strLastURL.ReleaseBuffer();

        if (strLastURL.GetLength() > 0)
        {
            LPITEMIDLIST pidl;

            if (CreatePIDLFromPath(strLastURL, &pidl) && pidl)
            {
                m_lCurrChannel = m_pPIDLList->Find(pidl);
                g_pMalloc->Free(pidl);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::SaveState
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::SaveState
(
)
{
    CString strLastURL;

    // Save the current URL being displayed.    
    if  (
        (m_pPIDLList != NULL)
        &&
        (m_lCurrChannel != CURRENT_CHANNEL_NONE)
        )
    {
        LPITEMIDLIST pidl = m_pPIDLList->Item(m_lCurrChannel);

        char * pszLastURL = strLastURL.GetBuffer(INTERNET_MAX_URL_LENGTH);
        EVAL(GetPIDLDisplayName(NULL,
                                pidl,
                                pszLastURL,
                                INTERNET_MAX_URL_LENGTH,
                                SHGDN_FORPARSING));
        strLastURL.ReleaseBuffer();
    }

    WriteRegValue(  HKEY_CURRENT_USER,
                    g_szRegSubKey,
                    s_szRegLastURL,
                    REG_SZ,
                    (LPBYTE)(char *)strLastURL,
                    strLastURL.GetLength());
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::LoadPasswordDLL
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::LoadPasswordDLL
(
)
{
    if (m_hPasswordDLL != NULL)
        UnloadPasswordDLL();

    // Look to see if password turned on, otherwise
    // don't bother to load password handler DLL.
    if (g_bPasswordEnabled)
    {
        // Try to load the DLL that contains password proc.
        if ((m_hPasswordDLL = LoadLibrary(s_szPasswordDLL)) != NULL)
        {
            m_pfnVerifyPassword =
                (VERIFYPWDPROC)GetProcAddress(  m_hPasswordDLL,
                                                s_szPasswordFnName);

            if (m_pfnVerifyPassword == NULL)
                UnloadPasswordDLL();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::UnloadPasswordDLL
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::UnloadPasswordDLL
(
)
{
    if (m_hPasswordDLL != NULL)
    {
        FreeLibrary(m_hPasswordDLL);

        m_hPasswordDLL = NULL;
        m_pfnVerifyPassword = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::SetHooks
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::SetHooks
(
)
{
    ASSERT(s_hKeyboardHook == NULL);
    ASSERT(s_hMouseHook == NULL);

    s_hKeyboardHook = SetWindowsHookEx( WH_KEYBOARD,
                                        KeyboardHookProc,
                                        _pModule->GetModuleInstance(),
                                        GetCurrentThreadId());

    s_hMouseHook = SetWindowsHookEx(    WH_MOUSE,
                                        MouseHookProc,
                                        _pModule->GetModuleInstance(),
                                        GetCurrentThreadId());

    ASSERT(s_hKeyboardHook != NULL);
    ASSERT(s_hMouseHook != NULL);
}

/////////////////////////////////////////////////////////////////////////////
// KeyboardHookProc
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK KeyboardHookProc
(
    int     nCode,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    if  (
        (nCode != HC_NOREMOVE)
        &&
        (s_pThis->ModelessEnabled())
        )
    {
        // Make sure the user is keyboarding in the Screen Saver.
        if  (s_pThis->m_hWnd != GetForegroundWindow())
        {
            // Ignore
            ;
        }
        else
        {
            switch (wParam)
            {
                // Ignore if the user is pressing ALT
                case VK_MENU:

                // Ignore IMM messages (just in case!)
                case VK_PROCESSKEY:
                    break;

                // Check for scrolling
                case VK_PRIOR:
                case VK_NEXT:
                case VK_END:
                case VK_HOME:
                case VK_LEFT:
                case VK_UP:
                case VK_RIGHT:
                case VK_DOWN:
                {
                    static BOOL bUp = TRUE;

                    if (bUp)
                        s_pThis->ShowControls(TRUE);

                    bUp = !bUp;
                    break;
                }

                // Hotkey to go to next channel.
                case VK_ADD:
                {
                    s_pThis->OnChannelChangeTimer();
                    break;
                }

                default:
                {
                    // Ignore F-keys for easier debugging.
                    if ((wParam >= VK_F1) && (wParam <= VK_F12))
                    {
                        DWORD dwFeatureFlags;
                        s_pThis->GetFeatures(&dwFeatureFlags);

                        if (dwFeatureFlags & FEATURE_IGNORE_FKEYS)
                            break;
                    }

                    s_pThis->Quit();
                    break;
                }
            }
        }
    }

    return CallNextHookEx(s_hKeyboardHook, nCode, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// MouseHookProc
/////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK MouseHookProc
(
    int     nCode,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    if  (
        (nCode != HC_NOREMOVE)
        &&
        (s_pThis->ModelessEnabled())
        &&
        // Make sure the user is mousing in the Screen Saver window.
        (s_pThis->m_hWnd == GetForegroundWindow())
    )
    {
        switch (wParam)
        {
            case WM_NCRBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_NCMBUTTONDOWN:
            case WM_MBUTTONDOWN:
            {
                DWORD dwFeatureFlags;
                s_pThis->GetFeatures(&dwFeatureFlags);

                if (!(dwFeatureFlags & FEATURE_IGNORE_MOUSE_CLICKS))
                    s_pThis->Quit();

                break;
            }

            case WM_MOUSEMOVE:
            {
                static  BOOL    bMouseMoved = FALSE;
                static  POINT   ptLast;

                TraceMsg(TF_ALWAYS, "MouseHookProc(WM_MOUSEMOVE)");

                // Check to see if the mouse pointer really moved.
                if (!bMouseMoved)
                {
                    GetCursorPos(&ptLast);
                    bMouseMoved = TRUE;
                }
                else
                {
                    POINT ptCursor;
                    POINT ptCheck;

                    GetCursorPos(&ptCheck);

                    if (ptCursor.x = ptCheck.x - ptLast.x)
                    {
                        if (ptCursor.x < 0)
                            ptCursor.x *= -1;
                    }

                    if (ptCursor.y = ptCheck.y - ptLast.y)
                    {
                        if (ptCursor.y < 0)
                            ptCursor.y *= -1;
                    }

                    if ((ptCursor.x + ptCursor.y) > MOUSEMOVE_THRESHOLD)
                    {
                        // Make sure the user is pressing ALT
                        // if we are in ALT+mouse mode.
                        if  (
                            s_pThis->ALTMouseMode()
                            &&
                            !(GetAsyncKeyState(VK_MENU) & 0x8000)
                            )
                        {
                            s_pThis->Quit();
                        }
                        else
                        {
                            // Display the scrollbar while the
                            // user is moving the mouse pointer
                            s_pThis->ShowControls(TRUE);
                        }

                        ptLast = ptCheck;
                    }
                }

                break;
            }

            default:
                break;
        }
    }

    return CallNextHookEx(s_hMouseHook, nCode, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::ReleaseHooks
/////////////////////////////////////////////////////////////////////////////
void CScreenSaverWindow::ReleaseHooks
(
)
{
    if (s_hKeyboardHook != NULL)
    {
        EVAL(UnhookWindowsHookEx(s_hKeyboardHook));
        s_hKeyboardHook = NULL;
    }

    if (s_hMouseHook != NULL)
    {
        EVAL(UnhookWindowsHookEx(s_hMouseHook));
        s_hMouseHook = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////////
// Interfaces
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// IUnknown interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::AddRef
/////////////////////////////////////////////////////////////////////////////
ULONG CScreenSaverWindow::AddRef
(
) 
{
    m_cRef++;

    TraceMsg(TF_DEBUGREFCOUNT, "CScreenSaverWindow::AddRef,  m_cRef = %d", m_cRef);

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::Release
/////////////////////////////////////////////////////////////////////////////
ULONG CScreenSaverWindow::Release
(
)
{
    m_cRef--;

    TraceMsg(TF_DEBUGREFCOUNT, "CScreenSaverWindow::Release, m_cRef = %d", m_cRef);

    if (m_cRef < 0)
    {
        ASSERT(FALSE);
        return 0;
    }

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::QueryInterface
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::QueryInterface
(
    REFIID      riid,
    LPVOID *    ppvObj
)
{
    IUnknown * pUnk;

    *ppvObj = NULL;

    if (InlineIsEqualGUID(riid, IID_IUnknown))
        pUnk = SAFECAST(this, IDispatch *);
#if 0
    else if (InlineIsEqualGUID(riid, IID_IServiceProvider))
        pUnk = SAFECAST(this, IServiceProvider *);
#endif
    else if (InlineIsEqualGUID(riid, IID_IOleClientSite))
        pUnk = SAFECAST(this, IOleClientSite *);
    else if (InlineIsEqualGUID(riid, IID_IOleInPlaceSite))
        pUnk = SAFECAST(this, IOleInPlaceSite *);
    else if (InlineIsEqualGUID(riid, IID_IOleInPlaceFrame))
        pUnk = SAFECAST(this, IOleInPlaceFrame *);
    else if (InlineIsEqualGUID(riid, IID_IOleContainer))
        pUnk = SAFECAST(this , IOleContainer *);
    else if (InlineIsEqualGUID(riid, IID_IOleCommandTarget))
        pUnk = SAFECAST(this, IOleCommandTarget *);
    else if (InlineIsEqualGUID(riid, IID_IBindStatusCallback))
        pUnk = SAFECAST(m_pBindStatusCallback, IBindStatusCallback *);
    else if (
            InlineIsEqualGUID(riid, IID_IDispatch)
            ||
            InlineIsEqualGUID(riid, DIID_DWebBrowserEvents)
            ||
            InlineIsEqualGUID(riid, DIID_HTMLDocumentEvents)
            )
    {
        pUnk = SAFECAST(this, IDispatch *);
    }
    else
    {
#ifdef _DEBUG
        TCHAR szName[128];
        TraceMsg(   TF_DEBUGQI,
                    "CScreenSaverWindow::QueryInterface(%s) -- FAILED",
                    DebugIIDName(riid, szName));
#endif  // _DEBUG

        return E_NOINTERFACE;
    }

    pUnk->AddRef();
    *ppvObj = (void *)pUnk;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IServiceProvider interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::QueryService
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::QueryService
(
    REFGUID guidService,
    REFIID  riid,
    void ** ppvObj
)
{
    IUnknown *  pUnk;
    HRESULT     hrResult = S_OK;

    *ppvObj = NULL;

    if (InlineIsEqualGUID(guidService, SID_STopLevelBrowser))
    {
        if (InlineIsEqualGUID(riid, IID_IServiceProvider))
            pUnk = SAFECAST(this, IServiceProvider *);
        else if (InlineIsEqualGUID(riid, IID_IOleCommandTarget))
            pUnk = SAFECAST(this, IOleCommandTarget *);
        else
            hrResult = E_NOINTERFACE;
    }
    else if (InlineIsEqualGUID(guidService, IID_IHlinkFrame))
    {
        if (InlineIsEqualGUID(riid, IID_IBindStatusCallback))
            pUnk = SAFECAST(m_pBindStatusCallback, IBindStatusCallback *);
    }
    else
        hrResult = E_NOINTERFACE;

    if (SUCCEEDED(hrResult))
    {
        pUnk->AddRef();
        *ppvObj = (void *)pUnk;
    }
#ifdef _DEBUG
    else
    {
        TCHAR szServiceName[128];
        DebugCLSIDName(guidService, szServiceName);

        TCHAR szRIIDName[128];
        DebugCLSIDName(riid, szRIIDName);

        TraceMsg(   TF_DEBUGQS,
                    "CScreenSaverWindow::QueryService(%s, %s) - FAILED",
                    szServiceName,
                    szRIIDName);
    }
#endif  // _DEBUG

    return hrResult;
}


/////////////////////////////////////////////////////////////////////////////
// IOleClientSite interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::SaveObject
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::SaveObject
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::SaveObject()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetMoniker
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::GetMoniker
(
    DWORD       dwAssign,
    DWORD       dwWhichMoniker,
    IMoniker ** ppmk
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::GetMoniker()");
    return E_INVALIDARG;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetContainer
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::GetContainer
(
    IOleContainer ** ppContainer
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::GetContainer()");

    *ppContainer = SAFECAST(this, IOleContainer *);
    (*ppContainer)->AddRef();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::ShowObject
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::ShowObject
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::ShowObject()");
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnShowWindow
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::OnShowWindow
(
    BOOL fShow
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::OnShowWindow()");
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::RequestNewObjectLayout
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::RequestNewObjectLayout
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::RequestNewObjectLayout()");
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// IOleWindow interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetWindow
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::GetWindow
(
    HWND * phWnd
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::GetWindow()");

    ASSERT(m_hWnd != NULL);

    *phWnd = m_hWnd;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::ContextSensitiveHelp
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::ContextSensitiveHelp
(
    BOOL fEnterMode
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::ContextSensitiveHelp()");
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// IOleInPlaceSite
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::CanInPlaceActivate
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::CanInPlaceActivate
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::CanInPlaceActivate()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnInPlaceActivate
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::OnInPlaceActivate
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::OnInPlaceActivate()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnUIActivate
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::OnUIActivate
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::OnUIActivate()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetWindowContext
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::GetWindowContext
(
    IOleInPlaceFrame **     ppFrame,
    IOleInPlaceUIWindow **  ppDoc,
    LPRECT                  lprcPosRect,
    LPRECT                  lprcClipRect,
    LPOLEINPLACEFRAMEINFO   lpFrameInfo
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::GetWindowContext()");

    *ppFrame = (IOleInPlaceFrame *)this;
    (*ppFrame)->AddRef();

    // NULL means same ptr as ppFrame.
    *ppDoc = NULL;

    GetClientRect(lprcClipRect);
    *lprcPosRect = *lprcClipRect;

    lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = m_hWnd;
    lpFrameInfo->haccel = 0;
    lpFrameInfo->cAccelEntries = 0;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::Scroll
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::Scroll
(
    SIZE scrollExtent
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::Scroll()");
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnUIDeactivate
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::OnUIDeactivate
(
    BOOL fUndoable
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::OnUIDeactivate()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnInPlaceDeactivate
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::OnInPlaceDeactivate
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::OnInPlaceDeactivate()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::DiscardUndoState
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::DiscardUndoState
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::DiscardUndoState()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::DeactivateAndUndo
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::DeactivateAndUndo
(
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::DeactivateAndUndo()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnPosRectChange
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::OnPosRectChange
(
    LPCRECT lprcPosRect
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::OnPosRectChange()");
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// IOleInPlaceUIWindow interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetBorder
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::GetBorder
(
    LPRECT lprectBorder
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::GetBorder()");

    GetClientRect(lprectBorder);
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::RequestBorderSpace
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::RequestBorderSpace
(
    LPCBORDERWIDTHS pbw
)
{
    RECT    rcClient;
    HRESULT hres = S_OK;

    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::RequestBorderSpace()");

    GetClientRect(&rcClient);
    if (pbw->left + pbw->right > rcClient.right/2)
        hres = INPLACE_E_NOTOOLSPACE;
    else if (pbw->top + pbw->bottom > rcClient.bottom/2)
        hres = INPLACE_E_NOTOOLSPACE;

    return hres;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::SetBorderSpace
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::SetBorderSpace
(
    LPCBORDERWIDTHS pbw
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::SetBorderSpace()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::SetActiveObject
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::SetActiveObject
(
    IOleInPlaceActiveObject *   pActiveObject,
    LPCOLESTR                   pszObjName
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::SetActiveObject()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IOleInPlaceFrame interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::InsertMenus
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::InsertMenus
(
    HMENU                   hmenuShared,
    LPOLEMENUGROUPWIDTHS    lpMenuWidths
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::InsertMenus()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::SetMenu
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::SetMenu
(
    HMENU       hmenuShared,
    HOLEMENU    holemenu,
    HWND        hwndActiveObject
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::SetMenu()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::RemoveMenus
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::RemoveMenus
(
    HMENU hmenuShared
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::RemoveMenus()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::SetStatusText
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::SetStatusText
(
    LPCOLESTR pszStatusText
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::SetStatusText()");
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::EnableModeless
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::EnableModeless
(
    BOOL fEnable
)
{
#ifdef _DEBUG
    TraceMsg(   TF_FUNCENTRY,
                "CScreenSaverWindow::EnableModeless(%s)\r\n",
                (fEnable ? TEXT("TRUE") : TEXT("FALSE")));
#endif  // _DEBUG

    m_bModelessEnabled = fEnable;
    EnableWindow(m_bModelessEnabled);

    if  (
        (m_pToolbar != NULL)
        &&
        IsWindow(m_pToolbar->m_hWnd)
        )
    {
        m_pToolbar->EnableWindow(m_bModelessEnabled);
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::TranslateAccelerator
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::TranslateAccelerator
(
    LPMSG   lpmsg,
    WORD    wID
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::TranslateAccelerator()");
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IOleContainer interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::ParseDisplayName
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::ParseDisplayName
(
    IBindCtx *  pbc,
    LPOLESTR    pszDisplayName,
    ULONG *     pchEaten,
    IMoniker ** ppmkOut
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::ParseDisplayName()");
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::EnumObjects
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::EnumObjects
(
    DWORD           grfFlags,
    IEnumUnknown ** ppenum
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::EnumObjects()");
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::LockContainer
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::LockContainer
(
    BOOL fLock
)
{
    TraceMsg(TF_FUNCENTRY, "CScreenSaverWindow::LockContianer(%d)", fLock);
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// IOleCommandTarget interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::QueryStatus
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::QueryStatus
(
    const GUID *    pguidCmdGroup,
    ULONG           cCmds,
    OLECMD          prgCmds[],
    OLECMDTEXT *    pCmdText
)
{
    if (prgCmds == NULL)
        return E_POINTER;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::Exec
/////////////////////////////////////////////////////////////////////////////
HRESULT CScreenSaverWindow::Exec
(
    const GUID *    pguidCmdGroup,
    DWORD           nCmdID,
    DWORD           nCmdExecOpt,
    VARIANTARG *    pvaIn,
    VARIANTARG *    pvaOut
)
{
    if (pguidCmdGroup == NULL)
        return OLECMDERR_E_NOTSUPPORTED;

    if (!InlineIsEqualGUID(*pguidCmdGroup, CGID_ShellDocView))
        return OLECMDERR_E_UNKNOWNGROUP;

    HRESULT hrResult;

    switch (nCmdID)
    {
        case SHDVID_GETTRANSITION:
        {
            // Make sure the feature is enabled
            DWORD dwFeatureFlags;
            EVAL(SUCCEEDED(m_pSSConfig->get_Features(&dwFeatureFlags)));
            if (!(dwFeatureFlags & FEATURE_DEFAULT_TRANSITIONS))
            {
                hrResult = OLECMDERR_E_NOTSUPPORTED;
                break;
            }

            switch (nCmdExecOpt)
            {
                case OLECMDEXECOPT_DODEFAULT:
                {
                    // Validate In/Out args.
                    if ((pvaIn == NULL) || (pvaOut == NULL))
                        return E_UNEXPECTED;

                    V_VT(&pvaOut[0]) = VT_I4;
                    V_I4(&pvaOut[0]) = 0;

                    V_VT(&pvaOut[1]) = VT_BSTR;
                    V_BSTR(&pvaOut[1]) = SysAllocString(s_szDefTransition);

                    if (NULL == V_BSTR(&pvaOut[1]))
                        hrResult = E_OUTOFMEMORY;
                    else
                        hrResult = S_OK;
                    break;
                }

                case OLECMDEXECOPT_SHOWHELP:
                {
                    hrResult = OLECMDERR_E_NOHELP;
                    break;
                }

                default:
                {
                    hrResult = OLECMDERR_E_NOTSUPPORTED;
                    break;
                }
            }

            break;
        }

        default:
        {
            hrResult = OLECMDERR_E_NOTSUPPORTED;
            break;
        }
    }

    return hrResult;
}


/////////////////////////////////////////////////////////////////////////////
// IDispatch interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetTypeInfoCount
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::GetTypeInfoCount
(
    UINT * pctInfo
)
{
    *pctInfo = NULL;

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetTypeInfo
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::GetTypeInfo
(
    UINT            itinfo,
    LCID            lcid,
    ITypeInfo **    pptInfo
)
{
    *pptInfo = NULL;

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::GetIDsOfNames
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::GetIDsOfNames
(
    REFIID      riid,
    OLECHAR **  rgszNames,
    UINT        cNames,
    LCID        lcid,
    DISPID *    rgDispID
)
{
    *rgszNames = NULL;
    *rgDispID = NULL;

    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::Invoke
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CScreenSaverWindow::Invoke
(
    DISPID          dispIDMember,
    REFIID          riid,
    LCID            lcid,
    unsigned short  wFlags,
    DISPPARAMS *    pDispParams,
    VARIANT *       pVarResult,
    EXCEPINFO *     pExcepInfo,
    UINT *          puArgErr)
{
    VARIANT varResult;
    HRESULT hrResult = S_OK;

    for (;;)
    {
        if (!InlineIsEqualGUID(riid, IID_NULL))
        {
            hrResult = E_INVALIDARG;
            break;
        }

        // We don't handle the return value of any events if
        // events have them.  We should, however, initialize an
        // empty return value just so it's not garbage.
        if (pVarResult == NULL)
            pVarResult = &varResult;

        VariantInit(pVarResult);
        V_VT(pVarResult) = VT_EMPTY;

        // Only method and get calls are valid.
        if (wFlags & ~(DISPATCH_METHOD | DISPATCH_PROPERTYGET))
        {
            hrResult = DISP_E_MEMBERNOTFOUND;
            break;
        }

        // Process the event by looking for dispIDMember in the
        // list maintained in the tenant that maps event IDs to
        // actions.  If we find the ID, then we execute the action,
        // otherwise we do nothing.
        switch (dispIDMember)
        {
            case DISPID_AMBIENT_DLCONTROL:
            {
                TraceMsg(TF_NAVIGATION, "NAV: Returning DLCTL ambient");
                
                VARIANT_BOOL bPlaySounds;
                EVAL(SUCCEEDED(m_pSSConfig->get_PlaySounds(&bPlaySounds)));

                VARIANT_BOOL bOffline;
                EVAL(SUCCEEDED(m_pWebBrowser->get_Offline(&bOffline)));

                V_VT(pVarResult) = VT_I4;
                V_I4(pVarResult) = DLCTL_SILENT
                                    | ((bOffline == VARIANT_TRUE) ? DLCTL_OFFLINE : 0)
                                    | DLCTL_DLIMAGES
                                    | DLCTL_VIDEOS
                                    | ((bPlaySounds == VARIANT_TRUE) ? DLCTL_BGSOUNDS : 0);
                break;                                        
            }
                
            case DISPID_BEFORENAVIGATE:
            case DISPID_FRAMEBEFORENAVIGATE:
            {
                TraceMsg(TF_NAVIGATION, "NAV: OnBeforeNavigate(Frame = %d)", (dispIDMember == DISPID_FRAMEBEFORENAVIGATE));

                m_bScrollbarVisible = TRUE;

                hrResult = OnBeforeNavigate(pDispParams, pVarResult);
                break;
            }

            case DISPID_NEWWINDOW:
            case DISPID_FRAMENEWWINDOW:
            {
                TraceMsg(TF_NAVIGATION, "NAV: OnNewWindow(Frame = %d)", (dispIDMember == DISPID_FRAMEBEFORENAVIGATE));
                m_bScrollbarVisible = TRUE;
                hrResult = OnNewWindow(pDispParams, pVarResult);  
                break;
            }

            case DISPID_DOCUMENTCOMPLETE:
            {
                TraceMsg(TF_NAVIGATION, "NAV: DocumentComplete()");

                if (pDispParams == NULL)
                {
                    hrResult = DISP_E_PARAMNOTOPTIONAL;
                    break;
                }

                ASSERT(V_VT(&pDispParams->rgvarg[0]) == (VT_VARIANT | VT_BYREF));
                ASSERT(V_VT(V_VARIANTREF(&pDispParams->rgvarg[0])) == VT_BSTR);

                OnDocumentComplete(V_BSTR(V_VARIANTREF(&pDispParams->rgvarg[0])));
                break;
            }

            case DISPID_TITLECHANGE:
            {
                if (pDispParams == NULL)
                {
                    hrResult = DISP_E_PARAMNOTOPTIONAL;
                    break;
                }

                if (pDispParams->cArgs != 1)
                {
                    hrResult = DISP_E_BADPARAMCOUNT;
                    break;
                }

                ASSERT(V_VT(&pDispParams->rgvarg[0]) == VT_BSTR);
    
                hrResult = OnTitleChange(V_BSTR(&pDispParams->rgvarg[0]));
                break;
            }

            case DISPID_HTMLDOCUMENTEVENTS_ONCLICK:
            {
                DWORD dwFeatureFlags;
                EVAL(SUCCEEDED(m_pSSConfig->get_Features(&dwFeatureFlags)));

                if (!(dwFeatureFlags & FEATURE_IGNORE_MOUSE_CLICKS))
                {
                    m_bMouseClicked = TRUE;

                    // Kill the current click check timer and restart if needed.
                    if (m_idClickCheckTimer != 0)
                    {
                        EVAL(KillTimer(m_idClickCheckTimer));
                        m_idClickCheckTimer = 0;
                    }

                    EVAL((m_idClickCheckTimer = SetTimer(   ID_CLICKCHECK_TIMER,
                                                            TIME_CLICKCHECK_TIMEOUT,
                                                            NULL)) != 0);
                }

                break;
            }

            default:
                hrResult = DISP_E_MEMBERNOTFOUND;
                break;
        }

        break;
    }

    return hrResult;
}

#if 0

// Darrenmi will wire this into the IOleCommandTarget interface 

/////////////////////////////////////////////////////////////////////////////
// INotificationSink interface
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CScreenSaverWindow::OnNotification
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP THIS_CLASS::OnNotification
(
    LPNOTIFICATION          pNotification,
    LPNOTIFICATIONREPORT    pNotfctnReport,
    DWORD                   dwReserved
)
{
    ASSERT(pNotification != NULL);

    TraceMsg(TF_NOTIFY, "OnNotification()");

    NOTIFICATIONTYPE nt;
    if (SUCCEEDED(pNotification->GetNotificationInfo(   &nt,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        0)))
    {
        if (InlineIsEqualGUID(nt, NOTIFICATIONTYPE_INET_OFFLINE))
        {
            TraceMsg(TF_NOTIFY, "Going OFFLINE");
            EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(VARIANT_TRUE)));
        }
        else if (InlineIsEqualGUID(nt, NOTIFICATIONTYPE_INET_ONLINE))
        {
            TraceMsg(TF_NOTIFY, "Going ONLINE");
            EVAL(SUCCEEDED(m_pWebBrowser->put_Offline(VARIANT_FALSE)));
        }
        else
            ASSERT(FALSE);
    }
    else
        ASSERT(FALSE);

    return S_OK;
}

#endif
