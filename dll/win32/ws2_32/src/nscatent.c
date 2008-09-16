/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        nscatent.c
 * PURPOSE:     Namespace Catalog Entry Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#include "ws2_32.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

PNSCATALOG_ENTRY
WSAAPI
WsNcEntryAllocate(VOID)
{
    PNSCATALOG_ENTRY CatalogEntry;

    /* Allocate the catalog */
    CatalogEntry = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*CatalogEntry));

    /* Set the default non-null members */
    CatalogEntry->RefCount = 1;
    CatalogEntry->Enabled = TRUE;
    CatalogEntry->AddressFamily = -1;

    /* Return it */
    return CatalogEntry;
}

VOID
WSAAPI
WsNcEntryDelete(IN PNSCATALOG_ENTRY CatalogEntry)
{
    /* Check if a provider is loaded */
    if (CatalogEntry->Provider)
    {
        /* Dereference it too */
        WsNpDereference(CatalogEntry->Provider);
        CatalogEntry->Provider = NULL;
    }

    /* Delete us */
    HeapFree(WsSockHeap, 0, CatalogEntry);
}

VOID
WSAAPI
WsNcEntryDereference(IN PNSCATALOG_ENTRY CatalogEntry)
{
    /* Dereference and check if it's now 0 */
    if (!(InterlockedDecrement(&CatalogEntry->RefCount)))
    {
        /* We can delete the Provider now */
        WsNcEntryDelete(CatalogEntry);
    }
}

INT
WSAAPI
WsNcEntryInitializeFromRegistry(IN PNSCATALOG_ENTRY CatalogEntry,
                                IN HKEY ParentKey,
                                IN ULONG UniqueId)
{
    CHAR CatalogEntryName[13];
    HKEY EntryKey;
    LONG Return;
    ULONG RegType = REG_SZ;
    ULONG RegSize = MAX_PATH;

    /* Convert to a 00000xxx string */
    sprintf(CatalogEntryName, "%0""12""i", UniqueId);

    /* Open the Entry */
    Return = RegOpenKeyEx(ParentKey,
                          CatalogEntryName,
                          0,
                          KEY_READ,
                          &EntryKey);

    /* Read the Library Path */
    Return = RegQueryValueExW(EntryKey,
                              L"LibraryPath",
                              0,
                              &RegType,
                              (LPBYTE)&CatalogEntry->DllPath,
                              &RegSize);

    /* Query Display String Size*/
    Return = RegQueryValueExW(EntryKey,
                              L"DisplayString",
                              0,
                              NULL,
                              NULL,
                              &RegSize);

    /* Allocate it */
    CatalogEntry->ProviderName = (LPWSTR)HeapAlloc(WsSockHeap, 0, RegSize);

    /* Read it */
    Return = RegQueryValueExW(EntryKey,
                              L"DisplayString",
                              0,
                              &RegType,
                              (LPBYTE)CatalogEntry->ProviderName,
                              &RegSize);

    /* Read the Provider Id */
    RegType = REG_BINARY;
    RegSize = sizeof(GUID);
    Return = RegQueryValueEx(EntryKey,
                             "ProviderId",
                             0,
                             &RegType,
                             (LPBYTE)&CatalogEntry->ProviderId,
                             &RegSize);

    /* Read the Address Family */
    RegType = REG_DWORD;
    RegSize = sizeof(DWORD);
    Return = RegQueryValueEx(EntryKey,
                             "AddressFamily",
                             0,
                             &RegType,
                             (LPBYTE)&CatalogEntry->AddressFamily,
                             &RegSize);

    /* Read the Namespace Id */
    Return = RegQueryValueEx(EntryKey,
                             "SupportedNamespace",
                             0,
                             &RegType,
                             (LPBYTE)&CatalogEntry->NamespaceId,
                             &RegSize);

    /* Read the Enabled Flag */
    Return = RegQueryValueEx(EntryKey,
                             "Enabled",
                             0,
                             &RegType,
                             (LPBYTE)&CatalogEntry->Enabled,
                             &RegSize);

    /* Read the Version */
    Return = RegQueryValueEx(EntryKey,
                             "Version",
                             0,
                             &RegType,
                             (LPBYTE)&CatalogEntry->Version,
                             &RegSize);

    /* Read the Support Service Class Info Flag */
    Return = RegQueryValueEx(EntryKey,
                             "Version",
                             0,
                             &RegType,
                             (LPBYTE)&CatalogEntry->StoresServiceClassInfo,
                             &RegSize);

    /* Done */
    RegCloseKey(EntryKey);
    return ERROR_SUCCESS;
}

VOID
WSAAPI
WsNcEntrySetProvider(IN PNSCATALOG_ENTRY Entry,
                     IN PNS_PROVIDER Provider)
{
    /* Reference the provider */
    InterlockedIncrement(&Provider->RefCount);

    /* Set it */
    Entry->Provider = Provider;
}
