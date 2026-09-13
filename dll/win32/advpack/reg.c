/*
 * Advpack registry functions
 *
 * Copyright 2004 Huw D M Davies
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "winerror.h"
#include "winuser.h"
#include "winternl.h"
#include "advpub.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

static BOOL get_temp_ini_path(LPWSTR name)
{
    WCHAR tmp_dir[MAX_PATH];

    if(!GetTempPathW(ARRAY_SIZE(tmp_dir), tmp_dir))
       return FALSE;

    if (!GetTempFileNameW(tmp_dir, L"avp", 0, name))
        return FALSE;
    return TRUE;
}

static BOOL create_tmp_ini_file(HMODULE hm, WCHAR *ini_file)
{
    HRSRC hrsrc;
    HGLOBAL hmem = NULL;
    DWORD rsrc_size, bytes_written;
    VOID *rsrc_data;
    HANDLE hf = INVALID_HANDLE_VALUE;

    if(!get_temp_ini_path(ini_file)) {
        ERR("Can't get temp ini file path\n");
        goto error;
    }

    if (!(hrsrc = FindResourceW(hm, L"REGINST", L"REGINST"))) {
        ERR("Can't find REGINST resource\n");
        goto error;
    }

    rsrc_size = SizeofResource(hm, hrsrc);
    hmem = LoadResource(hm, hrsrc);
    rsrc_data = LockResource(hmem);

    if(!rsrc_data || !rsrc_size) {
        ERR("Can't load REGINST resource\n");
        goto error;
    }       

    if((hf = CreateFileW(ini_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        ERR("Unable to create temp ini file\n");
        goto error;
    }
    if(!WriteFile(hf, rsrc_data, rsrc_size, &bytes_written, NULL) || rsrc_size != bytes_written) {
        ERR("Write failed\n");
        goto error;
    }
    FreeResource(hmem);
    CloseHandle(hf);
    return TRUE;
error:
    if(hmem) FreeResource(hmem);
    if(hf != INVALID_HANDLE_VALUE) CloseHandle(hf);
    return FALSE;
}

static void strentry_atow(const STRENTRYA *aentry, STRENTRYW *wentry)
{
    DWORD name_len, val_len;

    name_len = MultiByteToWideChar(CP_ACP, 0, aentry->pszName, -1, NULL, 0);
    val_len = MultiByteToWideChar(CP_ACP, 0, aentry->pszValue, -1, NULL, 0);

    wentry->pszName = malloc(name_len * sizeof(WCHAR));
    wentry->pszValue = malloc(val_len * sizeof(WCHAR));

    MultiByteToWideChar(CP_ACP, 0, aentry->pszName, -1, wentry->pszName, name_len);
    MultiByteToWideChar(CP_ACP, 0, aentry->pszValue, -1, wentry->pszValue, val_len);
}

static STRTABLEW *strtable_atow(const STRTABLEA *atable)
{
    STRTABLEW *wtable;
    DWORD j;

    wtable = malloc(sizeof(STRTABLEW));
    wtable->pse = malloc(atable->cEntries * sizeof(STRENTRYW));
    wtable->cEntries = atable->cEntries;

    for (j = 0; j < wtable->cEntries; j++)
        strentry_atow(&atable->pse[j], &wtable->pse[j]);

    return wtable;
}

static void free_strtable(STRTABLEW *wtable)
{
    DWORD j;

    for (j = 0; j < wtable->cEntries; j++)
    {
        free(wtable->pse[j].pszName);
        free(wtable->pse[j].pszValue);
    }

    free(wtable->pse);
    free(wtable);
}

/***********************************************************************
 *          RegInstallA (advpack.@)
 *
 * See RegInstallW.
 */
HRESULT WINAPI RegInstallA(HMODULE hm, LPCSTR pszSection, const STRTABLEA* pstTable)
{
    UNICODE_STRING section;
    STRTABLEW *wtable;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", hm, debugstr_a(pszSection), pstTable);

    if (pstTable)
        wtable = strtable_atow(pstTable);
    else
        wtable = NULL;

    RtlCreateUnicodeStringFromAsciiz(&section, pszSection);

    hr = RegInstallW(hm, section.Buffer, wtable);

    if (pstTable)
        free_strtable(wtable);

    RtlFreeUnicodeString(&section);

    return hr;
}

static HRESULT write_predefined_strings(HMODULE hm, LPCWSTR ini_path)
{
    WCHAR mod_path[MAX_PATH + 2];
    WCHAR sys_mod_path[MAX_PATH + 2];
    WCHAR sys_root[MAX_PATH];

    *mod_path = '\"';
    if (!GetModuleFileNameW(hm, mod_path + 1, ARRAY_SIZE(mod_path) - 2))
        return E_FAIL;

    lstrcatW(mod_path, L"\"");
    WritePrivateProfileStringW(L"Strings", L"_MOD_PATH", mod_path, ini_path);

    *sys_root = '\0';
    GetEnvironmentVariableW(L"SystemRoot", sys_root, ARRAY_SIZE(sys_root));

    if(!wcsnicmp(sys_root, mod_path + 1, lstrlenW(sys_root)))
    {
        *sys_mod_path = '\"';
        lstrcpyW(sys_mod_path + 1, L"%SystemRoot%");
        lstrcatW(sys_mod_path, mod_path + 1 + lstrlenW(sys_root));
    }
    else
    {
        FIXME("SYS_MOD_PATH needs more work\n");
        lstrcpyW(sys_mod_path, mod_path);
    }

    WritePrivateProfileStringW(L"Strings", L"_SYS_MOD_PATH", sys_mod_path, ini_path);

    return S_OK;
}

/***********************************************************************
 *          RegInstallW (advpack.@)
 *
 * Loads an INF from a string resource, adds entries to the string
 * substitution table, and executes the INF.
 *
 * PARAMS
 *   hm         [I] Module that contains the REGINST resource.
 *   pszSection [I] The INF section to execute.
 *   pstTable   [I] Table of string substitutions.
 * 
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 */
HRESULT WINAPI RegInstallW(HMODULE hm, LPCWSTR pszSection, const STRTABLEW* pstTable)
{
    unsigned int i;
    CABINFOW cabinfo;
    WCHAR tmp_ini_path[MAX_PATH];
    HRESULT hr = E_FAIL;

    TRACE("(%p, %s, %p)\n", hm, debugstr_w(pszSection), pstTable);

    if(!create_tmp_ini_file(hm, tmp_ini_path))
        return E_FAIL;

    if (write_predefined_strings(hm, tmp_ini_path) != S_OK)
        goto done;

    /* Write the additional string table */
    if (pstTable)
    {
        for(i = 0; i < pstTable->cEntries; i++)
        {
            WCHAR tmp_value[MAX_PATH + 2];
    
            tmp_value[0] = '\"';
            lstrcpyW(tmp_value + 1, pstTable->pse[i].pszValue);
            lstrcatW(tmp_value, L"\"");

            WritePrivateProfileStringW(L"Strings", pstTable->pse[i].pszName, tmp_value, tmp_ini_path);
        }
    }

    /* flush cache */
    WritePrivateProfileStringW(NULL, NULL, NULL, tmp_ini_path);

    /* FIXME: read AdvOptions val for dwFlags */
    ZeroMemory(&cabinfo, sizeof(CABINFOW));
    cabinfo.pszInf = tmp_ini_path;
    cabinfo.pszSection = (LPWSTR)pszSection;
    cabinfo.dwFlags = 0;

    hr = ExecuteCabW(NULL, &cabinfo, NULL);

done:

    DeleteFileW(tmp_ini_path);

    return hr;
}

/***********************************************************************
 *          RegRestoreAllA (advpack.@)
 *
 * See RegRestoreAllW.
 */
HRESULT WINAPI RegRestoreAllA(HWND hWnd, LPSTR pszTitleString, HKEY hkBackupKey)
{
    UNICODE_STRING title;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", hWnd, debugstr_a(pszTitleString), hkBackupKey);

    RtlCreateUnicodeStringFromAsciiz(&title, pszTitleString);

    hr = RegRestoreAllW(hWnd, title.Buffer, hkBackupKey);

    RtlFreeUnicodeString(&title);

    return hr;
}

/***********************************************************************
 *          RegRestoreAllW (advpack.@)
 *
 * Restores all saved registry entries.
 *
 * PARAMS
 *   hWnd           [I] Handle to the window used for the display.
 *   pszTitleString [I] Title of the window.
 *   hkBackupKey    [I] Handle to the backup key.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI RegRestoreAllW(HWND hWnd, LPWSTR pszTitleString, HKEY hkBackupKey)
{
    FIXME("(%p, %s, %p) stub\n", hWnd, debugstr_w(pszTitleString), hkBackupKey);
    
    return E_FAIL;   
}

/***********************************************************************
 *          RegSaveRestoreA (advpack.@)
 *
 * See RegSaveRestoreW.
 */
HRESULT WINAPI RegSaveRestoreA(HWND hWnd, LPCSTR pszTitleString, HKEY hkBackupKey,
                               LPCSTR pcszRootKey, LPCSTR pcszSubKey,
                               LPCSTR pcszValueName, DWORD dwFlags)
{
    UNICODE_STRING title, root, subkey, value;
    HRESULT hr;

    TRACE("(%p, %s, %p, %s, %s, %s, %ld)\n", hWnd, debugstr_a(pszTitleString),
          hkBackupKey, debugstr_a(pcszRootKey), debugstr_a(pcszSubKey),
          debugstr_a(pcszValueName), dwFlags);

    RtlCreateUnicodeStringFromAsciiz(&title, pszTitleString);
    RtlCreateUnicodeStringFromAsciiz(&root, pcszRootKey);
    RtlCreateUnicodeStringFromAsciiz(&subkey, pcszSubKey);
    RtlCreateUnicodeStringFromAsciiz(&value, pcszValueName);

    hr = RegSaveRestoreW(hWnd, title.Buffer, hkBackupKey, root.Buffer,
                         subkey.Buffer, value.Buffer, dwFlags);

    RtlFreeUnicodeString(&title);
    RtlFreeUnicodeString(&root);
    RtlFreeUnicodeString(&subkey);
    RtlFreeUnicodeString(&value);

    return hr;
}

/***********************************************************************
 *          RegSaveRestoreW (advpack.@)
 *
 * Saves or restores the specified registry value.
 *
 * PARAMS
 *   hWnd           [I] Handle to the window used for the display.
 *   pszTitleString [I] Title of the window.
 *   hkBackupKey    [I] Key used to store the backup data.
 *   pcszRootKey    [I] Root key of the registry value
 *   pcszSubKey     [I] Sub key of the registry value.
 *   pcszValueName  [I] Value to save or restore. 
 *   dwFlags        [I] See advpub.h.
 * 
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI RegSaveRestoreW(HWND hWnd, LPCWSTR pszTitleString, HKEY hkBackupKey,
                               LPCWSTR pcszRootKey, LPCWSTR pcszSubKey,
                               LPCWSTR pcszValueName, DWORD dwFlags)
{
    FIXME("(%p, %s, %p, %s, %s, %s, %ld): stub\n", hWnd, debugstr_w(pszTitleString),
          hkBackupKey, debugstr_w(pcszRootKey), debugstr_w(pcszSubKey),
          debugstr_w(pcszValueName), dwFlags);

    return E_FAIL;   
}

/***********************************************************************
 *          RegSaveRestoreOnINFA (advpack.@)
 *
 * See RegSaveRestoreOnINFW.
 */
HRESULT WINAPI RegSaveRestoreOnINFA(HWND hWnd, LPCSTR pszTitle, LPCSTR pszINF,
                                    LPCSTR pszSection, HKEY hHKLMBackKey,
                                    HKEY hHKCUBackKey, DWORD dwFlags)
{
    UNICODE_STRING title, inf, section;
    HRESULT hr;

    TRACE("(%p, %s, %s, %s, %p, %p, %ld)\n", hWnd, debugstr_a(pszTitle),
          debugstr_a(pszINF), debugstr_a(pszSection),
          hHKLMBackKey, hHKCUBackKey, dwFlags);

    RtlCreateUnicodeStringFromAsciiz(&title, pszTitle);
    RtlCreateUnicodeStringFromAsciiz(&inf, pszINF);
    RtlCreateUnicodeStringFromAsciiz(&section, pszSection);

    hr = RegSaveRestoreOnINFW(hWnd, title.Buffer, inf.Buffer, section.Buffer,
                              hHKLMBackKey, hHKCUBackKey, dwFlags);

    RtlFreeUnicodeString(&title);
    RtlFreeUnicodeString(&inf);
    RtlFreeUnicodeString(&section);

    return hr;
}

/***********************************************************************
 *          RegSaveRestoreOnINFW (advpack.@)
 *
 * Saves or restores the specified INF Reg section.
 *
 * PARAMS
 *   hWnd         [I] Handle to the window used for the display.
 *   pszTitle     [I] Title of the window.
 *   pszINF       [I] Filename of the INF.
 *   pszSection   [I] Section to save or restore.
 *   hHKLMBackKey [I] Opened key in HKLM to store data.
 *   hHKCUBackKey [I] Opened key in HKCU to store data.
 *   dwFlags      [I] See advpub.h
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI RegSaveRestoreOnINFW(HWND hWnd, LPCWSTR pszTitle, LPCWSTR pszINF,
                                    LPCWSTR pszSection, HKEY hHKLMBackKey,
                                    HKEY hHKCUBackKey, DWORD dwFlags)
{
    FIXME("(%p, %s, %s, %s, %p, %p, %ld): stub\n", hWnd, debugstr_w(pszTitle),
          debugstr_w(pszINF), debugstr_w(pszSection),
          hHKLMBackKey, hHKCUBackKey, dwFlags);

    return E_FAIL;   
}
