/*
 * Advpack main
 *
 * Copyright 2004 Huw D M Davies
 * Copyright 2005 Sami Aario
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
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winternl.h"
#include "winnls.h"
#include "setupapi.h"
#include "advpub.h"
#include "wine/debug.h"
#include "advpack_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

typedef HRESULT (WINAPI *DLLREGISTER) (void);

#define MAX_FIELD_LENGTH    512
#define PREFIX_LEN          5

/* registry path of the Installed Components key for per-user stubs */
static const WCHAR setup_key[] = L"SOFTWARE\\Microsoft\\Active Setup\\Installed Components";

/* Strip single quotes from a token - note size includes NULL */
static void strip_quotes(WCHAR *buffer, DWORD *size)
{
    if (buffer[0] == '\'' && (*size > 1) && buffer[*size-2]=='\'')
    {
        *size -= 2;
        buffer[*size] = 0x00;
        memmove(buffer, buffer + 1, *size * sizeof(WCHAR));
    }
}

/* parses the destination directory parameters from pszSection
 * the parameters are of the form: root,key,value,unknown,fallback
 * we first read the reg value root\\key\\value and if that fails,
 * use fallback as the destination directory
 */
static void get_dest_dir(HINF hInf, PCWSTR pszSection, PWSTR pszBuffer, DWORD dwSize)
{
    INFCONTEXT context;
    WCHAR key[MAX_PATH + 2], value[MAX_PATH + 2];
    WCHAR prefix[PREFIX_LEN + 2];
    HKEY root, subkey = 0;
    DWORD size;

    /* load the destination parameters */
    SetupFindFirstLineW(hInf, pszSection, NULL, &context);
    SetupGetStringFieldW(&context, 1, prefix, PREFIX_LEN + 2, &size);
    strip_quotes(prefix, &size);
    SetupGetStringFieldW(&context, 2, key, MAX_PATH + 2, &size);
    strip_quotes(key, &size);
    SetupGetStringFieldW(&context, 3, value, MAX_PATH + 2, &size);
    strip_quotes(value, &size);

    if (!lstrcmpW(prefix, L"HKLM"))
        root = HKEY_LOCAL_MACHINE;
    else if (!lstrcmpW(prefix, L"HKCU"))
        root = HKEY_CURRENT_USER;
    else
        root = NULL;

    size = dwSize * sizeof(WCHAR);

    /* fallback to the default destination dir if reg fails */
    if (RegOpenKeyW(root, key, &subkey) ||
        RegQueryValueExW(subkey, value, NULL, NULL, (LPBYTE)pszBuffer, &size))
    {
        SetupGetStringFieldW(&context, 5, pszBuffer, dwSize, &size);
        strip_quotes(pszBuffer, &size);
    }

    if (subkey) RegCloseKey(subkey);
}

/* loads the LDIDs specified in the install section of an INF */
void set_ldids(HINF hInf, LPCWSTR pszInstallSection, LPCWSTR pszWorkingDir)
{
    WCHAR field[MAX_FIELD_LENGTH];
    WCHAR line[MAX_FIELD_LENGTH];
    WCHAR dest[MAX_PATH];
    INFCONTEXT context;
    DWORD size;
    int ldid;

    if (!SetupGetLineTextW(NULL, hInf, pszInstallSection, L"CustomDestination",
                           field, MAX_FIELD_LENGTH, &size))
        return;

    if (!SetupFindFirstLineW(hInf, field, NULL, &context))
        return;

    do
    {
        LPWSTR value, ptr, key, key_copy = NULL;
        DWORD flags = 0;

        SetupGetLineTextW(&context, NULL, NULL, NULL,
                          line, MAX_FIELD_LENGTH, &size);

        /* SetupGetLineTextW returns the value if there is only one key, but
         * returns the whole line if there is more than one key
         */
        if (!(value = wcschr(line, '=')))
        {
            SetupGetStringFieldW(&context, 0, NULL, 0, &size);
            key = malloc(size * sizeof(WCHAR));
            key_copy = key;
            SetupGetStringFieldW(&context, 0, key, size, &size);
            value = line;
        }
        else
        {
            key = line;
            *(value++) = '\0';
        }

        /* remove leading whitespace from the value */
        while (*value == ' ')
            value++;

        /* Extract the flags */
        ptr = wcschr(value, ',');
        if (ptr) {
            *ptr = '\0';
            flags = wcstol(ptr+1, NULL, 10);
        }

        /* set dest to pszWorkingDir if key is SourceDir */
        if (pszWorkingDir && !lstrcmpiW(value, L"SourceDir"))
            lstrcpynW(dest, pszWorkingDir, MAX_PATH);
        else
            get_dest_dir(hInf, value, dest, MAX_PATH);

        /* If prompting required, provide dialog to request path */
        if (flags & 0x04)
            FIXME("Need to support changing paths - default will be used\n");

        /* set all ldids to dest */
        while ((ptr = get_parameter(&key, ',', FALSE)))
        {
            ldid = wcstol(ptr, NULL, 10);
            SetupSetDirectoryIdW(hInf, ldid, dest);
        }
        free(key_copy);
    } while (SetupFindNextLine(&context, &context));
}

/***********************************************************************
 *           CloseINFEngine (ADVPACK.@)
 *
 * Closes a handle to an INF file opened with OpenINFEngine.
 *
 * PARAMS
 *   hInf [I] Handle to the INF file to close.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 */
HRESULT WINAPI CloseINFEngine(HINF hInf)
{
    TRACE("(%p)\n", hInf);

    if (!hInf)
        return E_INVALIDARG;

    SetupCloseInfFile(hInf);
    return S_OK;
}

/***********************************************************************
 *              IsNTAdmin	(ADVPACK.@)
 *
 * Checks if the user has admin privileges.
 *
 * PARAMS
 *   reserved  [I] Reserved.  Must be 0.
 *   pReserved [I] Reserved.  Must be NULL.
 *
 * RETURNS
 *   TRUE if user has admin rights, FALSE otherwise.
 */
BOOL WINAPI IsNTAdmin(DWORD reserved, LPDWORD pReserved)
{
    SID_IDENTIFIER_AUTHORITY SidAuthority = {SECURITY_NT_AUTHORITY};
    PTOKEN_GROUPS pTokenGroups;
    BOOL bSidFound = FALSE;
    DWORD dwSize, i;
    HANDLE hToken;
    PSID pSid;

    TRACE("(%ld, %p)\n", reserved, pReserved);

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    if (!GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            CloseHandle(hToken);
            return FALSE;
        }
    }

    pTokenGroups = malloc(dwSize);
    if (!pTokenGroups)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    if (!GetTokenInformation(hToken, TokenGroups, pTokenGroups, dwSize, &dwSize))
    {
        free(pTokenGroups);
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);

    if (!AllocateAndInitializeSid(&SidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pSid))
    {
        free(pTokenGroups);
        return FALSE;
    }

    for (i = 0; i < pTokenGroups->GroupCount; i++)
    {
        if (EqualSid(pSid, pTokenGroups->Groups[i].Sid))
        {
            bSidFound = TRUE;
            break;
        }
    }

    free(pTokenGroups);
    FreeSid(pSid);

    return bSidFound;
}

/***********************************************************************
 *             NeedRebootInit  (ADVPACK.@)
 *
 * Sets up conditions for reboot checking.
 *
 * RETURNS
 *   Value required by NeedReboot.
 */
DWORD WINAPI NeedRebootInit(VOID)
{
    FIXME("(VOID): stub\n");
    return 0;
}

/***********************************************************************
 *             NeedReboot      (ADVPACK.@)
 *
 * Determines whether a reboot is required.
 *
 * PARAMS
 *   dwRebootCheck [I] Value from NeedRebootInit.
 *
 * RETURNS
 *   TRUE if a reboot is needed, FALSE otherwise.
 *
 * BUGS
 *   Unimplemented.
 */
BOOL WINAPI NeedReboot(DWORD dwRebootCheck)
{
    FIXME("(%ld): stub\n", dwRebootCheck);
    return FALSE;
}

/***********************************************************************
 *             OpenINFEngineA   (ADVPACK.@)
 *
 * See OpenINFEngineW.
 */
HRESULT WINAPI OpenINFEngineA(LPCSTR pszInfFilename, LPCSTR pszInstallSection,
                              DWORD dwFlags, HINF *phInf, PVOID pvReserved)
{
    UNICODE_STRING filenameW, installW;
    HRESULT res;

    TRACE("(%s, %s, %ld, %p, %p)\n", debugstr_a(pszInfFilename),
          debugstr_a(pszInstallSection), dwFlags, phInf, pvReserved);

    if (!pszInfFilename || !phInf)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&filenameW, pszInfFilename);
    RtlCreateUnicodeStringFromAsciiz(&installW, pszInstallSection);

    res = OpenINFEngineW(filenameW.Buffer, installW.Buffer,
                         dwFlags, phInf, pvReserved);

    RtlFreeUnicodeString(&filenameW);
    RtlFreeUnicodeString(&installW);

    return res;
}

/***********************************************************************
 *             OpenINFEngineW   (ADVPACK.@)
 *
 * Opens and returns a handle to an INF file to be used by
 * TranslateInfStringEx to continuously translate the INF file.
 *
 * PARAMS
 *   pszInfFilename    [I] Filename of the INF to open.
 *   pszInstallSection [I] Name of the Install section in the INF.
 *   dwFlags           [I] See advpub.h.
 *   phInf             [O] Handle to the loaded INF file.
 *   pvReserved        [I] Reserved.  Must be NULL.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 */
HRESULT WINAPI OpenINFEngineW(LPCWSTR pszInfFilename, LPCWSTR pszInstallSection,
                              DWORD dwFlags, HINF *phInf, PVOID pvReserved)
{
    TRACE("(%s, %s, %ld, %p, %p)\n", debugstr_w(pszInfFilename),
          debugstr_w(pszInstallSection), dwFlags, phInf, pvReserved);

    if (!pszInfFilename || !phInf)
        return E_INVALIDARG;

    *phInf = SetupOpenInfFileW(pszInfFilename, NULL, INF_STYLE_WIN4, NULL);
    if (*phInf == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    set_ldids(*phInf, pszInstallSection, NULL);
    
    return S_OK;
}

/***********************************************************************
 *             RebootCheckOnInstallA   (ADVPACK.@)
 *
 * See RebootCheckOnInstallW.
 */
HRESULT WINAPI RebootCheckOnInstallA(HWND hWnd, LPCSTR pszINF,
                                     LPCSTR pszSec, DWORD dwReserved)
{
    UNICODE_STRING infW, secW;
    HRESULT res;

    TRACE("(%p, %s, %s, %ld)\n", hWnd, debugstr_a(pszINF),
          debugstr_a(pszSec), dwReserved);

    if (!pszINF || !pszSec)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&infW, pszINF);
    RtlCreateUnicodeStringFromAsciiz(&secW, pszSec);

    res = RebootCheckOnInstallW(hWnd, infW.Buffer, secW.Buffer, dwReserved);

    RtlFreeUnicodeString(&infW);
    RtlFreeUnicodeString(&secW);

    return res;
}

/***********************************************************************
 *             RebootCheckOnInstallW   (ADVPACK.@)
 *
 * Checks if a reboot is required for an installed INF section.
 *
 * PARAMS
 *   hWnd       [I] Handle to the window used for messages.
 *   pszINF     [I] Filename of the INF file.
 *   pszSec     [I] INF section to check.
 *   dwReserved [I] Reserved.  Must be 0.
 *
 * RETURNS
 *   Success: S_OK - Reboot is needed if the INF section is installed.
 *            S_FALSE - Reboot is not needed.
 *   Failure: HRESULT of GetLastError().
 *
 * NOTES
 *   if pszSec is NULL, RebootCheckOnInstall checks the DefaultInstall
 *   or DefaultInstall.NT section.
 *
 * BUGS
 *   Unimplemented.
 */
HRESULT WINAPI RebootCheckOnInstallW(HWND hWnd, LPCWSTR pszINF,
                                     LPCWSTR pszSec, DWORD dwReserved)
{
    FIXME("(%p, %s, %s, %ld): stub\n", hWnd, debugstr_w(pszINF),
          debugstr_w(pszSec), dwReserved);

    return E_FAIL;
}

/* registers the OCX if do_reg is TRUE, unregisters it otherwise */
HRESULT do_ocx_reg(HMODULE hocx, BOOL do_reg, const WCHAR *flags, const WCHAR *param)
{
    DLLREGISTER reg_func;

    if (do_reg)
        reg_func = (DLLREGISTER)GetProcAddress(hocx, "DllRegisterServer");
    else
        reg_func = (DLLREGISTER)GetProcAddress(hocx, "DllUnregisterServer");

    if (!reg_func)
        return E_FAIL;

    reg_func();
    return S_OK;
}

/***********************************************************************
 *             RegisterOCX    (ADVPACK.@)
 *
 * Registers an OCX.
 *
 * PARAMS
 *   hWnd    [I] Handle to the window used for the display.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order OCX,flags,param.
 *   show    [I] How the window should be shown.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   OCX - Filename of the OCX to register.
 *   flags - Controls the operation of RegisterOCX.
 *    'I' Call DllRegisterServer and DllInstall.
 *    'N' Only call DllInstall.
 *   param - Command line passed to DllInstall.
 */
HRESULT WINAPI RegisterOCX(HWND hWnd, HINSTANCE hInst, LPCSTR cmdline, INT show)
{
    LPWSTR ocx_filename, str_flags, param;
    LPWSTR cmdline_copy, cmdline_ptr;
    UNICODE_STRING cmdlineW;
    HRESULT hr = E_FAIL;
    HMODULE hm = NULL;

    TRACE("(%s)\n", debugstr_a(cmdline));

    RtlCreateUnicodeStringFromAsciiz(&cmdlineW, cmdline);

    cmdline_copy = wcsdup(cmdlineW.Buffer);
    cmdline_ptr = cmdline_copy;

    ocx_filename = get_parameter(&cmdline_ptr, ',', TRUE);
    if (!ocx_filename || !*ocx_filename)
        goto done;

    str_flags = get_parameter(&cmdline_ptr, ',', TRUE);
    param = get_parameter(&cmdline_ptr, ',', TRUE);

    hm = LoadLibraryExW(ocx_filename, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hm)
        goto done;

    hr = do_ocx_reg(hm, TRUE, str_flags, param);

done:
    FreeLibrary(hm);
    free(cmdline_copy);
    RtlFreeUnicodeString(&cmdlineW);

    return hr;
}

/***********************************************************************
 *             SetPerUserSecValuesA   (ADVPACK.@)
 *
 * See SetPerUserSecValuesW.
 */
HRESULT WINAPI SetPerUserSecValuesA(PERUSERSECTIONA* pPerUser)
{
    PERUSERSECTIONW perUserW;

    TRACE("(%p)\n", pPerUser);

    if (!pPerUser)
        return E_INVALIDARG;

    MultiByteToWideChar(CP_ACP, 0, pPerUser->szGUID, -1, perUserW.szGUID, ARRAY_SIZE(perUserW.szGUID));
    MultiByteToWideChar(CP_ACP, 0, pPerUser->szDispName, -1, perUserW.szDispName, ARRAY_SIZE(perUserW.szDispName));
    MultiByteToWideChar(CP_ACP, 0, pPerUser->szLocale, -1, perUserW.szLocale, ARRAY_SIZE(perUserW.szLocale));
    MultiByteToWideChar(CP_ACP, 0, pPerUser->szStub, -1, perUserW.szStub, ARRAY_SIZE(perUserW.szStub));
    MultiByteToWideChar(CP_ACP, 0, pPerUser->szVersion, -1, perUserW.szVersion, ARRAY_SIZE(perUserW.szVersion));
    MultiByteToWideChar(CP_ACP, 0, pPerUser->szCompID, -1, perUserW.szCompID, ARRAY_SIZE(perUserW.szCompID));
    perUserW.dwIsInstalled = pPerUser->dwIsInstalled;
    perUserW.bRollback = pPerUser->bRollback;

    return SetPerUserSecValuesW(&perUserW);
}

/***********************************************************************
 *             SetPerUserSecValuesW   (ADVPACK.@)
 *
 * Prepares the per-user stub values under IsInstalled\{GUID} that
 * control the per-user installation.
 *
 * PARAMS
 *   pPerUser [I] Per-user stub values.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 */
HRESULT WINAPI SetPerUserSecValuesW(PERUSERSECTIONW* pPerUser)
{
    HKEY setup, guid;

    TRACE("(%p)\n", pPerUser);

    if (!pPerUser || !*pPerUser->szGUID)
        return S_OK;

    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, setup_key, 0, NULL, 0, KEY_WRITE,
                        NULL, &setup, NULL))
    {
        return E_FAIL;
    }

    if (RegCreateKeyExW(setup, pPerUser->szGUID, 0, NULL, 0, KEY_ALL_ACCESS,
                        NULL, &guid, NULL))
    {
        RegCloseKey(setup);
        return E_FAIL;
    }

    if (*pPerUser->szStub)
    {
        RegSetValueExW(guid, L"StubPath", 0, REG_SZ, (BYTE *)pPerUser->szStub,
                       (lstrlenW(pPerUser->szStub) + 1) * sizeof(WCHAR));
    }

    if (*pPerUser->szVersion)
    {
        RegSetValueExW(guid, L"Version", 0, REG_SZ, (BYTE *)pPerUser->szVersion,
                       (lstrlenW(pPerUser->szVersion) + 1) * sizeof(WCHAR));
    }

    if (*pPerUser->szLocale)
    {
        RegSetValueExW(guid, L"Locale", 0, REG_SZ, (BYTE *)pPerUser->szLocale,
                       (lstrlenW(pPerUser->szLocale) + 1) * sizeof(WCHAR));
    }

    if (*pPerUser->szCompID)
    {
        RegSetValueExW(guid, L"ComponentID", 0, REG_SZ, (BYTE *)pPerUser->szCompID,
                       (lstrlenW(pPerUser->szCompID) + 1) * sizeof(WCHAR));
    }

    if (*pPerUser->szDispName)
    {
        RegSetValueExW(guid, NULL, 0, REG_SZ, (LPBYTE)pPerUser->szDispName,
                       (lstrlenW(pPerUser->szDispName) + 1) * sizeof(WCHAR));
    }

    RegSetValueExW(guid, L"IsInstalled", 0, REG_DWORD,
                   (LPBYTE)&pPerUser->dwIsInstalled, sizeof(DWORD));

    RegCloseKey(guid);
    RegCloseKey(setup);

    return S_OK;
}

/***********************************************************************
 *             TranslateInfStringA   (ADVPACK.@)
 *
 * See TranslateInfStringW.
 */
HRESULT WINAPI TranslateInfStringA(LPCSTR pszInfFilename, LPCSTR pszInstallSection,
                LPCSTR pszTranslateSection, LPCSTR pszTranslateKey, LPSTR pszBuffer,
                DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved)
{
    UNICODE_STRING filenameW, installW;
    UNICODE_STRING translateW, keyW;
    LPWSTR bufferW;
    HRESULT res;
    DWORD len = 0;

    TRACE("(%s, %s, %s, %s, %p, %ld, %p, %p)\n",
          debugstr_a(pszInfFilename), debugstr_a(pszInstallSection),
          debugstr_a(pszTranslateSection), debugstr_a(pszTranslateKey),
          pszBuffer, dwBufferSize,pdwRequiredSize, pvReserved);

    if (!pszInfFilename || !pszTranslateSection ||
        !pszTranslateKey || !pdwRequiredSize)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&filenameW, pszInfFilename);
    RtlCreateUnicodeStringFromAsciiz(&installW, pszInstallSection);
    RtlCreateUnicodeStringFromAsciiz(&translateW, pszTranslateSection);
    RtlCreateUnicodeStringFromAsciiz(&keyW, pszTranslateKey);

    res = TranslateInfStringW(filenameW.Buffer, installW.Buffer,
                              translateW.Buffer, keyW.Buffer, NULL,
                              dwBufferSize, &len, NULL);

    if (res == S_OK)
    {
        bufferW = malloc(len * sizeof(WCHAR));

        res = TranslateInfStringW(filenameW.Buffer, installW.Buffer,
                                  translateW.Buffer, keyW.Buffer, bufferW,
                                  len, &len, NULL);
        if (res == S_OK)
        {
            *pdwRequiredSize = WideCharToMultiByte(CP_ACP, 0, bufferW, -1,
                                                   NULL, 0, NULL, NULL);

            if (dwBufferSize >= *pdwRequiredSize)
            {
                WideCharToMultiByte(CP_ACP, 0, bufferW, -1, pszBuffer,
                                    dwBufferSize, NULL, NULL);
            }
            else
                res = E_NOT_SUFFICIENT_BUFFER;
        }

        free(bufferW);
    }

    RtlFreeUnicodeString(&filenameW);
    RtlFreeUnicodeString(&installW);
    RtlFreeUnicodeString(&translateW);
    RtlFreeUnicodeString(&keyW);

    return res;
}

/***********************************************************************
 *             TranslateInfStringW   (ADVPACK.@)
 *
 * Translates the value of a specified key in an inf file into the
 * current locale by expanding string macros.
 *
 * PARAMS
 *   pszInfFilename      [I] Filename of the inf file.
 *   pszInstallSection   [I]
 *   pszTranslateSection [I] Inf section where the key exists.
 *   pszTranslateKey     [I] Key to translate.
 *   pszBuffer           [O] Contains the translated string on exit.
 *   dwBufferSize        [I] Size on input of pszBuffer.
 *   pdwRequiredSize     [O] Length of the translated key.
 *   pvReserved          [I] Reserved, must be NULL.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: An hresult error code.
 */
HRESULT WINAPI TranslateInfStringW(LPCWSTR pszInfFilename, LPCWSTR pszInstallSection,
                LPCWSTR pszTranslateSection, LPCWSTR pszTranslateKey, LPWSTR pszBuffer,
                DWORD dwBufferSize, PDWORD pdwRequiredSize, PVOID pvReserved)
{
    HINF hInf;
    HRESULT hret = S_OK;

    TRACE("(%s, %s, %s, %s, %p, %ld, %p, %p)\n",
          debugstr_w(pszInfFilename), debugstr_w(pszInstallSection),
          debugstr_w(pszTranslateSection), debugstr_w(pszTranslateKey),
          pszBuffer, dwBufferSize,pdwRequiredSize, pvReserved);

    if (!pszInfFilename || !pszTranslateSection ||
        !pszTranslateKey || !pdwRequiredSize)
        return E_INVALIDARG;

    hInf = SetupOpenInfFileW(pszInfFilename, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    set_ldids(hInf, pszInstallSection, NULL);

    if (!SetupGetLineTextW(NULL, hInf, pszTranslateSection, pszTranslateKey,
                           pszBuffer, dwBufferSize, pdwRequiredSize))
    {
        if (dwBufferSize < *pdwRequiredSize)
            hret = E_NOT_SUFFICIENT_BUFFER;
        else
            hret = SPAPI_E_LINE_NOT_FOUND;
    }

    SetupCloseInfFile(hInf);
    return hret;
}

/***********************************************************************
 *             TranslateInfStringExA   (ADVPACK.@)
 *
 * See TranslateInfStringExW.
 */
HRESULT WINAPI TranslateInfStringExA(HINF hInf, LPCSTR pszInfFilename,
                                    LPCSTR pszTranslateSection, LPCSTR pszTranslateKey,
                                    LPSTR pszBuffer, DWORD dwBufferSize,
                                    PDWORD pdwRequiredSize, PVOID pvReserved)
{
    UNICODE_STRING filenameW, sectionW, keyW;
    LPWSTR bufferW;
    HRESULT res;
    DWORD len = 0;

    TRACE("(%p, %s, %s, %s, %p, %ld, %p, %p)\n", hInf, debugstr_a(pszInfFilename),
          debugstr_a(pszTranslateSection), debugstr_a(pszTranslateKey),
          pszBuffer, dwBufferSize, pdwRequiredSize, pvReserved);

    if (!pszInfFilename || !pszTranslateSection ||
        !pszTranslateKey || !pdwRequiredSize)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&filenameW, pszInfFilename);
    RtlCreateUnicodeStringFromAsciiz(&sectionW, pszTranslateSection);
    RtlCreateUnicodeStringFromAsciiz(&keyW, pszTranslateKey);

    res = TranslateInfStringExW(hInf, filenameW.Buffer, sectionW.Buffer,
                                keyW.Buffer, NULL, 0, &len, NULL);

    if (res == S_OK)
    {
        bufferW = malloc(len * sizeof(WCHAR));

        res = TranslateInfStringExW(hInf, filenameW.Buffer, sectionW.Buffer,
                                keyW.Buffer, bufferW, len, &len, NULL);

        if (res == S_OK)
        {
            *pdwRequiredSize = WideCharToMultiByte(CP_ACP, 0, bufferW, -1,
                                                   NULL, 0, NULL, NULL);

            if (dwBufferSize >= *pdwRequiredSize)
            {
                WideCharToMultiByte(CP_ACP, 0, bufferW, -1, pszBuffer,
                                    dwBufferSize, NULL, NULL);
            }
            else
                res = E_NOT_SUFFICIENT_BUFFER;
        }

        free(bufferW);
    }

    RtlFreeUnicodeString(&filenameW);
    RtlFreeUnicodeString(&sectionW);
    RtlFreeUnicodeString(&keyW);

    return res;
}

/***********************************************************************
 *             TranslateInfStringExW   (ADVPACK.@)
 *
 * Using a handle to an INF file opened with OpenINFEngine, translates
 * the value of a specified key in an inf file into the current locale
 * by expanding string macros.
 *
 * PARAMS
 *   hInf                [I] Handle to the INF file.
 *   pszInfFilename      [I] Filename of the INF file.
 *   pszTranslateSection [I] Inf section where the key exists.
 *   pszTranslateKey     [I] Key to translate.
 *   pszBuffer           [O] Contains the translated string on exit.
 *   dwBufferSize        [I] Size on input of pszBuffer.
 *   pdwRequiredSize     [O] Length of the translated key.
 *   pvReserved          [I] Reserved.  Must be NULL.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * NOTES
 *   To use TranslateInfStringEx to translate an INF file continuously,
 *   open the INF file with OpenINFEngine, call TranslateInfStringEx as
 *   many times as needed, then release the handle with CloseINFEngine.
 *   When translating more than one keys, this method is more efficient
 *   than calling TranslateInfString, because the INF file is only
 *   opened once.
 */
HRESULT WINAPI TranslateInfStringExW(HINF hInf, LPCWSTR pszInfFilename,
                                     LPCWSTR pszTranslateSection, LPCWSTR pszTranslateKey,
                                     LPWSTR pszBuffer, DWORD dwBufferSize,
                                     PDWORD pdwRequiredSize, PVOID pvReserved)
{
    TRACE("(%p, %s, %s, %s, %p, %ld, %p, %p)\n", hInf, debugstr_w(pszInfFilename),
          debugstr_w(pszTranslateSection), debugstr_w(pszTranslateKey),
          pszBuffer, dwBufferSize, pdwRequiredSize, pvReserved);

    if (!hInf || !pszInfFilename || !pszTranslateSection || !pszTranslateKey)
        return E_INVALIDARG;

    if (!SetupGetLineTextW(NULL, hInf, pszTranslateSection, pszTranslateKey,
                           pszBuffer, dwBufferSize, pdwRequiredSize))
    {
        if (dwBufferSize < *pdwRequiredSize)
            return E_NOT_SUFFICIENT_BUFFER;

        return SPAPI_E_LINE_NOT_FOUND;
    }

    return S_OK;   
}

/***********************************************************************
 *             UserInstStubWrapperA   (ADVPACK.@)
 *
 * See UserInstStubWrapperW.
 */
HRESULT WINAPI UserInstStubWrapperA(HWND hWnd, HINSTANCE hInstance,
                                   LPSTR pszParms, INT nShow)
{
    UNICODE_STRING parmsW;
    HRESULT res;

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInstance, debugstr_a(pszParms), nShow);

    if (!pszParms)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&parmsW, pszParms);

    res = UserInstStubWrapperW(hWnd, hInstance, parmsW.Buffer, nShow);

    RtlFreeUnicodeString(&parmsW);

    return res;
}

/***********************************************************************
 *             UserInstStubWrapperW   (ADVPACK.@)
 *
 * Launches the user stub wrapper specified by the RealStubPath
 * registry value under Installed Components\szParms.
 *
 * PARAMS
 *   hWnd      [I] Handle to the window used for the display.
 *   hInstance [I] Instance of the process.
 *   szParms   [I] The GUID of the installation.
 *   show      [I] How the window should be shown.
 *
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 *
 * TODO
 *   If the type of the StubRealPath value is REG_EXPAND_SZ, then
 *   we should call ExpandEnvironmentStrings on the value and
 *   launch the result.
 */
HRESULT WINAPI UserInstStubWrapperW(HWND hWnd, HINSTANCE hInstance,
                                    LPWSTR pszParms, INT nShow)
{
    HKEY setup, guid;
    WCHAR stub[MAX_PATH];
    DWORD size = sizeof(stub);
    HRESULT hr = S_OK;
    BOOL res;

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInstance, debugstr_w(pszParms), nShow);

    if (!pszParms || !*pszParms)
        return E_INVALIDARG;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, setup_key, 0, KEY_READ, &setup))
    {
        return E_FAIL;
    }

    if (RegOpenKeyExW(setup, pszParms, 0, KEY_READ, &guid))
    {
        RegCloseKey(setup);
        return E_FAIL;
    }

    res = RegQueryValueExW(guid, L"RealStubPath", NULL, NULL, (BYTE *)stub, &size);
    if (res || !*stub)
        goto done;

    /* launch the user stub wrapper */
    hr = launch_exe(stub, NULL, NULL);

done:
    RegCloseKey(setup);
    RegCloseKey(guid);

    return hr;
}

/***********************************************************************
 *             UserUnInstStubWrapperA   (ADVPACK.@)
 *
 * See UserUnInstStubWrapperW.
 */
HRESULT WINAPI UserUnInstStubWrapperA(HWND hWnd, HINSTANCE hInstance,
                                      LPSTR pszParms, INT nShow)
{
    UNICODE_STRING parmsW;
    HRESULT res;

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInstance, debugstr_a(pszParms), nShow);

    if (!pszParms)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&parmsW, pszParms);

    res = UserUnInstStubWrapperW(hWnd, hInstance, parmsW.Buffer, nShow);

    RtlFreeUnicodeString(&parmsW);

    return res;
}

/***********************************************************************
 *             UserUnInstStubWrapperW   (ADVPACK.@)
 */
HRESULT WINAPI UserUnInstStubWrapperW(HWND hWnd, HINSTANCE hInstance,
                                      LPWSTR pszParms, INT nShow)
{
    FIXME("(%p, %p, %s, %i): stub\n", hWnd, hInstance, debugstr_w(pszParms), nShow);

    return E_FAIL;
}
