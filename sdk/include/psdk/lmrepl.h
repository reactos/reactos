/*
 * lmrepl.h
 *
 * This file is part of the ReactOS PSDK package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once
#define _LMREPL_

#ifdef __cplusplus
extern "C" {
#endif

#define REPL_ROLE_EXPORT 1
#define REPL_ROLE_IMPORT 2
#define REPL_ROLE_BOTH   3

#define REPL_INTERVAL_INFOLEVEL  (PARMNUM_BASE_INFOLEVEL+0)
#define REPL_PULSE_INFOLEVEL     (PARMNUM_BASE_INFOLEVEL+1)
#define REPL_GUARDTIME_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+2)
#define REPL_RANDOM_INFOLEVEL    (PARMNUM_BASE_INFOLEVEL+3)

#define REPL_UNLOCK_NOFORCE 0
#define REPL_UNLOCK_FORCE   1

#define REPL_STATE_OK 0
#define REPL_STATE_NO_MASTER 1
#define REPL_STATE_NO_SYNC 2
#define REPL_STATE_NEVER_REPLICATED 3

#define REPL_INTEGRITY_FILE 1
#define REPL_INTEGRITY_TREE 2

#define REPL_EXTENT_FILE 1
#define REPL_EXTENT_TREE 2

#define REPL_EXPORT_INTEGRITY_INFOLEVEL (PARMNUM_BASE_INFOLEVEL+0)
#define REPL_EXPORT_EXTENT_INFOLEVEL    (PARMNUM_BASE_INFOLEVEL+1)

typedef struct _REPL_INFO_0
{
    DWORD rp0_role;
    LPWSTR rp0_exportpath;
    LPWSTR rp0_exportlist;
    LPWSTR rp0_importpath;
    LPWSTR rp0_importlist;
    LPWSTR rp0_logonusername;
    DWORD rp0_interval;
    DWORD rp0_pulse;
    DWORD rp0_guardtime;
    DWORD rp0_random;
} REPL_INFO_0, * PREPL_INFO_0, * LPREPL_INFO_0;

typedef struct _REPL_INFO_1000
{
    DWORD rp1000_interval;
} REPL_INFO_1000,*PREPL_INFO_1000,*LPREPL_INFO_1000;

typedef struct _REPL_INFO_1001
{
    DWORD rp1001_pulse;
} REPL_INFO_1001,*PREPL_INFO_1001,*LPREPL_INFO_1001;

typedef struct _REPL_INFO_1002
{
    DWORD rp1002_guardtime;
} REPL_INFO_1002,*PREPL_INFO_1002,*LPREPL_INFO_1002;

typedef struct _REPL_INFO_1003
{
    DWORD rp1003_random;
} REPL_INFO_1003,*PREPL_INFO_1003,*LPREPL_INFO_1003;

NET_API_STATUS
NET_API_FUNCTION
NetReplGetInfo(
    _In_ LPCWSTR servername OPTIONAL,
    _In_ DWORD level,
    _Out_ LPBYTE* bufptr);

NET_API_STATUS
WINAPI
NetReplSetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _In_ const LPBYTE buf,
    _Out_opt_ LPDWORD parm_err);

typedef struct _REPL_EDIR_INFO_0
{
    LPWSTR rped0_dirname;
} REPL_EDIR_INFO_0, * PREPL_EDIR_INFO_0, * LPREPL_EDIR_INFO_0;

typedef struct _REPL_EDIR_INFO_1
{
    LPWSTR rped1_dirname;
    DWORD rped1_integrity;
    DWORD rped1_extent;
} REPL_EDIR_INFO_1, * PREPL_EDIR_INFO_1, * LPREPL_EDIR_INFO_1;

typedef struct _REPL_EDIR_INFO_2
{
    LPWSTR rped2_dirname;
    DWORD rped2_integrity;
    DWORD rped2_extent;
    DWORD rped2_lockcount;
    DWORD rped2_locktime;
} REPL_EDIR_INFO_2, * PREPL_EDIR_INFO_2, * LPREPL_EDIR_INFO_2;

typedef struct _REPL_EDIR_INFO_1000
{
    DWORD rped1000_integrity;
} REPL_EDIR_INFO_1000, * PREPL_EDIR_INFO_1000, * LPREPL_EDIR_INFO_1000;

typedef struct _REPL_EDIR_INFO_1001
{
    DWORD rped1001_extent;
} REPL_EDIR_INFO_1001, * PREPL_EDIR_INFO_1001, * LPREPL_EDIR_INFO_1001;

typedef struct _REPL_IDIR_INFO_0
{
    LPWSTR rpid0_dirname;
} REPL_IDIR_INFO_0, * PREPL_IDIR_INFO_0, * LPREPL_IDIR_INFO_0;

typedef struct _REPL_IDIR_INFO_1
{
    LPWSTR rpid1_dirname;
    DWORD rpid1_state;
    LPWSTR rpid1_mastername;
    DWORD rpid1_last_update_time;
    DWORD rpid1_lockcount;
    DWORD rpid1_locktime;
} REPL_IDIR_INFO_1, * PREPL_IDIR_INFO_1, * LPREPL_IDIR_INFO_1;

NET_API_STATUS
NET_API_FUNCTION
NetReplExportDirAdd(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _In_ const LPBYTE buf,
    _Out_opt_ LPDWORD parm_err);

NET_API_STATUS
NET_API_FUNCTION
NetReplExportDirDel(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname);

NET_API_STATUS
NET_API_FUNCTION
NetReplExportDirEnum(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE* bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_opt_ LPDWORD resumehandle);

NET_API_STATUS
NET_API_FUNCTION
NetReplExportDirGetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD level,
    _Out_ LPBYTE* bufptr);

NET_API_STATUS
NET_API_FUNCTION
NetReplExportDirSetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD level,
    _In_ const LPBYTE buf,
    _Out_opt_ LPDWORD parm_err);

NET_API_STATUS
NET_API_FUNCTION
NetReplExportDirLock(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname);

NET_API_STATUS
NET_API_FUNCTION
NetReplExportDirUnlock(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD unlockforce);

NET_API_STATUS
NET_API_FUNCTION
NetReplImportDirAdd(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _In_ const LPBYTE buf,
    _Out_opt_ LPDWORD parm_err);

NET_API_STATUS
NET_API_FUNCTION
NetReplImportDirDel(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname);

NET_API_STATUS
NET_API_FUNCTION
NetReplImportDirEnum(
    _In_opt_ LPCWSTR servername,
    _In_ DWORD level,
    _Out_ LPBYTE* bufptr,
    _In_ DWORD prefmaxlen,
    _Out_ LPDWORD entriesread,
    _Out_ LPDWORD totalentries,
    _Inout_opt_ LPDWORD resumehandle);

NET_API_STATUS
NET_API_FUNCTION
NetReplImportDirGetInfo(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD level,
    _Out_ LPBYTE* bufptr);

NET_API_STATUS
NET_API_FUNCTION
NetReplImportDirLock(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname);

NET_API_STATUS
NET_API_FUNCTION
NetReplImportDirUnlock(
    _In_opt_ LPCWSTR servername,
    _In_ LPCWSTR dirname,
    _In_ DWORD unlockforce);

#ifdef __cplusplus
}
#endif
