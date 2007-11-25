#ifndef _WINREG_H
#define _WINREG_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define HKEY_CLASSES_ROOT ((HKEY)0x80000000)
#define HKEY_CURRENT_USER ((HKEY)0x80000001)
#define HKEY_LOCAL_MACHINE ((HKEY)0x80000002)
#define HKEY_USERS ((HKEY)0x80000003)
#define HKEY_PERFORMANCE_DATA ((HKEY)0x80000004)
#define HKEY_CURRENT_CONFIG ((HKEY)0x80000005)
#define HKEY_DYN_DATA ((HKEY)0x80000006)
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

#if (_WIN32_WINNT >= 0x0600)
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
#endif

#ifndef RC_INVOKED
typedef ACCESS_MASK REGSAM;
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
BOOL WINAPI AbortSystemShutdownA(LPCSTR);
BOOL WINAPI AbortSystemShutdownW(LPCWSTR);
BOOL WINAPI InitiateSystemShutdownA(LPSTR,LPSTR,DWORD,BOOL,BOOL);
BOOL WINAPI InitiateSystemShutdownW(LPWSTR,LPWSTR,DWORD,BOOL,BOOL);
LONG WINAPI RegCloseKey(HKEY);
LONG WINAPI RegConnectRegistryA(LPCSTR,HKEY,PHKEY);
LONG WINAPI RegConnectRegistryW(LPCWSTR,HKEY,PHKEY);
#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegCopyTreeA(HKEY,LPCSTR,HKEY);
LONG WINAPI RegCopyTreeW(HKEY,LPCWSTR,HKEY);
#endif
LONG WINAPI RegCreateKeyA(HKEY,LPCSTR,PHKEY);
LONG WINAPI RegCreateKeyExA(HKEY,LPCSTR,DWORD,LPSTR,DWORD,REGSAM,LPSECURITY_ATTRIBUTES,PHKEY,PDWORD);
LONG WINAPI RegCreateKeyExW(HKEY,LPCWSTR,DWORD,LPWSTR,DWORD,REGSAM,LPSECURITY_ATTRIBUTES,PHKEY,PDWORD);
LONG WINAPI RegCreateKeyW(HKEY,LPCWSTR,PHKEY);
LONG WINAPI RegDeleteKeyA(HKEY,LPCSTR);
#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegDeleteKeyValueA(HKEY,LPCSTR,LPCSTR);
LONG WINAPI RegDeleteKeyValueW(HKEY,LPCWSTR,LPCWSTR);
#endif
LONG WINAPI RegDeleteKeyW(HKEY,LPCWSTR);
#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegDeleteTreeA(HKEY,LPCSTR);
LONG WINAPI RegDeleteTreeW(HKEY,LPCWSTR);
#endif
LONG WINAPI RegDeleteValueA(HKEY,LPCSTR);
LONG WINAPI RegDeleteValueW(HKEY,LPCWSTR);
#if (_WIN32_WINNT >= 0x0500)
LONG WINAPI RegDisablePredefinedCache(VOID);
#endif
#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegDisablePredefinedCacheEx(VOID);
LONG WINAPI RegDisableReflectionKey(HKEY);
LONG WINAPI RegEnableReflectionKey(HKEY);
#endif
LONG WINAPI RegEnumKeyA(HKEY,DWORD,LPSTR,DWORD);
LONG WINAPI RegEnumKeyW(HKEY,DWORD,LPWSTR,DWORD);
LONG WINAPI RegEnumKeyExA(HKEY,DWORD,LPSTR,PDWORD,PDWORD,LPSTR,PDWORD,PFILETIME);
LONG WINAPI RegEnumKeyExW(HKEY,DWORD,LPWSTR,PDWORD,PDWORD,LPWSTR,PDWORD,PFILETIME);
LONG WINAPI RegEnumValueA(HKEY,DWORD,LPSTR,PDWORD,PDWORD,PDWORD,LPBYTE,PDWORD);
LONG WINAPI RegEnumValueW(HKEY,DWORD,LPWSTR,PDWORD,PDWORD,PDWORD,LPBYTE,PDWORD);
#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegGetValueA(HKEY,LPCSTR,LPCSTR,DWORD,LPDWORD,PVOID,LPDWORD);
LONG WINAPI RegGetValueW(HKEY,LPCWSTR,LPCWSTR,DWORD,LPDWORD,PVOID,LPDWORD);
#endif
LONG WINAPI RegFlushKey(HKEY);
LONG WINAPI RegGetKeySecurity(HKEY,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,PDWORD);
LONG WINAPI RegLoadKeyA(HKEY,LPCSTR,LPCSTR);
LONG WINAPI RegLoadKeyW(HKEY,LPCWSTR,LPCWSTR);
#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegLoadMUIStringA(HKEY,LPCSTR,LPSTR,DWORD,LPDWORD,DWORD,LPCSTR);
LONG WINAPI RegLoadMUIStringW(HKEY,LPCWSTR,LPWSTR,DWORD,LPDWORD,DWORD,LPCWSTR);
#endif
LONG WINAPI RegNotifyChangeKeyValue(HKEY,BOOL,DWORD,HANDLE,BOOL);
LONG WINAPI RegOpenCurrentUser(REGSAM,PHKEY);
LONG WINAPI RegOpenKeyA(HKEY,LPCSTR,PHKEY);
LONG WINAPI RegOpenKeyExA(HKEY,LPCSTR,DWORD,REGSAM,PHKEY);
LONG WINAPI RegOpenKeyExW(HKEY,LPCWSTR,DWORD,REGSAM,PHKEY);
LONG WINAPI RegOpenKeyW(HKEY,LPCWSTR,PHKEY);
LONG WINAPI RegQueryInfoKeyA(HKEY,LPSTR,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PFILETIME);
LONG WINAPI RegQueryInfoKeyW(HKEY,LPWSTR,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PFILETIME);
LONG WINAPI RegQueryMultipleValuesA(HKEY,PVALENTA,DWORD,LPSTR,LPDWORD);
LONG WINAPI RegQueryMultipleValuesW(HKEY,PVALENTW,DWORD,LPWSTR,LPDWORD);
#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegQueryReflectionKey(HKEY,BOOL*);
#endif
LONG WINAPI RegQueryValueA(HKEY,LPCSTR,LPSTR,PLONG);
LONG WINAPI RegQueryValueExA(HKEY,LPCSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
LONG WINAPI RegQueryValueExW(HKEY,LPCWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
LONG WINAPI RegQueryValueW(HKEY,LPCWSTR,LPWSTR,PLONG);
LONG WINAPI RegReplaceKeyA(HKEY,LPCSTR,LPCSTR,LPCSTR);
LONG WINAPI RegReplaceKeyW(HKEY,LPCWSTR,LPCWSTR,LPCWSTR);
LONG WINAPI RegRestoreKeyA(HKEY,LPCSTR,DWORD);
LONG WINAPI RegRestoreKeyW(HKEY,LPCWSTR,DWORD);
LONG WINAPI RegSaveKeyA(HKEY,LPCSTR,LPSECURITY_ATTRIBUTES);
LONG WINAPI RegSaveKeyW(HKEY,LPCWSTR,LPSECURITY_ATTRIBUTES);
LONG WINAPI RegSetKeySecurity(HKEY,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
#if (_WIN32_WINNT >= 0x0600)
LONG WINAPI RegSetKeyValueA(HKEY,LPCSTR,LPCSTR,DWORD,LPCVOID,DWORD);
LONG WINAPI RegSetKeyValueW(HKEY,LPCWSTR,LPCWSTR,DWORD,LPCVOID,DWORD);
#endif
LONG WINAPI RegSetValueA(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
LONG WINAPI RegSetValueExA(HKEY,LPCSTR,DWORD,DWORD,const BYTE*,DWORD);
LONG WINAPI RegSetValueExW(HKEY,LPCWSTR,DWORD,DWORD,const BYTE*,DWORD);
LONG WINAPI RegSetValueW(HKEY,LPCWSTR,DWORD,LPCWSTR,DWORD);
LONG WINAPI RegUnLoadKeyA(HKEY,LPCSTR);
LONG WINAPI RegUnLoadKeyW(HKEY,LPCWSTR);

#ifdef UNICODE
typedef VALENTW VALENT,*PVALENT;
#define AbortSystemShutdown AbortSystemShutdownW
#define InitiateSystemShutdown InitiateSystemShutdownW
#define RegConnectRegistry RegConnectRegistryW
#if (_WIN32_WINNT >= 0x0600)
#define RegCopyTree RegCopyTreeW
#endif
#define RegCreateKey RegCreateKeyW
#define RegCreateKeyEx RegCreateKeyExW
#define RegDeleteKey RegDeleteKeyW
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
#define RegConnectRegistry RegConnectRegistryA
#if (_WIN32_WINNT >= 0x0600)
#define RegCopyTree RegCopyTreeA
#endif
#define RegCreateKey RegCreateKeyA
#define RegCreateKeyEx RegCreateKeyExA
#define RegDeleteKey RegDeleteKeyA
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
