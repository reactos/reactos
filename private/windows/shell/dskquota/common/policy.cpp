///////////////////////////////////////////////////////////////////////////////
/*  File: policy.cpp

    Description: Handles disk quota policy issues for both a GPE client 
        extension and a server MMC policy snapin (see snapin.cpp).

        ProgressGroupPolicy is called by winlogon to process disk quota policy
        on the client machine.   ProcessGroupPolicy instantiates a CDiskQuotaPolicy
        object to handle the loading and application of disk quota policy.

        The CDiskQuotaPolicy object is also instantiated by the MMC
        disk quota policy snapin to save quota policy information to the
        registry.

        A good deal of this module, especially in CDiskQuotaPolicy::Apply( ),
        is devoted to reporting errors to the NT event log.  This is necessary
        because much of this code runs without UI from within winlogon.  That's
        also why there's a lot of debugger spew.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    02/14/98    Initial creation.                                    BrianAu
    11/24/98    Added event logging settings to policy.              BrianAu
    11/30/98    Replaced ProcessGPO function with                    BrianAu
                ProcessGroupPolicy to support GPO interface changes.
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include <userenv.h>
#include <gpedit.h>
#include <dskquota.h>
#include "policy.h"
#include "registry.h"
#include "guidsp.h"
#include "msg.h"
#include "resource.h"

//
// Global NT event log object.
//
CEventLog g_Log;
//
// Name of the disk quota dll.
//
const TCHAR g_szDskquotaDll[] = TEXT("dskquota.dll");


//
// Exported function called by winlogon to update policy on the client machine.
//
// This function is registered as a GPO extension (see selfreg.inf).
//
DWORD
ProcessGroupPolicy(
    DWORD dwFlags,
    HANDLE hUserToken,
    HKEY hkeyRoot,
    PGROUP_POLICY_OBJECT pDeletedGPOList,
    PGROUP_POLICY_OBJECT pChangedGPOList,
    ASYNCCOMPLETIONHANDLE pHandle,
    BOOL *pbAbort,
    PFNSTATUSMESSAGECALLBACK pStatusCallback
    )
{
    HRESULT hr = ERROR_SUCCESS;

    DBGTRACE((DM_POLICY, DL_HIGH, TEXT("ProcessGroupPolicy")));
    DBGPRINT((DM_POLICY, DL_LOW, TEXT("\tdwFlags......: 0x%08X"), dwFlags));
    DBGPRINT((DM_POLICY, DL_LOW, TEXT("\thUserToken...: 0x%08X"), hUserToken));
    DBGPRINT((DM_POLICY, DL_LOW, TEXT("\thKeyRoot.....: 0x%08X"), hkeyRoot));

    //
    // BugBug:  Need to add support for pDeletedGPOList
    //          If pDeletedGPOList is non-null, you should
    //          reset the disk quotas back to their defaults first
    //          and then apply the new settings below if appropriate
    //

    if (pChangedGPOList)
    {
        hr = g_Log.Initialize(TEXT("DiskQuota"));

        if (FAILED(hr))
        {
            DBGERROR((TEXT("Error 0x%08X initializing NT event log."), hr));
            //
            // Continue without event log.
            //
        }


        //
        // Only process policy info when...
        //
        //  1. Not deleting policy.
        //

        try
        {
            DBGPRINT((DM_POLICY, DL_HIGH, TEXT("Set quota policy - START.")));

            autoptr<CDiskQuotaPolicy> ptrPolicy(new CDiskQuotaPolicy(NULL,
                                                                     hkeyRoot,
                                                                     0 != (GPO_INFO_FLAG_VERBOSE & dwFlags),
                                                                     pbAbort));
            DISKQUOTAPOLICYINFO dqpi;
            ZeroMemory(&dqpi, sizeof(dqpi));

            //
            // Load policy info from the registry and apply to local volumes.
            //

            hr = ptrPolicy->Load(&dqpi);

            if (SUCCEEDED(hr))
            {
                hr = ptrPolicy->Apply(&dqpi);
            }

            DBGPRINT((DM_POLICY, DL_HIGH, TEXT("Set quota policy - FINISHED.")));
        }
        catch(CAllocException& e)
        {
            DBGERROR((TEXT("Insufficient memory in ProcessGroupPolicy")));
            g_Log.ReportEvent(EVENTLOG_ERROR_TYPE,
                              0,
                              MSG_E_POLICY_OUTOFMEMORY);
        }
        catch(...)
        {
            DBGERROR((TEXT("C++ exception caught in ProcessGroupPOlicy")));
            g_Log.ReportEvent(EVENTLOG_ERROR_TYPE,
                              0,
                              MSG_E_POLICY_EXCEPTION);
        }
    }


    return hr;
}






//-----------------------------------------------------------------------------
// CDiskQuotaPolicy
//-----------------------------------------------------------------------------

//
// Location of disk quota policy information in the registry.  The "PolicyData"
// value name is somewhat arbitrary.  The policy key name string however must
// coordinate with other system policy locations in the registry.  It should 
// not change unless you have a good reason to do so and you understand the 
// consequences.
//
const TCHAR CDiskQuotaPolicy::m_szRegKeyPolicy[] = REGSTR_KEY_POLICYDATA;
#ifdef POLICY_MMC_SNAPIN
const TCHAR CDiskQuotaPolicy::m_szRegValPolicy[] = REGSTR_VAL_POLICYDATA;
#endif

CDiskQuotaPolicy::CDiskQuotaPolicy(
    LPGPEINFORMATION pGPEInfo,
    HKEY hkeyRoot,
    bool bVerboseEventLog,
    BOOL *pbAbort
    ) : m_cRef(0),
        m_pGPEInfo(pGPEInfo),
        m_hkeyRoot(hkeyRoot),
        m_pbAbort(pbAbort),
        m_bRootKeyOpened(false),
        m_bVerboseEventLog(bVerboseEventLog)
{
    DBGTRACE((DM_POLICY, DL_MID, TEXT("CDiskQuotaPolicy::CDiskQuotaPolicy")));
}


CDiskQuotaPolicy::~CDiskQuotaPolicy(
    void
    )
{
    DBGTRACE((DM_POLICY, DL_MID, TEXT("CDiskQuotaPolicy::~CDiskQuotaPolicy")));

    if (NULL != m_hkeyRoot && m_bRootKeyOpened)
        RegCloseKey(m_hkeyRoot);

    if (NULL != m_pGPEInfo)
        m_pGPEInfo->Release();
}


HRESULT
CDiskQuotaPolicy::QueryInterface(
    REFIID riid, 
    LPVOID *ppvOut
    )
{
    DBGTRACE((DM_POLICY, DL_MID, TEXT("CDiskQuotaPolicy::~QueryInterface")));
    HRESULT hr = E_NOINTERFACE;

    *ppvOut = NULL;

    if (IID_IUnknown == riid || IID_IDiskQuotaPolicy == riid)
    {
        *ppvOut = this;
        ((LPUNKNOWN)*ppvOut)->AddRef();
        hr = NOERROR;
    }

    return hr;
}



ULONG
CDiskQuotaPolicy::AddRef(
    void
    )
{
    DBGTRACE((DM_POLICY, DL_LOW, TEXT("CDiskQuotaPolicy::AddRef")));
    ULONG ulReturn = m_cRef + 1;
    InterlockedIncrement(&m_cRef);
    return ulReturn;
}


ULONG
CDiskQuotaPolicy::Release(
    void
    )
{
    DBGTRACE((DM_POLICY, DL_LOW, TEXT("CDiskQuotaPolicy::Release")));
    ULONG ulReturn = m_cRef - 1;
    if (InterlockedDecrement(&m_cRef) == 0)
    {   
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}


//
// Caller can init with either:
//
//   1. Ptr to IGPEInformation interface.  The snap in should initialize this 
//      way since it has a pointer to an IGPEInformation 
//      interface (LPGPEINFORMATION).
//
//   2. HKEY retrieved from IGPEInformation interface or from Group Policy
//      notification.  ProcessGroupPolicy should initialize this way since it is given 
//      the root key from winlogon.
//
// Can also init with both but pGPEInfo will be ignored if hkeyRoot is provided.
//
HRESULT
CDiskQuotaPolicy::Initialize(
    LPGPEINFORMATION pGPEInfo,
    HKEY hkeyRoot
    )
{
    DBGTRACE((DM_POLICY, DL_MID, TEXT("CDiskQuotaPolicy::Initialize")));

    if (NULL != m_pGPEInfo || NULL != m_hkeyRoot)
        return S_FALSE;     // Already intialized

    m_hkeyRoot = hkeyRoot;
    m_pGPEInfo = pGPEInfo;

    if (NULL != m_pGPEInfo)
        m_pGPEInfo->AddRef();

    return S_OK;
}

//
// Fill in a DISKQUOTAPOLICYINFO structure with default data.
//
void
CDiskQuotaPolicy::InitPolicyInfo(
    LPDISKQUOTAPOLICYINFO pInfo
    )
{
    pInfo->llDefaultQuotaThreshold = (LONGLONG)-1; // No limit.
    pInfo->llDefaultQuotaLimit     = (LONGLONG)-1; // No limit.
    pInfo->dwQuotaState            = 0;
    pInfo->dwQuotaLogFlags         = 0;
    pInfo->bRemovableMedia         = 0;
}


//
// Initialize and load policy information into a DISKQUOTAPOLICYINFO structure.
// If reg values don't exist, default values are used.
//
void
CDiskQuotaPolicy::LoadPolicyInfo(
    const RegKey& key,
    LPDISKQUOTAPOLICYINFO pInfo
    )
{
    DWORD dwValue = DWORD(-1);
    const struct
    {
        LPCTSTR pszValue; // Name of the "value" reg value.
        LPCTSTR pszUnits; // Name of the "units" reg value.
        LONGLONG *pValue; // Address of destination for computed limit or threshold.

    } rgValUnits[] = {
        { REGSTR_VAL_POLICY_LIMIT,     REGSTR_VAL_POLICY_LIMITUNITS,     &(pInfo->llDefaultQuotaLimit)     },
        { REGSTR_VAL_POLICY_THRESHOLD, REGSTR_VAL_POLICY_THRESHOLDUNITS, &(pInfo->llDefaultQuotaThreshold) }
                     };

    //
    // Initialize with defaults.
    //
    InitPolicyInfo(pInfo);

    //
    // Load the limit and threshold values along with their respective "units"
    // factor.  The factor is a number [1..6] that represents the required
    // multiplier to convert the "value" to a byte value.
    //
    //      1 = KB, 2 = MB, 3 = GB, 4 = TB, 5 = PB, 6 = EB
    //
    // Bytes = value << (factor * 10).
    //
    // Given: value  = 250
    //        factor = 2 (MB)
    //
    // Bytes = 250 << 20
    //       = 262,144,000 
    //       = 250 MB
    //
    for (int i = 0; i < ARRAYSIZE(rgValUnits); i++)
    {
        dwValue = DWORD(-1);
        DWORD dwUnits = DWORD(-1);

        key.GetValue(rgValUnits[i].pszValue, &dwValue);
        key.GetValue(rgValUnits[i].pszUnits, &dwUnits);
        *(rgValUnits[i].pValue) = LONGLONG(-1);
        if (0 <= dwValue && 0 <= dwUnits && 6 >= dwUnits)
        {
            *(rgValUnits[i].pValue) = LONGLONG(dwValue) << (10 * dwUnits);
        }
    }

    //
    // This logic for setting the dwQuotaState member is the same as that
    // used in VolumePropPage::QuotaStateFromControls (volprop.cpp).
    //
    DWORD dwEnable  = 0;
    DWORD dwEnforce = 0;
    key.GetValue(REGSTR_VAL_POLICY_ENABLE,  &dwEnable);
    key.GetValue(REGSTR_VAL_POLICY_ENFORCE, &dwEnforce);
    if (dwEnable)
    {
        if (dwEnforce)
        {
            pInfo->dwQuotaState = DISKQUOTA_STATE_ENFORCE;
        }
        else
        {
            pInfo->dwQuotaState = DISKQUOTA_STATE_TRACK;
        }
    }
    else
    {
        pInfo->dwQuotaState = DISKQUOTA_STATE_DISABLED;
    }

    //
    // Get event logging settings.
    //
    DWORD dwLog = 0;
    key.GetValue(REGSTR_VAL_POLICY_LOGLIMIT, &dwLog);
    DISKQUOTA_SET_LOG_USER_LIMIT(pInfo->dwQuotaLogFlags, dwLog);
    dwLog = 0;
    key.GetValue(REGSTR_VAL_POLICY_LOGTHRESHOLD, &dwLog);
    DISKQUOTA_SET_LOG_USER_THRESHOLD(pInfo->dwQuotaLogFlags, dwLog);

    //
    // Determine if policy is to be applied to removable as well as fixed 
    // media.
    //
    if (SUCCEEDED(key.GetValue(REGSTR_VAL_POLICY_REMOVABLEMEDIA, &dwValue)))
    {
        pInfo->bRemovableMedia = boolify(dwValue);
    }
}


//
// Load machine policy information from the registry.  See comment
// in CDiskQuotaPolicy::Save( ) for registry location information.
//
HRESULT
CDiskQuotaPolicy::Load(
    LPDISKQUOTAPOLICYINFO pInfo
    )
{
    DBGTRACE((DM_POLICY, DL_MID, TEXT("CDiskQuotaPolicy::Load")));
    DBGASSERT((NULL != pInfo));

    if (NULL == m_pGPEInfo && NULL == m_hkeyRoot)
    {
        DBGERROR((TEXT("Policy object not initialized")));
        return E_FAIL;      // Not initialized.
    }

    HRESULT hr = E_FAIL;

    if (NULL == m_hkeyRoot &&
        SUCCEEDED(hr = m_pGPEInfo->GetRegistryKey(GPO_SECTION_MACHINE, &m_hkeyRoot)))
    {
        m_bRootKeyOpened = true;
    }
    if (NULL != m_hkeyRoot)
    {
        DBGPRINT((DM_POLICY, DL_LOW, TEXT("Opening reg key 0x%08X \"%s\""), m_hkeyRoot, m_szRegKeyPolicy));
        RegKey key(m_hkeyRoot, m_szRegKeyPolicy);
        hr = key.Open(KEY_READ);
        if (SUCCEEDED(hr))
        {
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("Reading disk quota policy information.")));
            LoadPolicyInfo(key, pInfo);

            if (m_bVerboseEventLog)
            {
                //
                // Report successful information retrieval.
                //
                g_Log.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, MSG_I_POLICY_INFOLOADED);
            }
        }
        else if (ERROR_FILE_NOT_FOUND != HRESULT_CODE(hr)) // Key doesn't always exist.
        {
            DBGERROR((TEXT("Error 0x%08X opening policy reg key"), hr));
            g_Log.Push(hr, CEventLog::eFmtHex);
            g_Log.Push(m_szRegKeyPolicy),
            g_Log.Push(hr, CEventLog::eFmtSysErr);
            g_Log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_POLICY_GPEREGKEYOPEN);
        }
    }
    else
    {
        DBGERROR((TEXT("m_hkeyRoot is NULL")));
        g_Log.Push(hr, CEventLog::eFmtHex);
        g_Log.Push(hr, CEventLog::eFmtSysErr);
        g_Log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_POLICY_GPEREGKEYROOT);
    }

    return hr;
}

#ifdef POLICY_MMC_SNAPIN
//
// BUGBUG:  This code has been disabled because we're moving from using an MMC
//          snapin to an ADM-file format that fits better into the current
//          MMC software policy scheme.  This Save() function was required for
//          the snapin implementation but is not required when we use an ADM file.
//          The function was originally written to store a whole DISKQUOTAPOLICYINFO 
//          structure in the registry and the original version of Load() was 
//          written to read a whole DISKQUOTAPOLICYINFO structure as REG_BINARY.
//          Since the ADM file works with specific reg values rather than a single
//          REG_BINARY, Load() was modified to work with values stored using
//          the ADM format.  Save() has not been modified.
//          
//          If we reactivate this function to use in a snapin, we need to 
//          update it to write out data in a format acceptable to Load().
//          [brianau - 6/25/98]
//
// This function saves the policy info to the following registry value on the
// local machine.
//
//    HKCU\Software\Microsoft\GPE\{98E1D3C1-9DC1-11D1-8544-0000F8046117}Machine\Software\Policies\Microsoft\Windows NT\DiskQuota\PolicyData"
// 
// The call to PolicyChanged( ) flushes the data to the server file:
//
//    \\<server>\SysVol\Policies\{98E1D3C1-9DC1-11D1-8544-0000F8046117}\machine\registry.pol
//
// where <server> is the name of the server.
//
//
HRESULT
CDiskQuotaPolicy::Save(
    LPCDISKQUOTAPOLICYINFO pInfo
    )
{
    DBGTRACE((DM_POLICY, DL_MID, TEXT("CDiskQuotaPolicy::Save")));
    DBGASSERT((NULL != pInfo));

    if (NULL == m_pGPEInfo && NULL == m_hkeyRoot)
    {
        DBGERROR((TEXT("Policy object not initialized")));
        return E_FAIL;      // Not initialized.
    }

    HRESULT hr = E_FAIL;

    if (NULL == m_hkeyRoot &&
        SUCCEEDED(hr = m_pGPEInfo->GetRegistryKey(GPO_SECTION_MACHINE, &m_hkeyRoot)))
    {
        m_bRootKeyOpened = true;
    }
    if (NULL != m_hkeyRoot)
    {
        DBGPRINT((DM_POLICY, DL_LOW, TEXT("Creating reg key 0x%08X \"%s\""), m_hkeyRoot, m_szRegKeyPolicy));
        RegKey key(m_hkeyRoot, m_szRegKeyPolicy);
        hr = key.Open(KEY_WRITE, true);
        if (SUCCEEDED(hr))
        {
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("Setting reg value \"%s\""), m_szRegValPolicy));
            hr = key.SetValue(m_szRegValPolicy, (LPBYTE)pInfo, sizeof(*pInfo));
            if (SUCCEEDED(hr))
            {
                DBGPRINT((DM_POLICY, DL_LOW, TEXT("Calling PolicyChanged().")));
                if (FAILED(hr = m_pGPEInfo->PolicyChanged(TRUE)))
                    DBGERROR((TEXT("Error 0x%08X returned by PolicyChanged()"), hr));
            }
            else
                DBGERROR((TEXT("Error 0x%08X setting policy reg value"), hr));
        }
        else
            DBGERROR((TEXT("Error 0x%08X opening policy reg key"), hr));
    }
    else
        DBGERROR((TEXT("m_hkeyRoot is NULL")));

    return hr;
}
#endif // POLICY_MMC_SNAPIN

//
// Apply policy information to all local NTFS volumes.  Removable media
// are optional per a value in the policy information structure.
//
HRESULT
CDiskQuotaPolicy::Apply(
    LPCDISKQUOTAPOLICYINFO pInfo
    )
{
    DBGTRACE((DM_POLICY, DL_MID, TEXT("CDiskQuotaPolicy::Apply")));
    DBGASSERT((NULL != pInfo));

    HRESULT hr              = NOERROR;
    bool bCoUninitialize    = false;
    BOOL bAborted           = m_pbAbort ? *m_pbAbort : FALSE;

    try
    {
        if (!bAborted)
        {
            CString strVolCompleted;

            //
            // Get list of drives on which to set policy.
            //
            CArray<CString> rgstrDrives;
            hr = GetDriveNames(&rgstrDrives, pInfo->bRemovableMedia);
            int cDrives = rgstrDrives.Count();
            if (SUCCEEDED(hr) && 0 < cDrives)
            {
                hr = CoInitialize(NULL);  // Init COM only if we have drives to set.
                if (FAILED(hr))
                {
                    DBGERROR((TEXT("CoInitialize failed with error 0x%08X"), hr));
                    //
                    // Don't bother reporting this to the event log.  If we can't
                    // initialize COM, we have bigger problems than applying disk
                    // quota policy.
                    //
                    return hr;
                }

                bCoUninitialize = true;

                //
                // Get the disk quota class factory.  This way we don't
                // call CoCreateInstance for each drive.  Only call it once then
                // call the class factory's CreateInstance for each drive.  
                // Should be more efficient.
                // 
                com_autoptr<IClassFactory> pcf;
                hr = CoCreateInstance(CLSID_DiskQuotaControl, 
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IClassFactory,
                                      reinterpret_cast<void **>(pcf.getaddr()));
                if (SUCCEEDED(hr))
                {
                    bAborted = m_pbAbort ? *m_pbAbort : FALSE;

                    for (int i = 0; i < cDrives && !bAborted; i++)
                    {
                        DBGPRINT((DM_POLICY, DL_MID, TEXT("Setting policy for \"%s\""), rgstrDrives[i].Cstr()));
                        //
                        // Get a quota control object and initialize it for drive[i].
                        // Init with read/write access.
                        //
                        com_autoptr<IDiskQuotaControl> pdqc;
                        hr = pcf->CreateInstance(NULL, 
                                                 IID_IDiskQuotaControl, 
                                                 reinterpret_cast<void **>(pdqc.getaddr()));
                        if (SUCCEEDED(hr))
                        {
                            hr = pdqc->Initialize(rgstrDrives[i], TRUE);
                            if (SUCCEEDED(hr))
                            {
                                //
                                // Set the quota information on the volume.
                                //
                                if (FAILED(hr = pdqc->SetQuotaLogFlags(pInfo->dwQuotaLogFlags)))
                                {
                                    DBGERROR((TEXT("Error 0x%08X setting log flags"), hr));
                                    goto setpolerr;
                                }
                                if (FAILED(hr = pdqc->SetDefaultQuotaThreshold(pInfo->llDefaultQuotaThreshold)))
                                {
                                    DBGERROR((TEXT("Error 0x%08X setting default threshold"), hr));
                                    goto setpolerr;
                                }
                                if (FAILED(hr = pdqc->SetDefaultQuotaLimit(pInfo->llDefaultQuotaLimit)))
                                {
                                    DBGERROR((TEXT("Error 0x%08X setting default limit"), hr));
                                    goto setpolerr;
                                }
                                //
                                // Set state last in case we're enabling quotas.  That way
                                // any rebuild activity will come after the other settings have
                                // been set.
                                //
                                if (FAILED(hr = pdqc->SetQuotaState(DISKQUOTA_STATE_MASK & pInfo->dwQuotaState)))
                                {
                                    DBGERROR((TEXT("Error 0x%08X setting quota state"), hr));
                                    goto setpolerr;
                                }
                                goto setpolsuccess;
                            }
                            else
                                DBGERROR((TEXT("Error 0x%08X initializing vol \"%s\""), 
                                          hr, rgstrDrives[i].Cstr()));

setpolerr:
                            //
                            // Record error for this particular volume.
                            //
                            g_Log.Push(hr, CEventLog::eFmtHex);
                            g_Log.Push(rgstrDrives[i].Cstr());
                            g_Log.Push(hr, CEventLog::eFmtSysErr);
                            g_Log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_POLICY_SETQUOTA);
setpolsuccess:
                            if (m_bVerboseEventLog && SUCCEEDED(hr))
                            {
                                //
                                // Append name to list of drives that have been successful.
                                //
                                strVolCompleted += rgstrDrives[i];
                                strVolCompleted += CString(TEXT("  "));
                            }
                            pdqc = NULL;  // This releases pdqc.
                        }
                        else
                        {
                            DBGERROR((TEXT("CreateInstance failed with error 0x%08X"), hr));
                            g_Log.Push(hr, CEventLog::eFmtHex);
                            g_Log.Push(hr, CEventLog::eFmtSysErr);
                            g_Log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_POLICY_CREATEQUOTACONTROL);
                        }
                    }
                    pcf = NULL;  // This releases pcf.
                }
                else
                {
                    DBGERROR((TEXT("CoCreateInstance failed with error 0x%08X"), hr));
                    g_Log.Push(hr, CEventLog::eFmtHex);
                    g_Log.Push(hr, CEventLog::eFmtSysErr);
                    g_Log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_POLICY_CREATECLASSFACTORY);
                }
            }
            else
            {
                DBGERROR((TEXT("Error 0x%08X getting drive name list"), hr));
                g_Log.Push(hr, CEventLog::eFmtHex);
                g_Log.Push(hr, CEventLog::eFmtSysErr);
                g_Log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_POLICY_GETDRIVELIST);
            }

            if (m_bVerboseEventLog && 0 < strVolCompleted.Length())
            {
                //
                // Log successful completions by listing volumes
                // and applied policy values.
                //
                CString s;
                LONGLONG llValue;
                g_Log.Push(strVolCompleted);
                g_Log.Push(!DISKQUOTA_IS_DISABLED(pInfo->dwQuotaState));
                g_Log.Push(DISKQUOTA_IS_ENFORCED(pInfo->dwQuotaState));

                llValue = pInfo->llDefaultQuotaThreshold;
                if (LONGLONG(-1) != llValue)
                {
                    XBytes::FormatByteCountForDisplay(llValue, s.GetBuffer(40), 40);
                    s.ReleaseBuffer();
                }
                else
                {
                    s.Format(g_hInstDll, IDS_NO_LIMIT);
                }
                g_Log.Push(s);
                llValue = pInfo->llDefaultQuotaLimit;
                if (LONGLONG(-1) != llValue)
                {
                    XBytes::FormatByteCountForDisplay(llValue, s.GetBuffer(40), 40);
                    s.ReleaseBuffer();
                }
                else
                {
                    s.Format(g_hInstDll, IDS_NO_LIMIT);
                }
                g_Log.Push(s);
                g_Log.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, MSG_I_POLICY_FINISHED);
            }
        }
    }
    catch(CAllocException& e)
    {
        DBGERROR((TEXT("Insufficient memory.")));
        hr = E_OUTOFMEMORY;
        g_Log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_POLICY_OUTOFMEMORY);
    }
    catch(...)
    {
        DBGERROR((TEXT("C++ exception caught.")));
        hr = E_UNEXPECTED;
        g_Log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_POLICY_EXCEPTION);
    }

    if (bAborted)
    {
        g_Log.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, MSG_I_POLICY_ABORTED);
    }

    if (bCoUninitialize)
        CoUninitialize();

    return hr;
}

//
// Build a list of drives to which policy can be applied.
//
HRESULT
CDiskQuotaPolicy::GetDriveNames(   // [ static ]
    CArray<CString> *prgstrDrives,    // Output array of name strings.
    bool bRemovableMedia              // Include removable media?
    )
{
    DBGTRACE((DM_POLICY, DL_MID, TEXT("CDiskQuotaPolicy::GetDriveNames")));
    DBGASSERT((NULL != prgstrDrives));
    HRESULT hr = NOERROR;

    //
    // Get buffer size required to hold drive name strings.
    //
    int cch = GetLogicalDriveStrings(0, NULL);
    //
    // Allocate buffer and get the strings.
    //
    array_autoptr<TCHAR> ptrDrives(new TCHAR[cch + 1]);
    if (0 < GetLogicalDriveStrings(cch, ptrDrives.get()))
    {
        //
        // Iterate over all of the drive name strings.  Append to the 
        // string array each that can accept policy.
        //
        DblNulTermListIter iter(ptrDrives.get());
        LPCTSTR pszDrive;
        while(iter.Next(&pszDrive))
        {
            if (S_OK == OkToApplyPolicy(pszDrive, bRemovableMedia))
            {
                prgstrDrives->Append(CString(pszDrive));
            }
        }
    }
    else
    {
        DWORD dwErr = GetLastError();
        DBGERROR((TEXT("GetLogicalDriveStrings failed with error %d"), dwErr));
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    return hr;
}


//
// Returns:  S_OK    = OK to set policy on drive.
//           S_FALSE = Not OK to set policy
//           Other   = Error occured.  Not OK to set policy.
//
HRESULT
CDiskQuotaPolicy::OkToApplyPolicy(  // [ static ]
    LPCTSTR pszDrive,       // Drive (volume) name string.
    bool bRemovableMedia    // Include removable media?
    )
{
    DBGTRACE((DM_POLICY, DL_LOW, TEXT("CDiskQuotaPolicy::OkToApplyPolicy")));
    HRESULT hr = S_FALSE;
    //
    // Primary filter is drive type.
    //
    UINT uDriveType = GetDriveType(pszDrive);
    switch(uDriveType)
    {
        case DRIVE_UNKNOWN:
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("\"%s\" is UNKNOWN"), pszDrive));
            return S_FALSE;

        case DRIVE_NO_ROOT_DIR:
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("\"%s\" has no root dir"), pszDrive));
            return S_FALSE;

        case DRIVE_REMOTE:
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("\"%s\" is REMOTE"), pszDrive));
            return S_FALSE;

        case DRIVE_CDROM:
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("\"%s\" is CDROM"), pszDrive));
            return S_FALSE;

        case DRIVE_RAMDISK:
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("\"%s\" is RAMDISK"), pszDrive));
            return S_FALSE;

        case DRIVE_REMOVABLE:
            //
            // Removable is allowable if policy says it is.  It should be
            // disallowed by default since using disk quota on removable media
            // doesn't make a lot of sense in most situations.
            //
            if (!bRemovableMedia)
            {
                DBGPRINT((DM_POLICY, DL_LOW, TEXT("\"%s\" is REMOVABLE"), pszDrive));
                return S_FALSE;
            }
            //
            // Fall through...
            //
        case DRIVE_FIXED:
            //
            // Fixed drives are always acceptable.
            //
            break;

        default:
            DBGERROR((TEXT("Unknown drive type %d for \"%s\""), uDriveType, pszDrive));
            return S_FALSE;
    }

    //
    // Next filter is support for NTFS quotas.  We do the drive-type check first
    // because it doesn't require hitting the disks.  GetVolumeInformation does
    // hit the disk so we only want to do it if necessary.
    //
    DWORD dwFlags = 0;
    if (GetVolumeInformation(pszDrive, NULL, 0, NULL, NULL, &dwFlags, NULL, 0))
    {
        if (FILE_VOLUME_QUOTAS & dwFlags)
        {
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("Ok to set policy on \"%s\""), pszDrive));
            hr = S_OK;
        }
        else
        {
            DBGPRINT((DM_POLICY, DL_LOW, TEXT("\"%s\" doesn't support NTFS quotas"), pszDrive));
        }
    }
    else
    {
        DWORD dwErr = GetLastError();
        DBGERROR((TEXT("GetVolumeInformation failed with error %d for \"%s\""), 
                 dwErr, pszDrive));
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    return hr;
}
