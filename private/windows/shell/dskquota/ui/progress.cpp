///////////////////////////////////////////////////////////////////////////////
/*  File: progress.cpp

    Description: Implements the various flavors of progress dialogs used
        in the quota UI.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include "progress.h"
#include "resource.h"


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::SendToProgressBar [inline]

    Description: Inline function that sends a message to the dialog's
        progress bar control.  If there is no progress bar control, the
        function returns FALSE.

    Arguments: Standard Win32 message arguments.

    Returns:
        If progress bar window exists, returns the result of SendMessage.
        Otherwise, returns FALSE.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
inline INT_PTR
ProgressDialog::SendToProgressBar(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (NULL != m_hwndProgressBar)
        return SendMessage(m_hwndProgressBar, uMsg, wParam, lParam);
    else
        return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::ProgressDialog

    Description: Class constructor for progress dialog base class.

    Arguments:
        idDialogTemplate - ID number for the dialog's resource template.

        idProgressBar - ID number for the progress bar control.

        idTxtDescription - ID number for text description in dialog.

        idTxtFileName - ID number for file name field in dialog.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ProgressDialog::ProgressDialog(
    UINT idDialogTemplate,
    UINT idProgressBar,
    UINT idTxtDescription,
    UINT idTxtFileName
    ) : m_idDialogTemplate(idDialogTemplate),
        m_idProgressBar(idProgressBar),
        m_idTxtDescription(idTxtDescription),
        m_idTxtFileName(idTxtFileName),
        m_hWnd(NULL),
        m_bUserCancelled(FALSE)
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("ProgressDialog::ProgressDialog")));
    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("\tthis = 0x%08X"), this));
    //
    // Do nothing.
    //
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::~ProgressDialog

    Description: Class destructor for progress dialog base class.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
ProgressDialog::~ProgressDialog(
    VOID
    )
{
    DBGTRACE((DM_VIEW, DL_HIGH, TEXT("ProgressDialog::~ProgressDialog")));
    DBGPRINT((DM_VIEW, DL_HIGH, TEXT("\tthis = 0x%08X"), this));
    //
    // Call the Destroy() function to destroy the progress dialog window.
    //
    Destroy();
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::Create

    Description: Creates the dialog.

    Arguments:
        hInstance - Instance handle for the DLL containing the dialog
            resource template.

        hwndParent - Parent window for dialog.

    Returns:
        TRUE  = Dialog was created.
        FALSE = Dialog was not created.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
ProgressDialog::Create(
    HINSTANCE hInstance,
    HWND hwndParent
    )
{
    m_hWnd = CreateDialogParam(hInstance,
                               MAKEINTRESOURCE(m_idDialogTemplate),
                               hwndParent,
                               DlgProc,
                               (LPARAM)this);
    if (NULL != m_hWnd)
    {
        m_hwndProgressBar = GetDlgItem(m_hWnd, m_idProgressBar);
        DBGASSERT((NULL != m_hwndProgressBar));
    }
    return (NULL != m_hWnd);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::Destroy

    Description: Destroys the dialog window.

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ProgressDialog::Destroy(
    VOID
    )
{
    //
    // Note that m_hWnd is set to NULL in OnDestroy().
    //
    if (NULL != m_hWnd)
        DestroyWindow(m_hWnd);
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::DlgProc [static]

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
ProgressDialog::DlgProc(
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
    ProgressDialog *pThis = (ProgressDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            //
            // Store "this" ptr in window's userdata.
            //
            SetWindowLongPtr(hwnd, DWLP_USER, (INT_PTR)lParam);
            pThis = (ProgressDialog *)lParam;
            //
            // Description text control is hidden by default.
            // Calling SetDescription() will show it.
            //
            ShowWindow(GetDlgItem(hwnd, pThis->m_idTxtDescription), SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, pThis->m_idTxtFileName),    SW_HIDE);
            //
            // Center dialog in it's parent.
            //
            ::CenterPopupWindow(hwnd);
            //
            // Let derived classes respond to WM_INITDIALOG.
            //
            return pThis->OnInitDialog(hwnd, wParam, lParam);

        case WM_DESTROY:
            //
            // Let derived classes respond to WM_DESTROY.
            //
            return pThis->OnDestroy(hwnd);

        case WM_COMMAND:
            if (IDCANCEL == LOWORD(wParam))
            {
                //
                // User pressed "Cancel" button.
                // Set the "User cancelled" flag and let derived
                // classes respond to the cancellation.
                //
                pThis->m_bUserCancelled = TRUE;
                return pThis->OnCancel(hwnd, wParam, lParam);
            }
            break;

    }
    if (NULL != pThis)
    {
        //
        // Let derived classes respond to any message if they wish.
        // Note that only WM_INITDIALOG, WM_DESTROY and the "user cancelled"
        // events are the only special cases.
        //
        return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
    }
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::HandleMessage

    Description: Base class implementation of virtual function.  Derived
        classes can provide an implementation to handle any message other than
        WM_INITDIALOG or WM_DESTROY.  These two messages have their own
        virtual message handlers.

    Arguments: Standard Win32 message proc arguments.

    Returns: Always returns FALSE.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
ProgressDialog::HandleMessage(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::OnInitDialog

    Description: Base class implementation of virtual function.  Called
        when the base class receives WM_INITDIALOG.  Derived classes can
        provide an implementation if they wish to perform some operation
        in response to WM_INITDIALOG.

    Arguments:
        hwnd - Dialog window handle.

        wParam, lParam - Standard Win32 message proc arguments.

    Returns: Always returns TRUE so that USER will set the control focus.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
ProgressDialog::OnInitDialog(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::OnDestroy

    Description: Base class implementation of virtual function.  Called
        when the base class receives WM_DESTROY.  Derived classes can
        provide an implementation if they wish to perform some operation
        in response to WM_DESTROY.  Before returning, any derived implementation
        must call the base class implementation so that m_hWnd is properly
        set to NULL.

    Arguments:
        hwnd - Dialog window handle.

    Returns: Always returns FALSE.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
ProgressDialog::OnDestroy(
    HWND hwnd
    )
{
    m_hWnd = NULL;
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::OnCancel

    Description: Base class implementation of virtual function.  Called
        when the user presses the "Cancel" button in the dialog.  This
        implementation assumes that the Cancel button is assigned the ID
        of IDCANCEL (standard).

    Arguments:
        hwnd - Dialog window handle.

        wParam, lParam - Standard Win32 message proc arguments.

    Returns: Always returns FALSE.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
INT_PTR
ProgressDialog::OnCancel(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::FlushMessages

    Description: While the dialog is active, call this periodically so that
        the thread is able to properly update the dialog and respond to the
        user pressing "Cancel".

    Arguments: None.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ProgressDialog::FlushMessages(
    VOID
    )
{
    if (NULL != m_hWnd)
    {
        //
        // Process messages for the dialog's parent and all of it's children.
        //
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) &&
               WM_QUIT != msg.message)
        {
            GetMessage(&msg, NULL, 0, 0);
            if (!IsDialogMessage(m_hWnd, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::SetTitle

    Description: Sets the title string in the dialog.

    Arguments:
        pszTitle - Address of title string.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ProgressDialog::SetTitle(
    LPCTSTR pszTitle
    )
{
    DBGASSERT((NULL != pszTitle));
    if (NULL != m_hWnd)
    {
        if (0 == ((DWORD_PTR)pszTitle & ~0xffff))
        {
            TCHAR szText[MAX_PATH] = { TEXT('\0') };
            LoadString(g_hInstDll, (DWORD)((DWORD_PTR)pszTitle), szText, ARRAYSIZE(szText));
            pszTitle = szText;
        }
        SetWindowText(m_hWnd, pszTitle);
        FlushMessages();
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::SetDescription

    Description: Sets the progress description string in the dialog.

    Arguments:
        pszDescription - Address of description string.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ProgressDialog::SetDescription(
    LPCTSTR pszDescription
    )
{
    DBGASSERT((NULL != pszDescription));
    if (NULL != m_hWnd)
    {
        if (0 == ((DWORD_PTR)pszDescription & ~0xffff))
        {
            TCHAR szText[MAX_PATH] = { TEXT('\0') };
            LoadString(g_hInstDll, (DWORD)((DWORD_PTR)pszDescription), szText, ARRAYSIZE(szText));
            pszDescription = szText;
        }
        SetWindowText(GetDlgItem(m_hWnd, m_idTxtDescription), pszDescription);
        //
        // Description control is by default hidden.
        //
        ShowWindow(GetDlgItem(m_hWnd, m_idTxtDescription), SW_NORMAL);
        FlushMessages();
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::SetFileName

    Description: Sets the file name description string in the dialog.
        If the file name is too long, it is formatted with ellipses to
        fit in the space provided.

    Arguments:
        pszFileName - Address of file name string.

    Returns: Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ProgressDialog::SetFileName(
    LPCTSTR pszFileName
    )
{
    DBGASSERT((NULL != pszFileName));
    if (NULL != m_hWnd)
    {
        HWND hwndCtl   = GetDlgItem(m_hWnd, m_idTxtFileName);
        HDC hdc        = GetDC(hwndCtl);
        RECT rc;
        LPTSTR pszText = StringDup(pszFileName);

        if (NULL != pszText)
        {
            GetClientRect(hwndCtl, &rc);

            DrawText(hdc,
                     pszText,
                     -1,
                     &rc,
                     DT_CENTER |
                     DT_PATH_ELLIPSIS |
                     DT_MODIFYSTRING);

            SetWindowText(hwndCtl, pszText);
            delete[] pszText;

            //
            // FileName control is by default hidden.
            //
            ShowWindow(hwndCtl, SW_NORMAL);
        }
        FlushMessages();
        ReleaseDC(hwndCtl, hdc);
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::ProgressBarInit

    Description: Initializes the progress bar control with range and step
        values.  If this function is not called, the progress bar defaults
        to:
            iMin  =   0
            iMax  = 100
            iStep =  10

    Arguments:
        iMin - Minimum range value.

        iMax - Maximum range value.

        iStep - Amount bar advances each time PBM_STEPIT is received.

    Returns:
        TRUE  = Progress bar control accepted settings.
        FALSE = Progress bar rejected settings.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
BOOL
ProgressDialog::ProgressBarInit(
    UINT iMin,
    UINT iMax,
    UINT iStep
    )
{
    BOOL bResult = FALSE;

    if (0 != SendToProgressBar(PBM_SETSTEP,  iStep, 0))
        bResult = (0 != SendToProgressBar(PBM_SETRANGE, 0, MAKELPARAM(iMin, iMax)));

    FlushMessages();
    return bResult;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::ProgressBarReset

    Description: Resets the progres bar position at 0.

    Arguments: None.

    Returns:  Previous "position" of progress bar.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT
ProgressDialog::ProgressBarReset(
    VOID
    )
{
    UINT iReturn = (UINT)ProgressBarSetPosition(0);
    FlushMessages();
    return iReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::ProgressBarAdvance

    Description: Advances the progress bar a given number of counts.

    Arguments:
        iDelta - Number of counts to advance.  If -1, the bar is advanced
            by the step value supplied in ProgressBarInit.

    Returns:  Previous "position" of progress bar.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT
ProgressDialog::ProgressBarAdvance(
    UINT iDelta
    )
{
    UINT iReturn;
    if ((UINT)-1 == iDelta)
        iReturn = (UINT)SendToProgressBar(PBM_STEPIT, 0, 0);
    else
        iReturn = (UINT)SendToProgressBar(PBM_DELTAPOS, (WPARAM)iDelta, 0);

    FlushMessages();

    return iReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::ProgressBarSetPosition

    Description: Advances the progress bar to a specific position.

    Arguments:
        iPosition - Specific position count.

    Returns:  Previous "position" of progress bar.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
UINT
ProgressDialog::ProgressBarSetPosition(
    UINT iPosition
    )
{
    UINT iReturn = (UINT)SendToProgressBar(PBM_SETPOS, (WPARAM)iPosition, 0);
    FlushMessages();
    return iReturn;
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::Show

    Description: Makes the progress dialog visible.

    Arguments: None.

    Returns:  Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ProgressDialog::Show(
    VOID
    )
{
    if (NULL != m_hWnd)
    {
        ShowWindow(m_hWnd, SW_SHOWNORMAL);
        FlushMessages();
    }
}


///////////////////////////////////////////////////////////////////////////////
/*  Function: ProgressDialog::Hide

    Description: Hides the progress dialog.

    Arguments: None.

    Returns:  Nothing.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
VOID
ProgressDialog::Hide(
    VOID
    )
{
    if (NULL != m_hWnd)
    {
        ShowWindow(m_hWnd, SW_HIDE);
        FlushMessages();
    }
}
