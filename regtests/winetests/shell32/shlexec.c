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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static const char* testfiles[]=
{
    "%s\\test file.shlexec",
    "%s\\test file.noassoc",
    "%s\\test file.noassoc.shlexec",
    "%s\\test file.shlexec.noassoc",
    "%s\\test_shortcut_shlexec.lnk",
    NULL
};


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

    SetLastError(0xcafebabe);
    /* FIXME: We cannot use ShellExecuteEx() here because if there is no
     * association it displays the 'Open With' dialog and I could not find
     * a flag to prevent this.
     */
    return (int)ShellExecute(NULL, operation, file, parameters, directory,
                             SW_SHOWNORMAL);
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
    sei.fMask=mask;
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
    sei.hIcon=NULL;

    SetLastError(0xcafebabe);
    success=ShellExecuteEx(&sei);
    rc=(int)sei.hInstApp;
    ok((success && rc >= 32) || (!success && rc < 32),
       "%s rc=%d and hInstApp=%d is not allowed\n", shell_call, success, rc);
    return rc;
}

static void create_test_association(const char* extension)
{
    HKEY hkey, hkey_shell;
    char class[MAX_PATH];
    LONG rc;

    sprintf(class, "shlexec%s", extension);
    rc=RegCreateKeyEx(HKEY_CLASSES_ROOT, extension, 0, NULL, 0, KEY_SET_VALUE,
                      NULL, &hkey, NULL);
    assert(rc==ERROR_SUCCESS);
    rc=RegSetValueEx(hkey, NULL, 0, REG_SZ, class, strlen(class)+1);
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

static void create_test_verb(const char* extension, const char* verb)
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

    cmd=malloc(strlen(argv0)+13+1);
    sprintf(cmd,"%s shlexec \"%%1\"", argv0);
    rc=RegSetValueEx(hkey_cmd, NULL, 0, REG_SZ, cmd, strlen(cmd)+1);
    assert(rc==ERROR_SUCCESS);

    free(cmd);
    CloseHandle(hkey_shell);
    CloseHandle(hkey_verb);
    CloseHandle(hkey_cmd);
}


typedef struct
{
    char* basename;
    int rc;
    int todo;
} filename_tests_t;

static filename_tests_t filename_tests[]=
{
    /* Test bad / nonexistent filenames */
    {"%s\\nonexistent.shlexec", ERROR_FILE_NOT_FOUND, 1},
    {"%s\\nonexistent.noassoc", ERROR_FILE_NOT_FOUND, 1},

    /* Standard tests */
    {"%s\\test file.shlexec",   0, 0},
    {"%s\\test file.shlexec.",  0, 0},
    {"%s/test file.shlexec",    0, 0},

    /* Test filenames with no association */
    {"%s\\test file.noassoc",   SE_ERR_NOASSOC, 0},

    /* Test double extensions */
    {"%s\\test file.noassoc.shlexec", 0, 0},
    {"%s\\test file.shlexec.noassoc", SE_ERR_NOASSOC, 0},

    /* Test shortcuts */
    {"%s\\test_shortcut_shlexec.lnk", 0, 0},

    {NULL, 0, 0}
};

static void test_filename()
{
    char filename[MAX_PATH];
    const filename_tests_t* test;
    HMODULE hdll;
    DLLVERSIONINFO dllver;
    HRESULT (WINAPI *pDllGetVersion)(DLLVERSIONINFO*);
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
        rc=shell_execute(NULL, filename, NULL, NULL);
        if (test->rc==0)
        {
            if (test->todo)
            {
                todo_wine
                {
                    ok(rc>=32, "%s failed: rc=%d err=%ld\n", shell_call,
                       rc, GetLastError());
                }
            }
            else
            {
                ok(rc>=32, "%s failed: rc=%d err=%ld\n", shell_call,
                   rc, GetLastError());
            }
        }
        else
        {
            if (test->todo)
            {
                todo_wine
                {
                    ok(rc==test->rc, "%s returned %d\n", shell_call, rc);
                }
            }
            else
            {
                ok(rc==test->rc, "%s returned %d\n", shell_call, rc);
            }
        }
        test++;
    }

    hdll=GetModuleHandleA("shell32.dll");
    pDllGetVersion=(void*)GetProcAddress(hdll, "DllGetVersion");
    if (pDllGetVersion)
    {
        dllver.cbSize=sizeof(dllver);
        pDllGetVersion(&dllver);
        trace("major=%ld minor=%ld build=%ld platform=%ld\n",
              dllver.dwMajorVersion, dllver.dwMinorVersion,
              dllver.dwBuildNumber, dllver.dwPlatformID);

        /* The more recent versions of shell32.dll accept quoted filenames
         * while older ones (e.g. 4.00) don't. Still we want to test this
         * because IE 6 depends on the new behavior.
         * One day we may need to check the exact version of the dll but for
         * now making sure DllGetVersion() is present is sufficient.
         */
        sprintf(filename, "\"%s\\test file.shlexec\"", tmpdir);
        rc=shell_execute(NULL, filename, NULL, NULL);
        ok(rc>=32, "%s failed: rc=%d err=%ld\n", shell_call, rc,
           GetLastError());

        if (dllver.dwMajorVersion>=6)
        {
            /* Recent versions of shell32.dll accept '/'s in shortcut paths.
             * Older versions don't or are quite buggy in this regard.
             */
            sprintf(filename, "%s\\test_shortcut_shlexec.lnk", tmpdir);
            c=filename;
            while (*c)
            {
                if (*c=='\\')
                    *c='/';
                c++;
            }
            rc=shell_execute(NULL, filename, NULL, NULL);
            todo_wine {
            ok(rc>=32, "%s failed: rc=%d err=%ld\n", shell_call, rc,
               GetLastError());
            }
        }
    }
}


static void test_exes()
{
    char filename[MAX_PATH];
    int rc;

    /* We need NOZONECHECKS on Win2003 to block a dialog */
    rc=shell_execute_ex(SEE_MASK_NOZONECHECKS, NULL, argv0, "shlexec -nop",
                        NULL);
    ok(rc>=32, "%s returned %d\n", shell_call, rc);

    sprintf(filename, "%s\\test file.noassoc", tmpdir);
    if (CopyFile(argv0, filename, FALSE))
    {
        rc=shell_execute(NULL, filename, "shlexec -nop", NULL);
        todo_wine {
        ok(rc==SE_ERR_NOASSOC, "%s succeeded: rc=%d\n", shell_call, rc);
        }
    }
}


static void init_test()
{
    char filename[MAX_PATH];
    WCHAR lnkfile[MAX_PATH];
    const char* const * testfile;
    lnk_desc_t desc;
    DWORD rc;
    HRESULT r;

    r = CoInitialize(NULL);
    ok(SUCCEEDED(r), "CoInitialize failed (0x%08lx)\n", r);
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
            trace("unable to create '%s': err=%ld\n", filename, GetLastError());
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
    desc.arguments="";
    desc.showcmd=0;
    desc.icon=NULL;
    desc.icon_id=0;
    desc.hotkey=0;
    create_lnk(lnkfile, &desc, 0);

    /* Create a basic association suitable for most tests */
    create_test_association(".shlexec");
    create_test_verb(".shlexec", "Open");
}

static void cleanup_test()
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

    /* Delete the test association */
    delete_test_association(".shlexec");

    CoUninitialize();
}

START_TEST(shlexec)
{

    myARGC = winetest_get_mainargs(&myARGV);
    if (myARGC>=3)
    {
        /* FIXME: We should dump the parameters we got
         *        and have the parent verify them
         */
        exit(0);
    }

    init_test();

    test_filename();
    test_exes();

    cleanup_test();
}
