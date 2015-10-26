#ifndef __REGUTILS_H__
#define __REGUTILS_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////
typedef LRESULT
(*PQUERY_REGISTRY_KEYS_ROUTINE)(IN HKEY    hRootKey,
                                IN LPCWSTR KeyName,
                                IN LPWSTR  SubKeyName,
                                IN HKEY    hOpenedSubKey,
                                IN PVOID   Context,
                                IN PVOID   EntryContext);

#define QUERY_REGISTRY_KEYS_ROUTINE(fnName)     \
    LRESULT (fnName)(IN HKEY    hRootKey,       \
                     IN LPCWSTR KeyName,        \
                     IN LPWSTR  SubKeyName,     \
                     IN HKEY    hOpenedSubKey,  \
                     IN PVOID   Context,        \
                     IN PVOID   EntryContext)

typedef struct __tagQUERY_REGISTRY_KEYS_TABLE
{
    PQUERY_REGISTRY_KEYS_ROUTINE QueryRoutine;
    PVOID EntryContext;
    // Other fields ?
} QUERY_REGISTRY_KEYS_TABLE, *PQUERY_REGISTRY_KEYS_TABLE;

LRESULT
RegQueryRegistryKeys(IN HKEY    hRootKey,
                     IN LPCWSTR KeyName,
                     IN PQUERY_REGISTRY_KEYS_TABLE QueryTable,
                     IN PVOID   Context);

////////////////////////////////////////////////////////////////////////////////
typedef LRESULT
(*PQUERY_REGISTRY_VALUES_ROUTINE)(IN HKEY    hRootKey,
                                  IN LPCWSTR KeyName,
                                  IN LPWSTR  ValueName,
                                  IN DWORD   ValueType,
                                  IN LPBYTE  ValueData,
                                  IN DWORD   ValueLength,
                                  IN PVOID   Context,
                                  IN PVOID   EntryContext);

#define QUERY_REGISTRY_VALUES_ROUTINE(fnName)   \
    LRESULT (fnName)(IN HKEY    hRootKey,       \
                     IN LPCWSTR KeyName,        \
                     IN LPWSTR  ValueName,      \
                     IN DWORD   ValueType,      \
                     IN LPBYTE  ValueData,      \
                     IN DWORD   ValueLength,    \
                     IN PVOID   Context,        \
                     IN PVOID   EntryContext)

typedef struct __tagQUERY_REGISTRY_VALUES_TABLE
{
    PQUERY_REGISTRY_VALUES_ROUTINE QueryRoutine;
    PVOID EntryContext;
    // Other fields ?
} QUERY_REGISTRY_VALUES_TABLE, *PQUERY_REGISTRY_VALUES_TABLE;

LRESULT
RegQueryRegistryValues(IN HKEY    hRootKey,
                       IN LPCWSTR KeyName,
                       IN PQUERY_REGISTRY_VALUES_TABLE QueryTable,
                       IN PVOID   Context);

////////////////////////////////////////////////////////////////////////////////

LONG
RegGetDWORDValue(IN  HKEY    hKey,
                 IN  LPCWSTR lpSubKey OPTIONAL,
                 IN  LPCWSTR lpValue  OPTIONAL,
                 OUT LPDWORD lpData   OPTIONAL);

LONG
RegSetDWORDValue(IN HKEY    hKey,
                 IN LPCWSTR lpSubKey OPTIONAL,
                 IN LPCWSTR lpValue  OPTIONAL,
                 IN BOOL    bCreateKeyIfDoesntExist,
                 IN DWORD   dwData);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __REGUTILS_H__
