/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        include/handle.h
 * PURPOSE:     Provider handle definitions
 */
#ifndef __HANDLE_H
#define __HANDLE_H

#include <ws2_32.h>
#include <catalog.h>

typedef struct _PROVIDER_HANDLE {
    HANDLE Handle;
    PCATALOG_ENTRY Provider;
} PROVIDER_HANDLE, *PPROVIDER_HANDLE;

#define HANDLE_BLOCK_ENTRIES ((PAGESIZE-sizeof(LIST_ENTRY))/sizeof(PROVIDER_HANDLE))

typedef struct _PROVIDER_HANDLE_BLOCK {
    LIST_ENTRY Entry;
    PROVIDER_HANDLE Handles[HANDLE_BLOCK_ENTRIES];
} PROVIDER_HANDLE_BLOCK, *PPROVIDER_HANDLE_BLOCK;

extern PPROVIDER_HANDLE_BLOCK ProviderHandleTable;


HANDLE
CreateProviderHandle(HANDLE Handle,
                     PCATALOG_ENTRY Provider);

BOOL
ReferenceProviderByHandle(HANDLE Handle,
                          PCATALOG_ENTRY* Provider);

BOOL
CloseProviderHandle(HANDLE Handle);

BOOL
InitProviderHandleTable(VOID);

VOID
FreeProviderHandleTable(VOID);

#endif /* __HANDLE_H */

/* EOF */
