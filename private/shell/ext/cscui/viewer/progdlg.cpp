//////////////////////////////////////////////////////////////////////////////
/*  File: progdlg.cpp

    Description: Provides progress dialog used by the viewer for copy
        and deletion operations.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    01/16/98    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "progdlg.h"
#include "ccinline.h"
#include "pathstr.h"
#include "msgbox.h"
#include <resource.h>


//-----------------------------------------------------------------------------
// CProgressDialog
//-----------------------------------------------------------------------------
CProgressDialog::CProgressDialog(
    void
    ) throw()
      : m_hInstance(NULL),
        m_hwndDlg(NULL),
        m_hwndProgBar(NULL),
        m_cMax(0),
        m_bCancelled(false)
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CProgressDialog::CProgressDialog")));
}

CProgressDialog::~CProgressDialog(
    void
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CProgressDialog::~CProgressDialog")));
    if (NULL != m_hwndDlg)
        DestroyWindow(m_hwndDlg);
}

void
CProgressDialog::Run(
    HINSTANCE hInstance,
    HWND hwndParent,
    int cMax
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CProgressDialog::Run")));
    DBGASSERT((NULL != hInstance));
    DBGASSERT((NULL != hwndParent));
    DBGASSERT((0 < cMax));
   
    m_hInstance = hInstance;
    m_cMax      = cMax;
    if (2 < cMax)
    {
        m_hwndDlg = CreateDialogParam(hInstance, 
                                      MAKEINTRESOURCE(IDD_CACHEVIEW_PROGRESS),
                                      hwndParent,
                                      (DLGPROC)DlgProc,
                                      (LPARAM)this);
        if (NULL == m_hwndDlg)
            DBGERROR((TEXT("Error %d creating copy progress dialog"), GetLastError()));
    }
}


void
CProgressDialog::Cancel(
    void
    ) throw()
{
    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("Copy operation cancelled.")));
    m_bCancelled = true;
    //
    // This dialog is run modeless so we need to set it's parent (the 
    // cache viewer) back to the foreground if the operation
    // is cancelled.
    //
    SetForegroundWindow(GetParent(m_hwndDlg));
    if (NULL != m_hwndDlg)
        DestroyWindow(m_hwndDlg);
}


BOOL CALLBACK
CProgressDialog::DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    ) throw()
{
    CProgressDialog *pThis = reinterpret_cast<CProgressDialog *>(GetWindowLongPtr(hwnd, DWLP_USER));
    
    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
                pThis = reinterpret_cast<CProgressDialog *>(lParam);
                SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)pThis);
                DBGASSERT((NULL != pThis));
                pThis->OnInitDialog(hwnd);
                return TRUE;

            case WM_ENDSESSION:
                pThis->Cancel();
                break;

            case WM_COMMAND:    
                DBGASSERT((NULL != pThis));
                switch(LOWORD(wParam))
                {
                    case IDCANCEL:
                        pThis->Cancel();
                        break;
                                 
                    default:
                        break;
                }
                break;

            case WM_DESTROY:
                pThis->OnDestroy(hwnd);
                break;

            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CProgressDialog::DlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    catch(...)
    {
        DBGERROR((TEXT("Unknown exception caught in CProgressDialog::DlgProc")));
        CscWin32Message(NULL, DISP_E_EXCEPTION, CSCUI::SEV_ERROR);
    }
    return FALSE;
}


void
CProgressDialog::OnInitDialog(
    HWND hwnd
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CProgressDialog::OnInitDialog")));
    DBGASSERT((NULL != hwnd));

    m_hwndDlg = hwnd;
    m_hwndProgBar = GetDlgItem(hwnd, IDC_CACHEVIEW_PROGDLG_PROGBAR);

    ProgressBar_SetRange(m_hwndProgBar, 0, m_cMax);
    ProgressBar_SetStep(m_hwndProgBar, 1);
    ProgressBar_SetPos(m_hwndProgBar, 0);
    ::CenterPopupWindow(hwnd, GetParent(hwnd));
    FlushMessages();
}


void
CProgressDialog::OnDestroy(
    HWND hwnd
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CProgressDialog::OnDestroy")));
    m_hwndDlg     = NULL;
    m_hwndProgBar = NULL;
}


void 
CProgressDialog::SetOperationText(
    LPCTSTR pszText
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CProgressDialog::SetOperationText")));
    DBGPRINT((DM_VIEW, DL_LOW, TEXT("Operation: \"%s\""), pszText));

    if (NULL != m_hwndDlg)
    {
        SetWindowText(GetDlgItem(m_hwndDlg, IDC_TXT_CACHEVIEW_PROGDLG_OPER), pszText);
        FlushMessages();
    }
}

void
CProgressDialog::SetFilenameText(
    LPCTSTR pszText
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CProgressDialog::SetFilenameText")));
    DBGPRINT((DM_VIEW, DL_LOW, TEXT("File: \"%s\""), pszText));
    //
    // Update the file/folder name text.  Size path string to fit into
    // text control.
    //
    CPath strDisp(pszText);
    HDC hdc = GetDC(m_hwndDlg);
    RECT rc;
    HWND hwndText = GetDlgItem(m_hwndDlg, IDC_TXT_CACHEVIEW_PROGDLG_FILE);
    GetClientRect(hwndText, &rc);
    strDisp.Compact(hdc, rc.right - rc.left);
    ReleaseDC(m_hwndDlg, hdc);
    SetWindowText(hwndText, strDisp);
    FlushMessages();
}


void 
CProgressDialog::Advance(
    void
    ) throw()
{ 
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CProgressDialog::Advance")));
    if (NULL != m_hwndProgBar) 
        ProgressBar_StepIt(m_hwndProgBar); 
    FlushMessages();
}


void
CProgressDialog::FlushMessages(
    void
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CProgressDialog::FlushMessages")));
    MSG msg;

    while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) &&
           WM_QUIT != msg.message)
    {
        ::GetMessage(&msg, NULL, 0, 0);
        if (!::IsDialogMessage(m_hwndDlg, &msg))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}

