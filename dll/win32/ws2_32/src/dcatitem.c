/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/dcatitem.c
 * PURPOSE:     Transport Catalog Entry Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* FUNCTIONS *****************************************************************/

PTCATALOG_ENTRY
WSAAPI
WsTcEntryAllocate(VOID)
{
    PTCATALOG_ENTRY CatalogEntry;

    /* Allocate the catalog entry */
    CatalogEntry = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*CatalogEntry));
    if (CatalogEntry)
    {
        /* Set the default non-null members */
        CatalogEntry->RefCount = 1;
    }

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
    LPBYTE Buf;
    DWORD index;

    /* Convert to a 00000xxx string */
    sprintf(CatalogEntryName, "%0""12""lu", UniqueId);

    /* Open the Entry */
    Return = RegOpenKeyEx(ParentKey,
                          CatalogEntryName,
                          0,
                          KEY_READ,
                          &EntryKey);

    /* Get Size of Catalog Entry Structure */
    Return = RegQueryValueEx(EntryKey,
                              "PackedCatalogItem",
                              0,
                              NULL,
                              NULL,
                              &RegSize);

    if (!(Buf = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, RegSize)))
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Read the Whole Catalog Entry Structure */
    Return = RegQueryValueEx(EntryKey,
                              "PackedCatalogItem",
                              0,
                              &RegType,
                              Buf,
                              &RegSize);


    memcpy(CatalogEntry->DllPath, Buf, sizeof(CatalogEntry->DllPath));
    index = sizeof(CatalogEntry->DllPath);
    if (index < RegSize)
    {
        memcpy(&CatalogEntry->ProtocolInfo, &Buf[index], sizeof(WSAPROTOCOL_INFOW));
        index += sizeof(WSAPROTOCOL_INFOW);
    }
    HeapFree(WsSockHeap, 0, Buf);

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
