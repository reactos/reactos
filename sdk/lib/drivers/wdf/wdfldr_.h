/*
 * PROJECT:     Kernel Mode Device Framework
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Missing headers (wdfldr.h)
 * COPYRIGHT:   2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

// likely to be removed when the proper wdfldr.sys is ready

#ifndef _WDFLDR_H_
#define _WDFLDR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _LIBRARY_MODULE* PLIBRARY_MODULE;
typedef struct _WDF_LIBRARY_INFO* PWDF_LIBRARY_INFO;

typedef ULONG WDF_MAJOR_VERSION;
typedef ULONG WDF_MINOR_VERSION;
typedef ULONG WDF_BUILD_NUMBER;
typedef PVOID WDF_COMPONENT_GLOBALS, *PWDF_COMPONENT_GLOBALS;

typedef struct _WDF_INTERFACE_HEADER {
    const GUID *InterfaceType;
    ULONG InterfaceSize;
} WDF_INTERFACE_HEADER, *PWDF_INTERFACE_HEADER;

typedef struct _WDF_BIND_INFO *PWDF_BIND_INFO;

typedef NTSTATUS (STDCALL *PWDF_LDR_DIAGNOSTICS_VALUE_BY_NAME_AS_ULONG)(PUNICODE_STRING, PULONG);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _WDFLDR_H_
