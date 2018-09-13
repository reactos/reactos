//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dll.h
//
//--------------------------------------------------------------------------

#ifndef __dll_h
#define __dll_h

extern HINSTANCE g_hInstance;
#define GLOBAL_HINSTANCE (g_hInstance)

HRESULT ThreadCoInitialize(void);
void ThreadCoUninitialize(void);
DWORD WaitOnThread(HANDLE *phThread);

HRESULT _CreateSecurityPage(LPSECURITYINFO pSI, HPROPSHEETPAGE *phPage);
HRESULT _EditSecurity(HWND hwndOwner, LPSECURITYINFO pSI);

#endif
