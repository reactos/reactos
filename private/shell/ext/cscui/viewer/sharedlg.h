#ifndef _INC_CSCVIEW_SHAREDLG_H
#define _INC_CSCVIEW_SHAREDLG_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_CNXCACHE_H
#   include "cnxcache.h"
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

class SharePropSheet
{
    public:
        SharePropSheet(HINSTANCE hInstance,
                       LONG *pDllRefCnt,
                       HWND hwndParent,
                       LPCTSTR pszShare);

        ~SharePropSheet(void);

        DWORD Run(void);

    private:
        //
        // Increase this if more pages are required.
        // Currently, we only need the "General" page and
        // the "Settings" page.  The "General" page is implemented
        // in this module while the "Settings" page is implemented
        // in options.cpp.
        //
        enum { MAXPAGES = 2 };

        HINSTANCE m_hInstance;
        LONG     *m_pDllRefCnt;
        HWND      m_hwndParent;
        CString   m_strShare;
        static const DWORD  m_rgHelpIDs[];

        void OnInitDialog(HWND hwnd);
        static BOOL CALLBACK GenPgDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        static BOOL CALLBACK AddPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);
};

#endif // _INC_CSCVIEW_SHAREDLG_H
