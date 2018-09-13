#include <precomp.hxx>
#pragma hdrstop

#include "policy.h"

//
// Names of registry/INI file sections and values.
//
TCHAR CPolicy::SZ_REGINI_WATCHDOG[]              = TEXT("DiskQuotaWatchDog");
TCHAR CPolicy::SZ_REGINI_SHOW_CLIENT_DIALOG[]    = TEXT("ShowClientDialog");
TCHAR CPolicy::SZ_REGINI_SEND_USER_EMAIL[]       = TEXT("SendUserEmail");
TCHAR CPolicy::SZ_REGINI_SEND_EMAIL_TO[]         = TEXT("SendEmailTo");
TCHAR CPolicy::SZ_REGINI_SEND_EMAIL_CC[]         = TEXT("SendEmailCc");
TCHAR CPolicy::SZ_REGINI_SEND_EMAIL_BCC[]        = TEXT("SendEmailBcc");


BOOL  CPolicy::I_REGINI_SEND_USER_EMAIL_DEFAULT     = 1;
BOOL  CPolicy::I_REGINI_SHOW_CLIENT_DIALOG_DEFAULT  = 0;
TCHAR CPolicy::SZ_REGINI_SEND_EMAIL_TO_DEFAULT[]    = TEXT("brianau@microsoft.com");
TCHAR CPolicy::SZ_REGINI_SEND_EMAIL_CC_DEFAULT[]    = TEXT("");
TCHAR CPolicy::SZ_REGINI_SEND_EMAIL_BCC_DEFAULT[]   = TEXT("");


CPolicy::CPolicy(
    VOID
    ) : m_bSendUserEmail(FALSE),
        m_bShowClientDialog(FALSE)
{

}


CPolicy::~CPolicy(
    VOID
    )
{
    //
    // Nothing to do.
    //
}


VOID 
CPolicy::Reset(
    VOID
    )
{
    m_bSendUserEmail    = FALSE;
    m_bShowClientDialog = FALSE;

    m_strOtherEmailTo.Empty();
    m_strOtherEmailCc.Empty();
    m_strOtherEmailBcc.Empty();
}


HRESULT 
CPolicy::Load(
    LPCTSTR pszIniFile
    )
{
    //
    // Clear out previous policy contents.
    //
    Reset();

    //
    // Load the contents of the INI file using appropriate defaults
    // if any values are not found.
    //
    LoadString(pszIniFile,
               SZ_REGINI_WATCHDOG,
               SZ_REGINI_SEND_EMAIL_TO,
               SZ_REGINI_SEND_EMAIL_TO_DEFAULT,
               m_strOtherEmailTo);

    LoadString(pszIniFile,
               SZ_REGINI_WATCHDOG,
               SZ_REGINI_SEND_EMAIL_CC,
               SZ_REGINI_SEND_EMAIL_CC_DEFAULT,
               m_strOtherEmailCc);

    LoadString(pszIniFile,
               SZ_REGINI_WATCHDOG,
               SZ_REGINI_SEND_EMAIL_BCC,
               SZ_REGINI_SEND_EMAIL_BCC_DEFAULT,
               m_strOtherEmailBcc);

    m_bShowClientDialog = GetPrivateProfileInt(SZ_REGINI_WATCHDOG,
                                               SZ_REGINI_SHOW_CLIENT_DIALOG,
                                               I_REGINI_SHOW_CLIENT_DIALOG_DEFAULT,
                                               pszIniFile);

    m_bSendUserEmail = GetPrivateProfileInt(SZ_REGINI_WATCHDOG,
                                            SZ_REGINI_SEND_USER_EMAIL,
                                            I_REGINI_SEND_USER_EMAIL_DEFAULT,
                                            pszIniFile);
    return NO_ERROR;
}

//
// Loads policy string.  Handles proper sizing of the value buffer.
// Caller is responsible for calling delete[] on *ppsz.
//
HRESULT
CPolicy::LoadString(
    LPCTSTR pszFile,
    LPCTSTR pszSection,
    LPCTSTR pszValueName,
    LPCTSTR pszDefaultValue,
    CString& strOut
    )
{
    HRESULT hr      = E_OUTOFMEMORY;
    INT cchIncr     = MAX_PATH;
    INT cchValue    = 0;
    INT cchRead     = 0;

    do
    {
        cchValue += cchIncr;
        cchRead = GetPrivateProfileString(pszSection,
                                          pszValueName,
                                          pszDefaultValue,
                                          strOut.GetBuffer(cchValue),
                                          cchValue,
                                          pszFile);
        if (0 == cchRead)
        {
            hr = E_FAIL;
        }
    }
    while(SUCCEEDED(hr) && cchRead == (cchValue - 1));

    return hr;
}        



HRESULT 
CPolicy::Load(
    HKEY hkeyReg
    )
{
    HRESULT hr = NO_ERROR;
    //
    // Clear out previous policy contents.
    //
    Reset();

    return hr;
}


//
// Should we send any email?
//
BOOL 
CPolicy::ShouldSendAnyEmail(
    VOID
    ) const
{
    return ShouldSendUserEmail()           ||
           0 != m_strOtherEmailTo.Length() ||
           0 != m_strOtherEmailCc.Length() ||
           0 != m_strOtherEmailBcc.Length();
}

