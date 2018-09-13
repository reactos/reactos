#ifndef FTPROP_H
#define FTPROP_H

#include "ftdlg.h"

class CFTPropDlg : public CFTDlg
{
public:
    CFTPropDlg(BOOL fAutoDelete = FALSE);
    ~CFTPropDlg();

///////////////////////////////////////////////////////////////////////////////
//  Implementation
private:
// Message handlers
    LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

    // Dialog messages
    LRESULT OnCommand(WPARAM wParam, LPARAM lParam);
    LRESULT OnInitDialog(WPARAM wParam, LPARAM lParam);
    LRESULT OnFinishInitDialog();
    LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
    LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);

    // Misc
    LRESULT OnCtlColorStatic(WPARAM wParam, LPARAM lParam);

    // Control specific
        // ListView
    LRESULT OnNotifyListView(UINT uCode, LPNMHDR pNMHDR);
    LRESULT OnListViewSelItem(int iItem, LPARAM lParam);
    LRESULT OnListViewColumnClick(int iCol);
        // New, Remove, Edit buttons    
    LRESULT OnNewButton(WORD wNotif);
    LRESULT OnDeleteButton(WORD wNotif);
    LRESULT OnRemoveButton(WORD wNotif);
    LRESULT OnEditButton(WORD wNotif);
    LRESULT OnAdvancedButton(WORD wNotif);
    LRESULT OnChangeButton(WORD wNotif);


// Misc
    BOOL _GetListViewSelectedItem(UINT uMask, UINT uStateMask, LVITEM* plvItem);
// Member variables
private:
    HIMAGELIST          _hImageList;
    BOOL                _fPerUserAdvButton;

    BOOL                _fStopThread;
    HANDLE              _hThread;

    BOOL                _fUpdateImageAgain;

    // Optimization
    int                 _iLVSel;
///////////////////////////////////////////////////////////////////////////////
//  Helpers
private:
    // General
    inline HWND _GetLVHWND();
    // Lower pane
    HRESULT _UpdateProgIDButtons(LPTSTR pszExt, LPTSTR pszProgID);
    HRESULT _UpdateGroupBox(LPTSTR pszExt, BOOL fExt);
    HRESULT _UpdateDeleteButton(BOOL fExt);
    HRESULT _UpdateOpensWith(LPTSTR pszExt, LPTSTR pszProgID);
    HRESULT _UpdateAdvancedText(LPTSTR pszExt, LPTSTR pszFileType, BOOL fExt);
    HRESULT _EnableLowerPane(BOOL fEnable = TRUE);
    // ListView
    HRESULT _InitListView();
    HRESULT _FillListView();
    HRESULT _SelectListViewItem(int i);
    HRESULT _DeleteListViewItem(int i);
    void _UpdateListViewItem(LVITEM* plvItem);

    HRESULT _InitPreFillListView();
    HRESULT _InitPostFillListView();

    DWORD _UpdateAllListViewItemImages();

    void _SetAdvancedRestoreButtonHelpID(DWORD dwID);

    int _GetNextNAItemPos(int iFirstNAItem, int cNAItem, LPTSTR pszProgIDDescr);

    static DWORD WINAPI _UpdateAllListViewItemImagesWrapper(LPVOID lpParameter);
    static DWORD WINAPI _FillListViewWrapper(LPVOID lpParameter);

    int _InsertListViewItem(int iItem, LPTSTR pszExt, LPTSTR pszProgIDDescr, LPTSTR pszProgID = NULL);

    BOOL _ShouldEnableButtons();
};

#endif //FTPROP_H