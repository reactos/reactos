/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/nscatent.c
 * PURPOSE:     Namespace Catalog Entry Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* FUNCTIONS *****************************************************************/

PNSCATALOG_ENTRY
WSAAPI
WsNcEntryAllocate(VOID)
{
    PNSCATALOG_ENTRY CatalogEntry;

    /* Allocate the catalog */
    CatalogEntry = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*CatalogEntry));
    if (CatalogEntry)
    {
        /* Set the default non-null members */
        CatalogEntry->RefCount = 1;
        CatalogEntry->Enabled = TRUE;
        CatalogEntry->AddressFamily = -1;
    }

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
    INT ErrorCode;
    CHAR CatalogEntryName[13];
    HKEY EntryKey;
    ULONG RegType = REG_SZ;
    ULONG RegSize = MAX_PATH;
    ULONG RegValue;

    /* Convert to a 00000xxx string */
    sprintf(CatalogEntryName, "%0""12""i", (int)UniqueId);

    /* Open the Entry */
    ErrorCode = RegOpenKeyEx(ParentKey,
                             CatalogEntryName,
                             0,
                             KEY_READ,
                             &EntryKey);
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;
    /* Read the Library Path */
    ErrorCode = RegQueryValueExW(EntryKey,
                                 L"LibraryPath",
                                 0,
                                 &RegType,
                                 (LPBYTE)&CatalogEntry->DllPath,
                                 &RegSize);
    if (ErrorCode != ERROR_SUCCESS) goto out;
    /* Query Display String Size*/
    ErrorCode = RegQueryValueExW(EntryKey,
                                 L"DisplayString",
                                 0,
                                 NULL,
                                 NULL,
                                 &RegSize);
    if (ErrorCode != ERROR_SUCCESS) goto out;
    /* Allocate it */
    CatalogEntry->ProviderName = (LPWSTR)HeapAlloc(WsSockHeap, 0, RegSize);

    /* Read it */
    ErrorCode = RegQueryValueExW(EntryKey,
                                 L"DisplayString",
                                 0,
                                 &RegType,
                                 (LPBYTE)CatalogEntry->ProviderName,
                                 &RegSize);
    if (ErrorCode != ERROR_SUCCESS) goto out;
    /* Read the Provider Id */
    RegType = REG_BINARY;
    RegSize = sizeof(GUID);
    ErrorCode = RegQueryValueEx(EntryKey,
                                "ProviderId",
                                0,
                                &RegType,
                                (LPBYTE)&CatalogEntry->ProviderId,
                                &RegSize);
    if (ErrorCode != ERROR_SUCCESS) goto out;
    /* Read the Address Family */
    RegType = REG_DWORD;
    RegSize = sizeof(DWORD);
    ErrorCode = RegQueryValueEx(EntryKey,
                                "AddressFamily",
                                0,
                                &RegType,
                                (LPBYTE)&CatalogEntry->AddressFamily,
                                &RegSize);
    if (ErrorCode != ERROR_SUCCESS) goto out;
    /* Read the Namespace Id */
    ErrorCode = RegQueryValueEx(EntryKey,
                                "SupportedNamespace",
                                0,
                                &RegType,
                                (LPBYTE)&CatalogEntry->NamespaceId,
                                &RegSize);
    if (ErrorCode != ERROR_SUCCESS) goto out;
    /* Read the Enabled Flag */
    ErrorCode = RegQueryValueEx(EntryKey,
                                "Enabled",
                                0,
                                &RegType,
                                (LPBYTE)&RegValue,
                                &RegSize);
    if (ErrorCode != ERROR_SUCCESS) goto out;
    CatalogEntry->Enabled = RegValue != 0;

    /* Read the Version */
    ErrorCode = RegQueryValueEx(EntryKey,
                                "Version",
                                0,
                                &RegType,
                                (LPBYTE)&CatalogEntry->Version,
                                &RegSize);
    if (ErrorCode != ERROR_SUCCESS) goto out;
    /* Read the Support Service Class Info Flag */
    ErrorCode = RegQueryValueEx(EntryKey,
                                "StoresServiceClassInfo",
                                0,
                                &RegType,
                                (LPBYTE)&RegValue,
                                &RegSize);
    CatalogEntry->StoresServiceClassInfo = RegValue != 0;
out:
    /* Done */
    RegCloseKey(EntryKey);
    return ErrorCode;
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
