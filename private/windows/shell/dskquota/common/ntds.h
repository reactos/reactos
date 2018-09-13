#ifndef _INC_DSKQUOTA_NTDS_H
#define _INC_DSKQUOTA_NTDS_H
///////////////////////////////////////////////////////////////////////////////
/*  File: ntds.h

    Description: Contains declaration for class NTDS.
        This class provides a simple wrapper around NT Directory Service
        name translation features.  It has no data and no virtual functions.


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    06/01/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
#ifndef __SSPI_H__
#   define SECURITY_WIN32
#   include <security.h>        // For TranslateName
#endif


class NTDS
{
    public:
        NTDS(VOID) { }
        ~NTDS(VOID) { }

        HRESULT LookupAccountByName(
                        LPCTSTR pszSystem,
                        LPCTSTR pszLogonName,
                        CString *pstrContainerName,
                        CString *pstrDisplayName,
                        PSID    pSid,
                        LPDWORD pdwSid,
                        PSID_NAME_USE peUse);

        HRESULT LookupAccountBySid(
                        LPCTSTR pszSystem,
                        PSID    pSid,
                        CString *pstrContainerName,
                        CString *pstrLogonName,
                        CString *pstrDisplayName,
                        PSID_NAME_USE peUse);


        HRESULT TranslateFQDNsToLogonNames(
                        const CArray<CString>& rgstrFQDNs,
                        CArray<CString> *prgstrLogonNames);

        HRESULT TranslateFQDNToLogonName(
                        LPCTSTR pszFQDN,
                        CString *pstrLogonName);

        static void CreateSamLogonName(LPCTSTR pszSamDomain, LPCTSTR pszSamUser, CString *pstrSamLogonName);
        static LPCTSTR FindFQDNInADsPath(LPCTSTR pszADsPath);
        static LPCTSTR FindSamAccountInADsPath(LPCTSTR pszADsPath);


    private:
        HRESULT LookupSamAccountName(
                        LPCTSTR pszSystem,
                        LPCTSTR pszLogonName,
                        CString *pstrContainerName,
                        CString *pstrDisplayName,
                        PSID    pSid,
                        LPDWORD pdwSid,
                        PSID_NAME_USE peUse);

        HRESULT LookupDsAccountName(
                        LPCTSTR pszSystem,
                        LPCTSTR pszLogonName,
                        CString *pstrContainerName,
                        CString *pstrDisplayName,
                        PSID    pSid,
                        LPDWORD pdwSid,
                        PSID_NAME_USE peUse);


        HRESULT GetSamAccountDisplayName(
                        LPCTSTR pszLogonName,
                        CString *pstrDisplayName);


        HRESULT GetDsAccountDisplayName(
                        LPCTSTR pszLogonName,
                        CString *pstrDisplayName);

        HRESULT TranslateNameInternal(
                        LPCTSTR pszAccountName,
                        EXTENDED_NAME_FORMAT AccountNameFormat,
                        EXTENDED_NAME_FORMAT DesiredNameFormat,
                        CString *pstrTranslatedName);

        HRESULT LookupAccountNameInternal(
                        LPCTSTR pszSystemName,
                        LPCTSTR pszAccountName,
                        PSID Sid,
                        LPDWORD pcbSid,
                        CString *pstrReferencedDomainName,
                        PSID_NAME_USE peUse);

        HRESULT LookupAccountSidInternal(
                        LPCTSTR pszSystemName,
                        PSID Sid,
                        CString *pstrName,
                        CString *pstrReferencedDomainName,
                        PSID_NAME_USE peUse);
};

#endif // _INC_DSKQUOTA_NTDS_H
