//-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       catdb.hxx
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  History:    23-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#ifndef CATDB_HXX
#define CATDB_HXX

#include    "cbfile.hxx"

#pragma pack(8)

//
//  the following are global tracking files (eg: only one in the "CatRoot")
//
#define     SYSMAST_NAME                L"SYSMAST"
typedef struct SysMast_
{
    DWORD           SysId;   
    GUID            SysGuid;                // GuidIdx
    WCHAR           SubDir[REG_MAX_GUID_TEXT];

} SysMast;

typedef struct SysMastRec_
{
    DWORD           SysId;   
    GUID            SysGuid;                // GuidIdx
    char            SubDir[REG_MAX_GUID_TEXT];

} SysMastRec;

//
//  the following are "localized" tracking files.  eg: they live in each
//  sub-systems directory.
//
#define     CATMAST_NAME                L"CATMAST"
typedef struct CatMast_
{
    DWORD           CatId;                  // CatIdIdx
    DWORD           SysId;
    WCHAR           OrigName[MAX_PATH];
    WCHAR           CurName[MAX_PATH];         
    FILETIME        InstDate;

} CatMast;

typedef struct CatMastRec_
{
    DWORD           CatId;                  // CatIdIdx
    DWORD           SysId;
    char            OrigName[MAX_PATH];
    char            CurName[MAX_PATH];
    FILETIME        InstDate;

} CatMastRec;

#define     HASHMAST_NAME               L"HASHMAST"

#define     MAX_HASH_LEN                20  // SHA1

typedef struct HashMast_
{
    BYTE            Hash[MAX_HASH_LEN];     // HashIdx
    DWORD           SysId;
    WCHAR           CatName[MAX_PATH];

} HashMast;

typedef struct HashMastRec_
{
    BYTE            Hash[MAX_HASH_LEN];     // HashIdx
    DWORD           SysId;
    char            CatName[MAX_PATH];

} HashMastRec;

#pragma pack()

#define     CATDB_VERSION_1             0x0001

class cCatalogDB_
{
    public:
        cCatalogDB_(WCHAR *pwszBaseDirIn, WCHAR *pwszSubSysDirIn);
        virtual ~cCatalogDB_(void);

        BOOL        Initialize(void);

        DWORD       SysMast_GetNewId(void);
        BOOL        SysMast_Get(const GUID *pgSubSys, SysMast *psData);
        BOOL        SysMast_Add(SysMast *psData);
        BOOL        SysMast_GetFirst(SysMast *psData);
        BOOL        SysMast_GetNext(SysMast *psData);
        DWORD       SysMast_NumKeys(void);


        DWORD       CatMast_GetNewId(void);
        BOOL        CatMast_Get(DWORD ulId, CatMast *psData);
        BOOL        CatMast_Add(CatMast *psData);

    private:
        cBFile_     *pSysMast;
        cBFile_     *pCatMast;
};

class cHashDB_
{
    public:
        WCHAR   *pwszSubSysDir;

        cHashDB_(WCHAR *pwszBaseDirIn, WCHAR *pwszSubSysDirIn, BOOL *pfCreatedOK);
        virtual ~cHashDB_(void);

        BOOL        Initialize(void);

        BOOL        HashMast_Get(DWORD dwStartRec, BYTE *pbHash, DWORD cbHash, HashMast *psData);
        BOOL        HashMast_Add(HashMast *psData);

        DWORD       HashMast_GetRecNum(void) { return(pHashMast->getRecNum()); }

        DWORD       HashMast_GetKeyNum(void) { return(pHashMast->getKeyNum()); }

    private:
        cBFile_     *pHashMast;

};

#endif // CATDB_HXX