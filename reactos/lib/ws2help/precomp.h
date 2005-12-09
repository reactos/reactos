/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 Helper DLL
 * FILE:        lib/ws2help/precomp.h
 * PURPOSE:     WinSock 2 Helper DLL
 */
#ifndef __PRECOMP_H
#define __PRECOMP_H

/* Winsock Provider Headers */
#define WIN32_NO_STATUS
#define _WIN32_WINNT 0x502
#define NTOS_MODE_USER
#define INCL_WINSOCK_API_TYPEDEFS 1
#include <ws2spi.h>

/* NDK Headers */
#include <rtlfuncs.h>
#include <iofuncs.h>

/* Shared Winsock Helper headers */
#include <ws2help.h>
#include <wshdrv.h>

/* Missing definition */
#define SO_OPENTYPE 0x20

/* Global data */
extern HANDLE GlobalHeap;
extern PSECURITY_DESCRIPTOR pSDPipe;
extern HANDLE ghWriterEvent;
extern BOOL Ws2helpInitialized;
extern DWORD gdwSpinCount;
extern DWORD gHandleToIndexMask;

/* Types */
typedef struct _WSH_HASH_TABLE
{
    DWORD Size;
    PWAH_HANDLE Handles[1];
} WSH_HASH_TABLE, *PWAH_HASH_TABLE;

typedef struct _WSH_SEARCH_TABLE
{
    volatile PWAH_HASH_TABLE HashTable;
    volatile PLONG CurrentCount;
    LONG Count1;
    LONG Count2;
    LONG SpinCount;
    BOOL Expanding;
    CRITICAL_SECTION Lock;
} WSH_SEARCH_TABLE, *PWAH_SEARCH_TABLE;

typedef struct _WSH_HANDLE_TABLE 
{
    DWORD Mask;
    WSH_SEARCH_TABLE SearchTables[1];
} WSH_HANDLE_TABLE, *PWAH_HANDLE_TABLE;

/* Functions */
DWORD
WINAPI
Ws2helpInitialize(VOID);

/* Initialization macro */
#define WS2HELP_PROLOG() \
    (Ws2helpInitialized? ERROR_SUCCESS : Ws2helpInitialize())

#endif /* __WS2HELP_H */

/* EOF */
