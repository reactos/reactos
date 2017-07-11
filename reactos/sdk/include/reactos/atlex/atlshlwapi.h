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

#include <atlstr.h>
#include <Shlwapi.h>

///
/// \defgroup ATLShellWAPI Shell API
/// Integrates ATL classes with Microsoft Shell API
///
/// @{

///
/// Simplifies a path by removing navigation elements such as "." and ".." to produce a direct, well-formed path, and stores it in a ATL::CAtlStringA string.
///
/// \sa [PathCanonicalize function](https://msdn.microsoft.com/en-us/library/windows/desktop/bb773569.aspx)
///
inline BOOL PathCanonicalizeA(__out ATL::CAtlStringA &sValue, __in LPCSTR pszPath)
{
    // Allocate buffer on heap and read into it.
    LPSTR szBuffer = sValue.GetBuffer(MAX_PATH);
    if (!szBuffer) {
        ::SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    BOOL bResult = ::PathCanonicalizeA(szBuffer, pszPath);
    sValue.ReleaseBuffer(bResult ? (int)strnlen(szBuffer, MAX_PATH) : 0);
    sValue.FreeExtra();
    return bResult;
}


///
/// Simplifies a path by removing navigation elements such as "." and ".." to produce a direct, well-formed path, and stores it in a ATL::CAtlStringW string.
///
/// \sa [PathCanonicalize function](https://msdn.microsoft.com/en-us/library/windows/desktop/bb773569.aspx)
///
inline BOOL PathCanonicalizeW(__out ATL::CAtlStringW &sValue, __in LPCWSTR pszPath)
{
    // Allocate buffer on heap and read into it.
    LPWSTR szBuffer = sValue.GetBuffer(MAX_PATH);
    if (!szBuffer) {
        ::SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }
    BOOL bResult = ::PathCanonicalizeW(szBuffer, pszPath);
    sValue.ReleaseBuffer(bResult ? (int)wcsnlen(szBuffer, MAX_PATH) : 0);
    sValue.FreeExtra();
    return bResult;
}

/// @}
