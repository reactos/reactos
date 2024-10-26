/*
 * Unit test suite for file functions
 *
 * Copyright 2024 Eric Pouech for CodeWeavers
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

#include <errno.h>
#include <direct.h>
#include <stdarg.h>
#include <locale.h>
#include <process.h>
#include <share.h>
#include <sys/stat.h>

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include "wine/test.h"

static void test_std_stream_buffering(void)
{
    int dup_fd, ret, pos;
    FILE *file;
    char ch;

    dup_fd = _dup(STDOUT_FILENO);
    ok(dup_fd != -1, "_dup failed\n");

    file = freopen("std_stream_test.tmp", "w", stdout);
    ok(file != NULL, "freopen failed\n");

    ret = fprintf(stdout, "test");
    pos = _telli64(STDOUT_FILENO);

    fflush(stdout);
    _dup2(dup_fd, STDOUT_FILENO);
    close(dup_fd);
    setvbuf(stdout, NULL, _IONBF, 0);

    ok(ret == 4, "fprintf(stdout) returned %d\n", ret);
    ok(!pos, "expected stdout to be buffered\n");

    dup_fd = _dup(STDERR_FILENO);
    ok(dup_fd != -1, "_dup failed\n");

    file = freopen("std_stream_test.tmp", "w", stderr);
    ok(file != NULL, "freopen failed\n");

    ret = fprintf(stderr, "test");
    ok(ret == 4, "fprintf(stderr) returned %d\n", ret);
    pos = _telli64(STDERR_FILENO);
    if (broken(!GetProcAddress(GetModuleHandleA("ucrtbase"), "__CxxFrameHandler4") && !pos))
        trace("stderr is buffered\n");
    else
        ok(pos == 4, "expected stderr to be unbuffered (%d)\n", pos);

    fflush(stderr);
    _dup2(dup_fd, STDERR_FILENO);
    close(dup_fd);

    dup_fd = _dup(STDIN_FILENO);
    ok(dup_fd != -1, "_dup failed\n");

    file = freopen("std_stream_test.tmp", "r", stdin);
    ok(file != NULL, "freopen failed\n");

    ch = 0;
    ret = fscanf(stdin, "%c", &ch);
    ok(ret == 1, "fscanf returned %d\n", ret);
    ok(ch == 't', "ch = 0x%x\n", (unsigned char)ch);
    pos = _telli64(STDIN_FILENO);
    ok(pos == 4, "pos = %d\n", pos);

    fflush(stdin);
    _dup2(dup_fd, STDIN_FILENO);
    close(dup_fd);

    ok(DeleteFileA("std_stream_test.tmp"), "DeleteFile failed\n");
}

int CDECL _get_stream_buffer_pointers(FILE*,char***,char***,int**);
static void test_iobuf_layout(void)
{
    union
    {
        FILE *f;
        struct
        {
            char* _ptr;
            char* _base;
            int   _cnt;
            int   _flag;
            int   _file;
            int   _charbuf;
            int   _bufsiz;
            char* _tmpfname;
            CRITICAL_SECTION _crit;
        } *iobuf;
    } fp;
    char *tempf, *ptr, **file_ptr, **file_base;
    int cnt, r, *file_cnt;

    tempf = _tempnam(".","wne");
    fp.f = fopen(tempf, "wb");
    ok(fp.f != NULL, "fopen failed with error: %d\n", errno);

    ok(!(fp.iobuf->_flag & 0x440), "fp.iobuf->_flag = %x\n", fp.iobuf->_flag);
    r = fprintf(fp.f, "%s", "init");
    ok(r == 4, "fprintf returned %d\n", r);
    ok(fp.iobuf->_flag & 0x40, "fp.iobuf->_flag = %x\n", fp.iobuf->_flag);
    ok(fp.iobuf->_cnt + 4 == fp.iobuf->_bufsiz, "_cnt = %d, _bufsiz = %d\n",
            fp.iobuf->_cnt, fp.iobuf->_bufsiz);

    ptr = fp.iobuf->_ptr;
    cnt = fp.iobuf->_cnt;
    r = fprintf(fp.f, "%s", "hello");
    ok(r == 5, "fprintf returned %d\n", r);
    ok(ptr + 5 == fp.iobuf->_ptr, "fp.iobuf->_ptr = %p, expected %p\n", fp.iobuf->_ptr, ptr + 5);
    ok(cnt - 5 == fp.iobuf->_cnt, "fp.iobuf->_cnt = %d, expected %d\n", fp.iobuf->_cnt, cnt - 5);
    ok(fp.iobuf->_ptr + fp.iobuf->_cnt == fp.iobuf->_base + fp.iobuf->_bufsiz,
            "_ptr = %p, _cnt = %d, _base = %p, _bufsiz  = %d\n",
            fp.iobuf->_ptr, fp.iobuf->_cnt, fp.iobuf->_base, fp.iobuf->_bufsiz);

    _get_stream_buffer_pointers(fp.f, &file_base, &file_ptr, &file_cnt);
    ok(file_base == &fp.iobuf->_base, "_base = %p, expected %p\n", file_base, &fp.iobuf->_base);
    ok(file_ptr == &fp.iobuf->_ptr, "_ptr = %p, expected %p\n", file_ptr, &fp.iobuf->_ptr);
    ok(file_cnt == &fp.iobuf->_cnt, "_cnt = %p, expected %p\n", file_cnt, &fp.iobuf->_cnt);

    r = setvbuf(fp.f, NULL, _IONBF, 0);
    ok(!r, "setvbuf returned %d\n", r);
    ok(fp.iobuf->_flag & 0x400, "fp.iobuf->_flag = %x\n", fp.iobuf->_flag);

    ok(TryEnterCriticalSection(&fp.iobuf->_crit), "TryEnterCriticalSection section returned FALSE\n");
    LeaveCriticalSection(&fp.iobuf->_crit);

    fclose(fp.f);
    unlink(tempf);
}

static void test_std_stream_open(void)
{
    FILE *f;
    int fd;

    fd = _dup(STDIN_FILENO);
    ok(fd != -1, "_dup failed\n");

    ok(!fclose(stdin), "fclose failed\n");
    f = fopen("nul", "r");
    ok(f != stdin, "f = %p, stdin =  %p\n", f, stdin);
    ok(_fileno(f) == STDIN_FILENO, "_fileno(f) = %d\n", _fileno(f));
    ok(!fclose(f), "fclose failed\n");

    f = freopen("nul", "r", stdin);
    ok(f == stdin, "f = %p, expected %p\n", f, stdin);
    ok(_fileno(f) == STDIN_FILENO, "_fileno(f) = %d\n", _fileno(f));

    _dup2(fd, STDIN_FILENO);
    close(fd);
}

static void test_fopen(void)
{
    int i;
    FILE *f;
    wchar_t wpath[MAX_PATH];
    static const struct {
        const char *loc;
        const char *path;
    } tests[] = {
        { "German.utf8",    "t\xc3\xa4\xc3\x8f\xc3\xb6\xc3\x9f.txt" },
        { "Polish.utf8",    "t\xc4\x99\xc5\x9b\xc4\x87.txt" },
        { "Turkish.utf8",   "t\xc3\x87\xc4\x9e\xc4\xb1\xc4\xb0\xc5\x9e.txt" },
        { "Arabic.utf8",    "t\xd8\xaa\xda\x86.txt" },
        { "Japanese.utf8",  "t\xe3\x82\xaf\xe3\x83\xa4.txt" },
        { "Chinese.utf8",   "t\xe4\xb8\x82\xe9\xbd\xab.txt" },
        { "Japanese",       "t\xb8\xd5.txt" },

    };

    for(i=0; i<ARRAY_SIZE(tests); i++) {
        if(!setlocale(LC_ALL, tests[i].loc)) {
            win_skip("skipping locale %s\n", tests[i].loc);
            continue;
        }

        if(!MultiByteToWideChar(___lc_codepage_func() == CP_UTF8 ? CP_UTF8 : CP_ACP,
                    MB_PRECOMPOSED | MB_ERR_INVALID_CHARS, tests[i].path, -1, wpath, MAX_PATH))
            continue;

        f = _fsopen(tests[i].path, "w", SH_DENYNO);
        ok(!!f, "failed to create %s with locale %s\n",
                debugstr_a(tests[i].path), tests[i].loc);
        fclose(f);

        f = _wfsopen(wpath, L"r", SH_DENYNO);
        ok(!!f, "failed to open %s with locale %s\n",
                debugstr_a(tests[i].path), tests[i].loc);
        if(f) fclose(f);

        ok(!unlink(tests[i].path), "failed to unlink %s with locale %s\n",
                tests[i].path, tests[i].loc);
    }
    setlocale(LC_ALL, "C");
}

static void test_utf8(const char *argv0)
{
    const char file[] = "file\xc4\x99\xc5\x9b\xc4\x87.a";
    const char dir[] = "dir\xc4\x99\xc5\x9b\xc4\x87";
    const WCHAR fileW[] = L"file\x0119\x015b\x0107.a";
    const WCHAR dirW[] = L"dir\x0119\x015b\x0107";

    char file2[32], buf[256], *p, *q, *env[2];
    struct _finddata64i32_t fdata64i32;
    struct _finddata32_t fdata32;
    struct _finddata64_t fdata64;
    intptr_t hfind, hproc;
    WCHAR bufW[256], *pW;
    struct _stat64 stat;
    FILE *f;
    int ret;

    if (!setlocale(LC_ALL, ".utf8"))
    {
        win_skip("utf-8 tests\n");
        return;
    }

    ret = _mkdir(dir);
    if (ret == -1 && errno == ENOENT)
    {
        skip("can't create test environment\n");
        return;
    }
    ok(!ret, "_mkdir returned %d, error %d\n", ret, errno);

    ret = _chdir(dir);
    ok(!ret, "_chdir returned %d, error %d\n", ret, errno);

    p = _getcwd(buf, sizeof(buf));
    ok(p == buf, "_getcwd returned %p, errno %d\n", p, errno);
    p = strrchr(p, '\\');
    ok(!!p, "strrchr returned NULL, buf = %s\n", debugstr_a(buf));
    ok(!strcmp(p + 1, dir), "unexpected working directory: %s\n", debugstr_a(buf));

    p = _getdcwd(_getdrive(), buf, sizeof(buf));
    ok(p == buf, "_getdcwd returned %p, errno %d\n", p, errno);
    p = strrchr(p, '\\');
    ok(!!p, "strrchr returned NULL, buf = %s\n", debugstr_a(buf));
    ok(!strcmp(p + 1, dir), "unexpected working directory: %s\n", debugstr_a(buf));

    p = _fullpath(buf, NULL, sizeof(buf));
    ok(p == buf, "_fulpath returned %p, errno %d\n", p, errno);
    p = strrchr(p, '\\');
    ok(!!p, "strrchr returned NULL, buf = %s\n", debugstr_a(buf));
    ok(!strcmp(p + 1, dir), "unexpected working directory: %s\n", debugstr_a(buf));

    f = fopen(file, "w");
    ok(!!f, "fopen returned %d, error %d\n", ret, errno);
    fclose(f);

    ret = access(file, 0);
    ok(!ret, "access returned %d, error %d\n", ret, errno);

    ret = _stat64(file, &stat);
    ok(!ret, "_stat64 returned %d, error %d\n", ret, errno);

    ret = _chmod(file, _S_IREAD | _S_IWRITE);
    ok(!ret, "_chmod returned %d, error %d\n", ret, errno);

    strcpy(file2, file);
    strcat(file2, "XXXXXX");
    p = _mktemp(file2);
    ok(p == file2, "_mktemp returned %p, file2 %p, errno %d\n", p, file2, errno);
    ok(!memcmp(file2, file, sizeof(file) - 1), "file2 = %s\n", debugstr_a(file2));
    ok(p[ARRAY_SIZE(file) - 1] == 'a', "p = %s\n", debugstr_a(p));
    f = fopen(p, "w");
    ok(!!f, "fopen returned %d, error %d\n", ret, errno);
    fclose(f);

    strcpy(buf, file);
    strcat(buf, "XXXXXX");
    p = _mktemp(buf);
    ok(p == buf, "_mktemp returned %p, buf %p, errno %d\n", p, buf, errno);
    ok(!memcmp(buf, file, sizeof(file) - 1), "buf = %s\n", debugstr_a(buf));
    ok(p[ARRAY_SIZE(file) - 1] == 'b', "p = %s\n", debugstr_a(p));

    strcpy(buf, file);
    strcat(buf, "XXXXXX");
    ret = _mktemp_s(buf, sizeof(buf));
    ok(!memcmp(buf, file, sizeof(file) - 1), "buf = %s\n", debugstr_a(buf));
    ok(buf[ARRAY_SIZE(file) - 1] == 'b', "buf = %s\n", debugstr_a(buf));

    strcpy(buf, file);
    strcat(buf, "*");
    fdata32.name[0] = 'x';
    hfind = _findfirst32(buf, &fdata32);
    ok(hfind != -1, "_findfirst32 returned %Id, errno %d\n", hfind, errno);
    ok(!memcmp(file, fdata32.name, sizeof(file) - 1), "fdata32.name = %s\n", debugstr_a(fdata32.name));

    fdata32.name[0] = 'x';
    ret = _findnext32(hfind, &fdata32);
    ok(!ret, "_findnext32 returned %d, errno %d\n", ret, errno);
    ok(!memcmp(file, fdata32.name, sizeof(file) - 1), "fdata32.name = %s\n", debugstr_a(fdata32.name));
    ret = _findclose(hfind);
    ok(!ret, "_findclose returned %d, errno %d\n", ret, errno);


    strcpy(buf, file);
    strcat(buf, "*");
    fdata64.name[0] = 'x';
    hfind = _findfirst64(buf, &fdata64);
    ok(hfind != -1, "_findfirst64 returned %Id, errno %d\n", hfind, errno);
    ok(!memcmp(file, fdata64.name, sizeof(file) - 1), "fdata64.name = %s\n", debugstr_a(fdata64.name));

    fdata64.name[0] = 'x';
    ret = _findnext64(hfind, &fdata64);
    ok(!ret, "_findnext64 returned %d, errno %d\n", ret, errno);
    ok(!memcmp(file, fdata64.name, sizeof(file) - 1), "fdata64.name = %s\n", debugstr_a(fdata64.name));
    ret = _findclose(hfind);
    ok(!ret, "_findclose returned %d, errno %d\n", ret, errno);

    strcpy(buf, file);
    strcat(buf, "*");
    fdata64i32.name[0] = 'x';
    hfind = _findfirst64i32(buf, &fdata64i32);
    ok(hfind != -1, "_findfirst64i32 returned %Id, errno %d\n", hfind, errno);
    ok(!memcmp(file, fdata64i32.name, sizeof(file) - 1), "fdata64i32.name = %s\n", debugstr_a(fdata64i32.name));

    fdata64i32.name[0] = 'x';
    ret = _findnext64i32(hfind, &fdata64i32);
    ok(!ret, "_findnext64i32 returned %d, errno %d\n", ret, errno);
    ok(!memcmp(file, fdata64i32.name, sizeof(file) - 1), "fdata64i32.name = %s\n", debugstr_a(fdata64i32.name));
    ret = _findclose(hfind);
    ok(!ret, "_findclose returned %d, errno %d\n", ret, errno);

    ret = remove(file2);
    ok(!ret, "remove returned %d, errno %d\n", ret, errno);

    buf[0] = 'x';
    _searchenv(file, "env", buf);
    p = strrchr(buf, '\\');
    ok(!!p, "buf = %s\n", debugstr_a(buf));
    ok(!strcmp(p + 1, file), "buf = %s\n", debugstr_a(buf));

    ret = _wunlink(fileW);
    ok(!ret, "_wunlink returned %d, errno %d\n", ret, errno);

    ret = _chdir("..");
    ok(!ret, "_chdir returned %d, error %d\n", ret, errno);

    ret = _wrmdir(dirW);
    ok(!ret, "_wrmdir returned %d, errno %d\n", ret, errno);

    p = _tempnam(NULL, file);
    ok(!!p, "_tempnam returned NULL, error %d\n", errno);
    q = strrchr(p, '\\');
    ok(!!q, "_tempnam returned %s\n", debugstr_a(p));
    todo_wine ok(!memcmp(q + 1, file, ARRAY_SIZE(file) - 1),
            "incorrect file prefix: %s\n", debugstr_a(p));
    free(p);

    /* native implementation mixes CP_UTF8 and CP_ACP */
    if (GetACP() != CP_UTF8)
    {
        /* make sure wide environment is initialized (works around bug in native) */
        ret = _putenv("__wine_env_test=test");
        ok(!ret, "_putenv returned %d, errno %d\n", ret, errno);
        _wgetenv(L"__wine_env_test");

        strcpy(buf, file);
        strcat(buf, "=test");
        ret = _putenv(buf);
        ok(!ret, "_putenv returned %d, errno %d\n", ret, errno);
        /* bug in native _wgetenv/_putenv implementation */
        pW = _wgetenv(fileW);
        ok(!pW, "environment variable name was converted\n");
        bufW[0] = 0;
        ret = GetEnvironmentVariableW(fileW, bufW, ARRAY_SIZE(bufW));
        todo_wine ok(ret, "GetEnvironmentVariableW returned error %lu\n", GetLastError());
        todo_wine ok(!wcscmp(bufW, L"test"), "bufW = %s\n", debugstr_w(bufW));
        strcpy(buf, file);
        strcat(buf, "=");
        ret = _putenv(buf);
        ok(!ret, "_putenv returned %d, errno %d\n", ret, errno);

        strcpy(buf, "__wine_env_test=");
        strcat(buf, file);
        ret = _putenv(buf);
        ok(!ret, "_putenv returned %d, errno %d\n", ret, errno);
        /* bug in native _wgetenv/_putenv implementation */
        pW = _wgetenv(L"__wine_env_test");
        ok(wcscmp(pW, fileW), "pW = %s\n", debugstr_w(pW));
        ret = GetEnvironmentVariableW(L"__wine_env_test", bufW, ARRAY_SIZE(bufW));
        ok(ret, "GetEnvironmentVariableW returned error %lu\n", GetLastError());
        todo_wine ok(!wcscmp(bufW, fileW), "bufW = %s\n", debugstr_w(bufW));

        wcscpy(bufW, L"__wine_env_test=");
        wcscat(bufW, fileW);
        ret = _wputenv(bufW);
        ok(!ret, "_wputenv returned %d, errno %d\n", ret, errno);
        p = getenv("__wine_env_test");
        ok(strcmp(p, file), "environment variable was converted\n");
        strcpy(buf, "__wine_env_test=");
        ret = _putenv(buf);
        ok(!ret, "_putenv returned %d, errno %d\n", ret, errno);
    }

    strcpy(buf, "__wine_env_test=");
    strcat(buf, file);
    env[0] = buf;
    env[1] = NULL;
    hproc = _spawnle(_P_NOWAIT, argv0, argv0, "file", "utf8", file, NULL, env);
    ok(hproc != -1, "_spawnl returned %Id, errno %d\n", hproc, errno);
    wait_child_process((HANDLE)hproc);
    CloseHandle((HANDLE)hproc);

    setlocale(LC_ALL, "C");
}

static void test_utf8_argument(void)
{
    static const WCHAR nameW[] = L"file\x0119\x015b\x0107.a";
    const WCHAR *cmdline = GetCommandLineW(), *p;
    WCHAR buf[256];
    DWORD ret;

    p = wcsrchr(cmdline, ' ');
    ok(!!p, "cmdline = %s\n", debugstr_w(cmdline));
    ok(!wcscmp(p + 1, nameW), "cmdline = %s\n", debugstr_w(cmdline));

    ret = GetEnvironmentVariableW(L"__wine_env_test", buf, ARRAY_SIZE(buf));
    ok(ret, "GetEnvironmentVariableW returned error %lu\n", GetLastError());
    if (GetACP() == CP_UTF8)
        ok(!wcscmp(buf, nameW), "__wine_env_test = %s\n", debugstr_w(buf));
    else
        ok(wcscmp(buf, nameW), "environment was converted\n");
}

START_TEST(file)
{
    int arg_c;
    char** arg_v;

    arg_c = winetest_get_mainargs(&arg_v);
    if(arg_c == 4 && !strcmp(arg_v[2], "utf8"))
    {
        test_utf8_argument();
        return;
    }

    test_std_stream_buffering();
    test_iobuf_layout();
    test_std_stream_open();
    test_fopen();
    test_utf8(arg_v[0]);
}
