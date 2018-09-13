//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cBFile.cpp
//
//  Contents:   Microsoft Internet Security Certificate Class
//
//  History:    24-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef CBFILE_HXX
#define CBFILE_HXX

#pragma pack(8)

#define BFILE_VERSION_1             0x0001

typedef struct BFILE_HEADER_
{
    DWORD       fDirty      : 1;
    DWORD       sVersion    : 16;
    DWORD       dwFiller1   : 15;
    DWORD       sIntVersion : 16;
    DWORD       dwFiller2   : 16;
    DWORD       cbSortedEOF;
    DWORD       cbKey;
    DWORD       cbData;

    DWORD       dwLastRecNum;

    DWORD       dwFiller3;

} BFILE_HEADER;

typedef struct BFILE_RECORD_
{
    DWORD       dwRecNum;
    DWORD       cbKey;
    void        *pvKey;
    DWORD       cbDataOffset;
    DWORD       cbData;
    void        *pvData;

    DWORD       fDeleted    : 1;
    DWORD       dwFiller1   : 31;

} BFILE_RECORD;

#ifdef _WIN64
    #define     BFILE_SIZEOFSIG     8
#else
    #define     BFILE_SIZEOFSIG     6
#endif

#define     BFILE_SIG           "CBFILE"
#define     BFILE_KEYEXT        L".cbk"
#define     BFILE_DATAEXT       L".cbd"

#define     BFILE_HEADERSIZE    (sizeof(BFILE_HEADER) + BFILE_SIZEOFSIG)
#define     BFILE_KEYSIZE       (sHeader.cbKey + sizeof(DWORD))
#define     BFILE_RECSIZE       (sizeof(DWORD) + sHeader.cbData)

#pragma pack()

class cBFile_
{
    public:
        cBFile_(CRITICAL_SECTION *pCriticalSection,
                WCHAR *pwszBFilePath, WCHAR *pwszBFileBaseName,
                DWORD cbKey, DWORD cbData, SHORT sVersion, BOOL *pfCreatedOK);
        virtual ~cBFile_(void);

        BOOL                Initialize(void);

        void                setKey(void *pvInKey);
        void                setData(void *pvInData);

        DWORD               KeySize(void) { return(sHeader.cbKey); }

        void                *getKey(void)       { return(sRecord.pvKey); }
        void                *getData(void)      { return(sRecord.pvData); }

        DWORD               getRecNum(void)     { return(sRecord.dwRecNum); }
        DWORD               getKeyNum(void)     { return(dwFirstNextRecNum); }

        DWORD               getVersion(void)    { return(sHeader.sVersion); }

        void                UseRecNumAsKey(BOOL fUse) { fUseRecNumAsKey = fUse; }

        BOOL                Find(void);
        BOOL                Update(void);
        BOOL                Add(void);
        void                Sort(void);

        BOOL                GetFirst(void);
        BOOL                GetNext(DWORD dwCurRec = 0xffffffff);
        BOOL                GetPrev(DWORD dwCurRec = 0xffffffff);

        DWORD               GetNumKeys(void);

        BOOL                GetHeader(BFILE_HEADER *psHeader);
        void                *GetDumpKey(DWORD dwIdx, void *pvRetKey, DWORD *pdwRecOffset);

    protected:
        BOOL                BinaryFind(DWORD *pcbDataOffset);
        BOOL                GetDataRecord(DWORD cbDataOffset);
        void                UpdateDataRecord(DWORD cbDataOffset);
        void                AddDirtyKey(void);
        DWORD               AddDataRecord(void);
        BOOL                UpdateHeader(void);
        void                ReadHeader(void);

    private:
        CRITICAL_SECTION    *pCritical;
        BOOL                fInitialized;
        BOOL                fReadOnly;
        BOOL                fDirty;
        BOOL                fUseRecNumAsKey;
        BFILE_HEADER        sHeader;
        BFILE_RECORD        sRecord;
        HANDLE              hKFile;
        HANDLE              hDFile;
        BYTE                *pbKMap;
        DWORD               cbKMap;
        BYTE                *pbDMap;
        DWORD               cbDMap;
        HANDLE              hDMutex;
        WCHAR               *pwszPath;
        WCHAR               *pwszBaseName;
        DWORD               dwFirstNextRecNum;

        BOOL                OpenFiles(void);
        BOOL                Lock(void);
        BOOL                Unlock(void);
        BOOL                RemapKey(void);
        BOOL                RemapData(void);
        void                UnmapAll(void);
        void                SpeedSort(void);
        DWORD               GetInsertionPoint(void *pvIn);
};

#endif // CBFILE_HXX

