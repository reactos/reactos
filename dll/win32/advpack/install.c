/*
 * Advpack install functions
 *
 * Copyright 2006 James Hawkins
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
#include "ole2.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "advpack_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(advpack);

#define SPAPI_ERROR     0xE0000000L
#define SPAPI_PREFIX    0x800F0000L
#define SPAPI_MASK      0xFFFFL
#define HRESULT_FROM_SPAPI(x)   ((HRESULT)((x & SPAPI_MASK) | SPAPI_PREFIX))

#define ADV_HRESULT(x)  ((x & SPAPI_ERROR) ? HRESULT_FROM_SPAPI(x) : HRESULT_FROM_WIN32(x))

#define ADV_SUCCESS     0
#define ADV_FAILURE     1

/* contains information about a specific install instance */
typedef struct _ADVInfo
{
    HINF hinf;
    LPWSTR inf_path;
    LPWSTR inf_filename;
    LPWSTR install_sec;
    LPWSTR working_dir;
    DWORD flags;
    BOOL need_reboot;
} ADVInfo;

typedef HRESULT (*iterate_fields_func)(HINF hinf, PCWSTR field, const void *arg);

/* Advanced INF commands */
static const WCHAR CheckAdminRights[] = {
    'C','h','e','c','k','A','d','m','i','n','R','i','g','h','t','s',0
};
static const WCHAR DelDirs[] = {'D','e','l','D','i','r','s',0};
static const WCHAR PerUserInstall[] = {'P','e','r','U','s','e','r','I','n','s','t','a','l','l',0};
static const WCHAR RegisterOCXs[] = {'R','e','g','i','s','t','e','r','O','C','X','s',0};
static const WCHAR RunPreSetupCommands[] = {
    'R','u','n','P','r','e','S','e','t','u','p','C','o','m','m','a','n','d','s',0
};
static const WCHAR RunPostSetupCommands[] = {
    'R','u','n','P','o','s','t','S','e','t','u','p','C','o','m','m','a','n','d','s',0
};

/* Advanced INF callbacks */
static HRESULT del_dirs_callback(HINF hinf, PCWSTR field, const void *arg)
{
    INFCONTEXT context;
    HRESULT hr = S_OK;
    DWORD size;

    BOOL ok = SetupFindFirstLineW(hinf, field, NULL, &context);
    
    for (; ok; ok = SetupFindNextLine(&context, &context))
    {
        WCHAR directory[MAX_INF_STRING_LENGTH];

        if (!SetupGetLineTextW(&context, NULL, NULL, NULL, directory,
                               MAX_INF_STRING_LENGTH, &size))
            continue;

        if (DelNodeW(directory, ADN_DEL_IF_EMPTY) != S_OK)
            hr = E_FAIL;
    }

    return hr;
}

static HRESULT per_user_install_callback(HINF hinf, PCWSTR field, const void *arg)
{
    PERUSERSECTIONW per_user;
    INFCONTEXT context;
    DWORD size;

    static const WCHAR disp_name[] = {'D','i','s','p','l','a','y','N','a','m','e',0};
    static const WCHAR version[] = {'V','e','r','s','i','o','n',0};
    static const WCHAR is_installed[] = {'I','s','I','n','s','t','a','l','l','e','d',0};
    static const WCHAR comp_id[] = {'C','o','m','p','o','n','e','n','t','I','D',0};
    static const WCHAR guid[] = {'G','U','I','D',0};
    static const WCHAR locale[] = {'L','o','c','a','l','e',0};
    static const WCHAR stub_path[] = {'S','t','u','b','P','a','t','h',0};

    per_user.bRollback = FALSE;
    per_user.dwIsInstalled = 0;

    SetupGetLineTextW(NULL, hinf, field, disp_name, per_user.szDispName,
                     sizeof(per_user.szDispName) / sizeof(WCHAR), &size);

    SetupGetLineTextW(NULL, hinf, field, version, per_user.szVersion,
                     sizeof(per_user.szVersion) / sizeof(WCHAR), &size);

    if (SetupFindFirstLineW(hinf, field, is_installed, &context))
    {
        SetupGetIntField(&context, 1, (PINT)&per_user.dwIsInstalled);
    }

    SetupGetLineTextW(NULL, hinf, field, comp_id, per_user.szCompID,
                     sizeof(per_user.szCompID) / sizeof(WCHAR), &size);

    SetupGetLineTextW(NULL, hinf, field, guid, per_user.szGUID,
                     sizeof(per_user.szGUID) / sizeof(WCHAR), &size);

    SetupGetLineTextW(NULL, hinf, field, locale, per_user.szLocale,
                     sizeof(per_user.szLocale) / sizeof(WCHAR), &size);

    SetupGetLineTextW(NULL, hinf, field, stub_path, per_user.szStub,
                     sizeof(per_user.szStub) / sizeof(WCHAR), &size);

    return SetPerUserSecValuesW(&per_user);
}

static HRESULT register_ocxs_callback(HINF hinf, PCWSTR field, const void *arg)
{
    HMODULE hm;
    INFCONTEXT context;
    HRESULT hr = S_OK;

    BOOL ok = SetupFindFirstLineW(hinf, field, NULL, &context);
    
    for (; ok; ok = SetupFindNextLine(&context, &context))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];

        /* get OCX filename */
        if (!SetupGetStringFieldW(&context, 1, buffer,
                                  sizeof(buffer) / sizeof(WCHAR), NULL))
            continue;

        hm = LoadLibraryExW(buffer, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (hm)
        {
            if (do_ocx_reg(hm, TRUE, NULL, NULL) != S_OK)
                hr = E_FAIL;

            FreeLibrary(hm);
        }
        else
            hr = E_FAIL;

        if (FAILED(hr))
        {
            /* FIXME: display a message box */
            break;
        }
    }

    return hr;
}

static HRESULT run_setup_commands_callback(HINF hinf, PCWSTR field, const void *arg)
{
    const ADVInfo *info = (const ADVInfo *)arg;
    INFCONTEXT context;
    HRESULT hr = S_OK;
    DWORD size;

    BOOL ok = SetupFindFirstLineW(hinf, field, NULL, &context);

    for (; ok; ok = SetupFindNextLine(&context, &context))
    {
        WCHAR buffer[MAX_INF_STRING_LENGTH];

        if (!SetupGetLineTextW(&context, NULL, NULL, NULL, buffer,
                               MAX_INF_STRING_LENGTH, &size))
            continue;

        if (launch_exe(buffer, info->working_dir, NULL) != S_OK)
            hr = E_FAIL;
    }

    return hr;
}

/* sequentially returns pointers to parameters in a parameter list
 * returns NULL if the parameter is empty, e.g. one,,three  */
LPWSTR get_parameter(LPWSTR *params, WCHAR separator, BOOL quoted)
{
    LPWSTR token = *params;

    if (!*params)
        return NULL;

    if (quoted && *token == '"')
    {
        WCHAR *end = strchrW(token + 1, '"');
        if (end)
        {
            *end = 0;
            *params = end + 1;
            token = token + 1;
        }
    }

    *params = strchrW(*params, separator);
    if (*params)
        *(*params)++ = '\0';

    if (!*token)
        return NULL;

    return token;
}

static BOOL is_full_path(LPCWSTR path)
{
    const int MIN_PATH_LEN = 3;

    if (!path || lstrlenW(path) < MIN_PATH_LEN)
        return FALSE;

    if ((path[1] == ':' && path[2] == '\\') || (path[0] == '\\' && path[1] == '\\'))
        return TRUE;

    return FALSE;
}

/* retrieves the contents of a field, dynamically growing the buffer if necessary */
static WCHAR *get_field_string(INFCONTEXT *context, DWORD index, WCHAR *buffer,
                               const WCHAR *static_buffer, DWORD *size)
{
    DWORD required;

    if (SetupGetStringFieldW(context, index, buffer, *size, &required)) return buffer;

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        /* now grow the buffer */
        if (buffer != static_buffer) HeapFree(GetProcessHeap(), 0, buffer);
        if (!(buffer = HeapAlloc(GetProcessHeap(), 0, required*sizeof(WCHAR)))) return NULL;
        *size = required;
        if (SetupGetStringFieldW(context, index, buffer, *size, &required)) return buffer;
    }

    if (buffer != static_buffer) HeapFree(GetProcessHeap(), 0, buffer);
    return NULL;
}

/* iterates over all fields of a certain key of a certain section */
static HRESULT iterate_section_fields(HINF hinf, PCWSTR section, PCWSTR key,
                                      iterate_fields_func callback, void *arg)
{
    WCHAR static_buffer[200];
    WCHAR *buffer = static_buffer;
    DWORD size = sizeof(static_buffer) / sizeof(WCHAR);
    INFCONTEXT context;
    HRESULT hr = E_FAIL;

    BOOL ok = SetupFindFirstLineW(hinf, section, key, &context);
    while (ok)
    {
        UINT i, count = SetupGetFieldCount(&context);

        for (i = 1; i <= count; i++)
        {
            if (!(buffer = get_field_string(&context, i, buffer, static_buffer, &size)))
                goto done;

            if ((hr = callback(hinf, buffer, arg)) != S_OK)
                goto done;
        }

        ok = SetupFindNextMatchLineW(&context, key, &context);
    }

    hr = S_OK;

 done:
    if (buffer != static_buffer) HeapFree(GetProcessHeap(), 0, buffer);
    return hr;
}

static HRESULT check_admin_rights(const ADVInfo *info)
{
    INT check;
    INFCONTEXT context;
    HRESULT hr = S_OK;

    if (!SetupFindFirstLineW(info->hinf, info->install_sec,
                             CheckAdminRights, &context))
        return S_OK;

    if (!SetupGetIntField(&context, 1, &check))
        return S_OK;

    if (check == 1)
        hr = IsNTAdmin(0, NULL) ? S_OK : E_FAIL;

    return hr;
}

/* performs a setupapi-level install of the INF file */
static HRESULT spapi_install(const ADVInfo *info)
{
    BOOL ret;
    HRESULT res;
    PVOID context;

    context = SetupInitDefaultQueueCallbackEx(NULL, INVALID_HANDLE_VALUE, 0, 0, NULL);
    if (!context)
        return ADV_HRESULT(GetLastError());

    ret = SetupInstallFromInfSectionW(NULL, info->hinf, info->install_sec,
                                      SPINST_FILES, NULL, info->working_dir,
                                      SP_COPY_NEWER, SetupDefaultQueueCallbackW,
                                      context, NULL, NULL);
    if (!ret)
    {
        res = ADV_HRESULT(GetLastError());
        SetupTermDefaultQueueCallback(context);

        return res;
    }

    SetupTermDefaultQueueCallback(context);

    ret = SetupInstallFromInfSectionW(NULL, info->hinf, info->install_sec,
                                      SPINST_INIFILES | SPINST_REGISTRY | SPINST_REGSVR,
                                      HKEY_LOCAL_MACHINE, NULL, 0,
                                      NULL, NULL, NULL, NULL);
    if (!ret)
        return ADV_HRESULT(GetLastError());

    return S_OK;
}

/* processes the Advanced INF commands */
static HRESULT adv_install(ADVInfo *info)
{
    HRESULT hr;

    hr = check_admin_rights(info);
    if (hr != S_OK)
        return hr;

    hr = iterate_section_fields(info->hinf, info->install_sec, RunPreSetupCommands,
                                run_setup_commands_callback, info);
    if (hr != S_OK)
        return hr;

    OleInitialize(NULL);
    hr = iterate_section_fields(info->hinf, info->install_sec,
                                RegisterOCXs, register_ocxs_callback, NULL);
    OleUninitialize();
    if (hr != S_OK)
        return hr;

    hr = iterate_section_fields(info->hinf, info->install_sec,
                                PerUserInstall, per_user_install_callback, info);
    if (hr != S_OK)
        return hr;

    hr = iterate_section_fields(info->hinf, info->install_sec, RunPostSetupCommands,
                                run_setup_commands_callback, info);
    if (hr != S_OK)
        return hr;

    hr = iterate_section_fields(info->hinf, info->install_sec,
                                DelDirs, del_dirs_callback, info);
    if (hr != S_OK)
        return hr;

    return hr;
}

/* determines the proper working directory for the INF file */
static HRESULT get_working_dir(ADVInfo *info, LPCWSTR inf_filename, LPCWSTR working_dir)
{
    WCHAR path[MAX_PATH];
    LPCWSTR ptr;
    DWORD len;

    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR inf_dir[] = {'\\','I','N','F',0};

    if ((ptr = strrchrW(inf_filename, '\\')))
    {
        len = ptr - inf_filename + 1;
        ptr = inf_filename;
    }
    else if (working_dir && *working_dir)
    {
        len = lstrlenW(working_dir) + 1;
        ptr = working_dir;
    }
    else
    {
        GetCurrentDirectoryW(MAX_PATH, path);
        lstrcatW(path, backslash);
        lstrcatW(path, inf_filename);

        /* check if the INF file is in the current directory */
        if (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES)
        {
            GetCurrentDirectoryW(MAX_PATH, path);
        }
        else
        {
            /* default to the windows\inf directory if all else fails */
            GetWindowsDirectoryW(path, MAX_PATH);
            lstrcatW(path, inf_dir);
        }

        len = lstrlenW(path) + 1;
        ptr = path;
    }

    info->working_dir = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!info->working_dir)
        return E_OUTOFMEMORY;

    lstrcpynW(info->working_dir, ptr, len);

    return S_OK;
}

/* loads the INF file and performs checks on it */
static HRESULT install_init(LPCWSTR inf_filename, LPCWSTR install_sec,
                            LPCWSTR working_dir, DWORD flags, ADVInfo *info)
{
    DWORD len;
    HRESULT hr;
    LPCWSTR ptr, path;

    static const WCHAR backslash[] = {'\\',0};
    static const WCHAR default_install[] = {
        'D','e','f','a','u','l','t','I','n','s','t','a','l','l',0
    };

    if (!(ptr = strrchrW(inf_filename, '\\')))
        ptr = inf_filename;

    len = lstrlenW(ptr);

    info->inf_filename = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!info->inf_filename)
        return E_OUTOFMEMORY;

    lstrcpyW(info->inf_filename, ptr);

    /* FIXME: determine the proper platform to install (NTx86, etc) */
    if (!install_sec || !*install_sec)
    {
        len = sizeof(default_install) - 1;
        ptr = default_install;
    }
    else
    {
        len = lstrlenW(install_sec);
        ptr = install_sec;
    }

    info->install_sec = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (!info->install_sec)
        return E_OUTOFMEMORY;

    lstrcpyW(info->install_sec, ptr);

    hr = get_working_dir(info, inf_filename, working_dir);
    if (FAILED(hr))
        return hr;

    len = lstrlenW(info->working_dir) + lstrlenW(info->inf_filename) + 2;
    info->inf_path = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!info->inf_path)
        return E_OUTOFMEMORY;

    lstrcpyW(info->inf_path, info->working_dir);
    lstrcatW(info->inf_path, backslash);
    lstrcatW(info->inf_path, info->inf_filename);

    /* RunSetupCommand opens unmodifed filename parameter */
    if (flags & RSC_FLAG_INF)
        path = inf_filename;
    else
        path = info->inf_path;

    info->hinf = SetupOpenInfFileW(path, NULL, INF_STYLE_WIN4, NULL);
    if (info->hinf == INVALID_HANDLE_VALUE)
        return ADV_HRESULT(GetLastError());

    set_ldids(info->hinf, info->install_sec, info->working_dir);

    /* FIXME: check that the INF is advanced */

    info->flags = flags;
    info->need_reboot = FALSE;

    return S_OK;
}

/* release the install instance information */
static void install_release(const ADVInfo *info)
{
    SetupCloseInfFile(info->hinf);

    HeapFree(GetProcessHeap(), 0, info->inf_path);
    HeapFree(GetProcessHeap(), 0, info->inf_filename);
    HeapFree(GetProcessHeap(), 0, info->install_sec);
    HeapFree(GetProcessHeap(), 0, info->working_dir);
}

/* this structure very closely resembles parameters of RunSetupCommand() */
typedef struct
{
    HWND hwnd;
    LPCSTR title;
    LPCSTR inf_name;
    LPCSTR dir;
    LPCSTR section_name;
} SETUPCOMMAND_PARAMS;

typedef struct
{
    HWND hwnd;
    LPCWSTR title;
    LPCWSTR inf_name;
    LPCWSTR dir;
    LPCWSTR section_name;
} SETUPCOMMAND_PARAMSW;

/* internal: see DoInfInstall */
static HRESULT DoInfInstallW(const SETUPCOMMAND_PARAMSW *setup)
{
    ADVInfo info;
    HRESULT hr;

    TRACE("(%p)\n", setup);

    ZeroMemory(&info, sizeof(ADVInfo));

    hr = install_init(setup->inf_name, setup->section_name, setup->dir, 0, &info);
    if (hr != S_OK)
        goto done;

    hr = spapi_install(&info);
    if (hr != S_OK)
        goto done;

    hr = adv_install(&info);

done:
    install_release(&info);

    return S_OK;
}

/***********************************************************************
 *      DoInfInstall  (ADVPACK.@)
 *
 * Install an INF section.
 *
 * PARAMS
 *  setup [I] Structure containing install information.
 *
 * RETURNS
 *   S_OK                                Everything OK
 *   HRESULT_FROM_WIN32(GetLastError())  Some other error
 */
HRESULT WINAPI DoInfInstall(const SETUPCOMMAND_PARAMS *setup)
{
    UNICODE_STRING title, inf, section, dir;
    SETUPCOMMAND_PARAMSW params;
    HRESULT hr;

    if (!setup)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&title, setup->title);
    RtlCreateUnicodeStringFromAsciiz(&inf, setup->inf_name);
    RtlCreateUnicodeStringFromAsciiz(&section, setup->section_name);
    RtlCreateUnicodeStringFromAsciiz(&dir, setup->dir);

    params.title = title.Buffer;
    params.inf_name = inf.Buffer;
    params.section_name = section.Buffer;
    params.dir = dir.Buffer;
    params.hwnd = setup->hwnd;

    hr = DoInfInstallW(&params);

    RtlFreeUnicodeString(&title);
    RtlFreeUnicodeString(&inf);
    RtlFreeUnicodeString(&section);
    RtlFreeUnicodeString(&dir);

    return hr;
}

/***********************************************************************
 *             ExecuteCabA    (ADVPACK.@)
 *
 * See ExecuteCabW.
 */
HRESULT WINAPI ExecuteCabA(HWND hwnd, CABINFOA* pCab, LPVOID pReserved)
{
    UNICODE_STRING cab, inf, section;
    CABINFOW cabinfo;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", hwnd, pCab, pReserved);

    if (!pCab)
        return E_INVALIDARG;

    if (pCab->pszCab)
    {
        RtlCreateUnicodeStringFromAsciiz(&cab, pCab->pszCab);
        cabinfo.pszCab = cab.Buffer;
    }
    else
        cabinfo.pszCab = NULL;

    RtlCreateUnicodeStringFromAsciiz(&inf, pCab->pszInf);
    RtlCreateUnicodeStringFromAsciiz(&section, pCab->pszSection);
    
    MultiByteToWideChar(CP_ACP, 0, pCab->szSrcPath, -1, cabinfo.szSrcPath,
                        sizeof(cabinfo.szSrcPath) / sizeof(WCHAR));

    cabinfo.pszInf = inf.Buffer;
    cabinfo.pszSection = section.Buffer;
    cabinfo.dwFlags = pCab->dwFlags;

    hr = ExecuteCabW(hwnd, &cabinfo, pReserved);

    if (pCab->pszCab)
        RtlFreeUnicodeString(&cab);

    RtlFreeUnicodeString(&inf);
    RtlFreeUnicodeString(&section);

    return hr;
}

/***********************************************************************
 *             ExecuteCabW    (ADVPACK.@)
 * 
 * Installs the INF file extracted from a specified cabinet file.
 * 
 * PARAMS
 *   hwnd      [I] Handle to the window used for the display.
 *   pCab      [I] Information about the cabinet file.
 *   pReserved [I] Reserved.  Must be NULL.
 * 
 * RETURNS
 *   Success: S_OK.
 *   Failure: E_FAIL.
 */
HRESULT WINAPI ExecuteCabW(HWND hwnd, CABINFOW* pCab, LPVOID pReserved)
{
    ADVInfo info;
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", hwnd, pCab, pReserved);

    ZeroMemory(&info, sizeof(ADVInfo));

    if (pCab->pszCab && *pCab->pszCab)
        FIXME("Cab archive not extracted!\n");

    hr = install_init(pCab->pszInf, pCab->pszSection, pCab->szSrcPath, pCab->dwFlags, &info);
    if (hr != S_OK)
        goto done;

    hr = spapi_install(&info);
    if (hr != S_OK)
        goto done;

    hr = adv_install(&info);

done:
    install_release(&info);

    return hr;
}

/***********************************************************************
 *      LaunchINFSectionA   (ADVPACK.@)
 *
 * See LaunchINFSectionW.
 */
INT WINAPI LaunchINFSectionA(HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show)
{
    UNICODE_STRING cmd;
    HRESULT hr;

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInst, debugstr_a(cmdline), show);

    if (!cmdline)
        return ADV_FAILURE;

    RtlCreateUnicodeStringFromAsciiz(&cmd, cmdline);

    hr = LaunchINFSectionW(hWnd, hInst, cmd.Buffer, show);

    RtlFreeUnicodeString(&cmd);

    return hr;
}

/***********************************************************************
 *      LaunchINFSectionW   (ADVPACK.@)
 *
 * Installs an INF section without BACKUP/ROLLBACK capabilities.
 *
 * PARAMS
 *   hWnd    [I] Handle to parent window, NULL for desktop.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order INF,section,flags,reboot.
 *   show    [I] How the window should be shown.
 *
 * RETURNS
 *  Success: ADV_SUCCESS.
 *  Failure: ADV_FAILURE.
 *
 * NOTES
 *  INF - Filename of the INF to launch.
 *  section - INF section to install.
 *  flags - see advpub.h.
 *  reboot - smart reboot behavior
 *    'A' Always reboot.
 *    'I' Reboot if needed (default).
 *    'N' No reboot.
 */
INT WINAPI LaunchINFSectionW(HWND hWnd, HINSTANCE hInst, LPWSTR cmdline, INT show)
{
    ADVInfo info;
    LPWSTR cmdline_copy, cmdline_ptr;
    LPWSTR inf_filename, install_sec;
    LPWSTR str_flags;
    DWORD flags = 0;
    HRESULT hr = S_OK;

    TRACE("(%p, %p, %s, %d)\n", hWnd, hInst, debugstr_w(cmdline), show);

    if (!cmdline)
        return ADV_FAILURE;

    cmdline_copy = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(cmdline) + 1) * sizeof(WCHAR));
    cmdline_ptr = cmdline_copy;
    lstrcpyW(cmdline_copy, cmdline);

    inf_filename = get_parameter(&cmdline_ptr, ',', TRUE);
    install_sec = get_parameter(&cmdline_ptr, ',', TRUE);

    str_flags = get_parameter(&cmdline_ptr, ',', TRUE);
    if (str_flags)
    {
        DWORD inf_flags = atolW(str_flags);
        if (inf_flags & LIS_QUIET) flags |= RSC_FLAG_QUIET;
        if (inf_flags & LIS_NOGRPCONV) flags |= RSC_FLAG_NGCONV;
    }

    ZeroMemory(&info, sizeof(ADVInfo));

    hr = install_init(inf_filename, install_sec, NULL, flags, &info);
    if (hr != S_OK)
        goto done;

    hr = spapi_install(&info);
    if (hr != S_OK)
        goto done;

    hr = adv_install(&info);

done:
    install_release(&info);
    HeapFree(GetProcessHeap(), 0, cmdline_copy);

    return SUCCEEDED(hr) ? ADV_SUCCESS : ADV_FAILURE;
}

/***********************************************************************
 *      LaunchINFSectionExA (ADVPACK.@)
 *
 * See LaunchINFSectionExW.
 */
HRESULT WINAPI LaunchINFSectionExA(HWND hWnd, HINSTANCE hInst, LPSTR cmdline, INT show)
{
    UNICODE_STRING cmd;
    HRESULT hr;

    TRACE("(%p, %p, %s, %i)\n", hWnd, hInst, debugstr_a(cmdline), show);

    if (!cmdline)
        return ADV_FAILURE;

    RtlCreateUnicodeStringFromAsciiz(&cmd, cmdline);

    hr = LaunchINFSectionExW(hWnd, hInst, cmd.Buffer, show);

    RtlFreeUnicodeString(&cmd);

    return hr;
}

/***********************************************************************
 *      LaunchINFSectionExW (ADVPACK.@)
 *
 * Installs an INF section with BACKUP/ROLLBACK capabilities.
 *
 * PARAMS
 *   hWnd    [I] Handle to parent window, NULL for desktop.
 *   hInst   [I] Instance of the process.
 *   cmdline [I] Contains parameters in the order INF,section,CAB,flags,reboot.
 *   show    [I] How the window should be shown.
 *
 * RETURNS
 *  Success: ADV_SUCCESS.
 *  Failure: ADV_FAILURE.
 *
 * NOTES
 *  INF - Filename of the INF to launch.
 *  section - INF section to install.
 *  flags - see advpub.h.
 *  reboot - smart reboot behavior
 *    'A' Always reboot.
 *    'I' Reboot if needed (default).
 *    'N' No reboot.
 *
 * BUGS
 *  Doesn't handle the reboot flag.
 */
HRESULT WINAPI LaunchINFSectionExW(HWND hWnd, HINSTANCE hInst, LPWSTR cmdline, INT show)
{
    LPWSTR cmdline_copy, cmdline_ptr;
    LPWSTR flags, ptr;
    CABINFOW cabinfo;
    HRESULT hr;

    TRACE("(%p, %p, %s, %d)\n", hWnd, hInst, debugstr_w(cmdline), show);

    if (!cmdline)
        return ADV_FAILURE;

    cmdline_copy = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(cmdline) + 1) * sizeof(WCHAR));
    cmdline_ptr = cmdline_copy;
    lstrcpyW(cmdline_copy, cmdline);

    cabinfo.pszInf = get_parameter(&cmdline_ptr, ',', TRUE);
    cabinfo.pszSection = get_parameter(&cmdline_ptr, ',', TRUE);
    cabinfo.pszCab = get_parameter(&cmdline_ptr, ',', TRUE);
    *cabinfo.szSrcPath = '\0';

    flags = get_parameter(&cmdline_ptr, ',', TRUE);
    if (flags)
        cabinfo.dwFlags = atolW(flags);

    if (!is_full_path(cabinfo.pszCab) && !is_full_path(cabinfo.pszInf))
    {
        HeapFree(GetProcessHeap(), 0, cmdline_copy);
        return E_INVALIDARG;
    }

    /* get the source path from the cab filename */
    if (cabinfo.pszCab && *cabinfo.pszCab)
    {
        if (!is_full_path(cabinfo.pszCab))
            lstrcpyW(cabinfo.szSrcPath, cabinfo.pszInf);
        else
            lstrcpyW(cabinfo.szSrcPath, cabinfo.pszCab);

        ptr = strrchrW(cabinfo.szSrcPath, '\\');
        *(++ptr) = '\0';
    }

    hr = ExecuteCabW(hWnd, &cabinfo, NULL);
    HeapFree(GetProcessHeap(), 0, cmdline_copy);
    return SUCCEEDED(hr) ? ADV_SUCCESS : ADV_FAILURE;
}

HRESULT launch_exe(LPCWSTR cmd, LPCWSTR dir, HANDLE *phEXE)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    if (phEXE) *phEXE = NULL;

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcessW(NULL, (LPWSTR)cmd, NULL, NULL, FALSE,
                        CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_PROCESS_GROUP,
                        NULL, dir, &si, &pi))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    CloseHandle(pi.hThread);

    if (phEXE)
    {
        *phEXE = pi.hProcess;
        return S_ASYNCHRONOUS;
    }

    /* wait for the child process to finish */
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);

    return S_OK;
}

/***********************************************************************
 *      RunSetupCommandA  (ADVPACK.@)
 *
 * See RunSetupCommandW.
 */
HRESULT WINAPI RunSetupCommandA(HWND hWnd, LPCSTR szCmdName,
                                LPCSTR szInfSection, LPCSTR szDir,
                                LPCSTR lpszTitle, HANDLE *phEXE,
                                DWORD dwFlags, LPVOID pvReserved)
{
    UNICODE_STRING cmdname, infsec;
    UNICODE_STRING dir, title;
    HRESULT hr;

    TRACE("(%p, %s, %s, %s, %s, %p, %d, %p)\n",
          hWnd, debugstr_a(szCmdName), debugstr_a(szInfSection),
          debugstr_a(szDir), debugstr_a(lpszTitle),
          phEXE, dwFlags, pvReserved);

    if (!szCmdName || !szDir)
        return E_INVALIDARG;

    RtlCreateUnicodeStringFromAsciiz(&cmdname, szCmdName);
    RtlCreateUnicodeStringFromAsciiz(&infsec, szInfSection);
    RtlCreateUnicodeStringFromAsciiz(&dir, szDir);
    RtlCreateUnicodeStringFromAsciiz(&title, lpszTitle);

    hr = RunSetupCommandW(hWnd, cmdname.Buffer, infsec.Buffer, dir.Buffer,
                          title.Buffer, phEXE, dwFlags, pvReserved);

    RtlFreeUnicodeString(&cmdname);
    RtlFreeUnicodeString(&infsec);
    RtlFreeUnicodeString(&dir);
    RtlFreeUnicodeString(&title);

    return hr;
}

/***********************************************************************
 *      RunSetupCommandW  (ADVPACK.@)
 *
 * Executes an install section in an INF file or a program.
 *
 * PARAMS
 *   hWnd          [I] Handle to parent window, NULL for quiet mode
 *   szCmdName     [I] Inf or EXE filename to execute
 *   szInfSection  [I] Inf section to install, NULL for DefaultInstall
 *   szDir         [I] Path to extracted files
 *   szTitle       [I] Title of all dialogs
 *   phEXE         [O] Handle of EXE to wait for
 *   dwFlags       [I] Flags; see include/advpub.h
 *   pvReserved    [I] Reserved
 *
 * RETURNS
 *   S_OK                                 Everything OK
 *   S_ASYNCHRONOUS                       OK, required to wait on phEXE
 *   ERROR_SUCCESS_REBOOT_REQUIRED        Reboot required
 *   E_INVALIDARG                         Invalid argument given
 *   HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION)
 *                                        Not supported on this Windows version
 *   E_UNEXPECTED                         Unexpected error
 *   HRESULT_FROM_WIN32(GetLastError())   Some other error
 */
HRESULT WINAPI RunSetupCommandW(HWND hWnd, LPCWSTR szCmdName,
                                LPCWSTR szInfSection, LPCWSTR szDir,
                                LPCWSTR lpszTitle, HANDLE *phEXE,
                                DWORD dwFlags, LPVOID pvReserved)
{
    ADVInfo info;
    HRESULT hr;

    TRACE("(%p, %s, %s, %s, %s, %p, %d, %p)\n",
          hWnd, debugstr_w(szCmdName), debugstr_w(szInfSection),
          debugstr_w(szDir), debugstr_w(lpszTitle),
          phEXE, dwFlags, pvReserved);

    if (dwFlags & RSC_FLAG_UPDHLPDLLS)
        FIXME("Unhandled flag: RSC_FLAG_UPDHLPDLLS\n");

    if (!szCmdName || !szDir)
        return E_INVALIDARG;

    if (!(dwFlags & RSC_FLAG_INF))
        return launch_exe(szCmdName, szDir, phEXE);

    ZeroMemory(&info, sizeof(ADVInfo));

    hr = install_init(szCmdName, szInfSection, szDir, dwFlags, &info);
    if (hr != S_OK)
        goto done;

    hr = spapi_install(&info);
    if (hr != S_OK)
        goto done;

    hr = adv_install(&info);

done:
    install_release(&info);

    return hr;
}
