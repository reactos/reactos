/*
 *
 * PROJECT:         fontext.dll
 * FILE:            dll/shellext/fontext/regsvr.c
 * PURPOSE:         fontext.dll
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * UPDATE HISTORY:
 *      10-06-2008  Created
 */

#include <windows.h>
#include <ole2.h>

#include <fontext.h>

static HRESULT
REGSVR_RegisterServer()
{
    return S_OK;
}

static HRESULT
REGSVR_UnregisterServer()
{
    return S_OK;
}

HRESULT WINAPI
DllRegisterServer(VOID)
{
    return REGSVR_RegisterServer();
}

HRESULT WINAPI
DllUnregisterServer(VOID)
{
    return REGSVR_UnregisterServer();
}
