/*
 * Copyright 2006 Mike McCormack
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

#ifndef __WINE_NTQUERY_H__
#define __WINE_NTQUERY_H__

/* FIXME: #include <stgprop.h> */

#include <pshpack4.h>

typedef struct _CI_STATE
{
    DWORD cbStruct;
    DWORD cWordList;
    DWORD cPersistentIndex;
    DWORD cQueries;
    DWORD cDocuments;
    DWORD cFreshTest;
    DWORD dwMergeProgress;
    DWORD eState;
    DWORD cFilteredDocuments;
    DWORD cTotalDocuments;
    DWORD cPendingScans;
    DWORD dwIndexSize;
    DWORD cUniqueKeys;
    DWORD cSeqQDocuments;
    DWORD dwPropCacheSize;
} CI_STATE;

#include <poppack.h>

#define PSGUID_STORAGE {0xb725f130, 0x47ef, 0x101a, {0xa5, 0xf1, 0x02, 0x60, 0x8c, 0x9e, 0xeb, 0xac}}
#define PID_STG_DIRECTORY      ((PROPID)0x02)
#define PID_STG_CLASSID        ((PROPID)0x03)
#define PID_STG_STORAGETYPE    ((PROPID)0x04)
#define PID_STG_VOLUME_ID      ((PROPID)0x05)
#define PID_STG_PARENT_WORKID  ((PROPID)0x06)
#define PID_STG_SECONDARYSTORE ((PROPID)0x07)
#define PID_STG_FILEINDEX      ((PROPID)0x08)
#define PID_STG_LASTCHANGEUSN  ((PROPID)0x09)
#define PID_STG_NAME           ((PROPID)0x0a)
#define PID_STG_PATH           ((PROPID)0x0b)
#define PID_STG_SIZE           ((PROPID)0x0c)
#define PID_STG_ATTRIBUTES     ((PROPID)0x0d)
#define PID_STG_WRITETIME      ((PROPID)0x0e)
#define PID_STG_CREATETIME     ((PROPID)0x0f)
#define PID_STG_ACCESSTIME     ((PROPID)0x10)
#define PID_STG_CHANGETIME     ((PROPID)0x11)
#define PID_STG_CONTENTS       ((PROPID)0x13)
#define PID_STG_SHORTNAME      ((PROPID)0x14)
#define PID_STG_MAX            PID_STG_SHORTNAME


#ifdef __cplusplus
extern "C" {
#endif

STDAPI CIState(WCHAR const *, WCHAR const *, CI_STATE *);
STDAPI LocateCatalogsA(CHAR const *, ULONG, CHAR *, ULONG *, CHAR *, ULONG *);
STDAPI LocateCatalogsW(WCHAR const *, ULONG, WCHAR *, ULONG *, WCHAR *, ULONG *);
#define LocateCatalogs WINELIB_NAME_AW(LocateCatalogs)

#ifdef __cplusplus
}
#endif

#endif