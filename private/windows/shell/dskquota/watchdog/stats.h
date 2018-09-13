#ifndef __STATS_H
#define __STATS_H
///////////////////////////////////////////////////////////////////////////////
/*  File: stats.h

    Description: These classes provide temporary storage of quota
        information for a given volume/user pair.  Creation of the object
        automatically gathers the necessary quota and user information.  
        Clients then query the objects to retrieve quota statistics when
        desired.

            CStatistics
            CStatisticsList
                                                            +----------+
                                                       +--->| CVolume  |
                                                       |    +----------+
            +-----------------+     +-------------+<---+
            | CStatisticsList |<-->>| CStatistics | contains
            +-----------------+     +-------------+<---+
                                                       |    +-------------+
                                                       +--->| CVolumeUser |
                                                            +-------------+

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

#ifndef _WINDOWS_
#   include <windows.h>
#endif

#ifndef __VOLUSER_H
#   include "voluser.h"
#endif

#ifndef __VOLUME_H
#   include "volume.h"
#endif

class CStatistics
{
    public:
        CStatistics(TCHAR chVolLetter, 
                    LPCTSTR pszVolDisplayName, 
                    LPBYTE pUserSid);
        ~CStatistics(VOID) { };

        BOOL SameVolume(const CStatistics& rhs) const
            { return m_vol == rhs.m_vol; }

        TCHAR GetVolumeLetter(VOID) const
            { return m_vol.GetLetter(); }

        VOID GetVolumeDisplayName(CString& strOut) const
            { m_vol.GetDisplayName(strOut); }

        LPCTSTR GetVolumeDisplayName(VOID) const
            { return m_vol.GetDisplayName(); }

        BOOL QuotasEnabled(VOID) const
            { return m_bQuotaEnabled; }

        BOOL WarnAtThreshold(VOID) const
            { return m_bWarnAtThreshold; }

        BOOL DenyAtLimit(VOID) const
            { return m_bDenyAtLimit; }

        VOID GetUserDisplayName(CString& strOut) const
            { m_volUser.GetDisplayName(strOut); }

        LPCTSTR GetUserDisplayName(VOID) const
            { return m_volUser.GetDisplayName(); }

        VOID GetUserEmailName(CString& strOut) const
            { m_volUser.GetEmailName(strOut); }

        LPCTSTR GetUserEmailName(VOID) const
            { return m_volUser.GetEmailName(); }

        LARGE_INTEGER GetUserQuotaThreshold(VOID) const
            { return m_volUser.GetQuotaThreshold(); }

        LARGE_INTEGER GetUserQuotaLimit(VOID) const
            { return m_volUser.GetQuotaLimit(); }

        LARGE_INTEGER GetUserQuotaUsed(VOID) const
            { return m_volUser.GetQuotaUsed(); }

        BOOL IsValid(VOID) const
            { return m_bValid; }

        BOOL IncludeInReport(VOID) const;

    private:
        CVolume        m_vol;               // Volume-specific info.
        CVolumeUser    m_volUser;           // User-specific info.
        BOOL           m_bQuotaEnabled;     // Are quotas enabled?
        BOOL           m_bWarnAtThreshold;  // Warn user at threshold?
        BOOL           m_bDenyAtLimit;      // Deny disk space at limit?
        BOOL           m_bValid;            // Is this object valid?

        //
        // Prevent copy.
        //
        CStatistics(const CStatistics& rhs);
        CStatistics& operator = (const CStatistics& rhs);
};


class CStatisticsList
{
    public:
        CStatisticsList(VOID);
        ~CStatisticsList(VOID);

        INT Count(VOID)
            { return m_List.Count(); }

        const CStatistics *GetEntry(INT iEntry);

        HRESULT AddEntry(TCHAR chVolLetter,
                         LPCTSTR pszVolDisplayName,
                         LPBYTE pUserSid);

    private:
        CArray<CStatistics *> m_List;
};


#endif //__STATS_H
