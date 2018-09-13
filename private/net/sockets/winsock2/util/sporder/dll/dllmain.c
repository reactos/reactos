/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dllmain.c

Abstract:

    Main module for sporder.dll...  the 32-Bit Windows functions that are
    used to change the order or WinSock2 transport service providers and
    name space providers.

Revision History:

--*/

#include <windows.h>
#include <ws2spi.h>
#include <string.h>

#include "sporder.h"

#define MAX_ENTRIES 1000 // hack, make dynamic
void
_cdecl
MyDbgPrint(
    PSTR Format,
    ...
    )
{
    va_list arglist;
    char    OutputBuffer[1024];

    va_start (arglist, Format);
    wvsprintf (OutputBuffer, Format, arglist);
    va_end (arglist);
    OutputDebugString (OutputBuffer);
}

#if DBG
#define DBGOUT(args)    MyDbgPrint args
#else
#define DBGOUT(args)
#endif


typedef struct {
    GUID    ProviderId;
    char    DisplayString[MAX_PATH];
    DWORD   Enabled;
    char    LibraryPath[MAX_PATH];
    DWORD   StoresServiceClassInfo;
    DWORD   SupportedNameSpace;
    DWORD   Version;
} NSP_ITEM;

NSP_ITEM garNspCat[MAX_ENTRIES];
//
// hack, structure copied from winsock2\dll\winsock2\dcatitem.cpp.
//  code should eventually be common.
//

typedef struct {
    char            LibraryPath[MAX_PATH];
    // The unexpanded path where the provider DLL is found.

    WSAPROTOCOL_INFOW   ProtoInfo;
    // The  protocol information.  Note that if the WSAPROTOCOL_INFOW structure
    // is  ever changed to a non-flat structure (i.e., containing pointers)
    // then  this  type  definition  will  have  to  be changed, since this
    // structure must be strictly flat.

} PACKED_CAT_ITEM;

PACKED_CAT_ITEM garPackCat[MAX_ENTRIES];
DWORD garcbData[MAX_ENTRIES];


//
// When we first enumerate and read the child registry keys, store all of
//  those names for later use.
//

TCHAR pszKeyNames[MAX_ENTRIES][MAX_PATH];


//
// The name of the registry keys that we are interested in.
//

TCHAR pszBaseKey[]=                TEXT("SYSTEM\\CurrentControlSet\\Services\\WinSock2\\Parameters");
TCHAR pszProtocolCatalog[]=        TEXT("Protocol_Catalog9");
TCHAR pszNameSpaceCatalog[]=       TEXT("NameSpace_Catalog5");
TCHAR pszCurrentProtocolCatalog[]= TEXT("Current_Protocol_Catalog");
TCHAR pszCurrentNameSpaceCatalog[]=TEXT("Current_NameSpace_Catalog");
TCHAR pszCatalogEntries[]=         TEXT("Catalog_Entries");

TCHAR pszDisplayString[]=          TEXT("DisplayString");
TCHAR pszEnabled[]=                TEXT("Enabled");
TCHAR pszLibraryPath[]=            TEXT("LibraryPath");
TCHAR pszProviderId[]=             TEXT("ProviderId");
TCHAR pszStoresServiceClassInfo[]= TEXT("StoresServiceClassInfo");
TCHAR pszSupportedNameSpace[]=     TEXT("SupportedNameSpace");
TCHAR pszVersion[]=                TEXT("Version");

#define WS2_SZ_KEYNAME TEXT("PackedCatalogItem")


BOOL
WINAPI
DllMain (
    HANDLE hDLL,
    DWORD dwReason,
    LPVOID lpReserved)
/*++

    Obligatory main() routine for DLL.

--*/

{
    return TRUE;
}

int
WSPAPI
WSCWriteProviderOrder (
    IN LPDWORD lpwdCatalogEntryId,
    IN DWORD dwNumberOfEntries)
/*++

Routine Description:

    Reorder existing WinSock2 service providers.  The order of the service
    providers determines their priority in being selected for use.  The
    sporder.exe tool will show you the installed provider and their ordering,
    Alternately, WSAEnumProtocols(), in conjunction with this function,
    will allow you to write your own tool.

Arguments:

    lpwdCatalogEntryId  [in]
      An array of CatalogEntryId elements as found in the WSAPROTOCOL_INFO
      structure.  The order of the CatalogEntryId elements is the new
      priority ordering for the service providers.

    dwNumberOfEntries  [in]
      The number of elements in the lpwdCatalogEntryId array.


Return Value:

    ERROR_SUCCESS   - the service providers have been reordered.
    WSAEINVAL       - input parameters were bad, no action was taken.
    WSATRY_AGAIN    - the routine is being called by another thread or process.
    any registry error code


Comments:

    Here are scenarios in which the WSCWriteProviderOrder function may fail:

      The dwNumberOfEntries is not equal to the number of registered service
      providers.

      The lpwdCatalogEntryId contains an invalid catalog ID.

      The lpwdCatalogEntryId does not contain all valid catalog IDs exactly
      1 time.

      The routine is not able to access the registry for some reason
      (e.g. inadequate user persmissions)

      Another process (or thread) is currently calling the routine.


--*/
{
    int  iIndex;
    int  iNumRegCatEntries;
    int  iWPOReturn;
    DWORD i,j;
    LONG r;
    HKEY hKey;
    HKEY hSubKey;
    DWORD dwBytes;
    DWORD dwType;
    TCHAR pszBuffer[MAX_PATH];
    TCHAR pszFinalKey[MAX_PATH];
    DWORD dwMapping[MAX_ENTRIES];
    DWORD dwDummy[MAX_ENTRIES];
    DWORD dwWait;
    HANDLE hMutex;
    static char pszMutextName[] = TEXT("sporder.dll");
    HMODULE hWS2_32;


    hWS2_32 = LoadLibrary (TEXT ("WS2_32.DLL"));
    if (hWS2_32!=NULL) {
        LPWSCWRITEPROVIDERORDER  lpWSCWriteProviderOrder;
        lpWSCWriteProviderOrder = 
                (LPWSCWRITEPROVIDERORDER)GetProcAddress (
                                            hWS2_32,
                                            "WSCWriteProviderOrder");
        if (lpWSCWriteProviderOrder!=NULL) {
            //MyDbgPrint ("SPORDER: calling ws2_32!WSCWriteProviderOrder...\n");
            iWPOReturn = lpWSCWriteProviderOrder (
                                    lpwdCatalogEntryId,
                                    dwNumberOfEntries
                                    );

        }
        FreeLibrary (hWS2_32);
        if (lpWSCWriteProviderOrder!=NULL)
            return iWPOReturn;
    }


    //
    // Set function return code equal to success
    //  (assume the best and wait to be proven otherwise)
    //

    iWPOReturn = ERROR_SUCCESS;

    //
    // Make sure that we can handle a request of this size.
    //  Hack, this code needs to be replaced by dynamic memory allocation.
    //

    if (dwNumberOfEntries > MAX_ENTRIES) {
        return WSA_NOT_ENOUGH_MEMORY;
    }

    //
    // Protect the code that modifies the registry with a mutex.
    //

    hMutex = CreateMutexA (NULL, FALSE, pszMutextName);
    hMutex = OpenMutexA (SYNCHRONIZE, FALSE, pszMutextName);
    dwWait = WaitForSingleObject (hMutex, 0);
    if (dwWait == WAIT_TIMEOUT)
    {
        DBGOUT((TEXT("WaitForSingleObject, WAIT_TIMEOUT\n")));
        iWPOReturn = WSATRY_AGAIN;
        goto releaseMutex;
    }


    //
    // read catentry format & return error if mismatch
    //

    r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      pszBaseKey,
                      0,
                      KEY_QUERY_VALUE,
                      &hKey);

    if (r != ERROR_SUCCESS)
    {
        DBGOUT((TEXT("RegOpenKeyEx, pszBaseKey, failed \n")));
        iWPOReturn = r;
        goto releaseMutex;
    }

    //
    // Read the current registry storage format being used by WinSock2.
    //  Compare with expected value, and return failure if wrong format.
    //

    dwBytes = sizeof (pszBuffer);
    r = RegQueryValueEx (hKey,
                       pszCurrentProtocolCatalog,
                       NULL,
                       &dwType,
                       (LPVOID) pszBuffer,
                       &dwBytes);

    RegCloseKey (hKey);

    if (r != ERROR_SUCCESS)
    {
        DBGOUT((TEXT("RegQueryValueEx, pszCurrentProtocolCatalog, failed \n")));
        iWPOReturn = r;
        goto releaseMutex;
    }

    if (lstrcmp (pszProtocolCatalog, pszBuffer) != 0)
    {
        DBGOUT((TEXT("Wrong reg. format \n")));
        iWPOReturn = WSAEINVAL;
        goto releaseMutex;
    }


    //
    // Build the final registry key that has the actual catalogs in it
    //  pszBaseKey + \ + pszCurrentProtocolCatalog + \ + pszCatalogEntries
    //  and open it for enumeration
    //

    lstrcpy (pszFinalKey, pszBaseKey);
    lstrcat (pszFinalKey, TEXT("\\"));
    lstrcat (pszFinalKey, pszProtocolCatalog);
    lstrcat (pszFinalKey, TEXT("\\"));
    lstrcat (pszFinalKey, pszCatalogEntries);

    DBGOUT((pszFinalKey));
    DBGOUT((TEXT("\n")));

    r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      pszFinalKey,
                      0,
                      KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                      &hKey);

    if (r != ERROR_SUCCESS)
    {
        DBGOUT((TEXT("RegOpenKeyEx failed \n")));
        iWPOReturn = r;
        goto releaseMutex;
    }


    //
    // The initial open succeeded, now enumerate registry keys
    //  until we don't get any more back
    //

    for (iIndex = 0; ;iIndex++)
    {
        TCHAR    pszSubKey[MAX_PATH];
        TCHAR    szFinalPlusSubKey[MAX_PATH];
        FILETIME ftDummy;
        DWORD    dwSize;

        if (iIndex>=MAX_ENTRIES) {
            DBGOUT((TEXT("iIndex>=MAX_ENTRIES\n")));
            iWPOReturn = WSAEINVAL;
            goto releaseMutex;
        }


        dwSize = MAX_PATH;
        pszSubKey[0]=0;
        r=RegEnumKeyEx (hKey,
                         iIndex,
                         pszSubKey,
                         &dwSize,
                         NULL,
                         NULL,
                         NULL,
                         &ftDummy);

        //
        // Once we have all of the keys, we'll get return code: no_more_items.
        //  close the handle, and exit for loop.
        //

        if (r == ERROR_NO_MORE_ITEMS)
        {
            iNumRegCatEntries = iIndex;
            RegCloseKey(hKey);
            break; // exit for loop
        }


        //
        // Check for other, unexpected error conditions
        //

        if (r != ERROR_SUCCESS)
        {
            DBGOUT((TEXT("Unexpected Error \n")));
            iWPOReturn = r;
            goto releaseMutex;
        }


        //
        // Build up the complete name of the subkey, store it away in
        //  pszKeyNames for future use, and then open the key.
        //

        lstrcpy (szFinalPlusSubKey, pszFinalKey);
        lstrcat (szFinalPlusSubKey, TEXT("\\"));
        lstrcat (szFinalPlusSubKey, pszSubKey);

        lstrcpy (&pszKeyNames[iIndex][0],szFinalPlusSubKey);
        r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                           szFinalPlusSubKey,
                           0,
                           KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                           &hSubKey);

        if (r != ERROR_SUCCESS)
        {
            DBGOUT((TEXT("RegOpenKeyEx, Badly formated subkey \n")));
            iWPOReturn = r;
            goto releaseMutex;
        }


        //
        // Finally, read the binary catalog entry data into our global array.
        //

        dwBytes = sizeof (PACKED_CAT_ITEM);
        dwType = REG_BINARY;
        r = RegQueryValueEx (hSubKey,
                           WS2_SZ_KEYNAME,
                           NULL,
                           &dwType,
                           (LPVOID) &garPackCat[iIndex],
                           &dwBytes);
        garcbData[iIndex]=dwBytes;


        if (r != ERROR_SUCCESS)
        {
            DBGOUT((TEXT("RegQueryValueEx failed \n")));
            iWPOReturn = r;
            goto releaseMutex;
        }

        RegCloseKey(hSubKey);

    } // end for


    //
    // compare dwNumberOfEntries w/ actual number & fail if wrong
    //

    if (iNumRegCatEntries != (int) dwNumberOfEntries)
    {
        DBGOUT((TEXT("iNumRegCatEntries != dwNumberOfEntries \n")));
        iWPOReturn = WSAEINVAL;
        goto releaseMutex;
    }


    //
    // verify that array passed in has same entries as actual list,
    //  and construct index mapping at the same time.  index mapping says
    //  that entry dwMapping[i] should be written to key number i.
    //
    // for array validation:
    //   step through actual list of catalog entries,
    //    set dummy to -1 if match
    //   check that dummy array is all -1 and fail if not true.
    //

    ZeroMemory (dwDummy, dwNumberOfEntries * sizeof (DWORD));
    ZeroMemory (dwMapping, dwNumberOfEntries * sizeof (DWORD));

    for (i = 0; i < dwNumberOfEntries ;i++)
    {
        for (j = 0; j< dwNumberOfEntries ;j++)
        {
            if (garPackCat[i].ProtoInfo.dwCatalogEntryId ==
                    lpwdCatalogEntryId[j])
            {
                  dwDummy[j] = (DWORD)-1;
                  dwMapping[j] = i;
            }
        }
    }

    for (j = 0; j< dwNumberOfEntries ;j++)
    {
        if (dwDummy[j] != (DWORD)-1)
        {
            iWPOReturn = WSAEINVAL;
            goto releaseMutex;
        }
    }

    //
    // Finally, all parameter validation is complete,
    //  and we've read all of the catalog entries.
    //
    // step through array passed in
    //  and if not equal, lookup pre-read entry, and write as registry key
    //

    for (i = 0; i < dwNumberOfEntries ;i++)
    {
        if (dwMapping[i] != i)
        {
            r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      &pszKeyNames[i][0],
                      0,
                      KEY_SET_VALUE,
                      &hKey);

            if (r != ERROR_SUCCESS)
            {
                DBGOUT((TEXT("RegOpenKeyEx, KEY_SET_VALUE failed \n")));
                iWPOReturn = r;
                goto releaseMutex;
            }

            r = RegSetValueEx (hKey,
                               WS2_SZ_KEYNAME,
                               0,
                               REG_BINARY,
                               (LPVOID) &garPackCat[dwMapping[i]],
                               garcbData[i]);

            if (r != ERROR_SUCCESS)
            {
                DBGOUT((TEXT("RegSetValueEx failed \n")));
                iWPOReturn = r;
                goto releaseMutex;
            }

            RegCloseKey(hKey);

            DBGOUT((TEXT("wrote entry %d in location %d \n"), dwMapping[i], i));
        }
    }


    //
    // Release Mutex, close handle, and return.
    //  Notice that this function MUST return only from here at the
    //  end so that we are certain to release the mutex.
    //

releaseMutex:
    ReleaseMutex (hMutex);
    CloseHandle (hMutex);

    return iWPOReturn;
}


LONG
ReadNamspaceRegistry(
    HKEY hKey,
    NSP_ITEM *pItem
)
{
    LONG r;
    HKEY hSubKey;
    DWORD dwBytes;
    DWORD dwType;

    dwBytes = sizeof(pItem->DisplayString);
    r = RegQueryValueEx(hKey,
                        pszDisplayString,
                        NULL,
                        &dwType,
                        (LPVOID) &pItem->DisplayString,
                        &dwBytes);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegQueryValueEx, pszDisplayString, failed \n")));
        return r;
    }

    dwBytes = sizeof(pItem->Enabled);
    r = RegQueryValueEx(hKey,
                        pszEnabled,
                        NULL,
                        &dwType,
                        (LPVOID) &pItem->Enabled,
                        &dwBytes);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegQueryValueEx, pszEnabled, failed \n")));
        return r;
    }

    dwBytes = sizeof(pItem->LibraryPath);
    r = RegQueryValueEx(hKey,
                        pszLibraryPath,
                        NULL,
                        &dwType,
                        (LPVOID) &pItem->LibraryPath,
                        &dwBytes);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegQueryValueEx, pszLibraryPath, failed \n")));
        return r;
    }


    dwBytes = sizeof(pItem->ProviderId);
    r = RegQueryValueEx(hKey,
                        pszProviderId,
                        NULL,
                        &dwType,
                        (LPVOID) &pItem->ProviderId,
                        &dwBytes);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegQueryValueEx, pszProviderId, failed \n")));
        return r;
    }

    dwBytes = sizeof(pItem->StoresServiceClassInfo);
    r = RegQueryValueEx(hKey,
                        pszStoresServiceClassInfo,
                        NULL,
                        &dwType,
                        (LPVOID) &pItem->StoresServiceClassInfo,
                        &dwBytes);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegQueryValueEx, pszStoresServiceClassInfo, failed \n")));
        return r;
    }

    dwBytes = sizeof(pItem->SupportedNameSpace);
    r = RegQueryValueEx(hKey,
                        pszSupportedNameSpace,
                        NULL,
                        &dwType,
                        (LPVOID) &pItem->SupportedNameSpace,
                        &dwBytes);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegQueryValueEx, pszSupportedNameSpace, failed \n")));
        return r;
    }

    dwBytes = sizeof(pItem->Version);
    r = RegQueryValueEx(hKey,
                        pszVersion,
                        NULL,
                        &dwType,
                        (LPVOID) &pItem->Version,
                        &dwBytes);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegQueryValueEx, pszVersion, failed \n")));
        return r;
    }

    return ERROR_SUCCESS;
}

#define GUIDEQUAL(Guid1, Guid2)                     \
       ( (Guid1)->Data1 == (Guid2)->Data1 &&        \
         (Guid1)->Data2 == (Guid2)->Data2 &&        \
         (Guid1)->Data3 == (Guid2)->Data3 &&        \
         (Guid1)->Data4[0] == (Guid2)->Data4[0] &&  \
         (Guid1)->Data4[1] == (Guid2)->Data4[1] &&  \
         (Guid1)->Data4[2] == (Guid2)->Data4[2] &&  \
         (Guid1)->Data4[3] == (Guid2)->Data4[3] &&  \
         (Guid1)->Data4[4] == (Guid2)->Data4[4] &&  \
         (Guid1)->Data4[5] == (Guid2)->Data4[5] &&  \
         (Guid1)->Data4[6] == (Guid2)->Data4[6] &&  \
         (Guid1)->Data4[7] == (Guid2)->Data4[7] )


LONG
WriteNameSpaceRegistry(
    HKEY hKey,
    NSP_ITEM *pItem
)
{
    LONG r;
    HKEY hSubKey;

    r = RegSetValueEx (hKey,
                       pszDisplayString,
                       0,
                       REG_SZ,
                       (LPVOID) &pItem->DisplayString,
                       lstrlen(pItem->DisplayString) + 1);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegSetValueEx failed \n")));
        return r;
    }

    r = RegSetValueEx (hKey,
                       pszEnabled,
                       0,
                       REG_DWORD,
                       (LPVOID) &pItem->Enabled,
                       sizeof(DWORD));

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegSetValueEx failed \n")));
        return r;
    }

    r = RegSetValueEx (hKey,
                       pszLibraryPath,
                       0,
                       REG_SZ,
                       (LPVOID) &pItem->LibraryPath,
                       lstrlen(pItem->LibraryPath) + 1);

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegSetValueEx failed \n")));
        return r;
    }

    r = RegSetValueEx (hKey,
                       pszProviderId,
                       0,
                       REG_BINARY,
                       (LPVOID) &pItem->ProviderId,
                       sizeof(pItem->ProviderId));

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegSetValueEx failed \n")));
        return r;
    }

    r = RegSetValueEx (hKey,
                       pszStoresServiceClassInfo,
                       0,
                       REG_DWORD,
                       (LPVOID) &pItem->StoresServiceClassInfo,
                       sizeof(DWORD));

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegSetValueEx failed \n")));
        return r;
    }

    r = RegSetValueEx (hKey,
                       pszSupportedNameSpace,
                       0,
                       REG_DWORD,
                       (LPVOID) &pItem->SupportedNameSpace,
                       sizeof(DWORD));

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegSetValueEx failed \n")));
        return r;
    }

    r = RegSetValueEx (hKey,
                       pszVersion,
                       0,
                       REG_DWORD,
                       (LPVOID) &pItem->Version,
                       sizeof(DWORD));

    if (r != ERROR_SUCCESS) {
        DBGOUT((TEXT("RegSetValueEx failed \n")));
        return r;
    }

    return r;
}


int
WSPAPI
WSCWriteNameSpaceOrder (
    IN LPGUID lpProviderId,
    IN DWORD dwNumberOfEntries)
/*++


--*/
{
    int  iIndex;
    int  iNumRegCatEntries;
    int  iWPOReturn;
    DWORD i,j;
    LONG r;
    HKEY hKey;
    HKEY hSubKey;
    DWORD dwBytes;
    DWORD dwType;
    TCHAR pszBuffer[MAX_PATH];
    TCHAR pszFinalKey[MAX_PATH];
    DWORD dwMapping[MAX_ENTRIES];
    DWORD dwDummy[MAX_ENTRIES];
    DWORD dwWait;
    HANDLE hMutex;
    static char pszMutextName[] = TEXT("sporder.dll");
    HMODULE hWS2_32;


    hWS2_32 = LoadLibrary (TEXT ("WS2_32.DLL"));
    if (hWS2_32!=NULL) {
        LPWSCWRITENAMESPACEORDER lpWSCWriteNameSpaceOrder;
        lpWSCWriteNameSpaceOrder =
                (LPWSCWRITENAMESPACEORDER)GetProcAddress (
                                            hWS2_32,
                                            "WSCWriteNameSpaceOrder");
        if (lpWSCWriteNameSpaceOrder!=NULL) {
            //MyDbgPrint ("SPORDER: calling ws2_32!WSCWriteNameSpaceOrder...\n");
            iWPOReturn = lpWSCWriteNameSpaceOrder (
                                    lpProviderId,
                                    dwNumberOfEntries
                                    );

        }
        FreeLibrary (hWS2_32);
        if (lpWSCWriteNameSpaceOrder!=NULL)
            return iWPOReturn;
    }
    //
    // Set function return code equal to success
    //  (assume the best and wait to be proven otherwise)
    //

    iWPOReturn = ERROR_SUCCESS;

    //
    // Make sure that we can handle a request of this size.
    //  Hack, this code needs to be replaced by dynamic memory allocation.
    //

    if ( dwNumberOfEntries > MAX_ENTRIES)
        return WSA_NOT_ENOUGH_MEMORY;

    //
    // Protect the code that modifies the registry with a mutex.
    //

    hMutex = CreateMutexA (NULL, FALSE, pszMutextName);
    hMutex = OpenMutexA (SYNCHRONIZE, FALSE, pszMutextName);
    dwWait = WaitForSingleObject (hMutex, 0);
    if (dwWait == WAIT_TIMEOUT)
    {
        DBGOUT((TEXT("WaitForSingleObject, WAIT_TIMEOUT\n")));
        iWPOReturn = ERROR_BUSY;
        goto releaseMutex;
    }


    //
    // read catentry format & return error if mismatch
    //

    r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      pszBaseKey,
                      0,
                      KEY_QUERY_VALUE,
                      &hKey);

    if (r != ERROR_SUCCESS)
    {
        DBGOUT((TEXT("RegOpenKeyEx, pszBaseKey, failed \n")));
        iWPOReturn = r;
        goto releaseMutex;
    }

    //
    // Read the current registry storage format being used by WinSock2.
    //  Compare with expected value, and return failure if wrong format.
    //

    dwBytes = sizeof (pszBuffer);
    r = RegQueryValueEx (hKey,
                       pszCurrentNameSpaceCatalog,
                       NULL,
                       &dwType,
                       (LPVOID) pszBuffer,
                       &dwBytes);

    RegCloseKey (hKey);

    if (r != ERROR_SUCCESS)
    {
        DBGOUT((TEXT("RegQueryValueEx, pszCurrentNameSpaceCatalog, failed \n")));
        iWPOReturn = r;
        goto releaseMutex;
    }

    if (lstrcmp (pszNameSpaceCatalog, pszBuffer) != 0)
    {
        DBGOUT((TEXT("Wrong reg. format \n")));
        iWPOReturn = WSAEINVAL;
        goto releaseMutex;
    }


    //
    // Build the final registry key that has the actual catalogs in it
    //  pszBaseKey + \ + pszCurrentNameSpaceCatalog + \ + pszCatalogEntries
    //  and open it for enumeration
    //

    lstrcpy (pszFinalKey, pszBaseKey);
    lstrcat (pszFinalKey, TEXT("\\"));
    lstrcat (pszFinalKey, pszNameSpaceCatalog);
    lstrcat (pszFinalKey, TEXT("\\"));
    lstrcat (pszFinalKey, pszCatalogEntries);

    DBGOUT((pszFinalKey));
    DBGOUT((TEXT("\n")));

    r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      pszFinalKey,
                      0,
                      KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                      &hKey);

    if (r != ERROR_SUCCESS)
    {
        DBGOUT((TEXT("RegOpenKeyEx failed \n")));
        iWPOReturn = r;
        goto releaseMutex;
    }


    //
    // The initial open succeeded, now enumerate registry keys
    //  until we don't get any more back
    //

    for (iIndex = 0; ;iIndex++)
    {
        TCHAR    pszSubKey[MAX_PATH];
        TCHAR    szFinalPlusSubKey[MAX_PATH];
        FILETIME ftDummy;
        DWORD    dwSize;

        if (iIndex>=MAX_ENTRIES) {
            DBGOUT((TEXT("iIndex>=MAX_ENTRIES\n")));
            iWPOReturn = WSAEINVAL;
            goto releaseMutex;
        }

        dwSize = MAX_PATH;
        pszSubKey[0]=0;
        r=RegEnumKeyEx (hKey,
                         iIndex,
                         pszSubKey,
                         &dwSize,
                         NULL,
                         NULL,
                         NULL,
                         &ftDummy);

        //
        // Once we have all of the keys, we'll get return code: no_more_items.
        //  close the handle, and exit for loop.
        //

        if (r == ERROR_NO_MORE_ITEMS)
        {
            iNumRegCatEntries = iIndex;
            RegCloseKey(hKey);
            break; // exit for loop
        }


        //
        // Check for other, unexpected error conditions
        //

        if (r != ERROR_SUCCESS)
        {
            DBGOUT((TEXT("Unexpected Error \n")));
            iWPOReturn = r;
            goto releaseMutex;
        }


        //
        // Build up the complete name of the subkey, store it away in
        //  pszKeyNames for future use, and then open the key.
        //

        lstrcpy (szFinalPlusSubKey, pszFinalKey);
        lstrcat (szFinalPlusSubKey, TEXT("\\"));
        lstrcat (szFinalPlusSubKey, pszSubKey);

        lstrcpy (&pszKeyNames[iIndex][0],szFinalPlusSubKey);
        r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                           szFinalPlusSubKey,
                           0,
                           KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                           &hSubKey);

        if (r != ERROR_SUCCESS)
        {
            DBGOUT((TEXT("RegOpenKeyEx, Badly formated subkey \n")));
            iWPOReturn = r;
            goto releaseMutex;
        }


        //
        // Finally, read the binary catalog entry data into our global array.
        //

        dwBytes = sizeof (PACKED_CAT_ITEM);
        dwType = REG_BINARY;
        r = ReadNamspaceRegistry (hSubKey,
                                  &garNspCat[iIndex]);

        if (r != ERROR_SUCCESS)
        {
            DBGOUT((TEXT("ReadNamspaceRegistry failed \n")));
            iWPOReturn = r;
            goto releaseMutex;
        }

        RegCloseKey(hSubKey);

    } // end for


    //
    // compare dwNumberOfEntries w/ actual number & fail if wrong
    //

    if (iNumRegCatEntries != (int) dwNumberOfEntries)
    {
        DBGOUT((TEXT("iNumRegCatEntries != dwNumberOfEntries \n")));
        iWPOReturn = WSAEINVAL;
        goto releaseMutex;
    }


    //
    // verify that array passed in has same entries as actual list,
    //  and construct index mapping at the same time.  index mapping says
    //  that entry dwMapping[i] should be written to key number i.
    //
    // for array validation:
    //   step through actual list of catalog entries,
    //    set dummy to -1 if match
    //   check that dummy array is all -1 and fail if not true.
    //

    ZeroMemory (dwDummy, dwNumberOfEntries * sizeof (DWORD));
    ZeroMemory (dwMapping, dwNumberOfEntries * sizeof (DWORD));

    for (i = 0; i < dwNumberOfEntries ;i++)
    {
        for (j = 0; j< dwNumberOfEntries ;j++)
        {
            if (GUIDEQUAL(&garNspCat[i].ProviderId, &lpProviderId[j]))
            {
                  dwDummy[j] = (DWORD)-1;
                  dwMapping[j] = i;
            }
        }
    }

    for (j = 0; j< dwNumberOfEntries ;j++)
    {
        if (dwDummy[j] != (DWORD)-1)
        {
            iWPOReturn = WSAEINVAL;
            goto releaseMutex;
        }
    }

    //
    // Finally, all parameter validation is complete,
    //  and we've read all of the catalog entries.
    //
    // step through array passed in
    //  and if not equal, lookup pre-read entry, and write as registry key
    //

    for (i = 0; i < dwNumberOfEntries ;i++)
    {
        if (dwMapping[i] != i)
        {
            r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      &pszKeyNames[i][0],
                      0,
                      KEY_SET_VALUE,
                      &hKey);

            if (r != ERROR_SUCCESS)
            {
                DBGOUT((TEXT("RegOpenKeyEx, KEY_SET_VALUE failed \n")));
                iWPOReturn = r;
                goto releaseMutex;
            }

            r = WriteNameSpaceRegistry(hKey, &garNspCat[dwMapping[i]]);


            if (r != ERROR_SUCCESS)
            {
                DBGOUT((TEXT("RegSetValueEx failed \n")));
                iWPOReturn = r;
                goto releaseMutex;
            }

            RegCloseKey(hKey);

            DBGOUT((TEXT("wrote entry %d in location %d \n"), dwMapping[i], i));
        }
    }


    //
    // Release Mutex, close handle, and return.
    //  Notice that this function MUST return only from here at the
    //  end so that we are certain to release the mutex.
    //

releaseMutex:
    ReleaseMutex (hMutex);
    CloseHandle (hMutex);

    return iWPOReturn;
}



int
WSPAPI
WSCEnumNameSpaceProviders (
    IN OUT LPDWORD lpdwBufferLength,
    IN LPWSANAMESPACE_INFOW lpnspBuffer,
    LPINT lpErrno)
/*++


--*/
{
  return ERROR_CALL_NOT_IMPLEMENTED;
}


#if DBG
void
_cdecl
DbgPrint(
    PTCH Format,
    ...
    )
/*++

  Write debug output messages if compiled with DEBUG

--*/
{
    TCHAR buffer[MAX_PATH];

    va_list marker;
    va_start (marker,Format);
    wvsprintf (buffer,Format, marker);
    OutputDebugString (TEXT("SPORDER: "));
    OutputDebugString (buffer);

    return;
}
#endif
