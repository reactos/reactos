//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjFL.hxx    (Flat)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    15-Feb-1997 pberkman   created
//
//------------------------------------------------------------------

#ifndef SIPOBJFL_HXX
#define SIPOBJFL_HXX

#include    "sipobj.hxx"

class SIPObjectFlat_ : public SIPObject_
{
    public:
        SIPObjectFlat_(DWORD id);
        virtual ~SIPObjectFlat_(void) { ; }

        BOOL    GetSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx,
                                 DWORD *pdwDLen,BYTE *pbData,
                                 DWORD *pdwEncodeType);

        BOOL    PutSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD *dwIdx,
                                 DWORD dwDLen,BYTE *pbData,
                                 DWORD pdwEncodeType)
                        { return(FALSE); }

        BOOL    RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx)
                        { return(FALSE); }

        BOOL    VerifyIndirectData(SIP_SUBJECTINFO *pSI,
                                   SIP_INDIRECT_DATA *psData);

        virtual char    *GetDataObjectID(void)  { return(SPC_CAB_DATA_OBJID); }
        virtual char    *GetDataOIDHint(void)   { return((char *)SPC_CAB_DATA_STRUCT); }



    protected:
        void    *GetMyStructure(SIP_SUBJECTINFO *pSI) { return(&SpcLink); }
        DWORD   GetMyStructureSize(void) { return(sizeof(SPC_LINK)); }

        BOOL    PutMessageInFile(SIP_SUBJECTINFO *pSI, 
                                 WIN_CERTIFICATE *pWinCert,
                                 DWORD *pdwIndex)
                        { return(TRUE); }

        BOOL    GetMessageFromFile(SIP_SUBJECTINFO *pSI, 
                                   WIN_CERTIFICATE *pWinCert,
                                   DWORD dwIndex,DWORD *pcbCert)
                        { return(TRUE); }

        BOOL    GetDigestStream(DIGEST_DATA *pDigestData, 
                                DIGEST_FUNCTION pfnCallBack, DWORD dwFlags);


    private:
        SPC_LINK                SpcLink;

};


#endif // SIPOBJFL_HXX
