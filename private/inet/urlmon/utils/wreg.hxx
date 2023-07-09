#include "wreg.h"

class CRegistryW : public IRegistryW
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // can this method be supported
    STDMETHODIMP_(LONG) ConnectRegistry (
         LPWSTR lpMachineName,
         HKEY hKey,
         IRegistryW **ppReg
        );

    STDMETHODIMP_(LONG) CloseKey (
         HKEY hKey
        );

    STDMETHODIMP_(LONG) CreateKey (
         HKEY hKey,
         LPCWSTR lpSubKey,
         PHKEY phkResult
        );

    STDMETHODIMP_(LONG) CreateKeyEx (
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

    STDMETHODIMP_(LONG) DeleteKey (
         HKEY hKey,
         LPCWSTR lpSubKey
        );

    STDMETHODIMP_(LONG) DeleteValue (
         HKEY hKey,
         LPCWSTR lpValueName
        );

    STDMETHODIMP_(LONG) EnumKey (
         HKEY hKey,
         DWORD dwIndex,
         LPWSTR lpName,
         DWORD cbName
        );

    STDMETHODIMP_(LONG) EnumKeyEx (
         HKEY hKey,
         DWORD dwIndex,
         LPWSTR lpName,
         LPDWORD lpcbName,
         LPDWORD lpReserved,
         LPWSTR lpClass,
         LPDWORD lpcbClass,
         PFILETIME lpftLastWriteTime
        );

    STDMETHODIMP_(LONG) EnumValue (
         HKEY hKey,
         DWORD dwIndex,
         LPWSTR lpValueName,
         LPDWORD lpcbValueName,
         LPDWORD lpReserved,
         LPDWORD lpType,
         LPBYTE lpData,
         LPDWORD lpcbData
        );

    STDMETHODIMP_(LONG) FlushKey (
        HKEY hKey
        );

    STDMETHODIMP_(LONG) GetKeySecurity (
         HKEY hKey,
         SECURITY_INFORMATION SecurityInformation,
         PSECURITY_DESCRIPTOR pSecurityDescriptor,
         LPDWORD lpcbSecurityDescriptor
        );

    STDMETHODIMP_(LONG) LoadKey (
         HKEY  hKey,
         LPCWSTR  lpSubKey,
         LPCWSTR  lpFile
        );

    STDMETHODIMP_(LONG) NotifyChangeKeyValue (
         HKEY hKey,
         BOOL bWatchSubtree,
         DWORD dwNotifyFilter,
         HANDLE hEvent,
         BOOL fAsynchronus
        );

    STDMETHODIMP_(LONG) OpenKey (
         HKEY hKey,
         LPCWSTR lpSubKey,
         PHKEY phkResult
        );

    STDMETHODIMP_(LONG) OpenKeyEx (
         HKEY hKey,
         LPCWSTR lpSubKey,
         DWORD ulOptions,
         REGSAM samDesired,
         PHKEY phkResult
        );

    STDMETHODIMP_(LONG) QueryInfoKey (
         HKEY    hKey,
         LPWSTR  lpClass,
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

    STDMETHODIMP_(LONG) QueryValue (
         HKEY hKey,
         LPCWSTR lpSubKey,
         LPWSTR lpValue,
         LONG   *lpcbValue
        );

    STDMETHODIMP_(LONG) QueryMultipleValues (
         HKEY hKey,
         PVALENTXW val_list,
         DWORD num_vals,
         LPWSTR lpValueBuf,
         LPDWORD ldwTotsize
        );

    STDMETHODIMP_(LONG) QueryValueEx (
         HKEY hKey,
         LPCWSTR lpValueName,
         LPDWORD lpReserved,
         LPDWORD lpType,
         LPBYTE lpData,
         LPDWORD lpcbData
        );

    STDMETHODIMP_(LONG) ReplaceKey (
         HKEY     hKey,
         LPCWSTR  lpSubKey,
         LPCWSTR  lpNewFile,
         LPCWSTR  lpOldFile
        );

    STDMETHODIMP_(LONG) RestoreKey (
         HKEY hKey,
         LPCWSTR lpFile,
         DWORD   dwFlags
        );

    STDMETHODIMP_(LONG) SaveKey (
         HKEY hKey,
         LPCWSTR lpFile,
         LPSECURITY_ATTRIBUTES lpSecurityAttributes
        );

    STDMETHODIMP_(LONG) SetKeySecurity (
         HKEY hKey,
         SECURITY_INFORMATION SecurityInformation,
         PSECURITY_DESCRIPTOR pSecurityDescriptor
        );

    STDMETHODIMP_(LONG) SetValue (
         HKEY hKey,
         LPCWSTR lpSubKey,
         DWORD dwType,
         LPCWSTR lpData,
         DWORD cbData
        );

    STDMETHODIMP_(LONG) SetValueEx (
         HKEY hKey,
         LPCWSTR lpValueName,
         DWORD Reserved,
         DWORD dwType,
         const BYTE* lpData,
         DWORD cbData
        );

    STDMETHODIMP_(LONG) UnLoadKey (
         HKEY hKey,
         LPCWSTR lpSubKey
        );

public:

private:
    CRefCount           _CRefs;          // the total refcount of this object

};



class CRegistryA : public IRegistryA
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // can this method be supported
    STDMETHODIMP_(LONG) ConnectRegistry (
         LPSTR lpMachineName,
         HKEY hKey,
         IRegistryA **ppReg
        );

    STDMETHODIMP_(LONG) CloseKey (
         HKEY hKey
        );

    STDMETHODIMP_(LONG) CreateKey (
         HKEY hKey,
         LPCSTR lpSubKey,
         PHKEY phkResult
        );

    STDMETHODIMP_(LONG) CreateKeyEx (
         HKEY hKey,
         LPCSTR lpSubKey,
         DWORD Reserved,
         LPSTR lpClass,
         DWORD dwOptions,
         REGSAM samDesired,
         LPSECURITY_ATTRIBUTES lpSecurityAttributes,
         PHKEY phkResult,
         LPDWORD lpdwDisposition
        );

    STDMETHODIMP_(LONG) DeleteKey (
         HKEY hKey,
         LPCSTR lpSubKey
        );

    STDMETHODIMP_(LONG) DeleteValue (
         HKEY hKey,
         LPCSTR lpValueName
        );

    STDMETHODIMP_(LONG) EnumKey (
         HKEY hKey,
         DWORD dwIndex,
         LPSTR lpName,
         DWORD cbName
        );

    STDMETHODIMP_(LONG) EnumKeyEx (
         HKEY hKey,
         DWORD dwIndex,
         LPSTR lpName,
         LPDWORD lpcbName,
         LPDWORD lpReserved,
         LPSTR lpClass,
         LPDWORD lpcbClass,
         PFILETIME lpftLastWriteTime
        );

    STDMETHODIMP_(LONG) EnumValue (
         HKEY hKey,
         DWORD dwIndex,
         LPSTR lpValueName,
         LPDWORD lpcbValueName,
         LPDWORD lpReserved,
         LPDWORD lpType,
         LPBYTE lpData,
         LPDWORD lpcbData
        );

    STDMETHODIMP_(LONG) FlushKey (
        HKEY hKey
        );

    STDMETHODIMP_(LONG) GetKeySecurity (
         HKEY hKey,
         SECURITY_INFORMATION SecurityInformation,
         PSECURITY_DESCRIPTOR pSecurityDescriptor,
         LPDWORD lpcbSecurityDescriptor
        );

    STDMETHODIMP_(LONG) LoadKey (
         HKEY  hKey,
         LPCSTR  lpSubKey,
         LPCSTR  lpFile
        );

    STDMETHODIMP_(LONG) NotifyChangeKeyValue (
         HKEY hKey,
         BOOL bWatchSubtree,
         DWORD dwNotifyFilter,
         HANDLE hEvent,
         BOOL fAsynchronus
        );

    STDMETHODIMP_(LONG) OpenKey (
         HKEY hKey,
         LPCSTR lpSubKey,
         PHKEY phkResult
        );

    STDMETHODIMP_(LONG) OpenKeyEx (
         HKEY hKey,
         LPCSTR lpSubKey,
         DWORD ulOptions,
         REGSAM samDesired,
         PHKEY phkResult
        );

    STDMETHODIMP_(LONG) QueryInfoKey (
         HKEY    hKey,
         LPSTR  lpClass,
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

    STDMETHODIMP_(LONG) QueryValue (
         HKEY hKey,
         LPCSTR lpSubKey,
         LPSTR lpValue,
         LONG   *lpcbValue
        );

    STDMETHODIMP_(LONG) QueryMultipleValues (
         HKEY hKey,
         PVALENTXA val_list,
         DWORD num_vals,
         LPSTR lpValueBuf,
         LPDWORD ldwTotsize
        );

    STDMETHODIMP_(LONG) QueryValueEx (
         HKEY hKey,
         LPCSTR lpValueName,
         LPDWORD lpReserved,
         LPDWORD lpType,
         LPBYTE lpData,
         LPDWORD lpcbData
        );

    STDMETHODIMP_(LONG) ReplaceKey (
         HKEY     hKey,
         LPCSTR  lpSubKey,
         LPCSTR  lpNewFile,
         LPCSTR  lpOldFile
        );

    STDMETHODIMP_(LONG) RestoreKey (
         HKEY hKey,
         LPCSTR lpFile,
         DWORD   dwFlags
        );

    STDMETHODIMP_(LONG) SaveKey (
         HKEY hKey,
         LPCSTR lpFile,
         LPSECURITY_ATTRIBUTES lpSecurityAttributes
        );

    STDMETHODIMP_(LONG) SetKeySecurity (
         HKEY hKey,
         SECURITY_INFORMATION SecurityInformation,
         PSECURITY_DESCRIPTOR pSecurityDescriptor
        );

    STDMETHODIMP_(LONG) SetValue (
         HKEY hKey,
         LPCSTR lpSubKey,
         DWORD dwType,
         LPCSTR lpData,
         DWORD cbData
        );

    STDMETHODIMP_(LONG) SetValueEx (
         HKEY hKey,
         LPCSTR lpValueName,
         DWORD Reserved,
         DWORD dwType,
         const BYTE* lpData,
         DWORD cbData
        );

    STDMETHODIMP_(LONG) UnLoadKey (
         HKEY hKey,
         LPCSTR lpSubKey
        );

public:

private:
    CRefCount           _CRefs;          // the total refcount of this object

};


