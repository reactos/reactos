/*
    Copyright 1991-2017 Amebis

    This file is part of atlex.

    Setup is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Setup is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Setup. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "atlex.h"
#include <atlcoll.h>
#include <atlstr.h>
#include <Windows.h>

///
/// \defgroup ATLWinAPI Windows API
/// Integrates ATL classes with Microsoft Windows API
///
/// @{

///
/// Retrieves the fully qualified path for the file that contains the specified module and stores it in a ATL::CAtlStringA string.
///
/// \sa [GetModuleFileName function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms683197.aspx)
///
inline DWORD GetModuleFileNameA(_In_opt_ HMODULE hModule, _Out_ ATL::CAtlStringA &sValue)
{
    DWORD dwSize = 0;

    for (;;) {
        // Increment size and allocate buffer.
        LPSTR szBuffer = sValue.GetBuffer(dwSize += 1024);
        if (!szBuffer) {
            ::SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }

        // Try!
        DWORD dwResult = ::GetModuleFileNameA(hModule, szBuffer, dwSize);
        if (dwResult == 0) {
            // Error.
            sValue.ReleaseBuffer(0);
            return 0;
        } else if (dwResult < dwSize) {
            DWORD dwLength = (DWORD)strnlen(szBuffer, dwSize);
            sValue.ReleaseBuffer(dwLength++);
            if (dwLength == dwSize) {
                // Buffer was long exactly enough.
                return dwResult;
            } if (dwLength < dwSize) {
                // Buffer was long enough to get entire string, and has some extra space left.
                sValue.FreeExtra();
                return dwResult;
            }
        }
    }
}


///
/// Retrieves the fully qualified path for the file that contains the specified module and stores it in a ATL::CAtlStringW string.
///
/// \sa [GetModuleFileName function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms683197.aspx)
///
inline DWORD GetModuleFileNameW(_In_opt_ HMODULE hModule, _Out_ ATL::CAtlStringW &sValue)
{
    DWORD dwSize = 0;

    for (;;) {
        // Increment size and allocate buffer.
        LPWSTR szBuffer = sValue.GetBuffer(dwSize += 1024);
        if (!szBuffer) {
            ::SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }

        // Try!
        DWORD dwResult = ::GetModuleFileNameW(hModule, szBuffer, dwSize);
        if (dwResult == 0) {
            // Error.
            sValue.ReleaseBuffer(0);
            return 0;
        } else if (dwResult < dwSize) {
            DWORD dwLength = (DWORD)wcsnlen(szBuffer, dwSize);
            sValue.ReleaseBuffer(dwLength++);
            if (dwLength == dwSize) {
                // Buffer was long exactly enough.
                return dwResult;
            } if (dwLength < dwSize) {
                // Buffer was long enough to get entire string, and has some extra space left.
                sValue.FreeExtra();
                return dwResult;
            }
        }
    }
}


///
/// Copies the text of the specified window's title bar (if it has one) into a ATL::CAtlStringA string.
///
/// \sa [GetWindowText function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms633520.aspx)
///
inline int GetWindowTextA(_In_ HWND hWnd, _Out_ ATL::CAtlStringA &sValue)
{
    int iResult;

    // Query the final string length first.
    iResult = ::GetWindowTextLengthA(hWnd);
    if (iResult > 0) {
        // Allocate buffer on heap and read the string data into it.
        LPSTR szBuffer = sValue.GetBuffer(iResult++);
        if (!szBuffer) {
            SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }
        iResult = ::GetWindowTextA(hWnd, szBuffer, iResult);
        sValue.ReleaseBuffer(iResult);
        return iResult;
    } else {
        // The result is empty.
        sValue.Empty();
        return 0;
    }
}


///
/// Copies the text of the specified window's title bar (if it has one) into a ATL::CAtlStringW string.
///
/// \sa [GetWindowText function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms633520.aspx)
///
inline int GetWindowTextW(_In_ HWND hWnd, _Out_ ATL::CAtlStringW &sValue)
{
    int iResult;

    // Query the final string length first.
    iResult = ::GetWindowTextLengthW(hWnd);
    if (iResult > 0) {
        // Allocate buffer on heap and read the string data into it.
        LPWSTR szBuffer = sValue.GetBuffer(iResult++);
        if (!szBuffer) {
            SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }
        iResult = ::GetWindowTextW(hWnd, szBuffer, iResult);
        sValue.ReleaseBuffer(iResult);
        return iResult;
    } else {
        // The result is empty.
        sValue.Empty();
        return 0;
    }
}


///
/// Retrieves version information for the specified file and stores it in a ATL::CAtlStringA string.
///
/// \sa [GetFileVersionInfo function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms647003.aspx)
///
inline BOOL GetFileVersionInfoA(_In_ LPCSTR lptstrFilename, __reserved DWORD dwHandle, _Out_ ATL::CAtlArray<BYTE> &aValue)
{
    // Get version info size.
    DWORD dwVerInfoSize = ::GetFileVersionInfoSizeA(lptstrFilename, &dwHandle);
    if (dwVerInfoSize != 0) {
        if (aValue.SetCount(dwVerInfoSize)) {
            // Read version info.
            return ::GetFileVersionInfoA(lptstrFilename, dwHandle, dwVerInfoSize, aValue.GetData());
        } else {
            ::SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    } else
        return FALSE;
}


///
/// Retrieves version information for the specified file and stores it in a ATL::CAtlStringW string.
///
/// \sa [GetFileVersionInfo function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms647003.aspx)
///
inline BOOL GetFileVersionInfoW(_In_ LPCWSTR lptstrFilename, __reserved DWORD dwHandle, _Out_ ATL::CAtlArray<BYTE> &aValue)
{
    // Get version info size.
    DWORD dwVerInfoSize = ::GetFileVersionInfoSizeW(lptstrFilename, &dwHandle);
    if (dwVerInfoSize != 0) {
        if (aValue.SetCount(dwVerInfoSize)) {
            // Read version info.
            return ::GetFileVersionInfoW(lptstrFilename, dwHandle, dwVerInfoSize, aValue.GetData());
        } else {
            ::SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
    } else
        return FALSE;
}


///
/// Expands environment-variable strings, replaces them with the values defined for the current user, and stores it in a ATL::CAtlStringA string.
///
/// \sa [ExpandEnvironmentStrings function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724265.aspx)
///
inline DWORD ExpandEnvironmentStringsA(_In_ LPCSTR lpSrc, ATL::CAtlStringA &sValue)
{
    DWORD dwBufferSizeEst = (DWORD)strlen(lpSrc) + 0x100; // Initial estimate

    for (;;) {
        DWORD dwBufferSize = dwBufferSizeEst;
        LPSTR szBuffer = sValue.GetBuffer(dwBufferSize);
        if (!szBuffer) {
            ::SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        dwBufferSizeEst = ::ExpandEnvironmentStringsA(lpSrc, szBuffer, dwBufferSize);
        if (dwBufferSizeEst > dwBufferSize) {
            // The buffer was to small. Repeat with a bigger one.
            sValue.ReleaseBuffer(0);
        } else if (dwBufferSizeEst == 0) {
            // Error.
            sValue.ReleaseBuffer(0);
            return 0;
        } else {
            // The buffer was sufficient. Break.
            sValue.ReleaseBuffer();
            sValue.FreeExtra();
            return dwBufferSizeEst;
        }
    }
}


///
/// Expands environment-variable strings, replaces them with the values defined for the current user, and stores it in a ATL::CAtlStringW string.
///
/// \sa [ExpandEnvironmentStrings function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724265.aspx)
///
inline DWORD ExpandEnvironmentStringsW(_In_ LPCWSTR lpSrc, ATL::CAtlStringW &sValue)
{
    DWORD dwBufferSizeEst = (DWORD)wcslen(lpSrc) + 0x100; // Initial estimate

    for (;;) {
        DWORD dwBufferSize = dwBufferSizeEst;
        LPWSTR szBuffer = sValue.GetBuffer(dwBufferSize);
        if (!szBuffer) {
            ::SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        dwBufferSizeEst = ::ExpandEnvironmentStringsW(lpSrc, szBuffer, dwBufferSize);
        if (dwBufferSizeEst > dwBufferSize) {
            // The buffer was to small. Repeat with a bigger one.
            sValue.ReleaseBuffer(0);
        } else if (dwBufferSizeEst == 0) {
            // Error.
            sValue.ReleaseBuffer(0);
            return 0;
        } else {
            // The buffer was sufficient. Break.
            sValue.ReleaseBuffer();
            sValue.FreeExtra();
            return dwBufferSizeEst;
        }
    }
}


///
/// Formats GUID and stores it in a ATL::CAtlStringA string.
///
/// \param[in] lpGuid Pointer to GUID
/// \param[out] str String to store the result to
///
inline VOID GuidToString(_In_ LPCGUID lpGuid, _Out_ ATL::CAtlStringA &str)
{
    str.Format("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        lpGuid->Data1,
        lpGuid->Data2,
        lpGuid->Data3,
        lpGuid->Data4[0], lpGuid->Data4[1],
        lpGuid->Data4[2], lpGuid->Data4[3], lpGuid->Data4[4], lpGuid->Data4[5], lpGuid->Data4[6], lpGuid->Data4[7]);
}


///
/// Formats GUID and stores it in a ATL::CAtlStringW string.
///
/// \param[in] lpGuid Pointer to GUID
/// \param[out] str String to store the result to
///
inline VOID GuidToString(_In_ LPCGUID lpGuid, _Out_ ATL::CAtlStringW &str)
{
    str.Format(L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        lpGuid->Data1,
        lpGuid->Data2,
        lpGuid->Data3,
        lpGuid->Data4[0], lpGuid->Data4[1],
        lpGuid->Data4[2], lpGuid->Data4[3], lpGuid->Data4[4], lpGuid->Data4[5], lpGuid->Data4[6], lpGuid->Data4[7]);
}


///
/// Queries for a string value in the registry and stores it in a ATL::CAtlStringA string.
///
/// `REG_EXPAND_SZ` are expanded using `ExpandEnvironmentStrings()` before storing to sValue.
///
/// \param[in] hReg A handle to an open registry key. The key must have been opened with the KEY_QUERY_VALUE access right.
/// \param[in] pszName The name of the registry value. If lpValueName is NULL or an empty string, "", the function retrieves the type and data for the key's unnamed or default value, if any.
/// \param[out] sValue String to store the value to
/// \return
/// - `ERROR_SUCCESS` when query succeeds;
/// - `ERROR_INVALID_DATA` when the registy value type is not `REG_SZ`, `REG_MULTI_SZ`, or `REG_EXPAND_SZ`;
/// - `ERROR_OUTOFMEMORY` when the memory allocation for the sValue buffer fails;
/// - Error code when query fails. See `RegQueryValueEx()` for the list of error codes.
/// \sa [RegQueryValueEx function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724911.aspx)
/// \sa [ExpandEnvironmentStrings function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724265.aspx)
///
inline LSTATUS RegQueryStringValue(_In_ HKEY hReg, _In_z_ LPCSTR pszName, _Out_ ATL::CAtlStringA &sValue)
{
    LSTATUS lResult;
    BYTE aStackBuffer[ATL_STACK_BUFFER_BYTES];
    DWORD dwSize = sizeof(aStackBuffer), dwType;

    // Try with stack buffer first.
    lResult = ::RegQueryValueExA(hReg, pszName, NULL, &dwType, aStackBuffer, &dwSize);
    if (lResult == ERROR_SUCCESS) {
        if (dwType == REG_SZ || dwType == REG_MULTI_SZ) {
            // The value is REG_SZ or REG_MULTI_SZ. Allocate buffer on heap, copy from stack buffer.
            LPSTR szBuffer = sValue.GetBuffer(dwSize / sizeof(CHAR));
            if (!szBuffer) return ERROR_OUTOFMEMORY;
            memcpy(szBuffer, aStackBuffer, dwSize);
            sValue.ReleaseBuffer();
        } else if (dwType == REG_EXPAND_SZ) {
            // The value is REG_EXPAND_SZ. Expand it from stack buffer.
            if (::ExpandEnvironmentStringsA((const CHAR*)aStackBuffer, sValue) == 0)
                lResult = ::GetLastError();
        } else {
            // The value is not a string type.
            lResult = ERROR_INVALID_DATA;
        }
    } else if (lResult == ERROR_MORE_DATA) {
        if (dwType == REG_SZ || dwType == REG_MULTI_SZ) {
            // The value is REG_SZ or REG_MULTI_SZ. Read it now.
            LPSTR szBuffer = sValue.GetBuffer(dwSize / sizeof(CHAR));
            if (!szBuffer) return ERROR_OUTOFMEMORY;
            if ((lResult = ::RegQueryValueExA(hReg, pszName, NULL, NULL, (LPBYTE)szBuffer, &dwSize)) == ERROR_SUCCESS) {
                sValue.ReleaseBuffer();
            } else {
                // Reading of the value failed.
                sValue.ReleaseBuffer(0);
            }
        } else if (dwType == REG_EXPAND_SZ) {
            // The value is REG_EXPAND_SZ. Read it and expand environment variables.
            ATL::CTempBuffer<CHAR> sTemp(dwSize / sizeof(CHAR));
            if ((lResult = ::RegQueryValueExA(hReg, pszName, NULL, NULL, (LPBYTE)(CHAR*)sTemp, &dwSize)) == ERROR_SUCCESS)
                if (::ExpandEnvironmentStringsA((const CHAR*)sTemp, sValue) == 0)
                    lResult = ::GetLastError();
        } else {
            // The value is not a string type.
            lResult = ERROR_INVALID_DATA;
        }
    }

    return lResult;
}


///
/// Queries for a string value in the registry and stores it in a ATL::CAtlStringW string.
///
/// `REG_EXPAND_SZ` are expanded using `ExpandEnvironmentStrings()` before storing to sValue.
///
/// \param[in] hReg A handle to an open registry key. The key must have been opened with the KEY_QUERY_VALUE access right.
/// \param[in] pszName The name of the registry value. If lpValueName is NULL or an empty string, "", the function retrieves the type and data for the key's unnamed or default value, if any.
/// \param[out] sValue String to store the value to
/// \return
/// - `ERROR_SUCCESS` when query succeeds;
/// - `ERROR_INVALID_DATA` when the registy value type is not `REG_SZ`, `REG_MULTI_SZ`, or `REG_EXPAND_SZ`;
/// - `ERROR_OUTOFMEMORY` when the memory allocation for the sValue buffer fails;
/// - Error code when query fails. See `RegQueryValueEx()` for the list of error codes.
/// \sa [RegQueryValueEx function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724911.aspx)
/// \sa [ExpandEnvironmentStrings function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724265.aspx)
///
inline LSTATUS RegQueryStringValue(_In_ HKEY hReg, _In_z_ LPCWSTR pszName, _Out_ ATL::CAtlStringW &sValue)
{
    LSTATUS lResult;
    BYTE aStackBuffer[ATL_STACK_BUFFER_BYTES];
    DWORD dwSize = sizeof(aStackBuffer), dwType;

    // Try with stack buffer first.
    lResult = ::RegQueryValueExW(hReg, pszName, NULL, &dwType, aStackBuffer, &dwSize);
    if (lResult == ERROR_SUCCESS) {
        if (dwType == REG_SZ || dwType == REG_MULTI_SZ) {
            // The value is REG_SZ or REG_MULTI_SZ. Allocate buffer on heap, copy from stack buffer.
            LPWSTR szBuffer = sValue.GetBuffer(dwSize / sizeof(WCHAR));
            if (!szBuffer) return ERROR_OUTOFMEMORY;
            memcpy(szBuffer, aStackBuffer, dwSize);
            sValue.ReleaseBuffer();
        } else if (dwType == REG_EXPAND_SZ) {
            // The value is REG_EXPAND_SZ. Expand it from stack buffer.
            if (::ExpandEnvironmentStringsW((const WCHAR*)aStackBuffer, sValue) == 0)
                lResult = ::GetLastError();
        } else {
            // The value is not a string type.
            lResult = ERROR_INVALID_DATA;
        }
    } else if (lResult == ERROR_MORE_DATA) {
        if (dwType == REG_SZ || dwType == REG_MULTI_SZ) {
            // The value is REG_SZ or REG_MULTI_SZ. Read it now.
            LPWSTR szBuffer = sValue.GetBuffer(dwSize / sizeof(WCHAR));
            if (!szBuffer) return ERROR_OUTOFMEMORY;
            if ((lResult = ::RegQueryValueExW(hReg, pszName, NULL, NULL, (LPBYTE)szBuffer, &dwSize)) == ERROR_SUCCESS) {
                sValue.ReleaseBuffer();
            } else {
                // Reading of the value failed.
                sValue.ReleaseBuffer(0);
            }
        } else if (dwType == REG_EXPAND_SZ) {
            // The value is REG_EXPAND_SZ. Read it and expand environment variables.
            ATL::CTempBuffer<WCHAR> sTemp(dwSize / sizeof(WCHAR));
            if ((lResult = ::RegQueryValueExW(hReg, pszName, NULL, NULL, (LPBYTE)(WCHAR*)sTemp, &dwSize)) == ERROR_SUCCESS)
                if (::ExpandEnvironmentStringsW((const WCHAR*)sTemp, sValue) == 0)
                    lResult = ::GetLastError();
        } else {
            // The value is not a string type.
            lResult = ERROR_INVALID_DATA;
        }
    }

    return lResult;
}


///
/// Retrieves the type and data for the specified value name associated with an open registry key and stores the data in a ATL::CAtlArray<BYTE> buffer.
///
/// \sa [RegQueryValueEx function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724911.aspx)
///
inline LSTATUS RegQueryValueExA(_In_ HKEY hKey, _In_opt_ LPCSTR lpValueName, __reserved LPDWORD lpReserved, _Out_opt_ LPDWORD lpType, _Out_ ATL::CAtlArray<BYTE> &aData)
{
    LSTATUS lResult;
    BYTE aStackBuffer[ATL_STACK_BUFFER_BYTES];
    DWORD dwSize = sizeof(aStackBuffer);

    // Try with stack buffer first.
    lResult = RegQueryValueExA(hKey, lpValueName, lpReserved, NULL, aStackBuffer, &dwSize);
    if (lResult == ERROR_SUCCESS) {
        // Allocate buffer on heap, copy from stack buffer.
        if (!aData.SetCount(dwSize)) return ERROR_OUTOFMEMORY;
        memcpy(aData.GetData(), aStackBuffer, dwSize);
    } else if (lResult == ERROR_MORE_DATA) {
        // Allocate buffer on heap and retry.
        if (!aData.SetCount(dwSize)) return ERROR_OUTOFMEMORY;
        if ((lResult = RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, aData.GetData(), &dwSize)) != ERROR_SUCCESS)
            aData.SetCount(0);
    }

    return lResult;
}


///
/// Retrieves the type and data for the specified value name associated with an open registry key and stores the data in a ATL::CAtlArray<BYTE> buffer.
///
/// \sa [RegQueryValueEx function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724911.aspx)
///
inline LSTATUS RegQueryValueExW(_In_ HKEY hKey, _In_opt_ LPCWSTR lpValueName, __reserved LPDWORD lpReserved, _Out_opt_ LPDWORD lpType, _Out_ ATL::CAtlArray<BYTE> &aData)
{
    LSTATUS lResult;
    BYTE aStackBuffer[ATL_STACK_BUFFER_BYTES];
    DWORD dwSize = sizeof(aStackBuffer);

    // Try with stack buffer first.
    lResult = RegQueryValueExW(hKey, lpValueName, lpReserved, NULL, aStackBuffer, &dwSize);
    if (lResult == ERROR_SUCCESS) {
        // Allocate buffer on heap, copy from stack buffer.
        if (!aData.SetCount(dwSize)) return ERROR_OUTOFMEMORY;
        memcpy(aData.GetData(), aStackBuffer, dwSize);
    } else if (lResult == ERROR_MORE_DATA) {
        // Allocate buffer on heap and retry.
        if (!aData.SetCount(dwSize)) return ERROR_OUTOFMEMORY;
        if ((lResult = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, aData.GetData(), &dwSize)) != ERROR_SUCCESS)
            aData.SetCount(0);
    }

    return lResult;
}


#if _WIN32_WINNT >= _WIN32_WINNT_VISTA

///
/// Loads the specified string from the specified key and subkey, and stores it in a ATL::CAtlStringA string.
///
/// \sa [RegLoadMUIString function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724890.aspx)
///
inline LSTATUS RegLoadMUIStringA(_In_ HKEY hKey, _In_opt_ LPCSTR pszValue, _Out_ ATL::CAtlStringA &sOut, _In_ DWORD Flags, _In_opt_ LPCSTR pszDirectory)
{
    LSTATUS lResult;
    CHAR szStackBuffer[ATL_STACK_BUFFER_BYTES/sizeof(CHAR)];
    DWORD dwSize;

    Flags &= ~REG_MUI_STRING_TRUNCATE;

    // Try with stack buffer first.
    lResult = RegLoadMUIStringA(hKey, pszValue, szStackBuffer, _countof(szStackBuffer), &dwSize, Flags, pszDirectory);
    if (lResult == ERROR_SUCCESS) {
        // Allocate buffer on heap, copy from stack buffer.
        LPSTR szBuffer = sOut.GetBuffer(dwSize);
        if (!szBuffer) return ERROR_OUTOFMEMORY;
        memcpy(szBuffer, szStackBuffer, dwSize);
        sOut.ReleaseBuffer(dwSize);
    } else if (lResult == ERROR_MORE_DATA) {
        // Allocate buffer on heap and retry.
        LPSTR szBuffer = sOut.GetBuffer(dwSize);
        if (!szBuffer) return ERROR_OUTOFMEMORY;
        sOut.ReleaseBuffer((lResult = RegLoadMUIStringA(hKey, pszValue, szBuffer, dwSize, &dwSize, Flags, pszDirectory)) == ERROR_SUCCESS ? dwSize : 0);
    }

    return lResult;
}


///
/// Loads the specified string from the specified key and subkey, and stores it in a ATL::CAtlStringW string.
///
/// \sa [RegLoadMUIString function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724890.aspx)
///
inline LSTATUS RegLoadMUIStringW(_In_ HKEY hKey, _In_opt_ LPCWSTR pszValue, _Out_ ATL::CAtlStringW &sOut, _In_ DWORD Flags, _In_opt_ LPCWSTR pszDirectory)
{
    LSTATUS lResult;
    WCHAR szStackBuffer[ATL_STACK_BUFFER_BYTES/sizeof(WCHAR)];
    DWORD dwSize;

    Flags &= ~REG_MUI_STRING_TRUNCATE;

    // Try with stack buffer first.
    lResult = RegLoadMUIStringW(hKey, pszValue, szStackBuffer, _countof(szStackBuffer), &dwSize, Flags, pszDirectory);
    if (lResult == ERROR_SUCCESS) {
        // Allocate buffer on heap, copy from stack buffer.
        LPWSTR szBuffer = sOut.GetBuffer(dwSize);
        if (!szBuffer) return ERROR_OUTOFMEMORY;
        wmemcpy(szBuffer, szStackBuffer, dwSize);
        sOut.ReleaseBuffer(dwSize);
    } else if (lResult == ERROR_MORE_DATA) {
        // Allocate buffer on heap and retry.
        LPWSTR szBuffer = sOut.GetBuffer(dwSize);
        if (!szBuffer) return ERROR_OUTOFMEMORY;
        sOut.ReleaseBuffer((lResult = RegLoadMUIStringW(hKey, pszValue, szBuffer, dwSize, &dwSize, Flags, pszDirectory)) == ERROR_SUCCESS ? dwSize : 0);
    }

    return lResult;
}

#endif

/// @}


namespace ATL
{
    /// \addtogroup ATLWinAPI
    /// @{

    ///
    /// Module handle wrapper
    ///
    class CAtlLibrary : public CObjectWithHandleT<HMODULE>
    {
    public:
        ///
        /// Frees the module.
        ///
        /// \sa [FreeLibrary](https://msdn.microsoft.com/en-us/library/windows/desktop/ms683152.aspx)
        ///
        virtual ~CAtlLibrary()
        {
            if (m_h)
                FreeLibrary(m_h);
        }

        ///
        /// Loads the specified module into the address space of the calling process.
        ///
        /// \sa [LoadLibraryEx](https://msdn.microsoft.com/en-us/library/windows/desktop/ms684179.aspx)
        ///
        inline BOOL Load(_In_ LPCTSTR lpFileName, __reserved HANDLE hFile, _In_ DWORD dwFlags)
        {
            HANDLE h = LoadLibraryEx(lpFileName, hFile, dwFlags);
            if (h) {
                Attach(h);
                return TRUE;
            } else
                return FALSE;
        }

    protected:
        ///
        /// Frees the module.
        ///
        /// \sa [FreeLibrary](https://msdn.microsoft.com/en-us/library/windows/desktop/ms683152.aspx)
        ///
        virtual void InternalFree()
        {
            FreeLibrary(m_h);
        }
    };

    /// @}
}
