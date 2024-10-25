/*
 *          Shell Library Functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 2002 Eric Pouech
 * Copyright 2018-2024 Katayama Hirofumi MZ
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

#include "precomp.h"
#include <undocshell.h>

WINE_DEFAULT_DEBUG_CHANNEL(exec);

EXTERN_C BOOL PathIsExeW(LPCWSTR lpszPath);

#define SEE_MASK_CLASSALL (SEE_MASK_CLASSNAME | SEE_MASK_CLASSKEY)

typedef UINT_PTR (*SHELL_ExecuteW32)(const WCHAR *lpCmd, WCHAR *env, BOOL shWait,
                const SHELLEXECUTEINFOW *sei, LPSHELLEXECUTEINFOW sei_out);

// Is the current process a rundll32.exe?
static BOOL SHELL_InRunDllProcess(VOID)
{
    WCHAR szModule[MAX_PATH];
    static INT s_bInDllProcess = -1;

    if (s_bInDllProcess != -1)
        return s_bInDllProcess;

    s_bInDllProcess = GetModuleFileNameW(NULL, szModule, _countof(szModule)) &&
                      (StrStrIW(PathFindFileNameW(szModule), L"rundll") != NULL);
    return s_bInDllProcess;
}

static void ParseNoTildeEffect(PWSTR &res, LPCWSTR &args, DWORD &len, DWORD &used, int argNum)
{
    bool firstCharQuote = false;
    bool quotes_opened = false;
    bool backslash_encountered = false;

    for (int curArg = 0; curArg <= argNum && *args; ++curArg)
    {
        firstCharQuote = false;
        if (*args == '"')
        {
            quotes_opened = true;
            firstCharQuote = true;
            args++;
        }

        while(*args)
        {
            if (*args == '\\')
            {
                // if we found a backslash then flip the variable
                backslash_encountered = !backslash_encountered;
            }
            else if (*args == '"')
            {
                if (quotes_opened)
                {
                    if (*(args + 1) != '"')
                    {
                        quotes_opened = false;
                        args++;
                        break;
                    }
                    else
                    {
                        args++;
                    }
                }
                else
                {
                    quotes_opened = true;
                }

                backslash_encountered = false;
            }
            else
            {
                backslash_encountered = false;
                if (*args == ' ' && !firstCharQuote)
                    break;
            }

            if (curArg == argNum)
            {
                used++;
                if (used < len)
                    *res++ = *args;
            }

            args++;
        }

        while(*args == ' ')
            ++args;
    }
}

static void ParseTildeEffect(PWSTR &res, LPCWSTR &args, DWORD &len, DWORD &used, int argNum)
{
    bool quotes_opened = false;
    bool backslash_encountered = false;

    for (int curArg = 0; curArg <= argNum && *args; ++curArg)
    {
        while(*args)
        {
            if (*args == '\\')
            {
                // if we found a backslash then flip the variable
                backslash_encountered = !backslash_encountered;
            }
            else if (*args == '"')
            {
                if (quotes_opened)
                {
                    if (*(args + 1) != '"')
                    {
                        quotes_opened = false;
                    }
                    else
                    {
                        args++;
                    }
                }
                else
                {
                    quotes_opened = true;
                }

                backslash_encountered = false;
            }
            else
            {
                backslash_encountered = false;
                if (*args == ' ' && !quotes_opened && curArg != argNum)
                    break;
            }

            if (curArg == argNum)
            {
                used++;
                if (used < len)
                    *res++ = *args;
            }

            args++;
        }
    }
}

/***********************************************************************
 * SHELL_ArgifyW [Internal]
 *
 * this function is supposed to expand the escape sequences found in the registry
 * some diving reported that the following were used:
 * + %1, %2...  seem to report to parameter of index N in ShellExecute pmts
 * %1 file
 * %2 printer
 * %3 driver
 * %4 port
 * %I address of a global item ID (explorer switch /idlist)
 * %L seems to be %1 as long filename followed by the 8+3 variation
 * %S ???
 * %W Working directory
 * %V Use either %L or %W
 * %* all following parameters (see batfile)
 *
 * The way we parse the command line arguments was determined through extensive
 * testing and can be summed up by the following rules"
 *
 * - %2
 *     - if first letter is " break on first non literal " and include any white spaces
 *     - if first letter is NOT " break on first " or white space
 *     - if " is opened any pair of consecutive " results in ONE literal "
 *
 * - %~2
 *     - use rules from here http://www.autohotkey.net/~deleyd/parameters/parameters.htm
 */

static BOOL SHELL_ArgifyW(WCHAR* out, DWORD len, const WCHAR* fmt, const WCHAR* lpFile, LPITEMIDLIST pidl, LPCWSTR args, DWORD* out_len, const WCHAR* lpDir)
{
    WCHAR   xlpFile[1024];
    BOOL    done = FALSE;
    BOOL    found_p1 = FALSE;
    PWSTR   res = out;
    PCWSTR  cmd;
    DWORD   used = 0;
    bool    tildeEffect = false;

    TRACE("Before parsing: %p, %d, %s, %s, %p, %p\n", out, len, debugstr_w(fmt),
          debugstr_w(lpFile), pidl, args);

    while (*fmt)
    {
        if (*fmt == '%')
        {
            switch (*++fmt)
            {
                case '\0':
                case '%':
                {
                    used++;
                    if (used < len)
                        *res++ = '%';
                };
                break;

                case '*':
                {
                    if (args)
                    {
                        if (*fmt == '*')
                        {
                            used++;
                            while(*args)
                            {
                                used++;
                                if (used < len)
                                    *res++ = *args++;
                                else
                                    args++;
                            }
                            used++;
                            break;
                        }
                    }
                };
                break;

                case '~':

                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    //case '0':
                {
                    if (*fmt == '~')
                    {
                        fmt++;
                        tildeEffect = true;
                    }

                    if (args)
                    {
                        if (tildeEffect)
                        {
                            ParseTildeEffect(res, args, len, used, *fmt - '2');
                            tildeEffect = false;
                        }
                        else
                        {
                            ParseNoTildeEffect(res, args, len, used, *fmt - '2');
                        }
                    }
                };
                break;

                case '1':
                    if (!done || (*fmt == '1'))
                    {
                        /*FIXME Is the call to SearchPathW() really needed? We already have separated out the parameter string in args. */
                        if (SearchPathW(lpDir, lpFile, L".exe", ARRAY_SIZE(xlpFile), xlpFile, NULL))
                            cmd = xlpFile;
                        else
                            cmd = lpFile;

                        SIZE_T cmdlen = wcslen(cmd);
                        used += cmdlen;
                        if (used < len)
                        {
                            wcscpy(res, cmd);
                            res += cmdlen;
                        }
                    }
                    found_p1 = TRUE;
                    break;

                    /*
                     * IE uses this a lot for activating things such as windows media
                     * player. This is not verified to be fully correct but it appears
                     * to work just fine.
                     */
                case 'l':
                case 'L':
                    if (lpFile)
                    {
                        used += wcslen(lpFile);
                        if (used < len)
                        {
                            wcscpy(res, lpFile);
                            res += wcslen(lpFile);
                        }
                    }
                    found_p1 = TRUE;
                    break;

                case 'w':
                case 'W':
                    if (lpDir)
                    {
                        used += wcslen(lpDir);
                        if (used < len)
                        {
                            wcscpy(res, lpDir);
                            res += wcslen(lpDir);
                        }
                    }
                    break;

                case 'v':
                case 'V':
                    if (lpFile)
                    {
                        used += wcslen(lpFile);
                        if (used < len)
                        {
                            wcscpy(res, lpFile);
                            res += wcslen(lpFile);
                        }
                        found_p1 = TRUE;
                    }
                    else if (lpDir)
                    {
                        used += wcslen(lpDir);
                        if (used < len)
                        {
                            wcscpy(res, lpDir);
                            res += wcslen(lpDir);
                        }
                    }
                    break;

                case 'i':
                case 'I':
                    if (pidl)
                    {
                        DWORD chars = 0;
                        /* %p should not exceed 8, maybe 16 when looking forward to 64bit.
                            * allowing a buffer of 100 should more than exceed all needs */
                        WCHAR buf[100];
                        LPVOID  pv;
                        HGLOBAL hmem = SHAllocShared(pidl, ILGetSize(pidl), 0);
                        pv = SHLockShared(hmem, 0);
                        chars = swprintf(buf, L":%p", pv);

                        if (chars >= ARRAY_SIZE(buf))
                            ERR("pidl format buffer too small!\n");

                        used += chars;

                        if (used < len)
                        {
                            wcscpy(res, buf);
                            res += chars;
                        }
                        SHUnlockShared(pv);
                    }
                    found_p1 = TRUE;
                    break;

                default:
                    /*
                     * Check if this is an env-variable here...
                     */

                    /* Make sure that we have at least one more %.*/
                    if (strchrW(fmt, '%'))
                    {
                        WCHAR   tmpBuffer[1024];
                        PWSTR   tmpB = tmpBuffer;
                        WCHAR   tmpEnvBuff[MAX_PATH];
                        DWORD   envRet;

                        while (*fmt != '%')
                            *tmpB++ = *fmt++;
                        *tmpB++ = 0;

                        TRACE("Checking %s to be an env-var\n", debugstr_w(tmpBuffer));

                        envRet = GetEnvironmentVariableW(tmpBuffer, tmpEnvBuff, MAX_PATH);
                        if (envRet == 0 || envRet > MAX_PATH)
                        {
                            used += wcslen(tmpBuffer);
                            if (used < len)
                            {
                                wcscpy( res, tmpBuffer );
                                res += wcslen(tmpBuffer);
                            }
                        }
                        else
                        {
                            used += wcslen(tmpEnvBuff);
                            if (used < len)
                            {
                                wcscpy( res, tmpEnvBuff );
                                res += wcslen(tmpEnvBuff);
                            }
                        }
                    }
                    done = TRUE;
                    break;
            }
            /* Don't skip past terminator (catch a single '%' at the end) */
            if (*fmt != '\0')
            {
                fmt++;
            }
        }
        else
        {
            used ++;
            if (used < len)
                *res++ = *fmt++;
            else
                fmt++;
        }
    }

    used++;
    if (res - out < static_cast<int>(len))
        *res = '\0';
    else
        out[len-1] = '\0';

    TRACE("used %i of %i space\n", used, len);
    if (out_len)
        *out_len = used;

    TRACE("After parsing: %p, %d, %s, %s, %p, %p\n", out, len, debugstr_w(fmt),
          debugstr_w(lpFile), pidl, args);

    return found_p1;
}

static HRESULT SHELL_GetPathFromIDListForExecuteW(LPCITEMIDLIST pidl, LPWSTR pszPath, UINT uOutSize)
{
    STRRET strret;
    CComPtr<IShellFolder> desktop;

    HRESULT hr = SHGetDesktopFolder(&desktop);

    if (SUCCEEDED(hr))
    {
        hr = desktop->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strret);

        if (SUCCEEDED(hr))
            StrRetToStrNW(pszPath, uOutSize, &strret, pidl);
    }

    return hr;
}

/*************************************************************************
 *    SHELL_ExecuteW [Internal]
 *
 */
static UINT_PTR SHELL_ExecuteW(const WCHAR *lpCmd, WCHAR *env, BOOL shWait,
                               const SHELLEXECUTEINFOW *psei, LPSHELLEXECUTEINFOW psei_out)
{
    STARTUPINFOW  startup;
    PROCESS_INFORMATION info;
    UINT_PTR retval = SE_ERR_NOASSOC;
    UINT gcdret = 0;
    WCHAR curdir[MAX_PATH];
    DWORD dwCreationFlags;
    const WCHAR *lpDirectory = NULL;

    TRACE("Execute %s from directory %s\n", debugstr_w(lpCmd), debugstr_w(psei->lpDirectory));

    /* make sure we don't fail the CreateProcess if the calling app passes in
     * a bad working directory */
    if (!StrIsNullOrEmpty(psei->lpDirectory))
    {
        DWORD attr = GetFileAttributesW(psei->lpDirectory);
        if (attr != INVALID_FILE_ATTRIBUTES && attr & FILE_ATTRIBUTE_DIRECTORY)
            lpDirectory = psei->lpDirectory;
    }

    /* ShellExecute specifies the command from psei->lpDirectory
     * if present. Not from the current dir as CreateProcess does */
    if (lpDirectory)
        if ((gcdret = GetCurrentDirectoryW( MAX_PATH, curdir)))
            if (!SetCurrentDirectoryW( lpDirectory))
                ERR("cannot set directory %s\n", debugstr_w(lpDirectory));

    ZeroMemory(&startup, sizeof(STARTUPINFOW));
    startup.cb = sizeof(STARTUPINFOW);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = psei->nShow;
    dwCreationFlags = CREATE_UNICODE_ENVIRONMENT;
    if (!(psei->fMask & SEE_MASK_NO_CONSOLE))
        dwCreationFlags |= CREATE_NEW_CONSOLE;
    if (psei->fMask & SEE_MASK_FLAG_SEPVDM)
        dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
    startup.lpTitle = (LPWSTR)(psei->fMask & (SEE_MASK_HASLINKNAME | SEE_MASK_HASTITLE) ? psei->lpClass : NULL);

    if (psei->fMask & SEE_MASK_HASLINKNAME)
        startup.dwFlags |= STARTF_TITLEISLINKNAME;

    if (CreateProcessW(NULL, (LPWSTR)lpCmd, NULL, NULL, FALSE, dwCreationFlags, env,
                       lpDirectory, &startup, &info))
    {
        /* Give 30 seconds to the app to come up, if desired. Probably only needed
           when starting app immediately before making a DDE connection. */
        if (shWait)
            if (WaitForInputIdle(info.hProcess, 30000) == WAIT_FAILED)
                WARN("WaitForInputIdle failed: Error %d\n", GetLastError() );
        retval = 33;

        if (psei->fMask & SEE_MASK_NOCLOSEPROCESS)
            psei_out->hProcess = info.hProcess;
        else
            CloseHandle( info.hProcess );
        CloseHandle( info.hThread );
    }
    else if ((retval = GetLastError()) >= 32)
    {
        WARN("CreateProcess returned error %ld\n", retval);
        retval = ERROR_BAD_FORMAT;
    }

    TRACE("returning %lu\n", retval);

    psei_out->hInstApp = (HINSTANCE)retval;

    if (gcdret)
        if (!SetCurrentDirectoryW(curdir))
            ERR("cannot return to directory %s\n", debugstr_w(curdir));

    return retval;
}


/***********************************************************************
 *           SHELL_BuildEnvW    [Internal]
 *
 * Build the environment for the new process, adding the specified
 * path to the PATH variable. Returned pointer must be freed by caller.
 */
static LPWSTR SHELL_BuildEnvW( const WCHAR *path )
{
    CHeapPtr<WCHAR, CLocalAllocator> new_env;
    WCHAR *strings, *p, *p2;
    int total = wcslen(path) + 1;
    BOOL got_path = FALSE;

    if (!(strings = GetEnvironmentStringsW())) return NULL;
    p = strings;
    while (*p)
    {
        int len = wcslen(p) + 1;
        if (!_wcsnicmp( p, L"PATH=", 5 )) got_path = TRUE;
        total += len;
        p += len;
    }
    if (!got_path) total += 5;  /* we need to create PATH */
    total++;  /* terminating null */

    if (!new_env.Allocate(total))
    {
        FreeEnvironmentStringsW(strings);
        return NULL;
    }
    p = strings;
    p2 = new_env;
    while (*p)
    {
        int len = wcslen(p) + 1;
        memcpy(p2, p, len * sizeof(WCHAR));
        if (!_wcsnicmp( p, L"PATH=", 5 ))
        {
            p2[len - 1] = ';';
            wcscpy( p2 + len, path );
            p2 += wcslen(path) + 1;
        }
        p += len;
        p2 += len;
    }
    if (!got_path)
    {
        wcscpy(p2, L"PATH=");
        wcscat(p2, path);
        p2 += wcslen(p2) + 1;
    }
    *p2 = 0;
    FreeEnvironmentStringsW(strings);
    return new_env.Detach();
}

/***********************************************************************
 *           SHELL_TryAppPathW    [Internal]
 *
 * Helper function for SHELL_FindExecutable
 * @param lpResult - pointer to a buffer of size MAX_PATH
 * On entry: szName is a filename (probably without path separators).
 * On exit: if szName found in "App Path", place full path in lpResult, and return true
 */
static BOOL SHELL_TryAppPathW( LPCWSTR szName, LPWSTR lpResult, WCHAR **env)
{
    HKEY hkApp = NULL;
    WCHAR buffer[1024];
    DWORD len, dwType;
    LONG res;
    BOOL found = FALSE;

    if (env) *env = NULL;
    wcscpy(buffer, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\");
    wcscat(buffer, szName);
    res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hkApp);
    if (res)
    {
        // Add ".exe" extension, if extension does not exists
        if (PathAddExtensionW(buffer, L".exe"))
        {
            res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hkApp);
        }
        if (res) goto end;
    }

    len = MAX_PATH * sizeof(WCHAR);
    res = SHRegQueryValueExW(hkApp, NULL, NULL, &dwType, (LPBYTE)lpResult, &len);
    if (res != ERROR_SUCCESS || dwType != REG_SZ)
        goto end;

    found = TRUE;

    if (env)
    {
        len = sizeof(buffer);
        res = SHRegQueryValueExW(hkApp, L"Path", NULL, &dwType, (LPBYTE)buffer, &len);
        if (res == ERROR_SUCCESS && dwType == REG_SZ && buffer[0])
            *env = SHELL_BuildEnvW(buffer);
    }

end:
    if (hkApp) RegCloseKey(hkApp);
    return found;
}

/*************************************************************************
 *     SHELL_FindExecutableByVerb [Internal]
 *
 * called from SHELL_FindExecutable or SHELL_execute_class
 * in/out:
 *      classname a buffer, big enough, to get the key name to do actually the
 *              command   "WordPad.Document.1\\shell\\open\\command"
 *              passed as "WordPad.Document.1"
 * in:
 *      lpVerb the operation on it (open)
 *      commandlen the size of command buffer (in bytes)
 * out:
 *      command a buffer, to store the command to do the
 *              operation on the file
 *      key a buffer, big enough, to get the key name to do actually the
 *              command "WordPad.Document.1\\shell\\open\\command"
 *              Can be NULL
 */
static UINT SHELL_FindExecutableByVerb(LPCWSTR lpVerb, LPWSTR key, LPWSTR classname, LPWSTR command, LONG commandlen)
{
    HKEY hkeyClass;
    WCHAR verb[MAX_PATH];

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, classname, 0, 0x02000000, &hkeyClass))
        return SE_ERR_NOASSOC;
    if (!HCR_GetDefaultVerbW(hkeyClass, lpVerb, verb, ARRAY_SIZE(verb)))
        return SE_ERR_NOASSOC;
    RegCloseKey(hkeyClass);

    /* Looking for ...buffer\shell\<verb>\command */
    wcscat(classname, L"\\shell\\");
    wcscat(classname, verb);
    wcscat(classname, L"\\command");

    if (RegQueryValueW(HKEY_CLASSES_ROOT, classname, command,
                       &commandlen) == ERROR_SUCCESS)
    {
        commandlen /= sizeof(WCHAR);
        if (key) wcscpy(key, classname);
#if 0
        LPWSTR tmp;
        WCHAR param[256];
        LONG paramlen = sizeof(param);

        /* FIXME: it seems all Windows version don't behave the same here.
         * the doc states that this ddeexec information can be found after
         * the exec names.
         * on Win98, it doesn't appear, but I think it does on Win2k
         */
        /* Get the parameters needed by the application
           from the associated ddeexec key */
        tmp = strstrW(classname, L"\\command");
        tmp[0] = '\0';
        wcscat(classname, wDdeexec);
        if (RegQueryValueW(HKEY_CLASSES_ROOT, classname, param,
                           &paramlen) == ERROR_SUCCESS)
        {
            paramlen /= sizeof(WCHAR);
            wcscat(command, L" ");
            wcscat(command, param);
            commandlen += paramlen;
        }
#endif

        command[commandlen] = '\0';

        return 33; /* FIXME see SHELL_FindExecutable() */
    }

    return SE_ERR_NOASSOC;
}

/*************************************************************************
 *    SHELL_FindExecutable [Internal]
 *
 * Utility for code sharing between FindExecutable and ShellExecute
 * in:
 *      lpFile the name of a file
 *      lpVerb the operation on it (open)
 * out:
 *      lpResult a buffer, big enough :-(, to store the command to do the
 *              operation on the file
 *      key a buffer, big enough, to get the key name to do actually the
 *              command (it'll be used afterwards for more information
 *              on the operation)
 */
static UINT SHELL_FindExecutable(LPCWSTR lpPath, LPCWSTR lpFile, LPCWSTR lpVerb,
                                 LPWSTR lpResult, DWORD resultLen, LPWSTR key, WCHAR **env, LPITEMIDLIST pidl, LPCWSTR args)
{
    WCHAR *extension = NULL; /* pointer to file extension */
    WCHAR classname[256];     /* registry name for this file type */
    LONG  classnamelen = sizeof(classname); /* length of above */
    WCHAR command[1024];     /* command from registry */
    WCHAR wBuffer[256];      /* Used to GetProfileString */
    UINT  retval = SE_ERR_NOASSOC;
    WCHAR *tok;              /* token pointer */
    WCHAR xlpFile[256];      /* result of SearchPath */
    DWORD attribs;           /* file attributes */

    TRACE("%s\n", debugstr_w(lpFile));

    if (!lpResult)
        return ERROR_INVALID_PARAMETER;

    xlpFile[0] = '\0';
    lpResult[0] = '\0'; /* Start off with an empty return string */
    if (key) *key = '\0';

    /* trap NULL parameters on entry */
    if (!lpFile)
    {
        WARN("(lpFile=%s,lpResult=%s): NULL parameter\n",
             debugstr_w(lpFile), debugstr_w(lpResult));
        return ERROR_FILE_NOT_FOUND; /* File not found. Close enough, I guess. */
    }

    if (SHELL_TryAppPathW( lpFile, lpResult, env ))
    {
        TRACE("found %s via App Paths\n", debugstr_w(lpResult));
        return 33;
    }

    if (SearchPathW(lpPath, lpFile, L".exe", ARRAY_SIZE(xlpFile), xlpFile, NULL))
    {
        TRACE("SearchPathW returned non-zero\n");
        lpFile = xlpFile;
        /* The file was found in the application-supplied default directory (or the system search path) */
    }
    else if (lpPath && SearchPathW(NULL, lpFile, L".exe", ARRAY_SIZE(xlpFile), xlpFile, NULL))
    {
        TRACE("SearchPathW returned non-zero\n");
        lpFile = xlpFile;
        /* The file was found in one of the directories in the system-wide search path */
    }

    attribs = GetFileAttributesW(lpFile);
    if (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY))
    {
        wcscpy(classname, L"Folder");
    }
    else
    {
        /* Did we get something? Anything? */
        if (xlpFile[0] == 0)
        {
            TRACE("Returning SE_ERR_FNF\n");
            return SE_ERR_FNF;
        }
        /* First thing we need is the file's extension */
        extension = wcsrchr(xlpFile, '.'); /* Assume last "." is the one; */
        /* File->Run in progman uses */
        /* .\FILE.EXE :( */
        TRACE("xlpFile=%s,extension=%s\n", debugstr_w(xlpFile), debugstr_w(extension));

        if (extension == NULL || extension[1] == 0)
        {
            WARN("Returning SE_ERR_NOASSOC\n");
            return SE_ERR_NOASSOC;
        }

        /* Three places to check: */
        /* 1. win.ini, [windows], programs (NB no leading '.') */
        /* 2. Registry, HKEY_CLASS_ROOT\<classname>\shell\open\command */
        /* 3. win.ini, [extensions], extension (NB no leading '.' */
        /* All I know of the order is that registry is checked before */
        /* extensions; however, it'd make sense to check the programs */
        /* section first, so that's what happens here. */

        /* See if it's a program - if GetProfileString fails, we skip this
         * section. Actually, if GetProfileString fails, we've probably
         * got a lot more to worry about than running a program... */
        if (GetProfileStringW(L"windows", L"programs", L"exe pif bat cmd com", wBuffer, ARRAY_SIZE(wBuffer)) > 0)
        {
            CharLowerW(wBuffer);
            tok = wBuffer;
            while (*tok)
            {
                WCHAR *p = tok;
                while (*p && *p != ' ' && *p != '\t') p++;
                if (*p)
                {
                    *p++ = 0;
                    while (*p == ' ' || *p == '\t') p++;
                }

                if (_wcsicmp(tok, &extension[1]) == 0) /* have to skip the leading "." */
                {
                    wcscpy(lpResult, xlpFile);
                    /* Need to perhaps check that the file has a path
                     * attached */
                    TRACE("found %s\n", debugstr_w(lpResult));
                    return 33;
                    /* Greater than 32 to indicate success */
                }
                tok = p;
            }
        }

        /* Check registry */
        if (RegQueryValueW(HKEY_CLASSES_ROOT, extension, classname,
                           &classnamelen) == ERROR_SUCCESS)
        {
            classnamelen /= sizeof(WCHAR);
            if (classnamelen == ARRAY_SIZE(classname))
                classnamelen--;

            classname[classnamelen] = '\0';
            TRACE("File type: %s\n", debugstr_w(classname));
        }
        else
        {
            *classname = '\0';
        }
    }

    if (*classname)
    {
        /* pass the verb string to SHELL_FindExecutableByVerb() */
        retval = SHELL_FindExecutableByVerb(lpVerb, key, classname, command, sizeof(command));

        if (retval > 32)
        {
            DWORD finishedLen;
            SHELL_ArgifyW(lpResult, resultLen, command, xlpFile, pidl, args, &finishedLen, lpPath);
            if (finishedLen > resultLen)
                ERR("Argify buffer not large enough.. truncated\n");
            /* Remove double quotation marks and command line arguments */
            if (*lpResult == '"')
            {
                WCHAR *p = lpResult;
                while (*(p + 1) != '"')
                {
                    *p = *(p + 1);
                    p++;
                }
                *p = '\0';
            }
            else
            {
                /* Truncate on first space */
                WCHAR *p = lpResult;
                while (*p != ' ' && *p != '\0')
                    p++;
                *p = '\0';
            }
        }
    }
    else /* Check win.ini */
    {
        /* Toss the leading dot */
        extension++;
        if (GetProfileStringW(L"extensions", extension, L"", command, ARRAY_SIZE(command)) > 0)
        {
            if (wcslen(command) != 0)
            {
                wcscpy(lpResult, command);
                tok = wcschr(lpResult, '^'); /* should be ^.extension? */
                if (tok != NULL)
                {
                    tok[0] = '\0';
                    wcscat(lpResult, xlpFile); /* what if no dir in xlpFile? */
                    tok = wcschr(command, '^'); /* see above */
                    if ((tok != NULL) && (wcslen(tok) > 5))
                    {
                        wcscat(lpResult, &tok[5]);
                    }
                }
                retval = 33; /* FIXME - see above */
            }
        }
    }

    TRACE("returning %s\n", debugstr_w(lpResult));
    return retval;
}

/******************************************************************
 *        dde_cb
 *
 * callback for the DDE connection. not really useful
 */
static HDDEDATA CALLBACK dde_cb(UINT uType, UINT uFmt, HCONV hConv,
                                HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    TRACE("dde_cb: %04x, %04x, %p, %p, %p, %p, %08lx, %08lx\n",
          uType, uFmt, hConv, hsz1, hsz2, hData, dwData1, dwData2);
    return NULL;
}

/******************************************************************
 *        dde_connect
 *
 * ShellExecute helper. Used to do an operation with a DDE connection
 *
 * Handles both the direct connection (try #1), and if it fails,
 * launching an application and trying (#2) to connect to it
 *
 */
static unsigned dde_connect(const WCHAR* key, const WCHAR* start, WCHAR* ddeexec,
                            const WCHAR* lpFile, WCHAR *env,
                            LPCWSTR szCommandline, LPITEMIDLIST pidl, SHELL_ExecuteW32 execfunc,
                            const SHELLEXECUTEINFOW *psei, LPSHELLEXECUTEINFOW psei_out)
{
    WCHAR       regkey[256];
    WCHAR *     endkey = regkey + wcslen(key);
    WCHAR       app[256], topic[256], ifexec[256], static_res[256];
    CHeapPtr<WCHAR, CLocalAllocator> dynamic_res;
    WCHAR *     res;
    LONG        applen, topiclen, ifexeclen;
    WCHAR *     exec;
    DWORD       ddeInst = 0;
    DWORD       tid;
    DWORD       resultLen, endkeyLen;
    HSZ         hszApp, hszTopic;
    HCONV       hConv;
    HDDEDATA    hDdeData;
    unsigned    ret = SE_ERR_NOASSOC;
    BOOL unicode = !(GetVersion() & 0x80000000);

    if (strlenW(key) + 1 > ARRAY_SIZE(regkey))
    {
        FIXME("input parameter %s larger than buffer\n", debugstr_w(key));
        return 2;
    }
    wcscpy(regkey, key);
    endkeyLen = ARRAY_SIZE(regkey) - (endkey - regkey);
    if (strlenW(L"\\application") + 1 > endkeyLen)
    {
        FIXME("endkey %s overruns buffer\n", debugstr_w(L"\\application"));
        return 2;
    }
    wcscpy(endkey, L"\\application");
    applen = sizeof(app);
    if (RegQueryValueW(HKEY_CLASSES_ROOT, regkey, app, &applen) != ERROR_SUCCESS)
    {
        WCHAR command[1024], fullpath[MAX_PATH];
        LPWSTR ptr = NULL;
        DWORD ret = 0;

        /* Get application command from start string and find filename of application */
        if (*start == '"')
        {
            if (strlenW(start + 1) + 1 > ARRAY_SIZE(command))
            {
                FIXME("size of input parameter %s larger than buffer\n",
                      debugstr_w(start + 1));
                return 2;
            }
            wcscpy(command, start + 1);
            if ((ptr = wcschr(command, '"')))
                * ptr = 0;
            ret = SearchPathW(NULL, command, L".exe", ARRAY_SIZE(fullpath), fullpath, &ptr);
        }
        else
        {
            LPCWSTR p;
            LPWSTR space;
            for (p = start; (space = const_cast<LPWSTR>(strchrW(p, ' '))); p = space + 1)
            {
                int idx = space - start;
                memcpy(command, start, idx * sizeof(WCHAR));
                command[idx] = '\0';
                if ((ret = SearchPathW(NULL, command, L".exe", ARRAY_SIZE(fullpath), fullpath, &ptr)))
                    break;
            }
            if (!ret)
                ret = SearchPathW(NULL, start, L".exe", ARRAY_SIZE(fullpath), fullpath, &ptr);
        }

        if (!ret)
        {
            ERR("Unable to find application path for command %s\n", debugstr_w(start));
            return ERROR_ACCESS_DENIED;
        }
        if (strlenW(ptr) + 1 > ARRAY_SIZE(app))
        {
            FIXME("size of found path %s larger than buffer\n", debugstr_w(ptr));
            return 2;
        }
        wcscpy(app, ptr);

        /* Remove extensions (including .so) */
        ptr = app + wcslen(app) - 3;
        if (ptr > app && !wcscmp(ptr, L".so"))
            *ptr = 0;

        ptr = const_cast<LPWSTR>(strrchrW(app, '.'));
        assert(ptr);
        *ptr = 0;
    }

    if (strlenW(L"\\topic") + 1 > endkeyLen)
    {
        FIXME("endkey %s overruns buffer\n", debugstr_w(L"\\topic"));
        return 2;
    }
    wcscpy(endkey, L"\\topic");
    topiclen = sizeof(topic);
    if (RegQueryValueW(HKEY_CLASSES_ROOT, regkey, topic, &topiclen) != ERROR_SUCCESS)
    {
        wcscpy(topic, L"System");
    }

    if (unicode)
    {
        if (DdeInitializeW(&ddeInst, dde_cb, APPCMD_CLIENTONLY, 0L) != DMLERR_NO_ERROR)
            return 2;
    }
    else
    {
        if (DdeInitializeA(&ddeInst, dde_cb, APPCMD_CLIENTONLY, 0L) != DMLERR_NO_ERROR)
            return 2;
    }

    hszApp = DdeCreateStringHandleW(ddeInst, app, CP_WINUNICODE);
    hszTopic = DdeCreateStringHandleW(ddeInst, topic, CP_WINUNICODE);

    hConv = DdeConnect(ddeInst, hszApp, hszTopic, NULL);
    exec = ddeexec;
    if (!hConv)
    {
        TRACE("Launching %s\n", debugstr_w(start));
        ret = execfunc(start, env, TRUE, psei, psei_out);
        if (ret <= 32)
        {
            TRACE("Couldn't launch\n");
            goto error;
        }
        /* if ddeexec is NULL, then we just need to exit here */
        if (ddeexec == NULL)
        {
            TRACE("Exiting because ddeexec is NULL. ret=42.\n");
            /* See https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew */
            /* for reason why we use 42 here and also "Shell32_apitest ShellExecuteW" regression test */
            return 42;
        }
        /* if ddeexec is 'empty string', then we just need to exit here */
        if (wcscmp(ddeexec, L"") == 0)
        {
            TRACE("Exiting because ddeexec is 'empty string'. ret=42.\n");
            /* See https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew */
            /* for reason why we use 42 here and also "Shell32_apitest ShellExecuteW" regression test */
            return 42;
        }
        hConv = DdeConnect(ddeInst, hszApp, hszTopic, NULL);
        if (!hConv)
        {
            TRACE("Couldn't connect. ret=%d\n", ret);
            DdeUninitialize(ddeInst);
            SetLastError(ERROR_DDE_FAIL);
            return 30; /* whatever */
        }
        if (strlenW(L"\\ifexec") + 1 > endkeyLen)
        {
            FIXME("endkey %s overruns buffer\n", debugstr_w(L"\\ifexec"));
            return 2;
        }
        strcpyW(endkey, L"\\ifexec");
        ifexeclen = sizeof(ifexec);
        if (RegQueryValueW(HKEY_CLASSES_ROOT, regkey, ifexec, &ifexeclen) == ERROR_SUCCESS)
        {
            exec = ifexec;
        }
    }

    SHELL_ArgifyW(static_res, ARRAY_SIZE(static_res), exec, lpFile, pidl, szCommandline, &resultLen, NULL);
    if (resultLen > ARRAY_SIZE(static_res))
    {
        dynamic_res.Allocate(resultLen);
        res = dynamic_res;
        SHELL_ArgifyW(dynamic_res, resultLen, exec, lpFile, pidl, szCommandline, NULL, NULL);
    }
    else
        res = static_res;
    TRACE("%s %s => %s\n", debugstr_w(exec), debugstr_w(lpFile), debugstr_w(res));

    /* It's documented in the KB 330337 that IE has a bug and returns
     * error DMLERR_NOTPROCESSED on XTYP_EXECUTE request.
     */
    if (unicode)
        hDdeData = DdeClientTransaction((LPBYTE)res, (strlenW(res) + 1) * sizeof(WCHAR), hConv, 0L, 0, XTYP_EXECUTE, 30000, &tid);
    else
    {
        DWORD lenA = WideCharToMultiByte(CP_ACP, 0, res, -1, NULL, 0, NULL, NULL);
        CHeapPtr<char, CLocalAllocator> resA;
        resA.Allocate(lenA);
        WideCharToMultiByte(CP_ACP, 0, res, -1, resA, lenA, NULL, NULL);
        hDdeData = DdeClientTransaction( (LPBYTE)(LPSTR)resA, lenA, hConv, 0L, 0,
                                         XTYP_EXECUTE, 10000, &tid );
    }
    if (hDdeData)
        DdeFreeDataHandle(hDdeData);
    else
        WARN("DdeClientTransaction failed with error %04x\n", DdeGetLastError(ddeInst));
    ret = 33;

    DdeDisconnect(hConv);

error:
    DdeUninitialize(ddeInst);

    return ret;
}

/*************************************************************************
 *    execute_from_key [Internal]
 */
static UINT_PTR execute_from_key(LPCWSTR key, LPCWSTR lpFile, WCHAR *env,
                                 LPCWSTR szCommandline, LPCWSTR executable_name,
                                 SHELL_ExecuteW32 execfunc,
                                 LPSHELLEXECUTEINFOW psei, LPSHELLEXECUTEINFOW psei_out)
{
    WCHAR cmd[256], param[1024], ddeexec[256];
    DWORD cmdlen = sizeof(cmd), ddeexeclen = sizeof(ddeexec);
    UINT_PTR retval = SE_ERR_NOASSOC;
    DWORD resultLen;
    LPWSTR tmp;

    TRACE("%s %s %s %s %s\n", debugstr_w(key), debugstr_w(lpFile), debugstr_w(env),
          debugstr_w(szCommandline), debugstr_w(executable_name));

    cmd[0] = '\0';
    param[0] = '\0';

    /* Get the application from the registry */
    if (RegQueryValueW(HKEY_CLASSES_ROOT, key, cmd, (LONG *)&cmdlen) == ERROR_SUCCESS)
    {
        TRACE("got cmd: %s\n", debugstr_w(cmd));

        /* Is there a replace() function anywhere? */
        cmdlen /= sizeof(WCHAR);
        if (cmdlen >= ARRAY_SIZE(cmd))
            cmdlen = ARRAY_SIZE(cmd) - 1;
        cmd[cmdlen] = '\0';
        SHELL_ArgifyW(param, ARRAY_SIZE(param), cmd, lpFile, (LPITEMIDLIST)psei->lpIDList, szCommandline, &resultLen,
                      (psei->lpDirectory && *psei->lpDirectory) ? psei->lpDirectory : NULL);
        if (resultLen > ARRAY_SIZE(param))
            ERR("Argify buffer not large enough, truncating\n");
    }

    /* Get the parameters needed by the application
       from the associated ddeexec key */
    tmp = const_cast<LPWSTR>(strstrW(key, L"command"));
    assert(tmp);
    wcscpy(tmp, L"ddeexec");

    if (RegQueryValueW(HKEY_CLASSES_ROOT, key, ddeexec, (LONG *)&ddeexeclen) == ERROR_SUCCESS)
    {
        TRACE("Got ddeexec %s => %s\n", debugstr_w(key), debugstr_w(ddeexec));
        if (!param[0]) strcpyW(param, executable_name);
        retval = dde_connect(key, param, ddeexec, lpFile, env, szCommandline, (LPITEMIDLIST)psei->lpIDList, execfunc, psei, psei_out);
    }
    else if (param[0])
    {
        TRACE("executing: %s\n", debugstr_w(param));
        retval = execfunc(param, env, FALSE, psei, psei_out);
    }
    else
        WARN("Nothing appropriate found for %s\n", debugstr_w(key));

    return retval;
}

/*************************************************************************
 * FindExecutableA            [SHELL32.@]
 */
HINSTANCE WINAPI FindExecutableA(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
    HINSTANCE retval;
    WCHAR *wFile = NULL, *wDirectory = NULL;
    WCHAR wResult[MAX_PATH];

    if (lpFile) __SHCloneStrAtoW(&wFile, lpFile);
    if (lpDirectory) __SHCloneStrAtoW(&wDirectory, lpDirectory);

    retval = FindExecutableW(wFile, wDirectory, wResult);
    WideCharToMultiByte(CP_ACP, 0, wResult, -1, lpResult, MAX_PATH, NULL, NULL);
    SHFree(wFile);
    SHFree(wDirectory);

    TRACE("returning %s\n", lpResult);
    return retval;
}

/*************************************************************************
 * FindExecutableW            [SHELL32.@]
 *
 * This function returns the executable associated with the specified file
 * for the default verb.
 *
 * PARAMS
 *  lpFile   [I] The file to find the association for. This must refer to
 *               an existing file otherwise FindExecutable fails and returns
 *               SE_ERR_FNF.
 *  lpResult [O] Points to a buffer into which the executable path is
 *               copied. This parameter must not be NULL otherwise
 *               FindExecutable() segfaults. The buffer must be of size at
 *               least MAX_PATH characters.
 *
 * RETURNS
 *  A value greater than 32 on success, less than or equal to 32 otherwise.
 *  See the SE_ERR_* constants.
 *
 * NOTES
 *  On Windows XP and 2003, FindExecutable() seems to first convert the
 *  filename into 8.3 format, thus taking into account only the first three
 *  characters of the extension, and expects to find an association for those.
 *  However other Windows versions behave sanely.
 */
HINSTANCE WINAPI FindExecutableW(LPCWSTR lpFile, LPCWSTR lpDirectory, LPWSTR lpResult)
{
    UINT_PTR retval;
    WCHAR old_dir[MAX_PATH], res[MAX_PATH];
    DWORD cch = _countof(res);
    LPCWSTR dirs[2];

    TRACE("File %s, Dir %s\n", debugstr_w(lpFile), debugstr_w(lpDirectory));

    *lpResult = UNICODE_NULL;

    GetCurrentDirectoryW(_countof(old_dir), old_dir);

    if (lpDirectory && *lpDirectory)
    {
        SetCurrentDirectoryW(lpDirectory);
        dirs[0] = lpDirectory;
    }
    else
    {
        dirs[0] = old_dir;
    }
    dirs[1] = NULL;

    if (!GetShortPathNameW(lpFile, res, _countof(res)))
        StringCchCopyW(res, _countof(res), lpFile);

    if (PathResolveW(res, dirs, PRF_TRYPROGRAMEXTENSIONS | PRF_FIRSTDIRDEF))
    {
        // NOTE: The last parameter of this AssocQueryStringW call is "strange" in Windows.
        if (PathIsExeW(res) ||
            SUCCEEDED(AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_EXECUTABLE, res, NULL, res, &cch)))
        {
            StringCchCopyW(lpResult, MAX_PATH, res);
            retval = 42;
        }
        else
        {
            retval = SE_ERR_NOASSOC;
        }
    }
    else
    {
        retval = SE_ERR_FNF;
    }

    TRACE("returning %s\n", debugstr_w(lpResult));
    SetCurrentDirectoryW(old_dir);
    return (HINSTANCE)retval;
}

/* FIXME: is this already implemented somewhere else? */
static HKEY ShellExecute_GetClassKey(const SHELLEXECUTEINFOW *sei)
{
    LPCWSTR ext = NULL, lpClass = NULL;
    CHeapPtr<WCHAR, CLocalAllocator> cls;
    DWORD type = 0, sz = 0;
    HKEY hkey = 0;
    LONG r;

    if (sei->fMask & SEE_MASK_CLASSALL)
        return sei->hkeyClass;

    if (sei->fMask & SEE_MASK_CLASSNAME)
        lpClass = sei->lpClass;
    else
    {
        ext = PathFindExtensionW(sei->lpFile);
        TRACE("ext = %s\n", debugstr_w(ext));
        if (!ext)
            return hkey;

        r = RegOpenKeyW(HKEY_CLASSES_ROOT, ext, &hkey);
        if (r != ERROR_SUCCESS)
            return hkey;

        r = RegQueryValueExW(hkey, NULL, 0, &type, NULL, &sz);
        if (r == ERROR_SUCCESS && type == REG_SZ)
        {
            sz += sizeof (WCHAR);
            cls.Allocate(sz / sizeof(WCHAR));
            cls[0] = 0;
            RegQueryValueExW(hkey, NULL, 0, &type, (LPBYTE)(LPWSTR)cls, &sz);
        }

        RegCloseKey( hkey );
        lpClass = cls;
    }

    TRACE("class = %s\n", debugstr_w(lpClass));

    hkey = 0;
    if (lpClass)
        RegOpenKeyW( HKEY_CLASSES_ROOT, lpClass, &hkey);

    return hkey;
}

static HRESULT shellex_get_dataobj( LPSHELLEXECUTEINFOW sei, CComPtr<IDataObject>& dataObj)
{
    CComHeapPtr<ITEMIDLIST> allocatedPidl;
    LPITEMIDLIST pidl = NULL;

    if (sei->fMask & SEE_MASK_CLASSALL)
    {
        pidl = (LPITEMIDLIST)sei->lpIDList;
    }
    else
    {
        WCHAR fullpath[MAX_PATH];
        BOOL ret;

        fullpath[0] = 0;
        ret = GetFullPathNameW(sei->lpFile, MAX_PATH, fullpath, NULL);
        if (!ret)
            return HRESULT_FROM_WIN32(GetLastError());

        pidl = ILCreateFromPathW(fullpath);
        allocatedPidl.Attach(pidl);
    }
    return SHELL_GetUIObjectOfAbsoluteItem(NULL, pidl, IID_PPV_ARG(IDataObject, &dataObj));
}

static HRESULT shellex_run_context_menu_default(IShellExtInit *obj,
        LPSHELLEXECUTEINFOW sei)
{
    CComPtr<IContextMenu> cm = NULL;
    CMINVOKECOMMANDINFOEX ici;
    MENUITEMINFOW info;
    WCHAR string[0x80];
    INT i, n, def = -1;
    HMENU hmenu = 0;
    HRESULT r;

    TRACE("%p %p\n", obj, sei);

    r = obj->QueryInterface(IID_PPV_ARG(IContextMenu, &cm));
    if (FAILED(r))
        return r;

    hmenu = CreateMenu();
    if (!hmenu)
        goto end;

    /* the number of the last menu added is returned in r */
    r = cm->QueryContextMenu(hmenu, 0, 0x20, 0x7fff, CMF_DEFAULTONLY);
    if (FAILED(r))
        goto end;

    n = GetMenuItemCount(hmenu);
    for (i = 0; i < n; i++)
    {
        memset(&info, 0, sizeof(info));
        info.cbSize = sizeof info;
        info.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_DATA | MIIM_ID;
        info.dwTypeData = string;
        info.cch = sizeof string;
        string[0] = 0;
        GetMenuItemInfoW(hmenu, i, TRUE, &info);

        TRACE("menu %d %s %08x %08lx %08x %08x\n", i, debugstr_w(string),
              info.fState, info.dwItemData, info.fType, info.wID);
        if ((!sei->lpVerb && (info.fState & MFS_DEFAULT)) ||
            (sei->lpVerb && !lstrcmpiW(sei->lpVerb, string)))
        {
            def = i;
            break;
        }
    }

    r = E_FAIL;
    if (def == -1)
        goto end;

    memset(&ici, 0, sizeof ici);
    ici.cbSize = sizeof ici;
    ici.fMask = CMIC_MASK_UNICODE | (sei->fMask & (SEE_MASK_NO_CONSOLE | SEE_MASK_NOASYNC | SEE_MASK_ASYNCOK | SEE_MASK_FLAG_NO_UI));
    ici.nShow = sei->nShow;
    ici.lpVerb = MAKEINTRESOURCEA(def);
    ici.hwnd = sei->hwnd;
    ici.lpParametersW = sei->lpParameters;

    r = cm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);

    TRACE("invoke command returned %08x\n", r);

end:
    if (hmenu)
        DestroyMenu( hmenu );
    return r;
}

static HRESULT shellex_load_object_and_run(HKEY hkey, LPCGUID guid, LPSHELLEXECUTEINFOW sei)
{
    TRACE("%p %s %p\n", hkey, debugstr_guid(guid), sei);

    CCoInit coInit;

    if (FAILED_UNEXPECTEDLY(coInit.hr))
        return coInit.hr;

    CComPtr<IShellExtInit> obj;
    HRESULT hr = CoCreateInstance(*guid, NULL, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARG(IShellExtInit, &obj));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IDataObject> dataobj;
    hr = shellex_get_dataobj(sei, dataobj);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = obj->Initialize(NULL, dataobj, hkey);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComPtr<IObjectWithSite> ows;
    hr = obj->QueryInterface(IID_PPV_ARG(IObjectWithSite, &ows));
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    ows->SetSite(NULL);

    return shellex_run_context_menu_default(obj, sei);
}

static HRESULT shellex_get_contextmenu(LPSHELLEXECUTEINFOW sei, CComPtr<IContextMenu>& cm)
{
    CComHeapPtr<ITEMIDLIST> allocatedPidl;
    LPITEMIDLIST pidl = NULL;

    if (sei->lpIDList)
    {
        pidl = (LPITEMIDLIST)sei->lpIDList;
    }
    else
    {
        SFGAOF sfga = 0;
        HRESULT hr = SHParseDisplayName(sei->lpFile, NULL, &allocatedPidl, SFGAO_STORAGECAPMASK, &sfga);
        if (FAILED(hr))
        {
            WCHAR Buffer[MAX_PATH] = {};
            // FIXME: MAX_PATH.....
            UINT retval = SHELL_FindExecutable(sei->lpDirectory, sei->lpFile, sei->lpVerb, Buffer, _countof(Buffer), NULL, NULL, NULL, sei->lpParameters);
            if (retval <= 32)
                return HRESULT_FROM_WIN32(retval);

            hr = SHParseDisplayName(Buffer, NULL, &allocatedPidl, SFGAO_STORAGECAPMASK, &sfga);
            // This should not happen, we found it...
            if (FAILED_UNEXPECTEDLY(hr))
                return hr;
        }

        pidl = allocatedPidl;
    }

    CComPtr<IShellFolder> shf;
    LPCITEMIDLIST pidllast = NULL;
    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &shf), &pidllast);
    if (FAILED(hr))
        return hr;

    return shf->GetUIObjectOf(NULL, 1, &pidllast, IID_NULL_PPV_ARG(IContextMenu, &cm));
}

static HRESULT ShellExecute_ContextMenuVerb(LPSHELLEXECUTEINFOW sei)
{
    TRACE("%p\n", sei);

    CCoInit coInit;

    if (FAILED_UNEXPECTEDLY(coInit.hr))
        return coInit.hr;

    CComPtr<IContextMenu> cm;
    HRESULT hr = shellex_get_contextmenu(sei, cm);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    CComHeapPtr<char> verb, parameters, dir;
    __SHCloneStrWtoA(&verb, sei->lpVerb);
    __SHCloneStrWtoA(&parameters, sei->lpParameters);
    __SHCloneStrWtoA(&dir, sei->lpDirectory);

    BOOL fDefault = StrIsNullOrEmpty(sei->lpVerb);
    CMINVOKECOMMANDINFOEX ici = { sizeof(ici) };
    ici.fMask = SeeFlagsToCmicFlags(sei->fMask) | CMIC_MASK_UNICODE;
    ici.nShow = sei->nShow;
    if (!fDefault)
    {
        ici.lpVerb = verb;
        ici.lpVerbW = sei->lpVerb;
    }
    ici.hwnd = sei->hwnd;
    ici.lpParameters = parameters;
    ici.lpParametersW = sei->lpParameters;
    ici.lpDirectory = dir;
    ici.lpDirectoryW = sei->lpDirectory;
    ici.dwHotKey = sei->dwHotKey;
    ici.hIcon = sei->hIcon;
    if (ici.fMask & (CMIC_MASK_HASLINKNAME | CMIC_MASK_HASTITLE))
        ici.lpTitleW = sei->lpClass;

    enum { idFirst = 1, idLast = 0x7fff };
    HMENU hMenu = CreatePopupMenu();
    // Note: Windows does not pass CMF_EXTENDEDVERBS so "hidden" verbs cannot be executed
    hr = cm->QueryContextMenu(hMenu, 0, idFirst, idLast, fDefault ? CMF_DEFAULTONLY : 0);
    if (!FAILED_UNEXPECTEDLY(hr))
    {
        if (fDefault)
        {
            INT uDefault = GetMenuDefaultItem(hMenu, FALSE, 0);
            uDefault = (uDefault != -1) ? uDefault - idFirst : 0;
            ici.lpVerb = MAKEINTRESOURCEA(uDefault);
            ici.lpVerbW = MAKEINTRESOURCEW(uDefault);
        }

        hr = cm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
        if (!FAILED_UNEXPECTEDLY(hr))
            hr = S_OK;
    }

    DestroyMenu(hMenu);

    return hr;
}


/*************************************************************************
 *    ShellExecute_FromContextMenu [Internal]
 */
static LONG ShellExecute_FromContextMenuHandlers( LPSHELLEXECUTEINFOW sei )
{
    HKEY hkey, hkeycm = 0;
    WCHAR szguid[39];
    HRESULT hr;
    GUID guid;
    DWORD i;
    LONG r;

    TRACE("%s\n", debugstr_w(sei->lpFile));

    hkey = ShellExecute_GetClassKey(sei);
    if (!hkey)
        return ERROR_FUNCTION_FAILED;

    r = RegOpenKeyW(hkey, L"shellex\\ContextMenuHandlers", &hkeycm);
    if (r == ERROR_SUCCESS)
    {
        i = 0;
        while (1)
        {
            r = RegEnumKeyW(hkeycm, i++, szguid, ARRAY_SIZE(szguid));
            if (r != ERROR_SUCCESS)
                break;

            hr = CLSIDFromString(szguid, &guid);
            if (SUCCEEDED(hr))
            {
                /* stop at the first one that succeeds in running */
                hr = shellex_load_object_and_run(hkey, &guid, sei);
                if (SUCCEEDED(hr))
                    break;
            }
        }
        RegCloseKey(hkeycm);
    }

    if (hkey != sei->hkeyClass)
        RegCloseKey(hkey);
    return r;
}

static UINT_PTR SHELL_quote_and_execute(LPCWSTR wcmd, LPCWSTR wszParameters, LPCWSTR lpstrProtocol, LPCWSTR wszApplicationName, LPWSTR env, LPSHELLEXECUTEINFOW psei, LPSHELLEXECUTEINFOW psei_out, SHELL_ExecuteW32 execfunc);

static UINT_PTR SHELL_execute_class(LPCWSTR wszApplicationName, LPSHELLEXECUTEINFOW psei, LPSHELLEXECUTEINFOW psei_out, SHELL_ExecuteW32 execfunc)
{
    WCHAR execCmd[1024], classname[1024];
    /* launch a document by fileclass like 'WordPad.Document.1' */
    /* the Commandline contains 'c:\Path\wordpad.exe "%1"' */
    /* FIXME: wcmd should not be of a fixed size. Fixed to 1024, MAX_PATH is way too short! */
    ULONG cmask = (psei->fMask & SEE_MASK_CLASSALL);
    DWORD resultLen;
    BOOL done;
    UINT_PTR rslt;

    /* FIXME: remove following block when SHELL_quote_and_execute supports hkeyClass parameter */
    if (cmask != SEE_MASK_CLASSNAME)
    {
        WCHAR wcmd[1024];
        HCR_GetExecuteCommandW((cmask == SEE_MASK_CLASSKEY) ? psei->hkeyClass : NULL,
                               (cmask == SEE_MASK_CLASSNAME) ? psei->lpClass : NULL,
                               psei->lpVerb,
                               execCmd, sizeof(execCmd));

        /* FIXME: get the extension of lpFile, check if it fits to the lpClass */
        TRACE("SEE_MASK_CLASSNAME->%s, doc->%s\n", debugstr_w(execCmd), debugstr_w(wszApplicationName));

        wcmd[0] = '\0';
        done = SHELL_ArgifyW(wcmd, ARRAY_SIZE(wcmd), execCmd, wszApplicationName, (LPITEMIDLIST)psei->lpIDList, psei->lpParameters,
                             &resultLen, (psei->lpDirectory && *psei->lpDirectory) ? psei->lpDirectory : NULL);
        if (!done && wszApplicationName[0])
        {
#if 0       // Given HKCR\.test=SZ:"test" and HKCR\test\shell\open\command=SZ:"cmd.exe /K echo.Hello", no filename is
            // appended on Windows when there is no %1 nor %L when executed with: shlextdbg.exe /shellexec=c:\file.test /INVOKE
            strcatW(wcmd, L" ");
            if (*wszApplicationName != '"')
            {
                strcatW(wcmd, L"\"");
                strcatW(wcmd, wszApplicationName);
                strcatW(wcmd, L"\"");
            }
            else
                strcatW(wcmd, wszApplicationName);
#endif
        }
        if (resultLen > ARRAY_SIZE(wcmd))
            ERR("Argify buffer not large enough... truncating\n");
        return execfunc(wcmd, NULL, FALSE, psei, psei_out);
    }

    strcpyW(classname, psei->lpClass);
    rslt = SHELL_FindExecutableByVerb(psei->lpVerb, NULL, classname, execCmd, sizeof(execCmd));

    TRACE("SHELL_FindExecutableByVerb returned %u (%s, %s)\n", (unsigned int)rslt, debugstr_w(classname), debugstr_w(execCmd));
    if (33 > rslt)
        return rslt;
    rslt = SHELL_quote_and_execute( execCmd, L"", classname,
                                      wszApplicationName, NULL, psei,
                                      psei_out, execfunc );
    return rslt;

}

static BOOL SHELL_translate_idlist(LPSHELLEXECUTEINFOW sei, LPWSTR wszParameters, DWORD parametersLen, LPWSTR wszApplicationName, DWORD dwApplicationNameLen)
{
    WCHAR buffer[MAX_PATH];
    BOOL appKnownSingular = FALSE;

    /* last chance to translate IDList: now also allow CLSID paths */
    if (SUCCEEDED(SHELL_GetPathFromIDListForExecuteW((LPCITEMIDLIST)sei->lpIDList, buffer, ARRAY_SIZE(buffer)))) {
        if (buffer[0] == ':' && buffer[1] == ':') {
            /* open shell folder for the specified class GUID */
            if (strlenW(buffer) + 1 > parametersLen)
                ERR("parameters len exceeds buffer size (%i > %i), truncating\n",
                    lstrlenW(buffer) + 1, parametersLen);
            lstrcpynW(wszParameters, buffer, parametersLen);
            if (strlenW(L"explorer.exe") > dwApplicationNameLen)
                ERR("application len exceeds buffer size (%i), truncating\n",
                    dwApplicationNameLen);
            lstrcpynW(wszApplicationName, L"explorer.exe", dwApplicationNameLen);
            appKnownSingular = TRUE;

            sei->fMask &= ~SEE_MASK_INVOKEIDLIST;
        } else {
            WCHAR target[max(MAX_PATH, _countof(buffer))];
            DWORD attribs;
            DWORD resultLen;
            /* Check if we're executing a directory and if so use the
               handler for the Folder class */
            strcpyW(target, buffer);
            attribs = GetFileAttributesW(buffer);
            if (attribs != INVALID_FILE_ATTRIBUTES &&
                    (attribs & FILE_ATTRIBUTE_DIRECTORY) &&
                    HCR_GetExecuteCommandW(0, L"Folder",
                                           sei->lpVerb,
                                           buffer, sizeof(buffer))) {
                SHELL_ArgifyW(wszApplicationName, dwApplicationNameLen,
                              buffer, target, (LPITEMIDLIST)sei->lpIDList, NULL, &resultLen,
                              !StrIsNullOrEmpty(sei->lpDirectory) ? sei->lpDirectory : NULL);
                if (resultLen > dwApplicationNameLen)
                    ERR("Argify buffer not large enough... truncating\n"); // FIXME: Report this to the caller?
                appKnownSingular = FALSE;
                // HACKFIX: We really want the !appKnownSingular code in SHELL_execute to split the
                // parameters for us but we cannot guarantee that the exe in the registry is quoted.
                // We have now turned 'explorer.exe "%1" into 'explorer.exe "c:\path\from\pidl"' and
                // need to split to application and parameters.
                LPCWSTR params = PathGetArgsW(wszApplicationName);
                lstrcpynW(wszParameters, params, parametersLen);
                PathRemoveArgsW(wszApplicationName);
                PathUnquoteSpacesW(wszApplicationName);
                appKnownSingular = TRUE;
            }
            sei->fMask &= ~SEE_MASK_INVOKEIDLIST;
        }
    }
    return appKnownSingular;
}

static BOOL
SHELL_InvokePidl(
    _In_ LPSHELLEXECUTEINFOW sei,
    _In_ LPCITEMIDLIST pidl)
{
    // Bind pidl
    CComPtr<IShellFolder> psfFolder;
    LPCITEMIDLIST pidlLast;
    HRESULT hr = SHBindToParent(pidl, IID_PPV_ARG(IShellFolder, &psfFolder), &pidlLast);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    // Get the context menu to invoke a command
    CComPtr<IContextMenu> pCM;
    hr = psfFolder->GetUIObjectOf(NULL, 1, &pidlLast, IID_NULL_PPV_ARG(IContextMenu, &pCM));
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    // Invoke a command
    CMINVOKECOMMANDINFO ici = { sizeof(ici) };
    ici.fMask = (sei->fMask & (SEE_MASK_NO_CONSOLE | SEE_MASK_ASYNCOK | SEE_MASK_FLAG_NO_UI));
    ici.nShow = sei->nShow;
    ici.hwnd = sei->hwnd;
    char szVerb[VERBKEY_CCHMAX];
    if (sei->lpVerb && sei->lpVerb[0])
    {
        WideCharToMultiByte(CP_ACP, 0, sei->lpVerb, -1, szVerb, _countof(szVerb), NULL, NULL);
        szVerb[_countof(szVerb) - 1] = ANSI_NULL; // Avoid buffer overrun
        ici.lpVerb = szVerb;
    }
    else // The default verb?
    {
        HMENU hMenu = CreatePopupMenu();
        const INT idCmdFirst = 1, idCmdLast = 0x7FFF;
        hr = pCM->QueryContextMenu(hMenu, 0, idCmdFirst, idCmdLast, CMF_DEFAULTONLY);
        if (FAILED_UNEXPECTEDLY(hr))
        {
            DestroyMenu(hMenu);
            return FALSE;
        }

        INT nDefaultID = GetMenuDefaultItem(hMenu, FALSE, 0);
        DestroyMenu(hMenu);
        if (nDefaultID == -1)
            nDefaultID = idCmdFirst;

        ici.lpVerb = MAKEINTRESOURCEA(nDefaultID - idCmdFirst);
    }
    hr = pCM->InvokeCommand(&ici);

    return !FAILED_UNEXPECTEDLY(hr);
}

static UINT_PTR SHELL_quote_and_execute(LPCWSTR wcmd, LPCWSTR wszParameters, LPCWSTR wszKeyname, LPCWSTR wszApplicationName, LPWSTR env, LPSHELLEXECUTEINFOW psei, LPSHELLEXECUTEINFOW psei_out, SHELL_ExecuteW32 execfunc)
{
    UINT_PTR retval;
    DWORD len;
    CHeapPtr<WCHAR, CLocalAllocator> wszQuotedCmd;

    /* Length of quotes plus length of command plus NULL terminator */
    len = 2 + lstrlenW(wcmd) + 1;
    if (wszParameters[0])
    {
        /* Length of space plus length of parameters */
        len += 1 + lstrlenW(wszParameters);
    }
    wszQuotedCmd.Allocate(len);
    /* Must quote to handle case where cmd contains spaces,
     * else security hole if malicious user creates executable file "C:\\Program"
     */
    strcpyW(wszQuotedCmd, L"\"");
    strcatW(wszQuotedCmd, wcmd);
    strcatW(wszQuotedCmd, L"\"");
    if (wszParameters[0])
    {
        strcatW(wszQuotedCmd, L" ");
        strcatW(wszQuotedCmd, wszParameters);
    }

    TRACE("%s/%s => %s/%s\n", debugstr_w(wszApplicationName), debugstr_w(psei->lpVerb), debugstr_w(wszQuotedCmd), debugstr_w(wszKeyname));

    if (*wszKeyname)
        retval = execute_from_key(wszKeyname, wszApplicationName, env, psei->lpParameters, wcmd, execfunc, psei, psei_out);
    else
        retval = execfunc(wszQuotedCmd, env, FALSE, psei, psei_out);

    return retval;
}

static UINT_PTR SHELL_execute_url(LPCWSTR lpFile, LPCWSTR wcmd, LPSHELLEXECUTEINFOW psei, LPSHELLEXECUTEINFOW psei_out, SHELL_ExecuteW32 execfunc)
{
    UINT_PTR retval;
    CHeapPtr<WCHAR, CLocalAllocator> lpstrProtocol;
    LPCWSTR lpstrRes;
    INT iSize;
    DWORD len;

    lpstrRes = strchrW(lpFile, ':');
    if (lpstrRes)
        iSize = lpstrRes - lpFile;
    else
        iSize = strlenW(lpFile);

    TRACE("Got URL: %s\n", debugstr_w(lpFile));
    /* Looking for ...<protocol>\shell\<lpVerb>\command */
    len = iSize + lstrlenW(L"\\shell\\") + lstrlenW(L"\\command") + 1;
    if (psei->lpVerb && *psei->lpVerb)
        len += lstrlenW(psei->lpVerb);
    else
        len += lstrlenW(L"open");
    lpstrProtocol.Allocate(len);
    memcpy(lpstrProtocol, lpFile, iSize * sizeof(WCHAR));
    lpstrProtocol[iSize] = '\0';
    strcatW(lpstrProtocol, L"\\shell\\");
    strcatW(lpstrProtocol, psei->lpVerb && *psei->lpVerb ? psei->lpVerb : L"open");
    strcatW(lpstrProtocol, L"\\command");

    retval = execute_from_key(lpstrProtocol, lpFile, NULL, psei->lpParameters,
                              wcmd, execfunc, psei, psei_out);

    return retval;
}

static void do_error_dialog(UINT_PTR retval, HWND hwnd, WCHAR* filename)
{
    WCHAR msg[2048];
    DWORD_PTR msgArguments[3]  = { (DWORD_PTR)filename, 0, 0 };
    DWORD error_code;

    error_code = GetLastError();
    if (retval == SE_ERR_NOASSOC)
        LoadStringW(shell32_hInstance, IDS_SHLEXEC_NOASSOC, msg, ARRAY_SIZE(msg));
    else
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       NULL,
                       error_code,
                       LANG_USER_DEFAULT,
                       msg,
                       ARRAY_SIZE(msg),
                       (va_list*)msgArguments);

    MessageBoxW(hwnd, msg, NULL, MB_ICONERROR);
}

static WCHAR *expand_environment( const WCHAR *str )
{
    CHeapPtr<WCHAR, CLocalAllocator> buf;
    DWORD len;

    len = ExpandEnvironmentStringsW(str, NULL, 0);
    if (!len) return NULL;

    if (!buf.Allocate(len))
        return NULL;

    len = ExpandEnvironmentStringsW(str, buf, len);
    if (!len)
        return NULL;

    return buf.Detach();
}

/*************************************************************************
 *    SHELL_execute [Internal]
 */
static BOOL SHELL_execute(LPSHELLEXECUTEINFOW sei, SHELL_ExecuteW32 execfunc)
{
    static const DWORD unsupportedFlags =
        SEE_MASK_ICON         | SEE_MASK_HOTKEY |
        SEE_MASK_CONNECTNETDRV | SEE_MASK_FLAG_DDEWAIT |
        SEE_MASK_ASYNCOK      | SEE_MASK_HMONITOR;

    DWORD len;
    UINT_PTR retval = SE_ERR_NOASSOC;
    BOOL appKnownSingular = FALSE;

    /* make a local copy of the LPSHELLEXECUTEINFO structure and work with this from now on */
    SHELLEXECUTEINFOW sei_tmp = *sei;

    TRACE("mask=0x%08x hwnd=%p verb=%s file=%s parm=%s dir=%s show=0x%08x class=%s\n",
          sei_tmp.fMask, sei_tmp.hwnd, debugstr_w(sei_tmp.lpVerb),
          debugstr_w(sei_tmp.lpFile), debugstr_w(sei_tmp.lpParameters),
          debugstr_w(sei_tmp.lpDirectory), sei_tmp.nShow,
          ((sei_tmp.fMask & SEE_MASK_CLASSALL) == SEE_MASK_CLASSNAME) ?
          debugstr_w(sei_tmp.lpClass) : "not used");

    sei->hProcess = NULL;

    /* make copies of all path/command strings */
    CHeapPtr<WCHAR, CLocalAllocator> wszApplicationName;
    DWORD dwApplicationNameLen = MAX_PATH + 2;
    if (!sei_tmp.lpFile)
    {
        wszApplicationName.Allocate(dwApplicationNameLen);
        *wszApplicationName = '\0';
    }
    else if (*sei_tmp.lpFile == '\"' && sei_tmp.lpFile[(len = strlenW(sei_tmp.lpFile))-1] == '\"')
    {
        if(len-1 >= dwApplicationNameLen)
            dwApplicationNameLen = len;

        wszApplicationName.Allocate(dwApplicationNameLen);
        memcpy(wszApplicationName, sei_tmp.lpFile + 1, len * sizeof(WCHAR));

        if(len > 2)
            wszApplicationName[len-2] = '\0';
        appKnownSingular = TRUE;

        TRACE("wszApplicationName=%s\n", debugstr_w(wszApplicationName));
    }
    else
    {
        DWORD l = strlenW(sei_tmp.lpFile) + 1;
        if(l > dwApplicationNameLen) dwApplicationNameLen = l + 1;
        wszApplicationName.Allocate(dwApplicationNameLen);
        memcpy(wszApplicationName, sei_tmp.lpFile, l * sizeof(WCHAR));

        if (wszApplicationName[2] == 0 && wszApplicationName[1] == L':' &&
            ((L'A' <= wszApplicationName[0] && wszApplicationName[0] <= L'Z') ||
             (L'a' <= wszApplicationName[0] && wszApplicationName[0] <= L'z')))
        {
            // 'C:' --> 'C:\'
            PathAddBackslashW(wszApplicationName);
        }
    }

    WCHAR parametersBuffer[1024];
    LPWSTR wszParameters = parametersBuffer;
    CHeapPtr<WCHAR, CLocalAllocator> wszParamAlloc;
    DWORD parametersLen = _countof(parametersBuffer);

    if (sei_tmp.lpParameters)
    {
        len = lstrlenW(sei_tmp.lpParameters) + 1;
        if (len > parametersLen)
        {
            wszParamAlloc.Allocate(len);
            wszParameters = wszParamAlloc;
            parametersLen = len;
        }
        strcpyW(wszParameters, sei_tmp.lpParameters);
    }
    else
        *wszParameters = L'\0';

    // Get the working directory
    WCHAR dirBuffer[MAX_PATH];
    LPWSTR wszDir = dirBuffer;
    wszDir[0] = UNICODE_NULL;
    CHeapPtr<WCHAR, CLocalAllocator> wszDirAlloc;
    if (sei_tmp.lpDirectory && *sei_tmp.lpDirectory)
    {
        if (sei_tmp.fMask & SEE_MASK_DOENVSUBST)
        {
            LPWSTR tmp = expand_environment(sei_tmp.lpDirectory);
            if (tmp)
            {
                wszDirAlloc.Attach(tmp);
                wszDir = wszDirAlloc;
            }
        }
        else
        {
            __SHCloneStrW(&wszDirAlloc, sei_tmp.lpDirectory);
            if (wszDirAlloc)
                wszDir = wszDirAlloc;
        }
    }
    if (!wszDir[0])
    {
        ::GetCurrentDirectoryW(_countof(dirBuffer), dirBuffer);
        wszDir = dirBuffer;
    }
    // NOTE: ShellExecute should accept the invalid working directory for historical reason.
    if (!PathIsDirectoryW(wszDir))
    {
        INT iDrive = PathGetDriveNumberW(wszDir);
        if (iDrive >= 0)
        {
            PathStripToRootW(wszDir);
            if (!PathIsDirectoryW(wszDir))
            {
                ::GetWindowsDirectoryW(dirBuffer, _countof(dirBuffer));
                wszDir = dirBuffer;
            }
        }
    }

    /* adjust string pointers to point to the new buffers */
    sei_tmp.lpFile = wszApplicationName;
    sei_tmp.lpParameters = wszParameters;
    sei_tmp.lpDirectory = wszDir;

    if (sei_tmp.fMask & unsupportedFlags)
    {
        FIXME("flags ignored: 0x%08x\n", sei_tmp.fMask & unsupportedFlags);
    }

    /* process the IDList */
    if (sei_tmp.fMask & SEE_MASK_IDLIST &&
        (sei_tmp.fMask & SEE_MASK_INVOKEIDLIST) != SEE_MASK_INVOKEIDLIST)
    {
        CComPtr<IShellExecuteHookW> pSEH;

        HRESULT hr = SHBindToParent((LPCITEMIDLIST)sei_tmp.lpIDList, IID_PPV_ARG(IShellExecuteHookW, &pSEH), NULL);

        if (SUCCEEDED(hr))
        {
            hr = pSEH->Execute(&sei_tmp);
            if (hr == S_OK)
                return TRUE;
        }

        SHGetPathFromIDListW((LPCITEMIDLIST)sei_tmp.lpIDList, wszApplicationName);
        appKnownSingular = TRUE;
        TRACE("-- idlist=%p (%s)\n", sei_tmp.lpIDList, debugstr_w(wszApplicationName));
    }

    if ((sei_tmp.fMask & SEE_MASK_DOENVSUBST) && !StrIsNullOrEmpty(sei_tmp.lpFile))
    {
        WCHAR *tmp = expand_environment(sei_tmp.lpFile);
        if (tmp)
        {
            wszApplicationName.Attach(tmp);
            sei_tmp.lpFile = wszApplicationName;
        }
    }

    if ((sei_tmp.fMask & SEE_MASK_INVOKEIDLIST) == SEE_MASK_INVOKEIDLIST)
    {
        HRESULT hr = ShellExecute_ContextMenuVerb(&sei_tmp);
        if (SUCCEEDED(hr))
        {
            sei->hInstApp = (HINSTANCE)42;
            return TRUE;
        }
    }

    if (ERROR_SUCCESS == ShellExecute_FromContextMenuHandlers(&sei_tmp))
    {
        sei->hInstApp = (HINSTANCE) 33;
        return TRUE;
    }

    if (sei_tmp.fMask & SEE_MASK_CLASSALL)
    {
        retval = SHELL_execute_class(wszApplicationName, &sei_tmp, sei, execfunc);
        if (retval <= 32 && !(sei_tmp.fMask & SEE_MASK_FLAG_NO_UI))
        {
            OPENASINFO Info;

            //FIXME
            // need full path

            Info.pcszFile = wszApplicationName;
            Info.pcszClass = NULL;
            Info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_EXEC;

            //if (SHOpenWithDialog(sei_tmp.hwnd, &Info) != S_OK)
            DBG_UNREFERENCED_LOCAL_VARIABLE(Info);
            do_error_dialog(retval, sei_tmp.hwnd, wszApplicationName);
        }
        return retval > 32;
    }

    if (!(sei_tmp.fMask & SEE_MASK_IDLIST) && // Not an ID List
        (StrCmpNIW(sei_tmp.lpFile, L"shell:", 6) == 0 ||
         StrCmpNW(sei_tmp.lpFile, L"::{", 3) == 0))
    {
        CComHeapPtr<ITEMIDLIST> pidlParsed;
        HRESULT hr = SHParseDisplayName(sei_tmp.lpFile, NULL, &pidlParsed, 0, NULL);
        if (SUCCEEDED(hr) && SHELL_InvokePidl(&sei_tmp, pidlParsed))
        {
            sei_tmp.hInstApp = (HINSTANCE)UlongToHandle(42);
            return TRUE;
        }
    }

    /* Has the IDList not yet been translated? */
    if (sei_tmp.fMask & SEE_MASK_IDLIST)
    {
        appKnownSingular = SHELL_translate_idlist( &sei_tmp, wszParameters,
                           parametersLen,
                           wszApplicationName,
                           dwApplicationNameLen );
    }

    /* convert file URLs */
    if (UrlIsFileUrlW(sei_tmp.lpFile))
    {
        CHeapPtr<WCHAR, CLocalAllocator> buf;
        DWORD size = MAX_PATH;
        if (!buf.Allocate(size) || FAILED(PathCreateFromUrlW(sei_tmp.lpFile, buf, &size, 0)))
            return SE_ERR_OOM;

        wszApplicationName.Attach(buf.Detach());
        sei_tmp.lpFile = wszApplicationName;
    }

    /* Else, try to execute the filename */
    TRACE("execute: %s,%s,%s\n", debugstr_w(wszApplicationName), debugstr_w(wszParameters), debugstr_w(wszDir));

    /* separate out command line arguments from executable file name */
    LPCWSTR lpFile = sei_tmp.lpFile;
    if (!*sei_tmp.lpParameters && !appKnownSingular)
    {
        /* If the executable path is quoted, handle the rest of the command line as parameters. */
        if (sei_tmp.lpFile[0] == L'"')
        {
            LPWSTR pszArgs = PathGetArgsW(wszApplicationName);
            PathRemoveArgsW(wszApplicationName);
            PathUnquoteSpacesW(wszApplicationName);
            parametersLen = lstrlenW(pszArgs);
            if (parametersLen < _countof(parametersBuffer))
            {
                StringCchCopyW(parametersBuffer, _countof(parametersBuffer), pszArgs);
                wszParameters = parametersBuffer;
            }
            else
            {
                wszParamAlloc.Attach(StrDupW(pszArgs));
                wszParameters = wszParamAlloc;
            }
        }
        /* We have to test sei instead of sei_tmp because sei_tmp had its
         * input fMask modified above in SHELL_translate_idlist.
         * This code is needed to handle the case where we only have an
         * lpIDList with multiple CLSID/PIDL's (not 'My Computer' only) */
        else if ((sei->fMask & SEE_MASK_IDLIST) == SEE_MASK_IDLIST)
        {
            WCHAR buffer[MAX_PATH], xlpFile[MAX_PATH];
            LPWSTR space, s;

            LPWSTR beg = wszApplicationName;
            for(s = beg; (space = const_cast<LPWSTR>(strchrW(s, L' '))); s = space + 1)
            {
                int idx = space - sei_tmp.lpFile;
                memcpy(buffer, sei_tmp.lpFile, idx * sizeof(WCHAR));
                buffer[idx] = '\0';

                if (SearchPathW(*sei_tmp.lpDirectory ? sei_tmp.lpDirectory : NULL,
                    buffer, L".exe", _countof(xlpFile), xlpFile, NULL))
                {
                    /* separate out command from parameter string */
                    LPCWSTR p = space + 1;

                    while(isspaceW(*p))
                        ++p;

                    strcpyW(wszParameters, p);
                    *space = L'\0';

                    break;
                }
            }
        }
    }

    WCHAR wcmdBuffer[1024];
    LPWSTR wcmd = wcmdBuffer;
    DWORD wcmdLen = _countof(wcmdBuffer);
    CHeapPtr<WCHAR, CLocalAllocator> wcmdAlloc;

    /* Only execute if it has an executable extension */
    if (PathIsExeW(lpFile))
    {
        len = lstrlenW(wszApplicationName) + 3;
        if (sei_tmp.lpParameters[0])
            len += 1 + lstrlenW(wszParameters);
        if (len > wcmdLen)
        {
            wcmdAlloc.Allocate(len);
            wcmd = wcmdAlloc;
            wcmdLen = len;
        }
        swprintf(wcmd, L"\"%s\"", (LPWSTR)wszApplicationName);
        if (sei_tmp.lpParameters[0])
        {
            strcatW(wcmd, L" ");
            strcatW(wcmd, wszParameters);
        }

        retval = execfunc(wcmd, NULL, FALSE, &sei_tmp, sei);
        if (retval > 32)
            return TRUE;
    }

    /* Else, try to find the executable */
    WCHAR wszKeyname[256];
    CHeapPtr<WCHAR, CLocalAllocator> env;
    wcmd[0] = UNICODE_NULL;
    retval = SHELL_FindExecutable(sei_tmp.lpDirectory, lpFile, sei_tmp.lpVerb, wcmd, wcmdLen, wszKeyname, &env, (LPITEMIDLIST)sei_tmp.lpIDList, sei_tmp.lpParameters);
    if (retval > 32)  /* Found */
    {
        retval = SHELL_quote_and_execute(wcmd, wszParameters, wszKeyname,
                                         wszApplicationName, env, &sei_tmp,
                                         sei, execfunc);
    }
    else if (PathIsDirectoryW(lpFile))
    {
        WCHAR wExec[MAX_PATH];
        CHeapPtr<WCHAR, CLocalAllocator> lpQuotedFile;
        if (lpQuotedFile.Allocate(strlenW(lpFile) + 3))
        {
            retval = SHELL_FindExecutable(sei_tmp.lpDirectory, L"explorer",
                                          L"open", wExec, MAX_PATH,
                                          NULL, &env, NULL, NULL);
            if (retval > 32)
            {
                swprintf(lpQuotedFile, L"\"%s\"", lpFile);
                retval = SHELL_quote_and_execute(wExec, lpQuotedFile,
                                                 wszKeyname,
                                                 wszApplicationName, env,
                                                 &sei_tmp, sei, execfunc);
            }
        }
        else
            retval = 0; /* Out of memory */
    }
    else if (PathIsURLW(lpFile))    /* File not found, check for URL */
    {
        retval = SHELL_execute_url(lpFile, wcmd, &sei_tmp, sei, execfunc );
    }
    /* Check if file specified is in the form www.??????.*** */
    else if (!strncmpiW(lpFile, L"www", 3))
    {
        /* if so, prefix lpFile with http:// and call ShellExecute */
        WCHAR lpstrTmpFile[256];
        strcpyW(lpstrTmpFile, L"http://");
        strcatW(lpstrTmpFile, lpFile);
        retval = (UINT_PTR)ShellExecuteW(sei_tmp.hwnd, sei_tmp.lpVerb, lpstrTmpFile, NULL, NULL, 0);
    }

    TRACE("retval %lu\n", retval);

    if (retval <= 32 && !(sei_tmp.fMask & SEE_MASK_FLAG_NO_UI))
    {
        OPENASINFO Info;

        //FIXME
        // need full path

        Info.pcszFile = wszApplicationName;
        Info.pcszClass = NULL;
        Info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_EXEC;

        //if (SHOpenWithDialog(sei_tmp.hwnd, &Info) != S_OK)
        DBG_UNREFERENCED_LOCAL_VARIABLE(Info);
        do_error_dialog(retval, sei_tmp.hwnd, wszApplicationName);
    }

    sei->hInstApp = (HINSTANCE)(retval > 32 ? 33 : retval);

    return retval > 32;
}

/*************************************************************************
 * ShellExecuteA            [SHELL32.290]
 */
HINSTANCE WINAPI ShellExecuteA(HWND hWnd, LPCSTR lpVerb, LPCSTR lpFile,
                               LPCSTR lpParameters, LPCSTR lpDirectory, INT iShowCmd)
{
    SHELLEXECUTEINFOA sei;

    TRACE("%p,%s,%s,%s,%s,%d\n",
          hWnd, debugstr_a(lpVerb), debugstr_a(lpFile),
          debugstr_a(lpParameters), debugstr_a(lpDirectory), iShowCmd);

    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_FLAG_NO_UI;
    sei.hwnd = hWnd;
    sei.lpVerb = lpVerb;
    sei.lpFile = lpFile;
    sei.lpParameters = lpParameters;
    sei.lpDirectory = lpDirectory;
    sei.nShow = iShowCmd;
    sei.lpIDList = 0;
    sei.lpClass = 0;
    sei.hkeyClass = 0;
    sei.dwHotKey = 0;
    sei.hProcess = 0;

    ShellExecuteExA(&sei);
    return sei.hInstApp;
}

static DWORD
ShellExecute_Normal(_Inout_ LPSHELLEXECUTEINFOW sei)
{
    // FIXME
    return SHELL_execute(sei, SHELL_ExecuteW) ? ERROR_SUCCESS : ERROR_FILE_NOT_FOUND;
}

static VOID
ShellExecute_ShowError(
    _In_ const SHELLEXECUTEINFOW *ExecInfo,
    _In_opt_ LPCWSTR pszCaption,
    _In_ DWORD dwError)
{
    // FIXME: Show error message
}

/*************************************************************************
 * ShellExecuteExA                [SHELL32.292]
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
ShellExecuteExA(LPSHELLEXECUTEINFOA sei)
{
    SHELLEXECUTEINFOW seiW;
    BOOL ret;
    WCHAR *wVerb = NULL, *wFile = NULL, *wParameters = NULL, *wDirectory = NULL, *wClass = NULL;

    TRACE("%p\n", sei);

    if (sei->cbSize != sizeof(SHELLEXECUTEINFOA))
    {
        sei->hInstApp = (HINSTANCE)ERROR_ACCESS_DENIED;
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    memcpy(&seiW, sei, sizeof(SHELLEXECUTEINFOW));

    seiW.cbSize = sizeof(SHELLEXECUTEINFOW);

    if (sei->lpVerb)
        seiW.lpVerb = __SHCloneStrAtoW(&wVerb, sei->lpVerb);

    if (sei->lpFile)
        seiW.lpFile = __SHCloneStrAtoW(&wFile, sei->lpFile);

    if (sei->lpParameters)
        seiW.lpParameters = __SHCloneStrAtoW(&wParameters, sei->lpParameters);

    if (sei->lpDirectory)
        seiW.lpDirectory = __SHCloneStrAtoW(&wDirectory, sei->lpDirectory);

    if ((sei->fMask & SEE_MASK_CLASSALL) == SEE_MASK_CLASSNAME && sei->lpClass)
        seiW.lpClass = __SHCloneStrAtoW(&wClass, sei->lpClass);
    else
        seiW.lpClass = NULL;

    ret = ShellExecuteExW(&seiW);

    sei->hInstApp = seiW.hInstApp;

    if (sei->fMask & SEE_MASK_NOCLOSEPROCESS)
        sei->hProcess = seiW.hProcess;

    SHFree(wVerb);
    SHFree(wFile);
    SHFree(wParameters);
    SHFree(wDirectory);
    SHFree(wClass);

    return ret;
}

/*************************************************************************
 * ShellExecuteExW                [SHELL32.293]
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
ShellExecuteExW(LPSHELLEXECUTEINFOW sei)
{
    HRESULT hrCoInit;
    DWORD dwError;
    ULONG fOldMask;

    if (sei->cbSize != sizeof(SHELLEXECUTEINFOW))
    {
        sei->hInstApp = (HINSTANCE)UlongToHandle(SE_ERR_ACCESSDENIED);
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    hrCoInit = SHCoInitializeAnyApartment();

    if (SHRegGetBoolUSValueW(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer",
                             L"MaximizeApps", FALSE, FALSE))
    {
        switch (sei->nShow)
        {
            case SW_SHOW:
            case SW_SHOWDEFAULT:
            case SW_SHOWNORMAL:
            case SW_RESTORE:
                sei->nShow = SW_SHOWMAXIMIZED;
                break;
            default:
                break;
        }
    }

    fOldMask = sei->fMask;

    if (!(fOldMask & SEE_MASK_NOASYNC) && SHELL_InRunDllProcess())
        sei->fMask |= SEE_MASK_WAITFORINPUTIDLE | SEE_MASK_NOASYNC;

    dwError = ShellExecute_Normal(sei);

    if (dwError && dwError != ERROR_DLL_NOT_FOUND && dwError != ERROR_CANCELLED)
        ShellExecute_ShowError(sei, NULL, dwError);

    sei->fMask = fOldMask;

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();

    if (dwError)
        SetLastError(dwError);

    return dwError == ERROR_SUCCESS;
}

/*************************************************************************
 * ShellExecuteW            [SHELL32.294]
 * from shellapi.h
 * WINSHELLAPI HINSTANCE APIENTRY ShellExecuteW(HWND hwnd, LPCWSTR lpVerb,
 * LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
 */
HINSTANCE WINAPI ShellExecuteW(HWND hwnd, LPCWSTR lpVerb, LPCWSTR lpFile,
                               LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
    SHELLEXECUTEINFOW sei;

    TRACE("\n");
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_FLAG_NO_UI;
    sei.hwnd = hwnd;
    sei.lpVerb = lpVerb;
    sei.lpFile = lpFile;
    sei.lpParameters = lpParameters;
    sei.lpDirectory = lpDirectory;
    sei.nShow = nShowCmd;
    sei.lpIDList = 0;
    sei.lpClass = 0;
    sei.hkeyClass = 0;
    sei.dwHotKey = 0;
    sei.hProcess = 0;

    SHELL_execute(&sei, SHELL_ExecuteW);
    return sei.hInstApp;
}

/*************************************************************************
 * WOWShellExecute            [SHELL32.@]
 *
 * FIXME: the callback function most likely doesn't work the same way on Windows.
 */
EXTERN_C HINSTANCE WINAPI WOWShellExecute(HWND hWnd, LPCSTR lpVerb, LPCSTR lpFile,
        LPCSTR lpParameters, LPCSTR lpDirectory, INT iShowCmd, void *callback)
{
    SHELLEXECUTEINFOW seiW;
    WCHAR *wVerb = NULL, *wFile = NULL, *wParameters = NULL, *wDirectory = NULL;
    HANDLE hProcess = 0;

    seiW.lpVerb = lpVerb ? __SHCloneStrAtoW(&wVerb, lpVerb) : NULL;
    seiW.lpFile = lpFile ? __SHCloneStrAtoW(&wFile, lpFile) : NULL;
    seiW.lpParameters = lpParameters ? __SHCloneStrAtoW(&wParameters, lpParameters) : NULL;
    seiW.lpDirectory = lpDirectory ? __SHCloneStrAtoW(&wDirectory, lpDirectory) : NULL;

    seiW.cbSize = sizeof(seiW);
    seiW.fMask = 0;
    seiW.hwnd = hWnd;
    seiW.nShow = iShowCmd;
    seiW.lpIDList = 0;
    seiW.lpClass = 0;
    seiW.hkeyClass = 0;
    seiW.dwHotKey = 0;
    seiW.hProcess = hProcess;

    SHELL_execute(&seiW, (SHELL_ExecuteW32)callback);

    SHFree(wVerb);
    SHFree(wFile);
    SHFree(wParameters);
    SHFree(wDirectory);
    return seiW.hInstApp;
}

/*************************************************************************
 * OpenAs_RunDLLW          [SHELL32.@]
 */
EXTERN_C void WINAPI
OpenAs_RunDLLW(HWND hwnd, HINSTANCE hinst, LPCWSTR cmdline, int cmdshow)
{
    OPENASINFO info;
    TRACE("%p, %p, %s, %d\n", hwnd, hinst, debugstr_w(cmdline), cmdshow);

    ZeroMemory(&info, sizeof(info));
    info.pcszFile = cmdline;
    info.pcszClass = NULL;
    info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_REGISTER_EXT | OAIF_EXEC;

    SHOpenWithDialog(hwnd, &info);
}

/*************************************************************************
 * OpenAs_RunDLLA          [SHELL32.@]
 */
EXTERN_C void WINAPI
OpenAs_RunDLLA(HWND hwnd, HINSTANCE hinst, LPCSTR cmdline, int cmdshow)
{
    LPWSTR pszCmdLineW = NULL;
    TRACE("%p, %p, %s, %d\n", hwnd, hinst, debugstr_a(cmdline), cmdshow);

    if (cmdline)
        __SHCloneStrAtoW(&pszCmdLineW, cmdline);
    OpenAs_RunDLLW(hwnd, hinst, pszCmdLineW, cmdshow);
    SHFree(pszCmdLineW);
}

/*************************************************************************/

static LPCWSTR
SplitParams(LPCWSTR psz, LPWSTR pszArg0, size_t cchArg0)
{
    LPCWSTR pch;
    size_t ich = 0;
    if (*psz == L'"')
    {
        // 1st argument is quoted. the string in quotes is quoted 1st argument.
        // [pch] --> [pszArg0+ich]
        for (pch = psz + 1; *pch && ich + 1 < cchArg0; ++ich, ++pch)
        {
            if (*pch == L'"' && pch[1] == L'"')
            {
                // doubled double quotations found!
                pszArg0[ich] = L'"';
            }
            else if (*pch == L'"')
            {
                // single double quotation found!
                ++pch;
                break;
            }
            else
            {
                // otherwise
                pszArg0[ich] = *pch;
            }
        }
    }
    else
    {
        // 1st argument is unquoted. non-space sequence is 1st argument.
        // [pch] --> [pszArg0+ich]
        for (pch = psz; *pch && !iswspace(*pch) && ich + 1 < cchArg0; ++ich, ++pch)
        {
            pszArg0[ich] = *pch;
        }
    }
    pszArg0[ich] = 0;

    // skip space
    while (iswspace(*pch))
        ++pch;

    return pch;
}

HRESULT WINAPI ShellExecCmdLine(
    HWND hwnd,
    LPCWSTR pwszCommand,
    LPCWSTR pwszStartDir,
    int nShow,
    LPVOID pUnused,
    DWORD dwSeclFlags)
{
    SHELLEXECUTEINFOW info;
    DWORD dwSize, dwError, dwType, dwFlags = SEE_MASK_DOENVSUBST | SEE_MASK_NOASYNC;
    LPCWSTR pszVerb = NULL;
    WCHAR szFile[MAX_PATH], szFile2[MAX_PATH];
    HRESULT hr;
    LPCWSTR pchParams;
    LPWSTR lpCommand = NULL;

    if (pwszCommand == NULL)
        RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE,
                       1, (ULONG_PTR*)pwszCommand);

    __SHCloneStrW(&lpCommand, pwszCommand);
    StrTrimW(lpCommand, L" \t");

    if (dwSeclFlags & SECL_NO_UI)
        dwFlags |= SEE_MASK_FLAG_NO_UI;
    if (dwSeclFlags & SECL_LOG_USAGE)
        dwFlags |= SEE_MASK_FLAG_LOG_USAGE;
    if (dwSeclFlags & SECL_USE_IDLIST)
        dwFlags |= SEE_MASK_INVOKEIDLIST;

    if (dwSeclFlags & SECL_RUNAS)
    {
        dwSize = 0;
        hr = AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_COMMAND, lpCommand, L"RunAs", NULL, &dwSize);
        if (SUCCEEDED(hr) && dwSize != 0)
        {
            pszVerb = L"runas";
        }
    }

    if (PathIsURLW(lpCommand) || UrlIsW(lpCommand, URLIS_APPLIABLE))
    {
        StringCchCopyW(szFile, _countof(szFile), lpCommand);
        pchParams = NULL;
    }
    else
    {
        PCWSTR apPathList[2];

        pchParams = SplitParams(lpCommand, szFile, _countof(szFile));
        if (szFile[0] != UNICODE_NULL && szFile[1] == L':' &&
            szFile[2] == UNICODE_NULL)
        {
            PathAddBackslashW(szFile);
        }

        WCHAR szCurDir[MAX_PATH];
        GetCurrentDirectoryW(_countof(szCurDir), szCurDir);
        if (pwszStartDir)
        {
            SetCurrentDirectoryW(pwszStartDir);
        }

        if ((PathIsRelativeW(szFile) &&
             GetFullPathNameW(szFile, _countof(szFile2), szFile2, NULL) &&
             PathFileExistsW(szFile2)) ||
            SearchPathW(NULL, szFile, NULL, _countof(szFile2), szFile2, NULL))
        {
            StringCchCopyW(szFile, _countof(szFile), szFile2);
        }

        apPathList[0] = pwszStartDir;
        apPathList[1] = NULL;
        PathFindOnPathExW(szFile, apPathList, WHICH_DEFAULT);

        if (!(dwSeclFlags & SECL_ALLOW_NONEXE))
        {
            if (!GetBinaryTypeW(szFile, &dwType))
            {
                SHFree(lpCommand);

                if (!(dwSeclFlags & SECL_NO_UI))
                {
                    WCHAR szText[128 + MAX_PATH], szFormat[128];
                    LoadStringW(shell32_hInstance, IDS_FILE_NOT_FOUND, szFormat, _countof(szFormat));
                    StringCchPrintfW(szText, _countof(szText), szFormat, szFile);
                    MessageBoxW(hwnd, szText, NULL, MB_ICONERROR);
                }
                return CO_E_APPNOTFOUND;
            }
        }
        else
        {
            if (GetFileAttributesW(szFile) == INVALID_FILE_ATTRIBUTES)
            {
                SHFree(lpCommand);

                if (!(dwSeclFlags & SECL_NO_UI))
                {
                    WCHAR szText[128 + MAX_PATH], szFormat[128];
                    LoadStringW(shell32_hInstance, IDS_FILE_NOT_FOUND, szFormat, _countof(szFormat));
                    StringCchPrintfW(szText, _countof(szText), szFormat, szFile);
                    MessageBoxW(hwnd, szText, NULL, MB_ICONERROR);
                }
                return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
        }
    }

    ZeroMemory(&info, sizeof(info));
    info.cbSize = sizeof(info);
    info.fMask = dwFlags;
    info.hwnd = hwnd;
    info.lpVerb = pszVerb;
    info.lpFile = szFile;
    info.lpParameters = (pchParams && *pchParams) ? pchParams : NULL;
    info.lpDirectory = pwszStartDir;
    info.nShow = nShow;
    if (ShellExecuteExW(&info))
    {
        if (info.lpIDList)
            CoTaskMemFree(info.lpIDList);

        SHFree(lpCommand);

        return S_OK;
    }

    dwError = GetLastError();

    SHFree(lpCommand);

    return HRESULT_FROM_WIN32(dwError);
}

/*************************************************************************
 *                RealShellExecuteExA (SHELL32.266)
 */
EXTERN_C
HINSTANCE WINAPI
RealShellExecuteExA(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCSTR lpOperation,
    _In_opt_ LPCSTR lpFile,
    _In_opt_ LPCSTR lpParameters,
    _In_opt_ LPCSTR lpDirectory,
    _In_opt_ LPSTR lpReturn,
    _In_opt_ LPCSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess,
    _In_ DWORD dwFlags)
{
    SHELLEXECUTEINFOA ExecInfo;

    TRACE("(%p, %s, %s, %s, %s, %p, %s, %p, %u, %p, %lu)\n",
          hwnd, debugstr_a(lpOperation), debugstr_a(lpFile), debugstr_a(lpParameters),
          debugstr_a(lpDirectory), lpReserved, debugstr_a(lpTitle),
          lpReserved, nCmdShow, lphProcess, dwFlags);

    ZeroMemory(&ExecInfo, sizeof(ExecInfo));
    ExecInfo.cbSize = sizeof(ExecInfo);
    ExecInfo.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_UNKNOWN_0x1000;
    ExecInfo.hwnd = hwnd;
    ExecInfo.lpVerb = lpOperation;
    ExecInfo.lpFile = lpFile;
    ExecInfo.lpParameters = lpParameters;
    ExecInfo.lpDirectory = lpDirectory;
    ExecInfo.nShow = (WORD)nCmdShow;

    if (lpReserved)
    {
        ExecInfo.fMask |= SEE_MASK_USE_RESERVED;
        ExecInfo.hInstApp = (HINSTANCE)lpReserved;
    }

    if (lpTitle)
    {
        ExecInfo.fMask |= SEE_MASK_HASTITLE;
        ExecInfo.lpClass = lpTitle;
    }

    if (dwFlags & 1)
        ExecInfo.fMask |= SEE_MASK_FLAG_SEPVDM;

    if (dwFlags & 2)
        ExecInfo.fMask |= SEE_MASK_NO_CONSOLE;

    if (lphProcess == NULL)
    {
        ShellExecuteExA(&ExecInfo);
    }
    else
    {
        ExecInfo.fMask |= SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteExA(&ExecInfo);
        *lphProcess = ExecInfo.hProcess;
    }

    return ExecInfo.hInstApp;
}

/*************************************************************************
 *                RealShellExecuteExW (SHELL32.267)
 */
EXTERN_C
HINSTANCE WINAPI
RealShellExecuteExW(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCWSTR lpOperation,
    _In_opt_ LPCWSTR lpFile,
    _In_opt_ LPCWSTR lpParameters,
    _In_opt_ LPCWSTR lpDirectory,
    _In_opt_ LPWSTR lpReturn,
    _In_opt_ LPCWSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess,
    _In_ DWORD dwFlags)
{
    SHELLEXECUTEINFOW ExecInfo;

    TRACE("(%p, %s, %s, %s, %s, %p, %s, %p, %u, %p, %lu)\n",
          hwnd, debugstr_w(lpOperation), debugstr_w(lpFile), debugstr_w(lpParameters),
          debugstr_w(lpDirectory), lpReserved, debugstr_w(lpTitle),
          lpReserved, nCmdShow, lphProcess, dwFlags);

    ZeroMemory(&ExecInfo, sizeof(ExecInfo));
    ExecInfo.cbSize = sizeof(ExecInfo);
    ExecInfo.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_UNKNOWN_0x1000;
    ExecInfo.hwnd = hwnd;
    ExecInfo.lpVerb = lpOperation;
    ExecInfo.lpFile = lpFile;
    ExecInfo.lpParameters = lpParameters;
    ExecInfo.lpDirectory = lpDirectory;
    ExecInfo.nShow = (WORD)nCmdShow;

    if (lpReserved)
    {
        ExecInfo.fMask |= SEE_MASK_USE_RESERVED;
        ExecInfo.hInstApp = (HINSTANCE)lpReserved;
    }

    if (lpTitle)
    {
        ExecInfo.fMask |= SEE_MASK_HASTITLE;
        ExecInfo.lpClass = lpTitle;
    }

    if (dwFlags & 1)
        ExecInfo.fMask |= SEE_MASK_FLAG_SEPVDM;

    if (dwFlags & 2)
        ExecInfo.fMask |= SEE_MASK_NO_CONSOLE;

    if (lphProcess == NULL)
    {
        ShellExecuteExW(&ExecInfo);
    }
    else
    {
        ExecInfo.fMask |= SEE_MASK_NOCLOSEPROCESS;
        ShellExecuteExW(&ExecInfo);
        *lphProcess = ExecInfo.hProcess;
    }

    return ExecInfo.hInstApp;
}

/*************************************************************************
 *                RealShellExecuteA (SHELL32.265)
 */
EXTERN_C
HINSTANCE WINAPI
RealShellExecuteA(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCSTR lpOperation,
    _In_opt_ LPCSTR lpFile,
    _In_opt_ LPCSTR lpParameters,
    _In_opt_ LPCSTR lpDirectory,
    _In_opt_ LPSTR lpReturn,
    _In_opt_ LPCSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess)
{
    return RealShellExecuteExA(hwnd,
                               lpOperation,
                               lpFile,
                               lpParameters,
                               lpDirectory,
                               lpReturn,
                               lpTitle,
                               lpReserved,
                               nCmdShow,
                               lphProcess,
                               0);
}

/*************************************************************************
 *                RealShellExecuteW (SHELL32.268)
 */
EXTERN_C
HINSTANCE WINAPI
RealShellExecuteW(
    _In_opt_ HWND hwnd,
    _In_opt_ LPCWSTR lpOperation,
    _In_opt_ LPCWSTR lpFile,
    _In_opt_ LPCWSTR lpParameters,
    _In_opt_ LPCWSTR lpDirectory,
    _In_opt_ LPWSTR lpReturn,
    _In_opt_ LPCWSTR lpTitle,
    _In_opt_ LPVOID lpReserved,
    _In_ INT nCmdShow,
    _Out_opt_ PHANDLE lphProcess)
{
    return RealShellExecuteExW(hwnd,
                               lpOperation,
                               lpFile,
                               lpParameters,
                               lpDirectory,
                               lpReturn,
                               lpTitle,
                               lpReserved,
                               nCmdShow,
                               lphProcess,
                               0);
}
