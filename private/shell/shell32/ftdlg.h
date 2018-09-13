#ifndef FTDLG_H
#define FTDLG_H

class IAssocStore;

class CFTDlg
{
public:
    CFTDlg(ULONG_PTR ulpAHelpIDsArray, BOOL fAutoDelete);
    virtual ~CFTDlg();
public:
    INT_PTR DoModal(HINSTANCE hinst, LPTSTR pszResource, HWND hwndParent);
protected:
    virtual LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    virtual LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam) = 0;
    virtual LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);

    virtual LRESULT OnOK(WORD wNotif);
    virtual LRESULT OnCancel(WORD wNotif);

    LRESULT OnCtrlSetFocus(WPARAM wParam, LPARAM lParam);

    virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    LRESULT DefWndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT DefWndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                               LPARAM lParam);

    ULONG_PTR GetHelpIDsArray();

// Misc
    void SetHWND(HWND hwnd) { _hwnd = hwnd; }
    void ResetHWND() { _hwnd = NULL; }
    HRESULT _InitAssocStore();

    static void MakeDefaultProgIDDescrFromExt(LPTSTR pszProgIDDescr, DWORD dwProgIDDescr,
        LPTSTR pszExt);
protected:
    HWND            _hwnd;
    HCURSOR         _hcursorWait;
    HCURSOR         _hcursorOld;

    BOOL            _fAutoDelete;

    ULONG_PTR       _rgdwHelpIDsArray;
    // Our connection to the data
    IAssocStore*    _pAssocStore;
public:
    static BOOL_PTR CALLBACK FTDlgWndProc(HWND hwnd, UINT uMsg, 
        WPARAM wParam, LPARAM lParam);
};

#endif //FTDLG_H