//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjFL.cpp    (Flat)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    15-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "sipobjfl.hxx"

////////////////////////////////////////////////////////////////////////////
//
// construct/destruct:
//

SIPObjectFlat_::SIPObjectFlat_(DWORD id) : SIPObject_(id)
{
    memset(&SpcLink,0x00,sizeof(SPC_LINK));

    SpcLink.dwLinkChoice    = SPC_FILE_LINK_CHOICE;
    SpcLink.pwszFile        = OBSOLETE_TEXT_W;
}

////////////////////////////////////////////////////////////////////////////
//
// public:
//

BOOL SIPObjectFlat_::GetSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx,
                                     DWORD *pdwDLen,BYTE *pbData,
                                     DWORD *pdwEncodeType)
{
    *pdwDLen = 0;
    *pdwEncodeType  = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;
    return(TRUE);
}

BOOL SIPObjectFlat_::VerifyIndirectData(SIP_SUBJECTINFO *pSI,
                                      SIP_INDIRECT_DATA *psData)
{
    MS_ADDINFO_FLAT             *pAddInfo;
    SIP_INDIRECT_DATA           *pInd;

    if (!(WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(SIP_SUBJECTINFO, pSI->cbSize, dwUnionChoice)) ||
        !(pSI->psFlat) ||
        !(psData))
    {
        if (this->FileHandleFromSubject(pSI))   // if the file exists, set bad parameter!
        {
            SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        }
        return(FALSE);
    }

    if ((pSI->dwUnionChoice == MSSIP_ADDINFO_FLAT) &&
        (pSI->psFlat->cbStruct == sizeof(MS_ADDINFO_FLAT)))
    {
        pInd = pSI->psFlat->pIndirectData;
    }
    else if ((pSI->dwUnionChoice == MSSIP_ADDINFO_CATMEMBER) &&
             (pSI->psCatMember->cbStruct == sizeof(MS_ADDINFO_CATALOGMEMBER)))
    {
        if (!(pSI->psCatMember->pMember) ||
            (pSI->psCatMember->pMember->cbStruct != sizeof(CRYPTCATMEMBER)) ||
            !(pSI->psCatMember->pMember->pIndirectData))
        {
            if (this->FileHandleFromSubject(pSI))   // if the file exists, set bad parameter!
            {
                SetLastError((DWORD)ERROR_INVALID_PARAMETER);
            }
            return(FALSE);
        }

        pInd = pSI->psCatMember->pMember->pIndirectData;
    }
    else
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (this->FileHandleFromSubject(pSI))
    {
        DWORD   cbDigest;
        BYTE    *pbDigest;

        if (!(pbDigest = this->DigestFile(  pSI->hProv,
                                            0,
                                            pInd->DigestAlgorithm.pszObjId,
                                            &cbDigest)))
        {
            return(FALSE);
        }

        if ((cbDigest != pInd->Digest.cbData) ||
            (memcmp(pbDigest, pInd->Digest.pbData, cbDigest) != 0))
        {
            delete pbDigest;

            SetLastError(TRUST_E_BAD_DIGEST);
            return(FALSE);
        }

        delete pbDigest;

        return(TRUE);
    }

    return(FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
// protected:
//

BOOL SIPObjectFlat_::GetDigestStream(DIGEST_DATA *pDigestData,
                                   DIGEST_FUNCTION pfnCallBack, DWORD dwFlags)
{
    __try {

    if ((this->pbFileMap) && (this->cbFileMap > 0))
    {
        return(pfnCallBack((HSPCDIGESTDATA)pDigestData, this->pbFileMap, this->cbFileMap));
    }

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
    }

    return(FALSE);
}


