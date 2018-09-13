//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mscdfapi.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  CryptCATCDFOpen
//              CryptCATCDFClose
//              CryptCATCDFEnumMembers
//              CryptCATCDFEnumAttributes
//
//              *** local functions ***
//
//              CDFGetAttributes
//              CDFTextToGUID
//              CDFPositionAtGroupTag
//              CDFGetNextMember
//              CDFGetParam
//              CDFGetLine
//              CDFSplitAttrLine
//              CDFEOLOut
//              CDFCheckOID
//              CDFCalcIndirectData
//
//  History:    01-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    <objbase.h>

#include    "mscat32.h"
#include    "sipguids.h"

void    CDFTextToGUID(LPWSTR pwszText, GUID *pgBin, PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError);
BOOL    CDFPositionAtGroupTag(CRYPTCATCDF *pCDF, LPWSTR pwszTag);
BOOL    CDFPositionAtLastMember(CRYPTCATCDF *pCDF);
BOOL    CDFGetNextMember(CRYPTCATCDF *pCDF, LPWSTR pwszMember, LPWSTR pwszLastMember);
BOOL    CDFGetParam(CRYPTCATCDF *pCDF, LPWSTR pwszGroup, LPWSTR pwszItem,
                    LPWSTR pwszDefault, LPWSTR *ppwszRet, LPWSTR pwszMemberTag);
DWORD   CDFGetLine(CRYPTCATCDF *pCDF, LPWSTR pwszLineBuf, DWORD dwMaxRead);
BOOL    CDFSplitAttrLine(LPWSTR pwszLine, DWORD *pdwType, LPWSTR *pwszOID,
                         LPWSTR *pwszValue, PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError);
void    CDFEOLOut(WCHAR *pwsz, DWORD ccLen);

BOOL    CDFCalcIndirectData(CRYPTCATCDF *pCDF, WCHAR *pwszFileName, GUID *pgSubjectType, DWORD *pcbIndirectData,
                            BYTE **pIndirectData, DWORD *pdwCertVersion, PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError);

BOOL    CDFCheckOID(LPWSTR pwszOID, PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError);

#define     MAX_CDF_LINE_LEN            512

#define     CAT_HEADER_TAG              L"[CatalogHeader]"
#define     CAT_HEADER_NAME_TAG         L"Name"
#define     CAT_HEADER_RESDIR_TAG       L"ResultDir"
#define     CAT_HEADER_VERSION_TAG      L"PublicVersion"
#define     CAT_HEADER_ENCODETYPE_TAG   L"EncodingType"
#define     CAT_HEADER_ATTR_TAG         L"CATATTR"

#define     CAT_MEMBER_TAG              L"[CatalogFiles]"
#define     CAT_MEMBER_ALTSIP_TAG       L"ALTSIPID"
#define     CAT_MEMBER_ATTR_TAG         L"ATTR"
#define     CAT_MEMBER_HASH_TAG         L"<HASH>"


/////////////////////////////////////////////////////////////////////////////
//
//  Exported Functions
//

CRYPTCATCDF * WINAPI CryptCATCDFOpen(LPWSTR pwszFilePath,
                                     PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    CRYPTCATCDF *pCDF;
    HANDLE      hFile;

    if (!(pwszFilePath))
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        return(NULL);
    }

    if ((hFile = CreateFileU(pwszFilePath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL)) == INVALID_HANDLE_VALUE)
    {
        return(NULL);
    }

    if (!(pCDF = (CRYPTCATCDF *)CatalogNew(sizeof(CRYPTCATCDF))))
    {
        return(NULL);
    }

    WCHAR       wszRetValue[MAX_CDF_LINE_LEN + 4];
    LPWSTR      pwsz;

    memset(pCDF, 0x00, sizeof(CRYPTCATCDF));

    pCDF->cbStruct  = sizeof(CRYPTCATCDF);
    pCDF->hFile     = hFile;

    //
    //  Name
    //
    if (pwsz = wcsrchr(pwszFilePath, L'\\'))
    {
        wcscpy(&wszRetValue[0], &pwsz[1]);
    }
    else
    {
        wcscpy(&wszRetValue[0], pwszFilePath);
    }

    LPWSTR      pwszStoreName;


    pwszStoreName = NULL;

    if (!(CDFPositionAtGroupTag(pCDF, CAT_HEADER_TAG)))
    {
        CloseHandle(hFile);

        delete pCDF;

        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_HEADER, CRYPTCAT_E_CDF_TAGNOTFOUND,  CAT_HEADER_TAG);
        }

        return(NULL);
    }

    if (!(CDFGetParam(pCDF, CAT_HEADER_TAG, CAT_HEADER_NAME_TAG, &wszRetValue[0], &pwszStoreName, NULL)))
    {
        DELETE_OBJECT(pwszStoreName);

        CloseHandle(hFile);

        delete pCDF;

        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_HEADER, CRYPTCAT_E_CDF_TAGNOTFOUND,  CAT_HEADER_TAG);
        }

        return(NULL);
    }

    //
    //  ResultDir
    //
    CDFPositionAtGroupTag(pCDF, CAT_HEADER_TAG);
    CDFGetParam(pCDF, CAT_HEADER_TAG, CAT_HEADER_RESDIR_TAG, NULL, &pCDF->pwszResultDir, NULL);

    //
    //  actual file
    //
    DWORD  cw;
    LPWSTR pwszFile = NULL;

    cw = wcslen( pwszStoreName );
    if ( pCDF->pwszResultDir != NULL )
    {
        cw += wcslen( pCDF->pwszResultDir );
    }
    cw += wcslen( CRYPTCAT_FILEEXT );
    cw += 2;

    pwszFile = new WCHAR [ cw ];
    if ( pwszFile == NULL )
    {
        DELETE_OBJECT(pwszStoreName);

        CloseHandle(hFile);

        delete pCDF;

        return( NULL );
    }

    pwszFile[ 0 ] = L'\0';

    if (pCDF->pwszResultDir)
    {
        wcscpy(pwszFile, pCDF->pwszResultDir);

        if (pCDF->pwszResultDir[wcslen(pCDF->pwszResultDir) - 1] != L'\\')
        {
            wcscat(pwszFile, L"\\");
        }
    }

    wcscat(pwszFile, pwszStoreName);

    if (!(wcsrchr(pwszFile, '.')))
    {
        wcscat(pwszFile, L".");
        wcscat(pwszFile, CRYPTCAT_FILEEXT);
    }


    DWORD   dwPublicVersion;
    DWORD   dwEncodingType;

    //
    //  PublicVersion
    //
    CDFPositionAtGroupTag(pCDF, CAT_HEADER_TAG);
    wcscpy(&wszRetValue[0], L"0x00000001");
    CDFGetParam(pCDF, CAT_HEADER_TAG, CAT_HEADER_VERSION_TAG, &wszRetValue[0], &pwsz, NULL);
    if (pwsz)
    {
        dwPublicVersion = wcstol(pwsz, NULL, 16);
        delete pwsz;
    }

    //
    //  EncodingType
    //
    CDFPositionAtGroupTag(pCDF, CAT_HEADER_TAG);
    wcscpy(&wszRetValue[0], L"0x00010001");   // PKCS_7_ASN_ENCODING | X509_ASN_ENCODING
    CDFGetParam(pCDF, CAT_HEADER_TAG, CAT_HEADER_ENCODETYPE_TAG, &wszRetValue[0], &pwsz, NULL);
    if (pwsz)
    {
        dwEncodingType = wcstol(pwsz, NULL, 16);
        delete pwsz;
    }

    pCDF->hCATStore = CryptCATOpen(pwszFile, CRYPTCAT_OPEN_CREATENEW, NULL, dwPublicVersion, dwEncodingType);

    delete pwszStoreName;
    delete pwszFile;

    if ((pCDF->hCATStore == INVALID_HANDLE_VALUE) ||
        (!(pCDF->hCATStore)))
    {
        CryptCATCDFClose(pCDF);
        pCDF = NULL;
    }

    return(pCDF);
}

BOOL WINAPI CryptCATCDFClose(CRYPTCATCDF *pCDF)
{
    BOOL    fRet;

    if (!(pCDF) ||
        (pCDF->cbStruct != sizeof(CRYPTCATCDF)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    fRet = TRUE;

    if ((pCDF->hFile) && (pCDF->hFile != INVALID_HANDLE_VALUE))
    {
        fRet &= CloseHandle(pCDF->hFile);
    }

    if ((pCDF->hCATStore) && (pCDF->hCATStore != INVALID_HANDLE_VALUE))
    {
        fRet &= CatalogSaveP7UData((CRYPTCATSTORE *)pCDF->hCATStore);

        fRet &= CryptCATClose(pCDF->hCATStore);
    }

    DELETE_OBJECT(pCDF->pwszResultDir);

    delete pCDF;

    return(fRet);
}

CRYPTCATATTRIBUTE * WINAPI CryptCATCDFEnumCatAttributes(CRYPTCATCDF *pCDF,
                                                        CRYPTCATATTRIBUTE *pPrevAttr,
                                                        PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    if (!(pCDF) ||
        (pCDF->cbStruct != sizeof(CRYPTCATCDF)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    LPWSTR              pwsz;
    LPWSTR              pwszOID;
    LPWSTR              pwszValue;
    int                 iAttr;
    WCHAR               wszRetValue[MAX_CDF_LINE_LEN + 4];
    WCHAR               wszTemp[64];
    DWORD               dwType;
    CRYPTCATATTRIBUTE   *pAttr;


    iAttr = (pPrevAttr) ? pPrevAttr->dwReserved + 1 : 1;

    wcscpy(&wszRetValue[0], CAT_HEADER_ATTR_TAG);
    wcscat(&wszRetValue[0], _itow(iAttr, &wszTemp[0], 10));

    pwsz    = NULL;
    pAttr   = NULL;

    CDFPositionAtGroupTag(pCDF, CAT_HEADER_TAG);
    if (CDFGetParam(pCDF, CAT_HEADER_TAG, &wszRetValue[0], NULL, &pwsz, NULL))
    {
        if (pwsz)
        {
            if (CDFSplitAttrLine(pwsz,  &dwType, &pwszOID, &pwszValue, pfnParseError))
            {
                if (dwType & CRYPTCAT_ATTR_NAMEOBJID)
                {
                    //
                    //  make sure we have a valid objid in the name.
                    //  we might do something better than this (???)
                    //
                    if (!(CDFCheckOID(pwszOID, pfnParseError)))
                    {
                        delete pwsz;

                        return(NULL);
                    }
                }

                if (dwType & CRYPTCAT_ATTR_UNAUTHENTICATED)
                {
                    if (pfnParseError)
                    {
                        pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_UNSUPPORTED, pwsz);
                    }
                }
                else if (((dwType & CRYPTCAT_ATTR_NAMEOBJID) ||
                         (dwType & CRYPTCAT_ATTR_NAMEASCII)) &&

                         ((dwType & CRYPTCAT_ATTR_DATABASE64) ||
                          (dwType & CRYPTCAT_ATTR_DATAASCII)))
                {
                    pAttr = CryptCATPutCatAttrInfo(pCDF->hCATStore, pwszOID, dwType,
                                                    (wcslen(pwszValue) + 1) * sizeof(WCHAR),
                                                    (BYTE *)pwszValue);
                    if (pAttr)
                    {
                        pAttr->dwReserved = iAttr;
                    }
                }
                else
                {
                    if (pfnParseError)
                    {
                        pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TYPECOMBO,
                                        pwsz);
                    }
                }
            }
        }
    }

    DELETE_OBJECT(pwsz);

    return(pAttr);
}

CRYPTCATMEMBER * WINAPI CryptCATCDFEnumMembers(CRYPTCATCDF *pCDF, CRYPTCATMEMBER *pPrevMember,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    LPWSTR  pwszLastTag;
    BOOL    fFoundLastTag;

    pwszLastTag = NULL;

    if (pPrevMember)
    {
        if (pPrevMember->cbStruct != sizeof(CRYPTCATMEMBER))
        {
            SetLastError(ERROR_INVALID_PARAMETER);

            return(NULL);
        }

        if (pPrevMember->pwszReferenceTag)
        {
            if (!(pwszLastTag = (LPWSTR)CatalogNew(wcslen(pPrevMember->pwszReferenceTag) *
                                                   sizeof(WCHAR) + 4)))
            {
                return(NULL);
            }

            wcscpy(pwszLastTag, pPrevMember->pwszReferenceTag);
        }
    }

    if (!(pCDF) ||
        (pCDF->hFile == INVALID_HANDLE_VALUE) ||
        !(pCDF->hFile))
    {
        DELETE_OBJECT(pwszLastTag);

        SetLastError(ERROR_INVALID_PARAMETER);

        return(NULL);
    }

    WCHAR   wszRetValue[MAX_CDF_LINE_LEN + 4];

    CDFPositionAtLastMember(pCDF);

    if (CDFGetNextMember(pCDF, &wszRetValue[0], pwszLastTag))
    {
        LPWSTR  pwsz;

        DELETE_OBJECT(pwszLastTag);

        //
        //  file path/name (required!)
        //
        CDFPositionAtLastMember(pCDF);
        if (!(CDFGetParam(pCDF, CAT_MEMBER_TAG, &wszRetValue[0], NULL, &pwsz, &wszRetValue[0])))
        {
            if (pfnParseError)
            {
                pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_FILE_PATH,
                                &wszRetValue[0]);
            }
        }
        else
        {
            CRYPTCATMEMBER      *pMember;
            WCHAR               *pwszFileName;
            WCHAR               *pwszReferenceTag;
            GUID                gSubjectType;
            HANDLE              hFile;

            //
            //  file path/name
            //
            pwszFileName    = pwsz;
            // remember: don't delete pwsz this time!

            if ((hFile = CreateFileU(pwszFileName,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL)) == INVALID_HANDLE_VALUE)
            {
                if (pfnParseError)
                {
                    pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_FILENOTFOUND,
                                    pwszFileName);
                }

                DELETE_OBJECT(pwszFileName);

                return(NULL);
            }

            CloseHandle(hFile);

            //
            //  reference tag
            //
            if (!(pwszReferenceTag = (LPWSTR)CatalogNew((wcslen(&wszRetValue[0]) + 1) * sizeof(WCHAR))))
            {
                delete pwszFileName;

                return(NULL);
            }

            wcscpy(pwszReferenceTag, &wszRetValue[0]);

            //
            //  Alt SIP GUID
            //
            wcscpy(&wszRetValue[0], pwszReferenceTag);
            wcscat(&wszRetValue[0], CAT_MEMBER_ALTSIP_TAG);

            CDFPositionAtLastMember(pCDF);
            CDFGetParam(pCDF, CAT_MEMBER_TAG, &wszRetValue[0], NULL, &pwsz, pwszReferenceTag);

            if (pwsz)
            {
                CDFTextToGUID(pwsz, &gSubjectType, pfnParseError);

                DELETE_OBJECT(pwszFileName);

                DELETE_OBJECT(pwsz);
            }
            else
            {
                if (!(CryptSIPRetrieveSubjectGuid(pwszFileName, NULL, &gSubjectType)))
                {
                    GUID    gFlat = CRYPT_SUBJTYPE_FLAT_IMAGE;

                    memcpy(&gSubjectType, &gFlat, sizeof(GUID));
                }
            }

            //
            //  Indirect Data
            //
            BYTE                *pbIndirectData;
            DWORD               cbIndirectData;
            DWORD               dwCertVersion;

            if (!(CDFCalcIndirectData(pCDF, pwszFileName, &gSubjectType, &cbIndirectData, &pbIndirectData,
                                        &dwCertVersion, pfnParseError)))
            {
                DELETE_OBJECT(pwszReferenceTag);
                DELETE_OBJECT(pwszFileName);

                return(NULL);
            }

            pMember = CryptCATPutMemberInfo(pCDF->hCATStore,
                                            pwszFileName,
                                            pwszReferenceTag,
                                            &gSubjectType,
                                            dwCertVersion,
                                            cbIndirectData,
                                            pbIndirectData);

            if (!(pMember) && (GetLastError() == CRYPT_E_EXISTS))
            {
                if (pfnParseError)
                {
                    pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_DUPLICATE,
                                    pwszReferenceTag);
                }
            }

            DELETE_OBJECT(pbIndirectData);

            //
            //  Done!
            //

            DELETE_OBJECT(pwszReferenceTag);
            DELETE_OBJECT(pwszFileName);

            return(pMember);
        }
    }

    DELETE_OBJECT(pwszLastTag);

    return(NULL);
}

LPWSTR WINAPI CryptCATCDFEnumMembersByCDFTagEx(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember, BOOL fContinueOnError,
                                       LPVOID pvReserved)
{
    LPWSTR  pwszLastTag;
    BOOL    fFoundLastTag;

    pwszLastTag = pwszPrevCDFTag;

    if (!(pCDF) ||
        (pCDF->hFile == INVALID_HANDLE_VALUE) ||
        !(pCDF->hFile))
    {
        DELETE_OBJECT(pwszLastTag);

        SetLastError(ERROR_INVALID_PARAMETER);

        return(NULL);
    }

    WCHAR   wszRetValue[MAX_CDF_LINE_LEN + 4];

    CDFPositionAtLastMember(pCDF);

    if (CDFGetNextMember(pCDF, &wszRetValue[0], pwszLastTag))
    {
        LPWSTR  pwsz;

        DELETE_OBJECT(pwszLastTag);

        //
        //  file path/name (required!)
        //
        CDFPositionAtLastMember(pCDF);
        if (!(CDFGetParam(pCDF, CAT_MEMBER_TAG, &wszRetValue[0], NULL, &pwsz, &wszRetValue[0])))
        {
            if (pfnParseError)
            {
                pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_FILE_PATH,
                                &wszRetValue[0]);
            }
        }
        else
        {
            CRYPTCATMEMBER      *pMember;
            WCHAR               *pwszFileName;
            WCHAR               *pwszReferenceTag;
            GUID                gSubjectType;
            HANDLE              hFile;

            //
            //  reference tag
            //
            if (!(pwszReferenceTag = (LPWSTR)CatalogNew((wcslen(&wszRetValue[0]) + 1) * sizeof(WCHAR))))
            {
                return(NULL);
            }

            wcscpy(pwszReferenceTag, &wszRetValue[0]);

            //
            //  file path/name
            //
            pwszFileName    = pwsz;
            // remember: don't delete pwsz this time!

            if ((hFile = CreateFileU(pwszFileName,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL)) == INVALID_HANDLE_VALUE)
            {
                if (pfnParseError)
                {
                    pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_FILENOTFOUND,
                                    pwszFileName);
                }

                DELETE_OBJECT(pwszFileName);

                if ( fContinueOnError == FALSE )
                {
                    DELETE_OBJECT(pwszReferenceTag);
                    pwszReferenceTag = NULL;
                }

                return(pwszReferenceTag);
            }

            CloseHandle(hFile);

            //
            //  Alt SIP GUID
            //
            wcscpy(&wszRetValue[0], pwszReferenceTag);
            wcscat(&wszRetValue[0], CAT_MEMBER_ALTSIP_TAG);

            CDFPositionAtLastMember(pCDF);
            CDFGetParam(pCDF, CAT_MEMBER_TAG, &wszRetValue[0], NULL, &pwsz, pwszReferenceTag);

            if (pwsz)
            {
                CDFTextToGUID(pwsz, &gSubjectType, pfnParseError);

                DELETE_OBJECT(pwszFileName);

                DELETE_OBJECT(pwsz);
            }
            else
            {
                if (!(CryptSIPRetrieveSubjectGuid(pwszFileName, NULL, &gSubjectType)))
                {
                    GUID    gFlat = CRYPT_SUBJTYPE_FLAT_IMAGE;

                    memcpy(&gSubjectType, &gFlat, sizeof(GUID));
                }
            }

            //
            //  Indirect Data
            //
            BYTE                *pbIndirectData;
            DWORD               cbIndirectData;
            DWORD               dwCertVersion;
            SIP_INDIRECT_DATA*  pIndirectData;
            LPWSTR              pwszTagToPut;
            BOOL                fHashTagUsed = FALSE;

            if (!(CDFCalcIndirectData(pCDF, pwszFileName, &gSubjectType, &cbIndirectData, &pbIndirectData,
                                        &dwCertVersion, pfnParseError)))
            {
                DELETE_OBJECT(pwszFileName);

                if ( fContinueOnError == FALSE )
                {
                    DELETE_OBJECT(pwszReferenceTag);
                    pwszReferenceTag = NULL;
                }

                return(pwszReferenceTag);
            }

            pIndirectData = (SIP_INDIRECT_DATA *)pbIndirectData;
            pwszTagToPut = pwszReferenceTag;

            if (_wcsnicmp(pwszReferenceTag, CAT_MEMBER_HASH_TAG, wcslen(CAT_MEMBER_HASH_TAG)) == 0)
            {
                fHashTagUsed = TRUE;

                if (MsCatConstructHashTag(
                         pIndirectData->Digest.cbData,
                         pIndirectData->Digest.pbData,
                         &pwszTagToPut
                         ) == FALSE)
                {
                    DELETE_OBJECT(pwszFileName);

                    if ( fContinueOnError == FALSE )
                    {
                        DELETE_OBJECT(pwszReferenceTag);
                        pwszReferenceTag = NULL;
                    }

                    return(pwszReferenceTag);
                }
            }

            pMember = CryptCATPutMemberInfo(pCDF->hCATStore,
                                            pwszFileName,
                                            pwszTagToPut,
                                            &gSubjectType,
                                            dwCertVersion,
                                            cbIndirectData,
                                            pbIndirectData);

            if (!(pMember) && (GetLastError() == CRYPT_E_EXISTS))
            {
                if (pfnParseError)
                {
                    pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_DUPLICATE,
                                    pwszReferenceTag);
                }
            }

            DELETE_OBJECT(pbIndirectData);

            //
            //  Done!
            //

            if ( fHashTagUsed == TRUE )
            {
                MsCatFreeHashTag(pwszTagToPut);
            }

            DELETE_OBJECT(pwszFileName);

            *ppMember = pMember;

            return(pwszReferenceTag);
        }
    }

    DELETE_OBJECT(pwszLastTag);

    return(NULL);
}

LPWSTR WINAPI CryptCATCDFEnumMembersByCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszPrevCDFTag,
                                       PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError,
                                       CRYPTCATMEMBER** ppMember)
{
    return(CryptCATCDFEnumMembersByCDFTagEx(pCDF, pwszPrevCDFTag, pfnParseError, ppMember, FALSE, NULL));
}

BOOL CDFCalcIndirectData(CRYPTCATCDF *pCDF, WCHAR *pwszFileName, GUID *pgSubjectType, DWORD *pcbIndirectData,
                         BYTE **ppbIndirectData, DWORD *pdwCertVersion, PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    SIP_SUBJECTINFO     sSubjInfo;
    SIP_DISPATCH_INFO   sSip;
    CRYPTCATSTORE       *pCatStore;

    *pcbIndirectData    = 0;
    *ppbIndirectData    = NULL;

    pCatStore = (CRYPTCATSTORE *)pCDF->hCATStore;

    memset(&sSubjInfo, 0x00, sizeof(SIP_SUBJECTINFO));
    memset(&sSip,      0x00, sizeof(SIP_DISPATCH_INFO));

    sSubjInfo.cbSize                    = sizeof(SIP_SUBJECTINFO);

    sSubjInfo.hProv                     = pCatStore ->hProv;
    sSubjInfo.DigestAlgorithm.pszObjId  = (char *)CertAlgIdToOID(CALG_SHA1);
    sSubjInfo.dwFlags                   = SPC_INC_PE_RESOURCES_FLAG | SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG |
                                          MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE;
    sSubjInfo.dwEncodingType            = pCatStore->dwEncodingType;

    sSubjInfo.pgSubjectType             = pgSubjectType;
    sSubjInfo.pwsFileName               = pwszFileName;


    if (!(CryptSIPLoad(pgSubjectType, 0, &sSip)))
    {
        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_INDIRECTDATA, pwszFileName);
        }
        return(FALSE);
    }

    sSip.pfCreate(&sSubjInfo,
                  pcbIndirectData,
                  NULL);

    if (*pcbIndirectData < 1)
    {
        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_INDIRECTDATA, pwszFileName);
        }
        return(FALSE);
    }

    if (!(*ppbIndirectData = (BYTE *)CatalogNew(*pcbIndirectData)))
    {
        *pcbIndirectData = 0;

        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_INDIRECTDATA, pwszFileName);
        }
        return(FALSE);
    }

    if (!(sSip.pfCreate(&sSubjInfo,
                        pcbIndirectData,
                        (SIP_INDIRECT_DATA *)*ppbIndirectData)))
    {
        DELETE_OBJECT(*ppbIndirectData);

        *pcbIndirectData = 0;

        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_MEMBER, CRYPTCAT_E_CDF_MEMBER_INDIRECTDATA, pwszFileName);
        }
        return(FALSE);
    }

    *pdwCertVersion = sSubjInfo.dwIntVersion;

    return(TRUE);
}

CRYPTCATATTRIBUTE * WINAPI CryptCATCDFEnumAttributes(CRYPTCATCDF *pCDF, CRYPTCATMEMBER *pMember,
                                             CRYPTCATATTRIBUTE *pPrevAttr,
                                             PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    if (!(pCDF) ||
        (pCDF->cbStruct != sizeof(CRYPTCATCDF)) ||
        !(pMember) ||
        (pMember->cbStruct != sizeof(CRYPTCATMEMBER)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    LPWSTR              pwsz;
    LPWSTR              pwszOID;
    LPWSTR              pwszValue;
    int                 iAttr;
    WCHAR               wszRetValue[MAX_CDF_LINE_LEN + 4];
    WCHAR               wszTemp[64];
    DWORD               dwType;
    CRYPTCATATTRIBUTE   *pAttr;


    iAttr = (pPrevAttr) ? pPrevAttr->dwReserved + 1 : 1;

    wcscpy(&wszRetValue[0], pMember->pwszReferenceTag);
    wcscat(&wszRetValue[0], L"ATTR");
    wcscat(&wszRetValue[0], _itow(iAttr, &wszTemp[0], 10));

    pwsz    = NULL;
    pAttr   = NULL;

    CDFPositionAtLastMember(pCDF);
    if (CDFGetParam(pCDF, CAT_MEMBER_TAG, &wszRetValue[0], NULL, &pwsz, pMember->pwszReferenceTag))
    {
        if (pwsz)
        {
            if (CDFSplitAttrLine(pwsz,  &dwType, &pwszOID, &pwszValue, pfnParseError))
            {
                if (dwType & CRYPTCAT_ATTR_NAMEOBJID)
                {
                    //
                    //  make sure we have a valid objid in the name.
                    //  we might do something better than this (???)
                    //
                    if (!(CDFCheckOID(pwszOID, pfnParseError)))
                    {
                        delete pwsz;

                        return(NULL);
                    }
                }

                if (dwType & CRYPTCAT_ATTR_UNAUTHENTICATED)
                {
                    if (pfnParseError)
                    {
                        pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_UNSUPPORTED, pwsz);
                    }
                }
                else if (((dwType & CRYPTCAT_ATTR_NAMEOBJID) ||
                         (dwType & CRYPTCAT_ATTR_NAMEASCII)) &&

                         ((dwType & CRYPTCAT_ATTR_DATABASE64) ||
                          (dwType & CRYPTCAT_ATTR_DATAASCII)))
                {
                    pAttr = CryptCATPutAttrInfo(pCDF->hCATStore, pMember, pwszOID, dwType,
                                                (wcslen(pwszValue) + 1) * sizeof(WCHAR),
                                                (BYTE *)pwszValue);
                    if (pAttr)
                    {
                        pAttr->dwReserved = iAttr;
                    }
                }
                else
                {
                    if (pfnParseError)
                    {
                        pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TYPECOMBO,
                                        pwsz);
                    }
                }
            }
        }
    }

    DELETE_OBJECT(pwsz);

    return(pAttr);
}

CRYPTCATATTRIBUTE * WINAPI CryptCATCDFEnumAttributesWithCDFTag(CRYPTCATCDF *pCDF, LPWSTR pwszMemberTag, CRYPTCATMEMBER *pMember,
                                             CRYPTCATATTRIBUTE *pPrevAttr,
                                             PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    if (!(pCDF) ||
        (pCDF->cbStruct != sizeof(CRYPTCATCDF)) ||
        !(pwszMemberTag) ||
        !(pMember) ||
        (pMember->cbStruct != sizeof(CRYPTCATMEMBER)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    LPWSTR              pwsz;
    LPWSTR              pwszOID;
    LPWSTR              pwszValue;
    int                 iAttr;
    WCHAR               wszRetValue[MAX_CDF_LINE_LEN + 4];
    WCHAR               wszTemp[64];
    DWORD               dwType;
    CRYPTCATATTRIBUTE   *pAttr;


    iAttr = (pPrevAttr) ? pPrevAttr->dwReserved + 1 : 1;

    wcscpy(&wszRetValue[0], pwszMemberTag);
    wcscat(&wszRetValue[0], L"ATTR");
    wcscat(&wszRetValue[0], _itow(iAttr, &wszTemp[0], 10));

    pwsz    = NULL;
    pAttr   = NULL;

    CDFPositionAtLastMember(pCDF);
    if (CDFGetParam(pCDF, CAT_MEMBER_TAG, &wszRetValue[0], NULL, &pwsz, pwszMemberTag))
    {
        if (pwsz)
        {
            if (CDFSplitAttrLine(pwsz,  &dwType, &pwszOID, &pwszValue, pfnParseError))
            {
                if (dwType & CRYPTCAT_ATTR_NAMEOBJID)
                {
                    //
                    //  make sure we have a valid objid in the name.
                    //  we might do something better than this (???)
                    //
                    if (!(CDFCheckOID(pwszOID, pfnParseError)))
                    {
                        delete pwsz;

                        return(NULL);
                    }
                }

                if (dwType & CRYPTCAT_ATTR_UNAUTHENTICATED)
                {
                    if (pfnParseError)
                    {
                        pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_UNSUPPORTED, pwsz);
                    }
                }
                else if (((dwType & CRYPTCAT_ATTR_NAMEOBJID) ||
                         (dwType & CRYPTCAT_ATTR_NAMEASCII)) &&

                         ((dwType & CRYPTCAT_ATTR_DATABASE64) ||
                          (dwType & CRYPTCAT_ATTR_DATAASCII)))
                {
                    pAttr = CryptCATPutAttrInfo(pCDF->hCATStore, pMember, pwszOID, dwType,
                                                (wcslen(pwszValue) + 1) * sizeof(WCHAR),
                                                (BYTE *)pwszValue);
                    if (pAttr)
                    {
                        pAttr->dwReserved = iAttr;
                    }
                }
                else
                {
                    if (pfnParseError)
                    {
                        pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TYPECOMBO,
                                        pwsz);
                    }
                }
            }
        }
    }

    DELETE_OBJECT(pwsz);

    return(pAttr);
}

/////////////////////////////////////////////////////////////////////////////
//
//  Local Functions
//

BOOL CDFCheckOID(LPWSTR pwszOID, PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    DWORD   cbConv;
    char    *pszOID;

    cbConv = WideCharToMultiByte(0, 0,
                                pwszOID, wcslen(pwszOID),
                                NULL, 0, NULL, NULL);
    if (cbConv < 1)
    {
        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TYPECOMBO, pwszOID);
        }
        return(FALSE);
    }

    if (!(pszOID = (LPSTR)CatalogNew(cbConv)))
    {
        return(FALSE);
    }

    WideCharToMultiByte(0, 0,
                        pwszOID, wcslen(pwszOID),
                        pszOID, cbConv, NULL, NULL);

    DWORD   i;
    BOOL    fRet;

    fRet    = TRUE;
    i       = 0;

    while (i < cbConv)
    {
        if (((pszOID[i] < '0') || (pszOID[i] > '9')) &&
            (pszOID[i] != '.'))
        {
            fRet = FALSE;
            break;
        }

        i++;
    }

    delete pszOID;

    if (!(fRet))
    {
        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TYPECOMBO, pwszOID);
        }
    }

    return(fRet);

}

BOOL CDFSplitAttrLine(LPWSTR pwszLine, DWORD *pdwType, LPWSTR *ppwszOID, LPWSTR *ppwszValue,
                      PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    LPWSTR  pwszColon;
    LPWSTR  pwszStart;
    LPWSTR  pwsz;

    *pdwType    = 0;
    *ppwszValue = NULL;
    *ppwszOID   = NULL;

    if (!(pwsz = (WCHAR *)CatalogNew((wcslen(pwszLine) + 1) * sizeof(WCHAR))))
    {
        return(FALSE);
    }

    wcscpy(pwsz, pwszLine);

    pwszStart   = pwszLine;
    //
    //  first one is type
    //
    if (!(pwszColon = wcschr(pwszStart, L':')))
    {
        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TOOFEWVALUES, pwsz);
        }

        delete pwsz;

        return(FALSE);
    }

    *pwszColon  = NULL;
    *pdwType    = wcstol(pwszStart, NULL, 16);

    pwszStart   = &pwszColon[1];

    //
    //  next, oid/name
    //
    if (!(pwszColon = wcschr(pwszStart, L':')))
    {
        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TOOFEWVALUES, pwsz);
        }

        delete pwsz;

        return(FALSE);
    }

    *pwszColon  = NULL;
    *ppwszOID   = pwszStart;

    pwszStart   = &pwszColon[1];

    //
    //  next, value
    //
    if (!(pwszStart[0]))
    {
        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_ATTR_TOOFEWVALUES, pwsz);
        }

        delete pwsz;

        return(FALSE);
    }

    delete pwsz;

    *ppwszValue = pwszStart;

    return(TRUE);
}

void CDFTextToGUID(LPWSTR pwszText, GUID *pgBin, PFN_CDF_PARSE_ERROR_CALLBACK pfnParseError)
{
    WCHAR   wszGuid[256];
    GUID    gTemp;

    memset(pgBin, 0x00, sizeof(GUID));

    if ((pwszText[0] != L'[') &&
        (pwszText[0] != L'{'))
    {
        wcscpy(&wszGuid[0], L"{");
        wcscat(&wszGuid[0], pwszText);
        wcscat(&wszGuid[0], L"}");
    }
    else
    {
        wcscpy(&wszGuid[0], pwszText);
    }

    if (!(wstr2guid(&wszGuid[0], pgBin)))
    {
        if (pfnParseError)
        {
            pfnParseError(CRYPTCAT_E_AREA_ATTRIBUTE, CRYPTCAT_E_CDF_BAD_GUID_CONV, &wszGuid[0]);
        }
    }
}

BOOL CDFPositionAtGroupTag(CRYPTCATCDF *pCDF, LPWSTR pwszTag)
{
    if (SetFilePointer(pCDF->hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    WCHAR       wszRetValue[MAX_CDF_LINE_LEN + 4];
    DWORD       ccRet;

    while ((ccRet = CDFGetLine(pCDF, &wszRetValue[0], MAX_CDF_LINE_LEN * sizeof(WCHAR))) > 0)
    {
        if (wszRetValue[0] == L'#')
        {
            continue;
        }

        CDFEOLOut(&wszRetValue[0], ccRet);

        if (wszRetValue[0] == L'[')
        {
            if (_memicmp(&wszRetValue[0], pwszTag, wcslen(pwszTag) * sizeof(WCHAR)) == 0)
            {
                return(TRUE);
            }
        }
    }

    return(FALSE);
}

BOOL CDFPositionAtLastMember(CRYPTCATCDF *pCDF)
{
    if (pCDF->dwLastMemberOffset == 0)
    {
        return(CDFPositionAtGroupTag(pCDF, CAT_MEMBER_TAG));
    }
    else if (SetFilePointer(pCDF->hFile, pCDF->dwLastMemberOffset,
                                            NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL CDFGetNextMember(CRYPTCATCDF *pCDF, LPWSTR pwszMember, LPWSTR pwszLastMember)
{
    WCHAR   wszLine[MAX_CDF_LINE_LEN + 4];
    WCHAR   wszCheck[MAX_CDF_LINE_LEN + 1];
    LPWSTR  pwszEqual;
    DWORD   ccRet;
    DWORD   ccLastMember;
    BOOL    fFoundLast;

    if (pwszLastMember)
    {
        wcscpy(&wszCheck[0], pwszLastMember);

        ccLastMember = wcslen(&wszCheck[0]);
    }

    fFoundLast = FALSE;

    while ((ccRet = CDFGetLine(pCDF, &wszLine[0], MAX_CDF_LINE_LEN * sizeof(WCHAR))) > 0)
    {
        if (wszLine[0] == L'#')
        {
            continue;
        }

        CDFEOLOut(&wszLine[0], ccRet);

        if (wszLine[0] == L'[')
        {
            return(FALSE);
        }

        if (!(pwszEqual = wcschr(&wszLine[0], L'=')))
        {
            continue;
        }

        *pwszEqual = NULL;

        if (pwszLastMember)
        {
            if (fFoundLast)
            {
                //
                //  before we make the determination that we are in fact on a
                //  different member tag, make sure that we aren't just on the
                //  same tag's ALTSIP or ATTRx!!!
                //
                wcscpy(&wszCheck[ccLastMember], CAT_MEMBER_ALTSIP_TAG);
                if (_memicmp(&wszLine[0], &wszCheck[0], wcslen(&wszCheck[0]) * sizeof(WCHAR)) == 0)
                {
                    continue;
                }

                wcscpy(&wszCheck[ccLastMember], CAT_MEMBER_ATTR_TAG);
                if (_memicmp(&wszLine[0], &wszCheck[0], wcslen(&wszCheck[0]) * sizeof(WCHAR)) == 0)
                {
                    continue;
                }

                if (_wcsicmp(&wszLine[0], pwszLastMember) != 0)
                {
                    wcscpy(pwszMember, &wszLine[0]);

                    //
                    //  remember the position of the last entry for this member
                    //
                    *pwszEqual = L'=';
                    pCDF->dwLastMemberOffset    = pCDF->dwCurFilePos - wcslen(&wszLine[0]);

                    return(TRUE);
                }
            }
            else if (_wcsicmp(&wszLine[0], pwszLastMember) == 0)
            {
                fFoundLast = TRUE;
            }

            continue;
        }

        wcscpy(pwszMember, &wszLine[0]);

        //
        //  remember the position of the last entry for this member
        //
        *pwszEqual = L'=';
        pCDF->dwLastMemberOffset    = pCDF->dwCurFilePos - wcslen(&wszLine[0]);

        return(TRUE);
    }

    return(FALSE);
}

BOOL CDFGetParam(CRYPTCATCDF *pCDF, LPWSTR pwszGroup, LPWSTR pwszItem, LPWSTR pwszDefault, LPWSTR *ppwszRet,
                 LPWSTR pwszMemberTag)
{
    WCHAR   wszRetValue[MAX_CDF_LINE_LEN + 4];
    DWORD   ccRet;
    WCHAR   *pwsz;

    while ((ccRet = CDFGetLine(pCDF, &wszRetValue[0], MAX_CDF_LINE_LEN * sizeof(WCHAR))) > 0)
    {
        if (wszRetValue[0] == L'#')
        {
            continue;
        }

        CDFEOLOut(&wszRetValue[0], ccRet);

        if (wszRetValue[0] == L'[')
        {
            break;
        }

        if (pwsz = wcschr(&wszRetValue[0], L'='))
        {
            //
            //  if we have a member tag and we are past it, get out!
            //
            if (pwszMemberTag)
            {
                if (_memicmp(&wszRetValue[0], pwszMemberTag, wcslen(pwszMemberTag) * sizeof(WCHAR)) != 0)
                {
                    break;
                }
            }

            *pwsz = NULL;

            if (_memicmp(&wszRetValue[0], pwszItem, wcslen(pwszItem) * sizeof(WCHAR)) == 0)
            {
                if (wcslen(&pwsz[1]) < 1)
                {
                    break;
                }

                if (*ppwszRet = (LPWSTR)CatalogNew((wcslen(&pwsz[1]) + 1) * sizeof(WCHAR)))
                {
                    wcscpy(*ppwszRet, &pwsz[1]);

                    return(TRUE);
                }

                return(FALSE);
            }
        }
    }

    if (pwszDefault)
    {
        if (*ppwszRet = (LPWSTR)CatalogNew((wcslen(pwszDefault) + 1) * sizeof(WCHAR)))
        {
            wcscpy(*ppwszRet, pwszDefault);

            return(TRUE);
        }
    }

    *ppwszRet = NULL;

    return(FALSE);
}

DWORD CDFGetLine(CRYPTCATCDF *pCDF, LPWSTR pwszLineBuf, DWORD cbMaxRead)
{
        DWORD   dwHold;
        DWORD   cbRead;
    DWORD   cwbRead;
        DWORD   dw;
    int     iAmt;
    BYTE    *pb;

    if ((dwHold = SetFilePointer(pCDF->hFile, 0, NULL, FILE_CURRENT)) == 0xFFFFFFFF)
    {
        return(0);
    }

    if (!(pb = (BYTE *)CatalogNew(cbMaxRead + 2)))
    {
        return(0);
    }

    cbRead = 0;

    if (ReadFile(pCDF->hFile, pb, cbMaxRead, &cbRead, NULL))
    {
        if (cbRead == 0)
        {
            pCDF->fEOF = TRUE;

            delete pb;

            return(0);
        }

        pb[cbRead] = 0x00;

        pCDF->fEOF = FALSE;

        if (cbRead > 0)
        {
            iAmt = 0;
                    for (dw = 0; dw < (cbRead - 1); dw++)
                    {
                        if ((pb[dw] == 0x0d) ||
                    (pb[dw] == 0x0a))
                        {
                    iAmt++;
                                if (pb[dw + 1] == 0x0a)
                                {
                        dw++;
                        iAmt++;
                                }

                    if (SetFilePointer(pCDF->hFile, dwHold + (dw + 1),
                                        NULL, FILE_BEGIN) == 0xFFFFFFFF)
                    {
                        pCDF->dwCurFilePos = 0;
                    }
                    else
                    {
                        pCDF->dwCurFilePos = SetFilePointer(pCDF->hFile, 0, NULL, FILE_CURRENT) - iAmt;
                    }

                                pb[dw + 1] = 0x00;

                    cwbRead = MultiByteToWideChar(CP_ACP, 0, (const char *)pb, -1,
                                                    pwszLineBuf, cbMaxRead / sizeof(WCHAR));

                    delete pb;

                                return(cwbRead + 1);
                        }
                    }
        }
        }
        else
        {
        delete pb;

                return(0);
        }

        if (pb[cbRead - 1] == 0x1a)  /* EOF */
        {
                cbRead--;
        pCDF->dwCurFilePos  = 0;
        pCDF->fEOF          = TRUE;
        }
    else
    {
        pCDF->dwCurFilePos = dwHold;
    }

        pb[cbRead] = 0x00;

    cwbRead = MultiByteToWideChar(CP_ACP, 0, (const char *)pb, -1,
                                  pwszLineBuf, cbMaxRead / sizeof(WCHAR));


    delete pb;

        return(cwbRead);
}

void CDFEOLOut(WCHAR *pwsz, DWORD ccLen)
{
        DWORD   i;

        for (i = 0; i < ccLen; i++)
        {
                if ((pwsz[i] == (WCHAR)0x0a) || (pwsz[i] == (WCHAR)0x0d))
                {
                        pwsz[i] = NULL;
                        return;
                }
        }
        pwsz[ccLen] = NULL;
}


