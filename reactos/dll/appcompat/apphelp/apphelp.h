/*
 * Copyright 2013 Mislav Blažević
 * Copyright 2015,2016 Mark Jansen
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

typedef enum _SHIM_LOG_LEVEL {
    SHIM_ERR = 1,
    SHIM_WARN = 2,
    SHIM_INFO = 3,
}SHIM_LOG_LEVEL;

/* apphelp.c */
BOOL WINAPIV ShimDbgPrint(SHIM_LOG_LEVEL Level, PCSTR FunctionName, PCSTR Format, ...);
extern ULONG g_ShimDebugLevel;

#define SHIM_ERR(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_ERR, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_WARN(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_WARN, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)
#define SHIM_INFO(fmt, ...)  do { if (g_ShimDebugLevel) ShimDbgPrint(SHIM_INFO, __FUNCTION__, fmt, ##__VA_ARGS__ ); } while (0)


#include "sdbpapi.h"

PWSTR SdbpStrDup(LPCWSTR string);
BOOL WINAPI SdbpCheckTagType(TAG tag, WORD type);
BOOL WINAPI SdbpCheckTagIDType(PDB db, TAGID tagid, WORD type);

PDB WINAPI SdbOpenDatabase(LPCWSTR path, PATH_TYPE type);
void WINAPI SdbCloseDatabase(PDB);
BOOL WINAPI SdbIsNullGUID(CONST GUID *Guid);

/* sdbread.c */
BOOL WINAPI SdbpReadData(PDB db, PVOID dest, DWORD offset, DWORD num);
TAG WINAPI SdbGetTagFromTagID(PDB db, TAGID tagid);
TAGID WINAPI SdbFindFirstTag(PDB db, TAGID parent, TAG tag);
BOOL WINAPI SdbGetDatabaseID(PDB db, GUID* Guid);


/* layer.c */
BOOL WINAPI AllowPermLayer(PCWSTR path);
BOOL WINAPI SdbGetPermLayerKeys(PCWSTR wszPath, PWSTR pwszLayers, PDWORD pdwBytes, DWORD dwFlags);
BOOL WINAPI SetPermLayerState(PCWSTR wszPath, PCWSTR wszLayer, DWORD dwFlags, BOOL bMachine, BOOL bEnable);


#define ATTRIBUTE_AVAILABLE 0x1
#define ATTRIBUTE_FAILED 0x2

#include "sdbtagid.h"

#ifdef __cplusplus
} // extern "C"
#endif

#endif // APPHELP_H
