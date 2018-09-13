/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    schnlui.cxx

Abstract:

    Contains immplimentation of generic Windows Dialog
    Manipulation Code.  This code supports SCHANNEL
    (Secure Channel SSL/PCT) specific UI for Certifcates.

    Contents:
        CertPickDialogProc

Author:

    Arthur L Bierer (arthurbi) 27-Jun-1996

Revision History:

    27-Jun-1996 arthurbi
        Created

--*/

#include <wininetp.h>
#include <ntsecapi.h>
#include "resource.h"
#include "ierrui.hxx"
#include "inethelp.h"
#include <softpub.h>
#include <htmlhelp.h>

#define USE_NT5_CRYPTOUI
#ifdef USE_NT5_CRYPTOUI
#include <cryptui.h>
HINSTANCE g_hCryptoUI = NULL;   // handle for cryptui.dll
#endif

//
// private prototypes, and defines.
//

#define TYPICAL_MD5_HASH_SIZE 16

#define NUM_DN_UNITS    6
#define MAX_ITEM_LEN    1000

#define DN_COMMON_NAME  0
#define DN_COUNTRY      1
#define DN_ORG          2
#define DN_ORGUNIT      3
#define DN_LOCALE       4
#define DN_STATE        5

#define MAX_CERT_FIELDS 20

typedef struct _ATTR_MAP
{
    DWORD dwAttr;
    DWORD dwStringID;
} ATTR_MAP;

// Now for some common attribute maps


ATTR_MAP ProtocolAttrMap[] =
{
    {SP_PROT_SSL2_CLIENT, IDS_PROTOCOL_SSL2},
    {SP_PROT_SSL3_CLIENT, IDS_PROTOCOL_SSL3},
    {SP_PROT_PCT1_CLIENT, IDS_PROTOCOL_PCT1},
    {SP_PROT_TLS1_CLIENT, IDS_PROTOCOL_TLS1}
};

ATTR_MAP AlgAttrMap[] =
{
    {CALG_MD2, IDS_ALG_MD2},
    {CALG_MD4, IDS_ALG_MD4},
    {CALG_MD5, IDS_ALG_MD5},
    {CALG_SHA, IDS_ALG_SHA},
    {CALG_SHA1, IDS_ALG_SHA},
    {CALG_MAC, IDS_ALG_MAC},
    {CALG_HMAC, IDS_ALG_HMAC},
    {CALG_RSA_SIGN, IDS_ALG_RSA_SIGN},
    {CALG_DSS_SIGN, IDS_ALG_DSS_SIGN},
    {CALG_RSA_KEYX, IDS_ALG_RSA_KEYX},
    {CALG_DES, IDS_ALG_DES},
    {CALG_3DES_112, IDS_ALG_3DES_112},
    {CALG_3DES, IDS_ALG_3DES},
    {CALG_RC2, IDS_ALG_RC2},
    {CALG_RC4, IDS_ALG_RC4},
    {CALG_RC5, IDS_ALG_RC5},
    {CALG_SEAL, IDS_ALG_SEAL},
    {CALG_DH_SF, IDS_ALG_DH_SF},
    {CALG_DH_EPHEM, IDS_ALG_DH_EPHEM},
    {CALG_KEA_KEYX, IDS_ALG_KEA_KEYX},
    {CALG_SKIPJACK, IDS_ALG_SKIPJACK},
    {CALG_TEK, IDS_ALG_TEK}
};





typedef struct {
    LPWSTR lpszListBoxText;
    LPWSTR lpszEditBoxText;
    DWORD dwSpcCtlId;       // special id for item to be placed in ctl.
} ShowCertMapping;


#define szHelpFile "iexplore.hlp"




//
// private function declariations
//



PRIVATE
BOOL
PlaceCertContextsInListBox(
    IN HWND hWndListBox,
    IN HWND hWndViewCertButton,
    IN HWND hWndExportButton,
    IN CERT_CONTEXT_ARRAY* pCertContexts
    );

PRIVATE
BOOL
PlaceCertificateDataIntoListBox(
    IN HWND hWndDlg,
    IN HWND hWndListBox,
    IN ShowCertMapping *pMapCertFields
    );

PRIVATE
DWORD
OnSelectionOfACertField(
    IN HWND hWndListBox,
    IN HWND hWndEditBox,
    IN ShowCertMapping *pMapCertFields
    );

PRIVATE
BOOL
CALLBACK
ViewCertDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    );

LPWSTR  GetCertStatus(LPINTERNET_SECURITY_INFO pciCert);
//
// public functions
//

#ifdef USE_NT5_CRYPTOUI
//+-------------------------------------------------------------------------
// Returns TRUE if cryptui.dll was found
//--------------------------------------------------------------------------
BOOL UseCryptoUI()
{
    static long fTriedOnce = FALSE;

    // Only try to load the dll once
    if (!fTriedOnce)
    {
        //
        // Note: if this gets called by multiple threads the worst that will
        // happen is that load library will be called twice.  Apparently,
        // there is is no danger of the global getting mangled because
        // writes are atomic.
        //
        g_hCryptoUI = LoadLibrary("cryptui.dll");
        fTriedOnce = TRUE;
    }

    return (g_hCryptoUI != NULL);
}

//+-------------------------------------------------------------------------
// Delay load version of the function in cryptui.dll
//--------------------------------------------------------------------------
BOOL _CryptUIDlgViewCertificate(
    IN  PCCRYPTUI_VIEWCERTIFICATE_STRUCT   pCertViewInfo,
    OUT BOOL                               *pfPropertiesChanged  // OPTIONAL
    )
{
    // Caller must call UseCryptoUI() first to load the dll!
    INET_ASSERT(g_hCryptoUI);
    typedef BOOL (CALLBACK* CRYPTUIDLGVIEWCERTIFICATE)(PCCRYPTUI_VIEWCERTIFICATE_STRUCT, BOOL*);

    static CRYPTUIDLGVIEWCERTIFICATE fnCryptUIDlgViewCertificate = NULL;

    if (fnCryptUIDlgViewCertificate == NULL)
    {
        //
        // Note: if this gets called by multiple threads the worst that will
        // happen is that GetProcAddress will be called twice.  Apparently,
        // there is is no danger of the global getting mangled because
        // writes are atomic.
        //
        fnCryptUIDlgViewCertificate = (CRYPTUIDLGVIEWCERTIFICATE)GetProcAddress(g_hCryptoUI, "CryptUIDlgViewCertificateA");
        if (fnCryptUIDlgViewCertificate == NULL)
        {
            return FALSE;
        }
    }

    // Call the real function
    return fnCryptUIDlgViewCertificate(pCertViewInfo, pfPropertiesChanged);
}
#endif USE_NT5_CRYPTOUI

INTERNETAPI
BOOL
InternetAlgIdToStringA(
    IN ALG_ID       ai,
    IN LPSTR        lpstr,
    IN OUT LPDWORD  lpdwstrLength,
    IN DWORD        dwReserved /* Must be 0 */
    )
/*++

Routine Description:

    Converts a algid to a user-displayable string.

Arguments:

    ai - Algorithm identifiers ( defined in wincrypt.h)

    lpstr - Buffer to copy string into.

    lpdwstrLength - pass in num of characters, return no of characters copied if successful,
                       else no of chars required (including null terminator)

    dwReserved = Must be 0

Return Value:
    DWORD
        Win32 or WININET error code.
--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetAlgIdToStringA",
                     "%#x, %q, %#x, %#x",
                     ai,
                     lpstr,
                     lpdwstrLength,
                     dwReserved
                     ));

    DWORD error = ERROR_SUCCESS;

    if ((dwReserved!=0) || (lpdwstrLength == NULL))
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if (lpstr == NULL)
        *lpdwstrLength = 0;

    int i;
    for (i=0; i < ARRAY_ELEMENTS(AlgAttrMap) ; i++ )
    {
        if (ai == AlgAttrMap[i].dwAttr)
            break;
    }

    if ( i == ARRAY_ELEMENTS(AlgAttrMap) )
    {
        INET_ASSERT(FALSE);     // Could be because our table is not up to date.
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    CHAR szTempBuffer[100];

    int nRet;
    nRet = LoadStringA(GlobalDllHandle,
                           AlgAttrMap[i].dwStringID,
                           szTempBuffer,
                           ARRAY_ELEMENTS(szTempBuffer)
                           );

    // If the return value is within one less than the arraysize, it implies the
    // string could have been terminated by LoadString. This should not happen
    // since we have allocated a large enough buffer. If it does we need to bump the
    // size of the temporary array above.
    INET_ASSERT(nRet < ARRAY_ELEMENTS(szTempBuffer) - 1);

    if (*lpdwstrLength > (DWORD)nRet)
    {
        memcpy(lpstr, szTempBuffer, (nRet + 1));
        *lpdwstrLength = nRet;
        error = ERROR_SUCCESS;
    }
    else
    {
        *lpdwstrLength = nRet + 1;
        error = ERROR_INSUFFICIENT_BUFFER;
    }

quit:
    if (ERROR_SUCCESS != error)
    {
        SetLastError(error);
        DEBUG_ERROR(API, error);
    }
    DEBUG_LEAVE_API(error==ERROR_SUCCESS);
    return (error == ERROR_SUCCESS);
}

INTERNETAPI
BOOL
InternetSecurityProtocolToStringA(
    IN DWORD        dwProtocol,
    IN LPSTR        lpstr,
    IN OUT LPDWORD  lpdwstrLength,
    IN DWORD        dwReserved /* Must be 0 */
    )
/*++

Routine Description:

    Converts a security protocol to a user-displayable string.

Arguments:

    dwProtocol - Security protocol identifier ( defined in wincrypt.h)

    lpstr - Buffer to copy string into.

    lpdwstrLength - pass in num of characters, return no of characters copied if successful,
                       else no of chars required (including null terminator)

    dwReserved = Must be 0

Return Value:
    DWORD
        Win32 or WININET error code.
--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetSecurityProtocolToStringA",
                     "%d, %q, %#x, %#x",
                     dwProtocol,
                     lpstr,
                     lpdwstrLength,
                     dwReserved
                     ));

    DWORD error = ERROR_SUCCESS;

    if ((dwReserved!=0) || (lpdwstrLength == NULL))
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if (lpstr == NULL)
        *lpdwstrLength = 0;

    int i;
    for (i=0; i < ARRAY_ELEMENTS(ProtocolAttrMap) ; i++ )
    {
        if (dwProtocol == ProtocolAttrMap[i].dwAttr)
            break;
    }

    if ( i == ARRAY_ELEMENTS(ProtocolAttrMap) )
    {
        INET_ASSERT(FALSE);     // Could be because our table is not up to date.
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    CHAR szTempBuffer[100];

    int nRet;
    nRet = LoadStringA(GlobalDllHandle,
                           ProtocolAttrMap[i].dwStringID,
                           szTempBuffer,
                           ARRAY_ELEMENTS(szTempBuffer)
                           );

    // If the return value is within one less than the arraysize, it implies the
    // string could have been terminated by LoadString. This should not happen
    // since we have allocated a large enough buffer. If it does we need to bump the
    // size of the temporary array above.
    INET_ASSERT(nRet < ARRAY_ELEMENTS(szTempBuffer) - 1);

    if (*lpdwstrLength > (DWORD)nRet)
    {
        memcpy(lpstr, szTempBuffer, (nRet + 1));
        *lpdwstrLength = nRet;
        error = ERROR_SUCCESS;
    }
    else
    {
        *lpdwstrLength = nRet + 1;
        error = ERROR_INSUFFICIENT_BUFFER;
    }

quit:
    if (ERROR_SUCCESS != error)
    {
        SetLastError(error);
        DEBUG_ERROR(API, error);
    }
    DEBUG_LEAVE_API(error==ERROR_SUCCESS);
    return (error == ERROR_SUCCESS);
}


LPWSTR DupAnsiToUnicode(
    char *lpszAnsi,
    INT iLen
    )
{
    
    DWORD cbSize = (iLen > 0 ) ? iLen : (lstrlen(lpszAnsi) + 1);
    WCHAR *pwszUnicode = NULL;
    pwszUnicode = new WCHAR[cbSize];
    if(pwszUnicode)
    {
        SHAnsiToUnicode(lpszAnsi, pwszUnicode, cbSize);
    }
    return pwszUnicode;
}



DWORD
ShowSecurityInfo(
    IN HWND                            hWndParent,
    IN LPINTERNET_SECURITY_INFO        pSecurityInfo
    )

/*++

Routine Description:

    Displays a dialog box that shows the information found
    inside of a certificate.

Arguments:

    hWndParent - Parent Window Handle

    pCertInfoEx - Certificate Information structure, containing the
                fields of info to show.

Return Value:

    DWORD
        Win32 or WININET error code.
--*/

{
#ifdef USE_NT5_CRYPTOUI
    //
    // For now, we use the new UI only if we can load the DLL.  Otherwise we
    // resort to the old UI.  Eventually, we may nuke the old UI.
    //
    if (UseCryptoUI())
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCT cert;

        if(pWTHelperProvDataFromStateData && g_fDoSpecialMagicForSGCCerts)
        {
            WINTRUST_DATA           sWTD;
            WINTRUST_CERT_INFO      sWTCI;
            HTTPSPolicyCallbackData polHttps;
            LPCSTR pszPurpose = szOID_PKIX_KP_SERVER_AUTH;
            DWORD status;

            memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
            sWTD.cbStruct               = sizeof(WINTRUST_DATA);
            sWTD.dwUIChoice             = WTD_UI_NONE;
            sWTD.pPolicyCallbackData    = (LPVOID)&polHttps;
            sWTD.dwUnionChoice          = WTD_CHOICE_CERT;
            sWTD.pCert                  = &sWTCI;
            sWTD.pwszURLReference       = NULL;
            sWTD.dwStateAction          = WTD_STATEACTION_VERIFY;
        
            memset(&sWTCI, 0x00, sizeof(WINTRUST_CERT_INFO));
            sWTCI.cbStruct              = sizeof(WINTRUST_CERT_INFO);
            sWTCI.psCertContext         = (CERT_CONTEXT *)pSecurityInfo->pCertificate;
            sWTCI.chStores              = 1;
            sWTCI.pahStores             = (HCERTSTORE *)&pSecurityInfo->pCertificate->hCertStore;

            memset(&polHttps, 0x00, sizeof(HTTPSPolicyCallbackData));
            polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
            polHttps.dwAuthType         = AUTHTYPE_SERVER;
            polHttps.fdwChecks          = INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
                                          SECURITY_FLAG_IGNORE_WRONG_USAGE;
            polHttps.pwszServerName     = NULL;

            status = WinVerifySecureChannel(NULL, &sWTD);

            ZeroMemory(&cert, sizeof(cert));
            cert.dwSize         = sizeof(cert);
            cert.hwndParent     = hWndParent;
            cert.pCertContext   = pSecurityInfo->pCertificate;
            cert.hWVTStateData  = pWTHelperProvDataFromStateData(sWTD.hWVTStateData);
            cert.fpCryptProviderDataTrustedUsage = (status == ERROR_SUCCESS) ? TRUE : FALSE;
            cert.rgszPurposes = &pszPurpose;
            cert.cPurposes = 1;

            status = _CryptUIDlgViewCertificate(&cert, NULL);

            sWTD.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifySecureChannel(NULL, &sWTD);

            return status;    
        }
        else
        {
            ZeroMemory(&cert, sizeof(cert));
            cert.dwSize         = sizeof(cert);
            cert.hwndParent     = hWndParent;
            cert.pCertContext   = pSecurityInfo->pCertificate;
            cert.cStores        = 1;
            cert.rghStores      = (HCERTSTORE *) & (cert.pCertContext->hCertStore);

            return _CryptUIDlgViewCertificate(&cert, NULL);
        }

    }
#endif
#ifdef _WIN64
    return ERROR_INTERNET_INTERNAL_ERROR;
#else
    LPTSTR szResult = NULL;
    PLOCAL_STRINGS plszStrings;
    ShowCertMapping MapCertFields[MAX_CERT_FIELDS];

    WCHAR szTempBuffer[100];

    INT i = 0, j=0;
    DWORD error;
    LPTSTR lpszSubject = NULL;
    LPWSTR lpwszTempSubject = NULL;
    LPTSTR lpszIssuer = NULL;
    LPWSTR lpwszTempIssuer = NULL;

    WCHAR lpszProtocol[100];
    LPWSTR lpwszCipher = NULL;
    LPWSTR lpwszHash = NULL;
    LPWSTR lpwszExch = NULL;
    LPTSTR szFrom = NULL;
    LPWSTR pwszTempFrom = NULL;
    LPTSTR szUntil = NULL;
    LPWSTR pwszTempUntil = NULL;
    LPWSTR pwszStatus = NULL;
    LPSTR lpszHashStr = NULL;
    LPWSTR pwszTempHashStr = NULL;

    DWORD  adwFormatParams[3];


    PCERT_INFO pCertInfo =  NULL;
    DWORD  dwProtocolID =   IDS_PROTOCOL_UNKNOWN;
    DWORD  dwHashID =       IDS_HASH_UNKNOWN;
    DWORD  dwCipherID =     IDS_CIPHER_UNKNOWN;
    DWORD  dwExchID =       IDS_EXCH_UNKNOWN;
    DWORD cbSize;

    error = ERROR_SUCCESS;

    if((pSecurityInfo == NULL) || (pSecurityInfo->pCertificate == NULL))
    {
        return ERROR_INTERNET_INTERNAL_ERROR;
    }


    pCertInfo = pSecurityInfo->pCertificate->pCertInfo;

    if(pCertInfo == NULL)
    {
       return ERROR_INTERNET_INTERNAL_ERROR;
    }

    //
    // Get the Certificate Information.
    //

    plszStrings = FetchLocalStrings();

    if ( plszStrings == NULL )
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    szFrom = FTtoString(&pCertInfo->NotBefore);
    szUntil = FTtoString(&pCertInfo->NotAfter);



    //
    // Put a comment string about the certificate if there is one availble.
    //

    //
    // BUGBUG [arthurbi] This is broken.  We never determnine the host name,
    //  so therefore we never show a Comment for bad CA certificates.
    //

    pwszStatus = GetCertStatus(pSecurityInfo);

    if(pwszStatus)
    {
        MapCertFields[i].lpszListBoxText = plszStrings->szCertComment;
        MapCertFields[i].lpszEditBoxText = pwszStatus;
        MapCertFields[i].dwSpcCtlId      = 0;//IDC_CERT_COMMENT;
        i++;

    }

    if ( pCertInfo->Subject.cbData )
    {
        cbSize = CertNameToStr(pSecurityInfo->pCertificate->dwCertEncodingType,
                                     &pCertInfo->Subject,
                                      CERT_SIMPLE_NAME_STR |
                                      CERT_NAME_STR_CRLF_FLAG |
                                      CERT_NAME_STR_NO_PLUS_FLAG,
                                     NULL,
                                     0);

        lpszSubject = new TCHAR[cbSize];

        if ( lpszSubject )
        {
            CertNameToStr(pSecurityInfo->pCertificate->dwCertEncodingType,
                                     &pCertInfo->Subject,
                                      CERT_SIMPLE_NAME_STR |
                                      CERT_NAME_STR_CRLF_FLAG |
                                      CERT_NAME_STR_NO_PLUS_FLAG ,
                                     lpszSubject,
                                     cbSize);
            MapCertFields[i].lpszListBoxText = plszStrings->szCertSubject;
            lpwszTempSubject = DupAnsiToUnicode(lpszSubject, cbSize);
            MapCertFields[i].lpszEditBoxText = lpwszTempSubject;
            MapCertFields[i].dwSpcCtlId      = 0;//IDC_CERT_SUBJECT;
            i++;
        }
    }


    if ( pCertInfo->Issuer.cbData )
    {
        cbSize = CertNameToStr(pSecurityInfo->pCertificate->dwCertEncodingType,
                                     &pCertInfo->Issuer,
                                      CERT_SIMPLE_NAME_STR |
                                      CERT_NAME_STR_CRLF_FLAG |
                                      CERT_NAME_STR_NO_PLUS_FLAG,
                                     NULL,
                                     0);

        lpszIssuer = new TCHAR[cbSize];

        if ( lpszIssuer )
        {
            CertNameToStr(pSecurityInfo->pCertificate->dwCertEncodingType,
                                     &pCertInfo->Issuer,
                                      CERT_SIMPLE_NAME_STR |
                                      CERT_NAME_STR_CRLF_FLAG |
                                      CERT_NAME_STR_NO_PLUS_FLAG ,
                                     lpszIssuer,
                                     cbSize);
            MapCertFields[i].lpszListBoxText = plszStrings->szCertIssuer;
            lpwszTempIssuer = DupAnsiToUnicode(lpszIssuer, cbSize);
            MapCertFields[i].lpszEditBoxText = lpwszTempIssuer;
            MapCertFields[i].dwSpcCtlId      = 0;//IDC_CERT_ISSUER;
            i++;
        }
    }


    if ( szFrom )
    {
        MapCertFields[i].lpszListBoxText = plszStrings->szCertEffectiveDate;
        pwszTempFrom = DupAnsiToUnicode(szFrom, 0);
        MapCertFields[i].lpszEditBoxText = pwszTempFrom;
        MapCertFields[i].dwSpcCtlId      = 0;
        i++;
    }


    if ( szUntil )
    {
        MapCertFields[i].lpszListBoxText = plszStrings->szCertExpirationDate;
        pwszTempUntil = DupAnsiToUnicode(szUntil, 0);
        MapCertFields[i].lpszEditBoxText = pwszTempUntil;

        MapCertFields[i].dwSpcCtlId      = 0;//IDC_CERT_EXPIRES;
        i++;
    }

    //
    // Get the fingerprint... aka MD5 Hash
    //

    {
        CHAR lpMD5Hash[TYPICAL_MD5_HASH_SIZE];
        DWORD dwMD5HashSize = TYPICAL_MD5_HASH_SIZE;
        BOOL fSuccess;

        fSuccess = CertGetCertificateContextProperty(
                    pSecurityInfo->pCertificate,
                    CERT_MD5_HASH_PROP_ID,
                    (LPVOID) lpMD5Hash,
                    &dwMD5HashSize
                    );

        if ( fSuccess )
        {
            CertHashToStr( lpMD5Hash,
                           dwMD5HashSize,
                           &lpszHashStr
                           );

            if ( lpszHashStr )
            {
                MapCertFields[i].lpszListBoxText = plszStrings->szFingerprint;
                pwszTempHashStr = DupAnsiToUnicode(lpszHashStr, 0);
                MapCertFields[i].lpszEditBoxText = pwszTempHashStr;

                MapCertFields[i].dwSpcCtlId      = 0;
                i++;
            }
        }
    }

    // Now fill in the connection attributes
    if(pSecurityInfo->dwProtocol)
    {

        for(j=0; j < sizeof(ProtocolAttrMap)/sizeof(ProtocolAttrMap[0]); j++)
        {
            if(ProtocolAttrMap[j].dwAttr == pSecurityInfo->dwProtocol)
            {
                dwProtocolID = ProtocolAttrMap[j].dwStringID;
                break;
            }
        }
        if(LoadStringWrapW(GlobalDllHandle,
                   dwProtocolID,
                   lpszProtocol,
                   sizeof(lpszProtocol)/sizeof(lpszProtocol[0])))
        {

            MapCertFields[i].lpszEditBoxText = lpszProtocol;
            MapCertFields[i].lpszListBoxText = plszStrings->szCertProtocol;
            MapCertFields[i].dwSpcCtlId      = 0;
            i++;
        }
    }

    if(pSecurityInfo->aiCipher)
    {
        for(j=0; j < sizeof(AlgAttrMap)/sizeof(AlgAttrMap[0]); j++)
        {
            if(AlgAttrMap[j].dwAttr == pSecurityInfo->aiCipher)
            {
                dwCipherID = AlgAttrMap[j].dwStringID;
                break;
            }
        }

        LoadStringWrapW(GlobalDllHandle,
                   dwCipherID,
                   szTempBuffer,
                   sizeof(szTempBuffer)/sizeof(szTempBuffer[0]));
        adwFormatParams[0] = (DWORD)szTempBuffer;
        adwFormatParams[1] = (DWORD)pSecurityInfo->dwCipherStrength;

        if (96 <= pSecurityInfo->dwCipherStrength)  // Recommended Key strength
            adwFormatParams[2] = (DWORD)plszStrings->szStrengthHigh;
        else if (64 <= pSecurityInfo->dwCipherStrength) // Passable key strength
            adwFormatParams[2] = (DWORD)plszStrings->szStrengthMedium;
        else    // Ick!  Low key strength.
            adwFormatParams[2] = (DWORD)plszStrings->szStrengthLow;



        if(FormatMessageWrapW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_STRING |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          plszStrings->szCiphMsg,
                          0,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPWSTR)&lpwszCipher,
                          0,
                          (va_list *)adwFormatParams))
        {
            MapCertFields[i].lpszEditBoxText = lpwszCipher;
            MapCertFields[i].lpszListBoxText = plszStrings->szHttpsEncryptAlg;
            MapCertFields[i].dwSpcCtlId      = 0;

            i++;
        }
    }

    if(pSecurityInfo->aiHash)
    {
        for(j=0; j < sizeof(AlgAttrMap)/sizeof(AlgAttrMap[0]); j++)
        {
            if(AlgAttrMap[j].dwAttr == pSecurityInfo->aiHash)
            {
                dwHashID = AlgAttrMap[j].dwStringID;
                break;
            }
        }
        LoadStringWrapW(GlobalDllHandle,
                   dwHashID,
                   szTempBuffer,
                   sizeof(szTempBuffer)/sizeof(szTempBuffer[0]));
        adwFormatParams[0] = (DWORD)szTempBuffer;
        adwFormatParams[1] = (DWORD)pSecurityInfo->dwHashStrength;

        if (96 <= pSecurityInfo->dwHashStrength)  // Recommended Key strength
            adwFormatParams[2] = (DWORD)plszStrings->szStrengthHigh;
        else if (64 <= pSecurityInfo->dwHashStrength) // Passable key strength
            adwFormatParams[2] = (DWORD)plszStrings->szStrengthMedium;
        else    // Ick!  Low key strength.
            adwFormatParams[2] = (DWORD)plszStrings->szStrengthLow;

        if(FormatMessageWrapW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_STRING |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      plszStrings->szHashMsg,
                      0,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPWSTR)&lpwszHash,
                      0,
                      (va_list *)adwFormatParams))
        {
            MapCertFields[i].lpszEditBoxText = lpwszHash;
            MapCertFields[i].lpszListBoxText = plszStrings->szHttpsHashAlg;
            MapCertFields[i].dwSpcCtlId      = 0;
            i++;
        }
    }
    if(pSecurityInfo->aiExch)
    {
        for(j=0; j < sizeof(AlgAttrMap)/sizeof(AlgAttrMap[0]); j++)
        {
            if(AlgAttrMap[j].dwAttr == pSecurityInfo->aiExch)
            {
                dwExchID = AlgAttrMap[j].dwStringID;
                break;
            }
        }
        LoadStringWrapW(GlobalDllHandle,
                   dwExchID,
                   szTempBuffer,
                   sizeof(szTempBuffer)/sizeof(szTempBuffer[0]));
        adwFormatParams[0] = (DWORD)szTempBuffer;
        adwFormatParams[1] = (DWORD)pSecurityInfo->dwExchStrength;

        if (1024 <= pSecurityInfo->dwExchStrength)  // Recommended Key strength
            adwFormatParams[2] = (DWORD)plszStrings->szStrengthHigh;
         else    // Ick!  Low key strength.
            adwFormatParams[2] = (DWORD)plszStrings->szStrengthLow;

        if(FormatMessageWrapW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_STRING |
                         FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      plszStrings->szExchMsg,
                      0,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPWSTR)&lpwszExch,
                      0,
                      (va_list *)adwFormatParams))
        {
            MapCertFields[i].lpszEditBoxText = lpwszExch;
            MapCertFields[i].lpszListBoxText = plszStrings->szHttpsExchAlg;
            MapCertFields[i].dwSpcCtlId      = 0;
            i++;
        }
    }

    //
    // Last Array Item is marked with 2 NULLs
    //

    MapCertFields[i].lpszListBoxText = NULL;
    MapCertFields[i].lpszEditBoxText = NULL;
    MapCertFields[i].dwSpcCtlId      = 0;


    INET_ASSERT(i < MAX_CERT_FIELDS);

    //
    // Now Launch the Dlg so we can show it.
    //

    ERRORINFODLGTYPE CertDlgInfo;

    CertDlgInfo.dwDlgFlags      = DLG_FLAGS_HAS_CERT_INFO;
    CertDlgInfo.dwDlgId         = IDD_SHOW_CERT;
    CertDlgInfo.hInternetMapped = NULL;
    CertDlgInfo.lpVoid          = (LPVOID) MapCertFields;

    LaunchDlg(
              hWndParent,
              (LPVOID) &CertDlgInfo,
              IDD_VIEW_CERT,
              ViewCertDlgProc
              );



quit:
    if(lpszIssuer) {
        FREE_MEMORY(lpszIssuer);
    }
    if(lpwszTempIssuer) {
        FREE_MEMORY(lpwszTempIssuer);
    }
    if(lpszSubject)
    {
        FREE_MEMORY(lpszSubject);
    }
    if(lpwszTempSubject) {
        FREE_MEMORY(lpwszTempSubject);
    }
    if (szFrom) {
        FREE_MEMORY(szFrom);
    }
    if(pwszTempFrom) {
        FREE_MEMORY(pwszTempFrom);
    }
    if (szUntil) {
        FREE_MEMORY(szUntil);
    }
    if(pwszTempUntil) {
        FREE_MEMORY(pwszTempUntil);
    }
    if (lpwszCipher) {
        FREE_MEMORY(lpwszCipher);
    }
    if (lpwszHash) {
        FREE_MEMORY(lpwszHash);
    }
    if (lpwszExch) {
        FREE_MEMORY(lpwszExch);
    }

    if (lpszHashStr) {
        delete lpszHashStr;
    }
    if(pwszTempHashStr) {
        FREE_MEMORY(pwszTempHashStr);
    }

    return error;
#endif
}


#ifdef unix
extern "C"
#endif /* unix */
DWORD
ShowCertificate(
    IN HWND                            hWndParent,
    IN LPVOID  pCertInfoEx
    )

/*++

Routine Description:

    Displays a dialog box that shows the information found
    inside of a certificate.

Arguments:

    hWndParent - Parent Window Handle

    pCertInfoEx - Certificate Information structure, containing the
                fields of info to show.

Return Value:

    DWORD
        Win32 or WININET error code.
--*/

{
    //DWORD error = ShowSecurityInfo(
    //                 hWndParent,
    //                (LPINTERNET_SECURITY_INFO) pCertInfoEx // BAD..
    //                 );

    //return error;

    return ERROR_INTERNET_INTERNAL_ERROR;
}







BOOL
CALLBACK
ViewCertDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    )

/*++

Routine Description:

    Shows a Certificate, and relevent security information to the user.

Arguments:

    hwnd    - standard dialog params

    msg     - "

    wparam  - "

    lparam  - "

Return Value:

    BOOL
        TRUE    - we handled message

        FALSE   - Windows should handle message

--*/

{
    PERRORINFODLGTYPE pDlgInfo;

    const static DWORD mapIDCsToIDHs[] =
    {
         IDC_CERTPICKLIST                 ,IDH_LIST_CERT,
         ID_SHOW_CERTIFICATE              ,IDH_VIEW_CERT,
         IDC_DELETE_CERT                  ,IDH_DEL_CERT,
         0,0
    };

    switch (msg)
    {

    case WM_INITDIALOG:

        INET_ASSERT(lparam);

        pDlgInfo = (PERRORINFODLGTYPE)lparam;

        (void)SetWindowLongPtr(hwnd,
                               DWLP_USER,
                               lparam);

        INET_ASSERT(pDlgInfo->dwDlgFlags & DLG_FLAGS_HAS_CERT_INFO );
        INET_ASSERT(pDlgInfo->lpVoid);

        PlaceCertificateDataIntoListBox(
            hwnd,
            GetDlgItem(hwnd,IDC_FIELDLIST),
            (ShowCertMapping *) pDlgInfo->lpVoid
            );

        OnSelectionOfACertField(
            GetDlgItem(hwnd,IDC_FIELDLIST),
            GetDlgItem(hwnd,IDC_DETAILSLIST),
            (ShowCertMapping *) pDlgInfo->lpVoid
            );


        return TRUE;

    case WM_HELP:                   // F1
        WinHelp( (HWND)((LPHELPINFO)lparam)->hItemHandle,
                szHelpFile,
                HELP_WM_HELP,
                (ULONG_PTR)(LPSTR)mapIDCsToIDHs
                );
        break;

    case WM_CONTEXTMENU:        // right mouse click
        WinHelp( (HWND) wparam,
                 szHelpFile,
                 HELP_CONTEXTMENU,
                 (ULONG_PTR)(LPSTR)mapIDCsToIDHs
                 );
        break;


    case WM_COMMAND:
        {

        WORD wID = LOWORD(wparam);
        WORD wNotificationCode = HIWORD(wparam);
        HWND hWndCtrl = (HWND) lparam;

        pDlgInfo =
            (PERRORINFODLGTYPE) GetWindowLongPtr(hwnd,DWLP_USER);

            switch (wID)
            {
                case ID_TELL_ME_ABOUT_SECURITY:

                    //
                    // Launches help for this button.
                    //

                    WinHelp(
                            hwnd,
                            szHelpFile,
                            HELP_CONTEXT,
                            (ULONG_PTR)HELP_TOPIC_SECURITY
                            );
                                break;


                case IDC_FIELDLIST:

                    //
                    // If the user changes the selection of the listbox
                    //  move the edit control field data to the correct
                    //  entry.
                    //

                    if ( wNotificationCode == LBN_SELCHANGE )
                    {
                        OnSelectionOfACertField(
                            hWndCtrl,
                            GetDlgItem(hwnd,IDC_DETAILSLIST),
                            (ShowCertMapping *) pDlgInfo->lpVoid
                            );
                    }

                    break;

                case IDOK:
                case IDYES:

                    INET_ASSERT(pDlgInfo);
                    INET_ASSERT(pDlgInfo->dwDlgId != 0);

                    EndDialog(hwnd, TRUE);
                    break;

                case IDCANCEL:
                case IDNO:

                    EndDialog(hwnd, FALSE);
                    break;
            }

        return TRUE;
        }
    }

    return FALSE;
}



DWORD
ShowClientAuthCerts(
    IN HWND hWndParent
    )

/*++

Routine Description:

    Shows the Client Authentication Certificates found in the system.

Arguments:

    hWndParent - Parent Window Handle

Return Value:

    DWORD
        Win32 or WININET error code.

--*/


{
// With the NT5 crypto UI we don't need to display client-auth certs anymore.
// I left this in code in the source file in case we switch back to the old
// crypto dlls for some reason. Once we decide to move on to the new crypto dlls
// this code can be removed. - sgs
#ifndef USE_NT5_CRYPTOUI
    DWORD error;
    ERRORINFODLGTYPE ErrorInfoDlgInfo;

    ErrorInfoDlgInfo.hInternetMapped    = NULL;
    ErrorInfoDlgInfo.dwDlgId            = IDD_CERTVIEWER;
    ErrorInfoDlgInfo.lpVoid             = NULL;
    ErrorInfoDlgInfo.dwDlgFlags         = 0;


    CliAuthAquireCertChains(
        NULL,
        NULL,
        (CERT_CHAIN_ARRAY **) &ErrorInfoDlgInfo.lpVoid
        );

    //
    // Don't Care about error code, from function.
    //


    error = LaunchDlg(
              hWndParent,
              (LPVOID) &ErrorInfoDlgInfo,
              ErrorInfoDlgInfo.dwDlgId,
              CertPickDialogProc
              );

    if ( ErrorInfoDlgInfo.lpVoid )
        delete ErrorInfoDlgInfo.lpVoid;

    return error;
#else
    INET_ASSERT(FALSE);
    return ERROR_CALL_NOT_IMPLEMENTED;
#endif
}




DWORD
ParseX509EncodedCertificateForListBoxEntry(
    IN LPBYTE  lpCert,
    IN DWORD   cbCert,
    OUT LPSTR  lpszListBoxEntry,
    IN LPDWORD lpdwListBoxEntry
    )
/*++

Routine Description:

    Parses an X509 certificate, into a single text entry that
    can be displayed on a line in a listbox.

Arguments:

    lpCert           - Certificte bytes to parse

    cbCert           - Size of certificate to parse

    lpszListBoxEntry - Formated text to use in List Box.

    lpdwListBoxEntry - IN: size of lpszListBoxEntry, OUT: actual size of string.

Return Value:

    DWORD
        Win32 or WININET error code.

--*/

{
    BOOL fSuccess;
    DWORD error = ERROR_SUCCESS;
    PCCERT_CONTEXT pCert;
    PCERT_NAME_INFO pName = NULL;
    PCERT_RDN       pIdentRDN;
    PCERT_RDN_ATTR  pIdentifier;
    DWORD           cbIdentifier;
    DWORD cbName, cbName2;


    INET_ASSERT(lpdwListBoxEntry);

    if (lpszListBoxEntry == NULL)
        *lpdwListBoxEntry = 0;

    //
    //  30-Aug-1997 pberkman:
    //              we need to do this to be backwards compatible with this function.
    //              however, becuase the create context function does not get us properties
    //              when we are creating it from a already existing context, we need the
    //              ability to just pass in the pre-existing context.  To do this, just
    //              pass "-1" in for the cbCert and we'll do the "right thing".
    //
    if (cbCert == (-1))
    {
        pCert = (PCCERT_CONTEXT)lpCert;
    }
    else
    {
        pCert = CertCreateCertificateContext(X509_ASN_ENCODING,
                                    lpCert,
                                    cbCert);
    }

    //
    //  30-Aug-1997 pberkman:
    //              look at the "Friendly Name" property first.
    //
    cbName = 0;
    CertGetCertificateContextProperty(pCert, CERT_FRIENDLY_NAME_PROP_ID, NULL, &cbName);

    if (cbName > 0)
    {
        WCHAR *pbFName;

        if (pbFName = (WCHAR *)new BYTE[cbName])
        {
            if (CertGetCertificateContextProperty(pCert, CERT_FRIENDLY_NAME_PROP_ID, pbFName, &cbName))
            {
                cbName2 = WideCharToMultiByte(0, 0, (WCHAR *)pbFName, wcslen((WCHAR *)pbFName) + 1,
                                              lpszListBoxEntry, *lpdwListBoxEntry, NULL, NULL);

                if (cbName2 > *lpdwListBoxEntry)
                {
                    error = ERROR_INSUFFICIENT_BUFFER;
                    *lpdwListBoxEntry = cbName2;
                }

                delete pbFName;

                if ((pCert) && (cbCert != (-1)))
                {
                    CertFreeCertificateContext(pCert);
                }

                return(error);
            }

            delete pbFName;
        }
    }

    if(CryptDecodeObject(pCert->dwCertEncodingType,
                         X509_NAME,
                         pCert->pCertInfo->Subject.pbData,
                         pCert->pCertInfo->Subject.cbData,
                         0,
                         NULL,
                         &cbName))
    {
        pName = (PCERT_NAME_INFO)new BYTE[cbName];
        if(pName == NULL)
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if(!CryptDecodeObject(pCert->dwCertEncodingType,
                                     X509_NAME,
                                     pCert->pCertInfo->Subject.pbData,
                                     pCert->pCertInfo->Subject.cbData,
                                     0,
                                     pName,
                                     &cbName))
        {
            delete pName;
            pName = NULL;
            error = GetLastError();

        }
    }
    else
    {
        error = GetLastError();
    }

    if((NULL != pName) && (pName->cRDN > 0))
    {
        pIdentifier = CertFindRDNAttr(szOID_COMMON_NAME, pName);

        if(pIdentifier == NULL)
        {
            pIdentifier = CertFindRDNAttr(szOID_ORGANIZATIONAL_UNIT_NAME, pName);
            if(pIdentifier == NULL)
            {
                pIdentifier = CertFindRDNAttr(szOID_ORGANIZATION_NAME, pName);
                if(pIdentifier == NULL)
                {
                    pIdentRDN = &pName->rgRDN[pName->cRDN-1];
                    pIdentifier = &pIdentRDN->rgRDNAttr[pIdentRDN->cRDNAttr-1];
                }
            }
        }


        if(pIdentifier != NULL)
        {
            cbIdentifier = CertRDNValueToStr(pIdentifier->dwValueType,
                              &pIdentifier->Value,
                              NULL,
                              0);

            if(cbIdentifier == 0)
            {
                error = GetLastError();
            }
            else if ( (lpszListBoxEntry != NULL) && (cbIdentifier > (*lpdwListBoxEntry)) )
            {
                error = ERROR_INSUFFICIENT_BUFFER;
                *lpdwListBoxEntry = cbIdentifier;
            }
            else
            {

                *lpdwListBoxEntry = CertRDNValueToStr(pIdentifier->dwValueType,
                                  &pIdentifier->Value,
                                  lpszListBoxEntry,
                                  cbIdentifier);
                error = ERROR_SUCCESS;
            }
        }
        else
        {
            *lpdwListBoxEntry = 0;

            error = ERROR_INTERNET_INVALID_OPERATION;
        }


        delete pName;
    }

    //
    //  30-Aug-1997 pberkman:
    //              we have to free this thing!
    //
    if ((pCert) && (cbCert != (-1)))
    {
        CertFreeCertificateContext(pCert);
    }

    return error;
}



DWORD
ShowX509EncodedCertificate(
    IN HWND    hWndParent,
    IN LPBYTE  lpCert,
    IN DWORD   cbCert
    )
/*++

Routine Description:

    Shows an encoded set of bytes which represent a certificate,
    in a dialog box.

Arguments:

    hWndParent

    lpCert

    cbCert

Return Value:

    DWORD
        ERROR_SUCCESS


--*/

{

    DWORD error;

#ifdef USE_NT5_CRYPTOUI

    //
    // For now, we use the new UI only if we can load the DLL.  Otherwise we
    // resort to the old UI.  Eventually, we may nuke the old UI.
    //

    if (UseCryptoUI())
    {
        CRYPTUI_VIEWCERTIFICATE_STRUCT cert;

        ZeroMemory(&cert, sizeof(cert));
        cert.dwSize = sizeof(cert);
        cert.hwndParent = hWndParent;
        cert.pCertContext = (PCCERT_CONTEXT)lpCert;
        cert.cStores = 1;
        cert.rghStores = (HCERTSTORE *) & (cert.pCertContext->hCertStore);

        return _CryptUIDlgViewCertificate(&cert, NULL);
    }
    else
#endif
    {
        X509Certificate *pCertData = NULL;
        INTERNET_SECURITY_INFO ciInfo;

        ZeroMemory(&ciInfo, sizeof(ciInfo));

        ciInfo.dwSize = sizeof(ciInfo);

        ciInfo.pCertificate = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                         lpCert,
                                                         cbCert);
        error = ShowSecurityInfo(
                                hWndParent,
                                &ciInfo
                                );
    }

    return error;
}



BOOL _GetSelectedCertContext(HWND hwnd, PERRORINFODLGTYPE pDlgInfo, PCCERT_CONTEXT *ppCertContext)
{
    CERT_CONTEXT_ARRAY* pCertContextArray;

    if (ppCertContext == NULL)
        return FALSE;

    *ppCertContext = NULL;

    pCertContextArray =
                ((HTTP_REQUEST_HANDLE_OBJECT *)pDlgInfo->hInternetMapped)->GetCertContextArray();

    PCERT_CHAIN pcCertChain;
    LRESULT index;

    INET_ASSERT(pCertContextArray);
    if (!pCertContextArray)
        return FALSE;

    //
    // Retrieve the Cert from List box
    //

    index = SendDlgItemMessage(hwnd,
                               IDC_CERTPICKLIST,
                               LB_GETCURSEL,
                               0,
                               0);


    if ( index == LB_ERR )
        index = 0;


    if ( index >= (INT) pCertContextArray->GetArraySize())
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }

    *ppCertContext =  pCertContextArray->GetCertContext((INT)index);

    if (!*ppCertContext)
        return FALSE;
    else
        return TRUE;
}


INT_PTR
CALLBACK
CertPickDialogProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam
    )

/*++

Routine Description:

    Supports Ok/Cancel decisions for the Client Authentication UI.
    Allows the User to select a specific Certificate that he wishes
    to use for Client Auth.

Arguments:

    hwnd    - standard dialog params

    msg     - "

    wparam  - "

    lparam  - "

Return Value:

    BOOL
        TRUE    - we handled message

        FALSE   - Windows should handle message

--*/

{
    PERRORINFODLGTYPE pDlgInfo;
    CERT_CONTEXT_ARRAY* pCertContextArray;
    LRESULT index;

    const static DWORD mapIDCsToIDHs[] =
    {
         IDC_CERTPICKLIST,      IDH_CLIENT_AUTHENTICATION_LIST,
         ID_SHOW_CERTIFICATE,   IDH_CLIENT_AUTHENTICATION_CERT_PROPS,
         IDCLOSE,               IDH_ORG_FAVORITES_CLOSE,
         IDC_BUTTON_IMPORT,     IDH_CLIENTAUTH_IMPORT,
         IDC_BUTTON_EXPORT,     IDH_CLIENTAUTH_EXPORT,
         0,0
    };


    switch (msg)
    {

    case WM_INITDIALOG:

        {
            INET_ASSERT(lparam);

            pDlgInfo = (PERRORINFODLGTYPE)lparam;

            (void)SetWindowLongPtr(hwnd,
                                   DWLP_USER,
                                   lparam);

            // We used to have other dialogs use the same dialog proc, but this
            // shouldn't happen with the removal of the personal certs dialog.
            INET_ASSERT(pDlgInfo->dwDlgId == IDD_CERTPICKER);
            pCertContextArray =
                ((HTTP_REQUEST_HANDLE_OBJECT *)pDlgInfo->hInternetMapped)->GetCertContextArray();


            PlaceCertContextsInListBox(
                GetDlgItem(hwnd, IDC_CERTPICKLIST),
                GetDlgItem(hwnd, ID_SHOW_CERTIFICATE),
                GetDlgItem(hwnd, IDC_BUTTON_EXPORT),
                pCertContextArray
                );

            return TRUE;

        }

    case WM_HELP:                   // F1
        WinHelp( (HWND)((LPHELPINFO)lparam)->hItemHandle,
                szHelpFile,
                HELP_WM_HELP,
                (ULONG_PTR)(LPSTR)mapIDCsToIDHs
                );
        break;

    case WM_CONTEXTMENU:        // right mouse click
        WinHelp( (HWND) wparam,
                 szHelpFile,
                 HELP_CONTEXTMENU,
                 (ULONG_PTR)(LPSTR)mapIDCsToIDHs
                 );
        break;

    case WM_COMMAND:
        {
            WORD wID = LOWORD(wparam);
            WORD wNotificationCode = HIWORD(wparam);
            HWND hWndCtrl = (HWND) lparam;

            pDlgInfo =
                (PERRORINFODLGTYPE) GetWindowLongPtr(hwnd,DWLP_USER);

            switch (wID)
            {
                case ID_TELL_ME_ABOUT_SECURITY:

                    //
                    // Launches help for this button.
                    //

                    WinHelp(
                            hwnd,
                            szHelpFile,
                            HELP_CONTEXT,
                            (ULONG_PTR)HELP_TOPIC_SECURITY
                            );
                                break;


                case ID_SHOW_CERTIFICATE:
                {
                    //
                    // If this dialog supports this behavior, then launch
                    //  a show certficate dlg.
                    //
                    PCCERT_CONTEXT pCertContext;

                    if ( wNotificationCode == BN_CLICKED &&
                         _GetSelectedCertContext(hwnd, pDlgInfo, &pCertContext ))
                    {
                        ShowX509EncodedCertificate( hwnd, (LPBYTE)pCertContext, sizeof(pCertContext) );
                    }

                    break;
                }

                case ID_CERT_MORE_INFO:
                    HtmlHelp(hwnd, TEXT("iexplore.chm > iedefault"), HH_DISPLAY_TOPIC, (DWORD_PTR)TEXT("cert_ovr.htm"));
                    break;

                case IDCANCEL:

                    index = -1;
                    goto lskip_getcursel;


                case IDOK:

                    //
                    // Get the selected Cert.
                    //

                    index = SendDlgItemMessage(hwnd,
                                        IDC_CERTPICKLIST,
                                        LB_GETCURSEL,
                                        0,
                                        0);

                    if ( index == LB_ERR )
                        index = -1;


        lskip_getcursel:


                    INET_ASSERT(pDlgInfo);
                    INET_ASSERT(pDlgInfo->dwDlgId != 0);

                    //
                    // Select the Client Auth Cert to use,
                    //  but only if we've got the cert picker dialog.
                    //
                    pCertContextArray =
                        ((HTTP_REQUEST_HANDLE_OBJECT *)pDlgInfo->hInternetMapped)->GetCertContextArray();

                    if (pCertContextArray)
                    {
                        pCertContextArray->SelectCertContext((INT)index);
                    }


                    EndDialog(hwnd, TRUE);
                    break;


                case IDCLOSE:

                    //
                    // We're done, so return FALSE.
                    //

                    EndDialog(hwnd, FALSE);
                    break;


            }

        return TRUE;
        }
    }

    return FALSE;
}





//
// private functions
//


PRIVATE
BOOL
PlaceCertContextsInListBox(
    IN HWND hWndListBox,
    IN HWND hWndViewCertButton,
    IN HWND hWndExportButton,
    IN CERT_CONTEXT_ARRAY* pCertContextArray
    )

/*++

Routine Description:

    Takes an array of CertContext's and puts them into a listbox,
    by cracking their contents one at a time.

Arguments:

    hWndListBox         - Window Handle to ListBox to add items to.

    pCertContext's         - Pointer to array of cert chains.

    hWndViewCertButton  - Window handle to ViewCert Button, NULL if no button is around

Return Value:

    BOOL
        TRUE    - success.

        FALSE   - failure.

--*/

{
    DWORD i = 0;

    if ( !pCertContextArray)
    {
        goto quit;
    }

    SendMessage(hWndListBox, LB_RESETCONTENT, 0, 0 );

    for ( i = 0; i < pCertContextArray->GetArraySize(); i++ )
    {
        PCCERT_CONTEXT pCert;
//        PCERT_NAME_INFO pName = NULL;
//        PCERT_RDN_ATTR  pCommonName;
        DWORD cbName;
        DWORD   dwRet;

        pCert =  pCertContextArray->GetCertContext(i);

        INET_ASSERT(pCert);

        LPSTR lpszSubject;

        cbName = 0;
        ParseX509EncodedCertificateForListBoxEntry(pCert->pbCertEncoded,
                                                   pCert->cbCertEncoded,
                                                   NULL,
                                                   &cbName);
        if (cbName > 0)
        {
            if (lpszSubject = new TCHAR[cbName])
            {
                if (ParseX509EncodedCertificateForListBoxEntry(pCert->pbCertEncoded,
                                                               pCert->cbCertEncoded,
                                                               lpszSubject,
                                                               &cbName) == ERROR_SUCCESS)
                {
                    SendMessage(hWndListBox, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) lpszSubject);
                }

                FREE_MEMORY(lpszSubject);
            }
        }
    }


quit:


    // Select the first item in the list box
    // TODO: move selection to current default item.
    SendMessage(hWndListBox, LB_SETCURSEL, 0, 0 );

    // If nothing was added, disable the windows, otherwise enable them

    EnableWindow(hWndListBox, (i != 0));
    if ( hWndViewCertButton )
        EnableWindow(hWndViewCertButton, (i != 0));
    if ( hWndExportButton )
        EnableWindow(hWndExportButton, (i != 0));

    return TRUE;
}

PRIVATE
BOOL
PlaceCertificateDataIntoListBox(
    IN HWND hWndDlg,
    IN HWND hWndListBox,
    IN ShowCertMapping *pMapCertFields
    )
{

    DWORD i;

    INET_ASSERT(pMapCertFields);
    INET_ASSERT(IsWindow(hWndListBox));

    for ( i = 0; pMapCertFields[i].lpszListBoxText != NULL; i++ )
    {
        if ( pMapCertFields[i].dwSpcCtlId != 0 )
        {
            SetDlgItemTextWrapW(hWndDlg,pMapCertFields[i].dwSpcCtlId,
                pMapCertFields[i].lpszEditBoxText );

            SetWindowLong(GetDlgItem(hWndDlg,pMapCertFields[i].dwSpcCtlId),
                           GWL_STYLE, ES_READONLY |
                                        GetWindowLong(GetDlgItem(hWndDlg,pMapCertFields[i].dwSpcCtlId), GWL_STYLE));

        }

        SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)pMapCertFields[i].lpszListBoxText);
    }

    INET_ASSERT(i>0);

    SendMessage(hWndListBox, LB_SETCURSEL, 0, 0 );

    return TRUE;
}

PRIVATE
DWORD
OnSelectionOfACertField(
    IN HWND hWndListBox,
    IN HWND hWndEditBox,
    IN ShowCertMapping *pMapCertFields
    )
{
    LRESULT index;

    index = SendMessage(hWndListBox, LB_GETCURSEL, 0, 0);

    if (index == LB_ERR )
        index = 0;


    if ( pMapCertFields[index].lpszListBoxText != NULL )
    {
        SetWindowTextWrapW(hWndEditBox,
                        pMapCertFields[index].lpszEditBoxText );

        SetWindowLong(hWndEditBox,
                       GWL_STYLE, ES_READONLY |
                                    GetWindowLong(hWndEditBox, GWL_STYLE));
    }

    return ERROR_SUCCESS;
}

#ifndef CERT_E_WRONG_USAGE
#   define CERT_E_WRONG_USAGE   _HRESULT_TYPEDEF_(0x800B0110L)
#endif

#ifndef SECURITY_FLAG_IGNORE_WRONG_USAGE
#   define SECURITY_FLAG_IGNORE_WRONG_USAGE        0x00010000
#endif
/* get a string representing the status of a certificate */
LPWSTR  GetCertStatus(LPINTERNET_SECURITY_INFO pciCert)
{

    // We've done our handshake, now update the security info
    DWORD dwCertFlags;
    PLOCAL_STRINGS plszStrings;
    GUID                    gHTTPS = HTTPSPROV_ACTION;
    WINTRUST_DATA           sWTD;
    WINTRUST_CERT_INFO      sWTCI;
    HTTPSPolicyCallbackData polHttps;
    DWORD                   cbServerName;
    DWORD                   error;


    plszStrings = FetchLocalStrings();
    if(plszStrings == NULL)
    {
        return NULL;
    }

    if((pciCert == NULL) || (pciCert->pCertificate == NULL))
    {
        return NULL;
    }


    //
    //  initialize the structures for forward/backward support of wintrust.dll!!!
    //
    memset(&sWTD, 0x00, sizeof(WINTRUST_DATA));
    sWTD.cbStruct               = sizeof(WINTRUST_DATA);
    sWTD.dwUIChoice             = WTD_UI_NONE;
    sWTD.pPolicyCallbackData    = (LPVOID)&polHttps;
    sWTD.dwUnionChoice          = WTD_CHOICE_CERT;
    sWTD.pCert                  = &sWTCI;
    sWTD.pwszURLReference       = NULL;


    memset(&sWTCI, 0x00, sizeof(WINTRUST_CERT_INFO));
    sWTCI.cbStruct              = sizeof(WINTRUST_CERT_INFO);
    sWTCI.psCertContext         = (CERT_CONTEXT *)pciCert->pCertificate;
    sWTCI.chStores              = 1;
    sWTCI.pahStores  = (HCERTSTORE *)&pciCert->pCertificate->hCertStore;


    memset(&polHttps, 0x00, sizeof(HTTPSPolicyCallbackData));
    polHttps.cbStruct =  sizeof(HTTPSPolicyCallbackData);
    polHttps.dwAuthType = AUTHTYPE_SERVER;
    polHttps.fdwChecks = INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_WRONG_USAGE;




    polHttps.pwszServerName = NULL;


    sWTCI.pcwszDisplayName  = NULL;


    error = LoadWinTrust();

    if(ERROR_SUCCESS == error)
    {

        error = WinVerifySecureChannel(NULL, &sWTD);
    }

    // If we are unable to verify revocation, then ignore.
    if(error == CERT_E_REVOCATION_FAILURE)
    {
        error = ERROR_SUCCESS;
    }


    switch(error)
    {
        case CERT_E_EXPIRED:
        case CERT_E_VALIDITYPERIODNESTING:
            return plszStrings->szCommentExpires;

        case CERT_E_UNTRUSTEDROOT:
            return plszStrings->szCommentBadCA;


        case CERT_E_CN_NO_MATCH:
            return plszStrings->szCommentBadCN;

        case CRYPT_E_REVOKED:
            return plszStrings->szCommentRevoked;

        case CERT_E_WRONG_USAGE:
            return plszStrings->szCertUsage;

        case CERT_E_ROLE:
        case CERT_E_PATHLENCONST:
        case CERT_E_CRITICAL:
        case CERT_E_PURPOSE:
        case CERT_E_ISSUERCHAINING:
        case CERT_E_MALFORMED:
        case CERT_E_CHAINING:
            return plszStrings->szCommentNotValid;

        case ERROR_SUCCESS:
            return NULL;

        default:
             return plszStrings->szCommentNotValid;
   }

}
