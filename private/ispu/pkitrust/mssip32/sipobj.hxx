//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       SIPObj.hxx  (base)
//
//  Contents:   Microsoft SIP Provider
//
//  History:    15-Feb-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef SIPOBJ_HXX
#define SIPOBJ_HXX

#include    "global.hxx"

class SIPObject_
{
    public:
        SIPObject_(DWORD id);
        virtual ~SIPObject_(void);

        virtual BOOL    GetSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx,
                                         DWORD *pdwDLen,BYTE *pbData,
                                         DWORD *pdwEncodeType);
        virtual BOOL    PutSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD *dwIdx,
                                         DWORD dwDLen,BYTE *pbData,
                                         DWORD dwEncodeType);
        virtual BOOL    RemoveSignedDataMsg(SIP_SUBJECTINFO *pSI,DWORD dwIdx) { return(FALSE); }
        virtual BOOL    CreateIndirectData(SIP_SUBJECTINFO *pSI,DWORD *pdwDLen,
                                           SIP_INDIRECT_DATA *psData);
        virtual BOOL    VerifyIndirectData(SIP_SUBJECTINFO *pSI,
                                           SIP_INDIRECT_DATA *psData);

        virtual char    *GetDataObjectID(void) { return(NULL); }
        virtual char    *GetDataOIDHint(void) { return(NULL); }

        void            set_CertVersion(DWORD dwNewCertVersion);
        DWORD           get_CertVersion(void) { return(uCertVersion); }


    protected:
        HANDLE          hFile;
        BOOL            fUseFileMap;
        BYTE            *pbFileMap;
        DWORD           cbFileMap;
        HANDLE          hMappedFile;
        DWORD           dwFileAccess;
        HCRYPTPROV      hProv;
        BOOL            bCloseFile;
        BOOL            fSizeFileOnly;
        UINT            uSubjectForm;
        DWORD           uCertVersion;
                        
        void            *SIPNew(DWORD cbytes);

        virtual DWORD   GetDigestFlags(SIP_SUBJECTINFO *pSI) { return(0); }

        virtual void    *GetMyStructure(SIP_SUBJECTINFO *pSI) { return(NULL); }
        virtual DWORD   GetMyStructureSize(void) { return(0); }

        virtual BOOL    PutMessageInFile(SIP_SUBJECTINFO *pSI,
                                         WIN_CERTIFICATE *pWinCert,DWORD *pdwIndex)
                                        { return(FALSE); }

        virtual BOOL    GetMessageFromFile(SIP_SUBJECTINFO *pSI,
                                           WIN_CERTIFICATE *pWinCert,DWORD dwIndex,
                                           DWORD *pcbCert)
                                        { return(FALSE); }

        virtual BOOL    GetDigestStream(DIGEST_DATA *pDigestData, 
                                        DIGEST_FUNCTION pfnCallBack, DWORD dwFlags)
                                        { return(FALSE); }

        virtual BOOL    FileHandleFromSubject(SIP_SUBJECTINFO *pSubject,
                                              DWORD dwAccess = GENERIC_READ,
                                              DWORD dwShared = FILE_SHARE_READ);

        BYTE            *DigestFile(HCRYPTPROV hProv, DWORD dwFlags,    // allocates digest
                                    char *pszAlgObjId, DWORD *cbDigest); // delete it after use!!!
        BOOL            OpenFile(LPCWSTR wFileName, DWORD dwAccess, DWORD dwShared);
        void            AllocateAndFillCryptBitBlob(CRYPT_BIT_BLOB *bb,DWORD Flags,
                                                    DWORD cUnusedBits);
        void            DestroyCryptBitBlob(CRYPT_BIT_BLOB *bb);
        DWORD           CryptBitBlobToFlags(CRYPT_BIT_BLOB *bb);
        BOOL            LoadDefaultProvider(void);
        BOOL            SeekAndReadFile(DWORD lFileOffset, BYTE *pb, DWORD cb);
        BOOL            SeekAndWriteFile(DWORD lFileOffset,BYTE *pb, DWORD cb);
        BOOL            MapFile(void);
        BOOL            UnmapFile(void);
};


#endif // SIPOBJ_HXX
