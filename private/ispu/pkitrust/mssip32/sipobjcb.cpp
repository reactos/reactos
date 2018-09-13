//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjCB.cpp    (CAB)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    15-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "sipobjcb.hxx"

#include    "sha.h"
#include    "md5.h"

////////////////////////////////////////////////////////////////////////////
//
// construct/destruct:
//

SIPObjectCAB_::SIPObjectCAB_(DWORD id) : SIPObject_(id)
{
    memset(&Para, 0x00, sizeof(CAB_PARA));

    fUseV1Sig           = FALSE;
}

SIPObjectCAB_::~SIPObjectCAB_(void)
{
    FreeHeader();
}

////////////////////////////////////////////////////////////////////////////
//
// public:
//

BOOL SIPObjectCAB_::RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx)
{
    if (this->FileHandleFromSubject(pSI, GENERIC_READ | GENERIC_WRITE))
    {
        return(this->RemoveCertificate(dwIdx));
    }

    return(FALSE);
}

BOOL SIPObjectCAB_::CreateIndirectData(SIP_SUBJECTINFO *pSI,DWORD *pdwDLen,
                                   SIP_INDIRECT_DATA *psData)
{
    BOOL                    fRet;
    BYTE                    *pbDigest;
    BYTE                    *pbAttrData;

    SPC_LINK                SpcLink;
    DWORD                   cbDigest;
    HCRYPTPROV              hProvT;


    pbDigest    = NULL;
    pbAttrData  = NULL;
    fRet        = TRUE;

    hProvT = pSI->hProv;

    if (!(hProvT))
    {
        if (!(this->LoadDefaultProvider()))
        {
            goto GetProviderFailed;
        }

        hProvT = this->hProv;
    }

    memset(&SpcLink,0x00,sizeof(SPC_LINK));

    SpcLink.dwLinkChoice    = SPC_FILE_LINK_CHOICE;
    SpcLink.pwszFile        = OBSOLETE_TEXT_W;

    if (!(psData))
    {
        HCRYPTHASH  hHash;
        DWORD       dwRetLen;
        DWORD       dwEncLen;
        DWORD       dwAlgId;

        dwRetLen = sizeof(SIP_INDIRECT_DATA);

        // crypt_algorithm_identifier...
            // obj id
        dwRetLen += strlen(pSI->DigestAlgorithm.pszObjId);
        dwRetLen += 1;  // null term.
            // parameters (none)...

        // crypt_attribute_type_value size...
        dwRetLen += strlen(this->GetDataObjectID());
        dwRetLen += 1; // null term.

        // size of the value
        dwEncLen = 0;
        CryptEncodeObject(  PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
                            this->GetDataOIDHint(),
                            &SpcLink,
                            NULL,
                            &dwEncLen);

        if (dwEncLen < 1)
        {
            goto EncodeError;
        }

        dwRetLen += dwEncLen;

        if ((dwAlgId = CertOIDToAlgId(pSI->DigestAlgorithm.pszObjId)) == 0)
        {
            goto BadAlgId;
        }

        switch (dwAlgId)
        {
            case CALG_MD5:
                cbDigest = MD5DIGESTLEN;
                break;

            case CALG_SHA1:
                cbDigest = A_SHA_DIGEST_LEN;
                break;

            default:
                if (!(CryptCreateHash(hProvT, dwAlgId, NULL, 0, &hHash)))
                {
                    goto CreateHashFailed;
                }

                // just to get hash length
                if (!(CryptHashData(hHash,(const BYTE *)" ",1,0)))
                {
                    CryptDestroyHash(hHash);

                    goto HashDataFailed;
                }

                cbDigest = 0;

                CryptGetHashParam(hHash, HP_HASHVAL, NULL, &cbDigest,0);

                CryptDestroyHash(hHash);
        }


        dwRetLen += cbDigest;
        *pdwDLen = dwRetLen;

        goto CommonReturn;
    }

    if (!(this->FileHandleFromSubject(pSI, (pSI->dwFlags & MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE) ?
                                                    GENERIC_READ : (GENERIC_READ | GENERIC_WRITE))))
    {
        goto SubjectFileFailure;
    }

    //
    //  version 1 had the signature in the header.  We want
    //  the signature at the end and our structure in the
    //  header where the signature used to be.  -- check it.
    //
    if (!(pSI->dwFlags & MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE))
    {
        if (!(this->ReadHeader()))
        {
            goto ReadHeaderFailed;
        }

        if (!(this->ReserveSignedData(sizeof(CABSignatureStruct_))))
        {
            goto ReserveDataFailed;
        }

        if (!(this->MapFile()))
        {
            goto MapFileFailed;
        }
    }

    if (!(pbDigest = this->DigestFile(hProvT, 0, pSI->DigestAlgorithm.pszObjId, &cbDigest)))
    {
        goto DigestFileFailed;
    }

    DWORD_PTR dwOffset;
    DWORD   dwRetLen;

    dwRetLen = 0;

    CryptEncodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, this->GetDataOIDHint(), &SpcLink,
                        NULL, &dwRetLen);

    if (dwRetLen < 1)
    {
        goto EncodeError;
    }

    if (!(pbAttrData = (BYTE *)this->SIPNew(dwRetLen)))
    {
        goto MemoryError;
    }

    if (!(CryptEncodeObject(PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, this->GetDataOIDHint(), &SpcLink,
                            pbAttrData, &dwRetLen)))
    {
        goto EncodeError;
    }

    dwOffset =    (DWORD_PTR)psData + sizeof(SIP_INDIRECT_DATA);

    strcpy((char *)dwOffset, this->GetDataObjectID());
    psData->Data.pszObjId   = (LPSTR)dwOffset;
    dwOffset += (strlen(SPC_LINK_OBJID) + 1);

    memcpy((void *)dwOffset, pbAttrData,dwRetLen);
    psData->Data.Value.pbData   = (BYTE *)dwOffset;
    psData->Data.Value.cbData   = dwRetLen;
    dwOffset += dwRetLen;

    strcpy((char *)dwOffset, (char *)pSI->DigestAlgorithm.pszObjId);
    psData->DigestAlgorithm.pszObjId            = (char *)dwOffset;
    psData->DigestAlgorithm.Parameters.cbData   = 0;
    psData->DigestAlgorithm.Parameters.pbData   = NULL;
    dwOffset += (strlen(pSI->DigestAlgorithm.pszObjId) + 1);

    memcpy((void *)dwOffset,pbDigest,cbDigest);
    psData->Digest.pbData   = (BYTE *)dwOffset;
    psData->Digest.cbData   = cbDigest;

CommonReturn:

    if (pbDigest)
    {
        delete pbDigest;
    }

    if (pbAttrData)
    {
        delete pbAttrData;
    }

    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, EncodeError);
    TRACE_ERROR_EX(DBG_SS, SubjectFileFailure);
    TRACE_ERROR_EX(DBG_SS, HashDataFailed);
    TRACE_ERROR_EX(DBG_SS, CreateHashFailed);
    TRACE_ERROR_EX(DBG_SS, ReadHeaderFailed);
    TRACE_ERROR_EX(DBG_SS, ReserveDataFailed);
    TRACE_ERROR_EX(DBG_SS, MapFileFailed);
    TRACE_ERROR_EX(DBG_SS, DigestFileFailed);
    TRACE_ERROR_EX(DBG_SS, GetProviderFailed);

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,   ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, BadAlgId,      NTE_BAD_ALGID);
}

////////////////////////////////////////////////////////////////////////////
//
// protected:
//

BOOL SIPObjectCAB_::GetMessageFromFile(SIP_SUBJECTINFO *pSI,
                                      WIN_CERTIFICATE *pWinCert,
                                      DWORD dwIndex,DWORD *pcbCert)
{
    DWORD       cbCert;

    if (dwIndex != 0)
    {
        goto InvalidParam;
    }

    if (!(this->ReadHeader()))
    {
        goto ReadHeaderFailed;
    }

    if (Para.Hdr.cbSig == 0)
    {
        goto NoSignature;
    }

    if (!(fUseV1Sig))
    {
        //
        //  Version 2 header
        //

        cbCert          = OFFSETOF(WIN_CERTIFICATE, bCertificate) +
                          Para.Hdr.pCabSigStruct->cbSig;

        if (*pcbCert < cbCert)
        {
            *pcbCert = cbCert;

            goto BufferTooSmall;
        }

        if (pWinCert)
        {
            if (!(this->ReadSignedData(pWinCert->bCertificate)))
            {
                goto ReadSignedFailed;
            }

        }
    }
    else
    {
        //
        //  Version 1 header
        //
        cbCert          = OFFSETOF(WIN_CERTIFICATE, bCertificate) + Para.Hdr.cbSig;

        if (*pcbCert < cbCert)
        {
            *pcbCert = cbCert;

            goto BufferTooSmall;
        }

        if (pWinCert)
        {
            BYTE    *pbSignedData;

            pbSignedData = Para.Hdr.pbReserve + RESERVE_CNT_HDR_LEN + Para.Hdr.cbJunk;

            pWinCert->wRevision = WIN_CERT_REVISION_1_0;

            memcpy(pWinCert->bCertificate, pbSignedData, Para.Hdr.cbSig);
        }
    }

    pWinCert->dwLength          = cbCert;
    pWinCert->wCertificateType  = WIN_CERT_TYPE_PKCS_SIGNED_DATA;

    return(TRUE);

ErrorReturn:
    return(FALSE);

    TRACE_ERROR_EX(DBG_SS, ReadHeaderFailed);
    TRACE_ERROR_EX(DBG_SS, ReadSignedFailed);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, BufferTooSmall,ERROR_INSUFFICIENT_BUFFER);
    SET_ERROR_VAR_EX(DBG_SS, NoSignature,   TRUST_E_NOSIGNATURE);
}

BOOL SIPObjectCAB_::PutMessageInFile(SIP_SUBJECTINFO *pSI,
                                    WIN_CERTIFICATE *pWinCert,DWORD *pdwIndex)
{
    if ((pWinCert->dwLength <= OFFSETOF(WIN_CERTIFICATE,bCertificate))  ||
        (pWinCert->wCertificateType != WIN_CERT_TYPE_PKCS_SIGNED_DATA))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (this->ReadHeader())
    {
        if (!(fUseV1Sig))
        {
            //
            //  version 2
            //
            if (this->WriteSignedData((BYTE *)&(pWinCert->bCertificate),
                                      pWinCert->dwLength -
                                      OFFSETOF(WIN_CERTIFICATE, bCertificate)))
            {
                return(TRUE);
            }
        }
        else
        {
            //
            //  version 1
            //
            DWORD   dwCheck;
            DWORD   cbSignedData;

            cbSignedData    = pWinCert->dwLength - OFFSETOF(WIN_CERTIFICATE, bCertificate);

            dwCheck = RESERVE_LEN_ALIGN(RESERVE_CNT_HDR_LEN + Para.Hdr.cbJunk + cbSignedData) -
                        Para.Hdr.cfres.cbCFHeader;

            if (dwCheck > 0)
            {
                SetLastError(CRYPT_E_FILERESIZED);
                return(FALSE);
            }


            if (WriteSignedDataV1((PBYTE)&(pWinCert->bCertificate), cbSignedData))
            {
                return(TRUE);
            }
        }
    }

    return(FALSE);
}

BOOL SIPObjectCAB_::GetDigestStream(DIGEST_DATA *pDigestData,
                                   DIGEST_FUNCTION pfnCallBack, DWORD dwFlags)
{
    if (dwFlags != 0)
    {
        goto InvalidParam;
    }

    if (!(this->ReadHeader()))
    {
        goto ReadHeaderFailed;
    }

    if (!(this->DigestHeader(pfnCallBack, pDigestData)))
    {
        goto DigestFailed;
    }

    DWORD   cbRemain;

    cbRemain = this->cbFileMap - Para.Hdr.cbTotalHdr;

    if (!(fUseV1Sig) && (Para.Hdr.pCabSigStruct))
    {
        cbRemain -= Para.Hdr.pCabSigStruct->cbSig;
    }

    if ((Para.Hdr.cfheader.cbCabinet - Para.Hdr.cbTotalHdr) != cbRemain)
    {
        goto BadFileFormat;
    }

    if (this->cbFileMap < (Para.Hdr.cbTotalHdr + cbRemain))
    {
        goto BadFileFormat;
    }

    __try {

    if (!(pfnCallBack(pDigestData, &this->pbFileMap[Para.Hdr.cbTotalHdr], cbRemain)))
    {
        goto HashFailed;
    }

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        goto HashFailed;
    }


    return(TRUE);

ErrorReturn:
    return(FALSE);

    TRACE_ERROR_EX(DBG_SS, DigestFailed);
    TRACE_ERROR_EX(DBG_SS, ReadHeaderFailed);
    TRACE_ERROR_EX(DBG_SS, HashFailed);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,  ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, BadFileFormat, ERROR_BAD_FORMAT);
}


////////////////////////////////////////////////////////////////////////////
//
// private:
//

BOOL SIPObjectCAB_::RemoveCertificate(DWORD Index)
{
    return(FALSE);   // not yet!!! Currently, we only support 1.

#   ifdef _DONT_USE_YET

        BYTE            *pbFolders;
        DWORD           cbFolders;
        BYTE            *pbReserve;
        USHORT          cbReserve;

        if (Index != 0)
        {
            SetLastError((DWORD)ERROR_INVALID_PARAMETER);
            return(FALSE);
        }

        pbFolders   = NULL;
        cbFolders   = 0;

        Para.dwFlags = VERIFY_CAB_FLAG;

        if (this->ReadHeader())
        {
            if (Para.Hdr.cbSig <= (RESERVE_CNT_HDR_LEN + Para.Hdr.cbJunk))
            {
                SetLastError((DWORD)CRYPT_E_NO_MATCH);
                return(FALSE);
            }

            long    lShift;

            if (Para.Hdr.cbJunk)
            {
                lShift                                                  = Para.Hdr.cbSig;
                if (Para.Hdr.pbReserve)
                {
                    *((USHORT *)Para.Hdr.pbReserve)                     = Para.Hdr.cbJunk;
                    *((USHORT *)(Para.Hdr.pbReserve + sizeof(USHORT)))  = 0;    // no more sig
                }
            }
            else
            {
                lShift                  = Para.Hdr.cbSig + (sizeof(USHORT) * 2);
                Para.Hdr.cfheader.flags &= ~(cfhdrRESERVE_PRESENT);
                if (Para.Hdr.pbReserve)
                {
                    delete Para.Hdr.pbReserve;
                    Para.Hdr.pbReserve = NULL;
                }
            }

            Para.Hdr.cbSig              = 0;
            Para.Hdr.cfres.cbCFHeader   -= (USHORT)lShift;  // subtract the amount we want to shrink.

            // adjust the header offsets
            if (this->ShiftFileBytes(lShift))
            {
                Para.Hdr.cbTotalHdr         -= lShift;
                Para.Hdr.cfheader.cbCabinet -= lShift;
                Para.Hdr.cfheader.coffFiles -= lShift;
            }

            // redo checksums....
            this->ChecksumHeader();

            if (this->WriteHeader())
            {
                // We need to read in the folders to adjust their CFDATA file offset
                if (Para.Hdr.cfheader.cFolders)
                {
                    if (SetFilePointer(this->hFile,
                                        Para.Hdr.cbTotalHdr + lShift,
                                        NULL, FILE_BEGIN) == 0xFFFFFFFF)
                    {
                        return(FALSE);
                    }

                    USHORT  cFolders;
                    LONG    cbFolder;

                    cFolders    = Para.Hdr.cfheader.cFolders;
                    cbFolder    = sizeof(CFFOLDER) + Para.Hdr.cfres.cbCFFolder;
                    cbFolders   = cbFolder * cFolders;

                    if (!(pbFolders = (BYTE *)this->SIPNew(cbFolders)))
                    {
                        return(FALSE);
                    }
                    DWORD   cbFile;

                    if (!(ReadFile(this->hFile, pbFolders, cbFolders, &cbFile, NULL)) ||
                         (cbFile != cbFolders))
                    {
                        delete pbFolders;
                        SetLastError(ERROR_BAD_FORMAT);
                        return(FALSE);
                    }


                    BYTE    *pb;

                    pb = pbFolders;

                    while (cFolders > 0)
                    {
                        ((CFFOLDER *)pb)->coffCabStart -= lShift;
                        pb += cbFolder;
                        cFolders--;
                    }

                    // back up and write!
                    if (SetFilePointer(this->hFile, -((LONG)cbFolders),
                                        NULL, FILE_CURRENT) == 0xFFFFFFFF)
                    {
                        delete pbFolders;
                        return(FALSE);
                    }

                    if (!(WriteFile(this->hFile, pbFolders, cbFolders, &cbFile, NULL)) ||
                            (cbFile != cbFolders))
                    {
                        delete pbFolders;
                        return(FALSE);
                    }

                    delete pbFolders;
                }

                return(TRUE);
            }
        }

        return(FALSE);

#   endif // _DONT_USE_YET
}

BOOL SIPObjectCAB_::ReadSignedData(BYTE *pbRet)
{
    //
    //  this function is NOT called for version 1 Sigs!
    //

    if (Para.Hdr.pCabSigStruct->cbFileOffset != (DWORD)Para.Hdr.cfheader.cbCabinet)
    {
        SetLastError((DWORD)TRUST_E_NOSIGNATURE);
        return(FALSE);
    }

    if (this->cbFileMap < (Para.Hdr.pCabSigStruct->cbFileOffset +
                           Para.Hdr.pCabSigStruct->cbSig))
    {
        SetLastError(ERROR_BAD_FORMAT);
        return(FALSE);
    }

    __try {
    memcpy(pbRet, &this->pbFileMap[Para.Hdr.pCabSigStruct->cbFileOffset], Para.Hdr.pCabSigStruct->cbSig);
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        return(FALSE);
    }

    return(TRUE);
}

BOOL SIPObjectCAB_::WriteSignedData(BYTE *pbSig, DWORD cbSig)
{
    //
    //  this function is NOT called for version 1 Sigs!
    //

    if (!(pbSig) || (cbSig == 0))
    {
        return(FALSE);
    }

    CABSignatureStruct_     sSig;

    memset(&sSig, 0x00, sizeof(CABSignatureStruct_));

    sSig.cbFileOffset   = Para.Hdr.cfheader.cbCabinet;
    sSig.cbSig          = cbSig;

    memcpy(Para.Hdr.pbReserve + RESERVE_CNT_HDR_LEN + Para.Hdr.cbJunk,
            &sSig, sizeof(CABSignatureStruct_));

    if (!(this->WriteHeader()))
    {
        return(FALSE);
    }

    if (SetFilePointer(this->hFile, Para.Hdr.cfheader.cbCabinet, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    DWORD   cbWritten;

    if (!(WriteFile(this->hFile, pbSig, cbSig, &cbWritten, NULL)) ||
        (cbWritten != cbSig))
    {
        return(FALSE);
    }

    this->UnmapFile();

    SetEndOfFile(this->hFile);    // signature is the LAST thing!!!

    return(this->MapFile());
}

BOOL SIPObjectCAB_::WriteSignedDataV1(BYTE *pbSignedData, DWORD cbSignedData)
{
    if (!(pbSignedData) || (cbSignedData == 0))
    {
        return(FALSE);
    }

    memcpy(Para.Hdr.pbReserve + RESERVE_CNT_HDR_LEN + Para.Hdr.cbJunk,
                        pbSignedData, cbSignedData);
    Para.Hdr.cbSig = (USHORT)cbSignedData;

    ChecksumHeader();

    return(this->WriteHeader());
}

BOOL SIPObjectCAB_::ReadHeader(void)
{
    DWORD   cbOffset;
    BOOL    fRet;

    this->FreeHeader();

    if (this->cbFileMap < sizeof(Para.Hdr.cfheader))
    {
        goto BadCABFormat;
    }

    __try {

    memcpy(&Para.Hdr.cfheader, &this->pbFileMap[0], sizeof(Para.Hdr.cfheader));

    cbOffset = sizeof(Para.Hdr.cfheader);

    if (Para.Hdr.cfheader.sig != sigCFHEADER)
    {
        goto BadCABFormat;
    }

    if (Para.Hdr.cfheader.flags & cfhdrRESERVE_PRESENT)
    {
        if (this->cbFileMap < (cbOffset + sizeof(Para.Hdr.cfres)))
        {
            goto BadCABFormat;
        }

        memcpy(&Para.Hdr.cfres, &this->pbFileMap[cbOffset], sizeof(Para.Hdr.cfres));

        cbOffset += sizeof(Para.Hdr.cfres);

        Para.Hdr.cbcfres = sizeof(Para.Hdr.cfres);

        if (Para.Hdr.cfres.cbCFHeader > 0)
        {
            if (Para.Hdr.pbReserve = (BYTE *)this->SIPNew(Para.Hdr.cfres.cbCFHeader))
            {
                if (this->cbFileMap < (cbOffset + Para.Hdr.cfres.cbCFHeader))
                {
                    goto BadCABFormat;
                }

                memcpy(Para.Hdr.pbReserve, &this->pbFileMap[cbOffset], Para.Hdr.cfres.cbCFHeader);

                cbOffset += Para.Hdr.cfres.cbCFHeader;

                if (Para.Hdr.cfres.cbCFHeader >= RESERVE_CNT_HDR_LEN)
                {
                    Para.Hdr.cbJunk = *((USHORT *)Para.Hdr.pbReserve);
                    Para.Hdr.cbSig  = *((USHORT *)(Para.Hdr.pbReserve + sizeof(USHORT)));

                    if (RESERVE_CNT_HDR_LEN + Para.Hdr.cbJunk + Para.Hdr.cbSig > Para.Hdr.cfres.cbCFHeader)
                    {
                        goto BadCABFormat;
                    }

                    if (Para.Hdr.cbSig == sizeof(CABSignatureStruct_))
                    {
                        fUseV1Sig = FALSE;

                        Para.Hdr.pCabSigStruct = (CABSignatureStruct_ *)(Para.Hdr.pbReserve +
                                                                         RESERVE_CNT_HDR_LEN +
                                                                         Para.Hdr.cbJunk);
                    }
                    else
                    {
                        fUseV1Sig = TRUE;
                    }
                }
            }
        }
    }

    DWORD   cStrings;
    DWORD   cb;

    cStrings = 0;

    if (Para.Hdr.cfheader.flags & cfhdrPREV_CABINET)
    {
        cStrings += 2;
    }

    if (Para.Hdr.cfheader.flags & cfhdrNEXT_CABINET)
    {
        cStrings += 2;
    }

    if (cStrings > 0)
    {
        // First read to get total length of all the strings
        cb = 0;
        for (; cStrings > 0; cStrings--)
        {
            while (this->pbFileMap[cbOffset + cb])
            {
                cb++;

                if (this->cbFileMap < (cbOffset + cb))
                {
                    goto BadCABFormat;
                }
            }

            //Increment the counter for the NULL terminator
            cb++;
        }

        if (!(Para.Hdr.pbStrings = new BYTE[cb]))
        {
            goto MemoryError;
        }

        Para.Hdr.cbStrings  = cb;

        memcpy(Para.Hdr.pbStrings, &this->pbFileMap[cbOffset], cb);

        cbOffset += cb;
    }

    Para.Hdr.cbTotalHdr = sizeof(Para.Hdr.cfheader) + Para.Hdr.cbcfres +
                            Para.Hdr.cfres.cbCFHeader + Para.Hdr.cbStrings;

    if ((long)Para.Hdr.cbTotalHdr > Para.Hdr.cfheader.cbCabinet)
    {
        goto BadCABFormat;
    }

    fRet = TRUE;

    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(GetExceptionCode());
        goto ErrorReturn;
    }

CommonReturn:
    return(fRet);

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, BadCABFormat, ERROR_BAD_FORMAT);
    SET_ERROR_VAR_EX(DBG_SS, MemoryError,  ERROR_NOT_ENOUGH_MEMORY);
}

void SIPObjectCAB_::FreeHeader(void)
{
    DELETE_OBJECT(Para.Hdr.pbReserve);
    DELETE_OBJECT(Para.Hdr.pbStrings);

    memset(&Para, 0x00, sizeof(CAB_PARA));
}

BOOL SIPObjectCAB_::WriteHeader(void)
{
    DWORD cbWritten;

    // Position at beginning of file
    if (SetFilePointer(this->hFile, 0, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    if (!(WriteFile(this->hFile, &Para.Hdr.cfheader, sizeof(Para.Hdr.cfheader),
                        &cbWritten, NULL)) ||
        (cbWritten != sizeof(Para.Hdr.cfheader)))
    {
        return(FALSE);
    }

    if (Para.Hdr.cbcfres)
    {
        if (!(WriteFile(this->hFile, &Para.Hdr.cfres, sizeof(Para.Hdr.cfres),
                        &cbWritten, NULL)) ||
            (cbWritten != sizeof(Para.Hdr.cfres)))
        {
            return(FALSE);
        }

        if (Para.Hdr.pbReserve)
        {
            *((USHORT *)(Para.Hdr.pbReserve + sizeof(USHORT)))  = Para.Hdr.cbSig;

            if (!(WriteFile(this->hFile, Para.Hdr.pbReserve, Para.Hdr.cfres.cbCFHeader,
                                &cbWritten, NULL)) ||
                (cbWritten != Para.Hdr.cfres.cbCFHeader))
            {
                return(FALSE);
            }
        }
    }

    if (Para.Hdr.pbStrings)
    {
        if (!(WriteFile(this->hFile, Para.Hdr.pbStrings, Para.Hdr.cbStrings,
                            &cbWritten, NULL)) ||
            (cbWritten != Para.Hdr.cbStrings))
        {
            return(FALSE);
        }
    }

    return(TRUE);
}

BOOL SIPObjectCAB_::ShiftFileBytes(LONG lbShift)
{
    LONG    lStartOffset;
    LONG    lEndOffset;
    LONG    lNewEndOffset;
    LONG    cbTotalMove;
    LONG    cbMove;

    lStartOffset    = SetFilePointer(this->hFile, 0, NULL, FILE_CURRENT);
    lEndOffset      = (LONG)this->cbFileMap;

    lNewEndOffset   = lEndOffset + lbShift;
    cbTotalMove     = lEndOffset - lStartOffset;

    BYTE    szMove[512];

    while (cbTotalMove)
    {
        cbMove = min(cbTotalMove, sizeof(szMove));

        if (lbShift > 0)
        {
            if (!(SeekAndReadFile(lEndOffset - cbMove, &szMove[0], cbMove)))
            {
                return(FALSE);
            }
            if (!(SeekAndWriteFile((lEndOffset - cbMove) + lbShift, &szMove[0], cbMove)))
            {
                return(FALSE);
            }

            lEndOffset -= cbMove;
        }
        else if (lbShift < 0)
        {
            if (!(SeekAndReadFile(lStartOffset, &szMove[0], cbMove)))
            {
                return(FALSE);
            }
            if (!(SeekAndWriteFile(lStartOffset + lbShift, &szMove[0], cbMove)))
            {
                return(FALSE);
            }

            lStartOffset += cbMove;
        }

        cbTotalMove -= cbMove;
    }

    //
    // Set end of file
    //
    if (SetFilePointer(this->hFile, lNewEndOffset, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    this->UnmapFile();

    SetEndOfFile(this->hFile);

    return(this->MapFile());
}


BOOL SIPObjectCAB_::ReserveSignedData(DWORD cbSignedData)
{
    LONG    lbShift;
    USHORT  cbReserve;


    if (cbSignedData != sizeof(CABSignatureStruct_))
    {
        return(FALSE);
    }

    if (SetFilePointer(this->hFile, Para.Hdr.cbTotalHdr, NULL, FILE_BEGIN) == 0xFFFFFFFF)
    {
        return(FALSE);
    }

    fUseV1Sig           = FALSE;

    //
    // Calculate length needed for CFRESERVE's abReserve[] and allocate
    //
    cbReserve = (USHORT)(RESERVE_LEN_ALIGN(RESERVE_CNT_HDR_LEN +
                Para.Hdr.cbJunk + cbSignedData));

    //
    // Calculate number of bytes to grow or shrink the cab file
    //
    lbShift = cbReserve - Para.Hdr.cfres.cbCFHeader;

    //
    //  we're alread a v1 cab!
    //
    if (lbShift == 0)
    {
        return(TRUE);
    }

    BYTE    *pbReserve;
    BYTE    *pbFolders;
    DWORD   cbFolders;

    pbFolders   = NULL;
    cbFolders   = 0;


    if (!(pbReserve = (BYTE *)this->SIPNew(cbReserve)))
    {
        return(FALSE);
    }

    memset(pbReserve, 0x00, cbReserve);

    //
    // Update allocated abReserve[] with counts and old junk
    //
    if (Para.Hdr.cbJunk)
    {
        *((USHORT *)pbReserve) = Para.Hdr.cbJunk;
        memcpy(pbReserve + RESERVE_CNT_HDR_LEN,
                Para.Hdr.pbReserve + RESERVE_CNT_HDR_LEN, Para.Hdr.cbJunk);
    }
    *((USHORT *)(pbReserve + sizeof(USHORT))) = (USHORT)cbSignedData;

    //
    // Update Hdr's CFRESERVE abReserve[] to reflect above changes
    //
    if (Para.Hdr.pbReserve)
    {
        delete Para.Hdr.pbReserve;
        Para.Hdr.pbReserve = NULL;
    }
    Para.Hdr.pbReserve          = pbReserve;
    Para.Hdr.cfres.cbCFHeader   = cbReserve;
    Para.Hdr.cbSig              = (USHORT)cbSignedData;

    if (Para.Hdr.cbcfres == 0)
    {
        // Need to add CFRESERVE record
        Para.Hdr.cfheader.flags |= cfhdrRESERVE_PRESENT;
        Para.Hdr.cbcfres        = sizeof(CFRESERVE);
        lbShift                 += sizeof(CFRESERVE);
    }

    //
    // We need to read in the folders to adjust their CFDATA file offset
    //
    if (Para.Hdr.cfheader.cFolders)
    {
        USHORT  cFolders;
        LONG    cbFolder;
        BYTE    *pb;
        DWORD   cbRead;

        cFolders    = Para.Hdr.cfheader.cFolders;
        cbFolder    = sizeof(CFFOLDER) + Para.Hdr.cfres.cbCFFolder;
        cbFolders   = cbFolder * cFolders;

        if (!(pbFolders = (BYTE *)this->SIPNew(cbFolders)))
        {
            return(FALSE);
        }

        if (!(ReadFile(this->hFile, pbFolders, cbFolders, &cbRead, NULL)) ||
            (cbRead != cbFolders))
        {
            delete pbFolders;
            SetLastError(ERROR_BAD_FORMAT);
            return(FALSE);
        }

        pb = pbFolders;

        for (; cFolders > 0; cFolders--, pb += cbFolder)
        {
            ((CFFOLDER *) pb)->coffCabStart += lbShift;
        }
    }

    //
    // We need to shift the remaining contents of the cab file (CFFILE (s)
    // and CFDATA (s)) by lbShift
    //
    if (!(ShiftFileBytes(lbShift)))
    {
        if (pbFolders)
        {
            delete pbFolders;
        }
        return(FALSE);
    }

    //
    // Update lengths and offsets in the header by the delta shift needed
    // to store the signed data.
    //
    Para.Hdr.cbTotalHdr         += lbShift;
    Para.Hdr.cfheader.cbCabinet += lbShift;
    Para.Hdr.cfheader.coffFiles += lbShift;

    //
    //  pberkman - if someone starts using these, we don't want to screw them up!!!
    //
    // Para.Hdr.cfheader.csumHeader    = 0;
    // Para.Hdr.cfheader.csumFolders   = 0;
    // Para.Hdr.cfheader.csumFiles     = 0;

    //
    // Write the header and folders back to the cab file
    //
    if (!(this->WriteHeader()))
    {
        if (pbFolders)
        {
            delete pbFolders;
        }
        return(FALSE);
    }

    if (pbFolders)
    {
        DWORD cbWritten;

        cbWritten = 0;
        if (!(WriteFile(this->hFile, pbFolders, cbFolders, &cbWritten, NULL)) ||
            (cbWritten != cbFolders))
        {
            delete pbFolders;
            return(FALSE);
        }
        delete pbFolders;
    }

    return(TRUE);
}

BOOL SIPObjectCAB_::DigestHeader(DIGEST_FUNCTION pfnDigestData, DIGEST_HANDLE hDigestData)
{
    //
    // Digest CFHEADER, skipping the csumHeader field
    //
    if (!(pfnDigestData(hDigestData, (BYTE *)&Para.Hdr.cfheader.sig,
                        sizeof(Para.Hdr.cfheader.sig))))
    {
        return(FALSE);
    }

    if (!(pfnDigestData(hDigestData, (BYTE *)&Para.Hdr.cfheader.cbCabinet,
                        sizeof(CFHEADER) - sizeof(Para.Hdr.cfheader.sig) - sizeof(CHECKSUM))))
    {
        return(FALSE);
    }

    if (Para.Hdr.cbcfres)
    {
        // skip the cfres itself!

        if (Para.Hdr.cfres.cbCFHeader >= RESERVE_CNT_HDR_LEN)
        {
            // Digest any "junk" in abReserve[] before the signature
            if (!(pfnDigestData(hDigestData, (BYTE *)&Para.Hdr.cbJunk,
                                    sizeof(Para.Hdr.cbJunk))))
            {
                return(FALSE);
            }
            if (Para.Hdr.cbJunk)
            {
                if (!(pfnDigestData(hDigestData,
                                    Para.Hdr.pbReserve + RESERVE_CNT_HDR_LEN,
                                    Para.Hdr.cbJunk)))
                {
                    return(FALSE);
                }
            }
        }
    }

    if (Para.Hdr.pbStrings)
    {
        // Digest the strings
        if (!(pfnDigestData(hDigestData, Para.Hdr.pbStrings, Para.Hdr.cbStrings)))
        {
            return(FALSE);
        }
    }

    return(TRUE);
}

void SIPObjectCAB_::ChecksumHeader(void)
{
    return;

    // version 1 set checksum to zero.  this seems to be the correct thing to do????

#   ifdef _DONT_USE_YET

        CHECKSUM csum = 0;

        if (Para.Hdr.cfheader.csumHeader == 0)
        {
            return;
        }

        // Checksum CFHEADER, skipping the csumHeader field
        csum = CSUMCompute(&Para.Hdr.cfheader.sig, sizeof(Para.Hdr.cfheader.sig), csum);
        csum = CSUMCompute(&Para.Hdr.cfheader.cbCabinet,
                            sizeof(CFHEADER) -
                            sizeof(Para.Hdr.cfheader.sig) -
                            sizeof(CHECKSUM),
                            csum);

        if (Para.Hdr.cbcfres)
        {
            csum = CSUMCompute(&Para.Hdr.cfres, sizeof(Para.Hdr.cfres), csum);
            if (Para.Hdr.pbReserve) // BUGBUG: verify we set cert area to NULLs!!!
            {
                csum = CSUMCompute(Para.Hdr.pbReserve, Para.Hdr.cfres.cbCFHeader, csum);
            }
        }

        if (Para.Hdr.pbStrings)
        {
            csum = CSUMCompute(Para.Hdr.pbStrings, Para.Hdr.cbStrings, csum);
        }

        Para.Hdr.cfheader.csumHeader = csum;

#   endif

}

#ifdef _DONT_USE_YET

    CHECKSUM SIPObjectCAB_::CSUMCompute(void *pv, UINT cb, CHECKSUM seed)
    {
        int         cUlong;                 // Number of ULONGs in block
        CHECKSUM    csum;                   // Checksum accumulator
        BYTE       *pb;
        ULONG       ul;

        cUlong = cb / 4;                    // Number of ULONGs
        csum = seed;                        // Init checksum
        pb = (BYTE*)pv;                            // Start at front of data block

        //** Checksum integral multiple of ULONGs
        while (cUlong-- > 0) {
            //** NOTE: Build ULONG in big/little-endian independent manner
            ul = *pb++;                     // Get low-order byte
            ul |= (((ULONG)(*pb++)) <<  8); // Add 2nd byte
            ul |= (((ULONG)(*pb++)) << 16); // Add 3nd byte
            ul |= (((ULONG)(*pb++)) << 24); // Add 4th byte

            csum ^= ul;                     // Update checksum
        }

        //** Checksum remainder bytes
        ul = 0;
        switch (cb % 4) {
            case 3:
                ul |= (((ULONG)(*pb++)) << 16); // Add 3nd byte
            case 2:
                ul |= (((ULONG)(*pb++)) <<  8); // Add 2nd byte
            case 1:
                ul |= *pb++;                    // Get low-order byte
            default:
                break;
        }
        csum ^= ul;                         // Update checksum

        //** Return computed checksum
        return csum;
    }

#endif // _DONT_USE_YET

