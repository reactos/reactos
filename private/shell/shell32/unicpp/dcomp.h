#ifndef _DCOMP_H_
#define _DCOMP_H_

class CCompPropSheetPage : public PROPSHEETPAGE
{
public:
    CCompPropSheetPage(void);

protected:
    HWND _hwnd;
    HWND _hwndLV;
    BOOL _fAllowAdd;
    BOOL _fAllowDel;
    BOOL _fAllowEdit;
    BOOL _fAllowClose;
    BOOL _fAllowReset;
    BOOL _fForceAD;
    BOOL _fLaunchGallery;

    int  _iPreviousSelection;

    static BOOL_PTR CALLBACK _DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void _ConstructLVString(COMPONENTA *pcomp, LPTSTR pszBuf, DWORD cchBuf);
    void _AddComponentToLV(COMPONENTA *pcomp);
    void _SetUIFromDeskState(BOOL fEmpty);
    void _OnInitDialog(HWND hwnd);
    void _OnNotify(LPNMHDR lpnm);
    void _OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
    void _OnDestroy();
    void _OnGetCurSel(int *piIndex);
    void _EnableControls(void);
    BOOL _VerifyFolderOptions(void);
    void _SelectComponent(LPWSTR pwszUrl);
    
    void _NewComponent(void);
    void _EditComponent(void);
    void _DeleteComponent(void);
    void _TryIt(void);
};

BOOL FindComponent(LPCTSTR pszUrl);
void CreateComponent(COMPONENTA *pcomp, LPCTSTR pszUrl);
INT_PTR NewComponent(HWND hwndOwner, IActiveDesktop * pad, BOOL fDeferGallery, COMPONENT * pcomp);
BOOL LooksLikeFile(LPCTSTR psz);
BOOL IsUrlPicture(LPCTSTR pszUrl);

#define WM_COMP_GETCURSEL    (WM_USER+1)

#endif
