/*
 * Copyright 2017 Michael Müller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PATHCCH_OPTIONS
{
    PATHCCH_NONE = 0x00,
    PATHCCH_ALLOW_LONG_PATHS = 0x01,
    PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS = 0x02,
    PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS = 0x04,
    PATHCCH_DO_NOT_NORMALIZE_SEGMENTS = 0x08,
    PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH = 0x10,
    PATHCCH_ENSURE_TRAILING_SLASH = 0x20,
} PATHCCH_OPTIONS;

#define PATHCCH_MAX_CCH 0x8000

WINBASEAPI HRESULT WINAPI PathAllocCanonicalize(const WCHAR *path_in, DWORD flags, WCHAR **path_out);
WINBASEAPI HRESULT WINAPI PathAllocCombine(const WCHAR *path1, const WCHAR *path2, DWORD flags, WCHAR **out);
WINBASEAPI HRESULT WINAPI PathCchAddBackslash(WCHAR *path, SIZE_T size);
WINBASEAPI HRESULT WINAPI PathCchAddBackslashEx(WCHAR *path, SIZE_T size, WCHAR **end, SIZE_T *remaining);
WINBASEAPI HRESULT WINAPI PathCchAddExtension(WCHAR *path, SIZE_T size, const WCHAR *extension);
WINBASEAPI HRESULT WINAPI PathCchAppend(WCHAR *path1, SIZE_T size, const WCHAR *path2);
WINBASEAPI HRESULT WINAPI PathCchAppendEx(WCHAR *path1, SIZE_T size, const WCHAR *path2, DWORD flags);
WINBASEAPI HRESULT WINAPI PathCchCanonicalize(WCHAR *out, SIZE_T size, const WCHAR *in);
WINBASEAPI HRESULT WINAPI PathCchCanonicalizeEx(WCHAR *out, SIZE_T size, const WCHAR *in, DWORD flags);
WINBASEAPI HRESULT WINAPI PathCchCombine(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2);
WINBASEAPI HRESULT WINAPI PathCchCombineEx(WCHAR *out, SIZE_T size, const WCHAR *path1, const WCHAR *path2, DWORD flags);
WINBASEAPI HRESULT WINAPI PathCchFindExtension(const WCHAR *path, SIZE_T size, const WCHAR **extension);
WINBASEAPI BOOL    WINAPI PathCchIsRoot(const WCHAR *path);
WINBASEAPI HRESULT WINAPI PathCchRemoveBackslash(WCHAR *path, SIZE_T path_size);
WINBASEAPI HRESULT WINAPI PathCchRemoveBackslashEx(WCHAR *path, SIZE_T path_size, WCHAR **path_end, SIZE_T *free_size);
WINBASEAPI HRESULT WINAPI PathCchRemoveExtension(WCHAR *path, SIZE_T size);
WINBASEAPI HRESULT WINAPI PathCchRemoveFileSpec(WCHAR *path, SIZE_T size);
WINBASEAPI HRESULT WINAPI PathCchRenameExtension(WCHAR *path, SIZE_T size, const WCHAR *extension);
WINBASEAPI HRESULT WINAPI PathCchSkipRoot(const WCHAR *path, const WCHAR **root_end);
WINBASEAPI HRESULT WINAPI PathCchStripPrefix(WCHAR *path, SIZE_T size);
WINBASEAPI HRESULT WINAPI PathCchStripToRoot(WCHAR *path, SIZE_T size);
WINBASEAPI BOOL    WINAPI PathIsUNCEx(const WCHAR *path, const WCHAR **server);

#ifdef __cplusplus
}
#endif

#ifdef __REACTOS__
/* C++ non-const overloads */
#ifdef __cplusplus

__inline HRESULT
PathCchFindExtension(
    _In_reads_(cchPath) PWSTR pszPath,
    _In_ size_t cchPath,
    _Outptr_ PWSTR* ppszExt)
{
    return PathCchFindExtension(const_cast<PCWSTR>(pszPath), cchPath, const_cast<PCWSTR*>(ppszExt));
}

__inline HRESULT
PathCchSkipRoot(
    _In_ PWSTR pszPath,
    _Outptr_ PWSTR* ppszRootEnd)
{
    return PathCchSkipRoot(const_cast<PCWSTR>(pszPath), const_cast<PCWSTR*>(ppszRootEnd));
}

__inline BOOL
PathIsUNCEx(
    _In_ PWSTR pszPath,
    _Outptr_opt_ PWSTR* ppszServer)
{
    return PathIsUNCEx(const_cast<PCWSTR>(pszPath), const_cast<PCWSTR*>(ppszServer));
}
#endif
#endif // __cplusplus
