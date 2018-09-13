//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjPE.hxx    (Portable Executable)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    15-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef SIPOBJPE_HXX
#define SIPOBJPE_HXX

#include    "sipobj.hxx"

class SIPObjectPE_ : public SIPObject_
{
    public:
        SIPObjectPE_(DWORD id);
        virtual ~SIPObjectPE_(void) { ; }

        BOOL            CreateIndirectData(SIP_SUBJECTINFO *pSI,DWORD *pdwDLen,
                                        SIP_INDIRECT_DATA *psData);
        BOOL            VerifyIndirectData(SIP_SUBJECTINFO *pSI,
                                        SIP_INDIRECT_DATA *psData);

        virtual BOOL    RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx);

        virtual char    *GetDataObjectID(void)  { return(SPC_PE_IMAGE_DATA_OBJID); }
        virtual char    *GetDataOIDHint(void)   { return((char *)SPC_PE_IMAGE_DATA_STRUCT); }

    protected:

        DWORD           GetDigestFlags(SIP_SUBJECTINFO *pSI)
                            { return(this->ConvertSPCFlags(pSI->dwFlags)); }

        void            *GetMyStructure(SIP_SUBJECTINFO *pSI) { return(&PeInfo); }
        DWORD           GetMyStructureSize(void) { return(sizeof(SPC_PE_IMAGE_DATA)); }

        virtual BOOL    PutMessageInFile(SIP_SUBJECTINFO *pSI,
                                 WIN_CERTIFICATE *pWinCert,DWORD *pdwIndex);

        virtual BOOL    GetMessageFromFile(SIP_SUBJECTINFO *pSI,
                                        WIN_CERTIFICATE *pWinCert,DWORD dwIndex,
                                        DWORD *pcbCert)
                            { return(ImageGetCertificateData(this->hFile,dwIndex,pWinCert,pcbCert)); }

        virtual BOOL    GetDigestStream(DIGEST_DATA *pDigestData, 
                                        DIGEST_FUNCTION pfnCallBack, DWORD dwFlags);

        virtual DWORD   ConvertSPCFlags(DWORD InFlags);

    private:
        SPC_PE_IMAGE_DATA       PeInfo;
};

//
// code is in peimage2.cpp
//
extern BOOL imagehack_IsImagePEOnly(IN HANDLE FileHandle);
extern BOOL imagehack_AuImageGetDigestStream(   IN HANDLE           FileHandle,
                                                IN DWORD            DigestLevel,
                                                IN DIGEST_FUNCTION  DigestFunction,
                                                IN DIGEST_HANDLE    DigestHandle);

#endif // SIPOBJPE_HXX
