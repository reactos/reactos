/*++ BUILD Version: 0001    // Increment this if a change has global effects    ;both
                                                                                ;both
Copyright (c) 1991-1998 Microsoft Corporation                                   ;both
                                                                                ;both
Module Name:                                                                    ;both
                                                                                ;both
    Winreg.h
    Winregp.h                                                                   ;internal_NT
                                                                                ;both
Abstract:                                                                       ;both
                                                                                ;both
    This module contains the function prototypes and constant, type and         ;both
    structure definitions for the Windows 32-Bit Registry API.                  ;both
                                                                                ;both
--*/                                                                            ;both

#ifndef _WINREG_
#define _WINREG_

#ifndef _WINREGP_       ;internal_NT
#define _WINREGP_       ;internal_NT

#ifdef _MAC
#include <macwin32.h>
#endif

;begin_both
#ifdef __cplusplus
extern "C" {
#endif
;end_both

#ifndef WINVER                          ;public_win40
#define WINVER 0x0500   // version 5.0  ;public_win40
#endif /* !WINVER */                    ;public_win40

//
// Requested Key access mask type.
//

typedef ACCESS_MASK REGSAM;

//
// Reserved Key Handles.
//

#define HKEY_CLASSES_ROOT           (( HKEY ) (ULONG_PTR)((LONG)0x80000000) )
#define HKEY_CURRENT_USER           (( HKEY ) (ULONG_PTR)((LONG)0x80000001) )
#define HKEY_LOCAL_MACHINE          (( HKEY ) (ULONG_PTR)((LONG)0x80000002) )
#define HKEY_USERS                  (( HKEY ) (ULONG_PTR)((LONG)0x80000003) )
#define HKEY_PERFORMANCE_DATA       (( HKEY ) (ULONG_PTR)((LONG)0x80000004) )
#define HKEY_PERFORMANCE_TEXT       (( HKEY ) (ULONG_PTR)((LONG)0x80000050) )                      ;internal_NT
#define HKEY_PERFORMANCE_NLSTEXT    (( HKEY ) (ULONG_PTR)((LONG)0x80000060) )                      ;internal_NT
;begin_winver_400
#define HKEY_CURRENT_CONFIG         (( HKEY ) (ULONG_PTR)((LONG)0x80000005) )
#define HKEY_DYN_DATA               (( HKEY ) (ULONG_PTR)((LONG)0x80000006) )

/*NOINC*/
#ifndef _PROVIDER_STRUCTS_DEFINED
#define _PROVIDER_STRUCTS_DEFINED

#define PROVIDER_KEEPS_VALUE_LENGTH 0x1
struct val_context {
    int valuelen;       // the total length of this value
    LPVOID value_context;   // provider's context
    LPVOID val_buff_ptr;    // where in the ouput buffer the value is.
};

typedef struct val_context FAR *PVALCONTEXT;

typedef struct pvalue% {           // Provider supplied value/context.
    LPTSTR% pv_valuename;          // The value name pointer
    int pv_valuelen;
    LPVOID pv_value_context;
    DWORD pv_type;
}PVALUE%, FAR *PPVALUE%;

typedef
DWORD _cdecl
QUERYHANDLER (LPVOID keycontext, PVALCONTEXT val_list, DWORD num_vals,
          LPVOID outputbuffer, DWORD FAR *total_outlen, DWORD input_blen);

typedef QUERYHANDLER FAR *PQUERYHANDLER;

typedef struct provider_info {
    PQUERYHANDLER pi_R0_1val;
    PQUERYHANDLER pi_R0_allvals;
    PQUERYHANDLER pi_R3_1val;
    PQUERYHANDLER pi_R3_allvals;
    DWORD pi_flags;    // capability flags (none defined yet).
    LPVOID pi_key_context;
}REG_PROVIDER;

typedef struct provider_info FAR *PPROVIDER;

typedef struct value_ent% {
    LPTSTR% ve_valuename;
    DWORD ve_valuelen;
    DWORD_PTR ve_valueptr;
    DWORD ve_type;
}VALENT%, FAR *PVALENT%;

#endif // not(_PROVIDER_STRUCTS_DEFINED)
/*INC*/

;end_winver_400

//
// Default values for parameters that do not exist in the Win 3.1
// compatible APIs.
//

#define WIN31_CLASS                 NULL

//
// API Prototypes.
//


WINADVAPI
LONG
APIENTRY
RegCloseKey (
    IN HKEY hKey
    );

WINADVAPI
LONG
APIENTRY
RegOverridePredefKey (
    IN HKEY hKey,
    IN HKEY hNewHKey
    );

WINADVAPI
LONG
APIENTRY
RegOpenUserClassesRoot(
    HANDLE hToken,
    DWORD  dwOptions,
    REGSAM samDesired,
    PHKEY  phkResult
    );

WINADVAPI
LONG
APIENTRY
RegOpenCurrentUser(
    REGSAM samDesired,
    PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegDisablePredefinedCache(
    );

WINADVAPI
LONG
APIENTRY
RegConnectRegistry% (
    IN LPCTSTR% lpMachineName,
    IN HKEY hKey,
    OUT PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegCreateKey% (
    IN HKEY hKey,
    IN LPCTSTR% lpSubKey,
    OUT PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegCreateKeyEx% (
    IN HKEY hKey,
    IN LPCTSTR% lpSubKey,
    IN DWORD Reserved,
    IN LPTSTR% lpClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD lpdwDisposition
    );

WINADVAPI
LONG
APIENTRY
RegDeleteKey% (
    IN HKEY hKey,
    IN LPCTSTR% lpSubKey
    );

WINADVAPI
LONG
APIENTRY
RegDeleteValue% (
    IN HKEY hKey,
    IN LPCTSTR% lpValueName
    );

WINADVAPI
LONG
APIENTRY
RegEnumKey% (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPTSTR% lpName,
    IN DWORD cbName
    );

WINADVAPI
LONG
APIENTRY
RegEnumKeyEx% (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPTSTR% lpName,
    IN OUT LPDWORD lpcbName,
    IN LPDWORD lpReserved,
    IN OUT LPTSTR% lpClass,
    IN OUT LPDWORD lpcbClass,
    OUT PFILETIME lpftLastWriteTime
    );

WINADVAPI
LONG
APIENTRY
RegEnumValue% (
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT LPTSTR% lpValueName,
    IN OUT LPDWORD lpcbValueName,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

WINADVAPI
LONG
APIENTRY
RegFlushKey (
    IN HKEY hKey
    );

WINADVAPI
LONG
APIENTRY
RegGetKeySecurity (
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN OUT LPDWORD lpcbSecurityDescriptor
    );

WINADVAPI
LONG
APIENTRY
RegLoadKey% (
    IN HKEY    hKey,
    IN LPCTSTR%  lpSubKey,
    IN LPCTSTR%  lpFile
    );

WINADVAPI
LONG
APIENTRY
RegNotifyChangeKeyValue (
    IN HKEY hKey,
    IN BOOL bWatchSubtree,
    IN DWORD dwNotifyFilter,
    IN HANDLE hEvent,
    IN BOOL fAsynchronus
    );

WINADVAPI
LONG
APIENTRY
RegOpenKey% (
    IN HKEY hKey,
    IN LPCTSTR% lpSubKey,
    OUT PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegOpenKeyEx% (
    IN HKEY hKey,
    IN LPCTSTR% lpSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    );

WINADVAPI
LONG
APIENTRY
RegQueryInfoKey% (
    IN HKEY hKey,
    OUT LPTSTR% lpClass,
    IN OUT LPDWORD lpcbClass,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcbMaxSubKeyLen,
    OUT LPDWORD lpcbMaxClassLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcbMaxValueNameLen,
    OUT LPDWORD lpcbMaxValueLen,
    OUT LPDWORD lpcbSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    );

WINADVAPI
LONG
APIENTRY
RegQueryValue% (
    IN HKEY hKey,
    IN LPCTSTR% lpSubKey,
    OUT LPTSTR% lpValue,
    IN OUT PLONG   lpcbValue
    );

;begin_winver_400
WINADVAPI
LONG
APIENTRY
RegQueryMultipleValues% (
    IN HKEY hKey,
    OUT PVALENT% val_list,
    IN DWORD num_vals,
    OUT LPTSTR% lpValueBuf,
    IN OUT LPDWORD ldwTotsize
    );
;end_winver_400

WINADVAPI
LONG
APIENTRY
RegQueryValueEx% (
    IN HKEY hKey,
    IN LPCTSTR% lpValueName,
    IN LPDWORD lpReserved,
    OUT LPDWORD lpType,
    IN OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

WINADVAPI
LONG
APIENTRY
RegReplaceKey% (
    IN HKEY     hKey,
    IN LPCTSTR%  lpSubKey,
    IN LPCTSTR%  lpNewFile,
    IN LPCTSTR%  lpOldFile
    );

WINADVAPI
LONG
APIENTRY
RegRestoreKey% (
    IN HKEY hKey,
    IN LPCTSTR% lpFile,
    IN DWORD   dwFlags
    );

WINADVAPI
LONG
APIENTRY
RegSaveKey% (
    IN HKEY hKey,
    IN LPCTSTR% lpFile,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINADVAPI
LONG
APIENTRY
RegSetKeySecurity (
    IN HKEY hKey,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );

WINADVAPI
LONG
APIENTRY
RegSetValue% (
    IN HKEY hKey,
    IN LPCTSTR% lpSubKey,
    IN DWORD dwType,
    IN LPCTSTR% lpData,
    IN DWORD cbData
    );


WINADVAPI
LONG
APIENTRY
RegSetValueEx% (
    IN HKEY hKey,
    IN LPCTSTR% lpValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    );

WINADVAPI
LONG
APIENTRY
RegUnLoadKey% (
    IN HKEY    hKey,
    IN LPCTSTR% lpSubKey
    );

//
// Remoteable System Shutdown APIs
//

WINADVAPI
BOOL
APIENTRY
InitiateSystemShutdown%(
    IN LPTSTR% lpMachineName,
    IN LPTSTR% lpMessage,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown
    );


WINADVAPI
BOOL
APIENTRY
AbortSystemShutdown%(
    IN LPTSTR% lpMachineName
    );

//
// defines for InitiateSystemShutdownEx reason codes
//

#define REASON_SWINSTALL    0
#define REASON_HWINSTALL    1
#define REASON_SERVICEHANG  2
#define REASON_UNSTABLE     3
#define REASON_SWHWRECONF   4
#define REASON_OTHER        5
#define REASON_UNKNOWN      255
#define REASON_PLANNED_FLAG 0x80000000

WINADVAPI
BOOL
APIENTRY
InitiateSystemShutdownEx%(
    IN LPTSTR% lpMachineName,
    IN LPTSTR% lpMessage,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown,
    IN DWORD dwReason
    );

;begin_both
#ifdef __cplusplus
}
#endif
;end_both

#endif // _WINREGP_       ;internal_NT

#endif // _WINREG_
