//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObjSS.hxx
//
//  Contents:   Microsoft SIP Provider - Structured Storage
//
//  History:    07-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef SIPOBJSS_HXX
#define SIPOBJSS_HXX

#include    "sipobj.hxx"

class SIPObjectSS_ : public SIPObject_
{
    public:
        SIPObjectSS_(DWORD id);
        virtual ~SIPObjectSS_(void);

        BOOL            RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx);

        char            *GetDataObjectID(void) { return(SPC_SIGINFO_OBJID); }
        char            *GetDataOIDHint(void)  { return((char *)SPC_SIGINFO_STRUCT); }

    protected:
        void    *GetMyStructure(SIP_SUBJECTINFO *pSI) { return(&SpcSigInfo); }
        DWORD   GetMyStructureSize(void) { return(sizeof(SPC_SIGINFO)); }

        BOOL    PutMessageInFile(SIP_SUBJECTINFO *pSI,
                                 WIN_CERTIFICATE *pWinCert,DWORD *pdwIndex);
        BOOL    GetMessageFromFile(SIP_SUBJECTINFO *pSI,
                                   WIN_CERTIFICATE *pWinCert,DWORD dwIndex,
                                   DWORD *pcbCert);
        BOOL    GetDigestStream(DIGEST_DATA *pDigestData, 
                                DIGEST_FUNCTION pfnCallBack, DWORD dwFlags);

        BOOL    FileHandleFromSubject(SIP_SUBJECTINFO *pSubject,
                                      DWORD dwAccess = GENERIC_READ,
                                      DWORD dwShared = FILE_SHARE_READ);

    private:
        IStorage    *pTopStg;
        SPC_SIGINFO SpcSigInfo;

        BOOL        IStorageDigest(IStorage *pStg, DIGEST_DATA *pDigestData, DIGEST_FUNCTION pfnCallBack);
        void        FreeElements(DWORD *pcStg, STATSTG **ppStg);
        BOOL        SortElements(IStorage *pStg, DWORD *pcSortStg, STATSTG **ppSortStg);
};

#endif // SIPOBJSS_HXX
