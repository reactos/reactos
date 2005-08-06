/*
 * Wine Conformance Test EXE
 *
 * Copyright 2003, 2004 Jakob Eriksson   (for Solid Form Sweden AB)
 * Copyright 2003 Dimitrie O. Paun
 * Copyright 2003 Ferenc Wagner
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
 *
 * This program is dedicated to Anna Lindh,
 * Swedish Minister of Foreign Affairs.
 * Anna was murdered September 11, 2003.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <windows.h>

#include "winetest.h"
#include "resource.h"

struct wine_test
{
    char *name;
    int resource;
    int subtest_count;
    char **subtests;
    char *exename;
};

struct rev_info
{
    const char* file;
    const char* rev;
};

char *tag = NULL;
static struct wine_test *wine_tests;
static struct rev_info *rev_infos = NULL;
static const char whitespace[] = " \t\r\n";

static int running_under_wine (void)
{
    HMODULE module = GetModuleHandleA("ntdll.dll");

    if (!module) return 0;
    return (GetProcAddress(module, "wine_server_call") != NULL);
}

static int running_on_visible_desktop (void)
{
    HWND desktop;
    HMODULE huser32 = GetModuleHandle("user32.dll");
    FARPROC pGetProcessWindowStation = GetProcAddress(huser32, "GetProcessWindowStation");
    FARPROC pGetUserObjectInformationA = GetProcAddress(huser32, "GetUserObjectInformationA");

    desktop = GetDesktopWindow();
    if (!GetWindowLongPtrW(desktop, GWLP_WNDPROC)) /* Win9x */
        return IsWindowVisible(desktop);

    if (pGetProcessWindowStation && pGetUserObjectInformationA)
    {
        DWORD len;
        HWINSTA wstation;
        USEROBJECTFLAGS uoflags;

        wstation = (HWINSTA)pGetProcessWindowStation();
        assert(pGetUserObjectInformationA(wstation, UOI_FLAGS, &uoflags, sizeof(uoflags), &len));
        return (uoflags.dwFlags & WSF_VISIBLE) != 0;
    }
    return IsWindowVisible(desktop);
}

static void print_version (void)
{
    OSVERSIONINFOEX ver;
    BOOL ext;

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!(ext = GetVersionEx ((OSVERSIONINFO *) &ver)))
    {
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx ((OSVERSIONINFO *) &ver))
	    report (R_FATAL, "Can't get OS version.");
    }

    xprintf ("    bRunningUnderWine=%d\n", running_under_wine ());
    xprintf ("    bRunningOnVisibleDesktop=%d\n", running_on_visible_desktop ());
    xprintf ("    dwMajorVersion=%ld\n    dwMinorVersion=%ld\n"
             "    dwBuildNumber=%ld\n    PlatformId=%ld\n    szCSDVersion=%s\n",
             ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber,
             ver.dwPlatformId, ver.szCSDVersion);

    if (!ext) return;

    xprintf ("    wServicePackMajor=%d\n    wServicePackMinor=%d\n"
             "    wSuiteMask=%d\n    wProductType=%d\n    wReserved=%d\n",
             ver.wServicePackMajor, ver.wServicePackMinor, ver.wSuiteMask,
             ver.wProductType, ver.wReserved);
}

static inline int is_dot_dir(const char* x)
{
    return ((x[0] == '.') && ((x[1] == 0) || ((x[1] == '.') && (x[2] == 0))));
}

static void remove_dir (const char *dir)
{
    HANDLE  hFind;
    WIN32_FIND_DATA wfd;
    char path[MAX_PATH];
    size_t dirlen = strlen (dir);

    /* Make sure the directory exists before going further */
    memcpy (path, dir, dirlen);
    strcpy (path + dirlen++, "\\*");
    hFind = FindFirstFile (path, &wfd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        char *lp = wfd.cFileName;

        if (!lp[0]) lp = wfd.cAlternateFileName; /* ? FIXME not (!lp) ? */
        if (is_dot_dir (lp)) continue;
        strcpy (path + dirlen, lp);
        if (FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes)
            remove_dir(path);
        else if (!DeleteFile (path))
            report (R_WARNING, "Can't delete file %s: error %d",
                    path, GetLastError ());
    } while (FindNextFile (hFind, &wfd));
    FindClose (hFind);
    if (!RemoveDirectory (dir))
        report (R_WARNING, "Can't remove directory %s: error %d",
                dir, GetLastError ());
}

static const char* get_test_source_file(const char* test, const char* subtest)
{
    static const char* special_dirs[][2] = {
	{ "gdi32", "gdi"}, { "kernel32", "kernel" },
        { "msacm32", "msacm" },
	{ "user32", "user" }, { "winspool.drv", "winspool" },
	{ "ws2_32", "winsock" }, { 0, 0 }
    };
    static char buffer[MAX_PATH];
    int i;

    for (i = 0; special_dirs[i][0]; i++) {
	if (strcmp(test, special_dirs[i][0]) == 0) {
	    test = special_dirs[i][1];
	    break;
	}
    }

    snprintf(buffer, sizeof(buffer), "dlls/%s/tests/%s.c", test, subtest);
    return buffer;
}

static const char* get_file_rev(const char* file)
{
    const struct rev_info* rev;
 
    for(rev = rev_infos; rev->file; rev++) {
	if (strcmp(rev->file, file) == 0) return rev->rev;
    }

    return "-";
}

static void extract_rev_infos (void)
{
    char revinfo[256], *p;
    int size = 0, i;
    unsigned int len;
    HMODULE module = GetModuleHandle (NULL);

    for (i = 0; TRUE; i++) {
	if (i >= size) {
	    size += 100;
	    rev_infos = xrealloc (rev_infos, size * sizeof (*rev_infos));
	}
	memset(rev_infos + i, 0, sizeof(rev_infos[i]));

        len = LoadStringA (module, REV_INFO+i, revinfo, sizeof(revinfo));
        if (len == 0) break; /* end of revision info */
	if (len >= sizeof(revinfo) - 1) 
	    report (R_FATAL, "Revision info too long.");
	if(!(p = strrchr(revinfo, ':')))
	    report (R_FATAL, "Revision info malformed (i=%d)", i);
	*p = 0;
	rev_infos[i].file = strdup(revinfo);
	rev_infos[i].rev = strdup(p + 1);
    }
}

static void* extract_rcdata (int id, int type, DWORD* size)
{
    HRSRC rsrc;
    HGLOBAL hdl;
    LPVOID addr;
    
    if (!(rsrc = FindResource (NULL, (LPTSTR)id, MAKEINTRESOURCE(type))) ||
        !(*size = SizeofResource (0, rsrc)) ||
        !(hdl = LoadResource (0, rsrc)) ||
        !(addr = LockResource (hdl)))
        return NULL;
    return addr;
}

/* Fills in the name and exename fields */
static void
extract_test (struct wine_test *test, const char *dir, int id)
{
    BYTE* code;
    DWORD size;
    FILE* fout;
    int strlen, bufflen = 128;
    char *exepos;

    code = extract_rcdata (id, TESTRES, &size);
    if (!code) report (R_FATAL, "Can't find test resource %d: %d",
                       id, GetLastError ());
    test->name = xmalloc (bufflen);
    while ((strlen = LoadStringA (NULL, id, test->name, bufflen))
           == bufflen - 1) {
        bufflen *= 2;
        test->name = xrealloc (test->name, bufflen);
    }
    if (!strlen) report (R_FATAL, "Can't read name of test %d.", id);
    test->exename = strmake (NULL, "%s/%s", dir, test->name);
    exepos = strstr (test->name, "_test.exe");
    if (!exepos) report (R_FATAL, "Not an .exe file: %s", test->name);
    *exepos = 0;
    test->name = xrealloc (test->name, exepos - test->name + 1);
    report (R_STEP, "Extracting: %s", test->name);

    if (!(fout = fopen (test->exename, "wb")) ||
        (fwrite (code, size, 1, fout) != 1) ||
        fclose (fout)) report (R_FATAL, "Failed to write file %s.",
                               test->exename);
}

/* Run a command for MS milliseconds.  If OUT != NULL, also redirect
   stdout to there.

   Return the exit status, -2 if can't create process or the return
   value of WaitForSingleObject.
 */
static int
run_ex (char *cmd, const char *out, DWORD ms)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    int fd, oldstdout = -1;
    DWORD wait, status;

    GetStartupInfo (&si);
    si.wShowWindow = SW_HIDE;
    si.dwFlags = STARTF_USESHOWWINDOW;

    if (out) {
        fd = open (out, O_WRONLY | O_CREAT, 0666);
        if (-1 == fd)
            report (R_FATAL, "Can't open '%s': %d", out, errno);
        oldstdout = dup (1);
        if (-1 == oldstdout)
            report (R_FATAL, "Can't save stdout: %d", errno);
        if (-1 == dup2 (fd, 1))
            report (R_FATAL, "Can't redirect stdout: %d", errno);
        close (fd);
    }

    if (!CreateProcessA (NULL, cmd, NULL, NULL, TRUE, 0,
                         NULL, NULL, &si, &pi)) {
        status = -2;
    } else {
        CloseHandle (pi.hThread);
        wait = WaitForSingleObject (pi.hProcess, ms);
        if (wait == WAIT_OBJECT_0) {
            GetExitCodeProcess (pi.hProcess, &status);
        } else {
            switch (wait) {
            case WAIT_FAILED:
                report (R_ERROR, "Wait for '%s' failed: %d", cmd,
                        GetLastError ());
                break;
            case WAIT_TIMEOUT:
                report (R_ERROR, "Process '%s' timed out.", cmd);
                break;
            default:
                report (R_ERROR, "Wait returned %d", wait);
            }
            status = wait;
            if (!TerminateProcess (pi.hProcess, 257))
                report (R_ERROR, "TerminateProcess failed: %d",
                        GetLastError ());
            wait = WaitForSingleObject (pi.hProcess, 5000);
            switch (wait) {
            case WAIT_FAILED:
                report (R_ERROR,
                        "Wait for termination of '%s' failed: %d",
                        cmd, GetLastError ());
                break;
            case WAIT_OBJECT_0:
                break;
            case WAIT_TIMEOUT:
                report (R_ERROR, "Can't kill process '%s'", cmd);
                break;
            default:
                report (R_ERROR, "Waiting for termination: %d",
                        wait);
            }
        }
        CloseHandle (pi.hProcess);
    }

    if (out) {
        close (1);
        if (-1 == dup2 (oldstdout, 1))
            report (R_FATAL, "Can't recover stdout: %d", errno);
        close (oldstdout);
    }
    return status;
}

static void
get_subtests (const char *tempdir, struct wine_test *test, int id)
{
    char *subname, *cmd;
    FILE *subfile;
    size_t total;
    char buffer[8192], *index;
    static const char header[] = "Valid test names:";
    int allocated;

    test->subtest_count = 0;

    subname = tempnam (0, "sub");
    if (!subname) report (R_FATAL, "Can't name subtests file.");

    extract_test (test, tempdir, id);
    cmd = strmake (NULL, "%s --list", test->exename);
    run_ex (cmd, subname, 5000);
    free (cmd);

    subfile = fopen (subname, "r");
    if (!subfile) {
        report (R_ERROR, "Can't open subtests output of %s: %d",
                test->name, errno);
        goto quit;
    }
    total = fread (buffer, 1, sizeof buffer, subfile);
    fclose (subfile);
    if (sizeof buffer == total) {
        report (R_ERROR, "Subtest list of %s too big.",
                test->name, sizeof buffer);
        goto quit;
    }
    buffer[total] = 0;

    index = strstr (buffer, header);
    if (!index) {
        report (R_ERROR, "Can't parse subtests output of %s",
                test->name);
        goto quit;
    }
    index += sizeof header;

    allocated = 10;
    test->subtests = xmalloc (allocated * sizeof(char*));
    index = strtok (index, whitespace);
    while (index) {
        if (test->subtest_count == allocated) {
            allocated *= 2;
            test->subtests = xrealloc (test->subtests,
                                       allocated * sizeof(char*));
        }
        test->subtests[test->subtest_count++] = strdup (index);
        index = strtok (NULL, whitespace);
    }
    test->subtests = xrealloc (test->subtests,
                               test->subtest_count * sizeof(char*));

 quit:
    if (remove (subname))
        report (R_WARNING, "Can't delete file '%s': %d",
                subname, errno);
    free (subname);
}

static void
run_test (struct wine_test* test, const char* subtest)
{
    int status;
    const char* file = get_test_source_file(test->name, subtest);
    const char* rev = get_file_rev(file);
    char *cmd = strmake (NULL, "%s %s", test->exename, subtest);

    xprintf ("%s:%s start %s %s\n", test->name, subtest, file, rev);
    status = run_ex (cmd, NULL, 120000);
    free (cmd);
    xprintf ("%s:%s done (%d)\n", test->name, subtest, status);
}

static BOOL CALLBACK
EnumTestFileProc (HMODULE hModule, LPCTSTR lpszType,
                  LPTSTR lpszName, LONG_PTR lParam)
{
    (*(int*)lParam)++;
    return TRUE;
}

static char *
run_tests (char *logname)
{
    int nr_of_files = 0, nr_of_tests = 0, i;
    char *tempdir, *shorttempdir;
    int logfile;
    char *strres, *eol, *nextline;
    DWORD strsize;

    SetErrorMode (SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    if (!logname) {
        logname = tempnam (0, "res");
        if (!logname) report (R_FATAL, "Can't name logfile.");
    }
    report (R_OUT, logname);

    logfile = open (logname, O_WRONLY | O_CREAT | O_EXCL | O_APPEND,
                    0666);
    if (-1 == logfile) {
        if (EEXIST == errno)
            report (R_FATAL, "File %s already exists.", logname);
        else report (R_FATAL, "Could not open logfile: %d", errno);
    }
    if (-1 == dup2 (logfile, 1))
        report (R_FATAL, "Can't redirect stdout: %d", errno);
    close (logfile);

    tempdir = tempnam (0, "wct");
    if (!tempdir)
        report (R_FATAL, "Can't name temporary dir (check %%TEMP%%).");
    shorttempdir = strdup (tempdir);
    if (shorttempdir) {         /* try stable path for ZoneAlarm */
        strstr (shorttempdir, "wct")[3] = 0;
        if (CreateDirectoryA (shorttempdir, NULL)) {
            free (tempdir);
            tempdir = shorttempdir;
        } else free (shorttempdir);
    }
    if (tempdir != shorttempdir && !CreateDirectoryA (tempdir, NULL))
        report (R_FATAL, "Could not create directory: %s", tempdir);
    report (R_DIR, tempdir);

    xprintf ("Version 3\n");
    strres = extract_rcdata (WINE_BUILD, STRINGRES, &strsize);
    xprintf ("Tests from build ");
    if (strres) xprintf ("%.*s", strsize, strres);
    else xprintf ("-\n");
    strres = extract_rcdata (TESTS_URL, STRINGRES, &strsize);
    xprintf ("Archive: ");
    if (strres) xprintf ("%.*s", strsize, strres);
    else xprintf ("-\n");
    xprintf ("Tag: %s\n", tag);
    xprintf ("Build info:\n");
    strres = extract_rcdata (BUILD_INFO, STRINGRES, &strsize);
    while (strres) {
        eol = memchr (strres, '\n', strsize);
        if (!eol) {
            nextline = NULL;
            eol = strres + strsize;
        } else {
            strsize -= eol - strres + 1;
            nextline = strsize?eol+1:NULL;
            if (eol > strres && *(eol-1) == '\r') eol--;
        }
        xprintf ("    %.*s\n", eol-strres, strres);
        strres = nextline;
    }
    xprintf ("Operating system version:\n");
    print_version ();
    xprintf ("Test output:\n" );

    report (R_STATUS, "Counting tests");
    if (!EnumResourceNames (NULL, MAKEINTRESOURCE(TESTRES),
                            EnumTestFileProc, (LPARAM)&nr_of_files))
        report (R_FATAL, "Can't enumerate test files: %d",
                GetLastError ());
    wine_tests = xmalloc (nr_of_files * sizeof wine_tests[0]);

    report (R_STATUS, "Extracting tests");
    report (R_PROGRESS, 0, nr_of_files);
    for (i = 0; i < nr_of_files; i++) {
        get_subtests (tempdir, wine_tests+i, i);
        nr_of_tests += wine_tests[i].subtest_count;
    }
    report (R_DELTA, 0, "Extracting: Done");

    report (R_STATUS, "Running tests");
    report (R_PROGRESS, 1, nr_of_tests);
    for (i = 0; i < nr_of_files; i++) {
        struct wine_test *test = wine_tests + i;
        int j;

	for (j = 0; j < test->subtest_count; j++) {
            report (R_STEP, "Running: %s:%s", test->name,
                    test->subtests[j]);
	    run_test (test, test->subtests[j]);
        }
    }
    report (R_DELTA, 0, "Running: Done");

    report (R_STATUS, "Cleaning up");
    close (1);
    remove_dir (tempdir);
    free (tempdir);
    free (wine_tests);

    return logname;
}

static void
usage (void)
{
    fprintf (stderr, "\
Usage: winetest [OPTION]...\n\n\
  -c       console mode, no GUI\n\
  -e       preserve the environment\n\
  -h       print this message and exit\n\
  -q       quiet mode, no output at all\n\
  -o FILE  put report into FILE, do not submit\n\
  -s FILE  submit FILE, do not run tests\n\
  -t TAG   include TAG of characters [-.0-9a-zA-Z] in the report\n");
}

int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst,
                    LPSTR cmdLine, int cmdShow)
{
    char *logname = NULL;
    const char *cp, *submit = NULL;
    int reset_env = 1;
    int interactive = 1;

    /* initialize the revision information first */
    extract_rev_infos();

    cmdLine = strtok (cmdLine, whitespace);
    while (cmdLine) {
        if (cmdLine[0] != '-' || cmdLine[2]) {
            report (R_ERROR, "Not a single letter option: %s", cmdLine);
            usage ();
            exit (2);
        }
        switch (cmdLine[1]) {
        case 'c':
            report (R_TEXTMODE);
            interactive = 0;
            break;
        case 'e':
            reset_env = 0;
            break;
        case 'h':
            usage ();
            exit (0);
        case 'q':
            report (R_QUIET);
            interactive = 0;
            break;
        case 's':
            submit = strtok (NULL, whitespace);
            if (tag)
                report (R_WARNING, "ignoring tag for submission");
            send_file (submit);
            break;
        case 'o':
            logname = strtok (NULL, whitespace);
            break;
        case 't':
            tag = strtok (NULL, whitespace);
            if (strlen (tag) > MAXTAGLEN)
                report (R_FATAL, "tag is too long (maximum %d characters)",
                        MAXTAGLEN);
            cp = findbadtagchar (tag);
            if (cp) {
                report (R_ERROR, "invalid char in tag: %c", *cp);
                usage ();
                exit (2);
            }
            break;
        default:
            report (R_ERROR, "invalid option: -%c", cmdLine[1]);
            usage ();
            exit (2);
        }
        cmdLine = strtok (NULL, whitespace);
    }
    if (!submit) {
        report (R_STATUS, "Starting up");

        if (!running_on_visible_desktop ())
            report (R_FATAL, "Tests must be run on a visible desktop");

        if (reset_env && (putenv ("WINETEST_PLATFORM=windows") ||
                          putenv ("WINETEST_DEBUG=1") || 
                          putenv ("WINETEST_INTERACTIVE=0") ||
                          putenv ("WINETEST_REPORT_SUCCESS=0")))
            report (R_FATAL, "Could not reset environment: %d", errno);

        if (!tag) {
            if (!interactive)
                report (R_FATAL, "Please specify a tag (-t option) if "
                        "running noninteractive!");
            if (guiAskTag () == IDABORT) exit (1);
        }
        report (R_TAG);

        if (!logname) {
            logname = run_tests (NULL);
            if (report (R_ASK, MB_YESNO, "Do you want to submit the "
                        "test results?") == IDYES)
                if (!send_file (logname) && remove (logname))
                    report (R_WARNING, "Can't remove logfile: %d.", errno);
            free (logname);
        } else run_tests (logname);
        report (R_STATUS, "Finished");
    }
    exit (0);
}
