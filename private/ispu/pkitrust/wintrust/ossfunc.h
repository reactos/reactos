//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ossfunc.h
//
//--------------------------------------------------------------------------

#ifndef _OSS_FUNC_H
#define _OSS_FUNC_H

HRESULT WINAPI
ASNRegisterServer(LPCWSTR dllName);

HRESULT WINAPI
ASNUnregisterServer(void);

BOOL WINAPI
ASNDllMain(HMODULE hInst,
           ULONG  ulReason,
           LPVOID lpReserved);

#endif
