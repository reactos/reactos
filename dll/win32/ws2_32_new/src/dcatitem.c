/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dcatitem.c
 * PURPOSE:     Transport Catalog Entry Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#include "ws2_32.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

PTCATALOG_ENTRY
WSAAPI
WsTcEntryAllocate(VOID)
{
    PTCATALOG_ENTRY CatalogEntry;

    /* Allocate the catalog entry */
    CatalogEntry = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*CatalogEntry));

    /* Set the default non-null members */
    CatalogEntry->RefCount = 1;

    /* Return it */
    return CatalogEntry;
}

VOID
WSAAPI
WsTcEntryDelete(IN PTCATALOG_ENTRY CatalogEntry)
{
    /* Check if a provider is loaded */
    if (CatalogEntry->Provider)
    {
        /* Dereference it too */
        WsTpDereference(CatalogEntry->Provider);
        CatalogEntry->Provider = NULL;
    }

    /* Delete us */
    HeapFree(WsSockHeap, 0, CatalogEntry);
}

VOID
WSAAPI
WsTcEntryDereference(IN PTCATALOG_ENTRY CatalogEntry)
{
    /* Dereference and check if it's now 0 */
    if (!(InterlockedDecrement(&CatalogEntry->RefCount)))
    {
        /* We can delete the Provider now */
        WsTcEntryDelete(CatalogEntry);
    }
}

DWORD
WSAAPI
WsTcEntryInitializeFromRegistry(IN PTCATALOG_ENTRY CatalogEntry,
                                IN HKEY ParentKey,
                                IN DWORD UniqueId)
{
    CHAR CatalogEntryName[13];
    DWORD RegSize;
    DWORD RegType = REG_BINARY;
    HKEY EntryKey;
    DWORD Return;

    /* Convert to a 00000xxx string */
    sprintf(CatalogEntryName, "%0""12""lu", UniqueId);

    /* Open the Entry */
    Return = RegOpenKeyEx(ParentKey,
                          CatalogEntryName,
                          0,
                          KEY_READ,
                          &EntryKey);

    /* Get Size of Catalog Entry Structure */
    Return = RegQueryValueExW(EntryKey, 
                              L"PackedCatalogItem",
                              0,
                              NULL,
                              NULL,
                              &RegSize);
    
    /* Read the Whole Catalog Entry Structure */
    Return = RegQueryValueExW(EntryKey, 
                              L"PackedCatalogItem",
                              0,
                              &RegType,
                              (LPBYTE)&CatalogEntry->DllPath,
                              &RegSize);

    /* Done */
    RegCloseKey(EntryKey);
    return Return;
}

VOID
WSAAPI
WsTcEntrySetProvider(IN PTCATALOG_ENTRY Entry,
                     IN PTPROVIDER Provider)
{
    /* Reference the provider */
    InterlockedIncrement(&Provider->RefCount);

    /* Set it */
    Entry->Provider = Provider;
}
