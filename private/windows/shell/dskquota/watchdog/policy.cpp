#include <precomp.hxx>
#pragma hdrstop
///////////////////////////////////////////////////////////////////////////////
/*  File: policy.cpp

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

#include "policy.h"

//
// Defined in dskquowd.cpp
//
extern TCHAR g_szRegSubKeyUser[];
extern TCHAR g_szRegSubKeyAdmin[];

//
// Names of registry file sections and values.
//
const TCHAR CPolicy::SZ_REG_SHOW_USER_POPUP[]  = TEXT("ShowUserPopup");
const TCHAR CPolicy::SZ_REG_MIN_PERIOD_POPUP[] = TEXT("MinNotifyMinutesPopup");
const TCHAR CPolicy::SZ_REG_MIN_PERIOD_EMAIL[] = TEXT("MinNotifyMinutesEmail");
const TCHAR CPolicy::SZ_REG_SEND_USER_EMAIL[]  = TEXT("SendUserEmail");
const TCHAR CPolicy::SZ_REG_SEND_EMAIL_TO[]    = TEXT("SendEmailTo");
const TCHAR CPolicy::SZ_REG_SEND_EMAIL_CC[]    = TEXT("SendEmailCc");
const TCHAR CPolicy::SZ_REG_SEND_EMAIL_BCC[]   = TEXT("SendEmailBcc");

//
// Default values written to empty registry key.
//
const INT   CPolicy::I_REG_SEND_USER_EMAIL_DEFAULT   = 1;
const INT   CPolicy::I_REG_SHOW_USER_POPUP_DEFAULT   = 1;
const INT   CPolicy::I_REG_MIN_PERIOD_POPUP_DEFAULT  = 240;
const INT   CPolicy::I_REG_MIN_PERIOD_EMAIL_DEFAULT  = 240;
const TCHAR CPolicy::SZ_REG_SEND_EMAIL_TO_DEFAULT[]  = TEXT("");
const TCHAR CPolicy::SZ_REG_SEND_EMAIL_CC_DEFAULT[]  = TEXT("");
const TCHAR CPolicy::SZ_REG_SEND_EMAIL_BCC_DEFAULT[] = TEXT("");



CPolicy::CPolicy(
    VOID
    ) : m_bSendUserEmail(FALSE),
        m_bPopupDialog(FALSE),
        m_iMinPeriodPopupDialog(I_REG_MIN_PERIOD_POPUP_DEFAULT),
        m_iMinPeriodEmail(I_REG_MIN_PERIOD_EMAIL_DEFAULT)
{
    //
    // Create the registry param table entries we'll be using.
    // These statements also specify the default values to use as well
    // as allowable limits (for DWORD values).
    //
    m_RegParams.AddDWordParam(HKEY_CURRENT_USER,
                              g_szRegSubKeyAdmin,
                              SZ_REG_SEND_USER_EMAIL,
                              0,
                              1,
                              I_REG_SEND_USER_EMAIL_DEFAULT);

    m_RegParams.AddDWordParam(HKEY_CURRENT_USER,
                              g_szRegSubKeyAdmin,
                              SZ_REG_SHOW_USER_POPUP,
                              0,
                              1,
                              I_REG_SHOW_USER_POPUP_DEFAULT);

    m_RegParams.AddDWordParam(HKEY_CURRENT_USER,
                              g_szRegSubKeyAdmin,
                              SZ_REG_MIN_PERIOD_POPUP,
                              0,
                              0xFFFFFFFF,
                              I_REG_MIN_PERIOD_POPUP_DEFAULT);

    m_RegParams.AddDWordParam(HKEY_CURRENT_USER,
                              g_szRegSubKeyAdmin,
                              SZ_REG_MIN_PERIOD_EMAIL,
                              0,
                              0xFFFFFFFF,
                              I_REG_MIN_PERIOD_EMAIL_DEFAULT);

    m_RegParams.AddSzParam(HKEY_CURRENT_USER,
                           g_szRegSubKeyAdmin,
                           SZ_REG_SEND_EMAIL_TO,
                           SZ_REG_SEND_EMAIL_TO_DEFAULT);

    m_RegParams.AddSzParam(HKEY_CURRENT_USER,
                           g_szRegSubKeyAdmin,
                           SZ_REG_SEND_EMAIL_CC,
                           SZ_REG_SEND_EMAIL_CC_DEFAULT);

    m_RegParams.AddSzParam(HKEY_CURRENT_USER,
                           g_szRegSubKeyAdmin,
                           SZ_REG_SEND_EMAIL_BCC,
                           SZ_REG_SEND_EMAIL_BCC_DEFAULT);

    //
    // Now retrieve our parameters from the registry.
    // If the parameters don't exist, they'll be created.
    //
    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_SEND_EMAIL_TO,
                             m_strOtherEmailTo.GetBuffer(MAX_PATH),
                             MAX_PATH);

    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_SEND_EMAIL_CC,
                             m_strOtherEmailCc.GetBuffer(MAX_PATH),
                             MAX_PATH);

    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_SEND_EMAIL_BCC,
                             m_strOtherEmailBcc.GetBuffer(MAX_PATH),
                             MAX_PATH);

    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_SHOW_USER_POPUP,
                             (LPDWORD)&m_bPopupDialog);

    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_SEND_USER_EMAIL,
                             (LPDWORD)&m_bSendUserEmail);

    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_MIN_PERIOD_POPUP,
                             (LPDWORD)&m_iMinPeriodPopupDialog);

    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_MIN_PERIOD_EMAIL,
                             (LPDWORD)&m_iMinPeriodEmail);
}




CPolicy::~CPolicy(
    VOID
    )
{
    //
    // Nothing to do.
    //
}


//
// Should we send any email?
//
BOOL 
CPolicy::ShouldSendEmail(
    VOID
    ) const
{
    return ShouldSendUserEmail()           ||
           0 != m_strOtherEmailTo.Length() ||
           0 != m_strOtherEmailCc.Length() ||
           0 != m_strOtherEmailBcc.Length();
}

//
// Should we do any notifications (popup or email) ?
//
BOOL 
CPolicy::ShouldDoAnyNotifications(
    VOID
    ) const
{
    return ShouldSendEmail() ||
           ShouldPopupDialog();
}

