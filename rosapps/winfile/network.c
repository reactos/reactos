/*
 *  ReactOS winfile
 *
 *  network.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include "main.h"
#include "network.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

static HMODULE hMPR;
static BOOL bNetAvailable = FALSE;

typedef DWORD (WINAPI *WNetCloseEnum_Ptr)(HANDLE);
typedef DWORD (WINAPI *WNetConnectionDialog_Ptr)(HWND, DWORD);
typedef DWORD (WINAPI *WNetDisconnectDialog_Ptr)(HWND, DWORD);
typedef DWORD (WINAPI *WNetConnectionDialog1_Ptr)(LPCONNECTDLGSTRUCT);
typedef DWORD (WINAPI *WNetDisconnectDialog1_Ptr)(LPDISCDLGSTRUCT);
typedef DWORD (WINAPI *WNetEnumResourceA_Ptr)(HANDLE, LPDWORD, LPVOID, LPDWORD);
typedef DWORD (WINAPI *WNetOpenEnumA_Ptr)(DWORD, DWORD, DWORD, LPNETRESOURCE, LPHANDLE);

static WNetCloseEnum_Ptr pWNetCloseEnum;
static WNetConnectionDialog_Ptr pWNetConnectionDialog;
static WNetDisconnectDialog_Ptr pWNetDisconnectDialog;
static WNetConnectionDialog1_Ptr pWNetConnectionDialog1;
static WNetDisconnectDialog1_Ptr pWNetDisconnectDialog1;
static WNetEnumResourceA_Ptr pWNetEnumResource;
static WNetOpenEnumA_Ptr pWNetOpenEnum;

BOOL CheckNetworkAvailable(void)
{

	hMPR = LoadLibrary(_T("MPR.DLL"));
    if (hMPR) {
        pWNetCloseEnum = (WNetCloseEnum_Ptr)(FARPROC)GetProcAddress(hMPR, "WNetCloseEnum");
        pWNetConnectionDialog = (WNetConnectionDialog_Ptr)(FARPROC)GetProcAddress(hMPR, "WNetConnectionDialog");
        pWNetDisconnectDialog = (WNetDisconnectDialog_Ptr)(FARPROC)GetProcAddress(hMPR, "WNetDisconnectDialog");
        pWNetConnectionDialog1 = (WNetConnectionDialog1_Ptr)(FARPROC)GetProcAddress(hMPR, "WNetConnectionDialog1");
        pWNetDisconnectDialog1 = (WNetDisconnectDialog1_Ptr)(FARPROC)GetProcAddress(hMPR, "WNetDisconnectDialog1");
        pWNetEnumResource = (WNetEnumResourceA_Ptr)(FARPROC)GetProcAddress(hMPR, "WNetEnumResourceA");
        pWNetOpenEnum = (WNetOpenEnumA_Ptr)(FARPROC)GetProcAddress(hMPR, "WNetOpenEnumA");
//    	FreeLibrary(hMPR);
        bNetAvailable = TRUE;
    }
    return (hMPR != NULL);
}


LRESULT CALLBACK EnumNetConnectionsProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return 0;
}

/*
DWORD WNetOpenEnum(
  DWORD dwScope,                // scope of enumeration
  DWORD dwType,                 // resource types to list
  DWORD dwUsage,                // resource usage to list
  LPNETRESOURCE lpNetResource,  // resource structure
  LPHANDLE lphEnum              // enumeration handle buffer
);

    result = WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_DISK, RESOURCEUSAGE_ALL, NULL, &EnumNetConnectionsProc);

 */
DWORD MapNetworkDrives(HWND hWnd, BOOL connect)
{
    DWORD result = 0L;

    if (!bNetAvailable) return result;
#if 1
    if (connect) {
        pWNetConnectionDialog(hWnd, RESOURCETYPE_DISK);
    } else {
        pWNetDisconnectDialog(hWnd, RESOURCETYPE_DISK);
    }
#else
    if (connect) {
        NETRESOURCE netResouce;
        CONNECTDLGSTRUCT connectDlg;

        //netResouce.dwScope; 
        //netResouce.dwType; 
        netResouce.dwDisplayType = 0;
        //netResouce.dwUsage; 
        //netResouce.lpLocalName; 
        //netResouce.lpRemoteName; 
        //netResouce.lpComment; 
        //netResouce.lpProvider; 

        //connectDlg.cbStructure;
        connectDlg.hwndOwner = hWnd;
        connectDlg.lpConnRes = &netResouce;
        //connectDlg.dwFlags;
        //connectDlg.dwDevNum;

        result = WNetConnectionDialog1(&connectDlg);
    } else {
        DISCDLGSTRUCT disconnectDlg;
        //disconnectDlg.cbStructure;
        disconnectDlg.hwndOwner = hWnd;
        //disconnectDlg.lpLocalName;
        //disconnectDlg.lpRemoteName;
        //disconnectDlg.dwFlags;
        result = pWNetDisconnectDialog1(&disconnectDlg);
    }
#endif
    return result;
}

////////////////////////////////////
void NetErrorHandler(HWND hwnd, DWORD dwResult, LPTSTR str)
{
}

void DisplayStruct(HDC hdc, LPNETRESOURCE lpnrLocal)
{
}

////////////////////////////////////

BOOL WINAPI EnumerateFunc(HWND hwnd, HDC hdc, LPNETRESOURCE lpnr)
{ 
  DWORD dwResult, dwResultEnum;
  HANDLE hEnum;
  DWORD cbBuffer = 16384;      // 16K is a good size
  DWORD cEntries = -1;         // enumerate all possible entries
  LPNETRESOURCE lpnrLocal;     // pointer to enumerated structures
  DWORD i;

  if (!bNetAvailable) return FALSE;

  // Call the WNetOpenEnum function to begin the enumeration.
  dwResult = pWNetOpenEnum(RESOURCE_GLOBALNET, // all network resources
                          RESOURCETYPE_ANY,   // all resources
                          0,        // enumerate all resources
                          lpnr,     // NULL first time the function is called
                          &hEnum);  // handle to the resource

  if (dwResult != NO_ERROR) {  
    // Process errors with an application-defined error handler.
    NetErrorHandler(hwnd, dwResult, (LPTSTR)_T("WNetOpenEnum"));
    return FALSE;
  }

  // Call the GlobalAlloc function to allocate resources.
  lpnrLocal = (LPNETRESOURCE)GlobalAlloc(GPTR, cbBuffer);
 
  do {  
    // Initialize the buffer.
    ZeroMemory(lpnrLocal, cbBuffer);

    // Call the WNetEnumResource function to continue the enumeration.
    dwResultEnum = pWNetEnumResource(hEnum,      // resource handle
                                    &cEntries,  // defined locally as -1
                                    lpnrLocal,  // LPNETRESOURCE
                                    &cbBuffer); // buffer size

    // If the call succeeds, loop through the structures.
    if (dwResultEnum == NO_ERROR) {
      for (i = 0; i < cEntries; i++) {
        // Call an application-defined function to display the contents of the NETRESOURCE structures.
        DisplayStruct(hdc, &lpnrLocal[i]);

        // If the NETRESOURCE structure represents a container resource, call the EnumerateFunc function recursively.
        if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage & RESOURCEUSAGE_CONTAINER))
            if (!EnumerateFunc(hwnd, hdc, &lpnrLocal[i])) {
                //TextOut(hdc, 10, 10, _T("EnumerateFunc returned FALSE."), 29);
            }
      }
    }
    // Process errors.
    else if (dwResultEnum != ERROR_NO_MORE_ITEMS) {
      NetErrorHandler(hwnd, dwResultEnum, (LPTSTR)_T("WNetEnumResource"));
      break;
    }
  }
  // End do.

  while (dwResultEnum != ERROR_NO_MORE_ITEMS);

  // Call the GlobalFree function to free the memory.
  GlobalFree((HGLOBAL)lpnrLocal);

  // Call WNetCloseEnum to end the enumeration.
  dwResult = pWNetCloseEnum(hEnum);
  
  if (dwResult != NO_ERROR) { 
    // Process errors.
    NetErrorHandler(hwnd, dwResult, (LPTSTR)_T("WNetCloseEnum"));
    return FALSE;
  }
  return TRUE;
}

/*

DWORD WNetConnectionDialog(
  HWND hwnd,     // handle to window owning dialog box
  DWORD dwType   // resource type
);


DWORD WNetAddConnection(
  LPCTSTR lpRemoteName, // network device name
  LPCTSTR lpPassword,   // password
  LPCTSTR lpLocalName   // local device name
);


 */
