///////////////////////////////////////////////////////////////////////////////
/*  File: ntds.cpp

    Description: Contains definition for class NTDS.
        This class provides a simple wrapper around NT Directory Service
        name translation features.  Currently, the Win32 functions to perform
        DS-sensitive name-to-SID translations are not present.  These functions
        provide the same functionality.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/01/97    Initial creation.                                    BrianAu
    03/20/98    Reworked to use TranslateName rather than a combo    BrianAu
                of DsBind and DsCrackNames.  This ensures we're
                getting the proper info from the DS.  It's slower
                because we have to re-bind to the DS for each call
                but I'd rather do that than bind incorrectly and
                not get the proper name information.
*/
///////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#pragma hdrstop

#include <lm.h>        // For NetUserGetInfo and NetGetDCName.
#include "ntds.h"


//
// BUGBUG:  These DS_NAME_FORMAT codes (ntdsapi.h> are not yet in the 
//          corresponding EXTENDED_NAME_FORMAT enumeration in sspi.h.
//          Since TranslateName passes these codes on directly to DsCrackNames
//          I've defined these here so I can get the latest behavior until
//          Richard Ward updates TranslateNames and sspi.h.
//          Once he's updated that header, you can delete these three consts
//          and remove the "SSPI_" prefix from where they're used in the 
//          code.  [brianau - 3/19/98]
//
#define SSPI_NameUserPrincipal    ((EXTENDED_NAME_FORMAT)8)
#define SSPI_NameCanonicalEx      ((EXTENDED_NAME_FORMAT)9)
#define SSPI_NameServicePrincipal ((EXTENDED_NAME_FORMAT)10)


//
// Given an account name, find the account's SID and optionally the
// account's container and display names.
// The logon name may be either a DS "user principal" name or an
// NT4-style SAM-compatible name.
//
// DS UPN =         "brianau@microsoft.com"
// SAM compatible = "REDMOND\brianau"
//
HRESULT
NTDS::LookupAccountByName(
    LPCTSTR pszSystem,          // IN - optional.  Can be NULL.
    LPCTSTR pszLogonName,       // IN - "REDMOND\brianau" or "brianau@microsoft.com"
    CString *pstrContainerName, // OUT - optional.
    CString *pstrDisplayName,   // OUT - optional.  Can be NULL.
    PSID    pSid,               // OUT
    LPDWORD pdwSid,             // IN/OUT
    PSID_NAME_USE peUse         // OUT
    )
{
    DBGTRACE((DM_NTDS, DL_HIGH, TEXT("NTDS::LookupAccountByName")));
    DBGASSERT((NULL != pszLogonName));
    DBGASSERT((NULL != pSid));
    DBGASSERT((NULL != pdwSid));
    DBGASSERT((NULL != peUse));
    DBGPRINT((DM_NTDS, DL_HIGH, TEXT("Lookup \"%s\""), pszLogonName));

    HRESULT hr = NOERROR;

    //
    // Assume the presence of a '@' character means it's a UPN.
    //
    if (NULL != StrChr(pszLogonName, TEXT('@')))
    {
        hr = LookupDsAccountName(pszSystem,
                                 pszLogonName,
                                 pstrContainerName,
                                 pstrDisplayName,
                                 pSid,
                                 pdwSid,
                                 peUse);
    }
    else
    {
        hr = LookupSamAccountName(pszSystem,
                                  pszLogonName,
                                  pstrContainerName,
                                  pstrDisplayName,
                                  pSid,
                                  pdwSid,
                                  peUse);
    }
    return hr;
}


//
// Given an account SID, optionally find the account's logon name,
// container name and display name.  If a DS UPN is available for the
// user, the container name will be the canonical path to the user
// object and the display name will come from the DS.  If a
// DS UPN is not available, or the account is an NT4 account,
// the container returned is the NT4 domain name and the display name
// is retrieved using NetUserGetInfo.
//
HRESULT
NTDS::LookupAccountBySid(
    LPCTSTR pszSystem,           // optional.  Can be NULL.
    PSID    pSid,
    CString *pstrContainerName,  // optional.  Can be NULL.
    CString *pstrLogonName,      // optional.  Can be NULL.
    CString *pstrDisplayName,    // optional.  Can be NULL.
    PSID_NAME_USE peUse
    )
{
    DBGTRACE((DM_NTDS, DL_HIGH, TEXT("NTDS::LookupAccountBySid")));
    DBGASSERT((NULL != pSid));

    HRESULT hr = NOERROR;
    CString strSamUser;
    CString strSamDomain;
    CString strSamLogonName;

    //
    // Get the SAM-compatible domain\user name for the SID.
    //
    DBGPRINT((DM_NTDS, DL_LOW, TEXT("Calling ::LookupAccountSid")));
    hr = LookupAccountSidInternal(pszSystem,
                                  pSid,
                                  &strSamUser,
                                  &strSamDomain,
                                  peUse);

    if (FAILED(hr))
        return hr;

    //
    // No need to go further if caller doesn't want any name information in which
    // case all they're getting in return is an indication if the SID is for a known
    // account or not.
    //
    if (NULL != pstrLogonName || NULL != pstrContainerName || NULL != pstrDisplayName)
    {
        CString strFQDN;
        bool bUseSamCompatibleInfo = false;
        CreateSamLogonName(strSamDomain, strSamUser, &strSamLogonName);

        //
        // Start by getting the FQDN.  Cracking is most efficient when the
        // FQDN is the starting point.
        //
        if (FAILED(TranslateNameInternal(strSamLogonName,
                                         NameSamCompatible,
                                         NameFullyQualifiedDN,
                                         &strFQDN)))
        {
            //
            // No FQDN available for this account.  Must be an NT4
            // account.  Return SAM-compatible info to the caller.
            //
            bUseSamCompatibleInfo = true;
        }
        if (NULL != pstrLogonName)
        {
            if (bUseSamCompatibleInfo)
            {
                *pstrLogonName = strSamLogonName;
            }
            else
            {
                //
                // Get the DS user principal name
                //
                pstrLogonName->Empty();
                if (FAILED(TranslateNameInternal(strFQDN,
                                                 NameFullyQualifiedDN,
                                                 SSPI_NameUserPrincipal,
                                                 pstrLogonName)))
                {
                    //
                    // No UPN for this account.
                    // Default to returning SAM-compatible info.
                    //
                    bUseSamCompatibleInfo = true;
                    *pstrLogonName = strSamLogonName;
                }
            }
        }

        if (NULL != pstrContainerName)
        {
            if (bUseSamCompatibleInfo)
            {
                *pstrContainerName = strSamDomain;
            }
            else
            {
                pstrContainerName->Empty();
                if (SUCCEEDED(TranslateNameInternal(strFQDN,
                                                    NameFullyQualifiedDN,
                                                    NameCanonical,
                                                    pstrContainerName)))
                {
                    //
                    // Trim off the trailing account name from the canonical path
                    // so we're left with only the container name.
                    //
                    int iLastBS = pstrContainerName->Last(TEXT('/'));
                    if (-1 != iLastBS)
                    {
                        *pstrContainerName = pstrContainerName->SubString(0, iLastBS);
                    }
                }
            }
        }

        if (NULL != pstrDisplayName)
        {
            if (bUseSamCompatibleInfo || FAILED(GetDsAccountDisplayName(strFQDN, pstrDisplayName)))
            {
                GetSamAccountDisplayName(strSamLogonName, pstrDisplayName);
            }
        }
    }
    return hr;
}



//
// Input is a SAM-compatible account name.
// Retrieve the name information using the NT4-style methods.
// 
HRESULT
NTDS::LookupSamAccountName(
    LPCTSTR pszSystem,
    LPCTSTR pszLogonName,       // IN - "REDMOND\brianau"
    CString *pstrContainerName, // OUT - optional.
    CString *pstrDisplayName,   // OUT - optional.  Can be NULL.
    PSID    pSid,               // OUT
    LPDWORD pdwSid,             // IN/OUT
    PSID_NAME_USE peUse         // OUT
    )
{
    DBGTRACE((DM_NTDS, DL_MID, TEXT("NTDS::LookupSamAccountName")));
    DBGASSERT((NULL != pszLogonName));
    DBGASSERT((NULL != pdwSid));
    DBGASSERT((NULL != pSid));
    DBGASSERT((NULL != peUse));
    //
    // Get the SID using the SAM-compatible account name.
    //
    HRESULT hr = NOERROR;
    CString strDomain;
    hr = LookupAccountNameInternal(pszSystem,
                                   pszLogonName,
                                   pSid,
                                   pdwSid,
                                   &strDomain,
                                   peUse);
    if (SUCCEEDED(hr))
    {
        if (NULL != pstrContainerName)
            *pstrContainerName = strDomain;

        if (NULL != pstrDisplayName)
            GetSamAccountDisplayName(pszLogonName, pstrDisplayName);
    }
    return hr;
}



//
// Returns:
//    S_OK      = All information retrieved.
//    S_FALSE   = Container name returned is for SAM-compatible account.  
//                DS container information was not available.
HRESULT
NTDS::LookupDsAccountName(
    LPCTSTR pszSystem,
    LPCTSTR pszLogonName,       // IN - "brianau@microsoft.com"
    CString *pstrContainerName, // OUT - optional.
    CString *pstrDisplayName,   // OUT - optional.  Can be NULL.
    PSID    pSid,               // OUT
    LPDWORD pdwSid,             // IN/OUT
    PSID_NAME_USE peUse         // OUT
    )
{
    DBGTRACE((DM_NTDS, DL_MID, TEXT("NTDS::LookupDsAccountName")));
    DBGASSERT((NULL != pszLogonName));
    DBGASSERT((NULL != pSid));
    DBGASSERT((NULL != pdwSid));
    DBGASSERT((NULL != peUse));
    //
    // Get the SID using the SAM-compatible account name.
    //
    HRESULT hr = S_OK;

    //
    // Translate the DS user principal name to FQDN format.
    // Starting with FQDN is the most efficient for name cracking so
    // we get it once and use it multiple times.
    //
    CString strFQDN;
    hr = TranslateNameInternal(pszLogonName,
                               SSPI_NameUserPrincipal,
                               NameFullyQualifiedDN,
                               &strFQDN);
    if (FAILED(hr))
        return hr;


    CString strSamLogonName;
    hr = TranslateNameInternal(strFQDN,
                               NameFullyQualifiedDN,
                               NameSamCompatible,
                               &strSamLogonName);
    if (FAILED(hr))
        return hr;


    CString strDomain;
    hr = LookupAccountNameInternal(pszSystem,
                                   strSamLogonName,
                                   pSid,
                                   pdwSid,
                                   &strDomain,
                                   peUse);
    if (FAILED(hr))
        return hr;

    bool bUseSamCompatibleInfo = false;
    if (NULL != pstrContainerName)
    {
        //
        // Get the DS container name for the account.
        //
        hr = TranslateNameInternal(strFQDN,
                                   NameFullyQualifiedDN,
                                   NameCanonical,
                                   pstrContainerName);

        if (SUCCEEDED(hr))
        {
            //
            // Trim off the trailing account name from the canonical path
            // so we're left with only the container name.
            //
            int iLastBS = pstrContainerName->Last(TEXT('/'));
            if (-1 != iLastBS)
            {
                *pstrContainerName = pstrContainerName->SubString(0, iLastBS);
            }
        }
        else
        {
            DBGERROR((TEXT("Using SAM-compatible name info")));
            //
            // Can't get DS container name so use the SAM domain name.
            //
            *pstrContainerName = strDomain;
            bUseSamCompatibleInfo = true;
            hr = S_FALSE;
        }
    }
    if (NULL != pstrDisplayName)
    {
        if (bUseSamCompatibleInfo || FAILED(GetDsAccountDisplayName(strFQDN, pstrDisplayName)))
            GetSamAccountDisplayName(strSamLogonName, pstrDisplayName);
    }
    return hr;
}



HRESULT
NTDS::GetSamAccountDisplayName(
    LPCTSTR pszLogonName,
    CString *pstrDisplayName
    )
{
    DBGTRACE((DM_NTDS, DL_MID, TEXT("NTDS::GetSamAccountDisplayName")));
    DBGASSERT((NULL != pszLogonName));
    DBGASSERT((NULL != pstrDisplayName));
    DBGPRINT((DM_NTDS, DL_MID, TEXT("Translating \"%s\""), pszLogonName));

    HRESULT hr             = E_FAIL;
    LPTSTR pszComputerName = NULL;
    NET_API_STATUS status  = NERR_Success;
    CString strLogonName(pszLogonName);
    CString strDomain;
    CString strUser;
    //
    // Separate the domain\account string into two separate strings.
    //
    int iBackslash = strLogonName.Last(TEXT('\\'));
    if (-1 != iBackslash)
    {
        strDomain = strLogonName.SubString(0, iBackslash);
        if (iBackslash < (strLogonName.Length() - 1))
            strUser = strLogonName.SubString(iBackslash + 1);
    }

    pstrDisplayName->Empty();
    DBGPRINT((DM_NTDS, DL_LOW, TEXT("Calling ::NetGetDCName for domain \"%s\""), strDomain.Cstr()));
    status = ::NetGetDCName(NULL, strDomain, (LPBYTE *)&pszComputerName);
    if (NERR_Success == status || NERR_DCNotFound == status)
    {
        struct _USER_INFO_2 *pui = NULL;

        DBGPRINT((DM_NTDS, DL_LOW, TEXT("Calling ::NetGetUserInfo for \"%s\" on \"%s\""), strUser.Cstr(), pszComputerName));
        status = ::NetUserGetInfo(pszComputerName, strUser, 2, (LPBYTE *)&pui);
        if (NERR_Success == status)
        {
            *pstrDisplayName = pui->usri2_full_name;
            DBGPRINT((DM_NTDS, DL_LOW, TEXT("Translated to \"%s\""), pstrDisplayName->Cstr()));
            NetApiBufferFree(pui);
            hr = NOERROR;
        }
        else
        {
            DBGERROR((TEXT("NetUserGetInfo failed with error 0x%08X for \"%s\" on \"%s\""), 
                      status, strUser.Cstr(), pszComputerName ? pszComputerName : TEXT("local machine")));
            hr = HRESULT_FROM_WIN32(status);
        }
        if (NULL != pszComputerName)
            NetApiBufferFree(pszComputerName);
    }
    else
    {
        DBGERROR((TEXT("NetGetDCName failed with error 0x%08X for domain \"%s\""), 
                  status, strDomain.Cstr()));
        hr = HRESULT_FROM_WIN32(status);
    }
    return hr;
}

                 
HRESULT
NTDS::GetDsAccountDisplayName(
    LPCTSTR pszFQDN,
    CString *pstrDisplayName
    )
{
    DBGTRACE((DM_NTDS, DL_MID, TEXT("NTDS::GetDsAccountDisplayName")));
    DBGASSERT((NULL != pszFQDN));
    DBGASSERT((NULL != pstrDisplayName));

    //
    // Get the DS container name for the account.
    //
    pstrDisplayName->Empty();
    return TranslateNameInternal(pszFQDN,
                                 NameFullyQualifiedDN,
                                 NameDisplay,
                                 pstrDisplayName);
}



void
NTDS::CreateSamLogonName(
    LPCTSTR pszSamDomain,
    LPCTSTR pszSamUser,
    CString *pstrSamLogonName
    )
{
    DBGTRACE((DM_NTDS, DL_LOW, TEXT("NTDS::CreateSamLogonName")));
    DBGASSERT((NULL != pszSamDomain));
    DBGASSERT((NULL != pszSamUser));
    DBGASSERT((NULL != pstrSamLogonName));
    DBGPRINT((DM_NTDS, DL_LOW, TEXT("\tDomain.: \"%s\""), pszSamDomain));
    DBGPRINT((DM_NTDS, DL_LOW, TEXT("\tUser...: \"%s\""), pszSamUser));

    pstrSamLogonName->Format(TEXT("%1\\%2"), pszSamDomain, pszSamUser);

    DBGPRINT((DM_NTDS, DL_LOW, TEXT("\tAccount: \"%s\""), pstrSamLogonName->Cstr()));
}



HRESULT 
NTDS::TranslateFQDNsToLogonNames(
    const CArray<CString>& rgstrFQDNs, 
    CArray<CString> *prgstrLogonNames
    )
{
    HRESULT hr = NOERROR;
    prgstrLogonNames->Clear();
    int cItems = rgstrFQDNs.Count();
    CString strLogonName;
    for (int i = 0; i < cItems; i++)
    {
        if (FAILED(TranslateFQDNToLogonName(rgstrFQDNs[i], &strLogonName)))
            strLogonName.Empty();

        prgstrLogonNames->Append(strLogonName);
    }
    return hr;
}



HRESULT 
NTDS::TranslateFQDNToLogonName(
    LPCTSTR pszFQDN,
    CString *pstrLogonName
    )
{
    DBGTRACE((DM_NTDS, DL_MID, TEXT("NTDS::TranslateFQDNToLogonName")));
    DBGASSERT((NULL != pszFQDN));
    DBGASSERT((NULL != pstrLogonName));

    HRESULT hr = NOERROR;
    hr = TranslateNameInternal(pszFQDN,
                               NameFullyQualifiedDN,
                               SSPI_NameUserPrincipal,
                               pstrLogonName);
    if (FAILED(hr))
    {
        hr = TranslateNameInternal(pszFQDN,
                                   NameFullyQualifiedDN,
                                   NameSamCompatible,
                                   pstrLogonName);
    }
    return hr;
}


LPCTSTR 
NTDS::FindFQDNInADsPath(
    LPCTSTR pszADsPath
    )
{
    DBGTRACE((DM_NTDS, DL_MID, TEXT("NTDS::FindFQDNInADsPath")));
    DBGASSERT((NULL != pszADsPath));
    DBGPRINT((DM_NTDS, DL_MID, TEXT("Checking \"%s\""), pszADsPath));
    const TCHAR szCN[] = TEXT("CN=");
    while(*pszADsPath && CSTR_EQUAL != CompareString(LOCALE_USER_DEFAULT,
                                                     0,
                                                     pszADsPath,
                                                     ARRAYSIZE(szCN) - 1,
                                                     szCN,
                                                     ARRAYSIZE(szCN) - 1))
    {
        pszADsPath = CharNext(pszADsPath);
    }
    DBGPRINT((DM_NTDS, DL_MID, TEXT("Found \"%s\""), pszADsPath ? pszADsPath : TEXT("<null>")));
    return (*pszADsPath ? pszADsPath : NULL);
}


LPCTSTR 
NTDS::FindSamAccountInADsPath(
    LPCTSTR pszADsPath
    )
{
    DBGTRACE((DM_NTDS, DL_MID, TEXT("NTDS::FindSamAccountInADsPath")));
    DBGASSERT((NULL != pszADsPath));
    DBGPRINT((DM_NTDS, DL_MID, TEXT("Checking \"%s\""), pszADsPath));
    const TCHAR szPrefix[] = TEXT("WinNT://");
    if (0 == StrCmpN(pszADsPath, szPrefix, ARRAYSIZE(szPrefix)-1))
    {
        pszADsPath += (ARRAYSIZE(szPrefix) - 1);
        DBGPRINT((DM_NTDS, DL_MID, TEXT("Found \"%s\""), pszADsPath));
    }
    else
    {
        pszADsPath = NULL;
    }

    return pszADsPath;
}


//
// Wrapper around sspi's TranslateName that automatically handles
// the buffer sizing using a CString object.
//
HRESULT
NTDS::TranslateNameInternal(
    LPCTSTR pszAccountName,
    EXTENDED_NAME_FORMAT AccountNameFormat,
    EXTENDED_NAME_FORMAT DesiredNameFormat,
    CString *pstrTranslatedName
    )
{
#if DBG
    //
    // These match up with the EXTENDED_NAME_FORMAT enumeration.
    // They're for debugger output only.
    //
    static const LPCTSTR rgpszFmt[] = { 
                                TEXT("NameUnknown"),
                                TEXT("FullyQualifiedDN"),
                                TEXT("NameSamCompatible"),
                                TEXT("NameDisplay"),
                                TEXT("NameDomainSimple"),
                                TEXT("NameEnterpriseSimple"),
                                TEXT("NameUniqueId"),
                                TEXT("NameCanonical"),
                                TEXT("NameUserPrincipal"),
                                TEXT("NameCanonicalEx"),
                                TEXT("NameServicePrincipal") };
#endif // DBG

    DBGPRINT((DM_NTDS, DL_LOW, TEXT("Calling TranslateName for \"%s\""), pszAccountName));
    DBGPRINT((DM_NTDS, DL_LOW, TEXT("Translating %s -> %s"), 
              rgpszFmt[AccountNameFormat], rgpszFmt[DesiredNameFormat]));

    HRESULT hr = NOERROR;
    //
    // BUGBUG:  TranslateName doesn't properly set the required buffer size
    //          in cchTrans if the buffer size is too small.  I've notified
    //          Richard B. Ward about it.  Says he'll have the fix in
    //          on 3/24/98.  Should test with an initial value of 1
    //          just to make sure he fixed it.  [brianau - 03/20/98]
    //
    //
    // cchTrans is static so that if a particular installation's
    // account names are really long, we'll not be resizing the
    // buffer for each account.
    //
    static ULONG cchTrans = MAX_PATH;

    while(!::TranslateName(pszAccountName,
                           AccountNameFormat,
                           DesiredNameFormat,
                           pstrTranslatedName->GetBuffer(cchTrans),
                           &cchTrans))
    {
        DWORD dwErr = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != dwErr)
        {
            DBGERROR((TEXT("::TranslateName failed with error %d"), dwErr));
            hr = HRESULT_FROM_WIN32(dwErr);
            break;
        }
        DBGPRINT((DM_NTDS, DL_LOW, TEXT("Resizing buffer to %d chars"), cchTrans));
    }
    pstrTranslatedName->ReleaseBuffer();
    return hr;
}


//
// Wrapper around Win32's LookupAccountName that automatically handles
// the domain buffer sizing using a CString object.
//
HRESULT
NTDS::LookupAccountNameInternal(
    LPCTSTR pszSystemName,
    LPCTSTR pszAccountName,
    PSID pSid,
    LPDWORD pcbSid,
    CString *pstrReferencedDomainName,
    PSID_NAME_USE peUse
    )
{
    DBGPRINT((DM_NTDS, DL_MID, TEXT("Calling ::LookupAccountName for \"%s\" on \"%s\""),
              pszAccountName, pszSystemName ? pszSystemName : TEXT("<local system>")));

    HRESULT hr = NOERROR;
    //
    // cchDomain is static so that if a particular installation's
    // account names are really long, we'll not be resizing the
    // buffer for each account.
    //
    static ULONG cchDomain = MAX_PATH;

    while(!::LookupAccountName(pszSystemName,
                               pszAccountName,
                               pSid,
                               pcbSid,
                               pstrReferencedDomainName->GetBuffer(cchDomain),
                               &cchDomain,
                               peUse))
    {
        DWORD dwErr = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != dwErr)
        {
            DBGERROR((TEXT("::LookupAccountName failed with error %d"), dwErr));
            hr = HRESULT_FROM_WIN32(dwErr);
            break;
        }
        DBGPRINT((DM_NTDS, DL_LOW, TEXT("Resizing domain buffer to %d chars"), cchDomain));
    }
    pstrReferencedDomainName->ReleaseBuffer();
    return hr;
}
 
//
// Wrapper around Win32's LookupAccountSid that automatically handles
// the domain buffer sizing using a CString object.
//
HRESULT
NTDS::LookupAccountSidInternal(
    LPCTSTR pszSystemName,
    PSID pSid,
    CString *pstrName,
    CString *pstrReferencedDomainName,
    PSID_NAME_USE peUse
    )
{
    HRESULT hr = NOERROR;
    //
    // These are static so that if a particular installation's
    // account names are really long, we'll not be resizing the
    // buffer for each account.
    //
    static ULONG cchName   = MAX_PATH;
    static ULONG cchDomain = MAX_PATH;

    while(!::LookupAccountSid(pszSystemName,
                              pSid,
                              pstrName->GetBuffer(cchName),
                              &cchName,
                              pstrReferencedDomainName->GetBuffer(cchDomain),
                              &cchDomain,
                              peUse))
    {
        DWORD dwErr = GetLastError();
        if (ERROR_INSUFFICIENT_BUFFER != dwErr)
        {
            DBGERROR((TEXT("::LookupAccountSid failed with error %d"), dwErr));
            hr = HRESULT_FROM_WIN32(dwErr);
            break;
        }
        DBGPRINT((DM_NTDS, DL_LOW, TEXT("Resizing domain or name buffer")));
    }
    pstrName->ReleaseBuffer();
    pstrReferencedDomainName->ReleaseBuffer();
    return hr;
}


