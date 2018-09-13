//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mscatapi.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  CryptCATOpen
//              CryptCATClose
//              CryptCATStoreFromHandle
//              CryptCATGetMemberInfo
//              CryptCATPutMemberInfo
//              CryptCATVerifyMember
//              CryptCATGetAttrInfo
//              CryptCATPutAttrInfo
//              CryptCATEnumerateMember
//              CryptCATEnumerateAttr
//
//              *** local functions ***
//
//              CryptCATCreateStore
//              CryptCATOpenStore
//              FillNameValue
//              CatalogCheckForDuplicateMember
//
//  History:    29-Apr-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "base64.h"
#include    "mscat32.h"


BOOL    CryptCATCreateStore(CRYPTCATSTORE *pCat, LPWSTR pwszCatFile);
BOOL    CryptCATOpenStore(CRYPTCATSTORE *pCat, LPWSTR pwszCatFile);
BOOL    FillNameValue(CRYPTCATSTORE *pCatStore, CRYPTCATATTRIBUTE *pAttr);
BOOL    CatalogCheckForDuplicateMember(Stack_ *pMembers, WCHAR *pwszReferenceTag);


/////////////////////////////////////////////////////////////////////////////
//
//  Exported Functions
//  

HANDLE WINAPI CryptCATOpen(LPWSTR pwszCatFile, DWORD fdwOpenFlags, HCRYPTPROV hProv,
                           DWORD dwPublicVersion, DWORD dwEncodingType)
{
    CRYPTCATSTORE   *pCatStore;
    BOOL            fRet;

    fRet = TRUE;

    if (!(pwszCatFile))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return((HANDLE)INVALID_HANDLE_VALUE);
    }

    if (!(pCatStore = (CRYPTCATSTORE *)CatalogNew(sizeof(CRYPTCATSTORE))))
    {
        return((HANDLE)INVALID_HANDLE_VALUE);
    }

    memset(pCatStore, 0x00, sizeof(CRYPTCATSTORE));

    pCatStore->cbStruct         = sizeof(CRYPTCATSTORE);
    pCatStore->hProv            = hProv;
    pCatStore->dwPublicVersion  = (dwPublicVersion) ? dwPublicVersion : 0x00000100;
    pCatStore->dwEncodingType   = (dwEncodingType) ? dwEncodingType :
                                    PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;

    if (!(hProv))
    {
        pCatStore->hProv = I_CryptGetDefaultCryptProv(0);  // get the default and DONT RELEASE IT!!!!

        if (!(pCatStore->hProv))
        {
            fRet &= FALSE;
        }
    }

    if (fdwOpenFlags & CRYPTCAT_OPEN_CREATENEW)
    {
        fRet &= CryptCATCreateStore(pCatStore, pwszCatFile);
    }
    else
    {
        fRet &= CryptCATOpenStore(pCatStore, pwszCatFile);
        
        if (!(fRet) && (fdwOpenFlags & CRYPTCAT_OPEN_ALWAYS))
        {
            fRet &= CryptCATCreateStore(pCatStore, pwszCatFile);
        }
    }

    if (fRet)
    {
        return((HANDLE)pCatStore);
    }

    DWORD   dwLastError;

    dwLastError = GetLastError();
    CryptCATClose((HANDLE)pCatStore);
    SetLastError(dwLastError);

    return(INVALID_HANDLE_VALUE);
}

BOOL WINAPI CryptCATClose(HANDLE hCatalog)
{
    if (!(hCatalog) ||
        (hCatalog == INVALID_HANDLE_VALUE))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    CRYPTCATSTORE   *pCatStore;

    pCatStore = (CRYPTCATSTORE *)hCatalog;

    __try
    {
        if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)))
        {
            SetLastError((DWORD)ERROR_INVALID_PARAMETER);
            return(FALSE);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    BOOL            fRet;
    DWORD           dwlerr;

    dwlerr = GetLastError();

    fRet = TRUE;

    //
    //  file name
    //
    DELETE_OBJECT(pCatStore->pwszP7File);

    //
    //  attributes
    //
    if (_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hAttrs))
    {
        if (pCatStore->hAttrs)
        {
            Stack_              *ps;
            DWORD               cStack;
            CRYPTCATATTRIBUTE   *pAttr;
        
            ps = (Stack_ *)pCatStore->hAttrs;
        
            cStack = 0;

	        while (pAttr = (CRYPTCATATTRIBUTE *)ps->Get(cStack))
	        {
        		CatalogFreeAttribute(pAttr);

        		cStack++;
	        }
        
    	    DELETE_OBJECT(ps);
        
            pCatStore->hAttrs = NULL;
        }
    }

    //
    //  check hReserved to see if we used pStack
    //
    if (pCatStore->hReserved)
    {
        Stack_              *pStack;
        CRYPTCATMEMBER      *pCatMember;
        DWORD               dwCurPos;

        pStack = (Stack_ *)pCatStore->hReserved;

        dwCurPos = 0;

        while (pCatMember = (CRYPTCATMEMBER *)pStack->Get(dwCurPos))
        {
            CatalogFreeMember(pCatMember);
            
            dwCurPos++;
        }

        DELETE_OBJECT(pStack);

        pCatStore->hReserved = NULL;
    }

    pCatStore->cbStruct = 0;    // just in case they try to re-open it!

    DELETE_OBJECT(pCatStore);

    //
    //  restore last error
    //
    SetLastError(dwlerr);

    return(fRet);
}

CRYPTCATSTORE * WINAPI CryptCATStoreFromHandle(IN HANDLE hCatalog)
{
    return((CRYPTCATSTORE *)hCatalog);
}

HANDLE WINAPI CryptCATHandleFromStore(IN CRYPTCATSTORE *pCatStore)
{
    return((HANDLE)pCatStore);
}

BOOL WINAPI CryptCATPersistStore(IN HANDLE hCatalog)
{
    CRYPTCATSTORE   *pStore;

    if (!(hCatalog) ||
        (hCatalog == INVALID_HANDLE_VALUE))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pStore = CryptCATStoreFromHandle(hCatalog);

    if (!(pStore->pwszP7File))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    return(CatalogSaveP7UData(pStore));
}

CRYPTCATATTRIBUTE * WINAPI CryptCATGetCatAttrInfo(HANDLE hCatalog, LPWSTR pwszReferenceTag)
{
    CRYPTCATATTRIBUTE   *pAttr;
    CRYPTCATSTORE       *pCatStore;

    pAttr   = NULL;

    if (!(hCatalog) ||
        (hCatalog == INVALID_HANDLE_VALUE) ||
        !(pwszReferenceTag))
    {
        goto ErrorInvalidParam;
    }

    pCatStore = CryptCATStoreFromHandle(hCatalog);


    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hAttrs)))
    {
        goto ErrorInvalidParam;
    }

    while (pAttr = CryptCATEnumerateCatAttr(hCatalog, pAttr))
    {
        if (pAttr->pwszReferenceTag)
        {
            if (_wcsicmp(pwszReferenceTag, pAttr->pwszReferenceTag) == 0)
            {
                goto CommonReturn;
            }
        }
    }

    goto ErrorNotFound;

    CommonReturn:
    ErrorReturn:
        return(pAttr);

    SET_ERROR_VAR_EX(DBG_SS, ErrorInvalidParam, ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, ErrorNotFound,     CRYPT_E_NOT_FOUND);
}

CRYPTCATATTRIBUTE * WINAPI CryptCATPutCatAttrInfo(HANDLE hCatalog, LPWSTR pwszReferenceTag,
                                                  DWORD dwAttrTypeAndAction, DWORD cbData,
                                                  BYTE *pbData)
{
    CRYPTCATATTRIBUTE   *pAttr;

    pAttr = NULL;

    if (!(hCatalog) ||
        (hCatalog == INVALID_HANDLE_VALUE) ||
        !(pwszReferenceTag) ||
        ((cbData > 0) && !(pbData)))
    {
        goto ErrorInvalidParam;
    }

    if (!(dwAttrTypeAndAction & CRYPTCAT_ATTR_DATABASE64) &&
        !(dwAttrTypeAndAction & CRYPTCAT_ATTR_DATAASCII))
    {
        goto ErrorInvalidParam;
    }

    CRYPTCATSTORE   *pCatStore;

    pCatStore = (CRYPTCATSTORE *)hCatalog;

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hAttrs)))
    {
        goto ErrorInvalidParam;
    }

    Stack_              *pStack;

    if (!(pCatStore->hAttrs))
    {
        pStack = new Stack_(&MSCAT_CriticalSection);

        if (!(pStack))
        {
            goto ErrorMemory;
        }

        pCatStore->hAttrs = (HANDLE)pStack;
    }

    pStack = (Stack_ *)pCatStore->hAttrs;

    if (!(pAttr = (CRYPTCATATTRIBUTE *)pStack->Add(sizeof(CRYPTCATATTRIBUTE))))
    {
        goto StackError;
    }

    memset(pAttr, 0x00, sizeof(CRYPTCATATTRIBUTE));

    pAttr->cbStruct = sizeof(CRYPTCATATTRIBUTE);

    if (!(pAttr->pwszReferenceTag = (LPWSTR)CatalogNew((wcslen(pwszReferenceTag) + 1) *
                                                            sizeof(WCHAR))))
    {
        goto ErrorMemory;
    }

    wcscpy(pAttr->pwszReferenceTag, pwszReferenceTag);

    pAttr->dwAttrTypeAndAction = dwAttrTypeAndAction;

    if (pbData)
    {
        if (dwAttrTypeAndAction &  CRYPTCAT_ATTR_DATABASE64)
        {
            Base64DecodeW((WCHAR *)pbData, cbData / sizeof(WCHAR), NULL, &pAttr->cbValue);

            if (pAttr->cbValue < 1)
            {
                goto DecodeError;
            }

            if (!(pAttr->pbValue = (BYTE *)CatalogNew(pAttr->cbValue)))
            {
                pAttr->cbValue = 0;
                goto ErrorMemory;
            }

            if (Base64DecodeW((WCHAR *)pbData, cbData / sizeof(WCHAR), pAttr->pbValue, 
                                                    &pAttr->cbValue) != ERROR_SUCCESS)
            {
                goto DecodeError;
            }
        }
        else if (dwAttrTypeAndAction & CRYPTCAT_ATTR_DATAASCII)
        {
            if (!(pAttr->pbValue = (BYTE *)CatalogNew(cbData)))
            {
                goto ErrorMemory;
            }

            memcpy(pAttr->pbValue, pbData, cbData);
            pAttr->cbValue = cbData;
        }
    }

    // CommonReturn:
    ErrorReturn:
        return(pAttr);

    TRACE_ERROR_EX(DBG_SS, StackError);
    TRACE_ERROR_EX(DBG_SS, DecodeError);

    SET_ERROR_VAR_EX(DBG_SS, ErrorMemory,       ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, ErrorInvalidParam, ERROR_INVALID_PARAMETER);
}

CRYPTCATATTRIBUTE * WINAPI CryptCATEnumerateCatAttr(HANDLE hCatalog, CRYPTCATATTRIBUTE *pPrevAttr)
{
    CRYPTCATATTRIBUTE   *pAttr;

    pAttr = NULL;

    if (!(hCatalog) ||
        (hCatalog == (HANDLE)INVALID_HANDLE_VALUE))
    {
        goto ErrorInvalidParam;
    }

    CRYPTCATSTORE   *pStore;

    pStore = CryptCATStoreFromHandle(hCatalog);

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pStore->cbStruct, hAttrs)))
    {
        goto ErrorInvalidParam;
    }

    DWORD   dwNext;

    dwNext = 0;

    if (pPrevAttr)
    {
        if (!(_ISINSTRUCT(CRYPTCATATTRIBUTE, pPrevAttr->cbStruct, dwReserved)))
        {
            goto ErrorInvalidParam;
        }

        dwNext = pPrevAttr->dwReserved + 1;
    }

    if (pStore->hAttrs)
    {
        Stack_              *ps;

        ps = (Stack_ *)pStore->hAttrs;

        if (pAttr = (CRYPTCATATTRIBUTE *)ps->Get(dwNext))
        {
            //
            //  save our "id" for next time
            //
            pAttr->dwReserved = dwNext;

            //
            //  Done!
            //
            goto CommonReturn;
        }
    }

    goto ErrorNotFound;

    CommonReturn:
    ErrorReturn:
        return(pAttr);

    SET_ERROR_VAR_EX(DBG_SS, ErrorInvalidParam, ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, ErrorNotFound,     CRYPT_E_NOT_FOUND);
}

CRYPTCATMEMBER * WINAPI CryptCATGetMemberInfo(IN HANDLE hCatalog, 
                                              IN LPWSTR pwszReferenceTag)
{
    if (!(hCatalog) ||
        (hCatalog == (HANDLE)INVALID_HANDLE_VALUE) ||
        !(pwszReferenceTag))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    CRYPTCATSTORE   *pCatStore;
    CRYPTCATMEMBER  *pCatMember;

    pCatStore   = (CRYPTCATSTORE *)hCatalog;

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    pCatMember  = NULL;

    if (pCatStore->hReserved)
    {
        CRYPTCATMEMBER  *pCatMember;
        Stack_          *ps;

        ps = (Stack_ *)pCatStore->hReserved;

        if (pCatMember = (CRYPTCATMEMBER *)ps->Get(WVT_OFFSETOF(CRYPTCATMEMBER, pwszReferenceTag), 
                                                   sizeof(WCHAR *), 
                                                   STACK_SORTTYPE_PWSZ,
                                                   pwszReferenceTag))
        {
            if (!(pCatMember->pIndirectData))
            {
                CatalogReallyDecodeIndirectData(pCatStore, pCatMember, &pCatMember->sEncodedIndirectData);
            }

            if ((pCatMember->gSubjectType.Data1 == 0) &&
                (pCatMember->gSubjectType.Data2 == 0) &&
                (pCatMember->gSubjectType.Data3 == 0))
            {
                CatalogReallyDecodeMemberInfo(pCatStore, pCatMember, &pCatMember->sEncodedMemberInfo);
            }

            return(pCatMember);
        }
    }

    return(NULL);
}


CRYPTCATMEMBER * WINAPI CryptCATPutMemberInfo(HANDLE hCatalog,
                                              LPWSTR pwszFileName,
                                              LPWSTR pwszReferenceTag,
                                              GUID *pgSubjectType,
                                              DWORD dwCertVersion,
                                              DWORD cbIndirectData,
                                              BYTE *pbIndirectData)

{
    if (!(hCatalog) ||
        (hCatalog == (HANDLE)INVALID_HANDLE_VALUE) ||
        !(pwszReferenceTag) ||
        !(pgSubjectType) ||
        !(pbIndirectData) ||
        (wcslen(pwszReferenceTag) > CRYPTCAT_MAX_MEMBERTAG))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    CRYPTCATSTORE   *pCatStore;

    pCatStore = (CRYPTCATSTORE *)hCatalog;

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    Stack_          *pStack;
    CRYPTCATMEMBER  *pCatMember;

    if (!(pCatStore->hReserved))
    {
        pStack = new Stack_(&MSCAT_CriticalSection);

        if (!(pStack))
        {
            assert(0);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }

        pCatStore->hReserved = (HANDLE)pStack;
    }

    //
    //  the following is commented out -- too slow!!!!
    //
    // else if (CatalogCheckForDuplicateMember((Stack_ *)pCatStore->hReserved, pwszReferenceTag))
    // {
    //     SetLastError(CRYPT_E_EXISTS);
    //     return(NULL);
    // }

    pStack = (Stack_ *)pCatStore->hReserved;

    if (!(pCatMember = (CRYPTCATMEMBER *)pStack->Add(sizeof(CRYPTCATMEMBER))))
    {
        return(NULL);
    }

    memset(pCatMember, 0x00, sizeof(CRYPTCATMEMBER));

    pCatMember->cbStruct = sizeof(CRYPTCATMEMBER);

    if (pwszFileName)
    {
        if (!(pCatMember->pwszFileName = (LPWSTR)CatalogNew((wcslen(pwszFileName) + 1) *
                                                            sizeof(WCHAR))))
        {
            return(NULL);
        }

        wcscpy(pCatMember->pwszFileName, pwszFileName);
    }

    if (!(pCatMember->pwszReferenceTag = (LPWSTR)CatalogNew((wcslen(pwszReferenceTag) + 1) *
                                                            sizeof(WCHAR))))
    {
        return(NULL);
    }

    wcscpy(pCatMember->pwszReferenceTag, pwszReferenceTag);

    if (cbIndirectData > 0)
    {
        SIP_INDIRECT_DATA   *pInd;

        if (!(pCatMember->pIndirectData = (SIP_INDIRECT_DATA *)CatalogNew(sizeof(SIP_INDIRECT_DATA))))
        {
            return(NULL);
        }

        memset(pCatMember->pIndirectData, 0x00, sizeof(SIP_INDIRECT_DATA));

        pInd = (SIP_INDIRECT_DATA *)pbIndirectData;

        if (pInd->Data.pszObjId)
        {
            if (!(pCatMember->pIndirectData->Data.pszObjId = 
                                    (LPSTR)CatalogNew(strlen(pInd->Data.pszObjId) + 1)))
            {
                return(NULL);
            }
            
            strcpy(pCatMember->pIndirectData->Data.pszObjId, pInd->Data.pszObjId);
        }

        if (pInd->Data.Value.cbData > 0)
        {
            if (!(pCatMember->pIndirectData->Data.Value.pbData = 
                                    (BYTE *)CatalogNew(pInd->Data.Value.cbData)))
            {
                return(NULL);
            }
            
            memcpy(pCatMember->pIndirectData->Data.Value.pbData, 
                        pInd->Data.Value.pbData, pInd->Data.Value.cbData);
        }

        pCatMember->pIndirectData->Data.Value.cbData = pInd->Data.Value.cbData;


        if (!(pCatMember->pIndirectData->DigestAlgorithm.pszObjId = 
                    (LPSTR)CatalogNew(strlen(pInd->DigestAlgorithm.pszObjId) + 1)))
        {
            return(NULL);
        }
        strcpy(pCatMember->pIndirectData->DigestAlgorithm.pszObjId,
                pInd->DigestAlgorithm.pszObjId);


        if (!(pCatMember->pIndirectData->Digest.pbData = 
                    (BYTE *)CatalogNew(pInd->Digest.cbData)))
        {
            return(NULL);
        }
        memcpy(pCatMember->pIndirectData->Digest.pbData,
                    pInd->Digest.pbData, pInd->Digest.cbData);
        pCatMember->pIndirectData->Digest.cbData = pInd->Digest.cbData;
    }

    memcpy(&pCatMember->gSubjectType, pgSubjectType, sizeof(GUID));

    pCatMember->dwCertVersion = dwCertVersion;

    return(pCatMember);
}

BOOL WINAPI CryptCATVerifyMember(HANDLE hCatalog,
                                 CRYPTCATMEMBER *pCatMember,
                                 HANDLE hFileOrMemory)
{
    if (!(hCatalog) ||
        (hCatalog == (HANDLE)INVALID_HANDLE_VALUE) ||
        !(pCatMember) ||
        !(_ISINSTRUCT(CRYPTCATMEMBER, pCatMember->cbStruct, hReserved)) ||
        !(hFileOrMemory) ||
        (hFileOrMemory == INVALID_HANDLE_VALUE))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    CRYPTCATSTORE   *pCatStore;

    pCatStore = (CRYPTCATSTORE *)hCatalog;

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    // TBDTBD!!!

    return(FALSE);
}

CRYPTCATATTRIBUTE * WINAPI CryptCATGetAttrInfo(HANDLE hCatalog,
                                               CRYPTCATMEMBER *pCatMember,
                                               LPWSTR pwszReferenceTag)
{
    if (!(hCatalog) ||
        (hCatalog == (HANDLE)INVALID_HANDLE_VALUE) ||
        !(pCatMember) ||
        !(_ISINSTRUCT(CRYPTCATMEMBER, pCatMember->cbStruct, hReserved)) ||
        !(pwszReferenceTag))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    CRYPTCATSTORE       *pCatStore;
    CRYPTCATATTRIBUTE   *pCatAttr;

    pCatStore   = (CRYPTCATSTORE *)hCatalog;

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    pCatAttr  = NULL;

    while (pCatAttr = CryptCATEnumerateAttr(hCatalog, pCatMember, pCatAttr))
    {
        if (pCatAttr->pwszReferenceTag)
        {
            if (_wcsicmp(pwszReferenceTag, pCatAttr->pwszReferenceTag) == 0)
            {
                return(pCatAttr);
            }
        }
    }

    SetLastError(CRYPT_E_NOT_FOUND);

    return(NULL);
}


CRYPTCATATTRIBUTE * WINAPI CryptCATPutAttrInfo(HANDLE hCatalog,
                                               CRYPTCATMEMBER *pCatMember,
                                               LPWSTR pwszReferenceTag,
                                               DWORD dwAttrTypeAndAction,
                                               DWORD cbData,
                                               BYTE *pbData)
{
    if (!(hCatalog) ||
        (hCatalog == (HANDLE)INVALID_HANDLE_VALUE) ||
        !(pCatMember) ||
        !(_ISINSTRUCT(CRYPTCATMEMBER, pCatMember->cbStruct, hReserved)) ||
        !(pwszReferenceTag) ||
        ((cbData > 0) && !(pbData)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    if (!(dwAttrTypeAndAction & CRYPTCAT_ATTR_DATABASE64) &&
        !(dwAttrTypeAndAction & CRYPTCAT_ATTR_DATAASCII))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    CRYPTCATSTORE   *pCatStore;

    pCatStore = (CRYPTCATSTORE *)hCatalog;

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    Stack_              *pStack;
    CRYPTCATATTRIBUTE   *pAttr;

    if (!(pCatMember->hReserved))
    {
        pStack = new Stack_(&MSCAT_CriticalSection);

        if (!(pStack))
        {
            assert(0);
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return(NULL);
        }

        pCatMember->hReserved = (HANDLE)pStack;
    }

    pStack = (Stack_ *)pCatMember->hReserved;

    if (!(pAttr = (CRYPTCATATTRIBUTE *)pStack->Add(sizeof(CRYPTCATATTRIBUTE))))
    {
        return(NULL);
    }

    memset(pAttr, 0x00, sizeof(CRYPTCATATTRIBUTE));

    pAttr->cbStruct = sizeof(CRYPTCATATTRIBUTE);

    if (!(pAttr->pwszReferenceTag = (LPWSTR)CatalogNew((wcslen(pwszReferenceTag) + 1) *
                                                            sizeof(WCHAR))))
    {
        return(NULL);
    }

    wcscpy(pAttr->pwszReferenceTag, pwszReferenceTag);

    pAttr->dwAttrTypeAndAction = dwAttrTypeAndAction;

    if (pbData)
    {
        if (dwAttrTypeAndAction &  CRYPTCAT_ATTR_DATABASE64)
        {
            Base64DecodeW((WCHAR *)pbData, cbData / sizeof(WCHAR), NULL, &pAttr->cbValue);

            if (pAttr->cbValue < 1)
            {
                return(NULL);
            }

            if (!(pAttr->pbValue = (BYTE *)CatalogNew(pAttr->cbValue)))
            {
                pAttr->cbValue = 0;
                return(NULL);
            }

            memset(pAttr->pbValue, 0x00, pAttr->cbValue);

            if (Base64DecodeW((WCHAR *)pbData, cbData / sizeof(WCHAR), pAttr->pbValue, 
                                                    &pAttr->cbValue) != ERROR_SUCCESS)
            {
                return(NULL);
            }
        }
        else if (dwAttrTypeAndAction & CRYPTCAT_ATTR_DATAASCII)
        {
            if (!(pAttr->pbValue = (BYTE *)CatalogNew(cbData)))
            {
                return(NULL);
            }

            memcpy(pAttr->pbValue, pbData, cbData);
            pAttr->cbValue = cbData;
        }
    }

    return(pAttr);
}


CRYPTCATMEMBER * WINAPI CryptCATEnumerateMember(HANDLE hCatalog,
                                                CRYPTCATMEMBER *pPrevMember)
{
    if (!(hCatalog) ||
        (hCatalog == (HANDLE)INVALID_HANDLE_VALUE))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    CRYPTCATSTORE   *pCatStore;

    pCatStore = (CRYPTCATSTORE *)hCatalog;

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    DWORD   dwNext;

    dwNext = 0;

    if (pPrevMember)
    {
        if (!(_ISINSTRUCT(CRYPTCATMEMBER, pPrevMember->cbStruct, hReserved)))
        {
            SetLastError((DWORD)ERROR_INVALID_PARAMETER);
            return(NULL);
        }

        dwNext = pPrevMember->dwReserved + 1;
    }

    if (pCatStore->hReserved)
    {
        CRYPTCATMEMBER  *pCatMember;
        Stack_          *ps;

        ps = (Stack_ *)pCatStore->hReserved;

        if (pCatMember = (CRYPTCATMEMBER *)ps->Get(dwNext))
        {
            //
            //  save our "id" for next time
            //
            pCatMember->dwReserved = dwNext;


            if (!(pCatMember->pIndirectData))
            {
                CatalogReallyDecodeIndirectData(pCatStore, pCatMember, &pCatMember->sEncodedIndirectData);
            }

            if ((pCatMember->gSubjectType.Data1 == 0) &&
                (pCatMember->gSubjectType.Data2 == 0) &&
                (pCatMember->gSubjectType.Data3 == 0))
            {
                CatalogReallyDecodeMemberInfo(pCatStore, pCatMember, &pCatMember->sEncodedMemberInfo);
            }

            //
            //  Done!
            //
            return(pCatMember);
        }
    }

    return(NULL);
}

CRYPTCATATTRIBUTE * WINAPI CryptCATEnumerateAttr(HANDLE hCatalog,
                                                 CRYPTCATMEMBER *pCatMember,
                                                 CRYPTCATATTRIBUTE *pPrevAttr)
{
    if (!(hCatalog) ||
        (hCatalog == (HANDLE)INVALID_HANDLE_VALUE))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    CRYPTCATSTORE   *pCatStore;

    pCatStore = (CRYPTCATSTORE *)hCatalog;

    if (!(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    DWORD   dwNext;

    dwNext = 0;

    if (pPrevAttr)
    {
        if (!(_ISINSTRUCT(CRYPTCATATTRIBUTE, pPrevAttr->cbStruct, dwReserved)))
        {
            SetLastError((DWORD)ERROR_INVALID_PARAMETER);
            return(NULL);
        }

        dwNext = pPrevAttr->dwReserved + 1;
    }

    if (!(pCatStore) ||
        !(_ISINSTRUCT(CRYPTCATSTORE, pCatStore->cbStruct, hReserved)) ||
        !(pCatMember) ||
        !(_ISINSTRUCT(CRYPTCATMEMBER, pCatMember->cbStruct, hReserved)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(NULL);
    }

    if (pCatMember->hReserved)
    {
        CRYPTCATATTRIBUTE   *pCatAttr;
        Stack_              *ps;

        ps = (Stack_ *)pCatMember->hReserved;

        if (pCatAttr = (CRYPTCATATTRIBUTE *)ps->Get(dwNext))
        {
            //
            //  save our "id" for next time
            //
            pCatAttr->dwReserved = dwNext;

            //
            //  Done!
            //
            return(pCatAttr);
        }
    }

    return(NULL);
}


/////////////////////////////////////////////////////////////////////////////
//
//  Local Functions
//  

BOOL CryptCATCreateStore(CRYPTCATSTORE *pCatStore, LPWSTR pwszCatFile)
{
    HANDLE      hFile;
    DWORD       lErr;

    lErr = GetLastError();

    if ((hFile = CreateFileU(pwszCatFile,
                             GENERIC_WRITE | GENERIC_READ,
                             0,                 // no sharing!!
                             NULL,
                             CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL)) == INVALID_HANDLE_VALUE)
    {
        return(FALSE);
    }

    CloseHandle(hFile);

    SetLastError(lErr);

    return(CryptCATOpenStore(pCatStore, pwszCatFile));
}
                                            
BOOL CryptCATOpenStore(CRYPTCATSTORE *pCatStore, LPWSTR pwszCatFile)
{
    if (!(pCatStore->pwszP7File = (LPWSTR)CatalogNew((wcslen(pwszCatFile) + 1) *
                                                sizeof(WCHAR))))
    {
        return(FALSE);
    }

    wcscpy(pCatStore->pwszP7File, pwszCatFile);

    return(CatalogLoadFileData(pCatStore));
}
