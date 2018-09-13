#ifndef BROWSE_H_INCLUDED
#define BROWSE_H_INCLUDED

#include "dialog.h"

// CNoDsBrowseDialog
//  Invoked to find computers with shares on a network, even without a DS
//  Used when the computer is not a member of an NT5 domain or anytime a
//  DS isn't available

class CNoDsBrowseDialog: public CDialog
{
public:
    CNoDsBrowseDialog(NETPLACESDATA* pdata, TCHAR* pszBuffer, int cchBuffer): 
      m_pdata(pdata), m_pszBuffer(pszBuffer), m_cchBuffer(cchBuffer)
      {}
        

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnDestroy(HWND hwnd);
    BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg);

    // Helpers
    BOOL AddServerNamesToList(HWND hwndList);
    void EnableOKButton(HWND hwnd, BOOL fEnable);
    void OnOK(HWND hwnd, int iItem);

    static DWORD WINAPI AddServerNamesThread(LPVOID lpParam);

private:
    TCHAR* m_pszBuffer;
    int m_cchBuffer;
    NETPLACESDATA* m_pdata;
    
    // Some data needed by the 'Add server names to listview' thread
    HWND m_hwndList;
    BOOL m_fShowWaitCursor;
};


#endif //!BROWSE_H_INCLUDED
