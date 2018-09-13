//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       IndData.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//              implements the Certificate Trust List & persistent storage
//
//  Functions:  CatalogDecodeIndirectData
//              CatalogReallyDecodeIndirectData
//              CatalogEncodeIndirectData
//
//  History:    16-May-1997 pberkman    created
//              01-Oct-1997 pberkman    add lazy decode
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "mscat32.h"

BOOL CatalogDecodeIndirectData(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember, CRYPT_ATTRIBUTE *pAttr)
{
    DELETE_OBJECT(pMember->sEncodedIndirectData.pbData);

    if (pAttr->rgValue->cbData < 1)
    {
        return(FALSE);
    }

    if (!(pMember->sEncodedIndirectData.pbData = (BYTE *)CatalogNew(pAttr->rgValue->cbData)))
    {
        pMember->sEncodedIndirectData.cbData = 0;

        return(FALSE);
    }

    pMember->sEncodedIndirectData.cbData = pAttr->rgValue->cbData;
    memcpy(pMember->sEncodedIndirectData.pbData, pAttr->rgValue->pbData, pAttr->rgValue->cbData);

    return(TRUE);
}

BOOL CatalogReallyDecodeIndirectData(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember, CRYPT_ATTR_BLOB *pAttr)
{
    DWORD               cbDecode;
    SIP_INDIRECT_DATA   *pInd;

    cbDecode = 0;

    CryptDecodeObject(pCat->dwEncodingType,
                      SPC_INDIRECT_DATA_CONTENT_STRUCT,
                      pAttr->pbData,
                      pAttr->cbData,
                      0,
                      NULL,
                      &cbDecode);

    if (cbDecode > 0)
    {
        if (!(pInd = (SIP_INDIRECT_DATA *)CatalogNew(cbDecode)))
        {
            return(FALSE);
        }

        if (!(CryptDecodeObject(pCat->dwEncodingType,
                                SPC_INDIRECT_DATA_CONTENT_STRUCT,
                                pAttr->pbData,
                                pAttr->cbData,
                                0,
                                pInd,
                                &cbDecode)))
        {
            delete pInd;

            return(FALSE);
        }

        if (!(pMember->pIndirectData = (SIP_INDIRECT_DATA *)CatalogNew(sizeof(SIP_INDIRECT_DATA))))
        {
            delete pInd;

            return(FALSE);
        }

        memset(pMember->pIndirectData, 0x00, sizeof(SIP_INDIRECT_DATA));

        pMember->pIndirectData->Data.pszObjId = (LPSTR)CatalogNew(strlen(pInd->Data.pszObjId) + 1);

        if (pInd->Data.Value.cbData > 0)
        {
            pMember->pIndirectData->Data.Value.pbData =
                                                (BYTE *)CatalogNew(pInd->Data.Value.cbData);
            pMember->pIndirectData->Data.Value.cbData = pInd->Data.Value.cbData;
        }

        pMember->pIndirectData->DigestAlgorithm.pszObjId =
                                                (LPSTR)CatalogNew(strlen(pInd->DigestAlgorithm.pszObjId) + 1);

        if (pInd->Digest.cbData > 0)
        {
            pMember->pIndirectData->Digest.pbData = (BYTE *)CatalogNew(pInd->Digest.cbData);
            pMember->pIndirectData->Digest.cbData = pInd->Digest.cbData;
        }

        if (!(pMember->pIndirectData->Data.pszObjId) ||
            ((pInd->Data.Value.cbData > 0) && !(pMember->pIndirectData->Data.Value.pbData)) ||
            !(pMember->pIndirectData->DigestAlgorithm.pszObjId) ||
            ((pInd->Digest.cbData > 0) && !(pMember->pIndirectData->Digest.pbData)))
        {
            delete pInd;
            return(FALSE);
        }

        strcpy(pMember->pIndirectData->Data.pszObjId, pInd->Data.pszObjId);

        if (pInd->Data.Value.cbData > 0)
        {
            memcpy(pMember->pIndirectData->Data.Value.pbData,
                    pInd->Data.Value.pbData, pInd->Data.Value.cbData);
        }

        strcpy(pMember->pIndirectData->DigestAlgorithm.pszObjId,
                pInd->DigestAlgorithm.pszObjId);

        if (pInd->Digest.cbData > 0)
        {
            memcpy(pMember->pIndirectData->Digest.pbData,
                    pInd->Digest.pbData, pInd->Digest.cbData);
        }

        delete pInd;

        return(TRUE);
    }

    return(FALSE);
}


BOOL CatalogEncodeIndirectData(CRYPTCATSTORE *pCat, CRYPTCATMEMBER *pMember, 
                               PCRYPT_ATTRIBUTE pCryptAttr)
{
    if (!(pCryptAttr->rgValue = (PCRYPT_ATTR_BLOB)CatalogNew(sizeof(CRYPT_ATTR_BLOB))))
    {
        return(FALSE);
    }

    pCryptAttr->cValue = 1;

    memset(pCryptAttr->rgValue, 0x00, sizeof(CRYPT_ATTR_BLOB));

    if (!(pMember->pIndirectData))
    {
        return(FALSE);
    }

    pCryptAttr->pszObjId = SPC_INDIRECT_DATA_OBJID; 

    DWORD   cbEncoded;

    cbEncoded = 0;

    CryptEncodeObject(pCat->dwEncodingType,
                      pCryptAttr->pszObjId,
                      pMember->pIndirectData,
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
                                pMember->pIndirectData,
                                pCryptAttr->rgValue->pbData,
                                &cbEncoded)))
        {
            return(FALSE);
        }

        return(TRUE);
    }

    return(FALSE);
}

