#ifndef _INC_CSCVIEW_PROGDLG_H
#define _INC_CSCVIEW_PROGDLG_H
//////////////////////////////////////////////////////////////////////////////
/*  File: progdlg.h

    Description: Provides progress dialog used for viewer copy and deletion
        progress.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    01/16/98    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef _WINDOWS_
#   include <windows.h>
#endif

class CProgressDialog
{
    public:
        CProgressDialog(void) throw();
        ~CProgressDialog(void) throw();

        void Run(HINSTANCE hInstance, HWND hwndParent, int cMax) throw();
        void SetOperationText(LPCTSTR pszText) throw();
        void SetFilenameText(LPCTSTR pszText);
        void Advance(void) throw();
        void Cancel(void) throw();
        bool Cancelled(void) const throw()
            { return m_bCancelled; }
        HWND GetWindow(void) const throw()
            { return m_hwndDlg; }
        HINSTANCE GetModuleInstance(void) const throw()
            { return m_hInstance; }

    private:
        HINSTANCE m_hInstance;
        HWND m_hwndDlg;
        HWND m_hwndProgBar;
        int  m_cMax;
        bool m_bCancelled;

        static BOOL CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) throw();
        void OnInitDialog(HWND hwnd) throw();
        void OnDestroy(HWND hwnd) throw();
        void FlushMessages(void) throw();
};


#endif // _INC_CSCVIEW_PROGDLG_H
