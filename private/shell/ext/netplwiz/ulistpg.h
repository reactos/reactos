#ifndef ULISTPG_H_INCLUDED
#define ULISTPG_H_INCLUDED

#include "data.h"
#include "misc.h"

// All "add user to list" operations are done on the main UI thread - the
// filler thread posts this message to add a user
#define WM_ADDUSERTOLIST (WM_USER + 101)
// (LPARAM) CUserInfo* - the user to add to the listview
// (WPARAM) BOOL       - select this user (should always be 0 for now)

class CUserlistPropertyPage: public CPropertyPage
{
public:
    CUserlistPropertyPage(CUserManagerData* pdata): m_pData(pdata) 
        {m_himlLarge = NULL;}
    ~CUserlistPropertyPage();

    static HRESULT AddUserToListView(HWND hwndList, CUserInfo* pUserInfo, BOOL fSelectUser = FALSE);

private:
    // Dialog message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    BOOL OnGetInfoTip(HWND hwndList, LPNMLVGETINFOTIP pGetInfoTip);
    BOOL OnListViewItemChanged(HWND hwnd);
    BOOL OnListViewDeleteItem(HWND hwndList, int iItem);
    BOOL OnHelp(HWND hwnd, LPHELPINFO pHelpInfo);
    BOOL OnContextMenu(HWND hwnd);
    BOOL OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg);

    long OnApply(HWND hwnd);
 

private:
    // Functions
    HRESULT InitializeListView(HWND hwndList, BOOL fShowDomain);
    HRESULT LaunchNewUserWizard(HWND hwndParent);
    HRESULT LaunchAddNetUserWizard(HWND hwndParent);
    HRESULT LaunchUserProperties(HWND hwndParent);
    HRESULT LaunchSetPasswordDialog(HWND hwndParent);
    CUserInfo* GetSelectedUserInfo(HWND hwndList);
    void OnRemove(HWND hwnd);
    int ConfirmRemove(HWND hwnd, CUserInfo* pUserInfo);
    void RemoveSelectedUserFromList(HWND hwndList, BOOL fFreeUserInfo);
    void SetPageState(HWND hwnd);
    HRESULT SetAutologonState(HWND hwnd, BOOL fAutologon);
    void SetupList(HWND hwnd);
    HPSXA AddExtraUserPropPages(ADDPROPSHEETDATA* ppsd, PSID psid);
    
    static int CALLBACK ListCompare(LPARAM lParam1, LPARAM lParam2, 
	    LPARAM lParamSort);

private:
    // Data
    CUserManagerData* m_pData;
    HIMAGELIST m_himlLarge;

    // When a column header is clicked, the list is sorted based on that column
    // When this happens, we store the column number here so that if the same
    // column is clicked again, we sort it in reverse. A 0 is stored if no
    // column should be sorted in reverse when clicked.
    int m_iReverseColumnIfSelected;

    BOOL m_fAutologonCheckChanged;
};

#endif //!ULISTPG_H_INCLUDED
