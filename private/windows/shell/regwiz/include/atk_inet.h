//  File	 : ATK_INET.h
//  Author       : Suresh Krishnan 
//  Date         : 08/05/97
//  Wrapper for INetCFG.DLL exported functions
//  related  function declarations
//
//
#ifndef __ATH_INET__
#define __ATK_INET__

#include <windows.h>
#include <tchar.h>
#include <winnt.h>
#include <wininet.h>
#include <stdio.h>
#include "rw_common.h"

HRESULT ATK_InetGetAutoDial(LPBOOL lpEnable, LPSTR lpszEntryName, DWORD cbEntryName);
HRESULT ATK_InetSetAutoDial(BOOL fEnable, LPCSTR lpszEntryName);
HRESULT ATK_InetConfigSystem( HWND hwndParent, DWORD dwfOptions,
			 LPBOOL lpfNeedsRestart);
HRESULT ATK_InetGetProxy( LPBOOL lpfEnable,
			  LPSTR  lpszServer,
			  DWORD  cbServer,
			  LPSTR  lpszOverride,
			  DWORD  cbOverride);

#endif