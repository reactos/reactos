/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     WinRT Runtime Object API
 * COPYRIGHT:   Copyright 2024 Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#pragma once
#define __ROAPI_H_

#include <sal.h>
#include <activation.h>

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

DECLARE_HANDLE(APARTMENT_SHUTDOWN_REGISTRATION_COOKIE);

#ifdef __cplusplus
typedef struct {} *RO_REGISTRATION_COOKIE;
#else
typedef struct _RO_REGISTRATION_COOKIE *RO_REGISTRATION_COOKIE;
#endif

ROAPI
_Check_return_
HRESULT
WINAPI
RoActivateInstance(
    _In_ HSTRING classid,
    _Out_ IInspectable **instance
);

ROAPI
_Check_return_
HRESULT
WINAPI
RoGetActivationFactory(
    _In_ HSTRING classid, 
    _In_ REFIID iid, 
    _Out_ void **class_factory
);

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

typedef HRESULT (WINAPI *PFNGETACTIVATIONFACTORY)(_In_ HSTRING classId, _Out_ IActivationFactory ** factory);

#ifdef __cplusplus
} // extern "C"
#endif
