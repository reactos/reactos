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

#include <Security.h>

///
/// \defgroup ATLSecurityAPI Security API
/// Integrates ATL classes with Microsoft Security API
///
/// @{

///
/// Retrieves the name of the user or other security principal associated with the calling thread and stores it in a ATL::CAtlStringA string.
///
/// \sa [GetUserNameEx function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724435.aspx)
///
BOOLEAN GetUserNameExA(_In_ EXTENDED_NAME_FORMAT NameFormat, _Out_ ATL::CAtlStringA &sName)
{
    CHAR szStackBuffer[ATL_STACK_BUFFER_BYTES/sizeof(CHAR)];
    ULONG ulSize = _countof(szStackBuffer);

    // Try with stack buffer first.
    if (::GetUserNameExA(NameFormat, szStackBuffer, &ulSize)) {
        // Allocate buffer on heap, copy from stack buffer, and zero terminate.
        LPSTR szBuffer = sName.GetBuffer(ulSize);
        if (!szBuffer) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        memcpy(szBuffer, szStackBuffer, ulSize);
        sName.ReleaseBuffer(ulSize);
        return TRUE;
    } else {
        if (::GetLastError() == ERROR_MORE_DATA) {
            // Allocate buffer on heap and retry.
            LPSTR szBuffer = sName.GetBuffer(ulSize - 1);
            if (!szBuffer) {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
            if (::GetUserNameExA(NameFormat, szBuffer, &ulSize)) {
                sName.ReleaseBuffer(ulSize);
                return TRUE;
            } else {
                sName.ReleaseBuffer(0);
                return FALSE;
            }
        } else {
            // Return error.
            return FALSE;
        }
    }
}


///
/// Retrieves the name of the user or other security principal associated with the calling thread and stores it in a ATL::CAtlStringW string.
///
/// \sa [GetUserNameEx function](https://msdn.microsoft.com/en-us/library/windows/desktop/ms724435.aspx)
///
BOOLEAN GetUserNameExW(_In_ EXTENDED_NAME_FORMAT NameFormat, _Out_ ATL::CAtlStringW &sName)
{
    WCHAR szStackBuffer[ATL_STACK_BUFFER_BYTES/sizeof(WCHAR)];
    ULONG ulSize = _countof(szStackBuffer);

    // Try with stack buffer first.
    if (::GetUserNameExW(NameFormat, szStackBuffer, &ulSize)) {
        // Allocate buffer on heap, copy from stack buffer, and zero terminate.
        LPWSTR szBuffer = sName.GetBuffer(ulSize);
        if (!szBuffer) {
            SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        wmemcpy(szBuffer, szStackBuffer, ulSize);
        sName.ReleaseBuffer(ulSize);
        return TRUE;
    } else {
        if (::GetLastError() == ERROR_MORE_DATA) {
            // Allocate buffer on heap and retry.
            LPWSTR szBuffer = sName.GetBuffer(ulSize - 1);
            if (!szBuffer) {
                SetLastError(ERROR_OUTOFMEMORY);
                return FALSE;
            }
            if (::GetUserNameExW(NameFormat, szBuffer, &ulSize)) {
                sName.ReleaseBuffer(ulSize);
                return TRUE;
            } else {
                sName.ReleaseBuffer(0);
                return FALSE;
            }
        } else {
            // Return error.
            return FALSE;
        }
    }
}

/// @}
