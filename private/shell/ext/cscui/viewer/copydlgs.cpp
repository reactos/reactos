//////////////////////////////////////////////////////////////////////////////
/*  File: copydlgs.cpp

    Description: Provides dialogs used in the "Copy To Folder" operation.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    01/16/98    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "copydlgs.h"
#include "ccinline.h"
#include "pathstr.h"
#include "msgbox.h"
#include <resource.h>

//-----------------------------------------------------------------------------
// ConfirmCopyOverDialog
//
// Invoked when the copy process is about to overwrite an existing file.
//-----------------------------------------------------------------------------
    
ConfirmCopyOverDialog::ConfirmCopyOverDialog(
    void
    ) throw()
      : m_hInstance(NULL),
        m_hwnd(NULL)
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("ConfirmCopyOverDialog::ConfirmCopyOverDialog")));
}
       


ConfirmCopyOverDialog::~ConfirmCopyOverDialog(
    void
    ) throw()
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("ConfirmCopyOverDialog::~ConfirmCopyOverDialog")));
    if (NULL != m_hwnd)
        DestroyWindow(m_hwnd);
}


int
ConfirmCopyOverDialog::Run(
    HINSTANCE hInstance,
    HWND hwndParent,
    LPCTSTR pszFile
    )
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("ConfirmCopyOverDialog::Run")));
    DBGASSERT((NULL != hInstance));
    DBGASSERT((NULL != hwndParent));
   
    m_hInstance = hInstance;
    m_strFile   = pszFile;
    int iResult = DialogBoxParam(hInstance, 
                          MAKEINTRESOURCE(IDD_CACHEVIEW_CONFIRMCOPYOVER),
                          hwndParent,
                          (DLGPROC)DlgProc,
                          (LPARAM)this);
    if (-1 == iResult)
        DBGERROR((TEXT("Error %d creating confirm copy-over dialog"), GetLastError()));
    return iResult;
}


BOOL CALLBACK
ConfirmCopyOverDialog::DlgProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    ConfirmCopyOverDialog *pThis = reinterpret_cast<ConfirmCopyOverDialog *>(GetWindowLongPtr(hwnd, DWLP_USER));

    try
    {
        switch(message)
        {
            case WM_INITDIALOG:
                SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)lParam);
                pThis = reinterpret_cast<ConfirmCopyOverDialog *>(lParam);
                DBGASSERT((NULL != pThis));
                pThis->OnInitDialog(hwnd);
                return TRUE;

            case WM_ENDSESSION:
                EndDialog(hwnd, IDNO);
                return FALSE;

            case WM_COMMAND:    
                DBGASSERT((NULL != pThis));
                EndDialog(hwnd, LOWORD(wParam));
                return FALSE;

            case WM_DESTROY:
                pThis->m_hwnd = NULL;
                return FALSE;

            default:
                break;
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in ConfirmCopyOverDialog::DlgProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    catch(...)
    {
        DBGERROR((TEXT("Unknown exception caught in ConfirmCopyOverDialog::DlgProc")));
        CscWin32Message(NULL, DISP_E_EXCEPTION, CSCUI::SEV_ERROR);
    }

    return FALSE;
}


void
ConfirmCopyOverDialog::OnInitDialog(
    HWND hwnd
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("ConfirmCopyOverDialog::OnInitDialog")));
    DBGASSERT((NULL != hwnd));

    m_hwnd = hwnd;
    CString strText(m_hInstance, IDS_FMT_CACHEVIEW_CONFIRMCOPYOVER, m_strFile.Cstr());
    SetWindowText(GetDlgItem(hwnd, IDC_TXT_CACHEVIEW_CONFIRMCOPYOVER), strText);
}


//-----------------------------------------------------------------------------
// CopyProgressDialog
//-----------------------------------------------------------------------------
CopyProgressDialog::CopyProgressDialog(
    void
    ) throw()
      : m_iCopyOp(-1)
{
    DBGTRACE((DM_VIEW, DL_MID, TEXT("CopyProgressDialog::CopyProgressDialog")));
}


void 
CopyProgressDialog::UpdateStatusText(
    LPCTSTR pszName,
    bool bIsFolder
    )
{
    DBGTRACE((DM_VIEW, DL_LOW, TEXT("CopyProgressDialog::UpdateStatusText")));
    DBGPRINT((DM_VIEW, DL_LOW, TEXT("File: \"%s\""), pszName));

    //
    // Update "action" text only it it has changed.
    //
    if (m_iCopyOp != int(bIsFolder))
    {
        m_iCopyOp = int(bIsFolder);
        CString strText(GetModuleInstance(), bIsFolder ? IDS_TXT_CACHEVIEW_COPYPROG_CREATING : 
                                                         IDS_TXT_CACHEVIEW_COPYPROG_COPYING);
        SetOperationText(strText);
    }
    SetFilenameText(pszName);
}

