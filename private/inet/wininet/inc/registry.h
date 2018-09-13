/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    registry.h

Abstract:

    Prototypes, etc., for dll\registry.c

Author:

    Richard L Firth (rfirth) 20-Mar-1995

Revision History:

    20-Mar-1995
        Created

--*/

#if defined(__cplusplus)
extern "C" {
#endif

#define INTERNET_POLICY_KEY         "SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

//
// prototypes
//

DWORD
OpenInternetSettingsKey(
    VOID
    );

DWORD
CloseInternetSettingsKey(
    VOID
    );

DWORD
GetMyEmailName(
    OUT LPSTR EmailName,
    IN OUT LPDWORD Length
    );

DWORD
GetMyDomainName(
    OUT LPSTR DomainName,
    IN OUT LPDWORD Length
    );

BOOL
GetFileExtensionFromMimeType(
    IN LPCSTR  lpszMimeType,
    IN DWORD   dwMimeLen,
    IN LPSTR   lpszFileExtension,
    IN OUT LPDWORD lpdwLen
    );

//DWORD
//InternetGetComputerName(
//    OUT LPSTR Buffer,
//    IN OUT LPDWORD Length
//    );

DWORD
InternetDeleteRegistryValue(
    IN LPSTR ParameterName
    );

DWORD
InternetReadRegistryDword(
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    );

DWORD
InternetCacheReadRegistryDword(
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    );

DWORD
InternetWriteRegistryDword(
    IN LPCSTR ParameterName,
    IN DWORD ParameterValue
    );

DWORD
InternetReadRegistryDwordKey(
    IN HKEY ParameterKey,
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    );

DWORD
InternetReadRegistryString(
    IN LPCSTR ParameterName,
    OUT LPSTR ParameterValue,
    IN OUT LPDWORD ParameterLength
    );

DWORD
ReadRegistryDword(
    IN HKEY Key,
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    );


DWORD
InternetWriteRegistryString(
    IN LPCSTR ParameterName,
    IN LPSTR ParameterValue
    );

//DWORD
//InternetReadRegistryBinary(
//    IN LPCSTR ParameterName,
//    OUT LPBYTE ParameterValue,
//    IN OUT LPDWORD ParameterLength
//    );

DWORD
CreateMimeExclusionTableForCache(VOID);

DWORD
CreateHeaderExclusionTableForCache(VOID);

VOID
DestroyMimeExclusionTableForCache(VOID);

VOID
DestroyHeaderExclusionTableForCache(VOID);

#if INET_DEBUG

VOID
DbgRegKey_Init(
    VOID
    );

VOID
DbgRegKey_Terminate(
    VOID
    );

LONG
DbgRegOpenKey(
    IN HKEY hKey,
    IN LPCTSTR lpszSubKey,
    OUT PHKEY phkResult,
    char * file,
    int line
    );

LONG
DbgRegOpenKeyEx(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult,
    char * file,
    int line
    );

LONG
DbgRegCreateKeyEx(
    IN HKEY hKey,
    IN LPCSTR lpSubKey,
    IN DWORD Reserved,
    IN LPSTR lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition,
    char * file,
    int line
    );

LONG
DbgRegCloseKey(
    IN HKEY hKey
    );

#define INITIALIZE_DEBUG_REGKEY() \
    DbgRegKey_Init()

#define TERMINATE_DEBUG_REGKEY() \
    DbgRegKey_Terminate()

#define REGOPENKEY(a, b, c) \
    DbgRegOpenKey((a), (b), (c), __FILE__, __LINE__)

#define REGOPENKEYEX(a, b, c, d, e) \
    DbgRegOpenKeyEx((a), (b), (c), (d), (e), __FILE__, __LINE__)

#define REGCREATEKEYEX(a, b, c, d, e, f, g, h, i) \
    DbgRegCreateKeyEx((a), (b), (c), (d), (e), (f), (g), (h), (i), __FILE__, __LINE__)

#define REGCLOSEKEY(a) \
    DbgRegCloseKey(a)

#else

#define INITIALIZE_DEBUG_REGKEY() \
    /* NOTHING */

#define TERMINATE_DEBUG_REGKEY() \
    /* NOTHING */

#define REGOPENKEY(a, b, c) \
    RegOpenKey((a), (b), (c))

#define REGOPENKEYEX(a, b, c, d, e) \
    RegOpenKeyEx((a), (b), (c), (d), (e))

#define REGCREATEKEYEX(a, b, c, d, e, f, g, h, i) \
    RegCreateKeyEx((a), (b), (c), (d), (e), (f), (g), (h), (i))

#define REGCLOSEKEY(a) \
    RegCloseKey(a)

#endif // INET_DEBUG

#if defined(__cplusplus)
}
#endif
