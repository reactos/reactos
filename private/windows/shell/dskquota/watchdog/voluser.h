#ifndef __VOLUSER_H
#define __VOLUSER_H
///////////////////////////////////////////////////////////////////////////////
/*  File: voluser.h

    Description: The CVolumeUser class maintains quota information about a 
        particular user on a volume.  Objects of this class are used
        in the CStatistics class to store information about a user/volume
        pair.

            CVolumeUser


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef _WINDOWS_
#   include <windows.h>
#endif


class CVolumeUser
{
    public:
        CVolumeUser(VOID);
        CVolumeUser(
            LPCTSTR pszDisplayName, 
            LPCTSTR pszEmailName,
            LARGE_INTEGER liQuotaThreshold,
            LARGE_INTEGER liQuotaLimit,
            LARGE_INTEGER liQuotaUsed);

        ~CVolumeUser(VOID);

        HRESULT SetUserInfo(
            LPCTSTR pszDisplayName, 
            LPCTSTR pszEmailName,
            LARGE_INTEGER liQuotaThreshold,
            LARGE_INTEGER liQuotaLimit,
            LARGE_INTEGER liQuotaUsed);

        VOID GetDisplayName(CString& strOut) const
            { strOut = m_strDisplayName; }

        LPCTSTR GetDisplayName(VOID) const
            { return (LPCTSTR)m_strDisplayName; }

        VOID GetEmailName(CString& strOut) const
            { strOut = m_strEmailName; }

        LPCTSTR GetEmailName(VOID) const
            { return (LPCTSTR)m_strEmailName; }

        LARGE_INTEGER GetQuotaThreshold(VOID) const
            { return m_liQuotaThreshold; }

        LARGE_INTEGER GetQuotaLimit(VOID) const
            { return m_liQuotaLimit; }

        LARGE_INTEGER GetQuotaUsed(VOID) const
            { return m_liQuotaUsed; }

    private:
        CString       m_strDisplayName;    // User's name for display
        CString       m_strEmailName;      // User's name for sending email.
        LARGE_INTEGER m_liQuotaThreshold;  // User's quota threshold on vol
        LARGE_INTEGER m_liQuotaLimit;      // User's quota limi on vol.
        LARGE_INTEGER m_liQuotaUsed;       // Quota used on vol by user.
};

#endif //__VOLUSER_H
