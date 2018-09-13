//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       mssip32.cpp
//
//  Contents:   Microsoft SIP Provider
//
//  History:    14-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "sipobj.hxx"
#include    "sipobjpe.hxx"
#include    "sipobjjv.hxx"
#include    "sipobjcb.hxx"
#include    "sipobjfl.hxx"
#include    "sipobjct.hxx"
#include    "sipobjss.hxx"

#include    "crypthlp.h"
#include    "sha.h"
#include    "md5.h"

#define     MY_NAME     L"WINTRUST.DLL"

SIPObject_ *mssip_CreateSubjectObject(const GUID *chk);

//
//  the entries in SubjectsGuid MUST be in the same
//  relative position and coalate with those in the
//  SubjectsID.
//
static const GUID SubjectsGuid[] =
                    {
                        CRYPT_SUBJTYPE_PE_IMAGE,
                        CRYPT_SUBJTYPE_JAVACLASS_IMAGE,
                        CRYPT_SUBJTYPE_CABINET_IMAGE,
                        CRYPT_SUBJTYPE_FLAT_IMAGE,
                        CRYPT_SUBJTYPE_CATALOG_IMAGE,
                        CRYPT_SUBJTYPE_CTL_IMAGE
                    };

//                        CRYPT_SUBJTYPE_SS_IMAGE

static const UINT SubjectsID[] = 
                    {
                        MSSIP_ID_PE,
                        MSSIP_ID_JAVA,
                        MSSIP_ID_CAB,
                        MSSIP_ID_FLAT,
                        MSSIP_ID_CATALOG,
                        MSSIP_ID_CTL,
                        MSSIP_ID_NONE     // MUST be at the end!
                    };
                
//                         MSSIP_ID_SS,


BOOL WINAPI CryptSIPGetSignedDataMsg(  IN      SIP_SUBJECTINFO *pSubjectInfo,
                                OUT     DWORD           *dwEncodingType,
                                IN      DWORD           dwIndex,
                                IN OUT  DWORD           *pdwDataLen,
                                OUT     BYTE            *pbData)
{
    DWORD       dwLastError=0;

    if (!(pSubjectInfo) || !(pdwDataLen) || !(dwEncodingType))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    SIPObject_          *pSubjectObj;

    if (!(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pSubjectObj = mssip_CreateSubjectObject(pSubjectInfo->pgSubjectType);

    if (!(pSubjectObj))
    {
        if (!(pbData))
        {
            *pdwDataLen = 0;
        }
        return(FALSE);
    }

    pSubjectObj->set_CertVersion(pSubjectInfo->dwIntVersion);

    BOOL    bRet;

    bRet = pSubjectObj->GetSignedDataMsg(pSubjectInfo,
                                    dwIndex,pdwDataLen,pbData,dwEncodingType);

    dwLastError=GetLastError();

    delete pSubjectObj;

    SetLastError(dwLastError);

    return(bRet);
}

BOOL WINAPI CryptSIPPutSignedDataMsg(  IN      SIP_SUBJECTINFO *pSubjectInfo,
                                IN      DWORD           dwEncodingType,
                                OUT     DWORD           *pdwIndex,
                                IN      DWORD           dwDataLen,
                                IN      BYTE            *pbData)
{
    if (!(pSubjectInfo) ||
        (dwDataLen < 1) ||
        !(pdwIndex)     ||
        !(pbData))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    SIPObject_          *pSubjectObj;

    if (!(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pSubjectObj = mssip_CreateSubjectObject(pSubjectInfo->pgSubjectType);

    if (!(pSubjectObj))
    {
        return(FALSE);
    }

    pSubjectObj->set_CertVersion(pSubjectInfo->dwIntVersion);

    BOOL    bRet;

    bRet = pSubjectObj->PutSignedDataMsg(pSubjectInfo,
                                pdwIndex,dwDataLen,pbData,dwEncodingType);

    delete pSubjectObj;

    return(bRet);
}

BOOL WINAPI CryptSIPRemoveSignedDataMsg(   IN SIP_SUBJECTINFO  *pSubjectInfo,
                                    IN DWORD            dwIndex)
{
    if (!(pSubjectInfo))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    SIPObject_          *pSubjectObj;

    if (!(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pSubjectObj = mssip_CreateSubjectObject(pSubjectInfo->pgSubjectType);

    if (!(pSubjectObj))
    {
        return(FALSE);
    }
    
    pSubjectObj->set_CertVersion(pSubjectInfo->dwIntVersion);

    BOOL    bRet;

    bRet = pSubjectObj->RemoveSignedDataMsg(pSubjectInfo,dwIndex);

    delete pSubjectObj;

    return(bRet);
}


BOOL WINAPI CryptSIPCreateIndirectData( IN      SIP_SUBJECTINFO     *pSubjectInfo,
                                 IN OUT  DWORD               *pdwDataLen,
                                 OUT     SIP_INDIRECT_DATA   *psData)
{
    if (!(pSubjectInfo) || !(pdwDataLen))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    SIPObject_          *pSubjectObj;

    if (!(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pSubjectObj = mssip_CreateSubjectObject(pSubjectInfo->pgSubjectType);

    if (!(pSubjectObj))
    {
        if (!(psData))
        {
            *pdwDataLen = 0;
        }
        return(FALSE);
    }

    //
    //  ALWAYS set the latest version when we are creating the
    //  indirect data content!
    //
    pSubjectInfo->dwIntVersion = WIN_CERT_REVISION_2_0;
    pSubjectObj->set_CertVersion(pSubjectInfo->dwIntVersion);

    BOOL    bRet;

    bRet = pSubjectObj->CreateIndirectData(pSubjectInfo,pdwDataLen,psData);

    delete pSubjectObj;

    return(bRet);
}

BOOL WINAPI CryptSIPVerifyIndirectData(    IN SIP_SUBJECTINFO      *pSubjectInfo,
                                    IN SIP_INDIRECT_DATA    *psData)
{
    if (!(pSubjectInfo))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    SIPObject_          *pSubjectObj;

    if (!(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwEncodingType)))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    pSubjectObj = mssip_CreateSubjectObject(pSubjectInfo->pgSubjectType);

    if (!(pSubjectObj))
    {
        return(FALSE);
    }

    //
    //  if we are a catalog member, set the version number to whatever
    //  was set when the catalog file was created...
    //
    if ((WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSubjectInfo->cbSize, dwUnionChoice)) &&
        (pSubjectInfo->dwUnionChoice == MSSIP_ADDINFO_CATMEMBER) &&
        (pSubjectInfo->psCatMember))
    {
        if (pSubjectInfo->psCatMember->cbStruct == sizeof(MS_ADDINFO_CATALOGMEMBER))
        {
            if ((pSubjectInfo->psCatMember->pMember) &&
                (pSubjectInfo->psCatMember->pMember->cbStruct == sizeof(CRYPTCATMEMBER)))
            {
                pSubjectInfo->dwIntVersion = pSubjectInfo->psCatMember->pMember->dwCertVersion;
            }
        }
    }

    pSubjectObj->set_CertVersion(pSubjectInfo->dwIntVersion);

    if (pSubjectObj->get_CertVersion() < WIN_CERT_REVISION_2_0)
    {
        DWORD   dwCAPIFlags;

        CryptSIPGetRegWorkingFlags(&dwCAPIFlags);

        if (dwCAPIFlags & WTPF_VERIFY_V1_OFF)
        {
            delete pSubjectObj;

            SetLastError((DWORD)CRYPT_E_SECURITY_SETTINGS);

            return(FALSE);
        }
    }

    BOOL    bRet;

    bRet = pSubjectObj->VerifyIndirectData(pSubjectInfo, psData);

    delete pSubjectObj;

    return(bRet);
}

//////////////////////////////////////////////////////////////////////////////////////
//
// internal utility functions
//------------------------------------------------------------------------------------
//

SIPObject_ *mssip_CreateSubjectObject(const GUID *chk)
{
    UINT        idx;
    SIPObject_  *pSO;

    pSO = NULL;
    idx = 0;

    while (SubjectsID[idx] != MSSIP_ID_NONE)
    {
        if (SubjectsGuid[idx] == *chk)
        {
            switch (SubjectsID[idx])
            {
                case MSSIP_ID_PE:
                    pSO = (SIPObject_ *)new SIPObjectPE_(SubjectsID[idx]);
                    break;

                case MSSIP_ID_JAVA:
                    pSO = (SIPObject_ *)new SIPObjectJAVA_(SubjectsID[idx]);
                    break;

                case MSSIP_ID_CAB:
                    pSO = (SIPObject_ *)new SIPObjectCAB_(SubjectsID[idx]);
                    break;

                case MSSIP_ID_FLAT:
                    pSO = (SIPObject_ *)new SIPObjectFlat_(SubjectsID[idx]);
                    break;

                case MSSIP_ID_CTL:  // currently, the same logic as catalog files!
                case MSSIP_ID_CATALOG:
                    pSO = (SIPObject_ *)new SIPObjectCatalog_(SubjectsID[idx]);
                    break;

 /*               case MSSIP_ID_SS:
                    pSO = (SIPObject_ *)new SIPObjectSS_(SubjectsID[idx]);
                    break;  */

                case MSSIP_V1ID_PE:
                case MSSIP_V1ID_PE_EX:
                default:
                    SetLastError((DWORD)TRUST_E_SUBJECT_FORM_UNKNOWN);
                    return(NULL);
            }

            if (!(pSO))
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return(NULL);
            }

            break;
        }

        idx++;
    }

    if (!(pSO))
    {
        SetLastError((DWORD)TRUST_E_SUBJECT_FORM_UNKNOWN);
    }

    return(pSO);
}


BOOL WINAPI DigestFileData(  IN HSPCDIGESTDATA hDigestData,
                             IN const BYTE *pbData,
                             IN DWORD cbData)
{
    BOOL            fRet;
    PDIGEST_DATA    pDigestData;

    fRet        = TRUE;
    pDigestData = (PDIGEST_DATA)hDigestData;

    if (cbData > HASH_CACHE_LEN) 
    {
        if (pDigestData->cbCache > 0) 
        {
            fRet = SipHashData(pDigestData, pDigestData->pbCache, pDigestData->cbCache);

            pDigestData->cbCache = 0;

            if (!(fRet))
            {
                return(FALSE);
            }
        }

        fRet = SipHashData(pDigestData, (BYTE *)pbData, cbData);
    } 
    else 
    {
        while (cbData > 0) 
        {
            DWORD cbCopy;

            cbCopy = min(HASH_CACHE_LEN - pDigestData->cbCache, cbData);

            memcpy(&pDigestData->pbCache[pDigestData->cbCache], pbData,
                    cbCopy);

            cbData -= cbCopy;
            pbData += cbCopy;

            pDigestData->cbCache += cbCopy;

            if (pDigestData->cbCache == HASH_CACHE_LEN) 
            {
                pDigestData->cbCache = 0;

                if (!(fRet = SipHashData(pDigestData, pDigestData->pbCache, HASH_CACHE_LEN)))
                {
                    break;
                }
            }
        }
    }

    return(fRet);
}


BOOL SipCreateHash(HCRYPTPROV hProv, DIGEST_DATA *psDigestData)
{
    BOOL    fRet;

    fRet = TRUE;

    switch (psDigestData->dwAlgId)
    {
        case CALG_MD5:
            MD5Init((MD5_CTX *)psDigestData->pvSHA1orMD5Ctx);
            break;

        case CALG_SHA1:
            A_SHAInit((A_SHA_CTX *)psDigestData->pvSHA1orMD5Ctx);
            break;

        default:
            if (!(hProv))
            {
                hProv = I_CryptGetDefaultCryptProv(0);  // get the default and DONT RELEASE IT!!!!
            }

            fRet = CryptCreateHash(hProv, psDigestData->dwAlgId, NULL, 0, &psDigestData->hHash);
            break;
    }

    return(fRet);
}

BOOL SipHashData(DIGEST_DATA *psDigestData, BYTE *pbData, DWORD cbData)
{
    switch (psDigestData->dwAlgId)
    {
        case CALG_MD5:
            MD5Update((MD5_CTX *)psDigestData->pvSHA1orMD5Ctx, pbData, cbData);
            return(TRUE);

        case CALG_SHA1:
            A_SHAUpdate((A_SHA_CTX *)psDigestData->pvSHA1orMD5Ctx, pbData, cbData);
            return(TRUE);
    }

    return(CryptHashData(psDigestData->hHash, pbData, cbData, 0));
}

BYTE *SipGetHashValue(DIGEST_DATA *psDigestData, DWORD *pcbHash)
{
    BYTE    *pbRet;

    pbRet = NULL;

    switch (psDigestData->dwAlgId)
    {
        case CALG_MD5:
            *pcbHash = MD5DIGESTLEN;
            break;

        case CALG_SHA1:
            *pcbHash = A_SHA_DIGEST_LEN;
            break;

        default:
            *pcbHash = 0;
            CryptGetHashParam(psDigestData->hHash, HP_HASHVAL, NULL, pcbHash,0);
    }

    if (*pcbHash < 1)
    {
        goto HashLengthError;
    }

    if (!(pbRet = new BYTE[*pcbHash]))
    {
        goto MemoryError;
    }

    switch (psDigestData->dwAlgId)
    {
        case CALG_MD5:
            MD5_CTX *pMD5;

            pMD5 = (MD5_CTX *)psDigestData->pvSHA1orMD5Ctx;

            MD5Final(pMD5);

            memcpy(pbRet, pMD5->digest, MD5DIGESTLEN);
            psDigestData->pvSHA1orMD5Ctx = NULL;
            break;

        case CALG_SHA1:
            A_SHAFinal((A_SHA_CTX *)psDigestData->pvSHA1orMD5Ctx, pbRet);
            psDigestData->pvSHA1orMD5Ctx = NULL;
            break;

        default:
            if (CryptGetHashParam(psDigestData->hHash, HP_HASHVAL, pbRet, pcbHash, 0))
            {
                goto HashParamError;
            }
            break;
    }


    CommonReturn:
        return(pbRet);

    ErrorReturn:
        DELETE_OBJECT(pbRet);
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, HashLengthError,   GetLastError());
    SET_ERROR_VAR_EX(DBG_SS, HashParamError,    GetLastError());
}

void SipDestroyHash(DIGEST_DATA *psDigestData)
{
    switch (psDigestData->dwAlgId)
    {
        case CALG_MD5:
            if (psDigestData->pvSHA1orMD5Ctx)
            {
                MD5Final((MD5_CTX *)psDigestData->pvSHA1orMD5Ctx);
            }
            break;

        case CALG_SHA1:
            if (psDigestData->pvSHA1orMD5Ctx)
            {
                BYTE    bRet[A_SHA_DIGEST_LEN];

                A_SHAFinal((A_SHA_CTX *)psDigestData->pvSHA1orMD5Ctx, &bRet[0]);
            }
            break;

        default:
            CryptDestroyHash(psDigestData->hHash);
            break;
    }
}

//////////////////////////////////////////////////////////////////////////////////////
//
// standard DLL exports ...
//------------------------------------------------------------------------------------
//

BOOL WINAPI mssip32DllMain(HANDLE hInstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    return(TRUE);
}


STDAPI mssip32DllRegisterServer(void)
{
    BOOL                fRet;
    GUID                gFlat   = CRYPT_SUBJTYPE_FLAT_IMAGE;
    GUID                gPe     = CRYPT_SUBJTYPE_PE_IMAGE;
    GUID                gCb     = CRYPT_SUBJTYPE_CABINET_IMAGE;
    GUID                gJv     = CRYPT_SUBJTYPE_JAVACLASS_IMAGE;
    GUID                gCat    = CRYPT_SUBJTYPE_CATALOG_IMAGE;
    GUID                gCTL    = CRYPT_SUBJTYPE_CTL_IMAGE;
    GUID                gSS     = CRYPT_SUBJTYPE_SS_IMAGE;
    SIP_ADD_NEWPROVIDER sProv;

    fRet = TRUE;


    memset(&sProv, 0x00, sizeof(SIP_ADD_NEWPROVIDER));
    
    sProv.cbStruct              = sizeof(SIP_ADD_NEWPROVIDER);
    sProv.pwszDLLFileName       = MY_NAME;

    sProv.pwszGetFuncName       = L"CryptSIPGetSignedDataMsg";
    sProv.pwszPutFuncName       = L"CryptSIPPutSignedDataMsg";
    sProv.pwszCreateFuncName    = L"CryptSIPCreateIndirectData";
    sProv.pwszVerifyFuncName    = L"CryptSIPVerifyIndirectData";
    sProv.pwszRemoveFuncName    = L"CryptSIPRemoveSignedDataMsg";


    sProv.pgSubject             = &gFlat;
    fRet &= CryptSIPAddProvider(&sProv);

    sProv.pgSubject             = &gCb;
    sProv.pwszMagicNumber       = L"MSCF";

    fRet &= CryptSIPAddProvider(&sProv);

    sProv.pgSubject             = &gPe;
    sProv.pwszMagicNumber       = L"0x00004550";

    fRet &= CryptSIPAddProvider(&sProv);

    sProv.pgSubject             = &gJv;
    sProv.pwszMagicNumber       = L"0xcafebabe";

    fRet &= CryptSIPAddProvider(&sProv);

    sProv.pgSubject             = &gCat;

    fRet &= CryptSIPAddProvider(&sProv);

    sProv.pgSubject             = &gCTL;

    fRet &= CryptSIPAddProvider(&sProv);

    //
    //  structured storage is last becuase it
    //  has an "is" function...
    //
   /* sProv.pgSubject                 = &gSS;
    sProv.pwszIsFunctionNameFmt2    = L"IsStructuredStorageFile";

    fRet &= CryptSIPAddProvider(&sProv); */

    CryptSIPRemoveProvider(&gSS);

    return(fRet ? S_OK : S_FALSE);
}


STDAPI mssip32DllUnregisterServer(void)
{
    GUID                gFlat   = CRYPT_SUBJTYPE_FLAT_IMAGE;
    GUID                gPe     = CRYPT_SUBJTYPE_PE_IMAGE;
    GUID                gCb     = CRYPT_SUBJTYPE_CABINET_IMAGE;
    GUID                gJv     = CRYPT_SUBJTYPE_JAVACLASS_IMAGE;
    GUID                gCat    = CRYPT_SUBJTYPE_CATALOG_IMAGE;
    GUID                gCTL    = CRYPT_SUBJTYPE_CTL_IMAGE;
    GUID                gSS     = CRYPT_SUBJTYPE_SS_IMAGE;


    CryptSIPRemoveProvider(&gFlat);
    CryptSIPRemoveProvider(&gPe);
    CryptSIPRemoveProvider(&gCb);
    CryptSIPRemoveProvider(&gJv);
    CryptSIPRemoveProvider(&gCat);
    CryptSIPRemoveProvider(&gCTL);
    CryptSIPRemoveProvider(&gSS);

    return(S_OK);
}


void CryptSIPGetRegWorkingFlags(DWORD *pdwState) 
{
    WintrustGetRegPolicyFlags(pdwState);
}

//
//  support for Auth2 release
//
typedef struct _SIP_INFORMATION
{
    DWORD       cbSize;         // sizeof(SIP_INFORMATION)
    DWORD       cgSubjects;     // number of guids in array
    const GUID  *pgSubjects;    // array of supported guids/subjects
} SIP_INFORMATION, *PSIP_INFORMATION;

BOOL CryptSIPGetInfo(IN OUT SIP_INFORMATION    *pSIPInit)
{
    UINT    i;

    i = 0;

    pSIPInit->cbSize = sizeof(SIP_INFORMATION);

    while (SubjectsID[i] != MSSIP_ID_NONE)
    {
        i++;
    }

    pSIPInit->cgSubjects = i;
    pSIPInit->pgSubjects = &SubjectsGuid[0];

    return(TRUE);
}

