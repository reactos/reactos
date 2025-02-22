/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     WinRT Runtime Object API
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once
#define __ROAPI_H_

#include <sal.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _ROAPI_
   #define ROAPI
#else
   #define ROAPI DECLSPEC_IMPORT
#endif

typedef enum RO_INIT_TYPE
{
    RO_INIT_SINGLETHREADED = 0,
    RO_INIT_MULTITHREADED  = 1,
} RO_INIT_TYPE;

ROAPI
_Check_return_
HRESULT
WINAPI
RoInitialize(
    _In_ RO_INIT_TYPE initType);

ROAPI
void
WINAPI
RoUninitialize(void);

#ifdef __cplusplus
} // extern "C"
#endif
