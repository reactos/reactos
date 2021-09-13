/*
* PROJECT:     ReactOS Character Map
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        base/applications/charmap/MainWindow.cpp
* PURPOSE:     Implements the main dialog window
* COPYRIGHT:   Copyright 2015 Ged Murphy <gedmurphy@reactos.org>
*/


#include "precomp.h"
#include "MainWindow.h"


/* DATA *****************************************************/

#define ID_ABOUT    0x1

HINSTANCE g_hInstance = NULL;


/* PUBLIC METHODS **********************************************/

CCharMapWindow::CCharMapWindow(void) :
    m_hMainWnd(NULL),
    m_hStatusBar(NULL),
    m_CmdShow(0),
    m_hRichEd(NULL),
    m_GridView(nullptr)
{
    m_GridView = new CGridView();
}

CCharMapWindow::~CCharMapWindow(void)
{
}

bool
CCharMapWindow::Create(_In_ HINSTANCE hInst,
                       _In_ int nCmdShow)
{
    INITCOMMONCONTROLSEX icex;
    CAtlStringW szAppName;
    int Ret = 1;

    // Store the instance
    g_hInstance = hInst;
    m_CmdShow = nCmdShow;

    // Initialize common controls
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    // Load the application name
    if (szAppName.LoadStringW(g_hInstance, IDS_TITLE))
    {
        // Initialize the main window
        if (Initialize(szAppName, nCmdShow))
        {
            // Run the application
            Ret = Run();

            // Uninitialize the main window
            Uninitialize();
        }
    }

    return (Ret == 0);
}



/* PRIVATE METHODS **********************************************/

bool
CCharMapWindow::Initialize(_In_z_ LPCTSTR lpCaption,
                           _In_ int nCmdShow)
{
    // The dialog has a rich edit text box
    m_hRichEd = LoadLibraryW(L"riched20.DLL");
    if (m_hRichEd == NULL) return false;

    return !!(CreateDialogParamW(g_hInstance,
                                 MAKEINTRESOURCE(IDD_CHARMAP),
                                 NULL,
                                 DialogProc,
                                 (LPARAM)this));
}

void
CCharMapWindow::Uninitialize(void)
{
    if (m_hRichEd)
        FreeLibrary(m_hRichEd);
}

int
CCharMapWindow::Run(void)
{
    MSG Msg;

    // Pump the message queue
    while (GetMessageW(&Msg, NULL, 0, 0) != 0)
    {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }

    return 0;
}

void
CCharMapWindow::UpdateStatusBar(_In_ bool InMenuLoop)
{
    SendMessageW(m_hStatusBar,
                 SB_SIMPLE,
                 (WPARAM)InMenuLoop,
                 0);
}

bool
CCharMapWindow::CreateStatusBar(void)
{
    int StatWidths[] = { 110, -1 }; // widths of status bar
    bool bRet = FALSE;

    // Create the status bar
    m_hStatusBar = CreateWindowExW(0,
                                   STATUSCLASSNAME,
                                   NULL,
                                   WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                   0, 0, 0, 0,
                                   m_hMainWnd,
                                   (HMENU)IDD_STATUSBAR,
                                   g_hInstance,
                                   NULL);
    if (m_hStatusBar)
    {
        // Create the sections
        bRet = (SendMessageW(m_hStatusBar,
                             SB_SETPARTS,
                             sizeof(StatWidths) / sizeof(int),
                             (LPARAM)StatWidths) != 0);

        // Set the status bar for multiple parts output
        SendMessage(m_hStatusBar, SB_SIMPLE, (WPARAM)FALSE, (LPARAM)0);
    }

    return bRet;
}

bool
CCharMapWindow::StatusBarLoadString(_In_ HWND hStatusBar,
                                    _In_ INT PartId,
                                    _In_ HINSTANCE hInstance,
                                    _In_ UINT uID)
{
    CAtlStringW szMessage;
    bool bRet = false;

    // Load the string from the resource
    if (szMessage.LoadStringW(hInstance, uID))
    {
        // Display it on the status bar
        bRet = (SendMessageW(hStatusBar,
                             SB_SETTEXT,
                             (WPARAM)PartId,
                             (LPARAM)szMessage.GetBuffer()) != 0);
    }

    return bRet;
}

BOOL
CCharMapWindow::OnCreate(_In_ HWND hDlg)
{
    m_hMainWnd = hDlg;

    if (!CreateStatusBar())
        return FALSE;

    if (!m_GridView->Create(hDlg))
        return FALSE;

    // Load an 'about' option into the system menu
    HMENU hSysMenu;
    hSysMenu = GetSystemMenu(m_hMainWnd, FALSE);
    if (hSysMenu != NULL)
    {
        CAtlStringW AboutText;
        if (AboutText.LoadStringW(IDS_ABOUT))
        {
            AppendMenuW(hSysMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hSysMenu, MF_STRING, ID_ABOUT, AboutText);
        }
    }

    // Add all the fonts to the list
    if (!CreateFontComboBox())
        return FALSE;

    ChangeMapFont();

    // Configure Richedit control for sending notification changes.
    DWORD evMask;
    evMask = SendDlgItemMessage(hDlg, IDC_TEXTBOX, EM_GETEVENTMASK, 0, 0);
    evMask |= ENM_CHANGE;
    SendDlgItemMessage(hDlg, IDC_TEXTBOX, EM_SETEVENTMASK, 0, (LPARAM)evMask);

    // Display the window according to the user request
    ShowWindow(m_hMainWnd, m_CmdShow);

    return TRUE;
}

BOOL
CCharMapWindow::OnSize(
    _In_ WPARAM wParam
    )
{
    RECT rcClient, rcStatus;
    INT lvHeight, iStatusHeight;

    // Resize the status bar
    SendMessage(m_hStatusBar, WM_SIZE, 0, 0);

    // Get the statusbar rect and save the height
    GetWindowRect(m_hStatusBar, &rcStatus);
    iStatusHeight = rcStatus.bottom - rcStatus.top;

    // Get the full client rect
    GetClientRect(m_hMainWnd, &rcClient);

    // Calculate the remaining height for the gridview
    lvHeight = rcClient.bottom - iStatusHeight;

    // Resize the grid view
    SendMessageW(m_GridView->GetHwnd(), WM_SIZE, wParam, 0);

    return TRUE;
}

BOOL
CCharMapWindow::OnNotify(_In_ LPARAM lParam)
{
    LPNMHDR NmHdr = (LPNMHDR)lParam;
    LRESULT Ret = 0;

    switch (NmHdr->code)
    {
    case NM_RCLICK:
    {
        break;
    }

    case NM_DBLCLK:
    case NM_RETURN:
    {
        break;
    }
    }

    return Ret;
}

BOOL
CCharMapWindow::OnContext(_In_ LPARAM lParam)
{
    return 0;// m_GridView->OnContextMenu(lParam);
}

BOOL
CCharMapWindow::OnCommand(_In_ WPARAM wParam,
                          _In_ LPARAM /*lParam*/)
{
    LRESULT RetCode = 0;
    WORD Msg;

    // Get the message
    Msg = LOWORD(wParam);

    switch (Msg)
    {
    case IDC_CHECK_ADVANCED:
        break;

    case IDC_FONTCOMBO:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            ChangeMapFont();
        }
        break;

    default:
        // We didn't handle it
        RetCode = -1;
        break;
    }

    return RetCode;
}

BOOL
CCharMapWindow::OnDestroy(void)
{
    // Clear the user data pointer
    SetWindowLongPtr(m_hMainWnd, GWLP_USERDATA, 0);

    // Break the message loop
    PostQuitMessage(0);

    return TRUE;
}

INT_PTR CALLBACK
CCharMapWindow::DialogProc(
    _In_ HWND   hwndDlg,
    _In_ UINT   Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    CCharMapWindow *This;
    LRESULT RetCode = 0;

    // Get the object pointer from window context
    This = (CCharMapWindow *)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    if (This == NULL)
    {
        // Check that this isn't a create message
        if (Msg != WM_INITDIALOG)
        {
            // Don't handle null info pointer
            return FALSE;
        }
    }

    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        // Get the object pointer from the create param
        This = (CCharMapWindow *)lParam;

        // Store the pointer in the window's global user data
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)This);

        // Call the create handler
        return This->OnCreate(hwndDlg);
    }

    case WM_SIZE:
    {
        return This->OnSize(wParam);
    }

    case WM_NOTIFY:
    {
        return This->OnNotify(lParam);
    }

    case WM_CONTEXTMENU:
    {
        return This->OnContext(lParam);
    }

    case WM_COMMAND:
    {
        return This->OnCommand(wParam, lParam);
    }

    case WM_SYSCOMMAND:
        switch (wParam)
        {
        case ID_ABOUT:
            MessageBoxW(This->m_hMainWnd,
                        L"ReactOS Character Map\nCopyright Ged Murphy 2015",
                        L"About",
                        MB_OK | MB_APPLMODAL);
            break;
        }
        break;

    case WM_ENTERMENULOOP:
    {
        This->UpdateStatusBar(true);
        return TRUE;
    }

    case WM_EXITMENULOOP:
    {
        This->UpdateStatusBar(false);
        return TRUE;
    }

    case WM_CLOSE:
    {
        // Destroy the main window
        return DestroyWindow(hwndDlg);
    }


    case WM_DESTROY:
    {
        // Call the destroy handler
        return This->OnDestroy();
    }
    }

    return FALSE;
}

struct EnumFontParams
{
    CCharMapWindow *This;
    HWND hCombo;
};

int
CALLBACK
CCharMapWindow::EnumDisplayFont(ENUMLOGFONTEXW *lpelfe,
                                NEWTEXTMETRICEXW *lpntme,
                                DWORD FontType,
                                LPARAM lParam)
{
    EnumFontParams *Params = (EnumFontParams *)lParam;
    LPWSTR pszName = lpelfe->elfLogFont.lfFaceName;

    /* Skip rotated font */
    if (pszName[0] == L'@') return 1;

    /* make sure font doesn't already exist in our list */
    if (SendMessageW(Params->hCombo,
                     CB_FINDSTRINGEXACT,
                     0,
                     (LPARAM)pszName) == CB_ERR)
    {
        INT idx;
        idx = (INT)SendMessageW(Params->hCombo,
                                CB_ADDSTRING,
                                0,
                                (LPARAM)pszName);

        /* record the font's attributes (Fixedwidth and Truetype) */
        BOOL fFixed = (lpelfe->elfLogFont.lfPitchAndFamily & FIXED_PITCH) ? TRUE : FALSE;
        BOOL fTrueType = (lpelfe->elfLogFont.lfOutPrecision == OUT_STROKE_PRECIS) ? TRUE : FALSE;

        /* store this information in the list-item's userdata area */
        SendMessageW(Params->hCombo,
                     CB_SETITEMDATA,
                     idx,
                     MAKEWPARAM(fFixed, fTrueType));
    }

    return 1;
}


bool
CCharMapWindow::CreateFontComboBox()
{
    HWND hCombo;
    hCombo = GetDlgItem(m_hMainWnd, IDC_FONTCOMBO);

    NONCLIENTMETRICSW NonClientMetrics;
    NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICSW);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,
                          sizeof(NONCLIENTMETRICSW),
                          &NonClientMetrics,
                          0);

    // Get a handle to the font
    HFONT GuiFont;
    GuiFont = CreateFontIndirectW(&NonClientMetrics.lfMessageFont);

    // Set the font used in the combo box
    SendMessageW(hCombo,
                 WM_SETFONT,
                 (WPARAM)GuiFont,
                 0);

    // Set the fonts which we want to enumerate
    LOGFONTW FontsToEnum;
    ZeroMemory(&FontsToEnum, sizeof(LOGFONTW));
    FontsToEnum.lfCharSet = DEFAULT_CHARSET;

    // Set the params we want to pass to the callback
    EnumFontParams Params;
    Params.This = this;
    Params.hCombo = hCombo;

    // Get a DC for combo box
    HDC hdc;
    hdc = GetDC(hCombo);

    // Enumerate all the fonts
    int ret;
    ret = EnumFontFamiliesExW(hdc,
                              &FontsToEnum,
                              (FONTENUMPROCW)EnumDisplayFont,
                              (LPARAM)&Params,
                              0);

    ReleaseDC(hCombo, hdc);
    DeleteObject(GuiFont);

    // Select the first item in the list
    SendMessageW(hCombo,
                 CB_SETCURSEL,
                 0,
                 0);

    return (ret == 1);
}

bool
CCharMapWindow::ChangeMapFont(
    )
{
    HWND hCombo;
    hCombo = GetDlgItem(m_hMainWnd, IDC_FONTCOMBO);

    INT Length;
    Length = GetWindowTextLengthW(hCombo);
    if (!Length) return false;

    CAtlStringW FontName;
    FontName.Preallocate(Length);

    SendMessageW(hCombo,
                 WM_GETTEXT,
                 FontName.GetAllocLength(),
                 (LPARAM)FontName.GetBuffer());

    return m_GridView->SetFont(FontName);
}
