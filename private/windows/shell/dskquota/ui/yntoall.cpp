///////////////////////////////////////////////////////////////////////////////
/*  File: yntoall.cpp

    Description: Implements the YesNoToAll dialog.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "yntoall.h"
#include "resource.h"



///////////////////////////////////////////////////////////////////////////////
/*  Function: YesNoToAllDialog::YesNoToAllDialog

    Description: Class constructor.

    Arguments:
        idDialogTemplate - ID number for the dialog's resource template.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
YesNoToAllDialog::YesNoToAllDialog(
    UINT idDialogTemplate
    ) : m_idDialogTemplate(idDialogTemplate),
        m_hwndCbxApplyToAll(NULL),
        m_hwndTxtMsg(NULL),
        m_bApplyToAll(FALSE),
        m_pszTitle(NULL),
        m_pszText(NULL)
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("YesNoToAllDialog::YesNoToAllDialog")));
    //
    // Do nothing.
    //
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: YesNoToAllDialog::~YesNoToAllDialog

    Description: Class destructor.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
YesNoToAllDialog::~YesNoToAllDialog(
    VOID
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("YesNoToAllDialog::YesNoToAllDialog")));
    //
    // Call the Destroy() function to destroy the progress dialog window.
    //
    delete[] m_pszTitle;
    delete[] m_pszText;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: YesNoToAllDialog::Create

    Description: Creates the dialog.

    Arguments:
        hInstance - Instance handle for the DLL containing the dialog
            resource template.

        hwndParent - Parent window for dialog.

        lpszTitle - Title for dialog.

        lpszText - Text message for dialog.

    Returns:
        TRUE  = Dialog was created.
        FALSE = Dialog was not created.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
YesNoToAllDialog::CreateAndRun(
    HINSTANCE hInstance,
    HWND hwndParent,
    LPCTSTR pszTitle,
    LPCTSTR pszText
    )
{
    DBGASSERT((NULL != pszTitle));
    DBGASSERT((NULL != pszText));

    //
    // Set these in member variables so that the text can be set in the
    // dialog in response to WM_INITDIALOG.
    //
    m_pszTitle = StringDup(pszTitle);
    m_pszText  = StringDup(pszText);

    return DialogBoxParam(hInstance,
                          MAKEINTRESOURCE(m_idDialogTemplate),
                          hwndParent,
                          DlgProc,
                          (LPARAM)this);
}



///////////////////////////////////////////////////////////////////////////////
/*  Function: YesNoToAllDialog::DlgProc [static]

    Description: Message procedure for the dialog.

    Arguments: Standard Win32 message proc arguments.

    Returns: Standard Win32 message proc return values.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK
YesNoToAllDialog::DlgProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    //
    // Retrieve the dialog object's ptr from the window's userdata.
    // Place there in response to WM_INITDIALOG.
    //
    YesNoToAllDialog *pThis = (YesNoToAllDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            //
            // Store "this" ptr in window's userdata.
            //
            SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)lParam);
            pThis = (YesNoToAllDialog *)lParam;

            //
            // Center popup on the desktop.
            //
            ::CenterPopupWindow(hwnd, GetDesktopWindow());
            pThis->m_hwndTxtMsg        = GetDlgItem(hwnd, IDC_TXT_YNTOALL);
            pThis->m_hwndCbxApplyToAll = GetDlgItem(hwnd, IDC_CBX_YNTOALL);
            SetWindowText(pThis->m_hwndTxtMsg, pThis->m_pszText);
            SetWindowText(hwnd, pThis->m_pszTitle);
            SendMessage(pThis->m_hwndCbxApplyToAll,
                        BM_SETCHECK,
                        pThis->m_bApplyToAll ? (WPARAM)BST_CHECKED : (WPARAM)BST_UNCHECKED,
                        0);

            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDCANCEL:
                case IDYES:
                case IDNO:
                    DBGASSERT((NULL != pThis));
                    pThis->m_bApplyToAll = (BST_CHECKED == SendMessage(pThis->m_hwndCbxApplyToAll, BM_GETCHECK, 0, 0));
                    EndDialog(hwnd, LOWORD(wParam));
                    break;
            }
            break;

    }
    return FALSE;
}

