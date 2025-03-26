/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell objects header file
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <olectlid.h>
#include <exdispid.h>
#include "shdocvw.h"

#ifdef __cplusplus
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <undocshell.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <shlwapi_undoc.h>
#include <shdocvw_undoc.h>
#include <shdeprecated.h>
#include <shellutils.h>
#include <ui/rosctrls.h>
#include "CExplorerBand.h"
#include "CFavBand.h"
#include "utility.h"
void *operator new(size_t size);
void operator delete(void *ptr);
void operator delete(void *ptr, size_t size);
#endif /* def C++ */

EXTERN_C VOID SHDOCVW_Init(HINSTANCE hInstance);
EXTERN_C HRESULT SHDOCVW_DllCanUnloadNow(VOID);
EXTERN_C HRESULT SHDOCVW_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
EXTERN_C HRESULT SHDOCVW_DllRegisterServer(VOID);
EXTERN_C HRESULT SHDOCVW_DllUnregisterServer(VOID);
