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

    static BOOL CALLBACK _DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
