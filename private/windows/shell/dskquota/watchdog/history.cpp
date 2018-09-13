///////////////////////////////////////////////////////////////////////////////
/*  File: history.cpp

    Description: To prevent the watchdog from excessive repeate notifications,
        the system administrator can set a minimum period for notification
        silence.  This class (CHistory) manages the reading of this setting
        along with remembering the last time an action occured.  A client
        of the object is able to ask it if a given action should be performed
        on the basis of past actions.
        
            CHistory


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include <precomp.hxx>
#pragma hdrstop

#include "history.h"
#include "policy.h"

//
// Defined in dskquowd.cpp
//
extern TCHAR g_szRegSubKeyUser[];
extern TCHAR g_szRegSubKeyAdmin[];

//
// History-related registry parameter value names.
//
const TCHAR CHistory::SZ_REG_LAST_NOTIFY_POPUP_TIME[] = TEXT("LastNotifyPopupTime");
const TCHAR CHistory::SZ_REG_LAST_NOTIFY_EMAIL_TIME[] = TEXT("LastNotifyEmailTime");


CHistory::CHistory(
    CPolicy& policy
    ) : m_policy(policy)
{
    //
    // Build our registry parameter table.  This will create 
    // any keys (using the defaults provided) that don't already exist.
    //
    FILETIME ft = {0, 0};

    m_RegParams.AddBinaryParam(HKEY_CURRENT_USER,
                               g_szRegSubKeyAdmin,
                               SZ_REG_LAST_NOTIFY_POPUP_TIME,
                               (LPBYTE)&ft,
                               sizeof(ft));

    m_RegParams.AddBinaryParam(HKEY_CURRENT_USER,
                               g_szRegSubKeyAdmin,
                               SZ_REG_LAST_NOTIFY_EMAIL_TIME,
                               (LPBYTE)&ft,
                               sizeof(ft));
}



//
// Compare the current time with the last time a popup was displayed
// and the minimum popup period stored in the registry.
// If the elapsed time since the last popup exceeds the minimum period
// value stored in the registry, return TRUE.
//
BOOL
CHistory::ShouldPopupDialog(
    VOID
    )
{
    FILETIME ftLastPopup;
    FILETIME ftNow;
    DWORD    dwMinPeriod = 0;

    GetSysTime(&ftNow);
    
    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_LAST_NOTIFY_POPUP_TIME,
                             (LPBYTE)&ftLastPopup,
                             sizeof(ftLastPopup));


    //
    // Get the minimum popup period from the policy object.
    //
    dwMinPeriod = (DWORD)m_policy.GetMinNotifyPopupDialogPeriod();

    return (INT)dwMinPeriod <= CalcDiffMinutes(ftNow, ftLastPopup);
}


//
// Compare the current time with the last time email was sent
// and the minimum email period stored in the registry.
// If the elapsed time since the last email exceeds the minimum period
// value stored in the registry, return TRUE.
//
BOOL
CHistory::ShouldSendEmail(
    VOID
    )
{
    FILETIME ftLastEmail;
    FILETIME ftNow;
    DWORD    dwMinPeriod = 0;

    GetSysTime(&ftNow);
    
    m_RegParams.GetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_LAST_NOTIFY_EMAIL_TIME,
                             (LPBYTE)&ftLastEmail,
                             sizeof(ftLastEmail));

    //
    // Get the minimum popup period from the policy object.
    //
    dwMinPeriod = (DWORD)m_policy.GetMinNotifyEmailPeriod();

    return (INT)dwMinPeriod <= CalcDiffMinutes(ftNow, ftLastEmail);
}


//
// Should we do ANY notifications based on history information?
//
BOOL 
CHistory::ShouldDoAnyNotifications(
    VOID
    )
{
    return ShouldSendEmail() || 
           ShouldPopupDialog();
}       


//
// Save the current system time in the registry as the "last time"
// a popup was displayed.
//
VOID
CHistory::RecordDialogPoppedUp(
    VOID
    )
{
    FILETIME ftNow;

    GetSysTime(&ftNow);
    
    m_RegParams.SetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_LAST_NOTIFY_POPUP_TIME,
                             (LPBYTE)&ftNow,
                             sizeof(ftNow));
}


//
// Save the current system time in the registry as the "last time"
// an email message was sent.
//
VOID
CHistory::RecordEmailSent(
    VOID
    )
{
    FILETIME ftNow;

    GetSysTime(&ftNow);
    
    m_RegParams.SetParameter(HKEY_CURRENT_USER,
                             g_szRegSubKeyAdmin,
                             SZ_REG_LAST_NOTIFY_EMAIL_TIME,
                             (LPBYTE)&ftNow,
                             sizeof(ftNow));
}


//
// Get the system time as a FILETIME struct.
//
VOID
CHistory::GetSysTime(
    LPFILETIME pftOut
    )
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, pftOut);
}


//
// Calculate the difference between two FILETIME structures and
// return the difference converted to minutes.
//
INT
CHistory::CalcDiffMinutes(
    const FILETIME& ftA,
    const FILETIME& ftB
    )
{
    LARGE_INTEGER liMinA;
    LARGE_INTEGER liMinB;
    LARGE_INTEGER liDiff;

    liMinA.LowPart  = ftA.dwLowDateTime;
    liMinA.HighPart = ftA.dwHighDateTime;

    liMinB.LowPart  = ftB.dwLowDateTime;
    liMinB.HighPart = ftB.dwHighDateTime;
    
    liDiff.QuadPart = liMinA.QuadPart - liMinB.QuadPart;

    return (INT)(liDiff.QuadPart / ((__int64)600000000L));
}

