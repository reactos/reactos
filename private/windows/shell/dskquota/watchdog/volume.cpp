///////////////////////////////////////////////////////////////////////////////
/*  File: volume.cpp

    Description: The CVolume class maintains information about a particular
        disk volume.  During initialization, the object determines if the
        associated volume supports quotas.  The object's client can then query
        it for volume-specific quota information. Objects of this class are used
        in the CStatistics class to store information about a user/volume
        pair.

            CVolume


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    07/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#include <precomp.hxx>
#pragma hdrstop

#include "volume.h"


CVolume::CVolume(
    TCHAR chLetter,
    LPCTSTR pszDisplayName
    ) : m_chLetter(chLetter),
        m_dwSerialNo(0),
        m_strDisplayName(pszDisplayName),
        m_bSupportsQuotas(FALSE)
{
    TCHAR szVolume[] = TEXT("X:\\");
    DWORD dwFsFlags = 0;

    szVolume[0] = m_chLetter;

    if (::GetVolumeInformation(szVolume,      // i.e. "X:"
                               NULL,          // Vol name, don't need it.
                               0,             // Vol name buf len.
                               &m_dwSerialNo, // Serial number var ptr.
                               NULL,          // Max component len var ptr.
                               &dwFsFlags,    // File system flags var ptr.
                               NULL,          // File sys name, don't need it.
                               0))            // File sys name buf len.
    {
        m_bSupportsQuotas = (0 != (dwFsFlags & FILE_VOLUME_QUOTAS));
    }
    else
    {
        DebugMsg(DM_ERROR, 
                 TEXT("CVolume - GetVolumeInformation for \"%s\" failed with error %d"), 
                 pszDisplayName, GetLastError());
    }
}

CVolume::~CVolume(
    VOID
    )
{

}

