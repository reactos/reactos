#ifndef _PATTERN_H_
#define _PATTERN_H_

BOOL CALLBACK PatternDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

class CPattern
{
public:
protected:
    HWND _hwnd;
    HWND _hwndLB;
    HWND _hwndSample;
    TCHAR _szCurPattern[MAX_PATH];

    CPattern(void);

    BOOL _IsProbablyAValidPattern(LPCTSTR pszPat);
    HBRUSH _WordsToBrush(WORD *pwBits);
    void _GetPattern(LPTSTR pszPattern, int cchPattern);
    void _OnInitDialog(HWND hwnd);
    void _OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
    void _OnPaint(void);
    void _EnableControls(void);

    friend BOOL CALLBACK PatternDlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
