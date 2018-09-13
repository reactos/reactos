//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       meminfo.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  CatalogDecodeMemberInfo
//              CatalogReallyDecodeMemberInfo
//              CatalogEncodeMemberInfo
//
//  History:    16-May-1997 pberkman    created
//              01-Oct-1997 pberkman    add lazy decode
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "mscat32.h"

BOOL CatalogDecodeMemberInfo(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember, CRYPT_ATTRIBUTE *pAttr)
{
    DELETE_OBJECT(pMember->sEncodedMemberInfo.pbData);

    if (pAttr->rgValue->cbData < 1)
    {
        return(FALSE);
    }

    if (!(pMember->sEncodedMemberInfo.pbData = (BYTE *)CatalogNew(pAttr->rgValue->cbData)))
    {
        pMember->sEncodedMemberInfo.cbData = 0;

        return(FALSE);
    }

    pMember->sEncodedMemberInfo.cbData = pAttr->rgValue->cbData;
    memcpy(pMember->sEncodedMemberInfo.pbData, pAttr->rgValue->pbData, pAttr->rgValue->cbData);

    return(TRUE);
}

BOOL CatalogReallyDecodeMemberInfo(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember, CRYPT_ATTR_BLOB *pAttr)
{
    DWORD           cbDecode;
    CAT_MEMBERINFO  *pCatMemberInfo;

    cbDecode = 0;

    CryptDecodeObject(pCat->dwEncodingType,
                      CAT_MEMBERINFO_STRUCT,
                      pAttr->pbData,
                      pAttr->cbData,
                      0,
                      NULL,
                      &cbDecode);

    if (cbDecode > 0)
    {
        if (!(pCatMemberInfo = (CAT_MEMBERINFO *)CatalogNew(cbDecode)))
        {
            return(FALSE);
        }

        if (!(CryptDecodeObject(pCat->dwEncodingType,
                                CAT_MEMBERINFO_STRUCT,
                                pAttr->pbData,
                                pAttr->cbData,
                                0,
                                pCatMemberInfo,
                                &cbDecode)))
        {
            delete pCatMemberInfo;

            return(FALSE);
        }

        if (pCatMemberInfo->pwszSubjGuid)
        {
            if (!(wstr2guid(pCatMemberInfo->pwszSubjGuid, &pMember->gSubjectType)))
            {
                delete pCatMemberInfo;
            
                return(FALSE);
            }

            pMember->dwCertVersion  = pCatMemberInfo->dwCertVersion;

            delete pCatMemberInfo;

            return(TRUE);
        }

        delete pCatMemberInfo;
    }

    return(FALSE);
}


BOOL CatalogEncodeMemberInfo(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember, 
                              PCRYPT_ATTRIBUTE pCryptAttr)
{
    if (!(pCryptAttr->rgValue = (PCRYPT_ATTR_BLOB)CatalogNew(sizeof(CRYPT_ATTR_BLOB))))
    {
        return(FALSE);
    }

    pCryptAttr->cValue = 1;

    memset(pCryptAttr->rgValue, 0x00, sizeof(CRYPT_ATTR_BLOB));

    pCryptAttr->pszObjId = CAT_MEMBERINFO_OBJID;

    DWORD           cbEncoded;
    CAT_MEMBERINFO  sCatMemberInfo;
    WCHAR           wszGuid[41];

    sCatMemberInfo.pwszSubjGuid = &wszGuid[0];

    if (!(guid2wstr(&pMember->gSubjectType, sCatMemberInfo.pwszSubjGuid)))
    {
        assert(0);
        
        DELETE_OBJECT(pCryptAttr->rgValue);
        return(FALSE);
    }

    sCatMemberInfo.dwCertVersion = pMember->dwCertVersion;

    cbEncoded = 0;

    CryptEncodeObject(pCat->dwEncodingType,
                      pCryptAttr->pszObjId,
                      &sCatMemberInfo,
                      NULL,
                      &cbEncoded);

    if (cbEncoded > 0)
    {
        if (!(pCryptAttr->rgValue->pbData = (BYTE *)CatalogNew(cbEncoded)))
        {
            return(FALSE);
        }

        pCryptAttr->rgValue->cbData = cbEncoded;

        if (!(CryptEncodeObject(pCat->dwEncodingType,
                                pCryptAttr->pszObjId,
                                &sCatMemberInfo,
                                pCryptAttr->rgValue->pbData,
                                &cbEncoded)))
        {
            return(FALSE);
        }

        return(TRUE);
    }

    return(FALSE);
}


