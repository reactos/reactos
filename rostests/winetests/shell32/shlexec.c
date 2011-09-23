/*
 * Unit test of the ShellExecute function.
 *
 * Copyright 2005 Francois Gouget for CodeWeavers
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

/* TODO:
 * - test the default verb selection
 * - test selection of an alternate class
 * - try running executables in more ways
 * - try passing arguments to executables
 * - ShellExecute("foo.shlexec") with no path should work if foo.shlexec is
 *   in the PATH
 * - test associations that use %l, %L or "%1" instead of %1
 * - we may want to test ShellExecuteEx() instead of ShellExecute()
 *   and then we could also check its return value
 * - ShellExecuteEx() also calls SetLastError() with meaningful values which
 *   we could check
 */

/* Needed to get SEE_MASK_NOZONECHECKS with the PSDK */
#define NTDDI_WINXPSP1 0x05010100
#define NTDDI_VERSION NTDDI_WINXPSP1
#define _WIN32_WINNT 0x0501

#include <stdio.h>
#include <assert.h>

#include "wtypes.h"
#include "winbase.h"
#include "windef.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "wine/test.h"

#include "shell32_test.h"


static char argv0[MAX_PATH];
static int myARGC;
static char** myARGV;
static char tmpdir[MAX_PATH];
static char child_file[MAX_PATH];
static DLLVERSIONINFO dllver;
static BOOL skip_noassoc_tests = FALSE;
static HANDLE dde_ready_event;


/***
 *
 * ShellExecute wrappers
 *
 ***/
static void dump_child(void);

static HANDLE hEvent;
static void init_event(const char* child_file)
{
    char* event_name;
    event_name=strrchr(child_file, '\\')+1;
    hEvent=CreateEvent(NULL, FALSE, FALSE, event_name);
}

static void strcat_param(char* str, const char* param)
{
    if (param!=NULL)
    {
        strcat(str, "\"");
        strcat(str, param);
        strcat(str, "\"");
    }
    else
    {
        strcat(str, "null");
    }
}

static char shell_call[2048]="";
static int shell_execute(LPCSTR operation, LPCSTR file, LPCSTR parameters, LPCSTR directory)
{
    INT_PTR rc;

    strcpy(shell_call, "ShellExecute(");
    strcat_param(shell_call, operation);
    strcat(shell_call, ", ");
    strcat_param(shell_call, file);
    strcat(shell_call, ", ");
    strcat_param(shell_call, parameters);
    strcat(shell_call, ", ");
    strcat_param(shell_call, directory);
    strcat(shell_call, ")");
    if (winetest_debug > 1)
        trace("%s\n", shell_call);

    DeleteFile(child_file);
    SetLastError(0xcafebabe);

    /* FIXME: We cannot use ShellExecuteEx() here because if there is no
     * association it displays the 'Open With' dialog and I could not find
     * a flag to prevent this.
     */
    rc=(INT_PTR)ShellExecute(NULL, operation, file, parameters, directory, SW_SHOWNORMAL);

    if (rc > 32)
    {
        int wait_rc;
        wait_rc=WaitForSingleObject(hEvent, 5000);
        if (wait_rc == WAIT_TIMEOUT)
        {
            HWND wnd = FindWindowA("#32770", "Windows");
            if (wnd != NULL)
            {
                SendMessage(wnd, WM_CLOSE, 0, 0);
                win_skip("Skipping shellexecute of file with unassociated extension\n");
                skip_noassoc_tests = TRUE;
                rc = SE_ERR_NOASSOC;
            }
        }
        ok(wait_rc==WAIT_OBJECT_0 || rc <= 32, "WaitForSingleObject returned %d\n", wait_rc);
    }
    /* The child process may have changed the result file, so let profile
     * functions know about it
     */
    WritePrivateProfileStringA(NULL, NULL, NULL, child_file);
    if (rc > 32)
        dump_child();

    return rc;
}

static int shell_execute_ex(DWORD mask, LPCSTR operation, LPCSTR file,
                            LPCSTR parameters, LPCSTR directory)
{
    SHELLEXECUTEINFO sei;
    BOOL success;
    INT_PTR rc;

    strcpy(shell_call, "ShellExecuteEx(");
    strcat_param(shell_call, operation);
    strcat(shell_call, ", ");
    strcat_param(shell_call, file);
    strcat(shell_call, ", ");
    strcat_param(shell_call, parameters);
    strcat(shell_call, ", ");
    strcat_param(shell_call, directory);
    strcat(shell_call, ")");
    if (winetest_debug > 1)
        trace("%s\n", shell_call);

    sei.cbSize=sizeof(sei);
    sei.fMask=SEE_MASK_NOCLOSEPROCESS | mask;
    sei.hwnd=NULL;
    sei.lpVerb=operation;
    sei.lpFile=file;
    sei.lpParameters=parameters;
    sei.lpDirectory=directory;
    sei.nShow=SW_SHOWNORMAL;
    sei.hInstApp=NULL; /* Out */
    sei.lpIDList=NULL;
    sei.lpClass=NULL;
    sei.hkeyClass=NULL;
    sei.dwHotKey=0;
    U(sei).hIcon=NULL;
    sei.hProcess=NULL; /* Out */

    DeleteFile(child_file);
    SetLastError(0xcafebabe);
    success=ShellExecuteEx(&sei);
    rc=(INT_PTR)sei.hInstApp;
    ok((success && rc > 32) || (!success && rc <= 32),
       "%s rc=%d and hInstApp=%ld is not allowed\n", shell_call, success, rc);

    if (rc > 32)
    {
        int wait_rc;
        if (sei.hProcess!=NULL)
        {
            wait_rc=WaitForSingleObject(sei.hProcess, 5000);
            ok(wait_rc==WAIT_OBJECT_0, "WaitForSingleObject(hProcess) returned %d\n", wait_rc);
        }
        wait_rc=WaitForSingleObject(hEvent, 5000);
        ok(wait_rc==WAIT_OBJECT_0, "WaitForSingleObject returned %d\n", wait_rc);
    }
    /* The child process may have changed the result file, so let profile
     * functions know about it
     */
    WritePrivateProfileStringA(NULL, NULL, NULL, child_file);
    if (rc > 32)
        dump_child();

    return rc;
}



/***
 *
 * Functions to create / delete associations wrappers
 *
 ***/

static BOOL create_test_association(const char* extension)
{
    HKEY hkey, hkey_shell;
    char class[MAX_PATH];
    LONG rc;

    sprintf(class, "shlexec%s", extension);
    rc=RegCreateKeyEx(HKEY_CLASSES_ROOT, extension, 0, NULL, 0, KEY_SET_VALUE,
                      NULL, &hkey, NULL);
    if (rc != ERROR_SUCCESS)
        return FALSE;

    rc=RegSetValueEx(hkey, NULL, 0, REG_SZ, (LPBYTE) class, strlen(class)+1);
    ok(rc==ERROR_SUCCESS, "RegSetValueEx '%s' failed, expected ERROR_SUCCESS, got %d\n", class, rc);
    CloseHandle(hkey);

    rc=RegCreateKeyEx(HKEY_CLASSES_ROOT, class, 0, NULL, 0,
                      KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS, NULL, &hkey, NULL);
    ok(rc==ERROR_SUCCESS, "RegCreateKeyEx '%s' failed, expected ERROR_SUCCESS, got %d\n", class, rc);

    rc=RegCreateKeyEx(hkey, "shell", 0, NULL, 0,
                      KEY_CREATE_SUB_KEY, NULL, &hkey_shell, NULL);
    ok(rc==ERROR_SUCCESS, "RegCreateKeyEx 'shell' failed, expected ERROR_SUCCESS, got %d\n", rc);

    CloseHandle(hkey);
    CloseHandle(hkey_shell);

    return TRUE;
}

/* Based on RegDeleteTreeW from dlls/advapi32/registry.c */
static LSTATUS myRegDeleteTreeA(HKEY hKey, LPCSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    CHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExA(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret) return ret;
    }

    /* Get highest length for keys, values */
    ret = RegQueryInfoKeyA(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > sizeof(szNameBuf)/sizeof(CHAR))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = HeapAlloc( GetProcessHeap(), 0, dwMaxLen*sizeof(CHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }


    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExA(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = myRegDeleteTreeA(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyA(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueA(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueA(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    /* Free buffer if allocated */
    if (lpszName != szNameBuf)
        HeapFree( GetProcessHeap(), 0, lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}

static void delete_test_association(const char* extension)
{
    char class[MAX_PATH];

    sprintf(class, "shlexec%s", extension);
    myRegDeleteTreeA(HKEY_CLASSES_ROOT, class);
    myRegDeleteTreeA(HKEY_CLASSES_ROOT, extension);
}

static void create_test_verb_dde(const char* extension, const char* verb,
                                 int rawcmd, const char* cmdtail, const char *ddeexec,
                                 const char *application, const char *topic,
                                 const char *ifexec)
{
    HKEY hkey_shell, hkey_verb, hkey_cmd;
    char shell[MAX_PATH];
    char* cmd;
    LONG rc;

    sprintf(shell, "shlexec%s\\shell", extension);
    rc=RegOpenKeyEx(HKEY_CLASSES_ROOT, shell, 0,
                    KEY_CREATE_SUB_KEY, &hkey_shell);
    assert(rc==ERROR_SUCCESS);
    rc=RegCreateKeyEx(hkey_shell, verb, 0, NULL, 0, KEY_CREATE_SUB_KEY,
                      NULL, &hkey_verb, NULL);
    assert(rc==ERROR_SUCCESS);
    rc=RegCreateKeyEx(hkey_verb, "command", 0, NULL, 0, KEY_SET_VALUE,
                      NULL, &hkey_cmd, NULL);
    assert(rc==ERROR_SUCCESS);

    if (rawcmd)
    {
        rc=RegSetValueEx(hkey_cmd, NULL, 0, REG_SZ, (LPBYTE)cmdtail, strlen(cmdtail)+1);
    }
    else
    {
        cmd=HeapAlloc(GetProcessHeap(), 0, strlen(argv0)+10+strlen(child_file)+2+strlen(cmdtail)+1);
        sprintf(cmd,"%s shlexec \"%s\" %s", argv0, child_file, cmdtail);
        rc=RegSetValueEx(hkey_cmd, NULL, 0, REG_SZ, (LPBYTE)cmd, strlen(cmd)+1);
        assert(rc==ERROR_SUCCESS);
        HeapFree(GetProcessHeap(), 0, cmd);
    }

    if (ddeexec)
    {
        HKEY hkey_ddeexec, hkey_application, hkey_topic, hkey_ifexec;

        rc=RegCreateKeyEx(hkey_verb, "ddeexec", 0, NULL, 0, KEY_SET_VALUE |
                          KEY_CREATE_SUB_KEY, NULL, &hkey_ddeexec, NULL);
        assert(rc==ERROR_SUCCESS);
        rc=RegSetValueEx(hkey_ddeexec, NULL, 0, REG_SZ, (LPBYTE)ddeexec,
                         strlen(ddeexec)+1);
        assert(rc==ERROR_SUCCESS);
        if (application)
        {
            rc=RegCreateKeyEx(hkey_ddeexec, "application", 0, NULL, 0, KEY_SET_VALUE,
                              NULL, &hkey_application, NULL);
            assert(rc==ERROR_SUCCESS);
            rc=RegSetValueEx(hkey_application, NULL, 0, REG_SZ, (LPBYTE)application,
                             strlen(application)+1);
            assert(rc==ERROR_SUCCESS);
            CloseHandle(hkey_application);
        }
        if (topic)
        {
            rc=RegCreateKeyEx(hkey_ddeexec, "topic", 0, NULL, 0, KEY_SET_VALUE,
                              NULL, &hkey_topic, NULL);
            assert(rc==ERROR_SUCCESS);
            rc=RegSetValueEx(hkey_topic, NULL, 0, REG_SZ, (LPBYTE)topic,
                             strlen(topic)+1);
            assert(rc==ERROR_SUCCESS);
            CloseHandle(hkey_topic);
        }
        if (ifexec)
        {
            rc=RegCreateKeyEx(hkey_ddeexec, "ifexec", 0, NULL, 0, KEY_SET_VALUE,
                              NULL, &hkey_ifexec, NULL);
            assert(rc==ERROR_SUCCESS);
            rc=RegSetValueEx(hkey_ifexec, NULL, 0, REG_SZ, (LPBYTE)ifexec,
                             strlen(ifexec)+1);
            assert(rc==ERROR_SUCCESS);
            CloseHandle(hkey_ifexec);
        }
        CloseHandle(hkey_ddeexec);
    }

    CloseHandle(hkey_shell);
    CloseHandle(hkey_verb);
    CloseHandle(hkey_cmd);
}

static void create_test_verb(const char* extension, const char* verb,
                             int rawcmd, const char* cmdtail)
{
    create_test_verb_dde(extension, verb, rawcmd, cmdtail, NULL, NULL,
                         NULL, NULL);
}

/***
 *
 * Functions to check that the child process was started just right
 * (borrowed from dlls/kernel32/tests/process.c)
 *
 ***/

static const char* encodeA(const char* str)
{
    static char encoded[2*1024+1];
    char*       ptr;
    size_t      len,i;

    if (!str) return "";
    len = strlen(str) + 1;
    if (len >= sizeof(encoded)/2)
    {
        fprintf(stderr, "string is too long!\n");
        assert(0);
    }
    ptr = encoded;
    for (i = 0; i < len; i++)
        sprintf(&ptr[i * 2], "%02x", (unsigned char)str[i]);
    ptr[2 * len] = '\0';
    return ptr;
}

static unsigned decode_char(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    assert(c >= 'A' && c <= 'F');
    return c - 'A' + 10;
}

static char* decodeA(const char* str)
{
    static char decoded[1024];
    char*       ptr;
    size_t      len,i;

    len = strlen(str) / 2;
    if (!len--) return NULL;
    if (len >= sizeof(decoded))
    {
        fprintf(stderr, "string is too long!\n");
        assert(0);
    }
    ptr = decoded;
    for (i = 0; i < len; i++)
        ptr[i] = (decode_char(str[2 * i]) << 4) | decode_char(str[2 * i + 1]);
    ptr[len] = '\0';
    return ptr;
}

static void     childPrintf(HANDLE h, const char* fmt, ...)
{
    va_list     valist;
    char        buffer[1024];
    DWORD       w;

    va_start(valist, fmt);
    vsprintf(buffer, fmt, valist);
    va_end(valist);
    WriteFile(h, buffer, strlen(buffer), &w, NULL);
}

static DWORD ddeInst;
static HSZ hszTopic;
static char ddeExec[MAX_PATH], ddeApplication[MAX_PATH];
static BOOL post_quit_on_execute;

static HDDEDATA CALLBACK ddeCb(UINT uType, UINT uFmt, HCONV hConv,
                               HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                               ULONG_PTR dwData1, ULONG_PTR dwData2)
{
    DWORD size = 0;

    if (winetest_debug > 2)
        trace("dde_cb: %04x, %04x, %p, %p, %p, %p, %08lx, %08lx\n",
              uType, uFmt, hConv, hsz1, hsz2, hData, dwData1, dwData2);

    switch (uType)
    {
        case XTYP_CONNECT:
            if (!DdeCmpStringHandles(hsz1, hszTopic))
            {
                size = DdeQueryString(ddeInst, hsz2, ddeApplication, MAX_PATH, CP_WINANSI);
                assert(size < MAX_PATH);
                return (HDDEDATA)TRUE;
            }
            return (HDDEDATA)FALSE;

        case XTYP_EXECUTE:
            size = DdeGetData(hData, (LPBYTE)ddeExec, MAX_PATH, 0L);
            assert(size < MAX_PATH);
            DdeFreeDataHandle(hData);
            if (post_quit_on_execute)
                PostQuitMessage(0);
            return (HDDEDATA)DDE_FACK;

        default:
            return NULL;
    }
}

/*
 * This is just to make sure the child won't run forever stuck in a GetMessage()
 * loop when DDE fails for some reason.
 */
static void CALLBACK childTimeout(HWND wnd, UINT msg, UINT_PTR timer, DWORD time)
{
    trace("childTimeout called\n");

    PostQuitMessage(0);
}

static void doChild(int argc, char** argv)
{
    char *filename, longpath[MAX_PATH] = "";
    HANDLE hFile, map;
    int i;
    int rc;
    HSZ hszApplication;
    UINT_PTR timer;
    HANDLE dde_ready;
    MSG msg;
    char *shared_block;

    filename=argv[2];
    hFile=CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    /* Arguments */
    childPrintf(hFile, "[Arguments]\r\n");
    if (winetest_debug > 2)
        trace("argcA=%d\n", argc);
    childPrintf(hFile, "argcA=%d\r\n", argc);
    for (i = 0; i < argc; i++)
    {
        if (winetest_debug > 2)
            trace("argvA%d=%s\n", i, argv[i]);
        childPrintf(hFile, "argvA%d=%s\r\n", i, encodeA(argv[i]));
    }
    GetModuleFileNameA(GetModuleHandleA(NULL), longpath, MAX_PATH);
    childPrintf(hFile, "longPath=%s\r\n", encodeA(longpath));

    map = OpenFileMappingA(FILE_MAP_READ, FALSE, "winetest_shlexec_dde_map");
    if (map != NULL)
    {
        shared_block = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 4096);
        CloseHandle(map);
        if (shared_block[0] != '\0' || shared_block[1] != '\0')
        {
            post_quit_on_execute = TRUE;
            ddeInst = 0;
            rc = DdeInitializeA(&ddeInst, ddeCb, CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_ADVISES |
                                CBF_FAIL_POKES | CBF_FAIL_REQUESTS, 0L);
            assert(rc == DMLERR_NO_ERROR);
            hszApplication = DdeCreateStringHandleA(ddeInst, shared_block, CP_WINANSI);
            hszTopic = DdeCreateStringHandleA(ddeInst, shared_block + strlen(shared_block) + 1, CP_WINANSI);
            assert(hszApplication && hszTopic);
            assert(DdeNameService(ddeInst, hszApplication, 0L, DNS_REGISTER | DNS_FILTEROFF));

            timer = SetTimer(NULL, 0, 2500, childTimeout);

            dde_ready = OpenEvent(EVENT_MODIFY_STATE, FALSE, "winetest_shlexec_dde_ready");
            SetEvent(dde_ready);
            CloseHandle(dde_ready);

            while (GetMessage(&msg, NULL, 0, 0))
                DispatchMessage(&msg);

            Sleep(500);
            KillTimer(NULL, timer);
            assert(DdeNameService(ddeInst, hszApplication, 0L, DNS_UNREGISTER));
            assert(DdeFreeStringHandle(ddeInst, hszTopic));
            assert(DdeFreeStringHandle(ddeInst, hszApplication));
            assert(DdeUninitialize(ddeInst));
        }
        else
        {
            dde_ready = OpenEvent(EVENT_MODIFY_STATE, FALSE, "winetest_shlexec_dde_ready");
            SetEvent(dde_ready);
            CloseHandle(dde_ready);
        }

        UnmapViewOfFile(shared_block);

        childPrintf(hFile, "ddeExec=%s\r\n", encodeA(ddeExec));
    }

    CloseHandle(hFile);

    init_event(filename);
    SetEvent(hEvent);
    CloseHandle(hEvent);
}

static char* getChildString(const char* sect, const char* key)
{
    char        buf[1024];
    char*       ret;

    GetPrivateProfileStringA(sect, key, "-", buf, sizeof(buf), child_file);
    if (buf[0] == '\0' || (buf[0] == '-' && buf[1] == '\0')) return NULL;
    assert(!(strlen(buf) & 1));
    ret = decodeA(buf);
    return ret;
}

static void dump_child(void)
{
    if (winetest_debug > 1)
    {
        char key[18];
        char* str;
        int i, c;

        c=GetPrivateProfileIntA("Arguments", "argcA", -1, child_file);
        trace("argcA=%d\n",c);
        for (i=0;i<c;i++)
        {
            sprintf(key, "argvA%d", i);
            str=getChildString("Arguments", key);
            trace("%s=%s\n", key, str);
        }
    }
}

static int StrCmpPath(const char* s1, const char* s2)
{
    if (!s1 && !s2) return 0;
    if (!s2) return 1;
    if (!s1) return -1;
    while (*s1)
    {
        if (!*s2)
        {
            if (*s1=='.')
                s1++;
            return (*s1-*s2);
        }
        if ((*s1=='/' || *s1=='\\') && (*s2=='/' || *s2=='\\'))
        {
            while (*s1=='/' || *s1=='\\')
                s1++;
            while (*s2=='/' || *s2=='\\')
                s2++;
        }
        else if (toupper(*s1)==toupper(*s2))
        {
            s1++;
            s2++;
        }
        else
        {
            return (*s1-*s2);
        }
    }
    if (*s2=='.')
        s2++;
    if (*s2)
        return -1;
    return 0;
}

static void _okChildString(const char* file, int line, const char* key, const char* expected)
{
    char* result;
    result=getChildString("Arguments", key);
    if (!result)
    {
        ok_(file, line)(FALSE, "%s expected '%s', but key not found or empty\n", key, expected);
        return;
    }
    ok_(file, line)(lstrcmpiA(result, expected) == 0,
                    "%s expected '%s', got '%s'\n", key, expected, result);
}

static void _okChildPath(const char* file, int line, const char* key, const char* expected)
{
    char* result;
    result=getChildString("Arguments", key);
    if (!result)
    {
        ok_(file, line)(FALSE, "%s expected '%s', but key not found or empty\n", key, expected);
        return;
    }
    ok_(file, line)(StrCmpPath(result, expected) == 0,
                    "%s expected '%s', got '%s'\n", key, expected, result);
}

static void _okChildInt(const char* file, int line, const char* key, int expected)
{
    INT result;
    result=GetPrivateProfileIntA("Arguments", key, expected, child_file);
    ok_(file, line)(result == expected,
                    "%s expected %d, but got %d\n", key, expected, result);
}

#define okChildString(key, expected) _okChildString(__FILE__, __LINE__, (key), (expected))
#define okChildPath(key, expected) _okChildPath(__FILE__, __LINE__, (key), (expected))
#define okChildInt(key, expected)    _okChildInt(__FILE__, __LINE__, (key), (expected))

/***
 *
 * GetLongPathNameA equivalent that supports Win95 and WinNT
 *
 ***/

static DWORD get_long_path_name(const char* shortpath, char* longpath, DWORD longlen)
{
    char tmplongpath[MAX_PATH];
    const char* p;
    DWORD sp = 0, lp = 0;
    DWORD tmplen;
    WIN32_FIND_DATAA wfd;
    HANDLE goit;

    if (!shortpath || !shortpath[0])
        return 0;

    if (shortpath[1] == ':')
    {
        tmplongpath[0] = shortpath[0];
        tmplongpath[1] = ':';
        lp = sp = 2;
    }

    while (shortpath[sp])
    {
        /* check for path delimiters and reproduce them */
        if (shortpath[sp] == '\\' || shortpath[sp] == '/')
        {
            if (!lp || tmplongpath[lp-1] != '\\')
            {
                /* strip double "\\" */
                tmplongpath[lp++] = '\\';
            }
            tmplongpath[lp] = 0; /* terminate string */
            sp++;
            continue;
        }

        p = shortpath + sp;
        if (sp == 0 && p[0] == '.' && (p[1] == '/' || p[1] == '\\'))
        {
            tmplongpath[lp++] = *p++;
            tmplongpath[lp++] = *p++;
        }
        for (; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (shortpath + sp);
        lstrcpyn(tmplongpath + lp, shortpath + sp, tmplen + 1);
        /* Check if the file exists and use the existing file name */
        goit = FindFirstFileA(tmplongpath, &wfd);
        if (goit == INVALID_HANDLE_VALUE)
            return 0;
        FindClose(goit);
        strcpy(tmplongpath + lp, wfd.cFileName);
        lp += strlen(tmplongpath + lp);
        sp += tmplen;
    }
    tmplen = strlen(shortpath) - 1;
    if ((shortpath[tmplen] == '/' || shortpath[tmplen] == '\\') &&
        (tmplongpath[lp - 1] != '/' && tmplongpath[lp - 1] != '\\'))
        tmplongpath[lp++] = shortpath[tmplen];
    tmplongpath[lp] = 0;

    tmplen = strlen(tmplongpath) + 1;
    if (tmplen <= longlen)
    {
        strcpy(longpath, tmplongpath);
        tmplen--; /* length without 0 */
    }

    return tmplen;
}

/***
 *
 * PathFindFileNameA equivalent that supports WinNT
 *
 ***/

static LPSTR path_find_file_name(LPCSTR lpszPath)
{
  LPCSTR lastSlash = lpszPath;

  while (lpszPath && *lpszPath)
  {
    if ((*lpszPath == '\\' || *lpszPath == '/' || *lpszPath == ':') &&
        lpszPath[1] && lpszPath[1] != '\\' && lpszPath[1] != '/')
      lastSlash = lpszPath + 1;
    lpszPath = CharNext(lpszPath);
  }
  return (LPSTR)lastSlash;
}

/***
 *
 * Tests
 *
 ***/

static const char* testfiles[]=
{
    "%s\\test file.shlexec",
    "%s\\%%nasty%% $file.shlexec",
    "%s\\test file.noassoc",
    "%s\\test file.noassoc.shlexec",
    "%s\\test file.shlexec.noassoc",
    "%s\\test_shortcut_shlexec.lnk",
    "%s\\test_shortcut_exe.lnk",
    "%s\\test file.shl",
    "%s\\test file.shlfoo",
    "%s\\test file.sfe",
    "%s\\masked file.shlexec",
    "%s\\masked",
    "%s\\test file.sde",
    "%s\\test file.exe",
    "%s\\test2.exe",
    "%s\\simple.shlexec",
    "%s\\drawback_file.noassoc",
    "%s\\drawback_file.noassoc foo.shlexec",
    "%s\\drawback_nonexist.noassoc foo.shlexec",
    NULL
};

typedef struct
{
    const char* verb;
    const char* basename;
    int todo;
    int rc;
} filename_tests_t;

static filename_tests_t filename_tests[]=
{
    /* Test bad / nonexistent filenames */
    {NULL,           "%s\\nonexistent.shlexec", 0x0, SE_ERR_FNF},
    {NULL,           "%s\\nonexistent.noassoc", 0x0, SE_ERR_FNF},

    /* Standard tests */
    {NULL,           "%s\\test file.shlexec",   0x0, 33},
    {NULL,           "%s\\test file.shlexec.",  0x0, 33},
    {NULL,           "%s\\%%nasty%% $file.shlexec", 0x0, 33},
    {NULL,           "%s/test file.shlexec",    0x0, 33},

    /* Test filenames with no association */
    {NULL,           "%s\\test file.noassoc",   0x0,  SE_ERR_NOASSOC},

    /* Test double extensions */
    {NULL,           "%s\\test file.noassoc.shlexec", 0x0, 33},
    {NULL,           "%s\\test file.shlexec.noassoc", 0x0, SE_ERR_NOASSOC},

    /* Test alternate verbs */
    {"LowerL",       "%s\\nonexistent.shlexec", 0x0, SE_ERR_FNF},
    {"LowerL",       "%s\\test file.noassoc",   0x0,  SE_ERR_NOASSOC},

    {"QuotedLowerL", "%s\\test file.shlexec",   0x0, 33},
    {"QuotedUpperL", "%s\\test file.shlexec",   0x0, 33},

    /* Test file masked due to space */
    {NULL,           "%s\\masked file.shlexec",   0x1, 33},
    /* Test if quoting prevents the masking */
    {NULL,           "%s\\masked file.shlexec",   0x40, 33},

    {NULL, NULL, 0}
};

static filename_tests_t noquotes_tests[]=
{
    /* Test unquoted '%1' thingies */
    {"NoQuotes",     "%s\\test file.shlexec",   0xa, 33},
    {"LowerL",       "%s\\test file.shlexec",   0xa, 33},
    {"UpperL",       "%s\\test file.shlexec",   0xa, 33},

    {NULL, NULL, 0}
};

static void test_lpFile_parsed(void)
{
    /* basename tmpdir */
    const char* shorttmpdir;

    const char *testfile;
    char fileA[MAX_PATH];

    int rc;

    GetTempPathA(sizeof(fileA), fileA);
    shorttmpdir = tmpdir + strlen(fileA);

    /* ensure tmpdir is in %TEMP%: GetTempPath() can succeed even if TEMP is undefined */
    SetEnvironmentVariableA("TEMP", fileA);

    /* existing "drawback_file.noassoc" prevents finding "drawback_file.noassoc foo.shlexec" on wine */
    testfile = "%s\\drawback_file.noassoc foo.shlexec";
    sprintf(fileA, testfile, tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    todo_wine {
        ok(rc>32,
            "expected success (33), got %s (%d), lpFile: %s\n",
            rc > 32 ? "success" : "failure", rc, fileA
            );
    }

    /* if quoted, existing "drawback_file.noassoc" not prevents finding "drawback_file.noassoc foo.shlexec" on wine */
    testfile = "\"%s\\drawback_file.noassoc foo.shlexec\"";
    sprintf(fileA, testfile, tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    ok(rc>32 || broken(rc == 2) /* Win95/NT4 */,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );

    /* error should be 2, not 31 */
    testfile = "\"%s\\drawback_file.noassoc\" foo.shlexec";
    sprintf(fileA, testfile, tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    ok(rc==2,
        "expected failure (2), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );

    /* ""command"" not works on wine (and real win9x and w2k) */
    testfile = "\"\"%s\\simple.shlexec\"\"";
    sprintf(fileA, testfile, tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    todo_wine {
        ok(rc>32 || broken(rc == 2) /* Win9x/2000 */,
            "expected success (33), got %s (%d), lpFile: %s\n",
            rc > 32 ? "success" : "failure", rc, fileA
            );
    }

    /* nonexisting "drawback_nonexist.noassoc" not prevents finding "drawback_nonexist.noassoc foo.shlexec" on wine */
    testfile = "%s\\drawback_nonexist.noassoc foo.shlexec";
    sprintf(fileA, testfile, tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );

    /* is SEE_MASK_DOENVSUBST default flag? Should only be when XP emulates 9x (XP bug or real 95 or ME behavior ?) */
    testfile = "%%TEMP%%\\%s\\simple.shlexec";
    sprintf(fileA, testfile, shorttmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    todo_wine {
        ok(rc==2,
            "expected failure (2), got %s (%d), lpFile: %s\n",
            rc > 32 ? "success" : "failure", rc, fileA
            );
    }

    /* quoted */
    testfile = "\"%%TEMP%%\\%s\\simple.shlexec\"";
    sprintf(fileA, testfile, shorttmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    todo_wine {
        ok(rc==2,
            "expected failure (2), got %s (%d), lpFile: %s\n",
            rc > 32 ? "success" : "failure", rc, fileA
            );
    }

    /* test SEE_MASK_DOENVSUBST works */
    testfile = "%%TEMP%%\\%s\\simple.shlexec";
    sprintf(fileA, testfile, shorttmpdir);
    rc=shell_execute_ex(SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI, NULL, fileA, NULL, NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );

    /* quoted lpFile not works only on real win95 and nt4 */
    testfile = "\"%%TEMP%%\\%s\\simple.shlexec\"";
    sprintf(fileA, testfile, shorttmpdir);
    rc=shell_execute_ex(SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI, NULL, fileA, NULL, NULL);
    ok(rc>32 || broken(rc == 2) /* Win95/NT4 */,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );

}

static void test_argify(void)
{
    char fileA[MAX_PATH];

    int rc;

    sprintf(fileA, "%s\\test file.shlexec", tmpdir);

    /* %2 */
    rc=shell_execute("NoQuotesParam2", fileA, "a b", NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );
    if (rc>32)
    {
        okChildInt("argcA", 5);
        okChildString("argvA4", "a");
    }

    /* %2 */
    /* '"a"""'   -> 'a"' */
    rc=shell_execute("NoQuotesParam2", fileA, "\"a:\"\"some string\"\"\"", NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );
    if (rc>32)
    {
        okChildInt("argcA", 5);
        todo_wine {
            okChildString("argvA4", "a:some string");
        }
    }

    /* %2 */
    /* backslash isn't escape char
     * '"a\""'   -> '"a\""' */
    rc=shell_execute("NoQuotesParam2", fileA, "\"a:\\\"some string\\\"\"", NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );
    if (rc>32)
    {
        okChildInt("argcA", 5);
        todo_wine {
            okChildString("argvA4", "a:\\");
        }
    }

    /* "%2" */
    /* \t isn't whitespace */
    rc=shell_execute("QuotedParam2", fileA, "a\tb c", NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );
    if (rc>32)
    {
        okChildInt("argcA", 5);
        todo_wine {
            okChildString("argvA4", "a\tb");
        }
    }

    /* %* */
    rc=shell_execute("NoQuotesAllParams", fileA, "a b c d e f g h", NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );
    if (rc>32)
    {
        todo_wine {
            okChildInt("argcA", 12);
            okChildString("argvA4", "a");
            okChildString("argvA11", "h");
        }
    }

    /* %* can sometimes contain only whitespaces and no args */
    rc=shell_execute("QuotedAllParams", fileA, "   ", NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );
    if (rc>32)
    {
        todo_wine {
            okChildInt("argcA", 5);
            okChildString("argvA4", "   ");
        }
    }

    /* %~3 */
    rc=shell_execute("NoQuotesParams345etc", fileA, "a b c d e f g h", NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );
    if (rc>32)
    {
        todo_wine {
            okChildInt("argcA", 11);
            okChildString("argvA4", "b");
            okChildString("argvA10", "h");
        }
    }

    /* %~3 is rest of command line starting with whitespaces after 2nd arg */
    rc=shell_execute("QuotedParams345etc", fileA, "a    ", NULL);
    ok(rc>32,
        "expected success (33), got %s (%d), lpFile: %s\n",
        rc > 32 ? "success" : "failure", rc, fileA
        );
    if (rc>32)
    {
        okChildInt("argcA", 5);
        todo_wine {
            okChildString("argvA4", "    ");
        }
    }

}

static void test_filename(void)
{
    char filename[MAX_PATH];
    const filename_tests_t* test;
    char* c;
    int rc;

    test=filename_tests;
    while (test->basename)
    {
        BOOL quotedfile = FALSE;

        if (skip_noassoc_tests && test->rc == SE_ERR_NOASSOC)
        {
            win_skip("Skipping shellexecute of file with unassociated extension\n");
            test++;
            continue;
        }

        sprintf(filename, test->basename, tmpdir);
        if (strchr(filename, '/'))
        {
            c=filename;
            while (*c)
            {
                if (*c=='\\')
                    *c='/';
                c++;
            }
        }
        if ((test->todo & 0x40)==0)
        {
            rc=shell_execute(test->verb, filename, NULL, NULL);
        }
        else
        {
            char quoted[MAX_PATH + 2];

            quotedfile = TRUE;
            sprintf(quoted, "\"%s\"", filename);
            rc=shell_execute(test->verb, quoted, NULL, NULL);
        }
        if (rc > 32)
            rc=33;
        if ((test->todo & 0x1)==0)
        {
            ok(rc==test->rc ||
               broken(quotedfile && rc == 2), /* NT4 */
               "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        else todo_wine
        {
            ok(rc==test->rc, "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        if (rc == 33)
        {
            const char* verb;
            if ((test->todo & 0x2)==0)
            {
                okChildInt("argcA", 5);
            }
            else todo_wine
            {
                okChildInt("argcA", 5);
            }
            verb=(test->verb ? test->verb : "Open");
            if ((test->todo & 0x4)==0)
            {
                okChildString("argvA3", verb);
            }
            else todo_wine
            {
                okChildString("argvA3", verb);
            }
            if ((test->todo & 0x8)==0)
            {
                okChildPath("argvA4", filename);
            }
            else todo_wine
            {
                okChildPath("argvA4", filename);
            }
        }
        test++;
    }

    test=noquotes_tests;
    while (test->basename)
    {
        sprintf(filename, test->basename, tmpdir);
        rc=shell_execute(test->verb, filename, NULL, NULL);
        if (rc > 32)
            rc=33;
        if ((test->todo & 0x1)==0)
        {
            ok(rc==test->rc, "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        else todo_wine
        {
            ok(rc==test->rc, "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        if (rc==0)
        {
            int count;
            const char* verb;
            char* str;

            verb=(test->verb ? test->verb : "Open");
            if ((test->todo & 0x4)==0)
            {
                okChildString("argvA3", verb);
            }
            else todo_wine
            {
                okChildString("argvA3", verb);
            }

            count=4;
            str=filename;
            while (1)
            {
                char attrib[18];
                char* space;
                space=strchr(str, ' ');
                if (space)
                    *space='\0';
                sprintf(attrib, "argvA%d", count);
                if ((test->todo & 0x8)==0)
                {
                    okChildPath(attrib, str);
                }
                else todo_wine
                {
                    okChildPath(attrib, str);
                }
                count++;
                if (!space)
                    break;
                str=space+1;
            }
            if ((test->todo & 0x2)==0)
            {
                okChildInt("argcA", count);
            }
            else todo_wine
            {
                okChildInt("argcA", count);
            }
        }
        test++;
    }

    if (dllver.dwMajorVersion != 0)
    {
        /* The more recent versions of shell32.dll accept quoted filenames
         * while older ones (e.g. 4.00) don't. Still we want to test this
         * because IE 6 depends on the new behavior.
         * One day we may need to check the exact version of the dll but for
         * now making sure DllGetVersion() is present is sufficient.
         */
        sprintf(filename, "\"%s\\test file.shlexec\"", tmpdir);
        rc=shell_execute(NULL, filename, NULL, NULL);
        ok(rc > 32, "%s failed: rc=%d err=%d\n", shell_call, rc,
           GetLastError());
        okChildInt("argcA", 5);
        okChildString("argvA3", "Open");
        sprintf(filename, "%s\\test file.shlexec", tmpdir);
        okChildPath("argvA4", filename);
    }
}

static void test_find_executable(void)
{
    char filename[MAX_PATH];
    char command[MAX_PATH];
    const filename_tests_t* test;
    INT_PTR rc;

    if (!create_test_association(".sfe"))
    {
        skip("Unable to create association for '.sfe'\n");
        return;
    }
    create_test_verb(".sfe", "Open", 1, "%1");

    /* Don't test FindExecutable(..., NULL), it always crashes */

    strcpy(command, "your word");
    if (0) /* Can crash on Vista! */
    {
    rc=(INT_PTR)FindExecutableA(NULL, NULL, command);
    ok(rc == SE_ERR_FNF || rc > 32 /* nt4 */, "FindExecutable(NULL) returned %ld\n", rc);
    ok(strcmp(command, "your word") != 0, "FindExecutable(NULL) returned command=[%s]\n", command);
    }

    strcpy(command, "your word");
    rc=(INT_PTR)FindExecutableA(tmpdir, NULL, command);
    ok(rc == SE_ERR_NOASSOC /* >= win2000 */ || rc > 32 /* win98, nt4 */, "FindExecutable(NULL) returned %ld\n", rc);
    ok(strcmp(command, "your word") != 0, "FindExecutable(NULL) returned command=[%s]\n", command);

    sprintf(filename, "%s\\test file.sfe", tmpdir);
    rc=(INT_PTR)FindExecutableA(filename, NULL, command);
    ok(rc > 32, "FindExecutable(%s) returned %ld\n", filename, rc);
    /* Depending on the platform, command could be '%1' or 'test file.sfe' */

    rc=(INT_PTR)FindExecutableA("test file.sfe", tmpdir, command);
    ok(rc > 32, "FindExecutable(%s) returned %ld\n", filename, rc);

    rc=(INT_PTR)FindExecutableA("test file.sfe", NULL, command);
    ok(rc == SE_ERR_FNF, "FindExecutable(%s) returned %ld\n", filename, rc);

    delete_test_association(".sfe");

    if (!create_test_association(".shl"))
    {
        skip("Unable to create association for '.shl'\n");
        return;
    }
    create_test_verb(".shl", "Open", 0, "Open");

    sprintf(filename, "%s\\test file.shl", tmpdir);
    rc=(INT_PTR)FindExecutableA(filename, NULL, command);
    ok(rc == SE_ERR_FNF /* NT4 */ || rc > 32, "FindExecutable(%s) returned %ld\n", filename, rc);

    sprintf(filename, "%s\\test file.shlfoo", tmpdir);
    rc=(INT_PTR)FindExecutableA(filename, NULL, command);

    delete_test_association(".shl");

    if (rc > 32)
    {
        /* On Windows XP and 2003 FindExecutable() is completely broken.
         * Probably what it does is convert the filename to 8.3 format,
         * which as a side effect converts the '.shlfoo' extension to '.shl',
         * and then tries to find an association for '.shl'. This means it
         * will normally fail on most extensions with more than 3 characters,
         * like '.mpeg', etc.
         * Also it means we cannot do any other test.
         */
        win_skip("FindExecutable() is broken -> not running 4+ character extension tests\n");
        return;
    }

    test=filename_tests;
    while (test->basename)
    {
        sprintf(filename, test->basename, tmpdir);
        if (strchr(filename, '/'))
        {
            char* c;
            c=filename;
            while (*c)
            {
                if (*c=='\\')
                    *c='/';
                c++;
            }
        }
        /* Win98 does not '\0'-terminate command! */
        memset(command, '\0', sizeof(command));
        rc=(INT_PTR)FindExecutableA(filename, NULL, command);
        if (rc > 32)
            rc=33;
        if ((test->todo & 0x10)==0)
        {
            ok(rc==test->rc, "FindExecutable(%s) failed: rc=%ld\n", filename, rc);
        }
        else todo_wine
        {
            ok(rc==test->rc, "FindExecutable(%s) failed: rc=%ld\n", filename, rc);
        }
        if (rc > 32)
        {
            int equal;
            equal=strcmp(command, argv0) == 0 ||
                /* NT4 returns an extra 0x8 character! */
                (strlen(command) == strlen(argv0)+1 && strncmp(command, argv0, strlen(argv0)) == 0);
            if ((test->todo & 0x20)==0)
            {
                ok(equal, "FindExecutable(%s) returned command='%s' instead of '%s'\n",
                   filename, command, argv0);
            }
            else todo_wine
            {
                ok(equal, "FindExecutable(%s) returned command='%s' instead of '%s'\n",
                   filename, command, argv0);
            }
        }
        test++;
    }
}


static filename_tests_t lnk_tests[]=
{
    /* Pass bad / nonexistent filenames as a parameter */
    {NULL, "%s\\nonexistent.shlexec",    0xa, 33},
    {NULL, "%s\\nonexistent.noassoc",    0xa, 33},

    /* Pass regular paths as a parameter */
    {NULL, "%s\\test file.shlexec",      0xa, 33},
    {NULL, "%s/%%nasty%% $file.shlexec", 0xa, 33},

    /* Pass filenames with no association as a parameter */
    {NULL, "%s\\test file.noassoc",      0xa, 33},

    {NULL, NULL, 0}
};

static void test_lnks(void)
{
    char filename[MAX_PATH];
    char params[MAX_PATH];
    const filename_tests_t* test;
    int rc;

    sprintf(filename, "%s\\test_shortcut_shlexec.lnk", tmpdir);
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, filename, NULL, NULL);
    ok(rc > 32, "%s failed: rc=%d err=%d\n", shell_call, rc,
       GetLastError());
    okChildInt("argcA", 5);
    okChildString("argvA3", "Open");
    sprintf(params, "%s\\test file.shlexec", tmpdir);
    get_long_path_name(params, filename, sizeof(filename));
    okChildPath("argvA4", filename);

    sprintf(filename, "%s\\test_shortcut_exe.lnk", tmpdir);
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, filename, NULL, NULL);
    ok(rc > 32, "%s failed: rc=%d err=%d\n", shell_call, rc,
       GetLastError());
    okChildInt("argcA", 4);
    okChildString("argvA3", "Lnk");

    if (dllver.dwMajorVersion>=6)
    {
        char* c;
       /* Recent versions of shell32.dll accept '/'s in shortcut paths.
         * Older versions don't or are quite buggy in this regard.
         */
        sprintf(filename, "%s\\test_shortcut_exe.lnk", tmpdir);
        c=filename;
        while (*c)
        {
            if (*c=='\\')
                *c='/';
            c++;
        }
        rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, filename, NULL, NULL);
        ok(rc > 32, "%s failed: rc=%d err=%d\n", shell_call, rc,
           GetLastError());
        okChildInt("argcA", 4);
        okChildString("argvA3", "Lnk");
    }

    sprintf(filename, "%s\\test_shortcut_exe.lnk", tmpdir);
    test=lnk_tests;
    while (test->basename)
    {
        params[0]='\"';
        sprintf(params+1, test->basename, tmpdir);
        strcat(params,"\"");
        rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, filename, params,
                            NULL);
        if (rc > 32)
            rc=33;
        if ((test->todo & 0x1)==0)
        {
            ok(rc==test->rc, "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        else todo_wine
        {
            ok(rc==test->rc, "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        if (rc==0)
        {
            if ((test->todo & 0x2)==0)
            {
                okChildInt("argcA", 5);
            }
            else
            {
                okChildInt("argcA", 5);
            }
            if ((test->todo & 0x4)==0)
            {
                okChildString("argvA3", "Lnk");
            }
            else todo_wine
            {
                okChildString("argvA3", "Lnk");
            }
            sprintf(params, test->basename, tmpdir);
            if ((test->todo & 0x8)==0)
            {
                okChildPath("argvA4", params);
            }
            else
            {
                okChildPath("argvA4", params);
            }
        }
        test++;
    }
}


static void test_exes(void)
{
    char filename[MAX_PATH];
    char params[1024];
    int rc;

    sprintf(params, "shlexec \"%s\" Exec", child_file);

    /* We need NOZONECHECKS on Win2003 to block a dialog */
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, argv0, params,
                        NULL);
    ok(rc > 32, "%s returned %d\n", shell_call, rc);
    okChildInt("argcA", 4);
    okChildString("argvA3", "Exec");

    if (! skip_noassoc_tests)
    {
        sprintf(filename, "%s\\test file.noassoc", tmpdir);
        if (CopyFile(argv0, filename, FALSE))
        {
            rc=shell_execute(NULL, filename, params, NULL);
            todo_wine {
                ok(rc==SE_ERR_NOASSOC, "%s succeeded: rc=%d\n", shell_call, rc);
            }
        }
    }
    else
    {
        win_skip("Skipping shellexecute of file with unassociated extension\n");
    }
}

static void test_exes_long(void)
{
    char filename[MAX_PATH];
    char params[2024];
    char longparam[MAX_PATH];
    int rc;

    for (rc = 0; rc < MAX_PATH; rc++)
        longparam[rc]='a'+rc%26;
    longparam[MAX_PATH-1]=0;


    sprintf(params, "shlexec \"%s\" %s", child_file,longparam);

    /* We need NOZONECHECKS on Win2003 to block a dialog */
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, argv0, params,
                        NULL);
    ok(rc > 32, "%s returned %d\n", shell_call, rc);
    okChildInt("argcA", 4);
    okChildString("argvA3", longparam);

    if (! skip_noassoc_tests)
    {
        sprintf(filename, "%s\\test file.noassoc", tmpdir);
        if (CopyFile(argv0, filename, FALSE))
        {
            rc=shell_execute(NULL, filename, params, NULL);
            todo_wine {
                ok(rc==SE_ERR_NOASSOC, "%s succeeded: rc=%d\n", shell_call, rc);
            }
        }
    }
    else
    {
        win_skip("Skipping shellexecute of file with unassociated extension\n");
    }
}

typedef struct
{
    const char* command;
    const char* ddeexec;
    const char* application;
    const char* topic;
    const char* ifexec;
    int expectedArgs;
    const char* expectedDdeExec;
    int todo;
} dde_tests_t;

static dde_tests_t dde_tests[] =
{
    /* Test passing and not passing command-line
     * argument, no DDE */
    {"", NULL, NULL, NULL, NULL, FALSE, "", 0x0},
    {"\"%1\"", NULL, NULL, NULL, NULL, TRUE, "", 0x0},

    /* Test passing and not passing command-line
     * argument, with DDE */
    {"", "[open(\"%1\")]", "shlexec", "dde", NULL, FALSE, "[open(\"%s\")]", 0x0},
    {"\"%1\"", "[open(\"%1\")]", "shlexec", "dde", NULL, TRUE, "[open(\"%s\")]", 0x0},

    /* Test unquoted %1 in command and ddeexec
     * (test filename has space) */
    {"%1", "[open(%1)]", "shlexec", "dde", NULL, 2, "[open(%s)]", 0x0},

    /* Test ifexec precedence over ddeexec */
    {"", "[open(\"%1\")]", "shlexec", "dde", "[ifexec(\"%1\")]", FALSE, "[ifexec(\"%s\")]", 0x0},

    /* Test default DDE topic */
    {"", "[open(\"%1\")]", "shlexec", NULL, NULL, FALSE, "[open(\"%s\")]", 0x0},

    /* Test default DDE application */
    {"", "[open(\"%1\")]", NULL, "dde", NULL, FALSE, "[open(\"%s\")]", 0x0},

    {NULL, NULL, NULL, NULL, NULL, 0, 0x0}
};

static DWORD WINAPI hooked_WaitForInputIdle(HANDLE process, DWORD timeout)
{
    return WaitForSingleObject(dde_ready_event, timeout);
}

/*
 * WaitForInputIdle() will normally return immediately for console apps. That's
 * a problem for us because ShellExecute will assume that an app is ready to
 * receive DDE messages after it has called WaitForInputIdle() on that app.
 * To work around that we install our own version of WaitForInputIdle() that
 * will wait for the child to explicitly tell us that it is ready. We do that
 * by changing the entry for WaitForInputIdle() in the shell32 import address
 * table.
 */
static void hook_WaitForInputIdle(void *new_func)
{
    char *base;
    PIMAGE_NT_HEADERS nt_headers;
    DWORD import_directory_rva;
    PIMAGE_IMPORT_DESCRIPTOR import_descriptor;

    base = (char *) GetModuleHandleA("shell32.dll");
    nt_headers = (PIMAGE_NT_HEADERS)(base + ((PIMAGE_DOS_HEADER) base)->e_lfanew);
    import_directory_rva = nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;

    /* Search for the correct imported module by walking the import descriptors */
    import_descriptor = (PIMAGE_IMPORT_DESCRIPTOR)(base + import_directory_rva);
    while (U(*import_descriptor).OriginalFirstThunk != 0)
    {
        char *import_module_name;

        import_module_name = base + import_descriptor->Name;
        if (lstrcmpiA(import_module_name, "user32.dll") == 0 ||
            lstrcmpiA(import_module_name, "user32") == 0)
        {
            PIMAGE_THUNK_DATA int_entry;
            PIMAGE_THUNK_DATA iat_entry;

            /* The import name table and import address table are two parallel
             * arrays. We need the import name table to find the imported
             * routine and the import address table to patch the address, so
             * walk them side by side */
            int_entry = (PIMAGE_THUNK_DATA)(base + U(*import_descriptor).OriginalFirstThunk);
            iat_entry = (PIMAGE_THUNK_DATA)(base + import_descriptor->FirstThunk);
            while (int_entry->u1.Ordinal != 0)
            {
                if (! IMAGE_SNAP_BY_ORDINAL(int_entry->u1.Ordinal))
                {
                    PIMAGE_IMPORT_BY_NAME import_by_name;
                    import_by_name = (PIMAGE_IMPORT_BY_NAME)(base + int_entry->u1.AddressOfData);
                    if (lstrcmpA((char *) import_by_name->Name, "WaitForInputIdle") == 0)
                    {
                        /* Found the correct routine in the correct imported module. Patch it. */
                        DWORD old_prot;
                        VirtualProtect(&iat_entry->u1.Function, sizeof(ULONG_PTR), PAGE_READWRITE, &old_prot);
                        iat_entry->u1.Function = (ULONG_PTR) new_func;
                        VirtualProtect(&iat_entry->u1.Function, sizeof(ULONG_PTR), old_prot, &old_prot);
                        break;
                    }
                }
                int_entry++;
                iat_entry++;
            }
            break;
        }

        import_descriptor++;
    }
}

static void test_dde(void)
{
    char filename[MAX_PATH], defApplication[MAX_PATH];
    const dde_tests_t* test;
    char params[1024];
    int rc;
    HANDLE map;
    char *shared_block;

    hook_WaitForInputIdle((void *) hooked_WaitForInputIdle);

    sprintf(filename, "%s\\test file.sde", tmpdir);

    /* Default service is application name minus path and extension */
    strcpy(defApplication, strrchr(argv0, '\\')+1);
    *strchr(defApplication, '.') = 0;

    map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                             4096, "winetest_shlexec_dde_map");
    shared_block = MapViewOfFile(map, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 4096);

    test = dde_tests;
    while (test->command)
    {
        if (!create_test_association(".sde"))
        {
            skip("Unable to create association for '.sde'\n");
            return;
        }
        create_test_verb_dde(".sde", "Open", 0, test->command, test->ddeexec,
                             test->application, test->topic, test->ifexec);

        if (test->application != NULL || test->topic != NULL)
        {
            strcpy(shared_block, test->application ? test->application : defApplication);
            strcpy(shared_block + strlen(shared_block) + 1, test->topic ? test->topic : SZDDESYS_TOPIC);
        }
        else
        {
            shared_block[0] = '\0';
            shared_block[1] = '\0';
        }
        ddeExec[0] = 0;

        dde_ready_event = CreateEventA(NULL, FALSE, FALSE, "winetest_shlexec_dde_ready");
        rc = shell_execute_ex(SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI, NULL, filename, NULL, NULL);
        CloseHandle(dde_ready_event);
        if ((test->todo & 0x1)==0)
        {
            ok(32 < rc, "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        else todo_wine
        {
            ok(32 < rc, "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        if (32 < rc)
        {
            if ((test->todo & 0x2)==0)
            {
                okChildInt("argcA", test->expectedArgs + 3);
            }
            else todo_wine
            {
                okChildInt("argcA", test->expectedArgs + 3);
            }
            if (test->expectedArgs == 1)
            {
                if ((test->todo & 0x4) == 0)
                {
                    okChildPath("argvA3", filename);
                }
                else todo_wine
                {
                    okChildPath("argvA3", filename);
                }
            }
            if ((test->todo & 0x8) == 0)
            {
                sprintf(params, test->expectedDdeExec, filename);
                okChildPath("ddeExec", params);
            }
            else todo_wine
            {
                sprintf(params, test->expectedDdeExec, filename);
                okChildPath("ddeExec", params);
            }
        }

        delete_test_association(".sde");
        test++;
    }

    UnmapViewOfFile(shared_block);
    CloseHandle(map);
    hook_WaitForInputIdle((void *) WaitForInputIdle);
}

#define DDE_DEFAULT_APP_VARIANTS 2
typedef struct
{
    const char* command;
    const char* expectedDdeApplication[DDE_DEFAULT_APP_VARIANTS];
    int todo;
    int rc[DDE_DEFAULT_APP_VARIANTS];
} dde_default_app_tests_t;

static dde_default_app_tests_t dde_default_app_tests[] =
{
    /* Windows XP and 98 handle default DDE app names in different ways.
     * The application name we see in the first test determines the pattern
     * of application names and return codes we will look for. */

    /* Test unquoted existing filename with a space */
    {"%s\\test file.exe", {"test file", "test"}, 0x0, {33, 33}},
    {"%s\\test file.exe param", {"test file", "test"}, 0x0, {33, 33}},

    /* Test quoted existing filename with a space */
    {"\"%s\\test file.exe\"", {"test file", "test file"}, 0x0, {33, 33}},
    {"\"%s\\test file.exe\" param", {"test file", "test file"}, 0x0, {33, 33}},

    /* Test unquoted filename with a space that doesn't exist, but
     * test2.exe does */
    {"%s\\test2 file.exe", {"test2", "test2"}, 0x0, {33, 33}},
    {"%s\\test2 file.exe param", {"test2", "test2"}, 0x0, {33, 33}},

    /* Test quoted filename with a space that does not exist */
    {"\"%s\\test2 file.exe\"", {"", "test2 file"}, 0x0, {5, 33}},
    {"\"%s\\test2 file.exe\" param", {"", "test2 file"}, 0x0, {5, 33}},

    /* Test filename supplied without the extension */
    {"%s\\test2", {"test2", "test2"}, 0x0, {33, 33}},
    {"%s\\test2 param", {"test2", "test2"}, 0x0, {33, 33}},

    /* Test an unquoted nonexistent filename */
    {"%s\\notexist.exe", {"", "notexist"}, 0x0, {5, 33}},
    {"%s\\notexist.exe param", {"", "notexist"}, 0x0, {5, 33}},

    /* Test an application that will be found on the path */
    {"cmd", {"cmd", "cmd"}, 0x0, {33, 33}},
    {"cmd param", {"cmd", "cmd"}, 0x0, {33, 33}},

    /* Test an application that will not be found on the path */
    {"xyzwxyzwxyz", {"", "xyzwxyzwxyz"}, 0x0, {5, 33}},
    {"xyzwxyzwxyz param", {"", "xyzwxyzwxyz"}, 0x0, {5, 33}},

    {NULL, {NULL}, 0, {0}}
};

typedef struct
{
    char *filename;
    DWORD threadIdParent;
} dde_thread_info_t;

static DWORD CALLBACK ddeThread(LPVOID arg)
{
    dde_thread_info_t *info = arg;
    assert(info && info->filename);
    PostThreadMessage(info->threadIdParent,
                      WM_QUIT,
                      shell_execute_ex(SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI, NULL, info->filename, NULL, NULL),
                      0L);
    ExitThread(0);
}

static void test_dde_default_app(void)
{
    char filename[MAX_PATH];
    HSZ hszApplication;
    dde_thread_info_t info = { filename, GetCurrentThreadId() };
    const dde_default_app_tests_t* test;
    char params[1024];
    DWORD threadId;
    MSG msg;
    int rc, which = 0;

    post_quit_on_execute = FALSE;
    ddeInst = 0;
    rc = DdeInitializeA(&ddeInst, ddeCb, CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_ADVISES |
                        CBF_FAIL_POKES | CBF_FAIL_REQUESTS, 0L);
    assert(rc == DMLERR_NO_ERROR);

    sprintf(filename, "%s\\test file.sde", tmpdir);

    /* It is strictly not necessary to register an application name here, but wine's
     * DdeNameService implementation complains if 0L is passed instead of
     * hszApplication with DNS_FILTEROFF */
    hszApplication = DdeCreateStringHandleA(ddeInst, "shlexec", CP_WINANSI);
    hszTopic = DdeCreateStringHandleA(ddeInst, "shlexec", CP_WINANSI);
    assert(hszApplication && hszTopic);
    assert(DdeNameService(ddeInst, hszApplication, 0L, DNS_REGISTER | DNS_FILTEROFF));

    test = dde_default_app_tests;
    while (test->command)
    {
        if (!create_test_association(".sde"))
        {
            skip("Unable to create association for '.sde'\n");
            return;
        }
        sprintf(params, test->command, tmpdir);
        create_test_verb_dde(".sde", "Open", 1, params, "[test]", NULL,
                             "shlexec", NULL);
        ddeApplication[0] = 0;

        /* No application will be run as we will respond to the first DDE event,
         * so don't wait for it */
        SetEvent(hEvent);

        assert(CreateThread(NULL, 0, ddeThread, &info, 0, &threadId));
        while (GetMessage(&msg, NULL, 0, 0)) DispatchMessage(&msg);
        rc = msg.wParam > 32 ? 33 : msg.wParam;

        /* First test, find which set of test data we expect to see */
        if (test == dde_default_app_tests)
        {
            int i;
            for (i=0; i<DDE_DEFAULT_APP_VARIANTS; i++)
            {
                if (!strcmp(ddeApplication, test->expectedDdeApplication[i]))
                {
                    which = i;
                    break;
                }
            }
            if (i == DDE_DEFAULT_APP_VARIANTS)
                skip("Default DDE application test does not match any available results, using first expected data set.\n");
        }

        if ((test->todo & 0x1)==0)
        {
            ok(rc==test->rc[which], "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        else todo_wine
        {
            ok(rc==test->rc[which], "%s failed: rc=%d err=%d\n", shell_call,
               rc, GetLastError());
        }
        if (rc == 33)
        {
            if ((test->todo & 0x2)==0)
            {
                ok(!strcmp(ddeApplication, test->expectedDdeApplication[which]),
                   "Expected application '%s', got '%s'\n",
                   test->expectedDdeApplication[which], ddeApplication);
            }
            else todo_wine
            {
                ok(!strcmp(ddeApplication, test->expectedDdeApplication[which]),
                   "Expected application '%s', got '%s'\n",
                   test->expectedDdeApplication[which], ddeApplication);
            }
        }

        delete_test_association(".sde");
        test++;
    }

    assert(DdeNameService(ddeInst, hszApplication, 0L, DNS_UNREGISTER));
    assert(DdeFreeStringHandle(ddeInst, hszTopic));
    assert(DdeFreeStringHandle(ddeInst, hszApplication));
    assert(DdeUninitialize(ddeInst));
}

static void init_test(void)
{
    HMODULE hdll;
    HRESULT (WINAPI *pDllGetVersion)(DLLVERSIONINFO*);
    char filename[MAX_PATH];
    WCHAR lnkfile[MAX_PATH];
    char params[1024];
    const char* const * testfile;
    lnk_desc_t desc;
    DWORD rc;
    HRESULT r;

    hdll=GetModuleHandleA("shell32.dll");
    pDllGetVersion=(void*)GetProcAddress(hdll, "DllGetVersion");
    if (pDllGetVersion)
    {
        dllver.cbSize=sizeof(dllver);
        pDllGetVersion(&dllver);
        trace("major=%d minor=%d build=%d platform=%d\n",
              dllver.dwMajorVersion, dllver.dwMinorVersion,
              dllver.dwBuildNumber, dllver.dwPlatformID);
    }
    else
    {
        memset(&dllver, 0, sizeof(dllver));
    }

    r = CoInitialize(NULL);
    ok(r == S_OK, "CoInitialize failed (0x%08x)\n", r);
    if (FAILED(r))
        exit(1);

    rc=GetModuleFileName(NULL, argv0, sizeof(argv0));
    assert(rc!=0 && rc<sizeof(argv0));
    if (GetFileAttributes(argv0)==INVALID_FILE_ATTRIBUTES)
    {
        strcat(argv0, ".so");
        ok(GetFileAttributes(argv0)!=INVALID_FILE_ATTRIBUTES,
           "unable to find argv0!\n");
    }

    GetTempPathA(sizeof(filename), filename);
    GetTempFileNameA(filename, "wt", 0, tmpdir);
    DeleteFileA( tmpdir );
    rc = CreateDirectoryA( tmpdir, NULL );
    ok( rc, "failed to create %s err %u\n", tmpdir, GetLastError() );
    rc = GetTempFileNameA(tmpdir, "wt", 0, child_file);
    assert(rc != 0);
    init_event(child_file);

    /* Set up the test files */
    testfile=testfiles;
    while (*testfile)
    {
        HANDLE hfile;

        sprintf(filename, *testfile, tmpdir);
        hfile=CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile==INVALID_HANDLE_VALUE)
        {
            trace("unable to create '%s': err=%d\n", filename, GetLastError());
            assert(0);
        }
        CloseHandle(hfile);
        testfile++;
    }

    /* Setup the test shortcuts */
    sprintf(filename, "%s\\test_shortcut_shlexec.lnk", tmpdir);
    MultiByteToWideChar(CP_ACP, 0, filename, -1, lnkfile, sizeof(lnkfile)/sizeof(*lnkfile));
    desc.description=NULL;
    desc.workdir=NULL;
    sprintf(filename, "%s\\test file.shlexec", tmpdir);
    desc.path=filename;
    desc.pidl=NULL;
    desc.arguments="ignored";
    desc.showcmd=0;
    desc.icon=NULL;
    desc.icon_id=0;
    desc.hotkey=0;
    create_lnk(lnkfile, &desc, 0);

    sprintf(filename, "%s\\test_shortcut_exe.lnk", tmpdir);
    MultiByteToWideChar(CP_ACP, 0, filename, -1, lnkfile, sizeof(lnkfile)/sizeof(*lnkfile));
    desc.description=NULL;
    desc.workdir=NULL;
    desc.path=argv0;
    desc.pidl=NULL;
    sprintf(params, "shlexec \"%s\" Lnk", child_file);
    desc.arguments=params;
    desc.showcmd=0;
    desc.icon=NULL;
    desc.icon_id=0;
    desc.hotkey=0;
    create_lnk(lnkfile, &desc, 0);

    /* Create a basic association suitable for most tests */
    if (!create_test_association(".shlexec"))
    {
        skip("Unable to create association for '.shlexec'\n");
        return;
    }
    create_test_verb(".shlexec", "Open", 0, "Open \"%1\"");
    create_test_verb(".shlexec", "NoQuotes", 0, "NoQuotes %1");
    create_test_verb(".shlexec", "LowerL", 0, "LowerL %l");
    create_test_verb(".shlexec", "QuotedLowerL", 0, "QuotedLowerL \"%l\"");
    create_test_verb(".shlexec", "UpperL", 0, "UpperL %L");
    create_test_verb(".shlexec", "QuotedUpperL", 0, "QuotedUpperL \"%L\"");

    create_test_verb(".shlexec", "NoQuotesParam2", 0, "NoQuotesParam2 %2");
    create_test_verb(".shlexec", "QuotedParam2", 0, "QuotedParam2 \"%2\"");

    create_test_verb(".shlexec", "NoQuotesAllParams", 0, "NoQuotesAllParams %*");
    create_test_verb(".shlexec", "QuotedAllParams", 0, "QuotedAllParams \"%*\"");

    create_test_verb(".shlexec", "NoQuotesParams345etc", 0, "NoQuotesParams345etc %~3");
    create_test_verb(".shlexec", "QuotedParams345etc", 0, "QuotedParams345etc \"%~3\"");
}

static void cleanup_test(void)
{
    char filename[MAX_PATH];
    const char* const * testfile;

    /* Delete the test files */
    testfile=testfiles;
    while (*testfile)
    {
        sprintf(filename, *testfile, tmpdir);
        /* Make sure we can delete the files ('test file.noassoc' is read-only now) */
        SetFileAttributes(filename, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(filename);
        testfile++;
    }
    DeleteFile(child_file);
    RemoveDirectoryA(tmpdir);

    /* Delete the test association */
    delete_test_association(".shlexec");

    CloseHandle(hEvent);

    CoUninitialize();
}

static void test_commandline(void)
{
    static const WCHAR one[] = {'o','n','e',0};
    static const WCHAR two[] = {'t','w','o',0};
    static const WCHAR three[] = {'t','h','r','e','e',0};
    static const WCHAR four[] = {'f','o','u','r',0};

    static const WCHAR fmt1[] = {'%','s',' ','%','s',' ','%','s',' ','%','s',0};
    static const WCHAR fmt2[] = {' ','%','s',' ','%','s',' ','%','s',' ','%','s',0};
    static const WCHAR fmt3[] = {'%','s','=','%','s',' ','%','s','=','\"','%','s','\"',0};
    static const WCHAR fmt4[] = {'\"','%','s','\"',' ','\"','%','s',' ','%','s','\"',' ','%','s',0};
    static const WCHAR fmt5[] = {'\\','\"','%','s','\"',' ','%','s','=','\"','%','s','\\','\"',' ','\"','%','s','\\','\"',0};
    static const WCHAR fmt6[] = {0};

    static const WCHAR chkfmt1[] = {'%','s','=','%','s',0};
    static const WCHAR chkfmt2[] = {'%','s',' ','%','s',0};
    static const WCHAR chkfmt3[] = {'\\','\"','%','s','\"',0};
    static const WCHAR chkfmt4[] = {'%','s','=','%','s','\"',' ','%','s','\"',0};
    WCHAR cmdline[255];
    LPWSTR *args = (LPWSTR*)0xdeadcafe, pbuf;
    INT numargs = -1;
    size_t buflen;

    wsprintfW(cmdline,fmt1,one,two,three,four);
    args=CommandLineToArgvW(cmdline,&numargs);
    if (args == NULL && numargs == -1)
    {
        win_skip("CommandLineToArgvW not implemented, skipping\n");
        return;
    }
    ok(numargs == 4, "expected 4 args, got %i\n",numargs);
    ok(lstrcmpW(args[0],one)==0,"arg0 is not as expected\n");
    ok(lstrcmpW(args[1],two)==0,"arg1 is not as expected\n");
    ok(lstrcmpW(args[2],three)==0,"arg2 is not as expected\n");
    ok(lstrcmpW(args[3],four)==0,"arg3 is not as expected\n");

    wsprintfW(cmdline,fmt2,one,two,three,four);
    args=CommandLineToArgvW(cmdline,&numargs);
    ok(numargs == 5, "expected 5 args, got %i\n",numargs);
    ok(args[0][0]==0,"arg0 is not as expected\n");
    ok(lstrcmpW(args[1],one)==0,"arg1 is not as expected\n");
    ok(lstrcmpW(args[2],two)==0,"arg2 is not as expected\n");
    ok(lstrcmpW(args[3],three)==0,"arg3 is not as expected\n");
    ok(lstrcmpW(args[4],four)==0,"arg4 is not as expected\n");

    wsprintfW(cmdline,fmt3,one,two,three,four);
    args=CommandLineToArgvW(cmdline,&numargs);
    ok(numargs == 2, "expected 2 args, got %i\n",numargs);
    wsprintfW(cmdline,chkfmt1,one,two);
    ok(lstrcmpW(args[0],cmdline)==0,"arg0 is not as expected\n");
    wsprintfW(cmdline,chkfmt1,three,four);
    ok(lstrcmpW(args[1],cmdline)==0,"arg1 is not as expected\n");

    wsprintfW(cmdline,fmt4,one,two,three,four);
    args=CommandLineToArgvW(cmdline,&numargs);
    ok(numargs == 3, "expected 3 args, got %i\n",numargs);
    ok(lstrcmpW(args[0],one)==0,"arg0 is not as expected\n");
    wsprintfW(cmdline,chkfmt2,two,three);
    ok(lstrcmpW(args[1],cmdline)==0,"arg1 is not as expected\n");
    ok(lstrcmpW(args[2],four)==0,"arg2 is not as expected\n");

    wsprintfW(cmdline,fmt5,one,two,three,four);
    args=CommandLineToArgvW(cmdline,&numargs);
    ok(numargs == 2, "expected 2 args, got %i\n",numargs);
    wsprintfW(cmdline,chkfmt3,one);
    todo_wine ok(lstrcmpW(args[0],cmdline)==0,"arg0 is not as expected\n");
    wsprintfW(cmdline,chkfmt4,two,three,four);
    todo_wine ok(lstrcmpW(args[1],cmdline)==0,"arg1 is not as expected\n");

    wsprintfW(cmdline,fmt6);
    args=CommandLineToArgvW(cmdline,&numargs);
    ok(numargs == 1, "expected 1 args, got %i\n",numargs);
    if (numargs == 1) {
        buflen = max(lstrlenW(args[0])+1,256);
        pbuf = HeapAlloc(GetProcessHeap(), 0, buflen*sizeof(pbuf[0]));
        GetModuleFileNameW(NULL, pbuf, buflen);
        pbuf[buflen-1] = 0;
        /* check args[0] is module file name */
        ok(lstrcmpW(args[0],pbuf)==0, "wrong path to the current executable\n");
        HeapFree(GetProcessHeap(), 0, pbuf);
    }
}

static void test_directory(void)
{
    char path[MAX_PATH], newdir[MAX_PATH];
    char params[1024];
    int rc;

    /* copy this executable to a new folder and cd to it */
    sprintf(newdir, "%s\\newfolder", tmpdir);
    rc = CreateDirectoryA( newdir, NULL );
    ok( rc, "failed to create %s err %u\n", newdir, GetLastError() );
    sprintf(path, "%s\\%s", newdir, path_find_file_name(argv0));
    CopyFileA(argv0, path, FALSE);
    SetCurrentDirectory(tmpdir);

    sprintf(params, "shlexec \"%s\" Exec", child_file);

    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS|SEE_MASK_FLAG_NO_UI,
                        NULL, path_find_file_name(argv0), params, NULL);
    todo_wine ok(rc == SE_ERR_FNF, "%s returned %d\n", shell_call, rc);

    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS|SEE_MASK_FLAG_NO_UI,
                        NULL, path_find_file_name(argv0), params, newdir);
    ok(rc > 32, "%s returned %d\n", shell_call, rc);
    okChildInt("argcA", 4);
    okChildString("argvA3", "Exec");
    todo_wine okChildPath("longPath", path);

    DeleteFile(path);
    RemoveDirectoryA(newdir);
}

START_TEST(shlexec)
{

    myARGC = winetest_get_mainargs(&myARGV);
    if (myARGC >= 3)
    {
        doChild(myARGC, myARGV);
        exit(0);
    }

    init_test();

    test_argify();
    test_lpFile_parsed();
    test_filename();
    test_find_executable();
    test_lnks();
    test_exes();
    test_exes_long();
    test_dde();
    test_dde_default_app();
    test_commandline();
    test_directory();

    cleanup_test();
}
