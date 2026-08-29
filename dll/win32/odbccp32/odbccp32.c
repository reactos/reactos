/*
 * Implementation of the ODBC driver installer
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
 * Copyright 2005 Hans Leidekker
 * Copyright 2007 Bill Medland
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

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "sqlext.h"
#ifdef __REACTOS__
#undef TRACE_ON
#endif
#include "wine/debug.h"

#include "odbcinst.h"

WINE_DEFAULT_DEBUG_CHANNEL(odbc);

/* This config mode is known to be process-wide.
 * MSDN documentation suggests that the value is hidden somewhere in the registry but I haven't found it yet.
 * Although both the registry and the ODBC.ini files appear to be maintained together they are not maintained automatically through the registry's IniFileMapping.
 */
static UWORD config_mode = ODBC_BOTH_DSN;

/* MSDN documentation suggests that the error subsystem handles errors 1 to 8
 * only and experimentation (Windows 2000) shows that the errors are process-
 * wide so go for the simple solution; static arrays.
 */
static int num_errors;
static int error_code[8];
static const WCHAR *error_msg[8];

static BOOL (WINAPI *pConfigDSN)(HWND hwnd, WORD request, const char *driver, const char *attr);
static BOOL (WINAPI *pConfigDSNW)(HWND hwnd, WORD request, const WCHAR *driver, const WCHAR *attr);

/* Push an error onto the error stack, taking care of ranges etc. */
static void push_error(int code, LPCWSTR msg)
{
    if (num_errors < ARRAY_SIZE(error_code))
    {
        error_code[num_errors] = code;
        error_msg[num_errors] = msg;
        num_errors++;
    }
}

/* Clear the error stack */
static void clear_errors(void)
{
    num_errors = 0;
}

static inline WCHAR *strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = malloc(len * sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}


BOOL WINAPI ODBCCPlApplet( LONG i, LONG j, LONG * p1, LONG * p2)
{
    clear_errors();
    FIXME( "( %ld %ld %p %p) : stub!\n", i, j, p1, p2);
    return FALSE;
}

static LPWSTR SQLInstall_strdup_multi(LPCSTR str)
{
    LPCSTR p;
    LPWSTR ret = NULL;
    DWORD len;

    if (!str)
        return ret;

    for (p = str; *p; p += lstrlenA(p) + 1)
        ;

    len = MultiByteToWideChar(CP_ACP, 0, str, p - str, NULL, 0 );
    ret = malloc((len + 1) * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, p - str, ret, len );
    ret[len] = 0;

    return ret;
}

static LPSTR SQLInstall_strdup_multiWtoA(LPCWSTR str)
{
    LPCWSTR p;
    LPSTR ret = NULL;
    DWORD len;

    if (!str)
        return ret;

    for (p = str; *p; p += lstrlenW(p) + 1)
        ;

    len = WideCharToMultiByte(CP_ACP, 0, str,   p - str, NULL, 0, NULL, NULL );
    ret = malloc((len + 1));
    WideCharToMultiByte(CP_ACP, 0, str, p - str, ret, len, NULL, NULL );
    ret[len] = 0;

    return ret;
}

static inline char *strdupWtoA( const WCHAR *str )
{
    char *ret = NULL;
    if (str)
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
        if ((ret = malloc( len )))
            WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    }
    return ret;
}

static LPWSTR SQLInstall_strdup(LPCSTR str)
{
    DWORD len;
    LPWSTR ret = NULL;

    if (!str)
        return ret;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0 );
    ret = malloc(len * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len );

    return ret;
}

/* Convert the wide string or zero-length-terminated list of wide strings to a
 * narrow string or zero-length-terminated list of narrow strings.
 * Do not try to emulate windows undocumented excesses (e.g. adding a third \0
 * to a list)
 * Arguments
 *   mode Indicates the sort of string.
 *     1 denotes that the buffers contain strings terminated by a single nul
 *       character
 *     2 denotes that the buffers contain zero-length-terminated lists
 *       (frequently erroneously referred to as double-null-terminated)
 *   buffer The narrow-character buffer into which to place the result.  This
 *          must be a non-null pointer to the first element of a buffer whose
 *          length is passed in buffer_length.
 *   str The wide-character buffer containing the string or list of strings to
 *       be converted.  str_length defines how many wide characters in the
 *       buffer are to be converted, including all desired terminating nul
 *       characters.
 *   str_length Effective length of str
 *   buffer_length Length of buffer
 *   returned_length A pointer to a variable that will receive the number of
 *                   narrow characters placed into the buffer.  This pointer
 *                   may be NULL.
 */
static BOOL SQLInstall_narrow(int mode, LPSTR buffer, LPCWSTR str, WORD str_length, WORD buffer_length, WORD *returned_length)
{
    LPSTR pbuf; /* allows us to allocate a temporary buffer only if needed */
    int len; /* Length of the converted list */
    BOOL success = FALSE;
    assert(mode == 1 || mode == 2);
    assert(buffer_length);
    len = WideCharToMultiByte(CP_ACP, 0, str, str_length, 0, 0, NULL, NULL);
    if (len > 0)
    {
        if (len > buffer_length)
        {
            pbuf = malloc(len);
        }
        else
        {
            pbuf = buffer;
        }
        len = WideCharToMultiByte(CP_ACP, 0, str, str_length, pbuf, len, NULL, NULL);
        if (len > 0)
        {
            if (pbuf != buffer)
            {
                if (buffer_length > (mode - 1))
                {
                    memcpy (buffer, pbuf, buffer_length-mode);
                    *(buffer+buffer_length-mode) = '\0';
                }
                *(buffer+buffer_length-1) = '\0';
            }
            if (returned_length)
            {
                *returned_length = pbuf == buffer ? len : buffer_length;
            }
            success = TRUE;
        }
        else
        {
            ERR("transferring wide to narrow\n");
        }
        if (pbuf != buffer)
        {
            free(pbuf);
        }
    }
    else
    {
        ERR("measuring wide to narrow\n");
    }
    return success;
}

static HMODULE load_config_driver(const WCHAR *driver)
{
    long ret;
    HMODULE hmod;
    WCHAR *filename = NULL;
    DWORD size = 0, type;
    HKEY hkey;

    if ((ret = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBCINST.INI\\", &hkey)) == ERROR_SUCCESS)
    {
        HKEY hkeydriver;

        if ((ret = RegOpenKeyW(hkey, driver, &hkeydriver)) == ERROR_SUCCESS)
        {
            ret = RegGetValueW(hkeydriver, NULL, L"Setup", RRF_RT_REG_SZ, &type, NULL, &size);
            if(ret != ERROR_SUCCESS || type != REG_SZ)
            {
                RegCloseKey(hkeydriver);
                RegCloseKey(hkey);
                push_error(ODBC_ERROR_INVALID_DSN, L"Invalid DSN");

                return NULL;
            }

            filename = malloc(size);
            if(!filename)
            {
                RegCloseKey(hkeydriver);
                RegCloseKey(hkey);
                push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");

                return NULL;
            }
            ret = RegGetValueW(hkeydriver, NULL, L"Setup", RRF_RT_REG_SZ, &type, filename, &size);

            RegCloseKey(hkeydriver);
        }

        RegCloseKey(hkey);
    }

    if(ret != ERROR_SUCCESS)
    {
        free(filename);
        push_error(ODBC_ERROR_COMPONENT_NOT_FOUND, L"Component not found");
        return NULL;
    }

    hmod = LoadLibraryExW(filename, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    free(filename);

    if(!hmod)
        push_error(ODBC_ERROR_LOAD_LIB_FAILED, L"Load Library Failed");

    return hmod;
}

static BOOL write_config_value(const WCHAR *driver, const WCHAR *args)
{
    long ret;
    HKEY hkey, hkeydriver;
    WCHAR *name = NULL;

    if(!args)
        return FALSE;

    if((ret = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBCINST.INI\\", &hkey)) == ERROR_SUCCESS)
    {
        if((ret = RegOpenKeyW(hkey, driver, &hkeydriver)) == ERROR_SUCCESS)
        {
            WCHAR *divider, *value;

            name = malloc((wcslen(args) + 1) * sizeof(WCHAR));
            if(!name)
            {
                push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");
                goto fail;
            }
            lstrcpyW(name, args);

            divider = wcschr(name,'=');
            if(!divider)
            {
                push_error(ODBC_ERROR_INVALID_KEYWORD_VALUE, L"Invalid keyword value");
                goto fail;
            }

            value = divider + 1;
            *divider = '\0';

            TRACE("Write pair: %s = %s\n", debugstr_w(name), debugstr_w(value));
            if(RegSetValueExW(hkeydriver, name, 0, REG_SZ, (BYTE*)value,
                               (lstrlenW(value)+1) * sizeof(WCHAR)) != ERROR_SUCCESS)
                ERR("Failed to write registry installed key\n");
            free(name);

            RegCloseKey(hkeydriver);
        }

        RegCloseKey(hkey);
    }

    if(ret != ERROR_SUCCESS)
        push_error(ODBC_ERROR_COMPONENT_NOT_FOUND, L"Component not found");

    return ret == ERROR_SUCCESS;

fail:
    RegCloseKey(hkeydriver);
    RegCloseKey(hkey);
    free(name);

    return FALSE;
}

static WORD map_request(WORD request)
{
    switch (request)
    {
    case ODBC_ADD_DSN:
    case ODBC_ADD_SYS_DSN:
        return ODBC_ADD_DSN;

    case ODBC_CONFIG_DSN:
    case ODBC_CONFIG_SYS_DSN:
        return ODBC_CONFIG_DSN;

    case ODBC_REMOVE_DSN:
    case ODBC_REMOVE_SYS_DSN:
        return ODBC_REMOVE_DSN;

    default:
        FIXME("unhandled request %u\n", request);
        return 0;
    }
}

static UWORD get_config_mode(WORD request)
{
    if (request == ODBC_ADD_DSN || request == ODBC_CONFIG_DSN || request == ODBC_REMOVE_DSN) return ODBC_USER_DSN;
    return ODBC_SYSTEM_DSN;
}

BOOL WINAPI SQLConfigDataSourceW(HWND hwnd, WORD request, LPCWSTR driver, LPCWSTR attributes)
{
    HMODULE mod;
    BOOL ret = FALSE;
    UWORD config_mode_prev = config_mode;
    WORD mapped_request;

    TRACE("%p, %d, %s, %s\n", hwnd, request, debugstr_w(driver), debugstr_w(attributes));
    if (TRACE_ON(odbc) && attributes)
    {
        const WCHAR *p;
        for (p = attributes; *p; p += lstrlenW(p) + 1)
            TRACE("%s\n", debugstr_w(p));
    }

    clear_errors();

    mapped_request = map_request(request);
    if (!mapped_request)
        return FALSE;

    mod = load_config_driver(driver);
    if (!mod)
        return FALSE;

    config_mode = get_config_mode(request);

    pConfigDSNW = (void*)GetProcAddress(mod, "ConfigDSNW");
    if(pConfigDSNW)
        ret = pConfigDSNW(hwnd, mapped_request, driver, attributes);
    else
    {
        pConfigDSN = (void*)GetProcAddress(mod, "ConfigDSN");
        if (pConfigDSN)
        {
            LPSTR attr = SQLInstall_strdup_multiWtoA(attributes);
            char *driverA = strdupWtoA(driver);
            TRACE("Calling ConfigDSN\n");

            ret = pConfigDSN(hwnd, mapped_request, driverA, attr);
            free(attr);
            free(driverA);
        }
        else
            ERR("Failed to find ConfigDSN/W\n");
    }

    config_mode = config_mode_prev;

    if (!ret)
        push_error(ODBC_ERROR_REQUEST_FAILED, L"Request Failed");

    FreeLibrary(mod);

    return ret;
}

BOOL WINAPI SQLConfigDataSource(HWND hwnd, WORD request, LPCSTR driver, LPCSTR attributes)
{
    HMODULE mod;
    BOOL ret = FALSE;
    WCHAR *driverW;
    UWORD config_mode_prev = config_mode;
    WORD mapped_request;

    TRACE("%p, %d, %s, %s\n", hwnd, request, debugstr_a(driver), debugstr_a(attributes));

    if (TRACE_ON(odbc) && attributes)
    {
        const char *p;
        for (p = attributes; *p; p += lstrlenA(p) + 1)
            TRACE("%s\n", debugstr_a(p));
    }

    clear_errors();

    mapped_request = map_request(request);
    if (!mapped_request)
        return FALSE;

    driverW = strdupAtoW(driver);
    if (!driverW)
    {
        push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");
        return FALSE;
    }

    mod = load_config_driver(driverW);
    if (!mod)
    {
        free(driverW);
        return FALSE;
    }

    config_mode = get_config_mode(request);

    pConfigDSN = (void*)GetProcAddress(mod, "ConfigDSN");
    if (pConfigDSN)
    {
        TRACE("Calling ConfigDSN\n");
        ret = pConfigDSN(hwnd, mapped_request, driver, attributes);
    }
    else
    {
        pConfigDSNW = (void*)GetProcAddress(mod, "ConfigDSNW");
        if (pConfigDSNW)
        {
            WCHAR *attr = NULL;
            TRACE("Calling ConfigDSNW\n");

            attr = SQLInstall_strdup_multi(attributes);
            if(attr)
                ret = pConfigDSNW(hwnd, mapped_request, driverW, attr);
            free(attr);
        }
    }

    config_mode = config_mode_prev;

    if (!ret)
        push_error(ODBC_ERROR_REQUEST_FAILED, L"Request Failed");

    free(driverW);
    FreeLibrary(mod);

    return ret;
}

BOOL WINAPI SQLConfigDriverW(HWND hwnd, WORD request, LPCWSTR driver,
               LPCWSTR args, LPWSTR msg, WORD msgmax, WORD *msgout)
{
    BOOL (WINAPI *pConfigDriverW)(HWND hwnd, WORD request, const WCHAR *driver, const WCHAR *args, const WCHAR *msg, WORD msgmax, WORD *msgout);
    HMODULE hmod;
    BOOL funcret = FALSE;

    clear_errors();
    TRACE("(%p %d %s %s %p %d %p)\n", hwnd, request, debugstr_w(driver),
          debugstr_w(args), msg, msgmax, msgout);

    if(request == ODBC_CONFIG_DRIVER)
    {
        return write_config_value(driver, args);
    }

    hmod = load_config_driver(driver);
    if(!hmod)
        return FALSE;

    pConfigDriverW = (void*)GetProcAddress(hmod, "ConfigDriverW");
    if(pConfigDriverW)
        funcret = pConfigDriverW(hwnd, request, driver, args, msg, msgmax, msgout);

    if(!funcret)
        push_error(ODBC_ERROR_REQUEST_FAILED, L"Request Failed");

    FreeLibrary(hmod);

    return funcret;
}

BOOL WINAPI SQLConfigDriver(HWND hwnd, WORD request, LPCSTR driver,
               LPCSTR args, LPSTR msg, WORD msgmax, WORD *msgout)
{
    BOOL (WINAPI *pConfigDriverA)(HWND hwnd, WORD request, const char *driver, const char *args, const char *msg, WORD msgmax, WORD *msgout);
    HMODULE hmod;
    WCHAR *driverW;
    BOOL funcret = FALSE;

    clear_errors();
    TRACE("(%p %d %s %s %p %d %p)\n", hwnd, request, debugstr_a(driver),
          debugstr_a(args), msg, msgmax, msgout);

    driverW = strdupAtoW(driver);
    if(!driverW)
    {
        push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");
        return FALSE;
    }
    if(request == ODBC_CONFIG_DRIVER)
    {
        BOOL ret = FALSE;
        WCHAR *argsW = strdupAtoW(args);
        if(argsW)
        {
            ret = write_config_value(driverW, argsW);
            free(argsW);
        }
        else
        {
            push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");
        }

        free(driverW);

        return ret;
    }

    hmod = load_config_driver(driverW);
    free(driverW);
    if(!hmod)
        return FALSE;

    pConfigDriverA = (void*)GetProcAddress(hmod, "ConfigDriver");
    if(pConfigDriverA)
        funcret = pConfigDriverA(hwnd, request, driver, args, msg, msgmax, msgout);

    if(!funcret)
        push_error(ODBC_ERROR_REQUEST_FAILED, L"Request Failed");

    FreeLibrary(hmod);

    return funcret;
}

BOOL WINAPI SQLCreateDataSourceW(HWND hwnd, LPCWSTR lpszDS)
{
    clear_errors();
    FIXME("%p %s\n", hwnd, debugstr_w(lpszDS));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLCreateDataSource(HWND hwnd, LPCSTR lpszDS)
{
    clear_errors();
    FIXME("%p %s\n", hwnd, debugstr_a(lpszDS));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetAvailableDriversW(LPCWSTR lpszInfFile, LPWSTR lpszBuf,
               WORD cbBufMax, WORD *pcbBufOut)
{
    clear_errors();
    FIXME("%s %p %d %p\n", debugstr_w(lpszInfFile), lpszBuf, cbBufMax, pcbBufOut);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetAvailableDrivers(LPCSTR lpszInfFile, LPSTR lpszBuf,
               WORD cbBufMax, WORD *pcbBufOut)
{
    clear_errors();
    FIXME("%s %p %d %p\n", debugstr_a(lpszInfFile), lpszBuf, cbBufMax, pcbBufOut);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetConfigMode(UWORD *pwConfigMode)
{
    clear_errors();
    TRACE("%p\n", pwConfigMode);
    if (pwConfigMode)
        *pwConfigMode = config_mode;
    return TRUE;
}

BOOL WINAPI SQLGetInstalledDriversW(WCHAR *buf, WORD size, WORD *sizeout)
{
    WORD written = 0;
    DWORD index = 0;
    BOOL ret = TRUE;
    DWORD valuelen;
    WCHAR *value;
    HKEY drivers;
    DWORD len;
    LONG res;

    clear_errors();

    TRACE("%p %d %p\n", buf, size, sizeout);

    if (!buf || !size)
    {
        push_error(ODBC_ERROR_INVALID_BUFF_LEN, L"Invalid buffer length");
        return FALSE;
    }

    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBCINST.INI\\ODBC Drivers", 0,
                        KEY_QUERY_VALUE, &drivers);
    if (res)
    {
        push_error(ODBC_ERROR_COMPONENT_NOT_FOUND, L"Component not found");
        return FALSE;
    }

    valuelen = 256;
    value = malloc(valuelen * sizeof(WCHAR));

    size--;

    while (1)
    {
        len = valuelen;
        res = RegEnumValueW(drivers, index, value, &len, NULL, NULL, NULL, NULL);
        while (res == ERROR_MORE_DATA)
        {
            value = realloc(value, ++len * sizeof(WCHAR));
            res = RegEnumValueW(drivers, index, value, &len, NULL, NULL, NULL, NULL);
        }
        if (res == ERROR_SUCCESS)
        {
            lstrcpynW(buf + written, value, size - written);
            written += min(len + 1, size - written);
        }
        else if (res == ERROR_NO_MORE_ITEMS)
            break;
        else
        {
            push_error(ODBC_ERROR_GENERAL_ERR, L"General error");
            ret = FALSE;
            break;
        }
        index++;
    }

    buf[written++] = 0;

    free(value);
    RegCloseKey(drivers);
    if (sizeout)
        *sizeout = written;
    return ret;
}

BOOL WINAPI SQLGetInstalledDrivers(char *buf, WORD size, WORD *sizeout)
{
    WORD written;
    WCHAR *wbuf;
    BOOL ret;

    TRACE("%p %d %p\n", buf, size, sizeout);

    if (!buf || !size)
    {
        push_error(ODBC_ERROR_INVALID_BUFF_LEN, L"Invalid buffer length");
        return FALSE;
    }

    wbuf = malloc(size * sizeof(WCHAR));
    if (!wbuf)
    {
        push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");
        return FALSE;
    }

    ret = SQLGetInstalledDriversW(wbuf, size, &written);
    if (!ret)
    {
        free(wbuf);
        return FALSE;
    }

    if (sizeout)
        *sizeout = WideCharToMultiByte(CP_ACP, 0, wbuf, written, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, wbuf, written, buf, size, NULL, NULL);

    free(wbuf);
    return TRUE;
}

static HKEY get_privateprofile_sectionkey(HKEY root, const WCHAR *section, const WCHAR *filename)
{
    HKEY hkey, hkeyfilename, hkeysection;
    LONG ret;

    if (RegOpenKeyW(root, L"Software\\ODBC", &hkey))
        return NULL;

    ret = RegOpenKeyW(hkey, filename, &hkeyfilename);
    RegCloseKey(hkey);
    if (ret)
        return NULL;

    ret = RegOpenKeyW(hkeyfilename, section, &hkeysection);
    RegCloseKey(hkeyfilename);

    return ret ? NULL : hkeysection;
}

int WINAPI SQLGetPrivateProfileStringW(LPCWSTR section, LPCWSTR entry,
    LPCWSTR defvalue, LPWSTR buff, int buff_len, LPCWSTR filename)
{
    BOOL usedefault = TRUE;
    HKEY sectionkey;
    LONG ret = 0;

    TRACE("%s %s %s %p %d %s\n", debugstr_w(section), debugstr_w(entry),
               debugstr_w(defvalue), buff, buff_len, debugstr_w(filename));

    clear_errors();

    if (buff_len <= 0 || !section)
        return 0;

    if(buff)
        buff[0] = 0;

    if (!defvalue || !buff)
        return 0;

    /* odbcinit.ini is only for drivers, so default to local Machine */
    if (!wcsicmp(filename, L"ODBCINST.INI") || config_mode == ODBC_SYSTEM_DSN)
        sectionkey = get_privateprofile_sectionkey(HKEY_LOCAL_MACHINE, section, filename);
    else if (config_mode == ODBC_USER_DSN)
        sectionkey = get_privateprofile_sectionkey(HKEY_CURRENT_USER, section, filename);
    else
    {
        sectionkey = get_privateprofile_sectionkey(HKEY_CURRENT_USER, section, filename);
        if (!sectionkey) sectionkey = get_privateprofile_sectionkey(HKEY_LOCAL_MACHINE, section, filename);
    }

    if (sectionkey)
    {
        DWORD type, size;

        if (entry)
        {
            size = buff_len * sizeof(*buff);
            if (RegGetValueW(sectionkey, NULL, entry, RRF_RT_REG_SZ, &type, buff, &size) == ERROR_SUCCESS)
            {
                usedefault = FALSE;
                ret = (size / sizeof(*buff)) - 1;
            }
        }
        else
        {
            WCHAR name[MAX_PATH];
            DWORD index = 0;
            DWORD namelen;

            usedefault = FALSE;

            memset(buff, 0, buff_len);

            namelen = sizeof(name);
            while (RegEnumValueW(sectionkey, index, name, &namelen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                if ((ret +  namelen+1) > buff_len)
                    break;

                lstrcpyW(buff+ret, name);
                ret += namelen+1;
                namelen = sizeof(name);
                index++;
            }
        }

        RegCloseKey(sectionkey);
    }
    else
        usedefault = entry != NULL;

    if (usedefault)
    {
        lstrcpynW(buff, defvalue, buff_len);
        ret = lstrlenW(buff);
    }

    return ret;
}

int WINAPI SQLGetPrivateProfileString(LPCSTR section, LPCSTR entry,
    LPCSTR defvalue, LPSTR buff, int buff_len, LPCSTR filename)
{
    WCHAR *sectionW, *filenameW;
    BOOL usedefault = TRUE;
    HKEY sectionkey;
    LONG ret = 0;

    TRACE("%s %s %s %p %d %s\n", debugstr_a(section), debugstr_a(entry),
               debugstr_a(defvalue), buff, buff_len, debugstr_a(filename));

    clear_errors();

    if (buff_len <= 0)
        return 0;

    if (buff)
        buff[0] = 0;

    if (!section || !defvalue || !buff)
        return 0;

    if (!(sectionW = strdupAtoW(section))) return 0;
    if (!(filenameW = strdupAtoW(filename)))
    {
        free(sectionW);
        return 0;
    }

    if (config_mode == ODBC_USER_DSN)
        sectionkey = get_privateprofile_sectionkey(HKEY_CURRENT_USER, sectionW, filenameW);
    else if (config_mode == ODBC_SYSTEM_DSN)
        sectionkey = get_privateprofile_sectionkey(HKEY_LOCAL_MACHINE, sectionW, filenameW);
    else
    {
        sectionkey = get_privateprofile_sectionkey(HKEY_CURRENT_USER, sectionW, filenameW);
        if (!sectionkey) sectionkey = get_privateprofile_sectionkey(HKEY_LOCAL_MACHINE, sectionW, filenameW);
    }

    free(sectionW);
    free(filenameW);

    if (sectionkey)
    {
        DWORD type, size;

        if (entry)
        {
            size = buff_len * sizeof(*buff);
            if (RegGetValueA(sectionkey, NULL, entry, RRF_RT_REG_SZ, &type, buff, &size) == ERROR_SUCCESS)
            {
                usedefault = FALSE;
                ret = (size / sizeof(*buff)) - 1;
            }
        }
        else
        {
            char name[MAX_PATH] = {0};
            DWORD index = 0;
            DWORD namelen;

            usedefault = FALSE;

            memset(buff, 0, buff_len);

            namelen = sizeof(name);
            while (RegEnumValueA(sectionkey, index, name, &namelen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
            {
                if ((ret +  namelen+1) > buff_len)
                    break;

                lstrcpyA(buff+ret, name);

                ret += namelen+1;
                namelen = sizeof(name);
                index++;
            }
        }

        RegCloseKey(sectionkey);
    }
    else
        usedefault = entry != NULL;

    if (usedefault)
    {
        lstrcpynA(buff, defvalue, buff_len);
        ret = strlen(buff);
    }

    return ret;
}

BOOL WINAPI SQLGetTranslatorW(HWND hwndParent, LPWSTR lpszName, WORD cbNameMax,
               WORD *pcbNameOut, LPWSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut, DWORD *pvOption)
{
    clear_errors();
    FIXME("%p %s %d %p %p %d %p %p\n", hwndParent, debugstr_w(lpszName), cbNameMax,
               pcbNameOut, lpszPath, cbPathMax, pcbPathOut, pvOption);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetTranslator(HWND hwndParent, LPSTR lpszName, WORD cbNameMax,
               WORD *pcbNameOut, LPSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut, DWORD *pvOption)
{
    clear_errors();
    FIXME("%p %s %d %p %p %d %p %p\n", hwndParent, debugstr_a(lpszName), cbNameMax,
               pcbNameOut, lpszPath, cbPathMax, pcbPathOut, pvOption);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLInstallDriverW(LPCWSTR lpszInfFile, LPCWSTR lpszDriver,
               LPWSTR lpszPath, WORD cbPathMax, WORD * pcbPathOut)
{
    DWORD usage;

    clear_errors();
    TRACE("%s %s %p %d %p\n", debugstr_w(lpszInfFile),
          debugstr_w(lpszDriver), lpszPath, cbPathMax, pcbPathOut);

    if (lpszInfFile)
        return FALSE;

    return SQLInstallDriverExW(lpszDriver, NULL, lpszPath, cbPathMax,
                               pcbPathOut, ODBC_INSTALL_COMPLETE, &usage);
}

BOOL WINAPI SQLInstallDriver(LPCSTR lpszInfFile, LPCSTR lpszDriver,
               LPSTR lpszPath, WORD cbPathMax, WORD * pcbPathOut)
{
    DWORD usage;

    clear_errors();
    TRACE("%s %s %p %d %p\n", debugstr_a(lpszInfFile),
          debugstr_a(lpszDriver), lpszPath, cbPathMax, pcbPathOut);

    if (lpszInfFile)
        return FALSE;

    return SQLInstallDriverEx(lpszDriver, NULL, lpszPath, cbPathMax,
                              pcbPathOut, ODBC_INSTALL_COMPLETE, &usage);
}

static void write_registry_values(const WCHAR *regkey, const WCHAR *driver, const  WCHAR *path_in, WCHAR *path,
                                  DWORD *usage_count)
{
    HKEY hkey, hkeydriver;

    if (RegCreateKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBCINST.INI\\", &hkey) == ERROR_SUCCESS)
    {
        if (RegCreateKeyW(hkey, regkey, &hkeydriver) == ERROR_SUCCESS)
        {
            if(RegSetValueExW(hkeydriver, driver, 0, REG_SZ, (BYTE*)L"Installed", sizeof(L"Installed")) != ERROR_SUCCESS)
                ERR("Failed to write registry installed key\n");

            RegCloseKey(hkeydriver);
        }

        if (RegCreateKeyW(hkey, driver, &hkeydriver) == ERROR_SUCCESS)
        {
            WCHAR entry[1024];
            const WCHAR *p;
            DWORD usagecount = 0;
            DWORD type, size;

            /* Skip name entry */
            p = driver;
            p += lstrlenW(p) + 1;

            if (!path_in)
                GetSystemDirectoryW(path, MAX_PATH);
            else
                lstrcpyW(path, path_in);

            /* Store Usage */
            size = sizeof(usagecount);
            RegGetValueA(hkeydriver, NULL, "UsageCount", RRF_RT_DWORD, &type, &usagecount, &size);
            TRACE("Usage count %ld\n", usagecount);

            for (; *p; p += lstrlenW(p) + 1)
            {
                WCHAR *divider = wcschr(p,'=');

                if (divider)
                {
                    WCHAR *value;
                    int len;

                    /* Write pair values to the registry. */
                    lstrcpynW(entry, p, divider - p + 1);

                    divider++;
                    TRACE("Writing pair %s,%s\n", debugstr_w(entry), debugstr_w(divider));

                    /* Driver, Setup, Translator entries use the system path unless a path is specified. */
                    if(lstrcmpiW(L"Driver", entry) == 0 || lstrcmpiW(L"Setup", entry) == 0 ||
                       lstrcmpiW(L"Translator", entry) == 0)
                    {
                        if(GetFileAttributesW(divider) == INVALID_FILE_ATTRIBUTES)
                        {
                            int pathlen = lstrlenW(path);
                            len = pathlen + 1 + lstrlenW(divider) + 1;
                            value = malloc(len * sizeof(WCHAR));
                            if(!value)
                            {
                                RegCloseKey(hkeydriver);
                                ERR("Out of memory\n");
                                return;
                            }

                            lstrcpyW(value, path);
                            if (pathlen && path[pathlen - 1] != '\\')
                                lstrcatW(value, L"\\");
                        }
                        else
                        {
                            value = calloc(1, (lstrlenW(divider)+1) * sizeof(WCHAR));
                            if(!value)
                            {
                                RegCloseKey(hkeydriver);
                                ERR("Out of memory\n");
                                return;
                            }
                        }
                        lstrcatW(value, divider);
                    }
                    else
                    {
                        len = lstrlenW(divider) + 1;
                        value = malloc(len * sizeof(WCHAR));
                        lstrcpyW(value, divider);
                    }

                    if (RegSetValueExW(hkeydriver, entry, 0, REG_SZ, (BYTE*)value,
                                    (lstrlenW(value)+1)*sizeof(WCHAR)) != ERROR_SUCCESS)
                        ERR("Failed to write registry data %s %s\n", debugstr_w(entry), debugstr_w(value));
                    free(value);
                }
                else
                {
                    ERR("No pair found. %s\n", debugstr_w(p));
                    break;
                }
            }

            /* Set Usage Count */
            usagecount++;
            if (RegSetValueExA(hkeydriver, "UsageCount", 0, REG_DWORD, (BYTE*)&usagecount, sizeof(usagecount)) != ERROR_SUCCESS)
                ERR("Failed to write registry UsageCount key\n");

            if (usage_count)
                *usage_count = usagecount;

            RegCloseKey(hkeydriver);
        }

        RegCloseKey(hkey);
    }
}

BOOL WINAPI SQLInstallDriverExW(LPCWSTR lpszDriver, LPCWSTR lpszPathIn,
               LPWSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    UINT len;
    WCHAR path[MAX_PATH];

    clear_errors();
    TRACE("%s %s %p %d %p %d %p\n", debugstr_w(lpszDriver),
          debugstr_w(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    write_registry_values(L"ODBC Drivers", lpszDriver, lpszPathIn, path, lpdwUsageCount);

    len = lstrlenW(path);

    if (pcbPathOut)
        *pcbPathOut = len;

    if (lpszPathOut && cbPathOutMax > len)
    {
        lstrcpyW(lpszPathOut, path);
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI SQLInstallDriverEx(LPCSTR lpszDriver, LPCSTR lpszPathIn,
               LPSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    LPWSTR driver, pathin;
    WCHAR pathout[MAX_PATH];
    BOOL ret;
    WORD cbOut = 0;

    clear_errors();
    TRACE("%s %s %p %d %p %d %p\n", debugstr_a(lpszDriver),
          debugstr_a(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    driver = SQLInstall_strdup_multi(lpszDriver);
    pathin = SQLInstall_strdup(lpszPathIn);

    ret = SQLInstallDriverExW(driver, pathin, pathout, MAX_PATH, &cbOut,
                              fRequest, lpdwUsageCount);
    if (ret)
    {
        int len =  WideCharToMultiByte(CP_ACP, 0, pathout, -1, lpszPathOut,
                                       0, NULL, NULL);
        if (len)
        {
            if (pcbPathOut)
                *pcbPathOut = len - 1;

            if (!lpszPathOut || cbPathOutMax < len)
            {
                ret = FALSE;
                goto out;
            }
            len =  WideCharToMultiByte(CP_ACP, 0, pathout, -1, lpszPathOut,
                                       cbPathOutMax, NULL, NULL);
        }
    }

out:
    free(driver);
    free(pathin);
    return ret;
}

BOOL WINAPI SQLInstallDriverManagerW(LPWSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut)
{
    UINT len;
    WCHAR path[MAX_PATH];

    TRACE("(%p %d %p)\n", lpszPath, cbPathMax, pcbPathOut);

    if (cbPathMax < MAX_PATH)
        return FALSE;

    clear_errors();

    len = GetSystemDirectoryW(path, MAX_PATH);

    if (pcbPathOut)
        *pcbPathOut = len;

    if (lpszPath && cbPathMax > len)
    {
    	lstrcpyW(lpszPath, path);
    	return TRUE;
    }
    return FALSE;
}

BOOL WINAPI SQLInstallDriverManager(LPSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut)
{
    BOOL ret;
    WORD len, cbOut = 0;
    WCHAR path[MAX_PATH];

    TRACE("(%p %d %p)\n", lpszPath, cbPathMax, pcbPathOut);

    if (cbPathMax < MAX_PATH)
        return FALSE;

    clear_errors();

    ret = SQLInstallDriverManagerW(path, MAX_PATH, &cbOut);
    if (ret)
    {
        len =  WideCharToMultiByte(CP_ACP, 0, path, -1, lpszPath, 0,
                                   NULL, NULL);
        if (len)
        {
            if (pcbPathOut)
                *pcbPathOut = len - 1;

            if (!lpszPath || cbPathMax < len)
                return FALSE;

            len =  WideCharToMultiByte(CP_ACP, 0, path, -1, lpszPath,
                                       cbPathMax, NULL, NULL);
        }
    }
    return ret;
}

BOOL WINAPI SQLInstallODBCW(HWND hwndParent, LPCWSTR lpszInfFile,
               LPCWSTR lpszSrcPath, LPCWSTR lpszDrivers)
{
    clear_errors();
    FIXME("%p %s %s %s\n", hwndParent, debugstr_w(lpszInfFile),
               debugstr_w(lpszSrcPath), debugstr_w(lpszDrivers));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLInstallODBC(HWND hwndParent, LPCSTR lpszInfFile,
               LPCSTR lpszSrcPath, LPCSTR lpszDrivers)
{
    clear_errors();
    FIXME("%p %s %s %s\n", hwndParent, debugstr_a(lpszInfFile),
               debugstr_a(lpszSrcPath), debugstr_a(lpszDrivers));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

SQLRETURN WINAPI SQLInstallerErrorW(WORD iError, DWORD *pfErrorCode,
               LPWSTR lpszErrorMsg, WORD cbErrorMsgMax, WORD *pcbErrorMsg)
{
    TRACE("%d %p %p %d %p\n", iError, pfErrorCode, lpszErrorMsg,
          cbErrorMsgMax, pcbErrorMsg);

    if (iError == 0)
    {
        return SQL_ERROR;
    }
    else if (iError <= num_errors)
    {
        BOOL truncated = FALSE;
        WORD len;
        LPCWSTR msg;
        iError--;
        if (pfErrorCode)
            *pfErrorCode = error_code[iError];
        msg = error_msg[iError];
        len = msg ? lstrlenW(msg) : 0;
        if (pcbErrorMsg)
            *pcbErrorMsg = len;
        len++;
        if (cbErrorMsgMax < len)
        {
            len = cbErrorMsgMax;
            truncated = TRUE;
        }
        if (lpszErrorMsg && len)
        {
            if (msg)
            {
                memcpy (lpszErrorMsg, msg, len * sizeof(WCHAR));
            }
            else
            {
                assert(len==1);
                *lpszErrorMsg = 0;
            }
        }
        else
        {
            /* Yes.  If you pass a null pointer and a large length it is not an error! */
            truncated = TRUE;
        }

        return truncated ? SQL_SUCCESS_WITH_INFO : SQL_SUCCESS;
    }

    /* At least on Windows 2000 , the buffers are not altered in this case.  However that is a little too dangerous a test for just now */
    if (pcbErrorMsg)
        *pcbErrorMsg = 0;

    if (lpszErrorMsg && cbErrorMsgMax > 0)
        *lpszErrorMsg = '\0';

    return SQL_NO_DATA;
}

SQLRETURN WINAPI SQLInstallerError(WORD iError, DWORD *pfErrorCode,
               LPSTR lpszErrorMsg, WORD cbErrorMsgMax, WORD *pcbErrorMsg)
{
    SQLRETURN ret;
    LPWSTR wbuf;
    WORD cbwbuf;
    TRACE("%d %p %p %d %p\n", iError, pfErrorCode, lpszErrorMsg,
          cbErrorMsgMax, pcbErrorMsg);

    wbuf = 0;
    if (lpszErrorMsg && cbErrorMsgMax)
    {
        wbuf = malloc(cbErrorMsgMax * sizeof(WCHAR));
        if (!wbuf)
            return SQL_ERROR;
    }
    ret = SQLInstallerErrorW(iError, pfErrorCode, wbuf, cbErrorMsgMax, &cbwbuf);
    if (wbuf)
    {
        WORD cbBuf = 0;
        SQLInstall_narrow(1, lpszErrorMsg, wbuf, cbwbuf+1, cbErrorMsgMax, &cbBuf);
        free(wbuf);
        if (pcbErrorMsg)
            *pcbErrorMsg = cbBuf-1;
    }
    return ret;
}

BOOL WINAPI SQLInstallTranslatorExW(LPCWSTR lpszTranslator, LPCWSTR lpszPathIn,
               LPWSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    UINT len;
    WCHAR path[MAX_PATH];

    clear_errors();
    TRACE("%s %s %p %d %p %d %p\n", debugstr_w(lpszTranslator),
          debugstr_w(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    write_registry_values(L"ODBC Translators", lpszTranslator, lpszPathIn, path, lpdwUsageCount);

    len = lstrlenW(path);

    if (pcbPathOut)
        *pcbPathOut = len;

    if (lpszPathOut && cbPathOutMax > len)
    {
        lstrcpyW(lpszPathOut, path);
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI SQLInstallTranslatorEx(LPCSTR lpszTranslator, LPCSTR lpszPathIn,
               LPSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    LPCSTR p;
    LPWSTR translator, pathin;
    WCHAR pathout[MAX_PATH];
    BOOL ret;
    WORD cbOut = 0;

    clear_errors();
    TRACE("%s %s %p %d %p %d %p\n", debugstr_a(lpszTranslator),
          debugstr_a(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    for (p = lpszTranslator; *p; p += lstrlenA(p) + 1)
        TRACE("%s\n", debugstr_a(p));

    translator = SQLInstall_strdup_multi(lpszTranslator);
    pathin = SQLInstall_strdup(lpszPathIn);

    ret = SQLInstallTranslatorExW(translator, pathin, pathout, MAX_PATH,
                                  &cbOut, fRequest, lpdwUsageCount);
    if (ret)
    {
        int len =  WideCharToMultiByte(CP_ACP, 0, pathout, -1, lpszPathOut,
                                       0, NULL, NULL);
        if (len)
        {
            if (pcbPathOut)
                *pcbPathOut = len - 1;

            if (!lpszPathOut || cbPathOutMax < len)
            {
                ret = FALSE;
                goto out;
            }
            len =  WideCharToMultiByte(CP_ACP, 0, pathout, -1, lpszPathOut,
                                       cbPathOutMax, NULL, NULL);
        }
    }

out:
    free(translator);
    free(pathin);
    return ret;
}

BOOL WINAPI SQLInstallTranslator(LPCSTR lpszInfFile, LPCSTR lpszTranslator,
               LPCSTR lpszPathIn, LPSTR lpszPathOut, WORD cbPathOutMax,
               WORD *pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
    clear_errors();
    TRACE("%s %s %s %p %d %p %d %p\n", debugstr_a(lpszInfFile),
          debugstr_a(lpszTranslator), debugstr_a(lpszPathIn), lpszPathOut,
          cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);

    if (lpszInfFile)
        return FALSE;

    return SQLInstallTranslatorEx(lpszTranslator, lpszPathIn, lpszPathOut,
                       cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
}

BOOL WINAPI SQLInstallTranslatorW(LPCWSTR lpszInfFile, LPCWSTR lpszTranslator,
              LPCWSTR lpszPathIn, LPWSTR lpszPathOut, WORD cbPathOutMax,
              WORD *pcbPathOut, WORD fRequest, LPDWORD lpdwUsageCount)
{
    clear_errors();
    TRACE("%s %s %s %p %d %p %d %p\n", debugstr_w(lpszInfFile),
          debugstr_w(lpszTranslator), debugstr_w(lpszPathIn), lpszPathOut,
          cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);

    if (lpszInfFile)
        return FALSE;

    return SQLInstallTranslatorExW(lpszTranslator, lpszPathIn, lpszPathOut,
                        cbPathOutMax, pcbPathOut, fRequest, lpdwUsageCount);
}

BOOL WINAPI SQLManageDataSources(HWND hwnd)
{
    clear_errors();
    FIXME("%p\n", hwnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

SQLRETURN WINAPI SQLPostInstallerErrorW(DWORD fErrorCode, LPCWSTR szErrorMsg)
{
    FIXME("%lu %s\n", fErrorCode, debugstr_w(szErrorMsg));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

SQLRETURN WINAPI SQLPostInstallerError(DWORD fErrorCode, LPCSTR szErrorMsg)
{
    FIXME("%lu %s\n", fErrorCode, debugstr_a(szErrorMsg));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLReadFileDSNW(LPCWSTR lpszFileName, LPCWSTR lpszAppName,
               LPCWSTR lpszKeyName, LPWSTR lpszString, WORD cbString,
               WORD *pcbString)
{
    clear_errors();
    FIXME("%s %s %s %s %d %p\n", debugstr_w(lpszFileName), debugstr_w(lpszAppName),
               debugstr_w(lpszKeyName), debugstr_w(lpszString), cbString, pcbString);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLReadFileDSN(LPCSTR lpszFileName, LPCSTR lpszAppName,
               LPCSTR lpszKeyName, LPSTR lpszString, WORD cbString,
               WORD *pcbString)
{
    clear_errors();
    FIXME("%s %s %s %s %d %p\n", debugstr_a(lpszFileName), debugstr_a(lpszAppName),
               debugstr_a(lpszKeyName), debugstr_a(lpszString), cbString, pcbString);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDefaultDataSource(void)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDriverW(LPCWSTR drivername, BOOL remove_dsn, LPDWORD usage_count)
{
    HKEY hkey;
    DWORD usagecount = 1;

    clear_errors();
    TRACE("%s %d %p\n", debugstr_w(drivername), remove_dsn, usage_count);

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBCINST.INI\\", &hkey) == ERROR_SUCCESS)
    {
        HKEY hkeydriver;

        if (RegOpenKeyW(hkey, drivername, &hkeydriver) == ERROR_SUCCESS)
        {
            DWORD size, type;
            DWORD count;

            size = sizeof(usagecount);
            RegGetValueA(hkeydriver, NULL, "UsageCount", RRF_RT_DWORD, &type, &usagecount, &size);
            TRACE("Usage count %ld\n", usagecount);
            count = usagecount - 1;
            if (count)
            {
                 if (RegSetValueExA(hkeydriver, "UsageCount", 0, REG_DWORD, (BYTE*)&count, sizeof(count)) != ERROR_SUCCESS)
                    ERR("Failed to write registry UsageCount key\n");
            }

            RegCloseKey(hkeydriver);
        }

        if (usagecount)
            usagecount--;

        if (!usagecount)
        {
            if (RegDeleteKeyW(hkey, drivername) != ERROR_SUCCESS)
                ERR("Failed to delete registry key: %s\n", debugstr_w(drivername));

            if (RegOpenKeyW(hkey, L"ODBC Drivers", &hkeydriver) == ERROR_SUCCESS)
            {
                if(RegDeleteValueW(hkeydriver, drivername) != ERROR_SUCCESS)
                    ERR("Failed to delete registry value: %s\n", debugstr_w(drivername));
                RegCloseKey(hkeydriver);
            }
        }

        RegCloseKey(hkey);
    }

    if (usage_count)
        *usage_count = usagecount;

    return TRUE;
}

BOOL WINAPI SQLRemoveDriver(LPCSTR lpszDriver, BOOL fRemoveDSN,
               LPDWORD lpdwUsageCount)
{
    WCHAR *driver;
    BOOL ret;

    clear_errors();
    TRACE("%s %d %p\n", debugstr_a(lpszDriver), fRemoveDSN, lpdwUsageCount);

    driver = SQLInstall_strdup(lpszDriver);

    ret =  SQLRemoveDriverW(driver, fRemoveDSN, lpdwUsageCount);

    free(driver);
    return ret;
}

BOOL WINAPI SQLRemoveDriverManager(LPDWORD pdwUsageCount)
{
    clear_errors();
    FIXME("%p\n", pdwUsageCount);
    if (pdwUsageCount) *pdwUsageCount = 1;
    return TRUE;
}

BOOL WINAPI SQLRemoveDSNFromIniW(LPCWSTR lpszDSN)
{
    HKEY hkey, hkeyroot = HKEY_CURRENT_USER;

    TRACE("%s\n", debugstr_w(lpszDSN));

    if (!SQLValidDSNW(lpszDSN))
    {
        push_error(ODBC_ERROR_INVALID_DSN, L"Invalid DSN");
        return FALSE;
    }

    clear_errors();

    if (config_mode == ODBC_SYSTEM_DSN)
        hkeyroot = HKEY_LOCAL_MACHINE;
    else if (config_mode == ODBC_BOTH_DSN)
    {
        WCHAR *regpath = malloc( (wcslen(L"Software\\ODBC\\ODBC.INI\\") + wcslen(lpszDSN) + 1) * sizeof(WCHAR) );
        if (!regpath)
        {
            push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");
            return FALSE;
        }
        wcscpy(regpath, L"Software\\ODBC\\ODBC.INI\\");
        wcscat(regpath, lpszDSN);

        /* ONLY removes one DSN, USER or SYSTEM */
        if (RegOpenKeyW(HKEY_CURRENT_USER, regpath, &hkey) == ERROR_SUCCESS)
            hkeyroot = HKEY_CURRENT_USER;
        else
            hkeyroot = HKEY_LOCAL_MACHINE;

        RegCloseKey(hkey);
        free(regpath);
    }

    if (RegOpenKeyW(hkeyroot, L"Software\\ODBC\\ODBC.INI\\ODBC Data Sources", &hkey) == ERROR_SUCCESS)
    {
        RegDeleteValueW(hkey, lpszDSN);
        RegCloseKey(hkey);
    }

    if (RegOpenKeyW(hkeyroot, L"Software\\ODBC\\ODBC.INI", &hkey) == ERROR_SUCCESS)
    {
        RegDeleteTreeW(hkey, lpszDSN);
        RegCloseKey(hkey);
    }

    return TRUE;
}

BOOL WINAPI SQLRemoveDSNFromIni(LPCSTR lpszDSN)
{
    BOOL ret = FALSE;
    WCHAR *dsn;

    TRACE("%s\n", debugstr_a(lpszDSN));

    clear_errors();

    dsn = SQLInstall_strdup(lpszDSN);
    if (dsn)
        ret = SQLRemoveDSNFromIniW(dsn);
    else
        push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");

    free(dsn);

    return ret;
}

BOOL WINAPI SQLRemoveTranslatorW(const WCHAR *translator, DWORD *usage_count)
{
    HKEY hkey;
    DWORD usagecount = 1;
    BOOL ret = TRUE;

    clear_errors();
    TRACE("%s %p\n", debugstr_w(translator), usage_count);

    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBCINST.INI\\", &hkey) == ERROR_SUCCESS)
    {
        HKEY hkeydriver;

        if (RegOpenKeyW(hkey, translator, &hkeydriver) == ERROR_SUCCESS)
        {
            DWORD size, type;
            DWORD count;

            size = sizeof(usagecount);
            RegGetValueA(hkeydriver, NULL, "UsageCount", RRF_RT_DWORD, &type, &usagecount, &size);
            TRACE("Usage count %ld\n", usagecount);
            count = usagecount - 1;
            if (count)
            {
                 if (RegSetValueExA(hkeydriver, "UsageCount", 0, REG_DWORD, (BYTE*)&count, sizeof(count)) != ERROR_SUCCESS)
                    ERR("Failed to write registry UsageCount key\n");
            }

            RegCloseKey(hkeydriver);
        }

        if (usagecount)
            usagecount--;

        if (!usagecount)
        {
            if(RegDeleteKeyW(hkey, translator) != ERROR_SUCCESS)
            {
                push_error(ODBC_ERROR_COMPONENT_NOT_FOUND, L"Component not found");
                WARN("Failed to delete registry key: %s\n", debugstr_w(translator));
                ret = FALSE;
            }

            if (ret && RegOpenKeyW(hkey, L"ODBC Translators", &hkeydriver) == ERROR_SUCCESS)
            {
                if(RegDeleteValueW(hkeydriver, translator) != ERROR_SUCCESS)
                {
                    push_error(ODBC_ERROR_COMPONENT_NOT_FOUND, L"Component not found");
                    WARN("Failed to delete registry key: %s\n", debugstr_w(translator));
                    ret = FALSE;
                }

                RegCloseKey(hkeydriver);
            }
        }

        RegCloseKey(hkey);
    }

    if (ret && usage_count)
        *usage_count = usagecount;

    return ret;
}

BOOL WINAPI SQLRemoveTranslator(LPCSTR lpszTranslator, LPDWORD lpdwUsageCount)
{
    WCHAR *translator;
    BOOL ret;

    clear_errors();
    TRACE("%s %p\n", debugstr_a(lpszTranslator), lpdwUsageCount);

    translator = SQLInstall_strdup(lpszTranslator);
    ret =  SQLRemoveTranslatorW(translator, lpdwUsageCount);

    free(translator);
    return ret;
}

BOOL WINAPI SQLSetConfigMode(UWORD wConfigMode)
{
    clear_errors();
    TRACE("%u\n", wConfigMode);

    if (wConfigMode > ODBC_SYSTEM_DSN)
    {
        push_error(ODBC_ERROR_INVALID_PARAM_SEQUENCE, L"Invalid parameter sequence");
        return FALSE;
    }
    else
    {
        config_mode = wConfigMode;
        return TRUE;
    }
}

BOOL WINAPI SQLValidDSNW(LPCWSTR lpszDSN)
{
    clear_errors();
    TRACE("%s\n", debugstr_w(lpszDSN));

    if (!lpszDSN || !*lpszDSN || lstrlenW(lpszDSN) > SQL_MAX_DSN_LENGTH || wcspbrk(lpszDSN, L"[]{}(),;?*=!@\\"))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI SQLValidDSN(LPCSTR lpszDSN)
{
    static const char *invalid = "[]{}(),;?*=!@\\";
    clear_errors();
    TRACE("%s\n", debugstr_a(lpszDSN));

    if (!lpszDSN || !*lpszDSN || strlen(lpszDSN) > SQL_MAX_DSN_LENGTH || strpbrk(lpszDSN, invalid))
    {
        return FALSE;
    }

    return TRUE;
}

BOOL WINAPI SQLWriteDSNToIniW(LPCWSTR lpszDSN, LPCWSTR lpszDriver)
{
    DWORD ret;
    HKEY hkey, hkeydriver, hkeyroot = HKEY_CURRENT_USER;
    WCHAR filename[MAX_PATH];

    TRACE("%s %s\n", debugstr_w(lpszDSN), debugstr_w(lpszDriver));

    clear_errors();

    if (!SQLValidDSNW(lpszDSN))
    {
        push_error(ODBC_ERROR_INVALID_DSN, L"Invalid DSN");
        return FALSE;
    }

    /* It doesn't matter if we cannot find the driver, windows just writes a blank value. */
    filename[0] = 0;
    if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBCINST.INI\\", &hkey) == ERROR_SUCCESS)
    {
        HKEY hkeydriver;

        if (RegOpenKeyW(hkey, lpszDriver, &hkeydriver) == ERROR_SUCCESS)
        {
            DWORD size = MAX_PATH * sizeof(WCHAR);
            RegGetValueW(hkeydriver, NULL, L"driver", RRF_RT_REG_SZ, NULL, filename, &size);
            RegCloseKey(hkeydriver);
        }
        RegCloseKey(hkey);
    }

    if (config_mode == ODBC_SYSTEM_DSN)
        hkeyroot = HKEY_LOCAL_MACHINE;
    else if (config_mode == ODBC_BOTH_DSN)
    {
        WCHAR *regpath = malloc( (wcslen(L"Software\\ODBC\\ODBC.INI\\") + wcslen(lpszDSN) + 1) * sizeof(WCHAR) );
        if (!regpath)
        {
            push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");
            return FALSE;
        }
        wcscpy(regpath, L"Software\\ODBC\\ODBC.INI\\");
        wcscat(regpath, lpszDSN);

        /* Check for existing entry */
        if (RegOpenKeyW(HKEY_LOCAL_MACHINE, regpath, &hkey) == ERROR_SUCCESS)
            hkeyroot = HKEY_LOCAL_MACHINE;
        else
            hkeyroot = HKEY_CURRENT_USER;

        RegCloseKey(hkey);
        free(regpath);
    }

    if ((ret = RegCreateKeyW(hkeyroot, L"SOFTWARE\\ODBC\\ODBC.INI", &hkey)) == ERROR_SUCCESS)
    {
        HKEY sources;

        if ((ret = RegCreateKeyW(hkey, L"ODBC Data Sources", &sources)) == ERROR_SUCCESS)
        {
            RegSetValueExW(sources, lpszDSN, 0, REG_SZ, (BYTE*)lpszDriver, (lstrlenW(lpszDriver)+1)*sizeof(WCHAR));
            RegCloseKey(sources);

            RegDeleteTreeW(hkey, lpszDSN);
            if ((ret = RegCreateKeyW(hkey, lpszDSN, &hkeydriver)) == ERROR_SUCCESS)
            {
                RegSetValueExW(sources, L"driver", 0, REG_SZ, (BYTE*)filename, (lstrlenW(filename)+1)*sizeof(WCHAR));
                RegCloseKey(hkeydriver);
            }
        }
        RegCloseKey(hkey);
    }

    if (ret != ERROR_SUCCESS)
        push_error(ODBC_ERROR_REQUEST_FAILED, L"Request Failed");

    return ret == ERROR_SUCCESS;
}

BOOL WINAPI SQLWriteDSNToIni(LPCSTR lpszDSN, LPCSTR lpszDriver)
{
    BOOL ret = FALSE;
    WCHAR *dsn, *driver;

    TRACE("%s %s\n", debugstr_a(lpszDSN), debugstr_a(lpszDriver));

    dsn = SQLInstall_strdup(lpszDSN);
    driver = SQLInstall_strdup(lpszDriver);
    if (dsn && driver)
        ret = SQLWriteDSNToIniW(dsn, driver);
    else
        push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");

    free(dsn);
    free(driver);

    return ret;
}

BOOL WINAPI SQLWriteFileDSNW(LPCWSTR lpszFileName, LPCWSTR lpszAppName,
               LPCWSTR lpszKeyName, LPCWSTR lpszString)
{
    clear_errors();
    FIXME("%s %s %s %s\n", debugstr_w(lpszFileName), debugstr_w(lpszAppName),
                 debugstr_w(lpszKeyName), debugstr_w(lpszString));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteFileDSN(LPCSTR lpszFileName, LPCSTR lpszAppName,
               LPCSTR lpszKeyName, LPCSTR lpszString)
{
    clear_errors();
    FIXME("%s %s %s %s\n", debugstr_a(lpszFileName), debugstr_a(lpszAppName),
                 debugstr_a(lpszKeyName), debugstr_a(lpszString));
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWritePrivateProfileStringW(LPCWSTR lpszSection, LPCWSTR lpszEntry,
               LPCWSTR lpszString, LPCWSTR lpszFilename)
{
    LONG ret;
    HKEY hkey;
    WCHAR *regpath;

    clear_errors();
    TRACE("%s %s %s %s\n", debugstr_w(lpszSection), debugstr_w(lpszEntry),
                debugstr_w(lpszString), debugstr_w(lpszFilename));

    if(!lpszFilename || !*lpszFilename)
    {
        push_error(ODBC_ERROR_INVALID_STR, L"Invalid parameter string");
        return FALSE;
    }

    regpath = malloc ( (wcslen(L"Software\\ODBC\\") + wcslen(lpszFilename) + wcslen(L"\\")
                            + wcslen(lpszSection) + 1) * sizeof(WCHAR));
    if (!regpath)
    {
        push_error(ODBC_ERROR_OUT_OF_MEM, L"Out of memory");
        return FALSE;
    }
    wcscpy(regpath, L"Software\\ODBC\\");
    wcscat(regpath, lpszFilename);
    wcscat(regpath, L"\\");
    wcscat(regpath, lpszSection);

    /* odbcinit.ini is only for drivers, so default to local Machine */
    if (!wcsicmp(lpszFilename, L"ODBCINST.INI") || config_mode == ODBC_SYSTEM_DSN)
        ret = RegCreateKeyW(HKEY_LOCAL_MACHINE, regpath, &hkey);
    else if (config_mode == ODBC_USER_DSN)
        ret = RegCreateKeyW(HKEY_CURRENT_USER, regpath, &hkey);
    else
    {
        /* Check existing keys first */
        if ((ret = RegOpenKeyW(HKEY_CURRENT_USER, regpath, &hkey)) != ERROR_SUCCESS)
            ret = RegOpenKeyW(HKEY_LOCAL_MACHINE, regpath, &hkey);

        if (ret != ERROR_SUCCESS)
            ret = RegCreateKeyW(HKEY_CURRENT_USER, regpath, &hkey);
    }

    free(regpath);

    if (ret == ERROR_SUCCESS)
    {
        if(lpszString)
            ret = RegSetValueExW(hkey, lpszEntry, 0, REG_SZ, (BYTE*)lpszString, (lstrlenW(lpszString)+1)*sizeof(WCHAR));
        else
            ret = RegSetValueExW(hkey, lpszEntry, 0, REG_SZ, (BYTE*)L"", sizeof(L""));
         RegCloseKey(hkey);
    }

    return ret == ERROR_SUCCESS;
}

BOOL WINAPI SQLWritePrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
               LPCSTR lpszString, LPCSTR lpszFilename)
{
    BOOL ret;
    WCHAR *sect, *entry, *string, *file;
    clear_errors();
    TRACE("%s %s %s %s\n", lpszSection, lpszEntry, lpszString, lpszFilename);

    sect = strdupAtoW(lpszSection);
    entry = strdupAtoW(lpszEntry);
    string = strdupAtoW(lpszString);
    file = strdupAtoW(lpszFilename);

    ret = SQLWritePrivateProfileStringW(sect, entry, string, file);

    free(sect);
    free(entry);
    free(string);
    free(file);

    return ret;
}
