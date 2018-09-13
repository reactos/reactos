#ifndef _INC_DSKQUOTA_YNTOALL_H
#define _INC_DSKQUOTA_YNTOALL_H
///////////////////////////////////////////////////////////////////////////////
/*  File: yntoall.h

    Description: Declarations for class YesNoToAllDialog.
        This class provides a simple message box that includes an
        "apply to all" checkbox.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
class YesNoToAllDialog
{
    private:
        UINT   m_idDialogTemplate;
        HWND   m_hwndCbxApplyToAll;
        HWND   m_hwndTxtMsg;
        LPTSTR m_pszTitle;
        LPTSTR m_pszText;
        BOOL   m_bApplyToAll;

        static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);

    public:
        YesNoToAllDialog(UINT idDialogTemplate);
        ~YesNoToAllDialog(VOID);

        BOOL ApplyToAll(VOID)
            { return m_bApplyToAll; }

        INT_PTR CreateAndRun(HINSTANCE hInstance, HWND hwndParent, LPCTSTR pszTitle, LPCTSTR pszText);
};

#endif // _INC_DSKQUOTA_YNTOALL_H


