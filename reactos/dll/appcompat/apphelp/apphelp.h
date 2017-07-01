/*
 * Copyright 2013 Mislav Blažević
 * Copyright 2015-2017 Mark Jansen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef APPHELP_H
#define APPHELP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "sdbtypes.h"

/* Flags for SdbInitDatabase */
#define HID_DOS_PATHS 0x1
#define HID_DATABASE_FULLPATH 0x2
#define HID_NO_DATABASE 0x4
#define HID_DATABASE_TYPE_MASK 0xF00F0000
#define SDB_DATABASE_MAIN_MSI 0x80020000
#define SDB_DATABASE_MAIN_SHIM 0x80030000
#define SDB_DATABASE_MAIN_DRIVERS 0x80040000

typedef struct _SDB {
    PDB db;
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

#ifndef APPHELP_NOSDBPAPI
#include "sdbpapi.h"
#endif

/* sdbapi.c */
PWSTR SdbpStrDup(LPCWSTR string);
HSDB WINAPI SdbInitDatabase(DWORD, LPCWSTR);
void WINAPI SdbReleaseDatabase(HSDB);
BOOL WINAPI SdbGUIDToString(CONST GUID *Guid, PWSTR GuidString, SIZE_T Length);
LPCWSTR WINAPI SdbTagToString(TAG tag);

PDB WINAPI SdbOpenDatabase(LPCWSTR path, PATH_TYPE type);
void WINAPI SdbCloseDatabase(PDB);
BOOL WINAPI SdbIsNullGUID(CONST GUID *Guid);
BOOL WINAPI SdbGetAppPatchDir(HSDB db, LPWSTR path, DWORD size);
LPWSTR WINAPI SdbGetStringTagPtr(PDB db, TAGID tagid);
TAGID WINAPI SdbFindFirstNamedTag(PDB db, TAGID root, TAGID find, TAGID nametag, LPCWSTR find_name);

/* sdbread.c */
BOOL WINAPI SdbpReadData(PDB db, PVOID dest, DWORD offset, DWORD num);
TAG WINAPI SdbGetTagFromTagID(PDB db, TAGID tagid);
TAGID WINAPI SdbFindFirstTag(PDB db, TAGID parent, TAG tag);
TAGID WINAPI SdbFindNextTag(PDB db, TAGID parent, TAGID prev_child);
BOOL WINAPI SdbGetDatabaseID(PDB db, GUID* Guid);
DWORD WINAPI SdbReadDWORDTag(PDB db, TAGID tagid, DWORD ret);
QWORD WINAPI SdbReadQWORDTag(PDB db, TAGID tagid, QWORD ret);
TAGID WINAPI SdbGetFirstChild(PDB db, TAGID parent);
TAGID WINAPI SdbGetNextChild(PDB db, TAGID parent, TAGID prev_child);

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

#define ATTRIBUTE_AVAILABLE 0x1
#define ATTRIBUTE_FAILED 0x2

#include "sdbtagid.h"

#ifdef __cplusplus
} // extern "C"
#endif

#endif // APPHELP_H
