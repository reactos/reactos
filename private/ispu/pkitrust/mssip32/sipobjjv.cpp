//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjJV.cpp (JAVA)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    15-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"

#include    "sipobjjv.hxx"

////////////////////////////////////////////////////////////////////////////
//
// construct/destruct:
//

SIPObjectJAVA_::SIPObjectJAVA_(DWORD id) : SIPObject_(id)
{
    memset(&SpcLink,0x00,sizeof(SPC_LINK));

    SpcLink.dwLinkChoice    = SPC_FILE_LINK_CHOICE;
    SpcLink.pwszFile        = OBSOLETE_TEXT_W;
}

BOOL SIPObjectJAVA_::RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx)
{
    if (this->FileHandleFromSubject(pSI, GENERIC_READ | GENERIC_WRITE))
    {
        return(JavaRemoveCertificate(this->hFile,dwIdx));
    }

    return(FALSE);
}

//////////////////////////////////////////////////////////////////////////////////////
//
// protected:
//

BOOL SIPObjectJAVA_::GetMessageFromFile(SIP_SUBJECTINFO *pSI, WIN_CERTIFICATE *pWinCert,
                                        DWORD dwIndex,DWORD *pcbCert)
{
    return(JavaGetCertificateData(this->hFile,dwIndex,pWinCert,pcbCert));
}

BOOL SIPObjectJAVA_::PutMessageInFile(SIP_SUBJECTINFO *pSI, WIN_CERTIFICATE *pWinCert,
                                      DWORD *pdwIndex)
{
    if ((pWinCert->dwLength <= OFFSETOF(WIN_CERTIFICATE,bCertificate))  ||
        (pWinCert->wCertificateType != WIN_CERT_TYPE_PKCS_SIGNED_DATA))
    {
        SetLastError((DWORD)ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (pdwIndex)
    {
        *pdwIndex = 0; // java only has 1
    }

    return(JavaAddCertificate(this->hFile,pWinCert,pdwIndex));
}


BOOL SIPObjectJAVA_::GetDigestStream(DIGEST_DATA *pDigestData, 
                                     DIGEST_FUNCTION pfnCallBack, DWORD dwFlags)
{
    return(JavaGetDigestStream( this->hFile,
                                dwFlags,
                                pfnCallBack,
                                pDigestData));
}

