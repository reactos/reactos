//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sipobjcb.hxx    (CAB)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    14-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef SIPOBJCB_HXX
#define SIPOBJCB_HXX

#include    "sipobj.hxx"

extern "C"
{
#   include    "cabinet.h"
};

#define RESERVE_LEN_ALIGN(Len)  ((Len + 3) & ~3)        // all abReserve is alligned @ 4

#define RESERVE_CNT_HDR_LEN     (sizeof(USHORT) * 2)    // cbJund & cbSig

#define RESERVE_CAB_FLAG        0x00000001
#define VERIFY_CAB_FLAG         0x00000002

typedef struct _CAB_HDR_PARA 
{
    CFHEADER            cfheader;
    CFRESERVE           cfres;
    USHORT              cbcfres;        // 0 or sizeof(CFRESERVE)
    BYTE                *pbReserve;
    BYTE                *pbStrings;
    DWORD               cbStrings;
    USHORT              cbJunk;
    USHORT              cbSig;
    CABSignatureStruct_ *pCabSigStruct;
    DWORD               cbTotalHdr;
} CAB_HDR_PARA;

typedef struct _CAB_PARA 
{
    DWORD               dwFlags;
    CAB_HDR_PARA        Hdr;
} CAB_PARA, *PCAB_PARA;

class SIPObjectCAB_ : public SIPObject_
{
    public:
        SIPObjectCAB_(DWORD id);
        virtual ~SIPObjectCAB_(void);

        BOOL            CreateIndirectData(SIP_SUBJECTINFO *pSI,DWORD *pdwDLen,
                                        SIP_INDIRECT_DATA *psData);
        virtual BOOL    RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx);

        virtual char    *GetDataObjectID(void)  { return(SPC_CAB_DATA_OBJID); }
        virtual char    *GetDataOIDHint(void)   { return((char *)SPC_CAB_DATA_STRUCT); }

    protected:
        virtual BOOL    PutMessageInFile(SIP_SUBJECTINFO *pSI,WIN_CERTIFICATE *pWinCert,
                                         DWORD *pdwIndex);
        virtual BOOL    GetMessageFromFile(SIP_SUBJECTINFO *pSI,WIN_CERTIFICATE *pWinCert,
                                           DWORD dwIndex,DWORD *pcbCert);
        virtual BOOL    GetDigestStream(DIGEST_DATA *pDigestData, 
                                        DIGEST_FUNCTION pfnCallBack, DWORD dwFlags);

    private:
        CAB_PARA        Para;
        BOOL            fUseV1Sig;

        BOOL            ReadHeader(void);
        BOOL            ReadSignedData(BYTE *pbRet);
        BOOL            WriteSignedData(BYTE *pbSig, DWORD cbSig);
        BOOL            WriteSignedDataV1(BYTE *pbSignedData, DWORD cbSignedData);

        BOOL            WriteHeader(void);
        void            FreeHeader(void);
        BOOL            RemoveCertificate(DWORD Index);
        BOOL            ShiftFileBytes(LONG lbShift);   // this may be needed in SIPObject_ (base)
        BOOL            ReserveSignedData(DWORD cbSignedData);
        BOOL            DigestHeader(DIGEST_FUNCTION pfnDigestData, DIGEST_HANDLE hDigestData);
        void            ChecksumHeader(void);
};


#endif // SIPOBJCB_HXX
