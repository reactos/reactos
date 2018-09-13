/*++

Copyright (c) 1995-1996 Microsoft Corporation

Module Name:

    catentry

Abstract:

    Read & write the relevant registry entries.

Author:

    Steve Firebaugh (stevefir)         14-Jan-1995

Revision History:

--*/


#include <windows.h>
#include <commdlg.h>
#include <winsock2.h>
#include <nspapi.h>
#include <stdio.h>

#include "ws2spi.h"
#include "sporder.h"
#include "globals.h"




#define MAX_ENTRIES 1000 // hack, make dynamic

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

TCHAR pszKeyNames[MAX_ENTRIES][MAX_STR];


//
// The name of the registry key that we are interested in.
//

TCHAR pszWS2RegKey[]=TEXT("SYSTEM\\CurrentControlSet\\Services\\WinSock2\\Parameters\\Protocol_Catalog9\\Catalog_Entries");

#define WS2_SZ_KEYNAME TEXT("PackedCatalogItem")



//
//  keep track of the total number of entries in the list box;
//

int gNumRows = 0;





int CatReadRegistry (HWND hwnd)
/*++

  Called once when the dialog first comes up.  Read the registry and fill
   the listbox with all of the entries.

--*/
{
    TCHAR szOutput[MAX_STR];
    TCHAR szInput[MAX_STR];
    TCHAR szBuffer[MAX_STR];
    LONG r;
    DWORD iIndex;
    DWORD dwSize;
    HKEY hKey;
    HKEY hSubKey;
    FILETIME ftDummy;
    DWORD dwBytes;
    DWORD dwType;

    //
    // set a tab stop far off of the screen, so that we can store the original
    //  index there, and it will stay glued to the string even as the user
    //  reorders them (unless we are building a debug).
    //


#ifndef DEBUG
    {
    int iTab;

    iTab = 300;
    SendMessage (HWNDLISTCTL, LB_SETTABSTOPS, 1, (LPARAM) &iTab);
    }
#endif


    //
    // open the registry key for read and enum
    //

    r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      pszWS2RegKey,
                      0,
                      KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                      &hKey);

    if (r != ERROR_SUCCESS) {
        CatCheckRegErrCode (hwnd, r, TEXT("RegOpenKeyEx"));
        return FALSE;
    }

    //
    // The initial open succeeded, now enumerate registry keys
    //  until we don't get any more back
    //

    for (iIndex = 0; ;iIndex++) {
        ASSERT ((iIndex <MAX_ENTRIES), TEXT("Exceeded max number of protocol entries"));

        dwSize = MAX_STR;
        szOutput[0]=0;
        r=RegEnumKeyEx (hKey,
                         iIndex,
                         szOutput,
                         &dwSize,
                         NULL,
                         NULL,
                         NULL,
                         &ftDummy);

        //
        // Once we have all of the keys, we'll get return code: no_more_items.
        //  Set the global number of rows, close the handle, and return.
        //

        if (r == ERROR_NO_MORE_ITEMS) {
            gNumRows = iIndex;
            RegCloseKey(hKey);
            return FALSE;
        }


        //
        // Check for other, unexpected error conditions
        //

        if (r != ERROR_SUCCESS) {
            CatCheckRegErrCode (hwnd, r, TEXT("RegEnumKeyEx"));
            return FALSE;
        }


        //
        // Build up the complete name of the subkey, store it away in
        //  pszKeyNames for future use, and then open the key.
        //

        lstrcpy (szInput, pszWS2RegKey);
        lstrcat (szInput, TEXT("\\"));
        lstrcat (szInput, szOutput);

        lstrcpy (&pszKeyNames[iIndex][0],szInput);
        r = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                           szInput,
                           0,
                           KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                           &hSubKey);

        if (r != ERROR_SUCCESS) {
            CatCheckRegErrCode (hwnd, r, TEXT("RegOpenKeyEx"));
            return FALSE;
        }


        dwBytes = sizeof (PACKED_CAT_ITEM); // hack, string on end.
        // hack garPackCat
        dwType = REG_BINARY;
        r=RegQueryValueEx (hSubKey,
                           WS2_SZ_KEYNAME,
                           NULL,
                           &dwType,
                           (LPVOID) &garPackCat[iIndex],
                           &dwBytes);
        garcbData[iIndex]=dwBytes;


        if (r != ERROR_SUCCESS) {
            CatCheckRegErrCode (hwnd, r, TEXT("RegQueryValueEx"));
            return FALSE;
        }

        //
        // Now format a string for display in the list box.  Notice that
        //  we sneak an index in to the far right (not visible) to track
        //  the string's initial position (for purposes of mapping it to
        //  the gdwCatEntries later) regardless of re-ordering.
        //

        wsprintf (szBuffer,
                  TEXT("%ws \t%d \t%d"),
                  &garPackCat[iIndex].ProtoInfo.szProtocol,
                  iIndex,
                  garPackCat[iIndex].ProtoInfo.dwCatalogEntryId);
        ADDSTRING(szBuffer);



        RegCloseKey(hSubKey);

    } // end for
  return TRUE;
}



int CatDoMoreInfo (HWND hwnd, int iSelection)
/*++

  Given a dialog handle, and an index into our global array of catalog entries,
   fill a listbox with all of the information that we know about it.

--*/
{
    TCHAR szBuffer[MAX_STR];
    int iTab;
    BYTE pb[16];

    //
    // pick an arbitraty tab number that is far enough to the right to clear
    //  most of the long strings.
    //

    iTab = 90;
    SendMessage (HWNDLISTCTL, LB_SETTABSTOPS, 1, (LPARAM) &iTab);


    wsprintf (szBuffer, TEXT("LibraryPath \t%s"), &garPackCat[iSelection].LibraryPath);       ADDSTRING (szBuffer);

    wsprintf (szBuffer, TEXT("dwServiceFlags1 \t0x%x"), garPackCat[iSelection].ProtoInfo.dwServiceFlags1);       ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("dwServiceFlags2 \t0x%x"), garPackCat[iSelection].ProtoInfo.dwServiceFlags2);       ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("dwServiceFlags3 \t0x%x"), garPackCat[iSelection].ProtoInfo.dwServiceFlags3);       ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("dwServiceFlags4 \t0x%x"), garPackCat[iSelection].ProtoInfo.dwServiceFlags4);       ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("dwProviderFlags \t0x%x"), garPackCat[iSelection].ProtoInfo.dwProviderFlags);       ADDSTRING (szBuffer);

    //
    // format GUID for display.  do byte swapping to match expected format
    //

    memcpy (pb, (PBYTE) &(garPackCat[iSelection].ProtoInfo.ProviderId), sizeof (GUID));

    wsprintf (szBuffer,
              TEXT("ProviderId \t%02x%02x%02x%02x - %02x%02x - %02x%02x - %02x%02x - %02x%02x%02x%02x%02x%02x"),
              (BYTE)pb[3],
              (BYTE)pb[2],
              (BYTE)pb[1],
              (BYTE)pb[0],
              (BYTE)pb[5],
              (BYTE)pb[4],
              (BYTE)pb[7],
              (BYTE)pb[6],
              (BYTE)pb[8],
              (BYTE)pb[9],
              (BYTE)pb[10],
              (BYTE)pb[11],
              (BYTE)pb[12],
              (BYTE)pb[13],
              (BYTE)pb[14],
              (BYTE)pb[15] );
              ADDSTRING (szBuffer);

    wsprintf (szBuffer, TEXT("dwCatalogEntryId \t0x%x"), garPackCat[iSelection].ProtoInfo.dwCatalogEntryId);       ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("ProtocolChain.ChainLen \t%d"), garPackCat[iSelection].ProtoInfo.ProtocolChain.ChainLen); ADDSTRING (szBuffer);

    wsprintf (szBuffer, TEXT("iVersion       \t0x%x"), garPackCat[iSelection].ProtoInfo.iVersion);       ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("iAddressFamily \t0x%x"), garPackCat[iSelection].ProtoInfo.iAddressFamily); ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("iMaxSockAddr   \t0x%x"), garPackCat[iSelection].ProtoInfo.iMaxSockAddr);   ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("iMinSockAddr   \t0x%x"), garPackCat[iSelection].ProtoInfo.iMinSockAddr);   ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("iSocketType    \t0x%x"), garPackCat[iSelection].ProtoInfo.iSocketType);    ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("iProtocol      \t0x%x"), garPackCat[iSelection].ProtoInfo.iProtocol);      ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("iProtocolMaxOffset \t0x%x"), garPackCat[iSelection].ProtoInfo.iProtocolMaxOffset);      ADDSTRING (szBuffer);

    wsprintf (szBuffer, TEXT("iNetworkByteOrder      \t0x%x"), garPackCat[iSelection].ProtoInfo.iNetworkByteOrder);      ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("iSecurityScheme      \t0x%x"), garPackCat[iSelection].ProtoInfo.iSecurityScheme);      ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("dwMessageSize      \t0x%x"), garPackCat[iSelection].ProtoInfo.dwMessageSize);      ADDSTRING (szBuffer);
    wsprintf (szBuffer, TEXT("dwProviderReserved      \t0x%x"), garPackCat[iSelection].ProtoInfo.dwProviderReserved);      ADDSTRING (szBuffer);

    wsprintf (szBuffer, TEXT("szProtocol \t%ws"), &garPackCat[iSelection].ProtoInfo.szProtocol);       ADDSTRING (szBuffer);

    return TRUE;
}



int  CatDoWriteEntries (HWND hwnd)
/*++

  Here we step through all of the entries in the list box, check to see if
   it is out of order, and if it is, write data to the registry key in that
   position.

--*/
{
    TCHAR szBuffer[MAX_STR];
    int iRegKey;
    int iIndex;
    int iCatID;

    LONG r;

    DWORD lpdwCatID[MAX_ENTRIES];

    //
    // Step through all of the registry keys (catalog entries).
    //  and build array of catalog IDs to be passed to function in sporder.dll
    //

    for (iRegKey = 0; iRegKey < gNumRows; iRegKey++ ) {

        SendMessage (HWNDLISTCTL, LB_GETTEXT, iRegKey, (LPARAM) szBuffer);

        ASSERT (CatGetIndex (szBuffer, &iIndex, &iCatID),
                TEXT("CatDoWriteEntries, CatGetIndex failed."));

        //
        // build array of CatalogIDs
        //

        lpdwCatID[iRegKey] = iCatID;

    } // for


    r = WSCWriteProviderOrder (lpdwCatID, gNumRows);
    CatCheckRegErrCode (hwnd, r, TEXT("WSCWriteProviderOrder"));
    return r;
}






int CatCheckRegErrCode (HWND hwnd, LONG r, LPTSTR lpstr)
/*++

  Centralize checking the return code for Registry functions.
   Here we report the error if any with as helpful a message as we can.

--*/
{
    static TCHAR szTitle[] = TEXT("Registry error in service provider tool.");
    TCHAR szBuffer[MAX_STR];

    switch (r) {
        case ERROR_SUCCESS: return TRUE;
        break;

        case ERROR_ACCESS_DENIED : {
            lstrcpy (szBuffer, TEXT("ERROR_ACCESS_DENIED\n"));
            lstrcat (szBuffer, TEXT("You do not have the necessary privilege to call:\n"));
            lstrcat (szBuffer, lpstr);
            lstrcat (szBuffer, TEXT("\nLogon as Administrator."));

            MessageBox (hwnd, szBuffer, szTitle, MB_ICONSTOP | MB_OK);
            return FALSE;
        } break;

        //
        // As Keith & Intel change the registry format, they rename keys
        //  to avoid backward compatibility problems.  If we can't find
        //  the registry key, it is likely this EXE old and running against
        //  a new (incompatible) version of ws2.
        //

        case ERROR_FILE_NOT_FOUND : {
            lstrcpy (szBuffer, TEXT("ERROR_FILE_NOT_FOUND\n"));
            lstrcat (szBuffer, TEXT("You probably need an updated version of this tool.\n"));
            lstrcat (szBuffer, lpstr);

            MessageBox (hwnd, szBuffer, szTitle, MB_ICONSTOP | MB_OK);
            return FALSE;
        } break;


        case WSAEINVAL: {
            lstrcpy (szBuffer, TEXT("WinSock2 Registry format doesn't match \n"));
            lstrcat (szBuffer, TEXT("sporder [exe/dll]. You need updated tools. \n"));
            lstrcat (szBuffer, lpstr);

            MessageBox (hwnd, szBuffer, szTitle, MB_ICONSTOP | MB_OK);
            return FALSE;
        } break;


        default: {
            FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                           NULL,
                           r,
                           GetUserDefaultLangID(),
                           szBuffer,
                           sizeof (szBuffer),
                           0);

            lstrcat (szBuffer, TEXT("\n"));
            lstrcat (szBuffer, lpstr);
            MessageBox (hwnd, szBuffer, szTitle, MB_ICONSTOP | MB_OK);
            return FALSE;
        } break;

    }
    return TRUE;
}



int CatDoUpDown (HWND hwnd, WPARAM wParam)
/*++

  Given a dialog handle, and an up/down identifier, remove the entry, and
   reinsert it either one position up or down.

--*/
{
    TCHAR szBuffer[MAX_STR];
    DWORD iSelection;

    iSelection = (DWORD)SendMessage (HWNDLISTCTL, LB_GETCURSEL, 0, 0);

    if (iSelection != LB_ERR) {

        //
        // Read the current selecte string, delete the current selection, ...
        //

        SendMessage (HWNDLISTCTL, LB_GETTEXT, iSelection, (LPARAM)szBuffer);
        SendMessage (HWNDLISTCTL, LB_DELETESTRING, iSelection, 0);

        //
        // Adjust the position up or down by one, and make sure we are
        //  still clipped to within the valid range.
        //

        if (wParam == DID_UP) iSelection--;
        else iSelection++;

        if ((int) iSelection < 0) iSelection = 0 ;
        if ((int) iSelection >= gNumRows) iSelection = gNumRows-1 ;

        //
        // Re-insert the string and restore the selection
        //

        SendMessage (HWNDLISTCTL, LB_INSERTSTRING, iSelection, (LPARAM)szBuffer);
        SendMessage (HWNDLISTCTL, LB_SETCURSEL, iSelection, 0);
    }
    return TRUE;
}




BOOL CatGetIndex (LPTSTR szBuffer, LPINT lpIndex, LPINT lpCatID)
/*++

  The original index is stored after a tab stop, hidden from view far off
   screen to the right.  Parse the string for the tab stop, and read the
   next value.  The catalog ID is stored to the right of the index.


--*/
{
    int r;
    TCHAR *p;

    //
    // To get the index, start at the begining of the string, parse
    //  it for tokens based on tab as a separator, and take the
    //  second one.
    //

#ifdef UNICODE
    p = wcstok (szBuffer, TEXT("\t"));
    p = wcstok (NULL, TEXT("\t"));
    r = swscanf (p, TEXT("%d"), lpIndex);
    ASSERT((r == 1), TEXT("#1 ASSERT r == 1"))
    p = wcstok (NULL, TEXT("\t"));
    r = swscanf (p, TEXT("%d"), lpCatID);
    ASSERT((r == 1), TEXT("#2 ASSERT r == 1"))
#else
    p = strtok (szBuffer, TEXT("\t"));
    p = strtok (NULL, TEXT("\t"));
    r = sscanf (p, TEXT("%d"), lpIndex);
    ASSERT((r == 1), TEXT("#1 ASSERT r == 1"))
    p = strtok (NULL, TEXT("\t"));
    r = sscanf (p, TEXT("%d"), lpCatID);
    ASSERT((r == 1), TEXT("#2 ASSERT r == 1"))
#endif

    return TRUE;
}
