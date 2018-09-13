#ifndef GRPPAGE_H
#define GRPPAGE_H

#include "data.h"

#include "userinfo.h"

class CGroupPageBase
{

public:
    // Public interface (in the case where you're not using a
    // derived class
    CGroupPageBase(CUserInfo* pUserInfo, CDPA<CGroupInfo>* pGroupList):
         m_pUserInfo(pUserInfo), 
         m_pGroupList(pGroupList),
         m_hBoldFont(NULL) 
         {}

    ~CGroupPageBase() 
    {
        if (NULL != m_hBoldFont)
            DeleteObject((HGDIOBJ) m_hBoldFont);
    }

    INT_PTR HandleGroupMessage(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL GetSelectedGroup(HWND hwnd, LPTSTR pszGroupOut, DWORD cchGroup, CUserInfo::GROUPPSEUDONYM* pgsOut);

protected:

    void InitializeLocalGroupCombo(HWND hwndCombo);
    void SetGroupDescription(HWND hwndCombo, HWND hwndEdit);
    void BoldGroupNames(HWND hwnd);
    void SelectGroup(HWND hwnd, LPCTSTR pszSelect);
    UINT RadioIdForGroup(LPCTSTR pszGroup);

protected:
    // Message handlers
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    BOOL OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void OnRadioChanged(HWND hwnd, UINT idRadio);

protected:
    // Data
    CUserInfo* m_pUserInfo;
    CDPA<CGroupInfo>* m_pGroupList;
    HFONT m_hBoldFont;
};

class CGroupWizardPage: public CPropertyPage, public CGroupPageBase
{
public:
    CGroupWizardPage(CUserInfo* pUserInfo, 
        CDPA<CGroupInfo>* pGroupList): 
        CGroupPageBase(pUserInfo, pGroupList) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
};

class CGroupPropertyPage: public CPropertyPage, public CGroupPageBase
{
public:
    CGroupPropertyPage(CUserInfo* pUserInfo,
        CDPA<CGroupInfo>* pGroupList): 
        CGroupPageBase(pUserInfo, pGroupList) {}

protected:
    // Message handlers
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh);
};

#endif //!GRPPAGE_H
