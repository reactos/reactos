//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       NameVal.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//              implements the Certificate Trust List & persistent storage
//
//  Functions:  CatalogEncodeNameValue
//              CatalogDecodeNameValue
//              CatalogCertExt2CryptAttr
//              CatalogCryptAttr2CertExt
//
//              *** local functions ***
//              EncodeUserOID
//              DecodeUserOID
//
//  History:    16-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "mscat32.h"


BOOL EncodeUserOID(CRYPTCATSTORE *pCatStore, CAT_NAMEVALUE *pNameValue);
BOOL DecodeUserOID(CRYPTCATSTORE *pCatStore, CAT_NAMEVALUE *pNV, BYTE **ppbUserOIDDecode, 
                   DWORD *pcbUserOIDDecode);

void CatalogCertExt2CryptAttr(CERT_EXTENSION *pCertExt, CRYPT_ATTRIBUTE *pCryptAttr)
{
    memset(pCryptAttr, 0x00, sizeof(CRYPT_ATTRIBUTE));

    pCryptAttr->pszObjId    = pCertExt->pszObjId;
    pCryptAttr->cValue      = 1;
    pCryptAttr->rgValue     = &pCertExt->Value;
}

void CatalogCryptAttr2CertExt(CRYPT_ATTRIBUTE *pCryptAttr, CERT_EXTENSION *pCertExt)
{
    memset(pCertExt, 0x00, sizeof(CERT_EXTENSION));

    pCertExt->pszObjId      = pCryptAttr->pszObjId;
    pCertExt->fCritical     = FALSE;

    if ((pCryptAttr->cValue) && (pCryptAttr->rgValue))
    {
        memcpy(&pCertExt->Value, &pCryptAttr->rgValue[0], sizeof(CRYPT_ATTR_BLOB));
    }
}

BOOL CatalogEncodeNameValue(CRYPTCATSTORE *pCatStore, CRYPTCATATTRIBUTE *pAttr, 
                            PCRYPT_ATTRIBUTE pCryptAttr)
{
    CAT_NAMEVALUE   sNV;

    memset(&sNV, 0x00, sizeof(CAT_NAMEVALUE));

    sNV.pwszTag         = pAttr->pwszReferenceTag;
    sNV.fdwFlags        = pAttr->dwAttrTypeAndAction;
    sNV.Value.cbData    = pAttr->cbValue;

    if (!(sNV.Value.pbData = (BYTE *)CatalogNew(sNV.Value.cbData)))
    {
        return(FALSE);
    }

    memcpy(sNV.Value.pbData, pAttr->pbValue, sNV.Value.cbData);

    if (pAttr->dwAttrTypeAndAction & CRYPTCAT_ATTR_NAMEOBJID)
    {
        if (!(EncodeUserOID(pCatStore, &sNV)))
        {
            delete sNV.Value.pbData;

            return(FALSE);
        }
    }

    pCryptAttr->pszObjId = CAT_NAMEVALUE_OBJID;

    pCryptAttr->rgValue->cbData = 0;

    CryptEncodeObject(pCatStore->dwEncodingType,
                      pCryptAttr->pszObjId,
                      &sNV,
                      NULL,
                      &pCryptAttr->rgValue->cbData);

    if (pCryptAttr->rgValue->cbData > 0)
    {
        if (!(pCryptAttr->rgValue->pbData = (BYTE *)CatalogNew(pCryptAttr->rgValue->cbData)))
        {
            delete sNV.Value.pbData;

            return(FALSE);
        }

        if (!(CryptEncodeObject(pCatStore->dwEncodingType,
                                pCryptAttr->pszObjId,
                                &sNV,
                                pCryptAttr->rgValue->pbData,
                                &pCryptAttr->rgValue->cbData)))
        {
            delete sNV.Value.pbData;

            DELETE_OBJECT(pCryptAttr->rgValue->pbData);

            pCryptAttr->rgValue->cbData = 0;

            return(FALSE);
        }

        delete sNV.Value.pbData;

        return(TRUE);
    }

    delete sNV.Value.pbData;

    return(FALSE);
}

BOOL CatalogDecodeNameValue(CRYPTCATSTORE *pCatStore, PCRYPT_ATTRIBUTE pCryptAttr,
                            CRYPTCATATTRIBUTE *pCatAttr)
{
    CAT_NAMEVALUE   *pNV;
    DWORD           cbDecoded;

    cbDecoded   = 0;


    CryptDecodeObject(  pCatStore->dwEncodingType,
                        CAT_NAMEVALUE_STRUCT,
                        pCryptAttr->rgValue->pbData,
                        pCryptAttr->rgValue->cbData,
                        0,
                        NULL,
                        &cbDecoded);

    if (cbDecoded > 0)
    {
        if (!(pNV = (CAT_NAMEVALUE *)CatalogNew(cbDecoded)))
        {
            return(FALSE);
        }

        if (!(CryptDecodeObject(    pCatStore->dwEncodingType,
                                    CAT_NAMEVALUE_STRUCT,
                                    pCryptAttr->rgValue->pbData,
                                    pCryptAttr->rgValue->cbData,
                                    0,
                                    pNV,
                                    &cbDecoded)))
        {
            delete pNV;

            return(FALSE);
        }

        if (!(pCatAttr->pwszReferenceTag = (LPWSTR)CatalogNew((wcslen(pNV->pwszTag) + 1) * sizeof(WCHAR))))
        {
            delete pNV;

            return(FALSE);
        }
        wcscpy(pCatAttr->pwszReferenceTag, pNV->pwszTag);

        pCatAttr->dwAttrTypeAndAction = pNV->fdwFlags;

        if (pCatAttr->dwAttrTypeAndAction & CRYPTCAT_ATTR_NAMEOBJID)
        {
            DWORD   cbUserOIDDecode;
            BYTE    *pbUserOIDDecode;

            if (!(DecodeUserOID(pCatStore, pNV, &pbUserOIDDecode, &cbUserOIDDecode)))
            {
                delete pNV;

                return(FALSE);
            }

            delete pNV;

            pCatAttr->pbValue = pbUserOIDDecode;
            pCatAttr->cbValue = cbUserOIDDecode;

            return(TRUE);
        }

        if (!(pCatAttr->pbValue = (BYTE *)CatalogNew(pNV->Value.cbData)))
        {
            delete pNV;

            return(FALSE);
        }

        memcpy(pCatAttr->pbValue, pNV->Value.pbData, pNV->Value.cbData);
        pCatAttr->cbValue = pNV->Value.cbData;

        delete pNV;

        return(TRUE);
    }

    return(FALSE);
}

BOOL EncodeUserOID(CRYPTCATSTORE *pCatStore, CAT_NAMEVALUE *pNameValue)
{
    DWORD   cbEncoded;
    BYTE    *pbEncoded;
    DWORD   cbConv;
    LPSTR   pszObjId;

    pbEncoded           = NULL;

    cbConv = WideCharToMultiByte(0, 0,
                                pNameValue->pwszTag, wcslen(pNameValue->pwszTag) + 1,
                                NULL, 0, NULL, NULL);
    if (cbConv < 1)
    {
        return(FALSE);
    }

    if (!(pszObjId = (LPSTR)CatalogNew(cbConv + 1)))
    {
        return(FALSE);
    }

    WideCharToMultiByte(0, 0,
                        pNameValue->pwszTag, wcslen(pNameValue->pwszTag) + 1,
                        pszObjId, cbConv, NULL, NULL);

    pszObjId[cbConv] = NULL;

    cbEncoded = 0;

    CryptEncodeObject(pCatStore->dwEncodingType,
                      pszObjId,
                      pNameValue->Value.pbData,
                      NULL,
                      &cbEncoded);

    if (cbEncoded > 0)
    {
        if (!(pbEncoded = (BYTE *)CatalogNew(cbEncoded)))
        {
            delete pszObjId;

            return(FALSE);
        }

        if (!(CryptEncodeObject(pCatStore->dwEncodingType,
                                pszObjId,
                                pNameValue->Value.pbData,
                                pbEncoded,
                                &cbEncoded)))
        {
            delete pszObjId;

            delete pbEncoded;

            return(FALSE);
        }
    }

    delete pszObjId;

    DELETE_OBJECT(pNameValue->Value.pbData);

    pNameValue->Value.pbData    = pbEncoded;
    pNameValue->Value.cbData    = cbEncoded;

    return(TRUE);
    
}

BOOL DecodeUserOID(CRYPTCATSTORE *pCatStore, CAT_NAMEVALUE *pNV, BYTE **ppbUserOIDDecode, 
                   DWORD *pcbUserOIDDecode)
{
    *ppbUserOIDDecode   = NULL;
    *pcbUserOIDDecode   = 0;

    DWORD   cbConv;
    LPSTR   pszObjId;

    cbConv = WideCharToMultiByte(0, 0,
                                pNV->pwszTag, wcslen(pNV->pwszTag) + 1,
                                NULL, 0, NULL, NULL);
    if (cbConv < 1)
    {
        return(FALSE);
    }

    if (!(pszObjId = (LPSTR)CatalogNew(cbConv + 1)))
    {
        return(FALSE);
    }

    WideCharToMultiByte(0, 0,
                        pNV->pwszTag, wcslen(pNV->pwszTag) + 1,
                        pszObjId, cbConv, NULL, NULL);

    pszObjId[cbConv] = NULL;

    CryptDecodeObject(pCatStore->dwEncodingType,
                      pszObjId,
                      pNV->Value.pbData,
                      pNV->Value.cbData,
                      0,
                      NULL,
                      pcbUserOIDDecode);

    if (*pcbUserOIDDecode > 0)
    {
        if (!(*ppbUserOIDDecode = (BYTE *)CatalogNew(*pcbUserOIDDecode)))
        {
            delete pszObjId;

            return(FALSE);
        }

        if (!(CryptDecodeObject(pCatStore->dwEncodingType,
                                pszObjId,
                                pNV->Value.pbData,
                                pNV->Value.cbData,
                                0,
                                *ppbUserOIDDecode,
                                pcbUserOIDDecode)))
        {
            delete pszObjId;

            DELETE_OBJECT(*ppbUserOIDDecode);
            *pcbUserOIDDecode = 0;

            return(FALSE);
        }

        return(TRUE);
    }

    delete pszObjId;

    return(FALSE);
    
}
