//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mscatctl.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//              implements the Certificate Trust List & persistent storage
//
//  Functions:  CatalogLoadFileData
//              CatalogSaveP7UData
//              CatalogSaveP7SData
//              IsCatalogFile
//
//              *** local functions ***
//              CatalogLoadData
//              CatalogFillCatStore
//              CatalogFillCTL
//              CatalogFillCTLAttr
//              CatalogFreeCTL
//              CatalogFreeCTLAttr
//              CatalogFillCatMember
//              CatalogFillMemAttr
//              CatalogFillCatAttr
//              CatalogFillCatLevelAttr
//
//  History:    05-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "mscat32.h"

//
//  for each member, we have at minimum two authenticated attributes.
//      1 = Indirect Data
//      2 = Subject Guid
//
#define     CAT_RESERVE_CTL_ATTR        2


// WARNING: this function allocates pbData -- you must delete!
BOOL            CatalogLoadData(WCHAR *pwszCatFile, DWORD *cbData, BYTE **pbData);

BOOL            CatalogFillCatStore(CRYPTCATSTORE *pCat, PCTL_INFO pCTLInfo);
BOOL            CatalogFreeCTL(CTL_INFO *pCTL);
BOOL            CatalogFreeCTLAttr(CRYPT_ATTRIBUTE *pCryptAttr);
BOOL            CatalogFillCTL(CRYPTCATSTORE *pCat, CTL_INFO *pCTL);
BOOL            CatalogFillCatAttr(CRYPTCATSTORE *pCat, CERT_EXTENSION *pAttr);
BOOL            CatalogFillCatLevelAttr(CRYPTCATSTORE *pCatStore, CRYPTCATATTRIBUTE *pAttr,
                                        CERT_EXTENSION *pCertAttr);
BOOL            CatalogFillCTLAttr(CRYPTCATSTORE *pCatStore, CRYPTCATATTRIBUTE *pAttr,
                                   PCRYPT_ATTRIBUTE pCryptAttr);

CRYPTCATMEMBER  *CatalogFillCatMember(CRYPTCATSTORE *pCat, CTL_ENTRY *pEntry);
BOOL            CatalogFillMemAttr(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember, CRYPT_ATTRIBUTE *pAttr);

static const char *pszOID = szOID_CATALOG_LIST;

BOOL CatalogLoadFileData(CRYPTCATSTORE *pCat)
{
    BOOL    fRet;
    DWORD   cbData;
    BYTE    *pbData;

    cbData  = 0;
    pbData  = NULL;

    if (!(CatalogLoadData(pCat->pwszP7File, &cbData, &pbData)))
    {
        return(FALSE);
    }

    if (cbData < 1)
    {
        if (pbData)
        {
            UnmapViewOfFile(pbData);
        }

        //
        // not signed and we are probably creating it!
        //
        return(TRUE);
    }

    PCCTL_CONTEXT   pCTLContext;

    pCTLContext = (PCCTL_CONTEXT) CertCreateContext(
        CERT_STORE_CTL_CONTEXT,
        pCat->dwEncodingType,
        pbData,
        cbData,
        CERT_CREATE_CONTEXT_NOCOPY_FLAG |
            CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG,
        NULL                                        // pCreatePara
        );


    if (pCTLContext)
    {
        //
        // got it...  fill our arrays!
        //

        fRet = CatalogFillCatStore(pCat, pCTLContext->pCtlInfo);
        CertFreeCTLContext(pCTLContext);
    } else
        fRet = FALSE;

    UnmapViewOfFile(pbData);

    return(fRet);
}

BOOL CatalogSaveP7SData(CRYPTCATSTORE *pCat, CTL_CONTEXT *pCTLContext)
{
    assert(0);      // should never be called!
    return(TRUE);
}

BOOL CatalogSaveP7UData(CRYPTCATSTORE *pCat)
{
    CMSG_SIGNED_ENCODE_INFO sSignInfo;
    CTL_INFO                sCTLInfo;
    DWORD                   cbEncoded;
    BYTE                    *pbEncoded;
    Stack_                  *pStack;

    //
    //  sort the data...
    //
    if (pCat->hReserved)    // member stack_
    {
        pStack = (Stack_ *)pCat->hReserved;

        pStack->Sort(WVT_OFFSETOF(CRYPTCATMEMBER, pwszReferenceTag), sizeof(WCHAR *), STACK_SORTTYPE_PWSZ);
    }

    memset(&sSignInfo, 0x00, sizeof(CMSG_SIGNED_ENCODE_INFO));
    sSignInfo.cbSize = sizeof(CMSG_SIGNED_ENCODE_INFO);

    if (CatalogFillCTL(pCat, &sCTLInfo))
    {
        cbEncoded = 0;

        CryptMsgEncodeAndSignCTL(   pCat->dwEncodingType,
                                    &sCTLInfo,
                                    &sSignInfo,
                                    0,
                                    NULL,
                                    &cbEncoded);

        if (cbEncoded > 0)
        {
            BOOL    fRet;

            if (!(pbEncoded = (BYTE *)CatalogNew(cbEncoded)))
            {
                CatalogFreeCTL(&sCTLInfo);
                return(FALSE);
            }

            fRet = CryptMsgEncodeAndSignCTL(    pCat->dwEncodingType,
                                                &sCTLInfo,
                                                &sSignInfo,
                                                0,
                                                pbEncoded,
                                                &cbEncoded);
            CatalogFreeCTL(&sCTLInfo);

            if (fRet)
            {
                HANDLE  hFile;
                DWORD   lErr;

                lErr = GetLastError();

                if ((hFile = CreateFileU(pCat->pwszP7File,
                                        GENERIC_WRITE | GENERIC_READ,
                                        0,                 // no sharing!!
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL)) != INVALID_HANDLE_VALUE)
                {
                    DWORD   cbWritten;

                    if (!(WriteFile(hFile, pbEncoded, cbEncoded, &cbWritten, NULL)) ||
                        (cbEncoded != cbWritten))
                    {
                        fRet = FALSE;
                    }

                    CloseHandle(hFile);

                    if (fRet)
                    {
                        SetLastError(lErr);
                    }
                }
            }

            delete pbEncoded;

            return(fRet);
        }

        CatalogFreeCTL(&sCTLInfo);
    }

    return(FALSE);
}

BOOL CatalogLoadData(WCHAR *pwszCatFile, DWORD *cbData, BYTE **pbData)
{
    HANDLE  hFile;

    *cbData = 0;

    if ((hFile = CreateFileU(pwszCatFile,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                             NULL)) == INVALID_HANDLE_VALUE)
    {
        return(FALSE);
    }

    if ((*cbData = GetFileSize(hFile, NULL)) == 0xffffffff)
    {
        *cbData = 0;
        CloseHandle(hFile);
        return(FALSE);
    }

    if (*cbData < 10)
    {
        //
        //  just created file....
        //
        *cbData = 0;
        CloseHandle(hFile);
        return(TRUE);
    }

    HANDLE hMappedFile;

    hMappedFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!(hMappedFile) || (hMappedFile == INVALID_HANDLE_VALUE))
    {
        *cbData = 0;
        CloseHandle(hFile);
        return(FALSE);
    }

    *pbData = (BYTE *)MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0);

    CloseHandle(hMappedFile);

    CloseHandle(hFile);

    if ((*pbData) == NULL)
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL CatalogFillCatStore(CRYPTCATSTORE *pCat, CTL_INFO *pCTL)
{
    int     iAttr;

    if (pCTL->cCTLEntry > 0)
    {
        for (iAttr = 0; iAttr < (int)pCTL->cExtension; iAttr++)
        {
            if (!(CatalogFillCatAttr(pCat, &pCTL->rgExtension[iAttr])))
            {
                return(FALSE);
            }
        }

        CRYPTCATMEMBER  *pMember;

        for (int iEntry = 0; iEntry < (int)pCTL->cCTLEntry; iEntry++)
        {
            pMember = CatalogFillCatMember(pCat, &pCTL->rgCTLEntry[iEntry]);

            if (!(pMember))
            {
                return(FALSE);
            }

            if (pCTL->rgCTLEntry[iEntry].cAttribute > 0)
            {
                for (iAttr = 0; iAttr < (int)pCTL->rgCTLEntry[iEntry].cAttribute; iAttr++)
                {
                    if (!(CatalogFillMemAttr(pCat, pMember,
                                            &pCTL->rgCTLEntry[iEntry].rgAttribute[iAttr])))
                    {
                        return(FALSE);
                    }
                }
            }
        }

        return(TRUE);
    }

    return(FALSE);
}

CRYPTCATMEMBER *CatalogFillCatMember(CRYPTCATSTORE *pCat, CTL_ENTRY *pEntry)
{
    if (!(pEntry))
    {
        return(NULL);
    }

    Stack_          *pStack;
    CRYPTCATMEMBER  *pCatMember;

    if (!(pCat->hReserved))
    {
        pStack = new Stack_(&MSCAT_CriticalSection);

        if (!(pStack))
        {
            assert(0);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }

        pCat->hReserved = (HANDLE)pStack;
    }

    pStack = (Stack_ *)pCat->hReserved;

    if (!(pCatMember = (CRYPTCATMEMBER *)pStack->Add(sizeof(CRYPTCATMEMBER))))
    {
        return(NULL);
    }

    memset(pCatMember, 0x00, sizeof(CRYPTCATMEMBER));

    pCatMember->cbStruct = sizeof(CRYPTCATMEMBER);

    // pwszFileName

    // pwszReferenceTag
    if (!(pCatMember->pwszReferenceTag = (LPWSTR)CatalogNew(pEntry->SubjectIdentifier.cbData)))
    {
        return(NULL);
    }

    memcpy(pCatMember->pwszReferenceTag, pEntry->SubjectIdentifier.pbData,
                                         pEntry->SubjectIdentifier.cbData);

    // pIndirectData (will be filled in while getting attributes!

    // gSubjectType (will be filled in while getting attributes!

    return(pCatMember);
}

BOOL CatalogFillCatAttr(CRYPTCATSTORE *pCat, CERT_EXTENSION *pAttr)
{
    if (!(pAttr))
    {
        return(FALSE);
    }

    Stack_              *pStack;
    CRYPTCATATTRIBUTE   *pCatAttr;

    if (!(pCat->hAttrs))
    {
        pStack = new Stack_(&MSCAT_CriticalSection);

        if (!(pStack))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        pCat->hAttrs = (HANDLE)pStack;
    }

    pStack = (Stack_ *)pCat->hAttrs;

    if (!(pCatAttr = (CRYPTCATATTRIBUTE *)pStack->Add(sizeof(CRYPTCATATTRIBUTE))))
    {
        return(FALSE);
    }

    memset(pCatAttr, 0x00, sizeof(CRYPTCATATTRIBUTE));

    pCatAttr->cbStruct = sizeof(CRYPTCATATTRIBUTE);

    CRYPT_ATTRIBUTE     sCryptAttr;

    CatalogCertExt2CryptAttr(pAttr, &sCryptAttr);

    if (!(CatalogDecodeNameValue(pCat, &sCryptAttr, pCatAttr)))
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL CatalogFillMemAttr(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember, CRYPT_ATTRIBUTE *pAttr)
{
    if (!(pAttr))
    {
        return(FALSE);
    }

    if (strcmp(pAttr->pszObjId, SPC_INDIRECT_DATA_OBJID) == 0)
    {
        return(CatalogDecodeIndirectData(pCat, pMember, pAttr));
    }

    if (strcmp(pAttr->pszObjId, CAT_MEMBERINFO_OBJID) == 0)
    {
        return(CatalogDecodeMemberInfo(pCat, pMember, pAttr));
    }

    Stack_              *pStack;
    CRYPTCATATTRIBUTE   *pCatAttr;

    if (!(pMember->hReserved))
    {
        pStack = new Stack_(&MSCAT_CriticalSection);

        if (!(pStack))
        {
            assert(0);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(FALSE);
        }

        pMember->hReserved = (HANDLE)pStack;
    }

    pStack = (Stack_ *)pMember->hReserved;

    if (!(pCatAttr = (CRYPTCATATTRIBUTE *)pStack->Add(sizeof(CRYPTCATATTRIBUTE))))
    {
        return(FALSE);
    }

    memset(pCatAttr, 0x00, sizeof(CRYPTCATATTRIBUTE));

    pCatAttr->cbStruct = sizeof(CRYPTCATATTRIBUTE);

    if (!(CatalogDecodeNameValue(pCat, pAttr, pCatAttr)))
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL CatalogFillCTL(CRYPTCATSTORE *pCat, CTL_INFO *pCTL)
{
    typedef         HRESULT (WINAPI *pfnCoCreateGuid)(GUID FAR *pguid);
    HINSTANCE       hOLE;
    pfnCoCreateGuid pfnCreateGuid;

    memset(pCTL, 0x00, sizeof(CTL_INFO));

    // dwVersion
    pCTL->dwVersion = CTL_V1;

    // SubjectUsage
    pCTL->SubjectUsage.cUsageIdentifier = 1;
    pCTL->SubjectUsage.rgpszUsageIdentifier = (char **)&pszOID;

    // ListIdentifier
    if (hOLE = LoadLibraryA("OLE32.DLL"))
    {
        if (pfnCreateGuid = (pfnCoCreateGuid)GetProcAddress(hOLE, "CoCreateGuid"))
        {
            if (pCTL->ListIdentifier.pbData = (BYTE *)CatalogNew(sizeof(GUID)))
            {
                pCTL->ListIdentifier.cbData = sizeof(GUID);
                (*pfnCreateGuid)((GUID *)pCTL->ListIdentifier.pbData);
            }
        }

        FreeLibrary(hOLE);
    }

    // SequenceNumber
        // optional!

    // ThisUpdate
    GetSystemTimeAsFileTime(&pCTL->ThisUpdate);

    // NextUpdate
        // optional!

    // SubjectAlgorithm
    pCTL->SubjectAlgorithm.pszObjId = szOID_CATALOG_LIST_MEMBER;


    Stack_              *pStackMember;
    Stack_              *pStackAttr;
    CRYPTCATMEMBER      *pMember;
    CRYPTCATATTRIBUTE   *pAttr;
    DWORD               dwAttr;

    // cCTLEntry & rgCTLEntry
    if (pCat->hReserved)
    {
        pStackMember = (Stack_ *)pCat->hReserved;

        // cCTLEntry
        pCTL->cCTLEntry = pStackMember->Count();

        if (pCTL->cCTLEntry > 0)
        {
            if (!(pCTL->rgCTLEntry = (PCTL_ENTRY)CatalogNew(sizeof(CTL_ENTRY) *
                                                            pStackMember->Count())))
            {
                return(FALSE);
            }

            memset(pCTL->rgCTLEntry, 0x00, sizeof(CTL_ENTRY) * pStackMember->Count());
        }


        DWORD           dwMember;
        DWORD           dwSize;
        CTL_ENTRY       *pCTLEntry;
        //
        //  copy the members to the ctl_entry
        //

        dwMember = 0;

        while (dwMember < pStackMember->Count())
        {
            if (!(pMember = (CRYPTCATMEMBER *)pStackMember->Get(dwMember)))
            {
                return(FALSE);
            }

            //
            //  Subject Identifier
            //
            dwSize = (wcslen(pMember->pwszReferenceTag) + 1) * sizeof(WCHAR);

            pCTLEntry = &pCTL->rgCTLEntry[dwMember];

            if (!(pCTLEntry->SubjectIdentifier.pbData =
                                (BYTE *)CatalogNew(dwSize)))
            {
                return(FALSE);
            }

            memcpy(pCTLEntry->SubjectIdentifier.pbData,
                    pMember->pwszReferenceTag,dwSize);
            pCTLEntry->SubjectIdentifier.cbData = dwSize;

            //
            //  rgAttribute
            //              +1 for Indirect Data
            //              +1 for Subject Guid
            //
            if (pMember->hReserved)
            {
                pStackAttr = (Stack_ *)pMember->hReserved;

                pCTLEntry->cAttribute = pStackAttr->Count() + CAT_RESERVE_CTL_ATTR;
            }
            else
            {
                pCTLEntry->cAttribute = CAT_RESERVE_CTL_ATTR;
            }

            if (!(pCTLEntry->rgAttribute =
                        (PCRYPT_ATTRIBUTE)CatalogNew(sizeof(CRYPT_ATTRIBUTE) *
                                                                    pCTLEntry->cAttribute)))
            {
                return(FALSE);
            }

            memset(pCTLEntry->rgAttribute, 0x00,
                        sizeof(CRYPT_ATTRIBUTE) * pCTLEntry->cAttribute);

            //
            //  put our indirect data in an authenticated attribute
            //
            if (!(pMember->pIndirectData))
            {
                CatalogReallyDecodeIndirectData(pCat, pMember, &pMember->sEncodedIndirectData);
            }

            CatalogEncodeIndirectData(pCat, pMember, &pCTLEntry->rgAttribute[0]);

            //
            //  put our subject guid in an authenticated attribute
            //
            if ((pMember->gSubjectType.Data1 == 0) &&
                (pMember->gSubjectType.Data2 == 0) &&
                (pMember->gSubjectType.Data3 == 0))
            {
                CatalogReallyDecodeMemberInfo(pCat, pMember, &pMember->sEncodedMemberInfo);
            }

            CatalogEncodeMemberInfo(pCat, pMember, &pCTLEntry->rgAttribute[1]);

            if (pMember->hReserved)
            {
                dwAttr = 0;

                while (dwAttr < pStackAttr->Count())
                {
                    pAttr = (CRYPTCATATTRIBUTE *)pStackAttr->Get(dwAttr);

                    CatalogFillCTLAttr(pCat, pAttr,
                                       &pCTLEntry->rgAttribute[dwAttr + CAT_RESERVE_CTL_ATTR]);

                    //
                    //  increment our attribute counter!
                    //
                    dwAttr++;
                }
            }

            //
            //  increment our member counter!
            //
            dwMember++;
        }
    }

    //
    // cExtension
    // rgExtension
    //
    if (pCat->hAttrs)
    {
        pStackAttr = (Stack_ *)pCat->hAttrs;

        pCTL->cExtension = pStackAttr->Count();

        if (!(pCTL->rgExtension =
                        (CERT_EXTENSION *)CatalogNew(sizeof(CERT_EXTENSION) * pCTL->cExtension)))
        {
            return(FALSE);
        }

        memset(pCTL->rgExtension, 0x00, sizeof(CERT_EXTENSION) * pCTL->cExtension);

        dwAttr = 0;

        while (dwAttr < pStackAttr->Count())
        {
            pAttr = (CRYPTCATATTRIBUTE *)pStackAttr->Get(dwAttr);

            if (pAttr)
            {
                CatalogFillCatLevelAttr(pCat, pAttr, &pCTL->rgExtension[dwAttr]);
            }

            dwAttr++;
        }
    }

    return(TRUE);
}

BOOL CatalogFillCatLevelAttr(CRYPTCATSTORE *pCatStore, CRYPTCATATTRIBUTE *pAttr, CERT_EXTENSION *pCertAttr)
{
    CRYPT_ATTR_BLOB sAttrBlob;
    CRYPT_ATTRIBUTE sCryptAttr;

    memset(&sAttrBlob, 0x00, sizeof(CRYPT_ATTR_BLOB));
    memset(&sCryptAttr, 0x00, sizeof(CRYPT_ATTRIBUTE));

    sCryptAttr.cValue  = 1;
    sCryptAttr.rgValue = &sAttrBlob;

    if (!(CatalogEncodeNameValue(pCatStore, pAttr, &sCryptAttr)))
    {
        return(FALSE);
    }

    CatalogCryptAttr2CertExt(&sCryptAttr, pCertAttr);

    return(TRUE);
}

BOOL CatalogFillCTLAttr(CRYPTCATSTORE *pCatStore, CRYPTCATATTRIBUTE *pAttr, PCRYPT_ATTRIBUTE pCryptAttr)
{
    if (!(pCryptAttr->rgValue = (PCRYPT_ATTR_BLOB)CatalogNew(sizeof(CRYPT_ATTR_BLOB))))
    {
        return(FALSE);
    }

    pCryptAttr->cValue = 1;

    memset(pCryptAttr->rgValue, 0x00, sizeof(CRYPT_ATTR_BLOB));

    if (!(CatalogEncodeNameValue(pCatStore, pAttr, pCryptAttr)))
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL CatalogFreeCTL(CTL_INFO *pCTL)
{
    DWORD       dwEntries;
    DWORD       dwAttributes;
    CTL_ENTRY   *pCTLEntry;

    DELETE_OBJECT(pCTL->ListIdentifier.pbData);

    dwEntries = pCTL->cCTLEntry;

    while (dwEntries > 0)
    {
        pCTLEntry = &pCTL->rgCTLEntry[dwEntries - 1];

        DELETE_OBJECT(pCTLEntry->SubjectIdentifier.pbData);

        dwAttributes = pCTLEntry->cAttribute;

        while (dwAttributes > 0)
        {
            CatalogFreeCTLAttr(&pCTLEntry->rgAttribute[dwAttributes - 1]);

            dwAttributes--;
        }

        DELETE_OBJECT(pCTLEntry->rgAttribute);

        dwEntries--;
    }

    DELETE_OBJECT(pCTL->rgCTLEntry);

    for (dwEntries = 0; dwEntries < pCTL->cExtension; dwEntries++)
    {
        DELETE_OBJECT(pCTL->rgExtension[dwEntries].Value.pbData);
    }

    DELETE_OBJECT(pCTL->rgExtension);

    return(TRUE);
}

BOOL CatalogFreeCTLAttr(CRYPT_ATTRIBUTE *pCryptAttr)
{
    if (!(pCryptAttr))
    {
        return(FALSE);
    }

    if (pCryptAttr->rgValue)
    {
        DELETE_OBJECT(pCryptAttr->rgValue->pbData);
        DELETE_OBJECT(pCryptAttr->rgValue);
    }

    return(TRUE);
}

BOOL WINAPI IsCatalogFile(HANDLE hFile, WCHAR *pwszCatalogFile)
{
    char            *pszCatalogListUsageOID = szOID_CATALOG_LIST;
    BOOL            fCloseFile;
    BOOL            fRet;
    DWORD           cbRead;
    DWORD           cbFile;
    BYTE            *pbFile;
    PCCTL_CONTEXT   pCTLContext;


    pCTLContext = NULL;
    pbFile      = NULL;
    fCloseFile  = FALSE;
    fRet        = FALSE;

    if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE))
    {
        if (!(pwszCatalogFile))
        {
            goto IsCatInvalidParam;
        }

        if ((hFile = CreateFileU(pwszCatalogFile, GENERIC_READ, FILE_SHARE_READ, NULL,
                                 OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                 NULL)) == INVALID_HANDLE_VALUE)
        {
            goto IsCatFileError;
        }

        fCloseFile = TRUE;
    }

    HANDLE hMappedFile;

    hMappedFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!(hMappedFile) || (hMappedFile == INVALID_HANDLE_VALUE))
    {
        goto CreateFileMapFailed;
    }

    pbFile = (BYTE *)MapViewOfFile(hMappedFile, FILE_MAP_READ, 0, 0, 0);

    CloseHandle(hMappedFile);

    if (!(pbFile))
    {
        goto MapViewFailed;
    }

    if (((cbFile = GetFileSize(hFile, NULL)) == 0xffffffff) ||
        (cbFile < 1))
    {
        goto FileSizeError;
    }

    if (pbFile[0] != (BYTE)0x30)
    {
        goto IsCatNotCatalog;
    }

    pCTLContext = (PCCTL_CONTEXT) CertCreateContext(
        CERT_STORE_CTL_CONTEXT,
        PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
        pbFile,
        cbFile,
        CERT_CREATE_CONTEXT_NOCOPY_FLAG |
            CERT_CREATE_CONTEXT_NO_HCRYPTMSG_FLAG |
            CERT_CREATE_CONTEXT_NO_ENTRY_FLAG,
        NULL                                        // pCreatePara
        );

    if (pCTLContext)
    {
        if (pCTLContext->pCtlInfo->SubjectUsage.cUsageIdentifier)
        {
            if (strcmp(pCTLContext->pCtlInfo->SubjectUsage.rgpszUsageIdentifier[0],
                        pszCatalogListUsageOID) == 0)
            {
                fRet = TRUE;
                goto CommonReturn;
           }
        }
    }

    goto IsCatNotCatalog;

    CommonReturn:

        if (pCTLContext)
        {
            CertFreeCTLContext(pCTLContext);
        }

        if (pbFile)
        {
            UnmapViewOfFile(pbFile);
        }

        if (fCloseFile)
        {
            CloseHandle(hFile);
        }

        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, IsCatNotCatalog);
    TRACE_ERROR_EX(DBG_SS, IsCatFileError);
    TRACE_ERROR_EX(DBG_SS, CreateFileMapFailed);
    TRACE_ERROR_EX(DBG_SS, MapViewFailed);

    SET_ERROR_VAR_EX(DBG_SS, IsCatInvalidParam, ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, FileSizeError,     ERROR_INVALID_PARAMETER);
}
