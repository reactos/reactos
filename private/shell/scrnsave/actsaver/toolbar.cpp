/////////////////////////////////////////////////////////////////////////////
// TOOLBAR.CPP
//
// Implementation of CToolbarWindow
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     05/15/97    Created
// jaym     07/02/97    Updated to new UI
// jaym     07/30/97    Updated to newer UI
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "resource.h"
#include "toolbar.h"

extern BOOL g_bPasswordEnabled;

/////////////////////////////////////////////////////////////////////////////
// Design constants
/////////////////////////////////////////////////////////////////////////////
#define ID_PROPERTIES_BUTTON    0x0201
#define ID_CLOSE_BUTTON         0x0202
    // Child window IDs

static const TBBUTTON s_tbbProperties =
{
    IDTB_PROPERTIES,
    IDM_TOOLBAR_PROPERTIES,
    TBSTATE_ENABLED,
    TBSTYLE_BUTTON,
    0L,
    0
};
static const TBBUTTON s_tbbClose =
{
    IDTB_CLOSE,
    IDM_TOOLBAR_CLOSE,
    TBSTATE_ENABLED,
    TBSTYLE_BUTTON,
    0L,
    0
};
    // Toolbar buttons

#define CX_PROPERTIES           27
#define CY_PROPERTIES           27
#define CX_PROPERTIES_INDENT    2
#define CY_PROPERTIES_INDENT    1
#define CX_CLOSE                20
#define CY_CLOSE                18
#define CX_CLOSE_INDENT         2
#define CY_CLOSE_INDENT         1
#define CY_SLIDE_INCREMENT      1
    // Toolbar button tweaks

/////////////////////////////////////////////////////////////////////////////
// CToolbarWindow
/////////////////////////////////////////////////////////////////////////////
CToolbarWindow::CToolbarWindow
(
)
{
    m_pSSWnd            = NULL;
    m_cy                = 0;
    m_hwndProperties    = NULL;
    m_hwndClose         = NULL;
}

CToolbarWindow::~CToolbarWindow
(
)
{
    if  (
        (m_hwndProperties != NULL)
        &&
        IsWindow(m_hwndProperties)
        )
    {
        DestroyWindow(m_hwndProperties);
    }

    if  (
        (m_hwndClose != NULL)
        &&
        IsWindow(m_hwndClose)
        )
    {
        DestroyWindow(m_hwndClose);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CToolbarWindow::Create
/////////////////////////////////////////////////////////////////////////////
BOOL CToolbarWindow::Create
(
    const RECT &            rect,
    CScreenSaverWindow *    pParentWnd
)
{
    BOOL bResult = FALSE;

    ASSERT(pParentWnd != NULL);
    TraceMsg(TF_ALWAYS, "CToolbarWindow::Create(hwndParent=%x)", (pParentWnd) ? pParentWnd->m_hWnd : 0);

    // Initalize the Common Controls.
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_USEREX_CLASSES | ICC_COOL_CLASSES;  // REVIEW: COOL? [jm]
    EVAL(SUCCEEDED(InitCommonControlsEx(&icc)));

    for (;;)
    {
        if (!CWindow::CreateEx( NULL,
                                0,
                                WS_POPUP
                                    | WS_CLIPCHILDREN
                                    | WS_CLIPSIBLINGS,
                                rect,
                                pParentWnd->m_hWnd,
                                0))
        {
            break;
        }

        m_cy = (rect.bottom - rect.top);

        // Properties button
        TraceMsg(TF_ALWAYS, "CToolbarWindow::Create(m_hWnd=%x)", m_hWnd);

        if ((m_hwndProperties = CreateWindowEx( WS_EX_TOOLWINDOW,
                                                TOOLBARCLASSNAME,
                                                NULL,
                                                WS_CHILD
                                                    | WS_CLIPCHILDREN
                                                    | WS_CLIPSIBLINGS
                                                    | TBSTYLE_TRANSPARENT
                                                    | TBSTYLE_FLAT
                                                    | TBSTYLE_TOOLTIPS
                                                    | CCS_NORESIZE
                                                    | CCS_NODIVIDER
                                                    | CCS_NOPARENTALIGN,
                                                CX_PROPERTIES_INDENT,
                                                CY_PROPERTIES_INDENT,
                                                CX_PROPERTIES,
                                                CY_PROPERTIES,
                                                m_hWnd,
                                                (HMENU)ID_PROPERTIES_BUTTON,
                                                _pModule->GetModuleInstance(),
                                                NULL)) == NULL)
        {
            break;
        }

        // #63544: Toolbox doesn't appear when mouse is moved, NT only
        ::SendMessage(m_hwndProperties, TB_GETTOOLTIPS, 0, 0);

        // Set the default toolbar properties button image.
        HIMAGELIST hImgList;
        hImgList = ImageList_LoadImage( _pModule->GetModuleInstance(),
                                        MAKEINTRESOURCE(IDB_PROPERTIES_DEFAULT),
                                        CX_PROPERTIES_BUTTONBITMAP,
                                        0,
                                        RGB(255, 0, 255),
                                        IMAGE_BITMAP,
                                        LR_CREATEDIBSECTION);

        ImageList_SetBkColor(hImgList, RGB(0, 0, 0));
        ::SendMessage(m_hwndProperties, TB_SETIMAGELIST, 0, (LPARAM)hImgList);

        // Set the hot (rollover) toolbar properties button image.
        hImgList = ImageList_LoadImage( _pModule->GetModuleInstance(),
                                        MAKEINTRESOURCE(IDB_PROPERTIES),
                                        CX_PROPERTIES_BUTTONBITMAP,
                                        0,
                                        RGB(255, 0, 255),
                                        IMAGE_BITMAP,
                                        LR_CREATEDIBSECTION);

        ImageList_SetBkColor(hImgList, RGB(0, 0, 0));
        ::SendMessage(m_hwndProperties, TB_SETHOTIMAGELIST, 0, (LPARAM)hImgList);

        // Add the button to the toolbar.
        ::SendMessage(m_hwndProperties, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        ::SendMessage(m_hwndProperties, TB_ADDBUTTONS, 1, (LPARAM)(LPCTBBUTTON)&s_tbbProperties);

        // Close button
        if ((m_hwndClose = CreateWindowEx(  WS_EX_TOOLWINDOW,
                                            TOOLBARCLASSNAME,
                                            NULL,
                                            WS_CHILD
                                                | WS_CLIPCHILDREN
                                                | WS_CLIPSIBLINGS
                                                | TBSTYLE_TRANSPARENT
                                                | TBSTYLE_FLAT
                                                | TBSTYLE_TOOLTIPS
                                                | CCS_NORESIZE
                                                | CCS_NODIVIDER
                                                | CCS_NOPARENTALIGN,
                                            CX_PROPERTIES_INDENT + CX_PROPERTIES + CX_CLOSE_INDENT,
                                            CY_CLOSE_INDENT,
                                            CX_CLOSE,
                                            CY_CLOSE,
                                            m_hWnd,
                                            (HMENU)ID_CLOSE_BUTTON,
                                            _pModule->GetModuleInstance(),
                                            NULL)) == NULL)
        {
            break;
        }

        // #63544: Toolbox doesn't appear when mouse is moved, NT only
        ::SendMessage(m_hwndClose, TB_GETTOOLTIPS, 0, 0);

        // Set the hot (rollover) toolbar close button image.
#if 0
        hImgList = ImageList_LoadImage( _pModule->GetModuleInstance(),
                                        MAKEINTRESOURCE(IDB_CLOSE),
                                        CX_CLOSE_BUTTONBITMAP,
                                        0,
                                        RGB(255, 0, 255),
                                        IMAGE_BITMAP,
                                        LR_CREATEDIBSECTION);
        ImageList_SetBkColor(hImgList, RGB(0, 0, 0)); 
        
#endif
        // #65186: button image is invisible with High Contrast Black scheme
	    if ((hImgList = ImageList_Create(CX_CLOSE_BUTTONBITMAP,
                                         CY_CLOSE_BUTTONBITMAP,
                                         ILC_COLOR | ILC_MASK,
                                         1, 0)) == NULL)
	    {
	        break;
        }
        
        HBITMAP hBitmap;
        if ((hBitmap = (HBITMAP)CreateMappedBitmap(_pModule->GetModuleInstance(),
                                                IDB_CLOSE,
                                                0,
                                                NULL,
                                                0)) != NULL)
        {
            EVAL(ImageList_AddMasked(hImgList, hBitmap, CLR_DEFAULT) != -1);
            DeleteObject(hBitmap);
        }
        else
            break;

        ::SendMessage(m_hwndClose, TB_SETIMAGELIST, 0, (LPARAM)hImgList);

        // Add the button to the toolbar.
        ::SendMessage(m_hwndClose, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        ::SendMessage(m_hwndClose, TB_ADDBUTTONS, 1, (LPARAM)(LPCTBBUTTON)&s_tbbClose);

        //
        // Disable properties if not interactive or screensaver is password 
        // protected
        //
        if (g_bPasswordEnabled || !pParentWnd->UserIsInteractive())
            ::SendMessage(m_hwndProperties, TB_SETSTATE, IDM_TOOLBAR_PROPERTIES, 0);

        m_pSSWnd = pParentWnd;

        bResult = TRUE;
        break;
    }

    return bResult;
}

/////////////////////////////////////////////////////////////////////////////
// CToolbarWindow::OnCommand
/////////////////////////////////////////////////////////////////////////////
BOOL CToolbarWindow::OnCommand
(
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (wParam)
    {
        case IDM_TOOLBAR_PROPERTIES:
        {
            m_pSSWnd->ShowPropertiesDlg(m_hWnd);
            break;
        }

        case IDM_TOOLBAR_CLOSE:
        {
            m_pSSWnd->Quit();
            break;
        }

        default:
        {
            ASSERT(FALSE);
            return FALSE;
        }
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CToolbarWindow::OnEraseBkgnd
/////////////////////////////////////////////////////////////////////////////
BOOL CToolbarWindow::OnEraseBkgnd
(
    HDC hDC
)
{
    TraceMsg(TF_ALWAYS, "CToolbarWindow::OnEraseBkgnd");

    // Erase the background.
    RECT rectClient;
    GetClientRect(&rectClient);

    DrawEdge(   hDC,
                &rectClient,
                EDGE_RAISED,
                BF_BOTTOMLEFT | BF_MIDDLE | BF_SOFT);
                
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CToolbarWindow::OnNotify
/////////////////////////////////////////////////////////////////////////////
BOOL CToolbarWindow::OnNotify
(
    WPARAM      wParam,
    LPARAM      lParam,
    LRESULT *   plResult
)
{
    TOOLTIPTEXT * pttt = (TOOLTIPTEXT *)lParam;

    switch (pttt->hdr.code)
    {
        case TTN_NEEDTEXT:
        {
            // Handle ToolTip notification sent by Toolbar.
            pttt->hinst = _pModule->GetResourceInstance();
            pttt->lpszText = MAKEINTRESOURCE(wParam);

            return TRUE;
        }

        default:
            break;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CToolbarWindow::ShowToolbar
/////////////////////////////////////////////////////////////////////////////
void CToolbarWindow::ShowToolbar
(
    BOOL bShow
)
{
   TraceMsg(TF_ALWAYS, "CToolbarWindow::ShowToolbar(m_hWnd=%x, hWndParent=%x)", m_hWnd, ::GetParent(m_hWnd));

   if (bShow)
        ShowWindow(SW_SHOW);

    for (;;)
    {
        RECT rect;
        GetWindowRect(&rect);

        if (bShow)
        {
            rect.top += CY_SLIDE_INCREMENT;

            if (rect.top <= 0)
            {
                SetWindowPos(   NULL,
                                rect.left, rect.top,
                                0, 0,
                                SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
                UpdateWindow(m_hWnd);
                Sleep(10);
            }
            else
                break;
        }
        else
        {
            rect.top -= CY_SLIDE_INCREMENT;

            if (rect.top >= -m_cy)
            {
                SetWindowPos(   NULL,
                                rect.left, rect.top,
                                0, 0,
                                SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

                UpdateWindow(m_hWnd);
                UpdateWindow(::GetParent(m_hWnd));
                Sleep(10);
            }
            else
            {
                ShowWindow(SW_HIDE);
                break;
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// CToolbarWindow::OnShowWindow
/////////////////////////////////////////////////////////////////////////////
void CToolbarWindow::OnShowWindow
(
    BOOL    bShow,
    int     nStatus
)
{
    if  (
        (m_hwndProperties != NULL)
        &&
        IsWindow(m_hwndProperties)
        )
    {
        TraceMsg(TF_ALWAYS, "CToolbarWindow::OnShowWindow(Properties)");
        ::ShowWindow(m_hwndProperties, (bShow ? SW_SHOW : SW_HIDE));
    }

    if  (
        (m_hwndClose != NULL)
        &&
        IsWindow(m_hwndClose)
        )
    {
        TraceMsg(TF_ALWAYS, "CToolbarWindow::OnShowWindow(Close)");
        ::ShowWindow(m_hwndClose, (bShow ? SW_SHOW : SW_HIDE));
    }
}
