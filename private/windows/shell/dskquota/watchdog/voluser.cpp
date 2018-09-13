///////////////////////////////////////////////////////////////////////////////
/*  File: voluser.cpp

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
#include <precomp.hxx>
#pragma hdrstop

#include "voluser.h"


CVolumeUser::CVolumeUser(
    VOID
    )
{
    m_liQuotaThreshold.QuadPart = 0;
    m_liQuotaLimit.QuadPart     = 0;
    m_liQuotaUsed.QuadPart      = 0;
}

CVolumeUser::CVolumeUser(
    LPCTSTR pszDisplayName, 
    LPCTSTR pszEmailName,
    LARGE_INTEGER liQuotaThreshold,
    LARGE_INTEGER liQuotaLimit,
    LARGE_INTEGER liQuotaUsed
    )
{
    SetUserInfo(pszDisplayName, 
                pszEmailName,
                liQuotaThreshold, 
                liQuotaLimit, 
                liQuotaUsed);
}


CVolumeUser::~CVolumeUser(
    VOID
    )
{

}


HRESULT 
CVolumeUser::SetUserInfo(
    LPCTSTR pszDisplayName, 
    LPCTSTR pszEmailName, 
    LARGE_INTEGER liQuotaThreshold,
    LARGE_INTEGER liQuotaLimit,
    LARGE_INTEGER liQuotaUsed
    )
{
    m_strDisplayName = pszDisplayName;
    m_strEmailName   = pszEmailName;

    m_liQuotaThreshold = liQuotaThreshold;
    m_liQuotaLimit     = liQuotaLimit;
    m_liQuotaUsed      = liQuotaUsed;

    return NOERROR;
}

