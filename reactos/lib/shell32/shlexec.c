/*
 * 				Shell Library Functions
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 2002 Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "wownt32.h"
#include "heap.h"
#include "shellapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "ddeml.h"

#include "wine/winbase16.h"
#include "shell32_main.h"
#include "undocshell.h"
#include "pidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(exec);

/***********************************************************************
 * this function is supposed to expand the escape sequences found in the registry
 * some diving reported that the following were used:
 * + %1, %2...  seem to report to parameter of index N in ShellExecute pmts
 *	%1 file
 *	%2 printer
 *	%3 driver
 *	%4 port
 * %I address of a global item ID (explorer switch /idlist)
 * %L seems to be %1 as long filename followed by the 8+3 variation
 * %S ???
 * %* all following parameters (see batfile)
 */
static BOOL argifyA(char* out, int len, const char* fmt, const char* lpFile, LPITEMIDLIST pidl, LPCSTR args)
{
    char    xlpFile[1024];
    BOOL    done = FALSE;
    LPVOID  pv;
    char    *res = out;
    const char *cmd;

    while (*fmt)
    {
        if (*fmt == '%')
        {
            switch (*++fmt)
            {
              case '\0':
              case '%':
                *res++ = '%';
                break;

              case '2':
              case '3':
	      case '4':
	      case '5':
              case '6':
              case '7':
	      case '8':
	      case '9':
	      case '0':
              case '*':
		if (args)
		{
		    if (*fmt == '*')
		    {
			*res++ = '"';
			while(*args)
			    *res++ = *args++;
			*res++ = '"';
		    }
		    else
		    {
			*res++ = '"';
			while(*args && !isspace(*args))
			    *res++ = *args++;
			*res++ = '"';

			while(isspace(*args))
			    ++args;
		    }
		}
		else
		{
              case '1':
		    if (!done || (*fmt == '1'))
		    {
			/*FIXME Is SearchPath() really needed? We already have separated out the parameter string in args. */
			if (SearchPathA(NULL, lpFile, ".exe", sizeof(xlpFile), xlpFile, NULL))
			    cmd = xlpFile;
			else
			    cmd = lpFile;

			/* Add double quotation marks unless we already have them (e.g.: "%1" %* for exefile) */
			if (res != out && *(res - 1) == '"')
			{
			    strcpy(res, cmd);
			    res += strlen(cmd);
			}
			else
			{
			    *res++ = '"';
			    strcpy(res, cmd);
			    res += strlen(cmd);
			    *res++ = '"';
			}
		    }
		}
                break;

              /*
               * IE uses this alot for activating things such as windows media
               * player. This is not verified to be fully correct but it appears
               * to work just fine.
               */
              case 'l':
              case 'L':
		if (lpFile) {
		    strcpy(res, lpFile);
		    res += strlen(lpFile);
		}
                break;

              case 'i':
              case 'I':
		if (pidl) {
		    HGLOBAL hmem = SHAllocShared(pidl, ILGetSize(pidl), 0);
		    pv = SHLockShared(hmem, 0);
		    res += sprintf(res, ":%p", pv);
		    SHUnlockShared(pv);
		}
                break;

            default: FIXME("Unknown escape sequence %%%c\n", *fmt);
            }

            fmt++;
            done = TRUE;
        }
        else
            *res++ = *fmt++;
    }

    *res = '\0';

    return done;
}

/*************************************************************************
 *	SHELL_ResolveShortCutW [Internal]
 */
static HRESULT SHELL_ResolveShortCutW(LPWSTR wcmd, LPWSTR args, LPWSTR wdir, HWND hwnd, LPCWSTR lpVerb, int* pshowcmd, LPITEMIDLIST* ppidl)
{
    IShellFolder* psf;

    HRESULT hr = SHGetDesktopFolder(&psf);

    *ppidl = NULL;

    if (SUCCEEDED(hr)) {
	LPITEMIDLIST pidl;
	ULONG l;

    	hr = IShellFolder_ParseDisplayName(psf, 0, 0, wcmd, &l, &pidl, 0);

	if (SUCCEEDED(hr)) {
	    IShellLinkW* psl;

	    hr = IShellFolder_GetUIObjectOf(psf, NULL, 1, (LPCITEMIDLIST*)&pidl, &IID_IShellLinkW, NULL, (LPVOID*)&psl);

	    if (SUCCEEDED(hr)) {
		hr = IShellLinkW_Resolve(psl, hwnd, 0);

		if (SUCCEEDED(hr)) {
		    hr = IShellLinkW_GetPath(psl, wcmd, MAX_PATH, NULL, SLGP_UNCPRIORITY);

		    if (SUCCEEDED(hr)) {
			if (!*wcmd) {
			    /* We could not translate the PIDL in the shell link into a valid file system path - so return the PIDL instead. */
			    hr = IShellLinkW_GetIDList(psl, ppidl);

			    if (SUCCEEDED(hr) && *ppidl) {
				/* We got a PIDL instead of a file system path - try to translate it. */
				hr = SHELL_GetPathFromIDListW(*ppidl, wcmd, MAX_PATH);
				if (SUCCEEDED(hr))
				    *ppidl = NULL;

				if (wcmd[0]==':' && wcmd[1]==':') {
				    /* open shell folder for the specified class GUID */
				    strcpyW(wcmd, L"explorer.exe");
				} else if (HCR_GetExecuteCommandW(L"Folder", lpVerb?lpVerb:L"open", args, MAX_PATH/*sizeof(szParameters)*/)) {
				    //FIXME@@ argifyW(wcmd, MAX_PATH/*sizeof(szApplicationName)*/, args, NULL, *ppidl, NULL);
				}
			    }
			}

			if (SUCCEEDED(hr)) {
			    /* get command line arguments, working directory and display mode if available */
			    IShellLinkW_GetWorkingDirectory(psl, wdir, MAX_PATH);
			    IShellLinkW_GetArguments(psl, args, MAX_PATH);
			    IShellLinkW_GetShowCmd(psl, pshowcmd);
			}
		    }
		}

		IShellLinkW_Release(psl);
	    }

	    SHFree(pidl);
	}

	IShellFolder_Release(psf);
    }

    return hr;
}

/*************************************************************************
 *	SHELL_ResolveShortCutA [Internal]
 */
static HRESULT SHELL_ResolveShortCutA(LPSHELLEXECUTEINFOA psei)
{
    WCHAR wcmd[MAX_PATH], wargs[MAX_PATH], wdir[MAX_PATH], wverb[MAX_PATH];

    HRESULT hr = S_OK;

    if (MultiByteToWideChar(CP_ACP, 0, psei->lpFile, -1, wcmd, MAX_PATH)) {
	MultiByteToWideChar(CP_ACP, 0, psei->lpDirectory, -1, wdir, MAX_PATH);
	MultiByteToWideChar(CP_ACP, 0, psei->lpVerb?psei->lpVerb:"", -1, wverb, MAX_PATH);

	hr = SHELL_ResolveShortCutW(wcmd, wargs, wdir, psei->hwnd, wverb, &psei->nShow, (LPITEMIDLIST*)&psei->lpIDList);

	if (SUCCEEDED(hr)) {
	    WideCharToMultiByte(CP_ACP, 0, wcmd, -1, (LPSTR)psei->lpFile, MAX_PATH/*sizeof(szApplicationName)*/, NULL, NULL);
	    WideCharToMultiByte(CP_ACP, 0, wdir, -1, (LPSTR)psei->lpDirectory, MAX_PATH/*sizeof(szDir)*/, NULL, NULL);
	    WideCharToMultiByte(CP_ACP, 0, wargs, -1, (LPSTR)psei->lpParameters, MAX_PATH/*sizeof(szParameters)*/, NULL, NULL);
	} else
	    FIXME("We could not resolve the shell shortcut.\n");
    }

    return hr;
}

/*************************************************************************
 *	SHELL_ExecuteA [Internal]
 *
 */
static UINT SHELL_ExecuteA(const char *lpCmd, void *env, BOOL shWait,
			    LPSHELLEXECUTEINFOA psei, LPSHELLEXECUTEINFOA psei_out)
{
    STARTUPINFOA  startup;
    PROCESS_INFORMATION info;
    UINT retval = 31;

    TRACE("Execute %s from directory %s\n", lpCmd, psei->lpDirectory);

    ZeroMemory(&startup,sizeof(STARTUPINFOA));
    startup.cb = sizeof(STARTUPINFOA);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = psei->nShow;

    if (CreateProcessA(NULL, (LPSTR)lpCmd, NULL, NULL, FALSE, 0,
                       env, psei->lpDirectory, &startup, &info))
    {
        /* Give 30 seconds to the app to come up, if desired. Probably only needed
           when starting app immediately before making a DDE connection. */
        if (shWait)
            if (WaitForInputIdle( info.hProcess, 30000 ) == -1)
                WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        retval = 33;

        if (psei->fMask & SEE_MASK_NOCLOSEPROCESS)
            psei_out->hProcess = info.hProcess;
        else
            CloseHandle(info.hProcess);

        CloseHandle(info.hThread);
    }
    else if ((retval = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", retval);
        retval = ERROR_BAD_FORMAT;
    }

    psei_out->hInstApp = (HINSTANCE)retval;
    return retval;
}


/***********************************************************************
 *           build_env
 *
 * Build the environment for the new process, adding the specified
 * path to the PATH variable. Returned pointer must be freed by caller.
 */
static void *build_env( const char *path )
{
    char *strings, *new_env;
    char *p, *p2;
    int total = strlen(path) + 1;
    BOOL got_path = FALSE;

    if (!(strings = GetEnvironmentStringsA())) return NULL;
    p = strings;
    while (*p)
    {
        int len = strlen(p) + 1;
        if (!strncasecmp( p, "PATH=", 5 )) got_path = TRUE;
        total += len;
        p += len;
    }
    if (!got_path) total += 5;  /* we need to create PATH */
    total++;  /* terminating null */

    if (!(new_env = HeapAlloc( GetProcessHeap(), 0, total )))
    {
        FreeEnvironmentStringsA( strings );
        return NULL;
    }
    p = strings;
    p2 = new_env;
    while (*p)
    {
        int len = strlen(p) + 1;
        memcpy( p2, p, len );
        if (!strncasecmp( p, "PATH=", 5 ))
        {
            p2[len - 1] = ';';
            strcpy( p2 + len, path );
            p2 += strlen(path) + 1;
        }
        p += len;
        p2 += len;
    }
    if (!got_path)
    {
        strcpy( p2, "PATH=" );
        strcat( p2, path );
        p2 += strlen(p2) + 1;
    }
    *p2 = 0;
    FreeEnvironmentStringsA( strings );
    return new_env;
}


/***********************************************************************
 *           SHELL_TryAppPath
 *
 * Helper function for SHELL_FindExecutable
 * @param lpResult - pointer to a buffer of size MAX_PATH
 * On entry: szName is a filename (probably without path separators).
 * On exit: if szName found in "App Path", place full path in lpResult, and return true
 */
static BOOL SHELL_TryAppPath( LPCSTR szName, LPSTR lpResult, void**env)
{
    HKEY hkApp = 0;
    char buffer[256];
    LONG len;
    LONG res;
    BOOL found = FALSE;

    if (env) *env = NULL;
    sprintf(buffer, "Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\%s", szName);
    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, buffer, 0, KEY_READ, &hkApp);
    if (res) goto end;

    len = MAX_PATH;
    res = RegQueryValueA(hkApp, NULL, lpResult, &len);
    if (res) goto end;
    found = TRUE;

    if (env)
    {
        DWORD count = sizeof(buffer);
        if (!RegQueryValueExA(hkApp, "Path", NULL, NULL, buffer, &count) && buffer[0])
            *env = build_env( buffer );
    }

end:
    if (hkApp) RegCloseKey(hkApp);
    return found;
}

static UINT _FindExecutableByOperation(LPCSTR lpPath, LPCSTR lpFile, LPCSTR lpOperation, LPSTR key, LPSTR filetype, LPSTR command)
{
    LONG commandlen = 256;  /* This is the most DOS can handle :) */

    /* Looking for ...buffer\shell\<verb>\command */
    strcat(filetype, "\\shell\\");
    strcat(filetype, lpOperation);
    strcat(filetype, "\\command");

    if (RegQueryValueA(HKEY_CLASSES_ROOT, filetype, command, &commandlen) == ERROR_SUCCESS)
    {
        if (key) strcpy(key, filetype);
#if 0
        LPSTR tmp;
        char param[256];
	LONG paramlen = 256;

        /* FIXME: it seems all Windows version don't behave the same here.
         * the doc states that this ddeexec information can be found after
         * the exec names.
         * on Win98, it doesn't appear, but I think it does on Win2k
         */
	/* Get the parameters needed by the application
	   from the associated ddeexec key */
	tmp = strstr(filetype, "command");
	tmp[0] = '\0';
	strcat(filetype, "ddeexec");

	if (RegQueryValueA(HKEY_CLASSES_ROOT, filetype, param, &paramlen) == ERROR_SUCCESS)
	{
            strcat(command, " ");
            strcat(command, param);
            commandlen += paramlen;
	}
#endif

	command[commandlen] = '\0';

	return 33; /* FIXME see SHELL_FindExecutable() */
    }

    return 31;	/* default - 'No association was found' */
}

/*************************************************************************
 *	SHELL_FindExecutable [Internal]
 *
 * Utility for code sharing between FindExecutable and ShellExecute
 * in:
 *      lpFile the name of a file
 *      lpOperation the operation on it (open)
 * out:
 *      lpResult a buffer, big enough :-(, to store the command to do the
 *              operation on the file
 *      key a buffer, big enough, to get the key name to do actually the
 *              command (it'll be used afterwards for more information
 *              on the operation)
 */
UINT SHELL_FindExecutable(LPCSTR lpPath, LPCSTR lpFile, LPCSTR lpOperation,
                          LPSTR lpResult, LPSTR key, void **env, LPITEMIDLIST pidl, LPCSTR args)
{
    char *extension = NULL; /* pointer to file extension */
    char tmpext[5];         /* local copy to munge as we please */
    char filetype[256];     /* registry name for this filetype */
    LONG filetypelen = 256; /* length of above */
    char command[256];      /* command from registry */
    char buffer[256];       /* Used to GetProfileString */
    UINT retval = 31;	    /* default - 'No association was found' */
    char *tok;              /* token pointer */
    char xlpFile[256] = ""; /* result of SearchPath */
    DWORD attribs;	    /* file attributes */

    TRACE("%s\n", (lpFile != NULL) ? lpFile : "-");

    lpResult[0] = '\0'; /* Start off with an empty return string */
    if (key) *key = '\0';

    /* trap NULL parameters on entry */
    if ((lpFile == NULL) || (lpResult == NULL))
    {
        WARN("(lpFile=%s,lpResult=%s): NULL parameter\n", lpFile, lpResult);
        return 2; /* File not found. Close enough, I guess. */
    }

    if (SHELL_TryAppPath( lpFile, lpResult, env ))
    {
        TRACE("found %s via App Paths\n", lpResult);
        return 33;
    }

    if (SearchPathA(lpPath, lpFile, ".exe", sizeof(xlpFile), xlpFile, NULL))
    {
        TRACE("SearchPathA returned non-zero\n");
        lpFile = xlpFile;
        /* Hey, isn't this value ignored?  Why make this call?  Shouldn't we return here?  --dank*/
    }

    attribs = GetFileAttributesA(lpFile);

    if (attribs!=INVALID_FILE_ATTRIBUTES && (attribs&FILE_ATTRIBUTE_DIRECTORY))
    {
	strcpy(filetype, "Folder");
	filetypelen = 6;    /* strlen("Folder") */
    }
    else
    {
	/* First thing we need is the file's extension */
	extension = PathFindExtensionA(xlpFile); /* Assume last "." is the one; */
					   /* File->Run in progman uses */
					   /* .\FILE.EXE :( */
	TRACE("xlpFile=%s,extension=%s\n", xlpFile, extension);

	if ((extension == NULL) || (extension == &xlpFile[strlen(xlpFile)]))
	{
	    WARN("Returning 31 - No association\n");
	    return 31; /* no association */
	}

	/* Make local copy & lowercase it for reg & 'programs=' lookup */
	lstrcpynA(tmpext, extension, 5);
	CharLowerA(tmpext);
	TRACE("%s file\n", tmpext);

	/* Three places to check: */
	/* 1. win.ini, [windows], programs (NB no leading '.') */
	/* 2. Registry, HKEY_CLASS_ROOT\<filetype>\shell\open\command */
	/* 3. win.ini, [extensions], extension (NB no leading '.' */
	/* All I know of the order is that registry is checked before */
	/* extensions; however, it'd make sense to check the programs */
	/* section first, so that's what happens here. */

	/* See if it's a program - if GetProfileString fails, we skip this
	 * section. Actually, if GetProfileString fails, we've probably
	 * got a lot more to worry about than running a program... */
	if (GetProfileStringA("windows", "programs", "exe pif bat cmd com",
			      buffer, sizeof(buffer)) > 0)
	{
	    UINT i;

	    for (i=0; i<strlen(buffer); i++) buffer[i] = tolower(buffer[i]);

	    tok = strtok(buffer, " \t"); /* ? */
	    while (tok!= NULL)
	    {
		if (strcmp(tok, &tmpext[1]) == 0) /* have to skip the leading "." */
		{
		    strcpy(lpResult, xlpFile);
		    /* Need to perhaps check that the file has a path
		     * attached */
		    TRACE("found %s\n", lpResult);
		    return 33;

		    /* Greater than 32 to indicate success FIXME According to the
		     * docs, I should be returning a handle for the
		     * executable. Does this mean I'm supposed to open the
		     * executable file or something? More RTFM, I guess... */
		}
		tok = strtok(NULL, " \t");
	    }
	}

	/* Check registry */
	if (RegQueryValueA(HKEY_CLASSES_ROOT, tmpext, filetype, &filetypelen) != ERROR_SUCCESS)
	    *filetype = '\0';
    }

    if (*filetype)
    {
	if (lpOperation)
	{
	    /* pass the operation string to _FindExecutableByOperation() */
	    filetype[filetypelen] = '\0';
	    retval = _FindExecutableByOperation(lpPath, lpFile, lpOperation, key, filetype, command);
	}
	else
	{
	    char operation[MAX_PATH];
	    HKEY hkey;

	    strcat(filetype, "\\shell");

	    /* enumerate the operation subkeys in the registry and search for one with an associated command */
	    if (RegOpenKeyA(HKEY_CLASSES_ROOT, filetype, &hkey) == ERROR_SUCCESS)
	    {
		int idx = 0;
		for(;; ++idx)
		{
		    if (RegEnumKeyA(hkey, idx, operation, MAX_PATH) != ERROR_SUCCESS)
			break;

		    filetype[filetypelen] = '\0';
		    retval = _FindExecutableByOperation(lpPath, lpFile, operation, key, filetype, command);

		    if (retval > 32)
			break;
		}

		RegCloseKey(hkey);
	    }
	}

	if (retval > 32)
        {
            argifyA(lpResult, sizeof(lpResult), command, xlpFile, pidl, args);

            /* Remove double quotation marks */
            if (*lpResult == '"')
            {
                char *p = lpResult;
                while (*(p + 1) != '"' && *(p + 1) != ' ')
                {
                    *p = *(p + 1);
                    p++;
                }
                *p = '\0';
            }
        }
    }
    else if (extension) /* Check win.ini */
    {
	/* Toss the leading dot */
	extension++;
	if (GetProfileStringA("extensions", extension, "", command,
                              sizeof(command)) > 0)
        {
            if (strlen(command) != 0)
            {
                strcpy(lpResult, command);
                tok = strstr(lpResult, "^"); /* should be ^.extension? */

                if (tok != NULL)
                {
                    tok[0] = '\0';
                    strcat(lpResult, xlpFile); /* what if no dir in xlpFile? */
                    tok = strstr(command, "^"); /* see above */
                    if ((tok != NULL) && (strlen(tok)>5))
                        strcat(lpResult, &tok[5]);
                }

                retval = 33; /* FIXME - see above */
            }
        }
    }

    TRACE("returning %s\n", lpResult);
    return retval;
}

/******************************************************************
 *		dde_cb
 *
 * callback for the DDE connection. not really usefull
 */
static HDDEDATA CALLBACK dde_cb(UINT uType, UINT uFmt, HCONV hConv,
                                HSZ hsz1, HSZ hsz2,
                                HDDEDATA hData, DWORD dwData1, DWORD dwData2)
{
    return NULL;
}

/******************************************************************
 *		dde_connect
 *
 * ShellExecute helper. Used to do an operation with a DDE connection
 *
 * Handles both the direct connection (try #1), and if it fails,
 * launching an application and trying (#2) to connect to it
 *
 */
static unsigned dde_connect(char* key, char* start, char* ddeexec,
                            const char* lpFile, void *env,
			    LPCSTR szCommandline, LPITEMIDLIST pidl,
			    LPSHELLEXECUTEINFOA psei, LPSHELLEXECUTEINFOA psei_out)
{
    char*       endkey = key + strlen(key);
    char        app[256], topic[256], ifexec[256];
    LONG        applen, topiclen, ifexeclen;
    char*       exec;
    DWORD       ddeInst = 0;
    DWORD       tid;
    HSZ         hszApp, hszTopic;
    HCONV       hConv;
    unsigned    ret = 31;

    strcpy(endkey, "\\application");
    applen = sizeof(app);
    if (RegQueryValueA(HKEY_CLASSES_ROOT, key, app, &applen) != ERROR_SUCCESS)
    {
        FIXME("default app name NIY %s\n", key);
        return 2;
    }

    strcpy(endkey, "\\topic");
    topiclen = sizeof(topic);
    if (RegQueryValueA(HKEY_CLASSES_ROOT, key, topic, &topiclen) != ERROR_SUCCESS)
    {
        strcpy(topic, "System");
    }

    if (DdeInitializeA(&ddeInst, dde_cb, APPCMD_CLIENTONLY, 0L) != DMLERR_NO_ERROR)
    {
        return 2;
    }

    hszApp = DdeCreateStringHandleA(ddeInst, app, CP_WINANSI);
    hszTopic = DdeCreateStringHandleA(ddeInst, topic, CP_WINANSI);

    hConv = DdeConnect(ddeInst, hszApp, hszTopic, NULL);
    exec = ddeexec;
    if (!hConv)
    {
        TRACE("Launching '%s'\n", start);
        ret = SHELL_ExecuteA(start, env, TRUE, psei, psei_out);
        if (ret < 32)
        {
            TRACE("Couldn't launch\n");
            goto error;
        }
        hConv = DdeConnect(ddeInst, hszApp, hszTopic, NULL);
        if (!hConv)
        {
            TRACE("Couldn't connect. ret=%d\n", ret);
            ret = 30; /* whatever */
            goto error;
        }
        strcpy(endkey, "\\ifexec");
        ifexeclen = sizeof(ifexec);
        if (RegQueryValueA(HKEY_CLASSES_ROOT, key, ifexec, &ifexeclen) == ERROR_SUCCESS)
        {
            exec = ifexec;
        }
    }

#if 0 /* argifyA has already been called. */
    argifyA(res, sizeof(res), exec, lpFile, pidl, szCommandline);
    TRACE("%s %s => %s\n", exec, lpFile, res);
#endif

    ret = (DdeClientTransaction((LPBYTE)szCommandline, strlen(szCommandline) + 1, hConv, 0L, 0,
                                XTYP_EXECUTE, 10000, &tid) != DMLERR_NO_ERROR) ? 31 : 33;
    DdeDisconnect(hConv);
 error:
    DdeUninitialize(ddeInst);
    return ret;
}

/*************************************************************************
 *	execute_from_key [Internal]
 */
static UINT execute_from_key(LPSTR key, LPCSTR lpFile, void *env, LPCSTR szCommandline,
			     LPSHELLEXECUTEINFOA psei, LPSHELLEXECUTEINFOA psei_out)
{
    char cmd[1024] = "";
    LONG cmdlen = sizeof(cmd);
    UINT retval = 31;

    /* Get the application for the registry */
    if (RegQueryValueA(HKEY_CLASSES_ROOT, key, cmd, &cmdlen) == ERROR_SUCCESS)
    {
        LPSTR tmp;
        char param[256] = "";
        LONG paramlen = 256;

        /* Get the parameters needed by the application
           from the associated ddeexec key */
        tmp = strstr(key, "command");
        assert(tmp);
        strcpy(tmp, "ddeexec");

        if (RegQueryValueA(HKEY_CLASSES_ROOT, key, param, &paramlen) == ERROR_SUCCESS)
        {
            TRACE("Got ddeexec %s => %s\n", key, param);
            retval = dde_connect(key, cmd, param, lpFile, env, szCommandline, psei->lpIDList, psei, psei_out);
        }
        else
        {
#if 0 /* argifyA() has already been called. */
            /* Is there a replace() function anywhere? */
            cmd[cmdlen] = '\0';
            argifyA(param, sizeof(param), cmd, lpFile, psei->lpIDList, szCommandline);
#else
	    strcpy(param, szCommandline);
#endif
            retval = SHELL_ExecuteA(param, env, FALSE, psei, psei_out);
        }
    }
    else TRACE("ooch\n");

    return retval;
}

/*************************************************************************
 * FindExecutableA			[SHELL32.@]
 */
HINSTANCE WINAPI FindExecutableA(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
    UINT retval = 31;    /* default - 'No association was found' */
    char old_dir[1024];

    TRACE("File %s, Dir %s\n",
          (lpFile != NULL ? lpFile : "-"), (lpDirectory != NULL ? lpDirectory : "-"));

    lpResult[0] = '\0'; /* Start off with an empty return string */

    /* trap NULL parameters on entry */
    if ((lpFile == NULL) || (lpResult == NULL))
    {
        /* FIXME - should throw a warning, perhaps! */
	return (HINSTANCE)2; /* File not found. Close enough, I guess. */
    }

    if (lpDirectory)
    {
        GetCurrentDirectoryA(sizeof(old_dir), old_dir);
        SetCurrentDirectoryA(lpDirectory);
    }

    retval = SHELL_FindExecutable(lpDirectory, lpFile, "open", lpResult, NULL, NULL, NULL, NULL);

    TRACE("returning %s\n", lpResult);
    if (lpDirectory)
        SetCurrentDirectoryA(old_dir);
    return (HINSTANCE)retval;
}

/*************************************************************************
 * FindExecutableW			[SHELL32.@]
 */
HINSTANCE WINAPI FindExecutableW(LPCWSTR lpFile, LPCWSTR lpDirectory, LPWSTR lpResult)
{
    FIXME("(%p,%p,%p): stub\n", lpFile, lpDirectory, lpResult);
    return (HINSTANCE)31;    /* default - 'No association was found' */
}

/*************************************************************************
 *	ShellExecuteExA32 [Internal]
 *
 *  FIXME:  use PathProcessCommandA() to processes the command line string
 *	    use PathResolveA() to search for the fully qualified executable path
 */
BOOL WINAPI ShellExecuteExA32 (LPSHELLEXECUTEINFOA psei)
{
    CHAR szApplicationName[MAX_PATH+2], szParameters[MAX_PATH], fileName[MAX_PATH], szDir[MAX_PATH];
    SHELLEXECUTEINFOA sei_tmp;	/* modifyable copy of SHELLEXECUTEINFO struct */
    void *env;
    char szProtocol[256];
    LPCSTR lpFile;
    UINT retval = 31;
    char cmd[1024];
    const char* ext;
    BOOL done;

    memcpy(&sei_tmp, psei, sizeof(sei_tmp));

    TRACE("mask=0x%08lx hwnd=%p verb=%s file=%s parm=%s dir=%s show=0x%08x class=%s\n",
            sei_tmp.fMask, sei_tmp.hwnd, debugstr_a(sei_tmp.lpVerb),
            debugstr_a(sei_tmp.lpFile), debugstr_a(sei_tmp.lpParameters),
            debugstr_a(sei_tmp.lpDirectory), sei_tmp.nShow,
            (sei_tmp.fMask & SEE_MASK_CLASSNAME) ? debugstr_a(sei_tmp.lpClass) : "not used");

    psei->hProcess = NULL;

    if (sei_tmp.lpFile)
        strcpy(szApplicationName, sei_tmp.lpFile);
    else
	*szApplicationName = '\0';

    if (sei_tmp.lpParameters)
        strcpy(szParameters, sei_tmp.lpParameters);
    else
	*szParameters = '\0';

    if (sei_tmp.lpDirectory)
	strcpy(szDir, sei_tmp.lpDirectory);
    else
	*szDir = '\0';

    sei_tmp.lpFile = szApplicationName;
    sei_tmp.lpParameters = szParameters;
    sei_tmp.lpDirectory = szDir;

    if (sei_tmp.fMask & (SEE_MASK_ICON | SEE_MASK_HOTKEY |
        SEE_MASK_CONNECTNETDRV | SEE_MASK_FLAG_DDEWAIT |
        SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI | SEE_MASK_UNICODE |
        SEE_MASK_NO_CONSOLE | SEE_MASK_ASYNCOK | SEE_MASK_HMONITOR ))
    {
        FIXME("flags ignored: 0x%08lx\n", sei_tmp.fMask);
    }

    /* process the IDList */
    if (sei_tmp.fMask & SEE_MASK_INVOKEIDLIST) /* 0x0c: includes SEE_MASK_IDLIST */
    {
	IShellExecuteHookA* pSEH;

	HRESULT hr = SHBindToParent(sei_tmp.lpIDList, &IID_IShellExecuteHookA, (LPVOID*)&pSEH, NULL);

	if (SUCCEEDED(hr)) {
	    hr = IShellExecuteHookA_Execute(pSEH, psei);

	    IShellExecuteHookA_Release(pSEH);

	    if (hr == S_OK)
		return TRUE;
	}

	/* translate PIDL into the corresponding file system path */
        if (FAILED(SHELL_GetPathFromIDListA(sei_tmp.lpIDList, szApplicationName/*sei_tmp.lpFile*/, sizeof(szApplicationName))))
            return FALSE;

        TRACE("-- idlist=%p (%s)\n", sei_tmp.lpIDList, sei_tmp.lpFile);
    }

    if (sei_tmp.fMask & (SEE_MASK_CLASSNAME | SEE_MASK_CLASSKEY))
    {
        /* launch a document by fileclass like 'WordPad.Document.1' */
        /* the Commandline contains 'c:\Path\wordpad.exe "%1"' */
        /* FIXME: szParameters should not be of a fixed size. Plus MAX_PATH is way too short! */
        if (sei_tmp.fMask & SEE_MASK_CLASSKEY)
            HCR_GetExecuteCommandExA(sei_tmp.hkeyClass,
                                    sei_tmp.fMask&SEE_MASK_CLASSNAME? sei_tmp.lpClass: NULL,
                                    sei_tmp.lpVerb?sei_tmp.lpVerb:"open",
				    szParameters/*sei_tmp.lpParameters*/, sizeof(szParameters));
        else if (sei_tmp.fMask & SEE_MASK_CLASSNAME)
            HCR_GetExecuteCommandA(sei_tmp.lpClass, sei_tmp.lpVerb?sei_tmp.lpVerb:"open",
				    szParameters/*sei_tmp.lpParameters*/, sizeof(szParameters));

        /* FIXME: get the extension of lpFile, check if it fits to the lpClass */
        TRACE("SEE_MASK_CLASSNAME->'%s', doc->'%s'\n", szParameters, sei_tmp.lpFile);

        cmd[0] = '\0';
        done = argifyA(cmd, sizeof(cmd), szParameters, sei_tmp.lpFile, sei_tmp.lpIDList, NULL);
        if (!done && sei_tmp.lpFile[0])
        {
            strcat(cmd, " ");
            strcat(cmd, sei_tmp.lpFile);
        }

        retval = SHELL_ExecuteA(cmd, NULL, FALSE, &sei_tmp, psei);
        if (retval > 32)
            return TRUE;
        else
            return FALSE;
    }

    /* Else, try to execute the filename */
    TRACE("execute:'%s','%s'\n", sei_tmp.lpFile, szParameters);

    /* resolve shell shortcuts */
    ext = PathFindExtensionA(sei_tmp.lpFile);

    if (ext && !strcasecmp(ext, ".lnk"))	/* or check for: shell_attribs & SFGAO_LINK */
	SHELL_ResolveShortCutA(&sei_tmp);

    /* separate out command line arguments from executable file name */
    if (!*szParameters) {
	/* If the executable path is quoted, handle the rest of the command line as parameters. */
	if (sei_tmp.lpFile[0] == '"') {
	    LPSTR src = szApplicationName/*sei_tmp.lpFile*/ + 1;
	    LPSTR dst = fileName;
	    LPSTR end;

	    /* copy the unquoted executabe path to 'fileName' */
	    while(*src && *src!='"')
		*dst++ = *src++;

	    *dst = '\0';

	    if (*src == '"') {
		end = ++src;

		while(isspace(*src))
		    ++src;
	    } else
		end = src;

	    /* copy the paremeter string to 'szParameters' */
	    strcpy(szParameters, src);

	    /* terminate 'sei_tmp.lpFile' after the quote character */
	    *end = '\0';
	}
	else
	{
	    /* If the executable name is not quoted, we have to use this search loop here,
	       that in CreateProcess() is not sufficient because it does not handle shell links. */
	    char buffer[MAX_PATH], xlpFile[MAX_PATH];
	    LPSTR space, s;

	    LPSTR beg = szApplicationName/*sei_tmp.lpFile*/;
	    for(s=beg; (space=strchr(s, ' ')); s=space+1) {
		int idx = space-sei_tmp.lpFile;
		strncpy(buffer, sei_tmp.lpFile, idx);
		buffer[idx] = '\0';

		if (SearchPathA(*sei_tmp.lpDirectory? sei_tmp.lpDirectory: NULL, buffer, ".exe", sizeof(xlpFile), xlpFile, NULL))
		{
		    /* separate out command from parameter string */
		    LPCSTR p = space + 1;

		    while(isspace(*p))
			++p;

		    strcpy(szParameters, p);
		    *space = '\0';

		    break;
		}
	    }

	    strcpy(fileName, sei_tmp.lpFile);
	}
    } else
	strcpy(fileName, sei_tmp.lpFile);

    lpFile = fileName;

    if (szParameters[0]) {
        strcat(szApplicationName/*sei_tmp.lpFile*/, " ");
        strcat(szApplicationName/*sei_tmp.lpFile*/, szParameters);
    }

    retval = SHELL_ExecuteA(sei_tmp.lpFile, NULL, FALSE, &sei_tmp, psei);
    if (retval > 32)
    {
	/* Now, that we have successfully launched a process, we can free the PIDL.
	It may have been used before for %I command line options. */
	if (sei_tmp.lpIDList!=psei->lpIDList && sei_tmp.lpIDList)
	    SHFree(sei_tmp.lpIDList);

        TRACE("SHELL_ExecuteA: retval=%d psei->hInstApp=%p\n", retval, psei->hInstApp);
        return TRUE;
    }

    /* Else, try to find the executable */
    cmd[0] = '\0';
    retval = SHELL_FindExecutable(*sei_tmp.lpDirectory? sei_tmp.lpDirectory: NULL, lpFile, sei_tmp.lpVerb, cmd, szProtocol, &env, sei_tmp.lpIDList, szParameters);
    if (retval > 32)  /* Found */
    {
#if 0	/* SHELL_FindExecutable() already quoted by calling argifyA() */
        CHAR szQuotedCmd[MAX_PATH+2];
        /* Must quote to handle case where cmd contains spaces, 
         * else security hole if malicious user creates executable file "C:\\Program"
	 *
	 * FIXME: If we don't have set explicitly command line arguments, we must first
	 * split executable path from optional command line arguments. Otherwise we would quote
	 * the complete string with executable path _and_ arguments, which is not what we want.
         */
        if (szParameters[0])
            sprintf(szQuotedCmd, "\"%s\" %s", cmd, szParameters);
        else
            sprintf(szQuotedCmd, "\"%s\"", cmd);
        TRACE("%s/%s => %s/%s\n", sei_tmp.lpFile, sei_tmp.lpVerb?sei_tmp.lpVerb:"NULL", szQuotedCmd, szProtocol);
        if (*szProtocol)
            retval = execute_from_key(szProtocol, lpFile, env, sei, SHELL_ExecuteA, szParameters, &sei_tmp, psei);
        else
            retval = SHELL_ExecuteA(szQuotedCmd, env, sei_tmp.lpDirectory, sei, FALSE);
#else
        if (*szProtocol)
            retval = execute_from_key(szProtocol, lpFile, env, cmd, &sei_tmp, psei);
        else
            retval = SHELL_ExecuteA(cmd, env, FALSE, &sei_tmp, psei);
#endif
        if (env) HeapFree( GetProcessHeap(), 0, env );
    }
    else if (PathIsURLA((LPSTR)lpFile))    /* File not found, check for URL */
    {
        LPSTR lpstrRes;
        INT iSize;

        lpstrRes = strchr(lpFile, ':');
        if (lpstrRes)
            iSize = lpstrRes - lpFile;
        else
            iSize = strlen(lpFile);

        TRACE("Got URL: %s\n", lpFile);
        /* Looking for ...protocol\shell\<verb>\command */
        strncpy(szProtocol, lpFile, iSize);
        szProtocol[iSize] = '\0';
        strcat(szProtocol, "\\shell\\");
        strcat(szProtocol, sei_tmp.lpVerb? sei_tmp.lpVerb: "open");    /*FIXME: enumerate registry subkeys - compare with the loop into SHELL_FindExecutable() */
        strcat(szProtocol, "\\command");

        /* Remove File Protocol from lpFile */
        /* In the case file://path/file     */
        if (!strncasecmp(lpFile, "file", iSize))
        {
            lpFile += iSize;
            while (*lpFile == ':') lpFile++;
        }

        retval = execute_from_key(szProtocol, lpFile, NULL, szParameters, &sei_tmp, psei);
    }
    /* Check if file specified is in the form www.??????.*** */
    else if (!strncasecmp(lpFile, "www", 3))
    {
        /* if so, append lpFile http:// and call ShellExecute */
        char lpstrTmpFile[256] = "http://" ;
        strcat(lpstrTmpFile, lpFile);
        retval = (UINT)ShellExecuteA(sei_tmp.hwnd, sei_tmp.lpVerb, lpstrTmpFile, NULL, NULL, 0);
    }

    /* Now we can free the PIDL. It may have been used before for %I command line options. */
    if (sei_tmp.lpIDList!=psei->lpIDList && sei_tmp.lpIDList)
	SHFree(sei_tmp.lpIDList);

    TRACE("ShellExecuteExA32 retval=%d\n", retval);

    if (retval <= 32)
    {
        psei->hInstApp = (HINSTANCE)retval;
        return FALSE;
    }

    psei->hInstApp = (HINSTANCE)33;
    return TRUE;
}

/*************************************************************************
 * ShellExecuteA			[SHELL32.290]
 */
HINSTANCE WINAPI ShellExecuteA(HWND hWnd, LPCSTR lpOperation,LPCSTR lpFile,
                               LPCSTR lpParameters,LPCSTR lpDirectory, INT iShowCmd)
{
    SHELLEXECUTEINFOA sei;
    HANDLE hProcess = 0;

    TRACE("\n");
    sei.cbSize = sizeof(sei);
    sei.fMask = 0;
    sei.hwnd = hWnd;
    sei.lpVerb = lpOperation;
    sei.lpFile = lpFile;
    sei.lpParameters = lpParameters;
    sei.lpDirectory = lpDirectory;
    sei.nShow = iShowCmd;
    sei.lpIDList = 0;
    sei.lpClass = 0;
    sei.hkeyClass = 0;
    sei.dwHotKey = 0;
    sei.hProcess = hProcess;

    ShellExecuteExA32(&sei);
    return sei.hInstApp;
}

/*************************************************************************
 * ShellExecuteEx				[SHELL32.291]
 *
 */
BOOL WINAPI ShellExecuteExAW (LPVOID sei)
{
    if (SHELL_OsIsUnicode())
	return ShellExecuteExW (sei);
    return ShellExecuteExA32(sei);
}

/*************************************************************************
 * ShellExecuteExA				[SHELL32.292]
 *
 */
BOOL WINAPI ShellExecuteExA (LPSHELLEXECUTEINFOA sei)
{
    BOOL ret = ShellExecuteExA32(sei);

    TRACE("ShellExecuteExA(): ret=%d\n", ret);

    return ret;
}

/*************************************************************************
 * ShellExecuteExW				[SHELL32.293]
 *
 */
BOOL WINAPI ShellExecuteExW (LPSHELLEXECUTEINFOW sei)
{
    SHELLEXECUTEINFOA seiA;
    BOOL ret;

    TRACE("%p\n", sei);

    memcpy(&seiA, sei, sizeof(SHELLEXECUTEINFOA));

    if (sei->lpVerb)
        seiA.lpVerb = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpVerb);

    if (sei->lpFile)
        seiA.lpFile = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpFile);

    if (sei->lpParameters)
        seiA.lpParameters = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpParameters);

    if (sei->lpDirectory)
        seiA.lpDirectory = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpDirectory);

    if ((sei->fMask & SEE_MASK_CLASSNAME) && sei->lpClass)
        seiA.lpClass = HEAP_strdupWtoA( GetProcessHeap(), 0, sei->lpClass);
    else
        seiA.lpClass = NULL;

    ret = ShellExecuteExA(&seiA);

    if (seiA.lpVerb)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpVerb );
    if (seiA.lpFile)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpFile );
    if (seiA.lpParameters)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpParameters );
    if (seiA.lpDirectory)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpDirectory );
    if (seiA.lpClass)	HeapFree( GetProcessHeap(), 0, (LPSTR) seiA.lpClass );

    sei->hInstApp = seiA.hInstApp;

    TRACE("ShellExecuteExW(): ret=%d\n", ret);

    return ret;
}

/*************************************************************************
 * ShellExecuteW			[SHELL32.294]
 * from shellapi.h
 * WINSHELLAPI HINSTANCE APIENTRY ShellExecuteW(HWND hwnd, LPCWSTR lpOperation,
 * LPCWSTR lpFile, LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd);
 */
HINSTANCE WINAPI ShellExecuteW(HWND hwnd, LPCWSTR lpOperation, LPCWSTR lpFile,
                               LPCWSTR lpParameters, LPCWSTR lpDirectory, INT nShowCmd)
{
    SHELLEXECUTEINFOW sei;
    HANDLE hProcess = 0;
    int ret;

    TRACE("\n");
    sei.cbSize = sizeof(sei);
    sei.fMask = 0;
    sei.hwnd = hwnd;
    sei.lpVerb = lpOperation;
    sei.lpFile = lpFile;
    sei.lpParameters = lpParameters;
    sei.lpDirectory = lpDirectory;
    sei.nShow = nShowCmd;
    sei.lpIDList = 0;
    sei.lpClass = 0;
    sei.hkeyClass = 0;
    sei.dwHotKey = 0;
    sei.hProcess = hProcess;

    ret = ShellExecuteExW(&sei);

    TRACE("ShellExecuteW(): ret=%d module=%p", ret, sei.hInstApp);
    return sei.hInstApp;
}
