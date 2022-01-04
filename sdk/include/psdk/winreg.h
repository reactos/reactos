#ifndef _WINREG_H
#define _WINREG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <reason.h>

#define HKEY_CLASSES_ROOT        ((HKEY)(LONG_PTR)(LONG)0x80000000)
#define HKEY_CURRENT_USER        ((HKEY)(LONG_PTR)(LONG)0x80000001)
#define HKEY_LOCAL_MACHINE       ((HKEY)(LONG_PTR)(LONG)0x80000002)
#define HKEY_USERS               ((HKEY)(LONG_PTR)(LONG)0x80000003)
#define HKEY_PERFORMANCE_DATA    ((HKEY)(LONG_PTR)(LONG)0x80000004)
#define HKEY_CURRENT_CONFIG      ((HKEY)(LONG_PTR)(LONG)0x80000005)
#define HKEY_DYN_DATA            ((HKEY)(LONG_PTR)(LONG)0x80000006)
#define HKEY_PERFORMANCE_TEXT    ((HKEY)(LONG_PTR)(LONG)0x80000050)
#define HKEY_PERFORMANCE_NLSTEXT ((HKEY)(LONG_PTR)(LONG)0x80000060)

#define REG_OPTION_VOLATILE 1
#define REG_OPTION_NON_VOLATILE 0
#define REG_CREATED_NEW_KEY 1
#define REG_OPENED_EXISTING_KEY 2
#define REG_NONE 0
#define REG_SZ 1
#define REG_EXPAND_SZ 2
#define REG_BINARY 3
#define REG_DWORD_LITTLE_ENDIAN 4
#define REG_DWORD 4
#define REG_DWORD_BIG_ENDIAN 5
#define REG_LINK 6
#define REG_MULTI_SZ 7
#define REG_RESOURCE_LIST 8
#define REG_FULL_RESOURCE_DESCRIPTOR 9
#define REG_RESOURCE_REQUIREMENTS_LIST 10
#define REG_QWORD_LITTLE_ENDIAN 11
#define REG_QWORD 11
#define REG_NOTIFY_CHANGE_NAME 1
#define REG_NOTIFY_CHANGE_ATTRIBUTES 2
#define REG_NOTIFY_CHANGE_LAST_SET 4
#define REG_NOTIFY_CHANGE_SECURITY 8

/* Shutdown flags for InitiateShutdownA/W */
#define SHUTDOWN_FORCE_OTHERS           0x00000001
#define SHUTDOWN_FORCE_SELF             0x00000002
#define SHUTDOWN_GRACE_OVERRIDE         0x00000020
#define SHUTDOWN_INSTALL_UPDATES        0x00000040
#define SHUTDOWN_NOREBOOT               0x00000010
#define SHUTDOWN_POWEROFF               0x00000008
#define SHUTDOWN_RESTART                0x00000004
#define SHUTDOWN_RESTARTAPPS            0x00000080

#define MAX_SHUTDOWN_TIMEOUT (10*365*24*60*60)

#define RRF_RT_REG_NONE         (1 << 0)
#define RRF_RT_REG_SZ           (1 << 1)
#define RRF_RT_REG_EXPAND_SZ    (1 << 2)
#define RRF_RT_REG_BINARY       (1 << 3)
#define RRF_RT_REG_DWORD        (1 << 4)
#define RRF_RT_REG_MULTI_SZ     (1 << 5)
#define RRF_RT_REG_QWORD        (1 << 6)
#define RRF_RT_DWORD            (RRF_RT_REG_BINARY | RRF_RT_REG_DWORD)
#define RRF_RT_QWORD            (RRF_RT_REG_BINARY | RRF_RT_REG_QWORD)
#define RRF_RT_ANY              (0x0000FFFF)
#define RRF_NOEXPAND            (1 << 28)
#define RRF_ZEROONFAILURE       (1 << 29)

#ifndef RC_INVOKED
typedef ACCESS_MASK REGSAM;
typedef _Return_type_success_(return==ERROR_SUCCESS) LONG LSTATUS;
typedef struct value_entA {
	LPSTR ve_valuename;
	DWORD ve_valuelen;
	DWORD ve_valueptr;
	DWORD ve_type;
} VALENTA,*PVALENTA;
typedef struct value_entW {
	LPWSTR ve_valuename;
	DWORD ve_valuelen;
	DWORD ve_valueptr;
	DWORD ve_type;
} VALENTW,*PVALENTW;

BOOL WINAPI AbortSystemShutdownA(_In_opt_ LPCSTR);
BOOL WINAPI AbortSystemShutdownW(_In_opt_ LPCWSTR);

#if (_WIN32_WINNT >= 0x0600)
DWORD WINAPI InitiateShutdownA(_In_opt_ LPSTR, _In_opt_ LPSTR, _In_ DWORD, _In_ DWORD, _In_ DWORD);
DWORD WINAPI InitiateShutdownW(_In_opt_ LPWSTR, _In_opt_ LPWSTR, _In_ DWORD, _In_ DWORD, _In_ DWORD);
#endif

BOOL WINAPI InitiateSystemShutdownA(_In_opt_ LPSTR, _In_opt_ LPSTR, _In_ DWORD, _In_ BOOL, _In_ BOOL);
BOOL WINAPI InitiateSystemShutdownW(_In_opt_ LPWSTR, _In_opt_ LPWSTR, _In_ DWORD, _In_ BOOL, _In_ BOOL);
BOOL WINAPI InitiateSystemShutdownExA(_In_opt_ LPSTR, _In_opt_ LPSTR, _In_ DWORD, _In_ BOOL, _In_ BOOL, _In_ DWORD);
BOOL WINAPI InitiateSystemShutdownExW(_In_opt_ LPWSTR, _In_opt_ LPWSTR, _In_ DWORD, _In_ BOOL, _In_ BOOL, _In_ DWORD);
LSTATUS WINAPI RegCloseKey(_In_ HKEY hKey);
LSTATUS WINAPI RegConnectRegistryA(_In_opt_ LPCSTR, _In_ HKEY, _Out_ PHKEY);
LSTATUS WINAPI RegConnectRegistryW(_In_opt_ LPCWSTR,_In_ HKEY, _Out_ PHKEY);

#if (_WIN32_WINNT >= 0x0600)
LSTATUS WINAPI RegCopyTreeA(_In_ HKEY, _In_opt_ LPCSTR, _In_ HKEY);
LSTATUS WINAPI RegCopyTreeW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ HKEY);
#endif

LSTATUS WINAPI RegCreateKeyA(_In_ HKEY hKey, _In_opt_ LPCSTR lpSubKey, _Out_ PHKEY phkResult);
LSTATUS WINAPI RegCreateKeyW(_In_ HKEY hKey, _In_opt_ LPCWSTR lpSubKey, _Out_ PHKEY phkResult);
LSTATUS WINAPI RegCreateKeyExA(_In_ HKEY hKey, _In_ LPCSTR lpSubKey, _Reserved_ DWORD Reserved, _In_opt_ LPSTR lpClass, _In_ DWORD dwOptions, _In_ REGSAM samDesired, _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes, _Out_ PHKEY phkResult, _Out_opt_ PDWORD lpdwDisposition);
LSTATUS WINAPI RegCreateKeyExW(_In_ HKEY hKey, _In_ LPCWSTR lpSubKey, _Reserved_ DWORD Reserved, _In_opt_ LPWSTR lpClass, _In_ DWORD dwOptions, _In_ REGSAM samDesired, _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes, _Out_ PHKEY phkResult, _Out_opt_ PDWORD lpdwDisposition);
LSTATUS WINAPI RegDeleteKeyA(_In_ HKEY hKey, _In_ LPCSTR lpSubKey);
LSTATUS WINAPI RegDeleteKeyW(_In_ HKEY hKey, _In_ LPCWSTR lpSubKey);
LSTATUS WINAPI RegDeleteKeyExA(_In_ HKEY hKey, _In_ LPCSTR lpSubKey, _In_ REGSAM samDesired, _Reserved_ DWORD Reserved);
LSTATUS WINAPI RegDeleteKeyExW(_In_ HKEY hKey, _In_ LPCWSTR lpSubKey, _In_ REGSAM samDesired, _Reserved_ DWORD Reserved);

#if (_WIN32_WINNT >= 0x0600)
LSTATUS WINAPI RegDeleteKeyValueA(_In_ HKEY, _In_opt_ LPCSTR, _In_opt_ LPCSTR);
LSTATUS WINAPI RegDeleteKeyValueW(_In_ HKEY, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR);
LSTATUS WINAPI RegDeleteTreeA(_In_ HKEY, _In_opt_ LPCSTR);
LSTATUS WINAPI RegDeleteTreeW(_In_ HKEY, _In_opt_ LPCWSTR);
#endif

LSTATUS WINAPI RegDeleteValueA(_In_ HKEY, _In_opt_ LPCSTR);
LSTATUS WINAPI RegDeleteValueW(_In_ HKEY, _In_opt_ LPCWSTR);

#if (_WIN32_WINNT >= 0x0500)
LONG WINAPI RegDisablePredefinedCache(VOID);
LSTATUS WINAPI RegSaveKeyExA(_In_ HKEY, _In_ LPCSTR, _In_opt_ LPSECURITY_ATTRIBUTES, _In_ DWORD);
LSTATUS WINAPI RegSaveKeyExW(_In_ HKEY, _In_ LPCWSTR, _In_opt_ LPSECURITY_ATTRIBUTES, _In_ DWORD);
#endif

#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegDisablePredefinedCacheEx(VOID);
LONG WINAPI RegDisableReflectionKey(_In_ HKEY);
LONG WINAPI RegEnableReflectionKey(_In_ HKEY);
#endif

LSTATUS
WINAPI
RegEnumKeyA(
  _In_ HKEY hKey,
  _In_ DWORD dwIndex,
  _Out_writes_opt_(cchName) LPSTR lpName,
  _In_ DWORD cchName);

LSTATUS
WINAPI
RegEnumKeyW(
  _In_ HKEY hKey,
  _In_ DWORD dwIndex,
  _Out_writes_opt_(cchName) LPWSTR lpName,
  _In_ DWORD cchName);

LSTATUS
WINAPI
RegEnumKeyExA(
  _In_ HKEY hKey,
  _In_ DWORD dwIndex,
  _Out_writes_to_opt_(*lpcchName, *lpcchName + 1) LPSTR lpName,
  _Inout_ LPDWORD lpcchName,
  _Reserved_ LPDWORD lpReserved,
  _Out_writes_to_opt_(*lpcchClass,*lpcchClass + 1) LPSTR lpClass,
  _Inout_opt_ LPDWORD lpcchClass,
  _Out_opt_ PFILETIME lpftLastWriteTime);

LSTATUS
WINAPI
RegEnumKeyExW(
  _In_ HKEY hKey,
  _In_ DWORD dwIndex,
  _Out_writes_to_opt_(*lpcchName, *lpcchName + 1) LPWSTR lpName,
  _Inout_ LPDWORD lpcchName,
  _Reserved_ LPDWORD lpReserved,
  _Out_writes_to_opt_(*lpcchClass,*lpcchClass + 1) LPWSTR lpClass,
  _Inout_opt_ LPDWORD lpcchClass,
  _Out_opt_ PFILETIME lpftLastWriteTime);

LSTATUS
WINAPI
RegEnumValueA(
  _In_ HKEY hKey,
  _In_ DWORD dwIndex,
  _Out_writes_to_opt_(*lpcchValueName, *lpcchValueName + 1) LPSTR lpValueName,
  _Inout_ LPDWORD lpcchValueName,
  _Reserved_ LPDWORD lpReserved,
  _Out_opt_ LPDWORD lpType,
  _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPBYTE lpData,
  _Inout_opt_ LPDWORD lpcbData);

LSTATUS
WINAPI
RegEnumValueW(
  _In_ HKEY hKey,
  _In_ DWORD dwIndex,
  _Out_writes_to_opt_(*lpcchValueName, *lpcchValueName + 1) LPWSTR lpValueName,
  _Inout_ LPDWORD lpcchValueName,
  _Reserved_ LPDWORD lpReserved,
  _Out_opt_ LPDWORD lpType,
  _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPBYTE lpData,
  _Inout_opt_ LPDWORD lpcbData);

#if (_WIN32_WINNT >= 0x0600)

LSTATUS
WINAPI
RegGetValueA(
  _In_ HKEY hkey,
  _In_opt_ LPCSTR lpSubKey,
  _In_opt_ LPCSTR lpValue,
  _In_ DWORD dwFlags,
  _Out_opt_ LPDWORD pdwType,
  _When_((dwFlags & 0x7F) == RRF_RT_REG_SZ || (dwFlags & 0x7F) == RRF_RT_REG_EXPAND_SZ ||
    (dwFlags & 0x7F) == (RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ) || *pdwType == REG_SZ ||
    *pdwType == REG_EXPAND_SZ, _Post_z_)
    _When_((dwFlags & 0x7F) == RRF_RT_REG_MULTI_SZ || *pdwType == REG_MULTI_SZ, _Post_ _NullNull_terminated_)
      _Out_writes_bytes_to_opt_(*pcbData,*pcbData) PVOID pvData,
  _Inout_opt_ LPDWORD pcbData);

LSTATUS
WINAPI
RegGetValueW(
  _In_ HKEY hkey,
  _In_opt_ LPCWSTR lpSubKey,
  _In_opt_ LPCWSTR lpValue,
  _In_ DWORD dwFlags,
  _Out_opt_ LPDWORD pdwType,
  _When_((dwFlags & 0x7F) == RRF_RT_REG_SZ || (dwFlags & 0x7F) == RRF_RT_REG_EXPAND_SZ ||
    (dwFlags & 0x7F) == (RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ) || *pdwType == REG_SZ ||
    *pdwType == REG_EXPAND_SZ, _Post_z_)
    _When_((dwFlags & 0x7F) == RRF_RT_REG_MULTI_SZ || *pdwType == REG_MULTI_SZ, _Post_ _NullNull_terminated_)
      _Out_writes_bytes_to_opt_(*pcbData,*pcbData) PVOID pvData,
  _Inout_opt_ LPDWORD pcbData);

#endif

LSTATUS WINAPI RegFlushKey(_In_ HKEY);

LSTATUS
WINAPI
RegGetKeySecurity(
  _In_ HKEY hKey,
  _In_ SECURITY_INFORMATION SecurityInformation,
  _Out_writes_bytes_opt_(*lpcbSecurityDescriptor) PSECURITY_DESCRIPTOR pSecurityDescriptor,
  _Inout_ LPDWORD lpcbSecurityDescriptor);

LSTATUS WINAPI RegLoadKeyA(_In_ HKEY, _In_opt_ LPCSTR, _In_ LPCSTR);
LSTATUS WINAPI RegLoadKeyW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ LPCWSTR);

#if (_WIN32_WINNT >= 0x0600)

LSTATUS
WINAPI
RegLoadMUIStringA(
  _In_ HKEY hKey,
  _In_opt_ LPCSTR pszValue,
  _Out_writes_bytes_opt_(cbOutBuf) LPSTR pszOutBuf,
  _In_ DWORD cbOutBuf,
  _Out_opt_ LPDWORD pcbData,
  _In_ DWORD Flags,
  _In_opt_ LPCSTR pszDirectory);

LSTATUS
WINAPI
RegLoadMUIStringW(
  _In_ HKEY hKey,
  _In_opt_ LPCWSTR pszValue,
  _Out_writes_bytes_opt_(cbOutBuf) LPWSTR pszOutBuf,
  _In_ DWORD cbOutBuf,
  _Out_opt_ LPDWORD pcbData,
  _In_ DWORD Flags,
  _In_opt_ LPCWSTR pszDirectory);

#endif

LSTATUS WINAPI RegNotifyChangeKeyValue(_In_ HKEY, _In_ BOOL, _In_ DWORD, _In_opt_ HANDLE, _In_ BOOL);
LSTATUS WINAPI RegOpenCurrentUser(_In_ REGSAM, _Out_ PHKEY);
LSTATUS WINAPI RegOpenKeyA(_In_ HKEY, _In_opt_ LPCSTR, _Out_ PHKEY);
LSTATUS WINAPI RegOpenKeyW(_In_ HKEY, _In_opt_ LPCWSTR, _Out_ PHKEY);
LSTATUS WINAPI RegOpenKeyExA(_In_ HKEY, _In_opt_ LPCSTR, _In_opt_ DWORD, _In_ REGSAM, _Out_ PHKEY);
LSTATUS WINAPI RegOpenKeyExW(_In_ HKEY, _In_opt_ LPCWSTR, _In_opt_ DWORD, _In_ REGSAM, _Out_ PHKEY);

LSTATUS
WINAPI
RegQueryInfoKeyA(
  _In_ HKEY hKey,
  _Out_writes_to_opt_(*lpcchClass, *lpcchClass + 1) LPSTR lpClass,
  _Inout_opt_ LPDWORD lpcchClass,
  _Reserved_ LPDWORD lpReserved,
  _Out_opt_ LPDWORD lpcSubKeys,
  _Out_opt_ LPDWORD lpcbMaxSubKeyLen,
  _Out_opt_ LPDWORD lpcbMaxClassLen,
  _Out_opt_ LPDWORD lpcValues,
  _Out_opt_ LPDWORD lpcbMaxValueNameLen,
  _Out_opt_ LPDWORD lpcbMaxValueLen,
  _Out_opt_ LPDWORD lpcbSecurityDescriptor,
  _Out_opt_ PFILETIME lpftLastWriteTime);

LSTATUS
WINAPI
RegQueryInfoKeyW(
  _In_ HKEY hKey,
  _Out_writes_to_opt_(*lpcchClass, *lpcchClass + 1) LPWSTR lpClass,
  _Inout_opt_ LPDWORD lpcchClass,
  _Reserved_ LPDWORD lpReserved,
  _Out_opt_ LPDWORD lpcSubKeys,
  _Out_opt_ LPDWORD lpcbMaxSubKeyLen,
  _Out_opt_ LPDWORD lpcbMaxClassLen,
  _Out_opt_ LPDWORD lpcValues,
  _Out_opt_ LPDWORD lpcbMaxValueNameLen,
  _Out_opt_ LPDWORD lpcbMaxValueLen,
  _Out_opt_ LPDWORD lpcbSecurityDescriptor,
  _Out_opt_ PFILETIME lpftLastWriteTime);

LSTATUS
WINAPI
RegQueryMultipleValuesA(
  _In_ HKEY hKey,
  _Out_writes_(num_vals) PVALENTA val_list,
  _In_ DWORD num_vals,
  _Out_writes_bytes_to_opt_(*ldwTotsize, *ldwTotsize) __out_data_source(REGISTRY) LPSTR lpValueBuf,
  _Inout_opt_ LPDWORD ldwTotsize);

LSTATUS
WINAPI
RegQueryMultipleValuesW(
  _In_ HKEY hKey,
  _Out_writes_(num_vals) PVALENTW val_list,
  _In_ DWORD num_vals,
  _Out_writes_bytes_to_opt_(*ldwTotsize, *ldwTotsize) __out_data_source(REGISTRY) LPWSTR lpValueBuf,
  _Inout_opt_ LPDWORD ldwTotsize);

#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegQueryReflectionKey(_In_ HKEY, _Out_ BOOL*);
#endif

LSTATUS
WINAPI
RegQueryValueA(
  _In_ HKEY hKey,
  _In_opt_ LPCSTR lpSubKey,
  _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPSTR lpData,
  _Inout_opt_ PLONG lpcbData);

LSTATUS
WINAPI
RegQueryValueW(
  _In_ HKEY hKey,
  _In_opt_ LPCWSTR lpSubKey,
  _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPWSTR lpData,
  _Inout_opt_ PLONG lpcbData);

LSTATUS
WINAPI
RegQueryValueExA(
  _In_ HKEY hKey,
  _In_opt_ LPCSTR lpValueName,
  _Reserved_ LPDWORD lpReserved,
  _Out_opt_ LPDWORD lpType,
  _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPBYTE lpData,
  _When_(lpData == NULL, _Out_opt_)
    _When_(lpData != NULL, _Inout_opt_) LPDWORD lpcbData);

LSTATUS
WINAPI
RegQueryValueExW(
  _In_ HKEY hKey,
  _In_opt_ LPCWSTR lpValueName,
  _Reserved_ LPDWORD lpReserved,
  _Out_opt_ LPDWORD lpType,
  _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) __out_data_source(REGISTRY) LPBYTE lpData,
  _When_(lpData == NULL, _Out_opt_)
    _When_(lpData != NULL, _Inout_opt_) LPDWORD lpcbData);

LSTATUS WINAPI RegReplaceKeyA(_In_ HKEY, _In_opt_ LPCSTR, _In_ LPCSTR, _In_ LPCSTR);
LSTATUS WINAPI RegReplaceKeyW(_In_ HKEY, _In_opt_ LPCWSTR, _In_ LPCWSTR, _In_ LPCWSTR);
LSTATUS WINAPI RegRestoreKeyA(_In_ HKEY, _In_ LPCSTR, _In_ DWORD);
LSTATUS WINAPI RegRestoreKeyW(_In_ HKEY, _In_ LPCWSTR, _In_ DWORD);
LSTATUS WINAPI RegSaveKeyA(_In_ HKEY, _In_ LPCSTR, _In_opt_ LPSECURITY_ATTRIBUTES);
LSTATUS WINAPI RegSaveKeyW(_In_ HKEY, _In_ LPCWSTR, _In_opt_ LPSECURITY_ATTRIBUTES);
LSTATUS WINAPI RegSetKeySecurity(_In_ HKEY, _In_ SECURITY_INFORMATION, _In_ PSECURITY_DESCRIPTOR);

#if (_WIN32_WINNT >= 0x0600)

LSTATUS
WINAPI
RegSetKeyValueA(
  _In_ HKEY hKey,
  _In_opt_ LPCSTR lpSubKey,
  _In_opt_ LPCSTR lpValueName,
  _In_ DWORD dwType,
  _In_reads_bytes_opt_(cbData) LPCVOID lpData,
  _In_ DWORD cbData);

LSTATUS
WINAPI
RegSetKeyValueW(
  _In_ HKEY hKey,
  _In_opt_ LPCWSTR lpSubKey,
  _In_opt_ LPCWSTR lpValueName,
  _In_ DWORD dwType,
  _In_reads_bytes_opt_(cbData) LPCVOID lpData,
  _In_ DWORD cbData);

#endif

LSTATUS
WINAPI
RegSetValueA(
  _In_ HKEY hKey,
  _In_opt_ LPCSTR lpSubKey,
  _In_ DWORD dwType,
  _In_reads_bytes_opt_(cbData) LPCSTR lpData,
  _In_ DWORD cbData);

LSTATUS
WINAPI
RegSetValueW(
  _In_ HKEY hKey,
  _In_opt_ LPCWSTR lpSubKey,
  _In_ DWORD dwType,
  _In_reads_bytes_opt_(cbData) LPCWSTR lpData,
  _In_ DWORD cbData);

LSTATUS
WINAPI
RegSetValueExA(
  _In_ HKEY hKey,
  _In_opt_ LPCSTR lpValueName,
  _Reserved_ DWORD Reserved,
  _In_ DWORD dwType,
  _In_reads_bytes_opt_(cbData) const BYTE *lpData,
  _In_ DWORD cbData);

LSTATUS
WINAPI
RegSetValueExW(
  _In_ HKEY hKey,
  _In_opt_ LPCWSTR lpValueName,
  _Reserved_ DWORD Reserved,
  _In_ DWORD dwType,
  _In_reads_bytes_opt_(cbData) const BYTE *lpData,
  _In_ DWORD cbData);

LSTATUS WINAPI RegUnLoadKeyA(_In_ HKEY, _In_opt_ LPCSTR);
LSTATUS WINAPI RegUnLoadKeyW(_In_ HKEY, _In_opt_ LPCWSTR);

#ifdef UNICODE
typedef VALENTW VALENT,*PVALENT;
#define AbortSystemShutdown AbortSystemShutdownW
#define InitiateSystemShutdown InitiateSystemShutdownW
#define InitiateSystemShutdownEx InitiateSystemShutdownExW
#define RegConnectRegistry RegConnectRegistryW
#if (_WIN32_WINNT >= 0x0600)
#define InitiateShutdown InitiateShutdownW
#define RegCopyTree RegCopyTreeW
#endif
#define RegCreateKey RegCreateKeyW
#define RegCreateKeyEx RegCreateKeyExW
#define RegDeleteKey RegDeleteKeyW
#define RegDeleteKeyEx RegDeleteKeyExW
#if (_WIN32_WINNT >= 0x0600)
#define RegDeleteKeyValue RegDeleteKeyValueW
#define RegDeleteTree RegDeleteTreeW
#endif
#define RegDeleteValue RegDeleteValueW
#define RegEnumKey RegEnumKeyW
#define RegEnumKeyEx RegEnumKeyExW
#define RegEnumValue RegEnumValueW
#if (_WIN32_WINNT >= 0x0600)
#define RegGetValue RegGetValueW
#endif
#define RegLoadKey RegLoadKeyW
#if (_WIN32_WINNT >= 0x0600)
#define RegLoadMUIString RegLoadMUIStringW
#endif
#define RegOpenKey RegOpenKeyW
#define RegOpenKeyEx RegOpenKeyExW
#define RegQueryInfoKey RegQueryInfoKeyW
#define RegQueryMultipleValues RegQueryMultipleValuesW
#define RegQueryValue RegQueryValueW
#define RegQueryValueEx RegQueryValueExW
#define RegReplaceKey RegReplaceKeyW
#define RegRestoreKey RegRestoreKeyW
#define RegSaveKey RegSaveKeyW
#define RegSaveKeyEx RegSaveKeyExW
#if (_WIN32_WINNT >= 0x0600)
#define RegSetKeyValue RegSetKeyValueW
#endif
#define RegSetValue RegSetValueW
#define RegSetValueEx RegSetValueExW
#define RegUnLoadKey RegUnLoadKeyW
#else
typedef VALENTA VALENT,*PVALENT;
#define AbortSystemShutdown AbortSystemShutdownA
#define InitiateSystemShutdown InitiateSystemShutdownA
#define InitiateSystemShutdownEx InitiateSystemShutdownExA
#define RegConnectRegistry RegConnectRegistryA
#if (_WIN32_WINNT >= 0x0600)
#define InitiateShutdown InitiateShutdownA
#define RegCopyTree RegCopyTreeA
#endif
#define RegCreateKey RegCreateKeyA
#define RegCreateKeyEx RegCreateKeyExA
#define RegDeleteKey RegDeleteKeyA
#define RegDeleteKeyEx RegDeleteKeyExA
#if (_WIN32_WINNT >= 0x0600)
#define RegDeleteKeyValue RegDeleteKeyValueA
#define RegDeleteTree RegDeleteTreeA
#endif
#define RegDeleteValue RegDeleteValueA
#define RegEnumKey RegEnumKeyA
#define RegEnumKeyEx RegEnumKeyExA
#define RegEnumValue RegEnumValueA
#if (_WIN32_WINNT >= 0x0600)
#define RegGetValue RegGetValueA
#endif
#define RegLoadKey RegLoadKeyA
#if (_WIN32_WINNT >= 0x0600)
#define RegLoadMUIString RegLoadMUIStringA
#endif
#define RegOpenKey RegOpenKeyA
#define RegOpenKeyEx RegOpenKeyExA
#define RegQueryInfoKey RegQueryInfoKeyA
#define RegQueryMultipleValues RegQueryMultipleValuesA
#define RegQueryValue RegQueryValueA
#define RegQueryValueEx RegQueryValueExA
#define RegReplaceKey RegReplaceKeyA
#define RegRestoreKey RegRestoreKeyA
#define RegSaveKey RegSaveKeyA
#define RegSaveKeyEx RegSaveKeyExA
#if (_WIN32_WINNT >= 0x0600)
#define RegSetKeyValue RegSetKeyValueA
#endif
#define RegSetValue RegSetValueA
#define RegSetValueEx RegSetValueExA
#define RegUnLoadKey RegUnLoadKeyA
#endif
#endif
#ifdef __cplusplus
}
#endif
#endif
