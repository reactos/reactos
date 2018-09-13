/*****************************************************************************\
    FILE: ftpdhlp.h

    DESCRIPTION:
        Dialog box helper.  This class will fill in the parts of the dialog
    that pertain to the FTP information
\*****************************************************************************/

#ifndef _FTPDIALOGTEMPLATE_H
#define _FTPDIALOGTEMPLATE_H


/*****************************************************************************\
    CLASS: CFtpDialogTemplate

    DESCRIPTION:
        Dialog box helper.  This class will fill in the parts of the dialog
    that pertain to the FTP information
\*****************************************************************************/
class CFtpDialogTemplate
{
public:
    HRESULT InitDialog(HWND hdlg, BOOL fEditable, UINT id, CFtpFolder * pff, CFtpPidlList * pfpl);
    HRESULT InitDialogWithFindData(HWND hDlg, UINT id, CFtpFolder * pff, const FTP_FIND_DATA * pwfd, LPCWIRESTR pwWirePath, LPCWSTR pwzDisplayPath);
    BOOL OnClose(HWND hdlg, HWND hwndBrowser, CFtpFolder * pff, CFtpPidlList * pfpl);
    BOOL HasNameChanged(HWND hdlg, CFtpFolder * pff, CFtpPidlList * pPidlList);

    static int _InitSizeTally(LPVOID pvPidl, LPVOID pvSizeHolder);

private:
    HRESULT _ReinsertDlgText(HWND hwnd, LPCVOID pv, LPCTSTR ptszFormat);
    HRESULT _ReplaceIcon(HWND hwnd, HICON hicon);
    HRESULT _InitIcon(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl);
    HRESULT _InitNameEditable(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl);
    HRESULT _InitName(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl);
    HRESULT _InitType(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl);
    HRESULT _InitLocation(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl);
    HRESULT _InitSize(HWND hwnd, HWND hwndLabel, CFtpFolder * pff, CFtpPidlList * pflHfpl);
    HRESULT _InitTime(HWND hwnd, HWND hwndLabel, CFtpFolder * pff, CFtpPidlList * pflHfpl);
    HRESULT _InitCount(HWND hwnd, CFtpFolder * pff, CFtpPidlList * pflHfpl);

    BOOL m_fEditable;
};



#endif // _FTPDIALOGTEMPLATE_H
