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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/debug.h"

#include "odbcinst.h"

WINE_DEFAULT_DEBUG_CHANNEL(odbc);

/* Registry key names */
static const WCHAR drivers_key[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C','\\','O','D','B','C','I','N','S','T','.','I','N','I','\\','O','D','B','C',' ','D','r','i','v','e','r','s',0};

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
static const WCHAR odbc_error_general_err[] = {'G','e','n','e','r','a','l',' ','e','r','r','o','r',0};
static const WCHAR odbc_error_invalid_buff_len[] = {'I','n','v','a','l','i','d',' ','b','u','f','f','e','r',' ','l','e','n','g','t','h',0};
static const WCHAR odbc_error_component_not_found[] = {'C','o','m','p','o','n','e','n','t',' ','n','o','t',' ','f','o','u','n','d',0};
static const WCHAR odbc_error_out_of_mem[] = {'O','u','t',' ','o','f',' ','m','e','m','o','r','y',0};
static const WCHAR odbc_error_invalid_param_sequence[] = {'I','n','v','a','l','i','d',' ','p','a','r','a','m','e','t','e','r',' ','s','e','q','u','e','n','c','e',0};

/* Push an error onto the error stack, taking care of ranges etc. */
static void push_error(int code, LPCWSTR msg)
{
    if (num_errors < sizeof error_code/sizeof error_code[0])
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

BOOL WINAPI ODBCCPlApplet( LONG i, LONG j, LONG * p1, LONG * p2)
{
    clear_errors();
    FIXME( "( %d %d %p %p) : stub!\n", i, j, p1, p2);
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
    ret = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, str, p - str, ret, len );
    ret[len] = 0;

    return ret;
}

static LPWSTR SQLInstall_strdup(LPCSTR str)
{
    DWORD len;
    LPWSTR ret = NULL;

    if (!str)
        return ret;

    len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0 );
    ret = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
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
            pbuf = HeapAlloc(GetProcessHeap(), 0, len);
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
            HeapFree(GetProcessHeap(), 0, pbuf);
        }
    }
    else
    {
        ERR("measuring wide to narrow\n");
    }
    return success;
}

BOOL WINAPI SQLConfigDataSourceW(HWND hwndParent, WORD fRequest,
               LPCWSTR lpszDriver, LPCWSTR lpszAttributes)
{
    LPCWSTR p;

    clear_errors();
    FIXME("%p %d %s %s\n", hwndParent, fRequest, debugstr_w(lpszDriver),
          debugstr_w(lpszAttributes));

    for (p = lpszAttributes; *p; p += lstrlenW(p) + 1)
        FIXME("%s\n", debugstr_w(p));

    return TRUE;
}

BOOL WINAPI SQLConfigDataSource(HWND hwndParent, WORD fRequest,
               LPCSTR lpszDriver, LPCSTR lpszAttributes)
{
    FIXME("%p %d %s %s\n", hwndParent, fRequest, debugstr_a(lpszDriver),
          debugstr_a(lpszAttributes));
    clear_errors();
    return TRUE;
}

BOOL WINAPI SQLConfigDriverW(HWND hwndParent, WORD fRequest, LPCWSTR lpszDriver,
               LPCWSTR lpszArgs, LPWSTR lpszMsg, WORD cbMsgMax, WORD *pcbMsgOut)
{
    clear_errors();
    FIXME("(%p %d %s %s %p %d %p)\n", hwndParent, fRequest, debugstr_w(lpszDriver),
          debugstr_w(lpszArgs), lpszMsg, cbMsgMax, pcbMsgOut);
    return TRUE;
}

BOOL WINAPI SQLConfigDriver(HWND hwndParent, WORD fRequest, LPCSTR lpszDriver,
               LPCSTR lpszArgs, LPSTR lpszMsg, WORD cbMsgMax, WORD *pcbMsgOut)
{
    clear_errors();
    FIXME("(%p %d %s %s %p %d %p)\n", hwndParent, fRequest, debugstr_a(lpszDriver),
          debugstr_a(lpszArgs), lpszMsg, cbMsgMax, pcbMsgOut);
    return TRUE;
}

BOOL WINAPI SQLCreateDataSourceW(HWND hwnd, LPCWSTR lpszDS)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLCreateDataSource(HWND hwnd, LPCSTR lpszDS)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetAvailableDriversW(LPCWSTR lpszInfFile, LPWSTR lpszBuf,
               WORD cbBufMax, WORD *pcbBufOut)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetAvailableDrivers(LPCSTR lpszInfFile, LPSTR lpszBuf,
               WORD cbBufMax, WORD *pcbBufOut)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetConfigMode(UWORD *pwConfigMode)
{
    clear_errors();
    if (pwConfigMode)
        *pwConfigMode = config_mode;
    return TRUE;
}

/* This is implemented sensibly rather than according to exact conformance to Microsoft's buggy implementations
 * e.g. The Microsoft one occasionally actually adds a third nul character (possibly beyond the buffer).
 * e.g. If the key has no drivers then version 3.525.1117.0 does not modify the buffer at all, not even a nul character.
 */
BOOL WINAPI SQLGetInstalledDriversW(LPWSTR lpszBuf, WORD cbBufMax,
               WORD *pcbBufOut)
{
    HKEY hDrivers; /* Registry handle to the Drivers key */
    LONG reg_ret; /* Return code from registry functions */
    BOOL success = FALSE; /* The value we will return */

    clear_errors();
    if (!lpszBuf || cbBufMax == 0)
    {
        push_error(ODBC_ERROR_INVALID_BUFF_LEN, odbc_error_invalid_buff_len);
    }
    else if ((reg_ret = RegOpenKeyExW (HKEY_LOCAL_MACHINE /* The drivers does not depend on the config mode */,
            drivers_key, 0, KEY_READ /* Maybe overkill */,
            &hDrivers)) == ERROR_SUCCESS)
    {
        DWORD index = 0;
        cbBufMax--;
        success = TRUE;
        while (cbBufMax > 0)
        {
            DWORD size_name;
            size_name = cbBufMax;
            if ((reg_ret = RegEnumValueW(hDrivers, index, lpszBuf, &size_name, NULL, NULL, NULL, NULL)) == ERROR_SUCCESS)
            {
                index++;
                assert (size_name < cbBufMax && *(lpszBuf + size_name) == 0);
                size_name++;
                cbBufMax-= size_name;
                lpszBuf+=size_name;
            }
            else
            {
                if (reg_ret != ERROR_NO_MORE_ITEMS)
                {
                    success = FALSE;
                    push_error(ODBC_ERROR_GENERAL_ERR, odbc_error_general_err);
                }
                break;
            }
        }
        *lpszBuf = 0;
        if ((reg_ret = RegCloseKey (hDrivers)) != ERROR_SUCCESS)
            TRACE ("Error %d closing ODBC Drivers key\n", reg_ret);
    }
    else
    {
        /* MSDN states that it returns failure with COMPONENT_NOT_FOUND in this case.
         * Version 3.525.1117.0 (Windows 2000) does not; it actually returns success.
         * I doubt if it will actually be an issue.
         */
        push_error(ODBC_ERROR_COMPONENT_NOT_FOUND, odbc_error_component_not_found);
    }
    return success;
}

BOOL WINAPI SQLGetInstalledDrivers(LPSTR lpszBuf, WORD cbBufMax,
               WORD *pcbBufOut)
{
    BOOL ret;
    int size_wbuf = cbBufMax;
    LPWSTR wbuf;
    WORD size_used;
    wbuf = HeapAlloc(GetProcessHeap(), 0, size_wbuf*sizeof(WCHAR));
    if (wbuf)
    {
        ret = SQLGetInstalledDriversW(wbuf, size_wbuf, &size_used);
        if (ret)
        {
            if (!(ret = SQLInstall_narrow(2, lpszBuf, wbuf, size_used, cbBufMax, pcbBufOut)))
            {
                push_error(ODBC_ERROR_GENERAL_ERR, odbc_error_general_err);
            }
        }
        HeapFree(GetProcessHeap(), 0, wbuf);
        /* ignore failure; we have achieved the aim */
    }
    else
    {
        push_error(ODBC_ERROR_OUT_OF_MEM, odbc_error_out_of_mem);
        ret = FALSE;
    }
    return ret;
}

int WINAPI SQLGetPrivateProfileStringW(LPCWSTR lpszSection, LPCWSTR lpszEntry,
               LPCWSTR lpszDefault, LPCWSTR RetBuffer, int cbRetBuffer,
               LPCWSTR lpszFilename)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

int WINAPI SQLGetPrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
               LPCSTR lpszDefault, LPCSTR RetBuffer, int cbRetBuffer,
               LPCSTR lpszFilename)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetTranslatorW(HWND hwndParent, LPWSTR lpszName, WORD cbNameMax,
               WORD *pcbNameOut, LPWSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut, DWORD *pvOption)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLGetTranslator(HWND hwndParent, LPSTR lpszName, WORD cbNameMax,
               WORD *pcbNameOut, LPSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut, DWORD *pvOption)
{
    clear_errors();
    FIXME("\n");
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

BOOL WINAPI SQLInstallDriverExW(LPCWSTR lpszDriver, LPCWSTR lpszPathIn,
               LPWSTR lpszPathOut, WORD cbPathOutMax, WORD *pcbPathOut,
               WORD fRequest, LPDWORD lpdwUsageCount)
{
    UINT len;
    LPCWSTR p;
    WCHAR path[MAX_PATH];

    clear_errors();
    TRACE("%s %s %p %d %p %d %p\n", debugstr_w(lpszDriver),
          debugstr_w(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    for (p = lpszDriver; *p; p += lstrlenW(p) + 1)
        TRACE("%s\n", debugstr_w(p));

    len = GetSystemDirectoryW(path, MAX_PATH);

    if (pcbPathOut)
        *pcbPathOut = len;

    len = GetSystemDirectoryW(path, MAX_PATH);

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
    LPCSTR p;
    LPWSTR driver, pathin;
    WCHAR pathout[MAX_PATH];
    BOOL ret;
    WORD cbOut = 0;

    clear_errors();
    TRACE("%s %s %p %d %p %d %p\n", debugstr_a(lpszDriver),
          debugstr_a(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    for (p = lpszDriver; *p; p += lstrlenA(p) + 1)
        TRACE("%s\n", debugstr_a(p));

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
    HeapFree(GetProcessHeap(), 0, driver);
    HeapFree(GetProcessHeap(), 0, pathin);
    return ret;
}

BOOL WINAPI SQLInstallDriverManagerW(LPWSTR lpszPath, WORD cbPathMax,
               WORD *pcbPathOut)
{
    UINT len;
    WCHAR path[MAX_PATH];

    clear_errors();
    TRACE("(%p %d %p)\n", lpszPath, cbPathMax, pcbPathOut);

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

    clear_errors();
    TRACE("(%p %d %p)\n", lpszPath, cbPathMax, pcbPathOut);

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
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLInstallODBC(HWND hwndParent, LPCSTR lpszInfFile,
               LPCSTR lpszSrcPath, LPCSTR lpszDrivers)
{
    clear_errors();
    FIXME("\n");
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
        wbuf = HeapAlloc(GetProcessHeap(), 0, cbErrorMsgMax*sizeof(WCHAR));
        if (!wbuf)
            return SQL_ERROR;
    }
    ret = SQLInstallerErrorW(iError, pfErrorCode, wbuf, cbErrorMsgMax, &cbwbuf);
    if (wbuf)
    {
        WORD cbBuf = 0;
        SQLInstall_narrow(1, lpszErrorMsg, wbuf, cbwbuf+1, cbErrorMsgMax, &cbBuf);
        HeapFree(GetProcessHeap(), 0, wbuf);
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
    LPCWSTR p;
    WCHAR path[MAX_PATH];

    clear_errors();
    TRACE("%s %s %p %d %p %d %p\n", debugstr_w(lpszTranslator),
          debugstr_w(lpszPathIn), lpszPathOut, cbPathOutMax, pcbPathOut,
          fRequest, lpdwUsageCount);

    for (p = lpszTranslator; *p; p += lstrlenW(p) + 1)
        TRACE("%s\n", debugstr_w(p));

    len = GetSystemDirectoryW(path, MAX_PATH);

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
    HeapFree(GetProcessHeap(), 0, translator);
    HeapFree(GetProcessHeap(), 0, pathin);
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
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

SQLRETURN WINAPI SQLPostInstallerErrorW(DWORD fErrorCode, LPCWSTR szErrorMsg)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

SQLRETURN WINAPI SQLPostInstallerError(DWORD fErrorCode, LPCSTR szErrorMsg)
{
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLReadFileDSNW(LPCWSTR lpszFileName, LPCWSTR lpszAppName,
               LPCWSTR lpszKeyName, LPWSTR lpszString, WORD cbString,
               WORD *pcbString)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLReadFileDSN(LPCSTR lpszFileName, LPCSTR lpszAppName,
               LPCSTR lpszKeyName, LPSTR lpszString, WORD cbString,
               WORD *pcbString)
{
    clear_errors();
    FIXME("\n");
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

BOOL WINAPI SQLRemoveDriverW(LPCWSTR lpszDriver, BOOL fRemoveDSN,
               LPDWORD lpdwUsageCount)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDriver(LPCSTR lpszDriver, BOOL fRemoveDSN,
               LPDWORD lpdwUsageCount)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDriverManager(LPDWORD pdwUsageCount)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDSNFromIniW(LPCWSTR lpszDSN)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveDSNFromIni(LPCSTR lpszDSN)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveTranslatorW(LPCWSTR lpszTranslator, LPDWORD lpdwUsageCount)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLRemoveTranslator(LPCSTR lpszTranslator, LPDWORD lpdwUsageCount)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLSetConfigMode(UWORD wConfigMode)
{
    clear_errors();
    if (wConfigMode > ODBC_SYSTEM_DSN)
    {
        push_error(ODBC_ERROR_INVALID_PARAM_SEQUENCE, odbc_error_invalid_param_sequence);
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
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLValidDSN(LPCSTR lpszDSN)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteDSNToIniW(LPCWSTR lpszDSN, LPCWSTR lpszDriver)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteDSNToIni(LPCSTR lpszDSN, LPCSTR lpszDriver)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteFileDSNW(LPCWSTR lpszFileName, LPCWSTR lpszAppName,
               LPCWSTR lpszKeyName, LPCWSTR lpszString)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWriteFileDSN(LPCSTR lpszFileName, LPCSTR lpszAppName,
               LPCSTR lpszKeyName, LPCSTR lpszString)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWritePrivateProfileStringW(LPCWSTR lpszSection, LPCWSTR lpszEntry,
               LPCWSTR lpszString, LPCWSTR lpszFilename)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI SQLWritePrivateProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
               LPCSTR lpszString, LPCSTR lpszFilename)
{
    clear_errors();
    FIXME("\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
