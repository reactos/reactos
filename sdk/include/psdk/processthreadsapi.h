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

FORCEINLINE
HANDLE
GetCurrentThreadEffectiveToken(
    VOID)
{
    return (HANDLE)(LONG_PTR)-6;
}

#ifdef __cplusplus
} // extern "C"
#endif
