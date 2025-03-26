/*
 * PROJECT:     ReactOS Application compatibility module
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     common structures / functions
 * COPYRIGHT:   Copyright 2013 Mislav Blažević
 *              Copyright 2017-2019 Mark Jansen (mark.jansen@reactos.org)
 */

#ifndef APPHELP_H
#define APPHELP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <appcompat/sdbtypes.h>

/* Flags for SdbInitDatabase */
#define HID_DOS_PATHS 0x1
#define HID_DATABASE_FULLPATH 0x2
#define HID_NO_DATABASE 0x4
#define HID_DATABASE_TYPE_MASK 0xF00F0000
#define SDB_DATABASE_MAIN_MSI 0x80020000
#define SDB_DATABASE_MAIN_SHIM 0x80030000
#define SDB_DATABASE_MAIN_DRIVERS 0x80040000

// Shim database types
#define SDB_DATABASE_SHIM 0x00010000

typedef struct _SDB {
    PDB pdb;
    BOOL auto_loaded;
} SDB, *HSDB;

typedef struct tagATTRINFO {
    TAG   type;
    DWORD flags;
    union {
        QWORD qwattr;
        DWORD dwattr;
        WCHAR *lpattr;
    };
} ATTRINFO, *PATTRINFO;

#define SDB_MAX_SDBS 16
#define SDB_MAX_EXES 16
#define SDB_MAX_LAYERS 8

/* Flags for adwExeFlags */
#define SHIMREG_DISABLE_SHIM (0x00000001)
#define SHIMREG_DISABLE_APPHELP (0x00000002)
#define SHIMREG_APPHELP_NOUI (0x00000004)
#define SHIMREG_APPHELP_CANCEL (0x10000000)
#define SHIMREG_DISABLE_SXS (0x00000010)
#define SHIMREG_DISABLE_LAYER (0x00000020)
#define SHIMREG_DISABLE_DRIVER (0x00000040)

/* Flags for dwFlags */
#define SHIMREG_HAS_ENVIRONMENT (0x1)

/* Flags for SdbGetMatchingExe */
#define SDBGMEF_IGNORE_ENVIRONMENT (0x1)

typedef struct tagSDBQUERYRESULT {
    TAGREF atrExes[SDB_MAX_EXES];
    DWORD  adwExeFlags[SDB_MAX_EXES];
    TAGREF atrLayers[SDB_MAX_LAYERS];
    DWORD  dwLayerFlags;
    TAGREF trApphelp;
    DWORD  dwExeCount;
    DWORD  dwLayerCount;
    GUID   guidID;
    DWORD  dwFlags;
    DWORD  dwCustomSDBMap;
    GUID   rgGuidDB[SDB_MAX_SDBS];
} SDBQUERYRESULT, *PSDBQUERYRESULT;


#define DB_INFO_FLAGS_VALID_GUID    1

typedef struct _DB_INFORMATION
{
    DWORD dwFlags;
    DWORD dwMajor;
    DWORD dwMinor;
    LPCWSTR Description;
    GUID Id;
    /* Win10+ has an extra field here */
} DB_INFORMATION, *PDB_INFORMATION;


#ifndef APPHELP_NOSDBPAPI
#include "sdbpapi.h"
#endif

/* sdbapi.c */
PWSTR SdbpStrDup(LPCWSTR string);
DWORD SdbpStrsize(PCWSTR string);
HSDB WINAPI SdbInitDatabase(DWORD, LPCWSTR);
void WINAPI SdbReleaseDatabase(HSDB);
BOOL WINAPI SdbGUIDToString(CONST GUID *Guid, PWSTR GuidString, SIZE_T Length);
LPCWSTR WINAPI SdbTagToString(TAG tag);

PDB WINAPI SdbOpenDatabase(LPCWSTR path, PATH_TYPE type);
void WINAPI SdbCloseDatabase(PDB);
BOOL WINAPI SdbIsNullGUID(CONST GUID *Guid);
HRESULT WINAPI SdbGetAppPatchDir(HSDB db, LPWSTR path, DWORD size);
LPWSTR WINAPI SdbGetStringTagPtr(PDB pdb, TAGID tagid);
TAGID WINAPI SdbFindFirstNamedTag(PDB pdb, TAGID root, TAGID find, TAGID nametag, LPCWSTR find_name);
DWORD WINAPI SdbQueryDataExTagID(PDB pdb, TAGID tiExe, LPCWSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpcbBufferSize, TAGID *ptiData);
BOOL WINAPI SdbGetDatabaseInformation(PDB pdb, PDB_INFORMATION information);
VOID WINAPI SdbFreeDatabaseInformation(PDB_INFORMATION information);
BOOL WINAPI SdbRegisterDatabaseEx(_In_ LPCWSTR pszDatabasePath, _In_ DWORD dwDatabaseType, _In_opt_ const PULONGLONG pTimeStamp);
BOOL WINAPI SdbUnregisterDatabase(_In_ const GUID *pguidDB);

/* sdbread.c */
BOOL WINAPI SdbpReadData(PDB pdb, PVOID dest, DWORD offset, DWORD num);
TAG WINAPI SdbGetTagFromTagID(PDB pdb, TAGID tagid);
TAGID WINAPI SdbFindFirstTag(PDB pdb, TAGID parent, TAG tag);
TAGID WINAPI SdbFindNextTag(PDB pdb, TAGID parent, TAGID prev_child);
BOOL WINAPI SdbGetDatabaseID(PDB pdb, GUID* Guid);
DWORD WINAPI SdbReadDWORDTag(PDB pdb, TAGID tagid, DWORD ret);
QWORD WINAPI SdbReadQWORDTag(PDB pdb, TAGID tagid, QWORD ret);
TAGID WINAPI SdbGetFirstChild(PDB pdb, TAGID parent);
TAGID WINAPI SdbGetNextChild(PDB pdb, TAGID parent, TAGID prev_child);
DWORD WINAPI SdbGetTagDataSize(PDB pdb, TAGID tagid);
LPWSTR WINAPI SdbpGetString(PDB pdb, TAGID tagid, PDWORD size);
BOOL WINAPI SdbReadBinaryTag(PDB pdb, TAGID tagid, PBYTE buffer, DWORD size);

/* sdbfileattr.c*/
BOOL WINAPI SdbGetFileAttributes(LPCWSTR path, PATTRINFO *attr_info_ret, LPDWORD attr_count);
BOOL WINAPI SdbFreeFileAttributes(PATTRINFO attr_info);

/* layer.c */
BOOL WINAPI AllowPermLayer(PCWSTR path);
BOOL WINAPI SdbGetPermLayerKeys(PCWSTR wszPath, PWSTR pwszLayers, PDWORD pdwBytes, DWORD dwFlags);
BOOL WINAPI SetPermLayerState(PCWSTR wszPath, PCWSTR wszLayer, DWORD dwFlags, BOOL bMachine, BOOL bEnable);

/* hsdb.c */
BOOL WINAPI SdbGetMatchingExe(HSDB hsdb, LPCWSTR path, LPCWSTR module_name, LPCWSTR env, DWORD flags, PSDBQUERYRESULT result);
BOOL WINAPI SdbTagIDToTagRef(HSDB hsdb, PDB pdb, TAGID tiWhich, TAGREF* ptrWhich);
BOOL WINAPI SdbTagRefToTagID(HSDB hsdb, TAGREF trWhich, PDB* ppdb, TAGID* ptiWhich);
BOOL WINAPI SdbUnpackAppCompatData(HSDB hsdb, LPCWSTR pszImageName, PVOID pData, PSDBQUERYRESULT pQueryResult);
DWORD WINAPI SdbQueryData(HSDB hsdb, TAGREF trWhich, LPCWSTR lpszDataName, LPDWORD lpdwDataType, LPVOID lpBuffer, LPDWORD lpcbBufferSize);

#define ATTRIBUTE_AVAILABLE 0x1
#define ATTRIBUTE_FAILED 0x2

#include <appcompat/sdbtagid.h>

#ifdef __cplusplus
} // extern "C"
#endif

#endif // APPHELP_H
