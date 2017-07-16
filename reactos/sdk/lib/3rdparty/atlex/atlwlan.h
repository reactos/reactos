/*
    Copyright 1991-2017 Amebis

    This file is part of atlex.

    atlex is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    atlex is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with atlex. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <atlstr.h>
#include <wlanapi.h>


// Must not statically link to Wlanapi.dll as it is not available on Windows
// without a WLAN interface.
extern DWORD (WINAPI *pfnWlanReasonCodeToString)(__in DWORD dwReasonCode, __in DWORD dwBufferSize, __in_ecount(dwBufferSize) PWCHAR pStringBuffer, __reserved PVOID pReserved);


///
/// \defgroup ATLWLANAPI WLAN API
/// Integrates ATL classes with Microsoft WLAN API
///
/// @{

///
/// Retrieves a string that describes a specified reason code and stores it in a ATL::CAtlStringW string.
///
/// \sa [WlanReasonCodeToString function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms706768.aspx)
///
/// \note
/// Since Wlanapi.dll is not always present, the \c pfnWlanReasonCodeToString pointer to \c WlanReasonCodeToString
/// function must be loaded dynamically.
///
inline DWORD WlanReasonCodeToString(_In_ DWORD dwReasonCode, _Out_ ATL::CAtlStringW &sValue, __reserved PVOID pReserved)
{
    DWORD dwSize = 0;

    if (!::pfnWlanReasonCodeToString)
        return ERROR_CALL_NOT_IMPLEMENTED;

    for (;;) {
        // Increment size and allocate buffer.
        LPWSTR szBuffer = sValue.GetBuffer(dwSize += 1024);
        if (!szBuffer) return ERROR_OUTOFMEMORY;

        // Try!
        DWORD dwResult = ::pfnWlanReasonCodeToString(dwReasonCode, dwSize, szBuffer, pReserved);
        if (dwResult == NO_ERROR) {
            DWORD dwLength = (DWORD)wcsnlen(szBuffer, dwSize);
            sValue.ReleaseBuffer(dwLength++);
            if (dwLength == dwSize) {
                // Buffer was long exactly enough.
                return NO_ERROR;
            } else if (dwLength < dwSize) {
                // Buffer was long enough to get entire string, and has some extra space left.
                sValue.FreeExtra();
                return NO_ERROR;
            }
        } else {
            // Return error code.
            sValue.ReleaseBuffer(0);
            return dwResult;
        }
    }
}

/// @}
