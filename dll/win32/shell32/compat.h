/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Absorbing version differences for compatibility
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
    /*
     * For versions < Vista+, redefine ShellMessageBoxW to ShellMessageBoxWrapW
     * (this is needed to avoid a linker error). On Vista+ onwards, shell32.ShellMessageBoxW
     * redirects to shlwapi.ShellMessageBoxW so the #define should not be needed.
     */
    #define ShellMessageBoxW ShellMessageBoxWrapW

    LSTATUS
    WINAPI
    RegGetValueA(
        _In_ HKEY hkey,
        _In_opt_ LPCSTR lpSubKey,
        _In_opt_ LPCSTR lpValue,
        _In_ DWORD dwFlags,
        _Out_opt_ LPDWORD pdwType,
        _When_((dwFlags & 0x7F) == RRF_RT_REG_SZ || (dwFlags & 0x7F) == RRF_RT_REG_EXPAND_SZ ||
        (dwFlags & 0x7F) == (RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ) || *pdwType == REG_SZ ||
        *pdwType == REG_EXPAND_SZ, _Post_z_)
        _When_((dwFlags & 0x7F) == RRF_RT_REG_MULTI_SZ || *pdwType == REG_MULTI_SZ, _Post_ _NullNull_terminated_)
          _Out_writes_bytes_to_opt_(*pcbData,*pcbData) PVOID pvData,
        _Inout_opt_ LPDWORD pcbData);

    LSTATUS WINAPI
    RegGetValueW(
        HKEY hKey,
        LPCWSTR pszSubKey,
        LPCWSTR pszValue,
        DWORD dwFlags,
        LPDWORD pdwType,
        PVOID pvData,
        LPDWORD pcbData);

    LSTATUS
    WINAPI
    RegSetValueW(
        _In_ HKEY hKey,
        _In_opt_ LPCWSTR lpSubKey,
        _In_ DWORD dwType,
        _In_reads_bytes_opt_(cbData) LPCWSTR lpData,
        _In_ DWORD cbData);

    LSTATUS
    WINAPI
    RegLoadMUIStringA(
        _In_ HKEY hKey,
        _In_opt_ LPCSTR pszValue,
        _Out_writes_bytes_opt_(cbOutBuf) LPSTR pszOutBuf,
        _In_ DWORD cbOutBuf,
        _Out_opt_ LPDWORD pcbData,
        _In_ DWORD Flags,
        _In_opt_ LPCSTR pszDirectory);

    LSTATUS
    WINAPI
    RegLoadMUIStringW(
        _In_ HKEY hKey,
        _In_opt_ LPCWSTR pszValue,
        _Out_writes_bytes_opt_(cbOutBuf) LPWSTR pszOutBuf,
        _In_ DWORD cbOutBuf,
        _Out_opt_ LPDWORD pcbData,
        _In_ DWORD Flags,
        _In_opt_ LPCWSTR pszDirectory);

    #ifndef LVS_EX_TRANSPARENTSHADOWTEXT
        #define LVS_EX_TRANSPARENTSHADOWTEXT 0x800000
    #endif

    #ifndef LVS_EX_TRANSPARENTBKGND
        #define LVS_EX_TRANSPARENTBKGND 0x400000
    #endif

    #ifndef LVCFMT_FIXED_WIDTH
        #define LVCFMT_FIXED_WIDTH 0x100
    #endif

    #ifndef OAIF_HIDE_REGISTRATION
        #define OAIF_HIDE_REGISTRATION 32
    #endif

    #if (_WIN32_WINNT >= _WIN32_WINNT_XP && _WIN32_WINNT < _WIN32_WINNT_VISTA)
        /* In PSDK, V3 uses hBalloonIcon. This member is not defined in WinXP PSDK. */
        #undef NOTIFYICONDATAA_V3_SIZE
        #undef NOTIFYICONDATAW_V3_SIZE
        #undef NOTIFYICONDATA_V3_SIZE
        #define NOTIFYICONDATAA_V3_SIZE sizeof(NOTIFYICONDATAA)
        #define NOTIFYICONDATAW_V3_SIZE sizeof(NOTIFYICONDATAW)
        #define NOTIFYICONDATA_V3_SIZE  sizeof(NOTIFYICONDATA)
        #ifdef __cplusplus
            static_assert(NOTIFYICONDATAA_V3_SIZE > NOTIFYICONDATAA_V2_SIZE, "Logical error");
            static_assert(NOTIFYICONDATAW_V3_SIZE > NOTIFYICONDATAW_V2_SIZE, "Logical error");
            static_assert(NOTIFYICONDATA_V3_SIZE  > NOTIFYICONDATA_V2_SIZE , "Logical error");
        #endif
    #endif
#endif

#ifdef __cplusplus
} // extern "C"
#endif
