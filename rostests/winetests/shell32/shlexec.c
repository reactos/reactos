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

#include <stdio.h>
#include <assert.h>

/* Needed to get SEE_MASK_NOZONECHECKS with the PSDK */
#define NTDDI_WINXPSP1 0x05010100
#define NTDDI_VERSION NTDDI_WINXPSP1

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
    int rc;

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
    rc=(int)ShellExecute(NULL, operation, file, parameters, directory,
                         SW_SHOWNORMAL);

    if (rc > 32)
    {
        int wait_rc;
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

static int shell_execute_ex(DWORD mask, LPCSTR operation, LPCSTR file,
                            LPCSTR parameters, LPCSTR directory)
{
    SHELLEXECUTEINFO sei;
    BOOL success;
    int rc;

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
    rc=(int)sei.hInstApp;
    ok((success && rc > 32) || (!success && rc <= 32),
       "%s rc=%d and hInstApp=%d is not allowed\n", shell_call, success, rc);

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

static void create_test_association(const char* extension)
{
    HKEY hkey, hkey_shell;
    char class[MAX_PATH];
    LONG rc;

    sprintf(class, "shlexec%s", extension);
    rc=RegCreateKeyEx(HKEY_CLASSES_ROOT, extension, 0, NULL, 0, KEY_SET_VALUE,
                      NULL, &hkey, NULL);
    assert(rc==ERROR_SUCCESS);
    rc=RegSetValueEx(hkey, NULL, 0, REG_SZ, (LPBYTE) class, strlen(class)+1);
    assert(rc==ERROR_SUCCESS);
    CloseHandle(hkey);

    rc=RegCreateKeyEx(HKEY_CLASSES_ROOT, class, 0, NULL, 0,
                      KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS, NULL, &hkey, NULL);
    assert(rc==ERROR_SUCCESS);
    rc=RegCreateKeyEx(hkey, "shell", 0, NULL, 0,
                      KEY_CREATE_SUB_KEY, NULL, &hkey_shell, NULL);
    assert(rc==ERROR_SUCCESS);
    CloseHandle(hkey);
    CloseHandle(hkey_shell);
}

static void delete_test_association(const char* extension)
{
    char class[MAX_PATH];

    sprintf(class, "shlexec%s", extension);
    SHDeleteKey(HKEY_CLASSES_ROOT, class);
    SHDeleteKey(HKEY_CLASSES_ROOT, extension);
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
        cmd=malloc(strlen(argv0)+10+strlen(child_file)+2+strlen(cmdtail)+1);
        sprintf(cmd,"%s shlexec \"%s\" %s", argv0, child_file, cmdtail);
        rc=RegSetValueEx(hkey_cmd, NULL, 0, REG_SZ, (LPBYTE)cmd, strlen(cmd)+1);
        assert(rc==ERROR_SUCCESS);
        free(cmd);
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

static void doChild(int argc, char** argv)
{
    char* filename;
    HANDLE hFile;
    int i;

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

static int _okChildString(const char* file, int line, const char* key, const char* expected)
{
    char* result;
    result=getChildString("Arguments", key);
    return ok_(file, line)(lstrcmpiA(result, expected) == 0,
               "%s expected '%s', got '%s'\n", key, expected, result);
}

static int _okChildPath(const char* file, int line, const char* key, const char* expected)
{
    char* result;
    result=getChildString("Arguments", key);
    return ok_(file, line)(StrCmpPath(result, expected) == 0,
               "%s expected '%s', got '%s'\n", key, expected, result);
}

static int _okChildInt(const char* file, int line, const char* key, int expected)
{
    INT result;
    result=GetPrivateProfileIntA("Arguments", key, expected, child_file);
    return ok_(file, line)(result == expected,
               "%s expected %d, but got %d\n", key, expected, result);
}

#define okChildString(key, expected) _okChildString(__FILE__, __LINE__, (key), (expected))
#define okChildPath(key, expected) _okChildPath(__FILE__, __LINE__, (key), (expected))
#define okChildInt(key, expected)    _okChildInt(__FILE__, __LINE__, (key), (expected))



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
    {NULL,           "%s\\nonexistent.shlexec", 0x11, SE_ERR_FNF},
    {NULL,           "%s\\nonexistent.noassoc", 0x11, SE_ERR_FNF},

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
    {"LowerL",       "%s\\nonexistent.shlexec", 0x11, SE_ERR_FNF},
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

static void test_filename(void)
{
    char filename[MAX_PATH];
    const filename_tests_t* test;
    char* c;
    int rc;

    test=filename_tests;
    while (test->basename)
    {
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
            sprintf(quoted, "\"%s\"", filename);
            rc=shell_execute(test->verb, quoted, NULL, NULL);
        }
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
    int rc;

    create_test_association(".sfe");
    create_test_verb(".sfe", "Open", 1, "%1");

    /* Don't test FindExecutable(..., NULL), it always crashes */

    strcpy(command, "your word");
    rc=(int)FindExecutableA(NULL, NULL, command);
    ok(rc == SE_ERR_FNF || rc > 32 /* nt4 */, "FindExecutable(NULL) returned %d\n", rc);
    ok(strcmp(command, "your word") != 0, "FindExecutable(NULL) returned command=[%s]\n", command);

    strcpy(command, "your word");
    rc=(int)FindExecutableA(tmpdir, NULL, command);
    ok(rc == SE_ERR_NOASSOC /* >= win2000 */ || rc > 32 /* win98, nt4 */, "FindExecutable(NULL) returned %d\n", rc);
    ok(strcmp(command, "your word") != 0, "FindExecutable(NULL) returned command=[%s]\n", command);

    sprintf(filename, "%s\\test file.sfe", tmpdir);
    rc=(int)FindExecutableA(filename, NULL, command);
    ok(rc > 32, "FindExecutable(%s) returned %d\n", filename, rc);
    /* Depending on the platform, command could be '%1' or 'test file.sfe' */

    rc=(int)FindExecutableA("test file.sfe", tmpdir, command);
    ok(rc > 32, "FindExecutable(%s) returned %d\n", filename, rc);

    rc=(int)FindExecutableA("test file.sfe", NULL, command);
    todo_wine ok(rc == SE_ERR_FNF, "FindExecutable(%s) returned %d\n", filename, rc);

    delete_test_association(".sfe");

    create_test_association(".shl");
    create_test_verb(".shl", "Open", 0, "Open");

    sprintf(filename, "%s\\test file.shl", tmpdir);
    rc=(int)FindExecutableA(filename, NULL, command);
    ok(rc == SE_ERR_FNF /* NT4 */ || rc > 32, "FindExecutable(%s) returned %d\n", filename, rc);

    sprintf(filename, "%s\\test file.shlfoo", tmpdir);
    rc=(int)FindExecutableA(filename, NULL, command);

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
        trace("FindExecutable() is broken -> skipping 4+ character extension tests\n");
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
        rc=(int)FindExecutableA(filename, NULL, command);
        if (rc > 32)
            rc=33;
        if ((test->todo & 0x10)==0)
        {
            ok(rc==test->rc, "FindExecutable(%s) failed: rc=%d\n", filename, rc);
        }
        else todo_wine
        {
            ok(rc==test->rc, "FindExecutable(%s) failed: rc=%d\n", filename, rc);
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
    sprintf(filename, "%s\\test file.shlexec", tmpdir);
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

    sprintf(filename, "%s\\test file.noassoc", tmpdir);
    if (CopyFile(argv0, filename, FALSE))
    {
        rc=shell_execute(NULL, filename, params, NULL);
        todo_wine {
        ok(rc==SE_ERR_NOASSOC, "%s succeeded: rc=%d\n", shell_call, rc);
        }
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

    sprintf(filename, "%s\\test file.noassoc", tmpdir);
    if (CopyFile(argv0, filename, FALSE))
    {
        rc=shell_execute(NULL, filename, params, NULL);
        todo_wine {
        ok(rc==SE_ERR_NOASSOC, "%s succeeded: rc=%d\n", shell_call, rc);
        }
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
    int rc;
} dde_tests_t;

static dde_tests_t dde_tests[] =
{
    /* Test passing and not passing command-line
     * argument, no DDE */
    {"", NULL, NULL, NULL, NULL, FALSE, "", 0x0, 33},
    {"\"%1\"", NULL, NULL, NULL, NULL, TRUE, "", 0x0, 33},

    /* Test passing and not passing command-line
     * argument, with DDE */
    {"", "[open(\"%1\")]", "shlexec", "dde", NULL, FALSE, "[open(\"%s\")]", 0x0, 33},
    {"\"%1\"", "[open(\"%1\")]", "shlexec", "dde", NULL, TRUE, "[open(\"%s\")]", 0x0, 33},

    /* Test unquoted %1 in command and ddeexec
     * (test filename has space) */
    {"%1", "[open(%1)]", "shlexec", "dde", NULL, 2, "[open(%s)]", 0x0, 33},

    /* Test ifexec precedence over ddeexec */
    {"", "[open(\"%1\")]", "shlexec", "dde", "[ifexec(\"%1\")]", FALSE, "[ifexec(\"%s\")]", 0x0, 33},

    /* Test default DDE topic */
    {"", "[open(\"%1\")]", "shlexec", NULL, NULL, FALSE, "[open(\"%s\")]", 0x0, 33},

    /* Test default DDE application */
    {"", "[open(\"%1\")]", NULL, "dde", NULL, FALSE, "[open(\"%s\")]", 0x0, 33},

    {NULL, NULL, NULL, NULL, NULL, 0, 0x0, 0}
};

static DWORD ddeInst;
static HSZ hszTopic;
static char ddeExec[MAX_PATH], ddeApplication[MAX_PATH];
static BOOL denyNextConnection;

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
                if (denyNextConnection)
                    denyNextConnection = FALSE;
                else
                {
                    size = DdeQueryString(ddeInst, hsz2, ddeApplication, MAX_PATH, CP_WINANSI);
                    assert(size < MAX_PATH);
                    return (HDDEDATA)TRUE;
                }
            }
            return (HDDEDATA)FALSE;

        case XTYP_EXECUTE:
            size = DdeGetData(hData, (LPBYTE)ddeExec, MAX_PATH, 0L);
            assert(size < MAX_PATH);
            DdeFreeDataHandle(hData);
            return (HDDEDATA)DDE_FACK;

        default:
            return NULL;
    }
}

typedef struct
{
    char *filename;
    DWORD threadIdParent;
} dde_thread_info_t;

static DWORD CALLBACK ddeThread(LPVOID arg)
{
    dde_thread_info_t *info = (dde_thread_info_t *)arg;
    assert(info && info->filename);
    PostThreadMessage(info->threadIdParent,
                      WM_QUIT,
                      shell_execute_ex(SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI, NULL, info->filename, NULL, NULL),
                      0L);
    ExitThread(0);
}

/* ShellExecute won't successfully send DDE commands to console applications after starting them,
 * so we run a DDE server in this application, deny the first connection request to make
 * ShellExecute start the application, and then process the next DDE connection in this application
 * to see the execute command that is sent. */
static void test_dde(void)
{
    char filename[MAX_PATH], defApplication[MAX_PATH];
    HSZ hszApplication;
    dde_thread_info_t info = { filename, GetCurrentThreadId() };
    const dde_tests_t* test;
    char params[1024];
    DWORD threadId;
    MSG msg;
    int rc;

    ddeInst = 0;
    rc = DdeInitializeA(&ddeInst, ddeCb, CBF_SKIP_ALLNOTIFICATIONS | CBF_FAIL_ADVISES |
                        CBF_FAIL_POKES | CBF_FAIL_REQUESTS, 0L);
    assert(rc == DMLERR_NO_ERROR);

    sprintf(filename, "%s\\test file.sde", tmpdir);

    /* Default service is application name minus path and extension */
    strcpy(defApplication, strrchr(argv0, '\\')+1);
    *strchr(defApplication, '.') = 0;

    test = dde_tests;
    while (test->command)
    {
        create_test_association(".sde");
        create_test_verb_dde(".sde", "Open", 0, test->command, test->ddeexec,
                             test->application, test->topic, test->ifexec);
        hszApplication = DdeCreateStringHandleA(ddeInst, test->application ?
                                                test->application : defApplication, CP_WINANSI);
        hszTopic = DdeCreateStringHandleA(ddeInst, test->topic ? test->topic : SZDDESYS_TOPIC,
                                          CP_WINANSI);
        assert(hszApplication && hszTopic);
        assert(DdeNameService(ddeInst, hszApplication, 0L, DNS_REGISTER));
        denyNextConnection = TRUE;
        ddeExec[0] = 0;

        assert(CreateThread(NULL, 0, ddeThread, (LPVOID)&info, 0, &threadId));
        while (GetMessage(&msg, NULL, 0, 0)) DispatchMessage(&msg);
        rc = msg.wParam > 32 ? 33 : msg.wParam;
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
        if (rc == 33)
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
                ok(StrCmpPath(params, ddeExec) == 0,
                   "ddeexec expected '%s', got '%s'\n", params, ddeExec);
            }
            else todo_wine
            {
                sprintf(params, test->expectedDdeExec, filename);
                ok(StrCmpPath(params, ddeExec) == 0,
                   "ddeexec expected '%s', got '%s'\n", params, ddeExec);
            }
        }

        assert(DdeNameService(ddeInst, hszApplication, 0L, DNS_UNREGISTER));
        assert(DdeFreeStringHandle(ddeInst, hszTopic));
        assert(DdeFreeStringHandle(ddeInst, hszApplication));
        delete_test_association(".sde");
        test++;
    }

    assert(DdeUninitialize(ddeInst));
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
        create_test_association(".sde");
        sprintf(params, test->command, tmpdir);
        create_test_verb_dde(".sde", "Open", 1, params, "[test]", NULL,
                             "shlexec", NULL);
        denyNextConnection = FALSE;
        ddeApplication[0] = 0;

        /* No application will be run as we will respond to the first DDE event,
         * so don't wait for it */
        SetEvent(hEvent);

        assert(CreateThread(NULL, 0, ddeThread, (LPVOID)&info, 0, &threadId));
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
    ok(SUCCEEDED(r), "CoInitialize failed (0x%08x)\n", r);
    if (!SUCCEEDED(r))
        exit(1);

    rc=GetModuleFileName(NULL, argv0, sizeof(argv0));
    assert(rc!=0 && rc<sizeof(argv0));
    if (GetFileAttributes(argv0)==INVALID_FILE_ATTRIBUTES)
    {
        strcat(argv0, ".so");
        ok(GetFileAttributes(argv0)!=INVALID_FILE_ATTRIBUTES,
           "unable to find argv0!\n");
    }

    GetTempPathA(sizeof(tmpdir)/sizeof(*tmpdir), tmpdir);
    assert(GetTempFileNameA(tmpdir, "wt", 0, child_file)!=0);
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
    create_test_association(".shlexec");
    create_test_verb(".shlexec", "Open", 0, "Open \"%1\"");
    create_test_verb(".shlexec", "NoQuotes", 0, "NoQuotes %1");
    create_test_verb(".shlexec", "LowerL", 0, "LowerL %l");
    create_test_verb(".shlexec", "QuotedLowerL", 0, "QuotedLowerL \"%l\"");
    create_test_verb(".shlexec", "UpperL", 0, "UpperL %L");
    create_test_verb(".shlexec", "QuotedUpperL", 0, "QuotedUpperL \"%L\"");
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
        DeleteFile(filename);
        testfile++;
    }
    DeleteFile(child_file);

    /* Delete the test association */
    delete_test_association(".shlexec");

    CloseHandle(hEvent);

    CoUninitialize();
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

    test_filename();
    test_find_executable();
    test_lnks();
    test_exes();
    test_exes_long();
    test_dde();
    test_dde_default_app();

    cleanup_test();
}
