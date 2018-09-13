#ifndef _INC_CSCVIEW_COPYDLGS_H
#define _INC_CSCVIEW_COPYDLGS_H
//////////////////////////////////////////////////////////////////////////////
/*  File: copydlgs.h

    Description: Provides dialogs used in the "Copy To Folder" operation.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    01/16/98    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_PROGDLG_H
#   include "progdlg.h"
#endif

class CopyProgressDialog : public CProgressDialog
{
    public:
        CopyProgressDialog(void) throw();

        void UpdateStatusText(LPCTSTR pszName, bool bIsFolder);
    private:
        int  m_iCopyOp;    // 0 = copying file, 1 = creating folder.
};



class ConfirmCopyOverDialog
{
    public:
        ConfirmCopyOverDialog(void) throw();
        ~ConfirmCopyOverDialog(void) throw();

        int Run(HINSTANCE hInstance, HWND hwndParent, LPCTSTR pszFile);
    private:
        HINSTANCE m_hInstance;
        HWND      m_hwnd;
        CString   m_strFile;

        static BOOL CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        void OnInitDialog(HWND hwnd);
};



#endif // _INC_CSCVIEW_COPYDLGS_H
