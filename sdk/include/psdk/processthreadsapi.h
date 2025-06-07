/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     API definitions for api-ms-win-core-processthreads-l1
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI
BOOL
WINAPI
SetThreadStackGuarantee(
    _Inout_ PULONG StackSizeInBytes);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
FORCEINLINE
HANDLE
GetCurrentProcessToken(
    VOID)
{
    return (HANDLE)(LONG_PTR)-4;
}

FORCEINLINE
HANDLE
GetCurrentThreadToken(
    VOID)
{
    return (HANDLE)(LONG_PTR)-5;
}

FORCEINLINE
HANDLE
GetCurrentThreadEffectiveToken(
    VOID)
{
    return (HANDLE)(LONG_PTR)-6;
}
#endif // (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

#ifdef __cplusplus
} // extern "C"
#endif
