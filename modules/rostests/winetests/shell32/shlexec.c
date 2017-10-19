/*
 * Unit test of the ShellExecute function.
 *
 * Copyright 2005, 2016 Francois Gouget for CodeWeavers
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
#include "ddeml.h"
#include "wine/test.h"

#include "shell32_test.h"


static char argv0[MAX_PATH];
static int myARGC;
static char** myARGV;
static char tmpdir[MAX_PATH];
static char child_file[MAX_PATH];
static DLLVERSIONINFO dllver;
static BOOL skip_shlexec_tests = FALSE;
static BOOL skip_noassoc_tests = FALSE;
static HANDLE dde_ready_event;


/***
 *
 * Helpers to read from / write to the child process results file.
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

static void WINETEST_PRINTF_ATTR(2,3) childPrintf(HANDLE h, const char* fmt, ...)
{
    va_list     valist;
    char        buffer[1024];
    DWORD       w;

    va_start(valist, fmt);
    vsprintf(buffer, fmt, valist);
    va_end(valist);
    WriteFile(h, buffer, strlen(buffer), &w, NULL);
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


/***
 *
 * Child code
 *
 ***/

#define CHILD_DDE_TIMEOUT 2500
static DWORD ddeInst;
static HSZ hszTopic;
static char ddeExec[MAX_PATH], ddeApplication[MAX_PATH];
static BOOL post_quit_on_execute;

/* Handle DDE for doChild() and test_dde_default_app() */
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
                size = DdeQueryStringA(ddeInst, hsz2, ddeApplication, MAX_PATH, CP_WINANSI);
                ok(size < MAX_PATH, "got size %d\n", size);
                assert(size < MAX_PATH);
                return (HDDEDATA)TRUE;
            }
            return (HDDEDATA)FALSE;

        case XTYP_EXECUTE:
            size = DdeGetData(hData, (LPBYTE)ddeExec, MAX_PATH, 0);
            ok(size < MAX_PATH, "got size %d\n", size);
            assert(size < MAX_PATH);
            DdeFreeDataHandle(hData);
            if (post_quit_on_execute)
                PostQuitMessage(0);
            return (HDDEDATA)DDE_FACK;

        default:
            return NULL;
    }
}

static HANDLE hEvent;
static void init_event(const char* child_file)
{
    char* event_name;
    event_name=strrchr(child_file, '\\')+1;
    hEvent=CreateEventA(NULL, FALSE, FALSE, event_name);
}

/*
 * This is just to make sure the child won't run forever stuck in a
 * GetMessage() loop when DDE fails for some reason.
 */
static void CALLBACK childTimeout(HWND wnd, UINT msg, UINT_PTR timer, DWORD time)
{
    trace("childTimeout called\n");

    PostQuitMessage(0);
}

static void doChild(int argc, char** argv)
{
    char *filename, buffer[MAX_PATH];
    HANDLE hFile, map;
    int i;
    UINT_PTR timer;

    filename=argv[2];
    hFile=CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return;

    /* Arguments */
    childPrintf(hFile, "[Child]\r\n");
    if (winetest_debug > 2)
    {
        trace("cmdlineA='%s'\n", GetCommandLineA());
        trace("argcA=%d\n", argc);
    }
    childPrintf(hFile, "cmdlineA=%s\r\n", encodeA(GetCommandLineA()));
    childPrintf(hFile, "argcA=%d\r\n", argc);
    for (i = 0; i < argc; i++)
    {
        if (winetest_debug > 2)
            trace("argvA%d='%s'\n", i, argv[i]);
        childPrintf(hFile, "argvA%d=%s\r\n", i, encodeA(argv[i]));
    }
    GetModuleFileNameA(GetModuleHandleA(NULL), buffer, sizeof(buffer));
    childPrintf(hFile, "longPath=%s\r\n", encodeA(buffer));

    /* Check environment variable inheritance */
    *buffer = '\0';
    SetLastError(0);
    GetEnvironmentVariableA("ShlexecVar", buffer, sizeof(buffer));
    childPrintf(hFile, "ShlexecVarLE=%d\r\n", GetLastError());
    childPrintf(hFile, "ShlexecVar=%s\r\n", encodeA(buffer));

    map = OpenFileMappingA(FILE_MAP_READ, FALSE, "winetest_shlexec_dde_map");
    if (map != NULL)
    {
        HANDLE dde_ready;
        char *shared_block = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 4096);
        CloseHandle(map);
        if (shared_block[0] != '\0' || shared_block[1] != '\0')
        {
            HDDEDATA hdde;
            HSZ hszApplication;
            MSG msg;
            UINT rc;

            post_quit_on_execute = TRUE;
            ddeInst = 0;
            rc = DdeInitializeA(&ddeInst, ddeCb, CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_ADVISES |
                                CBF_FAIL_POKES | CBF_FAIL_REQUESTS, 0);
            ok(rc == DMLERR_NO_ERROR, "DdeInitializeA() returned %d\n", rc);
            hszApplication = DdeCreateStringHandleA(ddeInst, shared_block, CP_WINANSI);
            ok(hszApplication != NULL, "DdeCreateStringHandleA(%s) = NULL\n", shared_block);
            shared_block += strlen(shared_block) + 1;
            hszTopic = DdeCreateStringHandleA(ddeInst, shared_block, CP_WINANSI);
            ok(hszTopic != NULL, "DdeCreateStringHandleA(%s) = NULL\n", shared_block);
            hdde = DdeNameService(ddeInst, hszApplication, 0, DNS_REGISTER | DNS_FILTEROFF);
            ok(hdde != NULL, "DdeNameService() failed le=%u\n", GetLastError());

            timer = SetTimer(NULL, 0, CHILD_DDE_TIMEOUT, childTimeout);

            dde_ready = OpenEventA(EVENT_MODIFY_STATE, FALSE, "winetest_shlexec_dde_ready");
            SetEvent(dde_ready);
            CloseHandle(dde_ready);

            while (GetMessageA(&msg, NULL, 0, 0))
            {
                if (winetest_debug > 2)
                    trace("msg %d lParam=%ld wParam=%lu\n", msg.message, msg.lParam, msg.wParam);
                DispatchMessageA(&msg);
            }

            Sleep(500);
            KillTimer(NULL, timer);
            hdde = DdeNameService(ddeInst, hszApplication, 0, DNS_UNREGISTER);
            ok(hdde != NULL, "DdeNameService() failed le=%u\n", GetLastError());
            ok(DdeFreeStringHandle(ddeInst, hszTopic), "DdeFreeStringHandle(topic)\n");
            ok(DdeFreeStringHandle(ddeInst, hszApplication), "DdeFreeStringHandle(application)\n");
            ok(DdeUninitialize(ddeInst), "DdeUninitialize() failed\n");
        }
        else
        {
            dde_ready = OpenEventA(EVENT_MODIFY_STATE, FALSE, "winetest_shlexec_dde_ready");
            SetEvent(dde_ready);
            CloseHandle(dde_ready);
        }

        UnmapViewOfFile(shared_block);

        childPrintf(hFile, "ddeExec=%s\r\n", encodeA(ddeExec));
    }

    childPrintf(hFile, "Failures=%d\r\n", winetest_get_failures());
    CloseHandle(hFile);

    init_event(filename);
    SetEvent(hEvent);
    CloseHandle(hEvent);
}

static void dump_child_(const char* file, int line)
{
    if (winetest_debug > 1)
    {
        char key[18];
        char* str;
        int i, c;

        str=getChildString("Child", "cmdlineA");
        trace_(file, line)("cmdlineA='%s'\n", str);
        c=GetPrivateProfileIntA("Child", "argcA", -1, child_file);
        trace_(file, line)("argcA=%d\n",c);
        for (i=0;i<c;i++)
        {
            sprintf(key, "argvA%d", i);
            str=getChildString("Child", key);
            trace_(file, line)("%s='%s'\n", key, str);
        }

        c=GetPrivateProfileIntA("Child", "ShlexecVarLE", -1, child_file);
        trace_(file, line)("ShlexecVarLE=%d\n", c);
        str=getChildString("Child", "ShlexecVar");
        trace_(file, line)("ShlexecVar='%s'\n", str);

        c=GetPrivateProfileIntA("Child", "Failures", -1, child_file);
        trace_(file, line)("Failures=%d\n", c);
    }
}


/***
 *
 * Helpers to check the ShellExecute() / child process results.
 *
 ***/

static char shell_call[2048];
static void WINETEST_PRINTF_ATTR(2,3) _okShell(int condition, const char *msg, ...)
{
    va_list valist;
    char buffer[2048];

    strcpy(buffer, shell_call);
    strcat(buffer, " ");
    va_start(valist, msg);
    vsprintf(buffer+strlen(buffer), msg, valist);
    va_end(valist);
    winetest_ok(condition, "%s", buffer);
}
#define okShell_(file, line) (winetest_set_location(file, line), 0) ? (void)0 : _okShell
#define okShell okShell_(__FILE__, __LINE__)

static char assoc_desc[2048];
static void reset_association_description(void)
{
    *assoc_desc = '\0';
}

static void okChildString_(const char* file, int line, const char* key, const char* expected, const char* bad)
{
    char* result;
    result=getChildString("Child", key);
    if (!result)
    {
        okShell_(file, line)(FALSE, "%s expected '%s', but key not found or empty\n", key, expected);
        return;
    }
    okShell_(file, line)(lstrcmpiA(result, expected) == 0 ||
                         broken(lstrcmpiA(result, bad) == 0),
                         "%s expected '%s', got '%s'\n", key, expected, result);
}
#define okChildString(key, expected) okChildString_(__FILE__, __LINE__, (key), (expected), (expected))
#define okChildStringBroken(key, expected, broken) okChildString_(__FILE__, __LINE__, (key), (expected), (broken))

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

static void okChildPath_(const char* file, int line, const char* key, const char* expected)
{
    char* result;
    int equal, shortequal;
    result=getChildString("Child", key);
    if (!result)
    {
        okShell_(file,line)(FALSE, "%s expected '%s', but key not found or empty\n", key, expected);
        return;
    }
    shortequal = FALSE;
    equal = (StrCmpPath(result, expected) == 0);
    if (!equal)
    {
        char altpath[MAX_PATH];
        DWORD rc = GetLongPathNameA(expected, altpath, sizeof(altpath));
        if (0 < rc && rc < sizeof(altpath))
            equal = (StrCmpPath(result, altpath) == 0);
        if (!equal)
        {
            rc = GetShortPathNameA(expected, altpath, sizeof(altpath));
            if (0 < rc && rc < sizeof(altpath))
                shortequal = (StrCmpPath(result, altpath) == 0);
        }
    }
    okShell_(file,line)(equal || broken(shortequal) /* XP SP1 */,
                        "%s expected '%s', got '%s'\n", key, expected, result);
}
#define okChildPath(key, expected) okChildPath_(__FILE__, __LINE__, (key), (expected))

static void okChildInt_(const char* file, int line, const char* key, int expected)
{
    INT result;
    result=GetPrivateProfileIntA("Child", key, expected, child_file);
    okShell_(file,line)(result == expected,
                        "%s expected %d, but got %d\n", key, expected, result);
}
#define okChildInt(key, expected) okChildInt_(__FILE__, __LINE__, (key), (expected))

static void okChildIntBroken_(const char* file, int line, const char* key, int expected)
{
    INT result;
    result=GetPrivateProfileIntA("Child", key, expected, child_file);
    okShell_(file,line)(result == expected || broken(result != expected),
                        "%s expected %d, but got %d\n", key, expected, result);
}
#define okChildIntBroken(key, expected) okChildIntBroken_(__FILE__, __LINE__, (key), (expected))


/***
 *
 * ShellExecute wrappers
 *
 ***/

static void strcat_param(char* str, const char* name, const char* param)
{
    if (param)
    {
        if (str[strlen(str)-1] == '"')
            strcat(str, ", ");
        strcat(str, name);
        strcat(str, "=\"");
        strcat(str, param);
        strcat(str, "\"");
    }
}

static int _todo_wait = 0;
#define todo_wait for (_todo_wait = 1; _todo_wait; _todo_wait = 0)

static int bad_shellexecute = 0;

static INT_PTR shell_execute_(const char* file, int line, LPCSTR verb, LPCSTR filename, LPCSTR parameters, LPCSTR directory)
{
    INT_PTR rc, rcEmpty = 0;

    if(!verb)
        rcEmpty = shell_execute_(file, line, "", filename, parameters, directory);

    strcpy(shell_call, "ShellExecute(");
    strcat_param(shell_call, "verb", verb);
    strcat_param(shell_call, "file", filename);
    strcat_param(shell_call, "params", parameters);
    strcat_param(shell_call, "dir", directory);
    strcat(shell_call, ")");
    strcat(shell_call, assoc_desc);
    if (winetest_debug > 1)
        trace_(file, line)("Called %s\n", shell_call);

    DeleteFileA(child_file);
    SetLastError(0xcafebabe);

    /* FIXME: We cannot use ShellExecuteEx() here because if there is no
     * association it displays the 'Open With' dialog and I could not find
     * a flag to prevent this.
     */
    rc=(INT_PTR)ShellExecuteA(NULL, verb, filename, parameters, directory, SW_HIDE);

    if (rc > 32)
    {
        int wait_rc;
        wait_rc=WaitForSingleObject(hEvent, 5000);
        if (wait_rc == WAIT_TIMEOUT)
        {
            HWND wnd = FindWindowA("#32770", "Windows");
            if (!wnd)
                wnd = FindWindowA("Shell_Flyout", "");
            if (wnd != NULL)
            {
                SendMessageA(wnd, WM_CLOSE, 0, 0);
                win_skip("Skipping shellexecute of file with unassociated extension\n");
                skip_noassoc_tests = TRUE;
                rc = SE_ERR_NOASSOC;
            }
        }
        todo_wine_if(_todo_wait)
            okShell_(file, line)(wait_rc==WAIT_OBJECT_0 || rc <= 32,
                                 "WaitForSingleObject returned %d\n", wait_rc);
    }
    /* The child process may have changed the result file, so let profile
     * functions know about it
     */
    WritePrivateProfileStringA(NULL, NULL, NULL, child_file);
    if (GetFileAttributesA(child_file) != INVALID_FILE_ATTRIBUTES)
    {
        int c;
        dump_child_(file, line);
        c = GetPrivateProfileIntA("Child", "Failures", -1, child_file);
        if (c > 0)
            winetest_add_failures(c);
        okChildInt_(file, line, "ShlexecVarLE", 0);
        okChildString_(file, line, "ShlexecVar", "Present", "Present");
    }

    if(!verb)
    {
        if (rc != rcEmpty && rcEmpty == SE_ERR_NOASSOC) /* NT4 */
            bad_shellexecute = 1;
        okShell_(file, line)(rc == rcEmpty ||
                             broken(rc != rcEmpty && rcEmpty == SE_ERR_NOASSOC) /* NT4 */,
                             "Got different return value with empty string: %lu %lu\n", rc, rcEmpty);
    }

    return rc;
}
#define shell_execute(verb, filename, parameters, directory) \
    shell_execute_(__FILE__, __LINE__, verb, filename, parameters, directory)

static INT_PTR shell_execute_ex_(const char* file, int line,
                                 DWORD mask, LPCSTR verb, LPCSTR filename,
                                 LPCSTR parameters, LPCSTR directory,
                                 LPCSTR class)
{
    char smask[11];
    SHELLEXECUTEINFOA sei;
    BOOL success;
    INT_PTR rc;

    /* Add some flags so we can wait for the child process */
    mask |= SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;

    strcpy(shell_call, "ShellExecuteEx(");
    sprintf(smask, "0x%x", mask);
    strcat_param(shell_call, "mask", smask);
    strcat_param(shell_call, "verb", verb);
    strcat_param(shell_call, "file", filename);
    strcat_param(shell_call, "params", parameters);
    strcat_param(shell_call, "dir", directory);
    strcat_param(shell_call, "class", class);
    strcat(shell_call, ")");
    strcat(shell_call, assoc_desc);
    if (winetest_debug > 1)
        trace_(file, line)("Called %s\n", shell_call);

    sei.cbSize=sizeof(sei);
    sei.fMask=mask;
    sei.hwnd=NULL;
    sei.lpVerb=verb;
    sei.lpFile=filename;
    sei.lpParameters=parameters;
    sei.lpDirectory=directory;
    sei.nShow=SW_SHOWNORMAL;
    sei.hInstApp=NULL; /* Out */
    sei.lpIDList=NULL;
    sei.lpClass=class;
    sei.hkeyClass=NULL;
    sei.dwHotKey=0;
    U(sei).hIcon=NULL;
    sei.hProcess=(HANDLE)0xdeadbeef; /* Out */

    DeleteFileA(child_file);
    SetLastError(0xcafebabe);
    success=ShellExecuteExA(&sei);
    rc=(INT_PTR)sei.hInstApp;
    okShell_(file, line)((success && rc > 32) || (!success && rc <= 32),
                         "rc=%d and hInstApp=%ld is not allowed\n",
                         success, rc);

    if (rc > 32)
    {
        DWORD wait_rc, rc;
        if (sei.hProcess!=NULL)
        {
            wait_rc=WaitForSingleObject(sei.hProcess, 5000);
            okShell_(file, line)(wait_rc==WAIT_OBJECT_0,
                                 "WaitForSingleObject(hProcess) returned %d\n",
                                 wait_rc);
            wait_rc = GetExitCodeProcess(sei.hProcess, &rc);
            okShell_(file, line)(wait_rc, "GetExitCodeProcess() failed le=%u\n", GetLastError());
            todo_wine_if(_todo_wait)
                okShell_(file, line)(rc == 0, "child returned %u\n", rc);
            CloseHandle(sei.hProcess);
        }
        wait_rc=WaitForSingleObject(hEvent, 5000);
        todo_wine_if(_todo_wait)
            okShell_(file, line)(wait_rc==WAIT_OBJECT_0,
                                 "WaitForSingleObject returned %d\n", wait_rc);
    }
    else
        okShell_(file, line)(sei.hProcess==NULL,
                             "returned a process handle %p\n", sei.hProcess);

    /* The child process may have changed the result file, so let profile
     * functions know about it
     */
    WritePrivateProfileStringA(NULL, NULL, NULL, child_file);
    if (rc > 32 && GetFileAttributesA(child_file) != INVALID_FILE_ATTRIBUTES)
    {
        int c;
        dump_child_(file, line);
        c = GetPrivateProfileIntA("Child", "Failures", -1, child_file);
        if (c > 0)
            winetest_add_failures(c);
        /* When NOZONECHECKS is specified the environment variables are not
         * inherited if the process does not have elevated privileges.
         */
        if ((mask & SEE_MASK_NOZONECHECKS) && skip_shlexec_tests)
        {
            okChildInt_(file, line, "ShlexecVarLE", 203);
            okChildString_(file, line, "ShlexecVar", "", "");
        }
        else
        {
            okChildInt_(file, line, "ShlexecVarLE", 0);
            okChildString_(file, line, "ShlexecVar", "Present", "Present");
        }
    }

    return rc;
}
#define shell_execute_ex(mask, verb, filename, parameters, directory, class) \
    shell_execute_ex_(__FILE__, __LINE__, mask, verb, filename, parameters, directory, class)


/***
 *
 * Functions to create / delete associations wrappers
 *
 ***/

static BOOL create_test_class(const char* class, BOOL protocol)
{
    HKEY hkey, hkey_shell;
    LONG rc;

    rc = RegCreateKeyExA(HKEY_CLASSES_ROOT, class, 0, NULL, 0,
                         KEY_CREATE_SUB_KEY | KEY_SET_VALUE, NULL,
                         &hkey, NULL);
    ok(rc == ERROR_SUCCESS || rc == ERROR_ACCESS_DENIED,
       "could not create class %s (rc=%d)\n", class, rc);
    if (rc != ERROR_SUCCESS)
        return FALSE;

    if (protocol)
    {
        rc = RegSetValueExA(hkey, "URL Protocol", 0, REG_SZ, (LPBYTE)"", 1);
        ok(rc == ERROR_SUCCESS, "RegSetValueEx '%s' failed, expected ERROR_SUCCESS, got %d\n", class, rc);
    }

    rc = RegCreateKeyExA(hkey, "shell", 0, NULL, 0,
                         KEY_CREATE_SUB_KEY, NULL, &hkey_shell, NULL);
    ok(rc == ERROR_SUCCESS, "RegCreateKeyEx 'shell' failed, expected ERROR_SUCCESS, got %d\n", rc);

    CloseHandle(hkey);
    CloseHandle(hkey_shell);
    return TRUE;
}

static BOOL create_test_association(const char* extension)
{
    HKEY hkey;
    char class[MAX_PATH];
    LONG rc;

    sprintf(class, "shlexec%s", extension);
    rc=RegCreateKeyExA(HKEY_CLASSES_ROOT, extension, 0, NULL, 0, KEY_SET_VALUE,
                       NULL, &hkey, NULL);
    ok(rc == ERROR_SUCCESS || rc == ERROR_ACCESS_DENIED,
       "could not create association %s (rc=%d)\n", class, rc);
    if (rc != ERROR_SUCCESS)
        return FALSE;

    rc=RegSetValueExA(hkey, NULL, 0, REG_SZ, (LPBYTE) class, strlen(class)+1);
    ok(rc==ERROR_SUCCESS, "RegSetValueEx '%s' failed, expected ERROR_SUCCESS, got %d\n", class, rc);
    CloseHandle(hkey);

    return create_test_class(class, FALSE);
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

static void delete_test_class(const char* classname)
{
    myRegDeleteTreeA(HKEY_CLASSES_ROOT, classname);
}

static void delete_test_association(const char* extension)
{
    char classname[MAX_PATH];

    sprintf(classname, "shlexec%s", extension);
    delete_test_class(classname);
    myRegDeleteTreeA(HKEY_CLASSES_ROOT, extension);
}

static void create_test_verb_dde(const char* classname, const char* verb,
                                 int rawcmd, const char* cmdtail, const char *ddeexec,
                                 const char *application, const char *topic,
                                 const char *ifexec)
{
    HKEY hkey_shell, hkey_verb, hkey_cmd;
    char shell[MAX_PATH];
    char* cmd;
    LONG rc;

    strcpy(assoc_desc, " Assoc ");
    strcat_param(assoc_desc, "class", classname);
    strcat_param(assoc_desc, "verb", verb);
    sprintf(shell, "%d", rawcmd);
    strcat_param(assoc_desc, "rawcmd", shell);
    strcat_param(assoc_desc, "cmdtail", cmdtail);
    strcat_param(assoc_desc, "ddeexec", ddeexec);
    strcat_param(assoc_desc, "app", application);
    strcat_param(assoc_desc, "topic", topic);
    strcat_param(assoc_desc, "ifexec", ifexec);

    sprintf(shell, "%s\\shell", classname);
    rc=RegOpenKeyExA(HKEY_CLASSES_ROOT, shell, 0,
                     KEY_CREATE_SUB_KEY, &hkey_shell);
    ok(rc == ERROR_SUCCESS, "%s key creation failed with %d\n", shell, rc);

    rc=RegCreateKeyExA(hkey_shell, verb, 0, NULL, 0, KEY_CREATE_SUB_KEY,
                       NULL, &hkey_verb, NULL);
    ok(rc == ERROR_SUCCESS, "%s verb key creation failed with %d\n", verb, rc);

    rc=RegCreateKeyExA(hkey_verb, "command", 0, NULL, 0, KEY_SET_VALUE,
                       NULL, &hkey_cmd, NULL);
    ok(rc == ERROR_SUCCESS, "\'command\' key creation failed with %d\n", rc);

    if (rawcmd)
    {
        rc=RegSetValueExA(hkey_cmd, NULL, 0, REG_SZ, (LPBYTE)cmdtail, strlen(cmdtail)+1);
    }
    else
    {
        cmd=HeapAlloc(GetProcessHeap(), 0, strlen(argv0)+10+strlen(child_file)+2+strlen(cmdtail)+1);
        sprintf(cmd,"%s shlexec \"%s\" %s", argv0, child_file, cmdtail);
        rc=RegSetValueExA(hkey_cmd, NULL, 0, REG_SZ, (LPBYTE)cmd, strlen(cmd)+1);
        ok(rc == ERROR_SUCCESS, "setting command failed with %d\n", rc);
        HeapFree(GetProcessHeap(), 0, cmd);
    }

    if (ddeexec)
    {
        HKEY hkey_ddeexec, hkey_application, hkey_topic, hkey_ifexec;

        rc=RegCreateKeyExA(hkey_verb, "ddeexec", 0, NULL, 0, KEY_SET_VALUE |
                           KEY_CREATE_SUB_KEY, NULL, &hkey_ddeexec, NULL);
        ok(rc == ERROR_SUCCESS, "\'ddeexec\' key creation failed with %d\n", rc);
        rc=RegSetValueExA(hkey_ddeexec, NULL, 0, REG_SZ, (LPBYTE)ddeexec,
                          strlen(ddeexec)+1);
        ok(rc == ERROR_SUCCESS, "set value failed with %d\n", rc);

        if (application)
        {
            rc=RegCreateKeyExA(hkey_ddeexec, "application", 0, NULL, 0, KEY_SET_VALUE,
                               NULL, &hkey_application, NULL);
            ok(rc == ERROR_SUCCESS, "\'application\' key creation failed with %d\n", rc);

            rc=RegSetValueExA(hkey_application, NULL, 0, REG_SZ, (LPBYTE)application,
                              strlen(application)+1);
            ok(rc == ERROR_SUCCESS, "set value failed with %d\n", rc);
            CloseHandle(hkey_application);
        }
        if (topic)
        {
            rc=RegCreateKeyExA(hkey_ddeexec, "topic", 0, NULL, 0, KEY_SET_VALUE,
                               NULL, &hkey_topic, NULL);
            ok(rc == ERROR_SUCCESS, "\'topic\' key creation failed with %d\n", rc);
            rc=RegSetValueExA(hkey_topic, NULL, 0, REG_SZ, (LPBYTE)topic,
                              strlen(topic)+1);
            ok(rc == ERROR_SUCCESS, "set value failed with %d\n", rc);
            CloseHandle(hkey_topic);
        }
        if (ifexec)
        {
            rc=RegCreateKeyExA(hkey_ddeexec, "ifexec", 0, NULL, 0, KEY_SET_VALUE,
                               NULL, &hkey_ifexec, NULL);
            ok(rc == ERROR_SUCCESS, "\'ifexec\' key creation failed with %d\n", rc);
            rc=RegSetValueExA(hkey_ifexec, NULL, 0, REG_SZ, (LPBYTE)ifexec,
                              strlen(ifexec)+1);
            ok(rc == ERROR_SUCCESS, "set value failed with %d\n", rc);
            CloseHandle(hkey_ifexec);
        }
        CloseHandle(hkey_ddeexec);
    }

    CloseHandle(hkey_shell);
    CloseHandle(hkey_verb);
    CloseHandle(hkey_cmd);
}

/* Creates a class' non-DDE test verb.
 * This function is meant to be used to create long term test verbs and thus
 * does not trace them.
 */
static void create_test_verb(const char* classname, const char* verb,
                             int rawcmd, const char* cmdtail)
{
    create_test_verb_dde(classname, verb, rawcmd, cmdtail, NULL, NULL,
                         NULL, NULL);
    reset_association_description();
}


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
        lstrcpynA(tmplongpath + lp, shortpath + sp, tmplen + 1);
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
    "%s\\test file.sha",
    "%s\\test file.sfe",
    "%s\\test file.shlproto",
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
    INT_PTR rc;
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

    {"notaverb",     "%s\\test file.shlexec",   0x10, SE_ERR_NOASSOC},

    {"averb",        "%s\\test file.sha",       0x10, 33},

    /* Test file masked due to space */
    {NULL,           "%s\\masked file.shlexec",   0x0, 33},
    /* Test if quoting prevents the masking */
    {NULL,           "%s\\masked file.shlexec",   0x40, 33},
    /* Test with incorrect quote */
    {NULL,           "\"%s\\masked file.shlexec",   0x0, SE_ERR_FNF},

    /* Test extension / URI protocol collision */
    {NULL,           "%s\\test file.shlproto",   0x0, SE_ERR_NOASSOC},

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
    char fileA[MAX_PATH];
    INT_PTR rc;

    if (skip_shlexec_tests)
    {
        skip("No filename parsing tests due to lack of .shlexec association\n");
        return;
    }

    /* existing "drawback_file.noassoc" prevents finding "drawback_file.noassoc foo.shlexec" on wine */
    sprintf(fileA, "%s\\drawback_file.noassoc foo.shlexec", tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    okShell(rc > 32, "failed: rc=%lu\n", rc);

    /* if quoted, existing "drawback_file.noassoc" not prevents finding "drawback_file.noassoc foo.shlexec" on wine */
    sprintf(fileA, "\"%s\\drawback_file.noassoc foo.shlexec\"", tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    okShell(rc > 32 || broken(rc == SE_ERR_FNF) /* Win95/NT4 */,
            "failed: rc=%lu\n", rc);

    /* error should be SE_ERR_FNF, not SE_ERR_NOASSOC */
    sprintf(fileA, "\"%s\\drawback_file.noassoc\" foo.shlexec", tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    okShell(rc == SE_ERR_FNF, "returned %lu\n", rc);

    /* ""command"" not works on wine (and real win9x and w2k) */
    sprintf(fileA, "\"\"%s\\simple.shlexec\"\"", tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    todo_wine okShell(rc > 32 || broken(rc == SE_ERR_FNF) /* Win9x/2000 */,
                      "failed: rc=%lu\n", rc);

    /* nonexisting "drawback_nonexist.noassoc" not prevents finding "drawback_nonexist.noassoc foo.shlexec" on wine */
    sprintf(fileA, "%s\\drawback_nonexist.noassoc foo.shlexec", tmpdir);
    rc=shell_execute(NULL, fileA, NULL, NULL);
    okShell(rc > 32, "failed: rc=%lu\n", rc);

    /* is SEE_MASK_DOENVSUBST default flag? Should only be when XP emulates 9x (XP bug or real 95 or ME behavior ?) */
    rc=shell_execute(NULL, "%TMPDIR%\\simple.shlexec", NULL, NULL);
    todo_wine okShell(rc == SE_ERR_FNF, "returned %lu\n", rc);

    /* quoted */
    rc=shell_execute(NULL, "\"%TMPDIR%\\simple.shlexec\"", NULL, NULL);
    todo_wine okShell(rc == SE_ERR_FNF, "returned %lu\n", rc);

    /* test SEE_MASK_DOENVSUBST works */
    rc=shell_execute_ex(SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI,
                        NULL, "%TMPDIR%\\simple.shlexec", NULL, NULL, NULL);
    okShell(rc > 32, "failed: rc=%lu\n", rc);

    /* quoted lpFile does not work on real win95 and nt4 */
    rc=shell_execute_ex(SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI,
                        NULL, "\"%TMPDIR%\\simple.shlexec\"", NULL, NULL, NULL);
    okShell(rc > 32 || broken(rc == SE_ERR_FNF) /* Win95/NT4 */,
            "failed: rc=%lu\n", rc);
}

typedef struct
{
    const char* cmd;
    const char* args[11];
    int todo;
} cmdline_tests_t;

static const cmdline_tests_t cmdline_tests[] =
{
    {"exe",
     {"exe", NULL}, 0},

    {"exe arg1 arg2 \"arg three\" 'four five` six\\ $even)",
     {"exe", "arg1", "arg2", "arg three", "'four", "five`", "six\\", "$even)", NULL}, 0},

    {"exe arg=1 arg-2 three\tfour\rfour\nfour ",
     {"exe", "arg=1", "arg-2", "three", "four\rfour\nfour", NULL}, 0},

    {"exe arg\"one\" \"second\"arg thirdarg ",
     {"exe", "argone", "secondarg", "thirdarg", NULL}, 0},

    /* Don't lose unclosed quoted arguments */
    {"exe arg1 \"unclosed",
     {"exe", "arg1", "unclosed", NULL}, 0},

    {"exe arg1 \"",
     {"exe", "arg1", "", NULL}, 0},

    /* cmd's metacharacters have no special meaning */
    {"exe \"one^\" \"arg\"&two three|four",
     {"exe", "one^", "arg&two", "three|four", NULL}, 0},

    /* Environment variables are not interpreted either */
    {"exe %TMPDIR% %2",
     {"exe", "%TMPDIR%", "%2", NULL}, 0},

    /* If not followed by a quote, backslashes go through as is */
    {"exe o\\ne t\\\\wo t\\\\\\ree f\\\\\\\\our ",
     {"exe", "o\\ne", "t\\\\wo", "t\\\\\\ree", "f\\\\\\\\our", NULL}, 0},

    {"exe \"o\\ne\" \"t\\\\wo\" \"t\\\\\\ree\" \"f\\\\\\\\our\" ",
     {"exe", "o\\ne", "t\\\\wo", "t\\\\\\ree", "f\\\\\\\\our", NULL}, 0},

    /* When followed by a quote their number is halved and the remainder
     * escapes the quote
     */
    {"exe \\\"one \\\\\"two\" \\\\\\\"three \\\\\\\\\"four\" end",
     {"exe", "\"one", "\\two", "\\\"three", "\\\\four", "end", NULL}, 0},

    {"exe \"one\\\" still\" \"two\\\\\" \"three\\\\\\\" still\" \"four\\\\\\\\\" end",
     {"exe", "one\" still", "two\\", "three\\\" still", "four\\\\", "end", NULL}, 0},

    /* One can put a quote in an unquoted string by tripling it, that is in
     * effect quoting it like so """ -> ". The general rule is as follows:
     * 3n   quotes -> n quotes
     * 3n+1 quotes -> n quotes plus start of a quoted string
     * 3n+2 quotes -> n quotes (plus an empty string from the remaining pair)
     * Nicely, when n is 0 we get the standard rules back.
     */
    {"exe two\"\"quotes next",
     {"exe", "twoquotes", "next", NULL}, 0},

    {"exe three\"\"\"quotes next",
     {"exe", "three\"quotes", "next", NULL}, 0},

    {"exe four\"\"\"\" quotes\" next 4%3=1",
     {"exe", "four\" quotes", "next", "4%3=1", NULL}, 0},

    {"exe five\"\"\"\"\"quotes next",
     {"exe", "five\"quotes", "next", NULL}, 0},

    {"exe six\"\"\"\"\"\"quotes next",
     {"exe", "six\"\"quotes", "next", NULL}, 0},

    {"exe seven\"\"\"\"\"\"\" quotes\" next 7%3=1",
     {"exe", "seven\"\" quotes", "next", "7%3=1", NULL}, 0},

    {"exe twelve\"\"\"\"\"\"\"\"\"\"\"\"quotes next",
     {"exe", "twelve\"\"\"\"quotes", "next", NULL}, 0},

    {"exe thirteen\"\"\"\"\"\"\"\"\"\"\"\"\" quotes\" next 13%3=1",
     {"exe", "thirteen\"\"\"\" quotes", "next", "13%3=1", NULL}, 0},

    /* Inside a quoted string the opening quote is added to the set of
     * consecutive quotes to get the effective quotes count. This gives:
     * 1+3n   quotes -> n quotes
     * 1+3n+1 quotes -> n quotes plus closes the quoted string
     * 1+3n+2 quotes -> n+1 quotes plus closes the quoted string
     */
    {"exe \"two\"\"quotes next",
     {"exe", "two\"quotes", "next", NULL}, 0},

    {"exe \"two\"\" next",
     {"exe", "two\"", "next", NULL}, 0},

    {"exe \"three\"\"\" quotes\" next 4%3=1",
     {"exe", "three\" quotes", "next", "4%3=1", NULL}, 0},

    {"exe \"four\"\"\"\"quotes next",
     {"exe", "four\"quotes", "next", NULL}, 0},

    {"exe \"five\"\"\"\"\"quotes next",
     {"exe", "five\"\"quotes", "next", NULL}, 0},

    {"exe \"six\"\"\"\"\"\" quotes\" next 7%3=1",
     {"exe", "six\"\" quotes", "next", "7%3=1", NULL}, 0},

    {"exe \"eleven\"\"\"\"\"\"\"\"\"\"\"quotes next",
     {"exe", "eleven\"\"\"\"quotes", "next", NULL}, 0},

    {"exe \"twelve\"\"\"\"\"\"\"\"\"\"\"\" quotes\" next 13%3=1",
     {"exe", "twelve\"\"\"\" quotes", "next", "13%3=1", NULL}, 0},

    /* Escaped consecutive quotes are fun */
    {"exe \"the crazy \\\\\"\"\"\\\\\" quotes",
     {"exe", "the crazy \\\"\\", "quotes", NULL}, 0},

    /* The executable path has its own rules!!!
     * - Backslashes have no special meaning.
     * - If the first character is a quote, then the second quote ends the
     *   executable path.
     * - The previous rule holds even if the next character is not a space!
     * - If the first character is not a quote, then quotes have no special
     *   meaning either and the executable path stops at the first space.
     * - The consecutive quotes rules don't apply either.
     * - Even if there is no space between the executable path and the first
     *   argument, the latter is parsed using the regular rules.
     */
    {"exe\"file\"path arg1",
     {"exe\"file\"path", "arg1", NULL}, 0},

    {"exe\"file\"path\targ1",
     {"exe\"file\"path", "arg1", NULL}, 0},

    {"exe\"path\\ arg1",
     {"exe\"path\\", "arg1", NULL}, 0},

    {"\\\"exe \"arg one\"",
     {"\\\"exe", "arg one", NULL}, 0},

    {"\"spaced exe\" \"next arg\"",
     {"spaced exe", "next arg", NULL}, 0},

    {"\"spaced exe\"\t\"next arg\"",
     {"spaced exe", "next arg", NULL}, 0},

    {"\"exe\"arg\" one\" argtwo",
     {"exe", "arg one", "argtwo", NULL}, 0},

    {"\"spaced exe\\\"arg1 arg2",
     {"spaced exe\\", "arg1", "arg2", NULL}, 0},

    {"\"two\"\" arg1 ",
     {"two", " arg1 ", NULL}, 0},

    {"\"three\"\"\" arg2",
     {"three", "", "arg2", NULL}, 0},

    {"\"four\"\"\"\"arg1",
     {"four", "\"arg1", NULL}, 0},

    /* If the first character is a space then the executable path is empty */
    {" \"arg\"one argtwo",
     {"", "argone", "argtwo", NULL}, 0},

    {NULL, {NULL}, 0}
};

static BOOL test_one_cmdline(const cmdline_tests_t* test)
{
    WCHAR cmdW[MAX_PATH], argW[MAX_PATH];
    LPWSTR *cl2a;
    int cl2a_count;
    LPWSTR *argsW;
    int i, count;

    /* trace("----- cmd='%s'\n", test->cmd); */
    MultiByteToWideChar(CP_ACP, 0, test->cmd, -1, cmdW, sizeof(cmdW)/sizeof(*cmdW));
    argsW = cl2a = CommandLineToArgvW(cmdW, &cl2a_count);
    if (argsW == NULL && cl2a_count == -1)
    {
        win_skip("CommandLineToArgvW not implemented, skipping\n");
        return FALSE;
    }
    ok(!argsW[cl2a_count] || broken(argsW[cl2a_count] != NULL) /* before Vista */,
       "expected NULL-terminated list of commandline arguments\n");

    count = 0;
    while (test->args[count])
        count++;
    todo_wine_if(test->todo & 0x1)
        ok(cl2a_count == count, "%s: expected %d arguments, but got %d\n", test->cmd, count, cl2a_count);

    for (i = 0; i < cl2a_count; i++)
    {
        if (i < count)
        {
            MultiByteToWideChar(CP_ACP, 0, test->args[i], -1, argW, sizeof(argW)/sizeof(*argW));
            todo_wine_if(test->todo & (1 << (i+4)))
                ok(!lstrcmpW(*argsW, argW), "%s: arg[%d] expected %s but got %s\n", test->cmd, i, wine_dbgstr_w(argW), wine_dbgstr_w(*argsW));
        }
        else todo_wine_if(test->todo & 0x1)
            ok(0, "%s: got extra arg[%d]=%s\n", test->cmd, i, wine_dbgstr_w(*argsW));
        argsW++;
    }
    LocalFree(cl2a);
    return TRUE;
}

static void test_commandline2argv(void)
{
    static const WCHAR exeW[] = {'e','x','e',0};
    const cmdline_tests_t* test;
    WCHAR strW[MAX_PATH];
    LPWSTR *args;
    int numargs;
    DWORD le;

    test = cmdline_tests;
    while (test->cmd)
    {
        if (!test_one_cmdline(test))
            return;
        test++;
    }

    SetLastError(0xdeadbeef);
    args = CommandLineToArgvW(exeW, NULL);
    le = GetLastError();
    ok(args == NULL && le == ERROR_INVALID_PARAMETER, "expected NULL with ERROR_INVALID_PARAMETER got %p with %u\n", args, le);

    SetLastError(0xdeadbeef);
    args = CommandLineToArgvW(NULL, NULL);
    le = GetLastError();
    ok(args == NULL && le == ERROR_INVALID_PARAMETER, "expected NULL with ERROR_INVALID_PARAMETER got %p with %u\n", args, le);

    *strW = 0;
    args = CommandLineToArgvW(strW, &numargs);
    ok(numargs == 1 || broken(numargs > 1), "expected 1 args, got %d\n", numargs);
    ok(!args || (!args[numargs] || broken(args[numargs] != NULL) /* before Vista */),
       "expected NULL-terminated list of commandline arguments\n");
    if (numargs == 1)
    {
        GetModuleFileNameW(NULL, strW, sizeof(strW)/sizeof(*strW));
        ok(!lstrcmpW(args[0], strW), "wrong path to the current executable: %s instead of %s\n", wine_dbgstr_w(args[0]), wine_dbgstr_w(strW));
    }
    if (args) LocalFree(args);
}

/* The goal here is to analyze how ShellExecute() builds the command that
 * will be run. The tricky part is that there are three transformation
 * steps between the 'parameters' string we pass to ShellExecute() and the
 * argument list we observe in the child process:
 * - The parsing of 'parameters' string into individual arguments. The tests
 *   show this is done differently from both CreateProcess() and
 *   CommandLineToArgv()!
 * - The way the command 'formatting directives' such as %1, %2, etc are
 *   handled.
 * - And the way the resulting command line is then parsed to yield the
 *   argument list we check.
 */
typedef struct
{
    const char* verb;
    const char* params;
    int todo;
    cmdline_tests_t cmd;
    cmdline_tests_t broken;
} argify_tests_t;

static const argify_tests_t argify_tests[] =
{
    /* Start with three simple parameters. Notice that one can reorder and
     * duplicate the parameters. Also notice how %* take the raw input
     * parameters string, including the trailing spaces, no matter what
     * arguments have already been used.
     */
    {"Params232S", "p2 p3 p4 ", 0xc2,
     {" p2 p3 \"p2\" \"p2 p3 p4 \"",
      {"", "p2", "p3", "p2", "p2 p3 p4 ", NULL}, 0}},

    /* Unquoted argument references like %2 don't automatically quote their
     * argument. Similarly, when they are quoted they don't escape the quotes
     * that their argument may contain.
     */
    {"Params232S", "\"p two\" p3 p4  ", 0x3f3,
     {" p two p3 \"p two\" \"\"p two\" p3 p4  \"",
      {"", "p", "two", "p3", "p two", "p", "two p3 p4  ", NULL}, 0}},

    /* Only single digits are supported so only %1 to %9. Shown here with %20
     * because %10 is a pain.
     */
    {"Params20", "p", 0,
     {" \"p0\"",
      {"", "p0", NULL}, 0}},

    /* Only (double-)quotes have a special meaning. */
    {"Params23456", "'p2 p3` p4\\ $even", 0x40,
     {" \"'p2\" \"p3`\" \"p4\\\" \"$even\" \"\"",
      {"", "'p2", "p3`", "p4\" $even \"", NULL}, 0}},

    {"Params23456", "p=2 p-3 p4\tp4\rp4\np4", 0x1c2,
     {" \"p=2\" \"p-3\" \"p4\tp4\rp4\np4\" \"\" \"\"",
      {"", "p=2", "p-3", "p4\tp4\rp4\np4", "", "", NULL}, 0}},

    /* In unquoted strings, quotes are treated are a parameter separator just
     * like spaces! However they can be doubled to get a literal quote.
     * Specifically:
     * 2n   quotes -> n quotes
     * 2n+1 quotes -> n quotes and a parameter separator
     */
    {"Params23456789", "one\"quote \"p four\" one\"quote p7", 0xff3,
     {" \"one\" \"quote\" \"p four\" \"one\" \"quote\" \"p7\" \"\" \"\"",
      {"", "one", "quote", "p four", "one", "quote", "p7", "", "", NULL}, 0}},

    {"Params23456789", "two\"\"quotes \"p three\" two\"\"quotes p5", 0xf2,
     {" \"two\"quotes\" \"p three\" \"two\"quotes\" \"p5\" \"\" \"\" \"\" \"\"",
      {"", "twoquotes p", "three twoquotes", "p5", "", "", "", "", NULL}, 0}},

    {"Params23456789", "three\"\"\"quotes \"p four\" three\"\"\"quotes p6", 0xff3,
     {" \"three\"\" \"quotes\" \"p four\" \"three\"\" \"quotes\" \"p6\" \"\" \"\"",
      {"", "three\"", "quotes", "p four", "three\"", "quotes", "p6", "", "", NULL}, 0}},

    {"Params23456789", "four\"\"\"\"quotes \"p three\" four\"\"\"\"quotes p5", 0xf3,
     {" \"four\"\"quotes\" \"p three\" \"four\"\"quotes\" \"p5\" \"\" \"\" \"\" \"\"",
      {"", "four\"quotes p", "three fourquotes p5 \"", "", "", "", NULL}, 0}},

    /* Quoted strings cannot be continued by tacking on a non space character
     * either.
     */
    {"Params23456", "\"p two\"p3 \"p four\"p5 p6", 0x1f3,
     {" \"p two\" \"p3\" \"p four\" \"p5\" \"p6\"",
      {"", "p two", "p3", "p four", "p5", "p6", NULL}, 0}},

    /* In quoted strings, the quotes are halved and an odd number closes the
     * string. Specifically:
     * 2n   quotes -> n quotes
     * 2n+1 quotes -> n quotes and closes the string and hence the parameter
     */
    {"Params23456789", "\"one q\"uote \"p four\" \"one q\"uote p7", 0xff3,
     {" \"one q\" \"uote\" \"p four\" \"one q\" \"uote\" \"p7\" \"\" \"\"",
      {"", "one q", "uote", "p four", "one q", "uote", "p7", "", "", NULL}, 0}},

    {"Params23456789", "\"two \"\" quotes\" \"p three\" \"two \"\" quotes\" p5", 0x1ff3,
     {" \"two \" quotes\" \"p three\" \"two \" quotes\" \"p5\" \"\" \"\" \"\" \"\"",
      {"", "two ", "quotes p", "three two", " quotes", "p5", "", "", "", "", NULL}, 0}},

    {"Params23456789", "\"three q\"\"\"uotes \"p four\" \"three q\"\"\"uotes p7", 0xff3,
     {" \"three q\"\" \"uotes\" \"p four\" \"three q\"\" \"uotes\" \"p7\" \"\" \"\"",
      {"", "three q\"", "uotes", "p four", "three q\"", "uotes", "p7", "", "", NULL}, 0}},

    {"Params23456789", "\"four \"\"\"\" quotes\" \"p three\" \"four \"\"\"\" quotes\" p5", 0xff3,
     {" \"four \"\" quotes\" \"p three\" \"four \"\" quotes\" \"p5\" \"\" \"\" \"\" \"\"",
      {"", "four \"", "quotes p", "three four", "", "quotes p5 \"", "", "", "", NULL}, 0}},

    /* The quoted string rules also apply to consecutive quotes at the start
     * of a parameter but don't count the opening quote!
     */
    {"Params23456789", "\"\"twoquotes \"p four\" \"\"twoquotes p7", 0xbf3,
     {" \"\" \"twoquotes\" \"p four\" \"\" \"twoquotes\" \"p7\" \"\" \"\"",
      {"", "", "twoquotes", "p four", "", "twoquotes", "p7", "", "", NULL}, 0}},

    {"Params23456789", "\"\"\"three quotes\" \"p three\" \"\"\"three quotes\" p5", 0x6f3,
     {" \"\"three quotes\" \"p three\" \"\"three quotes\" \"p5\" \"\" \"\" \"\" \"\"",
      {"", "three", "quotes p", "three \"three", "quotes p5 \"", "", "", "", NULL}, 0}},

    {"Params23456789", "\"\"\"\"fourquotes \"p four\" \"\"\"\"fourquotes p7", 0xbf3,
     {" \"\"\" \"fourquotes\" \"p four\" \"\"\" \"fourquotes\" \"p7\" \"\" \"\"",
      {"", "\"", "fourquotes", "p four", "\"", "fourquotes", "p7", "", "", NULL}, 0}},

    /* An unclosed quoted string gets lost! */
    {"Params23456", "p2 \"p3\" \"p4 is lost", 0x1c3,
     {" \"p2\" \"p3\" \"\" \"\" \"\"",
      {"", "p2", "p3", "", "", "", NULL}, 0},
     {" \"p2\" \"p3\" \"p3\" \"\" \"\"",
       {"", "p2", "p3", "p3", "", "", NULL}, 0}},

    /* Backslashes have no special meaning even when preceding quotes. All
     * they do is start an unquoted string.
     */
    {"Params23456", "\\\"p\\three \"pfour\\\" pfive", 0x73,
     {" \"\\\" \"p\\three\" \"pfour\\\" \"pfive\" \"\"",
      {"", "\" p\\three pfour\"", "pfive", "", NULL}, 0}},

    /* Environment variables are left untouched. */
    {"Params23456", "%TMPDIR% %t %c", 0,
     {" \"%TMPDIR%\" \"%t\" \"%c\" \"\" \"\"",
      {"", "%TMPDIR%", "%t", "%c", "", "", NULL}, 0}},

    /* %~2 is equivalent to %*. However %~3 and higher include the spaces
     * before the parameter!
     * (but not the previous parameter's closing quote fortunately)
     */
    {"Params2345Etc", "p2  p3 \"p4\"  p5 p6 ", 0x3f3,
     {" ~2=\"p2  p3 \"p4\"  p5 p6 \" ~3=\"  p3 \"p4\"  p5 p6 \" ~4=\" \"p4\"  p5 p6 \" ~5=  p5 p6 ",
      {"", "~2=p2  p3 p4  p5 p6 ", "~3=  p3 p4  p5 p6 ", "~4= p4  p5 p6 ", "~5=", "p5", "p6", NULL}, 0}},

    /* %~n works even if there is no nth parameter. */
    {"Params9Etc", "p2 p3 p4 p5 p6 p7 p8   ", 0x12,
     {" ~9=\"   \"",
      {"", "~9=   ", NULL}, 0}},

    {"Params9Etc", "p2 p3 p4 p5 p6 p7   ", 0x12,
     {" ~9=\"\"",
      {"", "~9=", NULL}, 0}},

    /* The %~n directives also transmit the tenth parameter and beyond. */
    {"Params9Etc", "p2 p3 p4 p5 p6 p7 p8 p9 p10 p11 and beyond!", 0x12,
     {" ~9=\" p9 p10 p11 and beyond!\"",
      {"", "~9= p9 p10 p11 and beyond!", NULL}, 0}},

    /* Bad formatting directives lose their % sign, except those followed by
     * a tilde! Environment variables are not expanded but lose their % sign.
     */
    {"ParamsBad", "p2 p3 p4 p5", 0x12,
     {" \"% - %~ %~0 %~1 %~a %~* a b c TMPDIR\"",
      {"", "% - %~ %~0 %~1 %~a %~* a b c TMPDIR", NULL}, 0}},

    {NULL, NULL, 0, {NULL, {NULL}, 0}}
};

static void test_argify(void)
{
    BOOL has_cl2a = TRUE;
    char fileA[MAX_PATH], params[2*MAX_PATH+12];
    INT_PTR rc;
    const argify_tests_t* test;
    const cmdline_tests_t *bad;
    const char* cmd;
    unsigned i, count;

    /* Test with a long parameter */
    for (rc = 0; rc < MAX_PATH; rc++)
        fileA[rc] = 'a' + rc % 26;
    fileA[MAX_PATH-1] = '\0';
    sprintf(params, "shlexec \"%s\" %s", child_file, fileA);

    /* We need NOZONECHECKS on Win2003 to block a dialog */
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, argv0, params, NULL, NULL);
    okShell(rc > 32, "failed: rc=%lu\n", rc);
    okChildInt("argcA", 4);
    okChildPath("argvA3", fileA);

    if (skip_shlexec_tests)
    {
        skip("No argify tests due to lack of .shlexec association\n");
        return;
    }

    create_test_verb("shlexec.shlexec", "Params232S", 0, "Params232S %2 %3 \"%2\" \"%*\"");
    create_test_verb("shlexec.shlexec", "Params23456", 0, "Params23456 \"%2\" \"%3\" \"%4\" \"%5\" \"%6\"");
    create_test_verb("shlexec.shlexec", "Params23456789", 0, "Params23456789 \"%2\" \"%3\" \"%4\" \"%5\" \"%6\" \"%7\" \"%8\" \"%9\"");
    create_test_verb("shlexec.shlexec", "Params2345Etc", 0, "Params2345Etc ~2=\"%~2\" ~3=\"%~3\" ~4=\"%~4\" ~5=%~5");
    create_test_verb("shlexec.shlexec", "Params9Etc", 0, "Params9Etc ~9=\"%~9\"");
    create_test_verb("shlexec.shlexec", "Params20", 0, "Params20 \"%20\"");
    create_test_verb("shlexec.shlexec", "ParamsBad", 0, "ParamsBad \"%% %- %~ %~0 %~1 %~a %~* %a %b %c %TMPDIR%\"");

    sprintf(fileA, "%s\\test file.shlexec", tmpdir);

    test = argify_tests;
    while (test->params)
    {
        bad = test->broken.cmd ? &test->broken : &test->cmd;

        /* trace("***** verb='%s' params='%s'\n", test->verb, test->params); */
        rc = shell_execute_ex(SEE_MASK_DOENVSUBST, test->verb, fileA, test->params, NULL, NULL);
        okShell(rc > 32, "failed: rc=%lu\n", rc);

        count = 0;
        while (test->cmd.args[count])
            count++;
        /* +4 for the shlexec arguments, -1 because of the added ""
         * argument for the CommandLineToArgvW() tests.
         */
        todo_wine_if(test->todo & 0x1)
            okChildInt("argcA", 4 + count - 1);

        cmd = getChildString("Child", "cmdlineA");
        /* Our commands are such that the verb immediately precedes the
         * part we are interested in.
         */
        if (cmd) cmd = strstr(cmd, test->verb);
        if (cmd) cmd += strlen(test->verb);
        if (!cmd) cmd = "(null)";
        todo_wine_if(test->todo & 0x2)
            okShell(!strcmp(cmd, test->cmd.cmd) || broken(!strcmp(cmd, bad->cmd)),
                    "the cmdline is '%s' instead of '%s'\n", cmd, test->cmd.cmd);

        for (i = 0; i < count - 1; i++)
        {
            char argname[18];
            sprintf(argname, "argvA%d", 4 + i);
            todo_wine_if(test->todo & (1 << (i+4)))
                okChildStringBroken(argname, test->cmd.args[i+1], bad->args[i+1]);
        }

        if (has_cl2a)
            has_cl2a = test_one_cmdline(&(test->cmd));
        test++;
    }
}

static void test_filename(void)
{
    char filename[MAX_PATH];
    const filename_tests_t* test;
    char* c;
    INT_PTR rc;

    if (skip_shlexec_tests)
    {
        skip("No ShellExecute/filename tests due to lack of .shlexec association\n");
        return;
    }

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
        okShell(rc==test->rc ||
                broken(quotedfile && rc == SE_ERR_FNF), /* NT4 */
                "failed: rc=%ld err=%u\n", rc, GetLastError());
        if (rc == 33)
        {
            const char* verb;
            todo_wine_if(test->todo & 0x2)
                okChildInt("argcA", 5);
            verb=(test->verb ? test->verb : "Open");
            todo_wine_if(test->todo & 0x4)
                okChildString("argvA3", verb);
            todo_wine_if(test->todo & 0x8)
                okChildPath("argvA4", filename);
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
        todo_wine_if(test->todo & 0x1)
            okShell(rc==test->rc, "failed: rc=%ld err=%u\n", rc, GetLastError());
        if (rc==0)
        {
            int count;
            const char* verb;
            char* str;

            verb=(test->verb ? test->verb : "Open");
            todo_wine_if(test->todo & 0x4)
                okChildString("argvA3", verb);

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
                todo_wine_if(test->todo & 0x8)
                    okChildPath(attrib, str);
                count++;
                if (!space)
                    break;
                str=space+1;
            }
            todo_wine_if(test->todo & 0x2)
                okChildInt("argcA", count);
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
        okShell(rc > 32, "failed: rc=%ld err=%u\n", rc, GetLastError());
        okChildInt("argcA", 5);
        okChildString("argvA3", "Open");
        sprintf(filename, "%s\\test file.shlexec", tmpdir);
        okChildPath("argvA4", filename);
    }

    sprintf(filename, "\"%s\\test file.sha\"", tmpdir);
    rc=shell_execute(NULL, filename, NULL, NULL);
    todo_wine okShell(rc > 32, "failed: rc=%ld err=%u\n", rc, GetLastError());
    okChildInt("argcA", 5);
    todo_wine okChildString("argvA3", "averb");
    sprintf(filename, "%s\\test file.sha", tmpdir);
    todo_wine okChildPath("argvA4", filename);
}

typedef struct
{
    const char* urlprefix;
    const char* basename;
    int flags;
    int todo;
} fileurl_tests_t;

#define URL_SUCCESS  0x1
#define USE_COLON    0x2
#define USE_BSLASH   0x4

static fileurl_tests_t fileurl_tests[]=
{
    /* How many slashes does it take... */
    {"file:", "%s\\test file.shlexec", URL_SUCCESS, 0},
    {"file:/", "%s\\test file.shlexec", URL_SUCCESS, 0},
    {"file://", "%s\\test file.shlexec", URL_SUCCESS, 0},
    {"file:///", "%s\\test file.shlexec", URL_SUCCESS, 0},
    {"File:///", "%s\\test file.shlexec", URL_SUCCESS, 0},
    {"file:////", "%s\\test file.shlexec", URL_SUCCESS, 0},
    {"file://///", "%s\\test file.shlexec", 0, 0},

    /* Test with Windows-style paths */
    {"file:///", "%s\\test file.shlexec", URL_SUCCESS | USE_COLON, 0},
    {"file:///", "%s\\test file.shlexec", URL_SUCCESS | USE_BSLASH, 0},

    /* Check handling of hostnames */
    {"file://localhost/", "%s\\test file.shlexec", URL_SUCCESS, 0},
    {"file://localhost:80/", "%s\\test file.shlexec", 0, 0},
    {"file://LocalHost/", "%s\\test file.shlexec", URL_SUCCESS, 0},
    {"file://127.0.0.1/", "%s\\test file.shlexec", 0, 0},
    {"file://::1/", "%s\\test file.shlexec", 0, 0},
    {"file://notahost/", "%s\\test file.shlexec", 0, 0},

    /* Environment variables are not expanded in URLs */
    {"%urlprefix%", "%s\\test file.shlexec", 0, 0x1},
    {"file:///", "%%TMPDIR%%\\test file.shlexec", 0, 0},

    /* Test shortcuts vs. URLs */
    {"file://///", "%s\\test_shortcut_shlexec.lnk", 0, 0x1c},

    /* Confuse things by mixing protocols */
    {"file://", "shlproto://foo/bar", USE_COLON, 0},

    {NULL, NULL, 0, 0}
};

static void test_fileurls(void)
{
    char filename[MAX_PATH], fileurl[MAX_PATH], longtmpdir[MAX_PATH];
    char command[MAX_PATH];
    const fileurl_tests_t* test;
    char *s;
    INT_PTR rc;

    if (skip_shlexec_tests)
    {
        skip("No file URL tests due to lack of .shlexec association\n");
        return;
    }

    rc = shell_execute_ex(SEE_MASK_FLAG_NO_UI, NULL,
                          "file:///nosuchfile.shlexec", NULL, NULL, NULL);
    if (rc > 32)
    {
        win_skip("shell32 is too old (likely < 4.72). Skipping the file URL tests\n");
        return;
    }

    get_long_path_name(tmpdir, longtmpdir, sizeof(longtmpdir)/sizeof(*longtmpdir));
    SetEnvironmentVariableA("urlprefix", "file:///");

    test=fileurl_tests;
    while (test->basename)
    {
        /* Build the file URL */
        sprintf(filename, test->basename, longtmpdir);
        strcpy(fileurl, test->urlprefix);
        strcat(fileurl, filename);
        s = fileurl + strlen(test->urlprefix);
        while (*s)
        {
            if (!(test->flags & USE_COLON) && *s == ':')
                *s = '|';
            else if (!(test->flags & USE_BSLASH) && *s == '\\')
                *s = '/';
            s++;
        }

        /* Test it first with FindExecutable() */
        rc = (INT_PTR)FindExecutableA(fileurl, NULL, command);
        ok(rc == SE_ERR_FNF, "FindExecutable(%s) failed: bad rc=%lu\n", fileurl, rc);

        /* Then ShellExecute() */
        if ((test->todo & 0x10) == 0)
            rc = shell_execute(NULL, fileurl, NULL, NULL);
        else todo_wait
            rc = shell_execute(NULL, fileurl, NULL, NULL);
        if (bad_shellexecute)
        {
            win_skip("shell32 is too old (likely 4.72). Skipping the file URL tests\n");
            break;
        }
        if (test->flags & URL_SUCCESS)
        {
            todo_wine_if(test->todo & 0x1)
                okShell(rc > 32, "failed: bad rc=%lu\n", rc);
        }
        else
        {
            todo_wine_if(test->todo & 0x1)
                okShell(rc == SE_ERR_FNF || rc == SE_ERR_PNF ||
                        broken(rc == SE_ERR_ACCESSDENIED) /* win2000 */,
                        "failed: bad rc=%lu\n", rc);
        }
        if (rc == 33)
        {
            todo_wine_if(test->todo & 0x2)
                okChildInt("argcA", 5);
            todo_wine_if(test->todo & 0x4)
                okChildString("argvA3", "Open");
            todo_wine_if(test->todo & 0x8)
                okChildPath("argvA4", filename);
        }
        test++;
    }

    SetEnvironmentVariableA("urlprefix", NULL);
}

static void test_urls(void)
{
    char url[MAX_PATH];
    INT_PTR rc;

    if (!create_test_class("fakeproto", FALSE))
    {
        skip("Unable to create 'fakeproto' class for URL tests\n");
        return;
    }
    create_test_verb("fakeproto", "open", 0, "URL %1");

    create_test_class("shlpaverb", TRUE);
    create_test_verb("shlpaverb", "averb", 0, "PAVerb \"%1\"");

    /* Protocols must be properly declared */
    rc = shell_execute(NULL, "notaproto://foo", NULL, NULL);
    ok(rc == SE_ERR_NOASSOC || broken(rc == SE_ERR_ACCESSDENIED),
       "%s returned %lu\n", shell_call, rc);

    rc = shell_execute(NULL, "fakeproto://foo/bar", NULL, NULL);
    todo_wine ok(rc == SE_ERR_NOASSOC || broken(rc == SE_ERR_ACCESSDENIED),
                 "%s returned %lu\n", shell_call, rc);

    /* Here's a real live one */
    rc = shell_execute(NULL, "shlproto://foo/bar", NULL, NULL);
    ok(rc > 32, "%s failed: rc=%lu\n", shell_call, rc);
    okChildInt("argcA", 5);
    okChildString("argvA3", "URL");
    okChildString("argvA4", "shlproto://foo/bar");

    /* Check default verb detection */
    rc = shell_execute(NULL, "shlpaverb://foo/bar", NULL, NULL);
    todo_wine ok(rc > 32 || /* XP+IE7 - Win10 */
                 broken(rc == SE_ERR_NOASSOC), /* XP+IE6 */
                 "%s failed: rc=%lu\n", shell_call, rc);
    if (rc > 32)
    {
        okChildInt("argcA", 5);
        todo_wine okChildString("argvA3", "PAVerb");
        todo_wine okChildString("argvA4", "shlpaverb://foo/bar");
    }

    /* But alternative verbs are a recent feature! */
    rc = shell_execute("averb", "shlproto://foo/bar", NULL, NULL);
    ok(rc > 32 || /* Win8 - Win10 */
       broken(rc == SE_ERR_ACCESSDENIED), /* XP - Win7 */
       "%s failed: rc=%lu\n", shell_call, rc);
    if (rc > 32)
    {
        okChildString("argvA3", "AVerb");
        okChildString("argvA4", "shlproto://foo/bar");
    }

    /* A .lnk ending does not turn a URL into a shortcut */
    rc = shell_execute(NULL, "shlproto://foo/bar.lnk", NULL, NULL);
    ok(rc > 32, "%s failed: rc=%lu\n", shell_call, rc);
    okChildInt("argcA", 5);
    okChildString("argvA3", "URL");
    okChildString("argvA4", "shlproto://foo/bar.lnk");

    /* Neither does a .exe extension */
    rc = shell_execute(NULL, "shlproto://foo/bar.exe", NULL, NULL);
    ok(rc > 32, "%s failed: rc=%lu\n", shell_call, rc);
    okChildInt("argcA", 5);
    okChildString("argvA3", "URL");
    okChildString("argvA4", "shlproto://foo/bar.exe");

    /* But a class name overrides it */
    rc = shell_execute(NULL, "shlproto://foo/bar", "shlexec.shlexec", NULL);
    ok(rc > 32, "%s failed: rc=%lu\n", shell_call, rc);
    okChildInt("argcA", 5);
    okChildString("argvA3", "URL");
    okChildString("argvA4", "shlproto://foo/bar");

    /* Environment variables are expanded in URLs (but not in file URLs!) */
    rc = shell_execute_ex(SEE_MASK_DOENVSUBST | SEE_MASK_FLAG_NO_UI,
                          NULL, "shlproto://%TMPDIR%/bar", NULL, NULL, NULL);
    okShell(rc > 32, "failed: rc=%lu\n", rc);
    okChildInt("argcA", 5);
    sprintf(url, "shlproto://%s/bar", tmpdir);
    okChildString("argvA3", "URL");
    okChildStringBroken("argvA4", url, "shlproto://%TMPDIR%/bar");

    /* But only after the path has been identified as a URL */
    SetEnvironmentVariableA("urlprefix", "shlproto:///");
    rc = shell_execute(NULL, "%urlprefix%foo", NULL, NULL);
    todo_wine ok(rc == SE_ERR_FNF, "%s returned %lu\n", shell_call, rc);
    SetEnvironmentVariableA("urlprefix", NULL);

    delete_test_class("fakeproto");
    delete_test_class("shlpaverb");
}

static void test_find_executable(void)
{
    char notepad_path[MAX_PATH];
    char filename[MAX_PATH];
    char command[MAX_PATH];
    const filename_tests_t* test;
    INT_PTR rc;

    if (!create_test_association(".sfe"))
    {
        skip("Unable to create association for '.sfe'\n");
        return;
    }
    create_test_verb("shlexec.sfe", "Open", 1, "%1");

    /* Don't test FindExecutable(..., NULL), it always crashes */

    strcpy(command, "your word");
    if (0) /* Can crash on Vista! */
    {
    rc=(INT_PTR)FindExecutableA(NULL, NULL, command);
    ok(rc == SE_ERR_FNF || rc > 32 /* nt4 */, "FindExecutable(NULL) returned %ld\n", rc);
    ok(strcmp(command, "your word") != 0, "FindExecutable(NULL) returned command=[%s]\n", command);
    }

    GetSystemDirectoryA( notepad_path, MAX_PATH );
    strcat( notepad_path, "\\notepad.exe" );

    /* Search for something that should be in the system-wide search path (no default directory) */
    strcpy(command, "your word");
    rc=(INT_PTR)FindExecutableA("notepad.exe", NULL, command);
    ok(rc > 32, "FindExecutable(%s) returned %ld\n", "notepad.exe", rc);
    ok(strcasecmp(command, notepad_path) == 0, "FindExecutable(%s) returned command=[%s]\n", "notepad.exe", command);

    /* Search for something that should be in the system-wide search path (with default directory) */
    strcpy(command, "your word");
    rc=(INT_PTR)FindExecutableA("notepad.exe", tmpdir, command);
    ok(rc > 32, "FindExecutable(%s) returned %ld\n", "notepad.exe", rc);
    ok(strcasecmp(command, notepad_path) == 0, "FindExecutable(%s) returned command=[%s]\n", "notepad.exe", command);

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
    create_test_verb("shlexec.shl", "Open", 0, "Open");

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

    if (skip_shlexec_tests)
    {
        skip("No FindExecutable/filename tests due to lack of .shlexec association\n");
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
        todo_wine_if(test->todo & 0x10)
            ok(rc==test->rc, "FindExecutable(%s) failed: rc=%ld\n", filename, rc);
        if (rc > 32)
        {
            BOOL equal;
            equal=strcmp(command, argv0) == 0 ||
                /* NT4 returns an extra 0x8 character! */
                (strlen(command) == strlen(argv0)+1 && strncmp(command, argv0, strlen(argv0)) == 0);
            todo_wine_if(test->todo & 0x20)
                ok(equal, "FindExecutable(%s) returned command='%s' instead of '%s'\n",
                   filename, command, argv0);
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
    INT_PTR rc;

    if (skip_shlexec_tests)
        skip("No FindExecutable/filename tests due to lack of .shlexec association\n");
    else
    {
        /* Should open through our association */
        sprintf(filename, "%s\\test_shortcut_shlexec.lnk", tmpdir);
        rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, filename, NULL, NULL, NULL);
        okShell(rc > 32, "failed: rc=%lu err=%u\n", rc, GetLastError());
        okChildInt("argcA", 5);
        okChildString("argvA3", "Open");
        sprintf(params, "%s\\test file.shlexec", tmpdir);
        get_long_path_name(params, filename, sizeof(filename));
        okChildPath("argvA4", filename);

        rc=shell_execute_ex(SEE_MASK_NOZONECHECKS|SEE_MASK_DOENVSUBST, NULL, "%TMPDIR%\\test_shortcut_shlexec.lnk", NULL, NULL, NULL);
        okShell(rc > 32, "failed: rc=%lu err=%u\n", rc, GetLastError());
        okChildInt("argcA", 5);
        okChildString("argvA3", "Open");
        sprintf(params, "%s\\test file.shlexec", tmpdir);
        get_long_path_name(params, filename, sizeof(filename));
        okChildPath("argvA4", filename);
    }

    /* Should just run our executable */
    sprintf(filename, "%s\\test_shortcut_exe.lnk", tmpdir);
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, filename, NULL, NULL, NULL);
    okShell(rc > 32, "failed: rc=%lu err=%u\n", rc, GetLastError());
    okChildInt("argcA", 4);
    okChildString("argvA3", "Lnk");

    if (!skip_shlexec_tests)
    {
        /* An explicit class overrides lnk's ContextMenuHandler */
        rc=shell_execute_ex(SEE_MASK_CLASSNAME | SEE_MASK_NOZONECHECKS, NULL, filename, NULL, NULL, "shlexec.shlexec");
        okShell(rc > 32, "failed: rc=%lu err=%u\n", rc, GetLastError());
        okChildInt("argcA", 5);
        okChildString("argvA3", "Open");
        okChildPath("argvA4", filename);
    }

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
        rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, filename, NULL, NULL, NULL);
        okShell(rc > 32, "failed: rc=%lu err=%u\n", rc, GetLastError());
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
                            NULL, NULL);
        if (rc > 32)
            rc=33;
        todo_wine_if(test->todo & 0x1)
            okShell(rc==test->rc, "failed: rc=%lu err=%u\n", rc, GetLastError());
        if (rc==0)
        {
            todo_wine_if(test->todo & 0x2)
                okChildInt("argcA", 5);
            todo_wine_if(test->todo & 0x4)
                okChildString("argvA3", "Lnk");
            sprintf(params, test->basename, tmpdir);
            okChildPath("argvA4", params);
        }
        test++;
    }
}


static void test_exes(void)
{
    char filename[MAX_PATH];
    char params[1024];
    INT_PTR rc;

    sprintf(params, "shlexec \"%s\" Exec", child_file);

    /* We need NOZONECHECKS on Win2003 to block a dialog */
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, argv0, params,
                        NULL, NULL);
    okShell(rc > 32, "returned %lu\n", rc);
    okChildInt("argcA", 4);
    okChildString("argvA3", "Exec");

    if (! skip_noassoc_tests)
    {
        sprintf(filename, "%s\\test file.noassoc", tmpdir);
        if (CopyFileA(argv0, filename, FALSE))
        {
            rc=shell_execute(NULL, filename, params, NULL);
            todo_wine {
                okShell(rc==SE_ERR_NOASSOC, "returned %lu\n", rc);
            }
        }
    }
    else
    {
        win_skip("Skipping shellexecute of file with unassociated extension\n");
    }

    /* test combining executable and parameters */
    sprintf(filename, "%s shlexec \"%s\" Exec", argv0, child_file);
    rc = shell_execute(NULL, filename, NULL, NULL);
    okShell(rc == SE_ERR_FNF, "returned %lu\n", rc);

    sprintf(filename, "\"%s\" shlexec \"%s\" Exec", argv0, child_file);
    rc = shell_execute(NULL, filename, NULL, NULL);
    okShell(rc == SE_ERR_FNF, "returned %lu\n", rc);

    /* A verb, even if invalid, overrides the normal handling of executables */
    todo_wait rc = shell_execute_ex(SEE_MASK_FLAG_NO_UI,
                                    "notaverb", argv0, NULL, NULL, NULL);
    todo_wine okShell(rc == SE_ERR_NOASSOC, "returned %lu\n", rc);

    if (!skip_shlexec_tests)
    {
        /* A class overrides the normal handling of executables too */
        /* FIXME SEE_MASK_FLAG_NO_UI is only needed due to Wine's bug */
        rc = shell_execute_ex(SEE_MASK_CLASSNAME | SEE_MASK_FLAG_NO_UI,
                              NULL, argv0, NULL, NULL, ".shlexec");
        todo_wine okShell(rc > 32, "returned %lu\n", rc);
        okChildInt("argcA", 5);
        todo_wine okChildString("argvA3", "Open");
        todo_wine okChildPath("argvA4", argv0);
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
    BOOL broken;
} dde_tests_t;

static dde_tests_t dde_tests[] =
{
    /* Test passing and not passing command-line
     * argument, no DDE */
    {"", NULL, NULL, NULL, NULL, 0, ""},
    {"\"%1\"", NULL, NULL, NULL, NULL, 1, ""},

    /* Test passing and not passing command-line
     * argument, with DDE */
    {"", "[open(\"%1\")]", "shlexec", "dde", NULL, 0, "[open(\"%s\")]"},
    {"\"%1\"", "[open(\"%1\")]", "shlexec", "dde", NULL, 1, "[open(\"%s\")]"},

    /* Test unquoted %1 in command and ddeexec
     * (test filename has space) */
    {"%1", "[open(%1)]", "shlexec", "dde", NULL, 2, "[open(%s)]", TRUE /* before vista */},

    /* Test ifexec precedence over ddeexec */
    {"", "[open(\"%1\")]", "shlexec", "dde", "[ifexec(\"%1\")]", 0, "[ifexec(\"%s\")]"},

    /* Test default DDE topic */
    {"", "[open(\"%1\")]", "shlexec", NULL, NULL, 0, "[open(\"%s\")]"},

    /* Test default DDE application */
    {"", "[open(\"%1\")]", NULL, "dde", NULL, 0, "[open(\"%s\")]"},

    {NULL}
};

static int waitforinputidle_count;
static DWORD WINAPI hooked_WaitForInputIdle(HANDLE process, DWORD timeout)
{
    waitforinputidle_count++;
    if (winetest_debug > 1)
        trace("WaitForInputIdle() waiting for dde event timeout=min(%u,5s)\n", timeout);
    timeout = timeout < 5000 ? timeout : 5000;
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
static void hook_WaitForInputIdle(DWORD (WINAPI *new_func)(HANDLE, DWORD))
{
    char *base;
    PIMAGE_NT_HEADERS nt_headers;
    DWORD import_directory_rva;
    PIMAGE_IMPORT_DESCRIPTOR import_descriptor;
    int hook_count = 0;

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
                        if (winetest_debug > 1)
                            trace("Hooked %s.WaitForInputIdle\n", import_module_name);
                        hook_count++;
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
    ok(hook_count, "Could not hook WaitForInputIdle()\n");
}

static void test_dde(void)
{
    char filename[MAX_PATH], defApplication[MAX_PATH];
    const dde_tests_t* test;
    char params[1024];
    INT_PTR rc;
    HANDLE map;
    char *shared_block;
    DWORD ddeflags;

    hook_WaitForInputIdle(hooked_WaitForInputIdle);

    sprintf(filename, "%s\\test file.sde", tmpdir);

    /* Default service is application name minus path and extension */
    strcpy(defApplication, strrchr(argv0, '\\')+1);
    *strchr(defApplication, '.') = 0;

    map = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                             4096, "winetest_shlexec_dde_map");
    shared_block = MapViewOfFile(map, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 4096);

    ddeflags = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
    test = dde_tests;
    while (test->command)
    {
        if (!create_test_association(".sde"))
        {
            skip("Unable to create association for '.sde'\n");
            return;
        }
        create_test_verb_dde("shlexec.sde", "Open", 0, test->command, test->ddeexec,
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

        waitforinputidle_count = 0;
        dde_ready_event = CreateEventA(NULL, TRUE, FALSE, "winetest_shlexec_dde_ready");
        rc = shell_execute_ex(ddeflags, NULL, filename, NULL, NULL, NULL);
        CloseHandle(dde_ready_event);
        if (!(ddeflags & SEE_MASK_WAITFORINPUTIDLE) && rc == SE_ERR_DDEFAIL &&
            GetLastError() == ERROR_FILE_NOT_FOUND &&
            strcmp(winetest_platform, "windows") == 0)
        {
            /* Windows 10 does not call WaitForInputIdle() for DDE which creates
             * a race condition as the DDE server may not have time to start up.
             * When that happens the test fails with the above results and we
             * compensate by forcing the WaitForInputIdle() call.
             */
            trace("Adding SEE_MASK_WAITFORINPUTIDLE for Windows 10\n");
            ddeflags |= SEE_MASK_WAITFORINPUTIDLE;
            delete_test_association(".sde");
            Sleep(CHILD_DDE_TIMEOUT);
            continue;
        }
        okShell(32 < rc, "failed: rc=%lu err=%u\n", rc, GetLastError());
        if (test->ddeexec)
            okShell(waitforinputidle_count == 1 ||
                    broken(waitforinputidle_count == 0) /* Win10 race */,
                    "WaitForInputIdle() was called %u times\n",
                    waitforinputidle_count);
        else
            okShell(waitforinputidle_count == 0, "WaitForInputIdle() was called %u times for a non-DDE case\n", waitforinputidle_count);

        if (32 < rc)
        {
            if (test->broken)
                okChildIntBroken("argcA", test->expectedArgs + 3);
            else
                okChildInt("argcA", test->expectedArgs + 3);

            if (test->expectedArgs == 1) okChildPath("argvA3", filename);

            sprintf(params, test->expectedDdeExec, filename);
            okChildPath("ddeExec", params);
        }
        reset_association_description();

        delete_test_association(".sde");
        test++;
    }

    UnmapViewOfFile(shared_block);
    CloseHandle(map);
    hook_WaitForInputIdle((void *) WaitForInputIdle);
}

#define DDE_DEFAULT_APP_VARIANTS 3
typedef struct
{
    const char* command;
    const char* expectedDdeApplication[DDE_DEFAULT_APP_VARIANTS];
    int todo;
    int rc[DDE_DEFAULT_APP_VARIANTS];
} dde_default_app_tests_t;

static dde_default_app_tests_t dde_default_app_tests[] =
{
    /* There are three possible sets of results: Windows <= 2000, XP SP1 and
     * >= XP SP2. Use the first two tests to determine which results to expect.
     */

    /* Test unquoted existing filename with a space */
    {"%s\\test file.exe", {"test file", "test file", "test"}, 0x0, {33, 33, 33}},
    {"%s\\test2 file.exe", {"test2", "", "test2"}, 0x0, {33, 5, 33}},

    /* Test unquoted existing filename with a space */
    {"%s\\test file.exe param", {"test file", "test file", "test"}, 0x0, {33, 33, 33}},

    /* Test quoted existing filename with a space */
    {"\"%s\\test file.exe\"", {"test file", "test file", "test file"}, 0x0, {33, 33, 33}},
    {"\"%s\\test file.exe\" param", {"test file", "test file", "test file"}, 0x0, {33, 33, 33}},

    /* Test unquoted filename with a space that doesn't exist, but
     * test2.exe does */
    {"%s\\test2 file.exe param", {"test2", "", "test2"}, 0x0, {33, 5, 33}},

    /* Test quoted filename with a space that does not exist */
    {"\"%s\\test2 file.exe\"", {"", "", "test2 file"}, 0x0, {5, 2, 33}},
    {"\"%s\\test2 file.exe\" param", {"", "", "test2 file"}, 0x0, {5, 2, 33}},

    /* Test filename supplied without the extension */
    {"%s\\test2", {"test2", "", "test2"}, 0x0, {33, 5, 33}},
    {"%s\\test2 param", {"test2", "", "test2"}, 0x0, {33, 5, 33}},

    /* Test an unquoted nonexistent filename */
    {"%s\\notexist.exe", {"", "", "notexist"}, 0x0, {5, 2, 33}},
    {"%s\\notexist.exe param", {"", "", "notexist"}, 0x0, {5, 2, 33}},

    /* Test an application that will be found on the path */
    {"cmd", {"cmd", "cmd", "cmd"}, 0x0, {33, 33, 33}},
    {"cmd param", {"cmd", "cmd", "cmd"}, 0x0, {33, 33, 33}},

    /* Test an application that will not be found on the path */
    {"xyzwxyzwxyz", {"", "", "xyzwxyzwxyz"}, 0x0, {5, 2, 33}},
    {"xyzwxyzwxyz param", {"", "", "xyzwxyzwxyz"}, 0x0, {5, 2, 33}},

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
    PostThreadMessageA(info->threadIdParent,
                       WM_QUIT,
                       shell_execute_ex(SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI, NULL, info->filename, NULL, NULL, NULL),
                       0);
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
    INT_PTR rc;
    int which = 0;
    HDDEDATA ret;
    BOOL b;

    post_quit_on_execute = FALSE;
    ddeInst = 0;
    rc = DdeInitializeA(&ddeInst, ddeCb, CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_ADVISES |
                        CBF_FAIL_POKES | CBF_FAIL_REQUESTS, 0);
    ok(rc == DMLERR_NO_ERROR, "got %lx\n", rc);

    sprintf(filename, "%s\\test file.sde", tmpdir);

    /* It is strictly not necessary to register an application name here, but wine's
     * DdeNameService implementation complains if 0 is passed instead of
     * hszApplication with DNS_FILTEROFF */
    hszApplication = DdeCreateStringHandleA(ddeInst, "shlexec", CP_WINANSI);
    hszTopic = DdeCreateStringHandleA(ddeInst, "shlexec", CP_WINANSI);
    ok(hszApplication && hszTopic, "got %p and %p\n", hszApplication, hszTopic);
    ret = DdeNameService(ddeInst, hszApplication, 0, DNS_REGISTER | DNS_FILTEROFF);
    ok(ret != 0, "got %p\n", ret);

    test = dde_default_app_tests;
    while (test->command)
    {
        HANDLE thread;

        if (!create_test_association(".sde"))
        {
            skip("Unable to create association for '.sde'\n");
            return;
        }
        sprintf(params, test->command, tmpdir);
        create_test_verb_dde("shlexec.sde", "Open", 1, params, "[test]", NULL,
                             "shlexec", NULL);
        ddeApplication[0] = 0;

        /* No application will be run as we will respond to the first DDE event,
         * so don't wait for it */
        SetEvent(hEvent);

        thread = CreateThread(NULL, 0, ddeThread, &info, 0, &threadId);
        ok(thread != NULL, "got %p\n", thread);
        while (GetMessageA(&msg, NULL, 0, 0)) DispatchMessageA(&msg);
        rc = msg.wParam > 32 ? 33 : msg.wParam;

        /* The first two tests determine which set of results to expect.
         * First check the platform as only the first set of results is
         * acceptable for Wine.
         */
        if (strcmp(winetest_platform, "wine"))
        {
            if (test == dde_default_app_tests)
            {
                if (strcmp(ddeApplication, test->expectedDdeApplication[0]))
                    which = 2;
            }
            else if (test == dde_default_app_tests + 1)
            {
                if (which == 0 && rc == test->rc[1])
                    which = 1;
                trace("DDE result variant %d\n", which);
            }
        }

        todo_wine_if(test->todo & 0x1)
            okShell(rc==test->rc[which], "failed: rc=%lu err=%u\n",
                    rc, GetLastError());
        if (rc == 33)
        {
            todo_wine_if(test->todo & 0x2)
                ok(!strcmp(ddeApplication, test->expectedDdeApplication[which]),
                   "Expected application '%s', got '%s'\n",
                   test->expectedDdeApplication[which], ddeApplication);
        }
        reset_association_description();

        delete_test_association(".sde");
        test++;
    }

    ret = DdeNameService(ddeInst, hszApplication, 0, DNS_UNREGISTER);
    ok(ret != 0, "got %p\n", ret);
    b = DdeFreeStringHandle(ddeInst, hszTopic);
    ok(b, "got %d\n", b);
    b = DdeFreeStringHandle(ddeInst, hszApplication);
    ok(b, "got %d\n", b);
    b = DdeUninitialize(ddeInst);
    ok(b, "got %d\n", b);
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

    rc=GetModuleFileNameA(NULL, argv0, sizeof(argv0));
    ok(rc != 0 && rc < sizeof(argv0), "got %d\n", rc);
    if (GetFileAttributesA(argv0)==INVALID_FILE_ATTRIBUTES)
    {
        strcat(argv0, ".so");
        ok(GetFileAttributesA(argv0)!=INVALID_FILE_ATTRIBUTES,
           "unable to find argv0!\n");
    }

    /* Older versions (win 2k) fail tests if there is a space in
       the path. */
    if (dllver.dwMajorVersion <= 5)
        strcpy(filename, "c:\\");
    else
        GetTempPathA(sizeof(filename), filename);
    GetTempFileNameA(filename, "wt", 0, tmpdir);
    GetLongPathNameA(tmpdir, tmpdir, sizeof(tmpdir));
    DeleteFileA( tmpdir );
    rc = CreateDirectoryA( tmpdir, NULL );
    ok( rc, "failed to create %s err %u\n", tmpdir, GetLastError() );
    /* Set %TMPDIR% for the tests */
    SetEnvironmentVariableA("TMPDIR", tmpdir);

    rc = GetTempFileNameA(tmpdir, "wt", 0, child_file);
    ok(rc != 0, "got %d\n", rc);
    init_event(child_file);

    /* Set up the test files */
    testfile=testfiles;
    while (*testfile)
    {
        HANDLE hfile;

        sprintf(filename, *testfile, tmpdir);
        hfile=CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL, NULL);
        if (hfile==INVALID_HANDLE_VALUE)
        {
            trace("unable to create '%s': err=%u\n", filename, GetLastError());
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
        skip_shlexec_tests = TRUE;
        skip("Unable to create association for '.shlexec'\n");
        return;
    }
    create_test_verb("shlexec.shlexec", "Open", 0, "Open \"%1\"");
    create_test_verb("shlexec.shlexec", "NoQuotes", 0, "NoQuotes %1");
    create_test_verb("shlexec.shlexec", "LowerL", 0, "LowerL %l");
    create_test_verb("shlexec.shlexec", "QuotedLowerL", 0, "QuotedLowerL \"%l\"");
    create_test_verb("shlexec.shlexec", "UpperL", 0, "UpperL %L");
    create_test_verb("shlexec.shlexec", "QuotedUpperL", 0, "QuotedUpperL \"%L\"");

    create_test_association(".sha");
    create_test_verb("shlexec.sha", "averb", 0, "AVerb \"%1\"");

    create_test_class("shlproto", TRUE);
    create_test_verb("shlproto", "open", 0, "URL \"%1\"");
    create_test_verb("shlproto", "averb", 0, "AVerb \"%1\"");

    /* Set an environment variable to see if it is inherited */
    SetEnvironmentVariableA("ShlexecVar", "Present");
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
        SetFileAttributesA(filename, FILE_ATTRIBUTE_NORMAL);
        DeleteFileA(filename);
        testfile++;
    }
    DeleteFileA(child_file);
    RemoveDirectoryA(tmpdir);

    /* Delete the test association */
    delete_test_association(".shlexec");
    delete_test_association(".sha");
    delete_test_class("shlproto");

    CloseHandle(hEvent);

    CoUninitialize();
}

static void test_directory(void)
{
    char path[MAX_PATH], curdir[MAX_PATH];
    char params[1024], dirpath[1024];
    INT_PTR rc;

    sprintf(path, "%s\\test2.exe", tmpdir);
    CopyFileA(argv0, path, FALSE);

    sprintf(params, "shlexec \"%s\" Exec", child_file);

    /* Test with the current directory */
    GetCurrentDirectoryA(sizeof(curdir), curdir);
    SetCurrentDirectoryA(tmpdir);
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS|SEE_MASK_FLAG_NO_UI,
                        NULL, "test2.exe", params, NULL, NULL);
    okShell(rc > 32, "returned %lu\n", rc);
    okChildInt("argcA", 4);
    okChildString("argvA3", "Exec");
    todo_wine okChildPath("longPath", path);
    SetCurrentDirectoryA(curdir);

    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS|SEE_MASK_FLAG_NO_UI,
                        NULL, "test2.exe", params, NULL, NULL);
    okShell(rc == SE_ERR_FNF, "returned %lu\n", rc);

    /* Explicitly specify the directory to use */
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS|SEE_MASK_FLAG_NO_UI,
                        NULL, "test2.exe", params, tmpdir, NULL);
    okShell(rc > 32, "returned %lu\n", rc);
    okChildInt("argcA", 4);
    okChildString("argvA3", "Exec");
    todo_wine okChildPath("longPath", path);

    /* Specify it through an environment variable */
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS|SEE_MASK_FLAG_NO_UI,
                        NULL, "test2.exe", params, "%TMPDIR%", NULL);
    todo_wine okShell(rc == SE_ERR_FNF, "returned %lu\n", rc);

    rc=shell_execute_ex(SEE_MASK_DOENVSUBST|SEE_MASK_NOZONECHECKS|SEE_MASK_FLAG_NO_UI,
                        NULL, "test2.exe", params, "%TMPDIR%", NULL);
    okShell(rc > 32, "returned %lu\n", rc);
    okChildInt("argcA", 4);
    okChildString("argvA3", "Exec");
    todo_wine okChildPath("longPath", path);

    /* Not a colon-separated directory list */
    sprintf(dirpath, "%s:%s", curdir, tmpdir);
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS|SEE_MASK_FLAG_NO_UI,
                        NULL, "test2.exe", params, dirpath, NULL);
    okShell(rc == SE_ERR_FNF, "returned %lu\n", rc);
}

START_TEST(shlexec)
{

    myARGC = winetest_get_mainargs(&myARGV);
    if (myARGC >= 3)
    {
        doChild(myARGC, myARGV);
        /* Skip the tests/failures trace for child processes */
        ExitProcess(winetest_get_failures());
    }

    init_test();

    test_commandline2argv();
    test_argify();
    test_lpFile_parsed();
    test_filename();
    test_fileurls();
    test_urls();
    test_find_executable();
    test_lnks();
    test_exes();
    test_dde();
    test_dde_default_app();
    test_directory();

    cleanup_test();
}
