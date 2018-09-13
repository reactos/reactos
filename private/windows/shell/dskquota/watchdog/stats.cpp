///////////////////////////////////////////////////////////////////////////////
/*  File: stats.cpp

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
#include <precomp.hxx>
#pragma hdrstop

#include "dskquota.h"
#include "stats.h"

//
// Gather quota statistics for a given volume-user pair.
//
CStatistics::CStatistics(
    TCHAR chVolLetter,
    LPCTSTR pszVolDisplayName,
    LPBYTE pUserSid
    ) : m_vol(chVolLetter, pszVolDisplayName),
        m_bQuotaEnabled(FALSE),
        m_bWarnAtThreshold(FALSE),
        m_bDenyAtLimit(FALSE),
        m_bValid(FALSE)
{
    //
    // If the volume doesn't support quotas, no use in continuing
    // with the expensive stuff.
    //
    if (m_vol.SupportsQuotas())
    {
        HRESULT hr;

        hr = OleInitialize(NULL);
        if (SUCCEEDED(hr))
        {
            //
            // Load dskquota.dll, creating a new disk quota controller object.
            //
            IDiskQuotaControl *pQC;
            hr = CoCreateInstance(CLSID_DiskQuotaControl,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IDiskQuotaControl,
                                  (LPVOID *)&pQC);

            if (SUCCEEDED(hr))
            {
                //
                // Initialize the quota controller for this volume.
                // We need only read access.
                //
                TCHAR szVolume[] = TEXT("X:\\");
                szVolume[0] = m_vol.GetLetter();

                hr = IDiskQuotaControl_Initialize(pQC, szVolume, GENERIC_READ);
                if (SUCCEEDED(hr))
                {
                    DWORD dwQuotaState    = 0;
                    DWORD dwQuotaLogFlags = 0;

                    //
                    // Get the volume's quota state flags.
                    //
                    hr = pQC->GetQuotaState(&dwQuotaState);
                    if (SUCCEEDED(hr))
                    {
                        m_bQuotaEnabled = !DISKQUOTA_IS_DISABLED(dwQuotaState);
                        m_bDenyAtLimit  = DISKQUOTA_IS_ENFORCED(dwQuotaState);
                    }
                    //
                    // Get the volume's quota logging flags.
                    //
                    hr = pQC->GetQuotaLogFlags(&dwQuotaLogFlags);
                    if (SUCCEEDED(hr))
                    {
                        m_bWarnAtThreshold = m_bQuotaEnabled &&
                                             DISKQUOTA_IS_LOGGED_USER_THRESHOLD(dwQuotaLogFlags);
                    }

                    //
                    // Get the quota user object for the current user.
                    //
                    oleauto_ptr<IDiskQuotaUser> ptrUser;

                    hr = pQC->FindUserSid(pUserSid,
                                          ptrUser._getoutptr(),
                                          DISKQUOTA_USERNAME_RESOLVE_SYNC);
                    if (SUCCEEDED(hr))
                    {
                        //
                        // Get the user's account and friendly names.
                        //
                        TCHAR szUserName[MAX_PATH]      = { TEXT('\0') };
                        TCHAR szUserDomain[MAX_PATH]    = { TEXT('\0') };
                        TCHAR szUserAccount[MAX_PATH]   = { TEXT('\0') };
                        TCHAR szUserEmailName[MAX_PATH] = { TEXT('\0') };

                        IDiskQuotaUser_GetName(ptrUser, 
                                               szUserDomain, ARRAYSIZE(szUserDomain), 
                                               szUserAccount, ARRAYSIZE(szUserAccount),
                                               szUserName, ARRAYSIZE(szUserName));

                        //
                        // Get the user's quota statistics for the volume.
                        //
                        DISKQUOTA_USER_INFORMATION dui;

                        hr = ptrUser->GetQuotaInformation((LPBYTE)&dui, sizeof(dui));
                        if (SUCCEEDED(hr))
                        {
                            //
                            // Make sure we didn't enumerate a non-existant user.
                            // This is because of the way NTFS enumerates quota records.
                            // Even if there is no record currently in the quota file,
                            // enumeration returns a record.  This is consistent with 
                            // automatic addition of users upon first write to the
                            // volume.  We must check the quota information to
                            // determine if it's an active user.
                            //
                            if (((__int64)-2) != dui.QuotaLimit.QuadPart &&
                                (0 != dui.QuotaLimit.QuadPart ||
                                 0 != dui.QuotaThreshold.QuadPart ||
                                 0 != dui.QuotaUsed.QuadPart))
                            {
                                //
                                // Store user-specific info.
                                //
                                CString strUserDisplayName;
                                if (TEXT('\0') != szUserName[0])
                                {
                                    //
                                    // User has a "friendly" name associated
                                    // with their account.  Include it in the display
                                    // name.
                                    //
                                    strUserDisplayName.Format(TEXT("%1\\%2 (%3)"),
                                                              szUserDomain,
                                                              szUserAccount,
                                                              szUserName);
                                }
                                else
                                {
                                    //
                                    // No "friendly" name associated with user account.
                                    //
                                    strUserDisplayName.Format(TEXT("%1\\%2"),
                                                              szUserDomain,
                                                              szUserAccount,
                                                              szUserName);
                                }
                                //
                                // Store the user's information in the volume-user object.
                                //
                                hr = m_volUser.SetUserInfo(strUserDisplayName, 
                                                           szUserEmailName,
                                                           dui.QuotaThreshold,
                                                           dui.QuotaLimit,
                                                           dui.QuotaUsed);
                                if (FAILED(hr))
                                {
                                    DebugMsg(DM_ERROR, 
                                             TEXT("Error 0x%08X setting user info on volume \"%s\"."), 
                                             hr, pszVolDisplayName);
                                }
                                m_bValid = SUCCEEDED(hr);
                            }
                        }
                        else
                        {
                            DebugMsg(DM_ERROR, 
                                     TEXT("Error 0x%08X getting user quota info on volume \"%s\"."), 
                                     hr, pszVolDisplayName);
                        }
                    }
                    else
                    {
                        DebugMsg(DM_ERROR, 
                                 TEXT("Error 0x%08X finding quota user on volume \"%s\"."), 
                                 hr, pszVolDisplayName);
                    }
                }
                else
                {
                    DebugMsg(DM_ERROR, 
                             TEXT("Error 0x%08X initializing QC for volume \"%s\"."), 
                             hr, pszVolDisplayName);
                }
                pQC->ShutdownAndRelease(TRUE);
            } // if SUCCEEDED(CoCreateInstance())
            else
                DebugMsg(DM_ERROR, TEXT("Failed CoCreateInstance.  0x%08X"), hr);

            OleUninitialize();
        } // if SUCCEEDED(OleInitialize())
        else
            DebugMsg(DM_ERROR, TEXT("Failed OleInitialize.  0x%08X"), hr);
    } // if m_vol.SupportsQuotas()
    else
        DebugMsg(DM_ERROR, TEXT("Volume \"%s\" doesn't support quotas."), pszVolDisplayName);
}

//
// Determine if a specific statistics object contains statistics that
// will generate either an email notification or a popup notification.
//
BOOL 
CStatistics::IncludeInReport(
    VOID
    ) const
{
    BOOL bReport = FALSE;

    //
    // First check data validity and volume settings.
    //
    if (IsValid() &&        // Does volume support quotas and was vol opened?
        QuotasEnabled() &&  // Are quotas enabled on the volume?
        WarnAtThreshold())  // Is the "warn at threshold" bit set on the vol?
    {
        //
        // Now see if user's quota usage warrants reporting.
        // Report if AmtUsed > Threshold.
        //
        if (GetUserQuotaUsed().QuadPart > GetUserQuotaThreshold().QuadPart)
        {
            bReport = TRUE;
        }
    }
    return bReport;
}



CStatisticsList::CStatisticsList(
    VOID
    )
{
    //
    // Nothing to do.
    //
}

CStatisticsList::~CStatisticsList(
    VOID
    )
{
    INT cEntries = m_List.Count();
    for (INT i = 0; i < cEntries; i++)
    {
        delete m_List[i];
    }
    m_List.Clear();
}


const CStatistics *
CStatisticsList::GetEntry(
    INT iEntry
    )
{
    return m_List[iEntry];
}


HRESULT 
CStatisticsList::AddEntry(
    TCHAR chVolLetter,
    LPCTSTR pszVolDisplayName,
    LPBYTE pUserSid
    )
{
    HRESULT hr = E_OUTOFMEMORY;

    CStatistics *pStats = NULL;

    //
    // Create a new statistics object.
    // The ctor will fill in the volume and user information.
    //
    pStats = new CStatistics(chVolLetter, 
                             pszVolDisplayName, 
                             pUserSid);
    if (NULL != pStats)
    {
        if (pStats->IsValid())
        {
            BOOL bDuplicate = FALSE;

            //
            // Look for a duplicate.  Shouldn't be one but just to make sure.
            //
            INT cEntries = m_List.Count();

            for (INT i = 0; i < cEntries; i++)
            {
                const CStatistics *pEntry = m_List[i];

                if (pStats->SameVolume(*pEntry))
                    bDuplicate = TRUE;
            }
            if (!bDuplicate)
            {
                hr = S_OK;
                try
                {
                    m_List.Append(pStats);
                }
                catch(OutOfMemory)
                {
                    DebugMsg(DM_ERROR, TEXT("CStatisticsList::AddEntry - Insufficient memory."));
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                //
                // BUGBUG:  We should really assert here.
                //
                DebugMsg(DM_ERROR, TEXT("CStatisticsList::AddEntry - Duplicate entry found."));
                hr = S_FALSE;
            }
        }
        else
        {
            //
            // Stats object is invalid.  Probably because the volume
            // doesn't support quotas.
            //
            DebugMsg(DM_ERROR, TEXT("CStatisticsList::AddEntry - Volume stats not valid."));
            hr = S_FALSE;
        }
    }
    if (S_OK != hr)
    {
        //
        // CStatistics object wasn't added to the list.
        // Probably was a duplicate (shouldn't happen) or the volume
        // didn't support quotas.
        //
        delete pStats;
    }
    return hr;
}
