//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjJV.hxx (JAVA)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    15-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef SIPOBJJV_HXX
#define SIPOBJJV_HXX

#include    "sipobj.hxx"
#include    "sipobjcb.hxx"      // indirect data structs are the same!

class SIPObjectJAVA_ : public SIPObject_
{
    public:
        SIPObjectJAVA_(DWORD id);
        virtual ~SIPObjectJAVA_(void) { ; }

        BOOL    RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx);

        char    *GetDataObjectID(void) { return(SPC_JAVA_CLASS_DATA_OBJID); }
        char    *GetDataOIDHint(void)   { return((char *)SPC_JAVA_CLASS_DATA_STRUCT); }

    protected:
        void    *GetMyStructure(SIP_SUBJECTINFO *pSI) { return(&SpcLink); }
        DWORD   GetMyStructureSize(void) { return(sizeof(SPC_LINK)); }

        BOOL    PutMessageInFile(SIP_SUBJECTINFO *pSI,WIN_CERTIFICATE *pWinCert,
                                 DWORD *pdwIndex);
        BOOL    GetMessageFromFile(SIP_SUBJECTINFO *pSI,WIN_CERTIFICATE *pWinCert,
                                   DWORD dwIndex,DWORD *pcbCert);
        BOOL    GetDigestStream(DIGEST_DATA *pDigestData, 
                                DIGEST_FUNCTION pfnCallBack, DWORD dwFlags);
    private:
        SPC_LINK    SpcLink;
};

//
// code in jvimage.cpp
//
extern BOOL JavaGetDigestStream(    IN  HANDLE           FileHandle,
                                    IN  DWORD            DigestLevel,
                                    IN  DIGEST_FUNCTION  DigestFunction,
                                    IN  DIGEST_HANDLE    DigestHandle);

extern BOOL JavaRemoveCertificate(  IN      HANDLE      FileHandle,
                                    IN      DWORD       Index);

extern BOOL JavaGetCertificateData( IN      HANDLE              FileHandle,
                                    IN      DWORD               CertificateIndex,
                                    OUT     LPWIN_CERTIFICATE   Certificate,
                                    IN OUT  PDWORD              RequiredLength);

extern BOOL JavaAddCertificate(     IN      HANDLE              FileHandle,
                                    IN      LPWIN_CERTIFICATE   Certificate,
                                    OUT     PDWORD              Index);


#endif // SIPOBJJV_HXX
