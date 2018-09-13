//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       catadmin.cpp
//
//  Contents:   Microsoft Internet Security Catalog Utilities
//
//  Functions:  CryptCATAdminAcquireContext
//              CryptCATAdminReleaseContext
//              CryptCATAdminCalcHashFromFileHandle
//              CryptCATAdminEnumCatalogFromHash
//              CryptCATAdminAddCatalog
//              CryptCATAdminRemoveCatalog
//              CatAdminDllMain
//
//              *** local functions ***
//              _LoadHashDB
//              _UpdateHashIndex
//              SetupDefaults
//              CleanupDefaults
//              _AllocateInfoContext
//              _DeallocateInfoContext
//
//  History:    28-Aug-1997 pberkman   created
//
//--------------------------------------------------------------------------

#include    "global.hxx"
#include    "cryptreg.h"
#include    "wintrust.h"
#include    "softpub.h"
#include    "eventlst.h"
#include    "sipguids.h"
#include    "mscat32.h"
#include    "catdb.hxx"

typedef struct CATALOG_INFO_CONTEXT_
{
    DWORD                       cbStruct;

    WCHAR                       *pwszCatalogFile;

    DWORD_PTR                   dwEnumReserved; // used internally by enum.

} CATALOG_INFO_CONTEXT;


#define INVALID_CAT_ID                  0xffffffff

//
//  default system guid for apps that just make calls to CryptCATAdminAddCatalog with
//  hCatAdmin == NULL...
//
//          {127D0A1D-4EF2-11d1-8608-00C04FC295EE}
//
#define DEF_SUBSYS_ID                                                   \
                {                                                       \
                    0x127d0a1d,                                         \
                    0x4ef2,                                             \
                    0x11d1,                                             \
                    { 0x86, 0x8, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee }    \
                }

#define REG_CATALOG_BASE_DIRECTORY      L"CatRoot"

static WCHAR        *pwszCatalogBaseDirectory = NULL;

typedef struct CRYPT_CAT_ADMIN_
{
    DWORD           cbStruct;

    cCatalogDB_     *pDB;

    DWORD           cHashDB;
    cHashDB_        **paHashDB;

    BYTE            *pbCalculatedHash;

    BOOL            fConnected;

    BOOL            fUseDefSubSysId;

    DWORD           dwCurrentSysId;
    DWORD           dwCurrentCatId; // only filled out after the add or find functions

    WCHAR           *pwszSubSysDir; // entire directory to subsystems area

    WCHAR           *pwszCurCatName;
    WCHAR           *pwszOrigCatName;

    DWORD           dwLastDBError;

} CRYPT_CAT_ADMIN;

typedef struct ENUM_HASH_IDX_
{
    DWORD   dwDBIdx;
    DWORD   dwRecNum;

} ENUM_HASH_IDX;


BOOL _LoadHashDB(CRYPT_CAT_ADMIN *psCatAdmin);
BOOL _UpdateHashIndex(CRYPT_CAT_ADMIN *psCatAdmin, WCHAR *pwszCatName);
CATALOG_INFO_CONTEXT *_AllocateInfoContext(void);
void _DeallocateInfoContext(CATALOG_INFO_CONTEXT **psInfo);

static void SetupDefaults(void);
static void CleanupDefaults(void);


//////////////////////////////////////////////////////////////////////////
//
//  Exported functions:
//

BOOL WINAPI CryptCATAdminAcquireContext(HCATADMIN *phCatAdmin, const GUID *pgSubsystem, DWORD dwFlags)
{
    GUID                gDefault = DEF_SUBSYS_ID;
    CRYPT_CAT_ADMIN     *psCatAdmin;
    BOOL                fRet;

    fRet        = TRUE;

    psCatAdmin  = NULL;

    if (!(phCatAdmin))
    {
        goto InvalidParam;
    }

    *phCatAdmin = NULL;

    if (!(psCatAdmin = new CRYPT_CAT_ADMIN))
    {
        goto MemoryError;
    }

    memset(psCatAdmin, 0x00, sizeof(CRYPT_CAT_ADMIN));

    psCatAdmin->cbStruct        = sizeof(CRYPT_CAT_ADMIN);
    psCatAdmin->dwCurrentCatId  = INVALID_CAT_ID;

    *phCatAdmin = (HCATADMIN)psCatAdmin;

    if (!(pgSubsystem))
    {
        psCatAdmin->fUseDefSubSysId     = TRUE;
        pgSubsystem                     = &gDefault;
    }

    SysMast     sSysMast;

    memset(&sSysMast, 0x00, sizeof(SysMast));

    guid2wstr(pgSubsystem, &sSysMast.SubDir[0]);

    if (!(psCatAdmin->pDB = new cCatalogDB_(pwszCatalogBaseDirectory, &sSysMast.SubDir[0])))
    {
        goto DBErrorAlloc;
    }

    if (!(psCatAdmin->pDB->Initialize()))
    {
        goto DBErrorInit;
    }

    DELETE_OBJECT(psCatAdmin->pwszSubSysDir);

    psCatAdmin->pwszSubSysDir = new WCHAR[wcslen(pwszCatalogBaseDirectory) + wcslen(&sSysMast.SubDir[0]) + 2];

    if (!(psCatAdmin->pwszSubSysDir))
    {
        goto MemoryError;
    }

    wcscpy(psCatAdmin->pwszSubSysDir, pwszCatalogBaseDirectory);
    wcscat(psCatAdmin->pwszSubSysDir, &sSysMast.SubDir[0]);
    wcscat(psCatAdmin->pwszSubSysDir, L"\\");

    //
    //  get the current SysId
    //
    if (!(psCatAdmin->pDB->SysMast_Get(pgSubsystem, &sSysMast)))
    {
        sSysMast.SysId = psCatAdmin->pDB->SysMast_GetNewId();
        memcpy(&sSysMast.SysGuid, pgSubsystem, sizeof(GUID));
        guid2wstr(pgSubsystem, &sSysMast.SubDir[0]);

        if (!(psCatAdmin->pDB->SysMast_Add(&sSysMast)))
        {
            goto DBError;
        }
    }

    DWORD   dwCnt;

    if (!(psCatAdmin->fUseDefSubSysId))
    {
        psCatAdmin->paHashDB = new cHashDB_ *[1];
        dwCnt = 1;
    }
    else
    {
        dwCnt = psCatAdmin->pDB->SysMast_NumKeys();

        psCatAdmin->paHashDB = new cHashDB_ *[dwCnt];
    }

    if (!(psCatAdmin->paHashDB))
    {
        goto MemoryError;
    }

    psCatAdmin->cHashDB = dwCnt;

    memset(psCatAdmin->paHashDB, NULL, sizeof(cHashDB_ *) * dwCnt);

    psCatAdmin->dwCurrentSysId = (DWORD)sSysMast.SysId;

    CommonReturn:
        return(fRet);

    ErrorReturn:
        if (phCatAdmin)
        {
            DWORD   lErr;

            lErr = GetLastError();
            CryptCATAdminReleaseContext((HCATADMIN)psCatAdmin, 0);
            SetLastError(lErr);

            psCatAdmin  = NULL;
            *phCatAdmin = NULL;
        }

        fRet = FALSE;
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,      ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, DBErrorInit,       ERROR_DATABASE_FAILURE);
    SET_ERROR_VAR_EX(DBG_SS, DBErrorAlloc,      ERROR_DATABASE_FAILURE);
    SET_ERROR_VAR_EX(DBG_SS, DBError,           ERROR_DATABASE_FAILURE);
}

BOOL WINAPI CryptCATAdminReleaseContext(HCATADMIN hCatAdmin, DWORD dwFlags)
{
    CRYPT_CAT_ADMIN *psCatAdmin;
    DWORD           i;

    psCatAdmin = (CRYPT_CAT_ADMIN *)hCatAdmin;

    if ((psCatAdmin) && (psCatAdmin->cbStruct == sizeof(CRYPT_CAT_ADMIN)))
    {
        DELETE_OBJECT(psCatAdmin->pwszSubSysDir);
        DELETE_OBJECT(psCatAdmin->pwszCurCatName);
        DELETE_OBJECT(psCatAdmin->pwszOrigCatName);

        for (i = 0; i < psCatAdmin->cHashDB; i++)
        {
            DELETE_OBJECT(psCatAdmin->paHashDB[i]);
        }
        DELETE_OBJECT(psCatAdmin->paHashDB);

        DELETE_OBJECT(psCatAdmin->pDB);

        DELETE_OBJECT(psCatAdmin->pbCalculatedHash);

        DELETE_OBJECT(psCatAdmin);

        if (dwFlags != 0)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
        }

        return(TRUE);
    }

    SetLastError(ERROR_INVALID_PARAMETER);

    return(FALSE);
}

BOOL WINAPI CryptCATAdminCalcHashFromFileHandle(HANDLE hFile, DWORD *pcbHash, BYTE *pbHash,
                                                DWORD dwFlags)
{
    GUID                gFlat = CRYPT_SUBJTYPE_FLAT_IMAGE;
    BYTE                *pbRet;
    SIP_INDIRECT_DATA   *pbIndirectData;
    BOOL                fRet;

    pbIndirectData  = NULL;
    pbRet           = NULL;

    if (!(hFile) ||
        (hFile == INVALID_HANDLE_VALUE) ||
        !(pcbHash) ||
        (dwFlags != 0))
    {
        goto InvalidParam;
    }

    GUID                gSubject;
    SIP_DISPATCH_INFO   sSip;

    if (!(CryptSIPRetrieveSubjectGuid(L"CATADMIN", hFile, &gSubject)))
    {
        memcpy(&gSubject, &gFlat, sizeof(GUID));
    }

    memset(&sSip, 0x00, sizeof(SIP_DISPATCH_INFO));

    sSip.cbSize = sizeof(SIP_DISPATCH_INFO);

    if (!(CryptSIPLoad(&gSubject, 0, &sSip)))
    {
        goto SIPLoadError;
    }

    SIP_SUBJECTINFO     sSubjInfo;
    DWORD               cbIndirectData;

    memset(&sSubjInfo, 0x00, sizeof(SIP_SUBJECTINFO));
    sSubjInfo.cbSize                    = sizeof(SIP_SUBJECTINFO);

    sSubjInfo.DigestAlgorithm.pszObjId  = (char *)CertAlgIdToOID(CALG_SHA1);

    sSubjInfo.dwFlags                   = SPC_INC_PE_RESOURCES_FLAG | SPC_INC_PE_IMPORT_ADDR_TABLE_FLAG |
                                          MSSIP_FLAGS_PROHIBIT_RESIZE_ON_CREATE;
    sSubjInfo.pgSubjectType             = &gSubject;
    sSubjInfo.hFile                     = hFile;
    sSubjInfo.pwsFileName               = L"CATADMIN";

    sSubjInfo.dwEncodingType            = PKCS_7_ASN_ENCODING | X509_ASN_ENCODING;

    cbIndirectData = 0;

    sSip.pfCreate(&sSubjInfo, &cbIndirectData, NULL);

    if (cbIndirectData == 0)
    {
        SetLastError( E_NOTIMPL );
        goto SIPError;
    }

    if (!(pbIndirectData = (SIP_INDIRECT_DATA   *)new BYTE[cbIndirectData]))
    {
        goto MemoryError;
    }


    if (!(sSip.pfCreate(&sSubjInfo, &cbIndirectData, pbIndirectData)))
    {
        if ( GetLastError() == 0 )
        {
            SetLastError( ERROR_INVALID_DATA );
        }
        goto SIPError;
    }

    if ((pbIndirectData->Digest.cbData == 0) || (pbIndirectData->Digest.cbData > MAX_HASH_LEN))
    {
        SetLastError( ERROR_INVALID_DATA );
        goto SIPError;
    }

    if (!(pbRet = new BYTE[pbIndirectData->Digest.cbData]))
    {
        goto MemoryError;
    }

    memcpy(pbRet, pbIndirectData->Digest.pbData, pbIndirectData->Digest.cbData);

    fRet = TRUE;

    CommonReturn:
        if (pbRet)
        {
            if (*pcbHash < pbIndirectData->Digest.cbData)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                fRet = FALSE;
            }
            else if (pbHash)
            {
                memcpy(pbHash, pbRet, pbIndirectData->Digest.cbData);
            }

            *pcbHash = pbIndirectData->Digest.cbData;

            delete pbRet;
        }

        if (pbIndirectData)
        {
            delete pbIndirectData;
        }

        if ((GetLastError() == ERROR_INSUFFICIENT_BUFFER) && !(pbHash))
        {
            fRet = TRUE;
        }

        return(fRet);

    ErrorReturn:
        DELETE_OBJECT(pbRet);
        fRet = FALSE;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, SIPLoadError);
    TRACE_ERROR_EX(DBG_SS, SIPError);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,      ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
}

BOOL WINAPI CryptCATAdminReleaseCatalogContext(HCATADMIN hCatAdmin, HCATINFO hCatInfo, DWORD dwFlags)
{
    CRYPT_CAT_ADMIN             *psCatAdmin;
    CATALOG_INFO_CONTEXT        *pContext;

    psCatAdmin  = (CRYPT_CAT_ADMIN *)hCatAdmin;
    pContext    = (CATALOG_INFO_CONTEXT *)hCatInfo;

    if (!(psCatAdmin) ||
        (psCatAdmin->cbStruct != sizeof(CRYPT_CAT_ADMIN)) ||
        !(pContext) ||
        (pContext->cbStruct != sizeof(CATALOG_INFO_CONTEXT)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    _DeallocateInfoContext(&pContext);

    return(TRUE);

}

HCATINFO WINAPI CryptCATAdminEnumCatalogFromHash(HCATADMIN hCatAdmin,
                                                 BYTE *pbHash, DWORD cbHash,
                                                 DWORD dwFlags,
                                                 HCATINFO *phPrev)
{
    CRYPT_CAT_ADMIN             *psCatAdmin;
    WCHAR                       *pwsz;
    CATALOG_INFO_CONTEXT        *psRet;
    CATALOG_INFO_CONTEXT        *pPrev;

    pwsz        = NULL;
    psRet       = NULL;

    if (phPrev)
    {
        pPrev       = (CATALOG_INFO_CONTEXT *)*phPrev;
    }
    else
    {
        pPrev       = NULL;
    }

    psCatAdmin  = (CRYPT_CAT_ADMIN *)hCatAdmin;

    if (!(psCatAdmin) ||
        (psCatAdmin->cbStruct != sizeof(CRYPT_CAT_ADMIN)) ||
        !(psCatAdmin->pDB) ||
        !(psCatAdmin->paHashDB) ||
        !(pbHash) ||
        (cbHash == 0) ||
        (cbHash > MAX_HASH_LEN) ||
        (dwFlags != 0))
    {
        goto InvalidParam;
    }

    if (!(psRet = _AllocateInfoContext()))
    {
        goto MemoryError;
    }

    ENUM_HASH_IDX   *psIdx;

    psIdx = (ENUM_HASH_IDX *)psRet->dwEnumReserved;

    if (!(pPrev))
    {
        //
        //  first call or caller passed in NULL
        //
        psIdx->dwRecNum = 0xffffffff;
    }
    else
    {
        if (pPrev->cbStruct != sizeof(CATALOG_INFO_CONTEXT))
        {
            goto InvalidParam;
        }

        ENUM_HASH_IDX   *psPrevIdx;

        psPrevIdx = (ENUM_HASH_IDX *)pPrev->dwEnumReserved;

        if (!(psPrevIdx))
        {
            goto InvalidParam;
        }

        psIdx->dwRecNum   = psPrevIdx->dwRecNum;
        psIdx->dwDBIdx    = psPrevIdx->dwDBIdx;
    }

    DELETE_OBJECT(psCatAdmin->pwszCurCatName);
    psCatAdmin->dwCurrentCatId  = INVALID_CAT_ID;

    if (!(psCatAdmin->paHashDB[0]))
    {
        if (!(_LoadHashDB(psCatAdmin)))
        {
            goto DBError;
        }
    }

    HashMast    sHashMast;
    DWORD       dwIdx;

    memset(&sHashMast, 0x00, sizeof(HashMast));

    dwIdx = psIdx->dwDBIdx;
    while (dwIdx < psCatAdmin->cHashDB)
    {
        if (psCatAdmin->paHashDB[dwIdx])
        {
            if (psCatAdmin->paHashDB[dwIdx]->HashMast_Get(psIdx->dwRecNum, pbHash, cbHash, &sHashMast))
            {
                psIdx->dwDBIdx  = dwIdx;
                psIdx->dwRecNum = psCatAdmin->paHashDB[dwIdx]->HashMast_GetKeyNum();

                if (!(psCatAdmin->pwszCurCatName = new WCHAR[wcslen(psCatAdmin->paHashDB[dwIdx]->pwszSubSysDir) +
                                                             wcslen(&sHashMast.CatName[0]) + 2]))
                {
                    goto MemoryError;
                }

                wcscpy(psCatAdmin->pwszCurCatName, psCatAdmin->paHashDB[dwIdx]->pwszSubSysDir);

                if (psCatAdmin->pwszCurCatName[wcslen(psCatAdmin->pwszCurCatName) - 1] != L'\\')
                {
                    wcscat(psCatAdmin->pwszCurCatName, L"\\");
                }

                wcscat(psCatAdmin->pwszCurCatName, &sHashMast.CatName[0]);

                break;
            }
        }

        //
        //  if we are not using the default ID, we only want to check "our" subsystem.
        //
        if (!(psCatAdmin->fUseDefSubSysId))
        {
            break;
        }

        dwIdx++;
        psIdx->dwRecNum   = 0xffffffff;
        psIdx->dwDBIdx    = dwIdx;
    }

    if (!(psCatAdmin->pwszCurCatName))
    {
        goto CatNotFound;
    }

    if (!(psRet->pwszCatalogFile = new WCHAR[wcslen(psCatAdmin->pwszCurCatName) + 1]))
    {
        goto MemoryError;
    }

    wcscpy(psRet->pwszCatalogFile, psCatAdmin->pwszCurCatName);

    CommonReturn:
        if (pPrev)
        {
            _DeallocateInfoContext(&pPrev);
            *phPrev = NULL;
        }

        return((HCATINFO)psRet);

    ErrorReturn:
        _DeallocateInfoContext(&psRet);
        psRet = NULL;
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,      ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, DBError,           ERROR_DATABASE_FAILURE);
    SET_ERROR_VAR_EX(DBG_SS, CatNotFound,       ERROR_NOT_FOUND);
}

BOOL WINAPI CryptCATCatalogInfoFromContext(IN HCATINFO hCatInfo,
                                           IN OUT CATALOG_INFO *psCatInfo,
                                           IN DWORD dwFlags)
{
    CATALOG_INFO_CONTEXT    *pContext;

    pContext = (CATALOG_INFO_CONTEXT *)hCatInfo;

    if (!(pContext) ||
        !(psCatInfo))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }


    if (pContext->pwszCatalogFile)
    {
        wcscpy(&psCatInfo->wszCatalogFile[0], pContext->pwszCatalogFile);

        return(TRUE);
    }

    return(FALSE);
}

HCATINFO WINAPI CryptCATAdminAddCatalog(HCATADMIN hCatAdmin,
                                        WCHAR *pwszCatalogFile,
                                        WCHAR *pwszSelectBaseName,
                                        DWORD dwFlags)
{
    CRYPT_CAT_ADMIN         *psCatAdmin;
    CATALOG_INFO_CONTEXT    *psRet;
    BOOL                    fRet;
    LPWSTR                  pwszName = NULL;

    psRet       = NULL;
    psCatAdmin  = (CRYPT_CAT_ADMIN *)hCatAdmin;

    if (!(psCatAdmin) ||
        (psCatAdmin->cbStruct != sizeof(CRYPT_CAT_ADMIN)) ||
        !(pwszCatalogFile) ||
        (dwFlags != 0))
    {
        goto InvalidParam;
    }

    WCHAR               *pwszBaseName;

    DELETE_OBJECT(psCatAdmin->pwszCurCatName);
    psCatAdmin->dwCurrentCatId  = INVALID_CAT_ID;

    //
    //  first, check the catalog...
    //
    if (!(IsCatalogFile(INVALID_HANDLE_VALUE, pwszCatalogFile)))
    {
        if ( GetLastError() == ERROR_FILE_NOT_FOUND )
        {
            goto ErrorReturn;
        }

        goto BadFileFormat;
    }

    //
    //  set the base file name
    //
    if (!(pwszBaseName = wcsrchr(pwszCatalogFile, L'\\')))
    {
        pwszBaseName = wcsrchr(pwszCatalogFile, L':');
    }

    if (pwszBaseName)
    {
        *pwszBaseName++;
    }
    else
    {
        pwszBaseName = pwszCatalogFile;
    }

    CatMast     sCatMast;
    SYSTEMTIME  sTime;

    memset(&sCatMast, 0x00, sizeof(CatMast));

    //
    //  get the a new Id
    //
    sCatMast.CatId = psCatAdmin->pDB->CatMast_GetNewId();
    sCatMast.SysId = psCatAdmin->dwCurrentSysId;

    wcscpy(&sCatMast.OrigName[0], pwszBaseName);

    if (pwszSelectBaseName)
    {
        wcscat(&sCatMast.CurName[0], pwszSelectBaseName);
    }
    else
    {
        sCatMast.CurName[0] = NULL; // will get set in the add
    }

    GetSystemTime(&sTime);
    SystemTimeToFileTime(&sTime, &sCatMast.InstDate);

    if (!(psCatAdmin->pDB->CatMast_Add(&sCatMast)))
    {
        goto DBError;
    }

    //
    //  build return catalog name
    //
    psCatAdmin->pwszCurCatName = new WCHAR[wcslen(psCatAdmin->pwszSubSysDir) + wcslen(&sCatMast.CurName[0]) + 1];

    if (!(psCatAdmin->pwszCurCatName))
    {
        goto MemoryError;
    }

    CreateDirectoryU(psCatAdmin->pwszSubSysDir, NULL);

    wcscpy(psCatAdmin->pwszCurCatName, psCatAdmin->pwszSubSysDir);
    wcscat(psCatAdmin->pwszCurCatName, &sCatMast.CurName[0]);

    //
    //  save the catalog file to our directory
    //
    if (!(CopyFileU(pwszCatalogFile, psCatAdmin->pwszCurCatName, FALSE)))   // overwrite!
    {
        DELETE_OBJECT(psCatAdmin->pwszCurCatName);
        goto FileCopyError;
    }

    SetFileAttributesU(psCatAdmin->pwszCurCatName, FILE_ATTRIBUTE_NORMAL);

    if ( pwszSelectBaseName != NULL )
    {
        pwszName = sCatMast.CurName;
    }

    CatalogCompactHashDatabase(
           HASHMAST_NAME,
           psCatAdmin->pwszSubSysDir,
           HASHMAST_NAME,
           pwszName
           );

    if (!(_UpdateHashIndex(psCatAdmin, &sCatMast.CurName[0])))
    {
        DELETE_OBJECT(psCatAdmin->pwszCurCatName);
        goto ErrorReturn;
    }

    CatalogCompactHashDatabase(
           HASHMAST_NAME,
           psCatAdmin->pwszSubSysDir,
           HASHMAST_NAME,
           NULL
           );

    psCatAdmin->dwCurrentCatId  = sCatMast.CatId;

    if (!(psRet = _AllocateInfoContext()))
    {
        goto MemoryError;
    }

    if (!(psRet->pwszCatalogFile = new WCHAR[wcslen(psCatAdmin->pwszCurCatName) + 1]))
    {
        goto MemoryError;
    }

    wcscpy(psRet->pwszCatalogFile, psCatAdmin->pwszCurCatName);

    CommonReturn:
        return((HCATINFO)psRet);

    ErrorReturn:
        if (psCatAdmin)
        {
            DELETE_OBJECT(psCatAdmin->pwszCurCatName);
        }
        _DeallocateInfoContext(&psRet);
        psRet = NULL;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, FileCopyError);

    SET_ERROR_VAR_EX(DBG_SS, InvalidParam,      ERROR_INVALID_PARAMETER);
    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, DBError,           ERROR_DATABASE_FAILURE);
    SET_ERROR_VAR_EX(DBG_SS, BadFileFormat,     ERROR_BAD_FORMAT);
}

BOOL WINAPI CatAdminDllMain(HANDLE hInstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
                SetupDefaults();
                break;

        case DLL_PROCESS_DETACH:
                CleanupDefaults();
                break;
    }

    return(TRUE);
}


//////////////////////////////////////////////////////////////////////////
//
//  Local functions:
//

BOOL _LoadHashDB(CRYPT_CAT_ADMIN *psCatAdmin)
{
    DWORD   dwCnt;
    DWORD   dwIdx;
    DWORD   dwDBIdx;
    SysMast sRec;
    BOOL    fRet;
    BOOL    fLoop;
    BOOL    fCreatedOK = FALSE;

    if (!(psCatAdmin->fUseDefSubSysId))
    {
        if (!(psCatAdmin->paHashDB[0] = new cHashDB_(psCatAdmin->pwszSubSysDir, L"", &fCreatedOK)))
        {
            goto MemoryError;
        }

        if (!fCreatedOK)
        {
            goto ErrorReturn;
        }

        if (!(psCatAdmin->paHashDB[0]->Initialize()))
        {
            DELETE_OBJECT(psCatAdmin->paHashDB[0]);
            goto DBError;
        }

        fRet = TRUE;

        goto CommonReturn;
    }

    dwCnt = psCatAdmin->pDB->SysMast_NumKeys();

    if (!(fLoop = psCatAdmin->pDB->SysMast_GetFirst(&sRec)))
    {
        goto DBError;
    }

    dwIdx   = 0;
    dwDBIdx = 0;

    while (fLoop)
    {
        if (dwIdx >= dwCnt)
        {
            return(TRUE);  // we should do a re-alloc!
        }

        if (sRec.SubDir[0])
        {
            psCatAdmin->paHashDB[dwDBIdx] = new cHashDB_(pwszCatalogBaseDirectory, &sRec.SubDir[0], &fCreatedOK);

            if (!(psCatAdmin->paHashDB[dwDBIdx]))
            {
                goto MemoryError;
            }

            if (!fCreatedOK)
            {
                goto ErrorReturn;
            }

            if (!(psCatAdmin->paHashDB[dwDBIdx]->Initialize()))
            {
                DELETE_OBJECT(psCatAdmin->paHashDB[dwDBIdx]);
                goto DBError;
            }

            dwDBIdx++;
        }

        dwIdx++;

        fLoop = psCatAdmin->pDB->SysMast_GetNext(&sRec);
    }

    fRet = TRUE;

    CommonReturn:
        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        goto CommonReturn;

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, DBError,           ERROR_DATABASE_FAILURE);
}

static void SetupDefaults(void)
{
    char    szDefaultDir[MAX_PATH + 1];
    WCHAR   wszDefaultDir[MAX_PATH + 1];
    LPWSTR  pwszDefaultDir = NULL;
    DWORD   dwDisposition;
    HKEY    hKey;

    szDefaultDir[0] = NULL;
    GetSystemDirectory(&szDefaultDir[0], MAX_PATH);

    wszDefaultDir[0] = NULL;
    MultiByteToWideChar(0, 0, &szDefaultDir[0], -1, &wszDefaultDir[0], MAX_PATH);

    pwszDefaultDir = new WCHAR [ wcslen(wszDefaultDir)+wcslen(REG_CATALOG_BASE_DIRECTORY)+3 ];
    if ( pwszDefaultDir == NULL )
    {
        // BUGBUG: Not setup for upper layer to deal with errors
        SetLastError( E_OUTOFMEMORY );
        return;
    }

    wcscpy( pwszDefaultDir, wszDefaultDir );

    if ((wszDefaultDir[0]) && (wszDefaultDir[wcslen(&wszDefaultDir[0]) - 1] != L'\\'))
    {
        wcscat(pwszDefaultDir, L"\\");
    }
    wcscat(pwszDefaultDir, REG_CATALOG_BASE_DIRECTORY);
    wcscat(pwszDefaultDir, L"\\");

    DELETE_OBJECT(pwszCatalogBaseDirectory);

#if defined(ENABLE_REG_BASED_CATROOT)

    if (RegCreateKeyExU(    HKEY_LOCAL_MACHINE,
                            REG_MACHINE_SETTINGS_KEY,
                            0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS, NULL,
                            &hKey, &dwDisposition) == ERROR_SUCCESS)
    {
        DWORD   dwType;
        DWORD   cbSize;


        cbSize = 0;
        RegQueryValueExU(hKey, REG_CATALOG_BASE_DIRECTORY, NULL, &dwType, NULL, &cbSize);

        if (cbSize > 0)
        {
            if (!(pwszCatalogBaseDirectory = new WCHAR[(cbSize / sizeof(WCHAR)) + 3]))
            {
                RegCloseKey(hKey);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return;
            }

            pwszCatalogBaseDirectory[0] = NULL;

            RegQueryValueExU(hKey, REG_CATALOG_BASE_DIRECTORY, NULL, &dwType,
                             (BYTE *)pwszCatalogBaseDirectory, &cbSize);
        }
        else
        {
            RegSetValueExU(hKey, REG_CATALOG_BASE_DIRECTORY, 0, REG_SZ,
                            (const BYTE *)pwszDefaultDir,
                            (wcslen(pwszDefaultDir) + 1) * sizeof(WCHAR));
        }

        RegCloseKey(hKey);
    }

#endif

    if ((pwszCatalogBaseDirectory) && (pwszCatalogBaseDirectory[0]))
    {
        if (pwszCatalogBaseDirectory[wcslen(pwszCatalogBaseDirectory) - 1] != L'\\')
        {
            wcscat(pwszCatalogBaseDirectory, L"\\");
        }
    }
    else
    {
        DELETE_OBJECT(pwszCatalogBaseDirectory);

        if (!(pwszCatalogBaseDirectory = new WCHAR[wcslen(pwszDefaultDir) + 1]))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return;
        }

        wcscpy(pwszCatalogBaseDirectory, pwszDefaultDir);
    }

    DELETE_OBJECT(pwszDefaultDir);
}

static void CleanupDefaults(void)
{
    DELETE_OBJECT(pwszCatalogBaseDirectory);
}

BOOL _UpdateHashIndex(CRYPT_CAT_ADMIN *psCatAdmin, WCHAR *pwszCatName)
{
    cHashDB_    *pHashDB;
    BOOL        fRet;
    HANDLE      hCat;
    LPWSTR      pwszPath = NULL;
    BOOL        fCreatedOK = FALSE;

    hCat = NULL;

    if (!(pHashDB = new cHashDB_(psCatAdmin->pwszSubSysDir, L"", &fCreatedOK)))
    {
        goto MemoryError;
    }

    if (!fCreatedOK)
    {
        goto ErrorReturn;
    }

    if (!(pHashDB->Initialize()))
    {
        goto DBError;
    }

    pwszPath = new WCHAR [ wcslen(psCatAdmin->pwszSubSysDir)+wcslen(pwszCatName)+2 ];
    if ( pwszPath == NULL )
    {
        goto MemoryError;
    }

    wcscpy(pwszPath, psCatAdmin->pwszSubSysDir);

    if (pwszPath[wcslen(pwszPath) - 1] != L'\\')
    {
        wcscat(pwszPath, L"\\");
    }

    wcscat(pwszPath, pwszCatName);

    if (!(hCat = CryptCATOpen(pwszPath, 0, NULL, 0, 0)))
    {
        goto CATError;
    }

    CRYPTCATMEMBER  *pCatMember;
    HashMast        sHashMast;

    pCatMember = NULL;

    while (pCatMember = CryptCATEnumerateMember(hCat, pCatMember))
    {
        if (pCatMember->pIndirectData)
        {
            if (pCatMember->pIndirectData->Digest.pbData)
            {
                if (pCatMember->pIndirectData->Digest.cbData <= MAX_HASH_LEN)
                {
                    memset(&sHashMast, 0x00, sizeof(HashMast));

                    memcpy(&sHashMast.Hash[0], pCatMember->pIndirectData->Digest.pbData,
                                    pCatMember->pIndirectData->Digest.cbData);
                    sHashMast.SysId = psCatAdmin->dwCurrentSysId;
                    wcscpy(&sHashMast.CatName[0], pwszCatName);

                    if (!(pHashDB->HashMast_Add(&sHashMast)))
                    {
                        goto DBError;
                    }
                }
            }
        }
    }

    fRet = TRUE;

    CommonReturn:
        if (hCat)
        {
            CryptCATClose(hCat);
        }

        DELETE_OBJECT(pHashDB);

        DELETE_OBJECT(pwszPath);

        return(fRet);

    ErrorReturn:
        fRet = FALSE;
        goto CommonReturn;

    TRACE_ERROR_EX(DBG_SS, CATError);

    SET_ERROR_VAR_EX(DBG_SS, MemoryError,       ERROR_NOT_ENOUGH_MEMORY);
    SET_ERROR_VAR_EX(DBG_SS, DBError,           ERROR_DATABASE_FAILURE);
}

CATALOG_INFO_CONTEXT *_AllocateInfoContext(void)
{
    CATALOG_INFO_CONTEXT    *psRet;

    if (!(psRet = new CATALOG_INFO_CONTEXT))
    {
        return(NULL);
    }

    memset(psRet, 0x00, sizeof(CATALOG_INFO_CONTEXT));
    psRet->cbStruct     = sizeof(CATALOG_INFO_CONTEXT);

    ENUM_HASH_IDX   *psIdx;

    if (!(psIdx = new ENUM_HASH_IDX))
    {
        delete psRet;
        return(NULL);
    }

    memset(psIdx, 0x00, sizeof(ENUM_HASH_IDX));

    psRet->dwEnumReserved = (DWORD_PTR)psIdx;

    return(psRet);
}

void _DeallocateInfoContext(CATALOG_INFO_CONTEXT **psInfo)
{
    if (!(psInfo) ||
        !(*psInfo))
    {
        return;
    }

    ENUM_HASH_IDX   *psIdx;

    psIdx = (ENUM_HASH_IDX *)(*psInfo)->dwEnumReserved;

    DELETE_OBJECT(psIdx);

    DELETE_OBJECT((*psInfo)->pwszCatalogFile);

    DELETE_OBJECT(*psInfo);
}



