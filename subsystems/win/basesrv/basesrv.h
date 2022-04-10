/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/basesrv.h
 * PURPOSE:         Main header - Definitions
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#ifndef __BASESRV_H__
#define __BASESRV_H__

/* PSDK/NDK Headers */
#include <stdio.h>
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#include <windef.h>
#include <winbase.h>
#include <dbt.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/exfuncs.h>
#include <ndk/umfuncs.h>
#include <ndk/cmfuncs.h>
#include <ndk/sefuncs.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

/* BASE Headers */
#include <win/basemsg.h>
#include <win/base.h>

typedef struct _BASESRV_KERNEL_IMPORTS
{
    PCHAR  FunctionName;
    PVOID* FunctionPointer;
} BASESRV_KERNEL_IMPORTS, *PBASESRV_KERNEL_IMPORTS;

/* FIXME: BASENLS.H */
typedef NTSTATUS (WINAPI *POPEN_DATA_FILE)(HANDLE hFile,
                                           PWCHAR FileName);

typedef BOOL (WINAPI *PGET_CP_FILE_NAME_FROM_REGISTRY)(UINT   CodePage,
                                                       LPWSTR FileName,
                                                       ULONG  FileNameSize);

typedef BOOL (WINAPI *PGET_NLS_SECTION_NAME)(UINT   CodePage,
                                             UINT   Base,
                                             ULONG  Unknown,
                                             LPWSTR BaseName,
                                             LPWSTR Result,
                                             ULONG  ResultSize);

typedef BOOL (WINAPI *PVALIDATE_LOCALE)(IN ULONG LocaleId);
typedef NTSTATUS (WINAPI *PCREATE_NLS_SECURTY_DESCRIPTOR)(_Out_ PVOID *SecurityDescriptorBuffer,
                                                          _In_ ULONG DescriptorSize,
                                                          _In_ ULONG AccessMask);

/* Globals */
extern HANDLE BaseSrvHeap;
extern HANDLE BaseSrvSharedHeap;
extern PBASE_STATIC_SERVER_DATA BaseStaticServerData;
extern ULONG SessionId;
extern ULONG ProtectionMode;
extern RTL_CRITICAL_SECTION BaseSrvDDDBSMCritSec;

#define SM_REG_KEY \
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Session Manager"

#endif /* __BASESRV_H__ */
