#ifndef _WINREG_H
#define _WINREG_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif
#define HKEY_CLASSES_ROOT	((HKEY)0x80000000)
#define HKEY_CURRENT_USER	((HKEY)0x80000001)
#define HKEY_LOCAL_MACHINE	((HKEY)0x80000002)
#define HKEY_USERS	((HKEY)0x80000003)
#define HKEY_PERFORMANCE_DATA	((HKEY)0x80000004)
#define HKEY_CURRENT_CONFIG	((HKEY)0x80000005)
#define HKEY_DYN_DATA	((HKEY)0x80000006)
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
LONG WINAPI RegCreateKeyA(HKEY,LPCSTR,PHKEY);
LONG WINAPI RegCreateKeyExA(HKEY,LPCSTR,DWORD,LPSTR,DWORD,REGSAM,LPSECURITY_ATTRIBUTES,PHKEY,PDWORD);
LONG WINAPI RegCreateKeyExW(HKEY,LPCWSTR,DWORD,LPWSTR,DWORD,REGSAM,LPSECURITY_ATTRIBUTES,PHKEY,PDWORD);
LONG WINAPI RegCreateKeyW(HKEY,LPCWSTR,PHKEY);
LONG WINAPI RegDeleteKeyA(HKEY,LPCSTR);
LONG WINAPI RegDeleteKeyW(HKEY,LPCWSTR);
LONG WINAPI RegDeleteValueA(HKEY,LPCSTR);
LONG WINAPI RegDeleteValueW(HKEY,LPCWSTR);
LONG WINAPI RegEnumKeyA(HKEY,DWORD,LPSTR,DWORD);
LONG WINAPI RegEnumKeyW(HKEY,DWORD,LPWSTR,DWORD);
LONG WINAPI RegEnumKeyExA(HKEY,DWORD,LPSTR,PDWORD,PDWORD,LPSTR,PDWORD,PFILETIME);
LONG WINAPI RegEnumKeyExW(HKEY,DWORD,LPWSTR,PDWORD,PDWORD,LPWSTR,PDWORD,PFILETIME);
LONG WINAPI RegEnumValueA(HKEY,DWORD,LPSTR,PDWORD,PDWORD,PDWORD,LPBYTE,PDWORD);
LONG WINAPI RegEnumValueW(HKEY,DWORD,LPWSTR,PDWORD,PDWORD,PDWORD,LPBYTE,PDWORD);
LONG WINAPI RegFlushKey(HKEY);
LONG WINAPI RegGetKeySecurity(HKEY,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,PDWORD);
LONG WINAPI RegLoadKeyA(HKEY,LPCSTR,LPCSTR);
LONG WINAPI RegLoadKeyW(HKEY,LPCWSTR,LPCWSTR);
LONG WINAPI RegNotifyChangeKeyValue(HKEY,BOOL,DWORD,HANDLE,BOOL);
LONG WINAPI RegOpenKeyA(HKEY,LPCSTR,PHKEY);
LONG WINAPI RegOpenKeyExA(HKEY,LPCSTR,DWORD,REGSAM,PHKEY);
LONG WINAPI RegOpenKeyExW(HKEY,LPCWSTR,DWORD,REGSAM,PHKEY);
LONG WINAPI RegOpenKeyW(HKEY,LPCWSTR,PHKEY);
LONG WINAPI RegQueryInfoKeyA(HKEY,LPSTR,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PFILETIME);
LONG WINAPI RegQueryInfoKeyW(HKEY,LPWSTR,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PDWORD,PFILETIME);
LONG WINAPI RegQueryMultipleValuesA(HKEY,PVALENTA,DWORD,LPSTR,LPDWORD);
LONG WINAPI RegQueryMultipleValuesW(HKEY,PVALENTW,DWORD,LPWSTR,LPDWORD);
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
#define RegCreateKey RegCreateKeyW
#define RegCreateKeyEx RegCreateKeyExW
#define RegDeleteKey RegDeleteKeyW
#define RegDeleteValue RegDeleteValueW
#define RegEnumKey RegEnumKeyW
#define RegEnumKeyEx RegEnumKeyExW
#define RegEnumValue RegEnumValueW
#define RegLoadKey RegLoadKeyW
#define RegOpenKey RegOpenKeyW
#define RegOpenKeyEx RegOpenKeyExW
#define RegQueryInfoKey RegQueryInfoKeyW
#define RegQueryMultipleValues RegQueryMultipleValuesW
#define RegQueryValue RegQueryValueW
#define RegQueryValueEx RegQueryValueExW
#define RegReplaceKey RegReplaceKeyW
#define RegRestoreKey RegRestoreKeyW
#define RegSaveKey RegSaveKeyW
#define RegSetValue RegSetValueW
#define RegSetValueEx RegSetValueExW
#define RegUnLoadKey RegUnLoadKeyW
#else
typedef VALENTA VALENT,*PVALENT;
#define AbortSystemShutdown AbortSystemShutdownA
#define InitiateSystemShutdown InitiateSystemShutdownA
#define RegConnectRegistry RegConnectRegistryA
#define RegCreateKey RegCreateKeyA
#define RegCreateKeyEx RegCreateKeyExA
#define RegDeleteKey RegDeleteKeyA
#define RegDeleteValue RegDeleteValueA
#define RegEnumKey RegEnumKeyA
#define RegEnumKeyEx RegEnumKeyExA
#define RegEnumValue RegEnumValueA
#define RegLoadKey RegLoadKeyA
#define RegOpenKey RegOpenKeyA
#define RegOpenKeyEx RegOpenKeyExA
#define RegQueryInfoKey RegQueryInfoKeyA
#define RegQueryMultipleValues RegQueryMultipleValuesA
#define RegQueryValue RegQueryValueA
#define RegQueryValueEx RegQueryValueExA
#define RegReplaceKey RegReplaceKeyA
#define RegRestoreKey RegRestoreKeyA
#define RegSaveKey RegSaveKeyA
#define RegSetValue RegSetValueA
#define RegSetValueEx RegSetValueExA
#define RegUnLoadKey RegUnLoadKeyA
#endif
#endif
#ifdef __cplusplus
}
#endif
#endif
