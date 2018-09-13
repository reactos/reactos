//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjCT.hxx (Catalog)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    24-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef SIPOBJCT_HXX
#define SIPOBJCT_HXX

#include    "sipobj.hxx"

class SIPObjectCatalog_ : public SIPObject_
{
    public:
        SIPObjectCatalog_(DWORD id);
        virtual ~SIPObjectCatalog_(void) { ; }

        BOOL    GetSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx,
                                 DWORD *pdwDLen,BYTE *pbData,
                                 DWORD *pdwEncodeType);

        BOOL    PutSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD *dwIdx,
                                 DWORD dwDLen,BYTE *pbData,
                                 DWORD dwEncodeType);

        BOOL    RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx) { return(FALSE); }
        BOOL    CreateIndirectData(SIP_SUBJECTINFO *pSI,DWORD *pdwDLen,
                                   SIP_INDIRECT_DATA *psData);
        BOOL    VerifyIndirectData(SIP_SUBJECTINFO *pSI,
                                   SIP_INDIRECT_DATA *psData) 
                                    { return(TRUE); }
};


#endif // SIPOBJCT_HXX
