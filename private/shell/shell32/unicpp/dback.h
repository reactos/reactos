#ifndef _DBACK_H_
#define _DBACK_H_

class CBackPropSheetPage : public PROPSHEETPAGE
{
public:
    CBackPropSheetPage(void);

protected:
    HWND _hwnd;
    HWND _hwndLV;
    HWND _hwndWPStyle;
    BOOL _fAllowHtml;
    BOOL _fAllowAD;
    BOOL _fAllowChanges;
    BOOL _fPolicyForWallpaper;  //Is there a policy for wallpaper?
    BOOL _fPolicyForStyle;      //Is there a policy for Wallpaper style?
    BOOL _fForceAD;             //Is there a policy to force Active desktop to be ON?

    static BOOL_PTR CALLBACK _DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void _AddPicturesFromDir(LPCTSTR pszDirName);
    int _AddAFileToLV(LPCTSTR pszDir, LPTSTR pszFile, UINT nBitmap);
    void _AddFilesToLV(LPCTSTR pszDir, LPCTSTR pszSpec, UINT nBitmap);
    int _FindWallpaper(LPCTSTR pszFile);
    void _SetNewWallpaper(LPCTSTR pszFile);
    void _UpdatePreview(WPARAM flags);
    void _EnableControls(void);
    int _GetImageIndex(LPCTSTR pszFile);
    static int CALLBACK _SortBackgrounds(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

    void _OnInitDialog(HWND hwnd);
    void _OnNotify(LPNMHDR lpnm);
    void _OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
    void _OnDestroy(void);
};

#endif
