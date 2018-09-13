#ifndef __ECM_UNICODE_H__
#define __ECM_UNICODE_H__

// necessary defns -- remove?
#include <rpc.h>
#include <rpcdce.h>
#include <wincrypt.h>

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI FIsWinNT(void);
BOOL WINAPI MkMBStrEx(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, int cchW, char ** pszMB, int *pcbConverted);
BOOL WINAPI MkMBStr(PBYTE pbBuff, DWORD cbBuff, LPCWSTR wsz, char ** pszMB);
void WINAPI FreeMBStr(PBYTE pbBuff, char * szMB);

LPWSTR WINAPI MkWStr(char * szMB);
void WINAPI FreeWStr(LPWSTR wsz);

// The following is also needed for non-x86 due to a bug in advapi32 for
// CryptAcquireContextW. 
BOOL WINAPI CryptAcquireContextU(
    HCRYPTPROV *phProv,
    LPCWSTR lpContainer,
    LPCWSTR lpProvider,
    DWORD dwProvType,
    DWORD dwFlags
    );


#ifdef _M_IX86

// Reg.cpp
LONG WINAPI RegCreateKeyExU (
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

LONG WINAPI RegDeleteKeyU(
    HKEY hKey,
    LPCWSTR lpSubKey
   );

LONG WINAPI RegEnumKeyExU (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcbName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    PFILETIME lpftLastWriteTime
   );	

LONG WINAPI RegEnumValueU (
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG RegQueryValueExU(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG WINAPI RegSetValueExU (
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    CONST BYTE* lpData,
    DWORD cbData
    );

LONG WINAPI RegDeleteValueU (
    HKEY hKey,
    LPCWSTR lpValueName
    );

LONG WINAPI RegQueryInfoKeyU (
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcbClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
    );

LONG WINAPI RegOpenKeyExU(
    HKEY hKey,
    LPCWSTR lpSubKey,	
    DWORD ulOptions,	
    REGSAM samDesired,	
    PHKEY phkResult 	
   );


// File.cpp
HANDLE WINAPI CreateFileU (
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

HINSTANCE WINAPI LoadLibraryU(
    LPCWSTR lpLibFileName
    );

HINSTANCE WINAPI LoadLibraryExU(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    );

DWORD
WINAPI
ExpandEnvironmentStringsU(
    LPCWSTR lpSrc,
    LPWSTR lpDst,
    DWORD nSize
    );


// capi.cpp
BOOL WINAPI CryptSignHashU(
    HCRYPTHASH hHash,
    DWORD dwKeySpec,
    LPCWSTR lpDescription,
    DWORD dwFlags,
    BYTE *pbSignature,
    DWORD *pdwSigLen
    );

BOOL WINAPI CryptVerifySignatureU(
    HCRYPTHASH hHash,
    CONST BYTE *pbSignature,
    DWORD dwSigLen,
    HCRYPTKEY hPubKey,
    LPCWSTR lpDescription,
    DWORD dwFlags
    );

BOOL WINAPI CryptSetProviderU(
    LPCWSTR lpProvName,
    DWORD dwProvType
    );

// Ole.cpp
RPC_STATUS RPC_ENTRY UuidToStringU( 
    UUID *  Uuid, 	
    WCHAR * *  StringUuid	
   );

// nt.cpp
BOOL WINAPI GetUserNameU(
    LPWSTR lpBuffer,
    LPDWORD nSize
   );	
 
DWORD WINAPI GetModuleFileNameU(
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
   );

HMODULE WINAPI GetModuleHandleU(
    LPCWSTR lpModuleName 	// address of module name to return handle for  
   );

// user.cpp
int WINAPI LoadStringU(
    HINSTANCE hInstance,
    UINT uID,
    LPWSTR lpBuffer, 
    int nBufferMax
   );

DWORD WINAPI FormatMessageU(
    DWORD dwFlags,
    LPCVOID lpSource,
    DWORD dwMessageId,
    DWORD dwLanguageId,
    LPWSTR lpBuffer,
    DWORD nSize,
    va_list *Arguments
   );

BOOL WINAPI SetWindowTextU(
    HWND hWnd,
    LPCWSTR lpString
   );

UINT WINAPI GetDlgItemTextU(
    HWND hDlg,
    int nIDDlgItem,
    LPWSTR lpString,
    int nMaxCount
   );	
                 
int WINAPI MessageBoxU(
    HWND hWnd ,
    LPCWSTR lpText,
    LPCWSTR lpCaption,
    UINT uType
    );

int WINAPI LCMapStringU(
    LCID Locale,
    DWORD dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,	
    LPWSTR lpDestStr,
    int cchDest
    );

#else

#define RegQueryValueExU	    RegQueryValueExW
#define RegCreateKeyExU         RegCreateKeyExW
#define RegDeleteKeyU           RegDeleteKeyW
#define RegEnumKeyExU           RegEnumKeyExW
#define RegEnumValueU           RegEnumValueW
#define RegSetValueExU          RegSetValueExW
#define RegQueryInfoKeyU        RegQueryInfoKeyW
#define RegDeleteValueU         RegDeleteValueW
#define RegOpenKeyExU           RegOpenKeyExW
#define ExpandEnvironmentStringsU ExpandEnvironmentStringsW

#define CreateFileU             CreateFileW
#define LoadLibraryU            LoadLibraryW
#define LoadLibraryExU          LoadLibraryExW

#define CryptSignHashU          CryptSignHashW
#define CryptVerifySignatureU   CryptVerifySignatureW
#define CryptSetProviderU       CryptSetProviderW

#define UuidToStringU           UuidToStringW

#define GetUserNameU            GetUserNameW
#define GetModuleFileNameU      GetModuleFileNameW
#define GetModuleHandleU        GetModuleHandleW

#define LoadStringU             LoadStringW
#define FormatMessageU          FormatMessageW
#define SetWindowTextU          SetWindowTextW
#define GetDlgItemTextU         GetDlgItemTextW
#define MessageBoxU				MessageBoxW
#define LCMapStringU            LCMapStringW

#endif // _M_IX86

#ifdef __cplusplus
}       // Balance extern "C" above
#endif

#endif
