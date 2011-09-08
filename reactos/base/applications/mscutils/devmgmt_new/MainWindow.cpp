#include "StdAfx.h"
#include "devmgmt.h"
#include "MainWindow.h"


CMainWindow::CMainWindow(void) :
    m_hMainWnd(NULL),
    m_CmdShow(0)
{
    m_szMainWndClass = L"DevMgmtWndClass";
}


CMainWindow::~CMainWindow(void)
{
}

BOOL
CMainWindow::StatusBarLoadString(IN HWND hStatusBar,
                                 IN INT PartId,
                                 IN HINSTANCE hInstance,
                                 IN UINT uID)
{
    CAtlString szMessage;
    BOOL Ret = FALSE;

    /* Load the string */
    if (szMessage.LoadStringW(hInstance, uID))
    {
        /* Send the message to the status bar */
        Ret = (BOOL)SendMessageW(hStatusBar,
                                SB_SETTEXT,
                                (WPARAM)PartId,
                                (LPARAM)szMessage.GetBuffer());
    }

    return Ret;
}

LRESULT CALLBACK
CMainWindow::MainWndProc(HWND hwnd,
                         UINT msg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    CMainWindow *pThis;
    LRESULT Ret = 0;

    /* Get the object pointer from window context */
    pThis = (CMainWindow *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    /* Check for an invalid pointer */
    if (!pThis)
    {
        /* Check that this isn't a create message */
        if (msg != WM_CREATE)
        {
            /* Don't handle null info pointer */
            goto HandleDefaultMessage;
        }
    }

    switch(msg)
    {
        case WM_CREATE:
        {
            /* Get the object pointer from the create param */
            pThis = (CMainWindow *)((LPCREATESTRUCT)lParam)->lpCreateParams;

            /* Store the info pointer in the window's global user data */
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

            /* Display the window according to the user request */
            ShowWindow(hwnd, pThis->m_CmdShow);

            break;
        }

        default:
        {
HandleDefaultMessage:
            Ret = DefWindowProc(hwnd, msg, wParam, lParam);
            break;
        }
    }

    return Ret;
}

BOOL
CMainWindow::Initialize(LPCTSTR lpCaption,
                        int nCmdShow)
{
    CAtlString szCaption;
    WNDCLASSEXW wc = {0};

    /* Store the show window value */
    m_CmdShow = nCmdShow;

    /* Setup the window class struct */
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = g_hInstance;
    wc.hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAINMENU);
    wc.lpszClassName = m_szMainWndClass;
    wc.hIconSm = (HICON)LoadImage(g_hInstance,
                                  MAKEINTRESOURCE(IDI_MAIN_ICON),
                                  IMAGE_ICON,
                                  16,
                                  16,
                                  LR_SHARED);

    /* Register the window */
    if (RegisterClassExW(&wc))
    {
        /* Create the main window and store the info pointer */
        m_hMainWnd = CreateWindowExW(WS_EX_WINDOWEDGE,
                                     m_szMainWndClass,
                                     lpCaption,
                                     WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     600,
                                     450,
                                     NULL,
                                     NULL,
                                     g_hInstance,
                                     this);
    }

    /* Return creation result */
    return !!(m_hMainWnd);
}

VOID
CMainWindow::Uninitialize()
{
    /* Unregister the window class */
    UnregisterClassW(m_szMainWndClass, g_hInstance);
}

INT
CMainWindow::Run()
{
    MSG Msg;

    /* Pump the message queue */
    while (GetMessageW(&Msg, NULL, 0, 0 ))
    {
        TranslateMessage(&Msg);
        DispatchMessageW(&Msg);
    }

    return 0;
}