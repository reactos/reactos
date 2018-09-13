#ifndef __POLICY_H
#define __POLICY_H
///////////////////////////////////////////////////////////////////////////////
/*  File: policy.h

    Description: A system administrator is able to specify control parameters
        for the watchdog.  These include:
        
            Show popup dialog to user (yes/no)
            Send user email message (yes/no)
            Minimum period between popup dialogs (minutes)
            Minimum period between email messages (minutes)
            Add users to "To:" email list <email address list>
            Add users to "Cc:" email list <email address list>
            Add users to "Bcc:" email list <email address list>

        A client of the object is able to ask it if a given action should be
        performed (dialog, email) and for the additional email names.
        
            CPolicy

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
    07/10/97    Store info in HKEY_CURRENT_USER instead of           BrianAu
                policy.ini.
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef __DSKQUOTA_REG_PARAMS_H
#   include "regparam.h"
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

        INT GetMinNotifyPopupDialogPeriod(VOID)
            { return m_iMinPeriodPopupDialog; }

        INT GetMinNotifyEmailPeriod(VOID)
            { return m_iMinPeriodEmail; }

        BOOL ShouldSendUserEmail(VOID) const
            { return m_bSendUserEmail; }

        BOOL ShouldPopupDialog(VOID) const
            { return m_bPopupDialog; }

        BOOL ShouldSendEmail(VOID) const;

        BOOL ShouldDoAnyNotifications(VOID) const;

    private:
        RegParamTable m_RegParams;       // Registry parameter table.
        BOOL    m_bSendUserEmail;        // Should we send user email?
        BOOL    m_bPopupDialog;          // Should we popup a dialog on client?
        INT     m_iMinPeriodPopupDialog; // Minimum minutes between popup dialogs.
        INT     m_iMinPeriodEmail;       // Minimum minutes between email messages.
        CString m_strOtherEmailTo;       // Comma-sep list of email names.
        CString m_strOtherEmailCc;       // Comma-sep list of email names.
        CString m_strOtherEmailBcc;      // Comma-sep list of email names.

        //
        // Prevent copy.
        //
        CPolicy(const CPolicy& rhs);
        CPolicy& operator = (const CPolicy& rhs);

        //
        // Names of registry values.
        //
        static const TCHAR SZ_REG_SEND_USER_EMAIL[];
        static const TCHAR SZ_REG_SHOW_USER_POPUP[];
        static const TCHAR SZ_REG_MIN_PERIOD_POPUP[];
        static const TCHAR SZ_REG_MIN_PERIOD_EMAIL[];
        static const TCHAR SZ_REG_SEND_EMAIL_TO[];
        static const TCHAR SZ_REG_SEND_EMAIL_CC[];
        static const TCHAR SZ_REG_SEND_EMAIL_BCC[];

        //
        // Default values for registry.
        //
        static const INT   I_REG_SEND_USER_EMAIL_DEFAULT;
        static const INT   I_REG_SHOW_USER_POPUP_DEFAULT;
        static const INT   I_REG_MIN_PERIOD_POPUP_DEFAULT;
        static const INT   I_REG_MIN_PERIOD_EMAIL_DEFAULT;
        static const TCHAR SZ_REG_SEND_EMAIL_TO_DEFAULT[];
        static const TCHAR SZ_REG_SEND_EMAIL_CC_DEFAULT[];
        static const TCHAR SZ_REG_SEND_EMAIL_BCC_DEFAULT[];

};

#endif //__POLICY_H
