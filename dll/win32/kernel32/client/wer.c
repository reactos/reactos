/*
 * PROJECT:     VerOS NT System Libraries
 * LICENSE:     GNU Lesser General Public License, version 2.1 or later (LGPL-2.1-or-later)
 * FILE:        dll/win32/kernel32/client/wer.c
 * PURPOSE:     Windows Error Reporting registration helpers
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 */

/*
 * WerRegisterRuntimeExceptionModule / WerUnregisterRuntimeExceptionModule:
 * https://learn.microsoft.com/en-us/windows/win32/api/werapi/nf-werapi-werregisterruntimeexceptionmodule
 * https://learn.microsoft.com/en-us/windows/win32/api/werapi/nf-werapi-werunregisterruntimeexceptionmodule
 * Kernel32.dll, HRESULT, S_OK on success. Firefox PE import from mozglue.dll.
 * Full WER out-of-process callback plumbing is not wired; accept the registration.
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
HRESULT
WINAPI
WerRegisterRuntimeExceptionModule(IN PCWSTR pwszOutOfProcessCallbackDll,
                                   IN PVOID pContext OPTIONAL)
{
    UNREFERENCED_PARAMETER(pwszOutOfProcessCallbackDll);
    UNREFERENCED_PARAMETER(pContext);
    return S_OK;
}

/*
 * @implemented
 */
HRESULT
WINAPI
WerUnregisterRuntimeExceptionModule(IN PCWSTR pwszOutOfProcessCallbackDll,
                                     IN PVOID pContext OPTIONAL)
{
    UNREFERENCED_PARAMETER(pwszOutOfProcessCallbackDll);
    UNREFERENCED_PARAMETER(pContext);
    return S_OK;
}
