#ifndef _INC_CSCVIEW_DBGDLGS_H
#define _INC_CSCVIEW_DBGDLGS_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef _INC_CSCVIEW_CNXCACHE_H
#   include "cnxcache.h"
#endif

#ifndef _INC_CSCVIEW_STRCLASS_H
#   include "strclass.h"
#endif

class ShareDbgDialog
{
    public:
        ShareDbgDialog(LPCTSTR pszShare, const CnxNameCache& cnc);

        void Run(HINSTANCE hInstance, HWND hwndParent);

    private:
        HINSTANCE     m_hInstance;
        CString       m_strShare;
        const CnxNameCache& m_CnxNameCache;

        static BOOL CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        void OnInitDialog(HWND hwnd);
        void Refresh(HWND hwnd);

};


class FileDbgDialog
{
    public:
        FileDbgDialog(LPCTSTR pszFile, bool bIsDirectory);

        void Run(HINSTANCE hInstance, HWND hwndParent);

    private:
        HINSTANCE m_hInstance;
        CString   m_strFile;
        bool      m_bIsDirectory;

        static BOOL CALLBACK DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        void OnInitDialog(HWND hwnd);
        void Refresh(HWND hwnd);
};

#endif // _INC_CSCVIEW_DBGDLGS_H
