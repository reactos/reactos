/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     SHEvaluateSystemCommandTemplate
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

HRESULT _PathCopyExeAndTrimWhiteSpaces(PWSTR pszBuff, size_t cchBuff,
                                       PCWSTR pszSrc, size_t cchSrc);
HRESULT _PathFindInFolder(_In_ INT csidl, _In_ PCWSTR pszSrc, _Out_ PWSTR pszPath,
                          _In_ UINT cchPath);
HRESULT _PathFindInSystem(_Inout_ PWSTR pszPath, _In_ UINT cchPath);

#if /*TODO*/0 && DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA
#define SHELL32_CAssocElement_Version _WIN32_WINNT_VISTA
#else
#define SHELL32_CAssocElement_Version _WIN32_WINNT_WINXP
#endif

HRESULT WINAPI
AssocCreateElement(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ PVOID* ppvObj);

#ifdef __cplusplus
} // extern "C"
#endif
