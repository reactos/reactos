#ifndef FTADVDLG
#define FTADVDLG

#include "ftdlg.h"
#include "ftcmmn.h"

class CFTAdvDlg : public CFTDlg
{
public:
    CFTAdvDlg(LPTSTR pszProgID, BOOL fAutoDelete = FALSE);
    ~CFTAdvDlg();

///////////////////////////////////////////////////////////////////////////////
//  Implementation
private:

    LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
// Message handlers
    // Dialog messages
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);

    LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);
    LRESULT OnDrawItem(WPARAM wParam, LPARAM lParam);
    LRESULT OnMeasureItem(WPARAM wParam, LPARAM lParam);

    LRESULT OnOK(WORD wNotif);
    LRESULT OnCancel(WORD wNotif);

    // Control specific
    //
    //      Action buttons
    LRESULT OnNewButton(WORD wNotif);
    LRESULT OnEditButton(WORD wNotif);
    LRESULT OnChangeIcon(WORD wNotif);
    LRESULT OnSetDefault(WORD wNotif);
    LRESULT OnRemoveButton(WORD wNotif);
    //      ListView
    LRESULT OnNotifyListView(UINT uCode, LPNMHDR pNMHDR);
    LRESULT OnListViewSelItem(int iItem, LPARAM lParam);

private:
// Member variables
    TCHAR       _szProgID[MAX_PROGID];

    HICON       _hIcon;
    int         _iOriginalIcon;
    int         _iNewIcon;

    HFONT       _hfontReg;
    HFONT       _hfontBold;
    int         _iDefaultAction;
    int         _iLVSel;

    HDPA        _hdpaActions;
    HDPA        _hdpaRemovedActions;

    TCHAR       _szIconLoc[MAX_ICONLOCATION];

    HANDLE      _hHeapProgID;

///////////////////////////////////////////////////////////////////////////////
//  Helpers
    inline HWND _GetLVHWND();

    HRESULT _FillListView();
    HRESULT _FillProgIDDescrCombo();

    HRESULT _InitDefaultActionFont();
    HRESULT _InitListView();
    HRESULT _InitDefaultAction();
    HRESULT _InitChangeIconButton();
    HRESULT _InitDescription();

    HRESULT _SetDocIcon(int iIndex = -1);
    HRESULT _SelectListViewItem(int i);
    HRESULT _SetDefaultAction(int iIndex);

    HRESULT _UpdateActionButtons();
    HRESULT _UpdateCheckBoxes();

    // PROGIDACTION helpers
    HRESULT _RemovePROGIDACTION(PROGIDACTION* pPIDA);
    HRESULT _CreatePROGIDACTION(PROGIDACTION** ppPIDA);
    HRESULT _CopyPROGIDACTION(PROGIDACTION* pPIDADest, PROGIDACTION* pPIDASrc);
    HRESULT _GetPROGIDACTION(LPTSTR pszAction, PROGIDACTION** ppPIDA);
    HRESULT _AppendPROGIDACTION(PROGIDACTION* pPIDA);
    HRESULT _FillPROGIDACTION(PROGIDACTION* pPIDA, LPTSTR pszAction);
    void _DeletePROGIDACTION(PROGIDACTION* pPIDA);
    BOOL _IsNewPROGIDACTION(LPTSTR pszAction);

    BOOL _GetListViewSelectedItem(UINT uMask, UINT uStateMask, LVITEM* plvItem);
    int _InsertListViewItem(int iItem, LPTSTR pszAction);
    BOOL _IsDefaultAction(LPTSTR pszAction);
    void _CleanupProgIDs();
    LPTSTR _AddProgID(LPTSTR pszProgID);
    void _CheckDefaultAction();

    BOOL _CheckForDuplicateEditAction(LPTSTR pszActionOriginal, LPTSTR pszAction);
    BOOL _CheckForDuplicateNewAction(LPTSTR pszAction);
};

#endif //FTADVDLG