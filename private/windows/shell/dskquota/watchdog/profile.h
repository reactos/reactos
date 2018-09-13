#ifndef __POLICY_H
#define __POLICY_H

#ifndef _WINDOWS_
#   include <windows.h>
#endif

//
// This class provides an in-memory copy of the notification policy 
// information contained in an INI file or the registry.
//
class CPolicy
{
    public:
        CPolicy(VOID);
        ~CPolicy(VOID);

        HRESULT Load(LPCTSTR pszIniFile);
        HRESULT Load(HKEY hkeyReg);

        VOID GetOtherEmailTo(CString& strOut)
            { strOut = m_strOtherEmailTo; }

        LPTSTR GetOtherEmailTo(VOID)
            { return (LPTSTR)m_strOtherEmailTo; }

        VOID GetOtherEmailCc(CString& strOut)
            { strOut = m_strOtherEmailCc; }

        LPTSTR GetOtherEmailCc(VOID)
            { return (LPTSTR)m_strOtherEmailCc; }

        VOID GetOtherEmailBcc(CString& strOut)
            { strOut = m_strOtherEmailBcc; }

        LPTSTR GetOtherEmailBcc(VOID)
            { return (LPTSTR)m_strOtherEmailBcc; }

        BOOL ShouldSendUserEmail(VOID) const
            { return m_bSendUserEmail; }

        BOOL ShouldShowClientDialog(VOID) const
            { return m_bShowClientDialog; }

        BOOL ShouldSendAnyEmail(VOID) const;

    private:
        BOOL    m_bSendUserEmail;        // Should we send user email?
        BOOL    m_bShowClientDialog;     // Should we popup a dialog on client?
        CString m_strOtherEmailTo;       // Comma-sep list of email names.
        CString m_strOtherEmailCc;       // Comma-sep list of email names.
        CString m_strOtherEmailBcc;      // Comma-sep list of email names.

        VOID Reset(VOID);

        HRESULT LoadString(
            LPCTSTR pszFile,
            LPCTSTR pszSection,
            LPCTSTR pszValueName,
            LPCTSTR pszDefaultValue,
            CString& strOut);

        //
        // Prevent copy.
        //
        CPolicy(const CPolicy& rhs);
        CPolicy& operator = (const CPolicy& rhs);

        //
        // Names of registry/INI file sections and values.
        //
        static TCHAR SZ_REGINI_WATCHDOG[];
        static TCHAR SZ_REGINI_SEND_USER_EMAIL[];
        static TCHAR SZ_REGINI_SHOW_CLIENT_DIALOG[];
        static TCHAR SZ_REGINI_SEND_EMAIL_TO[];
        static TCHAR SZ_REGINI_SEND_EMAIL_CC[];
        static TCHAR SZ_REGINI_SEND_EMAIL_BCC[];

        static INT   I_REGINI_SEND_USER_EMAIL_DEFAULT;
        static INT   I_REGINI_SHOW_CLIENT_DIALOG_DEFAULT;
        static TCHAR SZ_REGINI_SEND_EMAIL_TO_DEFAULT[];
        static TCHAR SZ_REGINI_SEND_EMAIL_CC_DEFAULT[];
        static TCHAR SZ_REGINI_SEND_EMAIL_BCC_DEFAULT[];
};

#endif //__POLICY_H
