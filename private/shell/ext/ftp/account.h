/*****************************************************************************
 *	account.h
 *****************************************************************************/

#ifndef _ACCOUNT_H
#define _ACCOUNT_H


/*****************************************************************************\
  CLASS: CAccounts
\*****************************************************************************/

#define LOGINFLAGS_DEFAULT                  0x00000000  // Default to Anonymous when dialog is displayed
#define LOGINFLAGS_ANON_ISDEFAULT           0x00000001  // Default to Anonymous when dialog is displayed
#define LOGINFLAGS_ANON_LOGINJUSTFAILED     0x00000002  // The attempt to login anonymously just failed
#define LOGINFLAGS_USER_LOGINJUSTFAILED     0x00000004  // The attempt to login as a user just failed


class CAccounts
{
public:
    CAccounts();
    ~CAccounts();

    // Public Member Functions
    HRESULT DisplayLoginDialog(HWND hwnd, DWORD dwLoginFlags, LPCTSTR pszServer, LPTSTR pszUserName, DWORD cchUserNameSize, LPTSTR pszPassword, DWORD cchPasswordSize);

    HRESULT GetUserName(LPCTSTR pszServer, LPTSTR pszUserName, DWORD cchUserName);
    HRESULT GetPassword(LPCTSTR pszServer, LPCTSTR pszUserName, LPTSTR pszPassword, DWORD cchPassword);

protected:
    // Private Member Functions
    HRESULT _GetAccountKey(LPCTSTR pszServer, LPTSTR pszKey, DWORD cchKeySize);
    HRESULT _GetUserAccountKey(LPCTSTR pszServer, LPCTSTR pszUserName, LPTSTR pszKey, DWORD cchKeySize);
    HRESULT _LoadLoginAttributes(DWORD * pdwLoginAttribs);
    HRESULT _SaveLoginAttributes(LPCTSTR pszServer, DWORD dwLoginAttribs);
    HRESULT _SetLoginType(HWND hDlg, BOOL fLoginAnnonymously);
    HRESULT _LoadEMailName(HWND hDlg);
    HRESULT _SaveEMailName(HWND hDlg);
    BOOL _SaveDialogData(HWND hDlg);
    HRESULT _LoadMessage(HWND hDlg);
    HRESULT _PopulateUserNameDropDown(HWND hDlg, LPCTSTR pszServer);
    HRESULT _LoadDefaultPassword(BOOL fLoadPersisted);
    HRESULT _SaveUserName(HWND hDlg);
    HRESULT _UserChangeSelect(HWND hDlg, BOOL fSelectChange);
    HRESULT _SavePassword(HWND hDlg, LPCTSTR pszUser, BOOL fPersist);
    HRESULT _GetPassword(LPCTSTR pszServer, LPCTSTR pszUserName, LPTSTR pszPassword, DWORD cchPassword);

    BOOL _InitDialog(HWND hdlg);
    LRESULT _OnCommand(HWND hdlg, WPARAM wParam, LPARAM lParam);

    static INT_PTR CALLBACK _LoginDialogProc(HWND hdlg, UINT wm, WPARAM wParam, LPARAM lParam);

    // Private Variables Functions
    LPCTSTR             m_pszServer;        // What is the server name?
    LPCTSTR             m_pszUser;          // What is the user name?
    LPCTSTR             m_pszPassword;      // What is the password?
    UINT                m_uiMessageID;      // What is the String ID of the message for the dialog?
    BOOL                m_dwLoginFlags;     // How should we behave?
    HWND                m_hDlg;
};

#endif // _ACCOUNT_H
