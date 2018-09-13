//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       catdb.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  History:    23-Oct-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "catdb.hxx"

cCatalogDB_::cCatalogDB_(WCHAR *pwszBaseDirIn, WCHAR *pwszSubDirIn)
{
    WCHAR  wszPath[MAX_PATH];
    LPWSTR pwszPath;
    BOOL   fCreatedOK = FALSE;

    pCatMast    = NULL;

    pwszPath = new WCHAR [ wcslen(pwszBaseDirIn)+wcslen(pwszSubDirIn)+1 ];
    if ( pwszPath == NULL )
    {
        return;
    }

    wcscpy(pwszPath, pwszBaseDirIn);

    CreateDirectoryU(pwszPath, NULL);

    //
    //  sysmast is in the CatRoot directory.
    //
    if ((pSysMast = new cBFile_(&MSCAT_CriticalSection, 
                                pwszPath,
                                SYSMAST_NAME, 
                                sizeof(GUID), 
                                sizeof(SysMastRec), 
                                CATDB_VERSION_1, 
                                &fCreatedOK))               &&
        fCreatedOK                                          &&
        (pSysMast->Initialize()))
    {
        wcscat(pwszPath, pwszSubDirIn);

        // create the directory!
        CreateDirectoryU(pwszPath, NULL);

        if ((pCatMast = new cBFile_(&MSCAT_CriticalSection, 
                                    pwszPath,
                                    CATMAST_NAME, 
                                    sizeof(DWORD), 
                                    sizeof(CatMastRec), 
                                    CATDB_VERSION_1,
                                    &fCreatedOK))           &&
            fCreatedOK                                      &&
            (pCatMast->Initialize()))
        {
            pCatMast->UseRecNumAsKey(TRUE);
            DELETE_OBJECT(pwszPath);
            return;
        }
    }

    DELETE_OBJECT(pSysMast);
    DELETE_OBJECT(pCatMast);
    DELETE_OBJECT(pwszPath);
}

cCatalogDB_::~cCatalogDB_(void)
{
    DELETE_OBJECT(pSysMast);
    DELETE_OBJECT(pCatMast);
}

BOOL cCatalogDB_::Initialize(void)
{
    if ((pSysMast) &&
        (pCatMast))
    {
        return(TRUE);
    }

    return(FALSE);
}

ULONG cCatalogDB_::SysMast_GetNewId(void)
{
    return(0L); // handled in the add....
}

DWORD cCatalogDB_::SysMast_NumKeys(void)
{
    return(pSysMast->GetNumKeys());
}

BOOL cCatalogDB_::SysMast_Get(const GUID *pgSubSys, SysMast *psData)
{
    SysMastRec  *rec;

    memset(psData, 0x00, sizeof(SysMast));

    pSysMast->setKey((void *)pgSubSys);

    if (pSysMast->Find())
    {
        rec = (SysMastRec *)pSysMast->getData();

        MultiByteToWideChar(0, 0, rec->SubDir, -1, psData->SubDir, REG_MAX_GUID_TEXT);
        psData->SubDir[REG_MAX_GUID_TEXT - 1] = NULL;

        memcpy(&psData->SysGuid, &rec->SysGuid, sizeof(GUID));

        psData->SysId = rec->SysId;

        return(TRUE);
    }

    return(FALSE);
}

BOOL cCatalogDB_::SysMast_GetFirst(SysMast *psData)
{
    SysMastRec  *rec;

    memset(psData, 0x00, sizeof(SysMast));

    if (pSysMast->GetFirst())
    {
        rec = (SysMastRec *)pSysMast->getData();

        MultiByteToWideChar(0, 0, rec->SubDir, -1, psData->SubDir, REG_MAX_GUID_TEXT);
        psData->SubDir[REG_MAX_GUID_TEXT - 1] = NULL;

        memcpy((void *)&psData->SysGuid, &rec->SysGuid, sizeof(GUID));

        psData->SysId = rec->SysId;

        return(TRUE);
    }

    return(FALSE);
}

BOOL cCatalogDB_::SysMast_GetNext(SysMast *psData)
{
    SysMastRec  *rec;

    memset(psData, 0x00, sizeof(SysMast));

    if (pSysMast->GetNext())
    {
        rec = (SysMastRec *)pSysMast->getData();

        MultiByteToWideChar(0, 0, rec->SubDir, -1, psData->SubDir, REG_MAX_GUID_TEXT);
        psData->SubDir[REG_MAX_GUID_TEXT - 1] = NULL;

        memcpy((void *)&psData->SysGuid, &rec->SysGuid, sizeof(GUID));

        psData->SysId = rec->SysId;

        return(TRUE);
    }

    return(FALSE);
}

BOOL cCatalogDB_::SysMast_Add(SysMast *psData)
{
    SysMastRec  rec;

    memset(&rec, 0x00, sizeof(SysMastRec));

    WideCharToMultiByte(0, 0, psData->SubDir, -1, &rec.SubDir[0], REG_MAX_GUID_TEXT, NULL, NULL);
    rec.SubDir[REG_MAX_GUID_TEXT - 1] = NULL;

    memcpy(&rec.SysGuid, &psData->SysGuid, sizeof(GUID));

    rec.SysId = psData->SysId;

    pSysMast->setKey(&psData->SysGuid);
    pSysMast->setData(&rec);

    if (pSysMast->Add())
    {
        //
        // because the id is the record number, we need to add it first, get the rec
        // number, force an index update, then update the record.
        //
        psData->SysId = pSysMast->getRecNum();
        memcpy(&rec, pSysMast->getData(), sizeof(SysMastRec));

        rec.SysId = psData->SysId;

        pSysMast->Sort();       // force update of the index!

        pSysMast->setKey(&psData->SysGuid);
        pSysMast->setData(&rec);
        pSysMast->Update();

        return(TRUE);
    }

    return(FALSE);
}

ULONG cCatalogDB_::CatMast_GetNewId(void)
{
    return(0L); // handled in the add....
}

BOOL cCatalogDB_::CatMast_Get(DWORD ulId, CatMast *psData)
{
    CatMastRec  *rec;

    memset(psData, 0x00, sizeof(CatMast));

    pCatMast->setKey(&ulId);

    if (pCatMast->Find())
    {
        rec = (CatMastRec *)pCatMast->getData();

        MultiByteToWideChar(0, 0, rec->OrigName, -1, psData->OrigName, MAX_PATH);
        psData->OrigName[MAX_PATH - 1] = NULL;

        MultiByteToWideChar(0, 0, rec->CurName, -1, psData->CurName, MAX_PATH);
        psData->CurName[MAX_PATH - 1] = NULL;

        psData->CatId = rec->CatId;
        psData->SysId = rec->SysId;

        memcpy(&psData->InstDate, &rec->InstDate, sizeof(FILETIME));

        return(TRUE);
    }

    return(FALSE);
}

BOOL cCatalogDB_::CatMast_Add(CatMast *psData)
{
    CatMastRec  rec;

    memset(&rec, 0x00, sizeof(CatMastRec));

    WideCharToMultiByte(0, 0, psData->OrigName, -1, &rec.OrigName[0], MAX_PATH, NULL, NULL);
    rec.OrigName[MAX_PATH - 1] = NULL;

    WideCharToMultiByte(0, 0, psData->CurName, -1, &rec.CurName[0], MAX_PATH, NULL, NULL);
    rec.CurName[MAX_PATH - 1] = NULL;

    rec.SysId = psData->SysId;

    pCatMast->setData(&rec);

    if (pCatMast->Add())
    {
        memcpy(&rec, pCatMast->getData(), sizeof(CatMastRec));

        psData->CatId   = pCatMast->getRecNum();
        rec.CatId       = psData->CatId;

        pCatMast->Sort();               // force index update.

        if (!(rec.CurName[0]))
        {
            sprintf(&rec.CurName[0], "%08.8lX.CAT", rec.CatId);

            MultiByteToWideChar(0, 0, &rec.CurName[0], -1, psData->CurName, MAX_PATH);
            psData->CurName[MAX_PATH - 1] = NULL;

        }

        pCatMast->setKey(&psData->CatId);
        pCatMast->setData(&rec);
        pCatMast->Update();

        return(TRUE);
    }

    return(FALSE);
}

cHashDB_::cHashDB_(WCHAR *pwszBase, WCHAR *pwszSubSysDirIn, BOOL *pfCreatedOK)
{
    *pfCreatedOK = TRUE;

    pwszSubSysDir = new WCHAR[wcslen(pwszBase) + wcslen(pwszSubSysDirIn) + 2];

    if (pwszSubSysDir)
    {
        BOOL fCreatedOK;

        wcscpy(pwszSubSysDir, pwszBase);
        wcscat(pwszSubSysDir, pwszSubSysDirIn);

        pHashMast   = new cBFile_(&MSCAT_CriticalSection, pwszSubSysDir,
                              HASHMAST_NAME, MAX_HASH_LEN, sizeof(HashMastRec), CATDB_VERSION_1, pfCreatedOK);

        if (pHashMast == NULL)
        {
            *pfCreatedOK = FALSE;
        }
    }
    else
    {
        *pfCreatedOK = FALSE;
    }
}

cHashDB_::~cHashDB_(void)
{
    DELETE_OBJECT(pHashMast);
    DELETE_OBJECT(pwszSubSysDir);
}

BOOL cHashDB_::Initialize(void)
{
    if (pHashMast)
    {
        return(TRUE);
    }

    return(FALSE);
}


BOOL cHashDB_::HashMast_Get(DWORD dwLastRec, BYTE *pbHash, DWORD cbHash, HashMast *psData)
{
    HashMastRec     *rec;
    BYTE            bHash[MAX_HASH_LEN];
    BOOL            fRet;

    if (cbHash > MAX_HASH_LEN)
    {
        return(FALSE);
    }

    memset(&bHash[0], 0x00, MAX_HASH_LEN);
    memcpy(&bHash[0], pbHash, cbHash);

    memset(psData, 0x00, sizeof(HashMast));

    pHashMast->setKey(&bHash[0]);

    if (dwLastRec == 0xFFFFFFFF)
    {
        fRet = pHashMast->Find();
    }
    else
    {
        fRet = pHashMast->GetNext(dwLastRec);
    }

    if (fRet)
    {
        if (memcmp(&bHash[0], pHashMast->getKey(), MAX_HASH_LEN) != 0)
        {
            return(FALSE);
        }

        rec = (HashMastRec *)pHashMast->getData();

        MultiByteToWideChar(0, 0, rec->CatName, -1, psData->CatName, MAX_PATH);
        psData->CatName[MAX_PATH - 1] = NULL;

        return(TRUE);
    }

    return(FALSE);
}

BOOL cHashDB_::HashMast_Add(HashMast *psData)
{
    HashMastRec  rec;

    memset(&rec, 0x00, sizeof(HashMastRec));

    WideCharToMultiByte(0, 0, psData->CatName, -1, &rec.CatName[0], MAX_PATH, NULL, NULL);
    rec.CatName[MAX_PATH - 1] = NULL;

    pHashMast->setKey(&psData->Hash[0]);
    pHashMast->setData(&rec);

    if (pHashMast->Add())
    {
        return(TRUE);
    }

    return(FALSE);
}


